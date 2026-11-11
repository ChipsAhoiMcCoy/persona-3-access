// Shadows on a Tartarus floor. What the game holds:
//  - AEnemySymbolBaseCore: the roaming Shadow's actor (hidden while parked or defeated).
//  - AFldEnemyManager.Enemies: one FFldEnemyHolder { Actor, EnemyComp } per Shadow (Actor is that same actor). A
//    UFldEnemyNormalComp has CurrentMarkEffectParam, the mark over its head (its Asset is ExclamationEffect's or
//    QuestionEffect's), and TrackingTarget (an FFldPlayerHolder whose Pawn is whom it is after). Watched on floor 2
//    (2026-11-10): at rest no mark and no target; the exclamation mark came up as each Shadow noticed the hero (at
//    3.4 m and 7.3 m, running at them), and the target was the hero while it chased. Losing him (the question mark,
//    the target let go) is UNTESTED.
//  - UFldPlayerDungeonComp.AttackArea: an AFldCharArea whose box (AreaComp) rides in front of the hero. The strike's
//    hit test is that box's overlapping actors, so "inside the box" is "the strike connects". Measured on floor 5
//    (2026-11-10): centre 84 cm ahead of the hero, extent 90 x 60 x 50, so it reaches 1.7 m ahead and 0.6 m to each
//    side, plus the Shadow's own body. The box is read live, so whatever sizes it is followed. With a
//    long-range strike (IsUsingLongRangeAttack) the target is LockOnActor instead (UNTESTED).
#include "shadow.h"
#include "audio.h"
#include "core/sonar_cues.h"
#include "core/strings.h"
#include "devserver.h"
#include "log.h"
#include "poi.h"
#include "seh.h"
#include "settings.h"
#include "speech.h"
#include "watch.h"
#include "world.h"
#include <windows.h>
#include <cmath>
#include <format>
#include <map>
#include <vector>

namespace p3a::shadow {
namespace {
using namespace ue;
using core::SignalCue;
using world::FVector;

constexpr float kPi = 3.14159265f;
// cm: the growl. 15 m at first, and only for a Shadow within 3 m of the hero's height: it was not heard until it was
// about to strike (the user, 2026-11-10). A Tartarus floor has raised and sunken parts, so height filters nothing now.
constexpr float kPulseRange = 3000.0f;
constexpr float kMindRange = 4000.0f;    // cm: how far off a Shadow's noticing is told
constexpr ULONGLONG kReachMs = 380;      // the ring's rhythm while in reach

// What a Shadow's component shows this frame: the mark over its head, and whether it holds the hero as its target.
enum class Mark { None, Exclamation, Question };
struct Raw { Mark mark = Mark::None; bool tracking = false; };
struct Seen { UObject* actor; FVector at; float dist; Raw raw; };

std::vector<UObject*> live(std::string_view cls) { return watch::all(watch::want(cls, true)); }
float flat(const FVector& a, const FVector& b) { return std::hypot(a.x - b.x, a.y - b.y); }
float yaw_of(UObject* a) {
  Params p(find_function(class_of(a), "K2_GetActorRotation"));
  call(a, p);
  return p.ret<FRotator>().yaw;
}
float angle_diff(float a, float b) { return std::fabs(std::fmod(a - b + 540.0f, 360.0f) - 180.0f); }

// A struct property's first member, an object pointer (TrackingTarget.Pawn, an effect param's Asset).
UObject* first_object(UObject* o, const char* prop) {
  UObject** p = field<UObject*>(o, prop);
  return p ? *p : nullptr;
}
Raw raw_of(UObject* comp) {
  Raw r;
  if (!comp || !is_a(comp, "FldEnemyNormalComp")) return r;   // a floor's guardian has no such state
  UObject* mark = first_object(comp, "CurrentMarkEffectParam");
  if (mark && mark == first_object(comp, "ExclamationEffect")) r.mark = Mark::Exclamation;
  else if (mark && mark == first_object(comp, "QuestionEffect")) r.mark = Mark::Question;
  r.tracking = first_object(comp, "TrackingTarget") != nullptr;
  return r;
}

struct Holder { UObject* actor; UObject* comp; };
std::vector<Holder> holders() {
  static const int id = watch::want("FldEnemyManager");
  std::vector<Holder> out;
  UObject* mgr = watch::get(id);
  auto* arr = mgr ? field<TArray<Holder>>(mgr, "Enemies") : nullptr;
  if (arr && arr->data && arr->num > 0 && arr->num < 256)
    guarded([&] { for (int i = 0; i < arr->num; ++i) if (arr->data[i].actor && arr->data[i].comp) out.push_back(arr->data[i]); });
  return out;
}

// The visible Shadows near the hero's floor, each with what it knows of him.
std::vector<Seen> look(const FVector& me) {
  std::vector<Seen> out;
  for (UObject* a : live("EnemySymbolBaseCore")) {
    if (get_bool(a, "bHidden")) continue;
    FVector v{};
    if (!guarded([&] { v = world::actor_location(a); })) continue;
    out.push_back({a, v, std::hypot(flat(v, me), v.z - me.z), Raw{}});
  }
  if (out.empty()) return out;
  // A holder's Actor is the Shadow's own actor (floor 5's guardian, 2026-11-10).
  for (const Holder& h : holders())
    for (Seen& s : out)
      if (s.actor == h.actor) { guarded([&] { s.raw = raw_of(h.comp); }); break; }
  return out;
}

// ---- the strike's reach ----
struct Box { FVector center, extent; float yaw; };
UObject* dungeon_comp() {
  static const int id = watch::want("FldPlayerDungeonComp");
  return watch::get(id);
}
// A box component as it stands in the world now.
bool read_box(UObject* box, Box& b) {
  if (!box) return false;
  auto vcall = [&](const char* fn) { Params p(find_function(class_of(box), fn)); call(box, p); return p; };
  b.center = vcall("K2_GetComponentLocation").ret<FVector>();
  b.extent = vcall("GetScaledBoxExtent").ret<FVector>();
  b.yaw = vcall("K2_GetComponentRotation").ret<FRotator>().yaw;
  return b.extent.x > 1.0f && b.extent.y > 1.0f;
}
bool strike_box(UObject* dc, Box& b) {
  UObject* area = dc ? get<UObject*>(dc, "AttackArea") : nullptr;
  return read_box(area ? get<UObject*>(area, "AreaComp") : nullptr, b);
}
// Two boxes (each turned about the vertical only) share space: the separating-axis test on the ground plane over
// both boxes' sides, and their heights overlapping.
bool boxes_meet(const Box& a, const Box& b) {
  if (std::fabs(a.center.z - b.center.z) > a.extent.z + b.extent.z) return false;
  const float ya = a.yaw * kPi / 180.0f, yb = b.yaw * kPi / 180.0f;
  const float ax[4][2] = {{std::cos(ya), std::sin(ya)}, {-std::sin(ya), std::cos(ya)}, {std::cos(yb), std::sin(yb)}, {-std::sin(yb), std::cos(yb)}};
  const float dx = b.center.x - a.center.x, dy = b.center.y - a.center.y;
  for (const auto& n : ax) {
    // Each box's half width along this axis: its two sides' extents projected on it.
    float ra = a.extent.x * std::fabs(n[0] * ax[0][0] + n[1] * ax[0][1]) + a.extent.y * std::fabs(n[0] * ax[1][0] + n[1] * ax[1][1]);
    float rb = b.extent.x * std::fabs(n[0] * ax[2][0] + n[1] * ax[2][1]) + b.extent.y * std::fabs(n[0] * ax[3][0] + n[1] * ax[3][1]);
    if (std::fabs(dx * n[0] + dy * n[1]) > ra + rb) return false;
  }
  return true;
}
// A Shadow's body as the box meets it: its capsule (radius, half height), else a slim guess.
void body_of(UObject* shadow, float& radius, float& half_height) {
  radius = 30.0f; half_height = 60.0f;
  UObject* cap = get<UObject*>(shadow, "CapsuleComponent");
  if (!cap) return;
  Params r(find_function(class_of(cap), "GetScaledCapsuleRadius")), h(find_function(class_of(cap), "GetScaledCapsuleHalfHeight"));
  if (call(cap, r) && call(cap, h)) { radius = r.ret<float>(); half_height = h.ret<float>(); }
}
// The body's middle inside the box grown by part of the body. A roaming Shadow's capsule is 1 m in radius (floor 2,
// 2026-11-10); only six tenths of it is counted, so the ring means the strike is well inside its reach, not at the
// edge of it, and a Shadow that takes a step back is still hit.
bool box_holds(const Box& b, const FVector& p, float radius, float half_height) {
  float a = -b.yaw * kPi / 180.0f, dx = p.x - b.center.x, dy = p.y - b.center.y;
  float lx = dx * std::cos(a) - dy * std::sin(a), ly = dx * std::sin(a) + dy * std::cos(a);
  const float grow = radius * 0.6f;
  return std::fabs(lx) <= b.extent.x + grow && std::fabs(ly) <= b.extent.y + grow && std::fabs(p.z - b.center.z) <= b.extent.z + half_height;
}
// The strike would connect with this Shadow now. `known` false: the game's hit area could not be read.
bool in_reach(UObject* shadow, const FVector& at, bool& known) {
  UObject* dc = dungeon_comp();
  known = false;
  if (!dc) return false;
  if (get_bool(dc, "IsUsingLongRangeAttack")) { known = true; return get<UObject*>(dc, "LockOnActor") == shadow; }
  Box b;
  if (!strike_box(dc, b)) return false;
  known = true;
  float r, hh;
  body_of(shadow, r, hh);
  return box_holds(b, at, r, hh);
}

// A breakable thing (Tartarus' Shadow remains) the strike would break now: the game's hit test takes them from the
// same box as the Shadows (the actors overlapping AttackArea's AreaComp). That box collides with nothing between
// swings (CollisionEnabled NoCollision at rest, seen in the player's game 2026-11-10: the engine's overlap answered
// false beside a breakable), so the two boxes are compared here: the strike's and the breakable's own Box.
bool breakable_in_reach(const FVector& me, FVector& at) {
  UObject* dc = dungeon_comp();
  Box strike;
  if (!dc || get_bool(dc, "IsUsingLongRangeAttack") || !strike_box(dc, strike)) return false;
  UObject* found = nullptr;
  float best = 600.0f;   // cm: nothing farther is in the box
  guarded([&] {
    for (UObject* a : live("FldDungeonObjectActor")) {
      FVector v = world::actor_location(a);
      float d = std::hypot(flat(v, me), v.z - me.z);
      if (d >= best || !poi::breakable(a)) continue;
      Box body;
      if (read_box(get<UObject*>(a, "Box"), body) && boxes_meet(strike, body)) { best = d; found = a; at = v; }
    }
  });
  static WeakObj logged;
  if (WeakObj w = WeakObj::of(found); found && !(w == logged)) {
    logged = w;
    log::writef("shadow: breakable #{} in the strike's reach at {:.1f} m", w.index, best / 100.0f);
  }
  return found != nullptr;
}

// Within 4 m the nearest Shadow's heading is spoken once ("back turned" / "facing you"). In reach of the strike a
// blade's ring repeats; from behind it (the battle then starts with the advantage) two rising ticks instead.
// Returns the ring's cue while the Shadow is in reach, else -1.
int close_cues(const Seen& s, const FVector& me) {
  static WeakObj told;
  static int told_state = -1;
  float to_me = std::atan2(me.y - s.at.y, me.x - s.at.x) * 180.0f / kPi;
  bool back_turned = angle_diff(yaw_of(s.actor), to_me) > 110.0f;   // the hero is outside its front
  WeakObj w = WeakObj::of(s.actor);
  if (s.dist < 400.0f) {
    int st = back_turned ? 1 : 0;
    if (!(w == told) || st != told_state) {
      told = w; told_state = st;
      speech::speak(back_turned ? strings::kShadowBackTurned : strings::kShadowFacingYou, false);
    }
  } else if (s.dist > 600.0f && w.index == told.index) {
    told_state = -1;
  }
  if (s.dist > 800.0f) return -1;
  bool known = false;
  bool reach = in_reach(s.actor, s.at, known);
  if (!known) {
    // No hit area to read: the old estimate, close behind it and facing it.
    UObject* hero = world::player();
    float to_shadow = std::atan2(s.at.y - me.y, s.at.x - me.x) * 180.0f / kPi;
    reach = hero && back_turned && s.dist < 220.0f && angle_diff(yaw_of(hero), to_shadow) < 45.0f;
  }
  return !reach ? -1 : (int)(back_turned ? SignalCue::Ambush : SignalCue::Reach);
}

// What each Shadow knows of the hero, followed from the marks the game shows over it: the exclamation mark is the
// moment it notices him (the alarm), and it hunts him from then until its question mark (it lost him) or until it
// lets go of him as its target (the falling notes). The target alone starts nothing: only the mark says "noticed".
struct Mind { bool hunting = false, tracked = false; Mark last = Mark::None; };
std::map<WeakObj::Key, Mind> g_minds;
bool hunting(UObject* shadow) {
  auto it = g_minds.find(WeakObj::of(shadow).key());
  return it != g_minds.end() && it->second.hunting;
}
void mind_cues(const std::vector<Seen>& seen, const FVector& me) {
  std::map<WeakObj::Key, Mind> now_known;
  for (const Seen& s : seen) {
    if (s.dist > kMindRange) continue;
    auto key = WeakObj::of(s.actor).key();
    auto it = g_minds.find(key);
    Mind m = it == g_minds.end() ? Mind{} : it->second;
    bool was = m.hunting;
    if (s.raw.mark == Mark::Exclamation && m.last != Mark::Exclamation) { m.hunting = true; m.tracked = false; }
    else if (m.hunting) {
      if (s.raw.mark == Mark::Question && m.last != Mark::Question) m.hunting = false;
      else if (s.raw.tracking) m.tracked = true;
      else if (m.tracked) m.hunting = false;   // it held him as its target and has let go
    }
    m.last = s.raw.mark;
    if (m.hunting && !was) { audio::signal((int)SignalCue::Spotted, 1.4f, world::spot(me, s.at)); log::writef("shadow: #{} noticed the hero at {:.1f} m", std::get<0>(key), s.dist / 100.0f); }
    else if (was && !m.hunting) { audio::signal((int)SignalCue::Lost, 1.1f, world::spot(me, s.at)); log::writef("shadow: #{} lost the hero", std::get<0>(key)); }
    now_known[key] = m;
  }
  g_minds.swap(now_known);
}
}  // namespace

void tick() {
  if (!settings::on(settings::ShadowAlerts)) return;
  FVector me;
  if (!world::player_location(me)) return;
  std::vector<Seen> seen = look(me);
  mind_cues(seen, me);
  const Seen* nearest = nullptr;
  for (const Seen& s : seen) if (!nearest || s.dist < nearest->dist) nearest = &s;
  const bool shadow_near = nearest && nearest->dist <= kPulseRange;
  // The strike's ring: from the Shadow in reach, else from a breakable thing in reach.
  int ring = shadow_near ? close_cues(*nearest, me) : -1;
  FVector ring_at = shadow_near ? nearest->at : FVector{};
  if (ring < 0 && breakable_in_reach(me, ring_at)) ring = (int)SignalCue::Reach;
  ULONGLONG now = GetTickCount64();
  static ULONGLONG rang_at = 0;
  if (ring >= 0 && now - rang_at >= kReachMs) {
    rang_at = now;
    audio::signal(ring, 1.2f, world::spot(me, ring_at));
  }
  if (!shadow_near) return;
  static ULONGLONG next = 0;
  if (now < next) return;
  float closeness = 1.0f - nearest->dist / kPulseRange;
  bool hunted = hunting(nearest->actor);
  bool behind = std::cos(world::bearing(me, nearest->at) * kPi / 180.0f) < -0.2f && settings::on(settings::BehindPitch);
  // Hunting the hero: higher, louder and nearly twice as fast.
  audio::signal((int)SignalCue::Shadow, 1.0f + 0.8f * closeness + (hunted ? 0.3f : 0.0f), world::spot(me, nearest->at),
                (behind ? 0.7f : 1.0f) * (hunted ? 1.3f : 1.0f));
  next = now + (ULONGLONG)((220 + 780 * (1.0f - closeness)) * (hunted ? 0.55f : 1.0f));
  // A Shadow's first growl is logged with its distance (how early the player could have heard it).
  static WeakObj growled;
  if (WeakObj w = WeakObj::of(nearest->actor); !(w == growled)) {
    growled = w;
    log::writef("shadow: #{} first growls at {:.1f} m{}", w.index, nearest->dist / 100.0f, hunted ? ", hunting" : "");
  }
}

void install() {
  dev::route("/shadows", [](const dev::Request&, int&) {
    return dev::on_game([] {
      FVector me;
      if (!world::player_location(me)) return std::string("no hero\n");
      std::string s;
      UObject* dc = dungeon_comp();
      Box b{};
      bool has_box = strike_box(dc, b);
      UObject* area = dc ? get<UObject*>(dc, "AttackArea") : nullptr;
      UObject* comp = area ? get<UObject*>(area, "AreaComp") : nullptr;
      s += std::format("hero ({:.0f}, {:.0f}, {:.0f}) dungeon_comp={} long_range={} lock_on={}\n", me.x, me.y, me.z, dc ? name_of(dc) : "-",
                       dc ? get_bool(dc, "IsUsingLongRangeAttack") : false, dc && get<UObject*>(dc, "LockOnActor") ? name_of(get<UObject*>(dc, "LockOnActor")) : "-");
      if (has_box)
        s += std::format("strike box centre ({:.0f}, {:.0f}, {:.0f}) extent ({:.0f}, {:.0f}, {:.0f}) yaw {:.0f}, {:.0f} cm from the hero\n", b.center.x, b.center.y,
                         b.center.z, b.extent.x, b.extent.y, b.extent.z, b.yaw, flat(b.center, me));
      else s += "no strike box\n";
      for (UObject* a : live("FldDungeonObjectActor")) {
        if (!poi::breakable(a)) continue;
        FVector v = world::actor_location(a);
        bool engine = false;
        if (comp) { Params p(find_function(class_of(comp), "IsOverlappingActor")); p.set<UObject*>("Other", a); if (call(comp, p)) engine = p.ret<bool>(); }
        Box body{};
        bool has_body = read_box(get<UObject*>(a, "Box"), body);
        s += std::format("  breakable {} #{} {:.0f} cm bearing {:.0f} engine overlap={} box centre ({:.0f}, {:.0f}, {:.0f}) extent ({:.0f}, {:.0f}, {:.0f}) yaw {:.0f} meets={}\n",
                         name_of(a), index_of(a), std::hypot(flat(v, me), v.z - me.z), world::bearing(me, v), engine, body.center.x, body.center.y, body.center.z,
                         body.extent.x, body.extent.y, body.extent.z, body.yaw, has_body && has_box && boxes_meet(b, body));
      }
      if (FVector at; breakable_in_reach(me, at)) s += "  a breakable is in the strike's reach\n";
      for (const Seen& e : look(me)) {
        bool known = false;
        bool reach = in_reach(e.actor, e.at, known);
        float r, hh;
        body_of(e.actor, r, hh);
        bool engine = false;
        if (comp) { Params p(find_function(class_of(comp), "IsOverlappingActor")); p.set<UObject*>("Other", e.actor); if (call(comp, p)) engine = p.ret<bool>(); }
        s += std::format("  {} #{} {:.0f} cm mark={} tracking={} hunting={} reach={} (known={}, engine overlap={}) body r {:.0f} hh {:.0f}\n", name_of(e.actor),
                         index_of(e.actor), e.dist, (int)e.raw.mark, e.raw.tracking, hunting(e.actor), reach, known, engine, r, hh);
      }
      for (const Holder& h : holders()) {
        UObject* mark = first_object(h.comp, "CurrentMarkEffectParam");
        UObject* track = first_object(h.comp, "TrackingTarget");
        s += std::format("  holder {} ({}) comp {} state {} mark {} tracking {} dead {} vanish {}\n", name_of(h.actor), name_of(class_of(h.actor)),
                         name_of(class_of(h.comp)), get<int32_t>(h.comp, "State", -1), mark ? name_of(mark) : "-", track ? name_of(track) : "-",
                         get_bool(h.comp, "bIsDead"), get_bool(h.comp, "bVanish"));
      }
      return s;
    });
  }, "the Shadows on the floor: what each knows of the hero, and the strike's reach");
}
}  // namespace p3a::shadow

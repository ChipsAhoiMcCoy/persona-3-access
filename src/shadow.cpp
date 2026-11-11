// Shadows on a Tartarus floor. What the game holds:
//  - AEnemySymbolBaseCore: the roaming Shadow's actor (hidden while parked or defeated).
//  - AFldEnemyManager.Enemies: one FFldEnemyHolder { Actor, EnemyComp } per Shadow (Actor is that same actor). A
//    UFldEnemyNormalComp has CurrentMarkEffectParam, the mark over its head (its Asset is ExclamationEffect's or
//    QuestionEffect's), and TrackingTarget (an FFldPlayerHolder whose Pawn is whom it is after). Read from the
//    classes' layout, not yet watched in play: UNTESTED which moments set and clear them, so the mark alone starts
//    the alarm (mind_cues).
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
constexpr float kPulseRange = 1500.0f;   // cm: the growl
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
    if (std::fabs(v.z - me.z) > 300.0f) continue;
    out.push_back({a, v, flat(v, me), Raw{}});
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
bool strike_box(UObject* dc, Box& b) {
  UObject* area = dc ? get<UObject*>(dc, "AttackArea") : nullptr;
  UObject* box = area ? get<UObject*>(area, "AreaComp") : nullptr;
  if (!box) return false;
  auto vcall = [&](const char* fn) { Params p(find_function(class_of(box), fn)); call(box, p); return p; };
  b.center = vcall("K2_GetComponentLocation").ret<FVector>();
  b.extent = vcall("GetScaledBoxExtent").ret<FVector>();
  b.yaw = vcall("K2_GetComponentRotation").ret<FRotator>().yaw;
  return b.extent.x > 1.0f && b.extent.y > 1.0f;
}
// A Shadow's body as the box meets it: its capsule (radius, half height), else a slim guess.
void body_of(UObject* shadow, float& radius, float& half_height) {
  radius = 30.0f; half_height = 60.0f;
  UObject* cap = get<UObject*>(shadow, "CapsuleComponent");
  if (!cap) return;
  Params r(find_function(class_of(cap), "GetScaledCapsuleRadius")), h(find_function(class_of(cap), "GetScaledCapsuleHalfHeight"));
  if (call(cap, r) && call(cap, h)) { radius = r.ret<float>(); half_height = h.ret<float>(); }
}
// The body's middle inside the box grown by the body, less a hand's width so the edge is not called a hit.
bool box_holds(const Box& b, const FVector& p, float radius, float half_height) {
  float a = -b.yaw * kPi / 180.0f, dx = p.x - b.center.x, dy = p.y - b.center.y;
  float lx = dx * std::cos(a) - dy * std::sin(a), ly = dx * std::sin(a) + dy * std::cos(a);
  const float grow = std::max(radius - 10.0f, 0.0f);
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

// Within 4 m the nearest Shadow's heading is spoken once ("back turned" / "facing you"). In reach of the strike a
// blade's ring repeats; from behind it (the battle then starts with the advantage) two rising ticks instead.
void close_cues(const Seen& s, const FVector& me) {
  static WeakObj told;
  static int told_state = -1;
  static ULONGLONG rang_at = 0;
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
  if (s.dist > 800.0f) return;
  bool known = false;
  bool reach = in_reach(s.actor, s.at, known);
  if (!known) {
    // No hit area to read: the old estimate, close behind it and facing it.
    UObject* hero = world::player();
    float to_shadow = std::atan2(s.at.y - me.y, s.at.x - me.x) * 180.0f / kPi;
    reach = hero && back_turned && s.dist < 220.0f && angle_diff(yaw_of(hero), to_shadow) < 45.0f;
  }
  ULONGLONG now = GetTickCount64();
  if (reach && now - rang_at >= kReachMs) {
    rang_at = now;
    audio::signal((int)(back_turned ? SignalCue::Ambush : SignalCue::Reach), 1.2f, world::spot(me, s.at));
  }
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
    if (m.hunting && !was) audio::signal((int)SignalCue::Spotted, 1.4f, world::spot(me, s.at));
    else if (was && !m.hunting) audio::signal((int)SignalCue::Lost, 1.1f, world::spot(me, s.at));
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
  if (!nearest || nearest->dist > kPulseRange) return;
  close_cues(*nearest, me);
  static ULONGLONG next = 0;
  ULONGLONG now = GetTickCount64();
  if (now < next) return;
  float closeness = 1.0f - nearest->dist / kPulseRange;
  bool hunted = hunting(nearest->actor);
  bool behind = std::cos(world::bearing(me, nearest->at) * kPi / 180.0f) < -0.2f && settings::on(settings::BehindPitch);
  // Hunting the hero: higher, louder and nearly twice as fast.
  audio::signal((int)SignalCue::Shadow, 0.5f + 1.0f * closeness + (hunted ? 0.3f : 0.0f), world::spot(me, nearest->at),
                (behind ? 0.7f : 1.0f) * (hunted ? 1.3f : 1.0f));
  next = now + (ULONGLONG)((250 + 900 * (1.0f - closeness)) * (hunted ? 0.55f : 1.0f));
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

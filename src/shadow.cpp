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
#include "core/boxes.h"
#include "core/sonar_cues.h"
#include "core/strings.h"
#include "devserver.h"
#include "log.h"
#include "poi.h"
#include "seh.h"
#include "settings.h"
#include "speech.h"
#include "watch.h"
#include "topdown.h"
#include "world.h"
#include <windows.h>
#include <cmath>
#include <format>
#include <map>
#include <vector>

namespace p3a::shadow {
namespace {
using namespace ue;
using core::angle_diff;
using core::Box;
using core::boxes_meet;
using core::kPi;
using core::SignalCue;
using world::FVector;
using world::read_box;
using world::yaw_of;

// cm: the growl. 15 m at first, and only for a Shadow within 3 m of the hero's height: it was not heard until it was
// about to strike (the user, 2026-11-10). A Tartarus floor has raised and sunken parts, so height filters nothing now.
constexpr float kPulseRange = 3000.0f;
constexpr float kMindRange = 4000.0f;    // cm: how far off a Shadow's noticing is told
constexpr ULONGLONG kReachMs = 150;      // the ring's rhythm while in reach (the ring itself lasts 0.09 s)

// What a Shadow's component shows this frame: the mark over its head, and whether it holds the hero as its target.
enum class Mark { None, Exclamation, Question };
struct Raw { Mark mark = Mark::None; bool tracking = false; };
struct Seen { UObject* actor; FVector at; float dist; Raw raw; };

std::vector<UObject*> live(std::string_view cls) { return watch::all(watch::want(cls, true)); }
float flat(const FVector& a, const FVector& b) { return std::hypot(a.x - b.x, a.y - b.y); }

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

// The enemy manager's entries that have both an actor and a component.
std::vector<Holder> holders() {
  std::vector<Holder> out;
  for (const Holder& h : enemy_holders()) if (h.actor && h.comp) out.push_back(h);
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
UObject* dungeon_comp() {
  static const int id = watch::want("FldPlayerDungeonComp");
  return watch::get(id);
}
bool strike_box(UObject* dc, Box& b) {
  UObject* area = dc ? get<UObject*>(dc, "AttackArea") : nullptr;
  return read_box(area ? get<UObject*>(area, "AreaComp") : nullptr, b);
}
// A Shadow's body as the box meets it: its capsule (radius, half height), else a slim guess.
void body_of(UObject* shadow, float& radius, float& half_height) {
  radius = 30.0f; half_height = 60.0f;
  world::capsule_of(shadow, radius, half_height);
}
// The body's middle inside the box grown by part of the body. A roaming Shadow's capsule is 1 m in radius (floor 2,
// 2026-11-10); only six tenths of it is counted, so the ring means the strike is well inside its reach, not at the
// edge of it, and a Shadow that takes a step back is still hit.
bool box_holds(const Box& b, const FVector& p, float radius, float half_height) {
  return core::box_holds(b, world::vec3(p), radius * 0.6f, half_height);
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

// Within 4 m the nearest Shadow's heading is spoken ("back turned" / "facing you"): each of the two lines at most
// once per approach, however often the Shadow turns meanwhile. An approach ends when that Shadow is more than 6 m
// off, or when another Shadow becomes the close one. In reach of the strike a blade's ring repeats; from behind it
// (the battle then starts with the advantage) two rising ticks instead: the ring follows its facing as it is now.
// Returns the ring's cue while the Shadow is in reach, else -1.
int close_cues(const Seen& s, const FVector& me) {
  static WeakObj told;                 // the Shadow this approach is to
  static bool said[2] = {false, false};   // "facing you", "back turned": said on this approach
  float to_me = std::atan2(me.y - s.at.y, me.x - s.at.x) * 180.0f / kPi;
  bool back_turned = angle_diff(yaw_of(s.actor), to_me) > 110.0f;   // the hero is outside its front
  WeakObj w = WeakObj::of(s.actor);
  if (s.dist < 400.0f) {
    if (!(w == told)) { told = w; said[0] = said[1] = false; }
    int st = back_turned ? 1 : 0;
    if (!said[st]) {
      said[st] = true;
      speech::speak(back_turned ? strings::kShadowBackTurned : strings::kShadowFacingYou, false);
    }
  } else if (s.dist > 600.0f && w.index == told.index) {
    said[0] = said[1] = false;
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
// Every Shadow on the floor is followed for as long as its actor is there and shown, however far off it is, so one
// that hunts the hero out of earshot and back still reads as hunting; the two cues sound only within kMindRange.
struct Mind { bool hunting = false, tracked = false; Mark last = Mark::None; };
std::map<WeakObj::Key, Mind> g_minds;
bool hunting(UObject* shadow) {
  auto it = g_minds.find(WeakObj::of(shadow).key());
  return it != g_minds.end() && it->second.hunting;
}
void mind_cues(const std::vector<Seen>& seen, const FVector& me) {
  std::map<WeakObj::Key, Mind> now_known;
  for (const Seen& s : seen) {
    const bool heard = s.dist <= kMindRange;
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
    if (m.hunting && !was) {
      if (heard) audio::signal((int)SignalCue::Spotted, 1.4f, world::spot(me, s.at));
      log::writef("shadow: #{} noticed the hero at {:.1f} m{}", std::get<0>(key), s.dist / 100.0f, heard ? "" : " (too far off for the cue)");
    } else if (was && !m.hunting) {
      if (heard) audio::signal((int)SignalCue::Lost, 1.1f, world::spot(me, s.at));
      log::writef("shadow: #{} lost the hero{}", std::get<0>(key), heard ? "" : " (too far off for the cue)");
    }
    now_known[key] = m;
  }
  g_minds.swap(now_known);
}
}  // namespace

std::vector<Holder> enemy_holders() {
  static const int id = watch::want("FldEnemyManager");
  std::vector<Holder> out;
  UObject* mgr = watch::get(id);
  auto* arr = mgr ? field<TArray<Holder>>(mgr, "Enemies") : nullptr;
  if (arr && arr->data && arr->num > 0 && arr->num < 256)
    guarded([&] { for (int i = 0; i < arr->num; ++i) out.push_back(arr->data[i]); });
  return out;
}

void tick() {
  if (!settings::on(settings::ShadowAlerts) || !world::exploring()) return;
  FVector me;
  if (!world::player_location(me)) return;
  std::vector<Seen> seen = look(me);
  mind_cues(seen, me);
  const Seen* nearest = nullptr;
  for (const Seen& s : seen) if (!nearest || s.dist < nearest->dist) nearest = &s;
  const bool shadow_near = nearest && nearest->dist <= kPulseRange;
  // The strike's ring: a Shadow is in reach, else a breakable thing is. It says "you can strike now", so it comes
  // from the player (no place, both ears alike), not from the thing (the user's call, 2026-11-10).
  int ring = shadow_near ? close_cues(*nearest, me) : -1;
  FVector ring_at{};
  if (ring < 0 && breakable_in_reach(me, ring_at)) ring = (int)SignalCue::Reach;
  ULONGLONG now = GetTickCount64();
  static ULONGLONG rang_at = 0;
  if (ring >= 0 && now - rang_at >= kReachMs) {
    rang_at = now;
    audio::signal_here(ring, 1.2f);
  }
  if (!shadow_near) return;
  static ULONGLONG next = 0;
  if (now < next) return;
  float closeness = 1.0f - nearest->dist / kPulseRange;
  bool hunted = hunting(nearest->actor);
  bool behind = std::cos(world::bearing(me, nearest->at) * kPi / 180.0f) < -0.2f && settings::on(settings::BehindPitch) && !topdown::active();
  // Hunting the hero: higher, louder and nearly twice as fast.
  audio::signal((int)SignalCue::Shadow, 1.0f + 0.8f * closeness + (hunted ? 0.3f : 0.0f), world::spot(me, nearest->at),
                (behind ? 0.7f : 1.0f) * (hunted ? 1.3f : 1.0f));
  // From every 0.8 s at the range's edge to every 0.2 s beside the hero: under the growl's own length up close, where
  // one runs into the next and it is one unbroken snarl.
  next = now + (ULONGLONG)((200 + 600 * (1.0f - closeness)) * (hunted ? 0.55f : 1.0f));
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
                         b.center.z, b.extent.x, b.extent.y, b.extent.z, b.yaw, flat(world::fvec(b.center), me));
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

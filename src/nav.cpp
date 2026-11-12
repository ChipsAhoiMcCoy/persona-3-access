#include "nav.h"
#include "core/approach.h"
#include "core/boxes.h"
#include "core/strings.h"
#include "devserver.h"
#include "doors.h"
#include "hooks.h"
#include "input.h"
#include "keys.h"
#include "log.h"
#include "msg.h"
#include "poi.h"
#include "screens/screen.h"
#include "speech.h"
#include "seh.h"
#include "watch.h"
#include "world.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <memory>
#include <vector>

namespace p3a::nav {
using world::FVector;
using ue::WeakObj;

// One walk's plan: everything here starts over with each walk (walk_to resets it by value).
struct Walk {
  WeakObj target;
  std::string name;
  std::vector<FVector> path;
  ULONGLONG path_at = 0, progress_at = 0;
  FVector progress_pos{};
  bool last_valid = false;   // the hero position of the previous walking tick is known
  FVector detour{};
  bool detour_on = false;
  int detours = 0;            // detours tried on this walk
  float detour_base = 0;      // the side first blocked on (radians from the target)
  bool direct = false;        // after a detour: straight for the target, not back along the path
  bool off_mesh = false;      // the target's own spot has no navmesh path: it stands inside something
  bool planned = false;       // the ways in were looked for on this walk
  std::vector<FVector> entries;   // a standing point outside each way in to the target, nearest first
  size_t entry_next = 0;
  FVector side{};             // a step to the side, round someone or something the path runs into
  bool side_on = false;
  int side_tries = 0;         // steps tried after getting stuck, with the target no nearer since
  float side_best = 1e9f;     // the nearest to the target the walk was when it got stuck
};
static Walk g_w;
static bool g_walking = false;
static uint32_t g_held[2] = {0, 0};
// The stuck check starts again from here.
static void restart_progress(const FVector& me, ULONGLONG now) { g_w.progress_at = now; g_w.progress_pos = me; }
constexpr float kArrive = 110.0f;      // cm from the target's location
constexpr float kWaypointReached = 70.0f;

static float flat(const FVector& a, const FVector& b) { return core::flat(world::vec3(a), world::vec3(b)); }

// Keys let go of while the game took no field input (a battle opening, a message window, a menu): the key-up reaches
// the window but not the player controller, which kept A held into a battle (the target wheel ran left every 100 ms,
// 2026-11-01; the battle reads PlayerController key state, BP_BtlGuiComponent CheckKeyboardInput). Each is let go
// again once the game takes input: the battle's command wheel, or the field with nothing over it.
static uint32_t g_owed[4] = {};
static bool g_taking_input = true;   // the game took field or battle input at this tick's start
static void owe(uint32_t k) {
  if (g_taking_input) return;
  for (uint32_t& o : g_owed) if (o == k) return;
  for (uint32_t& o : g_owed) if (!o) { o = k; return; }
}

static void hold(uint32_t a, uint32_t b) {
  uint32_t want[2] = {a, b};
  for (uint32_t k : g_held) if (k && k != a && k != b) { input::post_key(k, false, true); owe(k); }
  for (uint32_t k : want) if (k && k != g_held[0] && k != g_held[1]) input::post_key(k, true, true);
  g_held[0] = a; g_held[1] = b;
}
static void release() { hold(0, 0); }

void stop(bool announce) {
  bool was = g_walking;
  g_walking = false;
  release();
  if (was && announce) speech::speak(strings::kStopped, true);
}
bool walking() { return g_walking; }
// The walk reached its target: logged only (the player will give arrival its own cue; "Arrived" was spoken until
// 2026-10-31).
static void arrived_at() { log::writef("nav: arrived, {}", g_w.name); }

static bool refresh_path(const FVector& me) {
  ue::UObject* t = g_w.target.resolve();
  if (!t) return false;
  // On the way round to another side of the target, the path leads to that side's standing point.
  bool round = g_walking && g_w.detour_on;
  FVector to = round ? g_w.detour : world::actor_location(t);
  g_w.path = world::find_path(me, to);
  if (!round) g_w.off_mesh = g_w.path.empty();
  // A thing standing off the navmesh (the entrance terminal, 2026-10-28) has no path to its centre: path to the
  // nearest navmesh point beside it, then head straight in. A story trigger can float well above its floor (the box
  // that starts the Takaya fight is 7 m over the ramp at the top of Tartarus, 2026-11-11: with no point within 3 m
  // the walk went straight at it, into the pillar the way leads round): the search widens once before giving up.
  FVector beside{};
  if (g_w.path.empty() && (world::project_to_nav(to, 300.0f, beside) || world::project_to_nav(to, 1200.0f, beside))) { g_w.path = world::find_path(me, beside); if (!g_w.path.empty()) g_w.path.push_back(to); }
  if (g_w.path.empty()) g_w.path.push_back(to);   // no navmesh here: head straight for it (the stuck check still guards)
  g_w.path_at = GetTickCount64();
  return true;
}

void walk_to(const WeakObj& target, const std::string& name) {
  FVector me;
  static const int battle = watch::want("BtlCoreComponent");
  if (!world::player_location(me) || watch::get(battle)) { speech::speak(strings::kNotInField, true); return; }
  // Past a closed door: the navmesh runs on under it, so there is a path, and it ends against the door ("Stuck",
  // the gym hallway, 2026-11-11).
  // In this room with no path inside it (classroom 2-F's desks): said so, the player walks the last of it.
  if (ue::UObject* t = target.resolve(); t && doors::walk_shut(t, me, world::actor_location(t))) {
    speech::speak(doors::behind(t, me, world::actor_location(t)) ? strings::kNoPath : strings::kNoPathInRoom, true);
    return;
  }
  g_w = Walk{};   // a new walk's plan starts empty; the keys held and g_walking are not part of it
  g_w.target = target;
  g_w.name = name;
  if (!refresh_path(me)) { speech::speak(strings::kNoPath, true); return; }
  g_walking = true;
  restart_progress(me, GetTickCount64());
  // Nothing said on starting (the player's call, 2026-10-29): only arriving, stopping or getting stuck is spoken.
}

// A hit actor's trigger box (HitComp_: BoxComponent world centre, scaled extent, yaw), or false if it has none.
using core::Box;
static bool trigger_box(ue::UObject* t, Box& b) {
  ue::UObject* hc = ue::get<ue::UObject*>(t, "HitComp_");
  if (!hc) return false;
  bool box = ue::is_a(hc, "BoxComponent"), sphere = ue::is_a(hc, "SphereComponent");   // Tartarus devices use spheres
  if (!box && !sphere) return false;
  if (box) {
    world::read_box(hc, b);   // a trigger counts however thin it is
  } else {
    auto vcall = [&](const char* fn) {
      ue::Params p(ue::find_function(ue::class_of(hc), fn));
      ue::call(hc, p);
      return p;
    };
    b.center = world::vec3(vcall("K2_GetComponentLocation").ret<FVector>());
    float r = vcall("GetScaledSphereRadius").ret<float>() * 0.7f;   // the square inside the circle
    b.extent = core::Vec3{r, r, r};
    b.yaw = 0;
  }
  return true;
}
// The point is inside the trigger, `margin` cm in from (negative) or out past its sides.
// Height too: a door upstairs is right above a spot downstairs (the dorm's "Your Room", 2026-10-27). The hero's
// location is about 90 cm above the floor; allow a step's worth more.
static bool inside(const Box& b, const FVector& p, float margin) { return core::box_holds(b, world::vec3(p), margin, 150.0f); }

// ---- the ways in to a target that stands inside something solid ----
// A teleporter's trigger is a 64 cm box in the middle of the device, between two walls and four corner pillars: the
// hero gets in through a 96 cm gap on one side or a 67 cm one opposite, and nowhere else (Thebel 11, 2026-11-10).
// Circling it a sixth of a turn at a time from the side first bumped missed both gaps every time. The ways in are
// asked of the engine instead: the hero's own body swept toward the trigger from every side.
struct Goal { bool has_box = false; Box box{}; FVector at{}; };
static FVector centre(const Goal& g) { return g.has_box ? world::fvec(g.box.center) : g.at; }
// Arrived = inside the trigger box; a target without one = close to its location.
static bool reaches(const Goal& g, const FVector& p) { return g.has_box ? inside(g.box, p, -5.0f) : flat(g.at, p) < kArrive; }

static void hero_body(float& radius, float& half) {
  radius = 26.5f; half = 85.0f;
  world::capsule_of(world::player(), radius, half);
}

static float path_length(const std::vector<FVector>& p) {
  float d = 0;
  for (size_t i = 1; i < p.size(); ++i) d += flat(p[i - 1], p[i]);
  return d;
}

// The hero's body swept a -> b at the height he walks at (`me` gives it): lifted off the floor by a step's height,
// as his own movement steps over a low edge. The characters it meets are looked past and listed in `people`.
static world::Hit sweep_body(const FVector& a, const FVector& b, const FVector& me, float radius, float half,
                             std::vector<ue::UObject*>& people) {
  float z = me.z + 10.0f, h = std::max(radius + 1.0f, half - 30.0f);
  return world::sweep(FVector{a.x, a.y, z}, FVector{b.x, b.y, z}, radius, h, &people);
}

// The body swept from `from` to the target's middle gets far enough to count as arrived. `me` gives the height.
static bool sweep_reaches(const Goal& g, const FVector& from, const FVector& me, float radius, float half,
                          std::vector<ue::UObject*>& people) {
  world::Hit hit = sweep_body(from, centre(g), me, radius, half, people);
  if (hit.start_inside) return false;
  return !hit.hit || reaches(g, FVector{hit.location.x, hit.location.y, me.z});
}

// A standing point outside each way in, the shortest walk first. Empty: no side is open (or every side is).
static std::vector<FVector> find_entries(const Goal& g, const FVector& me, std::string* report = nullptr) {
  constexpr int kDirs = 72;   // 5 degrees apart: the narrow gap above is 8 degrees wide from its middle
  float radius, half;
  hero_body(radius, half);
  FVector c = centre(g);
  std::vector<ue::UObject*> people;   // party members and other characters do not close a way in
  // Where the hero can stand on that side, on the navmesh and clear of the walls, with a free run in from there.
  auto stand = [&](float a, float spare, FVector& out) {
    for (float r : {330.0f, 240.0f, 170.0f}) {
      FVector on{};
      if (!world::project_to_nav(FVector{c.x + r * std::cos(a), c.y + r * std::sin(a), me.z - half}, 60.0f, on)) continue;
      if (!sweep_reaches(g, on, me, radius + spare, half, people)) continue;
      out = FVector{on.x, on.y, me.z};
      return true;
    }
    return false;
  };
  struct Entry { FVector at; float walk; };
  std::vector<Entry> found;
  // First with 10 cm to spare on each side, so a gap the hero only just fits is left for last: the walk holds its
  // line to about that.
  for (float spare : {10.0f, 2.0f}) {
    std::vector<bool> clear(kDirs);
    std::vector<FVector> at(kDirs);
    for (int i = 0; i < kDirs; ++i) clear[i] = stand(6.28318531f * (float)i / kDirs, spare, at[i]);
    auto open = core::openings(clear);
    if (report) *report += std::format("spare {:.0f} cm: {} opening(s)\n", spare, open.size());
    if (open.size() == 1 && open[0].width > 6.2f) return {};   // open all round: nothing to find
    for (const core::Opening& o : open) {
      FVector s{};
      if (!stand(o.angle, spare, s)) s = at[o.index];
      bool partial = false;
      auto path = world::find_path(me, s, &partial);
      bool ok = !path.empty() && !partial;
      if (report)
        *report += std::format("  at {:.0f} deg, {:.0f} deg wide: stand at ({:.0f}, {:.0f}), {}\n", o.angle * 57.29578f, o.width * 57.29578f, s.x, s.y,
                               ok ? std::format("{:.0f} cm to walk", path_length(path)) : std::string("no path to it"));
      if (ok) found.push_back(Entry{s, path_length(path)});
    }
    if (!found.empty()) break;
  }
  std::sort(found.begin(), found.end(), [](const Entry& a, const Entry& b) { return a.walk < b.walk; });
  std::vector<FVector> out;
  for (const Entry& e : found) out.push_back(e.at);
  return out;
}

// ---- round what the path runs into ----
// The navmesh knows the walls and nothing of who stands on the floor: a path down a hallway ran straight into a
// student walking the other way, and the walk pushed against them until it said "Stuck" (the Gym Hallway,
// 2026-11-11). The same goes for anything solid the navmesh was not cut round. The hero's body is swept ahead as
// the game would move it, and when it would not get through, a step to one side is looked for.

// Someone the body would walk into going a -> b. A person beside or behind the start does not count: the hero may
// be touching them already, and walks away from them.
static bool walks_into(const FVector& a, const FVector& b, const std::vector<ue::UObject*>& people, const FVector* goal = nullptr) {
  float len = flat(a, b);
  for (ue::UObject* p : people) {
    FVector at = world::actor_location(p);
    float d = flat(a, at);
    // The person the walk is going to (a character's body is another actor than its talk trigger) is not in the way.
    if (goal && flat(at, *goal) < 200.0f) continue;
    if (d < 1.0f || len < 1.0f) return true;
    if (((b.x - a.x) * (at.x - a.x) + (b.y - a.y) * (at.y - a.y)) / (len * d) > 0.3f) return true;
  }
  return false;
}
// The body can go a -> b: nothing solid on the way and nobody to walk into. `me` gives the height.
static bool body_clear(const FVector& a, const FVector& b, const FVector& me, float radius, float half) {
  std::vector<ue::UObject*> people;
  world::Hit hit = sweep_body(a, b, me, radius, half, people);
  return !hit.hit && !walks_into(a, b, people);
}
// Somebody stands in the next stretch toward the waypoint.
static bool person_ahead(const FVector& me, const FVector& wp, const FVector& goal) {
  float d = flat(me, wp);
  if (d < 1.0f) return false;
  float radius, half;
  hero_body(radius, half);
  float k = std::min(d, 150.0f) / d;
  FVector to{me.x + (wp.x - me.x) * k, me.y + (wp.y - me.y) * k, me.z};
  std::vector<ue::UObject*> people;
  sweep_body(me, to, me, radius, half, people);
  return !people.empty() && walks_into(me, to, people, &goal);
}
// A point to one side that the body can step to and walk on from toward the waypoint: the smallest turn off the
// line first, either side, then further out. `skip` passes over that many that would do (the ones tried already).
static bool find_sidestep(const FVector& me, const FVector& wp, int skip, FVector& out) {
  float radius, half;
  hero_body(radius, half);
  float heading = std::atan2(wp.y - me.y, wp.x - me.x);
  for (float r : {120.0f, 180.0f, 260.0f})
    for (float deg : {45.0f, -45.0f, 70.0f, -70.0f, 90.0f, -90.0f, 120.0f, -120.0f}) {
      float a = heading + deg * 3.14159265f / 180.0f;
      FVector on{};
      if (!world::project_to_nav(FVector{me.x + r * std::cos(a), me.y + r * std::sin(a), me.z - half}, 40.0f, on)) continue;
      FVector s{on.x, on.y, me.z};
      if (flat(s, me) < 60.0f || !body_clear(me, s, me, radius, half)) continue;
      float d = flat(s, wp);
      if (d < 30.0f) continue;
      float k = std::min(d, 250.0f) / d;
      if (!body_clear(s, FVector{s.x + (wp.x - s.x) * k, s.y + (wp.y - s.y) * k, me.z}, me, radius, half)) continue;
      if (skip-- > 0) continue;
      out = s;
      return true;
    }
  return false;
}

// The game shows a hit actor's prompt only while the hero faces the way its mHeroDirectType_ asks: Back = against
// the actor's own forward (into the door; the faculty office door, yaw -90, wants the hero at yaw 90, measured
// 2026-10-27), Front = along it, Left/Right = a quarter turn, Center = toward its location. BackNG (a seated Social
// Link partner) is Center too, refused only from within 10 degrees of straight behind the actor (the game's check,
// exe+0x11bf590; turned as Back, the hero stood beside Kenji's desk looking away and no prompt came, 2026-11-11).
// Turn the hero that way on arrival, as a sighted player would.
static void face(ue::UObject* t, const FVector& me) {
  // Things without a facing rule (Tartarus' breakables) are faced straight on, so a strike lands.
  std::string type = ue::find_prop(ue::class_of(t), "mHeroDirectType_") ? ue::enum_prop_name(t, "mHeroDirectType_") : "Center";
  float yaw = world::yaw_of(t);
  if (type == "Back") yaw += 180.0f;
  else if (type == "Left") yaw -= 90.0f;
  else if (type == "Right") yaw += 90.0f;
  else if (type == "Center" || type == "BackNG") { FVector a = world::actor_location(t); yaw = std::atan2(a.y - me.y, a.x - me.x) * 180.0f / 3.14159265f; }
  else if (type != "Front") return;
  ue::UObject* hero = world::player();
  if (!hero) return;
  ue::Params sp(ue::find_function(ue::class_of(hero), "K2_SetActorRotation"));
  sp.set<ue::FRotator>("NewRotation", ue::FRotator{0.0f, yaw, 0.0f});
  ue::call(hero, sp);
}

// The next waypoint not yet reached (the path's first point is our own projected position).
static bool next_waypoint(const FVector& me, FVector& out) {
  while (g_w.path.size() > 1 && flat(g_w.path.front(), me) < kWaypointReached) g_w.path.erase(g_w.path.begin());
  if (g_w.path.empty()) return false;
  out = g_w.path.front();
  return true;
}

void tick() {
  FVector me;
  // A menu up over the field (an exit's "Where should I go?" crossed on the way, a scripted area map; 2026-10-28)
  // holds the walk like a message window: held keys would move its cursor.
  bool located = world::player_location(me), blocking = msg::current().blocking || screens::current();
  // A battle (a Shadow met on the way) keeps the field hero but ends the walk: between turns no screen is up, and the
  // held keys went on into the fight (W tactics, A and D the target) until it said "Stuck" (2026-11-01).
  static const int battle = watch::want("BtlCoreComponent");
  bool fighting = watch::get(battle) != nullptr;
  bool in_field = located && !blocking && !fighting;
  ULONGLONG now = GetTickCount64();
  static const int gui = watch::want("BtlGuiStateManager");
  ue::UObject* g = watch::get(gui);
  bool battle_input = fighting && g && ue::get<uint8_t>(g, "nowState") != 0;
  g_taking_input = in_field || battle_input;
  if (g_taking_input) {
    for (uint32_t& o : g_owed)
      if (o) {
        if (o != g_held[0] && o != g_held[1]) { input::post_key(o, false, true); log::writef("nav: let go of {:#x} again", o); }
        o = 0;
      }
  }
  if (g_walking) {
    if (!located) { stop(false); return; }
    if (fighting) { log::write("nav: stopped, a battle began"); stop(false); return; }
    // A message window or a menu over the field ends the walk, without a word over what it says (the user's call,
    // 2026-11-09). It used to wait and walk on: Mitsuru's "Hold on. It's not safe to continue any further" turns the
    // hero back from a blocked way, and the walk took him straight into it again, over and over.
    if (blocking) { log::write("nav: stopped, a message or a menu came up"); stop(false); return; }
    if (keys::movement_key_down()) { log::write("nav: stopped, a movement key is held"); stop(true); return; }   // the player took over
    if (input::pad_moving()) { log::write("nav: stopped, the controller's stick is pushed"); stop(true); return; }
    // Stairs and doors are area changes that move the hero to another part of the map in one frame; the walk ends
    // there (otherwise it re-paths from the new floor and runs on through the next staircase).
    static FVector last{};
    bool jumped = g_w.last_valid && std::fabs(me.x - last.x) + std::fabs(me.y - last.y) + std::fabs(me.z - last.z) > 400.0f;
    FVector before = last;
    last = me; g_w.last_valid = true;
    ue::UObject* t = g_w.target.resolve();
    if (!t) { log::write("nav: stopped, the target is gone"); stop(true); return; }
    FVector goal = world::actor_location(t);
    // A teleport taken next to the target was the target (stairs); one taken on the way (the path crossed another
    // exit, the back alley's, 2026-10-27) only moved the hero: plan again from there. Where the hero stood before the
    // jump decides, not where he landed: the alley's exit drops him 7 m from the Velvet Room's door, which ended the
    // walk short of it (2026-11-12).
    if (jumped) {
      if (flat(goal, before) < 800.0f) { stop(false); arrived_at(); return; }
      g_w.side_on = false;
      refresh_path(me);
      restart_progress(me, now);
    }
    // A hit actor is usable from inside its trigger box, which can reach well past its centre point (the bed's box
    // is 1.3 x 1.4 m): arrived = inside the box; anything else = close to the location.
    Box box;
    bool has_box = trigger_box(t, box);
    bool arrived = has_box ? inside(box, me, -5.0f) : flat(goal, me) < kArrive;
    if (arrived) { stop(false); face(t, me); arrived_at(); return; }
    Goal target{has_box, box, goal};
    // The next way in: walk round to its standing point (the path leads there while g_w.detour_on), then straight in.
    auto next_entry = [&] {
      if (g_w.entry_next >= g_w.entries.size()) return false;
      g_w.detour = g_w.entries[g_w.entry_next++];
      g_w.detour_on = true;
      g_w.direct = false;
      refresh_path(me);
      restart_progress(me, now);
      log::writef("nav: way in {} of {} via ({:.0f}, {:.0f})", g_w.entry_next, g_w.entries.size(), g_w.detour.x, g_w.detour.y);
      return true;
    };
    auto plan = [&] {
      g_w.planned = true;
      g_w.entries = find_entries(target, me);
      g_w.entry_next = 0;
      log::writef("nav: {} way(s) in to {}", g_w.entries.size(), g_w.name);
    };
    // A target with no navmesh path to its own spot stands inside something. Close to it, before bumping into it:
    // when the hero's body cannot run straight in from here, find the sides it can come in from.
    if (g_w.off_mesh && !g_w.planned && flat(centre(target), me) < 500.0f) {
      float radius, half;
      hero_body(radius, half);
      std::vector<ue::UObject*> people;
      if (sweep_reaches(target, me, me, radius, half, people)) g_w.planned = true;
      else { plan(); next_entry(); }
    }
    // The step to the side is taken: back on the path from there.
    if (g_w.side_on && flat(g_w.side, me) < 45.0f) { g_w.side_on = false; refresh_path(me); }
    if (now - g_w.path_at > 700) refresh_path(me);
    FVector wp;
    if (!next_waypoint(me, wp)) { log::write("nav: stopped, no path"); stop(true); return; }
    if (g_w.detour_on) {
      // Round to another side of the target: at its standing point, turn in.
      if (flat(g_w.detour, me) < 80.0f) { g_w.detour_on = false; g_w.direct = true; refresh_path(me); }
    } else if (g_w.direct) {
      // Straight in from there, held to the line: a way in can be a gap with 20 cm to spare on each side.
      FVector c = centre(target);
      core::Vec2 w = core::follow(core::Vec2{g_w.detour.x, g_w.detour.y}, core::Vec2{c.x, c.y}, core::Vec2{me.x, me.y}, 40.0f);
      wp = FVector{w.x, w.y, me.z};
    } else if (g_w.path.size() == 1 && flat(wp, me) < kWaypointReached + 30.0f && has_box) {
      // The navmesh path ends where the floor ends (beside the bed); from its last point head straight for the box.
      wp = world::fvec(box.center);
    }
    // Somebody in the way is stepped round before the hero walks into them.
    FVector on_path = wp;
    if (!g_w.side_on && !g_w.direct && person_ahead(me, wp, goal) && find_sidestep(me, wp, 0, g_w.side)) {
      g_w.side_on = true;
      log::writef("nav: someone in the way at ({:.0f}, {:.0f}), round via ({:.0f}, {:.0f})", me.x, me.y, g_w.side.x, g_w.side.y);
    }
    if (g_w.side_on) wp = g_w.side;
    // 8-way chord toward the waypoint, camera-relative (the game's own movement is camera-relative too).
    float b = world::camera_bearing(me, wp);
    int sector = ((int)std::lround(b / 45.0f) % 8 + 8) % 8;
    static const uint32_t chords[8][2] = {{'W', 0}, {'W', 'D'}, {'D', 0}, {'S', 'D'}, {'S', 0}, {'S', 'A'}, {'A', 0}, {'W', 'A'}};
    hold(chords[sector][0], chords[sector][1]);
    if (now - g_w.progress_at > 1500) {
      if (flat(me, g_w.progress_pos) < 40.0f) {
        // Blocked by the thing itself (a chest's collision) while its prompt is already up: that is arriving.
        bool there = poi::prompt() == g_w.name;
        // Blocked close to it (the teleporter pad's pylons, 2026-10-27; the entrance terminal opens only to one
        // side, 2026-10-28): come in through the next way in the engine's sweeps found.
        if (!there && flat(goal, me) < 600.0f) {
          log::writef("nav: blocked at ({:.0f}, {:.0f})", me.x, me.y);
          if (!g_w.planned) plan();
          if (next_entry()) return;
          // No side the sweeps call open (the target's box may be reached some other way): circle round and come
          // in from another side, six tries a sixth of a turn apart.
          if (g_w.entries.empty() && g_w.detours < 6) {
            if (g_w.detours == 0) g_w.detour_base = std::atan2(me.y - goal.y, me.x - goal.x);
            ++g_w.detours;
            float a = g_w.detour_base + g_w.detours * 3.14159265f / 3.0f;
            g_w.detour = FVector{goal.x + 330.0f * std::cos(a), goal.y + 330.0f * std::sin(a), me.z};
            g_w.detour_on = true;
            g_w.direct = false;
            refresh_path(me);
            restart_progress(me, now);
            log::writef("nav: detour {} via ({:.0f}, {:.0f})", g_w.detours, g_w.detour.x, g_w.detour.y);
            return;
          }
        }
        // Blocked by something the path does not know of (or the step round it was): step to one side and go on
        // from there. Three steps with the target no nearer, and the walk is stuck.
        if (!there) {
          float left = flat(goal, me);
          if (left < g_w.side_best - 50.0f) { g_w.side_best = left; g_w.side_tries = 0; }
          g_w.side_on = false;
          if (g_w.side_tries < 3 && find_sidestep(me, on_path, g_w.side_tries, g_w.side)) {
            ++g_w.side_tries;
            g_w.side_on = true;
            restart_progress(me, now);
            log::writef("nav: blocked at ({:.0f}, {:.0f}), round via ({:.0f}, {:.0f})", me.x, me.y, g_w.side.x, g_w.side.y);
            return;
          }
        }
        stop(false);
        if (there) arrived_at(); else speech::speak(strings::kStuck, true);
        return;
      }
      restart_progress(me, now);
    }
  }
}

std::string status() {
  return std::format("nav walking={} target='{}' path={} held={:c}{:c}\n", g_walking, g_w.name, g_w.path.size(),
                     g_held[0] ? (char)g_held[0] : '-', g_held[1] ? (char)g_held[1] : '-');
}

// The mod is going (a hot reload): the walk's keys are let go. Left held, the game ran the hero on with nobody
// steering and wedged him in a corner, and the next walk was "Stuck" against the key still down (2026-11-09).
void unload() {
  g_walking = false;
  for (uint32_t& k : g_held) { if (k) input::post_key(k, false, true); k = 0; }
}

void install() {
  // A walk key an earlier copy of the mod left down (it was unloaded or died mid-walk) is let go, unless the player
  // has that key down now.
  for (uint32_t k : {(uint32_t)'W', (uint32_t)'A', (uint32_t)'S', (uint32_t)'D'})
    if (!(GetAsyncKeyState((int)k) & 0x8000)) input::post_key(k, false, true);
  // Dev: the field strike without a key. A posted Ctrl reaches the game's input layer but never starts the attack
  // (2026-10-27); the dungeon component's state machine does: NextBehavior = Attack (7) while Behavior is Free (1)
  // swings once and returns to Free. The player strikes with Ctrl as usual.
  dev::route("/strike", [](const dev::Request&, int&) {
    auto out = std::make_shared<std::string>("no dungeon player\n");
    hooks::run_on_game_thread([out] {
      if (!ue::known_build()) { *out = "another build of the game\n"; return; }   // the offsets below are this build's
      ue::UObject* c = ue::find_first("FldPlayerDungeonComp");
      if (!c) return;
      int32_t* b = ue::field<int32_t>(c, "Behavior");
      int32_t* nb = ue::field<int32_t>(c, "NextBehavior");
      if (!b || !nb) return;
      if (*b != 1) { *out = std::format("busy (behavior {})\n", *b); return; }
      // The attack looks its data up by AttackId (+0x514) in a native set at +0x188 (elements of 0x3c: key +0,
      // next +0x34; buckets inline at +0x1c0 or behind +0x1c8, size +0x1d0) and reads through null when the id is
      // missing: that crashed right after a battle (P3R.exe+0x1125183, 2026-10-27). Same lookup here first.
      uint8_t* p = (uint8_t*)c;
      // A fresh session's AttackId is 0, which has no data; a real first swing uses 1 (the combo's first blow).
      auto has = [&](int32_t id) {
      bool found = false;
      guarded([&] {
        if (*(int32_t*)(p + 0x190) == *(int32_t*)(p + 0x1bc)) return;   // empty
        int32_t* buckets = *(int32_t**)(p + 0x1c8) ? *(int32_t**)(p + 0x1c8) : (int32_t*)(p + 0x1c0);
        int32_t idx = buckets[(*(int32_t*)(p + 0x1d0) - 1) & id];
        uint8_t* elems = *(uint8_t**)(p + 0x188);
        for (int n = 0; idx != -1 && n < 64; ++n) {
          uint8_t* e = elems + (size_t)idx * 0x3c;
          if (*(int32_t*)e == id) { found = true; return; }
          idx = *(int32_t*)(e + 0x34);
        }
      });
      return found;
      };
      int32_t* id = (int32_t*)(p + 0x514);
      if (!has(*id)) {
        if (!has(1)) { *out = std::format("no attack data for id {}\n", *id); return; }
        *id = 1;
      }
      *nb = 7;
      *out = "ok\n";
    });
    return *out;
  }, "field strike in Tartarus (dev)");
  dev::route("/nav", [](const dev::Request& r, int&) {
    if (dev::has(r, "walk")) {
      int obj = (int)dev::arg_int(r, "walk", -1);
      hooks::run_on_game_thread([obj] { ue::UObject* o = ue::object_at(obj); if (!o) return;
        std::string n = poi::hit_name(o);
        walk_to(WeakObj::of(o), n.empty() ? ue::name_of(o) : n); });
    }
    if (dev::has(r, "stop")) hooks::run_on_game_thread([] { stop(true); });
    if (dev::has(r, "plan")) {
      // The ways in to a target as the walk would find them from here (reads only: sweeps and paths).
      int obj = (int)dev::arg_int(r, "plan", -1);
      auto out = std::make_shared<std::string>("timeout\n");
      hooks::run_on_game_thread([out, obj] {
        ue::UObject* o = ue::object_at(obj);
        FVector me;
        if (!o || !world::player_location(me)) { *out = "no player or target\n"; return; }
        Goal g;
        g.has_box = trigger_box(o, g.box);
        g.at = world::actor_location(o);
        float radius, half;
        hero_body(radius, half);
        std::vector<ue::UObject*> people;
        FVector c = centre(g);
        std::string s = std::format("target ({:.0f}, {:.0f}, {:.0f}) box={} hero ({:.0f}, {:.0f}) body {:.1f} x {:.1f}\nstraight in from here: {}\n", c.x, c.y, c.z,
                                    g.has_box, me.x, me.y, radius, half, sweep_reaches(g, me, me, radius, half, people) ? "clear" : "blocked");
        LARGE_INTEGER a, z, f;
        QueryPerformanceCounter(&a);
        auto entries = find_entries(g, me, &s);
        QueryPerformanceCounter(&z); QueryPerformanceFrequency(&f);
        s += std::format("{} way(s) in, found in {} us\n", entries.size(), (z.QuadPart - a.QuadPart) * 1000000 / f.QuadPart);
        *out = s;
      });
      return *out;
    }
    return status();
  }, "?walk=objindex | ?stop=1 the auto-walk | ?plan=objindex the ways in to a target");
}
}  // namespace p3a::nav

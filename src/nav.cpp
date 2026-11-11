#include "nav.h"
#include "audio.h"
#include "core/sonar_cues.h"
#include "core/strings.h"
#include "devserver.h"
#include "hooks.h"
#include "input.h"
#include "keys.h"
#include "log.h"
#include "msg.h"
#include "poi.h"
#include "screens/screen.h"
#include "speech.h"
#include "seh.h"
#include "settings.h"
#include "watch.h"
#include "world.h"
#include <cmath>
#include <format>
#include <memory>
#include <vector>

namespace p3a::nav {
using world::FVector;
using ue::WeakObj;

static WeakObj g_target;
static std::string g_name;
static bool g_walking = false;
static std::vector<FVector> g_path;
static ULONGLONG g_path_at = 0, g_progress_at = 0, g_beacon_at = 0;
static FVector g_progress_pos{};
static bool g_last_valid = false;   // the hero position of the previous walking tick is known
static FVector g_detour{};
static bool g_detour_on = false;
static int g_detours = 0;            // detours tried on this walk
static float g_detour_base = 0;      // the side first blocked on (radians from the target)
static bool g_direct = false;        // after a detour: straight for the target, not back along the path
static uint32_t g_held[2] = {0, 0};
constexpr float kArrive = 110.0f;      // cm from the target's location
constexpr float kWaypointReached = 70.0f;

static float flat(const FVector& a, const FVector& b) { return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); }

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
static void arrived_at() { log::writef("nav: arrived, {}", g_name); }

static bool refresh_path(const FVector& me) {
  ue::UObject* t = g_target.resolve();
  if (!t) return false;
  FVector to = world::actor_location(t);
  g_path = world::find_path(me, to);
  // A thing standing off the navmesh (the entrance terminal, 2026-10-28) has no path to its centre: path to the
  // nearest navmesh point beside it, then head straight in.
  FVector beside{};
  if (g_path.empty() && world::project_to_nav(to, 300.0f, beside)) { g_path = world::find_path(me, beside); if (!g_path.empty()) g_path.push_back(to); }
  if (g_path.empty()) g_path.push_back(to);   // no navmesh here: head straight for it (the stuck check still guards)
  g_path_at = GetTickCount64();
  return true;
}

void walk_to(const WeakObj& target, const std::string& name) {
  FVector me;
  static const int battle = watch::want("BtlCoreComponent");
  if (!world::player_location(me) || watch::get(battle)) { speech::speak(strings::kNotInField, true); return; }
  g_target = target;
  g_name = name;
  if (!refresh_path(me)) { speech::speak(strings::kNoPath, true); return; }
  g_walking = true;
  g_progress_at = GetTickCount64();
  g_progress_pos = me;
  g_last_valid = false;
  g_detour_on = false;
  g_detours = 0;
  g_direct = false;
  // Nothing said on starting (the player's call, 2026-10-29): only arriving, stopping or getting stuck is spoken.
}

// A hit actor's trigger box (HitComp_: BoxComponent world centre, scaled extent, yaw), or false if it has none.
struct Box { FVector center, extent; float yaw; };
static bool trigger_box(ue::UObject* t, Box& b) {
  ue::UObject* hc = ue::get<ue::UObject*>(t, "HitComp_");
  if (!hc) return false;
  bool box = ue::is_a(hc, "BoxComponent"), sphere = ue::is_a(hc, "SphereComponent");   // Tartarus devices use spheres
  if (!box && !sphere) return false;
  auto vcall = [&](const char* fn) {
    ue::Params p(ue::find_function(ue::class_of(hc), fn));
    ue::call(hc, p);
    return p;
  };
  b.center = vcall("K2_GetComponentLocation").ret<FVector>();
  if (box) {
    b.extent = vcall("GetScaledBoxExtent").ret<FVector>();
    b.yaw = vcall("K2_GetComponentRotation").ret<ue::FRotator>().yaw;
  } else {
    float r = vcall("GetScaledSphereRadius").ret<float>() * 0.7f;   // the square inside the circle
    b.extent = FVector{r, r, r};
    b.yaw = 0;
  }
  return true;
}
static bool inside(const Box& b, const FVector& p, float margin) {
  float a = -b.yaw * 3.14159265f / 180.0f;
  float dx = p.x - b.center.x, dy = p.y - b.center.y;
  float lx = dx * std::cos(a) - dy * std::sin(a), ly = dx * std::sin(a) + dy * std::cos(a);
  // Height too: a door upstairs is right above a spot downstairs (the dorm's "Your Room", 2026-10-27). The hero's
  // location is about 90 cm above the floor; allow a step's worth more.
  return std::fabs(lx) <= b.extent.x + margin && std::fabs(ly) <= b.extent.y + margin &&
         std::fabs(p.z - b.center.z) <= b.extent.z + 150.0f;
}

// The game shows a hit actor's prompt only while the hero faces the way its mHeroDirectType_ asks: Back = against
// the actor's own forward (into the door; the faculty office door, yaw -90, wants the hero at yaw 90, measured
// 2026-10-27), Front = along it, Left/Right = a quarter turn, Center = toward its location. Turn the hero that way
// on arrival, as a sighted player would.
static void face(ue::UObject* t, const FVector& me) {
  // Things without a facing rule (Tartarus' breakables) are faced straight on, so a strike lands.
  ue::FField* p = ue::find_prop(ue::class_of(t), "mHeroDirectType_");
  std::string type = p ? ue::enum_name(ue::prop_inner_object(p), *(uint8_t*)((uint8_t*)t + ue::prop_offset(p))) : "Center";
  ue::Params rp(ue::find_function(ue::class_of(t), "K2_GetActorRotation"));
  ue::call(t, rp);
  float yaw = rp.ret<ue::FRotator>().yaw;
  if (type == "Back" || type == "BackNG") yaw += 180.0f;
  else if (type == "Left") yaw -= 90.0f;
  else if (type == "Right") yaw += 90.0f;
  else if (type == "Center") { FVector a = world::actor_location(t); yaw = std::atan2(a.y - me.y, a.x - me.x) * 180.0f / 3.14159265f; }
  else if (type != "Front") return;
  ue::UObject* hero = world::player();
  if (!hero) return;
  ue::Params sp(ue::find_function(ue::class_of(hero), "K2_SetActorRotation"));
  sp.set<ue::FRotator>("NewRotation", ue::FRotator{0.0f, yaw, 0.0f});
  ue::call(hero, sp);
}

// The next waypoint not yet reached (the path's first point is our own projected position).
static bool next_waypoint(const FVector& me, FVector& out) {
  while (g_path.size() > 1 && flat(g_path.front(), me) < kWaypointReached) g_path.erase(g_path.begin());
  if (g_path.empty()) return false;
  out = g_path.front();
  return true;
}

void toggle_beacon() {
  // The beacon is a setting (F2), so it is remembered; the key flips it.
  settings::set(settings::Beacon, !settings::on(settings::Beacon));
  speech::speak(settings::on(settings::Beacon) ? strings::kBeaconOn : strings::kBeaconOff, true);
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
    // Stairs and doors are area changes that move the hero to another part of the map in one frame; the walk ends
    // there (otherwise it re-paths from the new floor and runs on through the next staircase).
    static FVector last{};
    bool jumped = g_last_valid && std::fabs(me.x - last.x) + std::fabs(me.y - last.y) + std::fabs(me.z - last.z) > 400.0f;
    last = me; g_last_valid = true;
    ue::UObject* t = g_target.resolve();
    if (!t) { log::write("nav: stopped, the target is gone"); stop(true); return; }
    FVector goal = world::actor_location(t);
    // A teleport next to the target was the target (stairs); one on the way (the path crossed another exit, the
    // back alley's, 2026-10-27) only moved the hero: plan again from there.
    if (jumped) {
      if (flat(goal, me) < 800.0f || std::fabs(goal.z - me.z) > 300.0f) { stop(false); arrived_at(); return; }
      refresh_path(me);
      g_progress_at = now; g_progress_pos = me;
    }
    // A hit actor is usable from inside its trigger box, which can reach well past its centre point (the bed's box
    // is 1.3 x 1.4 m): arrived = inside the box; anything else = close to the location.
    Box box;
    bool has_box = trigger_box(t, box);
    bool arrived = has_box ? inside(box, me, -5.0f) : flat(goal, me) < kArrive;
    if (arrived) { stop(false); face(t, me); arrived_at(); return; }
    if (now - g_path_at > 700) refresh_path(me);
    FVector wp;
    if (!next_waypoint(me, wp)) { log::write("nav: stopped, no path"); stop(true); return; }
    // The navmesh path ends where the floor ends (beside the bed); from its last point head straight for the box.
    if (g_path.size() == 1 && flat(wp, me) < kWaypointReached + 30.0f && has_box) wp = box.center;
    // A detour around something in the way (below): first its waypoint, then on to the target.
    if (g_detour_on) {
      if (flat(g_detour, me) < 80.0f) { g_detour_on = false; g_direct = true; }
      else wp = g_detour;
    }
    if (g_direct && !g_detour_on) wp = has_box ? box.center : goal;
    // 8-way chord toward the waypoint, camera-relative (the game's own movement is camera-relative too).
    float b = world::bearing(me, wp);
    int sector = ((int)std::lround(b / 45.0f) % 8 + 8) % 8;
    static const uint32_t chords[8][2] = {{'W', 0}, {'W', 'D'}, {'D', 0}, {'S', 'D'}, {'S', 0}, {'S', 'A'}, {'A', 0}, {'W', 'A'}};
    hold(chords[sector][0], chords[sector][1]);
    if (now - g_progress_at > 1500) {
      if (flat(me, g_progress_pos) < 40.0f) {
        // Blocked by the thing itself (a chest's collision) while its prompt is already up: that is arriving.
        bool there = poi::prompt() == g_name;
        // Blocked close to it (the teleporter pad's pylons, 2026-10-27): circle round and come in from another
        // side.
        // Six tries, a sixth of a turn apart: the entrance terminal opens only to one side (2026-10-28).
        if (!there && g_detours < 6 && flat(goal, me) < 600.0f) {
          if (g_detours == 0) g_detour_base = std::atan2(me.y - goal.y, me.x - goal.x);
          ++g_detours;
          float a = g_detour_base + g_detours * 3.14159265f / 3.0f;
          g_detour = FVector{goal.x + 330.0f * std::cos(a), goal.y + 330.0f * std::sin(a), me.z};
          g_detour_on = true;
          g_progress_at = now; g_progress_pos = me;
          log::writef("nav: blocked at ({:.0f}, {:.0f}), detour {} via ({:.0f}, {:.0f})", me.x, me.y, g_detours, g_detour.x, g_detour.y);
          return;
        }
        stop(false);
        if (there) arrived_at(); else speech::speak(strings::kStuck, true);
        return;
      }
      g_progress_at = now; g_progress_pos = me;
    }
  }
  if (settings::on(settings::Beacon) && in_field && now - g_beacon_at > 600) {
    g_beacon_at = now;
    poi::Poi p;
    if (poi::selected(p)) {
      if (now - g_path_at > 1000) { g_target = p.obj; refresh_path(me); }
      FVector wp;
      if (next_waypoint(me, wp)) {
        float b = world::bearing(me, wp) * 3.14159265f / 180.0f;
        bool behind = std::cos(b) < -0.2f && settings::on(settings::BehindPitch);
        float dist = flat(p.loc, me);
        float vol = 0.6f + 0.7f * std::max(0.0f, 1.0f - dist / 3000.0f);
        audio::Spot at = world::spot(me, wp);   // from the next waypoint, as far away as the target (air absorption)
        at.dist_m = dist / 100.0f;
        audio::signal((int)core::SignalCue::Beacon, vol, at, behind ? 0.67f : 1.0f);   // behind: a fifth lower
      }
    }
  }
}

std::string status() {
  return std::format("nav walking={} beacon={} target='{}' path={} held={:c}{:c}\n", g_walking, settings::on(settings::Beacon), g_name, g_path.size(),
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
    return status();
  }, "?walk=objindex | ?stop=1 the auto-walk");
}
}  // namespace p3a::nav

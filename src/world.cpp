#include "world.h"
#include "devserver.h"
#include "hooks.h"
#include "input.h"
#include "msg.h"
#include "seh.h"
#include "screens/screen.h"
#include "screens/util.h"
#include "watch.h"
#include <cmath>
#include <format>
#include <memory>
#include <algorithm>

namespace p3a::world {
using namespace ue;

static WeakObj g_player;
static uint64_t g_frame = 0;   // world::tick's count: what is worked out once per frame is keyed by it
static float g_cam_yaw = 0;
static WeakObj g_cam_mgr;
// The top-down mode (topdown.cpp) sets north: while it holds, every sound and bearing is heard facing it instead of
// the camera's way.
static bool g_north_on = false;
static float g_north_yaw = -90.0f;
static float listener_yaw() { return g_north_on ? g_north_yaw : g_cam_yaw; }
void set_north(bool on, float yaw) { g_north_on = on; g_north_yaw = yaw; }
bool north_up() { return g_north_on; }
float camera_yaw() { return g_cam_yaw; }

// The field player: one BP_FldPlayer_C per mode (DAILY, DUNGEON, ENCOUNT) exists at once; the inactive ones are
// parked far below the map (z = -3200 measured at Iwatodai Station, 2026-10-27).
static UObject* find_player() {
  static const int players = watch::want("BP_FldPlayer_C", true);
  for (UObject* p : watch::all(players)) {
    Params loc(find_function(class_of(p), "K2_GetActorLocation"));
    call(p, loc);
    FVector v = loc.ret<FVector>();
    if (v.z > -2500.0f && !get_bool(p, "bHidden")) return p;
  }
  return nullptr;
}

UObject* player() {
  if (UObject* p = g_player.resolve()) return p;
  // None yet (the title, a load): looked for once a frame, among the few hero actors the watcher holds.
  static uint64_t looked = ~0ull;
  if (looked == g_frame) return nullptr;
  looked = g_frame;
  UObject* p = find_player();
  g_player = WeakObj::of(p);
  return p;
}

static bool scene_flag(const char* fn_name) {
  static const int sub = watch::want("AtlEvtSubsystem");
  return call_ret<bool>(watch::get(sub), fn_name);
}
bool scene_playing() { return scene_flag("IsPlayingEvent"); }
bool scene_fast() { return scene_flag("IsHighSpeed"); }

// A battle keeps the field's hero and, between turns, shows no screen: only its own component says it is on
// (BtlCoreComponent, as the auto-walk checks). The wall tones went on through battles and
// story scenes until every field sound asked here (the user, 2026-11-10).
static bool exploring_now() {
  static const int battle = watch::want("BtlCoreComponent");
  if (!player() || screens::current() || msg::current().blocking || watch::get(battle) || input::modal()) return false;
  if (scene_playing()) return false;
  return !(ue::known_build() && screens::faded_out());
}
bool exploring() {
  static uint64_t at = ~0ull;
  static bool value = false;
  if (at != g_frame) { at = g_frame; value = exploring_now(); }
  return value;
}

bool player_location(FVector& out) {
  UObject* p = player();
  if (!p) return false;
  Params loc(find_function(class_of(p), "K2_GetActorLocation"));
  call(p, loc);
  out = loc.ret<FVector>();
  return true;
}

bool player_velocity(FVector& out) {
  UObject* p = player();
  if (!p) return false;
  Params v(find_function(class_of(p), "GetVelocity"));
  if (!call(p, v)) return false;
  out = v.ret<FVector>();
  return true;
}

void tick() {
  ++g_frame;
  UObject* cm = g_cam_mgr.resolve();
  if (!cm) { cm = get<UObject*>(player_controller(), "PlayerCameraManager"); g_cam_mgr = WeakObj::of(cm); }
  if (cm) {
    Params r(find_function(class_of(cm), "GetCameraRotation"));
    call(cm, r);
    g_cam_yaw = r.ret<FRotator>().yaw;
  }
  // The sounds that are playing stay at their places while the hero walks and the camera turns.
  if (FVector me; player_location(me)) audio::follow([&](float x, float y, float z) { return spot(me, FVector{x, y, z}); });
  // A mode change (daily -> dark hour -> dungeon) swaps the active player actor: the one held is let go the frame it
  // is parked under the map, and player() picks the active one.
  if (g_player.resolve()) {
    FVector v;
    if (player_location(v) && v.z < -2500.0f) g_player = {};
  }
}


FVector dir_vector(int dir) {
  float yaw = (listener_yaw() + 90.0f * dir) * 3.14159265f / 180.0f;
  return FVector{std::cos(yaw), std::sin(yaw), 0.0f};
}

// ---- traces: UKismetSystemLibrary through ProcessEvent (layouts from reflection) ----
// The function, its class object and its parameter layout are looked up afresh on the first trace of every frame
// (world::tick's count): nothing of the engine's is kept from one frame to the next, and a lookup that failed is
// simply made again.
struct TraceFn {
  const char* name;
  uint64_t frame = ~0ull;   // the frame the rest was read in
  UFunction* fn = nullptr;
  UObject* cdo = nullptr;
  int32_t o_world = -1, o_start = -1, o_end = -1, o_profile = -1, o_ignore = -1, o_hit = -1, o_ignore_self = -1, o_ret = -1, o_radius = -1, o_half = -1;
  int32_t h_distance = -1, h_point = -1, h_actor = -1, h_location = -1, h_normal = -1;
  FField* h_blocking = nullptr; FField* h_penetrating = nullptr;
  bool ok = false;
  explicit TraceFn(const char* fn_name) : name(fn_name) {}
  void refresh() {
    if (frame == g_frame) return;
    frame = g_frame;
    ok = false;
    UClass* k = find_class("KismetSystemLibrary");
    fn = find_function(k, name);
    cdo = cdo_of(k);
    if (!fn || !cdo) return;
    o_world = offset_of(fn, "WorldContextObject"); o_start = offset_of(fn, "Start"); o_end = offset_of(fn, "End");
    o_profile = offset_of(fn, "ProfileName"); o_ignore = offset_of(fn, "ActorsToIgnore"); o_hit = offset_of(fn, "OutHit");
    o_ignore_self = offset_of(fn, "bIgnoreSelf"); o_ret = offset_of(fn, "ReturnValue");
    o_radius = offset_of(fn, "Radius"); o_half = offset_of(fn, "HalfHeight");
    UStruct* hs = prop_inner_object(find_prop(fn, "OutHit"));
    h_distance = offset_of(hs, "Distance"); h_point = offset_of(hs, "ImpactPoint"); h_actor = offset_of(hs, "Actor"); h_location = offset_of(hs, "Location"); h_normal = offset_of(hs, "ImpactNormal");
    h_blocking = find_prop(hs, "bBlockingHit"); h_penetrating = find_prop(hs, "bStartPenetrating");
    ok = h_distance >= 0 && o_hit >= 0;
  }
};

static bool is_person(UObject* a) {
  if (!a) return false;
  // FldCrowdNpcBase: the background students and passers-by (BP_MCIdle*), plain actors with a capsule. One standing
  // by classroom 2-F's front door stopped the sightline over the desks, so a student in the room was taken for one
  // behind a closed door (2026-11-12).
  return is_a(a, find_class("FldHitCharacter")) || is_a(a, find_class("Pawn")) || is_a(a, find_class("FldCrowdNpcBase"));
}

static Hit run_trace(TraceFn& t, const FVector& from, const FVector& to, const char* profile, float radius, float half,
                     std::vector<UObject*>* people = nullptr) {
  Hit h;
  t.refresh();
  if (!t.ok) return h;
  UObject* p = player();
  std::vector<uint8_t> buf(params_size(t.fn), 0);
  uint8_t* b = buf.data();
  static int32_t name_idx = -1; static std::string name_for;
  if (name_for != profile) { name_idx = fname_index(profile); name_for = profile; }
  std::vector<UObject*> ignore;
  if (p) ignore.push_back(p);
  if (people) ignore.insert(ignore.end(), people->begin(), people->end());
  for (int attempt = 0;; ++attempt) {
    std::fill(buf.begin(), buf.end(), uint8_t{0});
    *(UObject**)(b + t.o_world) = p ? p : gworld();
    *(FVector*)(b + t.o_start) = from;
    *(FVector*)(b + t.o_end) = to;
    *(FName*)(b + t.o_profile) = FName{name_idx, 0};
    if (!ignore.empty()) *(TArray<UObject*>*)(b + t.o_ignore) = TArray<UObject*>{ignore.data(), (int32_t)ignore.size(), (int32_t)ignore.size()};
    b[t.o_ignore_self] = 1;
    if (t.o_radius >= 0) *(float*)(b + t.o_radius) = radius;
    if (t.o_half >= 0) *(float*)(b + t.o_half) = half;
    process_event(t.cdo, t.fn, b);
    // The engine must not free our array: clear it before the buffer goes away.
    *(TArray<UObject*>*)(b + t.o_ignore) = TArray<UObject*>{nullptr, 0, 0};
    if (!people || attempt >= 8 || !b[t.o_ret]) break;
    UObject* a = object_at(*(int32_t*)(b + t.o_hit + t.h_actor));
    if (!is_person(a)) break;
    people->push_back(a);   // a person: look past them
    ignore.push_back(a);
  }
  uint8_t* hit = b + t.o_hit;
  h.hit = b[t.o_ret] != 0;
  h.distance = *(float*)(hit + t.h_distance);
  h.point = *(FVector*)(hit + t.h_point);
  if (t.h_location >= 0) h.location = *(FVector*)(hit + t.h_location);
  if (t.h_normal >= 0) h.normal = *(FVector*)(hit + t.h_normal);
  h.start_inside = t.h_penetrating && prop_bool(t.h_penetrating, hit);
  if (h.hit) { int32_t idx = *(int32_t*)(hit + t.h_actor); h.actor = object_at(idx); }
  return h;
}

Hit sweep(const FVector& from, const FVector& to, float radius, float half_height, std::vector<UObject*>* people) {
  static TraceFn t("CapsuleTraceSingleByProfile");
  return run_trace(t, from, to, "Pawn", radius, half_height, people);
}
Hit line(const FVector& from, const FVector& to, const char* profile, std::vector<UObject*>* people) {
  static TraceFn t("LineTraceSingleByProfile");
  return run_trace(t, from, to, profile, 0, 0, people);
}

std::string status() {
  FVector p{};
  bool have = player_location(p);
  return std::format("player={} at ({:.0f}, {:.0f}, {:.0f}) camera yaw {:.1f}", have ? name_of(player()) : "none", p.x, p.y, p.z, g_cam_yaw) + (g_north_on ? std::format(" north {:.0f}", g_north_yaw) : std::string());
}

// The field streams whole districts at once: an actor counts only if its level is the persistent level or a streamed
// level that is visible right now (hidden sublevels keep their actors, parked at the origin; measured 2026-10-27).
// The levels are kept by index and name, not by pointer: a level that has gone matches nothing.
static std::vector<WeakObj> g_visible_levels;
static uint64_t g_levels_stamp = 0;
uint64_t levels_stamp() { return g_levels_stamp; }
static void refresh_levels() {
  // Once a frame: each streamed level's own "is visible" bit (what ULevelStreaming::IsLevelVisible answers from).
  static uint64_t at = ~0ull;
  if (at == g_frame) return;
  at = g_frame;
  // The list as it was: a change of it is counted (levels_stamp) when this one is complete.
  struct Count { std::vector<WeakObj> was; ~Count() { if (!(was == g_visible_levels)) ++g_levels_stamp; } } count{g_visible_levels};
  g_visible_levels.clear();
  UObject* w = gworld();
  if (!w) return;
  if (UObject* pl = get<UObject*>(w, "PersistentLevel")) g_visible_levels.push_back(WeakObj::of(pl));
  auto* levels = field<TArray<UObject*>>(w, "StreamingLevels");
  if (!levels) return;
  // A district streams a few hundred levels: the two properties are looked up once a frame (every streaming class
  // has LoadedLevel from ULevelStreaming, every level bIsVisible from ULevel) and each level is two reads.
  int32_t loaded = -1;
  FField* visible = nullptr;
  for (int i = 0; i < levels->num; ++i) {
    UObject* ls = levels->data[i];
    if (!ls) continue;
    if (loaded < 0 && (loaded = offset_of(class_of(ls), "LoadedLevel")) < 0) return;
    UObject* lvl = *(UObject**)((uint8_t*)ls + loaded);
    if (!lvl) continue;
    if (!visible && !(visible = find_prop(class_of(lvl), "bIsVisible"))) return;
    if (prop_bool(visible, lvl)) g_visible_levels.push_back(WeakObj::of(lvl));
  }
}
bool actor_visible_level(UObject* a) {
  refresh_levels();
  UObject* lvl = outer_of(a);
  if (!lvl) return false;
  WeakObj l = WeakObj::of(lvl);
  for (const WeakObj& v : g_visible_levels) if (v == l) return true;
  return false;
}
std::vector<std::string> visible_levels() {
  refresh_levels();
  std::vector<std::string> out;
  for (const WeakObj& v : g_visible_levels)
    if (UObject* lvl = v.resolve())
      if (UObject* w = outer_of(lvl)) out.push_back(name_of(w));   // a ULevel's outer is its UWorld
  return out;
}

FVector actor_location(UObject* a) {
  Params p(find_function(find_class("Actor"), "K2_GetActorLocation"));
  call(a, p);
  return p.ret<FVector>();
}

float yaw_of(UObject* actor) {
  Params p(find_function(class_of(actor), "K2_GetActorRotation"));
  call(actor, p);
  return p.ret<FRotator>().yaw;
}

bool read_box(UObject* box, core::Box& b) {
  if (!box) return false;
  auto vcall = [&](const char* fn) { Params p(find_function(class_of(box), fn)); call(box, p); return p; };
  b.center = vec3(vcall("K2_GetComponentLocation").ret<FVector>());
  b.extent = vec3(vcall("GetScaledBoxExtent").ret<FVector>());
  b.yaw = vcall("K2_GetComponentRotation").ret<FRotator>().yaw;
  return b.extent.x > 1.0f && b.extent.y > 1.0f;
}

bool capsule_of(UObject* actor, float& radius, float& half_height) {
  UObject* cap = actor ? get<UObject*>(actor, "CapsuleComponent") : nullptr;
  if (!cap) return false;
  Params r(find_function(class_of(cap), "GetScaledCapsuleRadius")), h(find_function(class_of(cap), "GetScaledCapsuleHalfHeight"));
  if (!call(cap, r) || !call(cap, h)) return false;
  radius = r.ret<float>(); half_height = h.ret<float>();
  return true;
}

// Bearing of `to` from `from` in degrees, from the listener's facing (north in the top-down mode, else the camera's):
// 0 ahead, 90 right, 180 behind, -90 left.
float bearing(const FVector& from, const FVector& to) { return core::bearing(vec3(from), vec3(to), listener_yaw()); }
float camera_bearing(const FVector& from, const FVector& to) { return core::bearing(vec3(from), vec3(to), g_cam_yaw); }

// The hero's location is the capsule's middle and most targets (navmesh points, hit boxes) sit near the floor: see
// core::listener_spot for how height is counted.
audio::Spot spot(const FVector& from, const FVector& to) {
  core::ListenerSpot l = core::listener_spot(vec3(from), vec3(to), listener_yaw());
  audio::Spot s;
  s.right = l.right;
  s.ahead = l.ahead;
  s.up = l.up;
  s.dist_m = l.dist_m;
  s.pan = l.pan;
  s.placed = true; s.wx = to.x; s.wy = to.y; s.wz = to.z;   // audio::follow keeps it there while it plays
  return s;
}

// Navmesh path through UNavigationSystemV1::FindPathToLocationSynchronously (static, on the CDO). Empty = no path.
std::vector<FVector> find_path(const FVector& from, const FVector& to, bool* partial) {
  std::vector<FVector> pts;
  UClass* nav = find_class("NavigationSystemV1");
  UFunction* fn = find_function(nav, "FindPathToLocationSynchronously");
  if (!fn) return pts;
  Params p(fn);
  p.set<UObject*>("WorldContextObject", player() ? player() : gworld());
  p.set<FVector>("PathStart", from);
  p.set<FVector>("PathEnd", to);
  call(cdo_of(nav), p);
  UObject* path = p.ret<UObject*>();
  if (!path) return pts;
  auto* arr = field<TArray<FVector>>(path, "PathPoints");
  if (arr) for (int i = 0; i < arr->num; ++i) pts.push_back(arr->data[i]);
  if (partial) {
    Params q(find_function(class_of(path), "IsPartial")); call(path, q); *partial = q.ret<bool>();
  }
  return pts;
}

// The nearest navmesh point to `p` within `extent` cm (NavigationSystemV1::K2_ProjectPointToNavigation).
bool project_to_nav(const FVector& p, float extent, FVector& out) {
  UClass* nav = find_class("NavigationSystemV1");
  UFunction* fn = find_function(nav, "K2_ProjectPointToNavigation");
  if (!fn) return false;
  Params q(fn);
  q.set<UObject*>("WorldContextObject", player() ? player() : gworld());
  q.set<FVector>("Point", p);
  q.set<FVector>("QueryExtent", FVector{extent, extent, extent});
  call(cdo_of(nav), q);
  if (!q.ret<bool>()) return false;
  out = *q.at<FVector>("ProjectedLocation");
  return true;
}

void register_routes() {
  dev::route("/path", [](const dev::Request& r, int&) {
    int obj = (int)dev::arg_int(r, "obj", -1);
    auto out = std::make_shared<std::string>("timeout\n");
    hooks::run_on_game_thread([out, obj] {
      FVector p;
      UObject* t = object_at(obj);
      if (!player_location(p) || !t) { *out = "no player or target\n"; return; }
      FVector to = actor_location(t);
      bool partial = false;
      auto pts = find_path(p, to, &partial);
      std::string s = std::format("{} points{} to {} ({:.0f} cm straight)\n", pts.size(), partial ? " (PARTIAL)" : "", name_of(t),
                                  std::sqrt((to.x - p.x) * (to.x - p.x) + (to.y - p.y) * (to.y - p.y)));
      for (auto& v : pts) s += std::format("  ({:.0f}, {:.0f}, {:.0f}) bearing {:.0f}\n", v.x, v.y, v.z, bearing(p, v));
      *out = s;
    });
    return *out;
  }, "?obj=N navmesh path from the player to an actor");
  dev::route("/tp", [](const dev::Request& r, int&) {
    // /tp?obj=N beside an actor (150 cm short of it, on its level) | /tp?x=&y=&z= to a place
    int obj = (int)dev::arg_int(r, "obj", -1);
    FVector to{(float)dev::arg_num(r, "x", 0), (float)dev::arg_num(r, "y", 0), (float)dev::arg_num(r, "z", 0)};
    return dev::on_game([=]() mutable {
      UObject* hero = player();
      FVector me;
      if (!hero || !player_location(me)) return std::string("no hero\n");
      if (obj >= 0) {
        UObject* a = ue::object_at(obj);
        if (!a) return std::string("no object\n");
        to = actor_location(a);
        float dx = me.x - to.x, dy = me.y - to.y, d = std::hypot(dx, dy);
        if (d > 1) { to.x += dx / d * 150.0f; to.y += dy / d * 150.0f; }
        to.z += 100.0f;
      }
      UFunction* f = ue::find_function(ue::class_of(hero), "K2_TeleportTo");
      if (!f) return std::string("no K2_TeleportTo\n");
      ue::Params p(f);
      p.set<FVector>("DestLocation", to);
      ue::call(hero, p);
      return std::format("teleported to ({:.0f}, {:.0f}, {:.0f}): {}\n", to.x, to.y, to.z, p.ret<bool>());
    });
  }, "?obj=N | ?x=&y=&z= teleport the hero (dev)");
  dev::route("/near", [](const dev::Request& r, int&) {
    // /near?class=FldHitCore&r=3000 actors of a class within r cm of the player, nearest first
    std::string cls = dev::arg(r, "class", "FldHitCore");
    float radius = (float)dev::arg_num(r, "r", 3000);
    auto out = std::make_shared<std::string>("timeout\n");
    hooks::run_on_game_thread([out, cls, radius] {
      FVector p;
      if (!player_location(p)) { *out = "no player\n"; return; }
      std::vector<std::pair<float, std::string>> rows;
      for (UObject* a : find_all(cls)) {
        if (!actor_visible_level(a)) continue;
        FVector v{};
        if (!guarded([&] { v = actor_location(a); })) continue;
        float d = std::sqrt((v.x - p.x) * (v.x - p.x) + (v.y - p.y) * (v.y - p.y) + (v.z - p.z) * (v.z - p.z));
        if (d > radius) continue;
        rows.emplace_back(d, std::format("{:6.0f} cm {:5.0f} deg dz {:5.0f}  #{} {} ({})", d, bearing(p, v), v.z - p.z, index_of(a), name_of(a), name_of(class_of(a))));
      }
      std::sort(rows.begin(), rows.end());
      std::string s;
      for (auto& [d, line] : rows) s += line + "\n";
      *out = s.empty() ? "none\n" : s;
    });
    return *out;
  }, "?class=FldHitCore&r=cm actors near the player");
  dev::route("/world", [](const dev::Request& r, int&) {
    auto out = std::make_shared<std::string>("timeout\n");
    float range = (float)dev::arg_num(r, "range", 600);
    bool skip = dev::arg(r, "people") == "0";
    hooks::run_on_game_thread([out, range, skip] {
      std::string s = status() + "\n";
      FVector p;
      if (player_location(p)) {
        static const char* names[4] = {"ahead", "right", "behind", "left"};
        for (int d = 0; d < 4; ++d) {
          FVector dv = dir_vector(d);
          FVector to{p.x + dv.x * range, p.y + dv.y * range, p.z};
          std::vector<UObject*> people;
          Hit h = sweep(FVector{p.x, p.y, p.z + 10}, FVector{to.x, to.y, to.z + 10}, 20, 40, skip ? &people : nullptr);
          s += std::format("  {}: {} {:.0f} {}{}\n", names[d], h.hit ? "blocked" : "clear", h.hit ? h.distance : range, h.actor ? name_of(h.actor) : "",
                           h.start_inside ? " (start inside)" : "");
          for (UObject* a : people) s += std::format("    past {}\n", name_of(a));
        }
      }
      *out = s;
    });
    return *out;
  }, "?range=cm&people=0 (look past people, as wall tones do) player, camera yaw and what blocks each direction");
}
}  // namespace p3a::world

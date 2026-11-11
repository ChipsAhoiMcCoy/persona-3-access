#include "doors.h"
#include "core/doorway.h"
#include "devserver.h"
#include "hooks.h"
#include "log.h"
#include "seh.h"
#include "watch.h"
#include "world.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <map>
#include <memory>
#include <string>
#include <vector>

// AFldDoorObjCore (BP_F101_103_DoorA_C, BP_F101_PassDoor_C, every room and shop door): a skeletal door with its own
// blocking boxes. Most hang in a wall beside the navmesh and no path meets them; a few stand across it (the gym
// hallway's is a 5 m x 20 cm box, WorldStatic, at (0, -2652)), shut until the game's own exit script plays them open
// (PlayDoorOpen; IsPlayingOpen while it does) and walks the hero through. The navmesh runs on underneath.
namespace p3a::doors {
using ue::UObject;
using ue::WeakObj;
using world::FVector;

struct Door { WeakObj obj; FVector lo{}, hi{}; };   // the bounds of its colliding parts
static std::vector<Door> g_doors;                   // the visible levels' doors
static std::vector<WeakObj::Key> g_keys;
// Counts every time the hero may be in another part of the map than before: answers are kept per count.
static uint64_t g_side = 1;
uint64_t side_stamp() { return g_side; }

// The doors come and go with their levels: read again when the set of visible levels changes (every door of every
// loaded level each frame cost 10 us, 2026-11-11), and while a visible door has no bounds yet (a level just shown).
static void refresh() {
  static const int id = watch::want("FldDoorObjCore", true);
  static uint64_t levels = ~0ull;
  static bool unready = false;
  uint64_t stamp = world::levels_stamp();
  if (stamp == levels && !unready) return;
  levels = stamp;
  unready = false;
  std::vector<Door> doors;
  std::vector<WeakObj::Key> keys;
  for (UObject* d : watch::all(id)) {
    if (!world::actor_visible_level(d)) continue;
    ue::Params p(ue::find_function(ue::class_of(d), "GetActorBounds"));
    if (!p.fn || !p.set<uint8_t>("bOnlyCollidingComponents", 1)) continue;
    FVector* o = p.at<FVector>("Origin");
    FVector* e = p.at<FVector>("BoxExtent");
    if (!o || !e || !guarded([&] { ue::call(d, p); })) continue;
    if (e->x <= 0 && e->y <= 0) { unready = true; continue; }   // nothing of it collides (yet)
    doors.push_back(Door{WeakObj::of(d), FVector{o->x - e->x, o->y - e->y, o->z - e->z}, FVector{o->x + e->x, o->y + e->y, o->z + e->z}});
    keys.push_back(doors.back().obj.key());
  }
  if (keys == g_keys) return;
  g_keys = std::move(keys);
  g_doors = std::move(doors);
  ++g_side;
  log::writef("doors: {} on the visible levels", g_doors.size());
}

static core::Rect rect(const Door& d) { return core::Rect{d.lo.x, d.lo.y, d.hi.x, d.hi.y}; }

void tick() {
  static WeakObj hero_seen;
  static FVector at{};
  static uint64_t levels = 0;
  UObject* hero = world::player();
  FVector me;
  if (!hero || !world::player_location(me)) return;   // hidden behind a menu: he comes back where he was, or jumps
  world::actor_visible_level(hero);   // keeps the list of visible levels current
  refresh();
  if (uint64_t l = world::levels_stamp(); l != levels) { levels = l; ++g_side; }   // a level came or went, its navmesh with it
  WeakObj h = WeakObj::of(hero);
  float step = std::fabs(me.x - at.x) + std::fabs(me.y - at.y) + std::fabs(me.z - at.z);
  if (!(h == hero_seen) || step > 400.0f) ++g_side;   // a new field's hero; stairs and exits move him in one frame
  else if (step > 1.0f)
    // The game walks him through an open door: the step that passes its bounds is the change of sides.
    for (const Door& d : g_doors) {
      core::Vec2 in, out;
      if (me.z < d.lo.z - 150.0f || me.z > d.hi.z + 150.0f) continue;
      if (core::clip(core::Vec2{at.x, at.y}, core::Vec2{me.x, me.y}, rect(d), in, out)) { ++g_side; break; }
    }
  hero_seen = h;
  at = me;
}

// The path's leg a -> b (navmesh points, at the floor) is shut by this door.
static bool shuts(const Door& d, const FVector& a, const FVector& b, std::vector<UObject*>& people) {
  if (std::max(a.z, b.z) + 250.0f < d.lo.z || std::min(a.z, b.z) - 100.0f > d.hi.z) return false;   // another storey's
  core::Vec2 in, out;
  if (!core::clip(core::Vec2{a.x, a.y}, core::Vec2{b.x, b.y}, rect(d), in, out)) return false;
  UObject* door = d.obj.resolve();
  if (!door) return false;
  // Playing open (the hero is being walked through it): it is a shut door again when that is over.
  if (ue::call_ret<bool>(door, "IsPlayingOpen")) return true;
  // The bounds are a box round the whole door, wider than a door that stands at an angle: the door itself is asked,
  // with a line at chest height along the leg's piece inside them. Whatever stops it there shuts the way.
  float dx = b.x - a.x, dy = b.y - a.y, len = std::hypot(dx, dy);
  if (len < 1.0f) return false;
  dx /= len; dy /= len;
  float z = std::clamp((a.z + b.z) * 0.5f + 90.0f, d.lo.z + 30.0f, std::max(d.lo.z + 30.0f, d.hi.z - 30.0f));
  world::Hit hit = world::line(FVector{in.x - dx * 5.0f, in.y - dy * 5.0f, z}, FVector{out.x + dx * 5.0f, out.y + dy * 5.0f, z}, "Pawn", &people);
  return hit.hit;
}

static bool work_out(const FVector& me, const FVector& at, std::string* report = nullptr) {
  bool partial = false;
  auto path = world::find_path(me, at, &partial);
  // A thing standing off the navmesh: the path to the navmesh beside it, as the auto-walk takes it.
  FVector beside{};
  if (path.empty() && world::project_to_nav(at, 300.0f, beside)) path = world::find_path(me, beside, &partial);
  if (report) *report += std::format("path: {} point(s){}\n", path.size(), partial ? " (partial)" : "");
  std::vector<UObject*> people;   // somebody standing in a doorway does not shut it
  // A thing at the door itself (the exit that opens it, a hit on the door) is not behind it: its path ends in the
  // door's bounds, a body's width allowed.
  auto at_door = [&](const Door& d) {
    return at.x > d.lo.x - 30.0f && at.x < d.hi.x + 30.0f && at.y > d.lo.y - 30.0f && at.y < d.hi.y + 30.0f && at.z > d.lo.z - 150.0f && at.z < d.hi.z + 150.0f;
  };
  for (size_t i = 1; i < path.size(); ++i)
    for (const Door& d : g_doors)
      if (!at_door(d) && shuts(d, path[i - 1], path[i], people)) {
        if (report) *report += std::format("shut by {} on the leg ({:.0f}, {:.0f}) -> ({:.0f}, {:.0f})\n", ue::name_of(d.obj.resolve()), path[i - 1].x, path[i - 1].y, path[i].x, path[i].y);
        return true;
      }
  return false;
}

bool behind(UObject* a, const FVector& me, const FVector& at) {
  if (g_doors.empty() || !a) return false;
  struct Answer { FVector at; bool behind; };
  static std::map<WeakObj::Key, Answer> answers;
  static uint64_t side = 0;
  if (side != g_side) { side = g_side; answers.clear(); }
  auto key = WeakObj::of(a).key();
  auto it = answers.find(key);
  // Somebody who has walked on is asked about again.
  if (it != answers.end() && std::fabs(it->second.at.x - at.x) + std::fabs(it->second.at.y - at.y) + std::fabs(it->second.at.z - at.z) < 100.0f)
    return it->second.behind;
  bool b = false;
  guarded([&] { b = work_out(me, at); });
  answers[key] = Answer{at, b};
  return b;
}

void install() {
  dev::route("/doors", [](const dev::Request& r, int&) {
    int obj = (int)dev::arg_int(r, "obj", -1);
    auto out = std::make_shared<std::string>("timeout\n");
    hooks::run_on_game_thread([out, obj] {
      std::string s = std::format("{} door(s) on the visible levels, side count {}\n", g_doors.size(), g_side);
      for (const Door& d : g_doors) {
        UObject* o = d.obj.resolve();
        s += std::format("  #{} {} ({:.0f}, {:.0f}, {:.0f}) .. ({:.0f}, {:.0f}, {:.0f}){}\n", d.obj.index, o ? ue::name_of(o) : "gone", d.lo.x, d.lo.y, d.lo.z,
                         d.hi.x, d.hi.y, d.hi.z, o && ue::call_ret<bool>(o, "IsPlayingOpen") ? " playing open" : "");
      }
      FVector me;
      if (UObject* t = obj >= 0 ? ue::object_at(obj) : nullptr; t && world::player_location(me)) {
        std::string why;
        bool b = work_out(me, world::actor_location(t), &why);
        s += std::format("{}: {}\n{}", ue::name_of(t), b ? "behind a closed door" : "not behind a door", why);
      }
      *out = s;
    });
    return *out;
  }, "?obj=N the visible levels' doors; whether the path to an actor goes through a closed one");
}
}  // namespace p3a::doors

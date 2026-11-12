// The points-of-interest list the review keys walk: its categories, the selection, and what each key says.
// Categories (left and right) each hold their points of interest nearest first (up and down). Changing category, or
// opening the controller's mod layer, selects the nearest; an empty category is skipped.
// Read here: the selected actor's own place (K2_GetActorLocation) and bHidden; everything else comes from poi::scan.
#include "poi_internal.h"
#include "core/poi_text.h"
#include "core/strings.h"
#include "core/units.h"
#include "doors.h"
#include "seh.h"
#include "sonar.h"
#include "speech.h"
#include "world.h"
#include <cmath>
#include <format>

namespace p3a::poi {
using namespace ue;

// Top-down, the bearing is from north and said by the compass; else by the clock face, from the camera.
std::string clock(float b) {
  if (world::north_up()) return strings::kCompass[core::compass_point(b)];
  return std::format("{} {}", core::clock_hour(b), strings::kOClock);
}

std::string describe(const Poi& p) {
  // The name stands alone, a way into another area's too: it is where it leads, and its category says what it is
  // ("Gekkoukan High School exit" was taken for the way out of the school; the user's call, 2026-11-09).
  std::string who = p.state.empty() ? p.name : p.name + ", " + p.state;
  return std::format("{}, {}, {}", who, core::meters(p.dist / 100.0f), clock(p.bearing));
}

enum class Cat { Exits, Doors, People, Shops, Things, Events, Shadows, Count };
static const char* cat_name(Cat c) {
  switch (c) {
    case Cat::Exits: return strings::kCatExits;
    case Cat::Doors: return strings::kCatDoors;
    case Cat::People: return strings::kCatPeople;
    case Cat::Shops: return strings::kCatShops;
    case Cat::Things: return strings::kCatThings;
    case Cat::Events: return strings::kCatEvents;
    default: return strings::kCatShadows;
  }
}
static Cat g_cat = Cat::Exits;
static WeakObj g_sel;
static Poi g_sel_poi;   // what the list said about it: selected() refreshes the place

static bool in_cat(const Poi& p, Cat c) {
  switch (c) {
    case Cat::Exits: return p.kind == Kind::Exit;
    case Cat::Doors: return p.kind == Kind::Door;
    case Cat::People: return p.kind == Kind::Person;
    case Cat::Shops: return p.kind == Kind::Shop;
    case Cat::Things: return p.kind == Kind::Thing || p.kind == Kind::Fragment;   // a fragment is an object with its own sound
    case Cat::Events: return p.kind == Kind::Event;
    default: return p.kind == Kind::Enemy;
  }
}

bool in_selected_category(const Poi& p) { return in_cat(p, g_cat); }

static std::vector<Poi> in_category(const std::vector<Poi>& all, Cat c) {
  std::vector<Poi> v;
  for (auto& p : all) if (in_cat(p, c)) v.push_back(p);
  return v;   // scan() sorts nearest first
}

// The current category, or the next one in `delta`'s direction that has something (staying put when it does).
static std::vector<Poi> settle(const std::vector<Poi>& all, int delta, bool move) {
  int c = core::settle_category((int)g_cat, (int)Cat::Count, delta, move, [&](int cat) {
    for (auto& p : all) if (in_cat(p, (Cat)cat)) return true;
    return false;
  });
  if (c < 0) return {};
  g_cat = (Cat)c;
  return in_category(all, g_cat);
}

// "Exits: Iwatodai Station, 6 meters, 2 o'clock, 1 of 3", the nearest selected.
static void announce_nearest(const std::vector<Poi>& list) {
  if (list.empty()) { speech::speak(strings::kNothingNearbyFull, true); g_sel = {}; return; }
  g_sel = list[0].obj;
  g_sel_poi = list[0];
  speech::speak(std::format("{}: {}, 1 {} {}", cat_name(g_cat), describe(list[0]), strings::kOf, list.size()), true);
  sonar::selected(list[0]);   // its sound, from where it is
}

// The list is of the field the player is walking: in a menu, a conversation, a battle or a scene its keys say so.
static bool in_field() {
  if (world::exploring()) return true;
  speech::speak(strings::kNotInField, true);
  return false;
}

void next(int delta) {
  if (!in_field()) return;
  auto all = scan();
  Cat was = g_cat;
  auto list = settle(all, 1, false);
  int cur = -1;
  for (size_t i = 0; i < list.size(); ++i) if (list[i].obj == g_sel) cur = (int)i;
  if (list.empty() || g_cat != was || cur < 0) { announce_nearest(list); return; }
  int n = (int)list.size();
  int i = ((cur + delta) % n + n) % n;
  g_sel = list[i].obj;
  g_sel_poi = list[i];
  speech::speak(std::format("{}, {} {} {}", describe(list[i]), i + 1, strings::kOf, n), true);
  sonar::selected(list[i]);
}

void next_category(int delta) {
  if (!in_field()) return;
  announce_nearest(settle(scan(), delta, true));
}

void announce_list() { announce_nearest(settle(scan(), 1, false)); }

// The selection as it is now, from its own actor: a rescan could miss it (an Event trigger is only listed when nothing
// else is near) and paid a navmesh query per far-off hit actor on every read.
bool selected(Poi& out) {
  UObject* a = g_sel.resolve();
  if (!a || get_bool(a, "bHidden")) return false;   // a defeated or parked Shadow
  world::FVector me, v{};
  if (!world::player_location(me) || !guarded([&] { v = world::actor_location(a); })) return false;
  if (doors::behind(a, me, v)) return false;   // the hero has gone through a door since: it is not in the list now
  out = g_sel_poi;
  out.loc = v;
  out.dist = std::sqrt((v.x - me.x) * (v.x - me.x) + (v.y - me.y) * (v.y - me.y) + (v.z - me.z) * (v.z - me.z));
  out.bearing = world::bearing(me, v);
  return true;
}
}  // namespace p3a::poi

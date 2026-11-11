// Where the hero is: the field area's name, its ids, the Tartarus floor, and the announcement on arrival.
// Read here:
//  - The hit-name tables (DT_FldDailyHitName, DT_FldDungeonHitName) name each field area at row
//    KIN_<major-100, 3 digits><minor, 3 digits>00 (KIN_00510100 = "Iwatodai Station" = field 105 area 101, verified
//    2026-10-27). The field's ids are in its hit actors' level names (LV_F102_121_Hit = field 102, area 121).
//  - The place banner (bottom left on arrival: "Lounge", "Hallway", "Thebel") is AUIAccessInfoDraw's: its task state
//    +0x294, its text the FString at +0x968 (2026-11-03; a DT_FldPlaceName row, finer than the hit-name table's
//    area), +0xA10 set when it shows a floor number and that number at +0xA14 (drawn as a sprite).
//  - AFldDungeonControlActor: +0x2A0 the dungeon (299 = Tartarus), +0x2A4 the floor.
#include "poi_internal.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "gametext.h"
#include "seh.h"
#include "speech.h"
#include "world.h"
#include <algorithm>
#include <cstdio>
#include <format>
#include <map>

namespace p3a::poi {
using namespace ue;

static std::string g_banner;   // the place banner's text
static int g_banner_floor = 0;  // its floor number, 0 when it shows none
static bool g_banner_moving = false;   // between places: the banner is not the new place's yet
static void banner_tick() {
  static const int id = watch::want("UIAccessInfoDraw");
  uint8_t* a = (uint8_t*)watch::get(id);
  g_banner_moving = false;
  if (!a) return;
  std::string b;
  int floor = 0;
  // Its state +0x294 on arrival: 13 leaving, 14 loading, 1 the new place (the floor is written first), 2 showing the
  // banner, 3 settled. Read in 2 and 3 only, once the name and the floor agree (2026-11-03, the dorm's stairs).
  // The new field's hero appears while it is still 14 with the old place's text ("Port Island Station, Paulownia
  // Mall" before "Port Island Station", 2026-11-03): until 2 the hero is between places.
  int st = 0;
  guarded([&] { st = *(int32_t*)(a + 0x294); });
  g_banner_moving = st == 13 || st == 14 || st == 1;
  if (g_banner_moving) { g_banner.clear(); g_banner_floor = 0; }
  if (st != 2 && st != 3) return;
  // +0xA10 set: the banner shows a floor number, +0xA14 (the dorm's 2F and 3F read 2 and 3).
  guarded([&] {
    b = core::clean_text(to_utf8(*(FString*)(a + 0x968)));
    if (*(int32_t*)(a + 0xA10)) floor = *(int32_t*)(a + 0xA14);
  });
  if (!core::has_words(b)) return;
  g_banner = b;
  g_banner_floor = floor > 0 && floor < 100 ? floor : 0;
}
bool area_ids(int& major, int& minor) {
  std::map<std::pair<int, int>, int> votes;
  // The field ids come from the hit actors' level names (LV_F102_121_Hit = field 102, area 121): mMajorID_/mMinorID_
  // name the area only for exits (a room's furniture carries script ids there, e.g. 1/5).
  for (UObject* a : live("FldHitActor")) {
    if (!world::actor_visible_level(a)) continue;
    std::string lvl = name_of(outer_of(outer_of(a)));
    int ma = 0, mi = 0;
    if (sscanf(lvl.c_str(), "LV_F%d_%d", &ma, &mi) == 2 && ma >= 100) ++votes[{ma, mi}];
  }
  if (votes.empty()) return false;
  auto best = std::max_element(votes.begin(), votes.end(), [](auto& a, auto& b) { return a.second < b.second; })->first;
  major = best.first; minor = best.second;
  return true;
}

int tartarus_floor() {
  static const int dcid = watch::want("FldDungeonControlActor");
  UObject* dc = watch::get(dcid);
  int32_t* f = dc ? (int32_t*)((uint8_t*)dc + 0x2A0) : nullptr;
  return f && f[0] == 299 && f[1] >= 1 && f[1] < 264 ? f[1] : 0;
}

std::string area_name() {
  banner_tick();
  int major = 0, minor = 0;
  if (area_ids(major, minor)) {
    std::string row = std::format("KIN_{:03}{:03}00", major - 100, minor);
    for (const char* table : {"DT_FldDailyHitName", "DT_FldDungeonHitName"}) {
      UObject* t = gametext::table(table);
      if (uint8_t* r = t ? datatable_find(t, row) : nullptr) {
        std::string s = to_utf8(*(FString*)(r + offset_of(datatable_struct(t), "Name")));
        // The banner names the part of it the hero is in ("Iwatodai Dorm, Lounge") when that says more.
        if (!s.empty() && !g_banner.empty() && s.find(g_banner) == std::string::npos && g_banner.find(s) == std::string::npos)
          return s + ", " + g_banner + (g_banner_floor ? std::format(", {} {}", strings::kFloor, g_banner_floor) : "");
        if (!s.empty()) return s;
      }
    }
  }
  // Places the tables do not name (Tartarus): the game's own area banner, bottom left (FontDrawEx, y ~910); the
  // floor number beside it is a sprite: AFldDungeonControlActor +0x2A0 is the dungeon (299 = Tartarus) and +0x2A4
  // the floor (2 and 3 on 2F and 3F, 2026-10-27; the tutorial floor read 264, its design id).
  int floor = tartarus_floor();
  bool tartarus = floor > 0;
  if (g_banner.empty() && !tartarus) return strings::kUnknownArea;
  // On a floor the last banner can still be the entrance's ("Tartarus: Entrance, floor 2", 2026-10-28): the
  // floor's own banner (the block, "Thebel") comes a moment after the floor is announced.
  bool stale = tartarus && g_banner.find("Entrance") != std::string::npos;
  std::string name = g_banner.empty() || stale ? strings::kTartarus : g_banner;
  if (tartarus) name += std::format(", {} {}", strings::kFloor, floor);
  return name;
}

// Speak the area when it changes (the game shows it as a banner on arrival).
void arrival_tick() {
  // The area is said on arrival: when the hero is a new actor (a field loaded) or the place banner changes (the game
  // sets it on every arrival, a floor or room of the same field included); until the hit actors that name it have
  // loaded it is looked up again each tick.
  static std::string spoken, banner_seen;
  static WeakObj hero_seen;
  static bool due = false;
  UObject* hero = world::player();
  if (!hero) return;   // a menu or scene hides the hero for a moment: not an arrival
  banner_tick();
  std::string key = g_banner + "|" + std::to_string(g_banner_floor);
  if (!(WeakObj::of(hero) == hero_seen)) { hero_seen = WeakObj::of(hero); spoken.clear(); due = true; }   // a field loaded: said again
  if (key != banner_seen) { banner_seen = key; due = true; }
  if (!due || g_banner_moving) return;
  std::string a = area_name();
  if (a == strings::kUnknownArea) return;
  due = false;
  if (a == spoken) return;
  spoken = a;
  speech::speak(a, false);
}
}  // namespace p3a::poi

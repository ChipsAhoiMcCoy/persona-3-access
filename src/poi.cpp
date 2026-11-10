#include "poi.h"
#include "party.h"
#include "watch.h"
#include "core/strings.h"
#include "core/units.h"
#include "core/text_clean.h"
#include "audio.h"
#include "msg.h"
#include "screens/screen.h"
#include "screens/util.h"
#include "textcap.h"
#include "devserver.h"
#include "gametext.h"
#include "hooks.h"
#include "seh.h"
#include "settings.h"
#include "social.h"
#include "speech.h"
#include "world.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <map>
#include <set>
#include <cstdio>
#include <memory>

namespace p3a::poi {
using namespace ue;

// Every live instance of a field class, kept by the object watcher (a find_all per class scanned all ~70k objects:
// 7 ms every 300 ms in the field, 2026-10-28).
static std::vector<UObject*> live(std::string_view cls) { return watch::all(watch::want(cls, true)); }

static const char* kind_name(Kind k) {
  switch (k) {
    case Kind::Exit: return strings::kPoiExit;
    case Kind::Person: return strings::kPoiPerson;
    case Kind::Shop: return strings::kPoiShop;
    case Kind::Door: return strings::kPoiDoor;
    case Kind::Enemy: return strings::kPoiShadow;
    default: return strings::kPoiThing;
  }
}

std::string clock(float b) {
  int h = (int)std::lround(b / 30.0f);
  if (h <= 0) h += 12;
  return std::format("{} o'clock", h);
}

// The cast's actor names carry the romanised Japanese spelling ("Mituru", "Sanada", "Fuka", "Aegis"): the names the
// English game uses. Everyone else reads by the actor-name token.
static std::string npc_actor_name(UObject* npc) {
  static const std::map<std::string, std::string> cast = {
    {"Yukari", "Yukari"}, {"Junpei", "Junpei"}, {"Mituru", "Mitsuru"}, {"Sanada", "Akihiko"}, {"Fuka", "Fuuka"},
    {"Aegis", "Aigis"}, {"Koromaru", "Koromaru"}, {"Ken", "Ken"}, {"Amada", "Ken"}, {"Shinjiro", "Shinjiro"}, {"Aragaki", "Shinjiro"},
    {"Ikutsuki", "Ikutsuki"}, {"Elizabeth", "Elizabeth"}, {"Igor", "Igor"}, {"Pharos", "Pharos"},
  };
  // FldNpcActor_105_102_Yukari_6 -> the first token after the field ids
  std::string n = name_of(npc);
  std::vector<std::string> tok;
  for (size_t s = 0; s <= n.size();) { size_t e = n.find('_', s); if (e == std::string::npos) e = n.size(); tok.push_back(n.substr(s, e - s)); s = e + 1; }
  for (auto& t : tok) { auto it = cast.find(t); if (it != cast.end()) return it->second; }
  // The first word after the class (FldNpcActor_Jin has no field ids at all: Adamah, 2026-10-28).
  for (size_t i = 1; i < tok.size(); ++i) if (!tok[i].empty() && !isdigit((unsigned char)tok[i][0])) return tok[i];
  return strings::kPoiPerson;
}

// A hit actor's own name: mNameIndex_ is the row KIN_<8 digits> of the daily or the dungeon hit-name table
// (10002006 = "Escalator", 10000001 = "Go Elsewhere"; verified at Iwatodai Station 2026-10-27).
static UObject* hit_table(const char* name) { return gametext::table(name); }
// UGlobalWorkBpLib::GWorkFlagChk(FName, out EGWorkOnOffEnum) -- the game's own story-flag check.
bool flag_on(const std::string& flag) {
  static UClass* lib = nullptr;
  static UFunction* fn = nullptr;
  if (!fn || !is_live(fn)) { lib = find_class("GlobalWorkBpLib"); fn = find_function(lib, "GWorkFlagChk"); }
  if (!fn) return false;
  // "FOO_12" is the FName FOO with number 13 (Unreal splits a numeric suffix without a leading zero off the name).
  FName name{fname_index(flag), 0};
  size_t us = flag.find_last_of('_');
  if (us != std::string::npos && us + 1 < flag.size() && flag.size() - us <= 10 &&
      flag.find_first_not_of("0123456789", us + 1) == std::string::npos && (flag[us + 1] != '0' || us + 2 == flag.size()))
    if (int32_t base = fname_index(std::string_view(flag).substr(0, us)); base >= 0) name = {base, std::stoi(flag.substr(us + 1)) + 1};
  if (name.index < 0) return false;
  Params p(fn);
  p.set<FName>("FlagName", name);
  call(cdo_of(lib), p);
  uint8_t* v = p.at<uint8_t>("OnOff");
  return v && enum_name(prop_inner_object(find_prop(fn, "OnOff")), *v) == "On";
}

// Row KIN_<idx, 8 digits> of the first of these name tables that has it (FldNpcNameTableRow and the hit-name rows
// share Name / flag / Name2).
static std::string kin_name(std::initializer_list<const char*> tables, int idx) {
  if (idx < 0) return {};
  std::string row = std::format("KIN_{:08}", idx);
  for (const char* tn : tables) {
    UObject* t = hit_table(tn);
    uint8_t* r = t ? datatable_find(t, row) : nullptr;
    if (!r) continue;
    UStruct* rs = datatable_struct(t);
    std::string n = to_utf8(*(FString*)(r + offset_of(rs, "Name")));
    // A row's Name2 replaces Name while its story flag is on ("Go Elsewhere" -> "Head to the Dorm" with
    // FLD_NOW_STATION_TO_DORMITORY): that is the name the game shows, and it tells the player where the story goes.
    std::string flag = to_utf8(*(FString*)(r + offset_of(rs, "flag")));
    std::string n2 = to_utf8(*(FString*)(r + offset_of(rs, "Name2")));
    if (!flag.empty() && !n2.empty() && flag_on(flag)) return n2;
    if (!n.empty()) return n;
  }
  return {};
}
std::string hit_name(UObject* hit) {
  return kin_name({"DT_FldDailyHitName", "DT_FldDungeonHitName"}, get<int32_t>(hit, "mNameIndex_", -1));
}
// An NPC's name as the game labels it: mDispName_, else row KIN_<mNameIndex_> of DT_FldNpcName (641000 = "Student",
// 1000 = "Kenji Tomochika"; verified at Gekkoukan 2026-10-27), else a cast name from the actor's own name.
static std::string npc_name(UObject* npc) {
  if (auto* dn = field<FString>(npc, "mDispName_")) { std::string s = to_utf8(*dn); if (!s.empty()) return s; }
  // A Social Link partner (FldCmmActor): mUniqueId_ is the arcana counted from the Fool as 0 (6 with the flag
  // CMM_06LOVERS..., 18 with CMM_18MOON..., 2026-10-27).
  if (is_a(npc, "FldCmmActor")) {
    int id = get<int32_t>(npc, "mUniqueId_", -1) + 1;
    if (id >= 1 && id <= 22) {
      std::string n = social::person(id);
      if (!n.empty()) return std::format("{} ({})", n, social::arcana_name(id));
    }
  }
  std::string n = kin_name({"DT_FldNpcName"}, get<int32_t>(npc, "mNameIndex_", -1));
  return n.empty() ? npc_actor_name(npc) : n;
}

// The game marks nearly every hit with the plain Check icon: a business's door (Be Blue V, the police station, Hagakure
// Ramen) and a classroom's look exactly like a bookshelf; only the Net Cafe and Umiushi carry the Shop icon. Their
// scripts tell them apart (HIT_SKILL_SHOP, HIT_POLICE_ENTER, CALL_PUBLIC_SHOP; FLD_OPEN_ORD_DOOR_FADE for a room), so
// these lists sort the Check hits by the name the game gives them (surveyed from every field level's hits and their
// BF_OBJ flowscripts, 2026-10-31).
static Kind check_kind(const std::string& name) {
  // Somewhere to buy, eat, work or spend time: a business's entrance, a vendor, a vending machine.
  static const std::set<std::string> shops = {
    "Aohige Pharmacy", "Azuki Arai Sweets", "Be Blue V", "Bookworms", "Used Bookstore", "Caf\xC3\xA9",
    "Chagall Caf\xC3\xA9", "Net Caf\xC3\xA9", "Game Parade", "Hagakure Ramen", "Ramen Shop", "Wakatsu Kitchen", "Umiushi Beef Bowls",
    "Hamburger Shop", "Wilduck Burger", "Kenneth Arnold", "Mahjong Parlor Red Hawk", "Mandragora", "Mayoido",
    "Mayoido Antiques", "Octopia", "Police Station", "Que Sera Sera", "Screen Shot", "Takoyaki Vendor",
    "Vending Machine", "Mystery Vending Machine", "Ticket Booth", "Kyoto Souvenirs", "Souvenir Sweets", "Kyoto Crafts",
  };
  // A room's door you check to go in (or find locked): classrooms, offices, restrooms, the dorm's bedrooms.
  static const std::set<std::string> doors = {
    "Classroom Door", "Classroom 2-E", "Computer Room", "Faculty Office", "Library", "Nurse's Office", "Laboratory",
    "Janitor's Room", "Art Room", "Science Room", "Music Room", "Music Prep Room", "Home Ec Prep Room",
    "Home Economics Room", "PA Room", "Printing Room", "Student Council Room", "Welfare Training Room", "Gym",
    "Indoor Swimming Pool", "Practice Field", "Men's Room", "Women's Room", "Restroom", "Bathroom", "Storage Room",
    "Storeroom", "Rooftop Storage", "Emergency Exit", "Rear Entrance", "Kitchen Door", "Locked Gate", "Metal Doors",
    "Large Door", "Command Room", "Sanada's Room", "Mitsuru's Room", "Yukari's Room", "Vacant Room", "Junpei's Room",
    "Aragaki's Room", "Amada's Room", "Fuuka's Room", "Aigis's Room", "His Room", "Door to Velvet Room", "Fuji Room", "Inn Exit", "Hot Spring",
    "School Building 2F", "School Building 3F",   // the school's stairs (a Check hit, HIT_SECOND_FLOOR_STAIRS)
  };
  if (shops.count(name)) return Kind::Shop;
  if (doors.count(name)) return Kind::Door;
  return Kind::Thing;
}

static std::string icon_of(UObject* hit) {
  FField* p = find_prop(class_of(hit), "mCheckIcon_");
  if (!p) return {};
  return enum_name(prop_inner_object(p), *(uint8_t*)((uint8_t*)hit + prop_offset(p)));
}

// A complete navmesh path ends near the thing (cached per actor for a few seconds: scans run twice a second).
static bool walkable(UObject* a, const world::FVector& me, const world::FVector& v) {
  static std::map<WeakObj::Key, std::pair<ULONGLONG, bool>> cache;
  auto key = WeakObj::of(a).key();
  ULONGLONG now = GetTickCount64();
  auto it = cache.find(key);
  if (it != cache.end() && now - it->second.first < 4000) return it->second.second;
  bool partial = true;
  auto path = world::find_path(me, v, &partial);
  bool ok = !partial && !path.empty() && std::fabs(path.back().z - v.z) < 250.0f &&
            std::hypot(path.back().x - v.x, path.back().y - v.y) < 300.0f;
  if (cache.size() > 2000) cache.clear();
  cache[key] = {now, ok};
  return ok;
}

std::vector<Poi> scan(float radius) {
  if (radius < 0) radius = settings::get(settings::PoiRange) * 100.0f;
  std::vector<Poi> out;
  world::FVector me;
  if (!world::player_location(me)) return out;
  UObject* hero = world::player();
  bool dungeon = hero && name_of(class_of(hero)).find("DUNGEON") != std::string::npos;
  auto add = [&](UObject* a, Kind k, std::string name, bool any_floor = false) {
    world::FVector v{};
    if (!guarded([&] { v = world::actor_location(a); })) return;
    float d = std::sqrt((v.x - me.x) * (v.x - me.x) + (v.y - me.y) * (v.y - me.y) + (v.z - me.z) * (v.z - me.z));
    if (d > radius) return;
    // Floors of one building share a level, stacked about 3.4 m apart and joined by teleporting stairs (the dorm,
    // 2026-10-27): another floor's things count only when the navmesh walks there (Tartarus' raised entrance does).
    // Tartarus floors are levels of their own with raised and sunken parts: no floor filter there.
    if (!dungeon && !any_floor && std::fabs(v.z - (me.z - 90.0f)) > 280.0f && !walkable(a, me, v)) return;
    // The daily and the Dark Hour version of a hit often stand on the same spot: one entry per name and place.
    for (auto& o : out)
      if (o.name == name && std::fabs(o.loc.x - v.x) + std::fabs(o.loc.y - v.y) + std::fabs(o.loc.z - v.z) < 150.0f) return;
    out.push_back(Poi{WeakObj::of(a), k, std::move(name), v, d, world::bearing(me, v)});
  };
  std::vector<UObject*> triggers;
  // A story trigger named for its scene, FldHitActor_<major>_<Scene>_H<n> (FldHitActor_299_Takaya_H0 and _Nyx_H0 at
  // the top of Tartarus, 2026-10-28), is where the story goes next: listed beside everything else, on any floor
  // (Takaya's box floats 7 m up a ramp, with no navmesh path to its centre), until its off flag is set.
  // UNTESTED: written while the game was in use; verify at Adamah floor 263 (slot 15).
  auto story_beat = [](UObject* a) -> std::string {
    std::string n = name_of(a);
    std::vector<std::string> tok;
    for (size_t b = 0; b <= n.size();) { size_t e = n.find('_', b); if (e == std::string::npos) e = n.size(); tok.push_back(n.substr(b, e - b)); b = e + 1; }
    if (tok.size() < 4 || tok[0] != "FldHitActor" || tok[1].empty() || !isdigit((unsigned char)tok[1][0])) return {};
    const std::string& w = tok[2];
    if (w.size() < 2 || !isalpha((unsigned char)w[0]) || (w[0] == 'H' && isdigit((unsigned char)w[1]))) return {};
    if (auto* off = field<FName>(a, "mOffFlagName_"); off && off->index > 0) {
      std::string flag = fname(*off);
      if (!flag.empty() && flag != "None" && flag_on(flag)) return {};
    }
    return w;
  };
  for (UObject* a : live("FldHitCore")) {
    if (!world::actor_visible_level(a) || is_a(a, "FldNpcActor")) continue;
    std::string icon = icon_of(a);
    if (icon == "None") continue;
    // Story triggers: unnamed boxes that only run a blueprint when walked through (the first Tartarus floor's
    // tutorial gates, 2026-10-27). Nothing there to use, but where the story leads: kept for an area with nothing
    // else (the monorail tracks, whose way on is a row of them, 2026-10-27).
    if (get<int32_t>(a, "mNameIndex_", -1) < 0 && !get<UObject*>(a, "mKeyPushBluePrint_") && get<UObject*>(a, "mOverlapBluePrint_")) {
      if (std::string beat = story_beat(a); !beat.empty()) add(a, Kind::Thing, std::format("{}: {}", strings::kPoiEvent, beat), true);
      else triggers.push_back(a);
      continue;
    }
    // Done already: an opened treasure chest keeps its hit, with its story flag on (FLD_TAR_TBOX_OPENED_04).
    if (auto* ex = field<FName>(a, "mExecutedFlagName_"); ex && ex->index > 0) {
      std::string flag = fname(*ex);
      if (!flag.empty() && flag != "None" && flag_on(flag)) continue;
    }
    // Turned off: a picked-up resource spot ("Something Shining", a Twilight Fragment) keeps its hit with its off flag
    // on (FLD_RESOURCE_SPOT10, Paulownia Mall 2026-10-31).
    if (auto* off = field<FName>(a, "mOffFlagName_"); off && off->index > 0) {
      std::string flag = fname(*off);
      if (!flag.empty() && flag != "None" && flag_on(flag)) continue;
    }
    std::string type;
    if (FField* tp = find_prop(class_of(a), "mType_")) type = enum_name(prop_inner_object(tp), *(uint8_t*)((uint8_t*)a + prop_offset(tp)));
    // A Social Link partner stands in the world as a hit actor of its own (FldCmmActor), hidden while away.
    bool cmm = is_a(a, "FldCmmActor");
    if (cmm && get_bool(a, "bHidden")) continue;
    std::string nm = cmm ? npc_name(a) : hit_name(a);
    if (type == "AreaChange" || icon == "Goto") add(a, Kind::Exit, nm.empty() ? strings::kPoiExit : nm);
    else if (icon == "Shop") add(a, Kind::Shop, nm.empty() ? strings::kPoiShop : nm);
    else if (icon == "Speak" || icon == "Listen") add(a, Kind::Person, nm.empty() ? strings::kPoiPerson : nm);
    else if (nm.empty()) add(a, Kind::Thing, strings::kPoiThing);
    else add(a, check_kind(nm), nm);
  }
  for (UObject* a : live("FldNpcActor")) {
    if (!world::actor_visible_level(a) || get_bool(a, "bHidden")) continue;
    // A placeholder with nobody spawned in it (no child actor, no class): not a person (the dorm lounge, 2026-10-27).
    auto* kids = field<TArray<UObject*>>(a, "Children");
    if (kids && kids->num == 0 && !get<UObject*>(a, "mNpcBaseClass_") && !get<UObject*>(a, "mCharaBaseClass_")) continue;
    add(a, Kind::Person, npc_name(a));
  }
  // Tartarus' breakable Shadow remains (BP_DOBJ_Broken*_C; strike them, Ctrl, for items). A broken one stays visible
  // but its Box stops colliding (NoCollision, 2026-10-27).
  for (UObject* a : live("FldDungeonObjectActor")) {
    std::string cn = name_of(class_of(a));
    if (cn.find("Broken") == std::string::npos || cn.find("Fragment") != std::string::npos || get_bool(a, "bHidden")) continue;
    if (UObject* box = get<UObject*>(a, "Box")) {
      UFunction* f = find_function(class_of(box), "GetCollisionEnabled");
      if (f) { Params p(f); call(box, p); if (p.ret<uint8_t>() == 0) continue; }
    }
    add(a, Kind::Thing, strings::kPoiBreakable);
  }
  // Shadows roaming a Tartarus floor (EnemySymbolBaseCore; parked or defeated ones are hidden).
  for (UObject* a : live("EnemySymbolBaseCore")) {
    if (get_bool(a, "bHidden")) continue;
    add(a, Kind::Enemy, strings::kPoiShadow);
  }
  if (out.empty()) for (UObject* a : triggers) add(a, Kind::Thing, strings::kPoiEvent);
  std::sort(out.begin(), out.end(), [](const Poi& a, const Poi& b) { return a.dist < b.dist; });
  // A door between two rooms is two exits a step apart, one on each side, each named for where it leads (the
  // monorail's cars, 1.4 m apart, 2026-10-27): only the one on the player's side, the nearer, counts.
  std::vector<Poi> kept;
  for (auto& p : out) {
    bool far_side = false;
    if (p.kind == Kind::Exit)
      for (auto& k : kept)
        if (k.kind == Kind::Exit && k.name != p.name &&
            std::hypot(k.loc.x - p.loc.x, k.loc.y - p.loc.y, k.loc.z - p.loc.z) < 200.0f) far_side = true;
    if (!far_side) kept.push_back(std::move(p));
  }
  return kept;
}

std::string describe(const Poi& p) {
  // The name stands alone, a way into another area's too: it is where it leads, and its category says what it is
  // ("Gekkoukan High School exit" was taken for the way out of the school; the user's call, 2026-11-09).
  return std::format("{}, {}, {}", p.name, core::meters(p.dist / 100.0f), clock(p.bearing));
}

// ---- the review selection ----
// Categories (left and right) each hold their points of interest nearest first (up and down). Changing category, or
// opening the controller's mod layer, selects the nearest; an empty category is skipped.
enum class Cat { Exits, Doors, People, Shops, Things, Shadows, Count };
static const char* cat_name(Cat c) {
  switch (c) {
    case Cat::Exits: return strings::kCatExits;
    case Cat::Doors: return strings::kCatDoors;
    case Cat::People: return strings::kCatPeople;
    case Cat::Shops: return strings::kCatShops;
    case Cat::Things: return strings::kCatThings;
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
    case Cat::Things: return p.kind == Kind::Thing;
    default: return p.kind == Kind::Enemy;
  }
}

static std::vector<Poi> in_category(const std::vector<Poi>& all, Cat c) {
  std::vector<Poi> v;
  for (auto& p : all) if (in_cat(p, c)) v.push_back(p);
  return v;   // scan() sorts nearest first
}

// The current category, or the next one in `delta`'s direction that has something (staying put when it does).
static std::vector<Poi> settle(const std::vector<Poi>& all, int delta, bool move) {
  int n = (int)Cat::Count;
  for (int step = move ? 1 : 0; step <= n; ++step) {
    Cat c = (Cat)((((int)g_cat + step * delta) % n + n) % n);
    auto v = in_category(all, c);
    if (!v.empty()) { g_cat = c; return v; }
  }
  return {};
}

// "Exits: Iwatodai Station, 6 meters, 2 o'clock, 1 of 3", the nearest selected.
static void announce_nearest(const std::vector<Poi>& list) {
  if (list.empty()) { speech::speak(strings::kNothingNearbyFull, true); g_sel = {}; return; }
  g_sel = list[0].obj;
  g_sel_poi = list[0];
  speech::speak(std::format("{}: {}, 1 {} {}", cat_name(g_cat), describe(list[0]), strings::kOf, list.size()), true);
}

void next(int delta) {
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
}

void next_category(int delta) { announce_nearest(settle(scan(), delta, true)); }

void announce_list() { announce_nearest(settle(scan(), 1, false)); }

// The selection as it is now, from its own actor: a rescan could miss it (an Event trigger is only listed when nothing
// else is near) and paid a navmesh query per far-off hit actor on every beacon ping.
bool selected(Poi& out) {
  UObject* a = g_sel.resolve();
  if (!a || get_bool(a, "bHidden")) return false;   // a defeated or parked Shadow
  world::FVector me, v{};
  if (!world::player_location(me) || !guarded([&] { v = world::actor_location(a); })) return false;
  out = g_sel_poi;
  out.loc = v;
  out.dist = std::sqrt((v.x - me.x) * (v.x - me.x) + (v.y - me.y) * (v.y - me.y) + (v.z - me.z) * (v.z - me.z));
  out.bearing = world::bearing(me, v);
  return true;
}

// The hit-name tables name each field area at row KIN_<major-100, 3 digits><minor, 3 digits>00 (KIN_00510100 =
// "Iwatodai Station" = field 105 area 101, verified 2026-10-27); every hit actor carries its own area's ids.
// The place banner (bottom left on arrival: "Lounge", "Hallway", "Thebel") is AUIAccessInfoDraw's: its text is the
// FString at +0x968 (2026-11-03; a DT_FldPlaceName row, finer than the hit-name table's area; the floor number
// beside it is a sprite, its value +0xA14).
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
      UObject* t = hit_table(table);
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
// The field's interaction prompt ("GO TO / Head to the Dorm") has no reflected state; its label is the one string the
// field draws through AUIDrawBaseActor::DrawFont (textcap source 0; the area banner uses FontDrawEx only). The verb
// is a sprite, so it comes from the hit actor with that name nearby (its mCheckIcon_).
// A point of interest named by a prompt's label: a Social Link partner's name carries its arcana ("Nozomi Suemitsu
// (Moon)"), the prompt only the name (his prompt had no verb and no rank, 2026-11-03).
static bool is_named(const std::string& poi_name, const std::string& label) {
  return poi_name == label || (poi_name.size() > label.size() + 2 && poi_name.starts_with(label) &&
                               poi_name.compare(label.size(), 2, " (") == 0);
}
// A way onward the game names as what you do there ("Go Elsewhere", "Head Up", "Return to Dorm", "Proceed Onward",
// "Enter the Velvet Room", "To furthest floor": every such first word in the DT_Fld*HitName tables), not as a place.
static bool named_as_action(const std::string& name) {
  for (const char* verb : {"Go ", "Head ", "Return ", "Proceed ", "Enter ", "Exit ", "To "})
    if (name.rfind(verb, 0) == 0) return true;
  return false;
}
static std::string verb_for(const std::string& label) {
  for (auto& p : scan(800.0f)) {
    if (!is_named(p.name, label)) continue;
    UObject* a = p.obj.resolve();
    std::string icon = a ? icon_of(a) : "";
    // "Go to" only before a place the way leads to ("Go to: Iwatodai Station"). A label that already says what you do
    // ("Enter the Velvet Room", "Go Elsewhere", "Head Up") and a thing that is not an area change (Tartarus' "Giant
    // Device", a hit of type Normal under the same icon) stand alone: "Go to: Giant Device" and "Go to: Enter the
    // Velvet Room" read oddly (the user's call, 2026-11-09).
    if (icon == "Goto") {
      FField* tp = a ? find_prop(class_of(a), "mType_") : nullptr;
      bool area_change = tp && enum_name(prop_inner_object(tp), *(uint8_t*)((uint8_t*)a + prop_offset(tp))) == "AreaChange";
      return area_change && !named_as_action(label) ? strings::kVerbGoTo : "";
    }
    if (icon == "Speak") return strings::kVerbTalk;
    if (icon == "Listen") return strings::kVerbListen;
    if (icon == "Shop") return strings::kVerbShop;
    if (icon == "Action") return strings::kVerbAction;
    return strings::kVerbCheck;
  }
  // The party members following the hero are no point of interest, but their prompt is a talk ("Yukari Takeba").
  std::string first = label.substr(0, label.find(' '));
  for (int id = 2; id <= 11; ++id)
    if (std::string n = party::name(id); !n.empty() && n == first) return strings::kVerbTalk;
  return {};
}
// The story objective (top right, in corner brackets: "Head to the faculty office") is drawn by the same DrawFont;
// it is told apart by its brackets and spoken once per change.
static std::string g_objective, g_sub_objective;
std::string objective() { return g_objective; }
std::string objectives() {
  if (g_sub_objective.empty() || g_objective.empty()) return g_objective;
  return g_objective + ". " + g_sub_objective;
}
static std::string g_prompt;   // the interaction prompt label on screen now, or ""
std::string prompt() { return g_prompt; }
// The interaction prompt is AUIMiscCheckDraw's (2026-11-03): its task state +0x294 is 1 while it appears and 2 while
// it is up (4 leaving, 5 gone), its label the FString at +0x9A8 ("Classroom 2-F"). The verb beside it is a sprite.
static std::string misc_check_label() {
  static const int id = watch::want("UIMiscCheckDraw");
  uint8_t* a = (uint8_t*)watch::get(id);
  std::string label;
  if (a) guarded([&] {
    int st = *(int32_t*)(a + 0x294);
    if (st == 1 || st == 2) label = core::clean_text(to_utf8(*(FString*)(a + 0x9A8)));
  });
  return core::has_words(label) ? label : std::string();
}
static void prompt_tick() {
  // The objectives (top right, in corner brackets, then stacked lines) are drawn through DrawFont; not while a
  // message window draws through the same routine. Only while the field's panel is up (AUIAccessInfoDraw's state +0x294
  // is 3; 13 and 14 from a camp menu until the field comes back, town_map.cpp) or a place has no panel at all: between two
  // camp menus no screen is current, and the Item menu's Consumable Items tab, drawn the same way, was taken for an
  // objective (2026-11-06). The capture holds what was drawn just before too: only a draw since the field came back counts.
  static const int access_info = watch::want("UIAccessInfoDraw");
  uint8_t* panel = (uint8_t*)watch::get(access_info);
  int panel_state = -1;
  if (panel) guarded([&] { panel_state = *(int32_t*)(panel + 0x294); });
  static bool field_was = false;
  static uint64_t field_since = 0;
  bool field_up = !panel || panel_state == 3;
  if (field_up && !field_was) field_since = GetTickCount64();
  field_was = field_up;
  if (!msg::current().open && field_up)
    for (auto& t : textcap::snapshot(200)) {
      if (t.source != 0 || t.seen_ms <= field_since || !core::has_words(t.text)) continue;
      // U+300E opening corner bracket, top right
      if (t.x > 1400 && t.y < 320 && t.text.rfind("\xE3\x80\x8E", 0) == 0) {
        std::string o = t.text.substr(3);
        if (o.size() >= 3 && o.compare(o.size() - 3, 3, "\xE3\x80\x8F") == 0) o.resize(o.size() - 3);
        if (o != g_objective) { g_objective = o; g_sub_objective.clear(); speech::speak(std::string(strings::kObjective) + ": " + o, false); }
        continue;
      }
      // Further objectives stack under the first in the same top-right panel, without brackets.
      if (t.x > 1400 && t.y < 320) {
        std::string o = core::clean_text(t.text);
        std::string& sub = g_sub_objective;
        if (o != sub && o != g_objective) { sub = o; speech::speak(std::string(strings::kObjective) + ": " + o, false); }
      }
    }
  std::string& last = g_prompt;
  std::string label = misc_check_label();
  if (label == last) return;
  if (label.empty()) {
    // The prompt hides and shows again while the hero closes in on the thing or person (Mitsuru's was said five times
    // walking up to her, 2026-11-03): it stays said until the hero is out of its reach. A following party member is
    // no point of interest: theirs stays said until another prompt shows.
    if (!last.empty()) {
      bool near_it = false, poi = false;
      for (auto& p : scan(800.0f))
        if (is_named(p.name, last)) { poi = true; if (p.dist <= 300.0f) near_it = true; }
      if (poi && !near_it) last.clear();
    }
    return;
  }
  // A prompt that comes up behind a faded-out screen is not shown: a menu that closes after its fade-out (Rewind, a
  // load) leaves the old place's prompt up for a few frames, and "Dorm Register" was said on the way out (2026-11-10).
  if (ue::known_build() && screens::faded_out()) return;
  last = label;
  std::string verb = verb_for(label);
  // A Social Link partner's prompt also shows the link's arcana and rank.
  std::string link = verb == strings::kVerbTalk ? social::partner_rank(label) : std::string();
  speech::speak((verb.empty() ? label : verb + ": " + label) + (link.empty() ? "" : ", " + link), true);
}

// A Shadow within 15 m pulses from its direction, faster and louder as it closes in (lower when it is behind).
// Striking a Shadow from behind starts the battle with the advantage (the game's Chance Encounter). Within 4 m the
// nearest Shadow's heading is spoken once ("back turned" / "facing you"); while the hero stands behind it, facing
// it, within reach, a high tick says the strike (Ctrl) will land from behind.
static float yaw_of(UObject* a) {
  Params p(find_function(class_of(a), "K2_GetActorRotation"));
  call(a, p);
  return p.ret<FRotator>().yaw;
}
static float angle_diff(float a, float b) { float d = std::fmod(a - b + 540.0f, 360.0f) - 180.0f; return std::fabs(d); }
static void ambush_cues(UObject* shadow, const world::FVector& at, const world::FVector& me, float dist) {
  static WeakObj told;
  static int told_state = -1;
  static ULONGLONG tick_at = 0;
  if (!shadow) return;
  float to_me = std::atan2(me.y - at.y, me.x - at.x) * 180.0f / 3.14159265f;
  bool back_turned = angle_diff(yaw_of(shadow), to_me) > 110.0f;   // the hero is outside its front
  WeakObj w = WeakObj::of(shadow);
  if (dist < 400.0f) {
    int st = back_turned ? 1 : 0;
    if (!(w == told) || st != told_state) {
      told = w; told_state = st;
      speech::speak(back_turned ? strings::kShadowBackTurned : strings::kShadowFacingYou, false);
    }
  } else if (dist > 600.0f && w.index == told.index) {
    told_state = -1;
  }
  UObject* hero = world::player();
  if (hero && back_turned && dist < 220.0f) {
    float to_shadow = std::atan2(at.y - me.y, at.x - me.x) * 180.0f / 3.14159265f;
    ULONGLONG now = GetTickCount64();
    if (angle_diff(yaw_of(hero), to_shadow) < 45.0f && now - tick_at > 700) { tick_at = now; audio::beep(1320.0f, 40, 0.5f, world::spot(me, at)); }
  }
}

static void shadow_alert() {
  if (!settings::on(settings::ShadowAlerts)) return;
  static ULONGLONG next = 0;
  ULONGLONG now = GetTickCount64();
  if (now < next) return;
  world::FVector me;
  if (!world::player_location(me)) return;
  float best = 1e9f, bearing = 0;
  UObject* nearest = nullptr;
  world::FVector at{};
  for (UObject* a : live("EnemySymbolBaseCore")) {
    if (get_bool(a, "bHidden")) continue;
    world::FVector v{};
    if (!guarded([&] { v = world::actor_location(a); })) continue;
    if (std::fabs(v.z - me.z) > 300.0f) continue;
    float d = std::hypot(v.x - me.x, v.y - me.y);
    if (d < best) { best = d; bearing = world::bearing(me, v); nearest = a; at = v; }
  }
  if (best > 1500.0f) { next = now + 500; return; }
  ambush_cues(nearest, at, me, best);
  float b = bearing * 3.14159265f / 180.0f;
  float closeness = 1.0f - best / 1500.0f;
  bool behind = std::cos(b) < -0.2f && settings::on(settings::BehindPitch);
  audio::pulse(behind ? 110.0f : 165.0f, 140, 0.15f + 0.45f * closeness, world::spot(me, at));
  next = now + (ULONGLONG)(250 + 900 * (1.0f - closeness));
}

// Items picked up in the field (Tartarus chests): "Revival Bead×1" fades in at the left (FontDrawEx, about
// x 270 y 345, measured 2026-10-27). Spoken once it is fully drawn, when it appears (not again while it stays).
static void pickup_tick() {
  static std::set<std::string> shown;   // the pickups drawn at full strength last tick
  std::set<std::string> now_shown;
  for (auto& t : textcap::snapshot(200)) {
    if (t.source != 2 || t.x > 600 || t.y < 250 || t.y > 650 || (t.color & 0xff) != 0xff) continue;
    size_t x = t.text.find("\xC3\x97");   // U+00D7 multiplication sign
    if (x == std::string::npos) continue;
    now_shown.insert(t.text);
    if (shown.count(t.text)) continue;
    std::string name = t.text.substr(0, x), count = t.text.substr(x + 2);
    while (!name.empty() && name.back() == ' ') name.pop_back();
    speech::speak(count == "1" ? std::format("{} {}", strings::kGot, name) : std::format("{} {}, {}", strings::kGot, name, count), false);
  }
  shown.swap(now_shown);
}

// Things coming into reach chime once from their direction (re-armed once the hero is 15 m away): exits low, things
// to check or open high, people in between. A sound, not words: the points of interest list has the words.
static void proximity_chimes() {
  if (!settings::on(settings::NearbyChimes)) return;
  static ULONGLONG last = 0;
  ULONGLONG now = GetTickCount64();
  if (now - last < 300) return;
  last = now;
  static std::set<WeakObj::Key> armed;
  world::FVector me;
  if (!world::player_location(me)) { armed.clear(); return; }
  std::set<WeakObj::Key> still;
  int played = 0;
  for (auto& p : scan(1500.0f)) {
    if (p.kind == Kind::Enemy) continue;   // Shadows have their own pulse
    auto key = p.obj.key();
    if (p.dist > 800.0f) { if (armed.count(key)) still.insert(key); continue; }
    still.insert(key);
    if (armed.count(key) || played >= 2) continue;
    float b = p.bearing * 3.14159265f / 180.0f;
    float f = p.kind == Kind::Exit ? 392.0f : p.kind == Kind::Door ? 523.0f : p.kind == Kind::Person ? 659.0f :
              p.kind == Kind::Shop ? 784.0f : 988.0f;
    if (std::cos(b) < -0.2f && settings::on(settings::BehindPitch)) f *= 0.85f;   // behind: a little lower
    audio::beep(f, 70, 0.3f, world::spot(me, p.loc));
    ++played;
  }
  armed.swap(still);
}

void tick() {
  if (screens::current() || msg::current().blocking) return;   // a menu, notice or message has the floor
  pickup_tick();
  shadow_alert();
  proximity_chimes();
  prompt_tick();
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

void install() {
  dev::route("/hits", [](const dev::Request& r, int&) {
    // /hits?idx=10002012 every hit actor with that name index (any level), with its level and visibility
    int want = (int)dev::arg_int(r, "idx", -1);
    auto out = std::make_shared<std::string>("timeout\n");
    hooks::run_on_game_thread([out, want] {
      std::string s;
      for (UObject* a : live("FldHitCore")) {
        int idx = get<int32_t>(a, "mNameIndex_", -1);
        if (want >= 0 && idx != want) continue;
        world::FVector v = world::actor_location(a);
        s += std::format("#{} {} idx {} '{}' level {} visible={} at ({:.0f}, {:.0f}, {:.0f})\n", index_of(a), name_of(a), idx, hit_name(a), name_of(outer_of(outer_of(a))),
                         world::actor_visible_level(a), v.x, v.y, v.z);
      }
      *out = s.empty() ? "none\n" : s;
    });
    return *out;
  }, "?idx=N hit actors by name index");
  dev::route("/poi", [](const dev::Request& r, int&) {
    auto out = std::make_shared<std::string>("timeout\n");
    float radius = (float)dev::arg_num(r, "r", 6000);
    hooks::run_on_game_thread([out, radius] {
      std::string s;
      for (auto& p : scan(radius)) s += std::format("#{} [{}] {}\n", p.obj.index, kind_name(p.kind), describe(p));
      *out = s.empty() ? "none\n" : s;
    });
    return *out;
  }, "?r=cm points of interest near the player");
}
}  // namespace p3a::poi

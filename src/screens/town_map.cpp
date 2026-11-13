// The town map ("Go Elsewhere"): open while the innermost pause is TownMap. The selected destination is
// AUITownMapActor +0x3998, in the order of DT_TownMapParamTable's positions (School, Dormitory, Paulownia Mall,
// Port Island, Iwatodai, Naganaki; measured 2026-10-27 by moving the cursor). The destination labels are sprites,
// so their names are the mod's (strings::kTownMapPlaces).
// A bespoke menu: the open destinations as a list in screen order (top to bottom, then left to right); A/D and the
// stick still move on the map, and the list follows. The game's own move
// (exe+0x1457ee0) picks a neighbour from the destinations' screen positions (+0x3950, an FVector each: School
// 1353,489, Dorm 250,198, Mall 1100,250, Port Island 1000,700, Iwatodai 600,195, Shrine 839,200), writes the cursor,
// then exe+0x144df60(actor, true) plays the cursor sound and moves the marker; in town-info mode (F) it also refreshes
// the info (exe+0x14431c0) and blends the camera to m_pInfoCamera[cursor]. A destination is selectable when
// exe+0x120d970(sub1, i) and exe+0x120da10(sub2, i) agree (sub1 = exe+0x122ab40(), sub2 = exe+0x122abb0()).
// The map opens with the cursor on the place the hero is in, and the list starts there.
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/camp.h"
#include "core/reading_order.h"
#include "core/strings.h"
#include "log.h"
#include "poi.h"
#include "seh.h"
#include "social.h"
#include "speech.h"
#include "world.h"
#include "core/text_clean.h"
#include "ue/scan.h"
#include <vector>
#include <format>

namespace p3a::screens {
using namespace ue;

class TownMapScreen : public MenuScreen {
  Finder map_{"UITownMapActor"};
  static constexpr int kCount = 6;
  int read() {
    UObject* m = map_.get();
    if (!m) return -1;
    int c = *(int32_t*)((uint8_t*)m + 0x3998);
    return c >= 0 && c < kCount ? c : -1;
  }
  static const char* place(int c) { return c >= 0 && c < kCount ? strings::kTownMapPlaces[c] : strings::kTownMap; }
  // The open destinations in screen order, built once per visit.
  std::vector<int> order_;
  static bool selectable(int i) {
    uintptr_t b = scan::exe_base();
    bool ok = true;
    guarded([&] {
      void* s1 = ((void* (*)())(b + 0x122ab40))();
      void* s2 = ((void* (*)())(b + 0x122abb0))();
      if (s1 && !((bool (*)(void*, int))(b + 0x120d970))(s1, i)) ok = false;
      if (ok && s2 && !((bool (*)(void*, int))(b + 0x120da10))(s2, i)) ok = false;
    });
    return ok;
  }
  void build_order() {
    UObject* m = map_.get();
    if (!m) return;
    std::vector<core::Spot> ps;
    for (int i = 0; i < kCount; ++i) {
      auto* v = (float*)((uint8_t*)m + 0x3950 + i * 12);
      if (selectable(i)) ps.push_back({i, v[0], v[1]});
    }
    if (ps.empty()) return;
    // Rows: places within 80 px of the row's first (topmost) place share it, read left to right.
    ps = core::reading_order(std::move(ps), 80);
    order_.clear();
    std::string log_line;
    for (auto& p : ps) { order_.push_back(p.id); log_line += std::format(" {}({:.0f},{:.0f})", p.id, p.x, p.y); }
    log::writef("town_map: order{}", log_line);
    for (int i : order_) who_[i] = people(i);
  }
  // Who is at a destination now: the list the game builds for town info's card icons, exe+0x11d8fc0(out, destination),
  // from the field characters' own tables, the date and the time of day. `out` is four arrays and a set, freed the
  // way the game's caller (exe+0x14431c0) frees them: +0x00 the Social Links (0x20 each: arcana, rank, reversed, the
  // name FString at +0x10; a maxed link has rank 10), +0x10 / +0x20 / +0x30 other characters by kind (0x18 each: the
  // kind 1-3, the name FString at +0x8).
  std::string who_[kCount];
  static std::string people(int dest) {
    alignas(16) uint8_t out[0x100] = {};
    uintptr_t b = scan::exe_base();
    std::string links, others;
    bool ok = guarded([&] {
      ((void* (*)(void*, int))(b + 0x11d8fc0))(out, dest);
      std::vector<social::Link> mine = social::read();
      auto* l = (TArray<uint8_t>*)out;
      for (int i = 0; l->data && i < l->num && i < 32; ++i) {
        uint8_t* e = l->data + i * 0x20;
        int arcana = *(int32_t*)e, rank = *(int32_t*)(e + 4);
        std::string n = core::clean_text(to_utf8(*(FString*)(e + 0x10)));
        log::writef("town_map: {} link {} rank {} reversed {} '{}'", dest, arcana, rank, e[8], n);
        std::string s = std::format("{} ({})", n.empty() ? social::person(arcana) : n, social::arcana_name(arcana));
        if (rank > 0) s += std::format(", {} {}", strings::kRank, rank);
        if (e[8]) s += std::string(", ") + strings::kReversed;
        else for (auto& m : mine) if (m.arcana == arcana && m.rank == rank && social::ready(m)) s += std::string(", ") + strings::kReadyToRankUp;
        links += (links.empty() ? "" : "; ") + s;
      }
      for (int k = 1; k <= 3; ++k) {
        auto* a = (TArray<uint8_t>*)(out + k * 0x10);
        for (int i = 0; a->data && i < a->num && i < 32; ++i) {
          std::string n = core::clean_text(to_utf8(*(FString*)(a->data + i * 0x18 + 8)));
          log::writef("town_map: {} other kind {} '{}'", dest, k, n);
          if (!n.empty() && others.find(n) == std::string::npos) others += (others.empty() ? "" : ", ") + n;
        }
      }
      ((void (*)(void*))(b + 0xa789b0))(out + 0x40);
      for (int k = 3; k >= 1; --k) ((void (*)(void*))(b + 0x9fbee0))(out + k * 0x10);
      ((void (*)(void*))(b + 0x8d8330))(out);
    });
    if (!ok) { log::write("town_map: reading who is there faulted"); return {}; }
    std::string s;
    if (!links.empty()) s = std::string(strings::kTownLinks) + ": " + links;
    if (!others.empty()) s += (s.empty() ? "" : ". ") + std::string(strings::kTownOthers) + ": " + others;
    return s;
  }
  // F (town info): the destination's shops and places, as the game draws them (FUN_141447000). The rows are the
  // destination's FTownMapInfoTable rows (LocationNo) of the UDatTownMapInfo subsystem's m_aTownMapInfoList (the
  // actor copies them per destination to +0x568, exe+0x14433b0), drawn by CategoryNo 0 to 8, in table order within
  // one. A row's name is its TextLabel's Text in m_aNameList (the dorm's one row is "NONE", drawn as it is); it is
  // dimmed where exe+0x120e430(subsystem, row) says it is not open now (the row's Noon or Night for the time of day,
  // and its HideFlag). Read while the info is up (step 6). (+0x3EB4, once taken for the info's own step, is a 0-3
  // frame counter stepped every 0.1 s.)
  Finder dat_{"DatTownMapInfo"};
  std::string town_info() {
    int dest = read();
    UObject* dat = dat_.get();
    if (dest < 0 || !dat || step() != 6 || !ue::known_build()) return {};
    TableRows rows = table_rows(dat, "m_aTownMapInfoList"), names = table_rows(dat, "m_aNameList");
    if (!rows || !names) return {};
    int loc = offset_of(rows.row, "LocationNo"), cat = offset_of(rows.row, "CategoryNo"),
        label = offset_of(rows.row, "TextLabel"), name_label = offset_of(names.row, "TextLabel"),
        text = offset_of(names.row, "Text");
    if (loc < 0 || cat < 0 || label < 0 || name_label < 0 || text < 0) return {};
    auto open = (bool (*)(void*, void*))(scan::exe_base() + 0x120e430);
    std::string s;
    guarded([&] {
      for (int c = 0; c < 9; ++c)
        for (int i = 0; i < rows.num; ++i) {
          uint8_t* r = rows.at(i);
          if (*(int32_t*)(r + loc) != dest || *(int32_t*)(r + cat) != c) continue;
          std::string want = to_utf8(*(FString*)(r + label)), n;
          for (int j = 0; j < names.num && n.empty(); ++j)
            if (to_utf8(*(FString*)(names.at(j) + name_label)) == want) n = core::clean_text(to_utf8(*(FString*)(names.at(j) + text)));
          if (n.empty()) continue;
          if (!open(dat, r)) n += std::string(" (") + strings::kUnavailable + ")";
          s += (s.empty() ? "" : ", ") + n;
        }
    });
    return s.empty() ? s : std::string(strings::kTownInfo) + ": " + s;
  }
  // Moves the game's cursor the way its own move does (see the top): the cursor, the town info and its camera.
  bool move_to(int to) {
    UObject* m = map_.get();
    if (!m || to == read()) return m != nullptr;
    bool info = step() == 6;   // town info is up (F): it follows the cursor
    uintptr_t b = scan::exe_base();
    auto* a = (uint8_t*)m;
    bool ok = guarded([&] {
      *(int32_t*)(a + 0x3998) = to;
      if (info) {
        ((void (*)(void*))(b + 0x14431c0))(a);
        UObject* cam = *(UObject**)(a + 0x500 + to * 8);
        UObject* pc = player_controller();
        if (cam && pc) {
          Params p(find_function(class_of(pc), "SetViewTargetWithBlend"));
          p.set<UObject*>("NewViewTarget", cam);
          p.set<float>("BlendTime", 0.3f);
          p.set<uint8_t>("BlendFunc", 2);
          p.set<float>("BlendExp", 0.7f);
          p.set<bool>("bLockOutgoing", true);
          call(pc, p);
        }
        *(float*)(a + 0x3ec0) = 0.3f;
      }
      ((void (*)(void*, bool))(b + 0x144df60))(a, true);
    });
    if (!ok) log::write("town_map: moving the cursor faulted");
    return true;
  }
  // AUITownMapActor +0x294 is its step: 4 the map, 5 the entrance list (a UILocationSelect: LocationListScreen),
  // 6 town info, then 11, 12, 13 and 14 within 160 ms of an entrance being confirmed (measured 2026-10-29).
  int step() { UObject* m = map_.get(); int s = 0; if (m) guarded([&] { s = *(int32_t*)((uint8_t*)m + 0x294); }); return s; }
  Mirror& map_list_ = mirror("town", [this] { int s = step(); return s == 4 || s == 6 ? read() : -1; });
  std::string info_;
 public:
  TownMapScreen() {
    map_list_.set = [this](int i) { return move_to(i); };
    // FUN_141454640 case 4: the fade over the map (FUN_14108e800), what its confirm waits for; town info has none.
    map_list_.ready = [this] { int s = step(); return s == 6 || (s == 4 && ((int (*)())(scan::exe_base() + 0x108e800))() != 0); };
  }
  const char* id() const override { return "town_map"; }
  int priority() const override { return 15; }
  // EAppPauseType::TownMap; never in a dungeon (Tartarus raised it during the Fuuka rescue, 2026-10-28).
  // The area map (M where travel is closed) raises the same pause with the actor idle at step 0: only steps 4-6 are
  // the town map (it claimed the area map until 2026-10-30).
  bool active() override {
    if (pause_top() != 2 || !map_.get()) return false;
    int s = step();
    if (s < 4 || s > 6) return false;
    UObject* p = world::player();
    return !p || name_of(class_of(p)).find("DUNGEON") == std::string::npos;
  }
  bool holds_keys() override { int s = step(); return s == 4 || s == 6; }
  std::string title() override { return strings::kTownMap; }
  void on_enter() override { order_.clear(); screens::set_keys(strings::kTownMapHelp); }
  void on_tick() override {
    if (order_.empty()) build_order();
    // F (town info): read when it has settled with a new list (town_info), again after it leaves and comes back.
    // Queued: it settles a moment after a move, and must not cut off the place just focused.
    std::string info = town_info();
    if (step() != 6) info_.clear();
    else if (!info.empty() && info != info_) { info_ = info; say(info, false); }
  }
  void build(ui::GraphBuilder& b) override {
    b.begin_stop("town");
    for (int i : order_) {
      std::string l = place(i);
      auto v = game_item(map_list_, i, [l] { return l; });
      describe_with(v, [this, i] { return who_[i]; });
      b.add_item(slot("town", i), v);
    }
    if (int c = read(); c >= 0) b.set_start(slot("town", c));
  }
};

std::unique_ptr<Screen> make_town_map() { return std::make_unique<TownMapScreen>(); }

// "Where should I go?": the town map's entrance list and the M key's own list in a daily area are both a
// UILocationSelect: m_aShortcutList (+0xD0) of FShortcutItem (0xF0: +0x8 the name FString,
// +0x18 mEnable), the cursor at +0x200 (never scrolls, wraps; a write is honoured, the bar animates to it).
// The M key's list belongs to AUIAccessInfoDraw (+0x338; its minimap +0x310), whose state +0x294 (previous +0x298) is
// 3 in the field, 5 opening, 6 open and taking input (FUN_141411ae0: confirm -> FUN_141430cf0 -> 7, 9 or 0xC; C -> 7;
// Tab zooms the map), 7 closing (measured 2026-11-03). The town map's own list is the actor's +0x480 on its entrance
// step (5).
class LocationListScreen : public MenuScreen {
  Finder town_{"UITownMapActor"};
  Finder access_{"UIAccessInfoDraw"};
  int access_state() {
    uint8_t* a = (uint8_t*)access_.get();
    int s = a ? *(int32_t*)(a + 0x294) : -1;
    static int logged = -2;
    if (s != logged) { log::writef("location_list: access info state {}", s); logged = s; }
    return s;
  }
  static bool has_items(uint8_t* u) { auto* a = u ? (TArray<uint8_t>*)(u + 0xD0) : nullptr; return a && a->data && a->num > 0 && a->num < 64; }
  uint8_t* list() {
    uint8_t* t = (uint8_t*)town_.get();
    if (t && pause_top() == 2) {
      uint8_t* mine = *(uint8_t**)(t + 0x480);
      return *(int32_t*)(t + 0x294) == 5 && has_items(mine) ? mine : nullptr;
    }
    uint8_t* a = (uint8_t*)access_.get();
    uint8_t* m = a && access_state() == 6 ? *(uint8_t**)(a + 0x338) : nullptr;
    return has_items(m) ? m : nullptr;
  }
  Mirror& list_ = mirror("places", [this] { uint8_t* l = list(); return l ? *(int32_t*)(l + 0x200) : -1; });
 public:
  LocationListScreen() {
    // FUN_1414371e0 case 6: the M list reads nothing until the fade over it ends (FUN_14108e800); an entrance's list has
    // no gate (FUN_141454640 case 5).
    list_.ready = [this] { return (town_.get() && pause_top() == 2) || ((int (*)())(scan::exe_base() + 0x108e800))() != 0; };
    list_.set = [this](int i) {
      uint8_t* l = list();
      auto* a = l ? (TArray<uint8_t>*)(l + 0xD0) : nullptr;
      if (!a || i < 0 || i >= a->num) return false;
      *(int32_t*)(l + 0x200) = i;
      cursor_sound();
      return true;
    };
  }
  const char* id() const override { return "location_list"; }
  int priority() const override { return 16; }
  bool active() override { return list() != nullptr; }
  std::string title() override { return strings::kTownMapWhere; }
  void on_enter() override { screens::set_keys(strings::kFastTravelKeys); }
  void build(ui::GraphBuilder& b) override {
    uint8_t* l = list();
    auto* a = l ? (TArray<uint8_t>*)(l + 0xD0) : nullptr;
    if (!a) return;
    std::string key = std::format("places.{}", a->num);
    list_.key = key;
    b.begin_stop(key);
    for (int i = 0; i < a->num; ++i) {
      uint8_t* e = a->data + i * 0xF0;
      std::string n = core::clean_text(to_utf8(*(FString*)(e + 8)));
      if (!e[0x18]) n += std::string(", ") + strings::kUnavailable;
      b.add_item(slot(key, i), game_item(list_, i, [n] { return n; }));
    }
    if (int c = list_.cursor(); c >= 0) b.set_start(slot(key, c));
  }
};

std::unique_ptr<Screen> make_fast_travel() { return std::make_unique<LocationListScreen>(); }

// The area map (M where the story allows no travel, 2026-10-28: Paulownia Mall while the Velvet Room was the
// objective): a zoomed minimap with "Tab Zoom In/Out, C Close" that holds the hero. It is AUIAccessInfoDraw's state 6
// (the M screen taking input, see LocationListScreen) with no places in its list: FUN_141411ae0 confirms a place only
// when the list has one, and builds the zoom key help (FUN_14142b1d0) in the same state. The area is the mod's own
// name for it (poi::area_name, from the hit-name table), not the drawn bar.
class AreaMapScreen : public Screen {
  Finder access_{"UIAccessInfoDraw"};
 public:
  const char* id() const override { return "area_map"; }
  int priority() const override { return 13; }
  bool active() override {
    uint8_t* a = (uint8_t*)access_.get();
    if (!a || *(int32_t*)(a + 0x294) != 6 || pause_top() == 2) return false;
    uint8_t* sel = *(uint8_t**)(a + 0x338);
    auto* list = sel ? (TArray<uint8_t>*)(sel + 0xD0) : nullptr;
    return list && list->num == 0 && world::player();
  }
  void enter() override { screens::set_keys(strings::kAreaMapKeys); speech::speak(describe(), true); }
  std::string describe() override {
    std::string a = poi::area_name();
    return a.empty() || a == strings::kUnknownArea ? std::string(strings::kAreaMap) : std::format("{}, {}", strings::kAreaMap, a);
  }
};

std::unique_ptr<Screen> make_area_map() { return std::make_unique<AreaMapScreen>(); }
}  // namespace p3a::screens

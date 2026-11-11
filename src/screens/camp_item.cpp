// The camp's Item submenu as a bespoke menu (menu.h), modelled from the game's own objects (the offsets are this
// build's, measured 2026-10-30). Every list uses the generic ListCursor (util.h): index = row + top.
//
//   UCmpItem +0x38 state (3 list, 4 member, 6 Persona), +0x44 category, +0x40 the state's open timer, +0x178 / +0x17C
//   the NEW mark's index (the list's move), +0x188 UCmpItemSystem: +0x788 TArray<{u32 id, u8 count at +4, ...}> (0x10
//   each), +0x768 TArray<i16> party ids, +0x778 TArray<i32> Persona stock slots, +0x28 the Persona panel.
//   Cursors: list +0x84, member +0x48, Persona +0xC0. Every cursor here is written directly, with what the game's own
//   move calls after it (each `set`).
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/camp.h"
#include "screens/camp_util.h"
#include "core/strings.h"
#include "items.h"
#include "persona.h"
#include "ue/scan.h"
#include <format>

namespace p3a::screens {
using namespace ue;

class CampItem : public MenuScreen {
  Finder menu_{"CmpItem"};
  uint8_t* m() { UObject* o = camp_open() ? menu_.get() : nullptr; return (uint8_t*)o; }
  int state() { return i32(m(), 0x38); }
  uint8_t* sys() { uint8_t* o = m(); return o ? *(uint8_t**)(o + 0x188) : nullptr; }
  Mirror& list_ = mirror("items", [this] { return state() == 3 ? ListCursor(m() + 0x84).index() : -1; });
  Mirror& who_ = mirror("item.member", [this] { return state() == 4 ? ListCursor(m() + 0x48).index() : -1; });
  Mirror& pers_ = mirror("item.persona", [this] { return state() == 6 ? ListCursor(m() + 0xC0).index() : -1; });
  std::string category_;
 public:
  // The item's target: 1 / 3 one member (a member cursor), 2 the whole party (none).
  int target_type() {
    uint8_t* s = sys();
    auto* items = s ? arr<uint8_t>(s, 0x788) : nullptr;
    int it = ListCursor(m() + 0x84).index();
    if (!items || it < 0 || it >= items->num) return 0;
    return fn<int (*)(void*, uint16_t)>(0x131aef0)(s, (uint16_t)*(uint32_t*)(items->data + it * 0x10));
  }
 public:
  CampItem() {
    // The state's open timer (FUN_141325240 case 3, what its confirm waits for; the same frames as the Persona list's).
    list_.ready = [this] { uint8_t* o = m(); return state() == 3 && open_timer_done(*(float*)(o + 0x40), *(void**)(o + 0x30)); };
    // The list's move helper (FUN_141322900(menu, row, 1)): the sound, the NEW mark of the item left (+0x17c; +0x178
    // is the index the tick saw) cleared, the bar's slide.
    list_.set = [this](int i) {
      uint8_t* o = m();
      if (state() != 3 || !ListCursor(o + 0x84).set(i)) return false;
      *(int32_t*)(o + 0x17C) = *(int32_t*)(o + 0x178);
      fn<void (*)(void*, int, char)>(0x1322900)(o, row_of(o + 0x84), 1);
      return true;
    };
    // FUN_1413229a0(menu, index, 1): the sound, the party panel's highlight and its slide.
    who_.ready = [this] { int t = state() == 4 ? target_type() : 0; return t == 1 || t == 3; };
    who_.set = [this](int i) {
      ListCursor lc(m() + 0x48);
      if (state() != 4 || !lc.set(i)) return false;
      fn<void (*)(void*, int, char)>(0x13229a0)(m(), lc.index(), 1);
      return true;
    };
    // FUN_141322b60(menu, row): the sound and the Persona panel's bar; not until the panel's animation is done.
    pers_.ready = [this] { uint8_t* s = sys(); return state() == 6 && s && fn<bool (*)(void*)>(0x13032b0)(*(void**)(s + 0x28)); };
    pers_.set = [this](int i) {
      if (state() != 6 || !ListCursor(m() + 0xC0).set(i)) return false;
      fn<void (*)(void*, int)>(0x1322b60)(m(), row_of(m() + 0xC0));
      return true;
    };
  }
  const char* id() const override { return "camp_item"; }
  int priority() const override { return 17; }
  // FUN_141325240: 5 is the frame from the member list back to the items, 7-10 a skill card being learned (the learn
  // screen, above this one, reads it), 0xB on closing. All of 3-10 is this menu; only 3, 4 and 6 are lists.
  bool active() override { int s = state(); return sys() && s >= 3 && s <= 10; }
  bool holds_keys() override { int s = state(); return s == 3 || s == 4 || s == 6; }
  std::string title() override { return strings::kCampItems[1]; }
  void on_enter() override { screens::set_keys([this] { return std::string(state() == 3 ? strings::kItemListKeys() : strings::kListKeys()); }); }
  void build(ui::GraphBuilder& b) override {
    uint8_t* s = sys();
    if (!s) return;
    int st = state();
    int cat = i32(m(), 0x44);
    if (st == 3) {
      auto* items = arr<uint8_t>(s, 0x788);
      // The category (Q and E): a new list, so its own ids.
      std::string key = std::format("items.{}", cat);
      list_.key = key;
      b.begin_stop(key);
      b.push_context(cat >= 0 && cat < 5 ? strings::kItemCategories[cat] : "");
      for (int i = 0; items && i < items->num; ++i) {
        uint8_t* e = items->data + i * 0x10;
        int id = *(int32_t*)e, n = e[4];
        std::string name = items::name_of(id);
        if (name.empty()) name = std::format("{} {:#x}", strings::kItem, id);
        // Dimmed in the game when it cannot be used here: its own check (exe+0x131cf50(system, id, &skill); field
        // skills, skill cards with a Persona to take them, the few always-usable items) or none left.
        uint32_t skill = 0;
        bool usable = n > 0 && ((bool (*)(void*, uint16_t, uint32_t*))(scan::exe_base() + 0x131cf50))(s, (uint16_t)id, &skill);
        std::string label = std::format("{}, {}{}", name, n, usable ? "" : std::string(", ") + strings::kUnavailable);
        std::string help = items::help(id);   // its file's line, not the drawn page (built once per highlight)
        auto v = game_item(list_, i, [label] { return label; });
        buffer_with(v, [help] { return help; });
        b.add_item(slot(key, i), v);
      }
      if (!items) b.add_item(ui::ControlId::structural("items.none"), item([] { return std::string(strings::kNoItems); }));
      b.pop_context();
      if (int c = list_.cursor(); c >= 0) b.set_start(slot(key, c));
    } else if (st == 4) {
      auto* ids = arr<int16_t>(s, 0x768);
      b.begin_stop("item.member");
      b.push_context(strings::kUseOnWhom);
      for (int i = 0; ids && i < ids->num; ++i) {
        int id = ids->data[i];
        auto v = game_item(who_, i, [id] { return member_line(id, false); });
        v->announcements[0].live = true;   // the new HP or SP is read once the item lands
        b.add_item(slot("item.member", i), v);
      }
      b.pop_context();
      if (int c = who_.cursor(); c >= 0) b.set_start(slot("item.member", c));
    } else if (st == 6) {
      auto* slots = arr<int32_t>(s, 0x778);
      auto stock = persona::stock();
      b.begin_stop("item.persona");
      b.push_context(strings::kWhichPersona);
      for (int i = 0; slots && i < slots->num; ++i) {
        int sl = slots->data[i];
        std::string label;
        for (auto& o : stock) if (o.slot == sl) label = persona::describe(o.id, o.level);
        if (label.empty() && i < (int)stock.size()) label = persona::describe(stock[i].id, stock[i].level);
        b.add_item(slot("item.persona", i), game_item(pers_, i, [label] { return label; }));
      }
      b.pop_context();
      if (int c = pers_.cursor(); c >= 0) b.set_start(slot("item.persona", c));
    }
  }
};

std::unique_ptr<Screen> make_camp_item() { return std::make_unique<CampItem>(); }
}  // namespace p3a::screens

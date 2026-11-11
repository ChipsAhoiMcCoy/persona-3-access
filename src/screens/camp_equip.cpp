// The camp's Equip submenu as a bespoke menu (menu.h), modelled from the game's own objects (the offsets are this
// build's, measured 2026-10-30). Every list uses the generic ListCursor (util.h): index = row + top.
//
//   UCmpEquip +0x38 state (3 member, 5 slot, 8/0xB gear), +0x40 the member list's open timer, +0xF8 the member row's
//   copy, +0x280 the draw, +0x278 UCmpEquipSystem: +0x28 TArray<i16> members, +0x38 TArray<{u32 id, u32 count}> the
//   gear that fits, +0x58 the party panel. Cursors: member +0x44, slot +0x80, gear +0xBC.
//   Worn gear: unit +0x28C + slot * 2 (0x8000 empty).
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/camp.h"
#include "screens/camp_util.h"
#include "core/strings.h"
#include "items.h"
#include "party.h"
#include <format>

namespace p3a::screens {
using namespace ue;

class CampEquip : public MenuScreen {
  Finder menu_{"CmpEquip"};
  uint8_t* m() { UObject* o = camp_open() ? menu_.get() : nullptr; return (uint8_t*)o; }
  int state() { return i32(m(), 0x38); }
  uint8_t* sys() { uint8_t* o = m(); return o ? *(uint8_t**)(o + 0x278) : nullptr; }
  Mirror& who_ = mirror("equip.member", [this] { return state() == 3 ? ListCursor(m() + 0x44).index() : -1; });
  Mirror& slot_ = mirror("equip.slot", [this] { return state() == 5 ? ListCursor(m() + 0x80).index() : -1; });
  Mirror& gear_ = mirror("equip.gear", [this] { int s = state(); return s == 8 || s == 0xB ? ListCursor(m() + 0xBC).index() : -1; });
  int member() {
    uint8_t* s = sys();
    auto* ids = s ? arr<int16_t>(s, 0x28) : nullptr;
    int i = ListCursor(m() + 0x44).index();
    return ids && i >= 0 && i < ids->num ? ids->data[i] : 0;
  }
  int worn(int who, int sl) {
    uint8_t* u = party::unit_work(who);
    int id = u ? *(uint16_t*)(u + 0x28C + sl * 2) : 0x8000;
    return id == 0x8000 ? -1 : id;
  }
  std::string gear_line(int id) {
    std::string n = items::name_of(id);
    items::Info in;
    if (!n.empty() && items::find(n, in)) n += items::stats(in);
    return n;
  }
 public:
  // Each list's move as FUN_141309010 makes it (draw = +0x280, the party panel = sys +0x58).
  CampEquip() {
    // The member: the panel, the sound, and the worn gear shown for them (FUN_141304660).
    // The open timer +0x40 (FUN_141309010 case 3, what its confirm waits for).
    who_.ready = [this] { uint8_t* o = m(); return state() == 3 && open_timer_done(*(float*)(o + 0x40), *(void**)(o + 0x30)); };
    who_.set = [this](int i) {
      uint8_t* o = m();
      uint8_t* s = sys();
      ListCursor lc(o + 0x44);
      if (state() != 3 || !s || !lc.set(i)) return false;
      int row = row_of(o + 0x44);
      *(int32_t*)(o + 0xF8) = row;
      panel_to(*(uint8_t**)(s + 0x58), row, o);
      cursor_sound();
      fn<void (*)(void*, int)>(0x1304660)(o, fn<int (*)(void*, int)>(0x131b070)(s, lc.index()));
      return true;
    };
    // The slot: member 6's key hints (the weapon slot has its own layout), the sound and the slot bar's slide (FUN_1413051f0).
    slot_.set = [this](int i) {
      uint8_t* o = m();
      if (state() != 5 || !ListCursor(o + 0x80).set(i)) return false;
      if (member() == 6) key_help(i != 0 ? 5 : 6);
      cursor_sound();
      fn<void (*)(void*, int, char)>(0x13051f0)(*(void**)(o + 0x280), row_of(o + 0x80), 0);
      return true;
    };
    // The gear: the description's slide (given the item left, FUN_141305b50) and the bar (FUN_141306b40) in state 8,
    // the scroll arrows in 0xB; the NEW mark of the item left cleared; the sound.
    gear_.set = [this](int i) {
      uint8_t* o = m();
      uint8_t* s = sys();
      int st = state();
      ListCursor lc(o + 0xBC);
      int old = lc.index();
      if ((st != 8 && st != 0xB) || !s || !lc.set(i)) return false;
      uint8_t* draw = *(uint8_t**)(o + 0x280);
      uint32_t e[2] = {0, 0};
      fn<void (*)(void*, uint32_t*, int)>(0x131aec0)(s, e, old);
      bool down = i > old;
      if (st == 8) {
        fn<void (*)(void*, int, int, uint32_t)>(0x1305b50)(draw, down ? 0 : 1, 2, e[0]);
        fn<void (*)(void*, int, int, char)>(0x1306b40)(draw, row_of(o + 0xBC), 0, 0);
      }
      if (fn<bool (*)(uint16_t)>(0x11ef120)((uint16_t)e[0])) fn<void (*)(uint16_t)>(0x11ef620)((uint16_t)e[0]);
      cursor_sound();
      if (st == 0xB) fn<void (*)(void*)>(down ? 0x1304f50 : 0x1304fc0)(draw);
      return true;
    };
  }
  const char* id() const override { return "camp_equip"; }
  int priority() const override { return 17; }
  // Open from the member list (3) until it starts closing (0xD); between its lists the menu passes through states of
  // its own (FUN_141309010: 4 member -> slot, 6 slot -> member, 7 slot -> gear, 9 gear -> slot, 0xA / 0xC into and
  // out of an item's details, 0xB): the screen stays, silent, so nothing under it is read in those frames.
  bool active() override { int s = state(); return sys() && s >= 3 && s <= 0xC; }
  bool holds_keys() override { int s = state(); return s == 3 || s == 5 || s == 8 || s == 0xB; }
  std::string title() override { return strings::kCampItems[2]; }
  // The gear's details (F / X) are read with each item: the key does nothing (the user's call, 2026-11-01).
  bool blocks_status() override { int s = state(); return s == 8 || s == 0xB; }
  void on_enter() override {
    screens::set_keys([this] { int s = state(); return std::string(s == 5 ? strings::kEquipSlotKeys() : s == 3 ? strings::kListKeys() : strings::kEquipGearKeys()); });
  }
  void build(ui::GraphBuilder& b) override {
    uint8_t* s = sys();
    if (!s || !holds_keys()) return;
    int st = state();
    if (st == 3) {
      auto* ids = arr<int16_t>(s, 0x28);
      b.begin_stop("equip.member");
      for (int i = 0; ids && i < ids->num; ++i) {
        int id = ids->data[i];
        b.add_item(slot("equip.member", i), game_item(who_, i, [id] { return member_line(id, false); }));
      }
      if (int c = who_.cursor(); c >= 0) b.set_start(slot("equip.member", c));
      return;
    }
    int who = member();
    if (st == 5) {
      // Q and E switch the member here: each member's slots are their own list.
      std::string key = std::format("equip.slot.{}", who);
      slot_.key = key;
      b.begin_stop(key);
      b.push_context(party::name(who));
      for (int i = 0; i < 5; ++i) {
        int w = worn(who, i);
        std::string label = std::format("{}: {}", strings::kEquipSlots[i], w >= 0 ? gear_line(w) : std::string(strings::kEmptySlot));
        b.add_item(slot(key, i), game_item(slot_, i, [label] { return label; }));
      }
      b.pop_context();
      if (int c = slot_.cursor(); c >= 0) b.set_start(slot(key, c));
      return;
    }
    int sl = ListCursor(m() + 0x80).index();
    int w = sl >= 0 && sl < 5 ? worn(who, sl) : -1;
    auto* gear = arr<uint8_t>(s, 0x38);
    std::string key = std::format("equip.gear.{}.{}", who, sl);
    gear_.key = key;
    b.begin_stop(key);
    b.push_context(sl >= 0 && sl < 5 ? strings::kEquipSlots[sl] : "");
    for (int i = 0; gear && i < gear->num; ++i) {
      uint8_t* e = gear->data + i * 8;
      int id = *(int32_t*)e, n = *(int32_t*)(e + 4);
      std::string label = gear_line(id);
      if (id == w) label += std::string(", ") + strings::kEquipped;
      else label += std::format(", {} {}", n, strings::kOwned);
      // The help line from the slot's own file (items::help), not the drawn page: the game builds that page once (the
      // equipped item's while its slot is highlighted in the slot list), so a drawn-page read missed it.
      std::string help = items::help(id);
      auto v = game_item(gear_, i, [label] { return label; });
      buffer_with(v, [help] { return help; });
      b.add_item(slot(key, i), v);
    }
    b.pop_context();
    if (int c = gear_.cursor(); c >= 0) b.set_start(slot(key, c));
  }
};

std::unique_ptr<Screen> make_camp_equip() { return std::make_unique<CampEquip>(); }
}  // namespace p3a::screens

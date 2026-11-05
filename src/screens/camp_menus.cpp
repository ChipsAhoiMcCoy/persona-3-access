// The camp submenus as bespoke menus (menu.h), modelled from the game's own objects (2026-10-30
// the offsets below are this build's). Every list uses the generic ListCursor
// (util.h): index = row + top.
//
//   Item     UCmpItem +0x38 state (3 list, 4 member, 6 Persona), +0x44 category, +0x188 UCmpItemSystem:
//            +0x788 TArray<{u32 id, u8 count at +4, ...}> (0x10 each), +0x768 TArray<i16> party ids,
//            +0x778 TArray<i32> Persona stock slots. Cursors: list +0x84, member +0x48, Persona +0xC0.
//            Every cursor here is written directly, with what the game's own move calls after it (each `set`).
//   Skill    UCmpSkill +0x3C state (3 member, 4 skill, 5 target), +0x108 UCmpSkillSystem: +0x28 TArray<i16> members,
//            +0x38 TArray<{u16 persona, u16 skill, i32 HP cost, i32 SP cost}> (0xC each) of the member's field
//            skills. Cursors: member +0x48, skill +0x84, target +0xC0.
//   Equip    UCmpEquip +0x38 state (3 member, 5 slot, 8/0xB gear), +0x278 UCmpEquipSystem: +0x28 TArray<i16>
//            members, +0x38 TArray<{u32 id, u32 count}> the gear that fits. Cursors: member +0x44, slot +0x80,
//            gear +0xBC. Worn gear: unit +0x28C + slot * 2 (0x8000 empty).
//   Persona  UCmpPersona +0x38 == 3, actor +0x1D0, its PersonaList_ (+0x40 mode: 0 list, 1 status page; +0x48 a
//            pointer to its ListCursor). Entries: the stock in order; the equipped slot is the hero's unit +0x48.
//   Stats    UCmpStatus +0x44 != 0, Actor_ +0x58 (UUICmpStatus): +0x28 TArray<u16> ids, ListCursor +0x68. The
//            scene on top of SceneFSM_ (+0x2B0): CmpStatusCharacterList (+0x48 mode 1 list, 3 Tactics, 4 order;
//            +0xA0 a pointer to the orders' ListCursor), CmpCharacterStatus (a member's page: CampStatsPage).
//   Social   UCmpCommu +0x58 open, Actor_ +0xC50: +0x428 TArray<entry*> (the link records), +0x448 a pointer to the
//            ListCursor; the details page is the scene at +0x418 (+0x54 == 1).
//   Quest    UCmpQuest +0x3C open, +0x1C4 tab (0 requests, 1 missing persons). UIRequest: +0x34 state (8 list,
//            9 info), +0xD0 TArray<{u16 quest, u8 status, u8 new, ...}> (0xC each), ListCursor +0xE0.
//            UIMissingPerson: +0x34 state (6 list, 7 detail), +0xB48 TArray<{u16, u16 case}>, +0x538 a pointer to
//            its ListCursor.
//   System   UCmpSystem +0x54 == 4, ListCursor +0x64 (7 entries; locked by flags 0x40000089..8F).
#include "screens/menu.h"
#include "screens/util.h"
#include "bmdfile.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "gametext.h"
#include "items.h"
#include "msg.h"
#include "party.h"
#include "persona.h"
#include "social.h"
#include "stats.h"
#include "speech.h"
#include "textcap.h"
#include "ue/scan.h"
#include <algorithm>
#include <climits>
#include <chrono>
#include <cmath>
#include <format>
#include <map>
#include <tuple>

namespace p3a::screens {
void build_status_page(ui::GraphBuilder& b, const uint8_t* e, const std::string& lead, const std::string& bonus,
                       const uint8_t* before, const ui::ControlId* name_id = nullptr,
                       ui::NodeVtablePtr name_item = {});   // persona_status.cpp
using namespace ue;
bool camp_open();

namespace {
template <class T> TArray<T>* arr(void* base, int off) {
  auto* a = (TArray<T>*)((uint8_t*)base + off);
  return a->data && a->num > 0 && a->num < 2000 ? a : nullptr;
}
int i32(void* base, int off) { return base ? *(int32_t*)((uint8_t*)base + off) : -1; }
std::string member_line(int id) {
  party::Member m;
  if (party::member(id, m)) return party::line(m);
  return party::name(id);
}
// A game function by its rva (the `set`s below call what the game's own move calls).
template <class F> F fn(uintptr_t rva) { return (F)(scan::exe_base() + rva); }
int row_of(uint8_t* lc) { return *(int32_t*)(lc + 0x24); }
// The party panel's highlight on `row` and its slide (FUN_14130fed0 + FUN_141307760: Item, Skill, Equip members).
void panel_to(uint8_t* panel, int row, uint8_t* menu) {
  *(int32_t*)(panel + 8) = row;
  fn<void (*)(void*, int, void*, char)>(0x1307760)(panel, 0, *(void**)(menu + 0x30), 0);
}
}  // namespace

// ---------------------------------------------------------------- Item
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
        describe_with(v, [help] { return help; });
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
        auto v = game_item(who_, i, [id] { return member_line(id); });
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

// ---------------------------------------------------------------- Skill
class CampSkill : public MenuScreen {
  Finder menu_{"CmpSkill"};
  uint8_t* m() { UObject* o = camp_open() ? menu_.get() : nullptr; return (uint8_t*)o; }
  int state() { return i32(m(), 0x3C); }
  uint8_t* sys() { uint8_t* o = m(); return o ? *(uint8_t**)(o + 0x108) : nullptr; }
  Mirror& who_ = mirror("skill.member", [this] { return state() == 3 ? ListCursor(m() + 0x48).index() : -1; });
  Mirror& list_ = mirror("skills", [this] { return state() == 4 ? ListCursor(m() + 0x84).index() : -1; });
  Mirror& target_ = mirror("skill.target", [this] { return state() == 5 ? ListCursor(m() + 0xC0).index() : -1; });
  int member_id(int i) {
    auto* ids = sys() ? arr<int16_t>(sys(), 0x28) : nullptr;
    return ids && i >= 0 && i < ids->num ? ids->data[i] : 0;
  }
 public:
  CampSkill() {
    list_.set = [this](int i) { return state() == 4 && write_cursor(ListCursor(m() + 0x84), i, *(uint8_t**)(m() + 0x100), 0x133c620); };
    // The member's move (FUN_14133efb0 case 3): the sound, the party panel, that member's skills rebuilt
    // (FUN_141337140) and the skill cursor reset to the top (FUN_1410bec60).
    who_.ready = [this] { return state() == 3 && sys(); };
    who_.set = [this](int i) {
      uint8_t* o = m();
      uint8_t* s = sys();
      if (state() != 3 || !s || !ListCursor(o + 0x48).set(i)) return false;
      int row = row_of(o + 0x48);
      cursor_sound();
      panel_to(*(uint8_t**)(s + 0x50), row, o);
      fn<void (*)(void*, int)>(0x1337140)(s, member_id(row));
      fn<void (*)(void*, int, int, int, int, int)>(0x10bec60)(o + 0x84, 1, 10, *(int32_t*)(s + 0x40), 0, 0);
      return true;
    };
    // Only a skill for one member has a target cursor (FUN_14130c910(skill) == 0).
    target_.ready = [this] {
      uint8_t* s = sys();
      auto* sk = state() == 5 && s ? arr<uint8_t>(s, 0x38) : nullptr;
      int k = sk ? row_of(m() + 0x84) : -1;
      return sk && k >= 0 && k < sk->num && fn<short (*)(uint16_t)>(0x130c910)(*(uint16_t*)(sk->data + k * 0xC + 2)) == 0;
    };
    target_.set = [this](int i) {
      uint8_t* o = m();
      uint8_t* s = sys();
      if (state() != 5 || !s || !ListCursor(o + 0xC0).set(i)) return false;
      panel_to(*(uint8_t**)(s + 0x50), row_of(o + 0xC0), o);
      cursor_sound();
      return true;
    };
  }
  const char* id() const override { return "camp_skill"; }
  int priority() const override { return 17; }
  // 6 is the frame after a skill is used, back to 4 (FUN_14133efb0): still this menu, nothing to read.
  bool active() override { int s = state(); return sys() && s >= 3 && s <= 6; }
  bool holds_keys() override { int s = state(); return s == 3 || s == 4 || s == 5; }
  std::string title() override { return strings::kCampItems[0]; }
  void on_enter() override { screens::set_keys(strings::kListKeys); }
  void build(ui::GraphBuilder& b) override {
    uint8_t* s = sys();
    if (!s) return;
    int st = state();
    auto* ids = arr<int16_t>(s, 0x28);
    if (st == 3 || st == 5) {
      Mirror& mm = st == 3 ? who_ : target_;
      b.begin_stop(mm.key);
      if (st == 5) b.push_context(strings::kUseOnWhom);
      for (int i = 0; ids && i < ids->num; ++i) {
        int id = ids->data[i];
        auto v = game_item(mm, i, [id] { return member_line(id); });
        v->announcements[0].live = true;   // a healing skill's result (and the caster's SP) once it lands
        b.add_item(slot(mm.key, i), v);
      }
      if (st == 5) b.pop_context();
      if (int c = mm.cursor(); c >= 0) b.set_start(slot(mm.key, c));
      return;
    }
    // The member's skills (the hero lists every stock Persona's): name, cost (the game's, with its modifiers).
    int who = *(int16_t*)(s + 0x48);
    auto* sk = arr<uint8_t>(s, 0x38);
    std::string key = std::format("skills.{}", who);
    list_.key = key;
    b.begin_stop(key);
    b.push_context(party::name(who));
    for (int i = 0; sk && i < sk->num; ++i) {
      uint8_t* e = sk->data + i * 0xC;
      int id = *(uint16_t*)(e + 2), hp = *(int32_t*)(e + 4), sp = *(int32_t*)(e + 8);
      std::string label = persona::skill_name(id);
      if (sp > 0) label += std::format(", {} SP", sp);
      else if (hp > 0) label += std::format(", {} HP", hp);
      // Dimmed in the game when the cost is more than the member has.
      party::Member mm;
      if (party::member(who, mm) && ((sp > 0 && sp > mm.sp) || (hp > 0 && hp >= mm.hp))) label += std::string(", ") + strings::kNotEnough;
      auto v = game_item(list_, i, [label] { return label; });
      describe_with(v, [id] { return persona::skill_help(id); });
      b.add_item(slot(key, i), v);
    }
    b.pop_context();
    if (int c = list_.cursor(); c >= 0) b.set_start(slot(key, c));
  }
};

// ---------------------------------------------------------------- Equip
class CampEquip : public MenuScreen {
  Finder menu_{"CmpEquip"};
  uint8_t* m() { UObject* o = camp_open() ? menu_.get() : nullptr; return (uint8_t*)o; }
  int state() { return i32(m(), 0x38); }
  uint8_t* sys() { uint8_t* o = m(); return o ? *(uint8_t**)(o + 0x278) : nullptr; }
  Mirror& who_ = mirror("equip.member", [this] { return state() == 3 ? ListCursor(m() + 0x44).index() : -1; });
  Mirror& slot_ = mirror("equip.slot", [this] { return state() == 5 ? ListCursor(m() + 0x80).index() : -1; });
  Mirror& gear_ = mirror("equip.gear", [this] { int s = state(); return s == 8 || s == 0xB ? ListCursor(m() + 0xBC).index() : -1; });
  static constexpr int kSlotNibble[5] = {0, 1, 2, 3, 8};
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
    // The slot: the sound and the slot bar's slide (FUN_1413051f0).
    slot_.set = [this](int i) {
      uint8_t* o = m();
      if (state() != 5 || !ListCursor(o + 0x80).set(i)) return false;
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
        fn<void (*)(void*, int, int, uint32_t)>(0x1305b50)(draw, down ? 1 : 0, 2, e[0]);
        fn<void (*)(void*, int, int, char)>(0x1306b40)(draw, row_of(o + 0xBC), 0, 0);
      }
      if (fn<bool (*)(uint16_t)>(0x11ef120)((uint16_t)e[0])) fn<void (*)(uint16_t)>(0x11ef620)((uint16_t)e[0]);
      cursor_sound();
      if (st == 0xB) fn<void (*)(void*)>(down ? 0x1304fc0 : 0x1304f50)(draw);
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
        b.add_item(slot("equip.member", i), game_item(who_, i, [id] { return member_line(id); }));
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
      describe_with(v, [help] { return help; });
      b.add_item(slot(key, i), v);
    }
    b.pop_context();
    if (int c = gear_.cursor(); c >= 0) b.set_start(slot(key, c));
  }
};

// ---------------------------------------------------------------- Persona
class CampPersona : public MenuScreen {
  Finder menu_{"CmpPersona"};
  uint8_t* list() {
    UObject* o = camp_open() ? menu_.get() : nullptr;
    if (!o || i32(o, 0x38) != 3) return nullptr;
    UObject* actor = *(UObject**)((uint8_t*)o + 0x1D0);
    return actor ? (uint8_t*)get<UObject*>(actor, "PersonaList_") : nullptr;
  }
  Mirror& list_ = mirror("personas", [this] {
    uint8_t* l = list();
    return l && l[0x40] == 0 ? cursor_at(l, 0x48).index() : -1;
  });
 public:
  // The list's move (FUN_141327b90): the sound, the Persona loaded into the status widget (model, stats, skills:
  // FUN_1414ebc00), the bar (FUN_141323cb0) and the description's fade (FUN_141323730). Moves wait for the list's open
  // timer and for the widget's swap animation to be nearly done (FUN_1414e7b70(widget, 3)), as the game's do.
  CampPersona() {
    list_.ready = [this] {
      uint8_t* l = list();
      if (!l || l[0x40] != 0) return false;
      uint8_t* host = *(uint8_t**)(l + 0x88);
      int frames = fn<int (*)(void*)>(0x12e13b0)(*(void**)(host + 0x9D0));
      return *(float*)(l + 0x98) >= frames * (1.f / 30) && fn<bool (*)(void*, int)>(0x14e7b70)(*(void**)(l + 0x90), 3);
    };
    list_.set = [this](int i) {
      uint8_t* l = list();
      ListCursor lc = cursor_at(l, 0x48);
      if (!l || !lc.set(i)) return false;
      int idx = lc.index();
      cursor_sound();
      fn<void (*)(void*, int, char, char, char, char)>(0x14ebc00)(*(void**)(l + 0x90), idx, 0, 0, 1, 1);
      fn<void (*)(void*, int, char)>(0x1323cb0)(l, idx, 0);
      fn<void (*)(void*)>(0x1323730)(*(void**)(*(uint8_t**)(l + 0x88) + 0x9D8));
      return true;
    };
  }
  const char* id() const override { return "camp_persona"; }
  int priority() const override { return 17; }
  bool active() override { uint8_t* l = list(); return l && l[0x40] == 0; }   // mode 1: the status page's screen
  std::string title() override { return strings::kCampItems[3]; }
  void on_enter() override { screens::set_keys(strings::kCampPersonaKeys); }
  // Q / E (LB / RB): the previous / next Persona, its page from the top (the game's list does not wrap; ours does).
  bool takes_bumpers() override { return true; }
  bool blocks_status() override { return true; }   // the game's status page (F / X) would only repeat this one
  void on_bumper(int dir) override {
    int n = (int)persona::stock().size(), c = list_.cursor();
    if (n < 2 || c < 0 || !list_.ready()) return;
    int to = ((c + dir) % n + n) % n;
    if (list_.set(to)) focus(slot("personas", to));
  }
  // The page of the Persona under the game's cursor (the status page's, F): its name line is the list's row (Enter
  // equips it), then its sections; the triggers jump them.
  void build(ui::GraphBuilder& b) override {
    auto stock = persona::stock();
    int c = list_.cursor();
    if (c < 0 || c >= (int)stock.size()) return;
    int sl = stock[c].slot;
    auto label = [sl] {
      std::string l;
      for (auto& o : persona::stock()) if (o.slot == sl) l = persona::describe(o.id, o.level);
      if (sl == persona::equipped()) l += std::string(", ") + strings::kEquipped;
      return l;
    };
    ui::ControlId id = slot("personas", c);
    build_status_page(b, persona::stock_entry(sl), {}, {}, nullptr, &id, game_item(list_, c, label));
    b.set_start(id);
  }
};

// ---------------------------------------------------------------- Stats (the party list and Tactics)
class CampStats : public MenuScreen {
  Finder menu_{"CmpStatus"};
  uint8_t* ui() {
    UObject* o = camp_open() ? menu_.get() : nullptr;
    return o && i32(o, 0x44) != 0 ? *(uint8_t**)((uint8_t*)o + 0x58) : nullptr;
  }
  // The scene on top of the UI's scene stack (UUISceneFSM.SceneStack_).
  UObject* scene() {
    uint8_t* u = ui();
    UObject* fsm = u ? *(UObject**)(u + 0x2B0) : nullptr;
    auto* st = fsm ? field<TArray<UObject*>>(fsm, "SceneStack_") : nullptr;
    return st && st->num > 0 ? st->data[st->num - 1] : nullptr;
  }
  int mode() { UObject* sc = scene(); return sc && is_a(sc, "CmpStatusCharacterList") ? i32(sc, 0x48) : -1; }
  Mirror& list_ = mirror("stats", [this] { int md = mode(); return md == 1 || md == 3 ? ListCursor(ui() + 0x68).index() : -1; });
  Mirror& order_ = mirror("stats.order", [this] { return mode() == 4 ? cursor_at(scene(), 0xA0).index() : -1; });
  std::vector<std::string> orders_;   // the tactics' names as drawn (sprites have none in memory): seen once listed
  int tactic(int id) { uint8_t* u = party::unit_work(id); return u ? *(uint8_t*)(u + 0x296) : -1; }
  std::string tactic_name(int t) {
    if (t >= 0 && t < (int)orders_.size()) return orders_[t];
    return t >= 0 && t < 5 ? strings::kTacticNames[t] : std::string();
  }
  // The UI layer has input (FUN_1412d5f90, the gate the scene's own moves pass).
  bool input_on() { uint8_t* u = ui(); return u && fn<bool (*)(void*)>(0x12d5f90)(*(void**)(u + 0x330)); }
 public:
  CampStats() {
    // The party list's move in one call (FUN_141352af0): the sound, the panel, the member shown, the camera.
    list_.ready = [this] { int md = mode(); return (md == 1 || md == 3) && input_on(); };
    list_.set = [this](int i) {
      uint8_t* u = ui();
      int md = mode();
      if ((md != 1 && md != 3) || !ListCursor(u + 0x68).set(i)) return false;
      fn<void (*)(void*, void*, int)>(0x1352af0)(scene(), *(void**)(u + 0x60), row_of(u + 0x68));
      return true;
    };
    // The order list: FUN_141352c40, the sound and the bar.
    order_.ready = [this] { return mode() == 4 && input_on(); };
    order_.set = [this](int i) {
      if (mode() != 4 || !cursor_at(scene(), 0xA0).set(i)) return false;
      fn<void (*)(void*)>(0x1352c40)(scene());
      return true;
    };
  }
  const char* id() const override { return "camp_stats"; }
  int priority() const override { return 17; }
  // The list scene in any mode is this menu (its other modes are the steps between the lists); only 1, 3 and 4 read.
  bool active() override { return mode() >= 1; }
  bool holds_keys() override { int md = mode(); return md == 1 || md == 3 || md == 4; }
  std::string title() override { return strings::kCampItems[4]; }
  void on_enter() override {
    screens::set_keys([this] { int md = mode(); return std::string(md == 1 ? strings::kStatsKeys() : md == 3 ? strings::kTacticsListKeys() : strings::kListKeys()); });
  }
  void on_tick() override {
    if (mode() != 4) return;
    // The order list is drawn on the right (x ~1204, from y 310): its names, top to bottom.
    std::vector<textcap::Item> rows;
    for (auto& t : textcap::snapshot(100))
      if (t.source == 2 && t.x > 1100 && t.y >= 290 && core::has_words(t.text)) rows.push_back(t);
    std::sort(rows.begin(), rows.end(), [](auto& a, auto& b) { return a.y < b.y; });
    std::vector<std::string> names;
    for (auto& r : rows) {
      std::string n = core::clean_text(r.text);
      if (std::find(names.begin(), names.end(), n) == names.end()) names.push_back(n);
    }
    if (names.size() >= 2) orders_ = names;
  }
  void build(ui::GraphBuilder& b) override {
    uint8_t* u = ui();
    if (!u || !holds_keys()) return;
    int md = mode();
    auto* ids = arr<uint16_t>(u, 0x28);
    if (md == 4) {
      ListCursor lc = cursor_at(scene(), 0xA0);
      b.begin_stop("stats.order");
      b.push_context(strings::kTacticsWhich);
      for (int i = 0; i < lc.count(); ++i)
        b.add_item(slot("stats.order", i), game_item(order_, i, [this, i] {
          std::string n = tactic_name(i);
          return n.empty() ? std::format("{} {}", strings::kTactic, i + 1) : n;
        }));
      b.pop_context();
      if (int c = order_.cursor(); c >= 0) b.set_start(slot("stats.order", c));
      return;
    }
    // The party list, or (E) the same members with their tactics.
    std::string key = md == 3 ? "stats.tactics" : "stats";
    list_.key = key;
    b.begin_stop(key);
    if (md == 3) b.push_context(strings::kTacticsHeading);
    for (int i = 0; ids && i < ids->num; ++i) {
      int id = ids->data[i];
      if (md == 3) {
        b.add_item(slot(key, i), game_item(list_, i, [this, id] {
          std::string t = tactic_name(tactic(id));
          return t.empty() ? party::name(id) : std::format("{}: {}", party::name(id), t);
        }));
      } else {
        auto persona_line = [id] {
          const uint8_t* e = party::persona_entry(id);
          return e ? persona::status_line(e) : std::string();
        };
        auto v = game_item(list_, i, [id] { return member_line(id); }, persona_line);
        describe_with(v, persona_line);
        b.add_item(slot(key, i), v);
      }
    }
    if (md == 3) b.pop_context();
    if (int c = list_.cursor(); c >= 0) b.set_start(slot(key, c));
  }
};

// ---------------------------------------------------------------- Social Link
class CampSocial : public MenuScreen {
  Finder menu_{"CmpCommu"};
  uint8_t* actor() {
    UObject* o = camp_open() ? menu_.get() : nullptr;
    return o && ((uint8_t*)o)[0x58] ? *(uint8_t**)((uint8_t*)o + 0xC50) : nullptr;
  }
  bool details() {
    uint8_t* a = actor();
    uint8_t* d = a ? *(uint8_t**)(a + 0x418) : nullptr;
    return d && i32(d, 0x54) == 1;
  }
  Mirror& list_ = mirror("links", [this] { uint8_t* a = actor(); return a && !details() ? cursor_at(a, 0x448).index() : -1; });
  // The list scene (UCmpCommuList, actor +0x410): +0x44 == 3 while it takes input.
  uint8_t* scene() { uint8_t* a = actor(); return a ? *(uint8_t**)(a + 0x410) : nullptr; }
 public:
  // The link's move (FUN_14130bee0 from the list scene's tick): the menu's previous / page entries, the link changed
  // event (the card in hand, FUN_1412f1800 on actor +0x2B0) and the bar with the sound (FUN_1412f2d40).
  CampSocial() {
    list_.ready = [this] { uint8_t* a = actor(); uint8_t* sc = scene(); return sc && i32(sc, 0x44) == 3 && a[0x2AD]; };
    list_.set = [this](int i) {
      uint8_t* a = actor();
      uint8_t* sc = scene();
      ListCursor lc = cursor_at(a, 0x448);
      int old = lc.index();
      if (!sc || !lc.set(i)) return false;
      uint8_t* c = *(uint8_t**)(a + 0x460);   // UCmpCommu
      *(int32_t*)(c + 0x4C) = fn<int (*)(void*)>(0x1300ba0)(c);
      *(int32_t*)(c + 0x48) = old;
      fn<void (*)(void*, void*, int)>(0x12f1800)(a + 0x2B0, fn<void* (*)(void*)>(0x1300ec0)(a), i);
      fn<void (*)(void*, int)>(0x12f2d40)(sc, row_of(lc.p));
      return true;
    };
  }
  const char* id() const override { return "camp_social"; }
  int priority() const override { return 17; }
  bool active() override { uint8_t* a = actor(); return a && arr<void*>(a, 0x428) && !details(); }
  std::string title() override { return strings::kCampItems[6]; }
  void on_enter() override { screens::set_keys(strings::kListKeys); }
  void build(ui::GraphBuilder& b) override {
    uint8_t* a = actor();
    auto* links = a ? arr<uint8_t*>(a, 0x428) : nullptr;
    if (!links) return;
    auto all = social::read();
    b.begin_stop("links");
    for (int i = 0; i < links->num; ++i) {
      uint8_t* e = links->data[i];
      int arcana = e ? *(uint16_t*)e : 0;
      social::Link l{arcana, 0, 0};
      for (auto& x : all) if (x.arcana == arcana) l = x;
      std::string label = social::describe(l);
      b.add_item(slot("links", i), game_item(list_, i, [label] { return label; }));
    }
    if (int c = list_.cursor(); c >= 0) b.set_start(slot("links", c));
  }
};

// ---------------------------------------------------------------- Quest: Elizabeth's requests and missing persons
class CampQuest : public MenuScreen {
 public:
  CampQuest() {
    // A request's move (FUN_1415a9370 state 8): the sound, the NEW mark of the request left cleared, the highlighted
    // request (+0x300), the bar (FUN_141585740) and Elizabeth's reaction to its status (FUN_141587280). Not while
    // a prompt is up (+0x5F8) or the list is leaving (+0xD60 / +0xD61).
    reqs_.ready = [this] { uint8_t* r = req(); return r && !r[0xD61] && r[0xD60] != 1 && i32(r, 0x5F8) == -1; };
    reqs_.set = [this](int i) {
      uint8_t* r = req();
      auto* list = r ? arr<uint8_t>(r, 0xD0) : nullptr;
      ListCursor lc(r ? r + 0xE0 : nullptr);
      int old = lc.index();
      if (!list || i >= list->num || old < 0 || old >= list->num || !lc.set(i)) return false;
      cursor_sound();
      fn<void (*)(uint16_t, char)>(0x1505690)(*(uint16_t*)(list->data + old * 0xC), 0);
      list->data[old * 0xC + 3] = 0;
      *(int32_t*)(r + 0x300) = i;
      fn<void (*)(void*)>(0x1585740)(r);
      fn<void (*)(void*, char, char, char, char)>(0x1587280)(r, r[0x309], list->data[i * 0xC + 2], 1, 0);
      return true;
    };
    // A case's move (FUN_14149c750 state 6): the bar (FUN_141490460, two floats), the sound, the case left marked
    // seen, the highlighted case (+0x728).
    cases_.ready = [this] { uint8_t* r = missing(); return r && !r[0x4C] && !r[0x4D]; };
    cases_.set = [this](int i) {
      uint8_t* r = missing();
      ListCursor lc = cursor_at(r, 0x538);
      if (!r || !lc.set(i)) return false;
      int old = i32(r, 0x728), n = i32(r, 0xB50);
      fn<void (*)(void*, float, float)>(0x1490460)(r + 0x548, *(float*)(r + 0x40), row_of(lc.p) * 60.f + *(float*)(r + 0x44));
      if (n != 0 && old != i) {
        cursor_sound();
        if (old >= 0 && old < n) {
          uint32_t f = *(uint16_t*)(*(uint8_t**)(r + 0xB48) + old * 4 + 2) + 0x20000E95u;
          void* g = fn<void* (*)()>(0x122ac90)();
          if (!fn<bool (*)(void*, uint32_t)>(0x122a080)(g, f)) fn<void (*)(void*, uint32_t, int)>(0x1230410)(g, f, 1);
        }
      }
      *(int32_t*)(r + 0x728) = i;
      return true;
    };
  }
  void on_enter() override {
    screens::set_keys([this] {
      return std::string(velvet() ? strings::kRequestKeys() : board() ? strings::kMissingBoardKeys()
                         : tab() == 1 ? strings::kMissingPersonsKeys() : strings::kCampRequestKeys());
    });
  }
 private:
  Finder menu_{"CmpQuest"}, req_{"UIRequest"}, missing_{"UIMissingPerson"}, velvet_{"UIVelvetRoomRequest"};
  Finder quests_data_{"DisappearDataAsset"};
  bool open() { UObject* q = camp_open() ? menu_.get() : nullptr; return q && ((uint8_t*)q)[0x3C]; }
  // Elizabeth's own counter in the Velvet Room shows the same list (UIRequest +0x309 = 0 there: accept and report).
  bool velvet() { return !open() && velvet_.get() != nullptr; }
  // The Missing Persons board on its own (AUIMissingPersonActor: UIMissingPerson +0x531 = 1), outside the camp.
  bool board() { uint8_t* r = missing(); return !open() && r && r[0x531]; }
  int tab() { if (board()) return 1; if (velvet()) return 0; UObject* q = menu_.get(); return q ? i32(q, 0x1C4) : 0; }
  uint8_t* req() { UObject* r = req_.get(); return r && i32(r, 0x34) == 8 ? (uint8_t*)r : nullptr; }
  uint8_t* missing() { UObject* r = missing_.get(); return r && i32(r, 0x34) == 6 ? (uint8_t*)r : nullptr; }
  Mirror& reqs_ = mirror("requests", [this] { uint8_t* r = req(); return r && tab() == 0 ? ListCursor(r + 0xE0).index() : -1; });
  Mirror& cases_ = mirror("cases", [this] { uint8_t* r = missing(); return r && tab() == 1 ? cursor_at(r, 0x538).index() : -1; });
  std::string quest_title(int id) {
    auto f = bmdfile::load("BMD_Quest");
    if (!f || !f->ok || id < 0 || id >= (int)f->dialogs.size()) return std::format("{} {}", strings::kNumber, id);
    auto& d = f->dialogs[id];
    if (d.speaker >= 0 && d.speaker < (int)f->speakers.size()) return core::clean_text(f->speakers[d.speaker]);
    return std::format("{} {}", strings::kNumber, id);
  }
  std::string quest_page(int id, int page, bool* gap = nullptr) {
    auto f = bmdfile::load("BMD_Quest");
    if (!f || !f->ok || id < 0 || id >= (int)f->dialogs.size() || page >= (int)f->dialogs[id].pages.size()) return {};
    auto& pg = f->dialogs[id].pages[page];
    // Table 4 functions are text the game fills in (an item's name): the file's text has a hole there.
    if (gap) { *gap = false; for (auto& fn : pg.functions) if (fn.table == 4) *gap = true; }
    return core::clean_text(core::bmd::plain(pg));
  }
 public:
  const char* id() const override { return "camp_quest"; }
  int priority() const override { return 21; }
  bool active() override { return (open() || velvet() || board()) && ((tab() == 0 && req()) || (tab() == 1 && missing())); }
  // A request's prompt (accept? / the report) is a message window over the list: the keys are its.
  bool holds_keys() override { uint8_t* r = req(); return tab() == 1 || !r || *(int32_t*)(r + 0x5F8) == -1; }
  // A case's profile is read with each row: the game's details key (F / X) does nothing there. A request's info (F)
  // stays the game's: Elizabeth's text names the item only on the page the game builds for that view.
  bool blocks_status() override { return tab() == 1; }
  std::string title() override { return tab() == 1 ? strings::kMissingPersons : strings::kRequests; }
  void build(ui::GraphBuilder& b) override {
    if (tab() == 0) {
      uint8_t* r = req();
      auto* list = r ? arr<uint8_t>(r, 0xD0) : nullptr;
      b.begin_stop("requests");
      for (int i = 0; list && i < list->num; ++i) {
        uint8_t* e = list->data + i * 0xC;
        int id = *(uint16_t*)e, status = e[2];
        std::string label = status == 5 ? std::string("???") : quest_title(id);
        if (status >= 0 && status < 5) label += std::string(", ") + strings::kRequestStatus[status];
        auto v = game_item(reqs_, i, [label] { return label; }, [this, id, status] {
          return status == 5 ? std::string() : quest_page(id, 1);
        });
        // The task after the title, as the game shows it beside the list. The message file's text leaves out what the
        // game fills in (an item's name: "Show Elizabeth a ."); the page the game built for the highlighted request
        // has it, so that one is read once the game's cursor is there.
        if (status != 5) {
          bool gap = false;
          std::string task = quest_page(id, 1, &gap);
          describe_with(v, [this, i, task, gap] {
            std::string fallback = gap ? std::string() : task;
            if (reqs_.seen != i || reqs_.target >= 0) return fallback;
            std::string head = task.substr(0, std::min<size_t>(task.size(), 8));
            for (auto& pg : msg::visible(500)) { std::string t = core::clean_text(pg.text); if (!head.empty() && t.rfind(head, 0) == 0) return t; }
            return fallback;
          });
        }
        b.add_item(slot("requests", i), v);
      }
      if (int c = reqs_.cursor(); c >= 0) b.set_start(slot("requests", c));
      return;
    }
    uint8_t* r = missing();
    auto* list = r ? arr<uint32_t>(r, 0xB48) : nullptr;
    auto f = bmdfile::load("BMD_DisappearHelp");
    // Each case's state from DisappearDataAsset (a TMap: elements of 0x30, key at +0, flags Active, Success,
    // Failed, Npc, InfoOn, Award at +8; missing.cpp), and its name only once InfoOn is set.
    std::map<int, const int32_t*> flags;
    if (UObject* d = quests_data_.get()) {
      auto* map = (TArray<uint8_t>*)((uint8_t*)d + 0x30);
      for (int i = 0; map->data && i < map->num && i < 200; ++i) flags[*(uint16_t*)(map->data + i * 0x30)] = (int32_t*)(map->data + i * 0x30 + 8);
    }
    b.begin_stop("cases");
    for (int i = 0; list && i < list->num; ++i) {
      int no = (list->data[i] >> 16) & 0xFFFF;
      std::string name = strings::kUnknown, profile, status;
      const int32_t* fl = flags.count(no) ? flags[no] : nullptr;
      if (fl) status = party::flag(fl[1]) ? strings::kMissingFound : party::flag(fl[2]) ? strings::kMissingDiscontinued : strings::kMissingSearching;
      if ((!fl || party::flag(fl[4])) && f && f->ok && no < (int)f->dialogs.size()) {
        auto& dlg = f->dialogs[no];
        if (dlg.speaker >= 0 && dlg.speaker < (int)f->speakers.size()) name = core::clean_text(f->speakers[dlg.speaker]);
        if (!dlg.pages.empty()) profile = core::clean_text(core::bmd::plain(dlg.pages[0]));
      }
      std::string label = std::format("{} {}, {}", strings::kNumber, no, name);
      if (!status.empty()) label += ", " + status;
      auto v = game_item(cases_, i, [label] { return label; }, [profile] { return profile; });
      describe_with(v, [profile] { return profile; });
      b.add_item(slot("cases", i), v);
    }
    if (int c = cases_.cursor(); c >= 0) b.set_start(slot("cases", c));
  }
};

// ---------------------------------------------------------------- System
class CampSystem : public MenuScreen {
  Finder menu_{"CmpSystem"};
  uint8_t* m() { UObject* o = camp_open() ? menu_.get() : nullptr; return o && i32(o, 0x54) == 4 ? (uint8_t*)o : nullptr; }
  Mirror& list_ = mirror("system", [this] { uint8_t* o = m(); return o ? ListCursor(o + 0x64).index() : -1; });
 public:
  // The draw reads the cursor every frame; the move adds its row copy (+0xA0), the help line's fade and the sound.
  CampSystem() {
    list_.set = [this](int i) {
      uint8_t* o = m();
      if (!o || !ListCursor(o + 0x64).set(i)) return false;
      *(int32_t*)(o + 0xA0) = row_of(o + 0x64);
      if (uint8_t* d = *(uint8_t**)(o + 0xA8)) *(uint64_t*)(d + 0x13E4) = 0;
      cursor_sound();
      return true;
    };
  }
  const char* id() const override { return "camp_system"; }
  int priority() const override { return 17; }
  bool active() override { return m() != nullptr; }
  std::string title() override { return strings::kCampItems[8]; }
  void on_enter() override { screens::set_keys(strings::kListKeys); }
  void build(ui::GraphBuilder& b) override {
    b.begin_stop("system");
    for (int i = 0; i < 7; ++i) {
      bool locked = party::flag(0x40000089 + i);
      std::string label = strings::kSystemItems[i];
      if (locked) label += std::string(", ") + strings::kUnavailable;
      b.add_item(slot("system", i), game_item(list_, i, [label] { return label; }));
    }
    if (int c = list_.cursor(); c >= 0) b.set_start(slot("system", c));
  }
};


// ---------------------------------------------------------------- Calendar
// UCmpCalendar +0x3C open, its actor +0x48 (AUICmpCalendar): +0x2A8 0 on the day grid, 1 on the day's plans; the day
// cursor (UCmpCldCursor, actor +0x2F8): +0xDC the day as an absolute number, held between +0xE0 and +0xE4. Up / down
// move a week, left / right a day; Q / E a month. The draw (UICmpCalendarDraw) holds the selected date: +0x188 month,
// +0x190 year, +0x1A4 day (measured 2026-10-27). The mod lays the month out as the game does, weeks as rows of seven
// (Sunday first), and steps the game's cursor through the same moves.
// A day's events are the game's data, so any day reads without the game's cursor on it (the panel is drawn from the
// same rows, and while it crossfades the last day's text is still drawn): CldDateMessageDataAsset, the game's common
// resource 0x2B (FUN_140fc85f0 -> FUN_1415695e0). Data (+0x30) a TArray per month (index = month), each of 0x14-byte
// rows: +0 Key, +4 TotalDay (u16, the cursor's absolute day), +6 month, +7 day, +8 Period, +0xC MsgLabel (a dialog of
// BMD_CalendarDateMessages), +0x10 VisibleFlag (shown when 0 or set). As FUN_14104ec60; the day-changed handler
// (FUN_1412b1a30) hands the old and new day's rows to the draw (+0x1F0 / +0x200). No row: the file's dialog 0
// ("Nothing in particular"), as the game shows. (The part-time jobs under them, ArbeitInfoDataAsset at draw +0x1E0,
// are drawn as images.) Measured 2026-10-31: April 29 = TotalDay 28, MsgLabel 7 "Showa Day".
// The grid itself (FUN_1412b9030) marks each day's number red for a holiday and draws a moon on four ages of
// CldDateDataAsset's 6-byte day rows (+2 MoonAge): 6 first quarter, 14 full (animated), 22 last quarter, 29 new.
class CampCalendar : public MenuScreen {
  Finder menu_{"CmpCalendar"}, draw_{"UICmpCalendarDraw"};
  uint8_t* actor() {
    UObject* o = camp_open() ? menu_.get() : nullptr;
    return o && ((uint8_t*)o)[0x3C] ? *(uint8_t**)((uint8_t*)o + 0x48) : nullptr;
  }
  uint8_t* cursor_obj() { uint8_t* a = actor(); return a ? *(uint8_t**)(a + 0x2F8) : nullptr; }
  Mirror& days_ = mirror("days", [this] { uint8_t* a = actor(); uint8_t* c = cursor_obj(); return a && c && i32(a, 0x2A8) == 0 ? i32(c, 0xDC) : -1; });
  // The cursor's day as a date: (absolute day) -> year, month, day. The month and day are the game's own split of the
  // cursor (FUN_14104c5d0: day % 365 through a month-length table from April), so they never lag it. Only the school
  // year (April to March) comes from the draw, whose selected date follows the cursor a frame or two late; the
  // calendar spans one school year, so that never changes under a move. Reading the draw's date itself labelled the
  // month a day off on the frame of a move: across a month's end the old month was built with the new day in it, and
  // its old date was said before the new month's (April 30, then May 1; 2026-11-01).
  bool anchor(int& abs, std::chrono::sys_days& date) {
    uint8_t* c = cursor_obj();
    uint8_t* b = (uint8_t*)draw_.get();
    if (!c || !b) return false;
    int dm = *(int32_t*)(b + 0x188), dy = *(int32_t*)(b + 0x190);
    if (dm < 1 || dm > 12 || dy < 2000) return false;
    abs = i32(c, 0xDC);
    int m = 0, d = 0;
    fn<void (*)(int, int*, int*)>(0x104c5d0)(abs, &m, &d);
    if (m < 1 || m > 12 || d < 1 || d > 31) return false;
    int year = dy - (dm <= 3) + (m <= 3);
    date = std::chrono::sys_days{std::chrono::year_month_day{std::chrono::year{year}, std::chrono::month{(unsigned)m}, std::chrono::day{(unsigned)d}}};
    return true;
  }
  // A day's events, by absolute day (cached per opening: the rows and their flags hold while the calendar is open).
  std::unordered_map<int, std::string> events_;
  std::string events(int day) {
    if (auto it = events_.find(day); it != events_.end()) return it->second;
    void* res = fn<void* (*)()>(0xfc85f0)();
    UObject* a = res ? fn<UObject* (*)(void*, uint8_t)>(0x15695e0)(res, 0x2B) : nullptr;
    auto f = bmdfile::load("BMD_CalendarDateMessages");
    if (!a || !ue::is_a(a, "CldDateMessageDataAsset") || !f || !f->ok) return {};
    auto text = [&](int label) {
      return label >= 0 && label < (int)f->dialogs.size() && !f->dialogs[label].pages.empty()
                 ? core::clean_text(core::bmd::plain(f->dialogs[label].pages[0])) : std::string();
    };
    void* g = fn<void* (*)()>(0x122ac90)();
    uint8_t* months = *(uint8_t**)((uint8_t*)a + 0x30);
    int n = i32((uint8_t*)a, 0x38);
    std::string out;
    for (int m = 0; months && m < n; m++) {
      uint8_t* rows = *(uint8_t**)(months + m * 0x10);
      int count = i32(months + m * 0x10, 8);
      for (int r = 0; rows && r < count; r++) {
        uint8_t* row = rows + r * 0x14;
        uint32_t flag = *(uint32_t*)(row + 0x10);
        if (*(uint16_t*)(row + 4) != day || (flag && !(g && fn<bool (*)(void*, uint32_t)>(0x122a080)(g, flag)))) continue;
        std::string t = text(i32(row, 0xC));
        if (!t.empty() && out.find(t) == std::string::npos) out += (out.empty() ? "" : ", ") + t;
      }
    }
    if (out.empty()) out = text(0);
    for (auto& j : jobs(day)) out += std::format(", {}: {}", strings::kJob, j);
    return events_[day] = out;
  }
  // The part-time jobs a day takes, as the calendar lights them. The draw's list (+0x1E0, count +0x1E8) holds the
  // unlocked jobs (ArbeitInfoDataAsset rows, 0x18 bytes: +0 HelpMSGLabel, +4 OrderableTimes, +5 OrderableWeeks, +8
  // UnlockFLG); each row is drawn lit when FUN_14104bf20(_, job, 1) holds for the calendar's selected day (calendar
  // FUN_1410511c0(1) +0xC0, which the day-changed handler writes), grey otherwise. The job number is FUN_1415264e0's:
  // UnlockFLG 0x15 -> 0, 0x1B -> 2, else 1. The check runs here with that day written for the call and put back.
  std::vector<std::string> jobs(int day) {
    std::vector<std::string> out;
    UObject* d = draw_.get();
    uint8_t* cal = fn<uint8_t* (*)(char)>(0x10511c0)(1);
    if (!d || !cal) return out;
    uint8_t* rows = *(uint8_t**)((uint8_t*)d + 0x1E0);
    int n = i32((uint8_t*)d, 0x1E8);
    int held = i32(cal, 0xC0);
    for (int r = 0; rows && r < n && r < 8; r++) {
      uint8_t* row = rows + r * 0x18;
      int flag = i32(row, 8), label = i32(row, 0), times = row[4] & 3;
      uint32_t job = flag == 0x15 ? 0 : flag == 0x1B ? 2 : 1;
      *(int32_t*)(cal + 0xC0) = day;
      bool lit = fn<bool (*)(void*, uint32_t, char)>(0x104bf20)(nullptr, job, 1);
      *(int32_t*)(cal + 0xC0) = held;
      if (!lit || label < 0 || label >= (int)std::size(strings::kJobs)) continue;
      out.push_back(times ? std::format("{}, {}", strings::kJobs[label], strings::kJobTimes[times]) : strings::kJobs[label]);
    }
    return out;
  }
  // The day's plans list (mode 1) is read as drawn.
  std::string plans(unsigned max_age = 200) {
    std::vector<textcap::Item> cols;
    for (auto& t : textcap::snapshot(max_age)) if (t.source == 2 && t.x > 1000 && t.y > 550 && (t.color & 0xff) && core::has_words(t.text)) cols.push_back(t);
    std::sort(cols.begin(), cols.end(), [](auto& a, auto& b) { return a.y < b.y; });
    std::string out;
    for (auto& t : cols) if (out.find(core::clean_text(t.text)) == std::string::npos) out += (out.empty() ? "" : ", ") + core::clean_text(t.text);
    return out;
  }
 public:
  CampCalendar() {
    // The cursor's move (FUN_1412cc8d0): the day written, the month-changed event when it crosses one, the sound, the
    // day-changed event (the draw's highlight and selected date follow it). Moves wait for the grid's open timer and
    // the draw's per-move cooldown (draw +0x250 against FUN_1412bfdc0), as the game's do.
    days_.ready = [this] {
      uint8_t* a = actor();
      uint8_t* d = a && i32(a, 0x2A8) == 0 ? *(uint8_t**)(a + 0x300) : nullptr;
      if (!d) return false;
      int open = fn<int (*)(void*)>(0x12e13b0)(*(void**)(fn<uint8_t* (*)()>(0x122ac90)() + 0x308A0));
      return *(float*)(a + 0x308) >= open * (1.f / 30) && *(float*)(d + 0x250) > fn<int (*)(void*)>(0x12bfdc0)(d) * (1.f / 30);
    };
    days_.set = [this](int i) {
      uint8_t* c = cursor_obj();
      if (!c || i < i32(c, 0xE0) || i > i32(c, 0xE4)) return false;
      int old = i32(c, 0xDC);
      int m0 = 0, m1 = 0, x = 0;
      auto split = fn<void (*)(int, int*, int*)>(0x104c5d0);
      auto fire = fn<void (*)(void*, int, int)>(0x12b32d0);
      split(old, &m0, &x);
      *(int32_t*)(c + 0xDC) = i;
      split(i, &m1, &x);
      if (m0 != m1) fire(c + 0x100, m0, m1);
      cursor_sound();
      fire(c + 0xE8, old, i);
      return true;
    };
  }
  const char* id() const override { return "camp_calendar"; }
  int priority() const override { return 17; }
  // F on a day with plans opens them as a list on the right (mode 1): the game's keys; the plans are read as they change.
  bool active() override { uint8_t* a = actor(); return a && cursor_obj(); }
  bool holds_keys() override { uint8_t* a = actor(); return a && i32(a, 0x2A8) == 0; }
  std::string plans_said_;
  void on_tick() override {
    uint8_t* a = actor();
    if (!a || i32(a, 0x2A8) != 1) { plans_said_.clear(); return; }
    std::string p = plans();
    if (!p.empty() && p != plans_said_) { plans_said_ = p; speech::speak(p, true); }
  }
  // Today's absolute day (GetNowTotalDay counts as the cursor does), read on entry: the date holds while it is open.
  int today_ = -1;
  void on_enter() override {
    events_.clear(); today_ = calendar::now().total;
    screens::set_keys([this] { uint8_t* a = actor(); return std::string(a && i32(a, 0x2A8) == 1 ? strings::kCalendarPlansKeys() : strings::kCalendarKeys()); });
  }
  std::string title() override { return strings::kCampItems[7]; }
  void build(ui::GraphBuilder& b) override {
    int abs; std::chrono::sys_days date;
    uint8_t* c = cursor_obj();
    if (!c || !anchor(abs, date)) return;
    int lo = i32(c, 0xE0), hi = i32(c, 0xE4);
    std::chrono::year_month_day ymd{date};
    auto first = std::chrono::sys_days{ymd.year() / ymd.month() / std::chrono::day{1}};
    auto last = std::chrono::sys_days{ymd.year() / ymd.month() / std::chrono::last};
    std::string key = std::format("days.{}", (unsigned)ymd.month());
    days_.key = key;
    b.begin_stop(key);
    // Raw nodes with the game's own moves as edges: left / right a day, up / down a week.
    std::vector<int> ids;
    for (auto di = first; di <= last; di += std::chrono::days{1}) {
      int i = abs + (int)(di - date).count();
      if (i < lo || i > hi) continue;
      std::chrono::year_month_day d{di};
      calendar::Now n;
      n.month = (int)(unsigned)d.month(); n.day = (int)(unsigned)d.day(); n.weekday = (int)std::chrono::weekday{di}.c_encoding(); n.valid = true;
      // The grid's marks for the day (FUN_1412b9030): the number red for a holiday (FUN_1412b0d50: 2 red, 1 blue for
      // Saturday; a Sunday is always red), and a moon on the quarters, the full moon and the new moon (FUN_141050530,
      // the day's ECldMoonAge from CldDateDataAsset).
      n.holiday = n.weekday != 0 && fn<uint8_t (*)(void*, int)>(0x12b0d50)(nullptr, i) == 2;
      std::string label = calendar::describe(n);
      if (std::string m = calendar::moon_mark(fn<uint8_t (*)(int)>(0x1050530)(i)); !m.empty()) label += ", " + m;
      if (i == today_) label += std::format(", {}", strings::kToday);
      auto v = game_item(days_, i, [label] { return label; }, [this, i] { return events(i); });
      describe_with(v, [this, i] { return events(i); });
      b.add_node(slot(key, i), v);
      ids.push_back(i);
    }
    auto has = [&](int i) { return std::find(ids.begin(), ids.end(), i) != ids.end(); };
    for (int i : ids) {
      if (has(i + 1)) { b.connect(slot(key, i), ui::GraphDir::Right, slot(key, i + 1)); b.connect(slot(key, i + 1), ui::GraphDir::Left, slot(key, i)); }
      if (has(i + 7)) { b.connect(slot(key, i), ui::GraphDir::Down, slot(key, i + 7)); b.connect(slot(key, i + 7), ui::GraphDir::Up, slot(key, i)); }
    }
    if (int cc = days_.cursor(); cc >= 0) b.set_start(slot(key, cc));
  }
};

std::unique_ptr<Screen> make_camp_calendar() { return std::make_unique<CampCalendar>(); }

// ---------------------------------------------------------------- Social Link: a link's page (its people)
// The details scene (Social Link actor +0x418): +0x54 == 1 while up, +0x58 the arcana, +0x68 a pointer to the people's
// ListCursor, +0x78 TArray of the people (0x20 each: +0x8 the name FString; read 2026-10-30). RB / LB (Q / E) switch
// the link. The link's memo is BMD_CmmOutlineHelp dialog arcana * 4 (+1 lover, +2 reversed), page rank - 1; a person's
// profile is the page drawn beside them.
class CampSocialDetails : public MenuScreen {
  Finder menu_{"CmpCommu"};
  uint8_t* details() {
    UObject* o = camp_open() ? menu_.get() : nullptr;
    uint8_t* a = o && ((uint8_t*)o)[0x58] ? *(uint8_t**)((uint8_t*)o + 0xC50) : nullptr;
    uint8_t* d = a ? *(uint8_t**)(a + 0x418) : nullptr;
    return d && i32(d, 0x54) == 1 ? d : nullptr;
  }
  Mirror& people_ = mirror("people", [this] { uint8_t* d = details(); return d ? cursor_at(d, 0x68).index() : -1; });
  social::Link link() {
    uint8_t* d = details();
    int arcana = d ? i32(d, 0x58) : 0;
    social::Link l{arcana, 0, 0};
    for (auto& x : social::read()) if (x.arcana == arcana) l = x;
    return l;
  }
  // Drawn pages are told apart by their file's buffer: dialog numbers repeat across files (the Magician's memo,
  // dialog 4, is also Yukari's name in the profile file). What a page showed is kept: the game builds it once, so a
  // later read (Backslash, coming back to the line) found nothing on screen.
  std::map<std::tuple<int, int, bool>, std::string> memos_;   // (arcana, rank, reversed) -> the memo drawn
  std::map<std::pair<int, std::string>, std::string> profiles_;   // (arcana, name) -> the profile drawn
  // The memo: BMD_CmmOutlineHelp dialog arcana * 4 (+1 lover, +2 reversed), page rank - 1. The drawn one when seen
  // (only the game knows the lover variant), else the file's.
  std::string memo() {
    social::Link l = link();
    bool rev = social::reversed(l.arcana);
    int dialog = l.arcana * 4 + (rev ? 2 : 0);
    auto key = std::make_tuple(l.arcana, l.rank, rev);
    if (const void* buf = bmdfile::buffer("BMD_CmmOutlineHelp"))
      for (auto& p : msg::visible(1000))
        if (p.buf == buf && (p.dialog == dialog || p.dialog == dialog + 1)) memos_[key] = core::clean_text(p.text);
    if (auto it = memos_.find(key); it != memos_.end()) return it->second;
    auto f = bmdfile::load("BMD_CmmOutlineHelp");
    int pg = l.rank - 1;
    if (!f || !f->ok || dialog >= (int)f->dialogs.size() || pg < 0 || pg >= (int)f->dialogs[dialog].pages.size()) return {};
    return bmdfile::text(f->dialogs[dialog].pages[pg]);
  }
  // A person's profile: BMD_CmmProfileHelp's dialogs from their MSG_PRF_<NAME>_NAME (the name as listed) up to the
  // next person's (the profile, then _REVERSE, _MISSING, _LOST variants). The drawn one of those once the game's
  // cursor is on them (only the game knows which variant), else the file's plain or reversed one.
  std::string profile(int i, const std::string& name) {
    social::Link l = link();
    auto key = std::make_pair(l.arcana, name);
    auto f = bmdfile::load("BMD_CmmProfileHelp");
    if (!f || !f->ok) return profiles_.contains(key) ? profiles_[key] : std::string();
    int first = -1, end = -1;
    for (int d = 0; d < (int)f->dialogs.size(); ++d) {
      auto& dl = f->dialogs[d];
      if (!dl.name.ends_with("_NAME")) continue;
      if (first >= 0) { end = d; break; }
      if (!dl.pages.empty() && bmdfile::text(dl.pages[0]) == name) first = d;
    }
    // A name the file does not list as drawn: any page of the profile file built with the cursor on them.
    if (first < 0) { first = 0; end = INT_MAX; }
    else if (end < 0) end = (int)f->dialogs.size();
    if (people_.seen == i && people_.target < 0)
      if (const void* buf = bmdfile::buffer("BMD_CmmProfileHelp"))
        for (auto& p : msg::visible(1000))
          if (p.buf == buf && p.dialog > first && p.dialog < end && p.dialog < (int)f->dialogs.size() &&
              !f->dialogs[p.dialog].name.ends_with("_NAME"))
            profiles_[key] = core::clean_text(p.text);
    if (auto it = profiles_.find(key); it != profiles_.end()) return it->second;
    if (end == INT_MAX) return {};
    int pick = first + 1;
    if (social::reversed(l.arcana))
      for (int d = first + 1; d < end; ++d) if (f->dialogs[d].name.ends_with("_REVERSE")) pick = d;
    if (pick >= end || f->dialogs[pick].pages.empty()) return {};
    return bmdfile::text(f->dialogs[pick].pages[0]);
  }
 public:
  CampSocialDetails() {
    // The bar's slide and the sound (FUN_1413043b0), what the details scene's move does.
    people_.set = [this](int i) {
      uint8_t* d = details();
      if (!d || !cursor_at(d, 0x68).set(i)) return false;
      fn<void (*)(void*)>(0x13043b0)(d);
      return true;
    };
  }
  const char* id() const override { return "camp_social_details"; }
  int priority() const override { return 17; }
  bool active() override { return details() != nullptr; }
  // No title: the first line, where the page opens, names the link and its rank.
  int arcana_ = -1;
  void on_enter() override { uint8_t* d = details(); arcana_ = d ? i32(d, 0x58) : -1; screens::set_keys(strings::kSocialPageKeys); }
  void on_tick() override {
    uint8_t* d = details();
    int a = d ? i32(d, 0x58) : -1;
    if (a > 0 && a != arcana_ && arcana_ > 0) focus(ui::ControlId::structural(std::format("people.{}.memo", a)));
    if (a > 0) arcana_ = a;
  }
  void build(ui::GraphBuilder& b) override {
    uint8_t* d = details();
    auto* ppl = d ? (TArray<uint8_t>*)(d + 0x78) : nullptr;
    if (!ppl || !ppl->data || ppl->num <= 0 || ppl->num > 16) return;
    std::string key = std::format("people.{}", i32(d, 0x58));
    if (people_.key != key) {   // another link (Q / E): its page opens on its own line too (on_tick), not on a member
      people_.key = people_.last_key = key;
      people_.seen = people_.target = people_.step_from = -1;
    }
    b.begin_stop(key);
    // The link itself first: its rank and memo, read on the way in.
    std::string head = social::describe(link());
    b.add_item(ui::ControlId::structural(key + ".memo"), item([this, head] { std::string m = memo(); return m.empty() ? head : head + ". " + m; }));
    for (int i = 0; i < ppl->num; ++i) {
      std::string n = core::clean_text(to_utf8(*(FString*)(ppl->data + i * 0x20 + 8)));
      auto v = game_item(people_, i, [n] { return n; });
      describe_with(v, [this, i, n] { return profile(i, n); });
      b.add_item(slot(key, i), v);
    }
    // The page opens on the link's own line, not the game's cursor on its first member (2026-10-31).
    b.set_start(ui::ControlId::structural(key + ".memo"));
  }
};

std::unique_ptr<Screen> make_camp_social_details() { return std::make_unique<CampSocialDetails>(); }

// ---------------------------------------------------------------- Theurgy (outside battle)
// UITheurgia: +0x34 state (0xF browsing, 0x10 a Theurgy's detail), +0xD8 mode (1 = opened from a Stats member's
// page, FUN_141351c00; offered once a party member knows a Theurgy, UUICmpStatus +0x58), +0xEBC busy; the
// carousel +0x140 (count +0x148), 0xA0 each: +0x84 the skill id, +0x9E new. The cursor +0x114 moves with left /
// right and wraps; the carousel is laid out again only on a move (the `set` runs the game's). Q / E switch the character
// (+0xCC the current one's id; +0x1A8 TArray<short> ids).
class TheurgyMenu : public MenuScreen {
  Finder ui_{"UITheurgia"};
  uint8_t* u() { UObject* o = ui_.get(); return o && (i32(o, 0x34) == 0xF || i32(o, 0x34) == 0x10) ? (uint8_t*)o : nullptr; }
  bool personality() { UObject* o = ui_.get(); return o && i32(o, 0x34) == 0x10; }
  // F: the character's personality (what fills their Theurgy gauge), BMD_TheurgiaFlavorText dialog = character id.
  static std::string personality_text(int who) {
    auto f = bmdfile::load("BMD_TheurgiaFlavorText");
    if (!f || !f->ok || who <= 0 || who >= (int)f->dialogs.size() || f->dialogs[who].pages.empty()) return {};
    return core::clean_text(core::bmd::plain(f->dialogs[who].pages[0]));
  }
  bool personality_said_ = false;
  Mirror& list_ = mirror("theurgy", [this] { uint8_t* p = u(); return p ? i32(p, 0x114) : -1; });
  int who() { uint8_t* p = u(); return p ? i32(p, 0xCC) : 0; }
 public:
  // The game's own select-entry handler (FUN_14151b180, an input delegate): the index, the sound, the carousel laid
  // out again, the entry left marked seen. Not on the index already there (it buzzes) nor while closing (+0xDD).
  TheurgyMenu() {
    list_.keep_edges = true;   // the game's row wraps
    list_.ready = [this] { uint8_t* p = u(); return p && i32(p, 0x34) == 0xF && !p[0xDD]; };
    list_.set = [this](int i) {
      uint8_t* p = u();
      auto* a = p ? arr<uint8_t>(p, 0x140) : nullptr;
      if (!a || i32(p, 0x34) != 0xF || p[0xDD] || i < 0 || i >= a->num) return false;
      if (i32(p, 0x114) == i) return true;
      return fn<bool (*)(void*, void*, int)>(0x151b180)(p, nullptr, i);
    };
  }
  const char* id() const override { return "theurgy"; }
  int priority() const override { return 18; }
  bool active() override { uint8_t* p = u(); return p && arr<uint8_t>(p, 0x140); }
  // States 2 to 0xD: the "Theurgy obtained" scene a Social Link event plays (MASTERY_THEURGIA_DRAW): each card said
  // once, from the carousel's new entry (+0x9E) or the one under the cursor. One scene can show several: the cursor
  // moves to each (Ken's awakening, 10/6: Divine Retribution, then Divine Intervention at +0x114 = 1, the flags
  // cleared; the second went unsaid, 2026-11-03).
  int obtained_said_ = 0;
  void background() override {
    UObject* o = ui_.get();
    int st = o ? i32(o, 0x34) : 0;
    if (st < 2 || st > 0xD) { if (st == 0 || st >= 0x11) obtained_said_ = 0; return; }
    auto* a = arr<uint8_t>(o, 0x140);
    if (!a) return;
    int pick = i32(o, 0x114);
    for (int i = 0; i < a->num; ++i) if (a->data[i * 0xA0 + 0x9E]) pick = i;
    if (pick < 0 || pick >= a->num) return;
    int id = *(uint16_t*)(a->data + pick * 0xA0 + 0x84);
    if (id == obtained_said_) return;
    obtained_said_ = id;
    std::string d = persona::skill_help(id);
    speech::speak(std::format("{}: {}{}", strings::kTheurgyObtained, persona::skill_name(id), d.empty() ? "" : ". " + d), true);
  }
  std::string title() override { return strings::kTheurgy; }
  void on_enter() override { screens::set_keys([this] { return std::string(personality() ? strings::kCBack() : strings::kTheurgyListKeys()); }); }
  bool holds_keys() override { return !personality(); }
  void on_tick() override {
    if (!personality()) { personality_said_ = false; return; }
    if (personality_said_) return;
    personality_said_ = true;
    std::string t = personality_text(who());
    if (!t.empty()) speech::speak(std::format("{}: {}", strings::kTheurgyGauge, t), true);
  }
  void build(ui::GraphBuilder& b) override {
    uint8_t* p = u();
    auto* a = p ? arr<uint8_t>(p, 0x140) : nullptr;
    if (!a) return;
    list_.count = a->num;
    std::string key = std::format("theurgy.{}", who());
    list_.key = key;
    b.begin_stop(key);
    b.push_context(party::name(who()));
    b.start_row(key);
    for (int i = 0; i < a->num; ++i) {
      uint8_t* e = a->data + i * 0xA0;
      int id = *(uint16_t*)(e + 0x84);
      std::string l = persona::skill_name(id);
      if (e[0x9E]) l += std::string(", ") + strings::kMailNew;
      auto v = game_item(list_, i, [l] { return l; });
      describe_with(v, [id] { return persona::skill_help(id); });
      b.add_item(slot(key, i), v);
    }
    b.end_row();
    b.pop_context();
    if (int c = list_.cursor(); c >= 0) b.set_start(slot(key, c));
  }
};

std::unique_ptr<Screen> make_theurgy_menu() { return std::make_unique<TheurgyMenu>(); }

// ---------------------------------------------------------------- Stats: a member's page
// The scene CmpCharacterStatus (or CmpHeroHumanStatus) on top of the Stats UI's scene stack; the member is the party
// list's (UUICmpStatus +0x28 ids, ListCursor +0x68, which RB / LB, Q / E, move while the page is up). A page of lines:
// the member, their Persona's stats and each skill with its description, its affinities; the hero's social stats. In
// sections (menu.h): the member, their Persona, Stats one per line, Skills, Affinities; Tab / RT and LT jump them.
class CampStatsPage : public MenuScreen {
  Finder menu_{"CmpStatus"};
  uint8_t* ui() {
    UObject* o = camp_open() ? menu_.get() : nullptr;
    return o && i32(o, 0x44) != 0 ? *(uint8_t**)((uint8_t*)o + 0x58) : nullptr;
  }
  UObject* scene() {
    uint8_t* u = ui();
    UObject* fsm = u ? *(UObject**)(u + 0x2B0) : nullptr;
    auto* st = fsm ? field<TArray<UObject*>>(fsm, "SceneStack_") : nullptr;
    return st && st->num > 0 ? st->data[st->num - 1] : nullptr;
  }
  int member() {
    uint8_t* u = ui();
    auto* ids = u ? arr<uint16_t>(u, 0x28) : nullptr;
    int c = u ? ListCursor(u + 0x68).index() : -1;
    return ids && c >= 0 && c < ids->num ? ids->data[c] : 0;
  }
  int shown_ = 0;
  static std::string skill_help(int id) { return persona::skill_help(id); }
 public:
  const char* id() const override { return "camp_stats_page"; }
  int priority() const override { return 17; }
  bool active() override {
    UObject* sc = scene();
    return sc && (is_a(sc, "CmpCharacterStatus") || is_a(sc, "CmpHeroHumanStatus"));
  }
  std::string title() override { return strings::kCampItems[4]; }
  void on_enter() override { screens::set_keys(strings::kStatsPageKeys); }
  bool game_triggers() override { return true; }
  void on_tick() override {
    int who = member();
    if (who && shown_ && who != shown_) focus(ui::ControlId::structural(std::format("member.{}.line", who)));   // Q / E
    if (who) shown_ = who;
  }
  void build(ui::GraphBuilder& b) override {
    int who = member();
    if (!who) return;
    std::string k = std::format("member.{}", who);
    b.begin_stop(k);
    b.set_region(k + ".member");
    b.add_item(ui::ControlId::structural(k + ".line"), item([who] { return member_line(who); }));
    if (who == 1) b.add_item(ui::ControlId::structural(k + ".social"), item([] { return stats::describe(); }));
    // A Persona: its name with the member's lines, then its sections: Stats (one per line), Skills, Affinities.
    auto persona_sections = [&](int pid, int lv, const uint8_t* st, auto skill_at) {
      b.add_item(ui::ControlId::structural(k + ".persona"), item([pid, lv] { return persona::describe(pid, lv); }));
      b.set_region(k + ".stats");
      b.push_context(strings::kStatsTitle);
      for (int i = 0; i < 5; ++i) {
        std::string l = std::format("{} {}", strings::kStatNames[i], st[i]);
        b.add_item(ui::ControlId::structural(std::format("{}.stat{}", k, i)), item([l] { return l; }));
      }
      b.pop_context();
      b.set_region(k + ".skills");
      b.push_context(strings::kSkills);
      for (int i = 0; i < 8; ++i) if (int sid = skill_at(i)) {
        std::string sl = persona::skill_name(sid), cost = persona::skill_cost(sid);
        if (!cost.empty()) sl += ", " + cost;
        auto v = item([sl] { return sl; }, {}, [sid] { return skill_help(sid); });
        describe_with(v, [sid] { return skill_help(sid); });
        b.add_item(ui::ControlId::structural(std::format("{}.skill{}", k, i)), v);
      }
      b.pop_context();
      if (auto groups = persona::affinity_groups(pid); !groups.empty()) {
        b.set_region(k + ".aff");
        b.push_context(strings::kAffinitiesTitle);
        for (size_t i = 0; i < groups.size(); ++i) {
          std::string g = groups[i];
          g[0] = (char)toupper((unsigned char)g[0]);
          b.add_item(ui::ControlId::structural(std::format("{}.aff{}", k, i)), item([g] { return g; }));
        }
        b.pop_context();
      }
    };
    const uint8_t* e = party::persona_entry(who);
    if (who == 1) {   // the hero's equipped Persona from the stock
      int eq = persona::equipped();
      for (auto& o : persona::stock()) if (o.slot == eq) persona_sections(o.id, o.level, o.stats, [&o](int i) { return (int)o.skills[i]; });
    } else if (e && *(uint16_t*)(e + 2)) {
      persona_sections(*(uint16_t*)(e + 2), *(uint16_t*)(e + 4), e + 0x1C, [e](int i) { return (int)*(uint16_t*)(e + 0xC + i * 2); });
    }
  }
};

std::unique_ptr<Screen> make_camp_stats_page() { return std::make_unique<CampStatsPage>(); }
std::unique_ptr<Screen> make_camp_item() { return std::make_unique<CampItem>(); }
std::unique_ptr<Screen> make_camp_skill() { return std::make_unique<CampSkill>(); }
std::unique_ptr<Screen> make_camp_equip() { return std::make_unique<CampEquip>(); }
std::unique_ptr<Screen> make_camp_persona() { return std::make_unique<CampPersona>(); }
std::unique_ptr<Screen> make_camp_stats() { return std::make_unique<CampStats>(); }
std::unique_ptr<Screen> make_camp_social() { return std::make_unique<CampSocial>(); }
std::unique_ptr<Screen> make_camp_quest() { return std::make_unique<CampQuest>(); }
std::unique_ptr<Screen> make_camp_system() { return std::make_unique<CampSystem>(); }
}  // namespace p3a::screens

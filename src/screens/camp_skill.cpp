// The camp's Skill submenu as a bespoke menu (menu.h), modelled from the game's own objects (the offsets are this
// build's, measured 2026-10-30). Every list uses the generic ListCursor (util.h): index = row + top.
//
//   UCmpSkill +0x3C state (3 member, 4 skill, 5 target), +0x44 the member list's open timer, +0x100 the draw (the
//   skill bar's slide), +0x108 UCmpSkillSystem: +0x28 TArray<i16> members, +0x38 TArray<{u16 persona, u16 skill,
//   i32 HP cost, i32 SP cost}> (0xC each) of the member's field skills (count also at +0x40), +0x48 the member they
//   are of, +0x50 the party panel. Cursors: member +0x48, skill +0x84, target +0xC0.
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/camp.h"
#include "screens/camp_util.h"
#include "core/strings.h"
#include "party.h"
#include "persona.h"
#include <format>

namespace p3a::screens {
using namespace ue;

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
    // The open timer +0x44 (FUN_14133efb0 case 3, what its confirm waits for).
    who_.ready = [this] { uint8_t* o = m(); return state() == 3 && sys() && open_timer_done(*(float*)(o + 0x44), *(void**)(o + 0x30)); };
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
        auto v = game_item(mm, i, [id] { return member_line(id, false); });
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
      if (sp > 0) label += std::format(", {} {}", sp, strings::kSP);
      else if (hp > 0) label += std::format(", {} {}", hp, strings::kHP);
      // Dimmed in the game when the cost is more than the member has.
      party::Member mm;
      if (party::member(who, mm) && ((sp > 0 && sp > mm.sp) || (hp > 0 && hp >= mm.hp))) label += std::string(", ") + strings::kNotEnough;
      auto v = game_item(list_, i, [label] { return label; });
      buffer_with(v, [id] { return persona::skill_help(id); });
      b.add_item(slot(key, i), v);
    }
    b.pop_context();
    if (int c = list_.cursor(); c >= 0) b.set_start(slot(key, c));
  }
};

std::unique_ptr<Screen> make_camp_skill() { return std::make_unique<CampSkill>(); }
}  // namespace p3a::screens

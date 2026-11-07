// A Persona learning a skill (a skill card from the camp Item menu; a level up when its eight slots are full): the
// Persona status page runs a learning sequence (APersonaStatus.Sequence_: PersonaStatusSkillCardSeq and its
// relatives) instead of the plain page (+0x528 stays 0). Native state, measured 2026-10-28 with Maragi on Satan:
// +0x460 the new skill's id (u16), +0x464 the cursor over the eight slots (0-3 the left column, 4-7 the right, the
// entry's skill order), +0x468 the previous cursor. The skills themselves are sprites: names come from the shown
// entry (APersonaStatusDraw +0x34C) and the game's skill table.
#include "screens/menu.h"
#include "screens/util.h"
#include "core/strings.h"
#include "persona.h"
#include "speech.h"
#include <windows.h>
#include <format>

namespace p3a::screens {
using namespace ue;
namespace { bool g_learned = false; }
// A skill was learned here since the screen under it (the Velvet Room's level up) was last entered: that entry does
// not talk over "Learned ...". Taking it clears it.
bool take_skill_learned() { bool l = g_learned; g_learned = false; return l; }

// The eight slots as the game lays them out: two columns of four (0-3 left, 4-7 right).
class SkillLearnScreen : public MenuScreen {
  Finder ps_{"PersonaStatus"};
  // The cursor (+0x464) also reaches 9, the next skill to come (+0x358, while +0x458 > 0), and 10, the new skill itself
  // (its info card): up from the top row.
  Mirror& slots_ = mirror("slots", [this] {
    UObject* p = ps_.get();
    int c = p && new_skill(p) ? *(int32_t*)((uint8_t*)p + 0x464) : -1;
    return (c >= 0 && c < 8) || c == 9 || c == 10 ? c : -1;
  });
  int shown_ = 0, skill_ = 0;   // shown_: the Persona id on the page (Q and E switch Personas); skill_: being learned
  WeakObj page_;                // the status page skill_ belongs to: a page left without learning never claims a later one
  // The skill being learned: +0x460 (a skill card, a battle's level up), or, in a fusion's level up (the Velvet Room,
  // its Social Link bonus: +0x460 stays 0), the next skill at +0x358 while the cursor offers it (slot 9; seen
  // 2026-10-31: Mithras 24 -> 28, eight slots full, Tentarafoo waiting with the cursor on it).
  static int new_skill(UObject* p) {
    auto* q = (uint8_t*)p;
    if (int sid = *(uint16_t*)(q + 0x460)) return sid;
    int c = *(int32_t*)(q + 0x464);
    return *(int32_t*)(q + 0x458) > 0 && ((c >= 0 && c < 8) || c == 9) ? *(uint16_t*)(q + 0x358) : 0;
  }
  static bool card(UObject* p) { return *(uint16_t*)((uint8_t*)p + 0x460) != 0; }   // the new skill has its own card (slot 10)
  bool learning(UObject* p) {
    UObject* seq = get<UObject*>(p, "Sequence_");
    if (!seq) return false;
    std::string c = name_of(class_of(seq));
    // A battle's level up (BtlResult UpdatePersonaStatus): the same page, its own sequence; only while a skill waits
    // for a slot, so a plain level up stays the Persona status page's.
    if (c == "PersonaStatusLvUpSeq") return (skill_ && WeakObj::of(p) == page_) || new_skill(p);
    return c == "PersonaStatusSkillCardSeq" || c == "PersonaStatusChangeSkillSeq" || c == "PersonaStatusRememberSkillSeq" ||
           c == "PersonaStatusSelectSkillSeq";
  }
  // The page's own skill list (APersonaStatus +0x310: 8 of {u32 flag, u32 skill}, count +0x354), as drawn: a fusion's
  // level up adds the skills it learned there (Agilao, Hama) before the entry has them (2026-10-31). Else the entry.
  std::string slot_text(int i) {
    const uint8_t* e = persona::shown_status();
    auto* p = (uint8_t*)ps_.get();
    if (!e || !p) return {};
    int n = *(int32_t*)(p + 0x354);
    int sid = n > 0 && n <= 8 ? (i < n ? (int)*(uint32_t*)(p + 0x310 + i * 8 + 4) : 0) : *(uint16_t*)(e + 0xC + i * 2);
    if (!sid) return strings::kEmptySlot;
    std::string s = persona::skill_name(sid), cost = persona::skill_cost(sid);
    return cost.empty() ? s : s + ", " + cost;
  }
 public:
  // The status page's own select-slot handler (exe+0x14c8cb0, its mouse path): +0x464, the sound and the draw's
  // highlight slide. It refuses slots 9 / 10 when they are not offered and any slot while a popup is up.
  SkillLearnScreen() {
    slots_.keep_edges = true;   // the game's rows wrap between the two columns
    // The select handler refuses on the sequence's virtual +0x280 (the Forget popup, FUN_1414f76d0): moves wait it out.
    slots_.ready = [this] {
      UObject* p = ps_.get();
      UObject* seq = p ? get<UObject*>(p, "Sequence_") : nullptr;
      return seq && !(*(bool (***)(void*))seq)[0x280 / 8](seq);
    };
    slots_.set = [this](int i) {
      UObject* p = ps_.get();
      return p && ((bool (*)(void*, void*, int))(scan::exe_base() + 0x14c8cb0))(p, nullptr, i);
    };
  }
  const char* id() const override { return "skill_learn"; }
  // Over the camp Item list and the Velvet Room (a fusion's level up); never up with the plain Persona status page.
  int priority() const override { return 20; }
  bool active() override {
    UObject* p = ps_.get();
    return p && !persona::status_page_up(p) && learning(p) && persona::shown_status();   // Sequence_ clears on close
  }
  std::string learn_line() {
    UObject* p = ps_.get();
    const uint8_t* e = persona::shown_status();
    int sid = p ? new_skill(p) : 0;
    if (!e || !sid) return {};
    return std::format("{}: {} {}", persona::name(*(uint16_t*)(e + 2)), strings::kLearnSkill, persona::skill_name(sid));
  }
  // Before the first press the line says it.
  std::string title() override { return slots_.cursor() < 0 ? std::string() : learn_line(); }
  void on_enter() override {
    UObject* p = ps_.get();
    const uint8_t* e = persona::shown_status();
    if (!p || !e) return;
    shown_ = *(uint16_t*)(e + 2);
    if (!(WeakObj::of(p) == page_)) skill_ = 0;
    if (int sid = new_skill(p)) { skill_ = sid; page_ = WeakObj::of(p); screens::set_keys(strings::kLearnSkillKeys); }
  }
  void on_tick() override {
    UObject* p = ps_.get();
    const uint8_t* e = persona::shown_status();
    if (!p || !e) return;
    if (int id = *(uint16_t*)(e + 2); id && id != shown_) { shown_ = id; speech::speak(persona::describe(id, *(uint16_t*)(e + 4)), true); }
    // The new skill's id clears once it is learned (the cursor goes to -1); the page then waits for C.
    if (skill_ && new_skill(p) == 0) {
      screens::set_keys(strings::kCBack);
      speech::speak(std::format("{} {}", strings::kLearned, persona::skill_name(skill_)), true);
      g_learned = true;
      skill_ = 0;
    }
  }
  void build(ui::GraphBuilder& b) override {
    if (slots_.cursor() < 0) {
      // A card's page before its first press has no cursor yet, and with no line the menu never opened: the page said
      // nothing (2026-11-06). Its press is the game's. Learned: nothing to choose.
      std::string l = learn_line();
      if (l.empty()) return;
      b.begin_stop("card");
      int sid = new_skill(ps_.get());
      auto v = item([l] { return l; });
      describe_with(v, [sid] { return persona::skill_help(sid); });
      b.add_item(ui::ControlId::structural("card"), v);
      return;
    }
    b.begin_stop("slots");
    UObject* p = ps_.get();
    int sid = new_skill(p);
    b.start_row("slots");
    if (card(p) && *(int32_t*)((uint8_t*)p + 0x458) > 0) {
      int next = *(uint16_t*)((uint8_t*)p + 0x358);
      auto v = game_item(slots_, 9, [next] { return std::format("{}: {}", strings::kNextSkill, persona::skill_name(next)); });
      v->speaks_own_position = true;
      b.add_item(slot("slots", 9), v);
    }
    {
      // Up from the top row: the new skill's own card (its cost and description on the right; seen 2026-10-30). Not a
      // way out: C declines. A fusion's level up offers it as the next skill instead (slot 9).
      int at = card(p) ? 10 : 9;
      auto v = game_item(slots_, at, [sid] {
        std::string c = persona::skill_cost(sid);
        return std::format("{}: {}{}", strings::kNewSkill, persona::skill_name(sid), c.empty() ? "" : ", " + c);
      });
      describe_with(v, [sid] { return persona::skill_help(sid); });
      v->speaks_own_position = true;
      b.add_item(slot("slots", at), v);
    }
    b.end_row();
    // Two columns side by side: rows of two (slot r beside slot r + 4), arrows move as on screen.
    for (int r = 0; r < 4; ++r) {
      b.start_row("slots");
      for (int c = 0; c < 2; ++c) {
        int i = c * 4 + r;
        // Its place among the eight, not within its row of two.
        auto v = game_item(slots_, i, [this, i] { return std::format("{}, {} {} {} 8", slot_text(i), strings::kSlot, i + 1, strings::kOf); });
        v->speaks_own_position = true;
        b.add_item(slot("slots", i), v);
      }
      b.end_row();
    }
    b.set_start(slot("slots", slots_.cursor()));
  }
};

std::unique_ptr<Screen> make_skill_learn() { return std::make_unique<SkillLearnScreen>(); }
}  // namespace p3a::screens

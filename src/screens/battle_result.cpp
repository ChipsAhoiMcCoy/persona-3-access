#include "screens/menu.h"
#include "screens/util.h"
#include "screens/battle.h"
#include "core/strings.h"
#include "items.h"
#include "log.h"
#include "msg.h"
#include "party.h"
#include "persona.h"
#include <windows.h>
#include <xinput.h>
#include <cstring>
#include <format>
#include <vector>

namespace p3a::screens {
using namespace ue;

void build_status_page(ui::GraphBuilder& b, const uint8_t* e, const std::string& lead, const std::string& bonus,
                       const uint8_t* before, const ui::ControlId* name_id, ui::NodeVtablePtr name_item);

UObject* result_ui() {
  static Finder core{"BtlCoreComponent"};
  UObject* c = core.get();
  UObject* phase = c ? get<UObject*>(c, "CurrentPhase") : nullptr;
  return phase && is_a(phase, "BtlPhaseResult") ? get<UObject*>(phase, "_uiBase") : nullptr;
}
int result_state(UObject* ui) { return ui ? get<uint8_t>(ui, "State") : -1; }

// The victory screen, a bespoke page per state of the game's own result state machine (BP_BtlResultUIBase "Update
// State"; the result phase's _uiBase, an ABtlResultUIBase): `State` (EBtlResultState) 1 Top (EXP and the items found;
// with level-ups it moves on to 2 by itself after SecondChangeToLevup), 2 Levelup (the members' levels, the risen
// ones marked; on to 3 by itself when a Persona rose), 3 PersonaLvup (the Personas that rose), 4 HeroHumanLevelup (the
// hero's own page), 7 PersonaStatus (a Persona's status page with its stats rising, _PersonaStatusActor, when it
// learned a skill), 8 End. States 1 to 3 are one page that grows: the summary stays drawn while the level-ups slide
// in over it (2026-11-02). Each page is read from the native functions the Blueprint draws it from: GetItemParams
// (id, count, name), GetAllyLevelupParam (unit id, level, risen), GetPersonaLevelupParam (id, level, name),
// GetHeroInfo / GetHeroHumanLvupParam. Nothing is read from drawn text (the item names used to be caught off
// FontDrawEx as they slid in, the level-ups by matching drawn names).
// A press goes on (InputButton): taken only while isAvailableKeyHelpButton is on and isStarttingKeyhelpOut off (the OK
// key help is up); one made before that is held until the game takes it, so each page is said as soon as it is up.
class BattleResultScreen : public MenuScreen {
  Finder view_{"BtlResultViewData"};
  struct Section { std::string key; std::vector<std::string> lines; };
  std::vector<Section> page_;
  int state_ = -1;            // the state the page was built for
  int persona_ = 0;           // the Persona whose status page is up (state 7)
  int armed_ = -1;            // a press held for this state
  bool pressed_ = false;      // the player pressed since the state changed: the next section interrupts

  static bool takes_press(UObject* ui) {
    if (result_state(ui) == 7) return true;   // the status pages take the game's own input (the key help stays off)
    return get_bool(ui, "isAvailableKeyHelpButton") && !get_bool(ui, "isStarttingKeyhelpOut");
  }
  static void press() { press_game(VK_RETURN, XINPUT_GAMEPAD_A); }
  // A TArray of structs a UFunction returned: its elements' bytes, the array freed (FStrings inside are the caller's).
  static std::vector<uint8_t> array_of(UObject* ui, const char* fn, size_t size, int& n) {
    n = 0;
    UFunction* f = find_function(class_of(ui), fn);
    if (!f) return {};
    Params p(f);
    if (!call(ui, p)) return {};
    auto* a = p.at<TArray<uint8_t>>("ReturnValue");
    if (!a || !a->data || a->num <= 0 || a->num > 64) return {};
    n = a->num;
    std::vector<uint8_t> out(a->data, a->data + n * size);
    msg::game_free(a->data);
    a->data = nullptr;
    return out;
  }
  static std::string take(uint8_t* at) { return msg::take_fstring(*(FString*)at); }

  // The summary, one line: the level-ups slide in over it a second or two later by themselves, and the focus going to
  // them left the items unsaid when they were lines of their own (2026-11-02).
  std::vector<std::string> top(UObject* ui) {
    UObject* v = view_.get();
    std::string s = std::format("{} {}", v ? get<int64_t>(v, "Exp") : 0, strings::kExp);
    if (int64_t m = v ? get<int64_t>(v, "money") : 0) s += std::format(". {} {}", m, strings::kYen);
    int n = 0;
    auto items = array_of(ui, "GetItemParams", 0x18, n);
    std::string got;
    for (int i = 0; i < n; ++i) {
      uint8_t* e = items.data() + i * 0x18;
      int id = *(int32_t*)e, num = *(int32_t*)(e + 4);
      std::string name = take(e + 8);
      if (name.empty()) name = items::name_of(id);
      if (name.empty()) continue;
      got += (got.empty() ? "" : ", ") + (num > 1 ? std::format("{} x{}", name, num) : name);
    }
    s += got.empty() ? std::format(". {}", strings::kNoItems) : std::format(". {} {}", strings::kGot, got);
    return {s};
  }
  // The members who rose (the hero's own page follows when the hero did).
  std::vector<std::string> allies(UObject* ui) {
    std::vector<std::string> out;
    int n = 0;
    auto a = array_of(ui, "GetAllyLevelupParam", 0xC, n);
    for (int i = 0; i < n; ++i) {
      uint8_t* e = a.data() + i * 0xC;
      int id = *(int32_t*)e, level = *(int32_t*)(e + 4);
      log::writef("result: ally {} level {} risen {}", id, level, e[8]);
      if (!e[8] || id <= 1 || id >= 32) continue;
      std::string name = !g_party_name[id].empty() ? g_party_name[id] : party::name(id);
      if (name.empty()) continue;
      std::string s = std::format("{} {}, {} {}", strings::kLevelUp, name, strings::kLevel, level);
      party::Member m;
      if (party::member(id, m) && m.max_hp > 0) s += ", " + std::format(strings::kMaxHpSp, m.max_hp, m.max_sp);
      out.push_back(s);
    }
    return out;
  }
  std::vector<std::string> personas(UObject* ui) {
    std::vector<std::string> out;
    int n = 0;
    auto a = array_of(ui, "GetPersonaLevelupParam", 0x18, n);
    for (int i = 0; i < n; ++i) {
      uint8_t* e = a.data() + i * 0x18;
      int id = *(int32_t*)e, level = *(int32_t*)(e + 4);
      std::string name = take(e + 8);
      if (name.empty()) name = persona::name(id);
      if (!name.empty()) out.push_back(std::format("{} {}, {} {}", strings::kLevelUp, name, strings::kLevel, level));
    }
    return out;
  }
  // The hero's page: level, HP and SP (now, of the new maximum), EXP to the next level, the equipped Persona.
  std::vector<std::string> hero(UObject* ui) {
    std::vector<std::string> out;
    UFunction* f = find_function(class_of(ui), "GetHeroInfo");
    UFunction* lv = find_function(class_of(ui), "GetHeroHumanLvupParam");
    if (!f || !lv) return out;
    Params p(f);
    if (!call(ui, p)) return out;
    auto* r = p.at<uint8_t>("ReturnValue");
    if (!r) return out;
    std::string name = take(r), persona_name = take(r + 0x30);
    Params q(lv);
    call(ui, q);
    out.push_back(std::format("{} {}, {} {}", strings::kLevelUp, name, strings::kLevel, q.ret<int32_t>("Level")));
    out.push_back(std::format("HP {} {} {}", *(int32_t*)(r + 0x10), strings::kOf, *(int32_t*)(r + 0x14)));
    out.push_back(std::format("SP {} {} {}", *(int32_t*)(r + 0x18), strings::kOf, *(int32_t*)(r + 0x1C)));
    out.push_back(std::format("{} {}", *(int64_t*)(r + 0x20), strings::kExpToNext));
    if (!persona_name.empty())
      out.push_back(std::format("{} {}, {} {}", strings::kResultPersona, persona_name, strings::kLevel, *(int32_t*)(r + 0x2C)));
    return out;
  }
  // The Persona status page on screen (state 7): the result UI's status actor once its entry is set. That entry is
  // the Persona before the level-up (the page animates from it: level 7 and no Maragi under "Lv 08, NEW Maragi",
  // 2026-11-02); the save data already holds it after.
  static const uint8_t* persona_page(UObject* ui) {
    if (!ui || get_bool(ui, "IsEndPersonaStatus")) return nullptr;
    UObject* ps = get<UObject*>(ui, "_PersonaStatusActor");
    const uint8_t* e = ps ? persona::status_entry(ps) : nullptr;
    return e && *(uint16_t*)(e + 2) ? e : nullptr;
  }
  // The save data takes the level-up a few frames after the status page comes up, the level a frame before the skill
  // (the page was said at "level 7", then without the skill, 2026-11-02): it is neither built nor said until the new
  // skill is in (state 7 comes only for a skill learned, CheckSKillGetPersona).
  static bool settled(const uint8_t* now, const uint8_t* was) {
    return !now || memcmp(now + 0xC, was + 0xC, 16) != 0;   // the page is up for a skill learned: wait for it
  }
  // The Persona's entry in the save data: the hero's stock, else a member's own.
  static const uint8_t* live_entry(int id) {
    for (auto& o : persona::stock())
      if (o.id == id) return persona::stock_entry(o.slot);
    for (int unit = 2; unit <= 10; ++unit)
      if (const uint8_t* e = party::persona_entry(unit); e && *(uint16_t*)(e + 2) == id) return e;
    return nullptr;
  }
  static ui::ControlId line_id(const std::string& key, int i) { return ui::ControlId::structural(std::format("res.{}.{}", key, i)); }
  // A section the game added: the focus goes to its first line, said after what is being said unless the player's
  // press brought it.
  void add(const std::string& key, std::vector<std::string> lines, bool first) {
    if (lines.empty()) return;
    page_.push_back({key, std::move(lines)});
    if (!first) focus(line_id(key, 0), pressed_);
  }

 public:
  const char* id() const override { return "battle_result"; }
  int priority() const override { return 27; }   // over the battle menus: the menu state is not cleared during results
  bool active() override {
    UObject* ui = result_ui();
    int s = result_state(ui);
    if (s == 7) return persona_page(ui) != nullptr;
    return s >= 1 && s <= 4 && view_.get();
  }
  std::string title() override { return strings::kVictory; }
  void on_enter() override { screens::set_keys(state_ == 7 ? strings::kLevelUpKeys : strings::kSpaceContinues); }
  // Tartarus keeps its battle core between fights, so the levels each battle starts from are forgotten here, not
  // when the core goes ("level up to 6" after every later victory, 2026-10-27).
  void on_leave() override {
    if (!active()) { forget_battle_start(); state_ = -1; persona_ = 0; armed_ = -1; page_.clear(); }
  }
  void on_tick() override {
    UObject* ui = result_ui();
    int s = result_state(ui);
    if (s != state_) {
      bool first = state_ < 0;
      log::writef("result: state {} -> {}", state_, s);
      if (s == 1 || s == 4 || first) page_.clear();
      if (s == 1) add("top", top(ui), first);
      else if (s == 2) add("allies", allies(ui), first);
      else if (s == 3) add("personas", personas(ui), first);
      else if (s == 4) add("hero", hero(ui), first);
      state_ = s;
      armed_ = -1;
      persona_ = 0;
      pressed_ = false;
      screens::set_keys(s == 7 ? strings::kLevelUpKeys : strings::kSpaceContinues);
    }
    if (s == 7) {
      const uint8_t* e = persona_page(ui);
      int id = e && settled(live_entry(*(uint16_t*)(e + 2)), e) ? *(uint16_t*)(e + 2) : 0;
      if (id && id != persona_) {
        focus(ui::ControlId::structural(std::format("ps.{}.name", id)));
        persona_ = id;
      }
    }
    if (armed_ == s && takes_press(ui)) {
      log::writef("result: held press taken in state {}", s);
      armed_ = -1;
      press();
    }
  }
  void build(ui::GraphBuilder& b) override {
    UObject* ui = result_ui();
    if (!ui) return;
    if (state_ == 7) {
      if (const uint8_t* was = persona_page(ui)) {
        const uint8_t* now = live_entry(*(uint16_t*)(was + 2));
        if (!settled(now, was)) return;
        // What the page is up for (the game's "NEW SKILL!!"), under the name.
        std::string learned;
        for (int i = 0; now && i < 8; ++i) {
          uint16_t sid = *(uint16_t*)(now + 0xC + i * 2);
          bool had = false;
          for (int j = 0; j < 8; ++j) had |= *(uint16_t*)(was + 0xC + j * 2) == sid;
          if (sid && !had) learned += (learned.empty() ? "" : ", ") + persona::skill_name(sid);
        }
        if (!learned.empty()) learned = std::format("{} {}", strings::kLearned, learned);
        build_status_page(b, now ? now : was, strings::kLevelUp, learned, now ? was : nullptr, nullptr, {});
      }
      return;
    }
    if (page_.empty()) return;
    auto go_on = [this] {
      UObject* u = result_ui();
      if (!u) return;
      pressed_ = true;
      if (takes_press(u)) press();
      else armed_ = result_state(u);
    };
    b.begin_stop("res");
    for (auto& sec : page_)
      for (int i = 0; i < (int)sec.lines.size(); ++i) {
        std::string t = sec.lines[i];
        b.add_item(line_id(sec.key, i), item([t] { return t; }, go_on));
      }
    b.set_start(line_id(page_.back().key, 0));
  }
};

std::unique_ptr<Screen> make_battle_result() { return std::make_unique<BattleResultScreen>(); }
}  // namespace p3a::screens

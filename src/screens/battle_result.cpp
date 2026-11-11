#include "screens/menu.h"
#include "screens/util.h"
#include "screens/battle.h"
#include "screens/battle_state.h"
#include "screens/persona_status.h"
#include "core/strings.h"
#include "items.h"
#include "log.h"
#include "input.h"
#include "msg.h"
#include "party.h"
#include "persona.h"
#include "speech.h"
#include <windows.h>
#include <xinput.h>
#include <cstring>
#include <format>
#include <vector>

namespace p3a::screens {
using namespace ue;

UObject* result_ui() {
  UObject* phase = result_phase();
  return phase ? get<UObject*>(phase, "_uiBase") : nullptr;
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
// Each thing on a page is a line of its own: said in turn as it comes up, and there to go back to with the arrows (the
// user's call, 2026-11-05). The level-ups slide in by themselves, so a section the game adds queues after what is being
// said and leaves the focus where the player is; only a new page or the player's press moves it.
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

  // The summary: the EXP, the money and each item found.
  std::vector<std::string> top(UObject* ui) {
    UObject* v = view_.get();
    std::vector<std::string> out{std::format("{} {}", v ? get<int64_t>(v, "Exp") : 0, strings::kExp)};
    if (int64_t m = v ? get<int64_t>(v, "money") : 0) out.push_back(std::format("{} {}", m, strings::kYen));
    int n = 0;
    auto items = array_of(ui, "GetItemParams", 0x18, n);
    for (int i = 0; i < n; ++i) {
      uint8_t* e = items.data() + i * 0x18;
      int id = *(int32_t*)e, num = *(int32_t*)(e + 4);
      std::string name = take(e + 8);
      if (name.empty()) name = items::name_of(id);
      if (name.empty()) continue;
      out.push_back(num > 1 ? std::format("{} {} x{}", strings::kGot, name, num) : std::format("{} {}", strings::kGot, name));
    }
    return out;
  }
  // The members who rose (the hero's own page follows when the hero did). Their new maximums are not known yet: the
  // save data takes a member's level after the results (it still held "max HP 153" under "level 12", 2026-11-05).
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
      out.push_back(std::format("{} {}, {} {}", strings::kLevelUp, name, strings::kLevel, level));
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
  // The hero's page: level, HP and SP (now, of the new maximum), EXP to the next level. The page also draws the
  // equipped Persona and its level; it is not said: among the hero's new numbers it was heard as the Persona having
  // risen (the user, 2026-11-10), and one that did rise has its own line on the page before.
  std::vector<std::string> hero(UObject* ui) {
    std::vector<std::string> out;
    UFunction* f = find_function(class_of(ui), "GetHeroInfo");
    UFunction* lv = find_function(class_of(ui), "GetHeroHumanLvupParam");
    if (!f || !lv) return out;
    Params p(f);
    if (!call(ui, p)) return out;
    auto* r = p.at<uint8_t>("ReturnValue");
    if (!r) return out;
    std::string name = take(r);
    take(r + 0x30);   // the Persona's name: the game's string, freed
    Params q(lv);
    call(ui, q);
    out.push_back(std::format("{} {}, {} {}", strings::kLevelUp, name, strings::kLevel, q.ret<int32_t>("Level")));
    out.push_back(std::format("HP {} {} {}", *(int32_t*)(r + 0x10), strings::kOf, *(int32_t*)(r + 0x14)));
    out.push_back(std::format("SP {} {} {}", *(int32_t*)(r + 0x18), strings::kOf, *(int32_t*)(r + 0x1C)));
    out.push_back(std::format("{} {}", *(int64_t*)(r + 0x20), strings::kExpToNext));
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
  // What a status page is up for (the game's "NEW SKILL!!"): `now` holds a skill `was` did not.
  static std::string learned_line(const uint8_t* now, const uint8_t* was) {
    std::string learned;
    for (int i = 0; now && i < 8; ++i) {
      uint16_t sid = *(uint16_t*)(now + 0xC + i * 2);
      bool had = false;
      for (int j = 0; j < 8; ++j) had |= *(uint16_t*)(was + 0xC + j * 2) == sid;
      if (sid && !had) learned += (learned.empty() ? "" : ", ") + persona::skill_name(sid);
    }
    return learned.empty() ? learned : std::format("{} {}", strings::kLearned, learned);
  }
  // A status page coming up: the level, then what it animates, the skill learned and the stats that rose. The rest of
  // the page is for the arrows.
  static void say_status(const uint8_t* now, const uint8_t* was) {
    const uint8_t* e = now ? now : was;
    speech::speak(std::format("{} {}", strings::kLevelUp, persona::describe(*(uint16_t*)(e + 2), *(uint16_t*)(e + 4))), true);
    if (!now) return;
    if (std::string l = learned_line(now, was); !l.empty()) speech::speak(l, false);
    for (int i = 0; i < 5; ++i) {
      int v = persona::stat_total(now, i), up = v - persona::stat_total(was, i);
      if (up > 0) speech::speak(std::format("{} {}, {} {}", strings::kStatNames[i], v, strings::kUp, up), false);
    }
  }
  static ui::ControlId line_id(const std::string& key, int i) { return ui::ControlId::structural(std::format("res.{}.{}", key, i)); }
  bool has(const std::string& key) const {
    for (auto& sec : page_) if (sec.key == key) return true;
    return false;
  }
  // ---- ahead of the animation (the user's call, 2026-11-09): the page's parts are known the moment it opens, so
  // nothing waits for the game's slides. The summary is said at once; the level-ups (read then too, pending_) come on
  // the player's confirm, or when the game slides them in by itself, whichever is first. A confirm with nothing left
  // to hear ends the page (finishing_): the game is taken through what it still has to show, its own wait before the
  // level-ups ended (WaitStateTime against SecondChangeToLevup, "Update State" 472) and a press made wherever it takes
  // one, up to the next page that is the player's to read (the hero's, a Persona's status) or the end.
  std::vector<Section> pending_;
  bool finishing_ = false;
  int press_state_ = -1;      // the state the last finishing press was made in
  int press_frames_ = 0;      // the game's frames since it was delivered
  // The parts the game has not shown yet: said now, the first cutting off what is being said.
  bool reveal_pending() {
    if (pending_.empty()) return false;
    auto parts = std::move(pending_);
    pending_.clear();
    for (auto& sec : parts) add(sec.key, std::move(sec.lines));
    return true;
  }
  bool reveal(const std::string& key) {
    for (size_t i = 0; i < pending_.size(); ++i)
      if (pending_[i].key == key) {
        Section sec = std::move(pending_[i]);
        pending_.erase(pending_.begin() + i);
        add(sec.key, std::move(sec.lines));
        return true;
      }
    return false;
  }
  // The game takes a press now: the OK key help, or a Persona's level-up animation (state 3 reads the button itself
  // and jumps to the last Persona, "Update State" 962).
  static bool wants_press(UObject* ui) {
    if (takes_press(ui)) return true;
    return result_state(ui) == 3 && get_bool(ui, "isStartAnimPersonaLvup") && !get_bool(ui, "isFinishPersonaLvupAnim");
  }
  // The game's wait before the level-ups slide in is over: it changes state on its next tick.
  static void end_wait(UObject* ui) {
    float* waited = field<float>(ui, "WaitStateTime");
    float* wait = field<float>(ui, "SecondChangeToLevup");
    if (waited && wait && *waited < *wait) *waited = *wait;
  }
  // A section the game added: its lines in turn, after what is being said unless the player's press brought it.
  void add(const std::string& key, std::vector<std::string> lines) {
    if (lines.empty()) return;
    bool go_there = page_.empty() || pressed_;
    bool cut = pressed_;
    for (auto& l : lines) { speech::speak(l, cut); cut = false; }
    page_.push_back({key, std::move(lines)});
    if (go_there) focus_quietly(line_id(key, 0));
    pressed_ = false;
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
  bool opening_interrupts() override { return false; }
  bool announce_entry() override { return state_ == 7; }   // a status page, back from a window: the line in focus
  // The hero's stock-up notices ("Update Stockup": two windows, then ResultStockType back to 0 and the key help).
  bool window_coming() override { UObject* ui = result_ui(); return ui && get<uint8_t>(ui, "ResultStockType") != 0; }
  void on_enter() override {
    screens::set_keys(state_ == 7 ? strings::kLevelUpKeys : strings::kResultKeys);
    if (state_ == 7) return;
    // Back from a message window over the page, which cut its reading off (the hero's level-up notices, a second into
    // the page): the page again.
    bool cut = true;
    for (auto& sec : page_)
      for (auto& l : sec.lines) { speech::speak(l, cut); cut = false; }
  }
  // Tartarus keeps its battle core between fights, so the levels each battle starts from are forgotten here, not
  // when the core goes ("level up to 6" after every later victory, 2026-10-27).
  void on_leave() override {
    if (!active()) {
      forget_battle_start();
      state_ = -1; persona_ = 0; armed_ = -1; pressed_ = false; page_.clear();
      pending_.clear(); finishing_ = false; press_state_ = -1;
    }
  }
  void on_tick() override {
    UObject* ui = result_ui();
    int s = result_state(ui);
    if (s != state_) {
      log::writef("result: state {} -> {}", state_, s);
      if (s == 1 || s == 4) { page_.clear(); pending_.clear(); }
      if (s != 1 && s != 2 && s != 3) finishing_ = false;   // a page of its own, or the end: the player's again
      // A state first seen late (a message window was over the screen) brings the sections before it.
      if (s >= 1 && s <= 3 && !has("top")) add("top", top(ui));
      // The level-ups, read as the summary opens (the game slides them in later; isLevelupScreen is set after this).
      if (s == 1) {
        if (auto l = allies(ui); !l.empty()) pending_.push_back({"allies", std::move(l)});
        if (auto l = personas(ui); !l.empty()) pending_.push_back({"personas", std::move(l)});
        log::writef("result: {} level-up part(s) read ahead", pending_.size());
      }
      // The game shows them now: said, unless the player's confirm brought them already.
      if ((s == 2 || s == 3) && !has("allies") && !reveal("allies")) add("allies", allies(ui));
      if (s == 3 && !has("personas") && !reveal("personas")) add("personas", personas(ui));
      if (s == 4) add("hero", hero(ui));
      state_ = s;
      armed_ = -1;
      persona_ = 0;
      pressed_ = false;
      press_state_ = -1;
      screens::set_keys(s == 7 ? strings::kLevelUpKeys : strings::kResultKeys);
    }
    // The player is ahead of the game (heard the level-ups, or is done with the page): its wait is over.
    if (s == 1 && (finishing_ || (pending_.empty() && (has("allies") || has("personas"))))) end_wait(ui);
    // Done with the page: a press wherever the game takes one. The next is made only once the last was delivered in
    // full and the game ran two frames on it without leaving the state it was made in.
    if (finishing_ && s >= 1 && s <= 3) {
      if (input::injected_held() || input::busy()) press_frames_ = 0;
      else ++press_frames_;
      if (wants_press(ui) && (press_state_ != s || press_frames_ > 2)) {
        log::writef("result: finishing press in state {}", s);
        press_state_ = s;
        press_frames_ = 0;
        press();
      }
    }
    if (s == 7) {
      const uint8_t* was = persona_page(ui);
      int id = was ? *(uint16_t*)(was + 2) : 0;
      const uint8_t* now = id ? live_entry(id) : nullptr;
      if (id && id != persona_ && settled(now, was)) {
        focus_quietly(status_name_id(id));
        say_status(now, was);
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
        build_status_page(b, now ? now : was, strings::kLevelUp, learned_line(now, was), now ? was : nullptr, nullptr, {});
      }
      return;
    }
    if (page_.empty()) return;
    auto go_on = [this] {
      UObject* u = result_ui();
      if (!u) return;
      speech::stop();   // a pad's press stops nothing by itself: the rest of the page is not read into what comes next
      pressed_ = true;
      int s = result_state(u);
      // The summary and its level-ups: the next part at once, or the page's end (on_tick takes the game there).
      if (s >= 1 && s <= 3) {
        if (!reveal_pending()) finishing_ = true;
        return;
      }
      if (takes_press(u)) press();
      else armed_ = s;
    };
    b.begin_stop("res");
    for (auto& sec : page_)
      for (int i = 0; i < (int)sec.lines.size(); ++i) {
        std::string t = sec.lines[i];
        b.add_item(line_id(sec.key, i), item([t] { return t; }, go_on));
      }
    b.set_start(line_id(page_.front().key, 0));
  }
};

std::unique_ptr<Screen> make_battle_result() { return std::make_unique<BattleResultScreen>(); }
}  // namespace p3a::screens

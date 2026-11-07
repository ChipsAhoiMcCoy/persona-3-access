// Battle (the skill, item and target lists are bespoke menus: battle_menus.cpp). The command wheel is a sprite; its
// state is reflected:
//   ABtlGuiStateManager.nowState      EBtlGuiState (TOP 1, SKILL 2, ITEM 3, TARGET 4, ...)
//   BP_BtlTargetManager_C             MainCharacter (whose turn), CursorTargets (the target the wheel points at)
// and the words come from the game's own battle database (UBtlDataBaseComponent: GetUnitName, GetUnitHP,
// GetSkillName, GetSkillCost, GatAnalizeAttr, GetAttackName). Keyboard: Space attack, E skills (Persona),
// R items, C guard, A and D change target (measured 2026-10-27 on the 4/9 rooftop battle).
// The wheel is played with the game's own keys (the player's call, 2026-11-01: no command list); F1 names the open
// ones. Left and right step BP_BtlGuiComponent_C.EnemyList (sorted live targets; "Target Index" + 1 mod its length,
// On Select Next Target): a target's position there is its "1 of 2". The game clears IsCreateEnemyList as the wheel
// opens a turn and builds the turn's list on its next tick, setting it again: until then EnemyList is the last turn's,
// empty on a battle's first.
#include "screens/screen.h"
#include "screens/util.h"
#include "screens/battle.h"
#include "screens/battle_words.h"
#include "core/strings.h"
#include "input.h"
#include "keys.h"
#include "msg.h"
#include "party.h"
#include "persona.h"
#include "speech.h"
#include "textcap.h"
#include "core/text_clean.h"
#include <windows.h>
#include <xinput.h>
#include <format>
#include <cstring>
#include <map>
#include <vector>

namespace p3a::screens {
using namespace ue;

int g_start_level[32];
std::string g_party_name[32];
std::map<int, StartPersona> g_start_personas;
void forget_battle_start() { memset(g_start_level, 0, sizeof g_start_level); g_start_personas.clear(); }
// A member's Personas as the battle begins: the hero's whole stock, a member's equipped one.
static void remember_personas(int unit_id) {
  auto keep = [](int pid, int level, const uint8_t* st, const uint16_t* sk) {
    StartPersona p;
    p.level = level;
    memcpy(p.stats.data(), st, sizeof p.stats);
    memcpy(p.skills.data(), sk, sizeof p.skills);
    g_start_personas[pid] = p;
  };
  if (unit_id == 1) { for (auto& o : persona::stock()) keep(o.id, o.level, o.stats, o.skills); return; }
  if (const uint8_t* e = party::persona_entry(unit_id))
    if (int pid = *(uint16_t*)(e + 2)) keep(pid, *(uint16_t*)(e + 4), e + 0x1C, (const uint16_t*)(e + 0xC));
}

class BattleScreen : public Screen {
  Finder gui_{"BtlGuiStateManager"}, top_{"BP_BtlTopUI_C"}, tmgr_{"BP_BtlTargetManager_C"}, guicomp_{"BP_BtlGuiComponent_C"};
  int state_ = -1;
  WeakObj actor_, target_, last_actor_;
  bool turn_due_ = false;   // the wheel opened a turn whose line is not said yet: it waits for the turn's list
  BtlWords w_;   // the battle database's words and numbers
  // An enemy on the wheel with its place among the targets left and right go through.
  std::string target_line(UObject* t) {
    std::string s = w_.enemy(t);
    UObject* g = guicomp_.get();
    auto* l = g ? field<TArray<UObject*>>(g, "EnemyList") : nullptr;
    for (int i = 0; l && l->num > 1 && i < l->num; ++i)
      if (l->data[i] == t) { s += std::format(", {} {} {}", i + 1, strings::kOf, l->num); break; }
    return s;
  }
  TArray<UObject*>* enemy_list() {
    UObject* g = guicomp_.get();
    return g ? field<TArray<UObject*>>(g, "EnemyList") : nullptr;
  }
  // The wheel's target: EnemyList["Target Index"], which only the player's left and right (and a new turn's list)
  // move. CursorTargets also follows other things (it flickered between enemies while a battle opened, 2026-10-27).
  UObject* wheel_target() {
    UObject* g = guicomp_.get();
    auto* l = enemy_list();
    int32_t* i = g ? field<int32_t>(g, "Target Index") : nullptr;
    if (l && i && *i >= 0 && *i < l->num && l->data[*i]) return l->data[*i];
    return cursor_target();
  }
  // The wheel's commands, each with the game's key (or button). key_index: BP_BtlTopUI_C.KeyControl's slot, the one
  // its key checks (0 attack, 1 guard, 2 Persona, 3 item, 4 Shift, 5 analyze, 6 tactics, 7 assist, 8 rush, 9 survey;
  // BP_BtlTopUI 47757-50523), -1 none.
  struct Cmd { int key_index; const strings::Hint* key; };
  static constexpr Cmd kCmds[] = {
      {0, &strings::kKeyAttack}, {2, &strings::kKeySkills},  {3, &strings::kKeyItems},   {1, &strings::kKeyGuard},
      {6, &strings::kKeyTactics}, {5, &strings::kKeyAnalyze}, {9, &strings::kKeySurvey}, {8, &strings::kKeyRush},
      {-1, &strings::kKeyTheurgy}, {4, &strings::kKeyShift},  {7, &strings::kKeyAssist},
  };
  bool open(const Cmd& c) {
    UObject* top = top_.get();
    if (c.key == &strings::kKeyTheurgy) {
      UObject* a = main_character();
      int tp = w_.unit_int("GetUnitTP", a), max_tp = w_.unit_int("GetUnitMaxTP", a);
      return max_tp > 0 && tp >= max_tp;
    }
    // Shift: after a 1 More with someone to pass to (BP_BtlTopUI_C.IsOneMore, which shows the game's Shift button).
    if (c.key == &strings::kKeyShift && (!top || !get_bool(top, "IsOneMore"))) return false;
    auto* kc = top ? field<TArray<bool>>(top, "KeyControl") : nullptr;
    return c.key_index < 0 || !kc || c.key_index >= kc->num || kc->data[c.key_index];
  }
  // F1 on the wheel: each open command's key (the game's tutorials lock some), then changing the target.
  std::string top_keys() {
    std::string s;
    for (auto& c : kCmds)
      if (open(c)) s += (s.empty() ? "" : ", ") + std::string((*c.key)());
    return s + ". " + strings::kChangeTargetKeys() + " " + strings::kDetailKeys();
  }
  UObject* main_character() { UObject* m = tmgr_.get(); return m ? get<UObject*>(m, "MainCharacter") : nullptr; }
  UObject* cursor_target() {
    UObject* m = tmgr_.get();
    auto* a = m ? field<TArray<UObject*>>(m, "CursorTargets") : nullptr;
    return a && a->num > 0 ? a->data[0] : nullptr;
  }
  int gui_state() { UObject* g = gui_.get(); return g ? get<uint8_t>(g, "nowState") : 0; }

  // ---- what happened: every unit's HP is watched, and the action banner (the skill name the game shows at the
  // top while someone acts, FontDrawEx around y 105) is spoken with the actor's name. Damage numbers are sprites.
  Finder core_{"BtlCoreComponent"};
  struct Unit { std::string name; int hp; bool enemy; uint32_t status = 0; };
  bool downed_since_turn_ = false;
  // EBtlDataBadStatus bits the game tracks per unit: 0 dizzy, 1 charm .. 8 poison, 13 down (GetBadStatus).
  uint32_t status_of(UObject* u) {
    UObject* d = w_.db();
    UFunction* f = d ? find_function(class_of(d), "GetBadStatus") : nullptr;
    if (!f) return 0;
    uint32_t bits = 0;
    for (int t : {0, 1, 2, 3, 4, 5, 6, 7, 8, 13}) {
      Params p(f);
      p.set<UObject*>("Actor", u);
      p.set<int32_t>("Type", t);
      call(d, p);
      if (p.ret<bool>()) bits |= 1u << t;
    }
    return bits;
  }
  std::map<WeakObj::Key, Unit> units_;
  std::string banner_;
  WeakObj banner_action_;     // whose action it was up under
  bool banner_gone_ = true;   // none drawn since banner_: the same words now are a new banner
  // Who the player just picked a skill, item or Guard for: that member's next banner is what the player chose, and is
  // not said (the user's call, 2026-11-05: any member the player commands). Whatever a member does of their own accord
  // is said: nothing is picked on the wheel (Rush), for a member on orders or an ailment (no wheel), or once anyone else acts.
  WeakObj picked_;
  // The one unit an action hits (ABtlActor.GetActionTargets), null for none or several.
  UObject* attacked(UObject* action) {
    UFunction* f = action ? find_function(class_of(action), "GetActionTargets") : nullptr;
    if (!f) return nullptr;
    Params p(f);
    if (!call(action, p)) return nullptr;
    auto* a = p.at<TArray<UObject*>>("ReturnValue");
    if (!a || !a->data) return nullptr;
    UObject* one = a->num == 1 ? a->data[0] : nullptr;
    msg::game_free(a->data);
    a->data = nullptr;
    return one;
  }
  void say_banner(UObject* actor, const std::string& b) {
    if (actor && WeakObj::of(actor) == picked_) { picked_ = {}; return; }
    std::string who = actor ? w_.unit_name(actor) : "";
    speech::speak(who.empty() ? b : std::format("{}: {}", who, b), false);
  }
  void watch() {
    UObject* core = core_.get();
    if (!core) return;
    int st = gui_state();
    if (st == 1) picked_ = {};
    else if (st == 2 || st == 3 || st == 4 || st == 7 || st == 11) picked_ = WeakObj::of(main_character());
    // The action banner (only while no battle menu is up: menus draw names in the same band).
    std::string b;
    UObject* phase = get<UObject*>(core, "CurrentPhase");
    if (st == 0 && !(phase && is_a(phase, "BtlPhaseResult")))   // results draw names in that band too
    for (auto& t : textcap::snapshot(200))
      if (t.source == 2 && t.y > 95 && t.y < 115 && t.x > 300 && t.x < 700 && (t.color & 0xff) == 0xff && core::has_words(t.text)) { b = t.text; break; }
    UObject* action_now = get<UObject*>(core, "CurrentAction");
    if (st == 0 && action_now && !(WeakObj::of(action_now) == picked_)) picked_ = {};
    if (b.empty()) {
      banner_gone_ = true;
    } else {
      // GetActionType (EBtlCommandType): 0 nothing chosen yet, so the same words are the last action's, still up as the
      // next unit's turn begins ("Merciless Maya: Guard", 2026-11-06).
      int type = action_now ? call_ret<uint8_t>(action_now, "GetActionType") : 0;
      if (b != banner_ || banner_gone_ || (!(WeakObj::of(action_now) == banner_action_) && type != 0)) {
        banner_ = b;
        banner_action_ = WeakObj::of(action_now);
        // 1 a plain attack has no banner, and a skill's is its own name: anything else is a passive of the one attacked
        // (a counter: "High Counter" came under the attacker's name, 2026-11-06).
        bool theirs = type != 1 && (type != 2 || w_.skill_name(call_ret<int32_t>(action_now, "GetSkillID")) == b);
        say_banner(theirs ? action_now : attacked(action_now), b);
      }
      banner_gone_ = false;
    }
    // HP changes and defeats.
    std::map<WeakObj::Key, Unit> next;
    std::vector<std::string> defeated;
    for (const char* list : {"PlayerList", "EnemyList"}) {
      auto* a = field<TArray<UObject*>>(core, list);
      for (int i = 0; a && i < a->num; ++i) {
        UObject* u = a->data[i];
        if (!u) continue;
        WeakObj w = WeakObj::of(u);
        auto key = w.key();
        Unit cur{w_.unit_name(u), w_.unit_int("GetUnitHP", u), list[0] == 'E', 0};
        cur.status = cur.hp > 0 ? status_of(u) : 0;
        if (!cur.enemy) {   // party levels as the battle began, for the victory screen's level ups
          int id = w_.unit_int("GetUnitID", u);
          if (id > 0 && id < 32 && !g_start_level[id]) {
            g_start_level[id] = persona::unit_level(id) ? persona::unit_level(id) : w_.unit_int("GetUnitLevel", u);
            g_party_name[id] = cur.name;
            remember_personas(id);
          }
        }
        auto it = units_.find(key);
        if (it != units_.end() && cur.hp != it->second.hp && cur.hp >= 0 && it->second.hp >= 0) {
          int d = cur.hp - it->second.hp;
          if (cur.hp == 0 && cur.enemy) defeated.push_back(cur.name);
          else if (cur.hp == 0) speech::speak(std::format("{} {}", cur.name, strings::kKnockedOut), false);
          else if (d < 0) speech::speak(std::format("{} {} {}", cur.name, -d, strings::kDamage), false);
          else speech::speak(std::format("{} {} {}", cur.name, strings::kHealed, d), false);
        }
        if (it != units_.end()) {
          uint32_t gained = cur.status & ~it->second.status;
          for (int t = 0; t <= 13; ++t)
            if (gained & (1u << t)) {
              speech::speak(std::format("{} {}", cur.name, strings::kBadStatus[t]), false);
              if (t == 13 && cur.enemy) downed_since_turn_ = true;
            }
        }
        next[key] = cur;
      }
    }
    for (auto& [k, u] : units_)
      if (u.enemy && u.hp > 0 && !next.count(k)) defeated.push_back(u.name);
    units_.swap(next);
    // Enemies falling together (an All-Out Attack) in one line: "2 Cowardly Maya defeated", not the same line twice.
    if (!defeated.empty()) {
      std::map<std::string, int> count;
      std::vector<std::string> order;
      for (auto& n : defeated) if (!count[n]++) order.push_back(n);
      std::string s;
      for (auto& n : order) s += (s.empty() ? "" : ", ") + (count[n] > 1 ? std::format("{} {}", count[n], n) : n);
      speech::speak(std::format("{} {}", s, strings::kDefeated), false);
    }
  }

  void announce_top() {
    UObject* a = main_character();
    UObject* t = wheel_target();
    actor_ = WeakObj::of(a);
    target_ = WeakObj::of(t);
    std::string s = std::format("{}. {}", w_.unit_name(a), w_.hp_sp(a));
    // The Theurgy gauge (GetUnitTP / GetUnitMaxTP): full means the Theurgy command is open.
    int tp = w_.unit_int("GetUnitTP", a), max_tp = w_.unit_int("GetUnitMaxTP", a);
    if (max_tp > 0 && tp >= max_tp) s += std::string(", ") + strings::kTheurgyReady();
    if (t) s += std::format(". {} {}", strings::kTarget, target_line(t));
    speech::speak(s, false);   // queued after what just happened
  }

  // TOP: whose turn it is and the target (said from background(): the command list is the menu in front).
  // Left or right pressed since the last tick (the keys and the D-pad), read every tick so that a press from another
  // list or from the field is never taken for one on the wheel.
  uint32_t target_keys_seen_ = 0;
  uint16_t target_pad_was_ = 0;
  bool target_press() {
    uint32_t n = 0;
    for (uint32_t vk : {(uint32_t)VK_LEFT, (uint32_t)VK_RIGHT, (uint32_t)'A', (uint32_t)'D'}) n += keys::movement_presses(vk);
    uint16_t pad = input::real_buttons() & (XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT);
    // A press made in the mod layer stays held as its button when the triggers let go: it was never one on the wheel.
    bool pressed = n != target_keys_seen_ || (!input::layer_open() && (pad & ~target_pad_was_));
    target_keys_seen_ = n;
    target_pad_was_ = pad;
    return pressed;
  }
  void top_tick(bool entering, bool target_pressed) {
    UObject* a = main_character();
    if (entering || (a && a != actor_.resolve())) turn_due_ = true;
    // The turn's target and its place are read from the turn's list. The line said before it was built had no "1 of 2"
    // on a battle's first turn, and the list appearing under it swallowed the player's first move (2026-11-05).
    UObject* g = guicomp_.get();
    if (!g || !get_bool(g, "IsCreateEnemyList")) return;
    UObject* t = wheel_target();
    if (a && turn_due_) {   // no acting member (between turns): nothing to say yet
      turn_due_ = false;
      // The same member acting again right after knocking an enemy down: the game's "1 More".
      bool again = a && a == last_actor_.resolve() && downed_since_turn_;
      // After a 1 More the wheel shows Shift (B) once it is unlocked (BP_BtlTopUI_C "Shift" is 1 then, but it
      // stays 1 between turns too, 2026-10-28: only said with the 1 More).
      UObject* top = top_.get();
      bool shift = top && get<float>(top, "Shift") > 0.5f;
      if (again) speech::speak(shift ? std::format("{} {}", strings::kOneMore, strings::kShiftAvailable()) : std::string(strings::kOneMore), false);
      downed_since_turn_ = false;
      last_actor_ = WeakObj::of(a);
      announce_top();
    }
    else if (t && t != target_.resolve()) {
      // The player moved the target: said at once, with its place (two of the same Shadow at the same health read
      // alike otherwise).
      target_ = WeakObj::of(t);
      speech::speak(target_line(t), true);
    }
    else if (t && target_pressed && screens::current() == this && !msg::current().blocking && !input::modal()) {
      // The last enemy left: the game's cursor has nowhere to go, so the press is answered with who it is still on.
      auto* l = enemy_list();
      if (l && l->num == 1) speech::speak(target_line(t), false);   // the focus did not move: nothing is cut off
    }
  }
  int bg_state_ = -1;
  // Said the current prompt (state 5 / 11): set when it is said, cleared when the game's state changes. The All-Out
  // state lasts through Shuffle Time and the results, so coming back to this screen is no new prompt.
  bool prompt_said_ = false;

 public:
  const char* id() const override { return "battle"; }
  // The All-Out and guard prompts and Analyze answer C (Cancel); on the command wheel C is Guard (and so is the
  // game's own Escape there, left to it).
  bool escape_is_back() override { int st = gui_state(); return st == 5 || st == 8 || st == 11; }
  int priority() const override { return 25; }
  bool active() override { return gui_state() != 0; }
  void enter() override { state_ = -1; }
  // Results are watched for as long as a battle exists, not only while a menu is up: the last blow ends the menus.
  void background() override {
    bool target_pressed = target_press();
    UObject* core = core_.get();
    if (!core) { units_.clear(); forget_battle_start(); downed_since_turn_ = false; last_actor_ = {}; return; }
    watch();
    int st = gui_state();
    // Assist (13, BTL_STATE_NAVIGATION) only plays the navigator's advice over the wheel: back from it is the same
    // turn, not the wheel opening again (each press re-read the turn line, 2026-11-01).
    bool entering = st != bg_state_ && bg_state_ != 13;
    bg_state_ = st;
    if (entering) prompt_said_ = false;
    if (st == 1) top_tick(entering, target_pressed);
    else turn_due_ = false;
  }
  void tick() override {
    int st = gui_state();
    bool entering = st != state_;
    state_ = st;
    switch (st) {
      case 1:   // TOP: the game's keys run the commands; the turn is read in background()
        if (entering) screens::set_keys([this] { return top_keys(); });
        break;
      // 2 skills, 3 items, 4 targets, 6 Survey, 7 Theurgy, 9 Shift, 10 Tactics: bespoke menus (battle_menus.cpp);
      // 12 the Persona status page (persona_status.cpp)
      // 5 All-Out Attack, 11 guard confirmation: button prompts with no cursor (BP_BtlPromiseCommon, BP_BtlGuard bind
      // only TrigOK / TrigCANCEL), so the game's own keys answer them; the question is said once with its two buttons
      // (the player's call, 2026-11-01: a prompt's buttons are its content, not a hint).
      case 5: {
        if (prompt_said_) break;
        // The All-Out state lingers into Shuffle Time and the results: not a prompt then.
        UObject* core = core_.get();
        UObject* phase = core ? get<UObject*>(core, "CurrentPhase") : nullptr;
        if (shuffle_ui() || (phase && is_a(phase, "BtlPhaseResult"))) break;
        prompt_said_ = true;
        screens::set_keys(strings::kAllOutKeys);
        speech::speak(std::format("{} {}", strings::kAllOutTitle, strings::kAllOutKeys()), true);
        break;
      }
      case 11:
        if (prompt_said_) break;
        prompt_said_ = true;
        screens::set_keys(strings::kGuardKeys);
        speech::speak(std::format("{} {}", strings::kGuardTitle, strings::kGuardKeys()), true);
        break;
      case 8: {   // Analyze: the target's name, level, health and the affinities found so far; A and D move it
        UObject* t = cursor_target();
        if (entering) screens::set_keys([] { return std::string(strings::kAnalyzeKeys()) + " " + strings::kDetailKeys(); });
        if (!t || (!entering && t == target_.resolve())) break;
        target_ = WeakObj::of(t);
        std::string s = std::format("{}, {} {}", w_.enemy(t), strings::kLevel, w_.unit_int("GetUnitLevel", t));
        speech::speak(entering ? std::string(strings::kAnalyze) + ". " + s : s, true);
        break;
      }
      default: break;
    }
  }
  std::string describe() override {
    UObject* a = main_character();
    UObject* t = wheel_target();
    std::string s = a ? std::format("{}. {}", w_.unit_name(a), w_.hp_sp(a)) : std::string(strings::kBattle);
    if (t) s += std::format(". {} {}", strings::kTarget, target_line(t));
    return s;
  }
};

std::unique_ptr<Screen> make_battle() { return std::make_unique<BattleScreen>(); }
}  // namespace p3a::screens

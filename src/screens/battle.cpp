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
#include <set>
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
  std::set<WeakObj::Key> won_enemies_;   // the enemies left when the battle was won (watch())
  std::string banner_;
  WeakObj banner_action_;     // whose action it was up under
  bool banner_gone_ = true;   // none drawn since banner_: the same words now are a new banner
  // Who the player just picked a skill, item or Guard for: that member's next banner is what the player chose, and is
  // not said (the user's call, 2026-11-05: any member the player commands). Whatever a member does of their own accord
  // is said: nothing is picked on the wheel (Rush), for a member on orders or an ailment (no wheel), or once anyone else acts.
  WeakObj picked_;
  // Whose action the player gave on the wheel or in its lists (a plain attack too): the one unit it hits is the one
  // the player chose, so its damage or healing is said without the name (the user's call, 2026-11-10). Dropped once
  // anyone else acts, so a later turn of the same member that no wheel opened (an ailment, orders) names its target.
  WeakObj commanded_;
  // Rush (Tab): BP_BtlGuiComponent ChangeRush flips the core's request (RequestRushMode) and shows or hides the RUSH
  // sign, a sprite; the fighting phase follows it at the next turn. The request is what the press changed.
  bool rush_ = false;
  // What a hit was: the word the game throws up beside the number (BP_BtlGuiDamage2_C, one actor per popup, made by
  // BP_BtlGuiIcon CreateDamage and started with the unit and a type: 5 WEAK, 6 CRITICAL, 7 RESIST, 8 MISS, 9 BLOCK;
  // 0 to 4 are the plain numbers, which the HP watch says). A miss or a block changes no HP: nothing was said at all.
  int popups_ = watch::want("BP_BtlGuiDamage2_C", true);
  std::set<WeakObj::Key> popups_seen_;
  void hit_words(UObject* action_now, int st) {
    std::set<WeakObj::Key> live;
    for (UObject* o : watch::all(popups_)) {
      UObject* u = get<UObject*>(o, "Chara");
      if (!u) continue;   // not started yet
      auto key = WeakObj::of(o).key();
      live.insert(key);
      if (popups_seen_.count(key)) continue;
      int type = get<int32_t>(o, "Type");
      if (type < 5 || type > 9) continue;
      const char* word = strings::kHitWords[type - 5];
      bool chosen = st == 0 && action_now && commanded_.resolve() == action_now && attacked(action_now) == u;
      speech::speak(chosen ? std::string(word) : std::format("{} {} {}", word, strings::kOn, w_.unit_name(u)), false);
    }
    popups_seen_.swap(live);
  }
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
    // On is said at once, over anything else: it answers the player's own press. Off queues: the game also ends a rush
    // itself (a boss's mid-battle lines: DisableRush), with the last blow's news still being read.
    if (bool rush = call_ret<bool>(core, "CheckRequestRushMode"); rush != rush_) {
      rush_ = rush;
      speech::speak(rush ? strings::kRushOn : strings::kRushOff, rush);
    }
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
    if (st != 0) commanded_ = WeakObj::of(main_character());
    else if (action_now && !(WeakObj::of(action_now) == commanded_)) commanded_ = {};
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
    hit_words(action_now, st);
    // HP changes and defeats. The enemies an All-Out Attack finishes keep their HP until the results take them away,
    // after Shuffle Time ("2 Muttering Tiara defeated" came after the cards, 2026-11-09): a deal with a card still to
    // take (the next battle's comes on the same actor, its last deal's picks all made) means the battle is won, and
    // whoever is left is defeated now.
    UObject* deal = shuffle_ui();
    int picks = deal ? get<int32_t>(deal, "EnableSelectNum") : 0;
    bool won = picks > 0 && get<int32_t>(deal, "SelectedCardNum") < picks;
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
        // Once counted as defeated an enemy stays so: with the card taken the deal no longer says the battle is won,
        // and the HP they still hold read as "healed 54", "is down" and a second "defeated" (2026-11-10).
        if (cur.enemy && (won || won_enemies_.count(key))) { cur.hp = 0; won_enemies_.insert(key); }
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
          else {
            // The player's own choice of this one unit: the number alone. Anything else names who it landed on (an
            // enemy's or an ally's own action, a whole side, an attack sent back at the attacker).
            bool chosen = st == 0 && action_now && commanded_.resolve() == action_now && attacked(action_now) == u;
            std::string s = d < 0 ? std::format("{} {}", -d, strings::kDamage) : std::format("{} {}", strings::kHealed, d);
            speech::speak(chosen ? s : std::format("{} {}", cur.name, s), false);
          }
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
    std::erase_if(won_enemies_, [&](const WeakObj::Key& k) { return !units_.count(k); });   // gone with the battle
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
    // Kept short, it is heard every turn (the user's call, 2026-11-11): HP and SP without their maxima, and the
    // target without its place in the row. A move reads the place; the details and the review key the rest.
    std::string s = std::format("{}, {}", w_.unit_name(a), w_.hp_sp_short(a));
    // The Theurgy gauge (GetUnitTP / GetUnitMaxTP): full means the Theurgy command is open.
    int tp = w_.unit_int("GetUnitTP", a), max_tp = w_.unit_int("GetUnitMaxTP", a);
    if (max_tp > 0 && tp >= max_tp) s += std::string(", ") + strings::kTheurgyReady();
    if (t) s += ". " + w_.enemy(t);
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
      if (l && l->num == 1) speech::speak(target_line(t), true);   // interrupting, like a menu's end (say_end)
    }
  }
  // ---- no wrapping (the players' call, 2026-11-09): the game's own target cursors go round (the wheel's "Target
  // Index" + 1 mod the list, BP_BtlGuiComponent 17497; Analyze's ListControll), so with two enemies nobody could tell
  // the leftmost from the rightmost. A press that would go round never reaches the game: the key is taken
  // (keys::set_capture), the button hidden (input::block_ends), and the press is answered with who the cursor is
  // still on. Which press lowers the index: the wheel's left (On Select Behind Target) and Analyze's R1 (NextTarget
  // is ListControll(false)); its L1 raises it.
  Finder analyze_{"BP_BtlGuiAnalyze_C"};
  struct Ends { bool low = false, high = false; UObject* on = nullptr; };   // the cursor is on the first / the last
  Ends ends(int st) {
    UObject* o = st == 1 ? guicomp_.get() : st == 8 ? analyze_.get() : nullptr;
    if (!o || (st == 1 && !get_bool(o, "IsCreateEnemyList"))) return {};   // the wheel's list is this turn's
    auto* l = field<TArray<UObject*>>(o, "EnemyList");
    int32_t* i = field<int32_t>(o, st == 1 ? "Target Index" : "TargetIndex");
    if (!l || !i || l->num < 2 || *i < 0 || *i >= l->num) return {};
    return {*i == 0, *i == l->num - 1, l->data[*i]};
  }
  bool mine() { return screens::current() == this && !msg::current().blocking && !input::modal(); }
  // -1: the press lowers the index, +1 raises it, 0: not a target key in this state.
  static int key_step(int st, uint32_t vk) {
    if (st == 1) return vk == VK_LEFT || vk == 'A' ? -1 : vk == VK_RIGHT || vk == 'D' ? 1 : 0;
    if (st == 8) return vk == 'E' || vk == 'D' ? -1 : vk == 'Q' || vk == 'A' ? 1 : 0;   // R1 / L1 on the keyboard
    return 0;
  }
  std::string analyze_line(UObject* t) { return std::format("{}, {} {}", w_.enemy(t), strings::kLevel, w_.unit_int("GetUnitLevel", t)); }
  void say_end(int st, UObject* t) {
    // Interrupting, like a menu's end: each press says it afresh instead of queueing the name (the user's call, 2026-11-09).
    if (t) speech::speak(st == 1 ? target_line(t) : analyze_line(t), true);
  }
  bool end_key(uint32_t vk, bool repeat) {
    if (!mine()) return false;
    int st = gui_state(), step = key_step(st, vk);
    if (!step) return false;
    Ends e = ends(st);
    if (!(step < 0 ? e.low : e.high)) return false;
    if (!repeat) say_end(st, e.on);
    return true;
  }
  uint16_t end_pad_was_ = 0, end_hidden_was_ = 0;
  void end_pad(int st) {
    Ends e = mine() ? ends(st) : Ends{};
    uint16_t hide = 0;
    int sticks = 0;
    if (st == 1) {
      if (e.low) { hide |= XINPUT_GAMEPAD_DPAD_LEFT; sticks |= 1; }
      if (e.high) { hide |= XINPUT_GAMEPAD_DPAD_RIGHT; sticks |= 2; }
    } else if (st == 8) {
      if (e.low) hide |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
      if (e.high) hide |= XINPUT_GAMEPAD_LEFT_SHOULDER;
    }
    input::block_ends(hide, sticks);
    uint16_t pad = input::layer_open() ? 0 : input::real_buttons() & (XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT |
                                                                        XINPUT_GAMEPAD_LEFT_SHOULDER | XINPUT_GAMEPAD_RIGHT_SHOULDER);
    // Only a press the game never got: its button was already hidden when it came. The press that brought the cursor
    // to this end was the game's, and the move says itself (both spoke: every target was sent twice, 2026-11-10).
    if (pad & ~end_pad_was_ & hide & end_hidden_was_) say_end(st, e.on);
    end_pad_was_ = pad;
    end_hidden_was_ = hide;
  }
  int bg_state_ = -1;
  // Said the current prompt (state 5 / 11): set when it is said, cleared when the game's state changes. The All-Out
  // state lasts through Shuffle Time and the results, so coming back to this screen is no new prompt.
  bool prompt_said_ = false;

 public:
  BattleScreen() { keys::set_capture([this](uint32_t vk, bool repeat) { return end_key(vk, repeat); }); }
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
    end_pad(core ? gui_state() : 0);
    if (!core) { units_.clear(); won_enemies_.clear(); forget_battle_start(); downed_since_turn_ = false; last_actor_ = {}; rush_ = false; popups_seen_.clear(); return; }
    watch();
    int st = gui_state();
    // Assist (13, BTL_STATE_NAVIGATION) only plays the navigator's advice over the wheel: back from it is the same
    // turn, not the wheel opening again (each press re-read the turn line, 2026-11-01).
    bool entering = st != bg_state_ && bg_state_ != 13;
    // The mod loaded into a battle already in the All-Out state (a hot reload): that state lasts through Shuffle Time
    // and the results, so it is taken as answered (the prompt was said again after Shuffle Time, 2026-11-09).
    bool loaded_into = bg_state_ == -1 && st == 5;
    bg_state_ = st;
    if (entering) prompt_said_ = loaded_into;
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
        std::string s = analyze_line(t);
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

// The battle's lists as bespoke menus (menu.h). Their state is reflected:
//   state 2 skills    BP_BtlSkillList_C      SkillIDList, SelectCursor (the index confirm uses; Q / E switch the
//                     equipped Persona, IsChangingPersona meanwhile)
//   state 3 items     BP_BtlItemList_C       ItemList (FBtlItemData: ID, Num, Name), CurrentIndex
//   state 4 targets   BP_BtlTargetSelect_C   targetList, TargetIndex, TargetType (not 0: the whole list is taken)
//   state 6 Survey    BP_BtlTacticsCheck_C   CursorList, TargetIndex
//   state 7 Theurgy   BP_BtlTheurgiaList_C   ItemList (skill ids), CurrentIndex
//   state 9 Shift     BP_BtlGuiShiftSelect_C PlayerList, TargetIndex
//   state 10 Tactics  BP_BtlStrategyInstruct_C inputState (0 Order/Escape, 1 member, 2 order), CursorIndex[level],
//                     PlayerList, OperationNameList (the game's English names); a member's order: GetOperation
// The game's state (gui_state) and whose turn it is (acting_member) come from battle_state.h. A skill's description
// is its help line from the game's file (persona::skill_help): the skill list keeps it for the detail buffer
// (battle_details.cpp) and the review key, Theurgy reads it after the name.
//
// Moving the game's cursors (no keys): the lists are actors (BP_BtlGuiBaseActor_C), not widgets.
// The native lists (skills, items, Theurgy, Tactics) take UBtlGuiListComponent.SetCursor(CurrentIndex), which moves
// no slide and plays no sound: the list's own cursor-update event (what its OnCurcorUpdate runs) follows it. Targets
// and Survey run their own move events (camera, marker, info panel and the targets the game acts on); Shift writes
// TargetIndex and runs UpdateTargets. A list takes moves while its state is current, set up and bound (gui_ready).
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/battle_state.h"
#include "screens/battle_words.h"
#include "core/strings.h"
#include "items.h"
#include "persona.h"
#include "speech.h"
#include <format>

namespace p3a::screens {
using namespace ue;

namespace {
// The GUI actor `a` takes moves: its state is the game's, set up and bound (BP_BtlGuiBaseActor_C).
bool gui_ready(UObject* a, int state) {
  return a && gui_state() == state && get_bool(a, "isStateInit") && get<uint8_t>(a, "State") == get<uint8_t>(a, "MainState") &&
         !get_bool(a, "BindFlag");
}
// UBtlGuiListComponent.SetCursor on the actor's list `comp`: the index, scrolled like the game's own move.
bool list_set(UObject* a, const char* comp, int i) {
  UObject* c = a ? get<UObject*>(a, comp) : nullptr;
  UFunction* f = c ? find_function(class_of(c), "SetCursor") : nullptr;
  if (!f) return false;
  Params p(f);
  p.set<int32_t>("CurrentIndex", i);
  return call(c, p);
}
constexpr int kTargetSound = 224;   // BtlSEPlay(224): a target change
}  // namespace

// One battle list: open while the GUI state is `state`.
class BattleList : public MenuScreen {
 protected:
  BtlWords& w_ = btl_words();
  int state_;
  Finder list_;
  // The list actor's array of units `prop` (the targets, the party), null while the actor is not there.
  TArray<UObject*>* units(const char* prop) { UObject* l = list_.get(); return l ? field<TArray<UObject*>>(l, prop) : nullptr; }
  const strings::Hint& keys_;   // F1
  BattleList(int state, const char* cls, const strings::Hint& keys = strings::kListKeys) : state_(state), list_(cls), keys_(keys) {}
 public:
  int priority() const override { return 26; }
  void on_enter() override { screens::set_keys(keys_); }
  bool active() override { return gui_state() == state_ && list_.get() != nullptr; }
};

class BattleSkills : public BattleList {
  // SelectCursor: the index confirm uses (CursorIndex is the row on screen).
  Mirror& m_ = mirror("skills", [this] { UObject* l = list_.get(); return l ? get<int32_t>(l, "SelectCursor") : -1; });
  int persona() { return w_.unit_int("GetPersonaEquipID", acting_member()); }
  TArray<int32_t>* skill_ids() { UObject* l = list_.get(); return l ? field<TArray<int32_t>>(l, "SkillIDList") : nullptr; }
  // A skill's row: its name and what it costs `who` now.
  std::string row(int id, UObject* who) {
    std::string s = w_.skill_name(id), cost = w_.skill_cost(id, who);
    return cost.empty() ? s : s + ", " + cost;
  }
 public:
  BattleSkills() : BattleList(2, "BP_BtlSkillList_C", strings::kSkillKeys) {
    // Not while Q / E change the Persona (that moves BtlGuiListPersona).
    m_.ready = [this] { UObject* l = list_.get(); return gui_ready(l, 2) && !get_bool(l, "IsChangingPersona"); };
    m_.set = [this](int i) {
      UObject* l = list_.get();
      if (!list_set(l, "BtlGuiList", i)) return false;
      call(l, "CallCursorUpdate");   // the cursor's slide
      call(l, "PlayCursorSE");
      return true;
    };
  }
  const char* id() const override { return "battle_skills"; }
  std::string title() override { return strings::kSkills; }
  void build(ui::GraphBuilder& b) override {
    auto* ids = skill_ids();
    if (!ids) return;
    // The equipped Persona heads the list; switching it (Q / E) is a new list.
    int pid = persona();
    b.begin_stop("skills");
    b.push_context(pid > 0 ? w_.persona_name(pid) : std::string(strings::kSkills));
    UObject* who = acting_member();
    for (int i = 0; i < ids->num; ++i) {
      int id = ids->data[i];
      std::string s = row(id, who);
      // Name and cost alone: the description was heard on every move through skills cast dozens of times a night. It
      // is the detail buffer's second line, and the review key still reads it (the user's call, 2026-11-11).
      b.add_item(slot(std::format("skills.{}", pid), i), game_item(m_, i, [s] { return s; }, [id] { return persona::skill_help(id); }));
    }
    b.pop_context();
    if (int c = m_.cursor(); c >= 0) b.set_start(slot(std::format("skills.{}", pid), c));
  }
  void on_tick() override { m_.key = std::format("skills.{}", persona()); }
  // The focused skill, what it does, then the acting member's own details.
  int focused_skill() { return focused_row(m_.key); }
  bool has_details() override { return focused_skill() >= 0; }
  std::vector<std::string> detail_lines() override {
    auto* ids = skill_ids();
    int i = focused_skill();
    if (!ids || i < 0 || i >= ids->num) return {};
    int id = ids->data[i];
    UObject* who = acting_member();
    std::vector<std::string> out{row(id, who)};
    if (std::string help = persona::skill_help(id); !help.empty()) out.push_back(help);
    for (auto& d : w_.details(who)) out.push_back(d);
    return out;
  }
};

class BattleItems : public BattleList {
  // CurrentIndex: the item confirm uses (CursorIndex is the row of the 8 on screen).
  Mirror& m_ = mirror("items", [this] { UObject* l = list_.get(); return l ? get<int32_t>(l, "CurrentIndex") : -1; });
 public:
  BattleItems() : BattleList(3, "BP_BtlItemList_C") {
    m_.ready = [this] { UObject* l = list_.get(); return gui_ready(l, 3) && !get_bool(l, "IsPlayingAnimation"); };
    m_.set = [this](int i) {
      UObject* l = list_.get();
      if (!list_set(l, "BtlGuiList", i)) return false;
      call(l, "ChangeCursor");   // the cursor's slide
      call(l, "PlsyCursorSE");   // (sic)
      return true;
    };
  }
  const char* id() const override { return "battle_items"; }
  std::string title() override { return strings::kItems; }
  void build(ui::GraphBuilder& b) override {
    UObject* l = list_.get();
    auto* list = l ? field<TArray<uint8_t>>(l, "ItemList") : nullptr;
    FField* ip = l ? find_prop(class_of(l), "ItemList") : nullptr;
    if (!list || !ip) return;
    UStruct* st = prop_inner_object(prop_inner_prop(ip));
    int size = struct_size(st), name = offset_of(st, "Name"), num = offset_of(st, "Num"), idp = offset_of(st, "ID");
    b.begin_stop("items");
    int c = m_.cursor();
    for (int i = 0; i < list->num; ++i) {
      uint8_t* it = list->data + i * size;
      std::string s = std::format("{}, {}", to_utf8(*(FString*)(it + name)), *(int32_t*)(it + num));
      // Its help line from the item's file (FBtlItemData.ID is the full item id the command takes).
      std::string help = idp >= 0 ? items::help(*(int32_t*)(it + idp)) : std::string();
      auto v = game_item(m_, i, [s] { return s; });
      buffer_with(v, [help] { return help; });
      b.add_item(slot("items", i), v);
    }
    if (c >= 0) b.set_start(slot("items", c));
  }
};

class BattleTargets : public BattleList {
  // A skill or item that takes a whole side (TargetType, EBtlDataTargetType: 0 one target, else the list): the game
  // still steps TargetIndex through the list, but confirm acts on all of it (the target manager's targets are the
  // list, BP_BtlTargetSelect 12608), so the menu is one item that says so and the game's cursor stays where it is
  // (each name alone read as a single target; the players' call, 2026-11-09). The item is a question with its
  // buttons, as Guard's is, and names nobody: each hit says who it landed on (the user's call, 2026-11-11).
  bool all() {
    UObject* l = list_.get();
    auto* list = units("targetList");
    return list && list->num > 1 && get<uint8_t>(l, "TargetType") != 0;
  }
  // What is being used: the GUI's CommandID, an item's id when the list came from the items (state 3), else a skill's
  // (the game's own CheckItemID asks the same of the state before this one).
  std::string command_name() {
    UObject* l = list_.get();
    UObject* gui = l ? get<UObject*>(l, "BtlGui") : nullptr;
    FField* p = gui ? find_prop(class_of(gui), "CommandID") : nullptr;
    int id = p ? get<int32_t>(gui, "CommandID") : -1;
    if (id < 0) return {};
    UObject* mgr = battle_objects().gui.get();
    return mgr && call_ret<uint8_t>(mgr, "GetPrevState") == 3 ? items::name_of(id) : w_.skill_name(id);
  }
  Mirror& m_ = mirror("targets", [this] {
    UObject* l = list_.get();
    return !l ? -1 : all() ? 0 : get<int32_t>(l, "TargetIndex");
  });
 public:
  // Confirm acts on the target manager's targets, which only the move sets (TargetIndex alone is drawn nowhere and
  // acted on by nothing): the game's own "On Select Next Target" runs from the index before i. It plays the sound,
  // sets the targets and the marker, and turns the camera and the info panel.
  BattleTargets() : BattleList(4, "BP_BtlTargetSelect_C", strings::kTargetKeys) {
    m_.keep_edges = true;   // the game's row wraps
    m_.ready = [this] {
      UObject* l = list_.get();
      auto* list = units("targetList");
      return gui_ready(l, 4) && list && list->num > 0 && get_bool(l, "IsCreateTargetList") && get<int32_t>(l, "FrameVisible") == 0;
    };
    m_.set = [this](int i) {
      UObject* l = list_.get();
      auto* list = units("targetList");
      int32_t* ti = l ? field<int32_t>(l, "TargetIndex") : nullptr;
      if (!list || !ti || i < 0 || i >= list->num) return false;
      if (all()) return true;   // one item, nothing to move
      *ti = (i - 1 + list->num) % list->num;
      return call(l, "On Select Next Target");
    };
  }
  const char* id() const override { return "battle_targets"; }
  // A whole side is a question, not a choice: it has no "Choose target" before it.
  std::string title() override { return all() ? std::string() : strings::kChooseTarget; }
  void on_enter() override {
    screens::set_keys([this] { return std::string(all() ? strings::kAllTargetsKeys() : strings::kTargetKeys()); });
  }
  void build(ui::GraphBuilder& b) override {
    auto* list = units("targetList");
    if (!list) return;
    b.begin_stop("targets");
    if (all()) {
      std::vector<WeakObj> units;
      for (int i = 0; i < list->num; ++i) units.push_back(WeakObj::of(list->data[i]));
      auto v = game_item(m_, 0, [this, units] {
        int enemies = 0;
        for (auto& w : units)
          if (UObject* t = w.resolve()) enemies += get_bool(t, "EnemyFlag") ? 1 : 0;
        const char* side = enemies == (int)units.size() ? strings::kAllEnemies : enemies == 0 ? strings::kAllAllies : strings::kAllUnits;
        std::string what = command_name();
        std::string ask = what.empty() ? std::format("{} {} {}?", strings::kUse, strings::kOn, side)
                                       : std::format("{} {} {} {}?", strings::kUse, what, strings::kOn, side);
        return std::format("{} {}", ask, strings::kAllTargetsKeys());
      });
      v->speaks_own_position = true;   // one thing to answer: no "1 of 1"
      b.add_item(slot("targets.all", 0), v);
      b.set_start(slot("targets.all", 0));
      return;
    }
    b.start_row("targets");
    for (int i = 0; i < list->num; ++i) {
      WeakObj wt = WeakObj::of(list->data[i]);
      b.add_item(slot("targets", i), game_item(m_, i, [this, wt] {
        UObject* t = wt.resolve();
        return t ? w_.unit_line(t) : std::string();
      }));
    }
    b.end_row();
    if (int c = m_.cursor(); c >= 0) b.set_start(slot("targets", c));
  }
  void on_tick() override { m_.key = all() ? "targets.all" : "targets"; }
};

class BattleTheurgy : public BattleList {
  Mirror& m_ = mirror("theurgy", [this] { UObject* l = list_.get(); return l ? get<int32_t>(l, "CurrentIndex") : -1; });
 public:
  BattleTheurgy() : BattleList(7, "BP_BtlTheurgiaList_C", strings::kTheurgyKeys) {
    m_.ready = [this] {
      UObject* l = list_.get();
      return gui_ready(l, 7) && get_bool(l, "InAnimEnd") && !get_bool(l, "OpenDetail") && !get_bool(l, "ExitingTopUI") &&
             !get_bool(l, "HeadLookAtHlag");
    };
    m_.set = [this](int i) {
      UObject* l = list_.get();
      int from = m_.cursor();
      if (!list_set(l, "BtlGuiList", i)) return false;
      // CursorUpdate(1 forward, 2 back): the sound, the cursor's slide, the icons and the head's look.
      UFunction* f = find_function(class_of(l), "CursorUpdate");
      if (!f) return true;
      Params p(f);
      p.set<int32_t>("CursorType", i > from ? 1 : 2);
      call(l, p);
      return true;
    };
  }
  const char* id() const override { return "battle_theurgy"; }
  std::string title() override { return strings::kTheurgy; }
  void build(ui::GraphBuilder& b) override {
    UObject* l = list_.get();
    auto* ids = l ? field<TArray<int32_t>>(l, "ItemList") : nullptr;
    if (!ids) return;
    b.begin_stop("theurgy");
    for (int i = 0; i < ids->num; ++i) {
      int id = ids->data[i];
      std::string s = w_.skill_name(id);
      auto v = game_item(m_, i, [s] { return s; });
      describe_with(v, [id] { return persona::skill_help(id); });
      b.add_item(slot("theurgy", i), v);
    }
    if (int c = m_.cursor(); c >= 0) b.set_start(slot("theurgy", c));
  }
};

class BattleShift : public BattleList {
  Mirror& m_ = mirror("shift", [this] { UObject* l = list_.get(); return l ? get<int32_t>(l, "TargetIndex") : -1; });
 public:
  // Confirm reads TargetIndex; UpdateTargets(false) is the rest of the game's move (targets, list, info panel, voice).
  BattleShift() : BattleList(9, "BP_BtlGuiShiftSelect_C", strings::kShiftKeys) {
    m_.keep_edges = true;   // the game's row wraps
    m_.ready = [this] {
      UObject* l = list_.get();
      auto* list = units("PlayerList");
      return gui_ready(l, 9) && list && list->num > 0;
    };
    m_.set = [this](int i) {
      UObject* l = list_.get();
      auto* list = units("PlayerList");
      int32_t* ti = l ? field<int32_t>(l, "TargetIndex") : nullptr;
      UFunction* f = l ? find_function(class_of(l), "UpdateTargets") : nullptr;
      if (!list || !ti || !f || i < 0 || i >= list->num) return false;
      if (list->num > 1) cursor_sound(kTargetSound);
      *ti = i;
      Params p(f);
      p.set<bool>("NoAnimation", false);
      return call(l, p);
    };
  }
  const char* id() const override { return "battle_shift"; }
  std::string title() override { return strings::kShift; }
  void build(ui::GraphBuilder& b) override {
    auto* list = units("PlayerList");
    if (!list) return;
    b.begin_stop("shift");
    b.start_row("shift");
    for (int i = 0; i < list->num; ++i) {
      WeakObj wt = WeakObj::of(list->data[i]);
      b.add_item(slot("shift", i), game_item(m_, i, [this, wt] {
        UObject* t = wt.resolve();
        return t ? std::format("{}, {}", w_.unit_name(t), w_.hp_sp(t)) : std::string();
      }));
    }
    b.end_row();
    if (int c = m_.cursor(); c >= 0) b.set_start(slot("shift", c));
  }
};

// Tactics: three levels, one list each; the level in front is the menu.
class BattleTactics : public BattleList {
  int level() { UObject* s = list_.get(); return s ? get<int32_t>(s, "inputState") : -1; }
  int level_cursor(int lv) {
    UObject* s = list_.get();
    auto* cur = s ? field<TArray<int32_t>>(s, "CursorIndex") : nullptr;
    return cur && lv >= 0 && lv < cur->num && level() == lv ? cur->data[lv] : -1;
  }
  Mirror& m0_ = mirror("tactics0", [this] { return level_cursor(0); });
  Mirror& m1_ = mirror("tactics1", [this] { return level_cursor(1); });
  Mirror& m2_ = mirror("tactics2", [this] { return level_cursor(2); });
  std::string operation_name(int op) {
    UObject* s = list_.get();
    auto* names = s ? field<TArray<FString>>(s, "OperationNameList") : nullptr;
    return names && op >= 0 && op < names->num ? to_utf8(names->data[op]) : std::string();
  }
  int operation_of(UObject* a) {
    UObject* d = w_.db();
    UFunction* f = d && a ? find_function(class_of(d), "GetOperation") : nullptr;
    if (!f) return -1;
    Params p(f);
    p.set<UObject*>("Action", a);
    call(d, p);
    return p.ret<int32_t>();
  }
  // A row that cannot be chosen now: it says so, and so does confirming it.
  static ui::NodeVtablePtr unavailable(std::function<std::string()> label) {
    return item([label] { return std::format("{}, {}", label(), strings::kUnavailable); },
                [] { speech::speak(strings::kUnavailable, true); });
  }
 public:
  BattleTactics() : BattleList(10, "BP_BtlStrategyInstruct_C") {
    // Each level's list (BtlGuiList, 1, 2) and its cursor event (CursorUpdate1..3: the slide and the sound). The game
    // updates the lists only once its keys are bound (KeyBindCompleted, 0.1 s after the state opens).
    Mirror* ms[] = {&m0_, &m1_, &m2_};
    for (int lv = 0; lv < 3; ++lv) {
      ms[lv]->ready = [this, lv] { UObject* s = list_.get(); return gui_ready(s, 10) && get_bool(s, "KeyBindCompleted") && level() == lv; };
      ms[lv]->set = [this, lv](int i) {
        static const char* lists[] = {"BtlGuiList", "BtlGuiList1", "BtlGuiList2"};
        UObject* s = list_.get();
        if (level() != lv || !list_set(s, lists[lv], i)) return false;
        call(s, std::format("CursorUpdate{}", lv + 1));
        return true;
      };
    }
  }
  const char* id() const override { return "battle_tactics"; }
  std::string title() override { return strings::kTacticsTitle; }
  void build(ui::GraphBuilder& b) override {
    UObject* s = list_.get();
    if (!s) return;
    int lv = level();
    if (lv == 0) {
      b.begin_stop("tactics0");
      // Order is the game's first item only while orders can be changed (EnableStrategyChange: not on a turn where
      // only the leader acts); otherwise it is hidden and Escape is item 0 (CallOK, 2026-10-31: a confirm on "Order"
      // there tried to flee). Escape works only where the battle allows it (permissionEscape; not in a boss fight).
      bool order = get_bool(s, "EnableStrategyChange");
      auto off = [&](const char* id, const char* name) {
        b.add_item(ui::ControlId::structural(id), unavailable([name] { return std::string(name); }));
      };
      if (order) b.add_item(slot("tactics0", 0), game_item(m0_, 0, [] { return std::string(strings::kTacticsOrder); }));
      else off("tactics0.order", strings::kTacticsOrder);
      if (get_bool(s, "permissionEscape") || !order) {
        int e = order ? 1 : 0;
        b.add_item(slot("tactics0", e), game_item(m0_, e, [] { return std::string(strings::kTacticsEscape); }));
      } else {
        off("tactics0.escape", strings::kTacticsEscape);
      }
      if (int c = level_cursor(0); c >= 0) b.set_start(slot("tactics0", c));
    } else if (lv == 1) {
      auto* party = field<TArray<UObject*>>(s, "PlayerList");
      if (!party) return;
      b.begin_stop("tactics1");
      b.push_context(strings::kTacticsWho);
      // Item 0 orders everyone; member i is item i + 1 (CallOK reads PlayerList[CurrentIndex[1] - 1]).
      auto* on = field<TArray<bool>>(s, "Lise1Enable");   // items whose orders can be changed (by item)
      for (int i = 0; i <= party->num; ++i) {
        std::function<std::string()> label = [] { return std::string(strings::kTacticsAll); };
        if (i > 0) {
          WeakObj wm = WeakObj::of(party->data[i - 1]);
          label = [this, wm] {
            UObject* m = wm.resolve();
            return m ? std::format("{}, {}", w_.unit_name(m), operation_name(operation_of(m))) : std::string();
          };
        }
        if (on && i < on->num && !on->data[i])
          b.add_item(ui::ControlId::structural(std::format("tactics1.off{}", i)), unavailable(label));
        else b.add_item(slot("tactics1", i), game_item(m1_, i, label));
      }
      b.pop_context();
      if (int c = level_cursor(1); c >= 0) b.set_start(slot("tactics1", c));
    } else if (lv == 2) {
      auto* names = field<TArray<FString>>(s, "OperationNameList");
      if (!names) return;
      b.begin_stop("tactics2");
      b.push_context(strings::kTacticsWhich);
      for (int i = 0; i < names->num; ++i) {
        std::string n = to_utf8(names->data[i]);
        b.add_item(slot("tactics2", i), game_item(m2_, i, [n] { return n; }));
      }
      b.pop_context();
      if (int c = level_cursor(2); c >= 0) b.set_start(slot("tactics2", c));
    }
  }
};

// Survey (state 6, S on the wheel): BP_BtlTacticsCheck_C CursorList / TargetIndex over everyone in the battle, each
// with their health and what the analysis knows. Up and down go through the list (the game's moveCursor(actor): the
// sound, the marker, the info panel). Left and right do nothing: the game's move over the field by position jumped
// the focus around the list (keep_edges holds them, 2026-11-01).
class BattleSurvey : public BattleList {
  Mirror& m_ = mirror("survey", [this] { UObject* l = list_.get(); return l ? get<int32_t>(l, "TargetIndex") : -1; });
 public:
  BattleSurvey() : BattleList(6, "BP_BtlTacticsCheck_C", strings::kSurveyKeys) {
    m_.keep_edges = true;
    m_.ready = [this] {
      UObject* l = list_.get();
      auto* list = units("CursorList");
      return gui_ready(l, 6) && list && list->num > 0;
    };
    m_.set = [this](int i) {
      UObject* l = list_.get();
      auto* list = units("CursorList");
      UFunction* f = l ? find_function(class_of(l), "moveCursor") : nullptr;
      if (!list || !f || i < 0 || i >= list->num || !list->data[i]) return false;
      Params p(f);
      p.set<UObject*>("In", list->data[i]);
      return call(l, p);
    };
  }
  const char* id() const override { return "battle_survey"; }
  std::string title() override { return strings::kSurveyTitle; }
  void build(ui::GraphBuilder& b) override {
    auto* list = units("CursorList");
    if (!list) return;
    b.begin_stop("survey");
    for (int i = 0; i < list->num; ++i) {
      WeakObj wt = WeakObj::of(list->data[i]);
      b.add_item(slot("survey", i), game_item(m_, i, [this, wt] {
        UObject* t = wt.resolve();
        return t ? w_.unit_line(t) : std::string();
      }));
    }
    if (int c = m_.cursor(); c >= 0) b.set_start(slot("survey", c));
  }
};

std::unique_ptr<Screen> make_battle_survey() { return std::make_unique<BattleSurvey>(); }
std::unique_ptr<Screen> make_battle_skills() { return std::make_unique<BattleSkills>(); }
std::unique_ptr<Screen> make_battle_items() { return std::make_unique<BattleItems>(); }
std::unique_ptr<Screen> make_battle_targets() { return std::make_unique<BattleTargets>(); }
std::unique_ptr<Screen> make_battle_theurgy() { return std::make_unique<BattleTheurgy>(); }
std::unique_ptr<Screen> make_battle_shift() { return std::make_unique<BattleShift>(); }
std::unique_ptr<Screen> make_battle_tactics() { return std::make_unique<BattleTactics>(); }
}  // namespace p3a::screens

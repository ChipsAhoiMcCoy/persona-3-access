// The battle's lists as bespoke menus (menu.h). Their state is reflected (battle.cpp's header has the classes):
//   state 2 skills    BP_BtlSkillList_C      SkillIDList, CursorIndex (Q / E switch the equipped Persona)
//   state 3 items     BP_BtlItemList_C       ItemList (FBtlItemData: ID, Num, Name), CursorIndex
//   state 4 targets   BP_BtlTargetSelect_C   targetList, TargetIndex
//   state 7 Theurgy   BP_BtlTheurgiaList_C   ItemList (skill ids), CursorIndex
//   state 9 Shift     BP_BtlGuiShiftSelect_C PlayerList, TargetIndex
//   state 10 Tactics  BP_BtlStrategyInstruct_C inputState (0 Order/Escape, 1 member, 2 order), CursorIndex[level],
//                     PlayerList, OperationNameList (the game's English names); a member's order: GetOperation
// The game's state is ABtlGuiStateManager.nowState. A skill's description is the BMD page whose dialog number is its
// id, built once the game's cursor is on it: read after the name when it comes.
//
// Moving the game's cursors (no keys): the lists are actors (BP_BtlGuiBaseActor_C), not widgets.
// The native lists (skills, items, Theurgy, Tactics) take UBtlGuiListComponent.SetCursor(CurrentIndex), which moves
// no slide and plays no sound: the list's own cursor-update event (what its OnCurcorUpdate runs) follows it. Targets
// and Survey run their own move events (camera, marker, info panel and the targets the game acts on); Shift writes
// TargetIndex and runs UpdateTargets. A list takes moves while its state is current, set up and bound (gui_ready).
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/battle_words.h"
#include "screens/battle.h"
#include "core/strings.h"
#include "items.h"
#include "persona.h"
#include "devserver.h"
#include "input.h"
#include "msg.h"
#include "seh.h"
#include "speech.h"
#include "ue/scan.h"
#include <windows.h>
#include <algorithm>
#include <format>

namespace p3a::screens {
using namespace ue;

namespace {
Finder& gui_state_mgr() { static Finder f{"BtlGuiStateManager"}; return f; }
int gui_state() { UObject* g = gui_state_mgr().get(); return g ? get<uint8_t>(g, "nowState") : 0; }
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
  BtlWords w_;
  int state_;
  Finder list_;
  Finder tmgr_{"BP_BtlTargetManager_C"};
  UObject* actor() { UObject* m = tmgr_.get(); return m ? get<UObject*>(m, "MainCharacter") : nullptr; }
  const strings::Hint& keys_;   // F1
  BattleList(int state, const char* cls, const strings::Hint& keys = strings::kListKeys) : state_(state), list_(cls), keys_(keys) {}
 public:
  int priority() const override { return 26; }
  void on_enter() override { screens::set_keys([this] { return std::string(keys_()) + " " + strings::kDetailKeys(); }); }
  bool active() override { return gui_state() == state_ && list_.get() != nullptr; }
};

class BattleSkills : public BattleList {
  // SelectCursor: the index confirm uses (CursorIndex is the row on screen).
  Mirror& m_ = mirror("skills", [this] { UObject* l = list_.get(); return l ? get<int32_t>(l, "SelectCursor") : -1; });
  int persona() { return w_.unit_int("GetPersonaEquipID", actor()); }
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
    UObject* l = list_.get();
    auto* ids = l ? field<TArray<int32_t>>(l, "SkillIDList") : nullptr;
    if (!ids) return;
    // The equipped Persona heads the list; switching it (Q / E) is a new list.
    int pid = persona();
    b.begin_stop("skills");
    b.push_context(pid > 0 ? w_.persona_name(pid) : std::string(strings::kSkills));
    UObject* who = actor();
    for (int i = 0; i < ids->num; ++i) {
      int id = ids->data[i];
      std::string s = w_.skill_name(id);
      std::string cost = w_.skill_cost(id, who);
      if (!cost.empty()) s += ", " + cost;
      auto v = game_item(m_, i, [s] { return s; });
      describe_with(v, [id] { return persona::skill_help(id); });
      b.add_item(slot(std::format("skills.{}", pid), i), v);
    }
    b.pop_context();
    if (int c = m_.cursor(); c >= 0) b.set_start(slot(std::format("skills.{}", pid), c));
  }
  void on_tick() override { m_.key = std::format("skills.{}", persona()); }
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
      describe_with(v, [help] { return help; });
      b.add_item(slot("items", i), v);
    }
    if (c >= 0) b.set_start(slot("items", c));
  }
};

class BattleTargets : public BattleList {
  // A skill or item that takes a whole side (TargetType, EBtlDataTargetType: 0 one target, else the list): the game
  // still steps TargetIndex through the list, but confirm acts on all of it (the target manager's targets are the
  // list, BP_BtlTargetSelect 12608), so the menu is one item that says so and the game's cursor stays where it is
  // (each name alone read as a single target; the players' call, 2026-11-09).
  bool all() {
    UObject* l = list_.get();
    auto* list = l ? field<TArray<UObject*>>(l, "targetList") : nullptr;
    return list && list->num > 1 && get<uint8_t>(l, "TargetType") != 0;
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
      auto* list = l ? field<TArray<UObject*>>(l, "targetList") : nullptr;
      return gui_ready(l, 4) && list && list->num > 0 && get_bool(l, "IsCreateTargetList") && get<int32_t>(l, "FrameVisible") == 0;
    };
    m_.set = [this](int i) {
      UObject* l = list_.get();
      auto* list = l ? field<TArray<UObject*>>(l, "targetList") : nullptr;
      int32_t* ti = l ? field<int32_t>(l, "TargetIndex") : nullptr;
      if (!list || !ti || i < 0 || i >= list->num) return false;
      if (all()) return true;   // one item, nothing to move
      *ti = (i - 1 + list->num) % list->num;
      return call(l, "On Select Next Target");
    };
  }
  const char* id() const override { return "battle_targets"; }
  std::string title() override { return strings::kChooseTarget; }
  void build(ui::GraphBuilder& b) override {
    UObject* l = list_.get();
    auto* list = l ? field<TArray<UObject*>>(l, "targetList") : nullptr;
    if (!list) return;
    b.begin_stop("targets");
    if (all()) {
      std::vector<WeakObj> units;
      for (int i = 0; i < list->num; ++i) units.push_back(WeakObj::of(list->data[i]));
      b.add_item(slot("targets.all", 0), game_item(m_, 0, [this, units] {
        int enemies = 0;
        std::string names;
        for (auto& w : units) {
          UObject* t = w.resolve();
          if (!t) continue;
          enemies += get_bool(t, "EnemyFlag") ? 1 : 0;
          names += (names.empty() ? "" : ", ") + w_.unit_name(t);
        }
        const char* side = enemies == (int)units.size() ? strings::kAllEnemies : enemies == 0 ? strings::kAllAllies : strings::kAllUnits;
        return std::format("{}: {}", side, names);
      }));
      b.set_start(slot("targets.all", 0));
      return;
    }
    b.start_row("targets");
    for (int i = 0; i < list->num; ++i) {
      UObject* t = list->data[i];
      WeakObj wt = WeakObj::of(t);
      b.add_item(slot("targets", i), game_item(m_, i, [this, wt] {
        UObject* t = wt.resolve();
        if (!t) return std::string();
        return get_bool(t, "EnemyFlag") ? w_.enemy(t) : std::format("{}, {}", w_.unit_name(t), w_.hp_sp(t));
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
      auto* list = l ? field<TArray<UObject*>>(l, "PlayerList") : nullptr;
      return gui_ready(l, 9) && list && list->num > 0;
    };
    m_.set = [this](int i) {
      UObject* l = list_.get();
      auto* list = l ? field<TArray<UObject*>>(l, "PlayerList") : nullptr;
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
    UObject* l = list_.get();
    auto* list = l ? field<TArray<UObject*>>(l, "PlayerList") : nullptr;
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
  int cursor_at(int lv) {
    UObject* s = list_.get();
    auto* cur = s ? field<TArray<int32_t>>(s, "CursorIndex") : nullptr;
    return cur && lv >= 0 && lv < cur->num && level() == lv ? cur->data[lv] : -1;
  }
  Mirror& m0_ = mirror("tactics0", [this] { return cursor_at(0); });
  Mirror& m1_ = mirror("tactics1", [this] { return cursor_at(1); });
  Mirror& m2_ = mirror("tactics2", [this] { return cursor_at(2); });
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
      auto unavailable = [&](const char* id, const char* name) {
        b.add_item(ui::ControlId::structural(id), item([name] { return std::format("{}, {}", name, strings::kUnavailable); },
                                                       [] { speech::speak(strings::kUnavailable, true); }));
      };
      if (order) b.add_item(slot("tactics0", 0), game_item(m0_, 0, [] { return std::string(strings::kTacticsOrder); }));
      else unavailable("tactics0.order", strings::kTacticsOrder);
      if (get_bool(s, "permissionEscape") || !order) {
        int e = order ? 1 : 0;
        b.add_item(slot("tactics0", e), game_item(m0_, e, [] { return std::string(strings::kTacticsEscape); }));
      } else {
        unavailable("tactics0.escape", strings::kTacticsEscape);
      }
      if (int c = cursor_at(0); c >= 0) b.set_start(slot("tactics0", c));
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
          b.add_item(ui::ControlId::structural(std::format("tactics1.off{}", i)), item([label] { return std::format("{}, {}", label(), strings::kUnavailable); },
                                                                                      [] { speech::speak(strings::kUnavailable, true); }));
        else b.add_item(slot("tactics1", i), game_item(m1_, i, label));
      }
      b.pop_context();
      if (int c = cursor_at(1); c >= 0) b.set_start(slot("tactics1", c));
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
      if (int c = cursor_at(2); c >= 0) b.set_start(slot("tactics2", c));
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
      auto* list = l ? field<TArray<UObject*>>(l, "CursorList") : nullptr;
      return gui_ready(l, 6) && list && list->num > 0;
    };
    m_.set = [this](int i) {
      UObject* l = list_.get();
      auto* list = l ? field<TArray<UObject*>>(l, "CursorList") : nullptr;
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
    UObject* l = list_.get();
    auto* list = l ? field<TArray<UObject*>>(l, "CursorList") : nullptr;
    if (!list) return;
    b.begin_stop("survey");
    for (int i = 0; i < list->num; ++i) {
      WeakObj wt = WeakObj::of(list->data[i]);
      b.add_item(slot("survey", i), game_item(m_, i, [this, wt] {
        UObject* t = wt.resolve();
        if (!t) return std::string();
        return get_bool(t, "EnemyFlag") ? w_.enemy(t) : std::format("{}, {}", w_.unit_name(t), w_.hp_sp(t));
      }));
    }
    if (int c = m_.cursor(); c >= 0) b.set_start(slot("survey", c));
  }
};

// The details of whoever is in focus (the Say the Spire model): the right stick up and down, or Ctrl+Up and Ctrl+Down,
// walk the lines of BtlWords::details from the top, the unit's name: down goes on, up goes back (the user's call,
// 2026-11-05); each read is fresh, and landing on someone starts at their top again. Who is in focus is the game's own
// cursor in each state; a list of the acting member's own (skills, items) reads them. A flick is one line. The stick is
// hidden from the game only while someone is in focus; no battle Blueprint reads it.
namespace {
BtlWords& buffer_words() { static BtlWords w; return w; }
Finder& target_list() { static Finder f{"BP_BtlTargetSelect_C"}; return f; }
Finder& survey_list() { static Finder f{"BP_BtlTacticsCheck_C"}; return f; }
Finder& shift_list() { static Finder f{"BP_BtlGuiShiftSelect_C"}; return f; }
Finder& tactics_list() { static Finder f{"BP_BtlStrategyInstruct_C"}; return f; }
Finder& target_mgr() { static Finder f{"BP_BtlTargetManager_C"}; return f; }
Finder& gui_comp() { static Finder f{"BP_BtlGuiComponent_C"}; return f; }
UObject* at(UObject* o, const char* list, const char* index) {
  auto* l = o ? field<TArray<UObject*>>(o, list) : nullptr;
  int32_t* i = o ? field<int32_t>(o, index) : nullptr;
  return l && i && *i >= 0 && *i < l->num ? l->data[*i] : nullptr;
}
UObject* focused_unit() {
  int st = gui_state();
  if (!st) return nullptr;
  UObject* m = target_mgr().get();
  UObject* acting = m ? get<UObject*>(m, "MainCharacter") : nullptr;
  auto cursor_target = [&]() -> UObject* {
    auto* a = m ? field<TArray<UObject*>>(m, "CursorTargets") : nullptr;
    return a && a->num > 0 ? a->data[0] : nullptr;
  };
  switch (st) {
    case 1: {   // the wheel: the target left and right change
      UObject* t = at(gui_comp().get(), "EnemyList", "Target Index");
      return t ? t : cursor_target();
    }
    case 2: case 3: case 7: return acting;
    case 4: return at(target_list().get(), "targetList", "TargetIndex");
    case 6: return at(survey_list().get(), "CursorList", "TargetIndex");
    case 8: return cursor_target();
    case 9: return at(shift_list().get(), "PlayerList", "TargetIndex");
    case 10: {
      UObject* s = tactics_list().get();
      auto* cur = s ? field<TArray<int32_t>>(s, "CursorIndex") : nullptr;
      if (!s || get<int32_t>(s, "inputState") != 1 || !cur || cur->num < 2) return acting;
      auto* party = field<TArray<UObject*>>(s, "PlayerList");
      int k = cur->data[1] - 1;   // item 0 is everyone
      return party && k >= 0 && k < party->num ? party->data[k] : acting;
    }
    default: return nullptr;
  }
}
WeakObj g_detail_unit;
int g_detail_line = 0;
void detail_landing(UObject* u) {
  WeakObj w = WeakObj::of(u);
  if (!(w == g_detail_unit)) { g_detail_unit = w; g_detail_line = 0; }
}
std::string detail_step(int dir) {
  UObject* u = focused_unit();
  if (!u) return {};
  detail_landing(u);
  auto lines = buffer_words().details(u);
  if (lines.empty()) return {};
  g_detail_line = std::clamp(g_detail_line + dir, 0, (int)lines.size() - 1);
  return lines[g_detail_line];
}
}  // namespace

namespace battle_details {
bool in_focus() { bool on = false; guarded([&] { on = focused_unit() != nullptr; }); return on && !msg::current().blocking; }
void step(int dir) {
  std::string l;
  guarded([&] { l = detail_step(dir); });
  if (!l.empty()) speech::speak(l, true);
}
void frame() {
  guarded([] { detail_landing(focused_unit()); });
  bool on = in_focus() && !input::modal();
  input::claim_right_stick(on);
  if (int d = input::take_right_flick(); d && on) step(-d);   // a flick up is +1: back toward the top
}
void install() {
  dev::route("/battle/details", [](const dev::Request& r, int&) {
    int d = (int)dev::arg_int(r, "step", 0);
    int give = (int)dev::arg_int(r, "ailment", -1);
    return dev::on_game([d, give] {
      std::string out;
      guarded([&] {
        UObject* u = focused_unit();
        // ?ailment=N: type N on the focused unit by the game's own route: its flag (SetBadStatus) and then the turn
        // counters the battle gives it (FUN_140fe2ef0(actor, bit), +0x38E and +0x3A6), for testing what a skill would do.
        if (give >= 0 && give < 12 && u) {
          UObject* db = buffer_words().db();
          UFunction* f = db ? find_function(class_of(db), "SetBadStatus") : nullptr;
          if (f) { Params p(f); p.set<UObject*>("Actor", u); p.set<int32_t>("Type", give); call(db, p); }
          ((void (*)(void*, uint32_t))(scan::exe_base() + 0xfe2ef0))(u, 1u << give);
        }
        if (d) step(d);
        if (u) for (auto& l : buffer_words().details(u)) out += l + "\n";
        out += std::format("line {}\n", g_detail_line);
      });
      return out;
    });
  }, "?step=1|-1 ?ailment=N the focused battle unit's detail lines (what the right stick reads)");
}
}  // namespace battle_details

std::unique_ptr<Screen> make_battle_survey() { return std::make_unique<BattleSurvey>(); }
std::unique_ptr<Screen> make_battle_skills() { return std::make_unique<BattleSkills>(); }
std::unique_ptr<Screen> make_battle_items() { return std::make_unique<BattleItems>(); }
std::unique_ptr<Screen> make_battle_targets() { return std::make_unique<BattleTargets>(); }
std::unique_ptr<Screen> make_battle_theurgy() { return std::make_unique<BattleTheurgy>(); }
std::unique_ptr<Screen> make_battle_shift() { return std::make_unique<BattleShift>(); }
std::unique_ptr<Screen> make_battle_tactics() { return std::make_unique<BattleTactics>(); }
}  // namespace p3a::screens

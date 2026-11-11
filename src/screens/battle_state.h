#pragma once
// The battle's state as its GUI objects hold it, shared by the battle screen (battle.cpp), its lists
// (battle_menus.cpp), the detail buffer (battle_details.cpp) and the victory screen (battle_result.cpp):
//   ABtlGuiStateManager.nowState      EBtlGuiState (TOP 1, SKILL 2, ITEM 3, TARGET 4, ...)
//   BP_BtlTargetManager_C             MainCharacter (whose turn), CursorTargets (the target the game's cursor is on)
//   BP_BtlGuiComponent_C              EnemyList (the wheel's sorted live targets), "Target Index" (the wheel's cursor)
//   UBtlCoreComponent                 CurrentPhase (a BtlPhaseResult once the battle is won), PlayerList, EnemyList
#include "screens/util.h"

namespace p3a::screens {
struct BattleObjects {
  Finder gui{"BtlGuiStateManager"}, target_mgr{"BP_BtlTargetManager_C"}, gui_comp{"BP_BtlGuiComponent_C"}, core{"BtlCoreComponent"};
};
inline BattleObjects& battle_objects() { static BattleObjects o; return o; }

// The GUI's state: 0 when no battle menu is up (or no battle).
inline int gui_state() {
  ue::UObject* g = battle_objects().gui.get();
  return g ? ue::get<uint8_t>(g, "nowState") : 0;
}
// Whose turn it is.
inline ue::UObject* acting_member() {
  ue::UObject* m = battle_objects().target_mgr.get();
  return m ? ue::get<ue::UObject*>(m, "MainCharacter") : nullptr;
}
// The first of the target manager's targets: what the game's cursor is on (Analyze; the wheel before its list is built).
inline ue::UObject* cursor_target() {
  ue::UObject* m = battle_objects().target_mgr.get();
  auto* a = m ? ue::field<ue::TArray<ue::UObject*>>(m, "CursorTargets") : nullptr;
  return a && a->num > 0 ? a->data[0] : nullptr;
}
// The wheel's targets, in the order left and right go through them.
inline ue::TArray<ue::UObject*>* wheel_enemies() {
  ue::UObject* g = battle_objects().gui_comp.get();
  return g ? ue::field<ue::TArray<ue::UObject*>>(g, "EnemyList") : nullptr;
}
// The wheel's target: EnemyList["Target Index"], which only the player's left and right (and a new turn's list)
// move. CursorTargets also follows other things (it flickered between enemies while a battle opened, 2026-10-27).
inline ue::UObject* wheel_target() {
  ue::UObject* g = battle_objects().gui_comp.get();
  auto* l = wheel_enemies();
  int32_t* i = g ? ue::field<int32_t>(g, "Target Index") : nullptr;
  if (l && i && *i >= 0 && *i < l->num && l->data[*i]) return l->data[*i];
  return cursor_target();
}
// The battle's core component, there for as long as a battle exists (Tartarus keeps it between fights).
inline ue::UObject* battle_core() { return battle_objects().core.get(); }
// The result phase while it is the battle's current one (the battle is won), or null.
inline ue::UObject* result_phase() {
  ue::UObject* c = battle_core();
  ue::UObject* phase = c ? ue::get<ue::UObject*>(c, "CurrentPhase") : nullptr;
  return phase && ue::is_a(phase, "BtlPhaseResult") ? phase : nullptr;
}
}  // namespace p3a::screens

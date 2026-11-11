#pragma once
// The camp menu's place among the game's pauses (camp.cpp).
namespace p3a::screens {
int pause_top();            // the innermost pause, an EAppPauseType: 1 Camp, 2 TownMap, 3 DialogueLog; 0 none
bool camp_open();           // the camp menu is the innermost pause
bool camp_submenu_open();   // one of the camp root's submenus is open
}  // namespace p3a::screens

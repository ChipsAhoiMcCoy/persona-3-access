#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <string>

namespace p3a::screens {
// Party members' levels when the current battle began (by unit id), and their names: the battle screen records
// them as the battle starts, the victory screen (battle_result.cpp) reads the names and clears the levels.
extern int g_start_level[32];
extern std::string g_party_name[32];
// Every party Persona as the battle began (by Persona id: the hero's stock, each member's own), so the victory
// screen's Persona pages can say what rose and what was learned. Cleared with g_start_level.
struct StartPersona { int level = 0; std::array<uint8_t, 5> stats{}; std::array<uint16_t, 8> skills{}; };
extern std::map<int, StartPersona> g_start_personas;
void forget_battle_start();
// The live Shuffle Time actor (cards dealt), or null (shuffle.cpp).
}  // namespace p3a::screens
namespace p3a::ue { struct UObject; }
namespace p3a::screens {
ue::UObject* shuffle_ui();
// The victory screen's UI (the result phase's ABtlResultUIBase) and its EBtlResultState (-1 without one); Shuffle Time
// comes before it, so a result state of 1 or more ends the deal (battle_result.cpp).
ue::UObject* result_ui();
int result_state(ue::UObject* ui);
}  // namespace p3a::screens

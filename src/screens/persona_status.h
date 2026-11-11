#pragma once
// The Persona status page as menu lines (persona_status.cpp), shared by every screen that shows one: the game's own
// page, the camp's Persona list, a fusion's result, a level up, Shuffle Time's Persona.
#include "core/control_id.h"
#include "core/graph_builder.h"
#include <cstdint>
#include <string>

namespace p3a::screens {
// One stop "ps.<id>" of lines for the FDatUnitPersonaEntry at `e`: the name (after `lead`), `bonus`, the EXP to the
// next level, the stats, the skills, the skills learned later, the affinities. `before` (a level up) marks what was
// gained. `name_item` at `name_id` replaces the name line (the camp's list row).
void build_status_page(ui::GraphBuilder& b, const uint8_t* e, const std::string& lead, const std::string& bonus,
                       const uint8_t* before, const ui::ControlId* name_id = nullptr, ui::NodeVtablePtr name_item = {});
// The id of a page's name line: where the focus goes when a page changes Persona.
ui::ControlId status_name_id(int persona);
// A skill was learned on the skill-learning screen since this was last asked (skill_learn.cpp); asking clears it.
bool take_skill_learned();
}  // namespace p3a::screens

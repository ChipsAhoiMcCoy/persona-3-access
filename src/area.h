#pragma once
#include <string>

namespace p3a::area {
// The area key (controller layer + B): a written description of where the player is, matched to the game's state
// (src/core/area_text: the text and its variants).
void describe();
void install();   // dev route /area
// Dev: why the field is not settled after a field change ("" once it is): the field manager's state machine is not
// back in its idle state, a field event is running, or the hero is not placed. A CALL_FIELD before then can crash
// the game (area.cpp, above field_busy). Game thread.
std::string field_busy();
}  // namespace p3a::area

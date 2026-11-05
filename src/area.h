#pragma once

namespace p3a::area {
// The area key (controller layer + B): a written description of where the player is, matched to the game's state
// (src/core/area_text: the text and its variants).
void describe();
void install();   // dev route /area
}  // namespace p3a::area

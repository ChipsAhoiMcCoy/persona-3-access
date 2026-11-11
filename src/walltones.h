// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
#include <string>

namespace p3a::walltones {
void install();
void tick();   // game thread, every mod tick
void set_enabled(bool on);
// The settings menu's sound test: one tone alone (0 ahead, 1 right, 2 behind, 3 left) for a little over a second,
// whatever the setting and wherever the player is.
void demo(int dir);
std::string status();
}  // namespace p3a::walltones

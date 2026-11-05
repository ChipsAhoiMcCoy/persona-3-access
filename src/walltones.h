// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
#include <string>

namespace p3a::walltones {
void install();
void tick();   // game thread, every mod tick
void set_enabled(bool on);
bool enabled();
std::string status();
}  // namespace p3a::walltones

// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#include "core/control_id.h"

#include <stdexcept>

namespace p3a::ui {

std::size_t Key::hash() const { return set_ ? std::hash<std::string>{}(text_) : 0; }

ControlId::ControlId(Key structural_key) : structural_key_(std::move(structural_key)) {
  if (structural_key_.empty()) throw std::invalid_argument("ControlId: structural key is required");
}

std::string ControlId::to_string() const { return "ControlId(" + structural_key_.to_string() + ")"; }

}  // namespace p3a::ui

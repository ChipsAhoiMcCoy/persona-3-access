#include "core/units.h"
#include "core/strings.h"
#include <cmath>

namespace p3a::core {
std::string meters(float m) {
  long n = std::lround(m);
  if (n < 1) return strings::kLessThanOneMeter;
  return std::to_string(n) + " " + (n == 1 ? strings::kMeterUnit : strings::kMetersUnit);
}
}  // namespace p3a::core

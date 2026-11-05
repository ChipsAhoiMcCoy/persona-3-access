#include "core/units.h"
#include <cmath>

namespace p3a::core {
std::string meters(float m) {
  long n = std::lround(m);
  if (n < 1) return "less than 1 meter";
  return std::to_string(n) + (n == 1 ? " meter" : " meters");
}
}  // namespace p3a::core

#include "core/reading_order.h"
#include <algorithm>

namespace p3a::core {
std::vector<Spot> reading_order(std::vector<Spot> ps, float row_height) {
  std::sort(ps.begin(), ps.end(), [](const Spot& a, const Spot& b) { return a.y < b.y; });
  for (size_t s = 0; s < ps.size();) {
    size_t e = s + 1;
    while (e < ps.size() && ps[e].y - ps[s].y < row_height) ++e;
    std::sort(ps.begin() + s, ps.begin() + e, [](const Spot& a, const Spot& b) { return a.x < b.x; });
    s = e;
  }
  return ps;
}
}  // namespace p3a::core

#include "core/doorway.h"
#include <algorithm>
#include <cmath>

namespace p3a::core {

bool clip(Vec2 a, Vec2 b, const Rect& r, Vec2& in, Vec2& out) {
  // Liang-Barsky: the range of t in [0, 1] for which a + t (b - a) is inside all four edges.
  float t0 = 0.0f, t1 = 1.0f;
  const float dx = b.x - a.x, dy = b.y - a.y;
  const float p[4] = {-dx, dx, -dy, dy};
  const float q[4] = {a.x - r.x0, r.x1 - a.x, a.y - r.y0, r.y1 - a.y};
  for (int i = 0; i < 4; ++i) {
    if (std::fabs(p[i]) < 1e-6f) {
      if (q[i] < 0.0f) return false;   // parallel to this edge and outside it
      continue;
    }
    float t = q[i] / p[i];
    if (p[i] < 0.0f) t0 = std::max(t0, t);
    else t1 = std::min(t1, t);
    if (t0 > t1) return false;
  }
  in = Vec2{a.x + dx * t0, a.y + dy * t0};
  out = Vec2{a.x + dx * t1, a.y + dy * t1};
  return true;
}

}  // namespace p3a::core

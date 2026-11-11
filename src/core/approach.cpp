#include "core/approach.h"
#include <algorithm>
#include <cmath>

namespace p3a::core {

std::vector<Opening> openings(const std::vector<bool>& clear) {
  std::vector<Opening> out;
  const int n = (int)clear.size();
  if (n == 0) return out;
  const float step = 6.28318531f / (float)n;
  int blocked = -1;
  for (int i = 0; i < n; ++i) if (!clear[i]) { blocked = i; break; }
  if (blocked < 0) { out.push_back(Opening{0.0f, 6.28318531f, 0}); return out; }
  // Start right after a blocked direction, so a run across the list's end is walked in one piece.
  for (int k = 1; k <= n;) {
    int i = (blocked + k) % n;
    if (!clear[i]) { ++k; continue; }
    int len = 0;
    while (k + len <= n && clear[(blocked + k + len) % n]) ++len;
    Opening o;
    o.width = step * (float)len;
    o.angle = std::fmod(step * ((float)i + (float)(len - 1) * 0.5f), 6.28318531f);
    o.index = (i + (len - 1) / 2) % n;
    out.push_back(o);
    k += len;
  }
  std::stable_sort(out.begin(), out.end(), [](const Opening& a, const Opening& b) { return a.width > b.width; });
  return out;
}

Vec2 follow(Vec2 a, Vec2 b, Vec2 p, float ahead) {
  float dx = b.x - a.x, dy = b.y - a.y;
  float len = std::sqrt(dx * dx + dy * dy);
  if (len < 1.0f) return b;
  dx /= len; dy /= len;
  float along = (p.x - a.x) * dx + (p.y - a.y) * dy;
  float s = std::clamp(along + ahead, 0.0f, len);
  return Vec2{a.x + dx * s, a.y + dy * s};
}

}  // namespace p3a::core

#include "core/boxes.h"
#include <cmath>

namespace p3a::core {

float flat(const Vec3& a, const Vec3& b) { return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); }

float dist3(const Vec3& a, const Vec3& b) {
  return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
}

float angle_diff(float a, float b) { return std::fabs(std::fmod(a - b + 540.0f, 360.0f) - 180.0f); }

bool box_holds(const Box& b, const Vec3& p, float grow_xy, float grow_z) {
  // The point in the box's own frame: turned back by its yaw.
  float a = -b.yaw * kPi / 180.0f, dx = p.x - b.center.x, dy = p.y - b.center.y;
  float lx = dx * std::cos(a) - dy * std::sin(a), ly = dx * std::sin(a) + dy * std::cos(a);
  return std::fabs(lx) <= b.extent.x + grow_xy && std::fabs(ly) <= b.extent.y + grow_xy &&
         std::fabs(p.z - b.center.z) <= b.extent.z + grow_z;
}

bool boxes_meet(const Box& a, const Box& b) {
  if (std::fabs(a.center.z - b.center.z) > a.extent.z + b.extent.z) return false;
  const float ya = a.yaw * kPi / 180.0f, yb = b.yaw * kPi / 180.0f;
  const float ax[4][2] = {{std::cos(ya), std::sin(ya)}, {-std::sin(ya), std::cos(ya)}, {std::cos(yb), std::sin(yb)}, {-std::sin(yb), std::cos(yb)}};
  const float dx = b.center.x - a.center.x, dy = b.center.y - a.center.y;
  for (const auto& n : ax) {
    // Each box's half width along this axis: its two sides' extents projected on it.
    float ra = a.extent.x * std::fabs(n[0] * ax[0][0] + n[1] * ax[0][1]) + a.extent.y * std::fabs(n[0] * ax[1][0] + n[1] * ax[1][1]);
    float rb = b.extent.x * std::fabs(n[0] * ax[2][0] + n[1] * ax[2][1]) + b.extent.y * std::fabs(n[0] * ax[3][0] + n[1] * ax[3][1]);
    if (std::fabs(dx * n[0] + dy * n[1]) > ra + rb) return false;
  }
  return true;
}

Vec3 box_nearest(const Box& b, const Vec3& p) {
  // Into the box's own frame (as box_holds), clamped to its sides, and turned back out.
  float a = -b.yaw * kPi / 180.0f, dx = p.x - b.center.x, dy = p.y - b.center.y;
  float lx = dx * std::cos(a) - dy * std::sin(a), ly = dx * std::sin(a) + dy * std::cos(a);
  lx = std::fmax(-b.extent.x, std::fmin(lx, b.extent.x));
  ly = std::fmax(-b.extent.y, std::fmin(ly, b.extent.y));
  float lz = std::fmax(-b.extent.z, std::fmin(p.z - b.center.z, b.extent.z));
  return Vec3{b.center.x + lx * std::cos(-a) - ly * std::sin(-a), b.center.y + lx * std::sin(-a) + ly * std::cos(-a), b.center.z + lz};
}

}  // namespace p3a::core

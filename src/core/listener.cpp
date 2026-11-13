#include "core/listener.h"
#include <cmath>

namespace p3a::core {

float bearing(const Vec3& from, const Vec3& to, float cam_yaw) {
  float a = std::atan2(to.y - from.y, to.x - from.x) * 180.0f / 3.14159265f - cam_yaw;
  while (a > 180) a -= 360;
  while (a < -180) a += 360;
  return a;
}

ListenerSpot listener_spot(const Vec3& from, const Vec3& to, float cam_yaw) {
  float dx = to.x - from.x, dy = to.y - from.y, dz = to.z - from.z;
  float flat = std::hypot(dx, dy);
  float b = bearing(from, to, cam_yaw) * 3.14159265f / 180.0f;
  float lift = std::fabs(dz) > 150.0f ? dz - std::copysign(150.0f, dz) : 0.0f;
  float e = std::atan2(lift, flat);
  ListenerSpot s;
  s.right = std::sin(b) * std::cos(e);
  s.ahead = std::cos(b) * std::cos(e);
  s.up = std::sin(e);
  s.dist_m = std::sqrt(flat * flat + dz * dz) / 100.0f;
  s.flat_m = flat / 100.0f;
  s.pan = std::sin(b);
  return s;
}

}  // namespace p3a::core

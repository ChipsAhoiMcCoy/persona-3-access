#pragma once
#include "core/boxes.h"

// Where a place is heard from: the listener stands at `from` and faces the camera's yaw (where pushing forward
// walks). Unreal's units: centimetres, yaw in degrees. Engine-free.
namespace p3a::core {

// Camera-relative bearing of `to` from `from` in degrees: 0 ahead, 90 right, 180 behind, -90 left.
float bearing(const Vec3& from, const Vec3& to, float cam_yaw);

// A unit direction in the listener's own frame, the distance in metres, and the stereo pan (the sine of the bearing).
struct ListenerSpot { float right = 0, up = 0, ahead = 1, dist_m = 0, pan = 0; };
// Height counts only past 1.5 m: the listener's place is his body's middle and most things sit near the floor, so a
// thing on the same floor would otherwise sound below him, the more so up close.
ListenerSpot listener_spot(const Vec3& from, const Vec3& to, float cam_yaw);

}  // namespace p3a::core

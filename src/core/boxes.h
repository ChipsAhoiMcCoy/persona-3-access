#pragma once

// Boxes on the field, turned about the vertical only (a hit actor's trigger, the strike's reach, a breakable's
// body), and the distances and angles the walk and the Shadow cues measure with. Unreal's units: centimetres, yaw in
// degrees. Engine-free.
namespace p3a::core {

constexpr float kPi = 3.14159265f;

struct Vec3 { float x = 0, y = 0, z = 0; };
// A box as it stands in the world: its centre, its half sizes along its own axes, and its yaw.
struct Box { Vec3 center, extent; float yaw = 0; };

float flat(const Vec3& a, const Vec3& b);    // the distance on the ground plane
float dist3(const Vec3& a, const Vec3& b);   // the distance in space
// How far apart two headings are, in degrees: 0 (the same way) to 180 (opposite).
float angle_diff(float a, float b);

// `p` is inside the box grown by `grow_xy` along both of its ground axes (negative: shrunk) and by `grow_z` up and
// down.
bool box_holds(const Box& b, const Vec3& p, float grow_xy, float grow_z);
// Two boxes share space (touching counts): the separating-axis test on the ground plane over both boxes' sides, and
// their heights overlapping.
bool boxes_meet(const Box& a, const Box& b);

}  // namespace p3a::core

#pragma once
#include "core/approach.h"
#include <functional>
#include <vector>

// A walk the navmesh does not have: classroom 2-F's desks cut its navmesh into islands (the navmesh keeps a wider
// berth than the hero needs), so the path from the back door to a front desk led out through one door and in at the
// other. The hero's own body fits between the desks. This searches a grid of floor points for a way, asking the
// caller whether the body moves from one point to the next (the engine's capsule sweep). Engine-free.
namespace p3a::core {

struct BodyPlan {
  bool found = false;
  std::vector<Vec2> points;   // the start, every turn, and the end (each leg a straight run the body makes)
  int expanded = 0;           // grid points taken off the open list
  int tested = 0;             // moves asked of `clear`
};

// A* over a square grid of `cell` cm with its axes turned `yaw` radians (the camera's, so every leg is one of the
// eight walking directions), from `from` toward `goal`. Eight neighbours; a diagonal costs its length.
// `clear(a, b)`: the body moves a -> b. `arrived(p)`: standing at p ends the walk. Gives up after `max_expand` points,
// or past `reach` cm from the start.
BodyPlan body_path(Vec2 from, Vec2 goal, float cell, float yaw, int max_expand, float reach,
                   const std::function<bool(Vec2, Vec2)>& clear, const std::function<bool(Vec2)>& arrived);

}  // namespace p3a::core

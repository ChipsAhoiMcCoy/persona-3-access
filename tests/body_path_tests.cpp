#include "doctest.h"
#include "core/body_path.h"
#include <cmath>

using namespace p3a::core;

namespace {
// A body of radius r moving a -> b meets the segment wall (wx, y0..y1) when its line passes within r of it.
bool crosses_wall(Vec2 a, Vec2 b, float wx, float y0, float y1, float r) {
  for (int k = 0; k <= 20; ++k) {
    float t = (float)k / 20.0f;
    float x = a.x + (b.x - a.x) * t, y = a.y + (b.y - a.y) * t;
    float cy = std::fmax(y0, std::fmin(y1, y));
    if (std::hypot(x - wx, y - cy) < r) return true;
  }
  return false;
}
}  // namespace

TEST_CASE("body_path: open floor is one straight leg") {
  auto plan = body_path(Vec2{0, 0}, Vec2{300, 0}, 25, 0, 1000, 2000, [](Vec2, Vec2) { return true; },
                        [](Vec2 p) { return std::hypot(p.x - 300, p.y) < 30; });
  REQUIRE(plan.found);
  REQUIRE(plan.points.size() == 2);
  CHECK(plan.points[0].x == doctest::Approx(0));
  CHECK(std::hypot(plan.points[1].x - 300, plan.points[1].y) < 30);   // the first grid point that arrives
}

TEST_CASE("body_path: through the one gap in a wall") {
  // A wall at x = 100 from y = -500 to 500 with a gap from 150 to 230; the body is 26 cm round.
  auto clear = [](Vec2 a, Vec2 b) { return !crosses_wall(a, b, 100, -500, 150, 26) && !crosses_wall(a, b, 100, 230, 500, 26); };
  auto plan = body_path(Vec2{0, 0}, Vec2{200, 0}, 25, 0, 5000, 2000, clear, [](Vec2 p) { return std::hypot(p.x - 200, p.y) < 30; });
  REQUIRE(plan.found);
  // Every leg is clear, and the walk goes by way of the gap.
  bool through = false;
  for (size_t i = 1; i < plan.points.size(); ++i) {
    CHECK(clear(plan.points[i - 1], plan.points[i]));
    if (plan.points[i].y > 150 && plan.points[i].y < 230) through = true;
  }
  CHECK(through);
}

TEST_CASE("body_path: a gap narrower than the body is no way") {
  auto clear = [](Vec2 a, Vec2 b) { return !crosses_wall(a, b, 100, -500, 150, 26) && !crosses_wall(a, b, 100, 180, 500, 26); };
  // Within 400 cm of the start: round the wall's ends is out of reach.
  auto plan = body_path(Vec2{0, 0}, Vec2{200, 0}, 25, 0, 3000, 400, clear, [](Vec2 p) { return std::hypot(p.x - 200, p.y) < 30; });
  CHECK_FALSE(plan.found);
  CHECK(plan.expanded > 0);
}

TEST_CASE("body_path: a diagonal over open floor is one leg, not a staircase") {
  auto plan = body_path(Vec2{0, 0}, Vec2{400, 130}, 10, 0, 5000, 2000, [](Vec2, Vec2) { return true; },
                        [](Vec2 p) { return std::hypot(p.x - 400, p.y - 130) < 8; });
  REQUIRE(plan.found);
  CHECK(plan.points.size() == 2);
}

TEST_CASE("body_path: the grid turns with the camera") {
  // Turned 90 degrees: the grid's first axis is world y, so a goal straight up the y axis is one leg.
  auto plan = body_path(Vec2{10, 10}, Vec2{10, 210}, 25, 1.5707963f, 1000, 2000, [](Vec2, Vec2) { return true; },
                        [](Vec2 p) { return std::hypot(p.x - 10, p.y - 210) < 5; });
  REQUIRE(plan.found);
  REQUIRE(plan.points.size() == 2);
  CHECK(plan.points[1].x == doctest::Approx(10).epsilon(0.01));
  CHECK(plan.points[1].y == doctest::Approx(210).epsilon(0.01));
}

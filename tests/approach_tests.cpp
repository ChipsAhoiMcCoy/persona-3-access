#include "doctest.h"
#include "core/approach.h"
#include <cmath>

using namespace p3a::core;

TEST_CASE("openings: none when every side is blocked") {
  CHECK(openings(std::vector<bool>(72, false)).empty());
  CHECK(openings({}).empty());
}

TEST_CASE("openings: open all round is one full turn") {
  auto o = openings(std::vector<bool>(8, true));
  REQUIRE(o.size() == 1);
  CHECK(o[0].width == doctest::Approx(6.28318531f));
}

TEST_CASE("openings: a run's middle, widest first") {
  // 72 directions, 5 degrees apart: a narrow gap at 90 degrees, a wide one round 270.
  std::vector<bool> c(72, false);
  c[18] = true;
  for (int i = 52; i <= 56; ++i) c[i] = true;
  auto o = openings(c);
  REQUIRE(o.size() == 2);
  CHECK(o[0].index == 54);
  CHECK(o[0].angle == doctest::Approx(270.0f * 3.14159265f / 180.0f));
  CHECK(o[0].width == doctest::Approx(25.0f * 3.14159265f / 180.0f));
  CHECK(o[1].index == 18);
  CHECK(o[1].angle == doctest::Approx(90.0f * 3.14159265f / 180.0f));
}

TEST_CASE("openings: a run across the end of the list is one opening") {
  std::vector<bool> c(72, false);
  c[70] = c[71] = c[0] = c[1] = c[2] = true;
  auto o = openings(c);
  REQUIRE(o.size() == 1);
  CHECK(o[0].index == 0);
  CHECK(std::cos(o[0].angle) == doctest::Approx(1.0f));
  CHECK(std::sin(o[0].angle) == doctest::Approx(0.0f).epsilon(0.001));
}

TEST_CASE("openings: an even run takes the direction below its middle") {
  std::vector<bool> c(8, false);
  c[2] = c[3] = true;
  auto o = openings(c);
  REQUIRE(o.size() == 1);
  CHECK(o[0].index == 2);
  CHECK(o[0].angle == doctest::Approx(2.5f * 6.28318531f / 8.0f));
}

TEST_CASE("follow: steers for a point just ahead on the line") {
  Vec2 a{0, 0}, b{300, 0};
  Vec2 w = follow(a, b, Vec2{100, 20}, 40);   // 20 cm off the line
  CHECK(w.x == doctest::Approx(140));
  CHECK(w.y == doctest::Approx(0));
}

TEST_CASE("follow: never past the end, never before the start") {
  Vec2 a{0, 0}, b{0, 300};
  Vec2 w = follow(a, b, Vec2{5, 290}, 40);
  CHECK(w.x == doctest::Approx(0));
  CHECK(w.y == doctest::Approx(300));
  w = follow(a, b, Vec2{30, -200}, 40);
  CHECK(w.y == doctest::Approx(0));
  w = follow(a, a, Vec2{9, 9}, 40);
  CHECK(w.x == doctest::Approx(0));
}

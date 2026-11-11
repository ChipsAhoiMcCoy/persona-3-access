#include "doctest.h"
#include "core/doorway.h"

using namespace p3a::core;

// A door across a hallway: 5 m wide, 20 cm thick (the gym hallway's, 2026-11-11).
static const Rect kDoor{-250, -2662, 250, -2642};

TEST_CASE("clip: a path leg through the door gives the piece inside it") {
  Vec2 in, out;
  REQUIRE(clip(Vec2{-51, -2691}, Vec2{-190, -1310}, kDoor, in, out));
  CHECK(in.y == doctest::Approx(-2662));
  CHECK(out.y == doctest::Approx(-2642));
  CHECK(in.x < -51);
  CHECK(out.x < in.x);
}

TEST_CASE("clip: a leg that stays on one side misses it") {
  Vec2 in, out;
  CHECK_FALSE(clip(Vec2{-51, -2691}, Vec2{200, -3400}, kDoor, in, out));
  CHECK_FALSE(clip(Vec2{-51, -2691}, Vec2{-51, -2670}, kDoor, in, out));   // stops short of it
  CHECK_FALSE(clip(Vec2{300, -2700}, Vec2{300, -2600}, kDoor, in, out));   // passes beside it
}

TEST_CASE("clip: a leg along the door, outside it, misses it") {
  Vec2 in, out;
  CHECK_FALSE(clip(Vec2{-400, -2700}, Vec2{400, -2700}, kDoor, in, out));
}

TEST_CASE("clip: an end inside the door is that end") {
  Vec2 in, out;
  REQUIRE(clip(Vec2{0, -2650}, Vec2{0, -2000}, kDoor, in, out));
  CHECK(in.y == doctest::Approx(-2650));
  CHECK(out.y == doctest::Approx(-2642));
  REQUIRE(clip(Vec2{0, -2650}, Vec2{0, -2650}, kDoor, in, out));   // standing in it
  CHECK(in.y == doctest::Approx(-2650));
}

TEST_CASE("clip: the order follows the leg's direction") {
  Vec2 in, out;
  REQUIRE(clip(Vec2{0, -2000}, Vec2{0, -3000}, kDoor, in, out));
  CHECK(in.y == doctest::Approx(-2642));
  CHECK(out.y == doctest::Approx(-2662));
}

#include "doctest.h"
#include "core/listener.h"
#include <cmath>

using namespace p3a::core;

TEST_CASE("bearing: ahead, right, behind and left of the camera") {
  Vec3 me{0, 0, 0};
  CHECK(bearing(me, Vec3{100, 0, 0}, 0) == doctest::Approx(0.0f));
  CHECK(bearing(me, Vec3{0, 100, 0}, 0) == doctest::Approx(90.0f));
  CHECK(bearing(me, Vec3{0, -100, 0}, 0) == doctest::Approx(-90.0f));
  CHECK(std::abs(bearing(me, Vec3{-100, 0, 0}, 0)) == doctest::Approx(180.0f));
}

TEST_CASE("bearing: turns with the camera and wraps into -180..180") {
  Vec3 me{10, 20, 0};
  CHECK(bearing(me, Vec3{10, 120, 0}, 90) == doctest::Approx(0.0f));     // the camera looks along y
  CHECK(bearing(me, Vec3{110, 20, 0}, 90) == doctest::Approx(-90.0f));   // x is then to the left
  // The camera at 170 degrees, the thing at -170: 20 degrees to the right, not 340 to the left.
  Vec3 at{10 - 98.48f, 20 - 17.36f, 0};
  CHECK(bearing(me, at, 170) == doctest::Approx(20.0f).epsilon(0.01));
  CHECK(std::abs(bearing(me, at, -170)) < 0.1f);   // and dead ahead of a camera that looks that way
}

TEST_CASE("listener_spot: a direction on the ground, the distance and the pan") {
  Vec3 me{0, 0, 0};
  ListenerSpot s = listener_spot(me, Vec3{200, 0, 0}, 0);
  CHECK(s.ahead == doctest::Approx(1.0f));
  CHECK(s.right == doctest::Approx(0.0f));
  CHECK(s.up == doctest::Approx(0.0f));
  CHECK(s.pan == doctest::Approx(0.0f));
  CHECK(s.dist_m == doctest::Approx(2.0f));
  s = listener_spot(me, Vec3{0, 300, 0}, 0);
  CHECK(s.right == doctest::Approx(1.0f));
  CHECK(s.pan == doctest::Approx(1.0f));
  CHECK(s.ahead == doctest::Approx(0.0f));
  s = listener_spot(me, Vec3{0, 300, 0}, 180);   // the camera turned round: it is on the left now
  CHECK(s.right == doctest::Approx(-1.0f));
  CHECK(s.pan == doctest::Approx(-1.0f));
  s = listener_spot(me, Vec3{-100, 0, 0}, 0);
  CHECK(s.ahead == doctest::Approx(-1.0f));
}

TEST_CASE("listener_spot: height counts only past a metre and a half") {
  Vec3 me{0, 0, 0};
  ListenerSpot s = listener_spot(me, Vec3{100, 0, 100}, 0);   // a little above: level
  CHECK(s.up == doctest::Approx(0.0f));
  CHECK(s.ahead == doctest::Approx(1.0f));
  CHECK(s.dist_m == doctest::Approx(1.41421f));   // the distance is the real one all the same
  s = listener_spot(me, Vec3{100, 0, 250}, 0);    // 2.5 m up: counted as 1 m up at 1 m ahead
  CHECK(s.up == doctest::Approx(0.70711f));
  CHECK(s.ahead == doctest::Approx(0.70711f));
  CHECK(s.dist_m == doctest::Approx(2.69258f));
  s = listener_spot(me, Vec3{100, 0, -250}, 0);   // and below
  CHECK(s.up == doctest::Approx(-0.70711f));
  s = listener_spot(me, Vec3{0, 0, 400}, 0);      // straight overhead
  CHECK(s.up == doctest::Approx(1.0f));
}

TEST_CASE("listener_spot: the direction is a unit vector") {
  ListenerSpot s = listener_spot(Vec3{5, -3, 90}, Vec3{-240, 310, 520}, 37);
  CHECK(s.right * s.right + s.up * s.up + s.ahead * s.ahead == doctest::Approx(1.0f));
}

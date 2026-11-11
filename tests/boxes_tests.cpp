#include "doctest.h"
#include "core/boxes.h"

using namespace p3a::core;

TEST_CASE("boxes: distances on the ground and in space") {
  CHECK(flat(Vec3{0, 0, 0}, Vec3{3, 4, 100}) == doctest::Approx(5.0f));   // height does not count
  CHECK(flat(Vec3{-1, -1, 0}, Vec3{-1, -1, 50}) == doctest::Approx(0.0f));
  CHECK(dist3(Vec3{0, 0, 0}, Vec3{2, 3, 6}) == doctest::Approx(7.0f));
  CHECK(dist3(Vec3{1, 2, 3}, Vec3{1, 2, 3}) == doctest::Approx(0.0f));
}

TEST_CASE("boxes: the angle between two headings") {
  CHECK(angle_diff(90.0f, 90.0f) == doctest::Approx(0.0f));
  CHECK(angle_diff(10.0f, 350.0f) == doctest::Approx(20.0f));     // across north
  CHECK(angle_diff(-170.0f, 170.0f) == doctest::Approx(20.0f));   // across south, Unreal's -180..180 yaws
  CHECK(angle_diff(0.0f, 180.0f) == doctest::Approx(180.0f));
  CHECK(angle_diff(30.0f, -60.0f) == doctest::Approx(90.0f));
}

TEST_CASE("box_holds: a point in an unturned box, its edge included") {
  Box b{Vec3{0, 0, 0}, Vec3{100, 50, 20}, 0};
  CHECK(box_holds(b, Vec3{99, 0, 0}, 0, 0));
  CHECK(box_holds(b, Vec3{100, 0, 0}, 0, 0));
  CHECK_FALSE(box_holds(b, Vec3{101, 0, 0}, 0, 0));
  CHECK(box_holds(b, Vec3{0, -49, 0}, 0, 0));
  CHECK_FALSE(box_holds(b, Vec3{0, -51, 0}, 0, 0));
}

TEST_CASE("box_holds: grown and shrunk on the ground, grown in height") {
  Box b{Vec3{10, 20, 30}, Vec3{100, 50, 20}, 0};
  CHECK(box_holds(b, Vec3{114, 20, 30}, 5, 0));        // 104 from the middle, grown to 105
  CHECK_FALSE(box_holds(b, Vec3{116, 20, 30}, 5, 0));
  CHECK(box_holds(b, Vec3{104, 20, 30}, -5, 0));       // 94 from the middle, shrunk to 95
  CHECK_FALSE(box_holds(b, Vec3{107, 20, 30}, -5, 0));
  CHECK_FALSE(box_holds(b, Vec3{10, 20, 55}, 0, 0));   // 25 above the middle, half height 20
  CHECK(box_holds(b, Vec3{10, 20, 55}, 0, 10));
  CHECK_FALSE(box_holds(b, Vec3{10, 20, 0}, 0, 5));    // and below
}

TEST_CASE("box_holds: a turned box") {
  // A long thin box turned a quarter: its length runs along y.
  Box quarter{Vec3{0, 0, 0}, Vec3{100, 10, 50}, 90};
  CHECK(box_holds(quarter, Vec3{0, 90, 0}, 0, 0));
  CHECK(box_holds(quarter, Vec3{0, -90, 0}, 0, 0));
  CHECK_FALSE(box_holds(quarter, Vec3{90, 0, 0}, 0, 0));
  // Turned an eighth: its length runs along the diagonal.
  Box eighth{Vec3{0, 0, 0}, Vec3{100, 10, 50}, 45};
  CHECK(box_holds(eighth, Vec3{60, 60, 0}, 0, 0));
  CHECK_FALSE(box_holds(eighth, Vec3{60, -60, 0}, 0, 0));
  CHECK(box_holds(eighth, Vec3{60, -60, 0}, 80, 0));   // 85 off its line, grown to 90
}

TEST_CASE("boxes_meet: unturned boxes apart, overlapping and touching") {
  Box a{Vec3{0, 0, 0}, Vec3{10, 10, 10}, 0};
  CHECK_FALSE(boxes_meet(a, Box{Vec3{25, 0, 0}, Vec3{10, 10, 10}, 0}));
  CHECK(boxes_meet(a, Box{Vec3{19, 0, 0}, Vec3{10, 10, 10}, 0}));
  CHECK(boxes_meet(a, Box{Vec3{20, 0, 0}, Vec3{10, 10, 10}, 0}));     // edges touching
  CHECK(boxes_meet(a, Box{Vec3{20, 20, 0}, Vec3{10, 10, 10}, 0}));    // corners touching
  CHECK_FALSE(boxes_meet(a, Box{Vec3{0, 21, 0}, Vec3{10, 10, 10}, 0}));
  CHECK(boxes_meet(a, a));
}

TEST_CASE("boxes_meet: heights must overlap") {
  Box a{Vec3{0, 0, 0}, Vec3{10, 10, 10}, 0};
  CHECK_FALSE(boxes_meet(a, Box{Vec3{0, 0, 25}, Vec3{10, 10, 10}, 0}));
  CHECK(boxes_meet(a, Box{Vec3{0, 0, 20}, Vec3{10, 10, 10}, 0}));   // one standing on the other
  CHECK(boxes_meet(a, Box{Vec3{0, 0, -15}, Vec3{10, 10, 10}, 0}));
}

TEST_CASE("boxes_meet: turned boxes") {
  Box a{Vec3{0, 0, 0}, Vec3{10, 10, 10}, 0};
  // Side by side these are 3 apart; turned an eighth, the second one's corner reaches into the first.
  CHECK_FALSE(boxes_meet(a, Box{Vec3{23, 0, 0}, Vec3{10, 10, 10}, 0}));
  CHECK(boxes_meet(a, Box{Vec3{23, 0, 0}, Vec3{10, 10, 10}, 45}));
  CHECK(boxes_meet(Box{Vec3{23, 0, 0}, Vec3{10, 10, 10}, 45}, a));
  // Off the first one's corner: the turned box's outline spans both of the first one's sides, yet they are apart.
  CHECK_FALSE(boxes_meet(a, Box{Vec3{20, 20, 0}, Vec3{10, 10, 10}, 45}));
  CHECK_FALSE(boxes_meet(Box{Vec3{20, 20, 0}, Vec3{10, 10, 10}, 45}, a));
  // A full turn changes nothing.
  CHECK(boxes_meet(a, Box{Vec3{19, 0, 0}, Vec3{10, 10, 10}, 360}));
  CHECK_FALSE(boxes_meet(a, Box{Vec3{25, 0, 0}, Vec3{10, 10, 10}, 360}));
}

TEST_CASE("boxes_meet: the strike's reach against a breakable") {
  // The hero at the origin facing yaw 180 (down the x axis): the strike's box is centred 84 cm ahead of him, 90 deep,
  // 60 to each side, 50 up and down. A breakable's own box is 40 x 40 x 75.
  Box strike{Vec3{-84, 0, 0}, Vec3{90, 60, 50}, 180};
  CHECK(boxes_meet(strike, Box{Vec3{-130, 0, 0}, Vec3{40, 40, 75}, 0}));        // 1.3 m ahead
  CHECK_FALSE(boxes_meet(strike, Box{Vec3{-300, 0, 0}, Vec3{40, 40, 75}, 0}));  // 3 m ahead
  CHECK(boxes_meet(strike, Box{Vec3{-130, 0, 0}, Vec3{40, 40, 75}, 30}));       // however the breakable is turned
  CHECK_FALSE(boxes_meet(strike, Box{Vec3{-300, 0, 0}, Vec3{40, 40, 75}, 30}));
  CHECK_FALSE(boxes_meet(strike, Box{Vec3{130, 0, 0}, Vec3{40, 40, 75}, 0}));   // 1.3 m behind him
  CHECK(boxes_meet(strike, Box{Vec3{-84, 95, 0}, Vec3{40, 40, 75}, 0}));        // beside the reach, overlapping its side
  CHECK_FALSE(boxes_meet(strike, Box{Vec3{-84, 105, 0}, Vec3{40, 40, 75}, 0}));
}

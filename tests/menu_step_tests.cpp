#include "doctest.h"
#include "core/menu_step.h"

using p3a::core::grid_step;
using p3a::core::list_step;
using p3a::core::MenuStep;
using p3a::core::overshot;

TEST_CASE("menu_step: a vertical list steps down to a higher index, up to a lower one") {
  CHECK(list_step(2, 5, false, false, 0) == MenuStep{false, 1});
  CHECK(list_step(5, 2, false, false, 0) == MenuStep{false, -1});
}

TEST_CASE("menu_step: a horizontal list steps right and left") {
  CHECK(list_step(0, 1, true, false, 0) == MenuStep{true, 1});
  CHECK(list_step(3, 1, true, false, 0) == MenuStep{true, -1});
}

TEST_CASE("menu_step: a reversed list steps the other way") {
  CHECK(list_step(2, 5, false, true, 0) == MenuStep{false, -1});
  CHECK(list_step(5, 2, true, true, 0) == MenuStep{true, 1});
}

TEST_CASE("menu_step: a wrapping list takes the short way round") {
  CHECK(list_step(0, 7, false, false, 8) == MenuStep{false, -1});   // one back, not seven on
  CHECK(list_step(7, 0, false, false, 8) == MenuStep{false, 1});    // one on, over the end
  CHECK(list_step(1, 3, false, false, 8) == MenuStep{false, 1});
  CHECK(list_step(3, 1, false, false, 8) == MenuStep{false, -1});
}

TEST_CASE("menu_step: halfway round a wrapping list goes forward") {
  CHECK(list_step(0, 4, false, false, 8) == MenuStep{false, 1});
  CHECK(list_step(4, 0, false, false, 8) == MenuStep{false, 1});
}

TEST_CASE("menu_step: a reversed wrapping list turns the short way round") {
  CHECK(list_step(0, 7, true, true, 8) == MenuStep{true, 1});
  CHECK(list_step(7, 0, true, true, 8) == MenuStep{true, -1});
}

TEST_CASE("menu_step: a grid goes to the row first") {
  CHECK(grid_step({0, 0}, {2, 3}, false) == MenuStep{false, 1});
  CHECK(grid_step({2, 3}, {0, 0}, false) == MenuStep{false, -1});
}

TEST_CASE("menu_step: a grid goes along the row once it is on it") {
  CHECK(grid_step({2, 0}, {2, 3}, false) == MenuStep{true, 1});
  CHECK(grid_step({2, 3}, {2, 0}, false) == MenuStep{true, -1});
}

TEST_CASE("menu_step: a reversed grid steps the other way on either axis") {
  CHECK(grid_step({0, 0}, {2, 3}, true) == MenuStep{false, -1});
  CHECK(grid_step({2, 0}, {2, 3}, true) == MenuStep{true, -1});
}

TEST_CASE("menu_step: a step that lands short of the target, or on the way, did not overshoot") {
  CHECK_FALSE(overshot(3, 5, 2));   // 2 -> 3, wanted 5
  CHECK_FALSE(overshot(4, 2, 5));   // 5 -> 4, wanted 2
}

TEST_CASE("menu_step: a step that carries the cursor past the target overshot") {
  CHECK(overshot(6, 5, 4));   // 4 -> 6: the game skipped 5
  CHECK(overshot(1, 2, 3));   // 3 -> 1: the game skipped 2
}

TEST_CASE("menu_step: a cursor that went the wrong way has the target still ahead of it") {
  CHECK_FALSE(overshot(1, 5, 2));   // 2 -> 1, wanted 5: still below it
}

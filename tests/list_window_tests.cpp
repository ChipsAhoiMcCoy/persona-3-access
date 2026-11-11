#include "doctest.h"
#include "core/list_window.h"

using p3a::core::scroll_top;

TEST_CASE("list_window: the window stays while the cursor is inside it") {
  CHECK(scroll_top(3, 2, 5, 20) == 2);
  CHECK(scroll_top(2, 2, 5, 20) == 2);
  CHECK(scroll_top(6, 2, 5, 20) == 2);
}

TEST_CASE("list_window: it moves only as far as it must") {
  CHECK(scroll_top(1, 2, 5, 20) == 1);    // above: the cursor becomes the top row
  CHECK(scroll_top(7, 2, 5, 20) == 3);    // below: the cursor becomes the bottom row
  CHECK(scroll_top(19, 0, 5, 20) == 15);  // a jump to the end
  CHECK(scroll_top(0, 15, 5, 20) == 0);   // and back to the start
}

TEST_CASE("list_window: it never runs past the ends") {
  CHECK(scroll_top(18, 17, 5, 20) == 15);   // a top left too low by a shorter list
  CHECK(scroll_top(0, -3, 5, 20) == 0);
}

TEST_CASE("list_window: a list that fits is not scrolled") {
  CHECK(scroll_top(2, 0, 5, 3) == 0);
  CHECK(scroll_top(2, 1, 0, 3) == 0);     // no row count: every entry shows
  CHECK(scroll_top(0, 0, 5, 0) == 0);     // an empty list
}

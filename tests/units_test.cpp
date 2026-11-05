#include "doctest.h"
#include "core/units.h"

TEST_CASE("meters") {
  CHECK(p3a::core::meters(0.3f) == "less than 1 meter");
  CHECK(p3a::core::meters(1.2f) == "1 meter");
  CHECK(p3a::core::meters(6.0f) == "6 meters");
}

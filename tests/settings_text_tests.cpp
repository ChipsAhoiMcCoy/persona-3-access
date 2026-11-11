#include "doctest.h"
#include <string>
#include <vector>
#include "core/settings_text.h"

using namespace p3a::core;

TEST_CASE("settings_text: a line of the file is a key and a number") {
  std::string key; int v = 0;
  CHECK(parse_setting("poi_range=40\n", key, v));
  CHECK(key == "poi_range");
  CHECK(v == 40);
  CHECK(parse_setting("  sonar = 1", key, v));
  CHECK(key == "sonar");
  CHECK(v == 1);
  CHECK(parse_setting("offset=-3", key, v));
  CHECK(v == -3);
  CHECK(parse_setting("wall_volume=80 percent", key, v));   // what follows the number is ignored
  CHECK(v == 80);
}

TEST_CASE("settings_text: a line that is not a setting is refused and changes nothing") {
  std::string key = "kept"; int v = 7;
  CHECK_FALSE(parse_setting("", key, v));
  CHECK_FALSE(parse_setting("\n", key, v));
  CHECK_FALSE(parse_setting("no value here", key, v));
  CHECK_FALSE(parse_setting("sonar=on", key, v));
  CHECK_FALSE(parse_setting("=5", key, v));
  CHECK_FALSE(parse_setting("two words=3", key, v));
  CHECK_FALSE(parse_setting("sonar==3", key, v));
  CHECK(key == "kept");
  CHECK(v == 7);
}

TEST_CASE("settings_text: where a value is among those an option offers") {
  const std::vector<int> delays = {0, 100, 150, 250};
  CHECK(value_index(delays, 0) == 0);
  CHECK(value_index(delays, 150) == 2);
  CHECK(value_index(delays, 250) == 3);
  CHECK(value_index(delays, 123) == 0);   // not offered: counts as the first
  CHECK(value_index({}, 5) == 0);
}

TEST_CASE("settings_text: Space steps forward and wraps") {
  CHECK(step_index(0, 4, 0) == 1);
  CHECK(step_index(2, 4, 0) == 3);
  CHECK(step_index(3, 4, 0) == 0);
  CHECK(step_index(0, 2, 0) == 1);
  CHECK(step_index(1, 2, 0) == 0);
  CHECK(step_index(0, 1, 0) == 0);   // one value: nothing to step to
}

TEST_CASE("settings_text: Left and Right stop at the ends") {
  CHECK(step_index(1, 4, 1) == 2);
  CHECK(step_index(3, 4, 1) == 3);
  CHECK(step_index(1, 4, -1) == 0);
  CHECK(step_index(0, 4, -1) == 0);
}

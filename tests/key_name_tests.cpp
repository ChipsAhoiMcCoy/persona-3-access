#include "doctest.h"
#include "core/key_name.h"
#include "core/log_index.h"

TEST_CASE("config_key_name: the game's defaults") {
  using p3a::core::config_key_name;
  CHECK(config_key_name(432) == "W");
  CHECK(config_key_name(442) == "S");
  CHECK(config_key_name(441) == "A");
  CHECK(config_key_name(443) == "D");
  CHECK(config_key_name(452) == "C");
  CHECK(config_key_name(431) == "Q");
  CHECK(config_key_name(433) == "E");
  CHECK(config_key_name(454) == "B");
  CHECK(config_key_name(469) == "Space");
}
TEST_CASE("config_key_name: the ends of each keyboard row and codes with no name") {
  using p3a::core::config_key_name;
  CHECK(config_key_name(440) == "P");
  CHECK(config_key_name(449) == "L");
  CHECK(config_key_name(450) == "Z");
  CHECK(config_key_name(456) == "M");
  CHECK(config_key_name(430) == "key code 430");
  CHECK(config_key_name(457) == "key code 457");
  CHECK(config_key_name(0) == "key code 0");
  CHECK(config_key_name(-1) == "key code -1");
}
TEST_CASE("log_record_index counts a log row from the newest record") {
  using p3a::core::log_record_index;
  // Six entries in the game's log, six records: row for row.
  CHECK(log_record_index(5, 6, 0, 6) == 5);
  CHECK(log_record_index(0, 6, 0, 6) == 0);
  // The game's log is longer than this session's records: its oldest rows have none.
  CHECK(log_record_index(5, 6, 0, 2) == 1);
  CHECK(log_record_index(4, 6, 0, 2) == 0);
  CHECK(log_record_index(3, 6, 0, 2) == -1);
  // The page on screen is recorded but not yet in the game's log: one record is skipped.
  CHECK(log_record_index(5, 6, 1, 7) == 5);
  CHECK(log_record_index(0, 6, 1, 7) == 0);
  CHECK(log_record_index(5, 6, 1, 1) == -1);
  // One entry, no records, and a row past the end.
  CHECK(log_record_index(0, 1, 0, 1) == 0);
  CHECK(log_record_index(0, 1, 0, 0) == -1);
  CHECK(log_record_index(6, 6, 0, 6) == -1);
}

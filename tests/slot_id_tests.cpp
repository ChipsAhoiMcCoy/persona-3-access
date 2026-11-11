#include "doctest.h"
#include "core/slot_id.h"

using p3a::core::parse_slot;
using p3a::core::slot_id;
using p3a::core::slot_row;

TEST_CASE("slot_id: a row's id is its list and index") {
  CHECK(slot_id("items", 0) == "items#0");
  CHECK(slot_id("equip.gear.2.0", 17) == "equip.gear.2.0#17");
}

TEST_CASE("slot_id: a row's id parses back") {
  std::string key;
  int i = -1;
  CHECK(parse_slot("items#3", key, i));
  CHECK(key == "items");
  CHECK(i == 3);
  CHECK(parse_slot(slot_id("equip.gear.2.0", 17), key, i));
  CHECK(key == "equip.gear.2.0");
  CHECK(i == 17);
}

TEST_CASE("slot_id: the last # splits the id") {
  std::string key;
  int i = -1;
  CHECK(parse_slot("a#b#12", key, i));
  CHECK(key == "a#b");
  CHECK(i == 12);
}

TEST_CASE("slot_id: an id that is no row is refused and leaves its outputs alone") {
  std::string key = "kept";
  int i = 7;
  CHECK_FALSE(parse_slot("items.none", key, i));
  CHECK_FALSE(parse_slot("items#", key, i));
  CHECK_FALSE(parse_slot("", key, i));
  CHECK(key == "kept");
  CHECK(i == 7);
}

TEST_CASE("slot_id: a walk line is no row, but names the row it belongs to") {
  std::string key;
  int i = -1;
  CHECK_FALSE(parse_slot("stats#3.1", key, i));
  CHECK(key == "stats");
  CHECK(i == 3);
}

TEST_CASE("slot_id: something that is no number after the # reads as row 0 of that list") {
  std::string key;
  int i = -1;
  CHECK_FALSE(parse_slot("stats#x", key, i));
  CHECK(key == "stats");
  CHECK(i == 0);
}

TEST_CASE("slot_id: the row an id is on") {
  CHECK(slot_row("stats#4", "stats") == 4);
  CHECK(slot_row("stats#4.2", "stats") == 4);     // a line of row 4's walk
  CHECK(slot_row("stats#12.0", "stats") == 12);
}

TEST_CASE("slot_id: an id of another list is on no row") {
  CHECK(slot_row("stats.tactics#4", "stats") == -1);   // a longer key that starts the same
  CHECK(slot_row("stat#4", "stats") == -1);
  CHECK(slot_row("stats", "stats") == -1);
  CHECK(slot_row("stats#", "stats") == -1);
  CHECK(slot_row("stats#x", "stats") == -1);
  CHECK(slot_row("stats#4x", "stats") == -1);
  CHECK(slot_row("", "stats") == -1);
}

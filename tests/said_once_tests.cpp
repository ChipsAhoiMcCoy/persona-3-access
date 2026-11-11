#include "doctest.h"
#include "core/said_once.h"

using p3a::core::SaidOnce;

TEST_CASE("said_once: a line is said once while nothing else happens") {
  SaidOnce s;
  CHECK(s.admit("Items", 0, 0, 0));
  s.spoke(1);
  CHECK_FALSE(s.admit("Items", 0, 0, 1));
  CHECK_FALSE(s.admit("Items", 0, 0, 1));
}

TEST_CASE("said_once: two lines that alternate are each said once") {
  SaidOnce s;
  CHECK(s.admit("Sword", 0, 0, 0));
  s.spoke(1);
  CHECK(s.admit("Shield", 0, 0, 1));
  s.spoke(2);
  CHECK_FALSE(s.admit("Sword", 0, 0, 2));
  CHECK_FALSE(s.admit("Shield", 0, 0, 2));
}

TEST_CASE("said_once: the player's move lets a line be said again") {
  SaidOnce s;
  CHECK(s.admit("Items", 0, 0, 0));
  s.spoke(1);
  CHECK(s.admit("Items", 1, 0, 1));
  s.spoke(2);
  CHECK_FALSE(s.admit("Items", 1, 0, 2));
}

TEST_CASE("said_once: a controller press lets a line be said again") {
  SaidOnce s;
  CHECK(s.admit("Items", 0, 100, 0));
  s.spoke(1);
  CHECK_FALSE(s.admit("Items", 0, 100, 1));
  CHECK(s.admit("Items", 0, 250, 1));
}

TEST_CASE("said_once: someone else speaking lets a line be said again") {
  SaidOnce s;
  CHECK(s.admit("Items", 0, 0, 0));
  s.spoke(1);
  CHECK(s.admit("Items", 0, 0, 2));   // a line that was not this speaker's raised the count
}

TEST_CASE("said_once: a refused line changes nothing") {
  SaidOnce s;
  CHECK(s.admit("Items", 0, 0, 0));
  s.spoke(1);
  CHECK_FALSE(s.admit("Items", 0, 0, 1));
  CHECK(s.admit("Skills", 0, 0, 1));
  s.spoke(2);
  CHECK_FALSE(s.admit("Items", 0, 0, 2));
}

#include "doctest.h"
#include "core/poi_text.h"
#include "core/strings.h"
#include <string>
#include <vector>

using namespace p3a;
using namespace p3a::core;

TEST_CASE("clock_hour: a bearing as a clock face") {
  CHECK(clock_hour(0.0f) == 12);
  CHECK(clock_hour(14.0f) == 12);     // still nearer 12 than 1
  CHECK(clock_hour(16.0f) == 1);
  CHECK(clock_hour(60.0f) == 2);
  CHECK(clock_hour(90.0f) == 3);
  CHECK(clock_hour(180.0f) == 6);
  CHECK(clock_hour(-180.0f) == 6);
  CHECK(clock_hour(-90.0f) == 9);
  CHECK(clock_hour(-30.0f) == 11);
  CHECK(clock_hour(-10.0f) == 12);
}

TEST_CASE("name_parts: split at underscores, empty parts kept") {
  auto p = name_parts("FldNpcActor_105_102_Yukari_6");
  REQUIRE(p.size() == 5);
  CHECK(p[0] == "FldNpcActor");
  CHECK(p[3] == "Yukari");
  CHECK(p[4] == "6");
  CHECK(name_parts("Plain").size() == 1);
  auto q = name_parts("A__B_");
  REQUIRE(q.size() == 4);
  CHECK(q[1].empty());
  CHECK(q[3].empty());
  CHECK(name_parts("").size() == 1);
}

TEST_CASE("person_from_actor_name: the cast by their romanised names, others by their own word") {
  CHECK(person_from_actor_name("FldNpcActor_105_102_Yukari_6") == "Yukari");
  CHECK(person_from_actor_name("FldNpcActor_102_101_Mituru_2") == "Mitsuru");
  CHECK(person_from_actor_name("FldNpcActor_102_101_Sanada") == "Akihiko");
  CHECK(person_from_actor_name("FldNpcActor_102_101_Aegis_1") == "Aigis");
  CHECK(person_from_actor_name("FldNpcActor_102_101_Amada_1") == "Ken");
  CHECK(person_from_actor_name("FldNpcActor_Jin") == "Jin");                 // no field ids at all
  CHECK(person_from_actor_name("FldNpcActor_101_103_Student_12") == "Student");
  CHECK(person_from_actor_name("FldNpcActor_101_103_12").empty());           // numbers only
  CHECK(person_from_actor_name("FldNpcActor").empty());
}

TEST_CASE("story_scene: a trigger named for its scene") {
  CHECK(story_scene("FldHitActor_299_Takaya_H0") == "Takaya");
  CHECK(story_scene("FldHitActor_299_Nyx_H0") == "Nyx");
  CHECK(story_scene("FldHitActor_105_101_H3").empty());      // field ids, then the hit's number
  CHECK(story_scene("FldHitActor_305_H30_X").empty());       // H<n> is a hit number, not a scene
  CHECK(story_scene("FldHitActor_299_Takaya").empty());      // too short to be one
  CHECK(story_scene("FldNpcActor_299_Takaya_H0").empty());   // not a hit actor
  CHECK(story_scene("FldHitActor_Takaya_Nyx_H0").empty());   // no field id
  CHECK(story_scene("FldHitActor_299_T_H0").empty());        // one letter is no scene
  CHECK(story_scene("FldHitActor_299_Hall_H0") == "Hall");   // H followed by a letter is a word
}

TEST_CASE("numbered_name: a numeric suffix is the name's number plus one") {
  std::string base;
  int n = 0;
  REQUIRE(numbered_name("FLD_TAR_TBOX_OPENED_12", base, n));
  CHECK(base == "FLD_TAR_TBOX_OPENED");
  CHECK(n == 13);
  REQUIRE(numbered_name("FOO_0", base, n));   // a lone zero is a number
  CHECK(base == "FOO");
  CHECK(n == 1);
  REQUIRE(numbered_name("FLD_RESOURCE_SPOT_7", base, n));
  CHECK(n == 8);
}

TEST_CASE("numbered_name: what stays one whole name") {
  std::string base = "untouched";
  int n = -1;
  CHECK_FALSE(numbered_name("FLD_RESOURCE_SPOT10", base, n));           // no underscore before the digits
  CHECK_FALSE(numbered_name("FLD_TAR_TBOX_OPENED_04", base, n));        // a leading zero
  CHECK_FALSE(numbered_name("FLD_MOON_MILITARY_R_MEDAL_01_GOT", base, n));
  CHECK_FALSE(numbered_name("FOO_", base, n));
  CHECK_FALSE(numbered_name("FOO", base, n));
  CHECK_FALSE(numbered_name("FOO_12a", base, n));
  CHECK_FALSE(numbered_name("FOO_1234567890", base, n));                // too long for a number
  CHECK(base == "untouched");
  CHECK(n == -1);
}

TEST_CASE("check_kind: shops, doors and things by the game's names") {
  CHECK(check_kind("Be Blue V") == CheckKind::Shop);
  CHECK(check_kind("Vending Machine") == CheckKind::Shop);
  CHECK(check_kind("Chagall Caf\xC3\xA9") == CheckKind::Shop);
  CHECK(check_kind("Police Station") == CheckKind::Shop);
  CHECK(check_kind("Faculty Office") == CheckKind::Door);
  CHECK(check_kind("Classroom 2-E") == CheckKind::Door);
  CHECK(check_kind("School Building 2F") == CheckKind::Door);
  CHECK(check_kind("Bookshelf") == CheckKind::Thing);
  CHECK(check_kind("be blue v") == CheckKind::Thing);   // the names are matched exactly
  CHECK(check_kind("") == CheckKind::Thing);
}

TEST_CASE("chest_name: by the dungeon object's type") {
  CHECK(chest_name("TBoxPrimField", "Treasure Chest") == "Treasure Chest");
  CHECK(chest_name("TBoxRare", "Treasure Chest") == strings::kPoiChestRare);
  CHECK(chest_name("TBoxJewelry", "Treasure Chest") == strings::kPoiChestGold);
  CHECK(chest_name("TBoxMedal1", "Treasure Chest") == std::string(strings::kPoiChestLocked) + ", 1 " + strings::kPoiFragment);
  CHECK(chest_name("TBoxMedal3", "Treasure Chest") == std::string(strings::kPoiChestLocked) + ", 3 " + strings::kPoiFragments);
  CHECK(chest_name("TBoxMedal3", "Treasure Chest") == "Locked Treasure Chest, 3 Twilight Fragments");
  CHECK(chest_name("", "Bed") == "Bed");
}

TEST_CASE("is_named: a prompt's label names a point of interest") {
  CHECK(is_named("Bed", "Bed"));
  CHECK(is_named("Nozomi Suemitsu (Moon)", "Nozomi Suemitsu"));
  CHECK_FALSE(is_named("Nozomi Suemitsu", "Nozomi Suemitsu (Moon)"));
  CHECK_FALSE(is_named("Bedroom", "Bed"));
  CHECK_FALSE(is_named("Bed 2", "Bed"));
  CHECK_FALSE(is_named("Bed", "Desk"));
}

TEST_CASE("named_as_action: a way onward that says what you do") {
  CHECK(named_as_action("Go Elsewhere"));
  CHECK(named_as_action("Head Up"));
  CHECK(named_as_action("Return to Dorm"));
  CHECK(named_as_action("Proceed Onward"));
  CHECK(named_as_action("Enter the Velvet Room"));
  CHECK(named_as_action("To furthest floor"));
  CHECK_FALSE(named_as_action("Iwatodai Station"));
  CHECK_FALSE(named_as_action("Gold Treasure Chest"));   // "Go" is only a word's start here
  CHECK_FALSE(named_as_action("Tofu Shop"));
  CHECK_FALSE(named_as_action("Exit"));                  // the verb alone, with nothing after it
}

TEST_CASE("settle_category: stay, move on past empty ones, wrap, or find none") {
  auto has = [](std::vector<int> full) {
    return [full](int c) { for (int f : full) if (f == c) return true; return false; };
  };
  CHECK(settle_category(2, 7, 1, false, has({2, 5})) == 2);    // the current one has something: stay
  CHECK(settle_category(2, 7, 1, false, has({5})) == 5);       // else the next that has
  CHECK(settle_category(2, 7, 1, true, has({2, 5})) == 5);     // a move leaves the current one
  CHECK(settle_category(5, 7, 1, true, has({2, 5})) == 2);     // and wraps
  CHECK(settle_category(2, 7, -1, true, has({2, 5})) == 5);    // backwards, wrapping below zero
  CHECK(settle_category(2, 7, 1, true, has({2})) == 2);        // nothing else: back where it was
  CHECK(settle_category(0, 7, -1, true, has({6})) == 6);
  CHECK(settle_category(3, 7, 1, false, has({})) == -1);
  CHECK(settle_category(3, 7, -1, true, has({})) == -1);
}

TEST_CASE("same_thing: one name on one spot") {
  CHECK(same_thing("Door", Vec3{0, 0, 0}, "Door", Vec3{50, 50, 40}));
  CHECK_FALSE(same_thing("Door", Vec3{0, 0, 0}, "Door", Vec3{50, 50, 50}));   // 150 over the three axes
  CHECK_FALSE(same_thing("Door", Vec3{0, 0, 0}, "Gate", Vec3{0, 0, 0}));
  CHECK(same_thing("Door", Vec3{-10, 20, 5}, "Door", Vec3{-10, 20, 5}));
}

TEST_CASE("near_sides: of a door's two exits only the nearer is kept") {
  std::vector<Place> list = {
    {true, "Car 2", Vec3{0, 0, 0}},        // this side of the door
    {false, "Seat", Vec3{50, 0, 0}},
    {true, "Car 1", Vec3{140, 0, 0}},      // its other side, 1.4 m on
    {true, "Car 3", Vec3{-900, 0, 0}},     // another door altogether
    {true, "Car 3", Vec3{-950, 0, 0}},     // the same name twice is not a pair of sides
    {false, "Bin", Vec3{-910, 0, 0}},      // a thing beside an exit is kept
  };
  CHECK(near_sides(list) == std::vector<size_t>{0, 1, 3, 4, 5});
}

TEST_CASE("near_sides: a far side does not hide a third exit") {
  // The middle exit goes (it is 1.5 m from the first); the last is 3 m from the first and is kept, although it is
  // 1.5 m from the one that went.
  std::vector<Place> list = {{true, "A", Vec3{0, 0, 0}}, {true, "B", Vec3{150, 0, 0}}, {true, "C", Vec3{300, 0, 0}}};
  CHECK(near_sides(list) == std::vector<size_t>{0, 2});
  CHECK(near_sides({}).empty());
  // Height counts: an exit 2.5 m above another is no far side of it.
  std::vector<Place> stacked = {{true, "Up", Vec3{0, 0, 0}}, {true, "Down", Vec3{0, 0, 250}}};
  CHECK(near_sides(stacked) == std::vector<size_t>{0, 1});
}

TEST_CASE("icon_state: what the icon over a person says") {
  using p3a::core::icon_kind;
  using p3a::core::icon_state;
  CHECK(icon_kind("BP_UI_AccessIcon_Talk_Read_C") == "Talk_Read");
  CHECK(icon_kind("BP_UI_AccessIcon_Commu_C") == "Commu");
  CHECK(icon_kind("FldNpcActor").empty());
  CHECK(icon_kind("BP_UI_AccessIcon__C").empty());
  CHECK(std::string(icon_state("Talk")).empty());       // the plain state: nothing to add
  CHECK(std::string(icon_state("Listen")).empty());
  CHECK(std::string(icon_state("Talk_Read")) == "talked to");
  CHECK(std::string(icon_state("Listen_Read")) == "heard");
  CHECK(std::string(icon_state("Commu")) == std::string(icon_state("Commu_Field")));
  CHECK(std::string(icon_state("Commu_Reverse")) == "Social Link, reversed");
  CHECK(std::string(icon_state("MaleQuest_Field")) == "Linked Episode");
  CHECK(std::string(icon_state("Something_New")).empty());
}

TEST_CASE("compass_point: eight points from north, clockwise, wrapping") {
  CHECK(p3a::core::compass_point(0) == 0);
  CHECK(p3a::core::compass_point(44) == 1);
  CHECK(p3a::core::compass_point(90) == 2);
  CHECK(p3a::core::compass_point(-90) == 6);
  CHECK(p3a::core::compass_point(-170) == 4);
  CHECK(p3a::core::compass_point(338) == 0);
  CHECK(p3a::core::compass_point(-23) == 7);
}

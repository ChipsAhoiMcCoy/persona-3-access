#include "doctest.h"
#include "core/portrait_text.h"
#include <string>

using namespace p3a::core::portrait;

namespace {
#include "portrait_cases.inc"
}

TEST_CASE("portrait texture names") {
  Shown s;
  REQUIRE(parse_texture("T_BU_PC0002_PoseA_C102", s));
  CHECK(s.chara == 2);
  CHECK(s.clothes == 102);
  CHECK(s.pose == 'A');
  REQUIRE(parse_texture("/Game/Xrd777/UI/Bustup/Textures/SC0101/T_BU_SC0101_PoseA_C158.T_BU_SC0101_PoseA_C158", s));
  CHECK(s.chara == 101);
  CHECK(s.clothes == 158);
  REQUIRE(parse_texture("T_BU_NC1581_PoseA_C006", s));
  CHECK(s.chara == 1581);
  Shown none;
  CHECK_FALSE(parse_texture("T_BU_PC0002_F03_C900_E1", none));   // a face part, not a portrait
  CHECK_FALSE(parse_texture("T_BU_PC0002_PoseA_Blush", none));
  CHECK_FALSE(parse_texture("T_UI_CI_PC0002_01", none));
  CHECK(none.chara == -1);
}

TEST_CASE("every portrait the game has is described") {
  for (auto& p : kGamePortraits) {
    INFO("character ", p.chara, " outfit ", p.clothes, " pose ", p.pose);
    CHECK(has_outfit(p.chara, p.clothes));
    CHECK(has_pose(p.chara, p.clothes, p.pose));
  }
}

TEST_CASE("a portrait's description: the outfit, then the pose") {
  Shown s{2, 1, 'A'};
  std::string d = describe(s);
  CHECK_FALSE(d.empty());
  CHECK(d.find("Yukari") == std::string::npos);   // no names: a name can spoil who someone is
  Shown b{2, 1, 'B'};
  CHECK(describe(b) != d);
  Shown unknown_pose{2, 1, 'Z'};
  CHECK(d.starts_with(describe(unknown_pose)));
  CHECK(describe(Shown{9999, 1, 'A'}).empty());
}

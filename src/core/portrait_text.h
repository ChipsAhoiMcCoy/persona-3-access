#pragma once
#include <string>
#include <string_view>

namespace p3a::core::portrait {
// Written descriptions of the dialogue portraits (bust-ups): the character as the game draws them beside a line, in
// the outfit and pose it shows (the area key during a conversation). Engine-free.
// The game names a portrait's base texture T_BU_<PC|SC|NC><character %04d>_Pose<letter>_C<outfit %03d>
// (FUN_1412c3c80 builds it from the BustupObject's character +0xB8 and outfit +0xC0).
struct Shown {
  int chara = -1;    // the game's character id (2 = Yukari, 101 = Kenji, 1081 = a teacher)
  int clothes = 0;   // the outfit (1 summer uniform, 2 winter uniform, 102 swimsuit...)
  char pose = 0;     // 'A', 'B'...; 0 = unknown
};
bool parse_texture(std::string_view name, Shown& out);   // "T_BU_PC0002_PoseA_C102" (a path's last part is enough)
// The character's look in that outfit, then the pose; "" when the outfit has no description.
std::string describe(const Shown& s);
// For the tests: how many outfits and poses are written, and whether one is.
size_t outfit_count();
size_t pose_count();
bool has_outfit(int chara, int clothes);
bool has_pose(int chara, int clothes, char pose);
}  // namespace p3a::core::portrait

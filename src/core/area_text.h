#pragma once
#include <string>

namespace p3a::core::area {
// Written descriptions of every field area (the area key: controller layer + B), engine-free.
// The scene is the game's own state: its field ids, the place name its banner shows, the calendar, and which of the
// area's variant sublevels it streams (DT_FldVariationSubLevelTable: seasons, Christmas, January's "Phase" levels;
// the time-of-day sets Noon/Evening/Night/Shadow_Only).
enum class Light { Day, Evening, Night, DarkHour };
enum class Season { Spring, Summer, Autumn, Winter };

struct Scene {
  int major = 0, minor = 0;   // field ids (LV_F102_121 = 102, 121); 0 = a place only the banner names (Tartarus)
  std::string name;           // the game's name for the area ("Thebel", "Tartarus: Entrance")
  int month = 0, day = 0;     // the calendar; 0 = unknown
  Light light = Light::Day;
  int floor = 0;              // the storey inside a multi-storey area (dorm, school, strip mall, inn); 0 = unknown
  bool christmas_tree = false;   // the dorm lounge's tree is up (an object the game shows in December)
};

Season season(int month, int day);   // the game's own season dates (4/27, 10/1, 12/4)
int january(int month, int day);     // January's decline: 1 for the 1st-14th, 2 from the 15th, else 0
int floor_of(int major, int minor, float z);   // the storey at height z (cm) in a multi-storey area, else 0
std::string describe(const Scene& s);   // "" when nothing is written for the area
bool known(int major, int minor);       // a description exists for these field ids
}  // namespace p3a::core::area

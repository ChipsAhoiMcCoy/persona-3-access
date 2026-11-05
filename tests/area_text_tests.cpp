#include "doctest.h"
#include "core/area_text.h"
#include <cstdio>
#include <cstdlib>

using namespace p3a::core::area;

static bool has(const std::string& s, const char* part) { return s.find(part) != std::string::npos; }

static Scene at(int major, int minor, int month, int day, Light light = Light::Day, int floor = 0) {
  Scene s;
  s.major = major; s.minor = minor; s.month = month; s.day = day; s.light = light; s.floor = floor;
  return s;
}

TEST_CASE("area seasons follow the game's variant dates") {
  CHECK(season(4, 26) == Season::Spring);
  CHECK(season(4, 27) == Season::Summer);
  CHECK(season(9, 30) == Season::Summer);
  CHECK(season(10, 1) == Season::Autumn);
  CHECK(season(12, 3) == Season::Autumn);
  CHECK(season(12, 4) == Season::Winter);
  CHECK(season(1, 31) == Season::Winter);
  CHECK(season(3, 5) == Season::Spring);
  CHECK(january(12, 31) == 0);
  CHECK(january(1, 14) == 1);
  CHECK(january(1, 15) == 2);
}

TEST_CASE("area storeys from height") {
  CHECK(floor_of(102, 102, 87) == 1);
  CHECK(floor_of(102, 102, 490) == 2);
  CHECK(floor_of(102, 102, 910) == 3);
  CHECK(floor_of(102, 102, 1330) == 4);
  CHECK(floor_of(102, 102, 1750) == 5);
  CHECK(floor_of(122, 102, 910) == 3);   // Episode Aigis' dorm
  CHECK(floor_of(101, 102, 620) == 2);
  CHECK(floor_of(105, 102, -30) == 1);
  CHECK(floor_of(103, 101, 420) == 0);   // one area, however high
}

TEST_CASE("area descriptions pick the storey and the light") {
  std::string lounge = describe(at(102, 102, 4, 21, Light::Night, 1));
  CHECK(has(lounge, "lounge"));
  CHECK(has(lounge, "ceiling lights"));
  CHECK(!has(lounge, "Christmas"));
  CHECK(has(describe(at(102, 102, 4, 21, Light::Day, 3)), "girls' floor"));
  std::string dark = describe(at(102, 121, 4, 9, Light::DarkHour));
  CHECK(has(dark, "Dark Hour"));
  CHECK(!has(dark, "Daylight"));
}

TEST_CASE("area outdoor seasons, Christmas and January") {
  CHECK(has(describe(at(101, 101, 4, 8)), "full bloom"));
  CHECK(has(describe(at(101, 101, 10, 20, Light::Evening)), "red and gold"));
  CHECK(has(describe(at(101, 101, 10, 20, Light::Evening)), "glows warm gold"));
  CHECK(has(describe(at(103, 101, 12, 24, Light::Night)), "Christmas trees"));
  CHECK(!has(describe(at(103, 101, 12, 23, Light::Night)), "Christmas"));
  Scene lounge = at(102, 102, 12, 20, Light::Night, 1);
  CHECK(!has(describe(lounge), "Christmas"));   // the dorm's tree is the game's object, not a date
  lounge.christmas_tree = true;
  CHECK(has(describe(lounge), "Christmas tree"));
  std::string jan = describe(at(106, 101, 1, 20));
  CHECK(has(jan, "almost grey"));
  CHECK(has(jan, "red symbol are everywhere"));
  CHECK(has(describe(at(102, 121, 1, 5, Light::Night)), "draining"));   // indoors: the colour, no posters
  CHECK(!has(describe(at(102, 121, 1, 5, Light::Night)), "posters"));
}

TEST_CASE("area places named only by the game") {
  Scene s;
  s.name = "Thebel";
  CHECK(has(describe(s), "Thebel block"));
  s.name = "Tartarus, floor 5";
  CHECK(has(describe(s), "tower of the Dark Hour"));
  s.name = "Monad Chambers";
  CHECK(has(describe(s), "Monad Chambers"));
  s.name = "Monad Passage";
  CHECK(has(describe(s), "Monad Passage"));
  s.name = "Nowhere";
  CHECK(describe(s).empty());
  // Dark Hour places say no time of day, and keep only their own seasons
  std::string gates = describe(at(201, 101, 4, 9, Light::Day));
  CHECK(has(gates, "cherry trees"));
  CHECK(!has(gates, "daytime"));
  CHECK(!has(describe(at(201, 101, 7, 7, Light::Day)), "cherry"));
  CHECK(has(describe(at(401, 102, 3, 31)), "golden sand"));   // Episode Aigis' Desert of Doors
  CHECK(has(describe(at(406, 101, 3, 31)), "burning torches"));   // a labyrinth's landing
  CHECK(has(describe(at(201, 102, 5, 8)), "gilded clock"));   // Tartarus's entrance hall
  CHECK(known(123, 101));
  CHECK(!known(150, 201));
}

TEST_CASE("area fields whose every area shares a description") {
  CHECK(has(describe(at(207, 102, 1, 31, Light::DarkHour)), "Adamah block"));
  CHECK(has(describe(at(304, 109, 7, 7, Light::DarkHour)), "round bed"));
  CHECK(has(describe(at(304, 101, 7, 7, Light::DarkHour)), "reception window"));   // an exact entry wins
}

// Every described area under every light and season resolves to text (P3A_AREA_DUMP=path writes them all out).
TEST_CASE("area every walkable area under every variant") {
  // The 78 walkable fields of the game data (each has an LV_F*_AreaChange level), storeys where they have them.
  const int areas[][3] = {
    {101,101,0},{101,102,1},{101,102,2},{101,103,0},{101,121,0},{101,141,0},{102,101,0},{102,102,1},{102,102,2},
    {102,102,3},{102,102,4},{102,102,5},{102,121,0},{102,125,0},{102,141,0},{102,151,0},{103,101,0},{103,111,0},
    {104,101,0},{104,102,0},{105,101,0},{105,102,1},{105,102,2},{105,102,3},{106,101,0},{107,101,0},{108,101,1},
    {108,101,2},{108,101,3},{108,102,0},{108,104,0},{108,105,0},{108,106,0},{108,107,0},{108,108,0},{108,109,0},
    {108,110,0},{108,111,0},{108,112,0},{122,101,0},{122,102,1},{122,141,0},{123,101,0},{150,109,0},{150,211,0},
    {170,211,0},{201,102,0},{201,103,0},{202,101,0},{203,101,0},{203,102,0},{204,101,0},{204,102,0},{205,101,0},
    {205,102,0},{206,101,0},{206,102,0},{207,101,0},{207,102,0},{207,103,0},{207,104,0},{302,101,0},{302,102,0},
    {304,101,0},{304,104,0},{304,105,0},{304,106,0},{304,107,0},{304,108,0},{304,109,0},{304,110,0},{304,111,0},
    {304,112,0},{304,113,0},{304,114,0},{305,101,0},{308,101,0},{401,102,0},{401,103,0},{402,101,0},{403,101,0},
    {404,101,0},{405,101,0},{406,101,0},{407,101,0},{408,101,0},{409,101,0}};
  const int dates[][2] = {{4, 10}, {7, 20}, {10, 20}, {12, 24}, {1, 5}, {1, 20}};
  const char* path = std::getenv("P3A_AREA_DUMP");
  FILE* f = path ? std::fopen(path, "w") : nullptr;
  for (auto& a : areas)
    for (auto& d : dates)
      for (Light l : {Light::Day, Light::Evening, Light::Night, Light::DarkHour}) {
        Scene s = at(a[0], a[1], d[0], d[1], l, a[2]);
        std::string t = describe(s);
        CHECK_MESSAGE(!t.empty(), a[0], "_", a[1]);
        if (f) std::fprintf(f, "%d_%d floor %d, %d/%d, light %d: %s\n", a[0], a[1], a[2], d[0], d[1], (int)l, t.c_str());
      }
  if (f) std::fclose(f);
}

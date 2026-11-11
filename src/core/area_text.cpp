// The area descriptions. Each place has a base description, then lines for what changes it: the light (day,
// evening, night, the Dark Hour), the season outdoors, Christmas, and January's decline.
#include "core/area_text.h"
#include <string_view>

namespace p3a::core::area {

Season season(int month, int day) {
  // DT_FldVariationSubLevelTable: Spring 4/1-4/26 and March, Summer 4/27-9/30, Autumn 10/1-12/3, Winter 12/4-1/31
  // (February, between the last day and graduation, is still winter).
  int md = month * 100 + day;
  if (md >= 427 && md <= 930) return Season::Summer;
  if (md >= 1001 && md <= 1203) return Season::Autumn;
  if (md >= 1204 || month <= 2) return Season::Winter;
  return Season::Spring;
}

int january(int month, int day) {
  // LV_FLD_Mono1 (colour 50% drained) and the Phase1 litter and posters from 1/1; Mono2 (70%) and Phase2 from 1/15.
  if (month != 1) return 0;
  return day >= 15 ? 2 : 1;
}

namespace {
struct Entry {
  int major = 0, minor = 0;      // field ids; 0 = matched by name
  int floor = 0;                 // a storey of a multi-storey area; 0 = any
  const char* name = nullptr;    // a substring of the game's name, for places only the banner names
  bool outdoor = false;          // the sky and the seasons show
  bool town = false;             // a street January fills with litter and the cult's posters
  const char* base = nullptr;
  const char* day = nullptr;
  const char* evening = nullptr;
  const char* night = nullptr;
  const char* dark = nullptr;    // the Dark Hour (replaces the light line)
  const char* spring = nullptr;
  const char* summer = nullptr;
  const char* autumn = nullptr;
  const char* winter = nullptr;
  const char* christmas = nullptr;
  bool dark_only = false;        // a place that exists only in the Dark Hour: no light line, and only its own seasons
};

#include "core/area_entries.inc"

// A storey's floor heights (cm) in each multi-storey area, lowest first; measured in the game.
struct Storeys { int major, minor; float tops[5]; };
const Storeys kStoreys[] = {
#include "core/area_storeys.inc"
};

const char* kOutdoorDay = "It is daytime under an open sky.";
const char* kOutdoorEvening = "The sun is setting; the sky glows orange and violet and long shadows stretch across the ground.";
const char* kOutdoorNight = "It is night. Streetlights and lit windows glow against the dark sky.";
const char* kIndoorDay = "Daylight comes in through the windows.";
const char* kIndoorEvening = "Warm sunset light slants in through the windows.";
const char* kIndoorNight = "It is dark outside the windows; the place runs on its ceiling lights.";
const char* kDarkHour =
    "It is the Dark Hour. Everything is washed in a sickly green light under a huge, swollen moon; the electric lights "
    "are dead, dark puddles like blood stain the ground, and anyone caught in it stands sealed in an upright coffin.";
const char* kSpring = "Spring: the trees are fresh and green, and the air is mild.";
const char* kSummer = "It is summer: the air is hot and humid, the sunlight harsh, the trees thick and deep green.";
const char* kAutumn = "It is autumn: the trees have turned orange and red, and dry leaves gather on the ground.";
const char* kWinter = "It is winter: the trees stand bare and the air is cold enough to see your breath.";
const char* kJanuary1 =
    "This January the colour seems to be draining out of the world; everything looks faded and grey.";
const char* kJanuary2 = "It is late January, and the world has gone almost grey, as if its colour were dying out.";
const char* kTown1 =
    "Litter lies uncollected, black garbage bags pile up in the corners, and white posters stamped with a red symbol, "
    "put up by the cult that welcomes the coming Fall, are pasted over the walls, pillars and signs.";
const char* kTown2 =
    "The cult's white posters with their red symbol are everywhere now, plastered over every wall, pillar, sign and "
    "shutter and scattered on the ground among the garbage bags and litter.";

// Episode Aigis rebuilds two fields under its own ids: the dorm (122) and Paulownia Mall (123).
int canonical(int major) { return major == 122 ? 102 : major == 123 ? 103 : major; }

const Entry* find(const Scene& s) {
  const Entry* any_floor = nullptr;
  if (int major = canonical(s.major)) {
    for (const Entry& e : kEntries) {
      if (e.major != major || e.minor != s.minor) continue;
      if (e.floor == s.floor) return &e;
      if (!e.floor && !any_floor) any_floor = &e;
    }
    if (any_floor) return any_floor;
    // a storey the table has no line for: the area's first
    for (const Entry& e : kEntries) if (e.major == major && e.minor == s.minor) return &e;
    for (const Entry& e : kEntries) if (e.major == major && e.minor == -1) return &e;   // any area of the field
  }
  for (const Entry& e : kEntries)
    if (e.name && !s.name.empty() && s.name.find(e.name) != std::string::npos) return &e;
  return nullptr;
}

void add(std::string& out, const char* line) {
  if (!line || !*line) return;
  if (!out.empty()) out += ' ';
  out += line;
}
}  // namespace

int floor_of(int major, int minor, float z) {
  major = canonical(major);
  for (const Storeys& st : kStoreys) {
    if (st.major != major || st.minor != minor) continue;
    int n = 1;
    for (int i = 1; i < 5 && st.tops[i] != 0; ++i) if (z >= st.tops[i]) n = i + 1;
    return n;
  }
  return 0;
}

std::string describe(const Scene& s) {
  const Entry* e = find(s);
  if (!e) return {};
  std::string out;
  add(out, e->base);
  if (e->dark_only) {
    // The game streams these places' own seasonal sets in the Dark Hour too (F201_101's blossoms and leaves).
    if (s.month) {
      Season se = season(s.month, s.day);
      add(out, se == Season::Spring ? e->spring : se == Season::Summer ? e->summer : se == Season::Autumn ? e->autumn : e->winter);
    }
    return out;
  }
  if (s.light == Light::DarkHour) {
    add(out, e->dark ? e->dark : kDarkHour);
    return out;   // the Dark Hour stops time: no season or day lines
  }
  const char* light = s.light == Light::Night ? (e->night ? e->night : e->outdoor ? kOutdoorNight : kIndoorNight)
                    : s.light == Light::Evening ? (e->evening ? e->evening : e->outdoor ? kOutdoorEvening : kIndoorEvening)
                    : (e->day ? e->day : e->outdoor ? kOutdoorDay : kIndoorDay);
  add(out, light);
  if (s.month) {
    int jan = january(s.month, s.day);
    // The mall's decorations are a dated set (12/24-12/25); the dorm's tree is an object the game shows itself.
    bool christmas = e->major == 102 ? s.christmas_tree : s.month == 12 && (s.day == 24 || s.day == 25);
    if (christmas && e->christmas) add(out, e->christmas);
    if (e->outdoor) {
      Season se = season(s.month, s.day);
      const char* line = se == Season::Spring ? (e->spring ? e->spring : kSpring)
                       : se == Season::Summer ? (e->summer ? e->summer : kSummer)
                       : se == Season::Autumn ? (e->autumn ? e->autumn : kAutumn)
                       : (e->winter ? e->winter : kWinter);
      if (*line) add(out, line);
    }
    if (jan) {
      add(out, jan == 1 ? kJanuary1 : kJanuary2);
      if (e->town) add(out, jan == 1 ? kTown1 : kTown2);
    }
  }
  return out;
}
}  // namespace p3a::core::area

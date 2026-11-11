#pragma once
#include <string>
#include <string_view>

namespace p3a::core {
// The moon as the game counts it (ECldMoonAge 0..29, full at 14): "Full moon", "Waxing gibbous moon", ...; "" outside
// that range. The quarters and the new moon are the ages the game's calendar marks: 6, 22 and 29.
std::string moon_phase(int age);
// The phases the calendar grid marks (the quarters, full and new), else "".
std::string moon_mark(int age);
// 0 Sunday .. 6 Saturday; "" for anything else.
const char* weekday_name(int weekday);
// "April 6, Monday", then ", holiday" on one and ", <time of day>" when one is given ("April 6, Monday, Late Night").
// "" for a month outside 1..12.
std::string date_text(int month, int day, int weekday, bool holiday, std::string_view time_of_day);
}  // namespace p3a::core

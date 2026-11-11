#pragma once
#include <string>

namespace p3a::calendar {
// The game's date (UCldBpFuncLib) and the announcements of a new day or time of day. Game thread.
struct Now { int month = 0, day = 0, weekday = 0, time_zone = 0; bool holiday = false; int moon = 0; int total = -1; bool valid = false; };
Now now();
std::string describe(const Now& n);               // "April 6, Monday, Late Night" (core/calendar_text + the game's time of day)
std::string moon_phase(int age);                  // ECldMoonAge -> "Full moon"
std::string moon_mark(int age);                   // the phases the calendar grid marks (quarters, full, new), else ""
void tick();                                      // announces a new day (the day change) or time of day (the time change)
void install();                                   // /calendar/change dev route
}  // namespace p3a::calendar

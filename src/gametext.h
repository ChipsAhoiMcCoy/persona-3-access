#pragma once
#include <string>
#include "ue/ue.h"

namespace p3a::gametext {
// The game's own words, from its data tables (game thread).
std::string system_text(int row);                 // DT_SystemTextName row "<row>" (Name)
std::string time_zone(int tz);                    // ECldTimeZone -> "Late Night" (rows 120..129)
const char* weekday(int youbi);                   // 0 Sunday .. 6 Saturday (mod wording: the table has "(Su)")
ue::UObject* table(const char* name);             // a DataTable by object name, cached
void install();                                   // /gametext dev route
}  // namespace p3a::gametext

namespace p3a::calendar {
struct Now { int month = 0, day = 0, weekday = 0, time_zone = 0; bool holiday = false; int moon = 0; int total = -1; bool valid = false; };
Now now();                                        // UCldBpFuncLib (game thread)
std::string describe(const Now& n);               // "April 6, Monday, Late Night"
std::string moon_phase(int age);                  // ECldMoonAge -> "Full moon"
std::string moon_mark(int age);                   // the phases the calendar grid marks (quarters, full, new), else ""
void tick();                                      // announces a new day (the day change) or time of day (the time change)
}  // namespace p3a::calendar

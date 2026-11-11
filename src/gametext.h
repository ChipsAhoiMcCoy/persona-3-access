#pragma once
#include <string>
#include "ue/ue.h"

namespace p3a::gametext {
// The game's own words, from its data tables (game thread).
std::string system_text(int row);                 // DT_SystemTextName row "<row>" (Name)
std::string time_zone(int tz);                    // ECldTimeZone -> "Late Night" (rows 120..129)
ue::UObject* table(const char* name);             // a DataTable by object name, cached
void install();                                   // /gametext dev route
}  // namespace p3a::gametext

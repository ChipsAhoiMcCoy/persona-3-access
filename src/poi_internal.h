#pragma once
// What the points-of-interest files share with each other (poi.cpp, poi_names.cpp, poi_list.cpp, field_area.cpp,
// field_hud.cpp). poi.h is the module's public face.
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>
#include "poi.h"
#include "watch.h"

namespace p3a::poi {
// Every live instance of a field class, kept by the object watcher (a find_all per class scanned all ~70k objects:
// 7 ms every 300 ms in the field, 2026-10-28).
inline std::vector<ue::UObject*> live(std::string_view cls) { return watch::all(watch::want(cls, true)); }

// ---- poi_names.cpp: what a thing is called ----
const char* kind_name(Kind k);                     // the kind's own word ("exit", "person")
bool flag_on(const std::string& flag);             // a story flag by name (the game's own check)
// Row KIN_<idx, 8 digits> of the first of these name tables that has it, "" when none does.
std::string kin_name(std::initializer_list<const char*> tables, int idx);
std::string npc_name(ue::UObject* npc);            // an NPC's (or Social Link partner's) name as the game labels it
Kind check_kind(const std::string& name);          // a Check-icon hit by its name: Shop, Door or Thing
std::string icon_of(ue::UObject* hit);             // mCheckIcon_'s value name ("Check", "Goto", "None")
bool fragment_spot(ue::UObject* hit);              // a Twilight Fragment lying in town
std::string chest_name(const std::string& type, const std::string& game_name);   // a Tartarus chest by its kind
const char* shadow_name(ue::UObject* actor, ue::UObject* comp);                  // a roaming Shadow by its kind

// ---- field_area.cpp ----
void arrival_tick();   // says the area when the hero arrives somewhere

// ---- field_hud.cpp ----
void pickup_tick();    // items picked up in the field
void prompt_tick();    // the objectives panel and the interaction prompt
}  // namespace p3a::poi

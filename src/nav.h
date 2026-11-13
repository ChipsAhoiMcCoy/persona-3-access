#pragma once
#include <string>
#include "ue/ue.h"

namespace p3a::nav {
// Getting somewhere without sight: the mod walks the player along the navmesh path to the selected point of interest by holding the game's own
// WASD keys (camera-relative 8-way chords), and stops on arrival, on any real movement key, or when stuck.
// use: on arriving, press Space (check, talk, open) once the game's prompt for it is up (within 1.5 s; nothing when
// none comes, as at an exit, or when the player moves meanwhile).
void walk_to(const ue::WeakObj& target, const std::string& name, bool use = false);
void stop(bool announce = true);
bool walking();
void tick();   // game thread
std::string status();
void install();
void unload();   // the mod is being unloaded: let go of the walk's keys (any thread)
}  // namespace p3a::nav

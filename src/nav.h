#pragma once
#include <string>
#include "ue/ue.h"

namespace p3a::nav {
// Getting somewhere without sight, two ways:
//  - walk: the mod walks the player along the navmesh path to the selected point of interest by holding the game's own
//    WASD keys (camera-relative 8-way chords), and stops on arrival, on any real movement key, or when stuck;
//  - beacon: a ping every 0.6 s panned toward the next waypoint of that path (lower pitch when it is behind you), for
//    walking it yourself.
void walk_to(const ue::WeakObj& target, const std::string& name);
void stop(bool announce = true);
bool walking();
void toggle_beacon();
void tick();   // game thread
std::string status();
void install();
void unload();   // the mod is being unloaded: let go of the walk's keys (any thread)
}  // namespace p3a::nav

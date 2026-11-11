#pragma once
#include <string>
#include <vector>
#include "ue/ue.h"

namespace p3a::poi {
// Points of interest in the current field: exits, people, shops and things to check, from the game's own hit
// actors (AFldHitCore: mType_, mCheckIcon_) and NPC actors, filtered to the visible streamed levels.
// Event: a box that starts something when walked into (a Gatekeeper's fight, a story scene): nothing to press.
enum class Kind { Exit, Person, Shop, Door, Thing, Enemy, Event };
struct Poi { ue::WeakObj obj; Kind kind; std::string name; ue::FVector loc; float dist; float bearing; };

std::vector<Poi> scan(float radius_cm = -1);   // nearest first (game thread); -1 = the range setting
std::string clock(float bearing_deg);                   // "12 o'clock" etc. (camera-relative)
std::string describe(const Poi& p);                     // "Exit, 6 meters, 2 o'clock"

// The selection the review keys walk: next/previous within the current category (nearest first), categories cycle
// Exits -> Doors -> People -> Shops -> Objects -> Events -> Shadows, skipping empty ones. selected() re-resolves the object every call.
void next(int delta);
void next_category(int delta);   // selects and speaks the new category's nearest
void announce_list();            // the current category (or the next with something) and its nearest, selected
bool selected(Poi& out);
bool in_selected_category(const Poi& p);   // p belongs to the category the review keys are on
std::string hit_name(ue::UObject* hit);   // a hit actor's display name ("Bed"), or ""
bool breakable(ue::UObject* dungeon_object);   // a FldDungeonObjectActor a strike can still break (not broken yet)
std::string prompt();      // the interaction prompt label showing now ("Treasure Chest"), or ""
std::string objective();   // the story objective last shown in the field ("Head to the faculty office"), or ""
std::string objectives();  // the same with the line stacked under it, if one was shown
std::string area_name();   // the current field area ("Iwatodai Station"), from the game's hit-name table
bool area_ids(int& major, int& minor);   // the field's ids from its visible hit actors' levels (LV_F102_121 = 102, 121)
int tartarus_floor();      // the Tartarus floor the player is on, else 0
void tick();      // game thread: announces area changes
void install();   // keys and dev routes
}  // namespace p3a::poi

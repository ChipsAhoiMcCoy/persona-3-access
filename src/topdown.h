#pragma once
#include <string>

namespace p3a::topdown {
// The top-down mode (the "Top-down exploration" setting), after Dark Vision's view of the field: while the player
// walks freely (world::exploring, the field operator in its Free state with its keys enabled), the game is heard as if
// from above with north always ahead.
//  - The free camera is fixed facing north: the stick's yaw speed is 0 and the camera's follow (it swung round behind
//    the hero) is off. Restored on leaving the mode.
//  - Up walks north whatever camera shows the field (the game walks the hero along the camera's way, and a place's
//    scene cameras have their own): the walk's direction is held north.
//  - The game's own 3D audio is heard from the hero facing north (APlayerController's audio listener override; CRI
//    Atom's listener reads it every frame). Cleared on leaving, so events are heard as authored.
//  - The mod's own sounds face north too (world::set_north).
//  - The right stick is the mod's: pushed toward a compass direction, it says the nearest point of interest that way
//    and how far the wall is, and plays that point's sound from where it is.
// North is the map's up: world -Y where the map is not turned (Tartarus' floor maps draw +X to the right, 2026-11-12);
// a field whose minimap the game turns (MiniMapInfoDataAsset's DegreeAngle) has its north turned the other way
// (yaw -90 - angle: the Command Room's +90 draws +X down, Port Island Station's 180 draws it left; 2026-11-12).
void tick();          // game thread, after the screens (world::exploring reads them)
bool active();        // the mode holds this frame
float north_yaw();
void install();       // dev route /topdown
void shutdown();      // the unload: the camera's values given back
}  // namespace p3a::topdown

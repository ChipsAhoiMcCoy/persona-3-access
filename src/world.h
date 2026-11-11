#pragma once
#include <string>
#include <vector>
#include "audio.h"
#include "ue/ue.h"

namespace p3a::world {
// The player's world, read through Unreal's own reflected API (all game-thread). Units are Unreal's: centimetres,
// X forward / Y right / Z up, yaw in degrees.
using ue::FVector;
// point = where the trace touched; location = the swept shape's centre at that moment; normal = the surface's.
struct Hit { bool hit = false; float distance = 0; FVector point{}; FVector location{}; FVector normal{}; ue::UObject* actor = nullptr; bool start_inside = false; };

void tick();                         // refresh the per-frame state (player, camera)
ue::UObject* player();               // the active field player character (BP_FldPlayer_C), or null
// The story's own state (AtlEvtSubsystem): a scene (event) is playing; it is being fast-forwarded (Tab, or Menu).
bool scene_playing();
bool scene_fast();
// The player is walking the field with nothing over it: a hero, and no menu or shop, no conversation, no battle, no
// story scene, no faded-out screen, no mod menu. Every sound of the field (wall tones, the sonar, the
// Shadow cues) plays only while this holds.
bool exploring();
bool player_location(FVector& out);
// Unit vectors on the ground plane for a camera-relative direction: 0 ahead, 1 right, 2 behind, 3 left.
FVector dir_vector(int dir);
// What blocks the player's body along `dir` (a capsule swept with the Pawn profile), within `range`.
// `people`: when given, characters (NPCs, any Pawn but the player) do not block: each one met is added to the list
// and the trace runs again past it. Keep one list across a tick's traces so each person costs one retry.
Hit sweep(const FVector& from, const FVector& to, float radius, float half_height, std::vector<ue::UObject*>* people = nullptr);
Hit line(const FVector& from, const FVector& to, const char* profile = "Pawn", std::vector<ue::UObject*>* people = nullptr);
std::string status();
ue::FVector actor_location(ue::UObject* a);
bool actor_visible_level(ue::UObject* a);
std::vector<std::string> visible_levels();   // the visible levels' map names ("LV_F103_101_BG_Night_Only")
bool project_to_nav(const ue::FVector& p, float extent, ue::FVector& out);   // nearest navmesh point within extent cm
std::vector<ue::FVector> find_path(const ue::FVector& from, const ue::FVector& to, bool* partial = nullptr);           // its level is the persistent level or a visible streamed one
float bearing(const ue::FVector& from, const ue::FVector& to);   // camera-relative degrees, 0 ahead, 90 right
// Where `to` is heard from `from` (the hero): camera-relative direction with its height, and the distance.
audio::Spot spot(const ue::FVector& from, const ue::FVector& to);
void register_routes();
}  // namespace p3a::world

#pragma once
#include <string>
#include <vector>
#include "audio.h"
#include "core/boxes.h"
#include "core/listener.h"
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
bool player_velocity(FVector& out);   // the hero's velocity (AActor::GetVelocity), cm/s
// Where the player is asking the hero to go (APawn::GetLastMovementInputVector, world space, length 0..1): the stick,
// the keys and the auto-walk alike, even while a wall stops the hero dead.
bool player_input(FVector& out);
// Unit vectors on the ground plane for a listener-relative direction: 0 ahead, 1 right, 2 behind, 3 left (north,
// east, south, west in the top-down mode).
FVector dir_vector(int dir);
// What blocks a body going `from` -> `to`: sweep moves a capsule (radius, half height) with the Pawn profile, line
// traces a line with the given profile. The player is never hit.
// `people`: when given, characters (NPCs, any Pawn but the player) do not block: each one met is added to the list
// and the trace runs again past it. Keep one list across a tick's traces so each person costs one retry.
Hit sweep(const FVector& from, const FVector& to, float radius, float half_height, std::vector<ue::UObject*>* people = nullptr);
Hit line(const FVector& from, const FVector& to, const char* profile = "Pawn", std::vector<ue::UObject*>* people = nullptr);
std::string status();
ue::FVector actor_location(ue::UObject* a);
float yaw_of(ue::UObject* actor);   // the actor's yaw (K2_GetActorRotation)
// A BoxComponent as it stands in the world now (centre, scaled extent, yaw). False without one, or when it is not
// more than 1 cm to each side on the ground.
bool read_box(ue::UObject* box_component, core::Box& out);
// The actor's CapsuleComponent: scaled radius and half height. False (and both left as they were) when it has none.
bool capsule_of(ue::UObject* actor, float& radius, float& half_height);
inline core::Vec3 vec3(const FVector& v) { return core::Vec3{v.x, v.y, v.z}; }
inline FVector fvec(const core::Vec3& v) { return FVector{v.x, v.y, v.z}; }
bool actor_visible_level(ue::UObject* a);   // its level is the persistent level or a visible streamed one
std::vector<std::string> visible_levels();   // the visible levels' map names ("LV_F103_101_BG_Night_Only")
uint64_t levels_stamp();   // goes up whenever the set of visible levels is found changed
bool project_to_nav(const ue::FVector& p, float extent, ue::FVector& out);   // nearest navmesh point within extent cm
// The navmesh path's points, empty when there is none; `partial`: it stops short of `to`.
std::vector<ue::FVector> find_path(const ue::FVector& from, const ue::FVector& to, bool* partial = nullptr);
// Degrees from the listener's facing (north in the top-down mode, else the camera's), 0 ahead, 90 right.
float bearing(const ue::FVector& from, const ue::FVector& to);
// Degrees from the camera's facing whatever the mode: where the game's own movement keys go (the auto-walk).
float camera_bearing(const ue::FVector& from, const ue::FVector& to);
float camera_yaw();
// The top-down mode (topdown.cpp): while on, sounds, bearings and dir_vector face `yaw` (north), not the camera.
void set_north(bool on, float yaw);
bool north_up();
// Where `to` is heard from `from` (the hero): camera-relative direction with its height, and the distance.
audio::Spot spot(const ue::FVector& from, const ue::FVector& to);
void register_routes();
}  // namespace p3a::world

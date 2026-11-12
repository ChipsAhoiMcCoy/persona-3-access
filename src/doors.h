#pragma once
#include "ue/ue.h"

// Closed doors split one loaded map into parts. The gym hallway and the corridor outside it are one level with one
// navmesh, and a door stands shut between them until its exit is used: everything in the corridor was listed from the
// hallway, sounded on the sonar, and the auto-walk ran the hero into the door ("Stuck", 2026-11-11). No hit actor
// says which part it is in, so the parts are found the way a walk would find them: a thing is in another part when
// the walking path to it goes through a closed door (AFldDoorObjCore). The points of interest, the sonar (through
// them) and the auto-walk all ask here. Game thread.
namespace p3a::doors {
void tick();   // follows the hero: every time he may have changed sides of a door, what was worked out is dropped
// The walking path from the hero (at `me`) to the actor at `at` goes through a closed door. Worked out once per actor
// and kept until the hero passes a door, jumps (stairs, a new field) or the visible levels change; false where the
// map has no doors, and for a thing no path leads to (nothing says a door is why).
// A thing the hero sees over the furniture is in his room all the same (classroom 2-F's desks split its navmesh, so
// the path to a front desk goes out of one door and in at the other).
bool behind(ue::UObject* a, const ue::FVector& me, const ue::FVector& at);
// The walking path itself goes through a closed door (behind one, or in this room with no path inside it): the
// auto-walk would end against the door.
bool walk_shut(ue::UObject* a, const ue::FVector& me, const ue::FVector& at);
void install();   // dev route
// Counts the hero changing sides: a door passed, a jump (stairs, an exit), a level coming or going. What was worked
// out about reaching a thing holds while it stands.
uint64_t side_stamp();
}  // namespace p3a::doors

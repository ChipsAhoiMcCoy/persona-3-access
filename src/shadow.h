#pragma once
#include <vector>
#include "ue/ue.h"

namespace p3a::shadow {
// The Shadows roaming a Tartarus floor, in sound (the Shadow alerts setting):
//  - the nearest within 30 m growls from its direction, faster and louder as it closes in;
//  - a Shadow that notices the hero sounds an alarm from where it is, and its growl turns urgent while it hunts him;
//    when it loses him two falling notes say so;
//  - while a Shadow stands inside the reach of the hero's strike (the game's own hit area) a blade's ring repeats:
//    the strike will connect. From behind it, the ring is two rising ticks instead: the strike brings the advantage.
void tick();      // game thread, in the field with nothing over it
void install();   // dev route /shadows
// The enemy manager's list (AFldEnemyManager.Enemies, FFldEnemyHolder): each Shadow's actor beside the component
// that holds its state. Every entry as the game has it (either pointer may be null); empty off a Tartarus floor.
struct Holder { ue::UObject* actor; ue::UObject* comp; };
std::vector<Holder> enemy_holders();
}  // namespace p3a::shadow

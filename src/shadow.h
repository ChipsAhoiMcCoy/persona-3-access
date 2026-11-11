#pragma once

namespace p3a::shadow {
// The Shadows roaming a Tartarus floor, in sound (the Shadow alerts setting):
//  - the nearest within 15 m growls from its direction, faster and louder as it closes in;
//  - a Shadow that notices the hero sounds an alarm from where it is, and its growl turns urgent while it hunts him;
//    when it loses him two falling notes say so;
//  - while a Shadow stands inside the reach of the hero's strike (the game's own hit area) a blade's ring repeats:
//    the strike will connect. From behind it, the ring is two rising ticks instead: the strike brings the advantage.
void tick();      // game thread, in the field with nothing over it
void install();   // dev route /shadows
}  // namespace p3a::shadow

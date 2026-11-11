#pragma once
#include <string>
#include "poi.h"
#include "settings.h"

namespace p3a::sonar {
// The sonar: every point of interest near the hero repeats its kind's sound from where it is, faster and louder the
// closer it is, so the place can be heard without asking for the list. The model is Grim Dark's (a field of
// per-thing metronomes, core/sonar_field); what differs here is the third-person view:
//  - the listener is the hero facing the way the camera looks (where pushing forward walks), through the HRTF with
//    height, so turning the camera turns the whole field;
//  - towns are crowded (a school corridor holds a dozen students), so only the nearest few things sound, and no more
//    than three of one kind;
//  - floors of one building are stacked: another floor's things are left out unless the navmesh walks there
//    (poi::scan's rule).
// It works with the points of interest list ("the radar"): the selected point
// always has a place among those that sound; choosing a point in the list plays its sound once from where it is,
// however far; and the "Sonar plays" setting narrows the field to the list's category or to the selected point alone
// (heard at any distance: a homing sound). Shadows keep their own pulse (poi.cpp).
void tick();                       // game thread
void toggle();                     // the key: on or off, said
void selected(const poi::Poi& p);  // the list chose this point: its sound once, from its place
// The settings menu changed `id`: the sound that shows the change (a kind's own, or one at the new volume), played
// straight ahead. False when the option has no sound to show.
bool preview(settings::Id id);
// The settings menu's sound test: kind 0 exit .. 4 object, from ahead or from behind (lower when that
// setting is on), on the sonar's volume.
void demo(int kind, bool behind = false);
void install();                    // dev route /sonar
}  // namespace p3a::sonar

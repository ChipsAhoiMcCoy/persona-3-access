#pragma once
#include <vector>

// The auto-walk's way in to a target that stands inside something solid (a teleporter between its pillars): which
// sides are open, and how to hold a straight line through a narrow gap with eight walking directions. Engine-free.
namespace p3a::core {

// A run of neighbouring directions the hero's body can come in from.
struct Opening {
  float angle = 0;   // its middle, radians
  float width = 0;   // radians (a full turn: open all round)
  int index = 0;     // the tested direction nearest its middle
};
// `clear[i]`: the body reaches the target from direction i of clear.size(), evenly spaced over a full turn from
// angle 0. Runs join across the end of the list. Widest first.
std::vector<Opening> openings(const std::vector<bool>& clear);

struct Vec2 { float x = 0, y = 0; };
// Walking the segment a -> b from p: the point to steer for, `ahead` cm further along the segment than p's own place
// on it, never past b. Steering for b itself lets the eight walking directions swing the hero round b by up to a
// sixteenth of a turn, off the line; steering for a point just ahead on the line holds him to it.
Vec2 follow(Vec2 a, Vec2 b, Vec2 p, float ahead);

}  // namespace p3a::core

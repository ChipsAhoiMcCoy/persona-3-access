#pragma once
#include "core/approach.h"

// Where a walking path meets a closed door: the piece of a path's leg that lies inside the door's bounds, which is
// then tested against the door itself. Engine-free.
namespace p3a::core {

struct Rect { float x0 = 0, y0 = 0, x1 = 0, y1 = 0; };   // axis-aligned on the ground plane: the low corner, the high one

// The part of the segment a -> b inside the rectangle (its edge counts): `in` and `out` are where that part starts
// and ends, in a -> b order (an end of the segment itself when that end is inside). False when no part is inside.
bool clip(Vec2 a, Vec2 b, const Rect& r, Vec2& in, Vec2& out);

}  // namespace p3a::core

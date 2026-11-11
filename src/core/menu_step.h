#pragma once
// How a game list's cursor is stepped toward the item in focus when it cannot be written: one press at a time, along
// the axis and in the direction the list was declared to move.
#include <utility>

namespace p3a::core {
// One press: along left / right (`horizontal`) or up / down, +1 for right / down, -1 for left / up.
struct MenuStep {
  bool horizontal = false;
  int dir = 1;
  constexpr bool operator==(const MenuStep&) const = default;
};

// A list: toward the higher index is down (right when `horizontal`). `count` > 0: the game's list wraps over that
// many, so the step goes the short way round (forward on a tie). `reversed`: up / left raise the game's index.
constexpr MenuStep list_step(int cursor, int target, bool horizontal, bool reversed, int count) {
  int d = target > cursor ? 1 : -1;
  if (count > 0) {
    int fwd = ((target - cursor) % count + count) % count;
    d = fwd <= count - fwd ? 1 : -1;
  }
  if (reversed) d = -d;
  return {horizontal, d};
}

// A grid, each item at a (row, column) on screen: up / down to the target's row first, then left / right to its
// column.
constexpr MenuStep grid_step(std::pair<int, int> from, std::pair<int, int> to, bool reversed) {
  bool horizontal = from.first == to.first;
  int d = horizontal ? (to.second > from.second ? 1 : -1) : (to.first > from.first ? 1 : -1);
  if (reversed) d = -d;
  return {horizontal, d};
}

// Whether a step from `from` toward `target` carried the cursor past it (the game skipped that item): the target is
// now on the other side of the cursor. For a plain list only: a wrapping list or a grid has no "past".
constexpr bool overshot(int cursor, int target, int from) { return (target - cursor > 0) != (target - from > 0); }
}  // namespace p3a::core

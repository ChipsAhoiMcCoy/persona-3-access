#pragma once
// A scrolling list's window: which entry is at the top when the cursor is on entry `i`, the way the game's own lists
// keep it. The window (`shown` rows over `count` entries, its top at `top` now) moves only as far as it must to hold
// the cursor, and never runs past either end.
namespace p3a::core {
// `shown` out of range (none, or more than there are) means every entry is on screen.
constexpr int scroll_top(int i, int top, int shown, int count) {
  if (shown <= 0 || shown > count) shown = count;
  if (i < top) top = i;
  else if (i >= top + shown) top = i - shown + 1;
  if (top > count - shown) top = count - shown;
  if (top < 0) top = 0;
  return top;
}
}  // namespace p3a::core

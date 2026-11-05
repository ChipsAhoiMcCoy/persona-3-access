#pragma once
#include "ue/ue.h"
#include "ue/scan.h"
#include "watch.h"
#include <windows.h>
#include <string>

namespace p3a::screens {
// The live instance of a class (the newest, if several), as the object watcher saw it made: no scanning, so a
// screen's object is found the tick after it appears and an absent one costs nothing. Never hold the returned pointer
// past the current tick.
class Finder {
 public:
  explicit Finder(const char* cls) : id_(watch::want(cls)) {}
  ue::UObject* get() { return watch::get(id_); }
 private:
  int id_;
};

// The game's generic list cursor (camp lists, shops, the Velvet Room, save list...):
// +0x04 count, +0x08 visible rows, +0x24 row on screen, +0x28 top of the window; the index is row + top. A fresh
// press wraps at the ends, a held key stops there; disabled entries are not skipped.
struct ListCursor {
  uint8_t* p = nullptr;
  explicit ListCursor(void* at) : p((uint8_t*)at) {}
  explicit operator bool() const { return p != nullptr; }
  int count() const { return p ? *(int32_t*)(p + 4) : 0; }
  int index() const { return p ? *(int32_t*)(p + 0x24) + *(int32_t*)(p + 0x28) : -1; }
  // Put the cursor on i the way the game's own clamp does (FUN_1410bebb0): for lists whose draw reads it each frame.
  bool set(int i) const {
    int n = count(), v = p ? *(int32_t*)(p + 8) : 0;
    if (!p || i < 0 || i >= n) return false;
    if (v <= 0 || v > n) v = n;
    int top = *(int32_t*)(p + 0x28);
    if (i < top) top = i;
    else if (i >= top + v) top = i - v + 1;
    if (top > n - v) top = n - v;
    if (top < 0) top = 0;
    *(int32_t*)(p + 0x28) = top;
    *(int32_t*)(p + 0x24) = i - top;
    return true;
  }
};
// A ListCursor stored behind a pointer.
inline ListCursor cursor_at(void* base, int off) { return ListCursor(base ? *(void**)((uint8_t*)base + off) : nullptr); }

// A game sound cue (exe+0x1286af0, BtlSEPlay's too): 0 = the cursor sound every list's move plays.
inline void cursor_sound(int cue = 0) { ((void (*)(int))(scan::exe_base() + 0x1286af0))(cue); }

// Write a list's cursor the way the game's own move does: the index, the cursor bar's slide (the draw's
// bar(draw, row), which the move calls; without it the bar stays on the old row; 0 = the draw reads the cursor every
// frame) and the cursor sound. A write to where the cursor already is does nothing.
inline bool write_cursor(ListCursor lc, int i, void* draw = nullptr, uintptr_t bar_rva = 0) {
  if (lc.index() == i) return lc.p != nullptr;
  if (!lc.set(i)) return false;
  if (draw && bar_rva) ((void (*)(void*, int))(scan::exe_base() + bar_rva))(draw, *(int32_t*)(lc.p + 0x24));
  cursor_sound();
  return true;
}

// Whether a UUserWidget runs no animation (an opening slide drops the game's input): the default Mirror::ready.
inline bool widget_idle(ue::UObject* w) { return !w || !ue::call_ret<bool>(w, "IsAnyAnimationPlaying"); }
}  // namespace p3a::screens

#pragma once
#include "ue/ue.h"
#include "ue/scan.h"
#include "watch.h"
#include "core/list_window.h"
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
    int top = core::scroll_top(i, *(int32_t*)(p + 0x28), v, n);
    *(int32_t*)(p + 0x28) = top;
    *(int32_t*)(p + 0x24) = i - top;
    return true;
  }
};
// A ListCursor stored behind a pointer.
inline ListCursor cursor_at(void* base, int off) { return ListCursor(base ? *(void**)((uint8_t*)base + off) : nullptr); }

// A game sound cue (exe+0x1286af0, BtlSEPlay's too): 0 = the cursor sound every list's move plays.
inline void cursor_sound(int cue = 0) { ((void (*)(int))(scan::exe_base() + 0x1286af0))(cue); }

// Whether the screen is faded out: the game's fade at the end of a fade-out (exe+0x1090af0, what its menus wait on
// before they close after a choice that leaves the place: FUN_14135b890 state 0xF). A menu still on its list then
// is behind a black screen, not in front.
inline bool faded_out() { return ((bool (*)())(scan::exe_base() + 0x1090af0))(); }

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

// Native reads at an offset (a menu's own fields, which no reflected property covers).
// A TArray at `base + off`, or null when it is empty or its count is not a plausible one.
template <class T> ue::TArray<T>* arr(void* base, int off) {
  auto* a = (ue::TArray<T>*)((uint8_t*)base + off);
  return a->data && a->num > 0 && a->num < 2000 ? a : nullptr;
}
// An int at `base + off`; -1 for a null base.
inline int i32(void* base, int off) { return base ? *(int32_t*)((uint8_t*)base + off) : -1; }
// A game function by its rva (a mirror's `set` calls what the game's own move calls).
template <class F> F fn(uintptr_t rva) { return (F)(scan::exe_base() + rva); }
// A ListCursor's row on screen (+0x24), given the cursor's address.
inline int row_of(uint8_t* lc) { return *(int32_t*)(lc + 0x24); }
// Whether a menu's open timer has run its opening: `elapsed` (the menu's own float, seconds) against the frame count
// the game asks of the UI layer `layer` (exe+0x12e13b0), at 30 frames a second. What the game's confirm waits for.
inline bool open_timer_done(float elapsed, void* layer) { return elapsed >= fn<int (*)(void*)>(0x12e13b0)(layer) * (1.f / 30); }
}  // namespace p3a::screens

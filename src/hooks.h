#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <windows.h>

namespace p3a::hooks {
// Detours one function. `orig` receives the trampoline. Every thread of the process is updated in the
// transaction (a thread sitting in a prologue being rewritten is moved), so this is safe at any time.
bool attach(void** orig, void* detour, const char* label);
void detach(void** orig, void* detour);
void detach_all();   // unload: removes every detour still in (the modules', then the Win32 layer's)

// The Win32 layer: the frame's pump (PeekMessageW), focus defang (P3A_NOFOCUS=1, the dev loop) and the low-level
// keyboard hook refusal. Its detours are recorded with the others; detach_all removes them.
bool install();
uint64_t key_ms();         // when the game window last got a key press, or 0
uint64_t key_count();      // how many key presses the game window has got

// Game-thread work. `tick()` is called once per game frame by pump(); it drains the
// queued jobs. run_on_game_thread blocks the caller until the job ran or the timeout passed (false). The job owns
// its captures: a timed-out caller returns while the job may still run later.
void tick();
void set_tick_callback(void (*cb)());   // the mod's per-tick work, run after the jobs
// Tick if this is the game thread and ~8 ms passed since the last tick. Called from PeekMessageW (the frame's
// message pump, every frame) and from ProcessEvent (a fallback while PeekMessageW has not ticked for 250 ms).
void pump();
uint64_t frame();
unsigned ms_since_tick();   // ~0u before the first tick
bool run_on_game_thread(std::function<void()> fn, unsigned timeout_ms = 8000);
unsigned timeouts_on_this_thread();   // run_on_game_thread calls that gave up on this thread (the dev server's 503)
void cancel_pending();                // unload: every waiter and later caller gives up at once
bool on_game_thread();

HWND game_window();   // the game's top-level window (UnrealWindow), or null before it exists
// Keyboard messages reach the filter first (game thread); returning true swallows the message from the game.
using KeyFilter = bool (*)(UINT msg, WPARAM vk, LPARAM lp);
void set_key_filter(KeyFilter f);
// Called (game thread) when the game window is told it loses the keyboard or stops being the active window. Never in
// the dev loop's no-focus mode, which drops those messages: there every key is posted, its key-up with it.
void set_focus_lost(void (*cb)());
std::string counters();
}  // namespace p3a::hooks

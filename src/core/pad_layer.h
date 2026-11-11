#pragma once
#include <cstdint>

namespace p3a::core {
// The controller's mod layer: both triggers held turn the controller into the mod's keys.
// While both triggers are down the game sees neither trigger nor any button (the sticks still walk); a button pressed
// then is the mod's, the held one repeating (first after 450 ms, then every 130 ms). A trigger pressed alone reaches
// the game a moment late (the grace, 150 ms by default), so the first trigger of a two-trigger press does nothing in
// the game; a shorter tap is replayed for the game (80 ms). Anything held when the layer closes stays hidden until it
// is released.
//
// One PadLayer per controller; filter() takes each polled state in turn. Engine-free: the caller reads the pad and the
// clock, and gives the events to whoever handles the mod's buttons.

// Button bits are XInput's (0x0001 .. 0x8000); the triggers are two more bits above them.
constexpr uint32_t kPadLT = 0x10000, kPadRT = 0x20000, kPadTriggers = kPadLT | kPadRT;

struct PadState {
  uint16_t buttons = 0;
  uint8_t lt = 0, rt = 0;                // 0 .. 255
  int16_t lx = 0, ly = 0, rx = 0, ry = 0;
};

struct PadLayerOptions {
  bool modal = false;         // a mod menu has the controller: the game sees nothing, every new button is an event
  bool claim_taps = false;    // a lone trigger is the caller's (a menu's section jump): reported as a tap, never the game's
  uint64_t grace_ms = 150;    // how long a lone trigger is held back in case the other follows
};

struct PadLayerEvent { uint16_t button; bool repeat; };   // button 0 = the layer opened

// What one filter() call produced, in order.
struct PadLayerOutput {
  static constexpr int kMax = 20;   // at most: the opening, sixteen buttons, one repeat
  PadLayerEvent events[kMax];
  int count = 0;
  int tap = 0;                      // a claimed trigger: -1 LT, +1 RT, 0 none
  void add(uint16_t button, bool repeat) { if (count < kMax) events[count++] = {button, repeat}; }
};

struct PadLayer {
  bool on = false;                  // both triggers are down
  uint32_t prev = 0, hidden = 0, pend = 0, replay = 0, held = 0;
  uint64_t pend_at = 0, replay_until = 0, repeat_at = 0;

  // One polled state at `now_ms` (any steady millisecond clock): rewrites `g` to what the game may see and reports
  // the mod's button events and a claimed trigger tap in `out`.
  void filter(PadState& g, uint64_t now_ms, const PadLayerOptions& o, PadLayerOutput& out);

 private:
  void buttons(uint32_t state, uint32_t pressed, uint64_t now, PadLayerOutput& out);
};
}  // namespace p3a::core

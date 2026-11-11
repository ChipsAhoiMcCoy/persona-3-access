#include "core/pad_layer.h"

namespace p3a::core {
// Each newly pressed button is an event and becomes the held one; the held one repeats while it stays down.
void PadLayer::buttons(uint32_t s, uint32_t pressed, uint64_t now, PadLayerOutput& out) {
  for (uint32_t bit = 1; bit <= 0x8000; bit <<= 1)
    if (pressed & bit) { out.add((uint16_t)bit, false); held = bit; repeat_at = now + 450; }
  if (held && (s & held) && now >= repeat_at) { out.add((uint16_t)held, true); repeat_at = now + 130; }
}

void PadLayer::filter(PadState& g, uint64_t now, const PadLayerOptions& o, PadLayerOutput& out) {
  // A trigger counts as held past 100 of 255 and until it falls under 40 (no flicker at the threshold).
  uint32_t trig = 0;
  if (g.lt > 100 || (g.lt >= 40 && (prev & kPadLT))) trig |= kPadLT;
  if (g.rt > 100 || (g.rt >= 40 && (prev & kPadRT))) trig |= kPadRT;
  uint32_t s = g.buttons | trig, pressed = s & ~prev;
  hidden &= s;   // released: no longer hidden
  bool both = (s & kPadTriggers) == kPadTriggers;
  if (o.modal) {
    // A mod menu: the game sees nothing (sticks included); each new button goes to the menu. Triggers still held
    // when it closes count as the layer already open, so closing it from the layer does not announce the layer again.
    on = both; pend = 0;
    hidden |= s;
    buttons(s, pressed, now, out);
    prev = s;
    g = PadState{};
    return;
  }
  const uint64_t grace = o.grace_ms;
  if (both && !on) { on = true; pend = 0; out.add(0, false); }
  else if (!both && on) on = false;
  if (on) {
    hidden |= s;
    buttons(s, pressed, now, out);
  } else {
    held = 0;
    // One trigger alone: held back for a moment in case the other follows.
    if (uint32_t t = pressed & kPadTriggers & ~hidden; t && !(s & kPadTriggers & ~t)) { pend = t; pend_at = now; }
    // A claimed lone trigger is taken once the other has had a moment to follow (the layer): the tap fires on the
    // press, not the release (an analog trigger takes a while to travel back), and the trigger stays hidden from the
    // game until let go.
    constexpr uint64_t kTapWindow = 50;
    if (o.claim_taps && pend && now - pend_at >= (grace < kTapWindow ? grace : kTapWindow)) {
      out.tap = pend == kPadLT ? -1 : 1;
      hidden |= pend;
      pend = 0;
    }
    if (pend && !(s & pend)) {   // let go within the grace: the game gets a short press after all
      if (now - pend_at < grace) {
        if (o.claim_taps) out.tap = pend == kPadLT ? -1 : 1;   // the caller's tap instead
        else { replay = pend; replay_until = now + 80; }
      }
      pend = 0;
    }
    if (pend && now - pend_at >= grace) pend = 0;
  }
  prev = s;
  uint32_t hide = hidden | pend;
  g.buttons &= ~(uint16_t)(hide & 0xFFFF);
  if (hide & kPadLT) g.lt = 0;
  if (hide & kPadRT) g.rt = 0;
  if (replay && now < replay_until) {
    if (replay & kPadLT) g.lt = 255;
    if (replay & kPadRT) g.rt = 255;
  } else {
    replay = 0;
  }
}
}  // namespace p3a::core

#pragma once
// The sonar's sounds, made here rather than shipped as files: one short sound per kind of thing, each with a
// character of its own (so they are told apart by more than pitch) and energy across the spectrum (a pure tone gives
// the ears little to place ahead or behind). Mono, engine-free, all at one loudness.
#include <vector>

namespace p3a::core {

enum class SonarCue {
  Exit,     // a way into another area: a breath of air rising through an opening, over a low thump
  Door,     // a room's door: two knocks on wood
  Person,   // someone to talk to: a two-note whistle, the second note lower ("yoo-hoo")
  Shop,     // a shop or vendor: two strikes of a small bell, the second higher (a till)
  Thing,    // something to check or open: one bright plucked note
  Count
};

// The sound at `rate` Hz: starts and ends on silence, under 0.2 s, RMS kSonarCueRms (peak under 0.9).
// brief: its clipped form for a thing too near for the whole sound to fit its rhythm, under 0.07 s: the first knock,
// the first note, the bell's higher strike, a puff of the air, the pluck's start.
std::vector<float> sonar_cue(SonarCue cue, int rate, bool brief = false);
inline constexpr float kSonarCueRms = 0.10f;   // what the spikiest of them (the pluck) reaches under the peak limit

// The mod's other signals, made the same way and at the same loudness (they were plain sine and triangle beeps):
enum class SignalCue {
  Shadow,   // a Shadow near: a low, rattling snarl that dies away
  Ambush,   // the strike will land from behind: two quick glassy ticks, the second higher
  Layer,    // the controller's mod layer opened: one soft tick
  Reach,    // a Shadow or a breakable thing is within the strike's reach: a quick ting of steel, repeated fast
  Spotted,  // a Shadow has noticed the hero: a harsh two-note alarm, rising
  Lost,     // it has lost him again: two soft notes, falling
  Count
};
std::vector<float> signal_cue(SignalCue cue, int rate);

}  // namespace p3a::core

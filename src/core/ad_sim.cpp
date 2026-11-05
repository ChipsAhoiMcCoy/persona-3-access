#include "core/ad_sim.h"

namespace p3a::core::ad {

int SimResult::count(State s) const {
  int n = 0;
  for (State x : states) n += x == s;
  return n;
}

SimResult simulate(const std::vector<Cue>& cues, const Scenario& sc) {
  Schedule sched;
  sched.load(cues);
  SimResult r;
  r.started.assign(cues.size(), -1);
  r.played.assign(cues.size(), 0);
  uint32_t rng = sc.seed * 2654435761u + 1;
  auto rnd = [&] { rng = rng * 1664525u + 1013904223u; return (rng >> 8) / double(1 << 24); };

  double t = 0;              // movie time
  int voice = -1;            // the narrator's clip
  double voice_pos = 0;
  bool voice_paused = false, voice_ended = false;
  size_t next_pause = 0, next_hitch = 0;
  double pause_left = 0;
  bool seeked = false;
  double after_stop = 0;
  const double end = sc.length > 0 ? sc.length : (cues.empty() ? 1 : cues.back().at + cues.back().dur + 2);

  for (int frame = 0; frame < 10000000; ++frame) {
    double dt = (1.0 / sc.fps) * (1.0 + sc.jitter * (2 * rnd() - 1));
    bool paused = pause_left > 0;
    // the movie clock for this frame
    bool was_stopped = (sc.stop_at >= 0 && t >= sc.stop_at) || t >= end;
    if (paused) pause_left -= dt;
    else if (was_stopped) {}
    else {
      if (next_hitch < sc.hitches.size() && t >= sc.hitches[next_hitch].at) dt = sc.hitches[next_hitch++].len;
      t += dt;
      if (next_pause < sc.pauses.size() && t >= sc.pauses[next_pause].at) { pause_left = sc.pauses[next_pause++].len; }
      if (!seeked && sc.seek_at >= 0 && t >= sc.seek_at) { t = sc.seek_to; seeked = true; }
    }
    bool stopped = (sc.stop_at >= 0 && t >= sc.stop_at) || t >= end;
    // the narrator advances in real time while it plays
    if (voice >= 0 && !voice_paused && !voice_ended) {
      voice_pos += dt;
      if (voice_pos >= cues[voice].dur) { voice_pos = cues[voice].dur; voice_ended = true; }
      r.played[voice] = voice_pos;
    }
    Clock c{!stopped, paused, t};
    Voice v{voice >= 0 && !voice_ended, voice >= 0 && voice_ended};
    for (const Action& a : sched.step(c, v)) {
      switch (a.act) {
        case Act::Play: voice = a.cue; voice_pos = 0; voice_paused = false; voice_ended = false; r.started[a.cue] = t; break;
        case Act::Pause: voice_paused = true; break;
        case Act::Resume: voice_paused = false; break;
        case Act::Stop: voice = -1; voice_ended = false; break;
      }
    }
    if (stopped && (sched.playing() < 0 || (after_stop += dt) > 3)) break;   // a last description may play out
  }
  for (size_t i = 0; i < cues.size(); ++i) { r.states.push_back(sched.state((int)i)); r.reasons.push_back(sched.reason((int)i)); }
  return r;
}

}  // namespace p3a::core::ad

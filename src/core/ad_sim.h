#pragma once
// A movie played against the real Schedule with a simulated narrator, frame by frame, far faster than real time:
// the unit tests and tools' ad_sim (every movie's real cue list) use it. The narrator plays a cue for exactly its
// length in unpaused time; the movie clock can pause, hitch, jump and stop where a scenario says.
#include "core/ad_schedule.h"
#include <cstdint>
#include <string>
#include <vector>

namespace p3a::core::ad {

struct Scenario {
  std::string name = "clean";
  double fps = 60;
  double jitter = 0.3;                 // each frame lasts 1/fps * (1 +- jitter), seeded
  uint32_t seed = 1;
  struct Pause { double at, len; };    // movie time to pause at, real seconds held
  std::vector<Pause> pauses;
  struct Hitch { double at, len; };    // a frame that takes len seconds (a load stall): the clock jumps
  std::vector<Hitch> hitches;
  double seek_at = -1, seek_to = -1;   // one jump of the clock (forward or back)
  double stop_at = -1;                 // the movie stops here (a skip)
  double length = 0;                   // the movie's length
};

struct SimResult {
  std::vector<State> states;
  std::vector<std::string> reasons;
  std::vector<double> started;         // movie time each cue started (-1 never)
  std::vector<double> played;          // seconds of it the narrator played
  int count(State s) const;
};

SimResult simulate(const std::vector<Cue>& cues, const Scenario& sc);

}  // namespace p3a::core::ad

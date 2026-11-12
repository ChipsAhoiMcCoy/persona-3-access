#pragma once
// Ported from Dark Vision (WrathAccess, src/Exploration/Overlays/SonarSystem.cs) by Bradley Renshaw; zlib, see
// LICENSES/. Altered: C++, engine-free, a clock handed in. This is not the original software.
//
// The sonar as a sweep: every thing in range sounds once, one at a time, west to east (so two of a kind read as "left
// ... right", never one averaged sound in the middle), then a rest, then the next sweep. The gap between two sounds
// shrinks as the crowd grows (spread / count, kept between gap_min and gap_max): a few things sound spacious, many
// compress, nothing is ever dropped. A thing gone from the items when its turn comes is skipped.
// Engine-free: the host hands (id, east offset, distance) each frame and plays what `update` reports.
#include <vector>

namespace p3a::core {

struct SweepParams {
  double spread_s = 0.75;   // the gap at one thing, before the clamp
  double gap_min_s = 0.10;
  double gap_max_s = 0.20;
  double rest_s = 0.40;     // after each sweep (and the wait before looking again when nothing is in range)
  double gap_for(int count) const;
};

// Distance to volume: ref / (ref + d), never under 0.08 (a far thing in range stays audible). Dark Vision's
// reference distance is 10 ft; here 3 m.
float sweep_gain(float dist_m, float ref_m = 3.0f);

class SonarSweep {
 public:
  struct Item { unsigned id; float east; int kind; };
  struct Ping { unsigned id; int kind; };
  // At most one ping per call: the next thing in this sweep once its moment has come.
  std::vector<Ping> update(const std::vector<Item>& items, double now_s);
  void reset();
  SweepParams& params() { return params_; }
  const SweepParams& params() const { return params_; }
  std::size_t pending() const { return order_.size() - next_; }

 private:
  SweepParams params_;
  std::vector<Item> order_;   // this sweep, west to east
  std::size_t next_ = 0;
  double due_ = 0;
  bool started_ = false;
};

}  // namespace p3a::core

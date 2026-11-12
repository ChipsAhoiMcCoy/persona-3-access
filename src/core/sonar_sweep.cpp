// Ported from Dark Vision (WrathAccess, src/Exploration/Overlays/SonarSystem.cs) by Bradley Renshaw; zlib, see
// LICENSES/. Altered: C++, engine-free. This is not the original software.
#include "core/sonar_sweep.h"
#include <algorithm>

namespace p3a::core {

double SweepParams::gap_for(int count) const {
  return std::clamp(spread_s / (double)std::max(1, count), gap_min_s, gap_max_s);
}

float sweep_gain(float dist_m, float ref_m) {
  return std::clamp(ref_m / (ref_m + std::max(dist_m, 0.0f)), 0.08f, 1.0f);
}

void SonarSweep::reset() { order_.clear(); next_ = 0; due_ = 0; started_ = false; }

std::vector<SonarSweep::Ping> SonarSweep::update(const std::vector<Item>& items, double now_s) {
  std::vector<Ping> out;
  if (!started_) { started_ = true; due_ = now_s; }
  if (now_s < due_) return out;
  if (next_ >= order_.size()) {   // the sweep is done (or none yet): a fresh one from what is in range now
    order_ = items;
    std::stable_sort(order_.begin(), order_.end(), [](const Item& a, const Item& b) { return a.east < b.east; });
    next_ = 0;
    if (order_.empty()) { due_ = now_s + params_.rest_s; return out; }
  }
  // The next one still in range (the others went away since the sweep began: skipped, no gap for them).
  while (next_ < order_.size()) {
    const Item& it = order_[next_++];
    auto live = std::find_if(items.begin(), items.end(), [&](const Item& i) { return i.id == it.id; });
    if (live == items.end()) continue;
    out.push_back({it.id, live->kind});
    break;
  }
  due_ = now_s + (next_ >= order_.size() ? params_.rest_s : params_.gap_for((int)order_.size()));
  return out;
}

}  // namespace p3a::core

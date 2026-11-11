// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#include "core/sonar_field.h"
#include <algorithm>
#include <cmath>

namespace p3a::core {

double FieldParams::period_for(float dist) const {
  float lo = dist_near > 0.01f ? dist_near : 0.01f;
  float hi = dist_far > lo ? dist_far : lo + 0.01f;
  float d = std::clamp(dist, lo, hi);
  // A straight line between the two ends with both axes logarithmic (a power law): halving the distance divides the
  // period by the same factor wherever it happens, so the rate climbs steeply over the last few metres.
  double u = std::log((double)d / lo) / std::log((double)hi / lo);   // 0 at lo, 1 at hi
  double a = period_near > 0.001 ? period_near : 0.001, b = period_far > a ? period_far : a;
  return a * std::pow(b / a, u);
}

static double hash01(unsigned id) {   // a stable fraction in [0, 1) per id: neighbouring ids land far apart
  unsigned h = id * 2654435761u; h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
  return (double)(h & 0xffffffu) / (double)0x1000000u;
}

std::vector<SonarField::Ping> SonarField::update(const std::vector<Item>& items, double now) {
  std::vector<Ping> out;
  const bool running = started_;
  started_ = true;
  std::unordered_map<unsigned, Slot> keep;
  keep.reserve(items.size() + slots_.size());
  // Whose turn: an arrival before anything else, then the most overdue against its own period, a short (near)
  // period counting for more (turn_power); among equals the host's `first`, then the nearer.
  const Item* turn = nullptr;
  bool turn_arrived = false;
  double turn_overdue = 0;
  const bool line_free = now >= free_at_;
  for (const Item& it : items) {
    double T = params_.period_for(it.dist);
    if (T < 0.01) T = 0.01;
    auto found = slots_.find(it.id);
    Slot s;
    if (found == slots_.end()) {
      // A thing that comes into a running field is due at once: waiting out a fraction of its slow far-off period,
      // the player was nearly on it before its first sound. A field starting up (after a reset: a menu closed, a
      // floor loaded) staggers what is already there instead.
      s = Slot{running ? now : now + hash01(it.id) * T, 0, now, false, running};
    } else {
      s = found->second;
      s.last_seen = now;
      // Its period follows its distance from one frame to the next: a thing that came closer is not left waiting on
      // the far-off period it last sounded by.
      if (s.played) s.want = s.last_start + T;
    }
    if (line_free && now >= s.want) {
      double overdue = (now - s.want) / std::pow(T, params_.turn_power);
      bool better = !turn || (s.arrived != turn_arrived ? s.arrived
                              : overdue != turn_overdue ? overdue > turn_overdue
                              : it.first != turn->first ? it.first : it.dist < turn->dist);
      if (better) { turn = &it; turn_arrived = s.arrived; turn_overdue = overdue; }
    }
    keep.emplace(it.id, s);
  }
  if (turn) {
    Slot& s = keep[turn->id];
    s.played = true; s.arrived = false; s.last_start = now;
    s.want = now + std::max(params_.period_for(turn->dist), 0.01);
    free_at_ = now + std::max(turn->len_s, 0.0f) + params_.gap_s;
    out.push_back({turn->id, turn->kind});
  }
  // Grace: an id not listed this frame keeps its place until it has been unseen for grace_s.
  for (const auto& [id, slot] : slots_)
    if (!keep.count(id) && now - slot.last_seen <= params_.grace_s) keep.emplace(id, slot);
  slots_.swap(keep);
  return out;
}

void SonarField::reset() { slots_.clear(); started_ = false; free_at_ = 0; }

}  // namespace p3a::core

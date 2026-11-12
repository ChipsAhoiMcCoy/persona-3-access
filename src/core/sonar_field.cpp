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
  struct Due { const Item* it; double overdue; };
  std::vector<Due> due;
  for (const Item& it : items) {
    double T = std::max(params_.period_for(it.dist), 0.01);
    auto found = slots_.find(it.id);
    Slot s;
    if (found == slots_.end()) {
      // A thing that comes into a running field is due at once: waiting out a fraction of its slow far-off period,
      // the player was nearly on it before its first sound. A field starting up (after a reset: a menu closed, a
      // floor loaded) staggers what is already there instead.
      s = Slot{running ? now : now + hash01(it.id) * T, -1e9, now, it.kind, false};
    } else {
      s = found->second;
      s.last_seen = now;
      s.kind = it.kind;
      // Its period follows its distance from one frame to the next: a thing that came closer is not left waiting on
      // the far-off period it last sounded by.
      if (s.played) s.want = s.last_start + T;
    }
    if (now >= s.want) due.push_back({&it, (now - s.want) / T});
    keep.emplace(it.id, s);
  }
  // Grace: an id not listed this frame keeps its place until it has been unseen for grace_s. Kept before the turns
  // are taken: its last start still keeps one of its kind from starting on top of it.
  for (const auto& [id, slot] : slots_)
    if (!keep.count(id) && now - slot.last_seen <= params_.grace_s) keep.emplace(id, slot);
  // Of those due, the most overdue against its own period first (among equals the nearer); each starts unless one of
  // its kind started within collide_s, and then waits for the next frame.
  std::sort(due.begin(), due.end(), [](const Due& a, const Due& b) {
    return a.overdue != b.overdue ? a.overdue > b.overdue : a.it->dist < b.it->dist;
  });
  for (const Due& d : due) {
    bool clear = true;
    for (const auto& [id, slot] : keep)
      if (id != d.it->id && slot.kind == d.it->kind && now - slot.last_start < params_.collide_s) { clear = false; break; }
    if (!clear) continue;
    Slot& s = keep[d.it->id];
    s.played = true; s.last_start = now;
    s.want = now + std::max(params_.period_for(d.it->dist), 0.01);
    out.push_back({d.it->id, d.it->kind});
  }
  slots_.swap(keep);
  return out;
}

void SonarField::reset() { slots_.clear(); started_ = false; }

}  // namespace p3a::core

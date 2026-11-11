// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#include "core/sonar_field.h"
#include <algorithm>
#include <cmath>

namespace p3a::core {

double FieldParams::period_for(float dist) const {
  float lo = dist_near > 0.01f ? dist_near : 0.01f;
  float hi = dist_far > lo ? dist_far : lo + 0.01f;
  float d = std::clamp(dist, lo, hi);
  // Linear in log(distance): d(log d)/dd = 1/d, so the rate swings hardest where d is smallest (up close).
  double u = std::log((double)d / lo) / std::log((double)hi / lo);   // 0 at lo, 1 at hi
  return period_near + u * (period_far - period_near);
}

static double hash01(unsigned id) {   // a stable fraction in [0, 1) per id: neighbouring ids land far apart
  unsigned h = id * 2654435761u; h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
  return (double)(h & 0xffffffu) / (double)0x1000000u;
}

double SonarField::push_clear(double due, unsigned id, int kind, const std::unordered_map<unsigned, Slot>& keep) const {
  const double w = params_.collide_s;
  if (w <= 0) return due;
  for (int pass = 0; pass < 16; ++pass) {   // each pass moves past the nearest clash; converges fast in practice
    bool moved = false;
    auto check = [&](const std::unordered_map<unsigned, Slot>& m, bool skip_kept) {
      for (const auto& [oid, slot] : m) {
        if (oid == id || slot.kind != kind) continue;
        if (skip_kept && keep.count(oid)) continue;   // next_at_ entry superseded by this frame's keep entry
        if (std::fabs(slot.due - due) < w) { due = slot.due + w; moved = true; }
      }
    };
    check(keep, false);
    check(next_at_, true);
    if (!moved) return due;
  }
  return due;
}

std::vector<SonarField::Ping> SonarField::update(const std::vector<Item>& items, double now) {
  std::vector<Ping> out;
  std::unordered_map<unsigned, Slot> keep;
  keep.reserve(items.size() + next_at_.size());
  const bool running = started_;
  started_ = true;
  for (const Item& it : items) {
    double T = params_.period_for(it.dist);
    if (T < 0.01) T = 0.01;
    auto found = next_at_.find(it.id);
    double due;
    if (found == next_at_.end()) {
      // A thing that comes into a running field sounds at once (the next frame): waiting out a fraction of its slow
      // far-off period, the player was nearly on it before its first pulse. A field starting up (after a reset: a
      // menu closed, a floor loaded) staggers what is already there instead, so they do not all sound together.
      double first = running ? now : now + hash01(it.id) * T;
      due = push_clear(first, it.id, it.kind, keep);
    } else {
      due = found->second.due;
      if (now >= due) {
        out.push_back({it.id, it.kind});
        do { due += T; } while (due <= now);   // advance whole periods: burst-proof, keeps the phase grid
        due = push_clear(due, it.id, it.kind, keep);   // and off any same-kind neighbour it would have landed on
      } else if (due - now > T * 1.25) {
        // It came closer: the next pulse is not left waiting on the far-off period it was scheduled by. (The margin
        // leaves alone a due that push_clear only nudged past a neighbour.)
        due = push_clear(now + T, it.id, it.kind, keep);
      }
    }
    keep.emplace(it.id, Slot{due, now, it.kind});
  }
  // Grace: an id not listed this frame keeps its grid until it has been unseen for grace_s.
  for (const auto& [id, slot] : next_at_)
    if (!keep.count(id) && now - slot.last_seen <= params_.grace_s) keep.emplace(id, slot);
  next_at_.swap(keep);
  return out;
}

void SonarField::reset() { next_at_.clear(); started_ = false; }

}  // namespace p3a::core

// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#pragma once
// The sonar as a field of per-thing metronomes. Every nearby thing repeats its OWN sound; the period shrinks as it
// nears the player -- mapped logarithmically in distance, so a step closer changes the rate a lot up close and
// little far off -- and its first pulse is seeded a stable per-id fraction of a period in (a hash of the id), so
// things at one distance stagger, and a pulse due within `collide_s` of another of the same kind is pushed past it,
// so two things never fire as one. Engine-free: the host hands (id, distance, kind) each frame and plays whatever
// `update` reports fired, working out direction and loudness at that moment.
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace p3a::core {

struct FieldParams {
  double period_near = 0.30;   // seconds between pulses at dist_near and closer
  double period_far = 1.00;    // seconds at dist_far and beyond (1.4 at first: too sparse to notice at a run)
  float dist_near = 1.5f;      // metres
  float dist_far = 10.0f;      // metres (the host sets it to the sonar's range)
  double grace_s = 1.5;        // an id missing from the items keeps its phase grid this long (the range's edge,
                               // a list rebuilt without it for a moment) before it is forgotten and seeded again
  double collide_s = 0.06;     // a due time within this of another same-kind due is pushed later past it (0 = off)
  double period_for(float dist) const;   // log-in-distance interpolation between the two endpoints
};

class SonarField {
 public:
  struct Item { unsigned id; float dist; int kind; };
  struct Ping { unsigned id; int kind; };
  // Advance to now_s over the current item set. Returns the ids whose sound pulses this frame (0 or more).
  // The ids there at the first update after a reset are seeded a phase-offset into their periods (so they stagger);
  // an id that arrives later pulses on the next update. An id absent from `items` keeps its grid for grace_s (an
  // overdue one fires once on its return, then continues on the grid), after which it is forgotten.
  std::vector<Ping> update(const std::vector<Item>& items, double now_s);
  void reset();
  FieldParams& params() { return params_; }
  const FieldParams& params() const { return params_; }
  std::size_t tracked() const { return next_at_.size(); }

 private:
  FieldParams params_;
  struct Slot { double due, last_seen; int kind; };   // when its next pulse is due (its own phase grid), when it was last listed
  // Push `due` later until no other same-kind due (already scheduled this frame in `keep`, or carried in next_at_)
  // lies within collide_s of it; a bounded walk, so a dense field costs at most a few windows of delay.
  double push_clear(double due, unsigned id, int kind, const std::unordered_map<unsigned, Slot>& keep) const;
  std::unordered_map<unsigned, Slot> next_at_;
  bool started_ = false;   // updated since the last reset: a new id is an arrival, not part of the opening set
};

}  // namespace p3a::core

// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#pragma once
// The sonar as a field of per-thing metronomes that take turns. Every nearby thing wants to repeat its OWN sound at
// its own rate; the period shrinks as it nears the player -- a power law in distance, in metres whatever the sonar's
// range is, so a given rate always means the same distance and halving the distance speeds it up by the same factor
// everywhere. But only one sound plays at a time: when a thing's moment comes it waits for the line to be free (the
// sound before it over, plus a breath), and of those waiting the one most overdue against its own period goes next,
// a short period counting for more. So nothing ever plays over anything else, a near thing keeps most of its rhythm
// in a crowd at the far ones' cost, and none is starved. (Grim Dark's field lets them overlap and only keeps two of
// one kind from starting together; six things beside the player were a pile-up here.)
// Engine-free: the host hands (id, distance, how long its sound lasts) each frame and plays what `update` reports.
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace p3a::core {

struct FieldParams {
  // 0.30 s at 1.5 m to 1.00 s at the range's edge at first: three to one, and stretched with the range setting, so
  // being close to a thing was not heard (the user, 2026-11-11). Now 0.12 s at 1 m, 0.30 at 3 m, 0.47 at 5 m, 0.84
  // at 10 m, 1.5 at 20 m and beyond.
  double period_near = 0.12;   // seconds between pulses at dist_near and closer
  double period_far = 1.50;    // seconds at dist_far and beyond
  float dist_near = 1.0f;      // metres
  float dist_far = 20.0f;      // metres
  // How a waiting thing's lateness is weighed: seconds overdue over its period to this power. 1 shares the line in
  // proportion to the rates wanted (a thing at 1 m among five others got a third of its rate); 2 lets the short
  // periods keep more of theirs and takes it from the long ones.
  double turn_power = 2.0;
  double grace_s = 1.5;        // an id missing from the items keeps its place this long (the range's edge, a list
                               // rebuilt without it for a moment) before it is forgotten and counts as new again
  double gap_s = 0.04;         // the breath between one sound's end and the next one's start
  double period_for(float dist) const;   // a power law through the two endpoints
};

class SonarField {
 public:
  // len_s: how long this thing's sound lasts as it will be played. first: among those equally due, it goes first
  // (the selected point).
  struct Item { unsigned id; float dist; int kind; float len_s = 0.2f; bool first = false; };
  struct Ping { unsigned id; int kind; };
  // Advance to now_s over the current item set. Returns the thing whose sound starts now: at most one, and none
  // while the last one is still sounding.
  // The ids there at the first update after a reset are each seeded a fraction of their period in (so the field does
  // not open on the nearest six in a row); an id that arrives later is due at once and goes ahead of the rest. An id
  // absent from `items` keeps its place for grace_s, after which it is forgotten.
  std::vector<Ping> update(const std::vector<Item>& items, double now_s);
  void reset();
  FieldParams& params() { return params_; }
  const FieldParams& params() const { return params_; }
  std::size_t tracked() const { return slots_.size(); }

 private:
  FieldParams params_;
  // want: when it next wants to sound (its last start plus its period now, or its seed); arrived: it came into a
  // running field and has not sounded yet.
  struct Slot { double want, last_start, last_seen; bool played, arrived; };
  std::unordered_map<unsigned, Slot> slots_;
  double free_at_ = 0;     // when the line is free again
  bool started_ = false;   // updated since the last reset: a new id is an arrival, not part of the opening set
};

}  // namespace p3a::core

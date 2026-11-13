// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#pragma once
// The sonar as a field of per-thing metronomes. Every nearby thing repeats its OWN sound on its own clock; the period
// shrinks as it nears the player -- a power law in distance, in metres whatever the sonar's range is, so a given rate
// always means the same distance and halving the distance speeds it up by the same factor everywhere. Things of
// different kinds sound independently, over one another if their moments meet (each kind is its own sound and comes
// from its own place). Only two of one kind are kept apart: a sound does not start within `collide_s` of another of
// its kind, and of those of one kind due together the most overdue against its own period goes first. That is Grim
// Dark's field (2026-11-12, the user's call): from 2026-11-11 only one sound played at a time, and every thing waited
// for the line, so in a crowd the rhythms fell into a queue that sounded like a slow sweep.
// Engine-free: the host hands (id, distance, kind) each frame and plays what `update` reports.
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace p3a::core {

struct FieldParams {
  // 0.30 s at 1.5 m to 1.00 s at the range's edge at first: three to one, and stretched with the range setting, so
  // being close to a thing was not heard (the user, 2026-11-11). Then 0.12 s at 1 m to 1.5 at 20 m; a quarter slower
  // since (the user, 2026-11-12, with every thing on its own clock): 0.15 s at 1 m, 0.27 at 2 m, 0.38 at 3 m, 0.59 at
  // 5 m, 1.06 at 10 m, 1.9 at 20 m and beyond.
  double period_near = 0.15;   // seconds between pulses at dist_near and closer
  double period_far = 1.90;    // seconds at dist_far and beyond
  float dist_near = 1.0f;      // metres
  float dist_far = 20.0f;      // metres
  double collide_s = 0.06;     // two sounds of one kind never start closer together than this (Grim Dark's 0.04, a
                               // little more for our longer sounds)
  double grace_s = 1.5;        // an id missing from the items keeps its place this long (the range's edge, a list
                               // rebuilt without it for a moment) before it is forgotten and counts as new again
  double gap_s = 0.04;         // the breath a sound needs after it before it repeats (what core/sonar_cues' brief
                               // forms were made to fit; the sonar plays whole sounds, over each other up close)
  double period_for(float dist) const;   // a power law through the two endpoints
};

class SonarField {
 public:
  struct Item { unsigned id; float dist; int kind; };
  struct Ping { unsigned id; int kind; };
  // Advance to now_s over the current item set. Returns the things whose sound starts now (0 or more, at most one of
  // each kind).
  // The ids there at the first update after a reset are each seeded a fraction of their period in (so the field does
  // not open on all of them at once); an id that arrives later is due at once. An id absent from `items` keeps its
  // place for grace_s, after which it is forgotten.
  std::vector<Ping> update(const std::vector<Item>& items, double now_s);
  void reset();
  FieldParams& params() { return params_; }
  const FieldParams& params() const { return params_; }
  std::size_t tracked() const { return slots_.size(); }

 private:
  FieldParams params_;
  // want: when it next wants to sound (its last start plus its period now, or its seed).
  struct Slot { double want, last_start, last_seen; int kind; bool played; };
  std::unordered_map<unsigned, Slot> slots_;
  bool started_ = false;   // updated since the last reset: a new id is an arrival, not part of the opening set
};

}  // namespace p3a::core

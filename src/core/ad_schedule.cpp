#include "core/ad_schedule.h"

namespace p3a::core::ad {

const char* state_name(State s) {
  switch (s) {
    case State::Pending: return "pending";
    case State::Playing: return "playing";
    case State::Done: return "done";
    case State::Cut: return "cut";
    case State::Missed: return "missed";
    case State::Skipped: return "skipped";
  }
  return "?";
}

void Schedule::load(std::vector<Cue> cues) {
  cues_ = std::move(cues);
  reset();
}

void Schedule::reset() {
  state_.assign(cues_.size(), State::Pending);
  reason_.assign(cues_.size(), "");
  cur_ = -1;
  paused_ = false;
  started_ = false;
  last_ = 0;
}

void Schedule::cut(std::vector<Action>& out, const char* why) {
  if (cur_ < 0) return;
  out.push_back({Act::Stop, cur_});
  state_[cur_] = State::Cut;
  reason_[cur_] = why;
  cur_ = -1;
  paused_ = false;
}

// After a seek (or joining a movie already under way): cues already behind the clock are skipped, cues ahead of it
// are armed again (a rewind replays them).
void Schedule::resync(double t) {
  for (size_t i = 0; i < cues_.size(); ++i) {
    if (cues_[i].at + kLate < t) {
      if (state_[i] == State::Pending) { state_[i] = State::Skipped; reason_[i] = "behind the clock"; }
    } else if (cues_[i].at >= t && state_[i] != State::Playing) {
      state_[i] = State::Pending;
      reason_[i].clear();
    }
  }
}

std::vector<Action> Schedule::step(const Clock& c, const Voice& v) {
  std::vector<Action> out;
  if (!c.playing) {   // the movie ended, was skipped, or is not up yet
    if (cur_ >= 0 && v.ended) { state_[cur_] = State::Done; cur_ = -1; }
    // a description in its last second finishes (a movie that ends a moment early, a skip at the very end)
    if (cur_ >= 0 && v.active && start_ + cues_[cur_].dur - last_ <= kFinish) { if (paused_) { out.push_back({Act::Resume, cur_}); paused_ = false; } return out; }
    cut(out, "the movie stopped");
    return out;
  }
  if (!started_) {
    started_ = true;
    last_ = c.time;
    resync(c.time);
  } else if (c.time < last_ - kBack || c.time > last_ + kJump) {
    cut(out, "the movie jumped");
    resync(c.time);
  }
  last_ = c.time;

  if (cur_ >= 0) {
    if (v.ended) { state_[cur_] = State::Done; cur_ = -1; paused_ = false; }
    else if (!v.active) { state_[cur_] = State::Cut; reason_[cur_] = "the narrator stopped"; cur_ = -1; paused_ = false; }
  }
  if (cur_ >= 0) {
    if (c.paused && !paused_) { out.push_back({Act::Pause, cur_}); paused_ = true; }
    else if (!c.paused && paused_) { out.push_back({Act::Resume, cur_}); paused_ = false; }
  }
  if (c.paused) return out;

  for (size_t i = 0; i < cues_.size(); ++i) {
    if (state_[i] != State::Pending || cues_[i].at > c.time) continue;
    if (c.time - cues_[i].at > kLate) { state_[i] = State::Missed; reason_[i] = cur_ >= 0 ? "the previous cue still played" : "late"; continue; }
    if (cur_ >= 0) break;   // its moment has come but the narrator is busy: wait (up to kLate)
    out.push_back({Act::Play, (int)i});
    state_[i] = State::Playing;
    cur_ = (int)i;
    start_ = c.time;
    break;
  }
  return out;
}

}  // namespace p3a::core::ad

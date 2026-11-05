#pragma once
// Audio description scheduling, engine-free (unit-tested in tests/ad_schedule_tests.cpp, and run over every movie's
// real cue list by the ad_sim tool). The caller feeds it the movie clock and the narrator's state each frame and
// carries out the actions it returns; it decides which cue plays when, follows pauses, seeks and skips, and keeps
// a record of how every cue ended, so a test can tell a cue that played in full from one cut short.
#include <string>
#include <vector>

namespace p3a::core::ad {

struct Cue { double at = 0; double dur = 0; };

// The movie clock, read each frame: the movie is playing at all, is paused, and its time in seconds.
struct Clock { bool playing = false; bool paused = false; double time = 0; };
// The narrator, as the mixer reports it: a clip is sounding (or held by a pause), or it reached its end.
struct Voice { bool active = false; bool ended = false; };

enum class Act { Play, Pause, Resume, Stop };
struct Action { Act act; int cue; };

// How a cue's turn went. Pending: not yet; Playing; Done: played to its end; Cut: stopped before its end (the
// reason says why); Missed: its moment passed while another cue still played or the narrator was busy; Skipped:
// the movie was already past it (joined late, or a seek jumped over it).
enum class State { Pending, Playing, Done, Cut, Missed, Skipped };
const char* state_name(State s);

class Schedule {
 public:
  // A cue may start this late after its time (a frame or two, a short hitch) and still play; later is Missed.
  static constexpr double kLate = 0.5;
  // A clock step larger than this forward, or any step back past kBack, is a seek (or a skip to the end).
  static constexpr double kJump = 1.5;
  static constexpr double kBack = 0.25;
  // When the movie stops, a description with no more than this left plays out instead of being cut.
  static constexpr double kFinish = 1.0;

  void load(std::vector<Cue> cues);   // a new movie: every cue Pending
  void reset();                       // the same movie from the start again
  std::vector<Action> step(const Clock& c, const Voice& v);

  int playing() const { return cur_; }
  State state(int i) const { return state_[i]; }
  const std::string& reason(int i) const { return reason_[i]; }
  size_t size() const { return cues_.size(); }
  const Cue& cue(int i) const { return cues_[i]; }

 private:
  void cut(std::vector<Action>& out, const char* why);
  void resync(double t);
  std::vector<Cue> cues_;
  std::vector<State> state_;
  std::vector<std::string> reason_;
  int cur_ = -1;
  bool paused_ = false;
  bool started_ = false;
  double last_ = 0;
  double start_ = 0;   // movie time the playing cue started
};

}  // namespace p3a::core::ad

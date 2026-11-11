#include "doctest.h"
#include "core/ad_sim.h"

using namespace p3a::core::ad;

static std::vector<Cue> three() { return {{1.0, 2.0}, {4.0, 1.5}, {8.0, 3.0}}; }

TEST_CASE("ad: a clean run plays every cue in full, on time") {
  Scenario sc; sc.length = 12;
  SimResult r = simulate(three(), sc);
  CHECK(r.count(State::Done) == 3);
  for (size_t i = 0; i < 3; ++i) {
    CHECK(r.played[i] == doctest::Approx(three()[i].dur));
    CHECK(r.started[i] - three()[i].at < 0.05);
  }
}

TEST_CASE("ad: a pause holds the narrator and it still finishes") {
  Scenario sc; sc.length = 12; sc.pauses = {{1.8, 3.0}, {8.5, 0.5}};
  SimResult r = simulate(three(), sc);
  CHECK(r.count(State::Done) == 3);
  CHECK(r.played[0] == doctest::Approx(2.0));
}

TEST_CASE("ad: a short hitch does not lose a cue") {
  Scenario sc; sc.length = 12; sc.hitches = {{3.9, 0.3}};
  SimResult r = simulate(three(), sc);
  CHECK(r.count(State::Done) == 3);
}

TEST_CASE("ad: skipping the movie cuts the cue that is playing and skips nothing else silently") {
  Scenario sc; sc.length = 12; sc.stop_at = 4.2;   // cue 1 (4.0, 1.5 s long) has 1.3 s left
  SimResult r = simulate(three(), sc);
  CHECK(r.states[0] == State::Done);
  CHECK(r.states[1] == State::Cut);
  CHECK(r.reasons[1] == "the movie stopped");
  CHECK(r.states[2] == State::Pending);
}

TEST_CASE("ad: a jump forward skips the cues behind it, a rewind plays them again") {
  Scenario fwd; fwd.length = 12; fwd.seek_at = 1.5; fwd.seek_to = 7.0;
  SimResult r = simulate(three(), fwd);
  CHECK(r.states[0] == State::Cut);
  CHECK(r.states[1] == State::Skipped);
  CHECK(r.states[2] == State::Done);
  Scenario back; back.length = 12; back.seek_at = 9.0; back.seek_to = 3.0;
  r = simulate(three(), back);
  CHECK(r.states[1] == State::Done);
  CHECK(r.states[2] == State::Done);   // played again after the rewind
}

TEST_CASE("ad: joining a movie late skips what is already past") {
  Schedule s; s.load(three());
  auto a = s.step({true, false, 5.0}, {});
  CHECK(s.state(0) == State::Skipped);
  CHECK(s.state(1) == State::Skipped);   // 4.0 + kLate < 5.0
  CHECK(a.empty());
}

TEST_CASE("ad: overlapping cues are reported, not hidden") {
  SimResult r = simulate({{1.0, 3.0}, {2.0, 1.0}}, Scenario{.length = 6});
  CHECK(r.states[0] == State::Done);
  CHECK(r.states[1] == State::Missed);
}

TEST_CASE("ad: a seek while a cue plays stops the narrator") {
  Schedule s; s.load(three());
  auto a = s.step({true, false, 1.0}, {});
  REQUIRE(a.size() == 1);
  CHECK(a[0].act == Act::Play);
  CHECK(a[0].cue == 0);
  CHECK(s.playing() == 0);
  a = s.step({true, false, 7.0}, {true, false});   // the clock jumped over cue 1
  REQUIRE(a.size() == 1);
  CHECK(a[0].act == Act::Stop);
  CHECK(a[0].cue == 0);
  CHECK(s.playing() == -1);
  CHECK(s.state(0) == State::Cut);
  CHECK(s.reason(0) == "the movie jumped");
  CHECK(s.state(1) == State::Skipped);
  CHECK(s.state(2) == State::Pending);
}

TEST_CASE("ad: a narrator that stops by itself leaves the cue cut, and the next one still plays") {
  Schedule s; s.load(three());
  s.step({true, false, 1.0}, {});
  auto a = s.step({true, false, 1.1}, {false, false});   // no clip sounding, and none reached its end
  CHECK(a.empty());
  CHECK(s.state(0) == State::Cut);
  CHECK(s.reason(0) == "the narrator stopped");
  a = s.step({true, false, 4.0}, {});
  REQUIRE(a.size() == 1);
  CHECK(a[0].act == Act::Play);
  CHECK(a[0].cue == 1);
}

TEST_CASE("ad: a movie that ends during a pause lets a nearly finished description play out") {
  Schedule s; s.load({{1.0, 2.0}});
  s.step({true, false, 1.0}, {});
  s.step({true, false, 2.0}, {true, false});
  auto a = s.step({true, true, 2.6}, {true, false});      // the player pauses: 0.4 s of it left
  REQUIRE(a.size() == 1);
  CHECK(a[0].act == Act::Pause);
  a = s.step({false, false, 2.6}, {true, false});         // the movie is gone while paused
  REQUIRE(a.size() == 1);
  CHECK(a[0].act == Act::Resume);
  CHECK(s.state(0) == State::Playing);
  a = s.step({false, false, 2.6}, {false, true});         // it reached its end
  CHECK(a.empty());
  CHECK(s.state(0) == State::Done);
  CHECK(s.playing() == -1);
}

TEST_CASE("ad: reset arms every cue again") {
  Schedule s; s.load(three());
  s.step({true, false, 9.0}, {});                          // joined late: the first two are behind the clock
  CHECK(s.state(0) == State::Skipped);
  s.reset();
  CHECK(s.size() == 3);
  CHECK(s.playing() == -1);
  for (int i = 0; i < 3; ++i) { CHECK(s.state(i) == State::Pending); CHECK(s.reason(i).empty()); }
  auto a = s.step({true, false, 1.0}, {});
  REQUIRE(a.size() == 1);
  CHECK(a[0].cue == 0);
}

TEST_CASE("ad: every state has a name for the log") {
  CHECK(std::string(state_name(State::Pending)) == "pending");
  CHECK(std::string(state_name(State::Playing)) == "playing");
  CHECK(std::string(state_name(State::Done)) == "done");
  CHECK(std::string(state_name(State::Cut)) == "cut");
  CHECK(std::string(state_name(State::Missed)) == "missed");
  CHECK(std::string(state_name(State::Skipped)) == "skipped");
}

TEST_CASE("ad: a description in its last second outlives the movie's end") {
  Scenario sc; sc.length = 2.6;   // cue 0 runs 1.0-3.0: 0.4 s left when the movie ends
  SimResult r = simulate({{1.0, 2.0}}, sc);
  CHECK(r.states[0] == State::Done);
  CHECK(r.played[0] == doctest::Approx(2.0));
}

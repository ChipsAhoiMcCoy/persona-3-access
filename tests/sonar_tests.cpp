#include "doctest.h"
#include "core/sonar_cues.h"
#include "core/sonar_field.h"
#include <cmath>
#include <map>

using namespace p3a::core;

namespace {
struct Fired { unsigned id; double at; };
// Runs the field for `seconds` in 10 ms frames over fixed items; returns every sound's start in order.
std::vector<Fired> run(SonarField& f, const std::vector<SonarField::Item>& items, double seconds, double from = 0.0) {
  std::vector<Fired> fired;
  for (double t = from; t < from + seconds; t += 0.01)
    for (auto& p : f.update(items, t)) fired.push_back({p.id, t});
  return fired;
}
std::map<unsigned, int> counts(const std::vector<Fired>& fired) {
  std::map<unsigned, int> n;
  for (auto& x : fired) ++n[x.id];
  return n;
}
}  // namespace

TEST_CASE("sonar field: the period shrinks with distance, logarithmically") {
  FieldParams p;
  CHECK(p.period_for(0.2f) == doctest::Approx(p.period_near));
  CHECK(p.period_for(p.dist_near) == doctest::Approx(p.period_near));
  CHECK(p.period_for(p.dist_far) == doctest::Approx(p.period_far));
  CHECK(p.period_for(100.0f) == doctest::Approx(p.period_far));
  // A step up close changes the rate more than the same step far off.
  double close = p.period_for(3.0f) - p.period_for(2.0f), far_off = p.period_for(9.0f) - p.period_for(8.0f);
  CHECK(close > far_off * 2);
}

TEST_CASE("sonar field: a thing alone keeps its own rate") {
  SonarField a, b;
  CHECK(counts(run(a, {{1, 1.0f, 0, 0.2f}}, 10.0))[1] >= 31);   // every 0.30 s
  CHECK(counts(run(a, {{1, 1.0f, 0, 0.2f}}, 10.0, 10.0))[1] <= 34);
  int far_n = counts(run(b, {{2, 10.0f, 0, 0.2f}}, 10.0))[2];
  CHECK(far_n >= 9);    // every 1.00 s
  CHECK(far_n <= 11);
}

TEST_CASE("sonar field: no sound starts before the one before it is over") {
  SonarField f;
  // Six things beside the player, each wanting three turns a second, sounds of 0.2 s.
  std::vector<SonarField::Item> items;
  for (unsigned i = 1; i <= 6; ++i) items.push_back({i, 1.0f, (int)(i % 3), 0.2f});
  auto fired = run(f, items, 20.0);
  REQUIRE(fired.size() > 20);
  for (size_t i = 1; i < fired.size(); ++i) CHECK(fired[i].at - fired[i - 1].at >= 0.2 + f.params().gap_s - 0.011);
  // The line is kept moving: about four sounds a second.
  CHECK(fired.size() >= 75);
  // And every one of them gets its turn, evenly at one distance.
  auto n = counts(fired);
  CHECK(n.size() == 6);
  for (auto& [id, c] : n) { CAPTURE(id); CHECK(c >= 12); CHECK(c <= 15); }
}

TEST_CASE("sonar field: in a crowd the near thing gets more turns than the far one, and none is starved") {
  SonarField f;
  std::vector<SonarField::Item> items = {{1, 1.0f, 0, 0.2f}, {2, 1.0f, 0, 0.2f}, {3, 9.0f, 0, 0.2f}, {4, 9.0f, 0, 0.2f},
                                         {5, 9.0f, 0, 0.2f}, {6, 9.0f, 0, 0.2f}};
  auto n = counts(run(f, items, 30.0));
  CHECK(n.size() == 6);
  CHECK(n[1] > n[3] * 2);
  CHECK(n[2] > n[4] * 2);
  CHECK(n[6] >= 10);   // the far ones still sound, a few seconds apart at worst
}

TEST_CASE("sonar field: a thing that arrives in a running field goes next") {
  SonarField f;
  std::vector<SonarField::Item> items;
  for (unsigned i = 1; i <= 5; ++i) items.push_back({i, 1.0f, 0, 0.2f});
  run(f, items, 5.0);
  items.push_back({9, 9.0f, 1, 0.2f});
  auto fired = run(f, items, 1.0, 5.0);
  REQUIRE(!fired.empty());
  CHECK(fired[0].id == 9);        // ahead of the five that are all overdue
  CHECK(fired[0].at < 5.0 + 0.26);   // as soon as the sound in progress is over
  // An empty field that has been running counts too: the first thing on a quiet floor sounds at once.
  SonarField g;
  g.update({}, 0.0);
  CHECK(g.update({{3, 10.0f, 0, 0.2f}}, 1.0).size() == 1);
}

TEST_CASE("sonar field: a field starting up staggers what is already there") {
  SonarField f;
  CHECK(f.update({{7, 10.0f, 0, 0.2f}}, 0.0).empty());
  int n = 0;
  for (double t = 0.01; t <= f.params().period_far + 0.01; t += 0.01) n += (int)f.update({{7, 10.0f, 0, 0.2f}}, t).size();
  CHECK(n == 1);
}

TEST_CASE("sonar field: among things equally due the host's first goes first") {
  SonarField f;
  f.update({}, 0.0);
  // Two arrivals in one frame: the host's first, though farther.
  auto p = f.update({{1, 2.0f, 0, 0.2f, false}, {2, 8.0f, 0, 0.2f, true}}, 1.0);
  REQUIRE(p.size() == 1);
  CHECK(p[0].id == 2);
}

TEST_CASE("sonar field: a thing that comes closer does not wait out its old, slow period") {
  SonarField f;
  f.update({}, 0.0);
  REQUIRE(f.update({{9, 10.0f, 0, 0.1f}}, 1.0).size() == 1);   // sounded far off: next in a second
  int n = 0;
  for (double t = 1.01; t <= 1.0 + f.params().period_near + 0.02; t += 0.01) n += (int)f.update({{9, 1.0f, 0, 0.1f}}, t).size();
  CHECK(n == 1);
}

TEST_CASE("sonar field: a thing gone for a moment keeps its place, gone for long is forgotten") {
  SonarField f;
  run(f, {{5, 10.0f, 0, 0.2f}}, 3.0);
  f.update({}, 3.0);
  CHECK(f.tracked() == 1);   // within the grace
  f.update({}, 3.0 + f.params().grace_s + 0.1);
  CHECK(f.tracked() == 0);
  f.reset();
  CHECK(f.tracked() == 0);
}

TEST_CASE("sonar cues: short, clean at both ends, one loudness") {
  const int rate = 48000;
  for (int k = 0; k < (int)SonarCue::Count; ++k) {
    CAPTURE(k);
    std::vector<float> v = sonar_cue((SonarCue)k, rate);
    REQUIRE(v.size() > (size_t)rate / 20);    // over 50 ms
    CHECK(v.size() < (size_t)rate * 3 / 10);  // under 0.3 s
    double sum = 0, peak = 0;
    bool finite = true;
    for (float s : v) { finite = finite && std::isfinite(s); sum += (double)s * s; peak = std::max(peak, (double)std::fabs(s)); }
    CHECK(finite);
    CHECK(peak <= 0.9001);
    CHECK(std::fabs(v.front()) < 0.01);
    CHECK(std::fabs(v.back()) < 0.01);
    double rms = std::sqrt(sum / (double)v.size());
    CHECK(rms > kSonarCueRms * 0.99);  // none is held under the common loudness by its peak
    CHECK(rms < kSonarCueRms * 1.01);
    // The same sound every time.
    CHECK(sonar_cue((SonarCue)k, rate) == v);
  }
}

TEST_CASE("signal cues: short, clean at both ends, the sonar's loudness") {
  const int rate = 48000;
  for (int k = 0; k < (int)SignalCue::Count; ++k) {
    CAPTURE(k);
    std::vector<float> v = signal_cue((SignalCue)k, rate);
    REQUIRE(v.size() > (size_t)rate / 50);    // over 20 ms
    CHECK(v.size() < (size_t)rate * 3 / 10);  // under 0.3 s
    double sum = 0, peak = 0;
    bool finite = true;
    for (float s : v) { finite = finite && std::isfinite(s); sum += (double)s * s; peak = std::max(peak, (double)std::fabs(s)); }
    CHECK(finite);
    CHECK(peak <= 0.9001);
    CHECK(std::fabs(v.front()) < 0.01);
    CHECK(std::fabs(v.back()) < 0.01);
    double rms = std::sqrt(sum / (double)v.size());
    CHECK(rms > kSonarCueRms * 0.8);   // the blade's ring is all attack: its peak holds it a little under
    CHECK(rms < kSonarCueRms * 1.01);
    CHECK(signal_cue((SignalCue)k, rate) == v);
  }
}

// The share of a sound's energy above `hz` (a two-pole high-pass, then the energies compared).
static double share_above(const std::vector<float>& v, double hz, int rate) {
  const double a = std::exp(-2.0 * 3.14159265358979 * hz / rate);
  double lp1 = 0, lp2 = 0, all = 0, high = 0;
  for (float s : v) {
    lp1 = a * lp1 + (1 - a) * s;
    lp2 = a * lp2 + (1 - a) * lp1;
    double h = s - lp2;
    all += (double)s * s; high += h * h;
  }
  return all > 0 ? high / all : 0;
}

TEST_CASE("signal cues: the Shadow's growl is carried where ears and small headphones hear it") {
  const int rate = 48000;
  CHECK(share_above(signal_cue(SignalCue::Shadow, rate), 400.0, rate) > 0.5);
}

TEST_CASE("sonar cues: each kind is a different sound") {
  const int rate = 48000;
  for (int a = 0; a < (int)SonarCue::Count; ++a)
    for (int b = a + 1; b < (int)SonarCue::Count; ++b)
      CHECK(sonar_cue((SonarCue)a, rate) != sonar_cue((SonarCue)b, rate));
}

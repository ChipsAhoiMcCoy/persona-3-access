#include "doctest.h"
#include "core/sonar_cues.h"
#include "core/sonar_field.h"
#include <cmath>
#include <map>

using namespace p3a::core;

namespace {
// Runs the field for `seconds` in 10 ms frames over fixed items; returns how often each id pulsed.
std::map<unsigned, int> run(SonarField& f, const std::vector<SonarField::Item>& items, double seconds, double from = 0.0) {
  std::map<unsigned, int> fired;
  for (double t = from; t < from + seconds; t += 0.01)
    for (auto& p : f.update(items, t)) ++fired[p.id];
  return fired;
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

TEST_CASE("sonar field: a near thing pulses more often than a far one") {
  SonarField f;
  auto fired = run(f, {{1, 1.0f, 0}, {2, 10.0f, 0}}, 10.0);
  CHECK(fired[1] >= 30);   // every 0.30 s
  CHECK(fired[1] <= 34);
  CHECK(fired[2] >= 9);    // every 1.00 s
  CHECK(fired[2] <= 11);
}

TEST_CASE("sonar field: a thing that arrives in a running field pulses at once") {
  SonarField f;
  run(f, {{1, 5.0f, 0}}, 2.0);
  for (auto& p : f.update({{1, 5.0f, 0}, {2, 10.0f, 1}}, 2.0)) CHECK(p.id != 2);   // seen this frame: due now, sounds next
  bool fired = false;
  for (auto& p : f.update({{1, 5.0f, 0}, {2, 10.0f, 1}}, 2.01)) fired = fired || p.id == 2;
  CHECK(fired);
  // An empty field that has been running counts too: the first thing on a quiet floor.
  SonarField g;
  g.update({}, 0.0);
  g.update({{3, 10.0f, 0}}, 1.0);
  CHECK(g.update({{3, 10.0f, 0}}, 1.01).size() == 1);
}

TEST_CASE("sonar field: a new thing is seeded, not fired, and pulses within one period") {
  SonarField f;
  CHECK(f.update({{7, 10.0f, 0}}, 0.0).empty());
  int n = 0;
  for (double t = 0.01; t <= f.params().period_far + 0.01; t += 0.01) n += (int)f.update({{7, 10.0f, 0}}, t).size();
  CHECK(n == 1);
}

TEST_CASE("sonar field: two things of one kind at one distance never pulse together") {
  SonarField f;
  std::vector<SonarField::Item> items = {{1, 4.0f, 0}, {2, 4.0f, 0}, {3, 4.0f, 0}, {4, 4.0f, 0}};
  int together = 0;
  for (double t = 0; t < 20.0; t += 0.01)
    if (f.update(items, t).size() > 1) ++together;
  CHECK(together == 0);
}

TEST_CASE("sonar field: a thing that comes closer does not wait out its old, slow period") {
  SonarField f;
  // Seeded far away, then at once beside the player: the next pulse comes within the near period.
  f.update({{9, 10.0f, 0}}, 0.0);
  int n = 0;
  for (double t = 0.01; t <= f.params().period_near + 0.02; t += 0.01) n += (int)f.update({{9, 1.0f, 0}}, t).size();
  CHECK(n >= 1);
}

TEST_CASE("sonar field: a thing gone for a moment keeps its rhythm, gone for long is forgotten") {
  SonarField f;
  run(f, {{5, 10.0f, 0}}, 3.0);
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
    // The marked one is the same sound after a tick.
    std::vector<float> m = sonar_cue_marked((SonarCue)k, rate);
    REQUIRE(m.size() > v.size());
    CHECK(std::equal(v.begin(), v.end(), m.end() - (ptrdiff_t)v.size()));
  }
}

TEST_CASE("signal cues: short, clean at both ends, the sonar's loudness") {
  const int rate = 48000;
  for (int k = 0; k < (int)SignalCue::Count; ++k) {
    CAPTURE(k);
    std::vector<float> v = signal_cue((SignalCue)k, rate);
    REQUIRE(v.size() > (size_t)rate / 50);    // over 20 ms
    CHECK(v.size() < (size_t)rate / 5);       // under 0.2 s
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
  std::vector<float> growl = signal_cue(SignalCue::Shadow, rate);
  double share = share_above(growl, 400.0, rate);
  MESSAGE("growl: share of energy above 400 Hz " << share);
  CHECK(share > 0.5);
}

TEST_CASE("sonar cues: each kind is a different sound") {
  const int rate = 48000;
  for (int a = 0; a < (int)SonarCue::Count; ++a)
    for (int b = a + 1; b < (int)SonarCue::Count; ++b)
      CHECK(sonar_cue((SonarCue)a, rate) != sonar_cue((SonarCue)b, rate));
}

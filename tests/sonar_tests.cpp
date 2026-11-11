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

TEST_CASE("sonar field: the period shrinks with distance, by a power law") {
  FieldParams p;
  CHECK(p.period_for(0.2f) == doctest::Approx(p.period_near));
  CHECK(p.period_for(p.dist_near) == doctest::Approx(p.period_near));
  CHECK(p.period_for(p.dist_far) == doctest::Approx(p.period_far));
  CHECK(p.period_for(100.0f) == doctest::Approx(p.period_far));
  // Halving the distance speeds it up by the same factor wherever it happens.
  CHECK(p.period_for(8.0f) / p.period_for(4.0f) == doctest::Approx(p.period_for(4.0f) / p.period_for(2.0f)));
  CHECK(p.period_for(4.0f) / p.period_for(2.0f) > 1.7);
  // So a step up close changes the rate far more than the same step far off.
  double close = 1 / p.period_for(2.0f) - 1 / p.period_for(3.0f), far_off = 1 / p.period_for(8.0f) - 1 / p.period_for(9.0f);
  CHECK(close > far_off * 5);
  // Much faster beside a thing than a room away.
  CHECK(p.period_for(10.0f) > p.period_for(1.0f) * 6);
}

TEST_CASE("sonar field: a thing alone keeps its own rate") {
  SonarField a, b;
  CHECK(counts(run(a, {{1, 1.0f, 0, 0.065f}}, 10.0))[1] >= 70);   // every 0.12 s (0.13 where a 10 ms frame falls late)
  CHECK(counts(run(a, {{1, 1.0f, 0, 0.065f}}, 10.0, 10.0))[1] <= 85);
  int far_n = counts(run(b, {{2, 20.0f, 0, 0.16f}}, 10.0))[2];
  CHECK(far_n >= 6);    // every 1.50 s
  CHECK(far_n <= 7);
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
  // Two beside the player (brief sounds), four across the room (whole ones).
  std::vector<SonarField::Item> items = {{1, 1.0f, 0, 0.065f}, {2, 1.0f, 0, 0.065f}, {3, 9.0f, 0, 0.16f}, {4, 9.0f, 0, 0.16f},
                                         {5, 9.0f, 0, 0.16f}, {6, 9.0f, 0, 0.16f}};
  auto n = counts(run(f, items, 30.0));
  CHECK(n.size() == 6);
  CHECK(n[1] > n[3] * 10);
  CHECK(n[2] > n[4] * 10);
  CHECK(n[1] >= 90);   // three times a second and more, each: near is still heard as near
  CHECK(n[6] >= 3);    // the far ones still sound, though seconds apart
}

TEST_CASE("sonar field: a near thing keeps most of its rhythm in a full room") {
  SonarField f;
  // One at arm's length, five spread over the room: it wants eight turns a second and still gets over three (it
  // got under one when the line was shared in proportion and the sounds were a quarter of a second).
  std::vector<SonarField::Item> items = {{1, 1.0f, 0, 0.065f}, {2, 3.0f, 0, 0.16f}, {3, 5.0f, 0, 0.16f}, {4, 6.0f, 0, 0.16f},
                                         {5, 8.0f, 0, 0.16f}, {6, 10.0f, 0, 0.16f}};
  auto n = counts(run(f, items, 30.0));
  CHECK(n.size() == 6);
  CHECK(n[1] >= 90);   // 3 a second
  CHECK(n[2] > n[3]);
  CHECK(n[3] > n[6]);
  CHECK(n[6] >= 3);
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
  CHECK(f.update({{7, 20.0f, 0, 0.2f}}, 0.0).empty());
  int n = 0;
  for (double t = 0.01; t <= f.params().period_far + 0.01; t += 0.01) n += (int)f.update({{7, 20.0f, 0, 0.2f}}, t).size();
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
  REQUIRE(f.update({{9, 10.0f, 0, 0.1f}}, 1.0).size() == 1);   // sounded far off: next in most of a second
  int n = 0;
  // Beside the player now: again as soon as its sound and the breath (0.14 s) are over.
  for (double t = 1.01; t <= 1.20; t += 0.01) n += (int)f.update({{9, 1.0f, 0, 0.1f}}, t).size();
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
    CHECK(v.size() < (size_t)rate * 2 / 10);  // under 0.2 s
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

TEST_CASE("sonar cues: the brief forms fit the fastest rhythm") {
  const int rate = 48000;
  FieldParams p;
  for (int k = 0; k < (int)SonarCue::Count; ++k) {
    CAPTURE(k);
    std::vector<float> v = sonar_cue((SonarCue)k, rate, true);
    double len = (double)v.size() / rate;
    CHECK(len > 0.04);
    CHECK(len / 0.84 + p.gap_s <= p.period_near);   // played lower from behind too
    CHECK(v != sonar_cue((SonarCue)k, rate));
    double sum = 0, peak = 0;
    for (float s : v) { sum += (double)s * s; peak = std::max(peak, (double)std::fabs(s)); }
    CHECK(peak <= 0.9001);
    CHECK(std::fabs(v.front()) < 0.01);
    CHECK(std::fabs(v.back()) < 0.01);
    double rms = std::sqrt(sum / (double)v.size());
    CHECK(rms > kSonarCueRms * 0.99);
    CHECK(rms < kSonarCueRms * 1.01);
    for (int b = k + 1; b < (int)SonarCue::Count; ++b) CHECK(sonar_cue((SonarCue)b, rate, true) != v);
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

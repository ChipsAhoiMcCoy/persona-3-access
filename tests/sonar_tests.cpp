#include "doctest.h"
#include "core/sonar_cues.h"
#include "core/sonar_field.h"
#include <algorithm>
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
  CHECK(counts(run(a, {{1, 1.0f, 0}}, 10.0))[1] >= 60);   // every 0.15 s (0.16 where a 10 ms frame falls late)
  CHECK(counts(run(a, {{1, 1.0f, 0}}, 10.0, 10.0))[1] <= 68);
  int far_n = counts(run(b, {{2, 20.0f, 0}}, 10.0))[2];
  CHECK(far_n >= 5);    // every 1.90 s
  CHECK(far_n <= 6);
}

TEST_CASE("sonar field: things of different kinds keep their own rhythms in a crowd") {
  SonarField f, alone;
  // One at arm's length and five spread over the room, each of its own kind: none waits on another, so the near
  // one keeps the rate it has alone (it got a third of it when only one sound could play at a time).
  std::vector<SonarField::Item> items = {{1, 1.0f, 0}, {2, 3.0f, 1}, {3, 5.0f, 2}, {4, 6.0f, 3}, {5, 8.0f, 4}, {6, 10.0f, 5}};
  auto n = counts(run(f, items, 30.0));
  int solo = counts(run(alone, {{1, 1.0f, 0}}, 30.0))[1];
  CHECK(n[1] >= solo - 1);
  for (auto& it : items) {
    CAPTURE(it.id);
    double want = 30.0 / f.params().period_for(it.dist);
    CHECK(n[it.id] >= want * 0.85);
    CHECK(n[it.id] <= want + 2);
  }
}

TEST_CASE("sonar field: two of one kind never start together") {
  SonarField f;
  // Three of a kind beside the player and three of another a step away: same-kind starts stay collide_s apart,
  // different kinds may meet.
  std::vector<SonarField::Item> items = {{1, 1.0f, 2}, {2, 1.2f, 2}, {3, 1.5f, 2}, {4, 2.0f, 3}, {5, 2.0f, 3}, {6, 2.5f, 3}};
  std::vector<std::pair<int, double>> starts;
  for (double t = 0; t < 20.0; t += 0.01)
    for (auto& p : f.update(items, t)) starts.push_back({p.kind, t});
  std::map<int, double> last;
  bool met = false;
  double prev = -1;
  for (auto& [kind, at] : starts) {
    if (last.count(kind)) { CAPTURE(at); CHECK(at - last[kind] >= f.params().collide_s - 1e-9); }
    last[kind] = at;
    met |= at == prev;
    prev = at;
  }
  CHECK(met);   // the two kinds did sound in the same frame at times
  // And each of the three of a kind still sounds often, close to evenly.
  auto n = counts(run(f, items, 20.0, 20.0));
  for (unsigned id = 1; id <= 3; ++id) { CAPTURE(id); CHECK(n[id] >= 60); }
}

TEST_CASE("sonar field: a thing that arrives in a running field sounds at once") {
  SonarField f;
  std::vector<SonarField::Item> items;
  for (unsigned i = 1; i <= 5; ++i) items.push_back({i, 1.0f, 0});
  run(f, items, 5.0);
  items.push_back({9, 9.0f, 1});
  auto p = f.update(items, 5.0);
  CHECK(std::any_of(p.begin(), p.end(), [](const SonarField::Ping& x) { return x.id == 9; }));
  // An empty field that has been running counts too: the first thing on a quiet floor sounds at once.
  SonarField g;
  g.update({}, 0.0);
  CHECK(g.update({{3, 10.0f, 0}}, 1.0).size() == 1);
}

TEST_CASE("sonar field: a field starting up staggers what is already there") {
  SonarField f;
  CHECK(f.update({{7, 20.0f, 0}}, 0.0).empty());
  int n = 0;
  for (double t = 0.01; t <= f.params().period_far + 0.01; t += 0.01) n += (int)f.update({{7, 20.0f, 0}}, t).size();
  CHECK(n == 1);
}

TEST_CASE("sonar field: a thing that comes closer does not wait out its old, slow period") {
  SonarField f;
  f.update({}, 0.0);
  REQUIRE(f.update({{9, 10.0f, 0}}, 1.0).size() == 1);   // sounded far off: next in most of a second
  int n = 0;
  // Beside the player now: again after its near period (0.15 s).
  for (double t = 1.01; t <= 1.20; t += 0.01) n += (int)f.update({{9, 1.0f, 0}}, t).size();
  CHECK(n == 1);
}

TEST_CASE("sonar field: a thing gone for a moment keeps its place, gone for long is forgotten") {
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

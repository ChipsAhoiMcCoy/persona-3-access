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
    REQUIRE(v.size() > (size_t)rate * 11 / 100);  // over 0.11 s: a 45 to 100 ms set was too clipped (2026-11-13)
    CHECK(v.size() < (size_t)rate * 2 / 10);      // under 0.2 s
    double peak = 0;
    bool finite = true;
    for (float s : v) { finite = finite && std::isfinite(s); peak = std::max(peak, (double)std::fabs(s)); }
    CHECK(finite);
    CHECK(peak <= kCuePeak + 1e-4);
    CHECK(std::fabs(v.front()) < 0.01);
    CHECK(std::fabs(v.back()) < 0.01);
    // As loud as each other to the ear; none is held under it by its peak.
    CHECK(cue_loudness(v, rate) == doctest::Approx(kCueLoudness).epsilon(0.01));
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
    double peak = 0;
    for (float s : v) peak = std::max(peak, (double)std::fabs(s));
    CHECK(peak <= kCuePeak + 1e-4);
    CHECK(std::fabs(v.front()) < 0.01);
    CHECK(std::fabs(v.back()) < 0.01);
    CHECK(cue_loudness(v, rate) == doctest::Approx(kCueLoudness).epsilon(0.01));
    for (int b = k + 1; b < (int)SonarCue::Count; ++b) CHECK(sonar_cue((SonarCue)b, rate, true) != v);
  }
}

TEST_CASE("signal cues: short, clean at both ends, the sonar's loudness") {
  const int rate = 48000;
  for (int k = 0; k < (int)SignalCue::Count; ++k) {
    CAPTURE(k);
    std::vector<float> v = signal_cue((SignalCue)k, rate);
    REQUIRE(v.size() > (size_t)rate * 7 / 100);  // over 70 ms
    CHECK(v.size() < (size_t)rate * 3 / 10);     // under 0.3 s
    double peak = 0;
    bool finite = true;
    for (float s : v) { finite = finite && std::isfinite(s); peak = std::max(peak, (double)std::fabs(s)); }
    CHECK(finite);
    CHECK(peak <= kCuePeak + 1e-4);
    CHECK(std::fabs(v.front()) < 0.01);
    CHECK(std::fabs(v.back()) < 0.01);
    CHECK(cue_loudness(v, rate) == doctest::Approx(kCueLoudness).epsilon(0.01));
    CHECK(signal_cue((SignalCue)k, rate) == v);
  }
}

TEST_CASE("cue loudness: weighted as the ear hears, and a short sound counts as quieter") {
  const int rate = 48000;
  auto tone = [&](double hz, double seconds) {
    std::vector<float> v((size_t)(rate * seconds));
    for (size_t i = 0; i < v.size(); ++i) v[i] = (float)(0.5 * std::sin(2.0 * 3.14159265358979 * hz * i / rate));
    return v;
  };
  // A 1 kHz tone reads near its own RMS (BS.1770 is set to about +0.7 dB there).
  CHECK(cue_loudness(tone(1000.0, 0.2), rate) == doctest::Approx(0.5 / std::sqrt(2.0)).epsilon(0.1));
  // The same level at 3 kHz, where the ear is keen, reads louder; at 60 Hz, where it is dull, quieter.
  CHECK(cue_loudness(tone(3000.0, 0.2), rate) > cue_loudness(tone(1000.0, 0.2), rate) * 1.2);
  CHECK(cue_loudness(tone(60.0, 0.2), rate) < cue_loudness(tone(1000.0, 0.2), rate) * 0.9);
  // Half the ear's window long, it reads quieter than the same tone held.
  CHECK(cue_loudness(tone(1000.0, 0.05), rate) < cue_loudness(tone(1000.0, 0.2), rate) * 0.75);
  CHECK(cue_loudness({}, rate) == 0.0);
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

TEST_CASE("file cues: the file's silence cut, clean ends, the common loudness; the brief form fits the fastest rhythm") {
  const int rate = 48000;
  // A file as Grim Dark's entrance cue is: silence before it, a decaying tone, a long silence after.
  std::vector<float> file(rate * 6 / 10, 0.0f);
  const size_t start = rate * 34 / 1000, len = rate * 3 / 10;
  for (size_t i = 0; i < len; ++i)
    file[start + i] = 0.6f * std::exp(-(float)i / (rate * 0.08f)) * std::sin(2.0f * 3.14159265f * 700.0f * (float)i / rate);
  std::vector<float> v = file_cue(file, rate);
  REQUIRE(!v.empty());
  CHECK(v.size() < file.size() - start);            // the silence after is gone
  CHECK(v.size() <= len + (size_t)rate / 50);       // and the one before
  CHECK(std::fabs(v.front()) < 0.01);
  CHECK(std::fabs(v.back()) < 0.01);
  double peak = 0;
  for (float s : v) peak = std::max(peak, (double)std::fabs(s));
  CHECK(peak <= kCuePeak + 1e-4);
  CHECK(cue_loudness(v, rate) == doctest::Approx(kCueLoudness).epsilon(0.01));

  std::vector<float> b = file_cue(file, rate, true);
  FieldParams p;
  CHECK((double)b.size() / rate / 0.84 + p.gap_s <= p.period_near);   // played lower from behind too
  CHECK(std::fabs(b.back()) < 0.01);
  CHECK(cue_loudness(b, rate) == doctest::Approx(kCueLoudness).epsilon(0.01));

  CHECK(file_cue(std::vector<float>(1000, 0.0f), rate).empty());   // a silent file: the made sound stands in
  CHECK(file_cue({}, rate).empty());
}

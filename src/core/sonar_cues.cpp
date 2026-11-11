#include "core/sonar_cues.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace p3a::core {
namespace {
constexpr double kPi = 3.14159265358979323846;

// White noise in -1..1, the same every run (a cue is the same sound each time it is made).
struct Noise {
  uint32_t s;
  explicit Noise(uint32_t seed) : s(seed) {}
  float next() { s = s * 1664525u + 1013904223u; return (float)((s >> 8) & 0xffff) / 32767.5f - 1.0f; }
};

// A two-pole band-pass (RBJ, constant peak gain); the centre may move from sample to sample.
struct BandPass {
  double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
  float run(float x, double hz, double q, int rate) {
    double w = 2.0 * kPi * hz / rate, alpha = std::sin(w) / (2.0 * q), a0 = 1.0 + alpha;
    double y = (alpha * x - alpha * x2 + 2.0 * std::cos(w) * y1 - (1.0 - alpha) * y2) / a0;
    x2 = x1; x1 = x; y2 = y1; y1 = y;
    return (float)y;
  }
};

int frames(int rate, double seconds) { return (int)std::lround(rate * seconds); }

// How long a brief cue is: with the field's breath after it, one fits the fastest rhythm (0.12 s), played lower
// from behind too.
constexpr double kBrief = 0.065;

// Exit: noise through a band that sweeps upward, swelling and fading, over a low thump.
std::vector<float> exit_cue(int rate, double seconds) {
  const int n = frames(rate, seconds);
  std::vector<float> v(n);
  Noise noise(0x51ed270b);
  BandPass bp;
  double phase = 0;
  for (int i = 0; i < n; ++i) {
    double t = (double)i / n;                       // 0..1 through the sound
    double hz = 450.0 * std::pow(5.5, t);           // 450 Hz -> ~2.5 kHz
    double swell = std::sin(kPi * std::pow(t, 0.7));   // peaks a little before the middle
    float air = bp.run(noise.next(), hz, 2.2, rate) * (float)(swell * swell);
    double sec = (double)i / rate;
    phase += 2.0 * kPi * (150.0 - 40.0 * t) / rate;   // the thump drops a little as it dies
    float thump = (float)(std::sin(phase) * std::exp(-sec / 0.045) * (1.0 - std::exp(-sec / 0.003)));
    v[i] = air * 1.6f + thump * 0.55f;
  }
  return v;
}

// One knock on wood into v at `at`: three damped modes and a click.
void knock(std::vector<float>& v, int rate, double at, float level, uint32_t seed) {
  static const struct { double hz, tau, amp; } modes[] = {{205.0, 0.030, 1.0}, {470.0, 0.018, 0.7}, {1180.0, 0.007, 0.45}};
  Noise noise(seed);
  int start = frames(rate, at);
  for (int i = 0; start + i < (int)v.size(); ++i) {
    double sec = (double)i / rate;
    double s = 0;
    for (const auto& m : modes) s += m.amp * std::sin(2.0 * kPi * m.hz * sec) * std::exp(-sec / m.tau);
    s += noise.next() * 0.5 * std::exp(-sec / 0.0025);   // the knuckle
    v[start + i] += (float)(s * (1.0 - std::exp(-sec / 0.0006))) * level;
  }
}
std::vector<float> door_cue(int rate, bool brief) {
  std::vector<float> v(frames(rate, brief ? kBrief : 0.16), 0.0f);
  knock(v, rate, 0.0, 1.0f, 0x1234abcd);
  if (!brief) knock(v, rate, 0.07, 0.8f, 0x0badf00d);
  return v;
}

// Person: someone whistling for you, two notes, the second a third lower ("yoo-hoo"). Each note is a nearly pure
// tone that scoops up into its pitch, with a little vibrato and the breath around it (noise through a band at the
// note's pitch, which also gives the ears something to place). It was a synthetic voiced "hm" at first: strange to
// hear (the user, 2026-11-10).
std::vector<float> person_cue(int rate, bool brief) {
  const int n = frames(rate, brief ? kBrief : 0.16);
  std::vector<float> v(n, 0.0f);
  static const struct { double at, len, hz; } notes[] = {{0.0, 0.07, 1568.0}, {0.075, 0.085, 1319.0}};   // G6, E6
  Noise noise(0x6a09e667);
  for (const auto& note : notes) {
    if (brief && note.at > 0) break;
    BandPass breath;
    const int start = frames(rate, note.at), count = frames(rate, note.len);
    double phase = 0;
    for (int i = 0; i < count && start + i < n; ++i) {
      double sec = (double)i / rate;
      double hz = note.hz * (1.0 - 0.06 * std::exp(-sec / 0.012)) * (1.0 + 0.004 * std::sin(2.0 * kPi * 6.0 * sec));
      phase += 2.0 * kPi * hz / rate;
      double env = (1.0 - std::exp(-sec / 0.010)) * std::min(1.0, (count - i) / (rate * 0.025));
      double tone = std::sin(phase) + 0.08 * std::sin(2.0 * phase);
      v[start + i] += (float)(tone * env) + breath.run(noise.next(), hz, 6.0, rate) * (float)(1.2 * env);
    }
  }
  return v;
}

// One strike of a small bell into v at `at`: inharmonic partials, each with its own decay.
void strike(std::vector<float>& v, int rate, double at, double hz, float level) {
  static const struct { double ratio, tau, amp; } parts[] = {{1.0, 0.050, 1.0}, {2.76, 0.036, 0.55}, {5.40, 0.022, 0.3}};
  int start = frames(rate, at);
  for (int i = 0; start + i < (int)v.size(); ++i) {
    double sec = (double)i / rate;
    double s = 0;
    for (const auto& p : parts)
      if (hz * p.ratio < rate * 0.45) s += p.amp * std::sin(2.0 * kPi * hz * p.ratio * sec) * std::exp(-sec / p.tau);
    v[start + i] += (float)(s * (1.0 - std::exp(-sec / 0.0008))) * level;
  }
}
std::vector<float> shop_cue(int rate, bool brief) {
  std::vector<float> v(frames(rate, brief ? kBrief : 0.16), 0.0f);
  // The brief one is the higher strike alone: the lower is the whistle's first note, and one of each would differ in
  // timbre only.
  if (brief) { strike(v, rate, 0.0, 2093.0, 1.0f); return v; }
  strike(v, rate, 0.0, 1568.0, 0.8f);     // G6
  strike(v, rate, 0.06, 2093.0, 1.0f);    // C7: the till's "ching"
  return v;
}

// Thing: a plucked string (Karplus-Strong): a burst of noise going round a delay line that dulls it each lap.
std::vector<float> thing_cue(int rate, double seconds) {
  const int n = frames(rate, seconds);
  const int period = std::max(2, (int)std::lround(rate / 880.0));   // A5
  std::vector<float> line(period), v(n);
  Noise noise(0x7f4a7c15);
  for (float& s : line) s = noise.next();
  int at = 0;
  // Three laps go by unheard: the raw burst is all spikes, far above the ringing note it turns into.
  for (int i = -3 * period; i < n; ++i) {
    int nx = (at + 1) % period;
    float out = line[at];
    line[at] = (line[at] + line[nx]) * 0.5f * 0.996f;
    at = nx;
    if (i >= 0) v[i] = out;
  }
  return v;
}

// Shadow: two low sawtooth voices a few hertz apart (they beat: the roughness) under a murmur of noise.
std::vector<float> shadow_cue(int rate) {
  // 0.28 s (0.18 at first: over before it was noticed), and it flutters: the loudness shakes 38 times a second, a
  // snarl's rattle, which no other sound of the mod has.
  const int n = frames(rate, 0.28);
  std::vector<float> v(n);
  Noise noise(0x9e3779b9);
  BandPass murmur;
  double pa = 0, pb = 0;
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate, t = (double)i / n;
    // Sinking a little: something heavy. An octave's worth of the weight is given up to be heard: at 82 Hz most of
    // it sat under what small headphones play and under where the ear is keen, and it went unnoticed until close.
    double fa = 110.0 - 12.0 * t, fb = 117.5 - 12.0 * t;
    pa += 2.0 * kPi * fa / rate; pb += 2.0 * kPi * fb / rate;
    double s = 0;
    // The overtones fall off slowly (a snarl, not a hum), so the 500 Hz - 3 kHz band carries it.
    for (int k = 1; k * fb < 4500.0; ++k) s += (std::sin(k * pa) + std::sin(k * pb)) / std::pow((double)k, 0.75);
    // Quick in, held for the first third, then a straight fade out; the rattle on top.
    double env = (1.0 - std::exp(-sec / 0.008)) * std::min(1.0, (1.0 - t) * 1.5);
    env *= 0.62 + 0.38 * std::cos(2.0 * kPi * 38.0 * sec);
    v[i] = (float)(s * env) + murmur.run(noise.next(), 1400.0, 1.0, rate) * (float)(2.5 * env);
  }
  return v;
}

// One glassy tick into v at `at`.
void glass(std::vector<float>& v, int rate, double at, double hz, float level) {
  int start = frames(rate, at);
  for (int i = 0; start + i < (int)v.size(); ++i) {
    double sec = (double)i / rate;
    double s = std::sin(2.0 * kPi * hz * sec) + 0.4 * std::sin(2.0 * kPi * hz * 2.41 * sec);
    v[start + i] += (float)(s * std::exp(-sec / 0.010) * (1.0 - std::exp(-sec / 0.0005))) * level;
  }
}
std::vector<float> ambush_cue(int rate) {
  std::vector<float> v(frames(rate, 0.10), 0.0f);
  glass(v, rate, 0.0, 1760.0, 0.85f);
  glass(v, rate, 0.040, 2637.0, 1.0f);
  return v;
}
std::vector<float> layer_cue(int rate) {
  std::vector<float> v(frames(rate, 0.05), 0.0f);
  glass(v, rate, 0.0, 1245.0, 1.0f);
  return v;
}

// Reach: a blade drawn: a hiss that sweeps up, then steel ringing on. The ring was 0.11 s of two partials at 3.5 and
// 5.3 kHz, thin and soon gone; it now sits at 2 kHz with two more above, a pair a few Hz apart so it shimmers, and
// rang on for a quarter of a second. That tail outlived the reach: it is 0.09 s now, a quick bright "ting", repeated
// fast, so it is gone the moment the reach is (the user's call, 2026-11-10).
std::vector<float> reach_cue(int rate) {
  const int n = frames(rate, 0.09);
  std::vector<float> v(n);
  Noise noise(0x3c6ef372);
  BandPass hiss;
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate;
    double sweep = 2000.0 + 5000.0 * std::min(1.0, sec / 0.015);
    double ring = std::sin(2.0 * kPi * 1976.0 * sec) + 0.8 * std::sin(2.0 * kPi * 1990.0 * sec) +
                  0.55 * std::sin(2.0 * kPi * 3140.0 * sec) + 0.35 * std::sin(2.0 * kPi * 4730.0 * sec);
    v[i] = hiss.run(noise.next(), sweep, 2.5, rate) * (float)(2.2 * std::exp(-sec / 0.008)) +
           (float)(ring * 0.5 * std::exp(-sec / 0.028) * (1.0 - std::exp(-sec / 0.002)));
  }
  return v;
}

// A sawtooth note into v from `at` for `len` seconds (band-limited, quick in and out).
void saw_note(std::vector<float>& v, int rate, double at, double len, double hz, float level) {
  int start = frames(rate, at), count = frames(rate, len);
  for (int i = 0; i < count && start + i < (int)v.size(); ++i) {
    double sec = (double)i / rate, s = 0;
    for (int k = 1; k * hz < 6000.0; ++k) s += std::sin(2.0 * kPi * hz * k * sec) / k;
    double env = (1.0 - std::exp(-sec / 0.003)) * std::min(1.0, (count - i) / (rate * 0.012));
    v[start + i] += (float)(s * env) * level;
  }
}
// Spotted: two harsh notes a tritone apart, the second higher and longer: an alarm.
std::vector<float> spotted_cue(int rate) {
  std::vector<float> v(frames(rate, 0.19), 0.0f);
  saw_note(v, rate, 0.0, 0.07, 622.0, 0.9f);
  saw_note(v, rate, 0.075, 0.115, 880.0, 1.0f);
  return v;
}
// Lost: two soft sine notes, falling a fourth.
std::vector<float> lost_cue(int rate) {
  std::vector<float> v(frames(rate, 0.19), 0.0f);
  const double hz[2] = {587.0, 440.0};
  for (int k = 0; k < 2; ++k) {
    int start = frames(rate, k * 0.085), count = frames(rate, 0.10);
    for (int i = 0; i < count && start + i < (int)v.size(); ++i) {
      double sec = (double)i / rate;
      double s = std::sin(2.0 * kPi * hz[k] * sec) + 0.2 * std::sin(4.0 * kPi * hz[k] * sec);
      v[start + i] += (float)(s * (1.0 - std::exp(-sec / 0.008)) * std::exp(-sec / 0.040));
    }
  }
  return v;
}

// Silence at both ends (a few ms of fade, so nothing clicks), no DC, then the common loudness.
void finish(std::vector<float>& v, int rate, float rms_target) {
  if (v.empty()) return;
  double mean = 0;
  for (float s : v) mean += s;
  mean /= (double)v.size();
  for (float& s : v) s -= (float)mean;
  const int in = std::min((int)v.size() / 2, frames(rate, 0.002)), out = std::min((int)v.size() / 2, frames(rate, 0.012));
  for (int i = 0; i < in; ++i) v[i] *= (float)i / in;
  for (int i = 0; i < out; ++i) v[v.size() - 1 - i] *= (float)i / out;
  double sum = 0, peak = 0;
  for (float s : v) { sum += (double)s * s; peak = std::max(peak, (double)std::fabs(s)); }
  double rms = std::sqrt(sum / (double)v.size());
  if (rms <= 1e-9) return;
  double g = rms_target / rms;
  if (peak * g > 0.9) g = 0.9 / peak;
  for (float& s : v) s *= (float)g;
}

// The full sounds were 0.16 to 0.28 s: the line fitted under four a second, so in a room of six things the nearest
// repeated about once a second however near it was. They are 0.11 to 0.16 s now (the user's call, 2026-11-11).
std::vector<float> raw(SonarCue cue, int rate, bool brief) {
  switch (cue) {
    case SonarCue::Exit: return exit_cue(rate, brief ? kBrief : 0.16);
    case SonarCue::Door: return door_cue(rate, brief);
    case SonarCue::Person: return person_cue(rate, brief);
    case SonarCue::Shop: return shop_cue(rate, brief);
    default: return thing_cue(rate, brief ? kBrief : 0.11);
  }
}
}  // namespace

std::vector<float> sonar_cue(SonarCue cue, int rate, bool brief) {
  std::vector<float> v = raw(cue, rate, brief);
  finish(v, rate, kSonarCueRms);
  return v;
}

std::vector<float> signal_cue(SignalCue cue, int rate) {
  std::vector<float> v;
  switch (cue) {
    case SignalCue::Shadow: v = shadow_cue(rate); break;
    case SignalCue::Ambush: v = ambush_cue(rate); break;
    case SignalCue::Reach: v = reach_cue(rate); break;
    case SignalCue::Spotted: v = spotted_cue(rate); break;
    case SignalCue::Lost: v = lost_cue(rate); break;
    default: v = layer_cue(rate); break;
  }
  finish(v, rate, kSonarCueRms);
  return v;
}

}  // namespace p3a::core

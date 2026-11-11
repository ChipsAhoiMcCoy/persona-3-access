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

// Exit: noise through a band that sweeps upward, swelling and fading, over a low thump.
std::vector<float> exit_cue(int rate) {
  const int n = frames(rate, 0.24);
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
std::vector<float> door_cue(int rate) {
  std::vector<float> v(frames(rate, 0.26), 0.0f);
  knock(v, rate, 0.0, 1.0f, 0x1234abcd);
  knock(v, rate, 0.105, 0.8f, 0x0badf00d);
  return v;
}

// Person: a voiced source (harmonics of a rising pitch) through three formants, an open "uh" turning up.
std::vector<float> person_cue(int rate) {
  const int n = frames(rate, 0.17);
  std::vector<float> v(n);
  BandPass f1, f2, f3;
  double phase = 0;
  for (int i = 0; i < n; ++i) {
    double t = (double)i / n, sec = (double)i / rate;
    double f0 = 170.0 + 75.0 * t * t;   // 170 -> 245 Hz: a questioning rise
    phase += 2.0 * kPi * f0 / rate;
    double src = 0;
    for (int k = 1; k * f0 < 5000.0; ++k) src += std::sin(k * phase) / k;   // a band-limited sawtooth
    float x = (float)src;
    float y = f1.run(x, 620.0, 5.0, rate) + 0.7f * f2.run(x, 1150.0, 7.0, rate) + 0.35f * f3.run(x, 2600.0, 8.0, rate);
    double env = (1.0 - std::exp(-sec / 0.012)) * std::min(1.0, (n - i) / (rate * 0.05));
    v[i] = y * (float)env;
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
std::vector<float> shop_cue(int rate) {
  std::vector<float> v(frames(rate, 0.28), 0.0f);
  strike(v, rate, 0.0, 1568.0, 0.8f);     // G6
  strike(v, rate, 0.085, 2093.0, 1.0f);   // C7: the till's "ching"
  return v;
}

// Thing: a plucked string (Karplus-Strong): a burst of noise going round a delay line that dulls it each lap.
std::vector<float> thing_cue(int rate) {
  const int n = frames(rate, 0.16);
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

// The selected thing's mark: a very short high tick.
std::vector<float> tick_mark(int rate) {
  const int n = frames(rate, 0.03);
  std::vector<float> v(n);
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate;
    v[i] = (float)((std::sin(2.0 * kPi * 3100.0 * sec) + 0.5 * std::sin(2.0 * kPi * 4650.0 * sec)) * std::exp(-sec / 0.005) *
                   (1.0 - std::exp(-sec / 0.0004)));
  }
  return v;
}

// Beacon: a chirp rising an octave with two overtones, and a breath of noise at its start for the ears to place.
std::vector<float> beacon_cue(int rate) {
  const int n = frames(rate, 0.13);
  std::vector<float> v(n);
  Noise noise(0x2545f491);
  double phase = 0;
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate, t = std::min(1.0, sec / 0.07);   // the rise takes 70 ms, then it rings
    phase += 2.0 * kPi * 660.0 * std::pow(2.0, t) / rate;
    double tone = std::sin(phase) + 0.35 * std::sin(2.0 * phase) + 0.15 * std::sin(3.0 * phase);
    double env = (1.0 - std::exp(-sec / 0.003)) * std::exp(-sec / 0.055);
    v[i] = (float)(tone * env + noise.next() * 0.35 * std::exp(-sec / 0.004));
  }
  return v;
}

// Shadow: two low sawtooth voices a few hertz apart (they beat: the roughness) under a murmur of noise.
std::vector<float> shadow_cue(int rate) {
  const int n = frames(rate, 0.18);
  std::vector<float> v(n);
  Noise noise(0x9e3779b9);
  BandPass murmur;
  double pa = 0, pb = 0;
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate, t = (double)i / n;
    double fa = 82.0 - 10.0 * t, fb = 88.5 - 10.0 * t;   // sinking a little: something heavy
    pa += 2.0 * kPi * fa / rate; pb += 2.0 * kPi * fb / rate;
    double s = 0;
    for (int k = 1; k * fb < 3000.0; ++k) s += (std::sin(k * pa) + std::sin(k * pb)) / std::pow((double)k, 1.15);
    double env = (1.0 - std::exp(-sec / 0.008)) * (1.0 - t);   // quick in, a straight fade out
    v[i] = (float)(s * env) + murmur.run(noise.next(), 900.0, 1.2, rate) * (float)(1.5 * env);
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

// Reach: a blade's ring: a hiss that sweeps up fast into two high, slightly sour partials.
std::vector<float> reach_cue(int rate) {
  const int n = frames(rate, 0.11);
  std::vector<float> v(n);
  Noise noise(0x3c6ef372);
  BandPass hiss;
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate;
    double sweep = 2500.0 + 5500.0 * std::min(1.0, sec / 0.02);
    double ring = std::sin(2.0 * kPi * 3520.0 * sec) + 0.6 * std::sin(2.0 * kPi * 5310.0 * sec);
    v[i] = hiss.run(noise.next(), sweep, 3.0, rate) * (float)(2.0 * std::exp(-sec / 0.012)) +
           (float)(ring * 0.5 * std::exp(-sec / 0.030) * (1.0 - std::exp(-sec / 0.002)));
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

std::vector<float> raw(SonarCue cue, int rate) {
  switch (cue) {
    case SonarCue::Exit: return exit_cue(rate);
    case SonarCue::Door: return door_cue(rate);
    case SonarCue::Person: return person_cue(rate);
    case SonarCue::Shop: return shop_cue(rate);
    default: return thing_cue(rate);
  }
}
}  // namespace

std::vector<float> sonar_cue(SonarCue cue, int rate) {
  std::vector<float> v = raw(cue, rate);
  finish(v, rate, kSonarCueRms);
  return v;
}

std::vector<float> signal_cue(SignalCue cue, int rate) {
  std::vector<float> v;
  switch (cue) {
    case SignalCue::Beacon: v = beacon_cue(rate); break;
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

std::vector<float> sonar_cue_marked(SonarCue cue, int rate) {
  std::vector<float> mark = tick_mark(rate), body = sonar_cue(cue, rate);
  finish(mark, rate, kSonarCueRms);
  for (float& s : mark) s *= 0.6f;   // all attack: at the cues' RMS it would stand well above them
  std::vector<float> v;
  v.reserve(mark.size() + frames(rate, 0.035) + body.size());
  v.insert(v.end(), mark.begin(), mark.end());
  v.insert(v.end(), (size_t)frames(rate, 0.035), 0.0f);
  v.insert(v.end(), body.begin(), body.end());
  return v;
}

}  // namespace p3a::core

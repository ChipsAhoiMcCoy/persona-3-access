#include "core/sonar_cues.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace p3a::core {
namespace {
constexpr double kPi = 3.14159265358979323846;

// The set as it is (the user's call, 2026-11-13): every cue is one event, not two to four hits in a row (those made
// each sound a little rhythm of its own, too much with several things around), and 0.12 to 0.16 s long: a 45 to
// 100 ms set was tried first and was too clipped to enjoy, and a denser, layered set after this one was not liked
// as well. They are made like real things (struck wood, a bell, a plucked string, a whistle) with soft attacks,
// gently rounded tops and a breath of room after them, and they are matched by how loud they sound, not by their
// raw level (see cue_loudness()).

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

// A one-pole low-pass: takes the fizz off a noise burst.
struct LowPass {
  double y = 0;
  float run(float x, double hz, int rate) { y += (1.0 - std::exp(-2.0 * kPi * hz / rate)) * (x - y); return (float)y; }
};

int frames(int rate, double seconds) { return (int)std::lround(rate * seconds); }

// From 0 to 1 over 0..1, easing in and out: a glide between two pitches.
double ease(double t) { t = std::clamp(t, 0.0, 1.0); return t * t * (3.0 - 2.0 * t); }

// A rise over `attack` seconds (a raised cosine: no click, no snap), then an exponential decay with time constant tau.
double env_ad(double sec, double attack, double tau) {
  double rise = sec < attack ? 0.5 - 0.5 * std::cos(kPi * sec / attack) : 1.0;
  return rise * std::exp(-std::max(0.0, sec - attack) / tau);
}

// A held note's envelope: the same soft rise, held, then a cosine fall over the last `release` seconds of `len`.
double env_note(double sec, double len, double attack, double release) {
  double rise = sec < attack ? 0.5 - 0.5 * std::cos(kPi * sec / attack) : 1.0;
  double left = len - sec;
  double fall = left < release ? 0.5 - 0.5 * std::cos(kPi * std::max(0.0, left) / release) : 1.0;
  return rise * fall;
}

// A small, dry room: four damped comb echoes (11 to 18 ms) through two diffusers, mixed in quietly, so a cue sounds
// like it happened somewhere rather than in a void. The tail is short, so it never blurs the next pulse, and the
// cue's place stays as sharp as its start.
void room(std::vector<float>& v, int rate, float mix) {
  struct Comb { std::vector<float> buf; size_t at = 0; float lp = 0; };
  static const double comb_ms[4] = {11.3, 13.7, 15.9, 17.9};
  static const double diff_ms[2] = {4.9, 1.7};
  Comb combs[4];
  for (int k = 0; k < 4; ++k) combs[k].buf.assign(std::max(1, frames(rate, comb_ms[k] / 1000.0)), 0.0f);
  std::vector<float> diff[2];
  size_t dat[2] = {0, 0};
  for (int k = 0; k < 2; ++k) diff[k].assign(std::max(1, frames(rate, diff_ms[k] / 1000.0)), 0.0f);
  const float feedback = 0.45f, damp = 0.35f, g = 0.6f;
  for (float& s : v) {
    float wet = 0;
    for (Comb& c : combs) {
      float out = c.buf[c.at];
      c.lp = out * (1.0f - damp) + c.lp * damp;   // each echo a little duller than the last
      c.buf[c.at] = s + c.lp * feedback;
      c.at = (c.at + 1) % c.buf.size();
      wet += out;
    }
    wet *= 0.25f;
    for (int k = 0; k < 2; ++k) {
      float d = diff[k][dat[k]];
      float in = wet + d * g;
      diff[k][dat[k]] = in;
      dat[k] = (dat[k] + 1) % diff[k].size();
      wet = d - in * g;
    }
    s += wet * mix;
  }
}

// ---- The sonar's sounds ------------------------------------------------------------------------------------------

// Exit: a breath of air rising through an opening (noise through two band-passes that sweep up together: rounder
// than one), swelling and fading, over a soft low thump.
std::vector<float> exit_cue(int rate, bool brief) {
  const int n = frames(rate, brief ? 0.085 : 0.15);
  std::vector<float> v(n);
  Noise noise(0x51ed270b);
  BandPass a, b;
  double phase = 0;
  for (int i = 0; i < n; ++i) {
    double t = (double)i / n, sec = (double)i / rate;
    double hz = 480.0 * std::pow(5.0, t);                 // 480 Hz -> 2.4 kHz
    double swell = std::sin(kPi * std::pow(t, 0.65));
    float air = b.run(a.run(noise.next(), hz, 1.4, rate), hz, 1.4, rate) * (float)(swell * swell);
    phase += 2.0 * kPi * (120.0 - 30.0 * t) / rate;
    float thump = (float)(std::sin(phase) * env_ad(sec, 0.004, 0.04));
    v[i] = air * 3.2f + thump * 0.45f;
  }
  return v;
}

// Door: one knock on a wooden door: five damped modes of a panel (the low ones ring longest) and the knuckle's
// tap, a little noise through a band. One knock, a hollow one, where there were two short ones.
std::vector<float> door_cue(int rate, bool brief) {
  const int n = frames(rate, brief ? 0.08 : 0.12);
  const double scale = brief ? 0.7 : 1.0;   // the brief knock is a firmer door: it stops sooner
  static const struct { double hz, tau, amp; } modes[] = {
      {185.0, 0.050, 1.0}, {410.0, 0.032, 0.65}, {735.0, 0.020, 0.40}, {1190.0, 0.012, 0.28}, {2040.0, 0.006, 0.18}};
  std::vector<float> v(n);
  Noise noise(0x1234abcd);
  BandPass tap;
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate, s = 0;
    for (const auto& m : modes) s += m.amp * std::sin(2.0 * kPi * m.hz * sec) * std::exp(-sec / (m.tau * scale));
    s += tap.run(noise.next(), 2400.0, 0.9, rate) * 1.4 * std::exp(-sec / 0.003);
    v[i] = (float)(s * (1.0 - std::exp(-sec / 0.0007)));
  }
  return v;
}

// Person: someone whistling for you: one note that scoops up into E6 and falls a minor third to C sharp 6 as it
// goes, the "yoo-hoo" in a single breath. A nearly pure tone with a slow vibrato and the breath around it (noise
// through a band at the note's pitch, which also gives the ears something to place); a little lower than it was,
// so it is sweeter. It was a synthetic voiced "hm" at first: strange to hear (the user, 2026-11-10).
std::vector<float> person_cue(int rate, bool brief) {
  const double len = brief ? 0.09 : 0.15;
  const int n = frames(rate, len);
  std::vector<float> v(n);
  Noise noise(0x6a09e667);
  BandPass breath;
  double phase = 0;
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate, t = (double)i / n;
    double fall = std::pow(1109.0 / 1319.0, ease((t - 0.35) / 0.45));   // held, then down
    double scoop = 1.0 - 0.05 * std::exp(-sec / 0.012);
    double hz = 1319.0 * fall * scoop * (1.0 + 0.005 * std::sin(2.0 * kPi * 5.5 * sec));
    phase += 2.0 * kPi * hz / rate;
    double env = env_note(sec, len, 0.014, len * 0.35);
    double tone = std::sin(phase) + 0.06 * std::sin(2.0 * phase);
    double air = 0.9 + 1.2 * std::exp(-sec / 0.02);   // more breath as it starts
    v[i] = (float)(tone * env) + breath.run(noise.next(), hz, 7.0, rate) * (float)(air * env);
  }
  return v;
}

// One strike of a small bell into v at `at`: a handbell's partials (inharmonic, each with its own decay, the high
// ones shortest) and the soft tick of the clapper.
void strike(std::vector<float>& v, int rate, double at, double hz, float level, double ring, uint32_t seed) {
  static const struct { double ratio, tau, amp; } parts[] = {
      {1.0, 0.130, 1.0}, {2.0, 0.085, 0.30}, {2.76, 0.060, 0.45}, {4.07, 0.035, 0.22}, {5.40, 0.020, 0.12}};
  Noise noise(seed);
  LowPass soft;
  int start = frames(rate, at);
  for (int i = 0; start + i < (int)v.size(); ++i) {
    double sec = (double)i / rate, s = 0;
    for (const auto& p : parts)
      if (hz * p.ratio < rate * 0.45) s += p.amp * std::sin(2.0 * kPi * hz * p.ratio * sec) * std::exp(-sec / (p.tau * ring));
    s += soft.run(noise.next(), 5000.0, rate) * 0.5 * std::exp(-sec / 0.0015);
    v[start + i] += (float)(s * (1.0 - std::exp(-sec / 0.0008))) * level;
  }
}
// Shop: a shop's bell, two bells a fifth apart struck together (C6 and G6): the till's "ching". An octave lower
// than it was, so it rings rather than pierces.
std::vector<float> shop_cue(int rate, bool brief) {
  std::vector<float> v(frames(rate, brief ? 0.09 : 0.15), 0.0f);
  const double ring = brief ? 0.55 : 1.0;
  strike(v, rate, 0.0, 1047.0, 1.0f, ring, 0x0badf00d);   // C6
  strike(v, rate, 0.0, 1568.0, 0.6f, ring, 0x5eed1234);   // G6
  return v;
}

// Thing: a plucked string at A5 (Karplus-Strong: a burst of noise going round a delay line that dulls it each lap).
// The burst is softened first, a finger rather than a pick, so the note is warm, not a snap.
std::vector<float> thing_cue(int rate, bool brief) {
  const int n = frames(rate, brief ? 0.085 : 0.13);
  const int period = std::max(2, (int)std::lround(rate / 880.0));
  const float keep = brief ? 0.985f : 0.996f;   // the brief one is damped by the hand
  std::vector<float> line(period), v(n);
  Noise noise(0x7f4a7c15);
  LowPass finger;
  for (float& s : line) s = finger.run(noise.next(), 3500.0, rate) * 2.0f;
  int at = 0;
  // Two laps go by unheard: the raw burst is all spikes, far above the ringing note it turns into.
  for (int i = -2 * period; i < n; ++i) {
    int nx = (at + 1) % period;
    float out = line[at];
    line[at] = (line[at] + line[nx]) * 0.5f * keep;
    at = nx;
    if (i >= 0) v[i] = out * (float)(1.0 - std::exp(-(double)i / rate / 0.001));
  }
  return v;
}

// Event: one hollow note (odd overtones only, rounded off above 3 kHz: a reed or an ocarina, where the alarm is a
// buzz and the whistle is pure) that leaps up a fourth, F5 to B flat 5: "bwip". It was a trill of four notes.
std::vector<float> event_cue(int rate, bool brief) {
  const double len = brief ? 0.085 : 0.13, leap = brief ? 0.018 : 0.028;
  const int n = frames(rate, len);
  std::vector<float> v(n);
  double phase = 0;
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate, s = 0;
    double hz = 698.0 * std::pow(932.0 / 698.0, ease((sec - leap) / 0.012));
    phase += 2.0 * kPi * hz / rate;
    for (int h = 1; h * hz < 8000.0; h += 2) s += std::sin(h * phase) / h * std::exp(-h * hz / 3000.0);
    v[i] = (float)(s * env_note(sec, len, 0.006, len * 0.4));
  }
  return v;
}

// Fragment: a sparkle, one chime of crystal at C7, a pair of tones a few hertz apart (it shimmers) over glassy
// inharmonic partials and a glint of high noise as it is struck. Higher than anything else of the mod's.
std::vector<float> fragment_cue(int rate, bool brief) {
  const int n = frames(rate, brief ? 0.09 : 0.15);
  const double tau = brief ? 0.04 : 0.075;
  std::vector<float> v(n);
  Noise noise(0x2545f491);
  BandPass glint;
  const double hz = 2093.0;
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate;
    double s = (std::sin(2.0 * kPi * hz * sec) + 0.8 * std::sin(2.0 * kPi * (hz + 9.0) * sec)) * std::exp(-sec / tau);
    s += 0.40 * std::sin(2.0 * kPi * hz * 2.76 * sec) * std::exp(-sec / (tau * 0.45));
    s += 0.20 * std::sin(2.0 * kPi * hz * 5.40 * sec) * std::exp(-sec / (tau * 0.2));
    s *= 1.0 - std::exp(-sec / 0.0006);
    s += glint.run(noise.next(), 9000.0, 1.2, rate) * 1.5 * std::exp(-sec / 0.004);
    v[i] = (float)s;
  }
  return v;
}

// ---- The other signals -------------------------------------------------------------------------------------------

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

// One glassy tick into v at `at`, its pitch flicking from hz to hz_to over its first 20 ms.
void glass(std::vector<float>& v, int rate, double at, double hz, double hz_to, double tau, float level) {
  int start = frames(rate, at);
  double phase = 0;
  for (int i = 0; start + i < (int)v.size(); ++i) {
    double sec = (double)i / rate;
    phase += 2.0 * kPi * hz * std::pow(hz_to / hz, ease(sec / 0.02)) / rate;
    double s = std::sin(phase) + 0.35 * std::sin(2.41 * phase) * std::exp(-sec / (tau * 0.5));
    v[start + i] += (float)(s * env_ad(sec, 0.0008, tau)) * level;
  }
}
// Ambush: the strike will land from behind: one glassy tick that flicks up a fifth (it was two ticks).
std::vector<float> ambush_cue(int rate) {
  std::vector<float> v(frames(rate, 0.11), 0.0f);
  glass(v, rate, 0.0, 1760.0, 2637.0, 0.035, 1.0f);
  return v;
}
// Layer: the controller's layer opened: one soft, low glass tick.
std::vector<float> layer_cue(int rate) {
  std::vector<float> v(frames(rate, 0.08), 0.0f);
  glass(v, rate, 0.0, 1245.0, 1245.0, 0.022, 1.0f);
  return v;
}

// Reach: a blade drawn: a hiss that sweeps up, then steel ringing on, a pair a few Hz apart so it shimmers, and two
// partials above. It rang on for a quarter of a second once; that tail outlived the reach. A quick bright "ting",
// repeated fast, gone the moment the reach is (the user's call, 2026-11-10).
std::vector<float> reach_cue(int rate) {
  const int n = frames(rate, 0.10);
  std::vector<float> v(n);
  Noise noise(0x3c6ef372);
  BandPass hiss;
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate;
    double sweep = 2000.0 + 4000.0 * std::min(1.0, sec / 0.015);
    double ring = std::sin(2.0 * kPi * 1976.0 * sec) + 0.8 * std::sin(2.0 * kPi * 1990.0 * sec) +
                  0.45 * std::sin(2.0 * kPi * 3140.0 * sec) * std::exp(-sec / 0.02) +
                  0.25 * std::sin(2.0 * kPi * 4730.0 * sec) * std::exp(-sec / 0.012);
    v[i] = hiss.run(noise.next(), sweep, 2.5, rate) * (float)(1.8 * env_ad(sec, 0.002, 0.008)) +
           (float)(ring * 0.5 * env_ad(sec, 0.003, 0.032));
  }
  return v;
}

// Spotted: an alarm: one buzzing note that jumps up a tritone, E flat 5 to A5 (it was two notes). A sawtooth, but
// rounded off above 2.5 kHz: urgent without the rasp.
std::vector<float> spotted_cue(int rate) {
  const double len = 0.14;
  const int n = frames(rate, len);
  std::vector<float> v(n);
  double phase = 0;
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate, s = 0;
    double hz = 622.0 * std::pow(880.0 / 622.0, ease((sec - 0.035) / 0.012));
    phase += 2.0 * kPi * hz / rate;
    for (int k = 1; k * hz < 7000.0; ++k) s += std::sin(k * phase) / k * std::exp(-k * hz / 2500.0);
    v[i] = (float)(s * env_note(sec, len, 0.004, 0.04));
  }
  return v;
}

// Lost: it has lost the hero again: one soft round note sinking a fourth, D5 to A4 (it was two notes, falling).
std::vector<float> lost_cue(int rate) {
  const int n = frames(rate, 0.16);
  std::vector<float> v(n);
  double phase = 0;
  for (int i = 0; i < n; ++i) {
    double sec = (double)i / rate;
    double hz = 587.0 * std::pow(440.0 / 587.0, ease((sec - 0.03) / 0.07));
    phase += 2.0 * kPi * hz / rate;
    double s = std::sin(phase) + 0.18 * std::sin(2.0 * phase) + 0.05 * std::sin(3.0 * phase);
    v[i] = (float)(s * env_ad(sec, 0.012, 0.07));
  }
  return v;
}

// ---- Finishing ---------------------------------------------------------------------------------------------------

// The K-weighting of ITU-R BS.1770 (what broadcast loudness meters hear through): a shelf that lifts the presence
// band the ear is keen in, and a high-pass under the bass it is deaf to. Made for any rate from the analogue
// prototypes (as libebur128 does).
struct Biquad {
  double b0, b1, b2, a1, a2, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
  double run(double x) {
    double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = x; y2 = y1; y1 = y;
    return y;
  }
};
std::vector<double> k_weighted(const std::vector<float>& v, int rate) {
  double f0 = 1681.974450955533, gain_db = 3.999843853973347, q = 0.7071752369554196;
  double k = std::tan(kPi * f0 / rate), vh = std::pow(10.0, gain_db / 20.0), vb = std::pow(vh, 0.4996667741545416);
  double a0 = 1.0 + k / q + k * k;
  Biquad shelf{(vh + vb * k / q + k * k) / a0, 2.0 * (k * k - vh) / a0, (vh - vb * k / q + k * k) / a0,
               2.0 * (k * k - 1.0) / a0, (1.0 - k / q + k * k) / a0};
  f0 = 38.13547087602444; q = 0.5003270373238773; k = std::tan(kPi * f0 / rate);
  a0 = 1.0 + k / q + k * k;
  Biquad high{1.0, -2.0, 1.0, 2.0 * (k * k - 1.0) / a0, (1.0 - k / q + k * k) / a0};
  std::vector<double> out(v.size());
  for (size_t i = 0; i < v.size(); ++i) out[i] = high.run(shelf.run(v[i]));
  return out;
}

// Silence at both ends (soft fades, so nothing clicks), no DC, a breath of room, then the common loudness.
void finish(std::vector<float>& v, int rate, float room_mix) {
  if (v.empty()) return;
  double mean = 0;
  for (float s : v) mean += s;
  mean /= (double)v.size();
  for (float& s : v) s -= (float)mean;
  if (room_mix > 0) room(v, rate, room_mix);
  const int in = std::min((int)v.size() / 2, frames(rate, 0.002)), out = std::min((int)v.size() / 2, frames(rate, 0.018));
  for (int i = 0; i < in; ++i) v[i] *= (float)(0.5 - 0.5 * std::cos(kPi * i / in));
  for (int i = 0; i < out; ++i) v[v.size() - 1 - i] *= (float)(0.5 - 0.5 * std::cos(kPi * i / out));
  double peak = 0;
  for (float s : v) peak = std::max(peak, (double)std::fabs(s));
  double now = cue_loudness(v, rate);
  if (now <= 1e-9 || peak <= 1e-9) return;
  double g = kCueLoudness / now;
  if (peak * g > kCuePeak) g = kCuePeak / peak;
  for (float& s : v) s *= (float)g;
}

std::vector<float> raw(SonarCue cue, int rate, bool brief) {
  switch (cue) {
    case SonarCue::Exit: return exit_cue(rate, brief);
    case SonarCue::Door: return door_cue(rate, brief);
    case SonarCue::Person: return person_cue(rate, brief);
    case SonarCue::Shop: return shop_cue(rate, brief);
    case SonarCue::Event: return event_cue(rate, brief);
    case SonarCue::Fragment: return fragment_cue(rate, brief);
    case SonarCue::Thing:
    case SonarCue::Count: break;
  }
  return thing_cue(rate, brief);   // Thing, and any value that names no kind
}
}  // namespace

// How loud a cue sounds: its K-weighted level over the loudest 100 ms of it. The ear sums a sound over about a tenth
// of a second, so a tick shorter than that is heard quieter than its level says; the window, run on past the end
// into silence, counts that in. As an RMS (0..1 full scale), not decibels.
double cue_loudness(const std::vector<float>& v, int rate) {
  if (v.empty() || rate <= 0) return 0;
  std::vector<double> k = k_weighted(v, rate);
  const size_t win = (size_t)std::max(1, frames(rate, 0.1)), step = (size_t)std::max(1, frames(rate, 0.002));
  std::vector<double> sq(k.size() + 1, 0.0);   // running sum of squares
  for (size_t i = 0; i < k.size(); ++i) sq[i + 1] = sq[i] + k[i] * k[i];
  double best = 0;
  for (size_t at = 0; at < k.size(); at += step) {
    size_t end = std::min(k.size(), at + win);
    best = std::max(best, (sq[end] - sq[at]) / (double)win);
  }
  return std::sqrt(best);
}

std::vector<float> sonar_cue(SonarCue cue, int rate, bool brief) {
  std::vector<float> v = raw(cue, rate, brief);
  finish(v, rate, 0.14f);
  return v;
}

std::vector<float> signal_cue(SignalCue cue, int rate) {
  std::vector<float> v;
  bool made = false;
  switch (cue) {
    case SignalCue::Shadow: v = shadow_cue(rate); made = true; break;
    case SignalCue::Ambush: v = ambush_cue(rate); made = true; break;
    case SignalCue::Reach: v = reach_cue(rate); made = true; break;
    case SignalCue::Spotted: v = spotted_cue(rate); made = true; break;
    case SignalCue::Lost: v = lost_cue(rate); made = true; break;
    case SignalCue::Layer:
    case SignalCue::Count: break;
  }
  if (!made) v = layer_cue(rate);   // Layer, and any value that names no signal
  // The growl is its own space already (a murmur under it); the room would only muddy it.
  finish(v, rate, cue == SignalCue::Shadow ? 0.0f : 0.12f);
  return v;
}

std::vector<float> file_cue(std::vector<float> v, int rate, bool brief) {
  if (v.empty() || rate <= 0) return {};
  // The silence a file starts and ends with (Grim Dark's entrance cue has 34 ms before it and a quarter second after)
  // would only delay the sound against its rhythm and lengthen it.
  double peak = 0;
  for (float s : v) peak = std::max(peak, (double)std::fabs(s));
  if (peak <= 1e-9) return {};
  const float floor = (float)(peak * 0.02);   // -34 dB under its peak
  size_t first = 0, last = v.size() - 1;
  while (first < last && std::fabs(v[first]) < floor) ++first;
  while (last > first && std::fabs(v[last]) < floor) --last;
  first -= std::min(first, (size_t)frames(rate, 0.002));
  last = std::min(v.size() - 1, last + (size_t)frames(rate, 0.01));
  v = std::vector<float>(v.begin() + (ptrdiff_t)first, v.begin() + (ptrdiff_t)last + 1);
  if (brief) v.resize(std::min(v.size(), (size_t)frames(rate, kFileBriefS)));
  finish(v, rate, 0.0f);   // the file's own space: no room added
  return v;
}

}  // namespace p3a::core

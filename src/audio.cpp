// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#include "audio.h"
#include "paths.h"
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_FLAC
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_DEVICE_IO_CUSTOM_BACKEND
#include <miniaudio.h>
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <format>
#include <memory>
#include <mutex>
#include <vector>
#include "core/sonar_cues.h"
#include "core/spatializer.h"
#include "devserver.h"
#include "hrtf.h"
#include "log.h"

namespace p3a::audio {
namespace {
constexpr int kRate = 48000;
constexpr int kPeriod = 480;   // 10 ms; miniaudio's fixed-size callback always delivers this many frames
constexpr float kPi = 3.14159265358979f;

// Every gain change glides instead of stepping: a volume that jumps between two samples is a click, and the
// drivers (wall tones every frame, 10 ms mixer blocks) change volumes in steps. One pole, ~15 ms time constant.
constexpr float kGlide = 1.0f - 0.99861f;   // 1 - exp(-1 / (0.015 s * 48000))
constexpr float kSilent = 1e-5f;            // below this a gliding voice counts as stopped

// A looping mono sample at a fixed pan (the wotr WallTones channel), on the Walls bus.
struct Loop {
  int id;
  std::vector<float> samples;
  size_t pos = 0;
  float volume = 0;         // requested by the driver
  float cur = 0;            // what plays, gliding toward volume * gain
  float gain = 1.0f;        // static trim (set_loop_gain, not clamped): the wall tones' per-file loudness match
  float gl = 0.70710677f, gr = 0.70710677f;
  ULONGLONG refreshed = 0;  // GetTickCount64 at the last set_loop_volume: a loop nobody drives goes silent
};
constexpr ULONGLONG kLoopStaleMs = 250;  // the game stops ticking us while paused; the loops must not sing on

// The narrator (audio description, src/ad.cpp): one mono sample at a time, centred, on the Narration bus. A new
// one replaces the old; stop fades out (kNarrFade: silent within 70 ms); pause holds the position. `played` counts the frames that reached
// the output, so the caller can tell a clip that played in full from one cut short.
struct Narr {
  int id = -1;
  std::shared_ptr<const std::vector<float>> pcm;
  size_t pos = 0;
  float cur = 0;            // gliding gain (fade in/out)
  bool paused = false;
  bool stopping = false;
  bool ended = false;       // reached the end of the sample
  bool active = false;
};
constexpr float kNarrFade = 1.0f - 0.99653f;   // 1 - exp(-1 / (0.006 s * 48000)): ~20 ms to 5 %, ~70 ms to kSilent

// A one-shot sample from a place (the sonar's sounds): read at `rate` with linear interpolation.
struct Shot {
  Pcm pcm;
  double pos = 0;
  float rate = 1.0f, vol = 1.0f;
  Bus bus = Bus::Cues;
  Spot spot;
  bool spatial = true;    // false once no HRTF slot was free: panned to its end
  int slot = -1;          // the HRTF slot, held one block past the end for the filter's tail
  bool dead = false;
  std::shared_ptr<core::SpatialVoice> stereo;   // Dark Vision's stereo model: its filters' and delay's state
};
constexpr size_t kMaxShots = 32;   // more at once is a runaway caller: further ones are dropped

std::mutex g_mu;
std::vector<Shot> g_shots;
Narr g_narr;
std::vector<Loop> g_loops;
ma_device g_device;
bool g_ready = false;
float g_master = 1.0f;   // the player's Master volume: scales every bus
// Buses: one volume per kind of sound, so each can be set apart (and later exposed as a setting). target is
// requested; cur is where the last block ended, and each block ramps linearly from cur to target.
struct BusState { float target = 1.0f, cur = 1.0f; };
BusState g_bus[(int)Bus::Count] = {{0.6f, 0.6f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {0.8f, 0.8f}};   // settings set them at load
// The output limiter: sums above the ceiling are turned down at once and recover over ~100 ms, instead of being
// clipped (a hard clip of several sounds at once crackles).
constexpr float kCeiling = 0.95f;
constexpr float kRelease = 1.0f - 0.99979f;   // 1 - exp(-1 / (0.1 s * 48000))
float g_limit = 1.0f;
uint32_t g_limited_blocks = 0;   // dev: blocks in which the limiter worked
// 3D sound: positional sounds go through an HRTF slot. Fallbacks (no free slot, or a block of an unexpected size)
// pan instead and are counted for /audio.
bool g_hrtf_on = false;
int g_spatial = 0;   // settings::SpatialModel
constexpr float kPanWidthM = 3.0f;   // Dark Vision's 10 ft: inside it the pan follows the sideways offset
uint32_t g_hrtf_fallbacks = 0;
bool g_odd_block_logged = false;
constexpr int kMaxBlock = 4096;
float g_mono[kMaxBlock], g_ear_l[kMaxBlock], g_ear_r[kMaxBlock];   // audio thread only
uint64_t g_cb_max_us = 0;   // dev: the slowest callback

void pan_gains(float pan, float& l, float& r) {
  float a = (pan + 1.0f) * (kPi / 4.0f);  // equal power
  l = std::cos(a); r = std::sin(a);
}

hrtf::Dir dir_of(const Spot& s) { return {s.right, s.up, s.ahead, s.dist_m}; }
// A slot's output added to the block.
void add_ears(float* o, ma_uint32 frames) {
  for (ma_uint32 i = 0; i < frames; ++i) { o[i * 2] += g_ear_l[i]; o[i * 2 + 1] += g_ear_r[i]; }
}

void data_callback(ma_device*, void* out, const void*, ma_uint32 frames) {
  LARGE_INTEGER t0; QueryPerformanceCounter(&t0);
  float* o = (float*)out;
  for (ma_uint32 i = 0; i < frames * 2; ++i) o[i] = 0;
  std::lock_guard lk(g_mu);
  // HRTF slots need the fixed block size they were made for (miniaudio's fixed-size callback gives it).
  const bool hrtf_block = g_hrtf_on && hrtf::available() && (int)frames == hrtf::frame() && frames <= (ma_uint32)kMaxBlock;
  if (g_hrtf_on && hrtf::available() && !hrtf_block && !g_odd_block_logged) {
    g_odd_block_logged = true;
    log::writef("audio: a {}-frame block (HRTF needs {}); positional sounds pan for such blocks", frames, hrtf::frame());
  }
  // Per-bus gain ramps for this block: from where the last block ended to the target (the master folded in).
  float b_from[(int)Bus::Count], b_step[(int)Bus::Count];
  for (int b = 0; b < (int)Bus::Count; ++b) {
    float target = g_bus[b].target * g_master;
    b_from[b] = g_bus[b].cur;
    b_step[b] = frames ? (target - g_bus[b].cur) / (float)frames : 0.0f;
    g_bus[b].cur = target;
  }
  auto bus_gain = [&](Bus b, ma_uint32 i) { return b_from[(int)b] + b_step[(int)b] * (float)(i + 1); };
  for (Shot& sh : g_shots) {
    if (sh.dead) {
      // The block after a shot ends: its slot plays the filters' tail, then is free again.
      if (sh.slot >= 0) {
        if (hrtf_block) {
          std::fill(g_mono, g_mono + frames, 0.0f);
          hrtf::process(sh.slot, g_mono, dir_of(sh.spot), g_ear_l, g_ear_r);
          add_ears(o, frames);
        }
        hrtf::release(sh.slot);
        sh.slot = -1;
      }
      continue;
    }
    if (sh.spatial && hrtf_block && sh.slot < 0) {
      sh.slot = hrtf::claim();
      if (sh.slot < 0) { ++g_hrtf_fallbacks; sh.spatial = false; }   // all slots busy: this one pans
    }
    if (sh.slot >= 0 && !hrtf_block) { hrtf::release(sh.slot); sh.slot = -1; }   // 3D sound switched off mid-shot
    // Dark Vision's stereo: the model when chosen, and the HRTF's own fallback (no slot, odd block, no phonon).
    const bool stereo = sh.spatial && sh.stereo && sh.slot < 0 && g_spatial != 2 && frames <= (ma_uint32)kMaxBlock;
    const bool ears = sh.slot >= 0 || stereo;
    if (ears) std::fill(g_mono, g_mono + frames, 0.0f);
    float l, r; pan_gains(sh.spot.pan, l, r);
    const std::vector<float>& v = *sh.pcm;
    for (ma_uint32 i = 0; i < frames; ++i) {
      size_t k = (size_t)sh.pos;
      if (k + 1 >= v.size()) { sh.dead = true; break; }
      float frac = (float)(sh.pos - (double)k);
      float s = (v[k] + (v[k + 1] - v[k]) * frac) * sh.vol * bus_gain(sh.bus, i);
      sh.pos += sh.rate;
      if (ears) g_mono[i] = s;
      else { o[i * 2] += s * l; o[i * 2 + 1] += s * r; }
    }
    if (sh.slot >= 0) {
      hrtf::process(sh.slot, g_mono, dir_of(sh.spot), g_ear_l, g_ear_r);
      add_ears(o, frames);
    } else if (stereo) {
      // The listener's own frame: right is east and ahead north in the top-down mode, the camera's otherwise.
      float flat = sh.spot.dist_m * std::sqrt(std::max(0.0f, 1.0f - sh.spot.up * sh.spot.up));
      float horiz = std::hypot(sh.spot.right, sh.spot.ahead);
      float east = horiz > 1e-4f ? sh.spot.right / horiz * flat : 0.0f, north = horiz > 1e-4f ? sh.spot.ahead / horiz * flat : 1.0f;
      if (sh.spot.dist_m <= 0.0f) { east = sh.spot.pan * kPanWidthM; north = sh.spot.ahead; }   // a Spot with no distance: by direction alone
      core::SpatialCue cue = core::spatial_cue(east, north, kPanWidthM, kRate);
      std::fill(g_ear_l, g_ear_l + frames, 0.0f);
      std::fill(g_ear_r, g_ear_r + frames, 0.0f);
      sh.stereo->process(g_mono, (int)frames, cue, kRate, g_ear_l, g_ear_r);
      add_ears(o, frames);
    }
  }
  const ULONGLONG now = GetTickCount64();
  for (Loop& lp : g_loops) {
    if (lp.samples.empty()) continue;
    float target = now - lp.refreshed > kLoopStaleMs ? 0.0f : lp.volume * lp.gain;  // watchdog: silent unless driven
    if (target <= 0.0f && lp.cur < kSilent) { lp.cur = 0; lp.pos = (lp.pos + frames) % lp.samples.size(); continue; }
    for (ma_uint32 i = 0; i < frames; ++i) {
      lp.cur += (target - lp.cur) * kGlide;
      float s = lp.samples[lp.pos] * lp.cur * bus_gain(Bus::Walls, i);  // the loop's own gain, then the Walls bus (the master in it)
      if (++lp.pos >= lp.samples.size()) lp.pos = 0;  // seamless wrap
      o[i * 2] += s * lp.gl; o[i * 2 + 1] += s * lp.gr;
    }
  }
  if (g_narr.active && g_narr.pcm) {
    const auto& v = *g_narr.pcm;
    for (ma_uint32 i = 0; i < frames; ++i) {
      float target = g_narr.stopping || g_narr.paused ? 0.0f : 1.0f;
      g_narr.cur += (target - g_narr.cur) * kNarrFade;
      if (g_narr.paused && g_narr.cur < kSilent) { g_narr.cur = 0; break; }   // hold the position
      if (g_narr.stopping && g_narr.cur < kSilent) { g_narr.active = false; break; }
      if (g_narr.pos >= v.size()) { g_narr.ended = true; g_narr.active = false; break; }
      float s = v[g_narr.pos++] * g_narr.cur * bus_gain(Bus::Narration, i) * 0.70710677f;
      o[i * 2] += s; o[i * 2 + 1] += s;
    }
  }
  bool limited = false;
  for (ma_uint32 i = 0; i < frames; ++i) {
    float peak = std::max(std::abs(o[i * 2]), std::abs(o[i * 2 + 1]));
    if (peak * g_limit > kCeiling) { g_limit = kCeiling / peak; limited = true; }
    o[i * 2] *= g_limit; o[i * 2 + 1] *= g_limit;
    g_limit += (1.0f - g_limit) * kRelease;
  }
  if (limited) ++g_limited_blocks;
  std::erase_if(g_shots, [](const Shot& s) { return s.dead && s.slot < 0; });
  LARGE_INTEGER t1, fq; QueryPerformanceCounter(&t1); QueryPerformanceFrequency(&fq);
  uint64_t us = (uint64_t)((t1.QuadPart - t0.QuadPart) * 1000000 / fq.QuadPart);
  if (us > g_cb_max_us) g_cb_max_us = us;
}
}  // namespace

bool init() {
  if (g_ready) return true;
  ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
  cfg.playback.format = ma_format_f32;
  cfg.playback.channels = 2;
  cfg.sampleRate = kRate;
  cfg.dataCallback = data_callback;
  cfg.periodSizeInFrames = kPeriod;   // noFixedSizedCallback stays false: every callback is exactly this long
  hrtf::init(kRate, kPeriod);         // before the device starts: the callback reads hrtf::available()
  if (ma_device_init(nullptr, &cfg, &g_device) != MA_SUCCESS) { log::write("audio: device init failed"); hrtf::shutdown(); return false; }
  if (ma_device_start(&g_device) != MA_SUCCESS) { log::write("audio: device start failed"); ma_device_uninit(&g_device); hrtf::shutdown(); return false; }
  g_ready = true;
  log::writef("audio: output ready ({} Hz, {}-frame periods)", kRate, cfg.periodSizeInFrames);
  return true;
}
void shutdown() {
  if (!g_ready) return;
  g_ready = false;
  ma_device_uninit(&g_device);   // no callback after this: the HRTF slots can go
  hrtf::shutdown();
  std::lock_guard lk(g_mu);
  g_shots.clear();
  g_loops.clear();
  g_narr = Narr{};
}

// One shot into the mixer, whole, under the lock. `spatial`: heard from `at` (through the HRTF, panned without it);
// false: from no place, both ears alike.
static void push_shot(Pcm pcm, float volume, const Spot& at, Bus bus, float rate, bool spatial) {
  if (!g_ready || !pcm || pcm->size() < 2 || (int)bus < 0 || bus >= Bus::Count) return;   // only the device callback drains g_shots
  std::lock_guard lk(g_mu);
  if (g_shots.size() >= kMaxShots) return;
  Shot s;
  s.pcm = std::move(pcm);
  s.vol = volume < 0 ? 0 : volume;
  s.rate = std::clamp(rate, 0.25f, 4.0f);
  s.bus = bus;
  s.spot = at;
  s.spatial = spatial;
  if (spatial) s.stereo = std::make_shared<core::SpatialVoice>();   // made here: the audio thread allocates nothing
  g_shots.push_back(std::move(s));
}
void play(Pcm pcm, float volume, const Spot& at, Bus bus, float rate) { push_shot(std::move(pcm), volume, at, bus, rate, true); }
int sample_rate() { return kRate; }
// A sound's volume and pitch stay as they started (the sonar's level for its distance, the lower pitch from behind):
// only where it is heard from moves. The HRTF takes a new direction per 10 ms block and blends from the last one.
std::atomic<int> g_followed{0};
void follow(const std::function<Spot(float, float, float)>& where) {
  if (!g_ready) return;
  std::lock_guard lk(g_mu);
  int n = 0;
  for (Shot& s : g_shots) if (!s.dead && s.spot.placed) { s.spot = where(s.spot.wx, s.spot.wy, s.spot.wz); s.spot.placed = true; ++n; }
  g_followed += n;
}
// A signal's samples, made on first use (game thread only). Null for a number that is no signal.
static Pcm signal_pcm(int signal) {
  static Pcm made[(int)core::SignalCue::Count];
  if (signal < 0 || signal >= (int)core::SignalCue::Count) return nullptr;
  if (!made[signal]) made[signal] = std::make_shared<const std::vector<float>>(core::signal_cue((core::SignalCue)signal, kRate));
  return made[signal];
}
void signal(int signal, float volume, const Spot& at, float rate) { push_shot(signal_pcm(signal), volume, at, Bus::Cues, rate, true); }
void signal_here(int signal, float volume) { push_shot(signal_pcm(signal), volume, Spot{}, Bus::Cues, 1.0f, false); }
void set_spatial(int model) { std::lock_guard lk(g_mu); g_spatial = model; g_hrtf_on = model == 1; }

bool render_beep_wav(float freq, int ms, const Spot& at, const std::string& path) {
  // A test beep: sine, ~10 ms smoothing in, 5 ms fade out.
  int n = ms * kRate / 1000;
  std::vector<float> mono(n + 2400);
  float vol = 0, phase = 0, step = 2.0f * kPi * freq / kRate;
  for (size_t i = 0; i < mono.size(); ++i) {
    float target = (int)i < n - 240 ? 1.0f : 0.0f;
    vol += (target - vol) * 0.002f;
    mono[i] = std::sin(phase) * vol;
    phase += step; if (phase > 2.0f * kPi) phase -= 2.0f * kPi;
  }
  std::vector<float> l, r;
  if (!hrtf::render(mono, dir_of(at), l, r)) return false;
  FILE* f = fopen(path.c_str(), "wb");
  if (!f) return false;
  uint32_t frames = (uint32_t)l.size(), data = frames * 4, riff = 36 + data, fmt = 16, rate = kRate, byte_rate = kRate * 4;
  uint16_t pcm = 1, ch = 2, align = 4, bits = 16;
  fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmt, 4, 1, f);
  fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&rate, 4, 1, f); fwrite(&byte_rate, 4, 1, f); fwrite(&align, 2, 1, f); fwrite(&bits, 2, 1, f);
  fwrite("data", 1, 4, f); fwrite(&data, 4, 1, f);
  for (uint32_t i = 0; i < frames; ++i) {
    int16_t s[2] = {(int16_t)std::lround(std::clamp(l[i], -1.0f, 1.0f) * 32767.0f), (int16_t)std::lround(std::clamp(r[i], -1.0f, 1.0f) * 32767.0f)};
    fwrite(s, 2, 2, f);
  }
  fclose(f);
  return true;
}

bool decode_mono(const std::string& path, std::vector<float>& out) {
  ma_decoder_config dc = ma_decoder_config_init(ma_format_f32, 1, kRate);
  ma_decoder dec;
  if (ma_decoder_init_file(path.c_str(), &dc, &dec) != MA_SUCCESS) return false;
  float buf[4096];
  out.clear();
  for (;;) {
    ma_uint64 got = 0;
    ma_result r = ma_decoder_read_pcm_frames(&dec, buf, 4096, &got);
    out.insert(out.end(), buf, buf + got);
    if (r != MA_SUCCESS || got == 0) break;
  }
  ma_decoder_uninit(&dec);
  return !out.empty();
}

int narrate(std::shared_ptr<const std::vector<float>> pcm) {
  static int next_id = 0;
  std::lock_guard lk(g_mu);
  g_narr = Narr{};
  g_narr.id = ++next_id;
  g_narr.pcm = std::move(pcm);
  g_narr.cur = 1.0f;   // the clips start on silence (trimmed 10 ms before the voice): no fade-in needed
  g_narr.active = g_ready && g_narr.pcm && !g_narr.pcm->empty();
  return g_narr.id;
}
void narrate_pause(bool paused) { std::lock_guard lk(g_mu); g_narr.paused = paused; }
void narrate_stop() { std::lock_guard lk(g_mu); if (g_narr.active) g_narr.stopping = true; }
Narration narration() {
  std::lock_guard lk(g_mu);
  return {g_narr.id, g_narr.active, g_narr.paused, g_narr.ended, g_narr.pos, g_narr.pcm ? g_narr.pcm->size() : 0};
}

bool load_loop(int id, const std::string& wav_path, float pan) {
  // Decoded to mono float at the mixer rate (miniaudio resamples; the wotr files are 44.1 kHz).
  std::vector<float> samples;
  if (!decode_mono(wav_path, samples)) { log::writef("audio: cannot open loop '{}', or it decoded to nothing", wav_path); return false; }
  Loop lp{id};
  lp.samples = std::move(samples);
  pan_gains(pan, lp.gl, lp.gr);
  std::lock_guard lk(g_mu);
  std::erase_if(g_loops, [id](const Loop& l) { return l.id == id; });
  g_loops.push_back(std::move(lp));
  log::writef("audio: loop {} = '{}' ({} frames, pan {:.1f})", id, wav_path, g_loops.back().samples.size(), pan);
  return true;
}
void set_loop_volume(int id, float volume) {
  std::lock_guard lk(g_mu);
  for (Loop& l : g_loops) if (l.id == id) { l.volume = volume < 0 ? 0 : volume > 1 ? 1 : volume; l.refreshed = GetTickCount64(); }
}
void set_loop_gain(int id, float gain) {
  std::lock_guard lk(g_mu);
  for (Loop& l : g_loops) if (l.id == id) l.gain = gain < 0 ? 0 : gain;
}
std::string module_dir() { return paths::dir(); }
void set_master_volume(float v) { std::lock_guard lk(g_mu); g_master = v < 0 ? 0 : v > 1 ? 1 : v; }
void set_bus_volume(Bus b, float v) {
  if ((int)b < 0 || b >= Bus::Count) return;
  std::lock_guard lk(g_mu);
  g_bus[(int)b].target = v < 0 ? 0 : v > 1 ? 1 : v;
}
void reset_stats() { std::lock_guard lk(g_mu); g_cb_max_us = 0; g_hrtf_fallbacks = 0; }
std::string status() {
  std::lock_guard lk(g_mu);
  int loud = 0;
  for (const Loop& l : g_loops) if (l.cur >= kSilent) ++loud;
  return std::format("audio ready={} master={:.2f} buses cues={:.2f} walls={:.2f} narration={:.2f} sonar={:.2f} shots={} loops={} ({} audible) limiter={:.2f} limited_blocks={}\n"
                     "hrtf {} slots {}/{} fallbacks={} air={:.1f} callback_max_us={} followed={}\n",
                     g_ready, g_master, g_bus[(int)Bus::Cues].target, g_bus[(int)Bus::Walls].target, g_bus[(int)Bus::Narration].target,
                     g_bus[(int)Bus::Sonar].target, g_shots.size(), g_loops.size(), loud, g_limit, g_limited_blocks,
                     !hrtf::available() ? "unavailable" : g_hrtf_on ? "on" : g_spatial == 0 ? "off (stereo model)" : "off (pan)", hrtf::slots_used(), hrtf::slot_count(), g_hrtf_fallbacks,
                     hrtf::air(), g_cb_max_us, g_followed.load());
}
// Dev routes: the mixer, the process mute, an offline HRTF render.
void install_routes() {
  dev::route("/audiomute", [](const dev::Request& r, int&) {
    bool on = dev::arg(r, "on", "1") == "1";
    return std::format("audio_muted={} ok={}\n", on, audio::mute_process(on));
  }, "?on=1|0 the process's audio session");
  dev::route("/audio", [](const dev::Request& r, int&) {
    const char* names[(int)audio::Bus::Count] = {"cues", "walls", "narration", "sonar"};   // one per bus
    for (int b = 0; b < (int)audio::Bus::Count; ++b)
      if (dev::has(r, names[b])) audio::set_bus_volume((audio::Bus)b, (float)dev::arg_num(r, names[b], 1.0));
    if (dev::has(r, "air")) hrtf::set_air((float)dev::arg_num(r, "air", 10.0));
    if (dev::has(r, "reset")) audio::reset_stats();
    return audio::status();
  }, "?cues=&walls=0..1 &air=N (HRTF air absorption multiplier) &reset=1 the mod's mixer: bus volumes, voices, limiter, HRTF");
  dev::route("/audio/render", [](const dev::Request& r, int&) {
    // A beep through the HRTF, offline, from azimuth az (degrees, 90 = right), elevation elev, dist metres.
    float az = (float)dev::arg_num(r, "az", 0) * 3.14159265f / 180.0f, el = (float)dev::arg_num(r, "elev", 0) * 3.14159265f / 180.0f;
    audio::Spot s{std::sin(az) * std::cos(el), std::sin(el), std::cos(az) * std::cos(el), (float)dev::arg_num(r, "dist", 1), std::sin(az)};
    std::string out = dev::has(r, "out") ? dev::arg(r, "out") : audio::module_dir() + "hrtf_test.wav";
    bool ok = audio::render_beep_wav((float)dev::arg_num(r, "freq", 880), (int)dev::arg_int(r, "ms", 200), s, out);
    return ok ? std::format("wrote {}\n", out) : std::string("HRTF unavailable or the file could not be written\n");
  }, "?az=&elev=&dist=m&freq=&ms=&out=path a beep rendered offline through the HRTF into a stereo WAV (dev check)");
}

}  // namespace p3a::audio

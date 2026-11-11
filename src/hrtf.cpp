#include "hrtf.h"
#include <phonon.h>
#include <windows.h>
#include <atomic>
#include <cmath>
#include <string>
#include "log.h"

namespace p3a::hrtf {
namespace {
constexpr int kSlots = 16;   // 8 until the sonar: its sounds overlap each other and the other cues
// Output trim so the cues are about as loud as when panned (0.707 per ear at the centre). Measured offline
// (2026-10-30, ear RMS averaged over six azimuths against the centre pan): the 880 Hz beacon +2.4 dB, the 392 Hz chime
// +0.9, the 165 Hz Shadow pulse +1.6, the 1320 Hz tick ~0. The HRTF's own response varies by pitch and direction
// (the beacon is +4.6 dB straight ahead), so one trim matches on average only: -1.2 dB.
constexpr float kGain = 0.87f;

struct Slot {
  IPLBinauralEffect bin = nullptr;
  IPLDirectEffect direct = nullptr;
  std::vector<float> a, b;   // mono in, mono after the air filter
  bool busy = false;
};

IPLContext g_ctx = nullptr;
IPLHRTF g_hrtf = nullptr;
IPLAudioSettings g_as{};
Slot g_slot[kSlots];
bool g_ok = false;
// The default (physical) model barely touches the cues' 165-1320 Hz tones: measured on the 880 Hz beacon at 30 m,
// x1 -0.2 dB, x3 -0.8, x10 -2.7 (a 4 kHz triangle: -0.8, -2.3, -6.7). x10 so distance is heard at all.
std::atomic<float> g_air{10.0f};

void IPLCALL on_log(IPLLogLevel level, const char* message) {
  if (level != IPL_LOGLEVEL_DEBUG) log::writef("[steamaudio] {}", message ? message : "");
}

bool make_effects(IPLBinauralEffect& bin, IPLDirectEffect& direct) {
  IPLBinauralEffectSettings bs{g_hrtf};
  if (iplBinauralEffectCreate(g_ctx, &g_as, &bs, &bin) != IPL_STATUS_SUCCESS) return false;
  IPLDirectEffectSettings ds{1};
  if (iplDirectEffectCreate(g_ctx, &g_as, &ds, &direct) != IPL_STATUS_SUCCESS) { iplBinauralEffectRelease(&bin); return false; }
  return true;
}

// The air filter's 3 bands at this distance: the default model's factors raised to the multiplier (each band is
// exp(-k * d), so a power scales k).
void air_bands(float dist_m, float out[3]) {
  IPLAirAbsorptionModel m{};
  m.type = IPL_AIRABSORPTIONTYPE_DEFAULT;
  IPLVector3 src{0, 0, -dist_m}, lis{0, 0, 0};
  iplAirAbsorptionCalculate(g_ctx, src, lis, &m, out);
  float k = g_air.load();
  for (int i = 0; i < 3; ++i) out[i] = k <= 0.0f ? 1.0f : std::pow(out[i], k);
}

// One frame through a pair of effects: air (mono) then binaural (mono -> stereo).
void run(IPLBinauralEffect bin, IPLDirectEffect direct, float* in, float* mid, float* l, float* r, const Dir& d) {
  IPLDirectEffectParams dp{};
  dp.flags = IPL_DIRECTEFFECTFLAGS_APPLYAIRABSORPTION;
  air_bands(d.dist_m, dp.airAbsorption);
  float* inp[1] = {in};
  float* midp[1] = {mid};
  IPLAudioBuffer ib{1, g_as.frameSize, inp}, mb{1, g_as.frameSize, midp};
  iplDirectEffectApply(direct, &dp, &ib, &mb);
  // Listener space: x right, y up, -z ahead. A zero direction (the source at the listener) means ahead.
  float n = std::sqrt(d.right * d.right + d.up * d.up + d.ahead * d.ahead);
  IPLVector3 dir = n > 1e-4f ? IPLVector3{d.right / n, d.up / n, -d.ahead / n} : IPLVector3{0, 0, -1};
  IPLBinauralEffectParams bp{};
  bp.direction = dir;
  bp.interpolation = IPL_HRTFINTERPOLATION_BILINEAR;
  bp.spatialBlend = 1.0f;
  bp.hrtf = g_hrtf;
  float* outp[2] = {l, r};
  IPLAudioBuffer ob{2, g_as.frameSize, outp};
  iplBinauralEffectApply(bin, &bp, &mb, &ob);
  for (int i = 0; i < g_as.frameSize; ++i) { l[i] *= kGain; r[i] *= kGain; }
}
}  // namespace

bool init(int rate, int frame) {
  if (g_ok) return true;
  // phonon.dll sits next to p3access.dll, which is NOT on the game's DLL search path: load it by full path first so
  // the delay-load resolver finds it by name (as speech::init does for prism.dll). No ipl* call before this succeeds.
  HMODULE self = nullptr;
  GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&init, &self);
  wchar_t dir[MAX_PATH]; GetModuleFileNameW(self, dir, MAX_PATH);
  std::wstring p(dir); p = p.substr(0, p.find_last_of(L'\\') + 1) + L"phonon.dll";
  if (!LoadLibraryW(p.c_str())) { log::writef("hrtf: no phonon.dll (err {}); positional cues pan in stereo", GetLastError()); return false; }
  IPLContextSettings cs{};
  cs.version = STEAMAUDIO_VERSION;
  cs.logCallback = on_log;
  cs.simdLevel = IPL_SIMDLEVEL_AVX2;
  if (iplContextCreate(&cs, &g_ctx) != IPL_STATUS_SUCCESS) { log::write("hrtf: Steam Audio context failed"); g_ctx = nullptr; return false; }
  g_as = {rate, frame};
  IPLHRTFSettings hs{};
  hs.type = IPL_HRTFTYPE_DEFAULT;
  hs.volume = 1.0f;
  hs.normType = IPL_HRTFNORMTYPE_RMS;
  if (iplHRTFCreate(g_ctx, &g_as, &hs, &g_hrtf) != IPL_STATUS_SUCCESS) { log::write("hrtf: HRTF failed"); g_hrtf = nullptr; shutdown(); return false; }
  for (Slot& s : g_slot) {
    if (!make_effects(s.bin, s.direct)) { log::write("hrtf: effect creation failed"); shutdown(); return false; }
    s.a.assign(frame, 0.0f); s.b.assign(frame, 0.0f);
    s.busy = false;
  }
  g_ok = true;
  log::writef("hrtf: Steam Audio {}.{}.{} ready ({} Hz, {}-sample frames, {} slots)", STEAMAUDIO_VERSION_MAJOR, STEAMAUDIO_VERSION_MINOR,
              STEAMAUDIO_VERSION_PATCH, rate, frame, kSlots);
  return true;
}

// The mixer stops its device first, so no callback runs into released effects. phonon.dll stays loaded (it starts no
// threads without a simulator), so a hot reload finds it again.
void shutdown() {
  g_ok = false;
  for (Slot& s : g_slot) {
    if (s.bin) iplBinauralEffectRelease(&s.bin);
    if (s.direct) iplDirectEffectRelease(&s.direct);
    s.bin = nullptr; s.direct = nullptr; s.busy = false;
  }
  if (g_hrtf) iplHRTFRelease(&g_hrtf);
  if (g_ctx) iplContextRelease(&g_ctx);
  g_hrtf = nullptr; g_ctx = nullptr;
}

bool available() { return g_ok; }
int frame() { return g_as.frameSize; }

int claim() {
  if (!g_ok) return -1;
  for (int i = 0; i < kSlots; ++i)
    if (!g_slot[i].busy) { g_slot[i].busy = true; return i; }
  return -1;
}
void release(int slot) {
  if (slot < 0 || slot >= kSlots || !g_slot[slot].busy) return;
  Slot& s = g_slot[slot];
  iplBinauralEffectReset(s.bin);
  iplDirectEffectReset(s.direct);
  s.busy = false;
}

void process(int slot, const float* mono, const Dir& d, float* out_l, float* out_r) {
  if (!g_ok || slot < 0 || slot >= kSlots) return;
  Slot& s = g_slot[slot];
  std::copy(mono, mono + g_as.frameSize, s.a.begin());
  run(s.bin, s.direct, s.a.data(), s.b.data(), out_l, out_r, d);
}

int slots_used() {
  int n = 0;
  for (const Slot& s : g_slot) n += s.busy ? 1 : 0;
  return n;
}
int slot_count() { return kSlots; }
void set_air(float mult) { g_air = mult < 0 ? 0 : mult; }
float air() { return g_air.load(); }

bool render(const std::vector<float>& mono, const Dir& d, std::vector<float>& l, std::vector<float>& r) {
  if (!g_ok) return false;
  IPLBinauralEffect bin = nullptr;
  IPLDirectEffect direct = nullptr;
  if (!make_effects(bin, direct)) return false;
  const int f = g_as.frameSize;
  std::vector<float> a(f), b(f), fl(f), fr(f);
  l.clear(); r.clear();
  size_t frames = (mono.size() + f - 1) / f + 2;   // two frames of silence after: the tail
  for (size_t k = 0; k < frames; ++k) {
    for (int i = 0; i < f; ++i) { size_t j = k * f + i; a[i] = j < mono.size() ? mono[j] : 0.0f; }
    run(bin, direct, a.data(), b.data(), fl.data(), fr.data(), d);
    l.insert(l.end(), fl.begin(), fl.end());
    r.insert(r.end(), fr.begin(), fr.end());
  }
  iplBinauralEffectRelease(&bin);
  iplDirectEffectRelease(&direct);
  return true;
}
}  // namespace p3a::hrtf

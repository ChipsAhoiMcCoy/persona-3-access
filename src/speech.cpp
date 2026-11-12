// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#include "speech.h"
#include "paths.h"
#include <atomic>
#include "log.h"
#include "core/text_clean.h"
#include <windows.h>
#include <prism.h>
#include <algorithm>
#include <format>
#include <mutex>
#include <string>

namespace p3a::speech {
static PrismContext* g_ctx;
static PrismBackend* g_backend;
static std::mutex g_mu;
static bool g_muted = false;
static uint64_t g_speak_us_max = 0;   // the slowest backend call (under g_mu)

static void prism_log(void*, PrismLogLevel level, const char* source, const char* message) {
  if (level >= PRISM_LOG_LEVEL_INFO) log::writef("[prism:{}] {}", source ? source : "", message ? message : "");
}

bool init() {
  // prism.dll sits next to p3access.dll, which is NOT on the game's DLL search path; load it by
  // full path first so the delay-load resolver finds the already-loaded module by name.
  std::wstring p = paths::dir_w() + L"prism.dll";
  if (!LoadLibraryW(p.c_str())) { log::writef("speech: failed to load {} (err {})", log::utf8((const char16_t*)p.c_str()), GetLastError()); return false; }

  prism_set_log_handler(PrismLogHandler{prism_log, nullptr});
  prism_set_log_level(PRISM_LOG_LEVEL_INFO);
  PrismConfig cfg = prism_config_init();
  g_ctx = prism_init(&cfg);
  if (!g_ctx) { log::write("speech: prism_init failed"); return false; }
  g_backend = prism_registry_create_best(g_ctx);
  if (!g_backend) { log::write("speech: no usable backend"); return false; }
  PrismError e = prism_backend_initialize(g_backend);
  if (e != PRISM_OK && e != PRISM_ERROR_ALREADY_INITIALIZED) {
    log::writef("speech: backend_initialize failed: {}", (int)e);
    prism_backend_free(g_backend);   // speak/stop check only g_backend: never call a backend that did not start
    g_backend = nullptr;
    return false;
  }
  log::writef("speech: using backend '{}'{}", backend_name(), g_muted ? " (muted)" : "");
  return true;
}

const char* backend_name() { return g_backend ? prism_backend_name(g_backend) : "(none)"; }

static std::atomic<uint64_t> g_count{0};
uint64_t count() { return g_count; }
void speak(std::string_view utf8, bool interrupt) {
  ++g_count;
  std::string s = core::tidy_joins(utf8);   // no ".," where the mod joined a position or a description on
  log::writef("[speak{}] {}", interrupt ? "!" : "", s);
  history().push(s);
  std::lock_guard lk(g_mu);
  if (!g_backend || g_muted) return;
  // The call runs on the game thread (a screen reader's RPC): its worst time shows in /health, a slow one in the log.
  LARGE_INTEGER t0, t1, f;
  QueryPerformanceCounter(&t0);
  PrismError e = prism_backend_speak(g_backend, s.c_str(), interrupt);
  QueryPerformanceCounter(&t1);
  QueryPerformanceFrequency(&f);
  uint64_t us = (uint64_t)((t1.QuadPart - t0.QuadPart) * 1000000 / f.QuadPart);
  g_speak_us_max = (std::max)(g_speak_us_max, us);
  if (us > 8000) log::writef("speech: the backend took {} ms", us / 1000);
  if (e != PRISM_OK) log::writef("speech: speak failed: {}", (int)e);
}
std::string timing() { return std::format("speak_max_us={}", g_speak_us_max); }
// Dev: the backend's cost for one interrupting line, timed with a single space (the screen reader says nothing), even
// while muted. Returns each call's microseconds.
std::vector<uint64_t> bench(int n) {
  std::vector<uint64_t> out;
  LARGE_INTEGER f;
  QueryPerformanceFrequency(&f);
  n = std::clamp(n, 0, 200);
  for (int i = 0; i < n; ++i) {
    {
      std::lock_guard lk(g_mu);   // per call: the game thread's speak() never waits out the whole bench
      if (!g_backend) break;
      LARGE_INTEGER t0, t1;
      QueryPerformanceCounter(&t0);
      prism_backend_speak(g_backend, " ", true);
      QueryPerformanceCounter(&t1);
      out.push_back((uint64_t)((t1.QuadPart - t0.QuadPart) * 1000000 / f.QuadPart));
    }
    Sleep(30);
  }
  return out;
}

void stop() { std::lock_guard lk(g_mu); if (g_backend && !g_muted) prism_backend_stop(g_backend); }
int speaking() {
  std::lock_guard lk(g_mu);
  bool on = false;
  if (!g_backend || g_muted || prism_backend_is_speaking(g_backend, &on) != PRISM_OK) return -1;
  return on ? 1 : 0;
}
void set_muted(bool m) { g_muted = m; }
bool muted() { return g_muted; }
// Built on first use: a line spoken from another file's static initializer finds it ready.
LineRing& history() { static LineRing r(500); return r; }

void shutdown() {
  std::lock_guard lk(g_mu);
  if (g_backend) { prism_backend_free(g_backend); g_backend = nullptr; }
  if (g_ctx) { prism_shutdown(g_ctx); g_ctx = nullptr; }
}
}  // namespace p3a::speech

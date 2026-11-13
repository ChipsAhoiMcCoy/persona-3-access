// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#include <windows.h>
#include "paths.h"
#include <format>
#include "ad.h"
#include "audio.h"
#include "crash.h"
#include "devserver.h"
#include "hooks.h"
#include "input.h"
#include "log.h"
#include "mod.h"
#include "nav.h"
#include "speech.h"
#include "ue/ue.h"

namespace p3a {
bool env_flag(const wchar_t* name) {
  wchar_t v[8];
  return GetEnvironmentVariableW(name, v, 8) > 0 && v[0] == L'1';
}
int env_int(const wchar_t* name, int def) {
  wchar_t v[16];
  return GetEnvironmentVariableW(name, v, 16) > 0 ? _wtoi(v) : def;
}
// The loaded copy's path and link time: the dev client checks them to confirm a hot reload took.
static HMODULE self_module() { return paths::self(); }
static std::string module_path() {
  wchar_t p[MAX_PATH]; DWORD n = GetModuleFileNameW(self_module(), p, MAX_PATH);
  return log::utf8(std::u16string_view((const char16_t*)p, n));
}
static uint32_t build_stamp() {
  auto* b = (const uint8_t*)self_module();
  return ((const IMAGE_NT_HEADERS*)(b + ((const IMAGE_DOS_HEADER*)b)->e_lfanew))->FileHeader.TimeDateStamp;
}
}  // namespace p3a

using namespace p3a;

static void base_routes() {
  dev::route("/health", [](const dev::Request&, int&) {
    return std::format("ok pid={} backend={} speech_muted={} {} {}\nmodule={} built={:#x}\n{}", GetCurrentProcessId(), speech::backend_name(),
                       speech::muted(), hooks::counters(), speech::timing(), module_path(), build_stamp(), mod::status()) + input::status() + "\n";
  }, "liveness and counters");
  auto ring_route = [](LineRing& ring) {
    return [&ring](const dev::Request& r, int&) {
      auto [lines, cur] = ring.since((uint64_t)dev::arg_int(r, "since", 0));
      std::string out = std::format("cursor: {}\n", cur);
      for (auto& [i, l] : lines) out += std::format("{}: {}\n", i, l);
      return out;
    };
  };
  dev::route("/speech", ring_route(speech::history()), "?since=N spoken lines");
  dev::route("/speech/bench", [](const dev::Request& r, int&) {
    std::string out;
    for (uint64_t us : speech::bench((int)dev::arg_int(r, "n", 20))) out += std::format("{} ", us);
    return out + "\n";
  }, "?n=20 time the speech backend's interrupting call (a single space, silent; dev)");
  dev::route("/log", ring_route(log::ring()), "?since=N mod log lines");
  dev::route("/say", [](const dev::Request& r, int&) { speech::speak(dev::has(r, "text") ? dev::arg(r, "text") : r.body, dev::arg(r, "interrupt", "1") == "1"); return std::string("ok\n"); }, "?text= speak");
  dev::route("/mute", [](const dev::Request& r, int&) {
    bool on = dev::arg(r, "on", "1") == "1";
    speech::set_muted(on);
    if (!on) SetEnvironmentVariableW(L"P3A_MUTE", nullptr);
    return std::format("speech_muted={}\n", speech::muted());
  }, "?on=1|0 mod speech mute (history still records)");
  dev::route("/env", [](const dev::Request& r, int&) {
    // /env?name=P3A_VPAD&value=1 set an environment variable in the game process (read by the next hot reload)
    std::string n = dev::arg(r, "name"), v = dev::arg(r, "value");
    std::u16string wn = ue::to_u16(n), wv = ue::to_u16(v);
    SetEnvironmentVariableW((LPCWSTR)wn.c_str(), v.empty() ? nullptr : (LPCWSTR)wv.c_str());
    return std::format("{}={}\n", n, v);
  }, "?name=&value= process environment for the next reload (dev)");
  audio::install_routes();
  dev::route("/loadlib", [](const dev::Request& r, int&) {
    std::string p = dev::arg(r, "path");
    std::u16string w = ue::to_u16(p);
    HMODULE m = LoadLibraryW((LPCWSTR)w.c_str());
    return std::format("LoadLibrary -> {} (err {})\n", (void*)m, m ? 0 : GetLastError());
  }, "?path= LoadLibrary in the game (dev tools)");
}

static bool g_up = false;
static HANDLE g_init = nullptr;   // the init thread: unload waits for it (its code is in this DLL)
static HANDLE g_quit = nullptr;   // set by unload: ends the init thread's wait early

static DWORD WINAPI init_thread(LPVOID) {
  log::init();
  log::write("p3access: loaded");
  crash::install();
  bool mute = env_flag(L"P3A_MUTE");
  speech::set_muted(mute);
  bool sp = speech::init();
  hooks::install();
  input::install();
  base_routes();
  if (GetEnvironmentVariableW(L"P3A_PORT", nullptr, 0) > 0) dev::start(env_int(L"P3A_PORT", 8793));
  g_up = true;
  mod::init(sp);
  // Windows keeps an app's mixer mute per executable across launches, so a dev launch's mute (P3A_MUTE) muted the
  // player's later launches of P3R.exe, game and mod sounds alike (2026-10-29). Every other launch clears it.
  if (WaitForSingleObject(g_quit, 4000) == WAIT_TIMEOUT) audio::mute_process(mute);
  return 0;
}

// Orderly shutdown, called by the dev injector on a remote thread BEFORE FreeLibrary (never from DllMain: joining
// the server thread under the loader lock deadlocks).
extern "C" __declspec(dllexport) DWORD WINAPI p3a_unload(LPVOID) {
  log::write("p3access: unloading");
  SetEvent(g_quit);
  if (g_init && WaitForSingleObject(g_init, 5000) == WAIT_TIMEOUT) log::write("p3access: the init thread is still running");
  if (!g_up) return 1;
  nav::unload();   // before the key filter goes: the walk's held keys are let go
  ad::unload();    // the clip decoders' worker threads end before their code goes
  // The server first, so no request runs mod code while it is torn down; cancelling releases handlers that wait
  // on the game thread.
  hooks::cancel_pending();
  dev::stop();
  mod::shutdown();
  input::remove();
  // Every detour still in: what a module did not remove itself (a stale detour into an unloaded DLL corrupts the
  // heap), then the Win32 layer.
  hooks::detach_all();
  crash::remove();
  speech::shutdown();
  Sleep(1000);   // let a hook body another thread was already running finish before the unmap (250 ms was not always enough)
  log::write("p3access: ready for FreeLibrary");
  return 1;
}

BOOL WINAPI DllMain(HMODULE h, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(h);
    g_quit = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_init = CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr);
  }
  return TRUE;
}

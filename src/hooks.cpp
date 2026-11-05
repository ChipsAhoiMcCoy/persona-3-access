// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#include "hooks.h"
#include "log.h"
#include "seh.h"
#include <detours.h>
#include <tlhelp32.h>
#include <intrin.h>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

namespace {
// Every Detours transaction updates EVERY thread, not just the caller: Detours moves a suspended thread's
// instruction pointer out of a prologue being rewritten, and the game thread runs our hooks every frame.
// (Grim Dark lesson: hot reloads from a remote thread crashed when only the calling thread was updated.)
struct ThreadUpdater {
  std::vector<HANDLE> handles;
  ThreadUpdater() {
    DetourUpdateThread(GetCurrentThread());
    DWORD pid = GetCurrentProcessId(), me = GetCurrentThreadId();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    THREADENTRY32 te{}; te.dwSize = sizeof te;
    for (BOOL ok = Thread32First(snap, &te); ok; ok = Thread32Next(snap, &te)) {
      if (te.th32OwnerProcessID != pid || te.th32ThreadID == me) continue;
      HANDLE h = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
      if (!h) continue;
      if (DetourUpdateThread(h) == NO_ERROR) handles.push_back(h); else CloseHandle(h);
    }
    CloseHandle(snap);
  }
  ~ThreadUpdater() { for (HANDLE h : handles) CloseHandle(h); }
};
}  // namespace

namespace p3a::hooks {
static std::mutex g_tx_mu;

// Every live attach, so unload can remove whatever a module forgot (a CreateBmdPage hook left behind by a hot
// reload sent the game into the next DLL's code and corrupted the heap, 2026-10-27).
struct Attached { void** orig; void* detour; const char* label; };
static std::vector<Attached> g_attached;

bool attach(void** orig, void* detour, const char* label) {
  std::lock_guard lk(g_tx_mu);
  if (!*orig) { log::writef("hook {}: no target", label); return false; }
  DetourTransactionBegin();
  ThreadUpdater threads;
  LONG r = DetourAttach(orig, detour);
  LONG c = DetourTransactionCommit();
  log::writef("hook {}: attach={} commit={}", label, r, c);
  bool ok = r == NO_ERROR && c == NO_ERROR;
  if (ok) g_attached.push_back(Attached{orig, detour, label});
  return ok;
}
void detach(void** orig, void* detour) {
  std::lock_guard lk(g_tx_mu);
  auto it = std::find_if(g_attached.begin(), g_attached.end(), [&](const Attached& a) { return a.orig == orig && a.detour == detour; });
  if (it == g_attached.end()) return;   // never attached (or already removed): Detours must not see it
  g_attached.erase(it);
  DetourTransactionBegin();
  ThreadUpdater threads;
  DetourDetach(orig, detour);
  DetourTransactionCommit();
}
void detach_all() {
  std::lock_guard lk(g_tx_mu);
  if (g_attached.empty()) return;
  DetourTransactionBegin();
  ThreadUpdater threads;
  for (auto& a : g_attached) { DetourDetach(a.orig, a.detour); log::writef("hook {}: detached at unload", a.label); }
  g_attached.clear();
  DetourTransactionCommit();
}

// ======================= game-thread job pump =======================
static std::mutex g_jobs_mu;
static std::deque<std::function<void()>> g_jobs;
static std::atomic<uint64_t> g_frame{0};
static std::atomic<DWORD> g_game_tid{0};
static std::atomic<ULONGLONG> g_last_tick_ms{0};
static std::atomic<bool> g_cancel{false};
static thread_local unsigned t_timeouts = 0;
uint64_t frame() { return g_frame; }
unsigned ms_since_tick() { ULONGLONG t = g_last_tick_ms; return t ? (unsigned)(GetTickCount64() - t) : ~0u; }
unsigned timeouts_on_this_thread() { return t_timeouts; }
void cancel_pending() { g_cancel = true; }
static void set_game_thread(DWORD tid) { g_game_tid = tid; }
bool on_game_thread() { return g_game_tid && GetCurrentThreadId() == g_game_tid; }

bool run_on_game_thread(std::function<void()> fn, unsigned timeout_ms) {
  if (on_game_thread()) { fn(); return true; }
  if (g_cancel) { ++t_timeouts; return false; }
  struct State { std::mutex m; std::condition_variable cv; bool done = false; std::function<void()> fn; };
  auto st = std::make_shared<State>();
  st->fn = std::move(fn);
  {
    std::lock_guard lk(g_jobs_mu);
    g_jobs.push_back([st] { if (st->fn) st->fn(); { std::lock_guard l2(st->m); st->done = true; } st->cv.notify_all(); });
  }
  // Waits in slices so an unload (cancel_pending) releases the dev server's handlers at once.
  std::unique_lock lk(st->m);
  auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (!st->done && !g_cancel && std::chrono::steady_clock::now() < until) st->cv.wait_for(lk, std::chrono::milliseconds(100));
  if (!st->done) ++t_timeouts;
  return st->done;
}

static std::atomic<void (*)()> g_tick_cb{nullptr};
void set_tick_callback(void (*cb)()) { g_tick_cb = cb; }
// The mod's own cost per frame (it runs inside the game's frame, so a slow tick is a game hitch): totals since load,
// the worst tick, and the count of ticks over 8 ms (/health).
static std::atomic<uint64_t> g_tick_us_total{0}, g_tick_us_max{0}, g_slow_ticks{0};
void tick() {
  LARGE_INTEGER t0, t1, qpf; QueryPerformanceCounter(&t0); QueryPerformanceFrequency(&qpf);
  struct Timer { LARGE_INTEGER& t0; LARGE_INTEGER& t1; LARGE_INTEGER& qpf; ~Timer() {
    QueryPerformanceCounter(&t1);
    uint64_t us = (uint64_t)((t1.QuadPart - t0.QuadPart) * 1000000 / qpf.QuadPart);
    g_tick_us_total += us;
    if (us > g_tick_us_max) g_tick_us_max = us;
    if (us > 8000) ++g_slow_ticks;
  } } timer{t0, t1, qpf};
  ++g_frame;
  g_last_tick_ms = GetTickCount64();
  std::deque<std::function<void()>> jobs;
  { std::lock_guard lk(g_jobs_mu); jobs.swap(g_jobs); }
  // A fault in the mod (a stale or misread game object) must not take the game down: jobs and the mod's tick run
  // under the access-violation guard and a fault is logged (a dev route reading the wrong object as a DataTable
  // crashed the game on the game thread, 2026-10-27).
  for (auto& j : jobs) if (!guarded([&] { j(); })) log::write("hooks: a game-thread job faulted");
  if (auto cb = g_tick_cb.load()) {
    static ULONGLONG last_fault_log = 0;
    if (!guarded([&] { cb(); }) && GetTickCount64() - last_fault_log > 5000) { last_fault_log = GetTickCount64(); log::write("hooks: the mod tick faulted"); }
  }
}

// ======================= the game window =======================
static HWND g_window;
static BOOL CALLBACK find_window_cb(HWND h, LPARAM) {
  DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
  if (pid != GetCurrentProcessId() || GetWindow(h, GW_OWNER)) return TRUE;
  wchar_t cls[64]; GetClassNameW(h, cls, 64);
  if (wcscmp(cls, L"UnrealWindow") == 0) { g_window = h; return FALSE; }
  return TRUE;
}
HWND game_window() {
  if (g_window && IsWindow(g_window)) return g_window;
  g_window = nullptr;
  EnumWindows(find_window_cb, 0);
  return g_window;
}

// ======================= dev mode: never let the game take the foreground =======================
// The dev loop runs the game visible but unfocused so the developer's screen reader is never interrupted. The
// game's own code (P3R.exe) is told its window is active, or Unreal treats itself as backgrounded (no input,
// muted audio, throttled ticking); every other module (Steam overlay, XInput, the IME) sees the truth.
static bool g_nofocus = false;
static bool caller_is_game(void* ret) {
  static HMODULE exe = GetModuleHandleW(nullptr);
  HMODULE m = nullptr;
  GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)ret, &m);
  return m == exe;
}
typedef HWND(WINAPI* HwndFn)();
static HwndFn GetForegroundWindow_orig = GetForegroundWindow, GetActiveWindow_orig = GetActiveWindow, GetFocus_orig = GetFocus;
static HWND WINAPI GetForegroundWindow_hook() { HWND r = GetForegroundWindow_orig(); if (g_nofocus && caller_is_game(_ReturnAddress())) { if (HWND g = game_window()) return g; } return r; }
static HWND WINAPI GetActiveWindow_hook() { HWND r = GetActiveWindow_orig(); if (g_nofocus && caller_is_game(_ReturnAddress())) { if (HWND g = game_window()) return g; } return r; }
static HWND WINAPI GetFocus_hook() { HWND r = GetFocus_orig(); if (g_nofocus && caller_is_game(_ReturnAddress())) { if (HWND g = game_window()) return g; } return r; }

static BOOL(WINAPI* SetForegroundWindow_orig)(HWND) = SetForegroundWindow;
static HWND(WINAPI* SetActiveWindow_orig)(HWND) = SetActiveWindow;
static HWND(WINAPI* SetFocus_orig)(HWND) = SetFocus;
static BOOL(WINAPI* BringWindowToTop_orig)(HWND) = BringWindowToTop;
static BOOL(WINAPI* ShowWindow_orig)(HWND, int) = ShowWindow;
static BOOL(WINAPI* SetWindowPos_orig)(HWND, HWND, int, int, int, int, UINT) = SetWindowPos;
static BOOL WINAPI SetForegroundWindow_hook(HWND h) { if (g_nofocus) return TRUE; return SetForegroundWindow_orig(h); }
static HWND WINAPI SetActiveWindow_hook(HWND h) { if (g_nofocus) return h; return SetActiveWindow_orig(h); }
static HWND WINAPI SetFocus_hook(HWND h) { if (g_nofocus && caller_is_game(_ReturnAddress())) return h; return SetFocus_orig(h); }
static BOOL WINAPI BringWindowToTop_hook(HWND h) { if (g_nofocus) return TRUE; return BringWindowToTop_orig(h); }
static BOOL WINAPI ShowWindow_hook(HWND h, int cmd) {
  if (g_nofocus && (cmd == SW_SHOW || cmd == SW_SHOWNORMAL || cmd == SW_RESTORE || cmd == SW_SHOWMAXIMIZED || cmd == SW_SHOWDEFAULT)) cmd = SW_SHOWNOACTIVATE;
  return ShowWindow_orig(h, cmd);
}
static BOOL WINAPI SetWindowPos_hook(HWND h, HWND after, int x, int y, int cx, int cy, UINT f) {
  if (g_nofocus && !(f & SWP_NOACTIVATE)) { f |= SWP_NOACTIVATE; if (after == HWND_TOPMOST || after == HWND_TOP) f |= SWP_NOZORDER; }
  return SetWindowPos_orig(h, after, x, y, cx, cy, f);
}

// ======================= low-level keyboard hooks =======================
// A WH_KEYBOARD_LL hook in a busy game process gets the screen reader's own low-level hook timed out and removed by
// Windows (Grim Dark, 2026-08-21: NVDA keys dead after alt-tab). Unreal does not need one; refuse any the game
// (or a library it loads) installs, and swallow the accessibility-shortcut toggles games like to flip.
static const HHOOK kFakeHook = (HHOOK)(uintptr_t)0x6D6F6E6B686F6F6BULL;
static HHOOK(WINAPI* SetWindowsHookExW_orig)(int, HOOKPROC, HINSTANCE, DWORD) = SetWindowsHookExW;
static HHOOK(WINAPI* SetWindowsHookExA_orig)(int, HOOKPROC, HINSTANCE, DWORD) = SetWindowsHookExA;
static BOOL(WINAPI* UnhookWindowsHookEx_orig)(HHOOK) = UnhookWindowsHookEx;
static BOOL(WINAPI* SystemParametersInfoW_orig)(UINT, UINT, PVOID, UINT) = SystemParametersInfoW;
static HHOOK WINAPI SetWindowsHookExW_hook(int id, HOOKPROC fn, HINSTANCE mod, DWORD tid) {
  if (id == WH_KEYBOARD_LL) { log::write("blocked: WH_KEYBOARD_LL hook (W)"); return kFakeHook; }
  return SetWindowsHookExW_orig(id, fn, mod, tid);
}
static HHOOK WINAPI SetWindowsHookExA_hook(int id, HOOKPROC fn, HINSTANCE mod, DWORD tid) {
  if (id == WH_KEYBOARD_LL) { log::write("blocked: WH_KEYBOARD_LL hook (A)"); return kFakeHook; }
  return SetWindowsHookExA_orig(id, fn, mod, tid);
}
static BOOL WINAPI UnhookWindowsHookEx_hook(HHOOK h) { return h == kFakeHook ? TRUE : UnhookWindowsHookEx_orig(h); }
static BOOL WINAPI SystemParametersInfoW_hook(UINT a, UINT p, PVOID pv, UINT f) {
  if (a == SPI_SETSTICKYKEYS || a == SPI_SETTOGGLEKEYS || a == SPI_SETFILTERKEYS) { log::writef("blocked: SystemParametersInfoW({:#x})", a); return TRUE; }
  return SystemParametersInfoW_orig(a, p, pv, f);
}

// ======================= the per-frame pump =======================
// Unreal pumps the window's messages on the game thread at the top of every FEngineLoop::Tick, whatever the
// game is showing; ProcessEvent can go quiet for seconds on a static screen (measured 2026-10-27 on the network
// notice), so the message pump is the mod's frame clock.
static LARGE_INTEGER g_qpf, g_last;

// Dev no-focus mode: P3R pauses its game logic on WM_ACTIVATE(inactive) / WM_ACTIVATEAPP(false) / WM_KILLFOCUS
// (the same messages p3rpc.essentials' NoPauseOnFocusLoss swallows). They are SENT, so they never pass the
// message pump: detour the game's own window procedure (in P3R.exe) and drop them, then tell the game it is active
// once. A Detours hook, not SetWindowLongPtr: a subclass outlives a hot reload and the window then calls into the
// unloaded DLL (crashed the game 2026-10-27); the detour is removed with the other hooks.
// The same window-procedure detour is the mod's keyboard: Unreal reads keys from these messages on the game thread,
// so a key the mod binds is handled here (on the game thread, where game objects may be touched) and swallowed.
using WndProc_t = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);
static WndProc_t g_wndproc_orig;
static bool g_wndproc_hooked;
static std::atomic<KeyFilter> g_key_filter{nullptr};
void set_key_filter(KeyFilter f) { g_key_filter = f; }
static std::atomic<uint64_t> g_key_ms{0};
uint64_t key_ms() { return g_key_ms; }
static LRESULT CALLBACK wndproc_hook(HWND h, UINT m, WPARAM w, LPARAM l) {
  if (g_nofocus && ((m == WM_ACTIVATE && LOWORD(w) == WA_INACTIVE) || (m == WM_ACTIVATEAPP && !w) || m == WM_KILLFOCUS)) return 0;
  if (m == WM_KEYDOWN || m == WM_SYSKEYDOWN) g_key_ms = GetTickCount64();
  if (m == WM_KEYDOWN || m == WM_KEYUP || m == WM_SYSKEYDOWN || m == WM_SYSKEYUP || m == WM_CHAR) {
    if (KeyFilter f = g_key_filter.load(); f && f(m, w, l)) return 0;
  }
  return g_wndproc_orig(h, m, w, l);
}
static void hook_wndproc(HWND w) {
  if (g_wndproc_hooked) return;
  g_wndproc_hooked = true;
  g_wndproc_orig = (WndProc_t)GetWindowLongPtrW(w, GWLP_WNDPROC);
  if (!attach((void**)&g_wndproc_orig, (void*)&wndproc_hook, "game WndProc")) return;
  if (g_nofocus) {
    wndproc_hook(w, WM_ACTIVATEAPP, TRUE, 0);
    wndproc_hook(w, WM_ACTIVATE, WA_ACTIVE, 0);
    log::write("nofocus: focus-loss pause disabled");
  }
}

void pump() {
  if (!g_game_tid) {
    HWND w = game_window();
    if (!w) return;
    set_game_thread(GetWindowThreadProcessId(w, nullptr));
  }
  if (GetCurrentThreadId() != g_game_tid) return;
  if (!g_wndproc_hooked) if (HWND w = game_window()) hook_wndproc(w);
  static thread_local bool inside = false;
  if (inside) return;   // a job that pumps messages itself must not re-enter
  if (!g_qpf.QuadPart) QueryPerformanceFrequency(&g_qpf);
  LARGE_INTEGER now; QueryPerformanceCounter(&now);
  if (now.QuadPart - g_last.QuadPart < g_qpf.QuadPart / 120) return;
  g_last = now;
  inside = true;
  tick();
  inside = false;
}
static BOOL(WINAPI* PeekMessageW_orig)(LPMSG, HWND, UINT, UINT, UINT) = PeekMessageW;
static BOOL WINAPI PeekMessageW_hook(LPMSG m, HWND h, UINT lo, UINT hi, UINT rm) {
  pump();
  return PeekMessageW_orig(m, h, lo, hi, rm);
}

struct W32Hook { void** orig; void* detour; const char* name; bool ok; };
static std::vector<W32Hook> g_w32;

bool install() {
  { wchar_t v[4]; g_nofocus = GetEnvironmentVariableW(L"P3A_NOFOCUS", v, 4) > 0 && v[0] == L'1'; }
  g_w32 = {
    {(void**)&SetWindowsHookExW_orig, (void*)&SetWindowsHookExW_hook, "SetWindowsHookExW", false},
    {(void**)&SetWindowsHookExA_orig, (void*)&SetWindowsHookExA_hook, "SetWindowsHookExA", false},
    {(void**)&UnhookWindowsHookEx_orig, (void*)&UnhookWindowsHookEx_hook, "UnhookWindowsHookEx", false},
    {(void**)&SystemParametersInfoW_orig, (void*)&SystemParametersInfoW_hook, "SystemParametersInfoW", false},
    {(void**)&GetForegroundWindow_orig, (void*)&GetForegroundWindow_hook, "GetForegroundWindow", false},
    {(void**)&GetActiveWindow_orig, (void*)&GetActiveWindow_hook, "GetActiveWindow", false},
    {(void**)&GetFocus_orig, (void*)&GetFocus_hook, "GetFocus", false},
    {(void**)&SetForegroundWindow_orig, (void*)&SetForegroundWindow_hook, "SetForegroundWindow", false},
    {(void**)&SetActiveWindow_orig, (void*)&SetActiveWindow_hook, "SetActiveWindow", false},
    {(void**)&SetFocus_orig, (void*)&SetFocus_hook, "SetFocus", false},
    {(void**)&BringWindowToTop_orig, (void*)&BringWindowToTop_hook, "BringWindowToTop", false},
    {(void**)&ShowWindow_orig, (void*)&ShowWindow_hook, "ShowWindow", false},
    {(void**)&SetWindowPos_orig, (void*)&SetWindowPos_hook, "SetWindowPos", false},
    {(void**)&PeekMessageW_orig, (void*)&PeekMessageW_hook, "PeekMessageW", false},
  };
  std::lock_guard lk(g_tx_mu);
  DetourTransactionBegin();
  ThreadUpdater threads;
  for (auto& h : g_w32) h.ok = DetourAttach(h.orig, h.detour) == NO_ERROR;
  LONG r = DetourTransactionCommit();
  log::writef("hooks: win32 layer commit={} nofocus={}", r, g_nofocus);
  return r == NO_ERROR;
}

void remove() {
  std::lock_guard lk(g_tx_mu);
  DetourTransactionBegin();
  ThreadUpdater threads;
  for (auto& h : g_w32) if (h.ok) { DetourDetach(h.orig, h.detour); h.ok = false; }
  DetourTransactionCommit();
}

std::string counters() {
  unsigned since = ms_since_tick();
  uint64_t f = g_frame.load();
  return std::format("frame={} last_tick_ms={} ticking={} game_tid={} window={} tick_avg_us={} tick_max_us={} slow_ticks={}", f, (int)since, since < 2000, g_game_tid.load(), (void*)game_window(), f ? g_tick_us_total.load() / f : 0, g_tick_us_max.load(), g_slow_ticks.load());
}
}  // namespace p3a::hooks

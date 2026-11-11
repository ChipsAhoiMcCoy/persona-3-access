#include "input.h"
#include "devserver.h"
#include "hooks.h"
#include "keys.h"
#include "settings.h"
#include "core/strings.h"
#include "log.h"
#include <windows.h>
#include <xinput.h>
#include <deque>
#include <vector>
#include <format>
#include <mutex>
#include <atomic>
#include <map>
#include <intrin.h>
#include <cstdlib>

namespace p3a::input {
struct Step { uint32_t mask; float lx, ly; unsigned hold_ms, gap_ms; float ry = 0; };
static std::mutex g_mu;
static std::deque<Step> g_queue;
static uint64_t g_step_start = 0;   // when the front step began (0 = not started)
static uint32_t g_latched = 0;
static bool g_vpad = false;
static uint16_t g_real_buttons = 0, g_swallow = 0;
static bool g_real_connected = false;
static uint64_t g_polls = 0, g_synth_polls = 0;

using GetState_t = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
static GetState_t g_get_state;

uint16_t button_mask(std::string_view n) {
  if (n == "a") return XINPUT_GAMEPAD_A; if (n == "b") return XINPUT_GAMEPAD_B;
  if (n == "x") return XINPUT_GAMEPAD_X; if (n == "y") return XINPUT_GAMEPAD_Y;
  if (n == "up") return XINPUT_GAMEPAD_DPAD_UP; if (n == "down") return XINPUT_GAMEPAD_DPAD_DOWN;
  if (n == "left") return XINPUT_GAMEPAD_DPAD_LEFT; if (n == "right") return XINPUT_GAMEPAD_DPAD_RIGHT;
  if (n == "start") return XINPUT_GAMEPAD_START; if (n == "back") return XINPUT_GAMEPAD_BACK;
  if (n == "lb") return XINPUT_GAMEPAD_LEFT_SHOULDER; if (n == "rb") return XINPUT_GAMEPAD_RIGHT_SHOULDER;
  if (n == "ls") return XINPUT_GAMEPAD_LEFT_THUMB; if (n == "rs") return XINPUT_GAMEPAD_RIGHT_THUMB;
  return 0;
}
static uint32_t name_mask(std::string_view n) {
  if (n == "lt") return 0x10000; if (n == "rt") return 0x20000;
  return button_mask(n);
}

// The current synthetic state, advancing the queue by wall-clock time.
static void synth(uint32_t& mask, float& lx, float& ly, float& ry) {
  std::lock_guard lk(g_mu);
  mask = g_latched; lx = ly = ry = 0;
  uint64_t now = GetTickCount64();
  while (!g_queue.empty()) {
    Step& s = g_queue.front();
    if (!g_step_start) g_step_start = now;
    uint64_t t = now - g_step_start;
    if (t < s.hold_ms) { mask |= s.mask; lx = s.lx; ly = s.ly; ry = s.ry; return; }
    if (t < (uint64_t)s.hold_ms + s.gap_ms) return;
    g_queue.pop_front();
    g_step_start = 0;
  }
}

static std::mutex g_caller_mu;
static std::map<std::string, uint64_t> g_callers;
static std::string module_of(void* p);
// ---- the mod layer: both triggers held turn the controller into the mod's keys (keys::bind_pad) ----
// While both triggers are down the game sees neither trigger nor any button (the sticks still walk); a button pressed
// then runs its mod key, the D-pad repeating while held. A trigger pressed alone reaches the game a moment late
// (settings::TriggerDelay, 150 ms by default), so the first trigger of a two-trigger press does nothing in the game
// (RT dashes, LT shifts the turn in battle); a shorter tap is replayed for the game. Anything held when the layer
// closes stays hidden until it is released.
constexpr uint32_t kLT = 0x10000, kRT = 0x20000, kTriggers = kLT | kRT;
struct Layer {
  bool on = false;
  uint32_t prev = 0, hidden = 0, pend = 0, replay = 0, held = 0;
  ULONGLONG pend_at = 0, replay_until = 0, repeat_at = 0;
};
static Layer g_layer[4];
static std::mutex g_layer_mu;
static std::vector<std::pair<uint16_t, bool>> g_layer_events;   // (button, repeat); button 0 = the layer opened
static std::atomic<bool> g_layer_open{false};
static std::atomic<bool> g_modal{false};   // a mod menu has the controller (set_modal)
static std::atomic<bool> g_claim_taps{false};
static std::atomic<int> g_tap{0};
void claim_trigger_taps(bool on) { g_claim_taps = on; if (!on) g_tap = 0; }
int take_trigger_tap() { return g_tap.exchange(0); }
static std::atomic<bool> g_claim_rstick{false};
static std::atomic<int> g_rflick{0};
static std::atomic<uint16_t> g_end_buttons{0};
static std::atomic<int> g_end_sticks{0};
void block_ends(uint16_t buttons, int stick_sides) { g_end_buttons = buttons; g_end_sticks = stick_sides; }
void claim_right_stick(bool on) { g_claim_rstick = on; if (!on) g_rflick = 0; }
int take_right_flick() { return g_rflick.exchange(0); }
void set_modal(bool on) { g_modal = on; }
bool modal() { return g_modal; }
static void layer_event(uint16_t button, bool repeat) { std::lock_guard lk(g_layer_mu); g_layer_events.push_back({button, repeat}); }

static void layer_filter(Layer& L, XINPUT_GAMEPAD& g) {
  ULONGLONG now = GetTickCount64();
  // A trigger counts as held past 100 of 255 and until it falls under 40 (no flicker at the threshold).
  uint32_t trig = 0;
  for (auto [bit, v] : {std::pair{kLT, g.bLeftTrigger}, std::pair{kRT, g.bRightTrigger}})
    if (v > 100 || (v >= 40 && (L.prev & bit))) trig |= bit;
  uint32_t s = g.wButtons | trig, pressed = s & ~L.prev;
  L.hidden &= s;   // released: no longer hidden
  bool both = (s & kTriggers) == kTriggers;
  if (g_modal) {
    // A mod menu: the game sees nothing (sticks included); each new button goes to the menu. Triggers still held
    // when it closes count as the layer already open, so closing it from the layer does not announce the layer again.
    L.on = both; L.pend = 0;
    L.hidden |= s;
    for (uint32_t bit = 1; bit <= 0x8000; bit <<= 1)
      if (pressed & bit) { layer_event((uint16_t)bit, false); L.held = bit; L.repeat_at = now + 450; }
    if (L.held && (s & L.held) && now >= L.repeat_at) { layer_event((uint16_t)L.held, true); L.repeat_at = now + 130; }
    L.prev = s;
    g.wButtons = 0; g.bLeftTrigger = g.bRightTrigger = 0;
    g.sThumbLX = g.sThumbLY = g.sThumbRX = g.sThumbRY = 0;
    return;
  }
  const ULONGLONG grace = (ULONGLONG)settings::get(settings::TriggerDelay);
  if (both && !L.on) { L.on = true; L.pend = 0; layer_event(0, false); }
  else if (!both && L.on) L.on = false;
  if (L.on) {
    L.hidden |= s;
    for (uint32_t bit = 1; bit <= 0x8000; bit <<= 1)
      if (pressed & bit) { layer_event((uint16_t)bit, false); L.held = bit; L.repeat_at = now + 450; }
    if (L.held && (s & L.held) && now >= L.repeat_at) { layer_event((uint16_t)L.held, true); L.repeat_at = now + 130; }
  } else {
    L.held = 0;
    // One trigger alone: held back for a moment in case the other follows.
    if (uint32_t t = pressed & kTriggers & ~L.hidden; t && !(s & kTriggers & ~t)) { L.pend = t; L.pend_at = now; }
    // A menu with sections claims a lone trigger once the other has had a moment to follow (the layer): the jump
    // fires on the press, not the release (an analog trigger takes a while to travel back), and the trigger stays
    // hidden from the game until let go.
    constexpr ULONGLONG kTapWindow = 50;
    bool claim = g_claim_taps && &L == &g_layer[0];
    if (claim && L.pend && now - L.pend_at >= (grace < kTapWindow ? grace : kTapWindow)) {
      g_tap = L.pend == kLT ? -1 : 1;
      L.hidden |= L.pend;
      L.pend = 0;
    }
    if (L.pend && !(s & L.pend)) {   // let go within the grace: the game gets a short press after all
      if (now - L.pend_at < grace) {
        if (claim) g_tap = L.pend == kLT ? -1 : 1;   // a menu's section jump instead
        else { L.replay = L.pend; L.replay_until = now + 80; }
      }
      L.pend = 0;
    }
    if (L.pend && now - L.pend_at >= grace) L.pend = 0;
  }
  L.prev = s;
  uint32_t hide = L.hidden | L.pend;
  g.wButtons &= ~(WORD)(hide & 0xFFFF);
  if (hide & kLT) g.bLeftTrigger = 0;
  if (hide & kRT) g.bRightTrigger = 0;
  if (L.replay && now < L.replay_until) {
    if (L.replay & kLT) g.bLeftTrigger = 255;
    if (L.replay & kRT) g.bRightTrigger = 255;
  } else {
    L.replay = 0;
  }
}

static DWORD WINAPI get_state_hook(DWORD user, XINPUT_STATE* st) {
  if (g_vpad) {   // dev: who polls which slot
    void* ret = _ReturnAddress();
    HMODULE m = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)ret, &m);
    std::string key = std::format("slot{} {}+{:#x}", user, module_of(ret), m ? (uintptr_t)ret - (uintptr_t)m : (uintptr_t)ret);
    std::lock_guard lk(g_caller_mu);
    ++g_callers[key];
  }
  DWORD r = g_get_state(user, st);
  if (user > 3) return r;
  uint32_t mask = 0; float lx = 0, ly = 0, ry = 0;
  if (user == 0) {
    ++g_polls;
    g_real_connected = r == ERROR_SUCCESS;
    if (r == ERROR_SUCCESS) {
      g_real_buttons = st->Gamepad.wButtons;
      st->Gamepad.wButtons &= ~(g_swallow | g_end_buttons.load());
      int sides = g_end_sticks;
      if (((sides & 1) && st->Gamepad.sThumbLX < 0) || ((sides & 2) && st->Gamepad.sThumbLX > 0)) st->Gamepad.sThumbLX = 0;
    } else {
      g_real_buttons = 0;
    }
    synth(mask, lx, ly, ry);
  }
  bool active = mask || lx || ly || ry;
  if (r != ERROR_SUCCESS) {
    if (user != 0 || (!g_vpad && !active)) { g_layer[user] = Layer{}; return r; }
    ZeroMemory(st, sizeof *st);
    r = ERROR_SUCCESS;
  }
  if (active) {
    ++g_synth_polls;
    st->Gamepad.wButtons |= (WORD)(mask & 0xFFFF);
    if (mask & 0x10000) st->Gamepad.bLeftTrigger = 255;
    if (mask & 0x20000) st->Gamepad.bRightTrigger = 255;
    if (lx || ly) { st->Gamepad.sThumbLX = (SHORT)(lx * 32767); st->Gamepad.sThumbLY = (SHORT)(ly * 32767); }
    if (ry) st->Gamepad.sThumbRY = (SHORT)(ry * 32767);
  }
  if (user == 0) {
    // A flick: past 20000 and more up or down than sideways; it counts again once back under 10000.
    static bool flicked = false;
    int y = st->Gamepad.sThumbRY, x = st->Gamepad.sThumbRX;
    if (flicked && std::abs(y) < 10000) flicked = false;
    if (g_claim_rstick) {
      if (!flicked && std::abs(y) > 20000 && std::abs(y) > std::abs(x)) {
        flicked = true;
        if (!g_layer_open) g_rflick = y > 0 ? 1 : -1;
      }
      st->Gamepad.sThumbRX = st->Gamepad.sThumbRY = 0;
    }
  }
  layer_filter(g_layer[user], st->Gamepad);
  bool open = false;
  for (auto& l : g_layer) open |= l.on;
  g_layer_open = open;
  // The packet number must change whenever the state does, or a poller that compares packets skips it.
  static XINPUT_GAMEPAD last[4];
  static DWORD packet[4] = {1, 1, 1, 1};
  if (memcmp(&st->Gamepad, &last[user], sizeof(XINPUT_GAMEPAD)) != 0) ++packet[user];
  last[user] = st->Gamepad;
  st->dwPacketNumber += packet[user];
  return r;
}

void press(uint32_t mask, unsigned hold_ms, unsigned gap_ms) { std::lock_guard lk(g_mu); g_queue.push_back(Step{mask, 0, 0, hold_ms, gap_ms}); }
void stick(float lx, float ly, unsigned ms) { std::lock_guard lk(g_mu); g_queue.push_back(Step{0, lx, ly, ms, 60}); }
void hold(uint32_t mask, bool on) { std::lock_guard lk(g_mu); if (on) g_latched |= mask; else g_latched &= ~mask; }
void clear() { std::lock_guard lk(g_mu); g_queue.clear(); g_latched = 0; g_step_start = 0; }
bool busy() { std::lock_guard lk(g_mu); return !g_queue.empty(); }
// Unreal (XInputInterface) re-enumerates pads only on WM_DEVICECHANGE; without one it never polls a slot that
// was empty at startup. Announce the virtual pad the way Windows announces a real one.
static void announce_device_change() { if (HWND w = hooks::game_window()) PostMessageW(w, WM_DEVICECHANGE, 0x0007 /*DBT_DEVNODES_CHANGED*/, 0); }
static void set_vpad_connected(bool on) {
  bool was = g_vpad;
  g_vpad = on;
  if (was != on) announce_device_change();
}
uint16_t real_buttons() { return g_real_buttons; }
void set_swallow(uint16_t m) { g_swallow = m; }

void post_key(uint32_t vk, bool down, bool synthetic) {
  HWND w = hooks::game_window();
  if (!w) return;
  UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
  LPARAM lp = 1 | (sc << 16) | (down ? 0 : (3u << 30)) | (synthetic ? 0x01000000 : 0);
  PostMessageW(w, down ? WM_KEYDOWN : WM_KEYUP, vk, lp);
}

static std::vector<std::pair<uint32_t, ULONGLONG>> g_inject_up;   // injected keys still down: (vk, when to lift)
static LPARAM inject_lparam(uint32_t vk, bool down) {
  UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
  bool ext = vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT || vk == VK_HOME || vk == VK_END ||
             vk == VK_PRIOR || vk == VK_NEXT || vk == VK_INSERT || vk == VK_DELETE;
  return (LPARAM)kInjectedRepeat | (sc << 16) | (ext ? 1u << 24 : 0) | (down ? 0 : (3u << 30));
}
void inject_key(uint32_t vk, unsigned hold_ms) {
  HWND w = hooks::game_window();
  if (!w) return;
  for (auto it = g_inject_up.begin(); it != g_inject_up.end(); ++it)   // still down: lift it first
    if (it->first == vk) { PostMessageW(w, WM_KEYUP, vk, inject_lparam(vk, false)); g_inject_up.erase(it); break; }
  PostMessageW(w, WM_KEYDOWN, vk, inject_lparam(vk, true));
  g_inject_up.push_back({vk, GetTickCount64() + hold_ms});
}
// Whether an injected key is still held (its key-up not yet posted): the press has not been delivered in full.
bool injected_held() { return !g_inject_up.empty(); }
static void inject_tick() {
  HWND w = hooks::game_window();
  ULONGLONG now = GetTickCount64();
  for (size_t i = 0; i < g_inject_up.size();)
    if (now >= g_inject_up[i].second) { if (w) PostMessageW(w, WM_KEYUP, g_inject_up[i].first, inject_lparam(g_inject_up[i].first, false)); g_inject_up.erase(g_inject_up.begin() + i); }
    else ++i;
}

// The player's controller as the game reads it, for "the player just pressed something" (pad_ms). Called through
// P3R.exe's own XInputGetState import: Steam Input's virtual pad lives inside the game process and answers there
// (nothing outside the process saw the player's pad, 2026-10-28). Disconnected slots are slow to ask: once a second.
static void** iat_slot(const char* dll, WORD ordinal);
static std::atomic<uint64_t> g_pad_ms{0};
static std::atomic<bool> g_pad_seen{false};   // a controller answered at least once
uint64_t pad_ms() { return g_pad_ms; }
static std::atomic<bool> g_pad_moving{false};
static void poll_pad() {
  static void** slot = iat_slot("XINPUT1_3.dll", 2);
  if (!slot || !*slot) return;
  // The raw state: past the mod layer's hook when it is in (it hides the layer's buttons from the game).
  auto get = g_get_state ? g_get_state : (GetState_t)*slot;
  static uint32_t prev[4];
  static ULONGLONG next_scan[4];
  ULONGLONG now = GetTickCount64();
  for (DWORD i = 0; i < 4; ++i) {
    if (now < next_scan[i]) continue;
    XINPUT_STATE st{};
    if (get(i, &st) != ERROR_SUCCESS) { next_scan[i] = now + 1000; prev[i] = 0; continue; }
    const XINPUT_GAMEPAD& g = st.Gamepad;
    // Buttons, triggers and stick directions past a deadzone: resting stick noise is not a press.
    uint32_t s = g.wButtons | (g.bLeftTrigger > 64) << 16 | (g.bRightTrigger > 64) << 17 | (g.sThumbLX > 16000) << 18 |
                 (g.sThumbLX < -16000) << 19 | (g.sThumbLY > 16000) << 20 | (g.sThumbLY < -16000) << 21 |
                 (g.sThumbRY > 16000) << 22 | (g.sThumbRY < -16000) << 23;
    if (s & ~prev[i]) g_pad_ms = now;
    prev[i] = s;
    g_pad_seen = true;
  }
  // The left stick pushed on any controller (bits 18 to 21): the player is walking the hero himself.
  bool moving = false;
  for (uint32_t s : prev) moving |= (s & (0xFu << 18)) != 0;
  g_pad_moving = moving;
}
bool pad_moving() { return g_pad_moving; }

// Key hints follow the device the player last drove the game with; before any input, a controller that answers.
bool pad_in_use() {
  uint64_t pad = g_pad_ms, key = keys::game_key_ms();
  return pad || key ? pad > key : g_pad_seen.load();
}

void tick() {
  static bool announced = false;
  if (!announced && g_vpad && hooks::game_window()) { announced = true; announce_device_change(); }
  poll_pad();
  inject_tick();
  std::vector<std::pair<uint16_t, bool>> events;
  { std::lock_guard lk(g_layer_mu); events.swap(g_layer_events); }
  for (auto [button, repeat] : events) keys::pad(button, repeat);
}
bool layer_open() { return g_layer_open; }

std::string status() {
  std::lock_guard lk(g_mu);
  return std::format("input hook={} polls={} synth={} queue={} latched={:#x} vpad={} real={} real_buttons={:#x} pad_seen={} pad_ms={} game_key_ms={} hints={} layer={}",
                     g_get_state != nullptr, g_polls, g_synth_polls, g_queue.size(), g_latched, g_vpad, g_real_connected, g_real_buttons, g_pad_seen.load(),
                     g_pad_ms.load(), keys::game_key_ms(), pad_in_use() ? "pad" : "keyboard", g_layer_open.load());
}
}  // namespace p3a::input

namespace p3a::strings {
bool pad_hints() { return input::pad_in_use(); }
}  // namespace p3a::strings

namespace p3a::input {

static uint32_t parse_vk(const std::string& n) {
  static const std::pair<const char*, uint32_t> named[] = {
    {"enter", VK_RETURN}, {"escape", VK_ESCAPE}, {"esc", VK_ESCAPE}, {"space", VK_SPACE}, {"tab", VK_TAB}, {"backspace", VK_BACK},
    {"up", VK_UP}, {"down", VK_DOWN}, {"left", VK_LEFT}, {"right", VK_RIGHT}, {"shift", VK_SHIFT}, {"ctrl", VK_CONTROL}, {"alt", VK_MENU},
    {"f1", VK_F1}, {"f2", VK_F2}, {"f3", VK_F3}, {"f4", VK_F4}, {"f5", VK_F5}, {"f6", VK_F6}, {"f7", VK_F7}, {"f8", VK_F8},
    {"f9", VK_F9}, {"f10", VK_F10}, {"f11", VK_F11}, {"f12", VK_F12}, {"home", VK_HOME}, {"end", VK_END}, {"pageup", VK_PRIOR}, {"pagedown", VK_NEXT},
  };
  for (auto& [k, v] : named) if (n == k) return v;
  if (n.size() == 1) return (uint32_t)toupper((unsigned char)n[0]);
  return (uint32_t)strtoul(n.c_str(), nullptr, 0);
}

// P3R.exe imports XINPUT1_3 by ordinal (2 = XInputGetState). Returns the import address table slot.
static void** iat_slot(const char* dll, WORD ordinal) {
  auto base = (uint8_t*)GetModuleHandleW(nullptr);
  auto nt = (IMAGE_NT_HEADERS64*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
  auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
  for (auto d = (IMAGE_IMPORT_DESCRIPTOR*)(base + dir.VirtualAddress); d->Name; ++d) {
    if (_stricmp((const char*)(base + d->Name), dll) != 0) continue;
    auto names = (IMAGE_THUNK_DATA64*)(base + d->OriginalFirstThunk);
    auto addrs = (IMAGE_THUNK_DATA64*)(base + d->FirstThunk);
    for (int i = 0; names[i].u1.Ordinal; ++i)
      if (IMAGE_SNAP_BY_ORDINAL64(names[i].u1.Ordinal) && IMAGE_ORDINAL64(names[i].u1.Ordinal) == ordinal) return (void**)&addrs[i].u1.Function;
  }
  return nullptr;
}
static void** g_iat;   // P3R.exe's XInputGetState import slot while it points at the hook
static bool set_slot(void** slot, void* to) {
  DWORD old;
  if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &old)) return false;
  InterlockedExchangePointer(slot, to);
  VirtualProtect(slot, sizeof(void*), old, &old);
  return true;
}
static void** iat_slot_by_name(const char* dll, const char* fn) {
  auto base = (uint8_t*)GetModuleHandleW(nullptr);
  auto nt = (IMAGE_NT_HEADERS64*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
  auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
  for (auto d = (IMAGE_IMPORT_DESCRIPTOR*)(base + dir.VirtualAddress); d->Name; ++d) {
    if (_stricmp((const char*)(base + d->Name), dll) != 0) continue;
    auto names = (IMAGE_THUNK_DATA64*)(base + d->OriginalFirstThunk);
    auto addrs = (IMAGE_THUNK_DATA64*)(base + d->FirstThunk);
    for (int i = 0; names[i].u1.Ordinal; ++i)
      if (!IMAGE_SNAP_BY_ORDINAL64(names[i].u1.Ordinal) &&
          strcmp((const char*)((IMAGE_IMPORT_BY_NAME*)(base + names[i].u1.AddressOfData))->Name, fn) == 0)
        return (void**)&addrs[i].u1.Function;
  }
  return nullptr;
}

// ---- dev: modifier keys the game polls (GetKeyState / GetAsyncKeyState for Ctrl = field strike, Shift = dash) ----
// A posted WM_KEYDOWN does not set them. P3R.exe's import slots for both are pointed at small stubs in a page of
// their own: a key whose byte is set in the page's table reads as held, anything else goes to user32. The page is
// never freed and holds no code of this DLL, so it outlives a hot reload (a Detours hook on these crashed at unload,
// 2026-10-27); a reload finds and reuses it.
//   stub: movzx eax, cl / lea rdx, [table] / cmp byte [rdx+rax], 0 / jne held / jmp [orig] / held: mov eax, 0x8000; ret
static uint8_t* g_keypage;   // +0 table[256], +0x100 orig GetKeyState, +0x108 orig GetAsyncKeyState, +0x120/+0x160 stubs
static void write_stub(uint8_t* stub, uint8_t* table, uint8_t* orig) {
  uint8_t s[] = {0x0F, 0xB6, 0xC1, 0x48, 0x8D, 0x15, 0, 0, 0, 0, 0x80, 0x3C, 0x02, 0x00, 0x75, 0x06, 0xFF, 0x25, 0, 0, 0, 0,
                 0xB8, 0x00, 0x80, 0x00, 0x00, 0xC3};
  int32_t d1 = (int32_t)(table - (stub + 10)), d2 = (int32_t)(orig - (stub + 22));
  memcpy(s + 6, &d1, 4);
  memcpy(s + 18, &d2, 4);
  memcpy(stub, s, sizeof s);
}
static bool install_modkeys() {
  void** ks = iat_slot_by_name("USER32.dll", "GetKeyState");
  void** aks = iat_slot_by_name("USER32.dll", "GetAsyncKeyState");
  if (!ks || !aks) return false;
  static const uint8_t sig[] = {0x0F, 0xB6, 0xC1, 0x48, 0x8D, 0x15};
  if (memcmp(*ks, sig, sizeof sig) == 0) { g_keypage = (uint8_t*)((uintptr_t)*ks & ~(uintptr_t)0xFFF); return true; }   // reload
  uint8_t* p = (uint8_t*)VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (!p) return false;
  memset(p, 0, 0x1000);
  *(void**)(p + 0x100) = *ks;
  *(void**)(p + 0x108) = *aks;
  write_stub(p + 0x120, p, p + 0x100);
  write_stub(p + 0x160, p, p + 0x108);
  FlushInstructionCache(GetCurrentProcess(), p, 0x1000);
  for (auto [slot, stub] : {std::pair{ks, p + 0x120}, std::pair{aks, p + 0x160}}) {
    DWORD old;
    VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &old);
    *slot = stub;
    VirtualProtect(slot, sizeof(void*), old, &old);
  }
  g_keypage = p;
  return true;
}

static std::string module_of(void* p) {
  HMODULE m = nullptr;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)p, &m)) return "?";
  char n[MAX_PATH]; GetModuleFileNameA(m, n, MAX_PATH);
  const char* s = strrchr(n, '\\');
  return s ? s + 1 : n;
}
std::string callers_report() {
  std::lock_guard lk(g_caller_mu);
  std::string out;
  for (auto& [k, n] : g_callers) out += std::format("  {} x{}\n", k, n);
  return out;
}
std::string iat_report() {
  void** slot = iat_slot("XINPUT1_3.dll", 2);
  if (!slot) return "XInputGetState IAT slot not found\n";
  return std::format("P3R.exe IAT XInputGetState -> {} ({}), our target {} ({})\n", *slot, module_of(*slot), (void*)g_get_state, module_of((void*)g_get_state));
}

bool install() {
  // The XInput detour (the mod layer, button swallowing, the dev virtual pad) on the function P3R.exe's own import
  // calls: where Steam Input's pad answers (see poll_pad). The virtual pad's fake connection only with P3A_VPAD=1: the
  // game ignores a virtual pad without a real controller, and a hot reload with it connected made the game report "A
  // device was disconnected" (2026-10-27).
  // The import slot itself is pointed at the hook, which calls what the slot held: the function's entry, where Steam
  // Input patches its own jump once the overlay loads (after the mod, in a normal launch). A Detours hook on those
  // same bytes was overwritten by Steam's, and its trampoline skipped Steam: a DualSense, which only Steam turns into
  // an XInput pad, never reached the layer (2026-10-29). Steam never touches P3R.exe's import table.
  void** slot = iat_slot("XINPUT1_3.dll", 2);
  if (GetEnvironmentVariableW(L"P3A_VPAD", nullptr, 0) > 0) g_vpad = true;
  bool ok = slot && *slot && *slot != (void*)&get_state_hook;
  if (ok) {
    g_get_state = (GetState_t)*slot;
    ok = set_slot(slot, (void*)&get_state_hook);
    if (ok) g_iat = slot; else g_get_state = nullptr;
  }
  // No GetKeyState/GetAsyncKeyState hooks: the game polls only the modifiers through them (measured 2026-10-27), and a
  // detour on a function every thread calls was freed under a running thread at unload (crash, 2026-10-27).
  dev::route("/pad", [](const dev::Request& r, int&) {
    // /pad?press=down,down,a [&ms=70&gap=90] | ?hold=lt&on=1 | ?stick=0,1&ms=500 | ?clear=1 | ?connect=1|0
    if (dev::has(r, "clear")) clear();
    if (dev::has(r, "connect")) set_vpad_connected(dev::arg(r, "connect") == "1");
    unsigned ms = (unsigned)dev::arg_int(r, "ms", 70), gap = (unsigned)dev::arg_int(r, "gap", 90);
    std::string seq = dev::arg(r, "press");
    for (size_t s = 0; s < seq.size();) {
      size_t e = seq.find(',', s); if (e == std::string::npos) e = seq.size();
      std::string step = seq.substr(s, e - s);
      uint32_t m = 0;
      for (size_t p = 0; p < step.size();) {   // "lb+a" presses together
        size_t q = step.find('+', p); if (q == std::string::npos) q = step.size();
        m |= name_mask(step.substr(p, q - p));
        p = q + 1;
      }
      if (m) press(m, ms, gap);
      s = e + 1;
    }
    if (dev::has(r, "hold")) hold(name_mask(dev::arg(r, "hold")), dev::arg(r, "on", "1") == "1");
    if (dev::has(r, "stick")) {
      std::string v = dev::arg(r, "stick"); size_t c = v.find(',');
      stick((float)atof(v.substr(0, c).c_str()), c == std::string::npos ? 0.f : (float)atof(v.substr(c + 1).c_str()), (unsigned)dev::arg_int(r, "ms", 500));
    }
    // ?rstick=y&ms= the right stick up (1) or down (-1), for the battle's details.
    if (dev::has(r, "rstick")) {
      std::lock_guard lk(g_mu);
      g_queue.push_back(Step{0, 0, 0, (unsigned)dev::arg_int(r, "ms", 200), 60, (float)atof(dev::arg(r, "rstick").c_str())});
    }
    if (dev::has(r, "wait")) { for (int i = 0; i < 300 && busy(); ++i) Sleep(20); }
    // ?poll=ms polls slot 0 through the hook every 10 ms, as the game does while focused (the dev game runs
    // unfocused, where Unreal polls no pad): drives the mod layer; reports what the game would have seen last.
    std::string polled;
    if (int poll_ms = (int)dev::arg_int(r, "poll", 0); poll_ms > 0 && g_get_state) {
      XINPUT_STATE st{};
      for (int t = 0; t < poll_ms; t += 10) { get_state_hook(0, &st); Sleep(10); }
      polled = std::format("game sees buttons={:#x} lt={} rt={}\n", st.Gamepad.wButtons, st.Gamepad.bLeftTrigger, st.Gamepad.bRightTrigger);
    }
    // ?raw=1: the controller itself (past the mod layer, no layer side effects), sticks included: a drifting stick
    // moves the game's cursors on its own.
    if (dev::has(r, "raw") && g_get_state) {
      XINPUT_STATE st{};
      DWORD rc = g_get_state(0, &st);
      const XINPUT_GAMEPAD& g = st.Gamepad;
      polled += std::format("raw rc={} buttons={:#x} lt={} rt={} lx={} ly={} rx={} ry={}\n", rc, g.wButtons, g.bLeftTrigger,
                            g.bRightTrigger, g.sThumbLX, g.sThumbLY, g.sThumbRX, g.sThumbRY);
    }
    return status() + "\n" + polled + (dev::has(r, "iat") ? iat_report() + callers_report() : std::string());
  }, "?press=a,down|lb+a[&ms=&gap=] ?hold=x&on=1 ?stick=x,y&ms= ?clear=1 ?connect=1 ?wait=1 ?poll=ms ?raw=1 virtual pad");
  dev::route("/key", [](const dev::Request& r, int&) {
    // /key?k=enter,down,a [&ms=60] keyboard messages to the game window
    std::string seq = dev::arg(r, "k");
    unsigned ms = (unsigned)dev::arg_int(r, "ms", 60);
    for (size_t s = 0; s < seq.size();) {
      size_t e = seq.find(',', s); if (e == std::string::npos) e = seq.size();
      // "w+d" holds both together (diagonal walking)
      std::string step = seq.substr(s, e - s);
      std::vector<uint32_t> vks;
      for (size_t p = 0; p <= step.size();) {
        size_t q = step.find('+', p); if (q == std::string::npos) q = step.size();
        if (q > p) vks.push_back(parse_vk(step.substr(p, q - p)));
        p = q + 1;
      }
      for (uint32_t vk : vks) post_key(vk, true);
      // ?held=N: N auto-repeats, as Windows sends for a key held down.
      for (int n = (int)dev::arg_int(r, "held", 0); n > 0; --n) {
        Sleep(30);
        for (uint32_t vk : vks)
          PostMessageW(hooks::game_window(), WM_KEYDOWN, vk, 1 | (MapVirtualKeyW(vk, MAPVK_VK_TO_VSC) << 16) | (1u << 30));
      }
      Sleep(ms);
      for (uint32_t vk : vks) post_key(vk, false);
      Sleep(ms + 30);
      s = e + 1;
    }
    return std::string("ok\n");
  }, "?k=enter,down,a keyboard to the game window");
  // Dev only (the dev server is on): the modifier stubs above.
  if (GetEnvironmentVariableW(L"P3A_PORT", nullptr, 0) > 0) log::writef("input: modifier stubs {}", install_modkeys() ? "on" : "FAILED");
  dev::route("/modkey", [](const dev::Request& r, int&) {
    // /modkey?vk=0xA2&ms=150 a modifier held as the game polls it (Ctrl 0xA2 = field strike, Shift 0xA0 = dash),
    // with the key messages a keyboard sends
    if (!g_keypage) return std::string("modifier stubs not installed\n");
    uint32_t vk = (uint32_t)dev::arg_int(r, "vk", VK_LCONTROL) & 0xFF;
    unsigned ms = (unsigned)dev::arg_int(r, "ms", 150);
    uint32_t generic = vk == VK_LCONTROL || vk == VK_RCONTROL ? VK_CONTROL : vk == VK_LSHIFT || vk == VK_RSHIFT ? VK_SHIFT : vk;
    post_key(generic, true);   // first: Unreal treats a key already held when its message arrives as a repeat
    Sleep(30);
    g_keypage[vk] = g_keypage[generic] = 1;
    Sleep(ms);
    post_key(generic, false);
    g_keypage[vk] = g_keypage[generic] = 0;
    return std::string("ok\n");
  }, "?vk=0xA2&ms=150 hold a modifier as the game polls it (dev)");
  log::writef("input: XInputGetState hook {}", ok ? "on" : "FAILED");
  return ok;
}
void remove() {
  if (g_iat) set_slot(g_iat, (void*)g_get_state);
  g_iat = nullptr;
}
}  // namespace p3a::input

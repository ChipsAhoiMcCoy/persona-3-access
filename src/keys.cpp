#include "keys.h"
#include "hooks.h"
#include "input.h"
#include "core/strings.h"
#include "log.h"
#include "msg.h"
#include "seh.h"
#include "ue/ue.h"
#include <windows.h>
#include <xinput.h>
#include <format>
#include <vector>
#include <atomic>

namespace p3a::keys {
struct Binding { uint32_t vk; int mods; std::string name, help; Handler h; bool repeat; uint16_t pad = 0; };
static std::vector<Binding> g_bindings;
static bool g_swallowing[256];   // a bound key's down was swallowed: swallow its char and up too

void bind(uint32_t vk, int mods, const char* name, const char* help, Handler h, bool repeat) {
  g_bindings.push_back(Binding{vk, mods, name, help, std::move(h), repeat});
}

static Handler g_on_layer;
static ModalKey g_modal_key;
static ModalPad g_modal_pad;
void set_modal(ModalKey key, ModalPad pad) { g_modal_key = std::move(key); g_modal_pad = std::move(pad); }
static CaptureKey g_capture;
void set_capture(CaptureKey key) { g_capture = std::move(key); }
static MenuKey g_menu;
void set_menu(MenuKey key) { g_menu = std::move(key); }
void bind_pad(const char* name, uint16_t button) { for (auto& b : g_bindings) if (b.name == name) b.pad = button; }
void on_pad_layer(Handler h) { g_on_layer = std::move(h); }
void pad(uint16_t button, bool repeat) {
  if (g_modal_pad) {   // a copy: the handler may close the menu, clearing g_modal_pad under itself
    if (button) { ModalPad h = g_modal_pad; if (!guarded([&] { h(button, repeat); })) log::write("keys: modal pad faulted"); }
    return;
  }
  if (!button) { if (g_on_layer && !guarded([&] { g_on_layer(); })) log::write("keys: pad layer faulted"); return; }
  for (auto& b : g_bindings) {
    if (b.pad != button) continue;
    if (repeat && !b.repeat) return;
    if (!guarded([&] { b.h(); })) log::writef("keys: {} faulted", b.name);
    return;
  }
}

static int current_mods() {
  int m = 0;
  if (GetKeyState(VK_SHIFT) & 0x8000) m |= Shift;
  if (GetKeyState(VK_CONTROL) & 0x8000) m |= Ctrl;
  if (GetKeyState(VK_MENU) & 0x8000) m |= Alt;
  return m;
}

static bool g_movement[256];
static std::atomic<uint64_t> g_game_key_ms{0};
uint64_t game_key_ms() { return g_game_key_ms; }
static uint32_t g_movement_presses[256];
uint32_t movement_presses(uint32_t vk) { return g_movement_presses[vk & 0xFF]; }
static bool is_movement(uint32_t vk) { return vk == 'W' || vk == 'A' || vk == 'S' || vk == 'D' || vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT; }
// A movement key is held from its key-down to its key-up. The window losing the keyboard takes the key-up with it
// (it goes to the other window; a stuck flag stopped every walk at once, 2026-10-28), so losing it lets go of every
// movement key (focus_lost, from the window procedure).
bool movement_key_down() {
  for (int i = 0; i < 256; ++i) if (g_movement[i]) return true;
  return false;
}
static void focus_lost() { for (bool& held : g_movement) held = false; }

// Synthetic keys the navigator posts carry this marker in the lParam's reserved bits, so they are neither bound
// commands nor a "player took over" signal.
constexpr LPARAM kSyntheticMark = 0x01000000;   // bit 24 = extended-key flag; the navigator's keys are never extended

static bool filter(UINT msg, WPARAM w, LPARAM l) {
  if (ue::exit_requested()) return false;   // the game is quitting: no mod key reads what it is taking down
  uint32_t vk = (uint32_t)w & 0xFF;
  // A key the mod pressed for the game (a menu stepping the game's cursor to its focus): the game's, untouched.
  if ((msg == WM_KEYDOWN || msg == WM_KEYUP) && (l & 0xFFFF) == input::kInjectedRepeat) return false;
  bool synthetic = (l & kSyntheticMark) != 0 && is_movement(vk) && vk >= 'A' && vk <= 'Z';
  if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
    if (is_movement(vk) && !synthetic) {
      if (!g_movement[vk]) log::writef("keys: movement key {:#x} down (lParam {:#x})", vk, (uint64_t)l);
      if (!(l & (1 << 30))) ++g_movement_presses[vk];
      g_movement[vk] = true;
    }
    bool repeat = (l & (1 << 30)) != 0;
    // Confirm or cancel from the player: a message window's reading stops before anything the press says itself.
    if (msg == WM_KEYDOWN && !repeat && !g_modal_key && (vk == VK_RETURN || vk == VK_SPACE || vk == VK_ESCAPE || vk == VK_BACK))
      p3a::msg::player_pressed();
    if (g_modal_key && !synthetic && msg == WM_KEYDOWN) {
      g_swallowing[vk] = true;
      ModalKey h = g_modal_key;
      if (!guarded([&] { h(vk, repeat); })) log::write("keys: modal key faulted");
      return true;
    }
    int mods = current_mods();
    if (g_menu && !synthetic && msg == WM_KEYDOWN) {
      MenuKey h = g_menu;
      bool taken = false;
      // A menu's key is the player on the keyboard too: the menu steps the game with keys, not pad buttons, and the
      // hints name keys (input::pad_in_use). Counted before the menu acts on it, so anything the menu's handler asks
      // about the device already sees this key.
      const uint64_t key_was = g_game_key_ms;
      if (!repeat) g_game_key_ms = GetTickCount64();
      if (!guarded([&] { taken = h(vk, mods, repeat); })) log::write("keys: menu key faulted");
      if (taken) { g_swallowing[vk] = true; return true; }
      g_game_key_ms = key_was;
    }
    if (g_capture && !synthetic && msg == WM_KEYDOWN && mods == None) {
      CaptureKey h = g_capture;
      bool taken = false;
      if (!guarded([&] { taken = h(vk, repeat); })) log::write("keys: capture faulted");
      if (taken) { g_swallowing[vk] = true; return true; }
    }
    for (auto& b : g_bindings) {
      if (b.vk != vk || b.mods != mods) continue;
      g_swallowing[vk] = true;
      if (repeat && !b.repeat) return true;
      if (!guarded([&] { b.h(); })) log::writef("keys: {} faulted", b.name);
      return true;
    }
    // A key the game gets: the player is on the keyboard (the mod's own keys say nothing about that).
    if (!synthetic && !repeat) g_game_key_ms = GetTickCount64();   // not the raw bit: real arrows are extended keys
    return false;
  }
  if (msg == WM_KEYUP || msg == WM_SYSKEYUP) {
    if (is_movement(vk) && !synthetic) g_movement[vk] = false;
    if (g_swallowing[vk]) { g_swallowing[vk] = false; return true; }
    return false;
  }
  if (msg == WM_CHAR) {
    if (g_modal_key) return true;
    // The char of a swallowed key: VkKeyScan maps it back (good enough for the punctuation keys the mod binds).
    SHORT k = VkKeyScanW((WCHAR)w);
    if (k != -1 && g_swallowing[k & 0xFF]) return true;
  }
  return false;
}

static std::string key_name(uint32_t vk, int mods) {
  std::string s;
  if (mods & Ctrl) s += strings::kModCtrl;
  if (mods & Alt) s += strings::kModAlt;
  if (mods & Shift) s += strings::kModShift;
  UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
  wchar_t buf[64] = {};
  LONG lp = (LONG)(sc << 16);
  if (vk >= VK_PRIOR && vk <= VK_DELETE) lp |= 1 << 24;
  if (GetKeyNameTextW(lp, buf, 64) > 0) { char nb[128]; WideCharToMultiByte(CP_UTF8, 0, buf, -1, nb, 128, nullptr, nullptr); s += nb; }
  else s += std::format("{} {:#x}", strings::kKeyUnnamed, vk);
  return s;
}

static const char* pad_name(uint16_t m) {
  switch (m) {
    case XINPUT_GAMEPAD_DPAD_UP: return strings::kPadDpadUp;
    case XINPUT_GAMEPAD_DPAD_DOWN: return strings::kPadDpadDown;
    case XINPUT_GAMEPAD_DPAD_LEFT: return strings::kPadDpadLeft;
    case XINPUT_GAMEPAD_DPAD_RIGHT: return strings::kPadDpadRight;
    case XINPUT_GAMEPAD_START: return strings::kPadMenu;
    case XINPUT_GAMEPAD_BACK: return strings::kPadView;
    case XINPUT_GAMEPAD_LEFT_THUMB: return strings::kPadL3;
    case XINPUT_GAMEPAD_RIGHT_THUMB: return strings::kPadR3;
    case XINPUT_GAMEPAD_LEFT_SHOULDER: return strings::kPadLB;
    case XINPUT_GAMEPAD_RIGHT_SHOULDER: return strings::kPadRB;
    case XINPUT_GAMEPAD_A: return strings::kPadA;
    case XINPUT_GAMEPAD_B: return strings::kPadB;
    case XINPUT_GAMEPAD_X: return strings::kPadX;
    case XINPUT_GAMEPAD_Y: return strings::kPadY;
    default: return strings::kPadUnknown;
  }
}

std::string help() {
  std::string s = std::string(strings::kModKeys) + ". ";
  if (strings::pad_hints()) {
    s += std::string(strings::kPadLayer) + ": ";
    for (auto& b : g_bindings) if (b.pad) s += std::string(pad_name(b.pad)) + ": " + b.help + ". ";
    return s;
  }
  for (auto& b : g_bindings) if (b.vk) s += key_name(b.vk, b.mods) + ": " + b.help + ". ";   // vk 0: a controller-only binding
  return s;
}

void install() {
  hooks::set_key_filter(&filter);
  hooks::set_focus_lost(&focus_lost);
}
}  // namespace p3a::keys

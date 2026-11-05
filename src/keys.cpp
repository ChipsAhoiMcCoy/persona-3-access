#include "keys.h"
#include "hooks.h"
#include "input.h"
#include "core/strings.h"
#include "log.h"
#include "msg.h"
#include "seh.h"
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
static ULONGLONG g_movement_at[256];   // the last down or auto-repeat of each held movement key
static bool is_movement(uint32_t vk) { return vk == 'W' || vk == 'A' || vk == 'S' || vk == 'D' || vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT; }
// A held key auto-repeats (after the first ~0.5 s): one silent for over a second lost its key-up (focus changes,
// 2026-10-28: a stuck flag stopped every walk at once), so it no longer counts as held.
bool movement_key_down() {
  ULONGLONG now = GetTickCount64();
  for (int i = 0; i < 256; ++i) if (g_movement[i] && now - g_movement_at[i] < 1200) return true;
  return false;
}

// Synthetic keys the navigator posts carry this marker in the lParam's reserved bits, so they are neither bound
// commands nor a "player took over" signal.
constexpr LPARAM kSyntheticMark = 0x01000000;   // bit 24 = extended-key flag; the navigator's keys are never extended

static bool filter(UINT msg, WPARAM w, LPARAM l) {
  uint32_t vk = (uint32_t)w & 0xFF;
  // A key the mod pressed for the game (a menu stepping the game's cursor to its focus): the game's, untouched.
  if ((msg == WM_KEYDOWN || msg == WM_KEYUP) && (l & 0xFFFF) == input::kInjectedRepeat) return false;
  bool synthetic = (l & kSyntheticMark) != 0 && is_movement(vk) && vk >= 'A' && vk <= 'Z';
  if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
    if (is_movement(vk) && !synthetic) {
      if (!g_movement[vk]) log::writef("keys: movement key {:#x} down (lParam {:#x})", vk, (uint64_t)l);
      g_movement[vk] = true; g_movement_at[vk] = GetTickCount64();
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
      if (!guarded([&] { taken = h(vk, mods, repeat); })) log::write("keys: menu key faulted");
      // A menu's key is the player on the keyboard too: the menu steps the game with keys, not pad buttons, and the
      // hints name keys (input::pad_in_use).
      if (taken) { g_swallowing[vk] = true; if (!repeat) g_game_key_ms = GetTickCount64(); return true; }
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
  if (mods & Ctrl) s += "Ctrl+";
  if (mods & Alt) s += "Alt+";
  if (mods & Shift) s += "Shift+";
  UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
  wchar_t buf[64] = {};
  LONG lp = (LONG)(sc << 16);
  if (vk >= VK_PRIOR && vk <= VK_DELETE) lp |= 1 << 24;
  if (GetKeyNameTextW(lp, buf, 64) > 0) { char nb[128]; WideCharToMultiByte(CP_UTF8, 0, buf, -1, nb, 128, nullptr, nullptr); s += nb; }
  else s += std::format("key {:#x}", vk);
  return s;
}

static const char* pad_name(uint16_t m) {
  switch (m) {
    case XINPUT_GAMEPAD_DPAD_UP: return "D-pad up";
    case XINPUT_GAMEPAD_DPAD_DOWN: return "D-pad down";
    case XINPUT_GAMEPAD_DPAD_LEFT: return "D-pad left";
    case XINPUT_GAMEPAD_DPAD_RIGHT: return "D-pad right";
    case XINPUT_GAMEPAD_START: return "Menu button";
    case XINPUT_GAMEPAD_BACK: return "View button";
    case XINPUT_GAMEPAD_LEFT_THUMB: return "L3";
    case XINPUT_GAMEPAD_RIGHT_THUMB: return "R3";
    case XINPUT_GAMEPAD_LEFT_SHOULDER: return "LB";
    case XINPUT_GAMEPAD_RIGHT_SHOULDER: return "RB";
    case XINPUT_GAMEPAD_A: return "A";
    case XINPUT_GAMEPAD_B: return "B";
    case XINPUT_GAMEPAD_X: return "X";
    case XINPUT_GAMEPAD_Y: return "Y";
    default: return "?";
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

void install() { hooks::set_key_filter(&filter); }
}  // namespace p3a::keys

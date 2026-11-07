#pragma once
#include <cstdint>
#include <functional>
#include <string>

namespace p3a::keys {
// The mod's own keys. Handlers run on the game thread from the window procedure; a bound key is swallowed (down,
// repeat, char and up) so the game never sees it. Modifiers: exact match.
enum Mods : int { None = 0, Shift = 1, Ctrl = 2, Alt = 4 };
using Handler = std::function<void()>;
void bind(uint32_t vk, int mods, const char* name, const char* help, Handler h, bool repeat = false);
void install();
std::string help();   // every binding: keyboard keys, or the controller's mod layer while the player is on a pad
// The controller's mod layer (both triggers held, input.cpp): a binding's button there (XINPUT_GAMEPAD_* mask).
void bind_pad(const char* name, uint16_t button);
void on_pad_layer(Handler h);           // the layer opened
void pad(uint16_t button, bool repeat); // a button pressed in the layer, or 0 = the layer opened (game thread)
// A mod menu that takes every key and button while open (the settings menu): no bindings run, the game gets nothing
// (keys swallowed here, buttons hidden by input::set_modal). nullptr clears.
using ModalKey = std::function<void(uint32_t vk, bool repeat)>;
using ModalPad = std::function<void(uint16_t button, bool repeat)>;
void set_modal(ModalKey key, ModalPad pad);
// A screen that takes some of the game's own keys (unmodified presses only): return true to swallow the key from the
// game (down, repeat, char and up). Runs before the mod's bindings. nullptr clears.
using CaptureKey = std::function<bool(uint32_t vk, bool repeat)>;
void set_capture(CaptureKey key);
// The bespoke menu's keys (screens/menu.cpp): every key down with its modifiers, before the capture and the
// bindings; true swallows it. Keys the mod injects for the game (input::inject_key) never reach it.
using MenuKey = std::function<bool(uint32_t vk, int mods, bool repeat)>;
void set_menu(MenuKey key);
// True while a mod-owned key is physically down (the game's own keys are not tracked).
bool movement_key_down();   // W/A/S/D or the arrow keys, from the real keyboard: cancels auto-walk
uint64_t game_key_ms();     // when the player last pressed a key the game gets (not a mod key), or 0
uint32_t movement_presses(uint32_t vk);   // how many times that movement key has been pressed (no auto-repeats)
}  // namespace p3a::keys

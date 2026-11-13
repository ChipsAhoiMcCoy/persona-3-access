#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace p3a::input {
// The virtual pad: XInputGetState for controller 0 merges synthetic buttons/sticks over the real pad. It drives the
// game in the dev loop (unfocused) and lets the mod press game buttons for the player (e.g. a menu jump).
// Buttons are XINPUT_GAMEPAD_* masks; names: a b x y up down left right start back lb rb ls rs lt rt.
bool install();
void remove();
uint16_t button_mask(std::string_view name);   // 0 if unknown; lt/rt return pseudo masks 0x10000/0x20000 (>16 bits)
// Queue a press: `mask` held for hold_ms, then released for gap_ms before the next queued step runs.
void press(uint32_t mask, unsigned hold_ms = 70, unsigned gap_ms = 90);
void stick(float lx, float ly, unsigned ms);   // left stick, -1..1, held for ms (queued like a press)
void hold(uint32_t mask, bool on);             // latch on/off until released
void clear();
bool busy();                                   // queued steps remain
// The real controller's state as of the game's last poll (for mod hotkeys on a pad).
uint16_t real_buttons();
// Swallow these real pad buttons from the game (while the mod owns them, e.g. a mod menu is open).
void set_swallow(uint16_t mask);
// A game cursor at an end of a list the game would wrap (the battle wheel's target): these real buttons, and the left
// stick pushed left (stick_sides bit 0) or right (bit 1), are hidden from the game while the cursor is there.
void block_ends(uint16_t buttons, int stick_sides);

// Keyboard to the game window via PostMessage (works unfocused): the dev loop's keys and the auto-walk's movement keys.
// synthetic = the mod's own navigator (marked so the key filter does not treat it as the player taking over).
void post_key(uint32_t vk, bool down, bool synthetic = false);
// A key press for the game on the player's behalf (a bespoke menu stepping the game's cursor): down now, up hold_ms
// later. Marked by its repeat count, so the mod's own key handling lets it through to the game.
constexpr uint32_t kInjectedRepeat = 0x5A3;
void inject_key(uint32_t vk, unsigned hold_ms = 40);
bool injected_held();   // an injected key is still down (its press not yet delivered in full)
void tick();   // game thread: announces a pad that was connected before the window existed; polls the player's pad
uint64_t pad_ms();   // when a controller button or stick direction was last newly pressed, or 0
uint64_t pad_count();   // how many such presses there have been
bool pad_moving();   // a real controller's left stick is pushed past its dead zone (as of the last tick)
bool pad_in_use();   // the player is on a controller (pressed it after the keyboard's last game key)
bool layer_open();   // both triggers held: the controller is the mod's (keys::bind_pad)
void set_modal(bool on);   // a mod menu is open: the game gets no buttons or sticks, every button goes to keys::pad
bool modal();
// Trigger presses for a menu with sections: while claimed, one trigger alone (the other not joining it within 50 ms,
// the mod layer) is the menu's and never reaches the game; take_trigger_tap returns the last one (-1 LT, +1 RT) once.
void claim_trigger_taps(bool on);
int take_trigger_tap();
// The right stick for the mod (the battle's details): while claimed the game does not see it; a flick up or down past
// the stick's dead zone is taken once (+1 up, -1 down) until it is let back toward the middle.
// Each claimant holds its own claim (the battle's details clear theirs every frame they are not in focus; the
// top-down mode's scan holds the stick meanwhile): the game sees no stick while any is held.
enum class StickClaim { BattleDetails, TopDown };
void claim_right_stick(bool on, StickClaim who = StickClaim::BattleDetails);
int take_right_flick();
// The claimed right stick where the player holds it now, each axis -1..1 (up and right positive); 0, 0 unclaimed.
void right_stick(float& x, float& y);
// RT pressed while the top-down scan holds the right stick past its deadzone (and LT is not down): the trigger is the
// mod's (hidden from the game, no dash, until it is let go) and taken here once. Pressed before the stick, it stays the game's.
bool take_stick_trigger();
std::string status();
}  // namespace p3a::input

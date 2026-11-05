#pragma once
#include <cstdint>
#include <string>

namespace p3a::voice {
// The game's voice players: when a voiced line starts (the setting "Voiced dialogue" keeps the screen reader quiet
// for it). Game thread.
void tick();                                   // every mod tick, before msg::tick
uint64_t last_start();   // when a voice last began playing (GetTickCount64), 0 for never
bool playing();
// The setting VoiceEndSound's cue from the game's system cue sheet, through USoundApp::PlayComSE (none for "Next"
// or off).
void play_end_sound();
// A cue of the game's system sheet (the menus' own sounds), through USoundApp::PlayComSE. Game thread.
enum SystemCue { CueCursor = 0, CueDecide = 1, CueCancel = 2, CueMenuOpen = 3, CueMenuClose = 4, CuePage = 5, CueMessage = 9, CueWindowOpen = 21, CueOption = 28 };
void play_system_sound(int cue);
void install();                                // dev route /voice
}  // namespace p3a::voice

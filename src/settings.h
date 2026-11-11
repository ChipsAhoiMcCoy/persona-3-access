#pragma once

namespace p3a::settings {
// The mod's options, saved to %LOCALAPPDATA%\P3RAccess\settings.ini (settings-dev.ini for dev launches), and the
// mod settings menu that changes them: F2, or both triggers and View on a controller (the game's Config keys say so).
// A new option: an Id here, a row in settings.cpp's table, its name in strings.h, and its README line.
enum Id { WallTones, Sonar, SonarScope, SonarRange, SonarExits, SonarDoors, SonarPeople, SonarShops, SonarThings,
          NearbyChimes, ShadowAlerts, Hrtf, BehindPitch, PoiRange, MasterVolume, SonarVolume, SoundVolume, WallVolume, TriggerDelay, EntryHints, VoicedLines, VoiceEndSound, Subtitles, AudioDescription, DescriptionVolume, Count };
int get(Id id);   // the value: 0/1, meters, percent, milliseconds or a choice's index
inline bool on(Id id) { return get(id) != 0; }
// VoicedLines: what the screen reader does with a line the game voices.
enum Voiced { VoicedRead, VoicedName, VoicedSilent };
// VoiceEndSound: what says a voiced line has finished: "Next" spoken, or a game sound (voice.cpp maps them to cues).
enum EndSound { EndNext, EndPage, EndMessage, EndCursor, EndOff };
// SonarScope: which points of interest the sonar sounds for.
enum Scope { ScopeAll, ScopeCategory, ScopeSelected };
void set(Id id, int value);   // applies it (wall tones, volume) and saves

// The menu: modal while open (the game gets no keys or buttons; the field keeps running).
void open();
void close();
bool is_open();
void install();   // loads and applies the file, binds nothing (commands.cpp binds F2), dev route /settings
}  // namespace p3a::settings

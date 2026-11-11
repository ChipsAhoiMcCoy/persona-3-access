// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
// The mod's own sounds (wall-tone loops and positional cues): a small mixer on miniaudio's
// default output. Everything is thread-safe and cheap enough to call every frame.
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace p3a::audio {
bool init();
void shutdown();
bool ready();
// A one-shot beep (freq in Hz, volume 0..1, pan -1 left .. +1 right), with a short fade in and out.
void beep(float freq, int ms, float volume, float pan = 0.0f);
void pulse(float freq, int ms, float volume, float pan = 0.0f);   // triangle wave, linear decay over ms (the Shadow pulse)
// Where a sound is, from the listener (the hero, facing the camera's yaw; world::spot makes one): a unit direction
// and the distance in metres. pan is the stereo fallback, the value the cues computed before (sin of the bearing).
// `placed`: the sound has a place in the world (wx, wy, wz, the game's units); while it plays, `follow` moves it in
// the listener's ears as the hero walks and the camera turns. Without it the sound stays where it started.
struct Spot { float right = 0, up = 0, ahead = 1, dist_m = 0, pan = 0; bool placed = false; float wx = 0, wy = 0, wz = 0; };
// Once a frame (game thread): every playing sound with a place is heard from where the listener is now. `where`
// turns a place into a Spot (world::spot from the hero).
void follow(const std::function<Spot(float x, float y, float z)>& where);
int followed();   // dev: how many times a playing sound was moved
// The same, from a place: through Steam Audio's HRTF (src/hrtf.cpp) while 3D sound is on and available, else panned.
void beep(float freq, int ms, float volume, const Spot& at);
void pulse(float freq, int ms, float volume, const Spot& at);
void set_hrtf(bool on);   // the 3D sound setting
// Dev: a beep rendered offline through the HRTF from `at` into a stereo 16-bit WAV (false = HRTF unavailable).
bool render_beep_wav(float freq, int ms, const Spot& at, const std::string& path);
// Looping sample channels (the wotr wall tones): a WAV decoded to mono at the mixer rate, looped
// seamlessly at a fixed pan, volume driven per frame (clamped 0..1, applied directly like wotr).
bool load_loop(int id, const std::string& wav_path, float pan);
void set_loop_volume(int id, float volume);
void set_loop_gain(int id, float gain);   // static trim multiplied with the volume (not clamped; loudness matching)
// Directory of this DLL (assets live next to it), with a trailing backslash.
std::string module_dir();
float master_volume();
void set_master_volume(float v);   // the player's Master volume: scales every bus
// Buses: every sound plays on one, and each bus has its own volume (0..1, changes glide over one 10 ms block).
// Cues = beeps and pulses; Walls = the looping channels. The master scales all of them; a new kind of sound gets
// its own bus (an entry here, a slider in settings.cpp). All voices' own volume changes glide (~15 ms) and the
// output goes through a limiter rather than a hard clip, so nothing clicks or crackles.
enum class Bus { Cues, Walls, Narration, Sonar, Count };
void set_bus_volume(Bus b, float v);
// A one-shot sample from a place: mono at the mixer rate (sample_rate), starting and ending on silence. Through the
// HRTF like the positional beeps (panned without it). rate != 1 plays it faster or slower, pitch and length together.
using Pcm = std::shared_ptr<const std::vector<float>>;
void play(Pcm pcm, float volume, const Spot& at, Bus bus, float rate = 1.0f);
int sample_rate();
// One of the mod's signals (core/sonar_cues.h: the Shadow's growl, the ambush ticks, the layer's
// tick) from a place, on the Cues bus. `signal` is the SignalCue's number.
void signal(int signal, float volume, const Spot& at, float rate = 1.0f);
// The same from no place at all: in the middle of the head, both ears alike (a cue about the player, not a thing).
void signal_here(int signal, float volume);
// The narrator (audio description): a mono clip decoded at the mixer rate (decode_mono: WAV or MP3, any thread),
// played centred on the Narration bus. narrate replaces whatever was playing and returns its id; stop fades out.
// narration() reports the current clip: `played` == `total` with `ended` set means it played in full.
bool decode_mono(const std::string& path, std::vector<float>& out);
int narrate(std::shared_ptr<const std::vector<float>> pcm);
void narrate_pause(bool paused);
void narrate_stop();
struct Narration { int id = -1; bool active = false, paused = false, ended = false; size_t played = 0, total = 0; };
Narration narration();
std::string status();   // dev: buses, voice counts, the limiter, the HRTF slots
void reset_stats();     // dev: the callback's max time and the HRTF fallback count start over
bool mute_process(bool mute);  // WASAPI session mute for this process (Windows keeps it for the next launch)
}  // namespace p3a::audio

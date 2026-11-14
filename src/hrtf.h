#pragma once
// Steam Audio's binaural renderer (HRTF) for the mixer's positional voices: a sound from a direction, as it reaches
// each ear, plus air absorption (far sounds lose treble). phonon.dll sits beside arcana.dll; without it (or if
// Steam Audio fails) available() is false and the mixer pans instead. The slots are made once at init, so nothing
// allocates in the audio callback; claim/release/process are for the audio callback only (under its lock).
#include <vector>
namespace p3a::hrtf {
// Listener space: a unit direction (right, up, ahead) and the distance in metres.
struct Dir { float right = 0, up = 0, ahead = 1, dist_m = 0; };
bool init(int rate, int frame);   // the mixer's rate and its fixed callback size
void shutdown();
bool available();
int frame();
int claim();                      // a free slot, or -1
void release(int slot);           // resets the slot's effects for the next voice
// One frame of mono in, the two ears out (overwritten, frame() samples each).
void process(int slot, const float* mono, const Dir& d, float* out_l, float* out_r);
int slots_used();
int slot_count();
void set_air(float mult);         // air absorption: Steam Audio's default coefficients times this (0 = none)
float air();
// Dev: a mono buffer through fresh effects of its own (any thread), tail included: the offline check.
bool render(const std::vector<float>& mono, const Dir& d, std::vector<float>& l, std::vector<float>& r);
}  // namespace p3a::hrtf

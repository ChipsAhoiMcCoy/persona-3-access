#pragma once
// Ported from Dark Vision (WrathAccess, src/Audio/Spatializer.cs) by Bradley Renshaw; zlib, see LICENSES/. Altered:
// C++, a block processor of our own, metres from Unreal's centimetres. This is not the original software.
//
// The stereo placement model for a sound heard top-down: the listener faces north (the top-down mode) or ahead
// (the camera mode); `east` is to its right, `north` ahead. Independent cues:
//  - east/west: a constant-power pan whose level difference is capped (12 dB, the far ear keeps signal), the
//    interaural time difference (Woodworth head, up to 0.66 ms) and a far-ear high-shelf cut (head shadow);
//  - north/south: things behind get a high-shelf cut at 1.2 kHz, down to -10 dB due south. Ahead is untouched; a
//    shelf, not a lowpass, so a bright cue is quieter behind but never gone;
//  - distance: the caller's own gain curve (nothing here).
// Engine-free.
#include <cstdint>

namespace p3a::core {

struct SpatialCue {
  float pan = 0;           // -1 west (left) .. +1 east (right), after the centre expansion
  float itd_samples = 0;   // the far ear's delay in samples; sign = the side (+ east: the left ear lags)
  float rear_shelf_db = 0; // whole sound: 0 ahead or level .. -10 due behind
  float far_shadow_db = 0; // far ear only: 0 centred .. -8 at the side
};

// The cues for a thing `east` / `north` metres from the listener. Within `pan_width` metres the pan follows the
// sideways offset itself; beyond it, the bearing.
SpatialCue spatial_cue(float east, float north, float pan_width, int sample_rate);
// Stereo gains for a pan: constant power, the level difference capped at 12 dB, centre 0.707 / 0.707.
void spatial_pan_gains(float pan, float& left, float& right);

// One RBJ high shelf (biquad), direct form I.
struct Shelf {
  float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
  float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
  void set(float corner_hz, float gain_db, int sample_rate);
  float run(float x) {
    float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = x; y2 = y1; y1 = y;
    return y;
  }
};

// One playing sound's stereo voice: mono in, two ears out (added to). The cue may change every block; gains and the
// delay glide across the block so a moving thing never clicks.
class SpatialVoice {
 public:
  void process(const float* mono, int frames, const SpatialCue& cue, int sample_rate, float* left, float* right);
  static constexpr int kDelay = 64;   // the delay line (0.66 ms at 48 kHz is ~32 samples)
 private:
  bool started_ = false;
  float gl_ = 0.70710677f, gr_ = 0.70710677f, itd_ = 0;
  Shelf rear_, shadow_l_, shadow_r_;
  float line_l_[kDelay] = {}, line_r_[kDelay] = {};
  int head_ = 0;
};

}  // namespace p3a::core

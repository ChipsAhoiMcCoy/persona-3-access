// Ported from Dark Vision (WrathAccess, src/Audio/Spatializer.cs) by Bradley Renshaw; zlib, see LICENSES/. Altered:
// C++, a block processor of our own. This is not the original software.
#include "core/spatializer.h"
#include <algorithm>
#include <cmath>

namespace p3a::core {
namespace {
constexpr float kPi = 3.14159265358979f;
constexpr float kMaxItdSeconds = 0.00066f;          // head width / speed of sound: 0.22 m / 343 m/s
constexpr float kWoodworthMax = kPi / 2.0f + 1.0f;  // (theta + sin theta) at 90 degrees
constexpr float kMaxIldDb = 12.0f;                  // finite: the far ear keeps signal for the time difference
constexpr float kPanExponent = 0.82f;               // steepens the pan near the centre
constexpr float kShadowCornerHz = 1500.0f;
constexpr float kShadowMaxDb = 8.0f;
constexpr float kRearCornerHz = 3000.0f;
constexpr float kRearMaxCutDb = 10.0f;
}  // namespace

SpatialCue spatial_cue(float east, float north, float pan_width, int sample_rate) {
  SpatialCue c;
  float dist = std::sqrt(east * east + north * north);
  float lat = dist > 1e-4f ? std::clamp(east / std::max(dist, pan_width), -1.0f, 1.0f) : 0.0f;
  if (lat != 0.0f) lat = std::copysign(std::pow(std::fabs(lat), kPanExponent), lat);
  c.pan = lat;
  float s = std::fabs(lat);
  float theta = std::asin(std::clamp(s, 0.0f, 1.0f));
  c.itd_samples = kMaxItdSeconds * (float)sample_rate * ((theta + s) / kWoodworthMax) * (lat < 0 ? -1.0f : lat > 0 ? 1.0f : 0.0f);
  c.far_shadow_db = -kShadowMaxDb * s;
  if (dist > 1e-4f) {
    float north_frac = std::clamp(north / dist, -1.0f, 1.0f);
    if (north_frac < 0.0f) c.rear_shelf_db = kRearMaxCutDb * north_frac;
  }
  return c;
}

void spatial_pan_gains(float pan, float& left, float& right) {
  float mag = std::clamp(std::fabs(pan), 0.0f, 1.0f);
  float far_g = std::pow(10.0f, -kMaxIldDb * mag / 20.0f);
  float norm = 1.0f / std::sqrt(1.0f + far_g * far_g);
  float near_g = norm;
  far_g *= norm;
  if (pan >= 0.0f) { right = near_g; left = far_g; }
  else { left = near_g; right = far_g; }
}

// RBJ cookbook high shelf, slope 1.
void Shelf::set(float corner_hz, float gain_db, int sample_rate) {
  if (std::fabs(gain_db) < 0.01f) { b0 = 1; b1 = b2 = a1 = a2 = 0; return; }
  float A = std::pow(10.0f, gain_db / 40.0f);
  float w0 = 2.0f * kPi * corner_hz / (float)sample_rate;
  float cw = std::cos(w0), sw = std::sin(w0);
  float alpha = sw / 2.0f * std::sqrt(2.0f);   // S = 1
  float sa = 2.0f * std::sqrt(A) * alpha;
  float a0 = (A + 1) - (A - 1) * cw + sa;
  b0 = A * ((A + 1) + (A - 1) * cw + sa) / a0;
  b1 = -2 * A * ((A - 1) + (A + 1) * cw) / a0;
  b2 = A * ((A + 1) + (A - 1) * cw - sa) / a0;
  a1 = 2 * ((A - 1) - (A + 1) * cw) / a0;
  a2 = ((A + 1) - (A - 1) * cw - sa) / a0;
}

void SpatialVoice::process(const float* mono, int frames, const SpatialCue& cue, int sample_rate, float* left, float* right) {
  float tl, tr;
  spatial_pan_gains(cue.pan, tl, tr);
  float titd = std::clamp(cue.itd_samples, -(float)(kDelay - 2), (float)(kDelay - 2));
  if (!started_) { gl_ = tl; gr_ = tr; itd_ = titd; started_ = true; }
  // The filters take the block's cue (a shelf's gain changing per 10 ms block is inaudible).
  rear_.set(kRearCornerHz, cue.rear_shelf_db, sample_rate);
  // The far ear is the one away from the side the sound is on: east (pan > 0) shadows the left.
  shadow_l_.set(kShadowCornerHz, cue.pan > 0 ? cue.far_shadow_db : 0.0f, sample_rate);
  shadow_r_.set(kShadowCornerHz, cue.pan < 0 ? cue.far_shadow_db : 0.0f, sample_rate);
  const float n = frames > 0 ? (float)frames : 1.0f;
  const float dgl = (tl - gl_) / n, dgr = (tr - gr_) / n, ditd = (titd - itd_) / n;
  for (int i = 0; i < frames; ++i) {
    gl_ += dgl; gr_ += dgr; itd_ += ditd;
    float s = rear_.run(mono[i]);
    line_l_[head_] = s * gl_;
    line_r_[head_] = s * gr_;
    // The lagging ear reads `itd` samples back (linear interpolation); the near ear reads now.
    float d = std::fabs(itd_);
    auto tap = [&](const float* line) {
      int k = (int)d;
      float frac = d - (float)k;
      int a = (head_ - k + kDelay) % kDelay, b = (head_ - k - 1 + kDelay) % kDelay;
      return line[a] + (line[b] - line[a]) * frac;
    };
    float l = itd_ > 0 ? tap(line_l_) : line_l_[head_];
    float r = itd_ < 0 ? tap(line_r_) : line_r_[head_];
    left[i] += shadow_l_.run(l);
    right[i] += shadow_r_.run(r);
    head_ = (head_ + 1) % kDelay;
  }
}

}  // namespace p3a::core

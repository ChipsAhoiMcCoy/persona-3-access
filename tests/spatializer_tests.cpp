#include "doctest.h"
#include "core/spatializer.h"
#include <cmath>
#include <vector>

using namespace p3a::core;

TEST_CASE("spatial_cue: ahead is centred and untouched, east pans right and delays the left ear") {
  SpatialCue c = spatial_cue(0, 5, 3, 48000);
  CHECK(c.pan == doctest::Approx(0.0f));
  CHECK(c.itd_samples == doctest::Approx(0.0f));
  CHECK(c.rear_shelf_db == doctest::Approx(0.0f));
  c = spatial_cue(5, 0, 3, 48000);
  CHECK(c.pan == doctest::Approx(1.0f));
  CHECK(c.itd_samples == doctest::Approx(0.00066f * 48000).epsilon(0.01));
  CHECK(c.far_shadow_db == doctest::Approx(-8.0f));
  c = spatial_cue(-5, 0, 3, 48000);
  CHECK(c.pan == doctest::Approx(-1.0f));
  CHECK(c.itd_samples < 0);
}

TEST_CASE("spatial_cue: only behind is darkened, most at due south") {
  CHECK(spatial_cue(0, -5, 3, 48000).rear_shelf_db == doctest::Approx(-10.0f));
  CHECK(spatial_cue(3, -3, 3, 48000).rear_shelf_db == doctest::Approx(-7.07f).epsilon(0.01));
  CHECK(spatial_cue(3, 3, 3, 48000).rear_shelf_db == doctest::Approx(0.0f));
}

TEST_CASE("spatial_cue: within the pan width the sideways offset itself pans") {
  // 1 m east, 0.1 m north: by bearing nearly hard right, but within 3 m it is a third of the way.
  SpatialCue c = spatial_cue(1, 0.1f, 3, 48000);
  CHECK(c.pan == doctest::Approx(std::pow(1.0f / 3.0f, 0.82f)).epsilon(0.01));
}

TEST_CASE("spatial_pan_gains: constant power, the far ear capped at 12 dB") {
  float l, r;
  spatial_pan_gains(0, l, r);
  CHECK(l == doctest::Approx(0.7071f).epsilon(0.001));
  CHECK(r == doctest::Approx(0.7071f).epsilon(0.001));
  spatial_pan_gains(1, l, r);
  CHECK(l * l + r * r == doctest::Approx(1.0f));
  CHECK(20.0f * std::log10(r / l) == doctest::Approx(12.0f));
  spatial_pan_gains(-1, l, r);
  CHECK(l > r);
}

TEST_CASE("SpatialVoice: a sound east is louder right and reaches the left ear later") {
  SpatialVoice v;
  std::vector<float> in(480, 0.0f), l(480, 0.0f), r(480, 0.0f);
  in[0] = 1.0f;   // an impulse
  SpatialCue c = spatial_cue(5, 0, 3, 48000);
  v.process(in.data(), 480, c, 48000, l.data(), r.data());
  int first_l = -1, first_r = -1;
  for (int i = 0; i < 480; ++i) {
    if (first_l < 0 && std::fabs(l[i]) > 1e-4f) first_l = i;
    if (first_r < 0 && std::fabs(r[i]) > 1e-4f) first_r = i;
  }
  CHECK(first_r == 0);
  CHECK(first_l >= 30);   // ~31.7 samples at 48 kHz
  float el = 0, er = 0;
  for (int i = 0; i < 480; ++i) { el += l[i] * l[i]; er += r[i] * r[i]; }
  CHECK(er > el * 4);
}

TEST_CASE("Shelf: zero gain passes through, a cut lowers highs but not lows") {
  Shelf s;
  s.set(3000, 0, 48000);
  CHECK(s.run(0.5f) == doctest::Approx(0.5f));
  // DC through a -10 dB high shelf stays near unity; Nyquist drops ~10 dB.
  Shelf dc; dc.set(3000, -10, 48000);
  float y = 0;
  for (int i = 0; i < 2000; ++i) y = dc.run(1.0f);
  CHECK(y == doctest::Approx(1.0f).epsilon(0.02));
  Shelf ny; ny.set(3000, -10, 48000);
  for (int i = 0; i < 2000; ++i) y = ny.run(i % 2 ? 1.0f : -1.0f);
  CHECK(std::fabs(y) == doctest::Approx(std::pow(10.0f, -0.5f)).epsilon(0.05));
}

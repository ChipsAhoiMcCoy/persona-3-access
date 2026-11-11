// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
// Wall tones, after wotr-access and Grim Dark: four authored loops (assets/audio/walltones/2: north.wav ahead and
// south.wav behind both centred, east.wav hard right, west.wav hard left), each at volume (1 - d/range)^2 of the free
// distance in its direction, updated every tick. Directions are CAMERA-relative: P3R moves the player relative to
// the camera, so "ahead" is where pushing up goes.
// The probe is Grim Dark's (2026-09-22, ported 2026-10-29): each direction's distance is how far HOLDING THAT KEY
// would carry you along it before the walk stops or leaves a truncated cone (half-width g_h0 at the feet, widening
// g_deg per side). The walk is simulated the way UE's character movement resolves it: the body (a Pawn-profile
// capsule) is swept in steps; on a hit it slides along the wall (the move minus its part into the wall's normal);
// each step follows the floor down a line trace, and a low line catches knee-high blockers (benches, planters) the
// capsule rides over. So a wall you would slide along, or a corridor that bends a little, reads open; a wall across
// the key reads at its distance. The old probe (one straight sweep per direction) read any wall that crossed the
// line, even one you would glide past.
#include "walltones.h"
#include "audio.h"
#include "devserver.h"
#include "mod.h"
#include "world.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <format>
#include <vector>

namespace p3a::walltones {
using world::FVector;
static float g_range = 500.0f;        // cm
static float g_gain = 1.0f;
static bool g_enabled = true;
static bool g_loaded = false;
static float g_dist[4] = {0, 0, 0, 0};
static float g_h0 = 60.0f, g_deg = 20.0f;   // the cone: half-width at the feet (cm), widening per side (degrees)
// Per-file trims in dB (ahead, right, behind, left): Grim Dark's by the user's ear (2026-09-01): south.wav is the
// lowest-pitched file (478 Hz) and needs +3 dB to sound as loud as the others.
static float g_trim_db[4] = {0.0f, 0.0f, 3.0f, 0.0f};
constexpr const char* kFile[4] = {"north.wav", "east.wav", "south.wav", "west.wav"};   // ahead, right, behind, left
constexpr float kPan[4] = {0.0f, 1.0f, 0.0f, -1.0f};
constexpr int kToneId = 100;
constexpr float kRadius = 20.0f, kHalfHeight = 40.0f;   // the swept body: 60 cm above the floor to ~1.4 m
constexpr float kLowAboveFeet = 25.0f;
// People (NPCs, Shadows, anyone but the player) are not walls: the tones describe the room (the user's call,
// 2026-10-29). The traces look past each one met; the list lives for one tick (no UObject* across ticks).
static std::vector<ue::UObject*> g_people;

static void ensure_loaded() {
  if (g_loaded) return;
  g_loaded = true;
  std::string dir = audio::module_dir() + "assets\\audio\\walltones\\2\\";
  for (int i = 0; i < 4; ++i) {
    audio::load_loop(kToneId + i, dir + kFile[i], kPan[i]);
    audio::set_loop_gain(kToneId + i, std::pow(10.0f, g_trim_db[i] / 20.0f));
  }
}
static void silence() { if (g_loaded) for (int i = 0; i < 4; ++i) audio::set_loop_volume(kToneId + i, 0.0f); }

constexpr float kStep = 100.0f;      // cm per simulated step
constexpr float kFeetBelow = 90.0f;  // actor location is the capsule centre, ~90 cm above the feet

static float hdot(const FVector& a, const FVector& b) { return a.x * b.x + a.y * b.y; }

// Moves the body from `pos` by `move` (horizontal), sliding once along whatever it meets. Returns false when the body
// starts inside geometry (nowhere to go).
static bool slide_move(FVector& pos, FVector move) {
  for (int pass = 0; pass < 2; ++pass) {
    FVector to{pos.x + move.x, pos.y + move.y, pos.z};
    world::Hit h = world::sweep(FVector{pos.x, pos.y, pos.z + 10}, FVector{to.x, to.y, to.z + 10}, kRadius, kHalfHeight, &g_people);
    if (!h.hit) { pos = to; return true; }
    if (h.start_inside) return false;
    float len = std::sqrt(hdot(move, move));
    float t = len > 0 ? std::clamp(h.distance / len, 0.0f, 1.0f) : 1.0f;
    pos = FVector{pos.x + move.x * t, pos.y + move.y * t, pos.z};
    FVector n{h.normal.x, h.normal.y, 0};
    float nl = std::sqrt(hdot(n, n));
    if (nl < 1e-3f) return true;
    n = FVector{n.x / nl, n.y / nl, 0};
    FVector rest{move.x * (1 - t), move.y * (1 - t), 0};
    float into = hdot(rest, n);
    move = FVector{rest.x - n.x * into, rest.y - n.y * into, 0};   // the part along the wall
    pos = FVector{pos.x + n.x * 0.5f, pos.y + n.y * 0.5f, pos.z};   // off the surface, or the slide starts inside it
    if (hdot(move, move) < 1.0f) return true;
  }
  return true;
}

// How far along camera-relative direction `d` (0 ahead, 1 right, 2 behind, 3 left) holding that key carries the body,
// capped at the range.
static float probe(const FVector& p, int d) {
  FVector v = world::dir_vector(d);
  float tan_deg = std::tan(g_deg * 3.14159265f / 180.0f);
  FVector pos = p;
  float along = 0;
  for (int step = 0; step < 16 && along < g_range; ++step) {
    float len = std::min(kStep, g_range - along);
    FVector from = pos;
    if (!slide_move(pos, FVector{v.x * len, v.y * len, 0})) break;
    // Knee-high blockers the capsule rides over; a rising floor (a ramp, a low step) is not one.
    float low_z = from.z - kFeetBelow + kLowAboveFeet;
    world::Hit low = world::line(FVector{from.x, from.y, low_z}, FVector{pos.x, pos.y, low_z}, "Pawn", &g_people);
    if (low.hit && !low.start_inside && low.normal.z < 0.6f) {
      float moved = std::sqrt(hdot(FVector{pos.x - from.x, pos.y - from.y, 0}, FVector{pos.x - from.x, pos.y - from.y, 0}));
      float t = moved > 0 ? std::clamp((low.distance - kRadius) / moved, 0.0f, 1.0f) : 0.0f;
      pos = FVector{from.x + (pos.x - from.x) * t, from.y + (pos.y - from.y) * t, pos.z};
      along = std::max(along, hdot(FVector{pos.x - p.x, pos.y - p.y, 0}, v));
      break;
    }
    // Follow the floor; no floor under the step (a drop) ends the walk where it was.
    world::Hit floor = world::line(FVector{pos.x, pos.y, pos.z + 50}, FVector{pos.x, pos.y, pos.z - kFeetBelow - 60}, "Pawn", &g_people);
    if (!floor.hit) { pos = from; break; }
    pos.z = floor.point.z + kFeetBelow;
    FVector off{pos.x - p.x, pos.y - p.y, 0};
    float a = hdot(off, v);
    float lateral = std::abs(off.x * v.y - off.y * v.x);
    if (lateral > g_h0 + std::max(a, 0.0f) * tan_deg) break;   // slid out of the cone: the key no longer goes this way
    if (a - along < len * 0.1f) { along = std::max(along, a); break; }   // stuck against a wall across the key
    along = a;
  }
  return std::min(std::max(along, 0.0f), g_range);
}

// The tones play while the player walks the field with nothing over it (not in a menu, a conversation, a battle or
// a story scene).
static bool walking() { return world::exploring(); }

// The sound test's tone: a sound's own length (1.2 s), not a wait on anything.
static int g_demo = -1;
static ULONGLONG g_demo_until = 0;
void demo(int dir) {
  if (dir < 0 || dir > 3) return;
  g_demo = dir;
  g_demo_until = GetTickCount64() + 1200;
}

void tick() {
  if (g_demo >= 0) {
    if (GetTickCount64() < g_demo_until) {
      ensure_loaded();
      for (int i = 0; i < 4; ++i) audio::set_loop_volume(kToneId + i, i == g_demo ? 0.6f * g_gain : 0.0f);
      return;
    }
    g_demo = -1;
  }
  if (!g_enabled || !walking()) { silence(); return; }
  ensure_loaded();
  g_people.clear();
  FVector p;
  if (!world::player_location(p)) { silence(); return; }
  for (int i = 0; i < 4; ++i) {
    float d = probe(p, i);
    g_dist[i] = d;
    float v = d >= g_range ? 0.0f : 1.0f - d / g_range;
    audio::set_loop_volume(kToneId + i, v * v * g_gain);
  }
}

void set_enabled(bool on) { g_enabled = on; if (!on) silence(); }
std::string status() {
  return std::format("walltones enabled={} range={:.0f} gain={:.2f} h0={:.0f} deg={:.0f} ahead={:.0f} right={:.0f} behind={:.0f} left={:.0f}\n", g_enabled, g_range, g_gain, g_h0, g_deg,
                     g_dist[0], g_dist[1], g_dist[2], g_dist[3]);
}

void install() {
  if (env_flag(L"P3A_NOWALLTONES")) g_enabled = false;   // dev launches: quiet while testing
  dev::route("/walltones", [](const dev::Request& r, int&) {
    if (dev::has(r, "on")) set_enabled(dev::arg(r, "on") == "1");
    if (dev::has(r, "range")) g_range = (float)dev::arg_num(r, "range", g_range);
    if (dev::has(r, "vol")) g_gain = (float)dev::arg_num(r, "vol", g_gain);
    if (dev::has(r, "h0")) g_h0 = (float)dev::arg_num(r, "h0", g_h0);
    if (dev::has(r, "deg")) g_deg = std::clamp((float)dev::arg_num(r, "deg", g_deg), 0.0f, 80.0f);
    // dev: the probing part of one tick, averaged (on the game thread: probe() shares g_people with tick)
    if (dev::has(r, "time")) return dev::on_game([n = std::clamp((int)dev::arg_num(r, "time", 100), 1, 2000)] {
      FVector p;
      if (!world::player_location(p)) return std::string("no player\n");
      LARGE_INTEGER f, t0, t1;
      QueryPerformanceFrequency(&f); QueryPerformanceCounter(&t0);
      float d[4];
      for (int k = 0; k < n; ++k) { g_people.clear(); for (int i = 0; i < 4; ++i) d[i] = probe(p, i); }
      QueryPerformanceCounter(&t1);
      return std::format("{:.1f} us per tick ({} ticks): ahead={:.0f} right={:.0f} behind={:.0f} left={:.0f}\n",(double)(t1.QuadPart - t0.QuadPart) * 1e6 / (double)f.QuadPart / n, n, d[0], d[1], d[2], d[3]);
    });
    return status();
  }, "?on=1|0&range=cm&vol=0..1&h0=cm&deg=&time=N wall tones state, tuning, probe cost");
}
}  // namespace p3a::walltones

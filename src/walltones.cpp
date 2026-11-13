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
// capsule rides over, unless it is a stair's riser (the floor past it no more than g_rise higher). So a wall you would
// slide along, a staircase, or a corridor that bends a little, reads open; a wall across
// the key reads at its distance. The old probe (one straight sweep per direction) read any wall that crossed the
// line, even one you would glide past.
// In the top-down mode (topdown.cpp) the four directions are the compass, fixed, so a room had a wall within reach in
// three or four of them all the time and turning no longer quietened them (the user, 2026-11-12). There, as Dark
// Vision starts its wall tones ("when moving"): they sound only while the player moves the hero (the movement input, so
// the stick, the keys and the auto-walk alike, even into a wall), reach 3 m, and the direction behind the walk is left out.
#include "walltones.h"
#include "audio.h"
#include "devserver.h"
#include "mod.h"
#include "topdown.h"
#include "world.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <format>
#include <vector>

namespace p3a::walltones {
using world::FVector;
static float g_range = 500.0f;        // cm
constexpr float kTopDownRange = 300.0f;   // cm, the top-down mode's reach
constexpr float kMovingCmS = 30.0f;       // the hero moves: faster than this on the ground (a walk is ~250)
constexpr float kBehindDeg = 100.0f;      // top-down: a direction more than this from the walk's is behind it
static bool g_moving = false;
static bool g_asking = false;             // the player pushes a way (the stick, the keys, the auto-walk)
constexpr float kAskingInput = 0.2f;      // the input vector's length above which the player pushes a way
static float g_heading = 0;               // top-down: the walk's bearing from north, degrees
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

// cm per simulated step. 100 at first: a staircase rises ~55 cm in a metre (Tartarus' exit stairs: 25 cm risers every
// ~45 cm), more than the floor below the step reaches down for coming down, and nearly the swept body's 60 cm going up.
static float g_step = 50.0f;      // dev: /walltones?step=
// The tallest riser the walk goes up: a stair's (15-25 cm), not a bench's or a planter's (40 cm and more).
static float g_rise = 30.0f;      // dev: /walltones?rise= (0: no riser is a step)
constexpr float kFeetBelow = 90.0f;  // actor location is the capsule centre, ~90 cm above the feet

static float hdot(const FVector& a, const FVector& b) { return a.x * b.x + a.y * b.y; }

// The floor's height a little before and a little past `at` (a face the low line met) along `v`, read down from
// above anything the walk could step onto. A step: the floor past it is no more than g_rise higher.
static bool step_up(const FVector& at, const FVector& v, float feet_z) {
  auto floor_z = [&](float d, float& z) {
    FVector q{at.x + v.x * d, at.y + v.y * d, feet_z + kLowAboveFeet + g_rise + 10};
    world::Hit h = world::line(q, FVector{q.x, q.y, feet_z - 60}, "Pawn", &g_people);
    if (!h.hit || h.start_inside || h.normal.z < 0.7f) return false;
    z = h.point.z;
    return true;
  };
  float before = 0, after = 0;
  return floor_z(-5, before) && floor_z(5, after) && after > before && after - before <= g_rise;
}

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
static float probe_along(const FVector& p, const FVector& v, float range) {
  float tan_deg = std::tan(g_deg * 3.14159265f / 180.0f);
  FVector pos = p;
  float along = 0;
  for (int step = 0; step < 32 && along < range; ++step) {
    float len = std::min(g_step, range - along);
    FVector from = pos;
    if (!slide_move(pos, FVector{v.x * len, v.y * len, 0})) break;
    // Knee-high blockers the capsule rides over; a rising floor (a ramp) is not one, nor a stair's riser: the stairs
    // up out of a Tartarus floor read as a wall at their foot (2026-11-13).
    float low_z = from.z - kFeetBelow + kLowAboveFeet;
    world::Hit low = world::line(FVector{from.x, from.y, low_z}, FVector{pos.x, pos.y, low_z}, "Pawn", &g_people);
    if (low.hit && !low.start_inside && low.normal.z < 0.6f && !step_up(low.point, v, from.z - kFeetBelow)) {
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
  return std::min(std::max(along, 0.0f), range);
}
static float probe(const FVector& p, int d, float range) { return probe_along(p, world::dir_vector(d), range); }

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
  const bool top = topdown::active();
  // Moving is what the player asks for, not how far the hero went: walked straight into a wall the hero's velocity is
  // nothing, and the tones fell silent just as the wall was nearest. The velocity stands in where no input is read.
  FVector in{}, vel{};
  g_asking = world::player_input(in) && std::hypot(in.x, in.y) > kAskingInput;
  g_moving = g_asking || (world::player_velocity(vel) && std::hypot(vel.x, vel.y) > kMovingCmS);
  if (top && !g_moving) { silence(); return; }
  const float range = top ? kTopDownRange : g_range;
  if (top) {
    const FVector& d = g_asking ? in : vel;
    g_heading = std::atan2(d.y, d.x) * 180.0f / 3.14159265f - topdown::north_yaw();
  }
  for (int i = 0; i < 4; ++i) {
    if (top && std::fabs(std::remainder(90.0f * i - g_heading, 360.0f)) > kBehindDeg) {
      g_dist[i] = range;
      audio::set_loop_volume(kToneId + i, 0.0f);
      continue;
    }
    float d = probe(p, i, range);
    g_dist[i] = d;
    float v = d >= range ? 0.0f : 1.0f - d / range;
    audio::set_loop_volume(kToneId + i, v * v * g_gain);
  }
}

void set_enabled(bool on) { g_enabled = on; if (!on) silence(); }
std::string status() {
  return std::format("walltones enabled={} range={:.0f} gain={:.2f} h0={:.0f} deg={:.0f} ahead={:.0f} right={:.0f} behind={:.0f} left={:.0f} topdown={} moving={} asking={} heading={:.0f}\n", g_enabled, g_range, g_gain, g_h0, g_deg,
                     g_dist[0], g_dist[1], g_dist[2], g_dist[3], topdown::active(), g_moving, g_asking, g_heading);
}

void install() {
  if (env_flag(L"P3A_NOWALLTONES")) g_enabled = false;   // dev launches: quiet while testing
  dev::route("/walltones", [](const dev::Request& r, int&) {
    if (dev::has(r, "on")) set_enabled(dev::arg(r, "on") == "1");
    if (dev::has(r, "range")) g_range = (float)dev::arg_num(r, "range", g_range);
    if (dev::has(r, "vol")) g_gain = (float)dev::arg_num(r, "vol", g_gain);
    if (dev::has(r, "h0")) g_h0 = (float)dev::arg_num(r, "h0", g_h0);
    if (dev::has(r, "step")) g_step = std::clamp((float)dev::arg_num(r, "step", g_step), 20.0f, 200.0f);
    if (dev::has(r, "rise")) g_rise = std::clamp((float)dev::arg_num(r, "rise", g_rise), 0.0f, 60.0f);
    if (dev::has(r, "deg")) g_deg = std::clamp((float)dev::arg_num(r, "deg", g_deg), 0.0f, 80.0f);
    // dev: the probe from a place along a world bearing (?from=x,y,z,yaw: z the hero's centre, ~90 cm above the feet)
    if (dev::has(r, "from")) return dev::on_game([s = dev::arg(r, "from")] {
      float a[4] = {};
      if (sscanf(s.c_str(), "%f,%f,%f,%f", &a[0], &a[1], &a[2], &a[3]) != 4) return std::string("bad\n");
      float yaw = a[3] * 3.14159265f / 180.0f;
      g_people.clear();
      return std::format("{:.0f}\n", probe_along(FVector{a[0], a[1], a[2]}, FVector{std::cos(yaw), std::sin(yaw), 0}, g_range));
    });
    // dev: the floor's height along a line, every 10 cm, read down from ztop (?prof=x0,y0,x1,y1,ztop)
    if (dev::has(r, "prof")) return dev::on_game([s = dev::arg(r, "prof")] {
      float a[5] = {};
      if (sscanf(s.c_str(), "%f,%f,%f,%f,%f", &a[0], &a[1], &a[2], &a[3], &a[4]) != 5) return std::string("bad\n");
      float len = std::hypot(a[2] - a[0], a[3] - a[1]);
      std::string o;
      for (float t = 0; t <= len; t += 10) {
        float x = a[0] + (a[2] - a[0]) * t / len, y = a[1] + (a[3] - a[1]) * t / len;
        world::Hit h = world::line(FVector{x, y, a[4]}, FVector{x, y, a[4] - 1500}, "Pawn");
        o += std::format("{:.0f} {:.1f} {} n.z={:.2f}\n", t, h.hit ? h.point.z : -99999.0f, h.actor ? ue::name_of(h.actor) : "-", h.normal.z);
      }
      return o;
    });
    // dev: the probing part of one tick, averaged (on the game thread: probe() shares g_people with tick)
    if (dev::has(r, "time")) return dev::on_game([n = std::clamp((int)dev::arg_num(r, "time", 100), 1, 2000)] {
      FVector p;
      if (!world::player_location(p)) return std::string("no player\n");
      LARGE_INTEGER f, t0, t1;
      QueryPerformanceFrequency(&f); QueryPerformanceCounter(&t0);
      float d[4];
      for (int k = 0; k < n; ++k) { g_people.clear(); for (int i = 0; i < 4; ++i) d[i] = probe(p, i, g_range); }
      QueryPerformanceCounter(&t1);
      return std::format("{:.1f} us per tick ({} ticks): ahead={:.0f} right={:.0f} behind={:.0f} left={:.0f}\n",(double)(t1.QuadPart - t0.QuadPart) * 1e6 / (double)f.QuadPart / n, n, d[0], d[1], d[2], d[3]);
    });
    return status();
  }, "?on=1|0&range=cm&vol=0..1&h0=cm&deg=&step=cm&rise=cm&time=N&from=x,y,z,yaw&prof=x0,y0,x1,y1,ztop wall tones state, tuning, probe cost");
}
}  // namespace p3a::walltones

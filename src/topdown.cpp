// The top-down mode: the camera fixed north, the game's and the mod's sounds heard from above, the right stick's scan.
// Reflection only (no fixed addresses). What it reads and writes:
//  - AFldOperator (one in the persistent level): KeyState (+0x280, EFldOperatorKeyState: Enable = 3), State (+0x284,
//    EFldOperatorState: Free = 1; Battle 11, Event 12, EventEnd 13), FreeCamera (+0x240, AFldCameraFree);
//    SetFreeCameraYaw(Angle) turns the free camera's YawSceneComp (its relative yaw is the world yaw).
//  - UFldCameraBehaviorFree (the free camera's Behavior, +0x270): YawParam (+0xE8, FldCameraRotParam: Speed first,
//    125 by default) is what the stick turns it by; CorrectionParam (+0x120, Margin first, 16 degrees) is the follow
//    that swings it behind the walking hero (native 0x110b090: it acts while the error is above Margin and under 175).
//    Speed 0 and Margin 180 hold it still (tested on Tartarus floor 259, 2026-11-12: keys, stick and strafing).
//  - APlayerController.SetAudioListenerOverride(null, Location, Rotation) / ClearAudioListenerOverride: CRI Atom's
//    listener (native 0xd05c60) takes GetAudioListenerPosition every frame, which honours the override.
#include "topdown.h"
#include "audio.h"
#include "core/sonar_cues.h"
#include "core/strings.h"
#include "core/units.h"
#include "devserver.h"
#include "input.h"
#include "log.h"
#include "poi.h"
#include "settings.h"
#include "sonar.h"
#include "speech.h"
#include "watch.h"
#include "world.h"
#include <cmath>
#include <format>
#include <vector>

namespace p3a::topdown {
namespace {
using namespace ue;
using world::FVector;

constexpr float kNorthYaw = -90.0f;     // world -Y: the maps' up
constexpr int kStateFree = 1, kKeyEnable = 3;
constexpr float kStopSpeed = 0.0f, kStopMargin = 180.0f;
constexpr float kScanDeadzone = 0.55f;   // of the stick's travel: a direction is chosen past it
constexpr float kScanHalfCone = 22.5f;   // degrees either side of the direction
constexpr float kScanWallCm = 2000.0f;

bool g_active = false;
float g_listener_h = 0.0f;   // cm above the hero's middle (dev: /topdown?h=)
bool g_listener_set = false;
// The camera behaviour's own values, kept while the mode holds them.
WeakObj g_behavior;
float g_saved_speed = 125.0f, g_saved_margin = 16.0f;
bool g_saved = false;
int g_scan_dir = -1;   // the compass point the stick is on, -1 at rest
bool g_dev_stick = false;
float g_dev_x = 0, g_dev_y = 0;
uint64_t g_relocks = 0;
// A run of frames each needing the camera turned back (3553 in one early session, 2026-11-12, cause unknown): its
// start and end are logged with what the camera was doing, to find what fights the lock.
int g_relock_run = 0;
bool g_relocked_now = false;

UObject* op() {
  static const int ops = watch::want("FldOperator");
  return watch::get(ops);
}

bool operator_free(UObject* o) {
  return o && get<int32_t>(o, "State", -1) == kStateFree && get<int32_t>(o, "KeyState", -1) == kKeyEnable;
}

UObject* behavior_of(UObject* o) {
  UObject* cam = o ? get<UObject*>(o, "FreeCamera") : nullptr;
  return cam ? get<UObject*>(cam, "Behavior") : nullptr;
}

// The behaviour's yaw speed and follow margin, where they live in this object.
float* speed_of(UObject* b) { int32_t off = b ? offset_of(class_of(b), "YawParam") : -1; return off < 0 ? nullptr : (float*)((uint8_t*)b + off); }
float* margin_of(UObject* b) { int32_t off = b ? offset_of(class_of(b), "CorrectionParam") : -1; return off < 0 ? nullptr : (float*)((uint8_t*)b + off); }

void hold_camera(UObject* o) {
  UObject* b = behavior_of(o);
  float* speed = speed_of(b);
  float* margin = margin_of(b);
  if (!speed || !margin) return;
  if (!g_saved || g_behavior.resolve() != b) {
    // A new camera (another field): its own values are kept, unless they are already ours.
    g_saved_speed = *speed != kStopSpeed ? *speed : g_saved_speed;
    g_saved_margin = *margin != kStopMargin ? *margin : g_saved_margin;
    g_behavior = WeakObj::of(b);
    g_saved = true;
  }
  *speed = kStopSpeed;
  *margin = kStopMargin;
  UObject* cam = get<UObject*>(o, "FreeCamera");
  UObject* yaw_comp = cam ? get<UObject*>(cam, "YawSceneComp") : nullptr;
  float yaw = yaw_comp ? get<FRotator>(yaw_comp, "RelativeRotation").yaw : 0.0f;
  float diff = std::remainder(yaw - kNorthYaw, 360.0f);
  g_relocked_now = !yaw_comp || std::fabs(diff) > 0.5f;
  if (g_relocked_now) {
    if (g_relock_run == 0) {
      UObject* cop = get<UObject*>(o, "OpCameraComp");
      UObject* hit = cop ? get<UObject*>(cop, "CurrentHitRef") : nullptr;
      log::writef("topdown: camera turned to {:.1f}, set back to north (behaviour state {}, hit camera {}, view yaw {:.1f})", yaw,
                  get<int32_t>(b, "State", -1), hit ? name_of(hit) : "none", world::camera_yaw());
    }
    ++g_relock_run;
    Params p(find_function(class_of(o), "SetFreeCameraYaw"));
    p.set<float>("Angle", kNorthYaw);
    call(o, p);
    ++g_relocks;
  }
}

// The end of a run of set-backs: how long it lasted.
void relock_run_end() {
  if (g_relocked_now || g_relock_run == 0) return;
  if (g_relock_run > 1) log::writef("topdown: the camera was set back {} frames in a row", g_relock_run);
  g_relock_run = 0;
}

void release_camera() {
  if (!g_saved) return;
  if (UObject* b = g_behavior.resolve()) {
    if (float* s = speed_of(b)) *s = g_saved_speed;
    if (float* m = margin_of(b)) *m = g_saved_margin;
  }
  g_saved = false;
  g_behavior = {};
}

void hold_listener() {
  UObject* pc = player_controller();
  FVector me;
  if (!pc || !world::player_location(me)) return;
  Params p(find_function(class_of(pc), "SetAudioListenerOverride"));
  p.set<UObject*>("AttachToComponent", nullptr);
  p.set<FVector>("Location", FVector{me.x, me.y, me.z + g_listener_h});
  p.set<FRotator>("Rotation", FRotator{0.0f, kNorthYaw, 0.0f});
  g_listener_set = call(pc, p);
}

void release_listener() {
  if (!g_listener_set) return;
  g_listener_set = false;
  if (UObject* pc = player_controller()) call(pc, "ClearAudioListenerOverride");
}

// ---- the right stick's scan ----
void scan_say(int dir) {
  FVector me;
  if (!world::player_location(me)) return;
  const float want = dir * 45.0f;
  const poi::Poi* best = nullptr;
  std::vector<poi::Poi> all = poi::scan();   // nearest first; bearings from north while the mode holds
  for (const poi::Poi& p : all)
    if (std::fabs(std::remainder(p.bearing - want, 360.0f)) <= kScanHalfCone) { best = &p; break; }
  // The wall that way: a line at the body's height, people looked past.
  float rad = (kNorthYaw + want) * 3.14159265f / 180.0f;
  FVector to{me.x + std::cos(rad) * kScanWallCm, me.y + std::sin(rad) * kScanWallCm, me.z};
  std::vector<UObject*> people;
  world::Hit wall = world::line(me, to, "Pawn", &people);
  std::string line = std::format("{}: {}", strings::kCompass[dir],
                                 best ? std::format("{}, {}", best->name, core::meters(best->dist / 100.0f)) : std::string(strings::kScanNothing));
  line += std::format(". {}", wall.hit ? std::format("{} {}", strings::kScanWall, core::meters(wall.distance / 100.0f)) : std::string(strings::kScanOpen));
  speech::speak(line, true);
  if (best) sonar::sound(*best);
}

void scan_tick() {
  float x = 0, y = 0;
  if (g_dev_stick) { x = g_dev_x; y = g_dev_y; }
  else input::right_stick(x, y);
  if (std::hypot(x, y) < kScanDeadzone) { g_scan_dir = -1; return; }
  // Up is north, right is east: the bearing clockwise from north.
  float b = std::atan2(x, y) * 180.0f / 3.14159265f;
  int dir = ((int)std::lround(b / 45.0f) % 8 + 8) % 8;
  if (dir == g_scan_dir) return;
  g_scan_dir = dir;
  scan_say(dir);
}

void enter() {
  g_active = true;
  world::set_north(true, kNorthYaw);
  input::claim_right_stick(true);
  log::write("topdown: on (camera north, listener on the hero)");
}

void leave() {
  g_active = false;
  world::set_north(false, kNorthYaw);
  input::claim_right_stick(false);
  release_camera();
  release_listener();
  g_scan_dir = -1;
  log::write("topdown: off (camera and listener back to the game)");
}
}  // namespace

bool active() { return g_active; }
float north_yaw() { return kNorthYaw; }

void tick() {
  UObject* o = op();
  bool want = settings::on(settings::TopDown) && world::exploring() && operator_free(o);
  if (want != g_active) want ? enter() : leave();
  if (!g_active) return;
  hold_camera(o);
  relock_run_end();
  hold_listener();
  scan_tick();
}

// The unload runs off the game thread: only the camera's two values are written back (plain memory). The listener
// override stays until the next mod's mode sets or clears it.
void shutdown() { release_camera(); }

void install() {
  dev::route("/topdown", [](const dev::Request& r, int&) {
    // /topdown | ?h=cm the listener's height | ?stick=x,y the scan as if the stick were there (?stick=off ends it)
    std::string h = dev::arg(r, "h"), stick = dev::arg(r, "stick");
    return dev::on_game([h, stick] {
      if (!h.empty()) g_listener_h = (float)atof(h.c_str());
      if (stick == "off") g_dev_stick = false;
      else if (size_t c = stick.find(','); c != std::string::npos) {
        g_dev_stick = true;
        g_dev_x = (float)atof(stick.c_str());
        g_dev_y = (float)atof(stick.c_str() + c + 1);
      }
      UObject* o = op();
      UObject* b = behavior_of(o);
      float* sp = speed_of(b);
      float* mg = margin_of(b);
      return std::format("topdown setting={} active={} exploring={} operator state={} keys={} listener_set={} h={:.0f}\n"
                         "camera speed={:.1f} margin={:.1f} saved={} ({:.1f}, {:.1f}) relocks={} camera_yaw={:.1f} scan_dir={}\n",
                         settings::on(settings::TopDown), g_active, world::exploring(), o ? get<int32_t>(o, "State", -1) : -1,
                         o ? get<int32_t>(o, "KeyState", -1) : -1, g_listener_set, g_listener_h, sp ? *sp : -1.0f, mg ? *mg : -1.0f,
                         g_saved, g_saved_speed, g_saved_margin, g_relocks, world::camera_yaw(), g_scan_dir);
    });
  }, "?h=cm ?stick=x,y|off the top-down mode: camera lock, listener override, the right stick's scan");
}
}  // namespace p3a::topdown

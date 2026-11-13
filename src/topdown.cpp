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
//  - UFldOperationPlayer (AFldOperator.OpPlayerComp): Direction (+0xC0, FMatrix) and CameraChanged (+0x120), see
//    hold_walk. Verified at Fountain Square's scene camera (yaw -50), 2026-11-12: each key walked the compass.
//  - MiniMapInfoDataAsset.Data (FMiniMapInfoTable: FieldID, DegreeAngle): each field's north, see map_turn.
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
#include <windows.h>
#include <cmath>
#include <format>
#include <vector>

namespace p3a::topdown {
namespace {
using namespace ue;
using world::FVector;

constexpr float kBaseNorthYaw = -90.0f;   // world -Y: the up of a map the game does not turn
constexpr int kStateFree = 1, kKeyEnable = 3;
constexpr int32_t kBehaviorFree = 1, kBehaviorMoveToDefault = 3;   // EFldCameraBehaviorFreeState
constexpr float kStopSpeed = 0.0f, kStopMargin = 180.0f;
constexpr float kScanDeadzone = 0.55f;   // of the stick's travel: a direction is chosen past it
// The stick snaps to the point nearest its direction; another takes over only once it is this many degrees nearer,
// so two points side by side do not flicker between them.
constexpr float kScanSwitchDeg = 4.0f;
constexpr ULONGLONG kScanRefreshMs = 250;   // how often the points are read again while the stick is held

bool g_active = false;
float g_north = kBaseNorthYaw;   // this field's north (map_turn)
WeakObj g_north_hero;            // the hero it was found for: a new hero is a new field
bool g_north_due = true;         // looked up again each frame until the field's ids are known
int g_field_major = 0, g_field_minor = 0;
float g_field_turn = 0;
float g_listener_h = 0.0f;   // cm above the hero's middle (dev: /topdown?h=)
bool g_listener_set = false;
// The camera behaviour's own values, kept while the mode holds them.
WeakObj g_behavior;
float g_saved_speed = 125.0f, g_saved_margin = 16.0f;
bool g_saved = false;
bool g_scan_held = false;            // the stick is past its deadzone
WeakObj g_scan_on;                   // the point it is on
std::string g_scan_name;             // dev: its name
std::vector<poi::Poi> g_scan_list;   // the points around, read every kScanRefreshMs while held
ULONGLONG g_scan_at = 0;
bool g_dev_stick = false;
float g_dev_x = 0, g_dev_y = 0;
uint64_t g_relocks = 0;
// A run of frames each needing the camera turned back (3553 in one early session, 2026-11-12, cause unknown): its
// start and end are logged with what the camera was doing, to find what fights the lock.
int g_relock_run = 0;
bool g_relocked_now = false;

// How far the game turns this field's minimap (MiniMapInfoDataAsset, rows "102_141_000": field, area, part; every
// part of an area is turned alike). Episode Aigis loads its own copy of the table: both agree on the fields they share.
float map_turn(int major, int minor) {
  static const int assets = watch::want("MiniMapInfoDataAsset", true);
  const std::string key = std::format("{:03}_{:03}_", major, minor);
  for (UObject* a : watch::all(assets)) {
    TableRows rows = table_rows(a);
    if (!rows) continue;
    const int32_t id_off = offset_of(rows.row, "FieldID"), angle_off = offset_of(rows.row, "DegreeAngle");
    if (id_off < 0 || angle_off < 0) continue;
    for (int32_t i = 0; i < rows.num; ++i) {
      uint8_t* r = rows.at(i);
      if (to_utf8(*(FString*)(r + id_off)).starts_with(key)) return *(float*)(r + angle_off);
    }
  }
  return 0.0f;
}

// North for the field the hero is in: found once per field (a field load makes a new hero), once its hit actors have
// streamed in and name it.
void north_tick() {
  UObject* hero = world::player();
  if (hero && !(WeakObj::of(hero) == g_north_hero)) { g_north_hero = WeakObj::of(hero); g_north_due = true; }
  if (!g_north_due) return;
  int major = 0, minor = 0;
  if (!poi::area_ids(major, minor)) return;
  g_north_due = false;
  g_field_major = major; g_field_minor = minor;
  g_field_turn = map_turn(major, minor);
  const float north = std::remainder(kBaseNorthYaw - g_field_turn, 360.0f);
  if (north == g_north) return;
  g_north = north;
  log::writef("topdown: field {} {}, map turned {:.0f}: north is yaw {:.0f}", major, minor, g_field_turn, g_north);
  if (g_active) world::set_north(true, g_north);
}

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
  // MoveToDefault (3) swings the camera back behind the hero over many frames (after a menu, a reset press): it fought
  // the lock for 2744 frames on 2026-11-12. Free (1) instead, and no reset pending.
  if (int32_t* st = field<int32_t>(b, "State"); st && *st == kBehaviorMoveToDefault) {
    *st = kBehaviorFree;
    if (int32_t* nx = field<int32_t>(b, "NextState")) *nx = kBehaviorFree;
  }
  if (uint8_t* once = field<uint8_t>(b, "bResetDirectionOnce")) *once = 0;
  UObject* cam = get<UObject*>(o, "FreeCamera");
  UObject* yaw_comp = cam ? get<UObject*>(cam, "YawSceneComp") : nullptr;
  float yaw = yaw_comp ? get<FRotator>(yaw_comp, "RelativeRotation").yaw : 0.0f;
  float diff = std::remainder(yaw - g_north, 360.0f);
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
    p.set<float>("Angle", g_north);
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
  p.set<FRotator>("Rotation", FRotator{0.0f, g_north, 0.0f});
  g_listener_set = call(pc, p);
}

void release_listener() {
  if (!g_listener_set) return;
  g_listener_set = false;
  if (UObject* pc = player_controller()) call(pc, "ClearAudioListenerOverride");
}

// ---- walking: up is north whatever camera shows the field ----
// The left stick walks along UFldOperationPlayer.Direction (an FMatrix: rows X, Y, Z, origin), which native 0x11d7db0
// takes from the camera showing the field (a hit camera's own when UFldOperationCamera.CurrentHitRef is set, else the
// free camera's) only while CameraChanged is false; while it is true the matrix kept is used (the game's own hold of
// the old direction across a camera cut). Held true with a north matrix, every camera walks the hero by the compass.
UObject* walker(UObject* o) { return o ? get<UObject*>(o, "OpPlayerComp") : nullptr; }

void hold_walk(UObject* o, float yaw) {
  UObject* w = walker(o);
  float* m = field<float>(w, "Direction");
  uint8_t* changed = field<uint8_t>(w, "CameraChanged");
  if (!m || !changed) return;
  const float rad = yaw * 3.14159265f / 180.0f, c = std::cos(rad), s = std::sin(rad);
  const float rows[3][4] = {{c, s, 0, 0}, {-s, c, 0, 0}, {0, 0, 1, 0}};   // FRotationMatrix(0, yaw, 0); the origin row kept
  for (int r = 0; r < 3; ++r)
    for (int k = 0; k < 4; ++k) m[r * 4 + k] = rows[r][k];
  *changed = 1;
}

// The game takes the camera's direction again: CameraChanged false makes 0x11d7db0 read it on its next frame.
void release_walk() {
  if (uint8_t* changed = field<uint8_t>(walker(op()), "CameraChanged")) *changed = 0;
}

// ---- the right stick's scan ----
// The stick points; it snaps to whichever point of interest is nearest that way, all round and at any distance in the
// range, and says what it is. Empty space and walls are never said: turning the stick on moves from one point to the
// next (the user, 2026-11-13: "never reading empty spaces"). The point alone is heard while it is held (sonar::stick).
void scan_release() {
  if (g_scan_held) sonar::stick_off();
  g_scan_held = false;
  g_scan_on = {};
  g_scan_name.clear();
  g_scan_list.clear();
}

void scan_tick() {
  float x = 0, y = 0;
  if (g_dev_stick) { x = g_dev_x; y = g_dev_y; }
  else input::right_stick(x, y);
  if (std::hypot(x, y) < kScanDeadzone) { scan_release(); return; }
  // Up is north, right is east: the bearing clockwise from north, as the points' bearings are while the mode holds.
  const float want = std::atan2(x, y) * 180.0f / 3.14159265f;
  const bool first = !g_scan_held;
  g_scan_held = true;
  const ULONGLONG now = GetTickCount64();
  if (first || now - g_scan_at >= kScanRefreshMs) { g_scan_at = now; g_scan_list = poi::scan(); }   // nearest first
  if (g_scan_list.empty()) {
    if (first) { speech::speak(strings::kScanNothing, true); sonar::stick(nullptr); }
    return;
  }
  const poi::Poi* best = nullptr;
  const poi::Poi* cur = nullptr;
  float best_off = 1e9f, cur_off = 1e9f;
  for (const poi::Poi& p : g_scan_list) {
    float off = std::fabs(std::remainder(p.bearing - want, 360.0f));
    if (off < best_off) { best = &p; best_off = off; }   // strictly less: of two the same way, the nearer (listed first)
    if (p.obj == g_scan_on) { cur = &p; cur_off = off; }
  }
  if (cur && cur_off <= best_off + kScanSwitchDeg) return;   // still on it
  g_scan_on = best->obj;
  g_scan_name = best->name;
  // What is there, not which way: the stick says that (the user, 2026-11-13).
  speech::speak(std::format("{}, {}", best->name, core::meters(best->dist / 100.0f)), true);
  sonar::stick(best);   // it alone is heard while the stick stays
}

void enter() {
  g_active = true;
  world::set_north(true, g_north);
  input::claim_right_stick(true, input::StickClaim::TopDown);
  log::write("topdown: on (camera north, listener on the hero)");
}

void leave() {
  g_active = false;
  world::set_north(false, g_north);
  input::claim_right_stick(false, input::StickClaim::TopDown);
  release_camera();
  release_listener();
  release_walk();
  scan_release();
  log::write("topdown: off (camera and listener back to the game)");
}
}  // namespace

bool active() { return g_active; }
float north_yaw() { return g_north; }

void tick() {
  UObject* o = op();
  bool want = settings::on(settings::TopDown) && world::exploring() && operator_free(o);
  if (want) north_tick();   // before entering: the mode starts facing this field's north
  if (want != g_active) want ? enter() : leave();
  if (!g_active) return;
  hold_camera(o);
  relock_run_end();
  hold_walk(o, g_north);
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
      UObject* w = walker(o);
      float* m = field<float>(w, "Direction");
      uint8_t* ch = field<uint8_t>(w, "CameraChanged");
      std::string walk = m ? std::format("walk X=({:.2f}, {:.2f}, {:.2f}) Y=({:.2f}, {:.2f}, {:.2f}) Z=({:.2f}, {:.2f}, {:.2f}) O=({:.0f}, {:.0f}, {:.0f}) held={}\n",
                                         m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10], m[12], m[13], m[14], ch ? *ch : -1)
                           : std::string("walk none\n");
      walk += std::format("field {} {} map turned {:.0f} north yaw {:.0f}{}\n", g_field_major, g_field_minor, g_field_turn, g_north,
                          g_north_due ? " (looking)" : "");
      return walk + std::format("topdown setting={} active={} exploring={} operator state={} keys={} listener_set={} h={:.0f}\n"
                         "camera speed={:.1f} margin={:.1f} saved={} ({:.1f}, {:.1f}) relocks={} camera_yaw={:.1f} scan held={} on={}\n",
                         settings::on(settings::TopDown), g_active, world::exploring(), o ? get<int32_t>(o, "State", -1) : -1,
                         o ? get<int32_t>(o, "KeyState", -1) : -1, g_listener_set, g_listener_h, sp ? *sp : -1.0f, mg ? *mg : -1.0f,
                         g_saved, g_saved_speed, g_saved_margin, g_relocks, world::camera_yaw(), g_scan_held, g_scan_name);
    });
  }, "?h=cm ?stick=x,y|off the top-down mode: camera lock, listener override, the right stick's scan");
}
}  // namespace p3a::topdown

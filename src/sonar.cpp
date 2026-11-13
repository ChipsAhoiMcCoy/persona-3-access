#include "sonar.h"
#include "audio.h"
#include "core/sonar_cues.h"
#include "core/sonar_field.h"
#include "core/strings.h"
#include "devserver.h"
#include "input.h"
#include "log.h"
#include "speech.h"
#include "world.h"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <memory>
#include <vector>

namespace p3a::sonar {
namespace {
using core::SonarCue;
using world::FVector;

constexpr int kKinds = (int)SonarCue::Count;
// Each kind of sound, in SonarCue's order: its name (the log, /sonar), the setting that switches it, and the points
// of interest it sounds for.
struct KindInfo { const char* name; settings::Id setting; poi::Kind poi; };
constexpr KindInfo kKind[kKinds] = {
  {"exit", settings::SonarExits, poi::Kind::Exit},       {"door", settings::SonarDoors, poi::Kind::Door},
  {"person", settings::SonarPeople, poi::Kind::Person},  {"shop", settings::SonarShops, poi::Kind::Shop},
  {"object", settings::SonarThings, poi::Kind::Thing},   {"event", settings::SonarEvents, poi::Kind::Event},
  {"fragment", settings::SonarFragments, poi::Kind::Fragment},
};
static_assert((int)SonarCue::Exit == 0 && (int)SonarCue::Door == 1 && (int)SonarCue::Person == 2 && (int)SonarCue::Shop == 3 &&
              (int)SonarCue::Thing == 4 && (int)SonarCue::Event == 5 && (int)SonarCue::Fragment == 6, "kKind is in SonarCue's order");
// The nearest things that sound at once, and of one kind. 6 and 3 at first: too much at once, and the thing beside
// the hero was lost in it (the user, 2026-11-13).
constexpr size_t kMaxSources = 4;
constexpr int kMaxPerKind = 2;         // a corridor of students must not drown the door beside you
// A thing this close takes the stage: everything else in the field drops kCloseDuckDb, so being beside it is heard
// (the user, 2026-11-13: beside a thing it was hard to tell among the others).
constexpr float kCloseM = 2.5f;
constexpr float kCloseDuckDb = 10.0f;
constexpr float kNearM = 1.5f;         // at and under this: full volume (the rhythm has its own scale: core/sonar_field)
// How much quieter a thing is at the range's edge (and beyond, when selected). 14 at first: with the far-off sound's
// duller treble on top, a thing was not noticed until the player was nearly on it (the user, 2026-11-10).
constexpr float kEdgeDb = 8.0f;
constexpr float kBehindRate = 0.84f;   // "Lower pitch behind you": three semitones down
constexpr ULONGLONG kRescanMs = 250;   // how often the list of things is read again (their sounds are placed per pulse)
constexpr float kLookAheadCm = 300.0f; // how far past the range that reading looks

// Per-kind level trims in dB (the cues are made at one RMS; a kind that still stands out is trimmed here; dev:
// /sonar?trim=kind,dB).
float g_trim_db[kKinds] = {0, 0, 0, 0, 0, 0, 0};

struct Source { ue::WeakObj obj; int kind; FVector loc; bool selected; };

// The focus: one thing heard alone, its sound repeating at its distance's rhythm while the rest of the mod's placed
// sounds (the field, the wall tones, a Shadow's growl) are silenced (audio::solo), so what it is and where can be
// heard (the user, 2026-11-13). Held while a point is chosen in the list (until the controller's layer closes, or
// kFocusMs after a key, or once the hero walks off) and while the right stick points somewhere.
struct Focus { ue::WeakObj obj; int kind = -1; bool shadow = false; FVector loc{}; bool has = false; };
Focus g_focus;
ULONGLONG g_focus_until = 0;     // the list's choice, by key: held this long
bool g_focus_layer = false;      // the list's choice with the controller's layer open: held until it closes
bool g_focus_stick = false;      // the right stick is on a direction
FVector g_focus_from{};          // where the hero was at the list's choice
double g_focus_next = 0;         // the focus's next sound
constexpr ULONGLONG kFocusMs = 3000;
constexpr float kFocusWalkM = 1.0f;          // the hero walked this far from the choice: the field comes back
constexpr double kFocusMinPeriod = 0.25;     // its rhythm, however near
std::vector<Source> g_sources;
core::SonarField g_field;
ULONGLONG g_scan_at = 0;
long long g_fired = 0;

int cue_of(poi::Kind k) {
  for (int i = 0; i < kKinds; ++i) if (kKind[i].poi == k) return i;
  return -1;   // Shadows: their own sounds
}
bool kind_on(int kind) { return kind >= 0 && kind < kKinds && settings::on(kKind[kind].setting); }

// The sounds, made once at the mixer's rate. A kind with a file in assets/audio/sonar (<its name>.wav) plays the
// file: Grim Dark's sonar sounds for their nearest kin here (the user, 2026-11-13) -- its dungeon entrance for the
// exits, its interactable (chests, levers, notes) for the objects, its ruined shrine (used, it starts a fight) for the
// events, its loot for the fragments. The rest (doors, people, shops) have no counterpart there and keep the sounds
// made in core/sonar_cues. Always the whole sound at its own pitch, as Grim Dark plays it: up close a sound longer
// than its rhythm starts again over its own tail, and the rhythm keeps following the distance. Tried and dropped
// (the user, 2026-11-13): the brief forms (cut to under 0.09 s for a thing too near for the whole one to fit its
// rhythm) cut a door's knock and the files short; holding the rhythm back for the whole sound kept the exit's 0.4 s
// sound from getting faster inside about 3.5 m; playing it faster to fit raised its pitch.
audio::Pcm cue(int kind) {
  static audio::Pcm made[kKinds];
  if (kind < 0 || kind >= kKinds) return nullptr;
  audio::Pcm& p = made[kind];
  if (p) return p;
  const int rate = audio::sample_rate();
  std::vector<float> file;
  const std::string path = audio::module_dir() + "assets\\audio\\sonar\\" + kKind[kind].name + ".wav";
  if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES && audio::decode_mono(path, file)) file = core::file_cue(std::move(file), rate);
  if (!file.empty()) {
    log::writef("sonar: {} plays {} ({:.0f} ms)", kKind[kind].name, path, 1000.0 * file.size() / rate);
    p = std::make_shared<const std::vector<float>>(std::move(file));
  } else {
    p = std::make_shared<const std::vector<float>>(core::sonar_cue((SonarCue)kind, rate));
  }
  return p;
}

// A thing's sound as it plays from `at`: lower, so longer, from behind.
struct Voice { audio::Pcm pcm; float rate = 1.0f; };
Voice voice(int kind, const audio::Spot& at) {
  Voice v;
  // Lower behind: in the top-down mode too (behind is the south there). The stereo model's darkening behind barely reaches
  // the exit's thump or the door's knocks, so without it they sounded the same north and south.
  v.rate = at.ahead < -0.2f && settings::on(settings::BehindPitch) ? kBehindRate : 1.0f;
  v.pcm = cue(kind);
  return v;
}

float range_m() { return (float)settings::get(settings::SonarRange); }
// Full volume up close, kEdgeDb down at the range's edge, even steps in dB per doubling of distance between.
float gain_at(float dist_m) {
  float far_m = std::max(range_m(), kNearM + 0.5f);
  float u = std::clamp(std::log(std::max(dist_m, kNearM) / kNearM) / std::log(far_m / kNearM), 0.0f, 1.0f);
  return std::pow(10.0f, -kEdgeDb * u / 20.0f);
}

// focus: it plays through the solo. db: added to its level.
void emit(int kind, const FVector& me, const FVector& at, bool focus = false, float db = 0.0f) {
  audio::Spot s = world::spot(me, at);
  Voice v = voice(kind, s);
  if (!v.pcm) return;
  float vol = gain_at(s.dist_m) * std::pow(10.0f, (g_trim_db[kind] + db) / 20.0f);
  audio::play(std::move(v.pcm), vol, s, audio::Bus::Sonar, v.rate, focus);
  ++g_fired;
}

// The field is heard while the player walks it: a hero, no message window, no menu, no battle, no story scene.
bool in_field() { return world::exploring(); }

// Which things sound: the nearest of the kinds switched on, a few of each, and the list's selected point whatever
// its kind. Their places are read here; direction and distance are worked out again at every pulse.
void rescan() {
  g_sources.clear();
  poi::Poi sel;
  bool has_sel = poi::selected(sel) && cue_of(sel.kind) >= 0;
  const int scope = settings::get(settings::SonarScope);
  if (scope == settings::ScopeSelected) {
    if (has_sel) g_sources.push_back({sel.obj, cue_of(sel.kind), sel.loc, false});
    return;
  }
  int per_kind[kKinds] = {};
  bool sel_listed = false;
  // Read a little past the range: the list is read four times a second, and at a run (7 m/s in Tartarus) a thing
  // found only at the next reading first sounded a metre or two inside the range. tick() holds it until it is in.
  for (const poi::Poi& p : poi::scan(range_m() * 100.0f + kLookAheadCm)) {   // nearest first
    int kind = cue_of(p.kind);
    if (kind < 0) continue;
    bool is_sel = has_sel && p.obj == sel.obj;
    if (!is_sel) {
      if (!kind_on(kind) || (scope == settings::ScopeCategory && !poi::in_selected_category(p))) continue;
      if (per_kind[kind] >= kMaxPerKind || g_sources.size() >= kMaxSources) continue;
    }
    ++per_kind[kind];
    sel_listed |= is_sel;
    g_sources.push_back({p.obj, kind, p.loc, is_sel});
  }
  // The selected point took a place past the limit: the farthest other one gives its place up.
  if (sel_listed && g_sources.size() > kMaxSources)
    for (size_t i = g_sources.size(); i-- > 0;)
      if (!g_sources[i].selected) { g_sources.erase(g_sources.begin() + (ptrdiff_t)i); break; }
}

double now_s() {
  using namespace std::chrono;
  return duration<double>(steady_clock::now().time_since_epoch()).count();
}
float dist_m(const FVector& a, const FVector& b) {
  return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z)) / 100.0f;
}
void silence() { g_field.reset(); g_sources.clear(); g_scan_at = 0; }

// The focused thing's sound once, from where it is now.
void sound_focus(const FVector& me) {
  if (!g_focus.has) return;
  if (g_focus.shadow) audio::signal((int)core::SignalCue::Shadow, 1.0f, world::spot(me, g_focus.loc), 1.0f, true);
  else emit(g_focus.kind, me, g_focus.loc, true);
  g_focus_next = now_s() + std::max(g_field.params().period_for(dist_m(me, g_focus.loc)), kFocusMinPeriod);
}

// `p` takes the stage (null: nothing does, but the stage is still cleared): its sound now, and the solo on.
void focus_on(const poi::Poi* p) {
  g_focus = {};
  if (p) {
    g_focus = {p->obj, cue_of(p->kind), p->kind == poi::Kind::Enemy, p->loc, false};
    g_focus.has = g_focus.kind >= 0 || g_focus.shadow;
  }
  audio::solo(true);
  silence();
  FVector me;
  if (world::player_location(me)) sound_focus(me);
}

bool focus_any() { return g_focus.has || g_focus_stick || g_focus_layer || g_focus_until; }

// Whether the focus holds this frame. The list's hold ends when the layer closes, its time is up, or the hero walks
// off; the stick's when it is let go.
bool focus_held() {
  if (g_focus_layer && !input::layer_open()) g_focus_layer = false;
  if (g_focus_layer || GetTickCount64() < g_focus_until) {
    FVector me;
    if (world::player_location(me) && dist_m(me, g_focus_from) > kFocusWalkM) { g_focus_layer = false; g_focus_until = 0; }
  }
  return g_focus_stick || g_focus_layer || GetTickCount64() < g_focus_until;
}

void focus_off() {
  if (focus_any()) silence();   // the field starts again, staggered
  g_focus = {};
  g_focus_stick = g_focus_layer = false;
  g_focus_until = 0;
  audio::solo(false);
}

// The focus's frame: true while it holds (the field is silent then).
bool focus_tick() {
  if (!in_field() || !focus_held()) { focus_off(); return false; }
  audio::solo(true);
  FVector me;
  if (!g_focus.has || !world::player_location(me)) return true;
  poi::Poi sel;   // the list's choice is followed where it goes (a person walking)
  if (poi::selected(sel) && sel.obj == g_focus.obj) g_focus.loc = sel.loc;
  if (now_s() >= g_focus_next) sound_focus(me);
  return true;
}
}  // namespace

void tick() {
  if (focus_tick()) return;
  if (!settings::on(settings::Sonar) || !in_field()) { silence(); return; }
  FVector me;
  if (!world::player_location(me)) { silence(); return; }
  ULONGLONG now = GetTickCount64();
  if (now - g_scan_at >= kRescanMs) { g_scan_at = now; rescan(); }
  std::vector<core::SonarField::Item> items;
  items.reserve(g_sources.size());
  const bool any_distance = settings::get(settings::SonarScope) == settings::ScopeSelected;
  float nearest = 1e9f;
  for (const Source& s : g_sources) {
    if (!any_distance && dist_m(me, s.loc) > range_m()) continue;   // read ahead of the range: not in it yet
    items.push_back({(unsigned)s.obj.index, dist_m(me, s.loc), s.kind});
    nearest = std::min(nearest, items.back().dist);
  }
  // What sounded last tick and no longer does is forgotten; a thing's first sound is logged with its distance (how
  // early the player could have heard it).
  static std::vector<unsigned> heard;
  std::erase_if(heard, [&](unsigned id) { return std::none_of(items.begin(), items.end(), [&](const auto& it) { return it.id == id; }); });
  for (const core::SonarField::Ping& p : g_field.update(items, now_s()))
    for (const Source& s : g_sources)
      if ((unsigned)s.obj.index == p.id) {
        // Something is beside the hero and this is not it (a metre's leeway: two things side by side both stay).
        bool upstaged = nearest <= kCloseM && dist_m(me, s.loc) > nearest + 1.0f;
        emit(s.kind, me, s.loc, false, upstaged ? -kCloseDuckDb : 0.0f);
        if (std::find(heard.begin(), heard.end(), p.id) == heard.end()) {
          heard.push_back(p.id);
          log::writef("sonar: {} #{} first sounds at {:.1f} m", kKind[s.kind].name, s.obj.index, dist_m(me, s.loc));
        }
        break;
      }
}

void toggle() {
  settings::set(settings::Sonar, !settings::on(settings::Sonar));
  speech::speak(settings::on(settings::Sonar) ? strings::kSonarOn : strings::kSonarOff, true);
}

void selected(const poi::Poi& p) {
  if (!settings::on(settings::Sonar) || !world::exploring()) return;
  FVector me;
  if (!world::player_location(me)) return;
  g_focus_from = me;
  g_focus_layer = input::layer_open();
  g_focus_until = GetTickCount64() + kFocusMs;
  focus_on(&p);
}

void stick(const poi::Poi* p) {
  if (!world::exploring()) return;
  g_focus_stick = true;
  focus_on(p);
}

void stick_off() {
  if (!g_focus_stick) return;
  g_focus_stick = false;
  if (!focus_held()) focus_off();
}

bool preview(settings::Id id) {
  int kind = id == settings::SonarVolume ? (int)SonarCue::Thing : -1;
  for (int k = 0; k < kKinds; ++k) if (kKind[k].setting == id) kind = k;
  if (kind < 0) return false;
  audio::play(cue(kind), 1.0f, audio::Spot{}, audio::Bus::Sonar);   // straight ahead, at arm's length
  return true;
}

void demo(int kind, bool behind) {
  audio::Spot s{};   // straight ahead
  s.dist_m = 2.0f;
  if (behind) s.ahead = -1;
  bool lower = behind && settings::on(settings::BehindPitch);
  audio::play(cue(kind), 1.0f, s, audio::Bus::Sonar, lower ? kBehindRate : 1.0f);
}

void install() {
  dev::route("/sonar", [](const dev::Request& r, int&) {
    // /sonar | ?play=person (its sound, ahead) | ?trim=person,-3 (a kind's level in dB)
    std::string play = dev::arg(r, "play"), trim = dev::arg(r, "trim");
    return dev::on_game([play, trim] {
      for (int k = 0; k < kKinds; ++k) {
        if (play == kKind[k].name) audio::play(cue(k), 1.0f, audio::Spot{}, audio::Bus::Sonar);
        if (size_t c = trim.find(','); c != std::string::npos && trim.substr(0, c) == kKind[k].name) g_trim_db[k] = (float)atof(trim.c_str() + c + 1);
      }
      std::string s = std::format("sonar on={} scope={} range={} m in_field={} sources={} tracked={} fired={}\ntrims dB:", settings::on(settings::Sonar),
                                  settings::get(settings::SonarScope), settings::get(settings::SonarRange), in_field(), g_sources.size(), g_field.tracked(), g_fired);
      for (int k = 0; k < kKinds; ++k) s += std::format(" {}={:+.1f}", kKind[k].name, g_trim_db[k]);
      s += "\n";
      FVector me;
      if (world::player_location(me))
        for (const Source& src : g_sources) {
          float d = dist_m(me, src.loc);
          audio::Spot at = world::spot(me, src.loc);
          s += std::format("  {:<7} {:5.1f} m  every {:.2f} s  gain {:.2f}  right {:+.2f} ahead {:+.2f} up {:+.2f}{}  #{}\n", kKind[src.kind].name, d,
                           g_field.params().period_for(d), gain_at(d), at.right, at.ahead, at.up, src.selected ? "  selected" : "", src.obj.index);
        }
      return s;
    });
  }, "?play=kind ?trim=kind,dB the sonar: what sounds now, each kind's sound and level");
}
}  // namespace p3a::sonar

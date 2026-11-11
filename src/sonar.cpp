#include "sonar.h"
#include "audio.h"
#include "core/sonar_cues.h"
#include "core/sonar_field.h"
#include "core/strings.h"
#include "devserver.h"
#include "log.h"
#include "msg.h"
#include "screens/screen.h"
#include "speech.h"
#include "watch.h"
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
constexpr const char* kKindName[kKinds] = {"exit", "door", "person", "shop", "object", "event"};
constexpr size_t kMaxSources = 6;      // the nearest things that sound at once
constexpr int kMaxPerKind = 3;         // of one kind: a corridor of students must not drown the door beside you
constexpr float kNearM = 1.5f;         // at and under this: full volume (the rhythm has its own scale: core/sonar_field)
// How much quieter a thing is at the range's edge (and beyond, when selected). 14 at first: with the far-off sound's
// duller treble on top, a thing was not noticed until the player was nearly on it (the user, 2026-11-10).
constexpr float kEdgeDb = 8.0f;
constexpr float kBehindRate = 0.84f;   // "Lower pitch behind you": three semitones down
constexpr ULONGLONG kRescanMs = 250;   // how often the list of things is read again (their sounds are placed per pulse)
constexpr float kLookAheadCm = 300.0f; // how far past the range that reading looks

// Per-kind level trims in dB (the cues are made at one RMS; a kind that still stands out is trimmed here; dev:
// /sonar?trim=kind,dB).
float g_trim_db[kKinds] = {0, 0, 0, 0, 0, 0};

struct Source { ue::WeakObj obj; int kind; FVector loc; bool selected; };
std::vector<Source> g_sources;
core::SonarField g_field;
ULONGLONG g_scan_at = 0;
long long g_fired = 0;

int cue_of(poi::Kind k) {
  switch (k) {
    case poi::Kind::Exit: return (int)SonarCue::Exit;
    case poi::Kind::Door: return (int)SonarCue::Door;
    case poi::Kind::Person: return (int)SonarCue::Person;
    case poi::Kind::Shop: return (int)SonarCue::Shop;
    case poi::Kind::Thing: return (int)SonarCue::Thing;
    case poi::Kind::Event: return (int)SonarCue::Event;
    default: return -1;   // Shadows: their own pulse
  }
}
bool kind_on(int kind) {
  static const settings::Id ids[kKinds] = {settings::SonarExits, settings::SonarDoors, settings::SonarPeople, settings::SonarShops, settings::SonarThings,
                                           settings::SonarEvents};
  return kind >= 0 && kind < kKinds && settings::on(ids[kind]);
}

// The sounds, made once at the mixer's rate.
audio::Pcm cue(int kind, bool brief = false) {
  static audio::Pcm made[kKinds][2];
  if (kind < 0 || kind >= kKinds) return nullptr;
  audio::Pcm& p = made[kind][brief ? 1 : 0];
  if (!p) p = std::make_shared<const std::vector<float>>(core::sonar_cue((SonarCue)kind, audio::sample_rate(), brief));
  return p;
}

// A thing's sound as it plays from `at`: lower, so longer, from behind; and its brief form once the whole sound,
// with the breath after it, no longer fits the rhythm of its distance (inside 2 to 3 m).
struct Voice { audio::Pcm pcm; float rate = 1.0f; float len_s = 0.2f; };
Voice voice(int kind, const audio::Spot& at, bool whole = false) {
  Voice v;
  v.rate = at.ahead < -0.2f && settings::on(settings::BehindPitch) ? kBehindRate : 1.0f;
  auto pick = [&](bool brief) {
    v.pcm = cue(kind, brief);
    v.len_s = v.pcm ? (float)v.pcm->size() / (float)audio::sample_rate() / v.rate : 0.2f;
  };
  pick(false);
  if (!whole && v.len_s + g_field.params().gap_s > g_field.params().period_for(at.dist_m)) pick(true);
  return v;
}

float range_m() { return (float)settings::get(settings::SonarRange); }
// Full volume up close, kEdgeDb down at the range's edge, even steps in dB per doubling of distance between.
float gain_at(float dist_m) {
  float far_m = std::max(range_m(), kNearM + 0.5f);
  float u = std::clamp(std::log(std::max(dist_m, kNearM) / kNearM) / std::log(far_m / kNearM), 0.0f, 1.0f);
  return std::pow(10.0f, -kEdgeDb * u / 20.0f);
}

// whole: the whole sound however near (a point landed on in the list).
void emit(int kind, const FVector& me, const FVector& at, bool whole = false) {
  audio::Spot s = world::spot(me, at);
  Voice v = voice(kind, s, whole);
  if (!v.pcm) return;
  float vol = gain_at(s.dist_m) * std::pow(10.0f, g_trim_db[kind] / 20.0f);
  audio::play(std::move(v.pcm), vol, s, audio::Bus::Sonar, v.rate);
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
}  // namespace

void tick() {
  if (!settings::on(settings::Sonar) || !in_field()) { silence(); return; }
  FVector me;
  if (!world::player_location(me)) { silence(); return; }
  ULONGLONG now = GetTickCount64();
  if (now - g_scan_at >= kRescanMs) { g_scan_at = now; rescan(); }
  std::vector<core::SonarField::Item> items;
  items.reserve(g_sources.size());
  const bool any_distance = settings::get(settings::SonarScope) == settings::ScopeSelected;
  for (const Source& s : g_sources) {
    if (!any_distance && dist_m(me, s.loc) > range_m()) continue;   // read ahead of the range: not in it yet
    // How long its sound will last as played now: the line is held that long.
    items.push_back({(unsigned)s.obj.index, dist_m(me, s.loc), s.kind, voice(s.kind, world::spot(me, s.loc)).len_s, s.selected});
  }
  // What sounded last tick and no longer does is forgotten; a thing's first sound is logged with its distance (how
  // early the player could have heard it).
  static std::vector<unsigned> heard;
  std::erase_if(heard, [&](unsigned id) { return std::none_of(items.begin(), items.end(), [&](const auto& it) { return it.id == id; }); });
  for (const core::SonarField::Ping& p : g_field.update(items, now_s()))
    for (const Source& s : g_sources)
      if ((unsigned)s.obj.index == p.id) {
        emit(s.kind, me, s.loc);
        if (std::find(heard.begin(), heard.end(), p.id) == heard.end()) {
          heard.push_back(p.id);
          log::writef("sonar: {} #{} first sounds at {:.1f} m", kKindName[s.kind], s.obj.index, dist_m(me, s.loc));
        }
        break;
      }
}

void toggle() {
  settings::set(settings::Sonar, !settings::on(settings::Sonar));
  speech::speak(settings::on(settings::Sonar) ? strings::kSonarOn : strings::kSonarOff, true);
}

void selected(const poi::Poi& p) {
  if (!settings::on(settings::Sonar)) return;
  FVector me;
  int kind = cue_of(p.kind);
  if (kind < 0 || !world::player_location(me)) return;
  emit(kind, me, p.loc, true);
}

bool preview(settings::Id id) {
  int kind = id == settings::SonarExits ? 0 : id == settings::SonarDoors ? 1 : id == settings::SonarPeople ? 2 :
             id == settings::SonarShops ? 3 : id == settings::SonarThings ? 4 : id == settings::SonarEvents ? 5 : id == settings::SonarVolume ? (int)SonarCue::Thing : -1;
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
        if (play == kKindName[k]) audio::play(cue(k), 1.0f, audio::Spot{}, audio::Bus::Sonar);
        if (size_t c = trim.find(','); c != std::string::npos && trim.substr(0, c) == kKindName[k]) g_trim_db[k] = (float)atof(trim.c_str() + c + 1);
      }
      std::string s = std::format("sonar on={} scope={} range={} m in_field={} sources={} tracked={} fired={}\ntrims dB:", settings::on(settings::Sonar),
                                  settings::get(settings::SonarScope), settings::get(settings::SonarRange), in_field(), g_sources.size(), g_field.tracked(), g_fired);
      for (int k = 0; k < kKinds; ++k) s += std::format(" {}={:+.1f}", kKindName[k], g_trim_db[k]);
      s += "\n";
      FVector me;
      if (world::player_location(me))
        for (const Source& src : g_sources) {
          float d = dist_m(me, src.loc);
          audio::Spot at = world::spot(me, src.loc);
          s += std::format("  {:<7} {:5.1f} m  every {:.2f} s  gain {:.2f}  right {:+.2f} ahead {:+.2f} up {:+.2f}{}  #{}\n", kKindName[src.kind], d,
                           g_field.params().period_for(d), gain_at(d), at.right, at.ahead, at.up, src.selected ? "  selected" : "", src.obj.index);
        }
      return s;
    });
  }, "?play=kind ?trim=kind,dB the sonar: what sounds now, each kind's sound and level");
}
}  // namespace p3a::sonar

#include "settings.h"
#include "audio.h"
#include "core/graph_builder.h"
#include "core/navigator.h"
#include "core/settings_text.h"
#include "core/sonar_cues.h"
#include "core/strings.h"
#include "devserver.h"
#include "input.h"
#include "keys.h"
#include "log.h"
#include "mod.h"
#include "sonar.h"
#include "speech.h"
#include "voice.h"
#include "walltones.h"
#include <windows.h>
#include <xinput.h>
#include <algorithm>
#include <cstdio>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace p3a::settings {
enum class Unit { Toggle, Meters, Percent, Ms, Choice };
// The menu's categories (the user's call, 2026-11-11: one list of 27 rows had grown too long). Each is a stop of its
// own: a short list with its own positions, its name said on landing; Tab / Shift+Tab (Page Down / Page Up, RB / LB)
// go to the next / previous one, wrapping. A row names its category, so the table's order stays the file's.
enum Category { CatExploring, CatSonar, CatSound, CatDialogue, CatControls, CatCount };
static const char* const kCategoryNames[CatCount] = {strings::kCatExploring, strings::kCatSonar, strings::kCatSound,
                                                     strings::kCatDialogue, strings::kCatControls};
struct Def {
  const char* key; const char* name;
  Category cat;        // where the menu lists it
  const char* help;    // what it does: read with its row, after the position
  Unit unit; std::vector<int> values; int def; std::vector<const char*> labels = {};
};
static const std::vector<int> kPercents = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
// One row per Id, in its order: what the file calls it, what the menu says and where, the values it cycles through.
static const Def kDefs[Count] = {
  {"wall_tones", strings::kSetWallTones, CatExploring, strings::kHelpWallTones, Unit::Toggle, {0, 1}, 1},
  // every nearby point of interest sounds (sonar.cpp)
  {"sonar", strings::kSetSonar, CatSonar, strings::kHelpSonar, Unit::Toggle, {0, 1}, 1},
  {"sonar_scope", strings::kSetSonarScope, CatSonar, strings::kHelpSonarScope, Unit::Choice, {ScopeAll, ScopeCategory, ScopeSelected}, ScopeAll,
   {strings::kScopeAll, strings::kScopeCategory, strings::kScopeSelected}},
  // 10 m (20 was tried for a day under the key sonar_distance: what made things seem late was the wait before a
  // thing's first sound, not the range; the user's call, 2026-11-10).
  {"sonar_range", strings::kSetSonarRange, CatSonar, strings::kHelpSonarRange, Unit::Meters, {5, 10, 15, 20, 30, 40}, 10},
  {"sonar_exits", strings::kSetSonarExits, CatSonar, strings::kHelpSonarKind, Unit::Toggle, {0, 1}, 1},
  {"sonar_doors", strings::kSetSonarDoors, CatSonar, strings::kHelpSonarKind, Unit::Toggle, {0, 1}, 1},
  {"sonar_people", strings::kSetSonarPeople, CatSonar, strings::kHelpSonarKind, Unit::Toggle, {0, 1}, 1},
  {"sonar_shops", strings::kSetSonarShops, CatSonar, strings::kHelpSonarKind, Unit::Toggle, {0, 1}, 1},
  {"sonar_objects", strings::kSetSonarThings, CatSonar, strings::kHelpSonarKind, Unit::Toggle, {0, 1}, 1},
  {"sonar_events", strings::kSetSonarEvents, CatSonar, strings::kHelpSonarKind, Unit::Toggle, {0, 1}, 1},
  {"sonar_fragments", strings::kSetSonarFragments, CatSonar, strings::kHelpSonarKind, Unit::Toggle, {0, 1}, 1},
  {"shadow_alerts", strings::kSetShadowAlerts, CatExploring, strings::kHelpShadowAlerts, Unit::Toggle, {0, 1}, 1},
  // how positional cues are placed: Dark Vision's stereo model, Steam Audio's HRTF, or a plain pan (a new key: the
  // old "hrtf" toggle's saved 1 would have kept the HRTF)
  {"spatial", strings::kSetSpatial, CatSound, strings::kHelpSpatial, Unit::Choice, {SpatialStereo, SpatialHrtf, SpatialPan}, SpatialStereo,
   {strings::kSpatialStereo, strings::kSpatialHrtf, strings::kSpatialPan}},
  {"behind_pitch", strings::kSetBehindPitch, CatSound, strings::kHelpBehindPitch, Unit::Toggle, {0, 1}, 1},
  {"poi_range", strings::kSetPoiRange, CatExploring, strings::kHelpPoiRange, Unit::Meters, {20, 40, 60, 100, 150}, 60},
  {"master_volume", strings::kSetMasterVolume, CatSound, strings::kHelpMasterVolume, Unit::Percent, kPercents, 100},
  {"sonar_volume", strings::kSetSonarVolume, CatSonar, strings::kHelpSonarVolume, Unit::Percent, kPercents, 80},     // the Sonar bus
  {"sound_volume", strings::kSetSoundVolume, CatSound, strings::kHelpSoundVolume, Unit::Percent, kPercents, 60},     // the Cues bus
  {"wall_volume", strings::kSetWallVolume, CatExploring, strings::kHelpWallVolume, Unit::Percent, kPercents, 100},
  {"trigger_delay", strings::kSetTriggerDelay, CatControls, strings::kHelpTriggerDelay, Unit::Ms, {0, 100, 150, 250}, 150},
  {"entry_hints", strings::kSetEntryHints, CatControls, strings::kHelpEntryHints, Unit::Toggle, {0, 1}, 0},
  {"voiced_lines", strings::kSetVoicedLines, CatDialogue, strings::kHelpVoicedLines, Unit::Choice, {VoicedSilent, VoicedName, VoicedRead}, VoicedSilent,
   {strings::kVoicedRead, strings::kVoicedName, strings::kVoicedSilent}},
  {"voice_end_sound", strings::kSetVoiceEndSound, CatDialogue, strings::kHelpVoiceEndSound, Unit::Choice, {EndNext, EndPage, EndMessage, EndCursor, EndOff}, EndNext,
   {strings::kEndNext, strings::kEndPage, strings::kEndMessage, strings::kEndCursor, strings::kOff}},
  // the screen reader reads a cutscene's subtitles (msg.cpp)
  {"subtitles", strings::kSetSubtitles, CatDialogue, strings::kHelpSubtitles, Unit::Toggle, {0, 1}, 0},
  // the narrator in story movies (ad.cpp)
  {"audio_description", strings::kSetAudioDescription, CatDialogue, strings::kHelpAudioDescription, Unit::Toggle, {0, 1}, 1},
  {"description_volume", strings::kSetDescriptionVolume, CatDialogue, strings::kHelpDescriptionVolume, Unit::Percent, kPercents, 100},
  // the camera fixed north and every sound heard from above while walking freely (topdown.cpp)
  {"top_down", strings::kSetTopDown, CatExploring, strings::kHelpTopDown, Unit::Toggle, {0, 1}, 1},
  {"sonar_mode", strings::kSetSonarMode, CatSonar, strings::kHelpSonarMode, Unit::Choice, {SonarSweep, SonarPulse}, SonarSweep,
   {strings::kSonarSweep, strings::kSonarPulse}},
};
static int g_value[Count] = {};   // load() fills it from kDefs, then the file

int get(Id id) { return id >= 0 && id < Count ? g_value[id] : 0; }

static std::wstring file() {
  std::wstring p = log::path();
  p = p.substr(0, p.find_last_of(L'\\') + 1);
  // Dev launches (unfocused) keep their own file: tests never change the player's settings.
  return p + (env_flag(L"P3A_NOFOCUS") ? L"settings-dev.ini" : L"settings.ini");
}

static void save() {
  FILE* f = _wfopen(file().c_str(), L"w");
  if (!f) { log::write("settings: cannot write the file"); return; }
  for (int i = 0; i < Count; ++i) fprintf(f, "%s=%d\n", kDefs[i].key, g_value[i]);
  fclose(f);
}

static void apply(Id id) {
  if (id == WallTones && !env_flag(L"P3A_NOWALLTONES")) walltones::set_enabled(g_value[id] != 0);
  if (id == MasterVolume) audio::set_master_volume(g_value[id] / 100.0f);
  if (id == SoundVolume) audio::set_bus_volume(audio::Bus::Cues, g_value[id] / 100.0f);
  if (id == SonarVolume) audio::set_bus_volume(audio::Bus::Sonar, g_value[id] / 100.0f);
  if (id == WallVolume) audio::set_bus_volume(audio::Bus::Walls, g_value[id] / 100.0f);
  if (id == DescriptionVolume) audio::set_bus_volume(audio::Bus::Narration, g_value[id] / 100.0f);
  if (id == Spatial) audio::set_spatial(g_value[id]);
}

void set(Id id, int value) {
  if (id < 0 || id >= Count) return;
  g_value[id] = value;
  apply(id);
  save();
}

static void load() {
  for (int i = 0; i < Count; ++i) g_value[i] = kDefs[i].def;
  if (FILE* f = _wfopen(file().c_str(), L"r")) {
    char line[256];
    while (fgets(line, sizeof line, f)) {
      std::string key; int v;
      if (!core::parse_setting(line, key, v)) continue;
      for (int i = 0; i < Count; ++i)   // a value the option does not offer (a hand edit) keeps the default
        if (key == kDefs[i].key && std::ranges::find(kDefs[i].values, v) != kDefs[i].values.end()) g_value[i] = v;
    }
    fclose(f);
  }
  for (int i = 0; i < Count; ++i) apply((Id)i);
}

static std::string label(Id id) {
  int v = g_value[id];
  switch (kDefs[id].unit) {
    case Unit::Toggle: return v ? strings::kOn : strings::kOff;
    case Unit::Meters: return std::format("{} {}", v, strings::kMetersUnit);
    case Unit::Percent: return std::format("{} {}", v, strings::kPercent);
    case Unit::Choice: return v >= 0 && v < (int)kDefs[id].labels.size() ? kDefs[id].labels[v] : std::to_string(v);
    default: return v ? std::format("{} {}", v, strings::kMilliseconds) : std::string(strings::kOff);
  }
}

// ---- the menu, on core/navigator like every bespoke menu: one item per option in its category, "Mod keys" (reads
// the mod's keys) last. The navigator owns focus, positions, edges and type-ahead (letters jump to an option by name); Left/Right
// adjust the focused option (on_adjust), Space/Enter steps it on. It plays the game's own menu sounds (the camp menu's
// open, close, cursor and confirm cues; Config's option cue on a change). Keys reach it through keys::set_modal. ----
static bool g_open = false;

static void change(Id id, int delta) {
  const auto& vals = kDefs[id].values;
  int at = core::value_index(vals, g_value[id]);
  // Space (0) steps forward and wraps; left and right stop at the ends.
  int next = core::step_index(at, (int)vals.size(), delta);
  if (next == at) return;   // state_text says the value again
  set(id, vals[next]);
  if (id == VoiceEndSound && g_value[id] != EndNext) voice::play_end_sound();   // a preview, in place of the menu's sound
  else if (sonar::preview(id)) {}   // a kind's own sound, or one at the new volume
  else voice::play_system_sound(voice::CueOption);   // the game's Config plays it on every value change (FUN_141391c40)
}

// The sound test (the last category): row i of strings::kSoundTests plays its sound, from the menu, wherever the
// player is and whatever is switched off. In the order of that table.
static void play_test(int i) {
  using core::SignalCue;
  const audio::Spot ahead{};
  switch (i) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: sonar::demo(i); break;
    case 7: sonar::demo(2, true); break;
    case 8: audio::signal((int)SignalCue::Shadow, 1.4f, ahead); break;
    case 9: audio::signal((int)SignalCue::Shadow, 1.7f, ahead, 1.3f); break;
    case 10: audio::signal((int)SignalCue::Spotted, 1.4f, ahead); break;
    case 11: audio::signal((int)SignalCue::Lost, 1.1f, ahead); break;
    case 12: audio::signal_here((int)SignalCue::Reach, 1.2f); break;
    case 13: audio::signal_here((int)SignalCue::Ambush, 1.2f); break;
    case 14: case 15: case 16: case 17: walltones::demo(i - 14); break;
    case 18: audio::signal((int)SignalCue::Layer, 0.9f, ahead); break;
  }
}

struct Menu : ui::Screen {
  std::string_view key() const override { return "mod_settings"; }
  std::string screen_name() const override { return strings::kModSettings; }
  bool wrap() const override { return true; }
  void build(ui::GraphBuilder& b) override {
    namespace k = ui::announcement_kinds;
    for (int c = 0; c < CatCount; ++c) {
      b.begin_stop(std::format("cat{}", c));
      b.push_context(kCategoryNames[c]);
      for (int i = 0; i < Count; ++i) {
        Id id = (Id)i;
        if (kDefs[i].cat != c) continue;
        auto v = std::make_shared<ui::NodeVtable>();
        // What the setting does is read with the row, after its position (the user's call, 2026-11-11: no details
        // keys in this menu).
        v->announcements = {ui::NodeAnnouncement([id] { return std::string(kDefs[id].name); }, false, k::kLabel),
                            ui::NodeAnnouncement([id] { return label(id); }, true, k::kValue),
                            ui::NodeAnnouncement([id] { return std::string(kDefs[id].help); }, false, k::kTooltip)};
        v->on_adjust = [id](int sign, bool) { change(id, sign); };
        v->on_activate = [id] { change(id, 0); };
        v->state_text = [id] { return label(id); };
        b.add_item(ui::ControlId::structural(kDefs[i].key), v);
      }
      if (c == CatControls) {
        auto v = std::make_shared<ui::NodeVtable>();
        v->announcements = {ui::NodeAnnouncement([] { return std::string(strings::kModKeys); }, false, k::kLabel),
                            ui::NodeAnnouncement([] { return std::string(strings::kHelpModKeys); }, false, k::kTooltip)};
        v->on_activate = [] { voice::play_system_sound(voice::CueDecide); speech::speak(keys::help(), true); };
        b.add_item(ui::ControlId::structural("mod_keys"), v);
      }
      b.pop_context();
    }
    // The sound test: Space or A plays the row's sound; the row says what the sound means. No cursor sound on
    // these rows' activation: only the sound asked for is heard.
    b.begin_stop("sound_test");
    b.push_context(strings::kCatSoundTest);
    for (int i = 0; i < (int)std::size(strings::kSoundTests); ++i) {
      const strings::SoundTest& t = strings::kSoundTests[i];
      auto v = std::make_shared<ui::NodeVtable>();
      v->announcements = {ui::NodeAnnouncement([&t] { return std::string(t.name); }, false, k::kLabel),
                          ui::NodeAnnouncement([&t] { return std::string(t.help); }, false, k::kTooltip)};
      v->on_activate = [i] { play_test(i); };
      b.add_item(ui::ControlId::structural(std::format("test{}", i)), v);
    }
    b.pop_context();
  }
};
static Menu g_menu;
static int g_frame = 0;
static std::optional<ui::ControlId> g_move_from;   // the focus before the key being handled
static ui::GraphNavigator& nav();
// The cursor sound, unless the press moved nothing: at a list's first or last row the row is said again without it,
// as in the game's menus (the user's call, 2026-11-11).
static void move_sound() {
  auto at = nav().focused_id();
  if (g_move_from && at && *g_move_from == *at) return;
  voice::play_system_sound(voice::CueCursor);
}
static ui::GraphNavigator& nav() {
  static ui::GraphNavigator n(ui::NavigatorHost{[](std::string_view t, bool interrupt) { speech::speak(t, interrupt); },
                                                [] { move_sound(); },
                                                [] { return g_open; }, [] { return g_frame; }, {}});
  return n;
}

// One key through the navigator, then its pull (the navigator rebuilds and announces in ensure_focus).
static void action(std::string_view a) {
  g_frame += 100;
  g_move_from = nav().focused_id();
  nav().on_action(a);
  nav().ensure_focus();
  g_move_from.reset();
}
// Up / Down while a type-ahead search is active walk its results (the navigator reads them as held keys, once per
// change of direction): one press, then the release. With no search they do nothing and the action moves.
static void arrow(std::string_view a, bool up) {
  ui::TypeaheadInput in;
  in.up_held = up; in.down_held = !up;
  nav().tick_typeahead(in);
  nav().tick_typeahead(ui::TypeaheadInput{});
  action(a);
}
static void typed(char16_t c) {
  char16_t buf[1] = {c};
  ui::TypeaheadInput in;
  in.typed = std::u16string_view(buf, 1);
  g_frame += 100;
  nav().tick_typeahead(in);
  nav().ensure_focus();
}

static void on_key(uint32_t vk, bool repeat) {
  using namespace ui::ui_actions;
  switch (vk) {
    case VK_UP: arrow(Up, true); break;
    case VK_DOWN: arrow(Down, false); break;
    case VK_LEFT: action(Left); break;
    case VK_RIGHT: action(Right); break;
    case VK_HOME: action(Home); break;
    case VK_END: action(End); break;
    case VK_TAB: action(GetKeyState(VK_SHIFT) & 0x8000 ? Prev : Next); break;
    case VK_NEXT: action(Next); break;
    case VK_PRIOR: action(Prev); break;
    case VK_SPACE: case VK_RETURN: if (!repeat) action(Activate); break;
    case VK_ESCAPE: case VK_BACK: case VK_F2: if (!repeat) close(); break;
    case VK_F1: if (!repeat) speech::speak(strings::kModSettingsKeys(), true); break;
    default:
      if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) typed((char16_t)(vk >= 'A' && vk <= 'Z' ? vk - 'A' + 'a' : vk));
      break;
  }
}

static void on_pad(uint16_t button, bool repeat) {
  using namespace ui::ui_actions;
  switch (button) {
    case XINPUT_GAMEPAD_DPAD_UP: arrow(Up, true); break;
    case XINPUT_GAMEPAD_DPAD_DOWN: arrow(Down, false); break;
    case XINPUT_GAMEPAD_DPAD_LEFT: action(Left); break;
    case XINPUT_GAMEPAD_DPAD_RIGHT: action(Right); break;
    case XINPUT_GAMEPAD_RIGHT_SHOULDER: action(Next); break;
    case XINPUT_GAMEPAD_LEFT_SHOULDER: action(Prev); break;
    case XINPUT_GAMEPAD_A: if (!repeat) action(Activate); break;
    case XINPUT_GAMEPAD_B: case XINPUT_GAMEPAD_BACK: if (!repeat) close(); break;
    case XINPUT_GAMEPAD_START: if (!repeat) speech::speak(strings::kModSettingsKeys(), true); break;
    default: break;
  }
}

void open() {
  if (g_open) return;
  g_open = true;
  keys::set_modal(on_key, on_pad);
  input::set_modal(true);
  voice::play_system_sound(voice::CueMenuOpen);
  // Every opening starts on the first option: a fresh state, the title, then the focused item (queued after it).
  nav().screen_closed(&g_menu);
  // The type-ahead starts fresh: a search left from the last opening is dropped, and the navigator's first look at a
  // screen (which drops a search and takes no letter) is had here, not on the player's first letter.
  nav().attach(nullptr);
  nav().tick_typeahead(ui::TypeaheadInput{});
  nav().attach(&g_menu);
  g_frame += 100;
  nav().settle_silently();   // the focus is read after the title
  nav().tick_typeahead(ui::TypeaheadInput{});
  speech::speak(strings::kModSettings, true);
  nav().announce_current();
}

void close() {
  if (!g_open) return;
  g_open = false;
  keys::set_modal(nullptr, nullptr);
  input::set_modal(false);
  voice::play_system_sound(voice::CueMenuClose);
  speech::speak(strings::kModSettingsClosed, true);
}

static std::string focused_key() {
  auto f = nav().focused_id();
  return f && f->structural_key().is_string() ? f->structural_key().text() : std::string();
}

void install() {
  load();
  std::wstring f = file();
  log::writef("settings: loaded {}", log::utf8(std::u16string(f.begin(), f.end())));
  dev::route("/settings", [](const dev::Request& r, int&) {
    // /settings?set=poi_range&v=40 | ?open=1 | ?close=1
    std::string key = dev::arg(r, "set");
    int v = (int)dev::arg_int(r, "v", 0);
    bool op = dev::has(r, "open"), cl = dev::has(r, "close");
    return dev::on_game([=] {
      for (int i = 0; i < Count; ++i) if (!key.empty() && key == kDefs[i].key) set((Id)i, v);
      if (op) open();
      if (cl) close();
      std::string s = std::format("open={} focus={}\n", g_open, focused_key());
      for (int i = 0; i < Count; ++i) s += std::format("{}={} ({})\n", kDefs[i].key, g_value[i], label((Id)i));
      return s;
    });
  }, "?set=key&v=N ?open=1 ?close=1 the mod's settings");
}
}  // namespace p3a::settings

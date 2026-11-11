// The player's keys (README.md lists them). Chosen from keys P3R's keyboard layout leaves free:
// punctuation on the right of the keyboard and F1.
#include "commands.h"
#include "area.h"
#include "audio.h"
#include "core/sonar_cues.h"
#include "core/strings.h"
#include "keys.h"
#include "msg.h"
#include "nav.h"
#include "party.h"
#include "settings.h"
#include "poi.h"
#include "calendar.h"
#include "screens/screen.h"
#include "screens/battle.h"
#include "speech.h"
#include "stats.h"
#include "social.h"
#include "sonar.h"
#include "world.h"
#include <windows.h>
#include <xinput.h>
#include <format>

namespace p3a::commands {
static void where_am_i() {
  if (!world::player()) { speech::speak(strings::kNotInField, true); return; }
  std::string s = poi::area_name();
  std::string date = calendar::describe(calendar::now());
  if (!date.empty()) s += ". " + date;
  if (int m = stats::money(); m >= 0) s += std::format(". {} {}", m, strings::kYen);
  // The story objective with the line stacked under it (the field's top-right panel, the last one shown).
  std::string goal = poi::objectives();
  s += std::string(". ") + (goal.empty() ? strings::kNoObjective : std::string(strings::kObjective) + ": " + goal);
  speech::speak(s, true);
}

static void review() {
  // A screen open over a conversation (the rank card and its poem, the log) is what is in front: it answers first.
  auto* s = screens::current();
  bool over = s && s->over_messages();
  if (msg::current().open && !over) { msg::repeat(); return; }
  if (s) { std::string d = s->describe(); if (!d.empty()) { speech::speak(d, true); return; } }
  if (msg::current().open) { msg::repeat(); return; }
  speech::speak(strings::kNothingToReview, true);
}

// F1: the keys of what is in front of the player (a message window, the current screen, or the field). Screens no
// longer say their keys on entry (the player's choice, 2026-10-28).
static void screen_keys() {
  msg::Current m = msg::current();
  std::string k;
  // A screen opened over a message window (the log over a conversation) has the keys.
  bool over = screens::current() && screens::current()->over_messages();
  if (m.blocking && !over) k = m.kind == "Tutorial" ? strings::kTutorialKeys() : !m.choices.empty() ? strings::kChooseKeys()
                    : m.kind == "System" ? strings::kSpaceContinues() : strings::kDialogueKeys();
  else if (screens::current()) {
    k = screens::keys();
    // Wherever the detail buffer has something to read (a battle's units, a row with a description kept off it).
    if (screens::battle_details::in_focus()) k += (k.empty() ? "" : " ") + std::string(strings::kDetailKeys());
  }
  else if (world::player())
    k = std::string(strings::kFieldKeys()) + " " + (poi::tartarus_floor() > 0 ? std::string(strings::kTartarusKeys()) + " " : std::string()) + strings::kModKeysHint();
  if (k.empty()) k = std::string(strings::kNoKeys) + " " + strings::kModKeysHint();
  speech::speak(k, true);
}

// The controller's mod layer opened (both triggers): a tick, and in the field the points of interest at once.
static void pad_layer() {
  audio::signal((int)core::SignalCue::Layer, 0.9f, audio::Spot{});
  if (!screens::current() && !msg::current().blocking && world::player()) poi::announce_list();
}

static void walk_selected() {
  if (nav::walking()) { nav::stop(true); return; }
  poi::Poi p;
  if (!poi::selected(p)) { speech::speak(strings::kNothingSelected, true); return; }
  nav::walk_to(p.obj, p.name);
}

void install() {
  using namespace keys;
  bind(VK_F1, None, "help", strings::kKeyHelpScreenKeys, screen_keys);
  bind(VK_F1, Shift, "help.mod", strings::kKeyHelpModKeys, [] { speech::speak(keys::help(), true); });
  bind(VK_F2, None, "settings", strings::kKeyHelpSettings, settings::open);
  bind(VK_F3, None, "sonar", strings::kKeyHelpSonar, sonar::toggle);
  bind(VK_OEM_2, None, "where", strings::kKeyHelpWhere, where_am_i);                                             // /
  bind(VK_OEM_2, Shift, "stats", strings::kKeyHelpStats, [] {                                                    // Shift+/
    std::string s = stats::describe();
    speech::speak(s.empty() ? strings::kNotInField : s, true);
  });
  bind(VK_OEM_5, Shift, "social", strings::kKeyHelpSocial, [] { speech::speak(social::summary(), true); });      // Shift+backslash
  bind(VK_OEM_5, None, "review", strings::kKeyHelpReview, review);                                               // backslash
  bind(VK_OEM_6, None, "poi.next", strings::kKeyHelpPoiNext, [] { poi::next(1); }, true);                        // ]
  bind(VK_OEM_4, None, "poi.prev", strings::kKeyHelpPoiPrev, [] { poi::next(-1); }, true);                       // [
  bind(VK_OEM_6, Shift, "poi.cat.next", strings::kKeyHelpPoiCatNext, [] { poi::next_category(1); });             // Shift+]
  bind(VK_OEM_4, Shift, "poi.cat.prev", strings::kKeyHelpPoiCatPrev, [] { poi::next_category(-1); });            // Shift+[
  bind(VK_OEM_7, Shift, "party", strings::kKeyHelpParty, [] { speech::speak(party::summary(), true); });         // Shift+'
  bind(VK_OEM_7, None, "nav.walk", strings::kKeyHelpWalk, walk_selected);                                        // '
  bind(VK_OEM_1, None, "area", strings::kKeyHelpArea, area::describe);                                           // ;
  // The same keys on a controller: hold both triggers, then (input.cpp keeps these buttons from the game meanwhile).
  struct PadKey { const char* name; uint16_t button; };
  const PadKey pad[] = {
    {"poi.next", XINPUT_GAMEPAD_DPAD_DOWN}, {"poi.prev", XINPUT_GAMEPAD_DPAD_UP},
    {"poi.cat.next", XINPUT_GAMEPAD_DPAD_RIGHT}, {"poi.cat.prev", XINPUT_GAMEPAD_DPAD_LEFT},
    {"nav.walk", XINPUT_GAMEPAD_A}, {"area", XINPUT_GAMEPAD_B}, {"review", XINPUT_GAMEPAD_X}, {"where", XINPUT_GAMEPAD_Y},
    {"stats", XINPUT_GAMEPAD_LEFT_SHOULDER}, {"social", XINPUT_GAMEPAD_RIGHT_SHOULDER}, {"party", XINPUT_GAMEPAD_LEFT_THUMB},
    {"sonar", XINPUT_GAMEPAD_RIGHT_THUMB}, {"help", XINPUT_GAMEPAD_START}, {"settings", XINPUT_GAMEPAD_BACK},
  };
  for (auto [name, button] : pad) bind_pad(name, button);
  on_pad_layer(pad_layer);
  keys::install();
}
}  // namespace p3a::commands

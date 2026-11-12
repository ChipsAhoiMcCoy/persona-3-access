// What the field's own HUD shows, spoken: the interaction prompt, the story objectives, and items picked up.
// Read here:
//  - The interaction prompt is AUIMiscCheckDraw's (2026-11-03): its task state +0x294 is 1 while it appears and 2
//    while it is up (4 leaving, 5 gone), its label the FString at +0x9A8 ("Classroom 2-F"). The verb beside it is a
//    sprite, so it comes from the hit actor with that name nearby (its mCheckIcon_ and mType_).
//  - The objectives (top right, in corner brackets, then stacked lines) are drawn through AUIDrawBaseActor::DrawFont
//    (textcap source 0), while the field's panel is up: AUIAccessInfoDraw's task state +0x294 is 3.
//  - A picked-up item's line ("Revival Bead×1") is drawn at the left through FontDrawEx (textcap source 2).
#include "hooks.h"
#include "poi_internal.h"
#include "core/poi_text.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "msg.h"
#include "party.h"
#include "screens/util.h"
#include "seh.h"
#include "social.h"
#include "speech.h"
#include "textcap.h"
#include "world.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <format>
#include <set>

namespace p3a::poi {
using namespace ue;

// A point of interest named by a prompt's label: a Social Link partner's name carries its arcana ("Nozomi Suemitsu
// (Moon)"), the prompt only the name (his prompt had no verb and no rank, 2026-11-03).
using core::is_named;
// A way onward the game names as what you do there ("Go Elsewhere", "Head Up", "Return to Dorm", "Proceed Onward",
// "Enter the Velvet Room", "To furthest floor": every such first word in the DT_Fld*HitName tables), not as a place.
using core::named_as_action;
// `of`: the point of interest the prompt belongs to, the nearest of that name (left alone when it is nobody's).
static std::string verb_for(const std::string& label, WeakObj* of = nullptr) {
  for (auto& p : scan(800.0f)) {
    UObject* a = p.obj.resolve();
    // A Twilight Fragment and a rare or locked chest are listed under the mod's name for them; their prompts have
    // the game's ("Something Shining", "Treasure Chest").
    if (!is_named(p.name, label) &&
        !((p.kind == Kind::Fragment || p.kind == Kind::Thing) && a && is_a(a, "FldHitCore") && hit_name(a) == label)) continue;
    if (of) *of = p.obj;
    std::string icon = a ? icon_of(a) : "";
    // "Go to" only before a place the way leads to ("Go to: Iwatodai Station"). A label that already says what you do
    // ("Enter the Velvet Room", "Go Elsewhere", "Head Up") and a thing that is not an area change (Tartarus' "Giant
    // Device", a hit of type Normal under the same icon) stand alone: "Go to: Giant Device" and "Go to: Enter the
    // Velvet Room" read oddly (the user's call, 2026-11-09).
    if (icon == "Goto") {
      bool area_change = enum_prop_name(a, "mType_") == "AreaChange";
      return area_change && !named_as_action(label) ? strings::kVerbGoTo : "";
    }
    if (icon == "Speak") return strings::kVerbTalk;
    if (icon == "Listen") return strings::kVerbListen;
    if (icon == "Shop") return strings::kVerbShop;
    if (icon == "Action") return strings::kVerbAction;
    return strings::kVerbCheck;
  }
  // The party members following the hero are no point of interest, but their prompt is a talk ("Yukari Takeba").
  std::string first = label.substr(0, label.find(' '));
  for (int id = 2; id <= 11; ++id)
    if (std::string n = party::name(id); !n.empty() && n == first) return strings::kVerbTalk;
  return {};
}
// The story objective (top right, in corner brackets: "Head to the faculty office") is drawn by DrawFont; it is told
// apart by its brackets and spoken once per change.
static std::string g_objective, g_sub_objective;
std::string objectives() {
  if (g_sub_objective.empty() || g_objective.empty()) return g_objective;
  return g_objective + ". " + g_sub_objective;
}
static std::string g_prompt;   // the interaction prompt label on screen now, or ""
std::string prompt() { return g_prompt; }
// The interaction prompt is AUIMiscCheckDraw's (2026-11-03): its task state +0x294 is 1 while it appears and 2 while
// it is up (4 leaving, 5 gone), its label the FString at +0x9A8 ("Classroom 2-F"). The verb beside it is a sprite.
static std::string misc_check_label() {
  static const int id = watch::want("UIMiscCheckDraw");
  uint8_t* a = (uint8_t*)watch::get(id);
  std::string label;
  if (a && ue::known_build()) guarded([&] {   // this build's offsets: on another one no prompt is read
    int st = *(int32_t*)(a + 0x294);
    if (st == 1 || st == 2) label = core::clean_text(to_utf8(*(FString*)(a + 0x9A8)));
  });
  return core::has_words(label) ? label : std::string();
}
void prompt_tick() {
  // The objectives (top right, in corner brackets, then stacked lines) are drawn through DrawFont; not while a
  // message window draws through the same routine. Only while the field's panel is up (AUIAccessInfoDraw's state +0x294
  // is 3; 13 and 14 from a camp menu until the field comes back, town_map.cpp) or a place has no panel at all: between two
  // camp menus no screen is current, and the Item menu's Consumable Items tab, drawn the same way, was taken for an
  // objective (2026-11-06). The capture holds what was drawn just before too: only a draw since the field came back counts.
  static const int access_info = watch::want("UIAccessInfoDraw");
  uint8_t* panel = (uint8_t*)watch::get(access_info);
  int panel_state = -1;
  // The state's offset is this build's: on another one the panel reads as never up, and no objective is taken.
  if (panel && ue::known_build()) guarded([&] { panel_state = *(int32_t*)(panel + 0x294); });
  static bool field_was = false;
  static uint64_t field_since = 0;
  bool field_up = !panel || panel_state == 3;
  if (field_up && !field_was) field_since = hooks::frame();
  field_was = field_up;
  if (!msg::current().open && field_up) {
    // The panel's lines, top to bottom. It can hold several objectives at once, each in brackets with its own lines
    // under it: a Tartarus floor showed "Face Nyx", "Defeat Strega" and a second bracketed one, "Explore the floor"
    // (2026-11-11). A line is said when it comes into the panel, not while it stays: with one slot for the bracketed
    // line, two of them took turns in it and were said over and over, every frame.
    static std::vector<std::string> shown;   // the panel's lines last time it was drawn
    std::vector<std::pair<float, std::string>> lines;
    for (auto& t : textcap::snapshot(12)) {
      if (t.source != 0 || t.seen_frame <= field_since || !core::has_words(t.text) || t.x <= 1400 || t.y >= 420) continue;
      std::string o = t.text;
      // U+300E / U+300F corner brackets around an objective's own line
      if (o.rfind("\xE3\x80\x8E", 0) == 0) {
        o = o.substr(3);
        if (o.size() >= 3 && o.compare(o.size() - 3, 3, "\xE3\x80\x8F") == 0) o.resize(o.size() - 3);
      } else o = core::clean_text(o);
      if (std::none_of(lines.begin(), lines.end(), [&](auto& l) { return l.second == o; })) lines.push_back({t.y, o});
    }
    if (!lines.empty()) {
      std::sort(lines.begin(), lines.end(), [](auto& a, auto& b) { return a.first < b.first; });
      std::vector<std::string> now;
      for (auto& l : lines) {
        now.push_back(l.second);
        if (std::find(shown.begin(), shown.end(), l.second) == shown.end()) speech::speak(std::string(strings::kObjective) + ": " + l.second, false);
      }
      shown = now;
      g_objective = now[0];
      g_sub_objective.clear();
      for (size_t i = 1; i < now.size(); ++i) g_sub_objective += (i > 1 ? ". " : "") + now[i];
    }
  }
  std::string& last = g_prompt;
  static WeakObj prompt_of;   // the point of interest that prompt belongs to; none for a party member's
  std::string label = misc_check_label();
  if (label == last) return;
  if (label.empty()) {
    // The prompt hides and shows again while the hero closes in on the thing or person (Mitsuru's was said five times
    // walking up to her, 2026-11-03): it stays said until the hero is out of its reach. A following party member is
    // no point of interest: theirs stays said until another prompt shows.
    // The thing is the one the prompt was said for, asked itself: looking it up by name in a fresh list every frame
    // cost 0.1 ms a frame, and went on for good once the thing had left the list (an exit gone through, 2026-11-11).
    if (!last.empty() && prompt_of.index >= 0) {
      UObject* a = prompt_of.resolve();
      world::FVector me, v{};
      bool near_it = a && world::player_location(me) && guarded([&] { v = world::actor_location(a); }) &&
                     std::hypot(v.x - me.x, v.y - me.y, v.z - me.z) <= 300.0f;
      if (!near_it) { last.clear(); prompt_of = {}; }
    }
    return;
  }
  // A prompt that comes up behind a faded-out screen is not shown: a menu that closes after its fade-out (Rewind, a
  // load) leaves the old place's prompt up for a few frames, and "Dorm Register" was said on the way out (2026-11-10).
  if (ue::known_build() && screens::faded_out()) return;
  last = label;
  prompt_of = {};
  std::string verb = verb_for(label, &prompt_of);
  // A Social Link partner's prompt also shows the link's arcana and rank.
  std::string link = verb == strings::kVerbTalk ? social::partner_rank(label) : std::string();
  speech::speak((verb.empty() ? label : verb + ": " + label) + (link.empty() ? "" : ", " + link), true);
}

// Items picked up in the field (Tartarus chests): "Revival Bead×1" fades in at the left (FontDrawEx, about
// x 270 y 345, measured 2026-10-27). Spoken once it is fully drawn, when it appears (not again while it stays).
void pickup_tick() {
  static std::set<std::string> shown;   // the pickups drawn at full strength last tick
  std::set<std::string> now_shown;
  for (auto& t : textcap::snapshot(12)) {
    if (t.source != 2 || t.x > 600 || t.y < 250 || t.y > 650 || (t.color & 0xff) != 0xff) continue;
    size_t x = t.text.find("\xC3\x97");   // U+00D7 multiplication sign
    if (x == std::string::npos) continue;
    now_shown.insert(t.text);
    if (shown.count(t.text)) continue;
    std::string name = t.text.substr(0, x), count = t.text.substr(x + 2);
    while (!name.empty() && name.back() == ' ') name.pop_back();
    speech::speak(count == "1" ? std::format("{} {}", strings::kGot, name) : std::format("{} {}, {}", strings::kGot, name, count), false);
  }
  shown.swap(now_shown);
}
}  // namespace p3a::poi

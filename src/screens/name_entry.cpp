// The contract (name entry), a bespoke menu over the game's own state (measured 2026-11-02, keyboard and pad):
// ANameEntry (the actor the event makes; its draw AUINameEntryDraw at +0x328)
//   +0x2A0 int    the field cursor: 0 first name, 1 last name (A / D / Left / Right toggle it, wrapping)
//   +0x2A8 FText  the first name, +0x2C0 FText the last name (set when Enter commits a field)
//   +0x294 int    the state (4 the list, 5 typing, 6 and on signing)
//   +0x334 byte   open the first name by itself once the opening animation ends (draw +0x2D1, ~4.5 s after it starts)
//   +0x340 byte   open the field by itself in 1/3 s: a first name committed while the last is blank (the cursor moves on)
//   +0x341 byte   typing: the text field (WBP_NameEntryInput_C.CurrentEditableText) has the keys
// AUINameEntryDraw +0x2D1 (bIsFinishInAnim) the opening animation is over; +0x2D4 (bIsKeyOutAnim) is set by H (exe+0x14b9210
// state 6, which also opens UI type 6 for "Did I spell that correctly?"); the draw and the actor outlive the contract, so
// that is where it ends.
// AUINameEntryDraw +0x34A / +0x34B: set by a move of the field cursor, +0x34A (and +0x34B cleared) when it goes to the
// last name, +0x34B (and +0x34A cleared) when it goes to the first.
// The screen is a vertical list from the start (the user's call, 2026-11-04): the two names, mirrored to +0x2A0, and
// Done, which presses the game's H (pad Menu; the game ignores it while a name is blank). The field cursor is written
// as the list state's own Left / Right does it (the state's tick, exe+0x14b9770 case 4): the cursor sound, the draw's
// two flags, then the cursor. The game would open the fields by itself in a row: the mod clears its two flags
// (+0x334, +0x340), so a field opens only on Enter (pad A, which opens the same text field) and the first name's
// commit leaves the cursor where it was. While typing the game has the keys and core::TextField reads the box like a
// screen reader's own edit field.
#include "screens/menu.h"
#include "screens/util.h"
#include "core/strings.h"
#include "core/text_field.h"
#include "keys.h"
#include "log.h"
#include "seh.h"
#include "speech.h"
#include <windows.h>
#include <xinput.h>
#include <format>

namespace p3a::screens {
using namespace ue;

class NameEntryScreen : public MenuScreen {
  Finder entry_{"NameEntry"};
  uint8_t* ne() { return (uint8_t*)entry_.get(); }
  uint8_t* draw() {
    uint8_t* n = ne();
    UObject* d = n ? *(UObject**)(n + 0x328) : nullptr;
    return d && is_live(d) ? (uint8_t*)d : nullptr;
  }
  int cursor() { uint8_t* n = ne(); int c = n ? *(int32_t*)(n + 0x2A0) : -1; return c == 0 || c == 1 ? c : -1; }
  bool typing() { uint8_t* n = ne(); return n && n[0x341]; }
  bool both_named() { return !name(0).empty() && !name(1).empty(); }   // what the game's H checks
  // The list takes input: the opening animation is over and no field is open.
  bool list_up() { uint8_t* n = ne(); uint8_t* d = draw(); return n && d && d[0x2D1] && *(int32_t*)(n + 0x294) == 4 && !n[0x341]; }
  std::string name(int f) {
    uint8_t* n = ne();
    std::string s;
    if (n) guarded([&] { s = text(*(FText*)(n + (f == 0 ? 0x2A8 : 0x2C0))); });
    return s;
  }
  UObject* edit_box() {
    static const int input_id = watch::want("WBP_NameEntryInput_C");
    UObject* in = watch::get(input_id);
    return in ? get<UObject*>(in, "CurrentEditableText") : nullptr;
  }
  // Whether the edit box has the keys. Enter takes them away as it commits; the box's text changes in that frame (it
  // empties, or goes back to the old name) while +0x341 is still set: that is not an edit to echo.
  bool box_focused() { UObject* et = edit_box(); return et && call_ret<bool>(et, "HasKeyboardFocus"); }
  // The text field's live text (the name being typed).
  std::string field_text() {
    UObject* et = edit_box();
    UFunction* f = et ? find_function(class_of(et), "GetText") : nullptr;
    if (!f) return {};
    Params p(f);
    call(et, p);
    FText* t = p.at<FText>("ReturnValue");
    if (!t) return {};
    std::string s = text(*t);
    release_text(*t);
    return s;
  }
  static const char* field_name(int f) { return f == 1 ? strings::kLastName : strings::kFirstName; }
  std::string name_or_blank(int f) { std::string n = name(f); return n.empty() ? std::string(strings::kBlank) : n; }
  // The game opens its fields by itself on these two flags; here a field opens on the player's Enter only.
  void stop_opening_fields() { if (uint8_t* n = ne()) { n[0x334] = 0; n[0x340] = 0; } }
  // The field cursor put on field f the way the game's own move writes it (without its sound). Also for the first
  // name's commit, which moved the game's cursor on to a blank last name: back to the field it was on.
  void back_to_field(int f) {
    uint8_t* n = ne();
    uint8_t* d = draw();
    if (!n || !d) return;
    d[0x34A] = f == 1;
    d[0x34B] = f == 0;
    n[0x2A0] = (uint8_t)f;
  }
  std::string summary() { return std::format("{} {}, {} {}", strings::kFirstName, name_or_blank(0), strings::kLastName, name_or_blank(1)); }

  Mirror& fields_ = mirror("fields", [this] { return cursor(); });
  // Not a game list (no cursor): a mirror only so Left / Right on Done stay with the menu, not move the game's fields.
  Mirror& done_ = mirror("done", [] { return -1; });
  bool typed_ = false;      // typing when last ticked
  int typed_field_ = -1;
  core::TextField field_;   // the open field's text and caret

 public:
  NameEntryScreen() {
    fields_.keep_edges = true;
    fields_.ready = [this] { return list_up(); };
    fields_.set = [this](int f) {
      if (!list_up() || (f != 0 && f != 1)) return false;
      if (cursor() == f) return true;
      cursor_sound();
      back_to_field(f);
      return true;
    };
    done_.keep_edges = true;
  }
  const char* id() const override { return "name_entry"; }
  int priority() const override { return 20; }
  bool active() override {
    uint8_t* d = draw();
    if (!d || d[0x2D4]) return false;
    return call_ret<bool>((UObject*)d, "IsCursorName1Use") || call_ret<bool>((UObject*)d, "IsCursorName2Use");
  }
  bool escape_is_back() override { return false; }   // Escape is the game's here
  // The game has the keys only while a field is being typed.
  bool holds_keys() override { return !typing(); }
  std::string title() override { return strings::kContract; }
  void on_enter() override {
    typed_ = false; typed_field_ = -1;
    stop_opening_fields();
    screens::set_keys([this] { return typing() ? std::string(strings::kTypeThenEnter()) : std::string(strings::kNameEntryOptions()); });
  }
  void on_tick() override {
    stop_opening_fields();
    bool ty = typing();                 // the game's flag: the field is open
    bool keys = ty && box_focused();    // ...and its box still has the keys (not committing)
    int f = cursor();
    if (ty && (!typed_ || f != typed_field_)) {
      // A field opened for typing: its name and what is in it.
      std::string t = field_text();
      field_.open(t);
      say(std::format("{}. {}", field_name(f), t.empty() ? strings::kTypeThenEnter() : t), true);
    } else if (keys) {
      // The echo of typing is spoken directly, here and in on_game_key: the game has the keys, so a menu's guard
      // against saying a line twice would take the second of two equal echoes (a doubled letter, the same character
      // stepped over again) for a repeat.
      std::string t = field_text();
      if (t != field_.text()) speech::speak(field_.edited(t), true);
    } else if (!ty && typed_) {
      // Committed (the game cleared its flag once it stored the name): the list has the keys again, on the same name.
      log::writef("name_entry: first '{}' last '{}'", name(0), name(1));
      if (typed_field_ >= 0 && f != typed_field_) back_to_field(typed_field_);
      if (typed_field_ >= 0) focus_quietly(slot("fields", typed_field_));
      say_focus();
    }
    typed_ = ty; typed_field_ = ty ? f : typed_field_;
  }
  // A key on its way to the open text box: the arrows read it, as in any edit field.
  void on_game_key(uint32_t vk, int mods) override {
    if (mods != keys::None || !typing() || !box_focused()) return;
    using Key = core::TextField::Key;
    std::string say;
    switch (vk) {
      case VK_LEFT: say = field_.pressed(Key::Left); break;
      case VK_RIGHT: say = field_.pressed(Key::Right); break;
      case VK_HOME: say = field_.pressed(Key::Home); break;
      case VK_END: say = field_.pressed(Key::End); break;
      case VK_UP: case VK_DOWN: say = field_.pressed(Key::Line); break;
      case VK_BACK: field_.pressed(Key::Backspace); break;
    }
    if (!say.empty()) speech::speak(say, true);
  }
  void build(ui::GraphBuilder& b) override {
    if (cursor() < 0) return;
    b.begin_stop("fields");
    for (int i = 0; i < 2; ++i)
      b.add_item(slot("fields", i), game_item(fields_, i, [this, i] { return std::format("{}, {}", field_name(i), name_or_blank(i)); }));
    b.add_item(slot("done", 0), item([] { return std::string(strings::kNameDone); }, [this] {
      if (both_named()) press_game('H', XINPUT_GAMEPAD_START);
      else say(strings::kNamesMissing, true);
    }));
    b.set_start(slot("fields", cursor()));
  }
  std::string describe() override {
    if (typing()) { std::string t = field_text(); return std::format("{}: {}", field_name(cursor()), t.empty() ? strings::kBlank : t); }
    return summary();
  }
};

std::unique_ptr<Screen> make_name_entry() { return std::make_unique<NameEntryScreen>(); }
}  // namespace p3a::screens

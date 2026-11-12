// The title flow: ATitleActor runs a state machine (CurrentTitleState, reflected at +0x280). The menu itself is
// sprites, so its labels are ours (strings::kTitleItems); the cursor is native: ATitleActor +0x4cc (measured
// 2026-10-27 by diffing the actor across Down presses: 0 New Game, 1 Load Game, 2 Config, 3 Change Episode) and
// the last index at UTitleStateSelect +0xa0 (3 with the Episode Aigis DLC installed).
#include "screens/menu.h"
#include "screens/util.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "log.h"
#include "msg.h"
#include "speech.h"
#include "voice.h"
#include "ue/ue.h"
#include <format>

namespace p3a::screens {
using namespace ue;

static Finder g_title_actor{"TitleActor"};
// The title actor's current state object (a UTitleState*), or null.
static UObject* title_state_obj() { return get<UObject*>(g_title_actor.get(), "CurrentTitleState"); }
// A state's class without its "TitleState" prefix ("Select", "NewGame"), or "" for none.
static std::string title_state(UObject* s) {
  if (!s) return {};
  std::string n = name_of(class_of(s));
  return n.rfind("TitleState", 0) == 0 ? n.substr(10) : n;
}
static bool title_state_is(const char* s) { std::string t = title_state(title_state_obj()); return t == s || t == std::string(s) + "_Astrea"; }
// The title menu's item count, from the last index its state holds (UTitleStateSelect +0xa0), and an item's label.
static int title_count(UObject* state) {
  int last = state ? *(int*)((uint8_t*)state + 0xa0) : 3;
  return last >= 1 && last <= 5 ? last + 1 : 4;
}
static std::string title_item(int i) { return i < 4 ? strings::kTitleItems[i] : std::format("{} {}", strings::kItemNumber, i + 1); }

bool title_opening() { return title_state_is("OP"); }
// The opening movie takes a press as a skip only once its state has run 1 s (UTitleStateOP +0x44, the time since it
// started, against 1.0 in its tick); before that Enter goes to the movie player and pauses it. The line came with the
// state, so a press on hearing it paused the movie (read from the tick and seen live, 2026-10-29). Whichever screen
// is current says it (the movie screen takes over once the movie plays), once per showing.
static bool g_opening_said = false;
bool opening_line_due() {
  UObject* s = title_state_obj();
  std::string n = s ? name_of(class_of(s)) : "";
  if (n != "TitleStateOP" && n != "TitleStateOP_Astrea") { g_opening_said = false; return false; }
  if (g_opening_said || *(float*)((uint8_t*)s + 0x44) < 1.0f) return false;
  return g_opening_said = true;
}

// The boot notices (TitleStateCaution, TitleStatePhotosensitiveCaution) queue three animations on entry: the fade-in
// at +0x48, the display at +0x78 (7 s: DT_TitleUI CautionVisibleFrame 210, a fixed 210 at +0xdc for the second) and
// the fade-out at +0xa8, given the other two's length as its delay; the state moves on when the fade-out ends. A
// press (any button) once the fade-in is done and a wait has run (CautionSkipWaitTime 5 s, +0xe0 2 s for the second)
// stops the display and starts the fade-out at once (read from both states' enter and tick, 2026-10-29). Animation:
// +0x14 running, +0x15 done, +0x18 delay left (s), +0x1c elapsed, +0x20 length (s). Here the notice stays until a
// press, like the network question after them: the fade-in is cut short so a press counts at once, and a press
// stops the speech and ends the fade-out on the spot, so the next screen follows the press with no gap.
// The sounds are the network question's: the window opening on entry (the caller), confirm on the press.
// `pressed` is the caller's per-showing latch.
static void hold_notice(UObject* s, bool caution, UObject* actor, bool& pressed) {
  uint8_t* p = (uint8_t*)s;
  auto finish = [p](int a) { *(float*)(p + a + 0x18) = 0.0f; *(float*)(p + a + 0x1c) = *(float*)(p + a + 0x20); };
  if (p[0x78 + 0x15]) {   // pressed: the display stopped and the fade-out runs
    if (!pressed) { pressed = true; speech::stop(); voice::play_system_sound(voice::CueDecide); log::writef("title: notice pressed, fade-out {:.2f} s cut", *(float*)(p + 0xa8 + 0x20)); }
    finish(0xa8);
    return;
  }
  if (!p[0x48 + 0x15]) finish(0x48);
  *(float*)(p + 0x78 + 0x18) = 0.0f;
  *(float*)(p + 0x78 + 0x20) = 1e6f;
  *(float*)(p + 0xa8 + 0x18) = 1e6f;
  if (!caution) { *(float*)(p + 0xe0) = 0.0f; return; }
  UObject* t = get<UObject*>(actor, "TitleDataTable");
  int32_t off = t ? offset_of(datatable_struct(t), "CautionSkipWaitTime") : -1;
  if (off < 0) return;
  for (auto& r : datatable_rows(t)) *(float*)(r.data + off) = 0.0f;
}

class TitleScreen : public Screen {
  std::string state_;
  int cursor_ = -1;
  bool notice_pressed_ = false;

 public:
  const char* id() const override { return "title"; }
  bool active() override { return title_state_obj() != nullptr; }
  // Entering again reads the state again (after a message window).
  void enter() override { state_.clear(); cursor_ = -1; }
  void tick() override {
    UObject* s = title_state_obj();
    std::string st = title_state(s);
    if (opening_line_due()) speech::speak(strings::kTitleOpening, true);
    if (st != state_) {
      state_ = st;
      cursor_ = -1;
      notice_pressed_ = false;
      if (st == "Caution" || st == "PhotosensitiveCaution") voice::play_system_sound(voice::CueWindowOpen);
      if (st == "Caution") { screens::set_keys(strings::kNoticeKeys); speech::speak(strings::kTitleCaution, true); }
      else if (st == "PhotosensitiveCaution") { screens::set_keys(strings::kNoticeKeys); speech::speak(strings::kTitlePhotosensitive, true); }
    }
    if (st == "Caution" || st == "PhotosensitiveCaution") hold_notice(s, st == "Caution", g_title_actor.get(), notice_pressed_);
    // The logos before the opening movie (about 13 s with nothing else on screen): UTitleStateLogo +0x44 is the one
    // shown, 0 Atlus, 1 P-Studio, 2 Unreal Engine, 3 fading out (measured 2026-10-28). cursor_ holds the last read.
    // Only the first interrupts: a description still running when the next logo comes is finished, not cut.
    if (st == "Logo" || st == "Logo_Astrea") {
      int l = *(int*)((uint8_t*)s + 0x44);
      if (l != cursor_ && l >= 0 && l < 3) speech::speak(strings::kTitleLogos[l], l == 0);
      cursor_ = l;
    }
    // "Press any button" waits until a press counts: the state plays the banner's animation 1.75 s in (+0x4c set) and
    // takes no key until it ends (ATitleActor +0x3da, inside CurvePressAnyButtonBlueRect, cleared on the way in), 2.7 s
    // after the state starts; the line used to come with the state, and a key pressed on hearing it did nothing
    // (measured and read from both states' tick, 2026-10-29). cursor_ 1 = said.
    if ((st == "PressWait" || st == "PressWait_Astrea") && cursor_ < 0 && *((uint8_t*)s + 0x4c) &&
        *((uint8_t*)g_title_actor.get() + 0x3da)) {
      screens::set_keys(strings::kPressAnyKey);
      speech::speak(st == "PressWait" ? strings::kTitlePressAny : strings::kTitlePressAnyAstrea, true);   // _Astrea: Episode Aigis
      cursor_ = 1;
    }
  }
  std::string describe() override { return std::string(strings::kTitleScreen) + ", " + state_; }
};

// The title and difficulty menus share one native select-cursor struct (title ATitleActor +0x478, difficulty
// ADifficultySelectionActor +0x3D0): +0x08 input locked, +0x18 step (2 takes input), +0x54 the cursor, +0x191 set once
// confirmed. exe+0x153f040(struct, nullptr, i) is its mouse path: the cursor, the sound and the items' highlight, as a
// key's move does (it refuses when the struct takes no input).
static bool select_ready(uint8_t* s) { return s && *(int32_t*)(s + 0x18) == 2 && !s[0x08] && !s[0x191]; }
static bool select_set(uint8_t* s, int i) { return s && ((bool (*)(void*, void*, int))(scan::exe_base() + 0x153f040))(s, nullptr, i); }

// The title menu (New Game, Load Game, Config, Change Episode): the menu (ATitleActor +0x478; +0x4cc is its cursor)
// slides its items in for about 2 s before it takes a key: its step at +0x490 is 1 meanwhile, 2 once up/down and
// confirm count (read from UTitleStateSelect's tick). The screen opens at 2: a press on hearing it counts.
class TitleMenuScreen : public MenuScreen {
  Mirror& list_ = mirror("title", [] { UObject* a = g_title_actor.get(); return a ? *(int*)((uint8_t*)a + 0x4cc) : -1; });
  static uint8_t* sel() { UObject* a = g_title_actor.get(); return a ? (uint8_t*)a + 0x478 : nullptr; }
 public:
  TitleMenuScreen() {
    list_.ready = [] { return select_ready(sel()); };
    list_.set = [](int i) { return select_set(sel(), i); };
  }
  const char* id() const override { return "title_menu"; }
  int priority() const override { return 1; }
  bool active() override {
    uint8_t* s = sel();
    return s && title_state_is("Select") && *(int32_t*)(s + 0x18) == 2;   // the menu's step: ATitleActor +0x490
  }
  std::string title() override { return strings::kTitleMenu; }
  void on_enter() override { screens::set_keys(strings::kChooseKeys); }
  void build(ui::GraphBuilder& b) override {
    b.begin_stop("title");
    int n = title_count(title_state_obj());
    for (int i = 0; i < n; ++i) {
      std::string label = title_item(i);
      b.add_item(slot("title", i), game_item(list_, i, [label] { return label; }));
    }
    if (int c = list_.cursor(); c >= 0) b.set_start(slot("title", c));
  }
};

// New Game -> difficulty. ADifficultySelectionActor: cursor +0x424 (0 Peaceful .. 4 Merciless), previous +0x43c
// (measured 2026-10-27). The descriptions are BMD pages dialog 40..44 of the actor's message file, all built when
// the screen opens; the labels are sprites (strings::kDifficulties).
class DifficultyScreen : public MenuScreen {
  Finder actor_{"DifficultySelectionActor"};
  Mirror& list_ = mirror("difficulty", [this] { UObject* a = actor_.get(); return a ? *(int*)((uint8_t*)a + 0x424) : -1; });
  uint8_t* sel() { UObject* a = actor_.get(); return a ? (uint8_t*)a + 0x3D0 : nullptr; }
  std::string description(int c) {
    for (auto& p : msg::visible(18)) if (p.dialog == 40 + c) return p.text;
    std::string best;
    for (auto& p : msg::created_since(0)) if (p.dialog == 40 + c) best = p.text;   // newest wins
    return best;
  }
 public:
  DifficultyScreen() {
    list_.ready = [this] { return select_ready(sel()); };
    list_.set = [this](int i) { return select_set(sel(), i); };
  }
  const char* id() const override { return "difficulty"; }
  int priority() const override { return 10; }
  bool active() override {
    UObject* a = actor_.get();
    // +0x3E8 is the actor's step: 2 while the list takes input (a cancelled "Begin the game at ...?" leaves it at 2),
    // 3 from the moment that question is confirmed. The actor stays up for a second after that, and the question's
    // window closing re-entered the screen and read it again just before the DLC notice (2026-10-29).
    // The actor is made at step 0 with the cursor on Peaceful; step 1 (the opening) already has the game's default
    // (Normal, or the last choice; measured 2026-11-03): the list is up from step 1.
    int step = a && !is_cdo(a) ? *(int*)((uint8_t*)a + 0x3E8) : -1;
    return a && !is_cdo(a) && get<UObject*>(a, "AssetLoader") != nullptr && *(int*)((uint8_t*)a + 0x424) >= 0 &&
           *(int*)((uint8_t*)a + 0x424) < 5 && (step == 1 || step == 2) && title_state_is("NewGame");
  }
  std::string title() override { return strings::kSelectDifficulty; }
  void on_enter() override { screens::set_keys(strings::kChooseKeys); }
  void build(ui::GraphBuilder& b) override {
    b.begin_stop("difficulty");
    for (int i = 0; i < 5; ++i) {
      std::string label = strings::kDifficulties[i];
      // On a move, only the part that differs: every description ends with the same "Your choice of difficulty..."
      auto v = game_item(list_, i, [label] { return label; }, [this, i] { return description(i); });
      describe_with(v, [this, i] { return core::first_sentence(description(i)); });
      b.add_item(slot("difficulty", i), v);
    }
    if (int c = list_.cursor(); c >= 0) b.set_start(slot("difficulty", c));
  }
};

std::unique_ptr<Screen> make_title() { return std::make_unique<TitleScreen>(); }
std::unique_ptr<Screen> make_title_menu() { return std::make_unique<TitleMenuScreen>(); }
std::unique_ptr<Screen> make_difficulty() { return std::make_unique<DifficultyScreen>(); }
}  // namespace p3a::screens

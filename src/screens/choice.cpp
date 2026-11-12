// A message window's choice list (USelItem, msg.cpp: +0x28 its state, +0x60 its ListCursor) as a bespoke menu over
// the window. The window reads the question and then the focused answer with its position (msg's say_after keeps that
// order around voiced lines), so entering the menu says nothing more; moves and Enter are the menu's, and the game's
// cursor follows. The answers are the ones msg reads (msg::current().choices).
#include "screens/menu.h"
#include "screens/util.h"
#include "core/speech_text.h"
#include "msg.h"
#include <format>

namespace p3a::screens {

class ChoiceScreen : public MenuScreen {
  Mirror& list_ = mirror("choice", [] {
    msg::Current c = msg::current();
    return c.open && c.choice >= 0 && c.choice < (int)c.choices.size() ? c.choice : -1;
  });
 public:
  // The USelItem's ListCursor (+0x60) written, then the cursor sound, as its tick's move does. It takes moves in its
  // states 5 and 6 while the global input lock (exe+0x56ac964) is off. (One subclass's tick plays cue 0x13 instead.)
  ChoiceScreen() {
    list_.ready = [] {
      auto* s = (uint8_t*)msg::sel_item();
      int st = s ? *(int32_t*)(s + 0x28) : 0;
      return (st == 5 || st == 6) && !*(uint8_t*)(scan::exe_base() + 0x56ac964);
    };
    list_.set = [](int i) {
      auto* s = (uint8_t*)msg::sel_item();
      if (!s || !ListCursor(s + 0x60).set(i)) return false;
      // The ending's last choice (USelAEventTypeB: two answers side by side) opens with no answer picked: its row on
      // screen (+0x40) is -1 until Left or Right is pressed, and Enter did nothing meanwhile (2026-11-11; after a
      // Right it read 1 and Enter was taken). The row is given with the cursor. UNTESTED: the write itself was not
      // tried (the ending cannot be replayed quickly); if Enter still does nothing there, Left or Right picks one.
      if (*(int32_t*)(s + 0x40) == -1) *(int32_t*)(s + 0x40) = i;
      cursor_sound();
      return true;
    };
  }
  const char* id() const override { return "choice"; }
  int priority() const override { return 35; }   // under the log (40), which opens over it
  bool over_messages() const override { return true; }
  bool active() override { msg::Current c = msg::current(); return c.open && c.choice >= 0 && core::all_filled(c.choices); }
  bool announce_entry() override { return false; }
  // F1 over a choice list: the screen is over the message window, so the keys are its own to name (it answered "No
  // keys to tell here", 2026-11-10).
  void on_enter() override { screens::set_keys(strings::kChooseKeys); }
  // The review key: the question, then the focused answer.
  std::string describe() override {
    msg::Current c = msg::current();
    std::string q = core::spoken_line(c.speaker, c.text);
    std::string a = MenuScreen::describe();
    return q.empty() ? a : a.empty() ? q : q + ". " + a;
  }
  void build(ui::GraphBuilder& b) override {
    msg::Current c = msg::current();
    if (!core::all_filled(c.choices)) return;
    // A new list is a new key: its answers, joined.
    std::string key = "choice";
    for (auto& x : c.choices) key += "|" + x;
    list_.key = std::format("choice.{}", std::hash<std::string>{}(key) & 0xFFFFFF);
    b.begin_stop(list_.key);
    for (int i = 0; i < (int)c.choices.size(); ++i) {
      // An answer's own full stop goes ("Nice to meet you." before its position was read as a dot).
      std::string a = core::without_own_stop(c.choices[i]);
      b.add_item(slot(list_.key, i), game_item(list_, i, [a] { return a; }));
    }
    if (c.choice >= 0) b.set_start(slot(list_.key, c.choice));
  }
};

std::unique_ptr<Screen> make_choice() { return std::make_unique<ChoiceScreen>(); }
}  // namespace p3a::screens

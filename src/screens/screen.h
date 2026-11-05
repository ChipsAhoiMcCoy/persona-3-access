#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "core/strings.h"

namespace p3a::screens {
// One game screen, modelled from the game's own objects. All methods run on the game thread.
// Speech rules (from Grim Dark): focus moves interrupt; entering a screen interrupts with its title and the focused
// item; nothing else interrupts. Positions ("2 of 5") are spoken on every move (the player's choice); the title on
// entry. Key hints only on demand (F1, the player's choice 2026-10-28): a screen names its keys with set_keys.
class Screen {
 public:
  virtual ~Screen() = default;
  virtual const char* id() const = 0;
  virtual int priority() const { return 0; }   // higher wins when several are active (a popup over a menu)
  virtual bool active() = 0;                   // is the game showing this screen now
  virtual void enter() {}                      // became the current screen
  virtual void tick() {}                       // every mod tick while current
  virtual void leave() {}
  virtual std::string describe() { return {}; }   // the review key: the whole screen state, spoken on demand
  virtual void background() {}                 // every mod tick, current or not, even under a message window
  virtual bool over_messages() const { return false; }   // opens over a message window (the log): runs under one too
  // A message window over this screen is one page of its own (a Dictionary term), never the first of a chain of
  // notices: once it closes, re-enter at once instead of waiting 700 ms for another window.
  virtual bool own_messages() { return false; }
  // Whether Escape is Back / Cancel here (the game's C, pressed for it; menu.cpp): every bespoke menu, and a battle
  // prompt. False where C is a command (the battle wheel's Guard) or Escape is the game's own.
  virtual bool escape_is_back() { return false; }
};

void add(std::unique_ptr<Screen> s);
void tick();                // picks the current screen, runs enter/leave/tick
Screen* current();
// The current screen's keys for F1: the latest a screen named while it was current (a screen with steps names each
// step's keys as it enters it).
void set_keys(const strings::Hint& h);
void set_keys(std::function<std::string()> keys);
std::string keys();   // "" when the current screen named none
// Should the current screen speak its title with this line: true once per opening of the screen (the caller then
// speaks it), false when the player comes back to it from a submenu, a message window or a page read over it.
// A screen with several menus keys each one's title by its step.
bool title_due(const char* step = "");
// A save is loading (from the boot, or the title or save list being up, until the hero is in the field): values the
// trackers watch (money, stats, links) jump from the title's zeros to the save's, which is not news.
bool loading();
// The title's opening movie (title.cpp): is it the title's state now, and is its "press any button to skip" due (true
// once per showing, when a press would skip it).
bool title_opening();
bool opening_line_due();
std::string status();
std::string perf(bool reset);   // each screen's active() and tick() cost (/perf)
void register_all();       // every screen the mod models (screens.cpp)
}  // namespace p3a::screens

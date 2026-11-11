#pragma once
// Bespoke menus (the Grim Dark model, 2026-10-30): the mod owns the menu. A MenuScreen declares a graph over its own
// model of what the game shows (core/graph_builder.h: lists, rows, Tab-stops, info lines), read from the game's
// objects, never from drawn text. The navigator (core/navigator.h) owns focus, movement, positions and every
// announcement, so a move is spoken from the model at once, the same way on every screen.
//
// The game's visuals follow the mod: a stop mirrored onto a game list (Mirror) drives the game's cursor to the focused
// item (written directly where the game allows it, else stepped with the game's own keys, retried when the game drops
// a key during an animation), and Enter confirms in the game once its cursor is there. A cursor the game moves on its
// own (the stick, the mouse, a tab switch) moves the mod's focus with it.
//
// Keys while a menu has focus (no message window over it): arrows and the D-pad move, Home/End jump, Tab / Shift+Tab
// change panel (stop) when the screen has several, Enter / Space / A activate. Everything else reaches the game.
// A page in sections (set_region within a stop, each section a context with its title): the arrows walk every line
// as one vertical list; Tab / Shift+Tab (Page Down / Page Up, RT / LT) jump to the next / previous section,
// wrapping, and say its title.
#include "screens/screen.h"
#include "ue/ue.h"
#include "core/graph_builder.h"
#include "core/graph_types.h"
#include "core/ui_screen.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace p3a::screens {

// A game list the mod's focus drives. Items of the stop are slot(key, i) for the game's cursor index i.
struct Mirror {
  std::string key;
  std::function<int()> cursor;             // the game's cursor now; -1 = the list is not up
  std::function<bool(int)> set;            // write the cursor directly (false = not now); empty = step with keys
  // Whether the game takes input now (false: an animation or an input lock). While it does not, the menu says nothing
  // of its opening and drops moves and confirms, as the game does.
  // Empty: the widget's IsAnyAnimationPlaying when `widget` is given, else always ready.
  std::function<bool()> ready;
  std::function<ue::UObject*()> widget;    // the UUserWidget that draws the list (the default `ready`)
  // How the game steps a list with no `set`, declared, never guessed:
  bool horizontal = false;                 // the game moves this cursor with left/right
  bool reversed = false;                   // up / left raise the game's index (a list drawn bottom to top)
  int count = 0;                           // >0: the game list wraps over this many (steps take the short way)
  // A grid: item i's (row, column) on screen. Steps go up/down to the row, then left/right to the column.
  std::function<std::pair<int, int>(int)> grid;
  uint32_t confirm_vk = 0x0D;              // the game's confirm (Enter); 0 = the screen's own on_activate only
  // A row whose game list wraps by itself: Left/Right at the mod's ends stay with the mod (passed on, they wrapped the
  // game's Shift cursor and the focus followed it, 2026-10-31). Off where an end's press is the game's (the calendar's
  // last day: the next month).
  bool keep_edges = false;
  // Whether a game list is behind this at all (Done on the contract has none: nothing to wait for, no cursor to tick).
  bool drives_game() const { return set || ready || widget; }
  // runtime (host)
  int seen = -1, target = -1, step_from = -1, steps = 0;
  std::string last_key;                    // the key last synced: a new key is a new list, whose cursor the game set
  bool follow = false;                     // focus goes to the game's cursor once it is up (a new list)
  uint64_t step_at = 0;
};

class MenuScreen : public Screen, public ui::Screen {
 public:
  // screens::Screen. Subclasses override on_enter/on_tick/on_leave instead.
  void enter() final;
  void tick() final;
  void leave() final;
  std::string describe() override;
  // ui::Screen: title() is spoken on entry (once per opening), before the focused item.
  std::string screen_name() const override { return const_cast<MenuScreen*>(this)->title(); }
  bool allows_typeahead() const override { return false; }   // letters are the game's keys
  bool escape_is_back() override { return true; }
  // The player moved: a new list's pending "focus follows the game's cursor" is dropped.
  void stop_following() { for (auto& m : mirrors_) m->follow = false; }
  // Whether the focused list takes input now (Mirror::ready, and its cursor is up): the menu waits for the game.
  bool takes_input();
  // Whether the menu has the keys now (false: something of the game's is over it, a term's page, a prompt).
  virtual bool holds_keys() { return true; }
  // A key down going to the game while it has the keys (a screen that reads what the game does with it).
  virtual void on_game_key(uint32_t, int) {}
  // Whether the title is said on every entry (not once per opening of what is underneath).
  virtual bool title_every_entry() { return false; }
  // Whether entering the menu reads its focused item (false: something else has just said where the player is).
  virtual bool announce_entry() { return true; }
  // Whether the title cuts off what is being said (false: what just happened finishes first, the blow that won a battle
  // before "Victory").
  virtual bool opening_interrupts() { return true; }
  // Whether Tab is the game's even on a page with sections (the compendium's registered stats); Page Up / Page Down
  // and the triggers still jump them.
  virtual bool game_tab() { return false; }
  // Whether a lone trigger is the game's even on a page with sections (a member's Stats page: LT Theurgy, RT Persona
  // status); the keyboard's Tab / Page keys still jump them.
  virtual bool game_triggers() { return false; }
  // Whether Q / E and LB / RB are the screen's (on_bumper, -1 / +1; the game never sees them): a page that shows one
  // entry of a list at a time (the camp's Persona list).
  virtual bool takes_bumpers() { return false; }
  virtual void on_bumper(int) {}
  // Whether F and X (the game's Persona status page) do nothing: a screen that already is that page.
  virtual bool blocks_status() { return false; }
  // The detail buffer (battle_details): what the right stick up and down, or Ctrl+Up and Ctrl+Down, walk line by line
  // for the focused item, its own line first. What a row would otherwise say on every move (a skill's description, a
  // member's Persona, stats and skills) goes here (the user's call, 2026-11-11). has_details is asked every frame and
  // is cheap; the lines are built on a press.
  // By default they are the focused row's own (buffer_with); a screen overrides both for more.
  virtual bool has_details();
  virtual std::vector<std::string> detail_lines();
  // The focused item's identity: landing on another starts the buffer at its top again.
  std::string detail_key() const;

  // The id of a mirrored list's item i.
  static ui::ControlId slot(const std::string& key, int i);
  // A plain item: its label (live), an optional activation, optional details (the review key).
  static ui::NodeVtablePtr item(std::function<std::string()> label, std::function<void()> activate = {},
                                std::function<std::string()> details = {});
  // A description read after the item's position, and again when it changes while focused (a page the game builds
  // once its cursor is on the item).
  static void describe_with(const ui::NodeVtablePtr& v, std::function<std::string()> text);
  // A description kept off the row: the detail buffer's second line (the row's own label is its first), and part of
  // what the review key reads. For a list gone through often, where the same words after every row were noise (the
  // user's call, 2026-11-11). A choice made once, or a row whose description is its content, keeps describe_with.
  static void buffer_with(const ui::NodeVtablePtr& v, std::function<std::string()> text);
  // A mirrored list's item: activation confirms it in the game once the game's cursor is on it.
  ui::NodeVtablePtr game_item(Mirror& m, int i, std::function<std::string()> label, std::function<std::string()> details = {});
  // Lines to the right of a list's row i (of n): Right walks them (each stat, each skill), Left walks back, Up and Down
  // from any of them go to the rows above and below. Call after every row is added (raw nodes declared between rows
  // break the list's own wiring). The game's cursor stays on the row, so Enter on a line confirms that row.
  // Only the focused row's (focused_row): a walk for every row of a long list cost 0.4 ms a frame (the compendium).
  static void add_walk(ui::GraphBuilder& b, const std::string& key, int i, int n, const std::vector<std::string>& lines);
  // The row of list `key` the focus is on (its item, or a line of its walk), or -1.
  int focused_row(const std::string& key) const;

 protected:
  virtual std::string title() { return {}; }
  virtual void on_enter() {}
  virtual void on_tick() {}   // before the navigator's pull: refresh the model here
  virtual void on_leave() {}
  Mirror& mirror(std::string key, std::function<int()> cursor);
  Mirror* mirror_of(const std::string& key);
  // Confirm in the game on the mirrored item once its cursor is there.
  void confirm(Mirror& m, int i);
  // Move the mod's focus (announced, interrupting unless `interrupt` is false) without a key: a screen that changed
  // its own model.
  void focus(const ui::ControlId& id, bool interrupt = true);
  // Move the focus without a word (the screen has said where it is): the game moved its cursor while it had the keys.
  void focus_quietly(const ui::ControlId& id);
  // Say the focused item again, interrupting (the focus may not have moved: the player came back to it from the game).
  void say_focus();
  // A line of the screen's own (a page that changed, a result, a notice). It goes the way every menu line goes: not
  // said twice until the player presses something or something else speaks.
  static void say(std::string_view text, bool interrupt = false);
  std::optional<ui::ControlId> focused() const;
 public:
  // Whether the focused item's list keeps the presses at its ends (Mirror::keep_edges).
  bool keeps_edges();
  // Whether Left/Right at the focused item's ends go to its game list, which answers them (a mirrored item, no keep_edges).
  bool edges_to_game();
 protected:
  // Key presses for the game, the player's device (keyboard, or the pad while they are on one).
  static void press_game(uint32_t vk, uint32_t pad_button);

 private:
  friend struct MenuHost;
  std::vector<std::unique_ptr<Mirror>> mirrors_;
  int pending_confirm_ = -1;
  uint64_t confirm_moves_ = 0;
  Mirror* confirm_mirror_ = nullptr;
  bool attached_ = false;
  // The opening line, said once the list takes input.
  bool entry_due_ = false;
  bool entry_say_ = true;
  std::string entry_title_;
  void say_entry();
  bool entry_ready();
};

namespace menu {
void install();                 // navigator hooks, key capture, the /menu dev route
void frame();                   // every mod tick (screens::tick): pad capture while a menu has focus
MenuScreen* active();           // the menu that has the keys now, or null
std::string dump();             // the focused screen's graph, one node per line (/menu)
}  // namespace menu
}  // namespace p3a::screens

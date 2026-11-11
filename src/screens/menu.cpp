// The bespoke-menu host (menu.h): one GraphNavigator for whichever MenuScreen is current, the keys and D-pad it takes
// from the game while that screen has focus, and the two-way sync between the mod's focus and the game's cursors.
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/battle_details.h"
#include "core/graph_announcer.h"
#include "core/navigator.h"
#include "core/strings.h"
#include "devserver.h"
#include "input.h"
#include "keys.h"
#include "log.h"
#include "msg.h"
#include "seh.h"
#include "speech.h"
#include <windows.h>
#include <xinput.h>
#include <format>
#include <set>

namespace p3a::screens {
struct MenuHost;
namespace {
MenuScreen* g_active = nullptr;   // the MenuScreen being ticked (current, no message window over it)
bool g_entry_line = false;        // the next navigator line opens the screen: it interrupts
// A confirm or cancel the player just pressed: the landing that answers it (a new list's, which the navigator queues)
// interrupts. NVDA stops speech on any key press by itself; a pad press stops nothing, so the member's first skill
// waited behind their name (2026-10-31). Only a line spoken with the focus somewhere new counts (a live part of the
// row pressed on still queues). It lasts until that landing or the player's next move.
bool g_press_armed = false;
std::optional<ui::ControlId> g_press_on;   // the focus when it was pressed
void armed_press();
uint64_t g_frame = 0;
std::optional<ui::ControlId> g_move_from;   // the focus before the player's key: a landing on it again is an edge
void move_sound();

// Every line of a menu goes through here, so that a menu can never repeat itself on its own (players heard NVDA "stuck
// on a menu option", 2026-11-05). A cursor flickering between two values, a live value that alternates, or a screen
// that keeps being entered would each read the same words over and over: a line already said is dropped until the
// player presses something or something else speaks. Nothing here counts time.
uint64_t g_moves = 0;   // the player's moves so far: a confirm waiting for the game's cursor is dropped by the next
void say_line(std::string_view text, bool interrupt) {
  static std::set<std::string, std::less<>> said;
  static uint64_t moves = 0, stick = 0, spoken = 0;
  uint64_t pad = input::pad_ms();
  if (moves != g_moves || stick != pad || spoken != speech::count()) { said.clear(); moves = g_moves; stick = pad; }
  if (!said.emplace(text).second) {
    log::writef("menu: not repeating \"{}\"", text);
    return;
  }
  speech::speak(text, interrupt);
  spoken = speech::count();
}
bool g_held_repeat = false;   // the press being handled is a held key's or D-pad's repeat
// The device of the player's last press in a menu: 0 the keyboard, 1 the controller, -1 none yet. What the menu then
// presses in the game for that press (a step, the confirm) goes out on the same device, also ticks later. Asking
// input::pad_in_use() at that moment instead sent a keyboard confirm out as the pad's A whenever a controller had
// stirred since the key (a stick at rest near its edge, a pad in use elsewhere).
int g_device = -1;

ui::GraphNavigator& nav() {
  static ui::GraphNavigator n(ui::NavigatorHost{
      [](std::string_view text, bool interrupt) {
        // A held key at the end of a list: its first press repeated the item, the rest say nothing (a stick that
        // drifts or a stuck key read one line again and again until it was let go).
        if (g_held_repeat && g_move_from && nav().focused_id() == g_move_from) return;
        if (g_entry_line) { interrupt = true; g_entry_line = false; }
        if (g_press_armed) {
          auto at = nav().focused_id();
          if (at != g_press_on) { interrupt = true; g_press_armed = false; }
        }
        say_line(text, interrupt);
      },
      [] { move_sound(); },
      [] { return g_active != nullptr; },
      [] { return (int)g_frame; },
      // Left/Right at the end of a game list that does not keep its edges go to the game, which moves on (the next
      // month, a wrap) and the focus follows it. The mod's own lines (a row's walk) have no game answer: they repeat.
      [](ui::GraphDir d) { return (d == ui::GraphDir::Left || d == ui::GraphDir::Right) && g_active && g_active->edges_to_game(); }});
  return n;
}

bool has_focus_now() {
  if (!g_active || screens::current() != g_active) return false;
  if (msg::current().blocking && !g_active->over_messages()) return false;   // the log opens over a conversation
  bool keys = true;
  guarded([&] { keys = g_active->holds_keys(); });
  return keys;
}

bool activatable() {
  const ui::GraphNode* f = nav().focused_node();
  return f && f->vtable && f->vtable->on_activate;
}

// Several stops: Tab moves between them; with one, Tab is the game's.
bool several_stops() { return nav().stop_count() > 1; }
// A page in sections (regions of one stop: a status page's stats, skills, affinities): Tab / Shift+Tab, Page Down /
// Page Up and a trigger press jump between them; the arrows walk on through every line.
bool sections() { return nav().section_count() > 1; }
bool tab_is_game() { bool t = false; guarded([&] { t = g_active->game_tab(); }); return t; }
bool triggers_are_game() { bool t = false; guarded([&] { t = g_active->game_triggers(); }); return t; }
bool bumpers() { bool t = false; guarded([&] { t = g_active->takes_bumpers(); }); return t; }
void bumper(int dir) { guarded([&] { g_active->on_bumper(dir); }); }
bool status_blocked() { bool t = false; guarded([&] { t = g_active->blocks_status(); }); return t; }
// Where the game's details key is blocked (the details are read with each row), it reads the focused row again,
// position and description included (the user's call, 2026-11-01).
void reread() {
  const ui::GraphNode* f = nav().focused_node();
  std::optional<std::string> t;
  if (f) guarded([&] { t = ui::GraphAnnouncer::compose_full(f); });
  if (t && !t->empty()) speech::speak(*t, true);
}

void player_moved() {
  ++g_moves;
  if (g_active) g_active->stop_following();
  g_move_from = nav().focused_id();
  g_press_armed = false;   // a move speaks for itself (interrupting)
}

void armed_press() {
  g_press_armed = true;
  g_press_on = nav().focused_id();
}

bool on_key(uint32_t vk, int mods, bool repeat) {
  // Ctrl+Up / Ctrl+Down: the details of what is in focus (a battle's units, a menu row that has some). Elsewhere the
  // keys stay the game's.
  if (mods == keys::Ctrl && (vk == VK_UP || vk == VK_DOWN) && battle_details::in_focus()) {
    if (!repeat) battle_details::step(vk == VK_DOWN ? 1 : -1);
    return true;
  }
  // Escape is Back / Cancel in every menu and on the battle screens (the user's call, 2026-11-02): the game's own key
  // for it is C, which still works. Elsewhere Escape stays the game's (a movie, name entry, the field).
  if (vk == VK_ESCAPE && mods == keys::None) {
    Screen* cur = screens::current();
    bool back = false;
    if (cur) guarded([&] { back = cur->escape_is_back(); });
    if ((has_focus_now() && back) || (back && !dynamic_cast<MenuScreen*>(cur))) {
      if (has_focus_now()) { player_moved(); armed_press(); }
      if (!repeat) input::inject_key('C');
      return true;
    }
  }
  if (!has_focus_now()) {
    if (g_active && screens::current() == g_active) guarded([&] { g_active->on_game_key(vk, mods); });
    return false;
  }
  // The game takes no input now (an opening animation, an input lock): nor does the menu. The press is dropped, as the
  // game drops it (the user's call, 2026-11-05: no moving ahead of an animation).
  bool nav_key = (mods == keys::None && (vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT || vk == VK_HOME || vk == VK_END ||
                                         vk == VK_RETURN || vk == VK_SPACE || vk == VK_TAB || vk == VK_NEXT || vk == VK_PRIOR)) ||
                 (mods == keys::Shift && vk == VK_TAB);
  if (nav_key && !g_active->takes_input()) return true;
  // A held confirm is one press: repeating it activated the row again on every auto-repeat.
  if (repeat && mods == keys::None && (vk == VK_RETURN || vk == VK_SPACE)) return true;
  g_held_repeat = repeat;
  struct HeldEnd { ~HeldEnd() { g_held_repeat = false; } } held_end;
  g_device = 0;
  player_moved();
  if (mods == keys::None && (vk == VK_RETURN || vk == VK_SPACE || vk == VK_ESCAPE || vk == VK_BACK)) armed_press();
  using namespace ui::ui_actions;
  auto& n = nav();
  if (mods == keys::None) {
    switch (vk) {
      case VK_UP: n.on_action(Up); return true;
      case VK_DOWN: n.on_action(Down); return true;
      case VK_LEFT: return n.on_action(Left) || g_active->keeps_edges();
      case VK_RIGHT: return n.on_action(Right) || g_active->keeps_edges();
      case VK_HOME: return n.on_action(Home);
      case VK_END: return n.on_action(End);
      // An item with nothing to do (a line of information) leaves Enter to the game: a status page's confirm.
      case VK_RETURN: case VK_SPACE: return activatable() && n.on_action(Activate);
      case VK_TAB:
        if (sections() && !tab_is_game()) return n.on_action(SectionNext);
        return several_stops() && n.on_action(Next);
      case VK_NEXT: return sections() && n.on_action(SectionNext);
      case VK_PRIOR: return sections() && n.on_action(SectionPrev);
      case 'F':
        if (!status_blocked()) return false;
        reread();
        return true;
      // The game's D-pad keys would move its cursor behind the navigator's back (wrapping at the ends).
      case 'W': case 'A': case 'S': case 'D': return true;
      case 'Q': case 'E':
        if (!bumpers()) return false;
        bumper(vk == 'E' ? 1 : -1);
        return true;
    }
  }
  if (mods == keys::Shift && vk == VK_TAB) {
    if (sections() && !tab_is_game()) return n.on_action(SectionPrev);
    return several_stops() && n.on_action(Prev);
  }
  return false;
}

// The D-pad and A while a menu has focus: the game never sees them (input::set_swallow); each press, and a held
// direction's repeats, go to the navigator. A press the navigator does not take (left/right in a plain list) is
// pressed for the game instead.
uint16_t g_pad_held = 0;
ULONGLONG g_pad_next = 0;
constexpr uint16_t kPadMask = XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT |
                              XINPUT_GAMEPAD_DPAD_RIGHT | XINPUT_GAMEPAD_A;
void pad_button(uint16_t b) {
  using namespace ui::ui_actions;
  if (!g_active->takes_input()) return;   // like on_key: dropped, as the game drops it
  g_device = 1;
  player_moved();
  auto& n = nav();
  bool taken = true;
  switch (b) {
    case XINPUT_GAMEPAD_DPAD_UP: n.on_action(Up); break;
    case XINPUT_GAMEPAD_DPAD_DOWN: n.on_action(Down); break;
    case XINPUT_GAMEPAD_DPAD_LEFT: taken = n.on_action(Left) || g_active->keeps_edges(); break;
    case XINPUT_GAMEPAD_DPAD_RIGHT: taken = n.on_action(Right) || g_active->keeps_edges(); break;
    case XINPUT_GAMEPAD_A: taken = activatable() && n.on_action(Activate); break;
    case XINPUT_GAMEPAD_LEFT_SHOULDER: bumper(-1); break;
    case XINPUT_GAMEPAD_RIGHT_SHOULDER: bumper(1); break;
    case XINPUT_GAMEPAD_X: reread(); break;
  }
  if (!taken) input::press(b);
}
void pad_tick(bool on) {
  static bool swallowing = false;
  // A trigger press jumps a page's sections (input.cpp: fired once the other trigger has had the trigger delay to follow for the
  // mod layer; the game does not see it).
  bool taps = on && sections() && !triggers_are_game();
  input::claim_trigger_taps(taps);
  if (!on) {
    if (swallowing) { input::set_swallow(0); swallowing = false; }
    g_pad_held = 0;
    return;
  }
  if (int t = input::take_trigger_tap(); t && taps && !input::layer_open()) {
    player_moved();
    nav().on_action(t > 0 ? ui::ui_actions::SectionNext : ui::ui_actions::SectionPrev);
  }
  // LB / RB too on a screen that takes the bumpers (the camp's Persona page); X where the status page is blocked
  // (pad_button: a press with no case is taken, the game never sees it).
  uint16_t mask = kPadMask | (bumpers() ? XINPUT_GAMEPAD_LEFT_SHOULDER | XINPUT_GAMEPAD_RIGHT_SHOULDER : 0) |
                  (status_blocked() ? XINPUT_GAMEPAD_X : 0);
  input::set_swallow(mask);
  uint16_t now = input::layer_open() ? 0 : (input::real_buttons() & mask);
  // Buttons already down when the menu takes the pad are the press before it (the A that closed the message a choice
  // list opened under was taken as choosing its first answer, 2026-10-31): only a fresh press counts.
  static uint16_t stale = 0;   // held since before: ignored until released
  bool fresh = !swallowing;    // the menu takes the pad this frame
  if (!swallowing) stale = now;
  stale &= now;
  now &= ~stale;
  swallowing = true;
  ULONGLONG t = GetTickCount64();
  uint16_t pressed = now & ~g_pad_held;
  // B goes to the game untouched; its press (like A's) arms the interrupting answer. A B already down when the menu
  // took the pad (the cancel that closed a submenu into it) is not a press.
  static bool b_held = false;
  bool b = !input::layer_open() && (input::real_buttons() & XINPUT_GAMEPAD_B);
  bool b_pressed = b && !b_held && !fresh;
  b_held = b;
  if (pressed) {
    g_pad_held = now;
    for (uint16_t bit = 1; bit; bit <<= 1)
      if (pressed & bit) pad_button(bit);
    g_pad_next = t + 450;
  }
  if (b_pressed || (pressed & XINPUT_GAMEPAD_A)) armed_press();   // after pad_button: a move clears it
  if (pressed) return;
  g_pad_held = now;
  uint16_t dirs = now & (kPadMask & ~XINPUT_GAMEPAD_A);
  if (dirs && t >= g_pad_next) {
    g_pad_next = t + 130;
    for (uint16_t bit = 1; bit; bit <<= 1)
      if (dirs & bit) { g_held_repeat = true; pad_button(bit); g_held_repeat = false; break; }
  }
}

// "list#3" -> ("list", 3)
bool parse_slot(const ui::ControlId& id, std::string& key, int& i) {
  const ui::Key& k = id.structural_key();
  if (!k.is_string()) return false;
  const std::string& s = k.text();
  size_t at = s.rfind('#');
  if (at == std::string::npos || at + 1 >= s.size()) return false;
  key = s.substr(0, at);
  i = 0;
  for (size_t j = at + 1; j < s.size(); ++j) {
    if (s[j] < '0' || s[j] > '9') return false;
    i = i * 10 + (s[j] - '0');
  }
  return true;
}
}  // namespace

struct MenuHost {
  static bool ready(Mirror& m) {
    bool r = true;
    guarded([&] {
      if (m.ready) r = m.ready();
      else if (m.widget) r = widget_idle(m.widget());
    });
    return r;
  }
  // Follow the game, then drive it: a cursor the game moved by itself (not toward our target) moves the focus; the
  // focused item of a mirrored stop pulls the game's cursor to it.
  static void sync(MenuScreen& s) {
    std::string fkey;
    int fi = -1;
    auto fid = nav().focused_id();
    if (fid) parse_slot(*fid, fkey, fi);
    for (auto& mp : s.mirrors_) {
      Mirror& m = *mp;
      int c = -1;
      if (m.cursor) guarded([&] { c = m.cursor(); });
      // A new list under this mirror (a category, a member, a Persona switched in the game): the game put its cursor
      // where it wants it; the focus goes there once the cursor is up, not the other way round -- unless the player
      // moves first (player_moved: stop_following clears it).
      if (m.key != m.last_key) {
        m.follow = !m.last_key.empty();
        m.last_key = m.key;
        m.target = -1;
        m.step_from = -1;
        m.seen = -1;
      }
      if (c < 0) { m.seen = -1; m.target = -1; continue; }
      if (m.follow) {
        m.follow = false;
        m.seen = c;
        if (!(fkey == m.key && fi == c)) nav().focus_node(MenuScreen::slot(m.key, c), !s.entry_due_, true);
        continue;
      }
      if (c != m.seen) {
        bool ours = m.target >= 0;
        if (ours && c == m.target) m.target = -1;
        else if (ours && m.step_from >= 0 && !m.count && !m.grid && ((m.target - c > 0) != (m.target - m.step_from > 0))) {
          log::writef("menu: {} overshot to {} (wanted {})", m.key, c, m.target);   // the game skips that item
          m.target = -1;
          ours = false;
        }
        if (!ours && m.seen >= 0 && !(fkey == m.key && fi == c))
          nav().focus_node(MenuScreen::slot(m.key, c), !s.entry_due_, true);
        if (!ours && m.seen < 0 && fkey != m.key && !fid) nav().focus_node(MenuScreen::slot(m.key, c), true, false);
        m.seen = c;
        m.step_from = -1;
      }
      if (fkey != m.key || fi < 0) continue;
      if (fi == c) { m.target = -1; continue; }
      if (m.target != fi) { m.target = fi; m.steps = 0; m.step_at = 0; }
      // A move made just before the game stopped taking them (takes_input drops any made since): it waits. A key sent
      // now would be dropped; a write could be undone by the opening.
      if (!ready(m)) { m.step_from = -1; continue; }
      // A list written directly is never stepped with keys (a write that cannot be done now is tried next tick). The
      // cursor some mirrors read follows the write a frame later (a draw's copy): the same write is made again (its
      // sound twice) only once the game has run two frames on it with the cursor unmoved.
      if (m.set) {
        if (m.step_from == c && g_frame - m.step_at < 2) continue;
        bool done = false;
        guarded([&] { done = m.set(fi); });
        if (done) { m.step_from = c; m.step_at = g_frame; }
        continue;
      }
      // One step at a time: the next once the last registered. A step still being delivered (its key down, the pad
      // queue busy) waits; one delivered in full that the game then ran two frames on without moving was dropped (an
      // animation the ready check missed): pressed again.
      if (m.step_from == c) {
        if (input::injected_held() || input::busy()) { m.step_at = g_frame; continue; }
        if (g_frame - m.step_at < 2) continue;
      }
      if (++m.steps > 40) { log::writef("menu: {} gave up stepping to {}", m.key, fi); m.target = -1; continue; }
      int d = fi > c ? 1 : -1;
      bool horizontal = m.horizontal;
      if (m.grid) {   // the row first, then the column
        auto [r0, c0] = m.grid(c);
        auto [r1, c1] = m.grid(fi);
        if (r0 != r1) { horizontal = false; d = r1 > r0 ? 1 : -1; }
        else { horizontal = true; d = c1 > c0 ? 1 : -1; }
      } else if (m.count > 0) {
        int fwd = ((fi - c) % m.count + m.count) % m.count;
        d = fwd <= m.count - fwd ? 1 : -1;
      }
      if (m.reversed) d = -d;
      uint32_t vk = horizontal ? (d > 0 ? VK_RIGHT : VK_LEFT) : (d > 0 ? VK_DOWN : VK_UP);
      uint16_t b = horizontal ? (d > 0 ? XINPUT_GAMEPAD_DPAD_RIGHT : XINPUT_GAMEPAD_DPAD_LEFT)
                              : (d > 0 ? XINPUT_GAMEPAD_DPAD_DOWN : XINPUT_GAMEPAD_DPAD_UP);
      MenuScreen::press_game(vk, b);
      m.step_from = c;
      m.step_at = g_frame;
    }
    // A confirm waiting for the game's cursor.
    if (s.confirm_mirror_ && s.pending_confirm_ >= 0) {
      Mirror& m = *s.confirm_mirror_;
      // The list went away, or the player moved since: the press is dropped, not left armed.
      if (m.seen < 0 || g_moves != s.confirm_moves_) {
        if (m.seen >= 0) log::writef("menu: confirm {}#{} dropped, the game's cursor is on {}", m.key, s.pending_confirm_, m.seen);
        s.confirm_mirror_ = nullptr; s.pending_confirm_ = -1;
      }
      // ...and for the list to take input (an animation that began after the press).
      else if (m.seen == s.pending_confirm_ && ready(m)) {
        log::writef("menu: confirm {}#{}", m.key, s.pending_confirm_);
        MenuScreen::press_game(m.confirm_vk, XINPUT_GAMEPAD_A);
        s.confirm_mirror_ = nullptr;
        s.pending_confirm_ = -1;
      }
    }
  }
  // The player's move landed on `id`. The game ticks when its own cursor moves, a list still opening once it takes the
  // write; a move that moves no game cursor (a line of the mod's, or a game row the cursor already sits on: a Social
  // Link page's memo and its first person, 2026-10-31) gets the game's cursor sound from here.
  static bool game_will_tick(MenuScreen& s, const ui::ControlId& id) {
    std::string key;
    int i = -1;
    if (!parse_slot(id, key, i)) return false;
    Mirror* m = s.mirror_of(key);
    return m && m->drives_game() && m->seen != i;
  }
};

namespace {
void move_sound() {
  auto id = nav().focused_id();
  if (!g_active || !id) return;
  if (g_move_from && *g_move_from == *id) return;   // an edge: nothing moved
  if (MenuHost::game_will_tick(*g_active, *id)) return;
  cursor_sound();
}
}  // namespace

ui::ControlId MenuScreen::slot(const std::string& key, int i) { return ui::ControlId::structural(std::format("{}#{}", key, i)); }

ui::NodeVtablePtr MenuScreen::item(std::function<std::string()> label, std::function<void()> activate,
                                   std::function<std::string()> details) {
  auto v = std::make_shared<ui::NodeVtable>();
  v->announcements = {ui::NodeAnnouncement(std::move(label), false, ui::announcement_kinds::kLabel)};
  v->on_activate = std::move(activate);
  v->details = std::move(details);
  return v;
}

void MenuScreen::describe_with(const ui::NodeVtablePtr& v, std::function<std::string()> text) {
  v->announcements.push_back(ui::NodeAnnouncement(std::move(text), true, ui::announcement_kinds::kTooltip));
}

void MenuScreen::buffer_with(const ui::NodeVtablePtr& v, std::function<std::string()> text) {
  if (!v->details) v->details = text;
  // The row as it reads now (its label and whatever was declared before this call: a value, a mark), on one line.
  auto parts = v->announcements;
  v->detail_lines = [parts, text = std::move(text)] {
    std::string row;
    for (auto& p : parts)
      if (std::string s = p.text ? p.text() : std::string(); !s.empty()) row += (row.empty() ? "" : ", ") + s;
    std::vector<std::string> out;
    if (!row.empty()) out.push_back(std::move(row));
    if (std::string t = text(); !t.empty()) out.push_back(std::move(t));
    return out;
  };
}

bool MenuScreen::has_details() {
  const ui::GraphNode* n = g_active == this ? nav().focused_node() : nullptr;
  return n && n->vtable && n->vtable->detail_lines;
}
std::vector<std::string> MenuScreen::detail_lines() {
  const ui::GraphNode* n = g_active == this ? nav().focused_node() : nullptr;
  return n && n->vtable && n->vtable->detail_lines ? n->vtable->detail_lines() : std::vector<std::string>();
}

void MenuScreen::add_walk(ui::GraphBuilder& b, const std::string& key, int i, int n, const std::vector<std::string>& lines) {
  ui::ControlId prev = slot(key, i);
  for (size_t j = 0; j < lines.size(); ++j) {
    auto id = ui::ControlId::structural(std::format("{}#{}.{}", key, i, j));
    std::string l = lines[j];
    b.add_node(id, item([l] { return l; }));
    b.connect(prev, ui::GraphDir::Right, id);
    b.connect(id, ui::GraphDir::Left, prev);
    // Up always set: a walk node without one is stitched under the list's last row (graph_builder), and the walk's
    // Down back into the list made End loop forever (the game froze, 2026-10-31).
    b.connect(id, ui::GraphDir::Up, slot(key, i > 0 ? i - 1 : i));
    if (i + 1 < n) b.connect(id, ui::GraphDir::Down, slot(key, i + 1));
    prev = id;
  }
}

ui::NodeVtablePtr MenuScreen::game_item(Mirror& m, int i, std::function<std::string()> label, std::function<std::string()> details) {
  Mirror* mp = &m;
  return item(std::move(label), m.confirm_vk ? std::function<void()>([this, mp, i] { confirm(*mp, i); }) : std::function<void()>(),
              std::move(details));
}

Mirror& MenuScreen::mirror(std::string key, std::function<int()> cursor) {
  for (auto& m : mirrors_) if (m->key == key) { m->cursor = std::move(cursor); return *m; }
  auto m = std::make_unique<Mirror>();
  m->key = std::move(key);
  m->cursor = std::move(cursor);
  mirrors_.push_back(std::move(m));
  return *mirrors_.back();
}
Mirror* MenuScreen::mirror_of(const std::string& key) {
  for (auto& m : mirrors_) if (m->key == key) return m.get();
  return nullptr;
}

void MenuScreen::confirm(Mirror& m, int i) {
  pending_confirm_ = i;
  confirm_mirror_ = &m;
  confirm_moves_ = g_moves;   // the move count when it was pressed: a later move drops it
  MenuHost::sync(*this);   // at once when the cursor is already there
}

// The focused list is up and the game takes input on it. An item of no game list (a page's line, the mod's own row) has
// nothing to wait for. A mirror that drives nothing (Done on the contract) is not a game list either.
bool MenuScreen::takes_input() {
  auto id = focused();
  std::string key;
  int i;
  Mirror* m = id && parse_slot(*id, key, i) ? mirror_of(key) : nullptr;
  if (!m || !m->drives_game()) return true;
  int c = -1;
  if (m->cursor) guarded([&] { c = m->cursor(); });
  return c >= 0 && MenuHost::ready(*m);
}

// A list whose rows are not read yet (no focus to ask about) is not ready to open.
bool MenuScreen::entry_ready() { return (focused() || mirrors_.empty()) && takes_input(); }

void MenuScreen::say_entry() {
  entry_due_ = false;
  if (!entry_title_.empty()) {
    bool cut = true;
    guarded([&] { cut = opening_interrupts(); });
    say_line(entry_title_, cut);
    g_entry_line = false;
    entry_title_.clear();
    g_press_armed = false;   // the title answers the press: the landing queues after it (it cut "Skills" off, 2026-11-05)
  }
  if (entry_say_) nav().announce_current();
}

void MenuScreen::focus(const ui::ControlId& id, bool interrupt) { if (g_active == this) nav().focus_node(id, true, interrupt); }
void MenuScreen::focus_quietly(const ui::ControlId& id) { if (g_active == this) nav().focus_node(id, false); }
void MenuScreen::say(std::string_view text, bool interrupt) { say_line(text, interrupt); }
void MenuScreen::say_focus() { if (g_active == this) { nav().ensure_focus(); g_entry_line = true; nav().announce_current(); } }
int MenuScreen::focused_row(const std::string& key) const {
  auto id = g_active == this ? nav().remembered_focus() : std::nullopt;   // build() runs with no render to ask
  if (!id || !id->structural_key().is_string()) return -1;
  const std::string& s = id->structural_key().text();
  if (s.size() <= key.size() + 1 || s.compare(0, key.size(), key) != 0 || s[key.size()] != '#') return -1;
  int i = 0;
  size_t j = key.size() + 1;
  for (; j < s.size() && s[j] >= '0' && s[j] <= '9'; ++j) i = i * 10 + (s[j] - '0');
  return j > key.size() + 1 && (j == s.size() || s[j] == '.') ? i : -1;
}

std::optional<ui::ControlId> MenuScreen::focused() const { return g_active == this ? nav().focused_id() : std::nullopt; }

std::string MenuScreen::detail_key() const {
  auto f = focused();
  return std::format("{}/{}", id(), f && f->structural_key().is_string() ? f->structural_key().text() : std::string());
}

bool MenuScreen::keeps_edges() {
  auto id = focused();
  std::string key;
  int i;
  Mirror* m = id && parse_slot(*id, key, i) ? mirror_of(key) : nullptr;
  return (m && m->keep_edges) || takes_bumpers();   // a bumper page: Left / Right would move the game's list under it
}

bool MenuScreen::edges_to_game() {
  auto id = focused();
  std::string key;
  int i;
  Mirror* m = id && parse_slot(*id, key, i) ? mirror_of(key) : nullptr;
  return m && !m->keep_edges;
}

void MenuScreen::press_game(uint32_t vk, uint32_t pad_button) {
  const bool pad = g_device < 0 ? input::pad_in_use() : g_device == 1;
  if (pad && pad_button) input::press(pad_button, 60, 40);
  else input::inject_key(vk);
}

void MenuScreen::enter() {
  for (auto& m : mirrors_) { m->seen = -1; m->target = -1; m->step_from = -1; }
  pending_confirm_ = -1;
  confirm_mirror_ = nullptr;
  guarded([&] { on_enter(); });
  g_active = this;
  // Entered again after a message window or a submenu over it: the focus survives (the navigator keeps the screen's
  // state while it stays registered); the title only once per opening.
  bool every = false;
  guarded([&] { every = title_every_entry(); });
  std::string t = title_due() || every ? title() : std::string();
  nav().attach(this);
  bool say = true;
  guarded([&] { say = announce_entry(); });
  g_entry_line = say;
  nav().settle_silently();
  attached_ = true;
  // The opening is said once the menu takes input: a line heard is a menu ready for its answer.
  entry_title_ = t;
  entry_say_ = say;
  entry_due_ = true;
  if (entry_ready()) say_entry();
}

void MenuScreen::tick() {
  ++g_frame;
  g_active = this;
  guarded([&] { on_tick(); });
  if (!attached_) { nav().attach(this); attached_ = true; }
  if (entry_due_) {
    nav().settle_silently();
    nav().forget_live();   // a description arriving meanwhile is no line before the opening's ("Restores 50 HP" before "Shop")
    if (entry_ready()) say_entry();
  }
  bool keys = true;
  guarded([&] { keys = holds_keys(); });
  if (!keys) return;   // something of the game's is over the menu: no reads, no driving
  nav().ensure_focus();
  MenuHost::sync(*this);
}

void MenuScreen::leave() {
  entry_due_ = false;
  guarded([&] { on_leave(); });
  if (g_active == this) g_active = nullptr;
  attached_ = false;
  // Closed for good (not covered by a submenu or a message): forget the focus, so the next opening starts fresh.
  // screens.cpp decides "closed" by the screen going inactive; a covered screen is still active.
  bool still = false;
  guarded([&] { still = active(); });
  if (!still) {
    if (!keep_state_on_pop()) nav().screen_closed(this);
    nav().attach(nullptr);
  }
}

std::string MenuScreen::describe() {
  if (g_active != this) return {};
  const ui::GraphNode* n = nav().focused_node();
  std::string out = title();
  if (n) {
    if (auto t = ui::GraphAnnouncer::compose_full(n)) out += (out.empty() ? "" : ". ") + *t;
    if (n->vtable && n->vtable->details) {
      std::string d;
      guarded([&] { d = n->vtable->details(); });
      if (!d.empty() && out.find(d) == std::string::npos) out += ". " + d;
    }
  }
  return out;
}

namespace menu {
MenuScreen* active() { return has_focus_now() ? g_active : nullptr; }

void frame() {
  // The menu ticked this frame is the one with focus; a message window over it hands everything back to the game.
  if (g_active && screens::current() != g_active) g_active = nullptr;
  // The mod's settings menu over it takes the pad through the layer filter, which never sees swallowed buttons.
  pad_tick(has_focus_now() && !input::modal());
  battle_details::frame();
}

std::string dump() {
  if (!g_active) return "no menu\n";
  return std::format("menu {} focused={}\n{}", g_active->id(), has_focus_now() ? "yes" : "no (message window)", nav().dump());
}

void install() {
  ui::GraphAnnouncer::position_text = [](int i, int n) { return std::format("{} {} {}", i, strings::kOf, n); };
  keys::set_menu(on_key);
  battle_details::install();
  dev::route("/menu", [](const dev::Request&, int&) { return dev::on_game([] { return dump(); }); }, "the bespoke menu's graph and focus");
}
}  // namespace menu
}  // namespace p3a::screens

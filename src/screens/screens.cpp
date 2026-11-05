#include "screens/screen.h"
#include "screens/menu.h"
#include "hooks.h"
#include "input.h"
#include "log.h"
#include "seh.h"
#include "msg.h"
#include "settings.h"
#include "speech.h"
#include "world.h"
#include "core/strings.h"
#include <algorithm>
#include <cstring>
#include <format>
#include <map>
#include <set>

namespace p3a::screens {
static std::vector<std::unique_ptr<Screen>> g_screens;
static Screen* g_current;

void add(std::unique_ptr<Screen> s) { g_screens.push_back(std::move(s)); }
Screen* current() { return g_current; }
static Screen* g_keys_screen;
static std::function<std::string()> g_keys;
static std::string g_hint_due;   // an entry hint to speak once this tick's lines are out
void set_keys(const strings::Hint& h) { set_keys([&h] { return std::string(h()); }); }
void set_keys(std::function<std::string()> keys) {
  g_keys_screen = g_current;
  g_keys = std::move(keys);
  // The option to hear them anyway, once per screen and set of keys: queued at the end of the tick, after the line
  // the screen speaks next (screens name their keys just before speaking, and that line interrupts).
  static Screen* said_screen;
  static std::string said;
  if (!settings::on(settings::EntryHints)) return;
  std::string k = g_keys();
  if (g_current == said_screen && k == said) return;
  said_screen = g_current; said = k;
  g_hint_due = k;
}
std::string keys() { return g_current && g_current == g_keys_screen && g_keys ? g_keys() : std::string(); }
// Screens whose title was spoken in this opening: coming back to one (from a submenu, a message window, a term read
// over it) gives the focused item alone. Cleared once the screen is gone (inactive with no message window up).
static std::map<Screen*, std::set<std::string>> g_titled;
bool title_due(const char* step) {
  return g_current && g_titled[g_current].insert(step).second;
}
static bool g_any_screen;
// A save is loading from the boot (the game fills in the last save's money, stats and links a frame or two before the
// title appears: 21 "New Social Link" lines at startup, 2026-10-28) and whenever the title or the save list is up,
// until the hero is in the field again. A hot reload in the field finds the hero at once.
static bool g_loading = true;
bool loading() { return g_loading; }
static void track_loading() {
  bool listing = g_current && (!strcmp(g_current->id(), "title") || !strcmp(g_current->id(), "title_menu") ||
                               !strcmp(g_current->id(), "save_load"));
  if (listing) g_loading = true;
  else if (g_loading && world::player()) g_loading = false;
}

// The cost of each screen's active() (every tick, current or not) and tick() (while current), for /perf.
struct Cost { uint64_t total_us = 0, max_us = 0, n = 0; };
struct ScreenCost { Cost active, tick; };
static std::map<Screen*, ScreenCost> g_perf;
struct Timed {
  Cost& c;
  LARGE_INTEGER t0;
  explicit Timed(Cost& c) : c(c) { QueryPerformanceCounter(&t0); }
  ~Timed() {
    static LARGE_INTEGER qpf = [] { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
    LARGE_INTEGER t1;
    QueryPerformanceCounter(&t1);
    uint64_t us = (uint64_t)((t1.QuadPart - t0.QuadPart) * 1000000 / qpf.QuadPart);
    c.total_us += us; ++c.n;
    if (us > c.max_us) c.max_us = us;
  }
};
std::string perf(bool reset) {
  std::string out = "screen      active avg/max us   tick avg/max us (ticks while current)\n";
  for (auto& s : g_screens) {
    ScreenCost& p = g_perf[s.get()];
    out += std::format("{:<16} {:>4} {:>6}   {:>5} {:>6} ({})\n", s->id(), p.active.n ? p.active.total_us / p.active.n : 0, p.active.max_us,
                       p.tick.n ? p.tick.total_us / p.tick.n : 0, p.tick.max_us, p.tick.n);
  }
  if (reset) g_perf.clear();
  return out;
}

void tick() {
  // A message window (system notice, dialogue, a choice) is modal over whatever screen is under it: the screen goes
  // quiet, and when the window closes the screen is entered again so the player hears where they are.
  // Back-to-back notices leave a gap of a few frames between windows, in which the message session stays open
  // (msg::session_open): the screen comes back once it has closed, or at once when the player presses a key or a pad
  // button after the window closed (moving through the menu after cancelling a prompt went unread, 2026-10-28).
  for (auto& s : g_screens) if (!guarded([&] { s->background(); })) log::writef("screens: {} faulted in background()", s->id());
  bool modal = msg::current().blocking;
  static bool was_modal = false;
  static ULONGLONG modal_end = 0;
  if (modal) {
    if (!was_modal) log::write("screens: modal on");
    was_modal = true; modal_end = 0;
    // A screen that opens over the window (the log, B, in a conversation: it went unread, 2026-10-29) runs anyway.
    Screen* over = nullptr;
    for (auto& s : g_screens) {
      if (!s->over_messages()) continue;
      bool on = false;
      if (guarded([&] { on = s->active(); }) && on && (!over || s->priority() > over->priority())) over = s.get();
    }
    if (over != g_current && (over || (g_current && g_current->over_messages()))) {
      if (g_current) guarded([&] { g_current->leave(); });
      log::writef("screen: {} -> {} (over a message)", g_current ? g_current->id() : "none", over ? over->id() : "none");
      g_current = over;
      if (g_current) guarded([&] { g_current->enter(); });
    }
    if (over) { Timed t(g_perf[over].tick); if (!guarded([&] { over->tick(); })) log::writef("screens: {} faulted in tick()", over->id()); }
    if (!g_hint_due.empty()) { speech::speak(g_hint_due, false); g_hint_due.clear(); }
    menu::frame();
    return;
  }
  Screen* best = nullptr;
  std::vector<Screen*> gone;   // inactive screens whose title was spoken
  for (auto& s : g_screens) {
    bool on = false;
    Timed t(g_perf[s.get()].active);
    if (!guarded([&] { on = s->active(); })) { log::writef("screens: {} faulted in active()", s->id()); continue; }
    if (on && (!best || s->priority() > best->priority())) best = s.get();
    else if (!on && g_titled.count(s.get())) gone.push_back(s.get());
  }
  // A screen left for a message or for one drawn over it (a submenu) is still open underneath; otherwise it closed.
  for (Screen* s : gone)
    if (!msg::current().open && !(best && best->priority() > s->priority())) g_titled.erase(s);
  if (was_modal) {
    if (!modal_end) { modal_end = GetTickCount64(); log::write("screens: modal off"); }
    bool own = false;
    if (g_current) guarded([&] { own = g_current->own_messages(); });
    // Another window of the same notice or conversation is coming while its message session is open; a key or a pad
    // button pressed since the window closed brings the screen back at once. So does another screen coming up: the
    // script went on to something else (a boss's talk leaves its proc "between windows" through the whole battle, so
    // the wheel never came up, 2026-11-03).
    bool pressed = std::max(hooks::key_ms(), input::pad_ms()) > modal_end;
    if (!own && !pressed && best == g_current && msg::session_open()) { menu::frame(); return; }
    was_modal = false;
    log::writef("screens: re-enter after {} ms", GetTickCount64() - modal_end);
    if (g_current && best == g_current) guarded([&] { g_current->enter(); });
  }
  if (best != g_current) {
    if (g_current) guarded([&] { g_current->leave(); });
    log::writef("screen: {} -> {}", g_current ? g_current->id() : "none", best ? best->id() : "none");
    g_current = best;
    if (g_current) g_any_screen = true;
    if (g_current) guarded([&] { g_current->enter(); });
  }
  track_loading();
  if (g_current) {
    Timed t(g_perf[g_current].tick);
    if (!guarded([&] { g_current->tick(); })) log::writef("screens: {} faulted in tick()", g_current->id());
  }
  if (!g_hint_due.empty()) { speech::speak(g_hint_due, false); g_hint_due.clear(); }
  menu::frame();
}

std::string status() { return std::format("screen={} ({} modelled)", g_current ? g_current->id() : "none", g_screens.size()); }
}  // namespace p3a::screens

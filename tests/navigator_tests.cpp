// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
// GraphNavigator through a real Screen: edges of a list repeat the focused item instead of going silent.
#include "doctest.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "core/graph_builder.h"
#include "core/navigator.h"
#include "core/ui_screen.h"

using namespace p3a::ui;

namespace {
ControlId id(const char* s) { return ControlId::structural(s); }
std::shared_ptr<NodeVtable> vt(std::string label) {
  auto v = std::make_shared<NodeVtable>();
  v->announcements = {NodeAnnouncement([label] { return label; }, false, announcement_kinds::kLabel)};
  return v;
}

struct ListScreen : Screen {
  bool row = false;
  std::string_view key() const override { return "list"; }
  bool is_active() override { return true; }
  std::string screen_name() const override { return "list"; }
  void build(GraphBuilder& b) override {
    b.begin_stop("items");
    if (row) b.start_row("items");
    b.add_item(id("a"), vt("a"));
    b.add_item(id("b"), vt("b"));
    b.add_item(id("c"), vt("c"));
    if (row) b.end_row();
  }
};

struct Harness {
  std::vector<std::string> spoken;
  int frame = 0;
  ListScreen screen;
  GraphNavigator nav{NavigatorHost{[this](std::string_view t, bool) { spoken.emplace_back(t); }, {}, [] { return true; }, [this] { return frame; }}};
  Harness() { nav.attach(&screen); nav.ensure_focus(); }
  std::string act(std::string_view key) { spoken.clear(); nav.on_action(key); return spoken.empty() ? std::string() : spoken.back(); }
};
}  // namespace

TEST_CASE("vertical list: Up at the top and Down at the bottom repeat the item; Left/Right say nothing") {
  Harness h;
  CHECK(h.spoken.back().find("a") != std::string::npos);
  CHECK(h.act(ui_actions::Up) == "a");        // already at the top: repeated
  CHECK(h.act(ui_actions::Down) == "b");
  CHECK(h.act(ui_actions::Down) == "c");
  CHECK(h.act(ui_actions::Down) == "c");      // bottom: repeated
  CHECK(h.act(ui_actions::Left) == "");       // not this list's axis
  CHECK(h.act(ui_actions::Right) == "");
  CHECK(h.act(ui_actions::End) == "c");       // Home/End at the edge repeat too
  CHECK(h.act(ui_actions::Home) == "a");
  CHECK(h.act(ui_actions::Home) == "a");
}

TEST_CASE("row: Left/Right repeat at the ends, Up/Down say nothing") {
  Harness h;
  h.screen.row = true;
  h.nav.attach(&h.screen);   // content changed: rebuild as a row
  h.frame += 100; h.nav.ensure_focus();
  CHECK(h.act(ui_actions::Left) == "a");
  CHECK(h.act(ui_actions::Right) == "b");
  CHECK(h.act(ui_actions::Right) == "c");
  CHECK(h.act(ui_actions::Right) == "c");
  CHECK(h.act(ui_actions::Up) == "");
  CHECK(h.act(ui_actions::Down) == "");
}

// The search scope must not hold GraphNode* from the render the letter was typed in: every arrow through the
// results rerenders (KeyGraph::focus), so by the second arrow such pointers would be freed arena memory. The scope
// is kept by ControlId.
namespace {
struct WordsScreen : Screen {
  int builds = 0;
  std::string_view key() const override { return "words"; }
  bool is_active() override { return true; }
  std::string screen_name() const override { return "words"; }
  void build(GraphBuilder& b) override {
    ++builds;
    b.begin_stop("items");
    b.add_item(id("apple"), vt("apple"));
    b.add_item(id("banana"), vt("banana"));
    b.add_item(id("avocado"), vt("avocado"));
    b.add_item(id("apricot"), vt("apricot"));
  }
};
}  // namespace

TEST_CASE("type-ahead results survive rerenders between the typing and the arrows") {
  std::vector<std::string> spoken;
  int frame = 0;
  WordsScreen screen;
  GraphNavigator nav{NavigatorHost{[&](std::string_view t, bool) { spoken.emplace_back(t); }, {}, [] { return true; }, [&] { return frame; }}};
  nav.attach(&screen); nav.ensure_focus();
  auto tick = [&](std::u16string_view typed, bool down) {
    TypeaheadInput in; in.typed = typed; in.down_held = down; in.dt = 0.016; ++frame;
    spoken.clear(); nav.tick_typeahead(in);
    return spoken.empty() ? std::string() : spoken.back();
  };
  tick(u"", false);                        // the first tick only registers the screen for type-ahead
  CHECK(tick(u"a", false) == "apple");
  // Many rerenders before the first arrow: the render the scope was built from is long retired.
  for (int i = 0; i < 5; ++i) nav.announce_current();
  CHECK(tick(u"", true) == "avocado");
  CHECK(tick(u"", false) == "");           // arrow released: no step
  for (int i = 0; i < 5; ++i) nav.announce_current();
  CHECK(tick(u"", true) == "apricot");
  CHECK(tick(u"", false) == "");
  CHECK(tick(u"", true) == "banana");      // the substring tier (the a in banana) ranks last
  CHECK(tick(u"", false) == "");
  CHECK(tick(u"", true) == "apple");       // and the results wrap
  CHECK(screen.builds > 10);
}

namespace {
// Two options under a title, as the mod's settings menu declares its rows: a value Left / Right change, and a
// switch Enter flips. Each says its state at once (state_text) and carries its value as a live part; the first has a
// description too (a live tooltip part).
struct OptionScreen : Screen {
  int volume = 5;
  bool muted = false;
  std::string help;
  std::string_view key() const override { return "options"; }
  bool is_active() override { return true; }
  std::string screen_name() const override { return "options"; }
  void build(GraphBuilder& b) override {
    b.begin_stop("options");
    b.push_context("Sound");
    auto v = std::make_shared<NodeVtable>();
    v->announcements = {NodeAnnouncement([] { return std::string("Volume"); }, false, announcement_kinds::kLabel),
                        NodeAnnouncement([this] { return std::to_string(volume); }, true, announcement_kinds::kValue),
                        NodeAnnouncement([this] { return help; }, true, announcement_kinds::kTooltip)};
    v->on_adjust = [this](int sign, bool) { volume += sign; };
    v->state_text = [this] { return std::to_string(volume); };
    b.add_item(id("volume"), v);
    auto m = std::make_shared<NodeVtable>();
    m->announcements = {NodeAnnouncement([] { return std::string("Mute"); }, false, announcement_kinds::kLabel),
                        NodeAnnouncement([this] { return std::string(muted ? "on" : "off"); }, true, announcement_kinds::kValue)};
    m->on_activate = [this] { muted = !muted; };
    m->state_text = [this] { return std::string(muted ? "on" : "off"); };
    b.add_item(id("mute"), m);
    b.pop_context();
  }
};

struct Said {
  std::string text;
  bool interrupt;
};

// A host that keeps what was said with its interrupt flag, counts the landing sound, and can switch focus mode off
// or answer the edges itself.
struct OptionHarness {
  std::vector<Said> said;
  int hovers = 0;
  int frame = 0;
  bool mode = true;
  bool answered = false;
  OptionScreen screen;
  GraphNavigator nav{NavigatorHost{[this](std::string_view t, bool interrupt) { said.push_back({std::string(t), interrupt}); },
                                   [this] { ++hovers; }, [this] { return mode; }, [this] { return frame; },
                                   [this](GraphDir) { return answered; }}};
  OptionHarness() { nav.attach(&screen); nav.ensure_focus(); }
  void pull() { frame += 100; nav.ensure_focus(); }   // a frame's pull, past the idle throttle
};
}  // namespace

TEST_CASE("an entry is queued; a move, a changed value and an edge repeat interrupt") {
  OptionHarness h;
  REQUIRE(h.said.size() == 1);
  CHECK(h.said[0].text == "Sound, Volume, 5");   // the title, then the row
  CHECK_FALSE(h.said[0].interrupt);              // an entry follows what was being said
  CHECK(h.hovers == 0);

  h.said.clear();
  CHECK(h.nav.on_action(ui_actions::Right));     // the value changes: no move
  REQUIRE(h.said.size() == 1);
  CHECK(h.said[0].text == "6");
  CHECK(h.said[0].interrupt);
  CHECK(h.hovers == 0);
  h.pull();
  CHECK(h.said.size() == 1);                     // and the live watch does not say it a second time

  h.said.clear();
  CHECK(h.nav.on_action(ui_actions::Down));
  REQUIRE(h.said.size() == 1);
  CHECK(h.said[0].text == "Mute, off");          // a sibling: no title again
  CHECK(h.said[0].interrupt);
  CHECK(h.hovers == 1);

  h.said.clear();
  CHECK(h.nav.on_action(ui_actions::Activate));
  REQUIRE(h.said.size() == 1);
  CHECK(h.said[0].text == "on");
  CHECK(h.said[0].interrupt);
  h.pull();
  CHECK(h.said.size() == 1);

  h.said.clear();
  CHECK_FALSE(h.nav.on_action(ui_actions::Down));   // the last row: said again, with the landing sound, not taken
  REQUIRE(h.said.size() == 1);
  CHECK(h.said[0].text == "Mute, on");
  CHECK(h.said[0].interrupt);
  CHECK(h.hovers == 2);

  h.answered = true;                             // the host hands the edge to something that answers it
  h.said.clear();
  CHECK_FALSE(h.nav.on_action(ui_actions::Down));
  CHECK(h.said.empty());
  CHECK(h.hovers == 2);
}

TEST_CASE("a live part that changes by itself is said alone: a value at once, a description queued") {
  OptionHarness h;
  h.pull();
  h.said.clear();
  h.screen.volume = 9;                           // the game changed it, not the player
  h.pull();
  REQUIRE(h.said.size() == 1);
  CHECK(h.said[0].text == "9");
  CHECK(h.said[0].interrupt);

  h.said.clear();
  h.screen.help = "How loud.";                   // a description that arrives late
  h.pull();
  REQUIRE(h.said.size() == 1);
  CHECK(h.said[0].text == "How loud.");
  CHECK_FALSE(h.said[0].interrupt);
  h.pull();
  CHECK(h.said.size() == 1);                     // once

  h.said.clear();
  h.nav.forget_live();                           // the next values are a fresh baseline
  h.screen.volume = 3;
  h.pull();
  CHECK(h.said.empty());
}

TEST_CASE("a focus the screen asks for is read as a move, interrupts on request, or stays silent") {
  OptionHarness h;
  h.said.clear();
  h.nav.focus_node(id("mute"));
  h.pull();
  REQUIRE(h.said.size() == 1);
  CHECK(h.said[0].text == "Mute, off");
  CHECK_FALSE(h.said[0].interrupt);
  CHECK(h.hovers == 0);                          // not a player's landing: no sound

  h.said.clear();
  h.nav.focus_node(id("volume"), true, true);    // the game moved its own cursor
  h.pull();
  REQUIRE(h.said.size() == 1);
  CHECK(h.said[0].text == "Volume, 5");
  CHECK(h.said[0].interrupt);

  h.said.clear();
  h.nav.focus_node(id("mute"), false);
  h.pull();
  CHECK(h.said.empty());
  CHECK(h.nav.focused_id() == id("mute"));

  h.mode = false;                                // the mod does not own the keys: landings are seated, not said
  h.nav.focus_node(id("volume"));
  h.pull();
  h.mode = true;
  h.pull();
  CHECK(h.said.empty());
  CHECK(h.nav.focused_id() == id("volume"));

  h.nav.focus_node(id("nowhere"));               // not in the menu: dropped
  h.pull();
  CHECK(h.said.empty());
  CHECK(h.nav.focused_id() == id("volume"));
}

TEST_CASE("a closed screen starts again from its first row; a kept one comes back where it was") {
  OptionHarness h;
  h.nav.on_action(ui_actions::Down);
  h.nav.attach(nullptr);
  h.nav.attach(&h.screen);                       // its state was kept
  h.said.clear();
  h.pull();
  CHECK(h.nav.focused_id() == id("mute"));
  REQUIRE(h.said.size() == 1);
  CHECK(h.said[0].text == "Sound, Mute, off");   // read in full again: a new entry

  h.nav.attach(nullptr);
  h.nav.screen_closed(&h.screen);                // its state is dropped
  h.nav.attach(&h.screen);
  h.nav.settle_silently();                       // the host says the title itself, then the focus
  h.said.clear();
  h.pull();
  CHECK(h.said.empty());
  CHECK(h.nav.focused_id() == id("volume"));
  h.nav.announce_current();
  REQUIRE(h.said.size() == 1);
  CHECK(h.said[0].text == "Sound, Volume, 5");
  CHECK_FALSE(h.said[0].interrupt);
}

namespace {
// A tab strip stop (a row), then a page stop in two sections: a header line, then two lines of stats.
struct PagedScreen : Screen {
  bool wraps = false;
  std::string_view key() const override { return "paged"; }
  bool is_active() override { return true; }
  std::string screen_name() const override { return "paged"; }
  bool wrap() const override { return wraps; }
  void build(GraphBuilder& b) override {
    b.begin_stop("tabs");
    b.start_row("tabs");
    b.add_item(id("t1"), vt("t1"));
    b.add_item(id("t2"), vt("t2"));
    b.end_row();
    b.begin_stop("page");
    b.set_region("top");
    b.add_item(id("hdr"), vt("hdr"));
    b.set_region("stats");
    b.add_item(id("s1"), vt("s1"));
    b.add_item(id("s2"), vt("s2"));
  }
};
}  // namespace

TEST_CASE("Tab moves between stops and remembers each one's place; Home/End and the sections stay inside a stop") {
  std::vector<std::string> spoken;
  int frame = 0;
  PagedScreen screen;
  GraphNavigator nav{NavigatorHost{[&](std::string_view t, bool) { spoken.emplace_back(t); }, {}, [] { return true; }, [&] { return frame; }}};
  nav.attach(&screen); nav.ensure_focus();
  auto act = [&](std::string_view k) { spoken.clear(); nav.on_action(k); return spoken.empty() ? std::string() : spoken.back(); };
  CHECK(nav.stop_count() == 2);
  CHECK(nav.section_count() == 0);               // the tab strip has no sections
  spoken.clear();
  CHECK_FALSE(nav.on_action(ui_actions::SectionNext));   // so the key is not taken there
  CHECK(spoken.empty());

  CHECK(act(ui_actions::Next) == "hdr");         // into the page stop
  CHECK(nav.section_count() == 2);
  CHECK(act(ui_actions::End) == "s2");
  CHECK(act(ui_actions::Home) == "hdr");         // the top of the page, never the tab strip
  CHECK(act(ui_actions::SectionNext) == "s1");
  CHECK(act(ui_actions::SectionNext) == "hdr");  // past the last section: the first again
  CHECK(act(ui_actions::SectionPrev) == "s1");

  CHECK(act(ui_actions::Next) == "");            // the last stop, no wrap: taken, nothing said
  CHECK(nav.focused_id() == id("s1"));
  CHECK(act(ui_actions::Prev) == "t1");
  CHECK(act(ui_actions::Prev) == "");            // the first stop, no wrap
  CHECK(act(ui_actions::Next) == "s1");          // back where the page was left, not on its first line

  screen.wraps = true;
  CHECK(act(ui_actions::Next) == "t1");          // past the last stop: the first
  CHECK(act(ui_actions::Prev) == "s1");          // and back round
}

namespace {
// A titled list whose rows remove themselves on Enter (an item sold or used up).
struct VanishScreen : Screen {
  std::vector<std::string> items{"a", "b", "c"};
  std::string_view key() const override { return "vanish"; }
  bool is_active() override { return true; }
  std::string screen_name() const override { return "vanish"; }
  void build(GraphBuilder& b) override {
    b.begin_stop("tabs");
    auto tab = vt("stash"); tab->on_activate = [] {};
    b.add_item(id("tab0"), tab);
    b.begin_stop("mine");
    b.push_context("your items", "list");
    for (const std::string& i : items) {
      auto v = vt(i);
      v->on_activate = [this, i] { items.erase(std::find(items.begin(), items.end(), i)); };
      b.add_item(ControlId::structural(i), v);
    }
    b.pop_context();
  }
};
}  // namespace

TEST_CASE("Enter that removes the focused row lands on the next row and reads it as a sibling, not the title again") {
  std::vector<std::string> spoken;
  int frame = 0;
  VanishScreen screen;
  GraphNavigator nav{NavigatorHost{[&](std::string_view t, bool) { spoken.emplace_back(t); }, {}, [] { return true; }, [&] { return frame; }}};
  nav.attach(&screen); nav.ensure_focus();
  nav.focus_node(id("a"), false); frame += 100; nav.ensure_focus();
  spoken.clear();
  nav.on_action("ui.activate");   // removes "a"
  frame += 100; nav.ensure_focus();      // past the idle-render throttle: the rebuild, focus must land on "b"
  REQUIRE(!spoken.empty());
  CHECK(spoken.back() == "b");           // not "your items, b"
  spoken.clear();
  nav.on_action("ui.activate");   // removes "b"
  frame += 100; nav.ensure_focus();
  REQUIRE(!spoken.empty());
  CHECK(spoken.back() == "c");
}

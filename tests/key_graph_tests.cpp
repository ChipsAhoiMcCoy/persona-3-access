// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
// KeyGraph: moves, stops, regions, tree operations and where focus lands after a rebuild.
#include "doctest.h"

#include <string>
#include <vector>

#include "core/graph_builder.h"
#include "core/key_graph.h"

using p3a::ui::ControlId;
using p3a::ui::GraphBuilder;
using p3a::ui::GraphDir;
using p3a::ui::GraphRenderPtr;
using p3a::ui::GraphState;
using p3a::ui::Key;
using p3a::ui::KeyGraph;
using p3a::ui::MoveResult;
using p3a::ui::NodeAnnouncement;
using p3a::ui::NodeVtable;
using p3a::ui::NodeVtablePtr;

namespace {

NodeVtablePtr vt(std::string label) {
  auto v = std::make_shared<NodeVtable>();
  v->announcements.push_back(NodeAnnouncement::fixed(std::move(label)));
  return v;
}

ControlId id(std::string key) { return ControlId::structural(Key(std::move(key))); }

NodeVtablePtr radio(std::string label, bool selected) {
  auto v = std::make_shared<NodeVtable>();
  v->announcements.push_back(NodeAnnouncement::fixed(std::move(label)));
  v->announcements.push_back(NodeAnnouncement(
      [selected] { return selected ? std::string("selected") : std::string(); }, false,
      p3a::ui::announcement_kinds::kSelected));
  return v;
}

}  // namespace

TEST_CASE("first render lands on start") {
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder().add_item(id("a"), vt("a")).add_item(id("b"), vt("b")).build();
      },
      &state);
  CHECK(g.rerender());
  CHECK(state.cur_key == id("a"));
}

TEST_CASE("move steps and stops at edges") {
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder().add_item(id("a"), vt("a")).add_item(id("b"), vt("b")).build();
      },
      &state);

  MoveResult r = g.move(GraphDir::Down);
  CHECK(r.moved);
  CHECK(r.to->id == id("b"));

  r = g.move(GraphDir::Down);  // at the end
  CHECK_FALSE(r.moved);
  CHECK(r.to->id == id("b"));
  CHECK(r.from == r.to);
}

TEST_CASE("move to edge goes all the way") {
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder()
            .add_item(id("a"), vt("a"))
            .add_item(id("b"), vt("b"))
            .add_item(id("c"), vt("c"))
            .add_item(id("d"), vt("d"))
            .build();
      },
      &state);
  const MoveResult r = g.move_to_edge(GraphDir::Down);
  CHECK(r.moved);
  CHECK(r.to->id == id("d"));
}

TEST_CASE("move to edge stops on a cycle") {
  // A raw line under a list's first row, stitched below its last row, with its Down back into the list: End looped
  // forever (the Velvet Room froze, 2026-10-31).
  GraphState state;
  KeyGraph g(
      [] {
        GraphBuilder b;
        b.add_item(id("a"), vt("a")).add_item(id("b"), vt("b")).add_item(id("c"), vt("c"));
        b.add_node(id("a.0"), vt("a.0"));
        b.connect(id("a"), GraphDir::Right, id("a.0"));
        b.connect(id("a.0"), GraphDir::Down, id("b"));
        return b.build();
      },
      &state);
  const MoveResult r = g.move_to_edge(GraphDir::Down);
  CHECK(r.moved);
}

TEST_CASE("transition label is reported") {
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder()
            .add_node(id("a"), vt("A"))
            .add_node(id("b"), vt("B"))
            .connect(id("a"), GraphDir::Right, id("b"), "lane change")
            .build();
      },
      &state);

  const MoveResult r = g.move(GraphDir::Right);
  CHECK(r.moved);
  CHECK(r.transition_label == "lane change");
}

TEST_CASE("reconcile follows the control's id across rebuilds") {
  GraphState state;
  int generation = 0;
  KeyGraph g(
      [&generation] {
        generation++;  // fresh vtables/nodes each render -- only the structural keys repeat
        return GraphBuilder()
            .add_item(id("a"), vt("A" + std::to_string(generation)))
            .add_item(id("b"), vt("B" + std::to_string(generation)))
            .build();
      },
      &state);

  g.move(GraphDir::Down);
  CHECK(state.cur_key == id("b"));
  CHECK(g.rerender());  // a whole new render
  CHECK(state.cur_key == id("b"));
}

TEST_CASE("reconcile falls back to the nearest survivor") {
  GraphState state;
  std::vector<std::string> items{"a", "b", "c", "d"};
  KeyGraph g(
      [&items] {
        GraphBuilder b;
        for (const std::string& i : items) b.add_item(id(i), vt(i));
        return b.build();
      },
      &state);

  g.move_to_edge(GraphDir::Down);  // on "d"
  items.pop_back();                // remove "d"
  items.pop_back();                // remove "c"
  CHECK(g.rerender());
  CHECK(state.cur_key == id("b"));  // nearest earlier survivor
}

TEST_CASE("suggested move is honored and consumed") {
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder()
            .add_item(id("a"), vt("a"))
            .add_item(id("b"), vt("b"))
            .add_item(id("c"), vt("c"))
            .build();
      },
      &state);
  state.next_suggested_move = id("c");
  CHECK(g.rerender());
  CHECK(state.cur_key == id("c"));
  CHECK_FALSE(state.next_suggested_move.has_value());
}

TEST_CASE("compute order covers all stops") {
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder()
            .add_item(id("a"), vt("A"))
            .begin_stop()
            .add_item(id("b"), vt("B"))
            .begin_stop()
            .add_item(id("c"), vt("C"))
            .build();
      },
      &state);

  CHECK(g.rerender());
  CHECK(state.key_order.size() == 3);  // later stops appended despite no cross-stop edges
  CHECK(state.key_order[0] == id("a"));
  CHECK(state.key_order[2] == id("c"));
  CHECK(g.stops().size() == 3);
}

TEST_CASE("a stop remembers the position focus left it on") {
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder()
            .begin_stop("a")
            .add_item(id("a1"), vt("A1"))
            .add_item(id("a2"), vt("A2"))
            .begin_stop("b")
            .add_item(id("b1"), vt("B1"))
            .add_item(id("b2"), vt("B2"))
            .build();
      },
      &state);
  // What Tab does: the stop's landing, then focus there.
  auto enter = [&g](const char* stop) {
    p3a::ui::GraphNode* landing = g.stop_landing(Key(stop));
    REQUIRE(landing != nullptr);
    const ControlId target = landing->id;
    REQUIRE(g.focus(target));
    return target;
  };

  CHECK(g.stops().empty());   // nothing rendered yet
  CHECK(g.rerender());
  CHECK(g.stops() == std::vector<Key>{Key("a"), Key("b")});
  g.move(GraphDir::Down);  // a2 (remembered for stop a)
  CHECK(enter("b") == id("b1"));   // never visited: its first node
  g.move(GraphDir::Down);  // b2
  CHECK(enter("a") == id("a2"));   // remembered, not the stop's first node
  CHECK(enter("b") == id("b2"));
  CHECK(g.stop_landing(Key("nowhere")) == nullptr);
}

TEST_CASE("section jumps wrap and land on a section's first line") {
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder()
            .set_region(Key("top"))
            .add_item(id("t1"), vt("T1"))
            .set_region(Key("stats"))
            .add_item(id("s1"), vt("S1"))
            .add_item(id("s2"), vt("S2"))
            .set_region(Key("skills"))
            .add_item(id("k1"), vt("K1"))
            .build();
      },
      &state);

  CHECK(g.rerender());
  CHECK(g.regions_of(g.current_node()->stop_key) == std::vector<Key>{Key("top"), Key("stats"), Key("skills")});
  MoveResult r = g.move_region(+1);
  CHECK(r.to->id == id("s1"));
  r = g.move(GraphDir::Down);  // arrows still walk on
  CHECK(r.to->id == id("s2"));
  r = g.move_region(+1);
  CHECK(r.to->id == id("k1"));
  r = g.move_region(+1);  // past the last section: the first again
  CHECK(r.moved);
  CHECK(r.to->id == id("t1"));
  r = g.move_region(-1);  // and back from the first to the last
  CHECK(r.to->id == id("k1"));
}

TEST_CASE("a page with one section, or a node in none, has nowhere to jump") {
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder()
            .add_item(id("plain"), vt("Plain"))
            .begin_stop()
            .set_region(Key("only"))
            .add_item(id("o1"), vt("O1"))
            .add_item(id("o2"), vt("O2"))
            .build();
      },
      &state);

  CHECK_FALSE(g.move_region(+1).moved);   // in no region
  CHECK(g.focus(id("o2")));
  CHECK_FALSE(g.move_region(+1).moved);   // its stop has one
  CHECK(state.cur_key == id("o2"));
}

TEST_CASE("behavior invokers report absence") {
  GraphState state;
  bool clicked = false;
  bool adjusted = false;
  KeyGraph g(
      [&clicked, &adjusted] {
        auto v = std::make_shared<NodeVtable>();
        v->announcements.push_back(NodeAnnouncement::fixed("A"));
        v->on_activate = [&clicked] { clicked = true; };
        v->on_adjust = [&adjusted](int sign, bool) { adjusted = sign > 0; };
        return GraphBuilder().add_item(id("a"), v).build();
      },
      &state);

  CHECK(g.activate());
  CHECK(clicked);
  CHECK(g.try_adjust(+1, false));
  CHECK(adjusted);

  GraphState plain_state;
  KeyGraph plain([] { return GraphBuilder().add_item(id("a"), vt("A")).build(); }, &plain_state);
  CHECK_FALSE(plain.activate());   // a line of information: nothing to activate or adjust
  CHECK_FALSE(plain.try_adjust(+1, false));
}

TEST_CASE("initial focus lands on the selected member") {
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder()
            .add_item(id("a"), radio("A", false))
            .add_item(id("b"), radio("B", false))
            .add_item(id("c"), radio("C", true))  // the checked radio, deep in the list
            .add_item(id("d"), radio("D", false))
            .build();
      },
      &state);

  CHECK(g.rerender());
  CHECK(state.cur_key == id("c"));  // not the first node
}

TEST_CASE("a stop's landing is its selected member when there is no memory") {
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder()
            .begin_stop("a")
            .add_item(id("a1"), vt("A1"))
            .begin_stop("b")
            .add_item(id("b1"), radio("B1", false))
            .add_item(id("b2"), radio("B2", true))  // selected in the second stop
            .build();
      },
      &state);

  CHECK(g.rerender());
  CHECK(g.stop_landing(Key("b"))->id == id("b2"));  // the checked one, not b1
  CHECK(g.focus(id("b2")));

  // But remembered position wins on return.
  g.move(GraphDir::Up);   // b1
  CHECK(g.focus(id("a1")));
  CHECK(g.stop_landing(Key("b"))->id == id("b1"));  // memory beats selection
}

TEST_CASE("a raw block between menu rows stays reachable") {
  // Menu row, raw block (its own internal wiring), menu row -- all one stop. Menu wiring must BREAK at
  // the raw block (declaration order), and the stitcher wires the seams; otherwise the menu edges skip
  // over the block and it becomes an unreachable island.
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder()
            .add_item(id("above"), vt("Above"))
            .add_node(id("raw1"), vt("Raw 1"))
            .add_node(id("raw2"), vt("Raw 2"))
            .connect(id("raw1"), GraphDir::Down, id("raw2"))
            .connect(id("raw2"), GraphDir::Up, id("raw1"))
            .add_item(id("below"), vt("Below"))
            .build();
      },
      &state);

  CHECK(g.rerender());
  CHECK(state.cur_key == id("above"));
  g.move(GraphDir::Down);
  CHECK(state.cur_key == id("raw1"));  // into the block, not over it
  g.move(GraphDir::Down);
  CHECK(state.cur_key == id("raw2"));
  g.move(GraphDir::Down);
  CHECK(state.cur_key == id("below"));  // out the bottom
  g.move(GraphDir::Up);
  CHECK(state.cur_key == id("raw2"));  // and back in
}

TEST_CASE("focus moves to a control by id") {
  GraphState state;
  KeyGraph g(
      [] {
        return GraphBuilder()
            .add_item(id("a"), vt("A"))
            .add_item(id("b"), vt("B"))
            .build();
      },
      &state);

  CHECK(g.focus(id("b")));
  CHECK(state.cur_key == id("b"));
  CHECK(g.focus(id("a")));
  CHECK(state.cur_key == id("a"));
  CHECK_FALSE(g.focus(id("nope")));
  CHECK(state.cur_key == id("a"));
}

TEST_CASE("a vanished last row lands on the row before it") {
  GraphState state;
  std::vector<std::string> items{"a", "b", "c"};
  auto actionable = [](std::string label) { NodeVtablePtr v = vt(std::move(label)); v->on_activate = [] {}; return v; };
  KeyGraph g(
      [&] {
        GraphBuilder b;
        b.begin_stop("mine");
        b.add_item(id("lead"), vt("Lead line"));   // no action: never the landing while a row survives
        for (const std::string& i : items) b.add_item(id(i), actionable(i));
        return b.build();
      },
      &state);
  CHECK(g.rerender());
  CHECK(g.focus(id("c")));
  items.pop_back();   // the last row is gone: no next row, so the previous one
  CHECK(g.rerender());
  CHECK(state.cur_key == id("b"));
}

TEST_CASE("a vanished first row lands on the next row of its own stop, not the previous stop's last node") {
  GraphState state;
  std::vector<std::string> items{"a", "b", "c"};
  auto actionable = [](std::string label) { NodeVtablePtr v = vt(std::move(label)); v->on_activate = [] {}; return v; };
  KeyGraph g(
      [&] {
        GraphBuilder b;
        b.begin_stop("tabs");
        b.add_item(id("tab0"), actionable("stash"));
        b.begin_stop("mine");
        for (const std::string& i : items) b.add_item(id(i), actionable(i));
        return b.build();
      },
      &state);
  CHECK(g.rerender());
  CHECK(g.focus(id("a")));
  items.erase(items.begin());   // Enter moved "a" away
  CHECK(g.rerender());
  CHECK(state.cur_key == id("b"));   // the builder stamped the rows with their stop: the row-vanish rule applies
}

// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
// GraphAnnouncer: what a focus change reads, from the old and new parent paths.
#include "doctest.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/graph_announcer.h"

using p3a::ui::ControlId;
using p3a::ui::GraphAnnouncer;
using p3a::ui::GraphNode;
using p3a::ui::Key;
using p3a::ui::NodeAnnouncement;
using p3a::ui::NodeVtable;
namespace kinds = p3a::ui::announcement_kinds;

namespace {

// The announcer works on raw GraphNode chains, which a render normally owns; these tests build them by
// hand, so this arena plays that role (stable addresses, parent pointers stay valid).
class Arena {
 public:
  GraphNode* node(const std::string& label, GraphNode* parent = nullptr) {
    auto vt = std::make_shared<NodeVtable>();
    vt->announcements.push_back(NodeAnnouncement::fixed(label));
    return add(ControlId::structural(Key(label)), std::move(vt), parent);
  }

  GraphNode* context(const std::string& label, const std::string& role = std::string(),
                     GraphNode* parent = nullptr) {
    auto vt = std::make_shared<NodeVtable>();
    vt->announcements.push_back(NodeAnnouncement::fixed(label));
    if (!role.empty()) vt->announcements.push_back(NodeAnnouncement::fixed(role));
    GraphNode* n = add(ControlId::structural(Key("ctx:" + label)), std::move(vt), parent);
    n->focusable = false;
    return n;
  }

  GraphNode* custom(const std::string& key, std::vector<NodeAnnouncement> parts, GraphNode* parent = nullptr) {
    auto vt = std::make_shared<NodeVtable>();
    vt->announcements = std::move(parts);
    return add(ControlId::structural(Key(key)), std::move(vt), parent);
  }

 private:
  GraphNode* add(ControlId id, p3a::ui::NodeVtablePtr vt, GraphNode* parent) {
    nodes_.push_back(std::make_unique<GraphNode>(std::move(id), std::move(vt)));
    GraphNode* n = nodes_.back().get();
    n->parent = parent;
    return n;
  }
  std::vector<std::unique_ptr<GraphNode>> nodes_;
};

// Readable failure output: the announcer returns nullopt for "nothing to say".
std::string spoken(const std::optional<std::string>& s) { return s ? *s : std::string("<nothing>"); }

// The announcer's position wording is a process-wide static; these cases set the host's ("2 of 5") and
// restore what was there however a case exits.
struct ScopedPositions {
  ScopedPositions() : saved(GraphAnnouncer::position_text) {
    GraphAnnouncer::position_text = [](int i, int n) { return std::to_string(i) + " of " + std::to_string(n); };
  }
  ~ScopedPositions() { GraphAnnouncer::position_text = saved; }
  decltype(GraphAnnouncer::position_text) saved;
};

}  // namespace

TEST_CASE("entry from nothing reads the full chain") {
  Arena a;
  GraphNode* options = a.context("Options");
  GraphNode* list = a.context("Difficulty settings", "list", options);
  GraphNode* node = a.node("Normal, radio button, selected", list);

  CHECK(spoken(GraphAnnouncer::compose_full(node)) ==
        "Options, Difficulty settings, list, Normal, radio button, selected");
}

TEST_CASE("a sibling move reads the leaf only") {
  Arena a;
  GraphNode* list = a.context("Difficulty settings", "list");
  GraphNode* from = a.node("Easy", list);
  GraphNode* to = a.node("Hard", list);

  CHECK(spoken(GraphAnnouncer::compose(from, to)) == "Hard");
}

TEST_CASE("entering a nested context reads the entered levels") {
  Arena a;
  GraphNode* outer = a.context("Options");
  GraphNode* from = a.node("Back", outer);
  GraphNode* list = a.context("Difficulty settings", "list", outer);
  GraphNode* to = a.node("Normal", list);

  CHECK(spoken(GraphAnnouncer::compose(from, to)) == "Difficulty settings, list, Normal");
}

TEST_CASE("an ascend reads the leaf only") {
  Arena a;
  GraphNode* outer = a.context("Options");
  GraphNode* list = a.context("Difficulty settings", "list", outer);
  GraphNode* from = a.node("Normal", list);
  GraphNode* to = a.node("Back", outer);

  CHECK(spoken(GraphAnnouncer::compose(from, to)) == "Back");
}

TEST_CASE("a duplicate container label is skipped") {
  // A "Game difficulty" section wrapping the "Game difficulty" control: the section stays silent.
  Arena a;
  GraphNode* section = a.context("Game difficulty");
  GraphNode* to = a.node("Game difficulty, menu button", section);
  CHECK(spoken(GraphAnnouncer::compose_full(to)) == "Game difficulty, menu button");

  // But a control that merely STARTS with different text keeps its container.
  Arena b;
  GraphNode* other = b.node("Game difficulty presets, menu button", b.context("Game difficulty"));
  CHECK(spoken(GraphAnnouncer::compose_full(other)) ==
        "Game difficulty, Game difficulty presets, menu button");
}

TEST_CASE("leaf text joins the announcement parts in declaration order") {
  Arena a;
  GraphNode* node = a.custom("x", {
                                      NodeAnnouncement::fixed("Hold position"),
                                      NodeAnnouncement::fixed("toggle"),
                                      NodeAnnouncement([] { return std::string("on"); }, true),
                                      NodeAnnouncement([] { return std::string(); }),  // silent
                                  });
  CHECK(spoken(GraphAnnouncer::compose_full(node)) == "Hold position, toggle, on");
}

TEST_CASE("the position follows the parts and a description follows the position") {
  // A menu row as the screens declare it: label, a live value, and its description as a tooltip part.
  Arena a;
  GraphNode* node = a.custom("dia", {
                                        NodeAnnouncement([] { return std::string("Dia"); }, false, kinds::kLabel),
                                        NodeAnnouncement([] { return std::string("Slightly restores 1 ally's HP."); }, true, kinds::kTooltip),
                                        NodeAnnouncement([] { return std::string("3 SP"); }, true, kinds::kValue),
                                    });
  node->position_index = 2;
  node->position_count = 5;
  ScopedPositions positions;
  CHECK(spoken(GraphAnnouncer::leaf_text(node)) == "Dia, 3 SP, 2 of 5, Slightly restores 1 ally's HP.");
}

TEST_CASE("a node that speaks its own position, or has none, gets no stamped one") {
  ScopedPositions positions;
  Arena a;
  GraphNode* own = a.custom("own", {NodeAnnouncement::fixed("Yes")});
  own->position_index = 1;
  own->position_count = 1;
  own->vtable->speaks_own_position = true;   // one thing to answer: no "1 of 1"
  CHECK(spoken(GraphAnnouncer::leaf_text(own)) == "Yes");

  GraphNode* part = a.custom("part", {NodeAnnouncement::fixed("April 30"),
                                      NodeAnnouncement([] { return std::string("day 30 of 30"); }, false, kinds::kPosition)});
  part->position_index = 3;
  part->position_count = 9;
  CHECK(spoken(GraphAnnouncer::leaf_text(part)) == "April 30, day 30 of 30");

  GraphNode* raw = a.custom("raw", {NodeAnnouncement::fixed("A line")});   // position_count 0: a raw node
  CHECK(spoken(GraphAnnouncer::leaf_text(raw)) == "A line");
}

TEST_CASE("the transition label leads") {
  Arena a;
  GraphNode* from = a.node("A");
  GraphNode* to = a.node("B");
  CHECK(spoken(GraphAnnouncer::compose(from, to, "next column")) == "next column, B");
}

TEST_CASE("a context change at the same depth reads the new level") {
  Arena a;
  GraphNode* from = a.node("Fireball", a.context("Level 1 spells", "table"));
  GraphNode* to = a.node("Haste", a.context("Level 2 spells", "table"));

  CHECK(spoken(GraphAnnouncer::compose(from, to)) == "Level 2 spells, table, Haste");
}

TEST_CASE("a remembered path stands in for a node that is gone") {
  // The row focus came from was removed by its own action: its path still says which list it sat in, so
  // the row that takes its place reads as a sibling, without the list's title.
  Arena a;
  GraphNode* list = a.context("Your items", "list");
  GraphNode* gone = a.node("Bead", list);
  const std::vector<ControlId> path = GraphAnnouncer::path_ids(gone);
  REQUIRE(path.size() == 2);
  CHECK(path[0] == list->id);
  CHECK(path[1] == gone->id);

  GraphNode* next = a.node("Soma", list);
  CHECK(spoken(GraphAnnouncer::compose_from_path(path, next)) == "Soma");
  CHECK(spoken(GraphAnnouncer::compose_from_path({}, next)) == "Your items, list, Soma");
  CHECK(GraphAnnouncer::path_ids(nullptr).empty());
}

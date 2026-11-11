// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/control_id.h"

// The data model of the UI navigation graph. Engine-free by construction: nothing here knows a game
// type; behaviours are std::function slots.
//
// String convention: an optional string is a std::string, and EMPTY means "none" (no kind, no label,
// no landing group). The announcer's RETURN values (compose/leaf_text/first_part_text) are
// std::optional<std::string> instead, because there "nothing to say" is the documented result the
// caller branches on.
namespace p3a::ui {

/// The four navigable directions between graph nodes (explicit edges). Tab-stop cycling and region
/// jumps are OPERATIONS over node metadata (GraphNode::stop_key / GraphNode::region_key), not edges --
/// they carry per-stop remembered positions, which a static edge can't express.
enum class GraphDir : int { Up = 0, Right = 1, Down = 2, Left = 3 };
inline constexpr int kGraphDirCount = 4;

/// The well-known announcement-part kinds. The engine reads four of them: a tooltip part is spoken
/// after the position, a position part replaces the auto-stamped one, a non-empty selected part marks
/// the member a stop lands on, and a live value part that changes interrupts speech.
namespace announcement_kinds {
inline constexpr std::string_view kLabel = "label";
inline constexpr std::string_view kValue = "value";
inline constexpr std::string_view kSelected = "selected";
inline constexpr std::string_view kTooltip = "tooltip";
inline constexpr std::string_view kPosition = "position";
}  // namespace announcement_kinds

/// One part of a control's spoken focus readout ("Hold position" / "toggle" / "on"), resolved live at
/// speak time. A LIVE part is additionally watched while its node is focused: when its resolved text
/// changes (a value the game flips, a description arriving), the navigator speaks just that part --
/// state feedback without re-reading the whole control.
struct NodeAnnouncement {
  /// The part's text, resolved live. An empty result (or an empty std::function) at speak time = the
  /// part stays silent.
  std::function<std::string()> text;
  /// Watch this part while the node is focused and speak it when its value changes.
  bool live = false;
  /// The part's kind (announcement_kinds), or EMPTY for a custom one-off part.
  std::string kind;

  NodeAnnouncement() = default;
  explicit NodeAnnouncement(std::function<std::string()> text_fn, bool is_live = false,
                            std::string_view part_kind = {})
      : text(std::move(text_fn)), live(is_live), kind(part_kind) {}

  /// A part whose text never changes.
  static NodeAnnouncement fixed(std::string text_value) {
    return NodeAnnouncement([t = std::move(text_value)] { return t; });
  }
};

/// The behaviors of a control, as data. `announcements` is required (its parts compose the spoken focus
/// readout; the first part is the control's label for search/dedupe purposes); the rest are optional --
/// an empty slot means the control doesn't have that behavior.
struct NodeVtable {
  /// Required, at least one part. The control's spoken focus readout, its parts in declaration order
  /// (tooltip-kind parts after the position). Parts marked `live` re-speak on change while focused.
  std::vector<NodeAnnouncement> announcements;

  /// Optional. The node's LANDING group -- the general row-vanish rule: when the focused node
  /// disappears on a rebuild, the reconcile fallback lands on the nearest surviving node of the SAME
  /// group (next first, then previous), never on a header, lead line or another panel. The builder
  /// stamps every actionable item with its Tab-stop's key (GraphBuilder::add_item); nodes without a
  /// group use the plain nearest-survivor walk.
  std::string land_group;

  /// Optional. Primary activation (the host's confirm: ui_actions::Activate).
  std::function<void()> on_activate;
  /// Optional. The control's details as text (a description, stats): what the host reads on demand (P3R: the review
  /// key). The navigator itself never reads it.
  std::function<std::string()> details;
  /// Optional. The control's details as lines the host walks one at a time on demand (P3R: the detail buffer, the
  /// right stick or Ctrl+Up / Down), the control's own line first.
  std::function<std::vector<std::string>()> detail_lines;
  /// Optional. Horizontal value adjust (a setting's value): sign is -1 (decrease) / +1 (increase),
  /// large requests a coarse step. When set, left/right do NOT navigate.
  std::function<void(int sign, bool large)> on_adjust;

  /// Optional. The control's state line, spoken IMMEDIATELY (interrupting) after an activation/adjust
  /// that changes state -- the synchronous feedback path for rapid key repeats. Asynchronous /
  /// game-driven changes ride the Live announcement watch instead.
  std::function<std::string()> state_text;

  /// Set when this node's announcements already include its list position, or it should have none (a
  /// lone prompt), so the announcer doesn't append the auto-stamped one.
  bool speaks_own_position = false;
};

using NodeVtablePtr = std::shared_ptr<NodeVtable>;

/// A directed edge to another node. Plain data: what a move reads is composed from node metadata by
/// the announcer, which keeps renders cheap to throw away.
struct Transition {
  ControlId destination;

  explicit Transition(ControlId dest) : destination(std::move(dest)) {}
};

/// A control: identity, behaviors, directional transitions, and structural metadata (its parent chain,
/// tab-stop and region membership).
///
/// Nodes are heap-allocated and owned by their GraphRender: the parent relation is an IDENTITY
/// relation (`n->parent == context`), so the addresses must be stable for the render's lifetime and
/// comparable by pointer.
class GraphNode {
 public:
  GraphNode(ControlId node_id, NodeVtablePtr node_vtable)
      : id(std::move(node_id)), vtable(std::move(node_vtable)) {}

  ControlId id;
  NodeVtablePtr vtable;  // null is legal on a hand-built node (the builder never makes one)

  const Transition* transition(GraphDir dir) const {
    const auto& slot = transitions_[static_cast<std::size_t>(dir)];
    return slot ? &*slot : nullptr;
  }
  bool has_transition(GraphDir dir) const {
    return transitions_[static_cast<std::size_t>(dir)].has_value();
  }
  void set_transition(GraphDir dir, Transition t) {
    transitions_[static_cast<std::size_t>(dir)] = std::move(t);
  }

  /// The node's structural parent within THIS render, or null at screen level. The parent chain IS the
  /// presentation hierarchy: the announcer prefix-diffs old/new chains by identity, so entering a
  /// context reads its levels outermost-first and a move inside one reads the control alone. A parent
  /// is pure structure (a titled list): it exists only on parent chains for announcements -- never
  /// navigable, never in the render's nodes/order.
  GraphNode* parent = nullptr;

  /// The Tab-stop this node belongs to. Nodes sharing a stop key form one stop; Tab cycles stops in
  /// first-appearance order, landing on the stop's remembered position.
  Key stop_key;
  /// The region (within a stop) this node belongs to, or empty: a page's section. The section actions
  /// jump between a stop's regions in first-appearance order.
  Key region_key;

  /// Auto-stamped sibling position (1-based) and count, from the builder: menu-mode nodes grouped by
  /// (parent, stop) -- "3 of 10" among the siblings arrows actually reach; a multi-item row's members
  /// are positioned within their row. 0 = none (raw/grid nodes).
  int position_index = 0;
  int position_count = 0;

 private:
  std::array<std::optional<Transition>, kGraphDirCount> transitions_;
};

/// One built snapshot of a graph: the nodes (keyed by structural identity), their order of declaration,
/// and where focus starts when there is no prior position. Rebuilt per operation and thrown away --
/// live state belongs in the node callbacks, not here. Owns every node it was built with, INCLUDING
/// the context nodes that appear only on parent chains.
class GraphRender {
 public:
  /// Where focus starts with no prior position. Always set by GraphBuilder::build(); unset only on a
  /// hand-built render, which then leaves focus unset.
  std::optional<ControlId> start_key;

  /// Take ownership of a node without publishing it (the context parents).
  GraphNode* adopt(std::unique_ptr<GraphNode> node) {
    arena_.push_back(std::move(node));
    return arena_.back().get();
  }
  /// Publish an already-adopted node: navigable, in declaration order.
  void publish(GraphNode* node) {
    nodes_.emplace(node->id, node);
    order_.push_back(node);
  }

  GraphNode* node_at(const ControlId& key) const {
    auto it = nodes_.find(key);
    return it == nodes_.end() ? nullptr : it->second;
  }
  GraphNode* node_at(const std::optional<ControlId>& key) const { return key ? node_at(*key) : nullptr; }
  bool contains(const ControlId& key) const { return nodes_.count(key) != 0; }
  bool contains(const std::optional<ControlId>& key) const { return key && contains(*key); }

  /// Declaration order -- drives stop/region cycling and type-ahead scan order.
  const std::vector<GraphNode*>& order() const { return order_; }
  std::size_t node_count() const { return nodes_.size(); }

 private:
  std::vector<std::unique_ptr<GraphNode>> arena_;
  std::unordered_map<ControlId, GraphNode*> nodes_;
  std::vector<GraphNode*> order_;
};

using GraphRenderPtr = std::unique_ptr<GraphRender>;

/// The persistent cursor for a graph -- the only thing that survives between renders. Holds where focus
/// is, the last computed traversal order (for closest-survivor recovery) and per-stop remembered
/// positions (so Tab returns to where you were in a stop).
struct GraphState {
  /// The focused control's id. Unset until first render.
  std::optional<ControlId> cur_key;

  /// The down-right total order from the previous render. EMPTY on first render.
  std::vector<ControlId> key_order;

  /// The focused node's NodeVtable::land_group (empty when it had none) -- the row-vanish fallback
  /// prefers survivors of this group.
  std::string last_land_group;

  /// Remembered position per Tab-stop: where Tab lands when cycling back into a stop.
  std::unordered_map<Key, ControlId> stop_memory;
};

}  // namespace p3a::ui

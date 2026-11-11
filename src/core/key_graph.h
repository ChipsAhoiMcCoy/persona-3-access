// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "core/graph_types.h"

namespace p3a::ui {

/// The outcome of a navigation operation, for the caller (navigator) to announce. The core never
/// speaks -- it returns what happened.
///
/// `from`/`to` point into the render they were read from. KeyGraph keeps the previous render alive
/// across exactly one rebuild (see KeyGraph::rerender), so a result stays readable until the operation
/// after the one that produced it.
struct MoveResult {
  bool moved = false;          // focus actually changed nodes
  GraphNode* from = nullptr;   // node before the operation (null on first landing)
  GraphNode* to = nullptr;     // node after (== from when at an edge; null when the graph is empty)
};

/// The navigation engine: a directed graph of controls rebuilt from a render callback on each
/// operation, with focus persisting in an external GraphState. Two invariants:
///
/// **Down-right total order** (compute_order): from the start node, go right until stuck, queueing each
/// down -- visits a planar UI in reading order. Nodes down-right can't reach (later Tab-stops) are
/// appended in declaration order, keeping the order total.
///
/// **Focus recovery on rebuild** (reconcile): the focused control is followed by its id; if it
/// vanished, focus lands on the nearest survivor rather than jumping to the start.
///
/// On top of the edges: Tab-stops (with per-stop remembered positions) and region jumps as operations
/// over node metadata, and the per-node activate/adjust behaviors.
class KeyGraph {
 public:
  /// `state` is external and must outlive the graph (the navigator owns it; it is the only thing that
  /// survives a rebuild).
  KeyGraph(std::function<GraphRenderPtr()> render_callback, GraphState* state)
      : render_callback_(std::move(render_callback)), state_(state) {}

  KeyGraph(const KeyGraph&) = delete;
  KeyGraph& operator=(const KeyGraph&) = delete;

  GraphState& state() { return *state_; }

  /// The most recently built render, or null if not yet rendered / empty.
  GraphRender* current() const { return current_.get(); }

  /// The focused node in the current render, or null.
  GraphNode* current_node() const { return current_ ? current_->node_at(state_->cur_key) : nullptr; }

  /// Rebuild the render and reconcile focus into it. False when the callback produced nothing (the
  /// caller should treat the graph as closed/empty).
  bool rerender();

  /// Move focus from the cached GraphState::cur_key to a valid control in `render`, then recompute the
  /// traversal order.
  static void reconcile(GraphRender& render, GraphState& state);

  /// The down-right total order: go right until stuck (recording each node), queue every down for a
  /// later pass, repeat -- then append any node the walk never reached (e.g. later Tab-stops, which
  /// have no cross-stop edges) in declaration order, so the order is total.
  static std::vector<ControlId> compute_order(const GraphRender& render);

  // ---- stops and regions of the current render ----

  /// The Tab-stops, in declaration order. Empty when there is no render.
  std::vector<Key> stops() const;
  /// The regions of a stop (a page's sections), in declaration order.
  std::vector<Key> regions_of(const Key& stop_key) const;

  // ---- navigation operations ----

  /// One step in `dir`. Not moved (at an edge / empty) -> to == from.
  MoveResult move(GraphDir dir);

  /// As far as possible in `dir` (Home/End within a row or column).
  MoveResult move_to_edge(GraphDir dir);

  /// Jump to the next/previous region within the current stop (declaration order), landing on the
  /// region's first node; past the last region to the first and back (a page's sections). Not moved
  /// when the focused node is in no region or its stop has one region only.
  MoveResult move_region(int dir);

  /// Move focus to a specific control (a screen's chosen landing, a stop's landing, a search result).
  /// False when it isn't in the render.
  bool focus(const ControlId& id);

  /// Where focus lands when entering a stop: the remembered position, else the SELECTED member (the
  /// item currently chosen -- a boon on long lists), else the stop's first node.
  GraphNode* stop_landing(const Key& stop_key) const;
  static GraphNode* stop_landing(const GraphRender& render, const GraphState& state, const Key& stop_key);

  /// The first node in a stop that reads as SELECTED -- carries a non-empty selected-kind announcement
  /// part -- among the siblings of `sibling_of` (a chosen item under another parent must not steal the
  /// landing from the line above it), or null.
  static GraphNode* selected_node_in_stop(const GraphRender& render, const Key& stop_key, const GraphNode* sibling_of);

  // ---- behavior invokers (the caller announces state) ----

  /// Run the focused control's primary activation. False = it has none.
  bool activate();
  /// If the focused control adjusts horizontally (a setting's value), adjust and return true; false = the caller
  /// should navigate instead.
  bool try_adjust(int sign, bool large);

 private:
  void set_current(GraphNode* node);

  std::function<GraphRenderPtr()> render_callback_;
  GraphState* state_;
  GraphRenderPtr current_;
  // The render before `current_`, kept alive for one more operation: every MoveResult hands the caller
  // nodes that the next rebuild would otherwise free.
  GraphRenderPtr previous_;
};

}  // namespace p3a::ui

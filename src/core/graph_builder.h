// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/graph_types.h"

namespace p3a::ui {

/// Builds a GraphRender. Two construction styles, freely mixable in one build:
///
/// **Menu mode** -- rows of controls, wired automatically: left/right within a row, up/down between
/// consecutive rows (two rows sharing a non-empty row key get column navigation -- up/down preserves
/// the position instead of snapping to the first item). Items added outside an explicit row become
/// single-item rows (a plain vertical menu).
///
/// **Raw mode** -- add_node() + connect() for arbitrary topologies.
///
/// Orthogonal to both: begin_stop() groups nodes into Tab-stops (arrows never cross a stop; Tab cycles
/// them), set_region() tags nodes with a region (a page's section, for the section jumps), and the
/// PARENT STACK builds the presentation hierarchy: push_context() pushes a non-focusable structural
/// level ("Difficulty settings, list" -- announced when focus enters from outside). Nesting recurses.
///
/// Ownership: every node the builder makes -- including the non-focusable context parents that are
/// never published -- is owned by the builder until build() hands the whole arena to the render, which
/// then owns it. Parent pointers therefore stay valid exactly as long as the render.
///
/// Misuse throws: std::invalid_argument for a bad argument (a control with no announcement),
/// std::logic_error for a call out of order (an unclosed row, a duplicate id, a pop with nothing pushed).
class GraphBuilder {
 public:
  GraphBuilder() = default;
  GraphBuilder(const GraphBuilder&) = delete;
  GraphBuilder& operator=(const GraphBuilder&) = delete;

  // ---- stops / regions ----

  /// Start a new Tab-stop; nodes added from here belong to it. `key` must be stable across rebuilds
  /// (it keys the stop's remembered position); an empty key auto-assigns by index, which is stable when
  /// the screen builds its stops in a fixed order.
  GraphBuilder& begin_stop(Key key = {});

  /// Tag nodes added from here with a region (a section-jump target) within the current stop; an empty
  /// key clears. Region keys must be stable across rebuilds.
  GraphBuilder& set_region(Key key);

  // ---- the parent stack: contexts ----

  /// Push one NON-FOCUSABLE level of presentation hierarchy ("Difficulty settings", "list") onto nodes
  /// added from here -- pure structure: never navigable, announced when focus enters from outside.
  /// Close with pop_context().
  GraphBuilder& push_context(std::string_view label, std::string_view role = {});

  GraphBuilder& pop_context();

  /// Focus starts here when the graph has no prior position (defaults to the first node).
  GraphBuilder& set_start(ControlId id);

  // ---- menu mode ----

  /// Open a horizontal row. Rows sharing a non-empty `row_key` with the row above/below get
  /// column-preserving vertical navigation.
  GraphBuilder& start_row(Key row_key = {});
  GraphBuilder& end_row();

  /// Add a control -- into the open row, or as its own single-item row.
  GraphBuilder& add_item(ControlId id, NodeVtablePtr vtable);

  // ---- raw mode ----

  /// Add a node with no automatic wiring (raw mode; wire with connect()).
  GraphBuilder& add_node(ControlId id, NodeVtablePtr vtable);

  /// Directed edge from -> to. Edges to/from undeclared nodes are dropped at build.
  GraphBuilder& connect(ControlId from, GraphDir dir, ControlId to);

  // ---- build ----

  /// Finalize into a render, or null when nothing was declared (treat as "closed"). Menu rows and raw
  /// nodes/edges may coexist in one build (a screen mixing lists with a grid whose topology is
  /// computed): rows wire themselves; raw edges may reference any node.
  GraphRenderPtr build();

 private:
  struct Row {
    std::vector<GraphNode*> items;
    Key key;
    Key stop_key;
  };

  struct RawEdge {
    ControlId from;
    GraphDir dir;
    ControlId to;
  };

  static Key auto_stop_key(int index);
  GraphNode* current_parent() const { return parents_.empty() ? nullptr : parents_.back(); }
  GraphNode* make_node(ControlId id, NodeVtablePtr vtable);
  GraphNode* own(std::unique_ptr<GraphNode> node);

  void wire_menu_edges();
  void stitch_mode_boundaries();
  void stamp_positions();
  static void stamp(const std::vector<GraphNode*>& siblings);
  static ControlId vertical_target(const Row& from, const Row& to, std::size_t pos);

  // Every node this builder made, in creation order; handed to the render at build().
  std::vector<std::unique_ptr<GraphNode>> owned_;
  std::vector<std::unique_ptr<Row>> owned_rows_;

  // Menu mode.
  std::vector<Row*> rows_;
  Row* current_row_ = nullptr;

  // Raw mode.
  std::vector<GraphNode*> raw_nodes_;
  std::vector<RawEdge> raw_edges_;

  // Every node in DECLARATION order regardless of mode -- the render's node order (and so the Tab-stop
  // cycle) must interleave menu rows and raw nodes as the screen declared them, not rows-then-raw
  // (which would put a raw stop behind the menu stops declared after it).
  std::vector<GraphNode*> declared_;

  // The menu row each menu-mode node belongs to (absent for raw nodes) -- for stitching the vertical
  // gap where a stop mixes menu rows with raw content (a grid below a list's rows).
  std::unordered_map<const GraphNode*, Row*> row_of_;

  // Shared.
  std::unordered_set<ControlId> ids_;
  std::optional<ControlId> start_;

  // Stop / region / parent state applied to nodes as they are added.
  Key stop_key_ = auto_stop_key(0);
  int stop_auto_ = 1;
  Key region_key_;
  std::vector<GraphNode*> parents_;   // the open contexts, outermost first
};

}  // namespace p3a::ui

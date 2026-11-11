// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/graph_types.h"

namespace p3a::ui {

/// Composes the spoken line for a focus change by diffing the old and new focus PATHS -- each node's
/// ancestor chain (GraphNode::parent) plus the node itself, compared by identity. Newly-entered levels
/// read outermost-first, then the landing control: "Difficulty settings, list, Normal, radio button,
/// selected", recursing as deep as the hierarchy goes. Sibling moves share the whole prefix and read
/// just the control; ascends likewise; and descending from a group onto its own child re-announces
/// nothing but the child -- the group is on the child's chain AND is the from-node, so the prefix
/// swallows it. The paths are rebuilt per render from parent pointers.
///
/// Every utterance is assembled with p3a::ui::MessageBuilder: one list item per spoken part, so the
/// comma discipline lives in one place.
///
/// The three hook points below are process-wide mutable statics: the host installs them once at
/// startup (they are where the mod's wording comes in), tests set and restore them.
class GraphAnnouncer {
 public:
  /// The line for landing on `to` having come from `from` (null = from nothing: the full path reads).
  /// `transition_label` is the crossed edge's spoken line, when it had one. nullopt when there is
  /// nothing to say.
  static std::optional<std::string> compose(const GraphNode* from, const GraphNode* to,
                                            std::string_view transition_label = {});
  /// The same, from a REMEMBERED path (root first, the node last): the node focus came from may no longer exist
  /// (its row vanished on the rebuild its own action caused), yet the levels it sat in are known, so its
  /// replacement reads as a sibling move, not as an entry from nothing that repeats every title above it.
  static std::optional<std::string> compose_from_path(const std::vector<ControlId>& from_path, const GraphNode* to,
                                                      std::string_view transition_label = {});
  static std::vector<ControlId> path_ids(const GraphNode* node);   // root first, `node` last; empty for null

  /// The full readout for a landing with no prior focus (screen entry, focus restore).
  static std::optional<std::string> compose_full(const GraphNode* to) { return compose(nullptr, to); }

  /// Pluggable per-part filter (by control type and by kind); unset = everything speaks. Returning
  /// false drops the part from readouts AND from the live watch. The control type is null for an
  /// untyped node.
  static inline std::function<bool(const ControlType*, const NodeAnnouncement&)> part_filter;

  /// Pluggable "n of m" wording (the host's); unset = no auto positions.
  static inline std::function<std::string(int index, int count)> position_text;

  /// Pluggable expanded/collapsed wording for group headers (the host's); unset = groups don't speak
  /// their state.
  static inline std::function<std::string(bool expanded)> expanded_state_text;

  /// A node's EFFECTIVE announcement parts: the control type's common parts (the role word) merged with
  /// the node's own -- a node part overrides a common part of the same kind -- sorted by the type's kind
  /// order (unknown/kindless parts append in declaration order), then filtered by part_filter.
  /// This is the single list readouts and the live watch operate on.
  ///
  /// Returns COPIES: a type's common parts are produced fresh by its `common` callback, so there is
  /// nothing stable to hand back a pointer to.
  static std::vector<NodeAnnouncement> effective_announcements(const GraphNode* node);

  /// A node's own readout: its effective announcement parts, resolved live, non-empty ones joined --
  /// plus, for an expandable group, its expanded/collapsed state word, and the auto-stamped sibling
  /// position; tooltip-kind parts (a description) come last, after the position. The first part is the
  /// control's label, so path dedupe's prefix check applies.
  static std::optional<std::string> leaf_text(const GraphNode* node);

  /// The first announcement part's text (the label) -- for the path dedupe.
  static std::optional<std::string> first_part_text(const GraphNode* node);
};

}  // namespace p3a::ui

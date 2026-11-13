// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/graph_types.h"

namespace p3a::ui {

/// Composes the spoken line for a focus change by diffing the old and new focus PATHS -- each node's
/// ancestor chain (GraphNode::parent) plus the node itself, compared by identity. Newly-entered levels
/// read outermost-first, then the landing control: "Difficulty settings, list, Normal, radio button,
/// selected", recursing as deep as the hierarchy goes. Sibling moves share the whole prefix and read
/// just the control; so does a move out to a level focus was already inside. The paths are rebuilt
/// per render from parent pointers.
///
/// Every utterance is assembled with p3a::ui::MessageBuilder: one list item per spoken part, so the
/// comma discipline lives in one place.
///
/// The position wording is the host's (where the mod's wording comes in): each navigator carries its own
/// announcer, made from its NavigatorHost.
class GraphAnnouncer {
 public:
  /// "n of m" wording for an auto-stamped sibling position; unset = no auto positions.
  using PositionText = std::function<std::string(int index, int count)>;
  GraphAnnouncer() = default;
  explicit GraphAnnouncer(PositionText positions) : position_text_(std::move(positions)) {}

  /// The line for landing on `to` having come from `from` (null = from nothing: the full path reads).
  /// nullopt when there is nothing to say.
  std::optional<std::string> compose(const GraphNode* from, const GraphNode* to) const;
  /// The same, from a REMEMBERED path (root first, the node last): the node focus came from may no longer exist
  /// (its row vanished on the rebuild its own action caused), yet the levels it sat in are known, so its
  /// replacement reads as a sibling move, not as an entry from nothing that repeats every title above it.
  std::optional<std::string> compose_from_path(const std::vector<ControlId>& from_path, const GraphNode* to) const;
  static std::vector<ControlId> path_ids(const GraphNode* node);   // root first, `node` last; empty for null

  /// The full readout for a landing with no prior focus (screen entry, focus restore).
  std::optional<std::string> compose_full(const GraphNode* to) const { return compose(nullptr, to); }

  /// A node's own readout: its announcement parts in declaration order, resolved live, non-empty ones
  /// joined -- plus the auto-stamped sibling position; tooltip-kind parts (a description) come last,
  /// after the position. The first part is the control's label, so path dedupe's prefix check applies.
  std::optional<std::string> leaf_text(const GraphNode* node) const;

  /// The first announcement part's text (the label) -- for the path dedupe.
  static std::optional<std::string> first_part_text(const GraphNode* node);

 private:
  PositionText position_text_;
};

}  // namespace p3a::ui

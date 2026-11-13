// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
// The graph-based navigator.
//
// Runs every screen on the key-graph core: the graph is rebuilt per operation and per frame, focus is
// reconciled by identity, and a focus change is announced exactly once no matter what caused it (input,
// a screen moving focus, a content rebuild, the game moving its own cursor). Announcements are PULL-based:
// screens never make per-callsite announce decisions. Engine-free: speech, hover sounds, the focus-mode
// flag and the position wording come from the host through NavigatorHost.
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "core/graph_announcer.h"
#include "core/graph_types.h"
#include "core/key_graph.h"
#include "core/ui_screen.h"

namespace p3a::ui {

struct NavigatorHost {
  std::function<void(std::string_view text, bool interrupt)> speak;
  std::function<void()> hover_sound;        // the sound of a landing (a move, an edge repeat)
  std::function<bool()> focus_mode_active;  // false = the mod does not own the keyboard; stay silent
  std::function<int()> frame_count;         // for the idle-rebuild throttle
  // An edge press the host hands on to something that answers it (a game list that moves on past the mod's ends):
  // no repeat of the focused item, the answer speaks. Empty = every edge repeats.
  std::function<bool(GraphDir)> edge_answered;
  // The "n of m" wording stamped on a list item; empty = no positions.
  GraphAnnouncer::PositionText position_text;
};

// The UI action ids the navigator understands: the host maps its keys and buttons to these and calls on_action.
namespace ui_actions {
inline constexpr std::string_view Up = "ui.up", Down = "ui.down", Left = "ui.left", Right = "ui.right";
inline constexpr std::string_view Next = "ui.next", Prev = "ui.prev", Home = "ui.home", End = "ui.end";
// A page's sections (regions of the focused stop), wrapping.
inline constexpr std::string_view SectionPrev = "ui.sectionPrev", SectionNext = "ui.sectionNext";
inline constexpr std::string_view Activate = "ui.activate";
}  // namespace ui_actions

class GraphNavigator {
 public:
  explicit GraphNavigator(NavigatorHost host);

  Screen* screen() const { return screen_; }

  // Bind to a screen. Re-attaching the SAME screen means "content changed" (focus and announce memory
  // survive); a new screen swaps to its own persistent state (created on first attach).
  void attach(Screen* screen);
  // A screen closed and its focus is not to be kept for its next opening: drop its state.
  void screen_closed(Screen* screen);
  // The per-frame pull: rebuild + reconcile, establish initial focus when content appears, apply pending
  // focus requests, announce any focus-identity change exactly once, watch live parts. Idle frames rebuild only
  // every kIdleRenderEvery-th frame; after invalidate() (and after any input or attach) the next pull rebuilds.
  void ensure_focus();
  // The screen's content changed: the next ensure_focus rebuilds whatever the idle throttle says.
  void invalidate() { render_due_ = true; }
  // The full readout of a node, as an entry reads it (no prior focus), with this navigator's position wording.
  std::optional<std::string> compose_full(const GraphNode* node) const { return announcer_.compose_full(node); }
  // Announce the current focus in full (a screen's entry, after its title).
  void announce_current();
  // Seat focus without a word (the landing counts as spoken): a screen whose entry something else announces.
  void settle_silently();
  // The focused node's live parts take their next values as a fresh baseline: nothing is said until they change again.
  void forget_live() { live_key_.reset(); }
  // Move focus to a node by id when it exists in a render (one retry frame for content appearing).
  // interrupt: the landing interrupts speech (the game moved its cursor on its own: a player's move, not an entry).
  void focus_node(const ControlId& id, bool announce = true, bool interrupt = false);
  std::optional<ControlId> focused_id() const;  // identity of the focused node, if any
  // The focus the state remembers: unlike focused_id(), known while the screen builds (the render is being replaced).
  std::optional<ControlId> remembered_focus() const { return state_ ? state_->cur_key : std::nullopt; }
  int stop_count() const;   // distinct Tab-stops in the current render
  int section_count() const;   // distinct regions in the focused node's stop (0 when it is in none)
  const GraphNode* focused_node() const { return graph_ ? graph_->current_node() : nullptr; }   // this render's

  // Input: returns true when consumed. Called by the host with the action id of the key or button pressed.
  bool on_action(std::string_view action_key);

  // Debug inspection: one line per node as the announcer reads it, focused one prefixed '>'.
  std::string dump();

 private:
  GraphRenderPtr build_render(Screen* screen);
  bool arrow(GraphDir dir);
  bool tab(int step);
  bool land_on_stop(const Key& stop_key);
  bool jump_edge(bool first);
  static bool on_list_axis(const GraphNode* node, GraphDir dir);  // is `dir` along the list the node sits in
  void repeat_focused();                                           // the edge case: say the focused item again
  bool section_jump(int dir);
  // A player's landing on `to`: the host's sound, then the line read as a move from `from`, interrupting.
  void land(const GraphNode* to, const GraphNode* from);
  bool vtable_activate();
  bool vtable_adjust(int sign);
  void feedback_after_change();  // speak state_text, rerender, rebaseline the live watch
  void play_hover();
  void speak(std::string_view text, bool interrupt);
  std::optional<std::string> compose_move(const GraphNode* from, const GraphNode* to, bool entry);
  void watch_live(const GraphNode* node);
  void mark_spoken(const GraphNode* node);

  NavigatorHost host_;
  GraphAnnouncer announcer_;
  Screen* screen_ = nullptr;
  std::unordered_map<Screen*, GraphState> states_;
  GraphState* state_ = nullptr;
  GraphState scratch_state_;
  std::unique_ptr<KeyGraph> graph_;
  std::optional<ControlId> last_spoken_key_;
  std::vector<ControlId> last_spoken_path_;   // its context path, for when the node itself has vanished
  // The last spoken node, resolved by id in the CURRENT render. Never keep a GraphNode* across frames: the
  // graph is rebuilt immediate-mode and a stored pointer dangles. Null when that node is not in this render.
  const GraphNode* last_spoken_node() const { return graph_ && graph_->current() ? graph_->current()->node_at(last_spoken_key_) : nullptr; }
  std::optional<ControlId> pending_focus_;
  bool pending_announce_ = true;
  bool pending_interrupt_ = false;
  int last_idle_render_ = -1000000;
  bool render_due_ = true;   // input, an attach or invalidate(): the next pull rebuilds
  static constexpr int kIdleRenderEvery = 6;  // ~100 ms at 60 fps
  // live watch
  std::optional<ControlId> live_key_;
  std::vector<std::optional<std::string>> live_values_;
};

}  // namespace p3a::ui

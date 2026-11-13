// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#include "navigator.h"
#include <algorithm>
#include "core/graph_builder.h"

namespace p3a::ui {

GraphNavigator::GraphNavigator(NavigatorHost host) : host_(std::move(host)), announcer_(host_.position_text) {}

void GraphNavigator::speak(std::string_view text, bool interrupt) {
  if (!text.empty() && host_.speak) host_.speak(text, interrupt);
}

// Screens declare fresh from live state on every render (immediate mode).
GraphRenderPtr GraphNavigator::build_render(Screen* screen) {
  GraphBuilder b;
  screen->build(b);
  return b.build();
}

void GraphNavigator::attach(Screen* screen) {
  bool same = screen == screen_;
  screen_ = screen;
  render_due_ = true;
  if (!same) {
    // Swap to this screen's own state (creating it on first attach). The differ memory resets so the
    // (possibly restored) landing announces itself on return.
    if (screen) state_ = &states_[screen];
    else { scratch_state_ = GraphState{}; state_ = &scratch_state_; }
    last_spoken_key_.reset();
    pending_focus_.reset();
    live_key_.reset();
  }
  if (!state_) { scratch_state_ = GraphState{}; state_ = &scratch_state_; }
  graph_ = screen ? std::make_unique<KeyGraph>([this, screen] { return build_render(screen); }, state_) : nullptr;
}

void GraphNavigator::screen_closed(Screen* screen) {
  if (!screen) return;
  if (state_ == &states_[screen]) { scratch_state_ = GraphState{}; state_ = &scratch_state_; graph_.reset(); if (screen_ == screen) screen_ = nullptr; }
  states_.erase(screen);
}

void GraphNavigator::focus_node(const ControlId& id, bool announce, bool interrupt) {
  pending_focus_ = id;
  pending_announce_ = announce;
  pending_interrupt_ = interrupt;
}

int GraphNavigator::stop_count() const { return graph_ ? (int)graph_->stops().size() : 0; }

int GraphNavigator::section_count() const {
  const GraphNode* cur = graph_ ? graph_->current_node() : nullptr;
  if (!cur || cur->region_key.empty()) return 0;
  return (int)graph_->regions_of(cur->stop_key).size();
}

void GraphNavigator::mark_spoken(const GraphNode* node) {
  last_spoken_key_ = node ? std::optional<ControlId>(node->id) : std::nullopt;
  last_spoken_path_ = GraphAnnouncer::path_ids(node);
}

// The per-frame pull: the one place a landing is seated and announced, whatever caused it.
void GraphNavigator::ensure_focus() {
  if (!screen_ || !graph_) return;
  // The IDLE rebuild runs every kIdleRenderEvery-th frame: a full build each frame is costly on a long list, and
  // this per-frame pull only needs to catch content appearing, a focus the screen or the game moved, and
  // live-state flips. After input, an attach or invalidate() it rebuilds at once (render_due_). On the frames
  // between, the focused node's live parts are still watched against the cached render.
  int frame = host_.frame_count ? host_.frame_count() : 0;
  bool must_render = render_due_ || !state_->cur_key || pending_focus_;
  if (!must_render && frame - last_idle_render_ < kIdleRenderEvery) {
    if (const GraphNode* cached = graph_->current_node()) watch_live(cached);
    return;
  }
  last_idle_render_ = frame;
  render_due_ = false;

  if (!graph_->rerender()) return;  // no content yet; reconcile seats the start node once there is
  if (pending_focus_) {
    // One retry frame for a target focused mid-build; a target still not in the render was removed --
    // drop the request rather than re-seating every frame.
    if (graph_->current()->contains(*pending_focus_)) {
      graph_->focus(*pending_focus_);
      if (!pending_announce_) mark_spoken(graph_->current_node());
    }
    pending_focus_.reset();
  }

  const GraphNode* node = graph_->current_node();
  if (!node) return;
  bool interrupt = pending_interrupt_;
  pending_interrupt_ = false;
  if (!last_spoken_key_ || *last_spoken_key_ != node->id) {
    // Queued (not interrupting): landings follow the screen name / preceding feedback. A requested interrupting
    // landing (the game moved its own cursor) reads as a move.
    if (!host_.focus_mode_active || host_.focus_mode_active()) {
      if (auto t = compose_move(last_spoken_node(), node, !last_spoken_key_)) speak(*t, interrupt);
    }
    mark_spoken(node);
  }
  watch_live(node);
}

// Watch the FOCUSED node's Live parts and speak a part when its value changes (a setting's value, a
// description the game fills in late). Baselines silently whenever focus lands on a new identity.
void GraphNavigator::watch_live(const GraphNode* node) {
  if (!node->vtable) return;
  const std::vector<NodeAnnouncement> anns = node->vtable->announcements;   // a copy: a part's text runs the screen's code
  if (anns.empty()) return;
  bool baseline = !live_key_ || *live_key_ != node->id || live_values_.size() != anns.size();
  if (baseline) { live_key_ = node->id; live_values_.clear(); }
  for (size_t i = 0; i < anns.size(); ++i) {
    if (!anns[i].live) { if (baseline) live_values_.push_back(std::nullopt); continue; }
    std::optional<std::string> v;
    if (anns[i].text) { try { v = anns[i].text(); } catch (...) {} }
    if (baseline) { live_values_.push_back(v); continue; }
    if (live_values_[i] != v) {
      live_values_[i] = v;
      // A value the player just changed (left / right on a setting) answers at once; other live parts queue.
      if (v && !v->empty() && (!host_.focus_mode_active || host_.focus_mode_active())) speak(*v, anns[i].kind == announcement_kinds::kValue);
    }
  }
}

void GraphNavigator::announce_current() {
  if (!graph_) return;
  if (!graph_->rerender()) return;
  const GraphNode* node = graph_->current_node();
  if (!node) return;
  if (auto t = compose_move(nullptr, node, true)) speak(*t, false);
  mark_spoken(node);
}

void GraphNavigator::settle_silently() {
  if (!graph_ || !graph_->rerender()) return;
  mark_spoken(graph_->current_node());
}

// ---- input ----

std::optional<ControlId> GraphNavigator::focused_id() const {
  const GraphNode* n = graph_ ? graph_->current_node() : nullptr;
  if (!n) return std::nullopt;
  return n->id;
}

bool GraphNavigator::on_action(std::string_view key) {
  using namespace ui_actions;
  render_due_ = true;   // whatever the key did, the next pull sees the screen as it is now
  if (key == Up) return arrow(GraphDir::Up);
  if (key == Down) return arrow(GraphDir::Down);
  if (key == Left) return arrow(GraphDir::Left);
  if (key == Right) return arrow(GraphDir::Right);
  if (key == Next) return tab(1);
  if (key == Prev) return tab(-1);
  if (key == Home) return jump_edge(true);
  if (key == End) return jump_edge(false);
  const GraphNode* node = graph_ ? graph_->current_node() : nullptr;
  // Section jumps consume only when the focused node is IN a region; elsewhere the key is not taken.
  if (key == SectionPrev) return node && !node->region_key.empty() && section_jump(-1);
  if (key == SectionNext) return node && !node->region_key.empty() && section_jump(1);
  if (key == Activate) { if (!node) return false; vtable_activate(); return true; }
  return false;
}

bool GraphNavigator::arrow(GraphDir dir) {
  const GraphNode* focus_node = graph_ ? graph_->current_node() : nullptr;
  if (!focus_node) return false;
  // A focused control with on_adjust (a setting's value) adjusts on Left/Right (priority over any navigation).
  if (dir == GraphDir::Left || dir == GraphDir::Right)
    if (vtable_adjust(dir == GraphDir::Right ? 1 : -1)) return true;
  // Movement rides the edges (rows, lists, grids).
  MoveResult move = graph_->move(dir);
  if (move.moved) { land(move.to, move.from); return true; }
  focus_node = graph_->current_node();  // move() rerendered: the node captured above may be a stale render's
  if (!focus_node) return false;
  // Nothing moved: at the edge of the list, repeat the focused item rather than go silent. Only along
  // the list's own axis: Left/Right in a vertical list are not an edge, they are nothing -- and Up/Down in
  // a pure row likewise.
  // The press is not taken, so the host can hand it on (to the game's own list). A press something else
  // answers is not repeated: the repeat and then the answer were both said (the calendar's April 30, then
  // May 1, 2026-11-01).
  if (on_list_axis(focus_node, dir) && (!host_.edge_answered || !host_.edge_answered(dir))) repeat_focused();
  return false;
}

bool GraphNavigator::on_list_axis(const GraphNode* node, GraphDir dir) {
  bool vertical = node->has_transition(GraphDir::Up) || node->has_transition(GraphDir::Down);
  bool horizontal = node->has_transition(GraphDir::Left) || node->has_transition(GraphDir::Right);
  if (dir == GraphDir::Up || dir == GraphDir::Down) return vertical || !horizontal;  // a lone item counts as vertical
  return horizontal;
}

// The focused item's own readout again, as a fresh landing (hover sound, interrupting).
void GraphNavigator::repeat_focused() {
  const GraphNode* node = graph_ ? graph_->current_node() : nullptr;
  if (!node) return;
  play_hover();
  if (auto t = announcer_.leaf_text(node)) speak(*t, true);
  mark_spoken(node);
}

bool GraphNavigator::tab(int step) {
  // Snapshot BEFORE rerendering: reconcile seats a cursor that was not set yet at the start node, and a
  // Tab pressed before any focus was seated must enter at the first (or last) stop, not step from there.
  if (!graph_ || !state_) return false;
  bool was_unfocused = !state_->cur_key;
  if (!graph_->rerender()) return false;
  const std::vector<Key> stops = graph_->stops();
  if (stops.empty()) return false;
  const GraphNode* cur = was_unfocused ? nullptr : graph_->current_node();
  int idx = -1;
  if (cur) { auto it = std::find(stops.begin(), stops.end(), cur->stop_key); if (it != stops.end()) idx = (int)(it - stops.begin()); }
  if (idx < 0) return land_on_stop(stops[step >= 0 ? 0 : stops.size() - 1]);  // no focus yet: enter at first/last
  int ni = idx + step;
  int n = (int)stops.size();
  if (ni < 0 || ni >= n) {
    if (screen_ && screen_->wrap()) ni = ((ni % n) + n) % n;
    else return true;  // at the end; consume, no wrap
  }
  return land_on_stop(stops[(size_t)ni]);
}

// Remembered position -> SELECTED member -> first node (so Tab into a list lands on the item currently chosen).
bool GraphNavigator::land_on_stop(const Key& stop_key) {
  GraphNode* target = graph_->stop_landing(stop_key);
  if (!target || !graph_->focus(target->id)) return true;
  land(graph_->current_node(), last_spoken_node());
  return true;
}

void GraphNavigator::land(const GraphNode* to, const GraphNode* from) {
  if (!to) return;
  play_hover();
  if (auto t = compose_move(from, to, false)) speak(*t, true);
  mark_spoken(to);
}

void GraphNavigator::play_hover() {
  if (host_.hover_sound) host_.hover_sound();
}

bool GraphNavigator::jump_edge(bool first) {
  const GraphNode* focus_node = graph_ ? graph_->current_node() : nullptr;
  if (!focus_node) return false;
  // Home/End run along the list; in a row (no vertical edges) that is left/right.
  bool vertical = focus_node->has_transition(GraphDir::Up) || focus_node->has_transition(GraphDir::Down);
  MoveResult move = graph_->move_to_edge(vertical ? (first ? GraphDir::Up : GraphDir::Down) : (first ? GraphDir::Left : GraphDir::Right));
  if (move.moved) land(move.to, move.from); else repeat_focused();   // already there: say so by repeating it
  return true;
}

bool GraphNavigator::section_jump(int dir) {
  MoveResult r = graph_->move_region(dir);
  if (r.moved) land(r.to, r.from);
  return true;  // one section only: consumed, nothing said
}

// Run the focused node's activation; speak its state_text as immediate feedback when it declares one,
// and rebaseline the live watch so the same change is not spoken twice. The rebaseline must see the
// post-activation values: the current render still carries the old ones (its announcements captured them
// at build time), so rerender first -- otherwise the next tick baselines on the stale render and then
// announces the change a second time.
void GraphNavigator::feedback_after_change() {
  const GraphNode* node = graph_->current_node();
  if (!node || !node->vtable || !node->vtable->state_text) return;
  speak(node->vtable->state_text(), true);
  graph_->rerender();
  live_key_.reset();
}
bool GraphNavigator::vtable_activate() {
  const GraphNode* node = graph_->current_node();
  if (!node || !node->vtable || !node->vtable->on_activate) return false;
  graph_->activate();
  feedback_after_change();
  return true;
}

bool GraphNavigator::vtable_adjust(int sign) {
  const GraphNode* node = graph_->current_node();
  if (!node || !node->vtable || !node->vtable->on_adjust) return false;
  graph_->try_adjust(sign, false);
  feedback_after_change();
  return true;
}

std::optional<std::string> GraphNavigator::compose_move(const GraphNode* from, const GraphNode* to, bool entry) {
  if (entry) return announcer_.compose(nullptr, to);
  // The node last spoken may have vanished (its row was removed by the action it just ran): its remembered path
  // still says which levels we were inside, so the landing reads as a sibling move, not as an entry that
  // repeats the list's title.
  if (from == nullptr && last_spoken_key_ && !last_spoken_path_.empty()) return announcer_.compose_from_path(last_spoken_path_, to);
  return announcer_.compose(from, to);
}

std::string GraphNavigator::dump() {
  if (!graph_ || !screen_) return "(no screen)\n";
  if (!graph_->rerender()) return "(no content)\n";
  std::string out;
  const GraphNode* cur = graph_->current_node();
  Key last_stop;
  for (const GraphNode* n : graph_->current()->order()) {
    if (n->stop_key != last_stop) { out += "-- stop " + n->stop_key.to_string() + "\n"; last_stop = n->stop_key; }
    auto t = announcer_.leaf_text(n);
    out += std::string(n == cur ? "> " : "  ") + (t ? *t : std::string("(silent)")) + "\n";
  }
  return out;
}

}  // namespace p3a::ui

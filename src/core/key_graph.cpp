// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#include "core/key_graph.h"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace p3a::ui {
namespace {

int index_of(const std::vector<ControlId>& order, const ControlId& key) {
  for (std::size_t i = 0; i < order.size(); i++)
    if (order[i] == key) return static_cast<int>(i);
  return -1;
}

int index_of(const std::vector<Key>& keys, const Key& key) {
  for (std::size_t i = 0; i < keys.size(); i++)
    if (keys[i] == key) return static_cast<int>(i);
  return -1;
}

// A survivor at a previous-order index belonging to the remembered landing group, else none.
std::optional<ControlId> group_survivor_at(const GraphRender& render, const GraphState& state, int index) {
  if (index < 0 || index >= static_cast<int>(state.key_order.size())) return std::nullopt;
  GraphNode* n = render.node_at(state.key_order[static_cast<std::size_t>(index)]);
  if (n == nullptr) return std::nullopt;
  if (n->vtable != nullptr && n->vtable->land_group == state.last_land_group) return n->id;
  return std::nullopt;
}

// Track the focused node's landing group (empty when it has none) for the row-vanish rule.
void remember_land_group(const GraphRender& render, GraphState& state, const std::optional<ControlId>& key) {
  GraphNode* node = render.node_at(key);
  if (node != nullptr) state.last_land_group = node->vtable != nullptr ? node->vtable->land_group : std::string();
}

void remember_stop(const GraphRender& render, GraphState& state, const std::optional<ControlId>& key) {
  GraphNode* node = render.node_at(key);
  if (node != nullptr && !node->stop_key.empty()) state.stop_memory.insert_or_assign(node->stop_key, *key);
}

}  // namespace

bool KeyGraph::rerender() {
  // Retire the render one generation at a time: callers legitimately keep reading nodes they read
  // before the rebuild.
  previous_ = std::move(current_);
  current_ = render_callback_();
  if (current_ == nullptr || current_->node_count() == 0) {
    current_ = nullptr;
    return false;
  }
  reconcile(*current_, *state_);
  return true;
}

void KeyGraph::reconcile(GraphRender& render, GraphState& state) {
  const std::optional<ControlId> old = state.cur_key;
  std::optional<ControlId> resolved;

  if (old) {
    // The same control: its id is in this render too.
    GraphNode* same = render.node_at(*old);
    if (same != nullptr) resolved = same->id;

    // It vanished -- the GENERAL row-vanish rule: land on the nearest survivor OF THE SAME LANDING GROUP
    // (this list's rows), next-first then previous -- never a header, a lead line or another panel. Only
    // nodes stamped with a land_group participate; the vanished node's group was remembered at focus time.
    const int old_index = !resolved && !state.key_order.empty() ? index_of(state.key_order, *old) : -1;
    if (old_index >= 0 && !state.last_land_group.empty()) {
      const int count = static_cast<int>(state.key_order.size());
      for (int off = 1; off < count && !resolved; off++) {
        resolved = group_survivor_at(render, state, old_index + off);  // next row first
        if (!resolved) resolved = group_survivor_at(render, state, old_index - off);
      }
    }

    // Fallback: the nearest survivor walking the previous order backward.
    if (!resolved && old_index >= 0)
      for (int i = old_index; i >= 0; i--) {
        GraphNode* survivor = render.node_at(state.key_order[static_cast<std::size_t>(i)]);
        if (survivor != nullptr) {
          resolved = survivor->id;
          break;
        }
      }
  }

  // Nothing matched (or first render): the start node -- but prefer the SELECTED member of its stop
  // (initial focus lands on the item currently chosen, not the top of a long list).
  if (!resolved) {
    GraphNode* start_node = render.node_at(render.start_key);
    GraphNode* sel = start_node != nullptr ? selected_node_in_stop(render, start_node->stop_key, start_node) : nullptr;
    if (sel != nullptr) resolved = sel->id;
    else if (start_node != nullptr) resolved = start_node->id;
    else resolved = render.start_key;  // a hand-built render with no start key leaves focus unset
  }

  state.cur_key = resolved;
  remember_stop(render, state, resolved);
  remember_land_group(render, state, resolved);
  state.key_order = compute_order(render);
}

std::vector<ControlId> KeyGraph::compute_order(const GraphRender& render) {
  std::vector<ControlId> order;
  std::unordered_set<ControlId> seen;
  std::vector<ControlId> down_fringe;
  if (render.start_key) down_fringe.push_back(*render.start_key);

  std::size_t i = 0;
  while (i < down_fringe.size()) {
    ControlId k = down_fringe[i];
    while (seen.count(k) == 0) {
      seen.insert(k);
      order.push_back(k);

      GraphNode* n = render.node_at(k);
      if (n == nullptr) break;

      const Transition* d = n->transition(GraphDir::Down);
      if (d != nullptr) down_fringe.push_back(d->destination);
      const Transition* t = n->transition(GraphDir::Right);
      if (t == nullptr) break;
      k = t->destination;
    }
    i++;
  }

  for (GraphNode* node : render.order())
    if (seen.insert(node->id).second) order.push_back(node->id);

  return order;
}

void KeyGraph::set_current(GraphNode* node) {
  state_->cur_key = node->id;
  if (!node->stop_key.empty()) state_->stop_memory.insert_or_assign(node->stop_key, node->id);
  // Row-vanish rule (see reconcile).
  state_->last_land_group = node->vtable != nullptr ? node->vtable->land_group : std::string();
}

// ---- stops and regions ----

std::vector<Key> KeyGraph::stops() const {
  std::vector<Key> stops;
  if (current_ == nullptr) return stops;
  for (GraphNode* n : current_->order())
    if (!n->stop_key.empty() && std::find(stops.begin(), stops.end(), n->stop_key) == stops.end()) stops.push_back(n->stop_key);
  return stops;
}

std::vector<Key> KeyGraph::regions_of(const Key& stop_key) const {
  std::vector<Key> regions;
  if (current_ == nullptr) return regions;
  for (GraphNode* n : current_->order())
    if (n->stop_key == stop_key && !n->region_key.empty() && std::find(regions.begin(), regions.end(), n->region_key) == regions.end())
      regions.push_back(n->region_key);
  return regions;
}

// ---- navigation operations ----

MoveResult KeyGraph::move(GraphDir dir) {
  MoveResult result;
  if (!rerender()) return result;

  GraphNode* node = current_node();
  result.from = node;
  result.to = node;
  if (node == nullptr) return result;

  const Transition* t = node->transition(dir);
  GraphNode* dest = t != nullptr ? current_->node_at(t->destination) : nullptr;
  if (dest == nullptr || dest == node) return result;

  set_current(dest);
  result.to = dest;
  result.moved = true;
  return result;
}

MoveResult KeyGraph::move_to_edge(GraphDir dir) {
  MoveResult result;
  if (!rerender()) return result;

  GraphNode* node = current_node();
  result.from = node;
  result.to = node;
  if (node == nullptr) return result;

  // A cycle in `dir` (raw edges stitched into a list) froze the game in this loop (End, 2026-10-31): stop at the
  // first node seen twice, and never walk more nodes than the render has.
  GraphNode* cur = node;
  std::unordered_set<const GraphNode*> seen{node};
  for (std::size_t steps = 0; steps <= current_->node_count(); ++steps) {
    const Transition* t = cur->transition(dir);
    if (t == nullptr) break;
    GraphNode* next = current_->node_at(t->destination);
    if (next == nullptr || !seen.insert(next).second) break;
    cur = next;
  }

  if (cur != node) {
    set_current(cur);
    result.to = cur;
    result.moved = true;
  }
  return result;
}

MoveResult KeyGraph::move_region(int dir) {
  MoveResult result;
  if (!rerender()) return result;

  GraphNode* node = current_node();
  result.from = node;
  result.to = node;
  if (node == nullptr || node->region_key.empty()) return result;

  const std::vector<Key> regions = regions_of(node->stop_key);
  const int idx = index_of(regions, node->region_key);
  const int count = static_cast<int>(regions.size());
  if (idx < 0 || count < 2) return result;
  const int ni = (((idx + dir) % count) + count) % count;
  if (ni == idx) return result;

  for (GraphNode* n : current_->order())
    if (n->stop_key == node->stop_key && n->region_key == regions[static_cast<std::size_t>(ni)]) {
      set_current(n);
      result.to = n;
      result.moved = true;
      return result;
    }
  return result;
}

bool KeyGraph::focus(const ControlId& id) {
  if (!rerender()) return false;
  GraphNode* node = current_->node_at(id);
  if (node == nullptr) return false;
  set_current(node);
  return true;
}

GraphNode* KeyGraph::stop_landing(const Key& stop_key) const {
  return stop_landing(*current_, *state_, stop_key);
}

GraphNode* KeyGraph::stop_landing(const GraphRender& render, const GraphState& state, const Key& stop_key) {
  auto it = state.stop_memory.find(stop_key);
  if (it != state.stop_memory.end()) {
    GraphNode* node = render.node_at(it->second);
    if (node != nullptr && node->stop_key == stop_key) return node;
  }
  GraphNode* first = nullptr;
  for (GraphNode* n : render.order())
    if (n->stop_key == stop_key) { first = n; break; }
  if (first == nullptr) return nullptr;
  GraphNode* selected = selected_node_in_stop(render, stop_key, first);
  return selected != nullptr ? selected : first;
}

GraphNode* KeyGraph::selected_node_in_stop(const GraphRender& render, const Key& stop_key, const GraphNode* sibling_of) {
  for (GraphNode* n : render.order()) {
    if (!(n->stop_key == stop_key)) continue;
    if (sibling_of != nullptr && n->parent != sibling_of->parent) continue;
    if (n->vtable == nullptr) continue;
    for (const NodeAnnouncement& a : n->vtable->announcements) {
      if (std::string_view(a.kind) != announcement_kinds::kSelected) continue;
      std::string t;
      // A part's text resolver runs game code; a throwing one must not break stop entry.
      try {
        if (a.text) t = a.text();
      } catch (...) {
      }
      if (!t.empty()) return n;
    }
  }
  return nullptr;
}

// ---- behavior invokers ----

bool KeyGraph::activate() {
  if (!rerender()) return false;
  GraphNode* node = current_node();
  if (node == nullptr || node->vtable == nullptr || !node->vtable->on_activate) return false;
  node->vtable->on_activate();
  return true;
}

bool KeyGraph::try_adjust(int sign, bool large) {
  if (!rerender()) return false;
  GraphNode* node = current_node();
  if (node == nullptr || node->vtable == nullptr || !node->vtable->on_adjust) return false;
  node->vtable->on_adjust(sign, large);
  return true;
}

}  // namespace p3a::ui

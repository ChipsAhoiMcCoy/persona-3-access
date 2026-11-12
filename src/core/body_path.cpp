#include "core/body_path.h"
#include <cmath>
#include <cstdint>
#include <queue>
#include <unordered_map>

namespace p3a::core {

BodyPlan body_path(Vec2 from, Vec2 goal, float cell, float yaw, int max_expand, float reach,
                   const std::function<bool(Vec2, Vec2)>& clear, const std::function<bool(Vec2)>& arrived) {
  BodyPlan plan;
  const float c = std::cos(yaw), s = std::sin(yaw);
  // Grid (i, j): i along the turned x axis, j along the turned y axis, (0, 0) at the start.
  auto at = [&](int i, int j) { float u = (float)i * cell, v = (float)j * cell; return Vec2{from.x + u * c - v * s, from.y + u * s + v * c}; };
  auto key = [](int i, int j) { return ((uint64_t)(uint32_t)i << 32) | (uint32_t)j; };
  auto h = [&](Vec2 p) { return std::hypot(goal.x - p.x, goal.y - p.y); };
  struct Node { float g = 0; uint64_t parent = 0; bool closed = false; bool root = false; };
  std::unordered_map<uint64_t, Node> nodes;
  struct Open { float f; float g; int i, j; };
  auto later = [](const Open& a, const Open& b) { return a.f > b.f; };
  std::priority_queue<Open, std::vector<Open>, decltype(later)> open(later);
  nodes[key(0, 0)] = Node{0, 0, false, true};
  open.push(Open{h(from), 0, 0, 0});
  static const int di[8] = {1, 0, -1, 0, 1, 1, -1, -1}, dj[8] = {0, 1, 0, -1, 1, -1, 1, -1};
  const float diag = cell * 1.41421356f;
  while (!open.empty()) {
    Open o = open.top();
    open.pop();
    Node& n = nodes[key(o.i, o.j)];
    if (n.closed || o.g > n.g) continue;
    n.closed = true;
    Vec2 p = at(o.i, o.j);
    if (arrived(p)) {
      // Back along the parents, keeping only the points where the direction turns.
      std::vector<std::pair<int, int>> cells;
      for (uint64_t k = key(o.i, o.j);;) {
        cells.emplace_back((int)(int32_t)(k >> 32), (int)(int32_t)(k & 0xffffffffu));
        const Node& m = nodes[k];
        if (m.root) break;
        k = m.parent;
      }
      plan.found = true;
      for (size_t a = cells.size(); a-- > 0;) {
        bool turn = a == 0 || a + 1 == cells.size();
        if (!turn) {
          auto [pi, pj] = cells[a + 1];
          auto [ci, cj] = cells[a];
          auto [ni, nj] = cells[a - 1];
          turn = (ci - pi) != (ni - ci) || (cj - pj) != (nj - cj);
        }
        if (turn) plan.points.push_back(at(cells[a].first, cells[a].second));
      }
      plan.points.front() = from;
      // A grid's diagonal runs as a staircase of short legs: from each point, on to the furthest later one the body
      // reaches straight. Through a narrow gap only the straight legs are clear, so those stay.
      std::vector<Vec2> pulled{plan.points.front()};
      for (size_t a = 0; a + 1 < plan.points.size();) {
        size_t b = plan.points.size() - 1;
        while (b > a + 1 && !(++plan.tested, clear(plan.points[a], plan.points[b]))) --b;
        pulled.push_back(plan.points[b]);
        a = b;
      }
      plan.points = std::move(pulled);
      return plan;
    }
    if (++plan.expanded > max_expand) break;
    for (int d = 0; d < 8; ++d) {
      int i = o.i + di[d], j = o.j + dj[d];
      Vec2 q = at(i, j);
      if (std::hypot(q.x - from.x, q.y - from.y) > reach) continue;
      float g = o.g + (d < 4 ? cell : diag);
      auto it = nodes.find(key(i, j));
      if (it != nodes.end() && (it->second.closed || it->second.g <= g)) continue;
      ++plan.tested;
      if (!clear(p, q)) continue;
      Node& m = nodes[key(i, j)];
      m.g = g;
      m.parent = key(o.i, o.j);
      m.root = false;
      open.push(Open{g + h(q), g, i, j});
    }
  }
  return plan;
}

}  // namespace p3a::core

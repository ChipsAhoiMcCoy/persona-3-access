#pragma once
#include <vector>

namespace p3a::core {
// Things placed on a screen, in the order a page is read: top to bottom by rows, each row left to right. A row is
// every spot within `row_height` below the row's topmost one (measured from that one, not from its neighbour).
struct Spot { int id; float x, y; };
std::vector<Spot> reading_order(std::vector<Spot> spots, float row_height);
}  // namespace p3a::core

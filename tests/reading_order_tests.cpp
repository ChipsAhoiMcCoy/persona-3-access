#include "doctest.h"
#include "core/reading_order.h"
#include <vector>

static std::vector<int> ids(const std::vector<p3a::core::Spot>& ps) {
  std::vector<int> out;
  for (auto& p : ps) out.push_back(p.id);
  return out;
}

TEST_CASE("reading_order: rows top to bottom, each left to right") {
  using p3a::core::reading_order;
  // The town map's six places: four along the top, one in the middle, one at the bottom.
  std::vector<p3a::core::Spot> town = {{0, 1353, 489}, {1, 250, 198}, {2, 1100, 250}, {3, 1000, 700}, {4, 600, 195}, {5, 839, 200}};
  CHECK(ids(reading_order(town, 80)) == std::vector<int>{1, 4, 5, 2, 0, 3});
}
TEST_CASE("reading_order: a row is measured from its topmost spot") {
  using p3a::core::reading_order;
  // 100, 150 and 190 would chain 50 apart; the row from 100 ends before 190.
  std::vector<p3a::core::Spot> chain = {{0, 30, 190}, {1, 20, 150}, {2, 10, 100}, {3, 5, 150}};
  CHECK(ids(reading_order(chain, 80)) == std::vector<int>{3, 2, 1, 0});
  CHECK(ids(reading_order(chain, 100)) == std::vector<int>{3, 2, 1, 0});
  CHECK(ids(reading_order(chain, 1)) == std::vector<int>{2, 3, 1, 0});
}
TEST_CASE("reading_order: nothing, one, and a single row") {
  using p3a::core::reading_order;
  CHECK(reading_order({}, 80).empty());
  CHECK(ids(reading_order({{7, 3, 4}}, 80)) == std::vector<int>{7});
  CHECK(ids(reading_order({{0, 300, 10}, {1, 100, 12}, {2, 200, 8}}, 80)) == std::vector<int>{1, 2, 0});
}

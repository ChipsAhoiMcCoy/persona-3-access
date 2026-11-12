#include "doctest.h"
#include "core/sonar_sweep.h"

using namespace p3a::core;

TEST_CASE("sweep: west to east, one at a time, then a rest") {
  SonarSweep s;
  std::vector<SonarSweep::Item> items = {{1, 4.0f, 0}, {2, -3.0f, 1}, {3, 0.5f, 2}};
  auto p = s.update(items, 0.0);
  REQUIRE(p.size() == 1);
  CHECK(p[0].id == 2);   // the westmost first
  CHECK(s.update(items, 0.1).empty());   // gap for three: 0.75 / 3 = 0.25 clamped to 0.20
  p = s.update(items, 0.2);
  REQUIRE(p.size() == 1);
  CHECK(p[0].id == 3);
  p = s.update(items, 0.4);
  REQUIRE(p.size() == 1);
  CHECK(p[0].id == 1);
  CHECK(s.update(items, 0.7).empty());   // the rest: 0.4 s
  p = s.update(items, 0.8);
  REQUIRE(p.size() == 1);
  CHECK(p[0].id == 2);   // the next sweep
}

TEST_CASE("sweep: a thing gone when its turn comes is skipped") {
  SonarSweep s;
  std::vector<SonarSweep::Item> items = {{1, -1.0f, 0}, {2, 0.0f, 0}, {3, 1.0f, 0}};
  CHECK(s.update(items, 0.0)[0].id == 1);
  std::vector<SonarSweep::Item> fewer = {{1, -1.0f, 0}, {3, 1.0f, 0}};
  auto p = s.update(fewer, 0.2);
  REQUIRE(p.size() == 1);
  CHECK(p[0].id == 3);
}

TEST_CASE("sweep: the gap shrinks with the crowd, within its bounds") {
  SweepParams p;
  CHECK(p.gap_for(1) == doctest::Approx(0.20));
  CHECK(p.gap_for(5) == doctest::Approx(0.15));
  CHECK(p.gap_for(20) == doctest::Approx(0.10));
}

TEST_CASE("sweep_gain: full up close, never under the floor") {
  CHECK(sweep_gain(0) == doctest::Approx(1.0f));
  CHECK(sweep_gain(3) == doctest::Approx(0.5f));
  CHECK(sweep_gain(1000) == doctest::Approx(0.08f));
}

TEST_CASE("sweep: nothing in range waits a rest before looking again") {
  SonarSweep s;
  CHECK(s.update({}, 0.0).empty());
  std::vector<SonarSweep::Item> items = {{7, 0.0f, 0}};
  CHECK(s.update(items, 0.2).empty());
  CHECK(s.update(items, 0.4).size() == 1);
}

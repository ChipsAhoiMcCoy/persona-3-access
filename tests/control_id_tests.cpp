// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
// Key and ControlId: the identities the navigation graph keeps its nodes, stops and regions by.
#include "doctest.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "core/control_id.h"

using p3a::ui::ControlId;
using p3a::ui::Key;

TEST_CASE("a key is empty or a string, and an empty string is still a key") {
  const Key none;
  CHECK(none.empty());
  CHECK_FALSE(none.is_string());
  CHECK(none.text().empty());
  CHECK(none.to_string().empty());

  const Key blank("");
  CHECK_FALSE(blank.empty());
  CHECK(blank.is_string());
  CHECK(none != blank);          // "no key" and the key "" are told apart
  CHECK(none == Key());

  const Key a("personas#3");
  CHECK(a.is_string());
  CHECK(a.text() == "personas#3");
  CHECK(a.to_string() == "personas#3");
  CHECK(a == Key(std::string("personas#3")));
  CHECK(a == Key(std::string_view("personas#3")));
  CHECK(a != Key("personas#4"));
}

TEST_CASE("keys hash by their text, so they key a map") {
  CHECK(Key("a").hash() == Key(std::string("a")).hash());
  CHECK(Key().hash() == Key().hash());
  std::unordered_map<Key, int> stops;
  stops[Key("tabs")] = 1;
  stops[Key("page")] = 2;
  stops[Key("tabs")] = 3;
  CHECK(stops.size() == 2);
  CHECK(stops.at(Key("tabs")) == 3);
  CHECK(stops.count(Key("nowhere")) == 0);
}

TEST_CASE("a control id is its structural key") {
  const ControlId a = ControlId::structural("items#0");
  const ControlId same = ControlId::structural(std::string("items#0"));
  const ControlId other = ControlId::structural("items#1");
  CHECK(a == same);
  CHECK(a != other);
  CHECK(a.structural_key() == Key("items#0"));
  CHECK(a.structural_key().text() == "items#0");
  CHECK(a.to_string() == "ControlId(items#0)");

  std::unordered_set<ControlId> ids;
  CHECK(ids.insert(a).second);
  CHECK_FALSE(ids.insert(same).second);   // the same control, declared twice
  CHECK(ids.insert(other).second);
}

TEST_CASE("a control id needs a key") {
  CHECK_THROWS_AS(ControlId::structural(Key()), std::invalid_argument);
  CHECK_NOTHROW(ControlId::structural(Key("")));
}

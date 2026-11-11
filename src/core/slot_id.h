#pragma once
// The ids of a menu list's items: "<list>#<index>" for a row, "<list>#<index>.<n>" for the nth line of the walk to its
// right. The list's key may itself hold dots and numbers ("equip.gear.2.0").
#include <string>
#include <string_view>

namespace p3a::core {
// The id of row `i` of list `key`.
inline std::string slot_id(std::string_view key, int i) { return std::string(key) + "#" + std::to_string(i); }

// A row's id back into its list and index: "list#3" -> ("list", 3). False for anything else. For an id with something
// after the index (a walk line, "list#3.1") it is false too, but `key` and `i` then hold the row the line belongs to
// ("list", 3): what was read up to the character that is no digit.
inline bool parse_slot(std::string_view s, std::string& key, int& i) {
  size_t at = s.rfind('#');
  if (at == std::string_view::npos || at + 1 >= s.size()) return false;
  key = std::string(s.substr(0, at));
  i = 0;
  for (size_t j = at + 1; j < s.size(); ++j) {
    if (s[j] < '0' || s[j] > '9') return false;
    i = i * 10 + (s[j] - '0');
  }
  return true;
}

// The row of list `key` that the id `s` is on: the row itself or a line of its walk. -1 for an id of another list.
inline int slot_row(std::string_view s, std::string_view key) {
  if (s.size() <= key.size() + 1 || s.compare(0, key.size(), key) != 0 || s[key.size()] != '#') return -1;
  int i = 0;
  size_t j = key.size() + 1;
  for (; j < s.size() && s[j] >= '0' && s[j] <= '9'; ++j) i = i * 10 + (s[j] - '0');
  return j > key.size() + 1 && (j == s.size() || s[j] == '.') ? i : -1;
}
}  // namespace p3a::core

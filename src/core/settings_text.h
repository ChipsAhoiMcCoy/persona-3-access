#pragma once
#include <string>
#include <vector>

namespace p3a::core {
// One line of the settings file, "key=12" (blanks before the key and around the "=" are fine; anything after the
// number is ignored): true with the key and the value, false for a line that is not one (no "=", no number, a key
// with a blank inside it).
bool parse_setting(const char* line, std::string& key, int& value);
// Where `value` is in the list of values an option offers; the first place when it is not there (the last of two
// equal ones).
int value_index(const std::vector<int>& values, int value);
// The place an option moves to from `at` in a list of `count` values: delta 0 steps forward and wraps past the end,
// +1 / -1 step and stop at the ends. The same place when nothing moves.
int step_index(int at, int count, int delta);
}  // namespace p3a::core

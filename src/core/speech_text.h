#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace p3a::core {
// The small pieces spoken lines are put together from, the same way wherever a line is built. UTF-8 in and out; the
// functions only look at ASCII punctuation and digits, so other bytes pass through untouched.

// "Yes, 1 of 2": a line with its place in a list or a run of pages (`place` counts from 1).
std::string position(std::string_view text, size_t place, size_t count);
// "Yukari: Hello." -- a line with who says it; the text alone when nobody is named.
std::string spoken_line(std::string_view speaker, std::string_view text);
// An entry without its own full stop ("Nice to meet you." -> "Nice to meet you": joined to a position, the stop was
// read as a dot); an ellipsis stays, and so does a lone ".".
std::string without_own_stop(std::string s);
// Whether a list has entries and every one has its text (a list's entries are empty for a frame or two as it opens).
bool all_filled(const std::vector<std::string>& entries);
// A line with each run of digits as one '#': two readings of a countdown ("...in 9 seconds", "...in 10 seconds")
// compare equal.
std::string no_digits(std::string_view s);
// A line that ends with a stop: a full stop is added unless it ends with '.', '?' or '!' already; "" stays "".
std::string sentence(std::string s);
}  // namespace p3a::core

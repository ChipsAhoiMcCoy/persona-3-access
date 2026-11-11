#pragma once
#include <map>
#include <string>
#include <string_view>

namespace p3a::core {
// A request to the dev server, as its text: the path, the query's keys and values, and a POST body.
using Query = std::map<std::string, std::string>;
struct Request { std::string path; Query q; std::string body; };

// "%41" -> "A". "+" is NOT a space here: every client encodes spaces as %20, and key chords (k=w+d) arrive intact.
std::string url_decode(std::string_view s);
// "a=1&b=x%20y&c" -> {a: "1", b: "x y", c: "1"}: a key without "=" reads as "1"; a later key replaces an earlier one.
Query parse_query(std::string_view q);
// "/world?range=300" or "screen" -> the path (given its leading slash when it has none) and the parsed query.
Request parse_target(const std::string& target);
// A query value as an integer: hexadecimal when it starts with "0x" or "-0x", else decimal (0 for anything else).
long long parse_int(const std::string& s);
// Case-insensitive substring (ASCII). An empty needle is found in any text but an empty one.
bool contains_ci(std::string_view haystack, std::string_view needle);
}  // namespace p3a::core

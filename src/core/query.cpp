#include "core/query.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace p3a::core {
std::string url_decode(std::string_view s) {
  std::string o;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size()) { o += (char)strtol(std::string(s.substr(i + 1, 2)).c_str(), nullptr, 16); i += 2; }
    else o += s[i];
  }
  return o;
}

Query parse_query(std::string_view q) {
  Query m;
  size_t p = 0;
  while (p <= q.size()) {
    size_t e = q.find('&', p); if (e == std::string_view::npos) e = q.size();
    std::string_view kv = q.substr(p, e - p);
    size_t eq = kv.find('=');
    if (!kv.empty()) m[url_decode(kv.substr(0, eq))] = eq == std::string_view::npos ? "1" : url_decode(kv.substr(eq + 1));
    p = e + 1;
  }
  return m;
}

Request parse_target(const std::string& target) {
  Request req;
  size_t qm = target.find('?');
  req.path = target.substr(0, qm);
  if (req.path.empty() || req.path[0] != '/') req.path = "/" + req.path;
  if (qm != std::string::npos) req.q = parse_query(std::string_view(target).substr(qm + 1));
  return req;
}

long long parse_int(const std::string& s) {
  return strtoll(s.c_str(), nullptr, s.rfind("0x", 0) == 0 || s.rfind("-0x", 0) == 0 ? 16 : 10);
}

bool contains_ci(std::string_view h, std::string_view n) {
  return std::search(h.begin(), h.end(), n.begin(), n.end(),
                     [](char a, char b) { return tolower((unsigned char)a) == tolower((unsigned char)b); }) != h.end();
}
}  // namespace p3a::core

#include "core/settings_text.h"
#include <algorithm>
#include <cstdio>

namespace p3a::core {
bool parse_setting(const char* line, std::string& key, int& value) {
  char k[128]; int v;
  if (sscanf(line, " %127[^= ] = %d", k, &v) != 2) return false;
  key = k;
  value = v;
  return true;
}

int value_index(const std::vector<int>& values, int value) {
  int at = 0;
  for (size_t i = 0; i < values.size(); ++i) if (values[i] == value) at = (int)i;
  return at;
}

int step_index(int at, int count, int delta) {
  if (count <= 0) return at;
  return delta == 0 ? (at + 1) % count : std::clamp(at + delta, 0, count - 1);
}
}  // namespace p3a::core

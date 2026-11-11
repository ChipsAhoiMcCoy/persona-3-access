#include "core/key_name.h"
#include "core/strings.h"
#include <format>

namespace p3a::core {
std::string config_key_name(int v) {
  static const char* kRows = "QWERTYUIOPASDFGHJKLZXCVBNM";
  if (v >= 431 && v <= 440) return std::string(1, kRows[v - 431]);
  if (v >= 441 && v <= 449) return std::string(1, kRows[10 + v - 441]);
  if (v >= 450 && v <= 456) return std::string(1, kRows[19 + v - 450]);
  if (v == 469) return strings::kKeySpace;
  return std::format("{} {}", strings::kKeyCode, v);
}
}  // namespace p3a::core

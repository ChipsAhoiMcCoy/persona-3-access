#pragma once
#include <cstdint>
#include <string_view>
#include <functional>

namespace p3a::scan {
// Byte-pattern search over P3R.exe's executable sections ("48 8B ?? 05", ?? = any byte). Returns 0 when absent.
uintptr_t find(std::string_view pattern);
// Same, but fails (returns 0) unless the pattern matches exactly once.
uintptr_t find_unique(std::string_view pattern);
// Resolve a rip-relative operand: the int32 at `at` is relative to the end of the 4 bytes.
inline uintptr_t rip(uintptr_t at) { return at + 4 + *(const int32_t*)at; }
// Calls hit(address) for each match until it returns false.
void each_match(std::string_view pattern, const std::function<bool(uintptr_t)>& hit);
uintptr_t exe_base();
uint32_t exe_timestamp();
}  // namespace p3a::scan

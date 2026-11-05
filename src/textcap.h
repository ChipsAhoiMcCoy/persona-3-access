#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace p3a::textcap {
// Captures strings the game draws through its native UI font routines (AUIDrawBaseActor::DrawFont / DrawFont2 /
// FontDrawEx). A dev discovery tool (/text) and the last-resort name source; screens read the game's own objects.
struct Item { float x, y; uint32_t color; std::string text; uint64_t seen_ms; int source; };
bool install();
void remove();
std::vector<Item> snapshot(unsigned max_age_ms = 150);   // drawn within the last max_age_ms, sorted top-to-bottom
std::string status();
}  // namespace p3a::textcap

#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace p3a::textcap {
// Captures strings the game draws through its native UI font routines (AUIDrawBaseActor::DrawFont / DrawFont2 /
// FontDrawEx). A dev discovery tool (/text) and the last-resort source: screens read the game's own objects, and
// come here only for rows that exist nowhere but drawn (the town map's info, which entries of a SELECT list show).
struct Item { float x, y; uint32_t color; std::string text; uint64_t seen_frame; int source; };
bool install();
void remove();
// What was drawn within the last `frames` frames of the mod's tick (one a game frame), sorted top to bottom. A row
// the game draws every frame is always 0 or 1 frame old; the count is frames the game ran, never time: a paused or
// stalled game ages nothing.
std::vector<Item> snapshot(unsigned frames = 9);
std::string status();
}  // namespace p3a::textcap

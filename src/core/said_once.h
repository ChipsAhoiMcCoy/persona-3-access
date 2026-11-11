#pragma once
// A speaker that never repeats itself on its own: a line it has said is not said again until the player presses
// something or something else speaks. A cursor flickering between two values, or a value that alternates, would
// otherwise read the same words over and over. Nothing here counts time: the three numbers it is given are counters
// (or stamps) that only need to change when their event happens.
#include <cstdint>
#include <set>
#include <string>
#include <string_view>

namespace p3a::core {
class SaidOnce {
 public:
  // Whether `text` is to be said now. `moves` and `pad` change with the player's presses (keys, the controller);
  // `spoken` is how many lines anyone has spoken so far. A change in any of them since the last line forgets what was
  // said.
  bool admit(std::string_view text, uint64_t moves, uint64_t pad, uint64_t spoken) {
    if (moves_ != moves || pad_ != pad || spoken_ != spoken) { said_.clear(); moves_ = moves; pad_ = pad; }
    return said_.emplace(text).second;
  }
  // The admitted line was spoken: `spoken` is the count with it, so that it is not taken for someone else's.
  void spoke(uint64_t spoken) { spoken_ = spoken; }

 private:
  std::set<std::string, std::less<>> said_;
  uint64_t moves_ = 0, pad_ = 0, spoken_ = 0;
};
}  // namespace p3a::core

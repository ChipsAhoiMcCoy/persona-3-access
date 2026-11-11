#pragma once
#include <string>

namespace p3a::core {
// The name of a key from the game's key configuration, by the code the configuration holds for it: the letters run
// in keyboard rows from 431 (Q..P 431-440, A..L 441-449, Z..M 450-456) and Space is 469. Any other code is said as
// "key code N".
std::string config_key_name(int code);
}  // namespace p3a::core

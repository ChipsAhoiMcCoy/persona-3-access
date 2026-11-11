#pragma once
#include <string>

namespace p3a {
bool env_flag(const wchar_t* name);
int env_int(const wchar_t* name, int def);
}  // namespace p3a

namespace p3a::mod {
// Everything above the base layer: the Unreal reflection bootstrap, the frame hook, the screens and the world.
void init(bool speech_ok);
void shutdown();
std::string status();
}  // namespace p3a::mod

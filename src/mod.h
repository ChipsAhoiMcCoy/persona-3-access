#pragma once
#include <string>

namespace p3a {
bool env_flag(const wchar_t* name);
int env_int(const wchar_t* name, int def);
std::wstring mod_dir();   // the folder p3access.dll was loaded from, with a trailing backslash
}  // namespace p3a

namespace p3a::mod {
// Everything above the base layer: the Unreal reflection bootstrap, the frame hook, the screens and the world.
void init(bool speech_ok);
void shutdown();
std::string status();
}  // namespace p3a::mod

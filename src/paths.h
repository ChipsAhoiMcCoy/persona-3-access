#pragma once
#include <windows.h>
#include <string>

// Where the mod's own DLL is: its companions (prism.dll, phonon.dll, the assets folder) sit beside it, and that folder
// is not on the game's DLL search path.
namespace p3a::paths {
HMODULE self();             // the mod's module
std::wstring dir_w();       // its folder, with the trailing backslash
std::string dir();          // the same as narrow text (asset paths)
}  // namespace p3a::paths

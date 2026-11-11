// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#pragma once
#include <format>
#include <string>
#include <string_view>
#include "ring.h"

namespace p3a::log {
void init();  // %LOCALAPPDATA%\P3RAccess\p3access.log (a new process keeps the old one aside; a hot reload into a player's session appends)
void write(std::string_view line);
template <typename... A>
void writef(std::format_string<A...> fmt, A&&... a) { write(std::format(fmt, std::forward<A>(a)...)); }
std::string utf8(std::u16string_view s);
std::string utf8(const char16_t* s);
std::wstring path();
LineRing& ring();  // recent lines, for the dev server
}  // namespace p3a::log

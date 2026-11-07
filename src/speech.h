// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#pragma once
#include <string>
#include <cstdint>
#include <string_view>
#include <vector>
#include "ring.h"
namespace p3a::speech {
bool init();            // loads prism, picks the best available backend
void shutdown();
void speak(std::string_view utf8, bool interrupt = true);
void stop();
uint64_t count();       // lines spoken so far (a reader can tell whether anything else spoke since its last line)
int speaking();         // 1 speaking, 0 quiet, -1 the backend cannot tell (or muted)
const char* backend_name();
void set_muted(bool m);  // muted: nothing reaches the backend, but history still records
bool muted();
std::string timing();   // the backend's slowest speak call (/health)
std::vector<uint64_t> bench(int n);   // dev: n interrupting calls with a single space, each call's microseconds
LineRing& history();     // every line ever passed to speak(), for the dev server
}  // namespace p3a::speech

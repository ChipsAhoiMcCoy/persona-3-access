// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#pragma once
// Crash logger: a vectored exception handler that writes every serious exception's code, address and the return
// addresses found on the faulting thread's stack (module+rva) to the mod log, with no heap use, so a fault that
// kills the process before the game's crash reporter runs (heap corruption fast-fails) still leaves its location.
namespace p3a::crash {
void install();   // right after log::init
void remove();
}  // namespace p3a::crash

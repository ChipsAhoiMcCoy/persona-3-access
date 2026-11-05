#pragma once

namespace p3a::cutin {
// Story cut-ins: the character close-up a scene flashes for a moment to stress a line, read as a short description
// (src/cutin.cpp). Game thread.
void tick();
void register_routes();   // /cutin/test (dev)
}  // namespace p3a::cutin

#pragma once

namespace p3a::picture {
// Pictures a scene shows full screen (a class question's chalkboard picture, the story's sketch), read as a short
// description (src/picture.cpp). Game thread.
void tick();
void register_routes();   // /picture (dev)
}  // namespace p3a::picture

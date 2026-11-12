#pragma once

namespace p3a::portrait {
// The area key during a conversation: what the character speaking looks like, in the outfit and pose of the portrait
// beside the line (src/core/portrait_text: the words). False (nothing said) when no portrait is up.
bool describe();   // game thread
void install();    // dev route /portrait
}  // namespace p3a::portrait

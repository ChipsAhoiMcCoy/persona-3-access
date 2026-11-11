#pragma once
// The detail buffer (battle_details.cpp): the details of what is in focus, a line at a time. In a battle that is the
// unit under the game's cursor; anywhere, a menu row that declares details (MenuScreen::has_details) comes first.
// The right stick up and down, or Ctrl+Up and Ctrl+Down.
namespace p3a::screens::battle_details {
bool in_focus();     // something with details is in focus and no message window is over it
void step(int dir);  // +1 the next line, -1 the previous, and read it
void frame();        // every mod tick: claims the stick while something is in focus and reads its flicks
void install();      // the /battle/details dev route
}  // namespace p3a::screens::battle_details

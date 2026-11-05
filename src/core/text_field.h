#pragma once
// A text box the game owns, read the way a screen reader reads an edit field: what an edit added or removed, the
// character the caret lands on, the whole line. The box tells us its text but not its caret, so the caret is modelled
// from the keys and the edits (a mouse or Ctrl+arrow move is not seen; the next edit corrects it).
#include <cstddef>
#include <string>

namespace p3a::core {

class TextField {
 public:
  enum class Key { Left, Right, Home, End, Line, Backspace };

  // The box opened on this text, the caret at its end.
  void open(std::string text);
  const std::string& text() const { return text_; }
  size_t caret() const { return caret_; }

  // The player pressed a key in the box: what to say (empty for Backspace, whose edit speaks for itself).
  std::string pressed(Key key);
  // The box's text is now `now`: what to say. One character added or removed is that character; anything larger (an
  // on-screen keyboard's commit, a selection typed over) is the whole text.
  std::string edited(const std::string& now);

 private:
  std::string at_caret() const;
  std::string text_;
  size_t caret_ = 0;    // bytes into text_, on a character's first byte
  bool back_ = false;   // Backspace was pressed: its edit removes the character before the caret
};

}  // namespace p3a::core

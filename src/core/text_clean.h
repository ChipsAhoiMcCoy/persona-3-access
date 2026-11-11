#pragma once
#include <string>
#include <string_view>

namespace p3a::core {
// Game text -> speakable text: line breaks and runs of whitespace become single spaces, a bullet glyph at the start
// of a line is dropped, control characters and the '@' left for an inline icon are removed, and a space left in front
// of closing punctuation goes ("Wait ... what ?" -> "Wait... what?"; an ellipsis that opens a word keeps its space).
// UTF-8 in and out.
std::string clean_text(std::string_view s);
// True if `s` holds at least one letter or digit (skip speaking pure punctuation / glyph placeholders).
bool has_words(std::string_view s);

// The first sentence of `s` (through the first ". ", "! " or "? "), or all of it.
std::string first_sentence(std::string_view s);

// Joined lines without a comma or a second period after a sentence's end ("member., 3 of 4" -> "member. 3 of 4",
// "HP.. Next" -> "HP. Next", "yo!. y'know" -> "yo! y'know"); ellipses stay. Every spoken line goes through it.
std::string tidy_joins(std::string_view s);
}  // namespace p3a::core

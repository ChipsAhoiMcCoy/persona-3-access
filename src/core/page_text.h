#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace p3a::core {
// The engine-free rules of reading a message page: the names of the buttons it draws as icons, pages that are a list,
// and whether a page's data says the game will voice it.

// The name of a button icon (strings::kIconKeys), or null for an id with none. `pad`: the controller's buttons, else
// the keyboard's keys, which differ by place: W and S in battle for the D-pad's U and O, and in the field the key the
// words `around` the icon call for (the menu: Tab; the map: M; exploring or recovering: Y).
const char* icon_key(int id, bool pad, bool battle, std::string_view around = {});

// A page rendered with placeholders, finished. kIconByPlace followed by the icon id's byte stands for a button whose
// key depends on its sentence (from the full stop before it to the one after): named with icon_key. kIconGlyph stands
// for an icon with no name: "a key" between words ("pressing [x] will"), nothing where it runs into a word (an item's
// glyph before its name) or ends the text (a picture closing a line).
inline constexpr char kIconByPlace = '\x02';
inline constexpr char kIconGlyph = '\x01';
std::string finish_icons(std::string page, bool pad, bool battle);

// A page whose rows are a list of names ("P4G EX BGM Set" / "P5R EX BGM Set" / ...), joined with commas; "" when it
// is not one. Rows are cleaned and empty ones dropped; a list has two rows or more, no punctuation in any, and every
// row after the first starts with a capital or a digit (prose wraps across rows too, but has its full stops). `joined`
// is the page as rendered, rows joined with spaces: the rows must say the same, or the answer is "".
std::string rows_as_list(const std::vector<std::string>& rows, std::string_view joined);

// Whether a page's BMD data declares a voice. The data opens with message functions: FE, a byte F0 | (1 + argument
// count), the function (table << 5 | index), then two bytes per argument, each stored plus one. The voice function is
// 0x61 (table 3, function 1) with four arguments or more: its first argument 1 (bytes 02 01) marks a voiced line, and
// its fourth is the cue, where 0 (bytes 01 01) means no voice. `any`: any first argument counts (support talk).
// Reads at most `size` bytes and the first 96 bytes' worth of functions.
bool declares_voice(const uint8_t* data, size_t size, bool any = false);
}  // namespace p3a::core

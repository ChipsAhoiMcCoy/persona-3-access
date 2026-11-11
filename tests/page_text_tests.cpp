#include "doctest.h"
#include "core/page_text.h"
#include <cstdint>
#include <string>
#include <vector>

using p3a::core::kIconByPlace;
using p3a::core::kIconGlyph;

TEST_CASE("icon_key names a button for the keyboard or the controller") {
  using p3a::core::icon_key;
  CHECK(std::string(icon_key(113, false, false)) == "Space");
  CHECK(std::string(icon_key(113, true, false)) == "A");
  CHECK(std::string(icon_key(121, false, false)) == "I, J, K and L");
  CHECK(std::string(icon_key(119, true, false)) == "the right stick button");
  CHECK(icon_key(137, false, false) == nullptr);
  CHECK(icon_key(0, true, true) == nullptr);
}
TEST_CASE("icon_key: the keyboard's keys by place") {
  using p3a::core::icon_key;
  CHECK(std::string(icon_key(104, false, true)) == "W");
  CHECK(std::string(icon_key(104, false, false)) == "U");
  CHECK(std::string(icon_key(105, false, true)) == "S");
  CHECK(std::string(icon_key(105, false, false)) == "O");
  CHECK(std::string(icon_key(104, true, true)) == "D-pad up");
  CHECK(std::string(icon_key(112, false, false)) == "E");
  CHECK(std::string(icon_key(112, false, false, "opening the Menu with")) == "Tab");
  CHECK(std::string(icon_key(112, false, true, "opening the menu with")) == "E");   // in battle the words do not count
  CHECK(std::string(icon_key(112, true, false, "opening the menu with")) == "Y");
  CHECK(std::string(icon_key(114, false, false, "will display a map")) == "M");
  CHECK(std::string(icon_key(114, false, false, "the previous page")) == "Q");
  CHECK(std::string(icon_key(117, false, false, "during exploration")) == "Y");
  CHECK(std::string(icon_key(117, false, false, "to Auto-Recover")) == "Y");
  CHECK(std::string(icon_key(117, false, false, "the next page")) == "E");
}
TEST_CASE("finish_icons names a button from its own sentence") {
  using p3a::core::finish_icons;
  std::string menu = std::string("Turn pages with ") + kIconByPlace + (char)114 + ". Pressing " + kIconByPlace + (char)114 + " will display a map.";
  CHECK(finish_icons(menu, false, false) == "Turn pages with Q. Pressing M will display a map.");
  CHECK(finish_icons(menu, true, false) == "Turn pages with LB. Pressing LB will display a map.");
  CHECK(finish_icons(menu, false, true) == "Turn pages with Q. Pressing Q will display a map.");
  CHECK(finish_icons("No icons here.", false, false) == "No icons here.");
  CHECK(finish_icons("", false, false) == "");
  std::string accent = std::string("Caf\xC3\xA9: open the menu with ") + kIconByPlace + (char)112;
  CHECK(finish_icons(accent, false, false) == "Caf\xC3\xA9: open the menu with Tab");
}
TEST_CASE("finish_icons: an icon with no name") {
  using p3a::core::finish_icons;
  CHECK(finish_icons(std::string("pressing ") + kIconGlyph + " will", false, false) == "pressing a key will");
  CHECK(finish_icons(std::string("a ") + kIconGlyph + "Twilight Fragment", false, false) == "a Twilight Fragment");
  CHECK(finish_icons(std::string("See you") + kIconGlyph, false, false) == "See you");
  CHECK(finish_icons(std::string("") + kIconGlyph + " " + kIconGlyph + " go", true, false) == "a key a key go");
}
TEST_CASE("rows_as_list joins a page of names with commas") {
  using p3a::core::rows_as_list;
  CHECK(rows_as_list({"P4G EX BGM Set", "P5R EX BGM Set", "2 Costumes"}, "P4G EX BGM Set P5R EX BGM Set 2 Costumes") ==
        "P4G EX BGM Set, P5R EX BGM Set, 2 Costumes");
  CHECK(rows_as_list({"  First  ", "", "Second"}, "First Second") == "First, Second");   // rows are cleaned, empty ones dropped
  CHECK(rows_as_list({"\xC3\x89t\xC3\xA9", "Hiver"}, "\xC3\x89t\xC3\xA9 Hiver") == "\xC3\x89t\xC3\xA9, Hiver");
}
TEST_CASE("rows_as_list leaves prose and single rows alone") {
  using p3a::core::rows_as_list;
  CHECK(rows_as_list({"Only one"}, "Only one") == "");
  CHECK(rows_as_list({}, "") == "");
  CHECK(rows_as_list({"The door is", "locked."}, "The door is locked.") == "");          // punctuation
  CHECK(rows_as_list({"A long line that", "wraps here"}, "A long line that wraps here") == "");   // a row in lower case
  CHECK(rows_as_list({"Alpha", "Beta"}, "Alpha Beta Gamma") == "");                        // not what the page said
  CHECK(rows_as_list({"Alpha", "\xC3\xA9t\xC3\xA9"}, "Alpha \xC3\xA9t\xC3\xA9") == "");   // only ASCII capitals and digits open a row
}
TEST_CASE("declares_voice reads the voice function of a page's data") {
  using p3a::core::declares_voice;
  // FE F7 61: function 0x61 with six arguments; the first is 1 (02 01), the fourth the cue.
  const uint8_t voiced[] = {0xFE, 0xF7, 0x61, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 'H', 'i'};
  const uint8_t no_cue[] = {0xFE, 0xF7, 0x61, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 'H', 'i'};
  const uint8_t other[] = {0xFE, 0xF7, 0x61, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 'H', 'i'};
  CHECK(declares_voice(voiced, sizeof voiced));
  CHECK_FALSE(declares_voice(no_cue, sizeof no_cue));
  CHECK_FALSE(declares_voice(other, sizeof other));          // another first argument: not a voiced line...
  CHECK(declares_voice(other, sizeof other, true));          // ...unless any first argument counts
  CHECK_FALSE(declares_voice(no_cue, sizeof no_cue, true));
}
TEST_CASE("declares_voice walks the functions before the voice one and stops at text") {
  using p3a::core::declares_voice;
  // FE F2 25 xx xx: a function with one argument, then the voice function.
  const uint8_t second[] = {0xFE, 0xF2, 0x25, 0x05, 0x01, 0xFE, 0xF7, 0x61, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 'A'};
  CHECK(declares_voice(second, sizeof second));
  const uint8_t text[] = {'H', 'e', 'l', 'l', 'o', 0};
  CHECK_FALSE(declares_voice(text, sizeof text));
  const uint8_t few_args[] = {0xFE, 0xF4, 0x61, 0x02, 0x01, 0x01, 0x01, 0x05, 0x01, 'A', 'B', 'C'};   // three arguments: no cue
  CHECK_FALSE(declares_voice(few_args, sizeof few_args));
  const uint8_t bad[] = {0xFE, 0x17, 0x61, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x01};   // not a function header
  CHECK_FALSE(declares_voice(bad, sizeof bad));
  const uint8_t cut[] = {0xFE, 0xF7, 0x61, 0x02, 0x01};   // ends before the cue
  CHECK_FALSE(declares_voice(cut, sizeof cut));
  CHECK_FALSE(declares_voice(nullptr, 0));
  CHECK_FALSE(declares_voice(cut, 0));
}

TEST_CASE("declared_cue gives the voice function's cue") {
  using p3a::core::declared_cue;
  const uint8_t two[] = {0xFE, 0xF7, 0x61, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 'H', 'i'};
  CHECK(declared_cue(two, sizeof two) == 2);
  const uint8_t big[] = {0xFE, 0xF7, 0x61, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x0B, 0x03, 0x01, 0x01, 0x01, 0x01, 'H', 'i'};
  CHECK(declared_cue(big, sizeof big) == 10 + 2 * 255);   // the second byte counts 255s
  const uint8_t none[] = {0xFE, 0xF7, 0x61, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 'H', 'i'};
  CHECK(declared_cue(none, sizeof none) == 0);
  CHECK(declared_cue(nullptr, 0) == 0);
}

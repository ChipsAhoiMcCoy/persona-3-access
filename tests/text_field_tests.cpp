#include "core/text_field.h"
#include "doctest.h"

using p3a::core::TextField;
using Key = TextField::Key;

TEST_CASE("typing and deleting at the end says the character") {
  TextField f;
  f.open("");
  CHECK(f.edited("M") == "M");
  CHECK(f.edited("Me") == "e");
  CHECK(f.caret() == 2);
  f.pressed(Key::Backspace);
  CHECK(f.edited("M") == "e");
  f.pressed(Key::Backspace);
  CHECK(f.edited("") == "M");
  CHECK(f.caret() == 0);
}

TEST_CASE("the arrows read the character the caret lands on") {
  TextField f;
  f.open("Abc");
  CHECK(f.caret() == 3);
  CHECK(f.pressed(Key::Right) == "blank");
  CHECK(f.pressed(Key::Left) == "c");
  CHECK(f.pressed(Key::Left) == "b");
  CHECK(f.pressed(Key::Home) == "A");
  CHECK(f.pressed(Key::Left) == "A");
  CHECK(f.pressed(Key::Right) == "b");
  CHECK(f.pressed(Key::End) == "blank");
}

TEST_CASE("an empty field answers every key with blank") {
  TextField f;
  f.open("");
  for (Key k : {Key::Left, Key::Right, Key::Home, Key::End, Key::Line}) CHECK(f.pressed(k) == "blank");
}

TEST_CASE("up and down read the whole line and leave the caret") {
  TextField f;
  f.open("Jones");
  f.pressed(Key::Left);
  CHECK(f.pressed(Key::Line) == "Jones");
  CHECK(f.caret() == 4);
}

TEST_CASE("edits in the middle follow the caret, repeated letters included") {
  TextField f;
  f.open("Col");
  f.pressed(Key::Left);            // Co|l
  CHECK(f.edited("Coll") == "l");  // one typed before the l
  CHECK(f.caret() == 3);
  f.pressed(Key::Backspace);       // takes the l before the caret
  CHECK(f.edited("Col") == "l");
  CHECK(f.caret() == 2);
  CHECK(f.edited("Co") == "l");    // Delete: the l after it
  CHECK(f.caret() == 2);
}

TEST_CASE("a larger change reads the whole text") {
  TextField f;
  f.open("Jones");
  CHECK(f.edited("Carter") == "Carter");
  CHECK(f.caret() == 6);
  CHECK(f.edited("") == "blank");
}

TEST_CASE("a character of several bytes is one character") {
  TextField f;
  f.open("Jos\xC3\xA9");
  CHECK(f.pressed(Key::Left) == "\xC3\xA9");
  CHECK(f.pressed(Key::Left) == "s");
  f.pressed(Key::End);
  f.pressed(Key::Backspace);
  CHECK(f.edited("Jos") == "\xC3\xA9");
  CHECK(f.edited("Jos\xC3\xA8") == "\xC3\xA8");
}

#include "doctest.h"
#include "core/utf8.h"

#include <string>

using namespace p3a::core::utf8;

TEST_CASE("utf8: a walk goes a character at a time, whatever its length") {
  const std::string s = "a\xC3\xA9\xE3\x81\x82\xF0\x9F\x98\x80z";   // a, e acute, hiragana a, an emoji, z
  CHECK(next_char(s, 0) == 1);
  CHECK(next_char(s, 1) == 3);
  CHECK(next_char(s, 3) == 6);
  CHECK(next_char(s, 6) == 10);
  CHECK(next_char(s, 10) == 11);
  CHECK(next_char(s, 11) == 11);   // the end stays the end
  CHECK(prev_char(s, 11) == 10);
  CHECK(prev_char(s, 10) == 6);
  CHECK(prev_char(s, 6) == 3);
  CHECK(prev_char(s, 3) == 1);
  CHECK(prev_char(s, 1) == 0);
  CHECK(prev_char(s, 0) == 0);     // the start stays the start
  CHECK(is_continuation(s, 2));
  CHECK_FALSE(is_continuation(s, 1));
  CHECK_FALSE(is_continuation(s, 0));
  CHECK_FALSE(is_continuation(s, 99));   // past the end
}

TEST_CASE("utf8: code points decode and encode back to the same bytes") {
  const std::string s = "a\xC3\xA9\xE3\x81\x82\xF0\x9F\x98\x80";
  std::size_t i = 0;
  CHECK(next_codepoint(s, i) == 0x61);
  CHECK(i == 1);
  CHECK(next_codepoint(s, i) == 0xE9);
  CHECK(i == 3);
  CHECK(next_codepoint(s, i) == 0x3042);
  CHECK(i == 6);
  CHECK(next_codepoint(s, i) == 0x1F600);
  CHECK(i == 10);

  std::string out;
  for (std::uint32_t cp : {0x61u, 0xE9u, 0x3042u, 0x1F600u}) append_codepoint(out, cp);
  CHECK(out == s);
}

TEST_CASE("utf8: a byte that fits no sequence passes through as itself") {
  const std::string stray = "\xA9" "b";        // a continuation byte with no lead before it
  std::size_t i = 0;
  CHECK(next_codepoint(stray, i) == 0xA9);
  CHECK(i == 1);
  CHECK(next_codepoint(stray, i) == 0x62);

  const std::string cut = "\xE3\x81";          // a three-byte character missing its last byte
  i = 0;
  CHECK(next_codepoint(cut, i) == 0xE3);
  CHECK(i == 1);
  CHECK(next_codepoint(cut, i) == 0x81);
  CHECK(i == 2);
  CHECK(prev_char(stray, 1) == 0);
  CHECK(next_char(cut, 0) == 2);               // the walk still steps over what continuation bytes there are
}

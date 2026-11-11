#include "doctest.h"
#include "core/bmd.h"
#include <cstring>
#include <vector>

using namespace p3a::core::bmd;

// A two-dialog file built by hand: a message (2 pages, one with a function) and a selection (2 options). `big`: the
// stored form (big-endian, "1GSM", offsets from 0x20); otherwise the converted one (self-relative offsets).
static std::vector<uint8_t> sample(bool big = false) {
  std::vector<uint8_t> b(0x200, 0);
  auto w32 = [&](size_t o, uint32_t v) { for (int i = 0; i < 4; ++i) b[o + i] = (uint8_t)(v >> (big ? 24 - 8 * i : 8 * i)); };
  auto w16 = [&](size_t o, uint16_t v) { for (int i = 0; i < 2; ++i) b[o + i] = (uint8_t)(v >> (big ? 8 - 8 * i : 8 * i)); };
  auto off = [&](size_t field, size_t target) { w32(field, (uint32_t)(target - (big ? 0x20 : field))); };
  b[0] = 7;
  memcpy(&b[8], big ? "1GSM" : "MSG1", 4);
  w32(0x18, 2);
  // dialog headers at 0x20
  w32(0x20, 0); off(0x24, 0x60);   // message at 0x60
  w32(0x28, 1); off(0x2C, 0xA0);   // selection at 0xA0
  w32(0x30, 0); w32(0x34, 0);      // speaker header: no speakers
  // message at 0x60
  memcpy(&b[0x60], "MSG_000", 7);
  w16(0x60 + 24, 2); w16(0x60 + 26, 0xFFFF);
  off(0x60 + 28, 0x110);   // page 0 text at 0x110
  off(0x60 + 32, 0x120);   // page 1 text at 0x120
  const uint8_t p0[] = {'H', 'i', 0xFE, 0xF2, 0x21, 0x01, 0x01, '\n', 'y', 'o', 'u', 0};
  memcpy(&b[0x110], p0, sizeof p0);
  memcpy(&b[0x120], "Bye", 4);
  // selection at 0xA0
  memcpy(&b[0xA0], "SEL_000", 7);
  w16(0xA0 + 26, 2);
  off(0xA0 + 32, 0x140);   // option 0 at 0x140
  off(0xA0 + 36, 0x150);   // option 1 at 0x150
  memcpy(&b[0x140], "Yes", 4);
  memcpy(&b[0x150], "No", 3);
  return b;
}

TEST_CASE("bmd parses messages, functions and selections") {
  auto b = sample();
  File f = parse(b.data(), b.size());
  REQUIRE(f.ok);
  REQUIRE(f.dialogs.size() == 2);
  CHECK(f.dialogs[0].name == "MSG_000");
  REQUIRE(f.dialogs[0].pages.size() == 2);
  CHECK(plain(f.dialogs[0].pages[0]) == "Hi you");
  REQUIRE(f.dialogs[0].pages[0].functions.size() == 1);
  CHECK(f.dialogs[0].pages[0].functions[0].table == 1);
  CHECK(f.dialogs[0].pages[0].functions[0].index == 1);
  CHECK(f.dialogs[0].pages[0].functions[0].args.size() == 1);
  CHECK(plain(f.dialogs[0].pages[1]) == "Bye");
  CHECK(f.dialogs[1].kind == 1);
  REQUIRE(f.dialogs[1].pages.size() == 2);
  CHECK(plain(f.dialogs[1].pages[1]) == "No");
}

TEST_CASE("bmd reads a file in its stored, big-endian form") {
  auto b = sample(true);
  File f = parse(b.data(), b.size());
  REQUIRE(f.ok);
  REQUIRE(f.dialogs.size() == 2);
  CHECK(f.dialogs[0].name == "MSG_000");
  REQUIRE(f.dialogs[0].pages.size() == 2);
  CHECK(plain(f.dialogs[0].pages[0]) == "Hi you");
  CHECK(f.dialogs[0].speaker == -1);
  CHECK(plain(f.dialogs[1].pages[0]) == "Yes");
}

TEST_CASE("bmd rejects garbage") {
  uint8_t junk[64] = {};
  CHECK_FALSE(parse(junk, sizeof junk).ok);
}

TEST_CASE("bmd keeps a function's arguments and where in the text it stood") {
  auto b = sample();
  File f = parse(b.data(), b.size());
  REQUIRE(f.ok);
  const Function& fn = f.dialogs[0].pages[0].functions[0];
  REQUIRE(fn.args.size() == 1);
  CHECK(fn.args[0] == 0x0101);
  CHECK(fn.pos == 2);                      // after "Hi"
  CHECK(f.dialogs[0].pages[0].text == "Hi\nyou");
}

TEST_CASE("bmd reads the speaker table and a message's speaker") {
  auto b = sample();
  auto w32 = [&](size_t o, uint32_t v) { for (int i = 0; i < 4; ++i) b[o + i] = (uint8_t)(v >> (8 * i)); };
  b[0x60 + 26] = 1; b[0x60 + 27] = 0;      // the message's speaker: the second name
  w32(0x30, 0x160 - 0x30);                 // the names array at 0x160 (offsets are from the field itself)
  w32(0x34, 2);
  w32(0x160, 0x170 - 0x160);
  w32(0x164, 0x180 - 0x164);
  memcpy(&b[0x170], "Yukari", 7);
  memcpy(&b[0x180], "Mitsuru\nKirijo", 15);
  File f = parse(b.data(), b.size());
  REQUIRE(f.ok);
  REQUIRE(f.speakers.size() == 2);
  CHECK(f.speakers[0] == "Yukari");
  CHECK(f.speakers[1] == "Mitsuru Kirijo");   // a name's line break reads as a space
  CHECK(f.dialogs[0].speaker == 1);
  CHECK(f.dialogs[1].speaker == -1);          // a selection has none
}

TEST_CASE("bmd stops at the end of a cut-off file") {
  auto b = sample();
  CHECK_FALSE(parse(b.data(), 0x1F).ok);   // no header
  CHECK_FALSE(parse(b.data(), 0x30).ok);   // the dialog headers run past the end
  File f = parse(b.data(), 0x40);          // the headers are there, what they point at is not
  CHECK(f.ok);
  REQUIRE(f.dialogs.size() == 2);
  CHECK(f.dialogs[0].pages.empty());
  CHECK(f.dialogs[1].pages.empty());
  CHECK(f.speakers.empty());
  b[0x1B] = 0x10;                          // a dialog count far beyond the file
  CHECK_FALSE(parse(b.data(), b.size()).ok);
}

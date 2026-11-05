#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace p3a::core::bmd {
// Atlus message script binary ("MSG1", file type 7), as P3R stores it in UBmdAsset.mBuf:
//   0x00 u8 type, u8 compressed, u16 user, u32 file size, char[4] "MSG1", u32, u32 reloc offset, u32 reloc size,
//   u32 dialog count, u16 relocated, u16; then dialog headers {u32 kind (0 message, 1 selection), u32 offset} and
//   the speaker table header. Offsets are self-relative (from the offset field's own position). Until the game first
//   uses a file it is stored big-endian ("1GSM") with offsets from 0x20; parse reads both.
//   message:   char name[24], u16 page count, u16 speaker id, u32 page offsets[], u32 text size, text
//   selection: char name[24], u16, u16 option count, u16, u16, u32 option offsets[], u32 text size, text
// Text is UTF-8 with function tokens: 0xFE, 0xF0|n, id (table << 5 | index), then (n - 1) u16 arguments;
// 0x0A is a line break; 0x00 ends a page.
struct Function { int table, index; std::vector<int> args; size_t pos; };   // pos: byte offset in the page's text
struct Page { std::string text; std::vector<Function> functions; };
struct Dialog { int kind = 0; std::string name; int speaker = -1; std::vector<Page> pages; };
struct File { std::vector<Dialog> dialogs; std::vector<std::string> speakers; bool ok = false; };

File parse(const uint8_t* data, size_t size);
// One page's text, functions removed and line breaks turned into spaces.
std::string plain(const Page& p);
}  // namespace p3a::core::bmd

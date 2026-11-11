#include "core/bmd.h"
#include <cstring>

namespace p3a::core::bmd {
// The file comes in two forms: as stored (big-endian, "1GSM", offsets from 0x20) until the game first uses it, then
// converted in place (little-endian, "MSG1", self-relative offsets). BMD_CalendarDateMessages is stored until the
// calendar opens (2026-10-31).
namespace {
// Reads the file's integers in the byte order of the form it is in.
struct Reader {
  bool big;
  uint16_t u16(const uint8_t* p) const { return big ? (uint16_t)(p[0] << 8 | p[1]) : (uint16_t)(p[0] | p[1] << 8); }
  uint32_t u32(const uint8_t* p) const {
    return big ? (uint32_t)((uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]) : (uint32_t)(p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24);
  }
};
}  // namespace

// Decode text from `p` up to the terminating 0 (or `end`) into a page.
static Page decode(const Reader& r, const uint8_t* p, const uint8_t* end) {
  Page pg;
  while (p < end && *p) {
    uint8_t b = *p;
    // P3R escapes function tokens with 0xFE (its text is UTF-8, whose 4-byte lead bytes overlap 0xF0..0xF4):
    // FE, F0|n, id (table << 5 | index), then (n - 1) little-endian u16 arguments (verified on BMD_CampSystem).
    if (b == 0xFE && p + 2 < end && (p[1] & 0xF0) == 0xF0) {
      int n = p[1] & 0x0F;
      int id = p[2];
      Function f{(id & 0xE0) >> 5, id & 0x1F, {}, pg.text.size()};
      const uint8_t* a = p + 3;
      for (int i = 0; i < n - 1 && a + 1 < end; ++i, a += 2) f.args.push_back(r.u16(a));
      pg.functions.push_back(std::move(f));
      p = a;
      continue;
    }
    pg.text += (char)b;
    ++p;
  }
  return pg;
}

File parse(const uint8_t* d, size_t size) {
  File f;
  if (size < 0x20) return f;
  Reader r{false};
  if (memcmp(d + 8, "MSG1", 4) == 0) r.big = false;
  else if (memcmp(d + 8, "1GSM", 4) == 0) r.big = true;
  else return f;
  auto u16 = [&r](const uint8_t* p) { return r.u16(p); };
  auto u32 = [&r](const uint8_t* p) { return r.u32(p); };
  uint32_t count = u32(d + 0x18);
  constexpr size_t kBase = 0x20;
  if (kBase + (size_t)count * 8 + 16 > size) return f;
  // Converted, every offset is relative to the position of the offset field itself (dialog i's header is at
  // (0x24 + 8 i) + value; measured on BMD_CampSystem, 2026-10-27); stored, relative to 0x20.
  auto rel = [&](const uint8_t* field) -> const uint8_t* {
    size_t pos = (r.big ? kBase : (size_t)(field - d)) + u32(field);
    return pos < size ? d + pos : nullptr;
  };
  for (uint32_t i = 0; i < count; ++i) {
    const uint8_t* e = d + kBase + i * 8;
    Dialog dl;
    dl.kind = (int)u32(e);
    const uint8_t* h = rel(e + 4);
    if (!h || h + 0x20 > d + size) { f.dialogs.push_back(dl); continue; }
    dl.name.assign((const char*)h, strnlen((const char*)h, 24));
    uint16_t pages;
    const uint8_t* offs;
    if (dl.kind == 0) { pages = u16(h + 24); dl.speaker = u16(h + 26) == 0xFFFF ? -1 : u16(h + 26); offs = h + 28; }
    else { pages = u16(h + 26); offs = h + 32; }
    if (offs + pages * 4 + 4 > d + size) { f.dialogs.push_back(dl); continue; }
    for (uint16_t p = 0; p < pages; ++p) {
      const uint8_t* t = rel(offs + p * 4);
      dl.pages.push_back(t ? decode(r, t, d + size) : Page{});
    }
    f.dialogs.push_back(std::move(dl));
  }
  // Speaker table header right after the dialog headers: {u32 names (self-relative), u32 count, u32, u32}; the names
  // array holds self-relative pointers to the name texts.
  const uint8_t* sh = d + kBase + count * 8;
  uint32_t n = u32(sh + 4);
  const uint8_t* names = n ? rel(sh) : nullptr;
  if (names && n < 4096)
    for (uint32_t i = 0; i < n && names + i * 4 + 4 <= d + size; ++i) {
      const uint8_t* name = rel(names + i * 4);
      f.speakers.push_back(name ? plain(decode(r, name, d + size)) : "");
    }
  f.ok = true;
  return f;
}

std::string plain(const Page& p) {
  std::string out;
  bool space = false;
  for (char c : p.text) {
    if (c == '\n' || c == ' ') { space = true; continue; }
    if (space && !out.empty()) out += ' ';
    space = false;
    out += c;
  }
  return out;
}
}  // namespace p3a::core::bmd

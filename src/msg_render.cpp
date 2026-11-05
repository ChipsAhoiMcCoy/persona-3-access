// Rendering the game's BMD pages to text: BmdPageToFString for plain pages, the letter records for pages holding
// button icons (named as keys), rows that are a list joined with commas, and each entry of a choice list.
#include "msg_render.h"
#include "msg.h"
#include "core/text_clean.h"
#include "core/strings.h"
#include "screens/screen.h"
#include "log.h"
#include "seh.h"
#include <cctype>
#include <climits>
#include <cstring>
#include <format>
#include <set>
#include <vector>

namespace p3a::msg {
using namespace ue;
using BmdPageToFString_t = void (*)(FString*, void*);
static BmdPageToFString_t g_page_to_string;
void set_renderer(void* fn) { g_page_to_string = (BmdPageToFString_t)fn; }
void* renderer() { return (void*)g_page_to_string; }

// A page (FCurrentBmdPage*) to text; empty on any fault.
// Inline icons (tutorials' button glyphs) are line nodes with no letter records; +0x58 is the icon (118 = the dash
// key; measured 2026-10-27). BmdPageToFString drops them, so pages holding one are rendered here from the letter
// records (inline UTF-8 at +0, next at +0x28; a new row where a line's +8 changes), with the key's name in place.
// The ids are the controller's buttons (one per button, though E stands for two of them on the keyboard).
// Two of the controller's buttons are different keys by place: Y is E in battle (skills) but Tab in the field (the
// menu: "opening the menu with [112]"), LB is Q for a previous page but M in the field (the map: "[114] will display
// a map", 2026-10-29). Those are decided once the whole page is rendered, from the words around them (`around`).
static const char* icon_name(int id, const std::string& around = {}) {
  bool pad = strings::pad_hints();
  // The D-pad's keyboard keys differ: W and S in battle, U and O (camera zoom) in the field.
  screens::Screen* sc = screens::current();
  bool battle = sc && std::strcmp(sc->id(), "battle") == 0;
  auto nearby = [&](const char* w) {
    std::string a = around;
    for (auto& ch : a) ch = (char)tolower((unsigned char)ch);
    return !battle && a.find(w) != std::string::npos;
  };
  switch (id) {   // keyboard defaults, from the tutorials' own sentences; the controller's from its key list
    case 104: return pad ? "D-pad up" : battle ? "W" : "U";     // battle Tactics (measured 2026-10-27); field zoom in
    case 105: return pad ? "D-pad down" : battle ? "S" : "O";   // battle Survey; field camera reset (2026-10-28)
    case 119: return pad ? "the right stick button" : "R";      // recenter the camera
    case 121: return pad ? "the right stick" : "I, J, K and L"; // rotate the camera
    case 108: return pad ? "Menu" : "H";       // save ("save your progress by pressing the [108] button", 2026-10-28)
    case 110: return pad ? "B" : "C";          // cancel
    case 112: return pad ? "Y" : nearby("menu") ? "Tab" : "E";   // battle skills (Persona); the field menu
    case 113: return pad ? "A" : "Space";      // attack, confirm
    case 114: return pad ? "LB" : nearby("map") ? "M" : "Q";     // left page / previous (with 117); the field map
    case 117: return pad ? "RB" : "E";         // right page / next
    case 115: return pad ? "LT" : "B";         // battle Shift (B on the command wheel, measured 2026-10-28)
    case 118: return pad ? "RT" : "Shift";     // dash
    default: return nullptr;
  }
}
static constexpr char kByPlace = '\x02';   // followed by the icon id's byte: decided from the finished page
static constexpr char kGlyph[] = "\x01";   // an unmapped glyph's place while a page is rendered
// A page's line list (+0x40 next), capped: a stale page's chain could loop.
static std::vector<uint8_t*> lines(uint8_t* first) {
  std::vector<uint8_t*> out;
  for (uint8_t* l = first; l && out.size() < 64; l = *(uint8_t**)(l + 0x40)) out.push_back(l);
  return out;
}
static bool render_with_icons(uint8_t* first, std::string& out) {
  bool icon = false;   // an empty line node with id 0 is a blank line, not an icon
  for (uint8_t* l : lines(first)) if (!*(uint8_t**)(l + 0x28) && *(int32_t*)(l + 0x58)) icon = true;
  if (!icon) return false;
  int row = -1;
  for (uint8_t* l : lines(first)) {
    int r = *(int32_t*)(l + 8);
    if (row >= 0 && r != row) out += ' ';
    row = r;
    uint8_t* tk = *(uint8_t**)(l + 0x28);
    if (!tk) {
      int id = *(int32_t*)(l + 0x58);
      if (!id) continue;
      if (id == 112 || id == 114) { out += kByPlace; out += (char)id; continue; }
      const char* n = icon_name(id);
      static std::set<int> logged;
      if (!n && logged.insert(id).second) log::writef("msg: unmapped icon {} after: {}", id, out);
      out += n ? std::string(n) : std::string(kGlyph);   // decided below
      continue;
    }
    for (int i = 0; tk && i < 512; ++i, tk = *(uint8_t**)(tk + 0x28)) out.append((const char*)tk, strnlen((const char*)tk, 8));
  }
  // The sentence around a button decides its key: from the last full stop before it to the next one after.
  for (size_t p; (p = out.find(kByPlace)) != std::string::npos && p + 1 < out.size();) {
    int id = (unsigned char)out[p + 1];
    size_t from = out.rfind('.', p), to = out.find('.', p);
    std::string around = out.substr(from == std::string::npos ? 0 : from, (to == std::string::npos ? out.size() : to) - (from == std::string::npos ? 0 : from));
    out.replace(p, 2, icon_name(id, around));
  }
  // An unmapped glyph: a button sits between words ("pressing [x] will"), an item glyph runs into its word ("a
  // [key item]Twilight Fragment" read as "a a keyTwilight", 2026-10-27) and is left out.
  // A glyph that ends the text is a picture closing a line (a text message's emoji, icon 137, 2026-10-27).
  for (size_t p; (p = out.find(kGlyph)) != std::string::npos;) {
    bool inline_glyph = p + 1 >= out.size() || isalnum((unsigned char)out[p + 1]);
    out.replace(p, 1, inline_glyph ? "" : strings::kAKey);
  }
  return true;
}

// A page whose rows are a list of names (the DLC notice: "P4G EX BGM Set" / "P5R EX BGM Set" / ..., 2026-10-28) ran
// together into one sentence. Rows with no punctuation, each after the first starting with a capital or a digit,
// are a list: joined with commas. Prose wraps across rows too, but a page of prose has its full stops.
static std::string as_list(uint8_t* first, const std::string& joined) {
  std::vector<std::string> rows;
  int row = INT_MIN;
  for (uint8_t* l : lines(first)) {
    uint8_t* tk = *(uint8_t**)(l + 0x28);
    if (!tk) continue;
    int r = *(int32_t*)(l + 8);
    if (r != row || rows.empty()) rows.emplace_back();
    row = r;
    for (int i = 0; tk && i < 512; ++i, tk = *(uint8_t**)(tk + 0x28)) rows.back().append((const char*)tk, strnlen((const char*)tk, 8));
  }
  std::vector<std::string> kept;
  for (auto& r : rows) if (std::string c = core::clean_text(r); !c.empty()) kept.push_back(c);
  if (kept.size() < 2) return {};
  std::string spaced, listed;
  for (size_t i = 0; i < kept.size(); ++i) {
    const std::string& r = kept[i];
    if (r.find_first_of(".,!?:;") != std::string::npos) return {};
    if (i && !isupper((unsigned char)r[0]) && !isdigit((unsigned char)r[0])) return {};
    spaced += (i ? " " : "") + r;
    listed += (i ? ", " : "") + r;
  }
  return spaced == joined ? listed : std::string{};   // the letter records must say what the renderer said
}

std::string page_text(void* page) {
  if (!page || !g_page_to_string) return {};
  std::string out;
  uint8_t* first = nullptr;
  guarded([&] {
    void* overview = *(void**)((uint8_t*)page + 0x28);
    if (!overview) return;
    first = *(uint8_t**)((uint8_t*)overview + 0x48);
    if (!first) return;
    if (render_with_icons(first, out)) { first = nullptr; return; }
    FString s{};
    g_page_to_string(&s, first);
    out = take_fstring(s);
  });
  out = core::clean_text(out);
  if (first && !out.empty()) {
    std::string list;
    guarded([&] { list = as_list(first, out); });
    if (!list.empty()) return list;
  }
  return out;
}
std::string page_string(void* page) { return page_text(page); }

// The choice list of a USelItem: entries (+0x50) are BMD line nodes -- +0x28 the first letter (a linked list of
// letter records), +0x40 the next line (null here). BmdPageToFString takes exactly such a line and walks the letters
// and the +0x40 lines (disassembled 2026-10-27, exe+0x135eeb0), so each entry renders on its own.
static std::string line_text(void* line) {
  if (!line || !g_page_to_string) return {};
  std::string out;
  guarded([&] {
    FString s{};
    g_page_to_string(&s, line);
    out = take_fstring(s);
  });
  return core::clean_text(out);
}
std::vector<std::string> choices_of(UObject* sel) {
  std::vector<std::string> out;
  guarded([&] {
    auto& entries = *(TArray<void*>*)((uint8_t*)sel + 0x50);
    for (int i = 0; i < entries.num && i < 16; ++i) out.push_back(line_text(entries.data[i]));
  });
  return out;
}
// A page's lines and letter records (page+0x28 -> +0x48 first line; line +0x40 next line, +0x28 first record;
// record: inline UTF-8 text at +0, next at +0x28), each record's first 0x60 bytes. For decoding what the text
// renderer leaves out (tutorial button icons).
std::string dump_tokens(uintptr_t page) {
  std::string s;
  guarded([&] {
    uint8_t* ov = *(uint8_t**)(page + 0x28);
    uint8_t* line = ov ? *(uint8_t**)(ov + 0x48) : nullptr;
    for (int li = 0; line && li < 32; ++li, line = *(uint8_t**)(line + 0x40)) {
      s += std::format("line {} @{} +8={} :", li, (void*)line, *(int32_t*)(line + 8));
      for (int i = 0; i < 0x80; ++i) s += std::format("{}{:02x}", i % 8 ? "" : " ", line[i]);   // an icon is a line of its own
      s += "\n";
      uint8_t* tk = *(uint8_t**)(line + 0x28);
      for (int ti = 0; tk && ti < 64; ++ti, tk = *(uint8_t**)(tk + 0x28)) {
        s += std::format("  tok {} @{} '{}' :", ti, (void*)tk, std::string((const char*)tk, strnlen((const char*)tk, 0x28)));
        for (int i = 0; i < 0x60; ++i) s += std::format("{}{:02x}", i % 8 ? "" : " ", tk[i]);
        s += "\n";
      }
    }
  });
  return s;
}

// An inline icon (a tutorial's button glyph) is a line node with no letter records: log such a page once.
void log_if_icons(uintptr_t page, const std::string& text) {
  static std::string last;
  if (text == last) return;
  bool icon = false;
  guarded([&] {
    uint8_t* ov = *(uint8_t**)(page + 0x28);
    uint8_t* line = ov ? *(uint8_t**)(ov + 0x48) : nullptr;
    for (int li = 0; line && li < 32; ++li, line = *(uint8_t**)(line + 0x40))
      if (!*(uint8_t**)(line + 0x28)) icon = true;
  });
  if (!icon) return;
  last = text;
  log::writef("msg: page with an icon: {}\n{}", text, dump_tokens(page));
}
}  // namespace p3a::msg

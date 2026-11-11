// Rendering the game's BMD pages to text: BmdPageToFString for plain pages, the letter records for pages holding
// button icons (named as keys), rows that are a list joined with commas, and each entry of a choice list.
#include "msg_render.h"
#include "msg.h"
#include "core/page_text.h"
#include "core/text_clean.h"
#include "core/strings.h"
#include "screens/screen.h"
#include "log.h"
#include "seh.h"
#include <climits>
#include <cstring>
#include <format>
#include <set>
#include <utility>
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
// a map", 2026-10-29). Those are decided once the whole page is rendered, from the words around them
// (core::finish_icons). The names are the keyboard's defaults, from the tutorials' own sentences, and the controller's
// from its key list (strings::kIconKeys): 104 / 105 the D-pad (battle Tactics and Survey, measured 2026-10-27; the
// field's camera zoom and reset), 108 save, 110 cancel, 112 battle skills and the field menu, 113 attack and confirm,
// 114 / 117 the previous / next page, 115 battle Shift, 118 dash, 119 recenter the camera, 121 rotate it.
// The D-pad's and the by-place keys depend on whether the battle screen is up.
static bool in_battle() {
  screens::Screen* sc = screens::current();
  return sc && std::strcmp(sc->id(), "battle") == 0;
}
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
  bool pad = strings::pad_hints(), battle = in_battle();
  int row = -1;
  for (uint8_t* l : lines(first)) {
    int r = *(int32_t*)(l + 8);
    if (row >= 0 && r != row) out += ' ';
    row = r;
    uint8_t* tk = *(uint8_t**)(l + 0x28);
    if (!tk) {
      int id = *(int32_t*)(l + 0x58);
      if (!id) continue;
      if (id == 112 || id == 114) { out += core::kIconByPlace; out += (char)id; continue; }   // decided from the finished page
      const char* n = core::icon_key(id, pad, battle);
      static std::set<int> logged;
      if (!n && logged.insert(id).second) log::writef("msg: unmapped icon {} after: {}", id, out);
      if (n) out += n;
      else out += core::kIconGlyph;   // an unmapped glyph's place: decided from the finished page too
      continue;
    }
    for (int i = 0; tk && i < 512; ++i, tk = *(uint8_t**)(tk + 0x28)) out.append((const char*)tk, strnlen((const char*)tk, 8));
  }
  // The by-place buttons take their key from the sentence around them. An unmapped glyph between words ("pressing
  // [x] will") is "a key"; an item glyph that runs into its word ("a [key item]Twilight Fragment" read as "a a
  // keyTwilight", 2026-10-27) or a picture that closes a line (a text message's emoji, icon 137) is left out.
  out = core::finish_icons(std::move(out), pad, battle);
  return true;
}

// A page whose rows are a list of names (the DLC notice: "P4G EX BGM Set" / "P5R EX BGM Set" / ..., 2026-10-28) ran
// together into one sentence. Its rows are gathered here; core::rows_as_list says whether they are a list and joins
// them with commas (the letter records must say what the renderer said).
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
  return core::rows_as_list(rows, joined);
}

std::string page_string(void* page) {
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

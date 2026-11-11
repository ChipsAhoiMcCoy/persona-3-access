#include "ue/scan.h"
#include <windows.h>
#include <cstring>
#include <string>
#include <vector>

namespace p3a::scan {
static uintptr_t g_base;
static size_t g_size;

static void init() {
  if (g_base) return;
  g_base = (uintptr_t)GetModuleHandleW(nullptr);
  auto dos = (IMAGE_DOS_HEADER*)g_base;
  auto nt = (IMAGE_NT_HEADERS64*)(g_base + dos->e_lfanew);
  g_size = nt->OptionalHeader.SizeOfImage;
}
uintptr_t exe_base() { init(); return g_base; }
uint32_t exe_timestamp() {
  init();
  auto nt = (IMAGE_NT_HEADERS64*)(g_base + ((IMAGE_DOS_HEADER*)g_base)->e_lfanew);
  return nt->FileHeader.TimeDateStamp;
}

struct Pat { std::vector<uint8_t> bytes; std::vector<bool> mask; };
static Pat parse(std::string_view s) {
  Pat p;
  for (size_t i = 0; i < s.size();) {
    if (s[i] == ' ') { ++i; continue; }
    if (s[i] == '?') { p.bytes.push_back(0); p.mask.push_back(false); i += (i + 1 < s.size() && s[i + 1] == '?') ? 2 : 1; continue; }
    p.bytes.push_back((uint8_t)strtoul(std::string(s.substr(i, 2)).c_str(), nullptr, 16));
    p.mask.push_back(true);
    i += 2;
  }
  return p;
}

// Calls `hit` for each match until it returns false. The first byte of every pattern we use is concrete, so memchr
// drives the scan (a 266 MB .text takes ~100 ms).
template <typename F>
static void each(std::string_view pattern, F hit) {
  init();
  Pat p = parse(pattern);
  if (p.bytes.empty() || !p.mask[0]) return;
  auto nt = (IMAGE_NT_HEADERS64*)(g_base + ((IMAGE_DOS_HEADER*)g_base)->e_lfanew);
  auto sec = IMAGE_FIRST_SECTION(nt);
  for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
    if (!(sec->Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
    const uint8_t* lo = (const uint8_t*)(g_base + sec->VirtualAddress);
    const uint8_t* hi = lo + sec->Misc.VirtualSize;
    if (hi - lo < (ptrdiff_t)p.bytes.size()) continue;
    const uint8_t* end = hi - p.bytes.size();
    const uint8_t* cur = lo;
    while (cur <= end) {
      cur = (const uint8_t*)memchr(cur, p.bytes[0], end - cur + 1);
      if (!cur) break;
      bool ok = true;
      for (size_t k = 1; k < p.bytes.size(); ++k) if (p.mask[k] && cur[k] != p.bytes[k]) { ok = false; break; }
      if (ok && !hit((uintptr_t)cur)) return;
      ++cur;
    }
  }
}

void each_match(std::string_view pattern, const std::function<bool(uintptr_t)>& hit) { each(pattern, hit); }

uintptr_t find(std::string_view pattern) {
  uintptr_t r = 0;
  each(pattern, [&](uintptr_t a) { r = a; return false; });
  return r;
}
uintptr_t find_unique(std::string_view pattern) {
  uintptr_t r = 0; int n = 0;
  each(pattern, [&](uintptr_t a) { if (!n) r = a; return ++n < 2; });
  return n == 1 ? r : 0;
}

uintptr_t find_string(const char* text) {
  init();
  auto* base = (uint8_t*)g_base;
  auto* nt = (IMAGE_NT_HEADERS64*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
  auto* sec = IMAGE_FIRST_SECTION(nt);
  size_t len = strlen(text) + 1;
  for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i)
    if (!(sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE))
      for (uint8_t *p = base + sec[i].VirtualAddress, *e = p + sec[i].Misc.VirtualSize - len; p < e; ++p)
        if (*p == (uint8_t)text[0] && memcmp(p, text, len) == 0 && p[-1] == 0) return (uintptr_t)p;
  return 0;
}

uintptr_t find_pointer(uintptr_t value, size_t skip) {
  init();
  auto* base = (uint8_t*)g_base;
  auto* nt = (IMAGE_NT_HEADERS64*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
  auto* sec = IMAGE_FIRST_SECTION(nt);
  for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i)
    if (!(sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE))
      for (uint8_t *p = base + sec[i].VirtualAddress + skip, *e = base + sec[i].VirtualAddress + sec[i].Misc.VirtualSize - 8;
           p < e; p += 8)
        if (*(uintptr_t*)p == value) return (uintptr_t)p;
  return 0;
}
}  // namespace p3a::scan

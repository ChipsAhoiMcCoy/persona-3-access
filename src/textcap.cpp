#include "textcap.h"
#include "devserver.h"
#include "hooks.h"
#include "log.h"
#include "seh.h"
#include "ue/scan.h"
#include "ue/ue.h"
#include <windows.h>
#include <format>
#include <algorithm>
#include <map>
#include <mutex>

namespace p3a::textcap {
// The font draw entry points and their argument shapes come from the P3R modding community's reverse engineering
// (rirurin/p3rpc.slplus CampMenuHooks, AnimatedSwine37/p3rpc.socialStatTracker UI.cs): TextData has x/y at 0/4,
// an ARGB colour at 0xC and the FString at 0x18.
struct TextData { float x, y; uint32_t pad8; uint32_t color; float scale; uint32_t pad14; ue::FString str; };
using DrawFont_t = void (*)(TextData*, intptr_t, float, float, float, float, uint8_t, int);
using FontDrawEx_t = void (*)(float, float, float, uint32_t, float, float, float, float, ue::FString*, uint32_t, uint32_t, float*);
static DrawFont_t g_df1, g_df2;
static FontDrawEx_t g_fdx;
static std::mutex g_mu;
static std::map<std::string, Item> g_items;   // keyed by text + rounded position
static uint64_t g_calls[3];

static void record(float x, float y, uint32_t color, const ue::FString& s, int src) {
  ++g_calls[src];
  std::string t;
  if (!guarded([&] { t = ue::to_utf8(s); })) return;
  if (t.empty()) return;
  std::string key = std::format("{}|{}|{}", (int)x, (int)y, t);
  std::lock_guard lk(g_mu);
  auto& it = g_items[key];
  it = Item{x, y, color, std::move(t), GetTickCount64(), src};
  if (g_items.size() > 4000) {   // prune the stale half
    uint64_t now = GetTickCount64();
    for (auto i = g_items.begin(); i != g_items.end();) i = now - i->second.seen_ms > 2000 ? g_items.erase(i) : std::next(i);
  }
}
static void df1_hook(TextData* t, intptr_t a2, float a3, float a4, float a5, float a6, uint8_t a7, int a8) {
  if (t) record(t->x, t->y, t->color, t->str, 0);
  g_df1(t, a2, a3, a4, a5, a6, a7, a8);
}
static void df2_hook(TextData* t, intptr_t a2, float a3, float a4, float a5, float a6, uint8_t a7, int a8) {
  if (t) record(t->x, t->y, t->color, t->str, 1);
  g_df2(t, a2, a3, a4, a5, a6, a7, a8);
}
static void fdx_hook(float x, float y, float z, uint32_t color, float size, float angle, float ax, float ay, ue::FString* s, uint32_t id, uint32_t pt, float* m) {
  if (s) record(x, y, color, *s, 2);
  g_fdx(x, y, z, color, size, angle, ax, ay, s, id, pt, m);
}

std::vector<Item> snapshot(unsigned max_age_ms) {
  std::vector<Item> out;
  uint64_t now = GetTickCount64();
  {
    std::lock_guard lk(g_mu);
    for (auto& [k, v] : g_items) if (now - v.seen_ms <= max_age_ms) out.push_back(v);
  }
  std::sort(out.begin(), out.end(), [](const Item& a, const Item& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
  return out;
}
std::string status() { return std::format("textcap calls drawfont={} drawfont2={} fontdrawex={}", g_calls[0], g_calls[1], g_calls[2]); }

bool install() {
  uintptr_t a1 = scan::find_unique("4C 8B DC 49 89 5B ?? 49 89 73 ?? 55 49 8D AB ?? ?? ?? ??");
  uintptr_t a2 = scan::find_unique("4C 8B DC 53 55 48 81 EC A8 01 00 00");
  uintptr_t a3 = scan::find("E8 ?? ?? ?? ?? 48 8B 4D ?? E9 ?? ?? ?? ?? 44 0F 29 9C 24 ?? ?? ?? ??");
  uintptr_t b = scan::exe_base();
  log::writef("textcap: DrawFont={:#x} DrawFont2={:#x} FontDrawEx call={:#x}", a1 ? a1 - b : 0, a2 ? a2 - b : 0, a3 ? a3 - b : 0);
  if (a1) { g_df1 = (DrawFont_t)a1; hooks::attach((void**)&g_df1, (void*)&df1_hook, "DrawFont"); }
  if (a2) { g_df2 = (DrawFont_t)a2; hooks::attach((void**)&g_df2, (void*)&df2_hook, "DrawFont2"); }
  if (a3) { g_fdx = (FontDrawEx_t)scan::rip(a3 + 1); hooks::attach((void**)&g_fdx, (void*)&fdx_hook, "FontDrawEx"); }
  dev::route("/text", [](const dev::Request& r, int&) {
    std::string out = status() + "\n";
    for (auto& it : snapshot((unsigned)dev::arg_int(r, "age", 150)))
      out += std::format("{:7.1f} {:7.1f} {:08x} s{} {}\n", it.x, it.y, it.color, it.source, it.text);
    return out;
  }, "?age=ms strings drawn by the game's font routines recently");
  return a1 || a2;
}
void remove() {
  if (g_df1) hooks::detach((void**)&g_df1, (void*)&df1_hook);
  if (g_df2) hooks::detach((void**)&g_df2, (void*)&df2_hook);
  if (g_fdx) hooks::detach((void**)&g_fdx, (void*)&fdx_hook);
}
}  // namespace p3a::textcap

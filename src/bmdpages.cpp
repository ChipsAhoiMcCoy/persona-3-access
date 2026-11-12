// Every BMD page the game builds, rendered to text once and kept with its time: what the message windows read their
// later pages from, and what screens ask for ("which help line appeared since the cursor moved").
#include "msg.h"
#include "devserver.h"
#include "hooks.h"
#include "log.h"
#include "seh.h"
#include "ue/scan.h"
#include <windows.h>
#include <climits>
#include <deque>
#include <format>
#include <map>
#include <mutex>
#include <tuple>
#include <algorithm>

namespace p3a::msg {
// ---- every BMD page the game builds (UI descriptions, help lines, menus drawn from message files) ----
// FBmdPage::CreateBmdPage(wrapper, dialog, page, flags) -> FCurrentBmdPage*; we render each new page once and keep
// it in a ring with its time, so screens can ask "what text appeared since the cursor moved".
using CreatePage_t = void* (*)(void*, int, int, uint32_t);
static CreatePage_t g_create_page;
static std::mutex g_bmd_mu;
static std::deque<Created> g_created;
static uint64_t g_created_seq = 0;
// Pages are rebuilt every frame by UI code: a page is identified by its BMD buffer and page data (+0x18/+0x20), and
// rendered only when it is new or its layout changed: a page given new text (a menu's help line following the cursor
// in the same page, 2026-10-27) gets a new overview (+0x28) or first line (overview +0x48), so the text is read the
// frame it changes. Two entry points feed it: CreateBmdPage (most UI and
// message windows) and BmdSetTextSize, the layout pass 37 call sites run on pages they build themselves
// (subtitles, battle and camp text; exe+0x124b4b0, found 2026-10-27 from the p3rpc.slplus call-site signature).
struct LivePage { std::string text; uint64_t first_ms, last_frame; void* wrapper; int dialog, page; void* layout = nullptr; void* line = nullptr; uint64_t seq = 0; uint64_t built_frame = 0; };   // seq: its text's Created; built_frame: when its text was built
static std::map<std::pair<void*, void*>, LivePage> g_live;
// (buffer, dialog, page) -> text, by CreateBmdPage. A buffer's address is reused by the next message file loaded
// there, so a dialog built again from its page 0 drops its later pages: a tutorial's page turned must not join a page
// of an earlier file's dialog (2026-10-31).
static std::map<std::tuple<void*, int, int>, std::string> g_dialogs;
static void record_page(void* r, void* wrapper, int dialog, int page) {
  void* buf = nullptr; void* data = nullptr;
  if (!guarded([&] { buf = *(void**)((uint8_t*)r + 0x18); data = *(void**)((uint8_t*)r + 0x20); })) return;
  uint64_t now = GetTickCount64();
  auto key = std::make_pair(buf, data);
  void* layout = nullptr; void* line = nullptr;
  guarded([&] { layout = *(void**)((uint8_t*)r + 0x28); line = layout ? *(void**)((uint8_t*)layout + 0x48) : nullptr; });
  bool known = false;
  {
    std::lock_guard lk(g_bmd_mu);
    auto it = g_live.find(key);
    if (it != g_live.end()) {
      it->second.last_frame = hooks::frame();
      if (dialog >= 0) { it->second.dialog = dialog; it->second.page = page; it->second.wrapper = wrapper; }
      // The same page laid out again: nothing new. New text in it: rendered, a new page if the text changed.
      if (it->second.layout == layout && it->second.line == line) return;
      it->second.layout = layout; it->second.line = line;
      known = true;
    }
  }
  std::string t = page_string(r);
  std::lock_guard lk(g_bmd_mu);
  if (known) {
    auto it = g_live.find(key);
    if (it == g_live.end() || it->second.text == t) return;
    it->second.text = t;
    it->second.first_ms = now;
    it->second.built_frame = hooks::frame();
  } else {
    g_live[key] = LivePage{t, now, hooks::frame(), wrapper, dialog, page, layout, line, 0, hooks::frame()};
    if (g_live.size() > 512) for (auto i = g_live.begin(); i != g_live.end();) i = hooks::frame() - i->second.last_frame > 300 ? g_live.erase(i) : std::next(i);
  }
  if (!t.empty()) {
    g_created.push_back(Created{++g_created_seq, now, wrapper, dialog, page, t, buf, data, r, hooks::frame()});
    if (g_created.size() > 200) g_created.pop_front();
    if (auto it = g_live.find(key); it != g_live.end()) it->second.seq = g_created_seq;
    if (dialog >= 0) {
      if (g_dialogs.size() > 2000) g_dialogs.clear();
      for (auto it = g_dialogs.lower_bound({buf, dialog, INT_MIN}); it != g_dialogs.end() && std::get<0>(it->first) == buf && std::get<1>(it->first) == dialog;)
        it = page == 0 && std::get<2>(it->first) > 0 ? g_dialogs.erase(it) : std::next(it);
      g_dialogs[{buf, dialog, page}] = t;
    }
  }
}
// The pages of one dialog of a buffer as last built, joined (a tutorial's page turned back to: nothing is rebuilt).
std::string dialog_text(void* buf, int dialog) {
  std::lock_guard lk(g_bmd_mu);
  std::string out;
  for (auto it = g_dialogs.lower_bound({buf, dialog, INT_MIN}); it != g_dialogs.end() && std::get<0>(it->first) == buf && std::get<1>(it->first) == dialog; ++it)
    out += (out.empty() ? "" : " ") + it->second;
  return out;
}
static void* create_page_hook(void* wrapper, int dialog, int page, uint32_t flags) {
  void* r = g_create_page(wrapper, dialog, page, flags);
  if (r) record_page(r, wrapper, dialog, page);
  return r;
}
using SetTextSize_t = void* (*)(void*, float, uint32_t, uint32_t, uint8_t, uint8_t, void*);
static SetTextSize_t g_set_text_size;
static void* set_text_size_hook(void* page, float a2, uint32_t lines, uint32_t a4, uint8_t a5, uint8_t a6, void* a7) {
  void* r = g_set_text_size(page, a2, lines, a4, a5, a6, a7);
  if (page) record_page(page, nullptr, -1, -1);
  return r;
}
std::vector<Created> visible(unsigned frames) {
  std::lock_guard lk(g_bmd_mu);
  std::vector<Created> out;
  uint64_t now = hooks::frame();
  for (auto& [k, v] : g_live)
    if (now - v.last_frame <= frames && !v.text.empty()) out.push_back(Created{v.seq, v.first_ms, v.wrapper, v.dialog, v.page, v.text, k.first, k.second});
  std::sort(out.begin(), out.end(), [](const Created& a, const Created& b) { return a.ms < b.ms; });
  return out;
}
std::vector<Created> pages_of(const void* buf) {
  std::lock_guard lk(g_bmd_mu);
  std::vector<Created> out;
  for (auto& [k, v] : g_live)
    if (k.first == buf && !v.text.empty()) out.push_back(Created{v.seq, v.first_ms, v.wrapper, v.dialog, v.page, v.text, k.first, k.second});
  return out;
}
PageInfo page_info(const void* page) {
  void* buf = nullptr; void* data = nullptr;
  if (!page || !guarded([&] { buf = *(void**)((uint8_t*)page + 0x18); data = *(void**)((uint8_t*)page + 0x20); })) return {};
  std::lock_guard lk(g_bmd_mu);
  auto it = g_live.find({buf, data});
  return it != g_live.end() ? PageInfo{it->second.dialog, it->second.page, it->second.built_frame} : PageInfo{};
}
std::vector<Created> created_since(uint64_t seq) {
  std::lock_guard lk(g_bmd_mu);
  std::vector<Created> out;
  for (auto& c : g_created) if (c.seq > seq) out.push_back(c);
  return out;
}
uint64_t created_seq() { std::lock_guard lk(g_bmd_mu); return g_created_seq; }
void forget_pages(const void* buf) {
  std::lock_guard lk(g_bmd_mu);
  for (auto i = g_live.begin(); i != g_live.end();) i = i->first.first == buf ? g_live.erase(i) : std::next(i);
}

// The hooks need the page renderer (msg::install found it): pages are rendered as they are recorded.
void install_pages(bool renderer) {
  uintptr_t b = scan::exe_base();
  uintptr_t cp = scan::find_unique("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC 20 48 63 D2");   // Episode Aigis build
  if (!cp) cp = scan::find_unique("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 63 D2");
  log::writef("msg: CreateBmdPage={:#x}", cp ? cp - b : 0);
  if (cp && renderer) { g_create_page = (CreatePage_t)cp; hooks::attach((void**)&g_create_page, (void*)&create_page_hook, "FBmdPage::CreateBmdPage"); }
  uintptr_t st = scan::find("E8 ?? ?? ?? ?? 48 8D 4C 24 ?? 4C 8B F8");
  if (st && renderer) { g_set_text_size = (SetTextSize_t)scan::rip(st + 1); hooks::attach((void**)&g_set_text_size, (void*)&set_text_size_hook, "FBmdPage::BmdSetTextSize"); }
  dev::route("/bmd", [](const dev::Request& r, int&) {
    std::string out;
    for (auto& c : created_since((uint64_t)dev::arg_int(r, "since", 0)))
      out += std::format("{} +{}ms d{} p{} w{} buf{} data{}: {}\n", c.seq, GetTickCount64() - c.ms, c.dialog, c.page, c.wrapper, c.buf, c.data, c.text);
    if (dev::has(r, "live")) { out += "-- visible:\n"; for (auto& c : visible(18)) out += std::format("d{} p{} w{}: {}\n", c.dialog, c.page, c.wrapper, c.text); }
    return out.empty() ? std::string("none\n") : out;
  }, "?since=seq BMD pages the game built (text); &live=1 adds what is on screen now");
}
}  // namespace p3a::msg

#include "bmdfile.h"
#include "core/text_clean.h"
#include "devserver.h"
#include "hooks.h"
#include "party.h"
#include "seh.h"
#include "watch.h"
#include <format>
#include <map>

namespace p3a::bmdfile {
using namespace ue;

struct Entry { const void* data; int size; std::shared_ptr<const core::bmd::File> file; };
static std::map<UObject*, Entry> g_cache;

std::shared_ptr<const core::bmd::File> load(UObject* a) {
  if (!a || !is_live(a)) return nullptr;
  auto* buf = field<TArray<uint8_t>>(a, "mBuf");
  if (!buf || !buf->data || buf->num <= 0) return nullptr;
  auto it = g_cache.find(a);
  if (it != g_cache.end() && it->second.data == buf->data && it->second.size == buf->num) return it->second.file;
  auto f = std::make_shared<core::bmd::File>();
  // A file that does not parse is not cached: the game may still be filling or converting it in place.
  if (!guarded([&] { *f = core::bmd::parse(buf->data, (size_t)buf->num); }) || !f->ok) return nullptr;
  g_cache[a] = Entry{buf->data, buf->num, f};
  return f;
}

std::shared_ptr<const core::bmd::File> load(const char* name) {
  static int assets = watch::want("BmdAsset", true);   // no GObjects scan per call
  for (UObject* a : watch::all(assets)) if (name_of(a) == name) return load(a);
  return nullptr;
}

const void* buffer(const char* name) {
  static int assets = watch::want("BmdAsset", true);
  for (UObject* a : watch::all(assets))
    if (name_of(a) == name) {
      auto* buf = field<TArray<uint8_t>>(a, "mBuf");
      return buf && buf->num > 0 ? buf->data : nullptr;
    }
  return nullptr;
}

std::string text(const core::bmd::Page& p) {
  core::bmd::Page pg = p;
  std::string hero;
  for (auto it = pg.functions.rbegin(); it != pg.functions.rend(); ++it) {
    if (it->table != 4 || it->index != 2 || it->pos > pg.text.size()) continue;
    if (hero.empty()) hero = party::name(1);
    pg.text.insert(it->pos, hero);
  }
  return core::clean_text(core::bmd::plain(pg));
}

void install() {
  dev::route("/bmdfile", [](const dev::Request& r, int&) {
    // /bmdfile?name=BMD_CampSystem[&fn=1] every dialog of a live BMD asset (fn=1 lists function tokens too)
    std::string name = dev::arg(r, "name");
    bool fns = dev::has(r, "fn");
    auto out = std::make_shared<std::string>("timeout\n");
    hooks::run_on_game_thread([out, name, fns] {
      auto f = load(name.c_str());
      if (!f || !f->ok) { *out = "not loaded or not a BMD\n"; return; }
      std::string s = std::format("{} dialogs, {} speakers\n", f->dialogs.size(), f->speakers.size());
      for (size_t i = 0; i < f->dialogs.size(); ++i) {
        auto& d = f->dialogs[i];
        s += std::format("[{}] {} {} speaker={}\n", i, d.kind == 1 ? "SEL" : "MSG", d.name, d.speaker);
        for (size_t p = 0; p < d.pages.size(); ++p) {
          s += std::format("   {}: {}\n", p, core::bmd::plain(d.pages[p]));
          if (fns) for (auto& fn : d.pages[p].functions) {
            std::string a;
            for (int v : fn.args) a += std::format(" {}", v);
            s += std::format("      f {} {}{} @{}\n", fn.table, fn.index, a, fn.pos);
          }
        }
      }
      *out = s;
    });
    return *out;
  }, "?name=BMD_xxx[&fn=1] dialogs of a loaded BMD asset");
}
}  // namespace p3a::bmdfile

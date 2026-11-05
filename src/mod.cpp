#include "mod.h"
#include "ad.h"
#include "cutin.h"
#include "picture.h"
#include "hooks.h"
#include "log.h"
#include "input.h"
#include "msg.h"
#include "world.h"
#include "walltones.h"
#include "settings.h"
#include "commands.h"
#include "nav.h"
#include "poi.h"
#include "area.h"
#include "gametext.h"
#include "stats.h"
#include "party.h"
#include "items.h"
#include "persona.h"
#include "social.h"
#include "bmdfile.h"
#include "audio.h"
#include "devserver.h"
#include "screens/screen.h"
#include "speech.h"
#include "textcap.h"
#include "voice.h"
#include "watch.h"
#include "ue/routes.h"
#include "ue/scan.h"
#include "ue/ue.h"
#include <windows.h>
#include <thread>
#include <memory>

namespace p3a::mod {
static bool g_ue_up = false;

// Once per mod tick on the game thread (hooks::tick). Every reader polls the game's own objects here, each timed for
// /perf (the average and worst microseconds per reader since the last reset).
struct Section { const char* name; void (*fn)(); uint64_t total_us = 0, max_us = 0; };
static Section g_sections[] = {
  {"watch", watch::tick}, {"probe", ue::probe_tick}, {"input", input::tick}, {"world", world::tick},
  {"voice", voice::tick}, {"msg", msg::tick}, {"screens", screens::tick},
  {"walltones", walltones::tick}, {"nav", nav::tick}, {"poi", poi::tick}, {"calendar", calendar::tick},
  {"stats", stats::tick}, {"social", social::tick}, {"party", party::tick}, {"ad", ad::tick}, {"cutin", cutin::tick}, {"picture", picture::tick},
};
static uint64_t g_perf_ticks = 0;
static void tick() {
  static LARGE_INTEGER qpf = [] { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
  for (auto& s : g_sections) {
    LARGE_INTEGER t0, t1;
    QueryPerformanceCounter(&t0);
    s.fn();
    QueryPerformanceCounter(&t1);
    uint64_t us = (uint64_t)((t1.QuadPart - t0.QuadPart) * 1000000 / qpf.QuadPart);
    s.total_us += us;
    if (us > s.max_us) s.max_us = us;
  }
  ++g_perf_ticks;
}
static std::string perf(bool reset) {
  std::string out = std::format("{} ticks\n", g_perf_ticks);
  for (auto& s : g_sections)
    out += std::format("{:<10} avg {:>5} us  max {:>6} us\n", s.name, g_perf_ticks ? s.total_us / g_perf_ticks : 0, s.max_us);
  out += screens::perf(reset);
  if (reset) { for (auto& s : g_sections) s.total_us = s.max_us = 0; g_perf_ticks = 0; }
  return out;
}

// The proxy loads us before the engine exists: wait until the object array and name pool are populated (the protection layer
// has also finished with the code by then) before scanning or hooking anything.
static bool wait_for_engine() {
  for (int i = 0; i < 1200; ++i) {   // up to 2 minutes
    if (ue::init() && ue::num_objects() > 20000) return true;
    Sleep(100);
  }
  return false;
}

void init(bool speech_ok) {
  if (!wait_for_engine()) {
    log::write("mod: the engine never came up (unknown build?); staying inert");
    speech::speak("Persona 3 Access could not find the game engine. This game version may be unsupported.", false);
    return;
  }
  ue::install_hooks();
  ue::register_routes();
  textcap::install();
  msg::install();
  watch::install();
  screens::register_all();
  world::register_routes();
  ad::register_routes();
  cutin::register_routes();
  picture::register_routes();
  audio::init();
  walltones::install();
  settings::install();
  voice::install();
  poi::install();
  area::install();
  nav::install();
  commands::install();
  bmdfile::install();
  gametext::install();
  stats::install();
  social::install();
  persona::install();
  party::install();
  dev::route("/screen", [](const dev::Request&, int&) {
    return dev::on_game([] { auto* s = screens::current(); return screens::status() + "\n" + (s ? s->describe() : std::string()) + "\n"; });
  }, "current modelled screen and its description");
  dev::route("/item", [](const dev::Request& r, int&) {
    auto out = std::make_shared<std::string>("timeout\n");
    std::string name = dev::arg(r, "name");
    hooks::run_on_game_thread([out, name] {
      items::Info in;
      *out = items::find(name, in) ? std::format("{} {}: id {:#x}, bag {:#x} (GlobalWork +{:#x}), owned {}\n", in.category, name, in.id, in.bag,
                                                 0x24E8 + in.bag, items::owned(in)) : std::string("unknown item\n");
    });
    return *out;
  }, "?name= an item's id, bag slot and how many are carried (dev)");
  dev::route("/perf", [](const dev::Request& r, int&) {
    bool reset = dev::has(r, "reset");
    return dev::on_game([reset] { return perf(reset); });
  }, "?reset=1 the mod tick's cost per reader and per screen since the last reset");
  g_ue_up = true;
  hooks::set_tick_callback(&tick);
  speech::speak(speech_ok ? "Persona 3 Access loaded" : "Persona 3 Access loaded, no speech backend", false);
}
void shutdown() {
  if (!g_ue_up) return;
  hooks::set_tick_callback(nullptr);
  audio::shutdown();
  watch::remove();
  textcap::remove();
  ue::remove_hooks();
}
std::string status() { return g_ue_up ? ue::status() + "\n" + textcap::status() + "\n" + msg::status() + "\n" + watch::status() + "\n" : "engine not up\n"; }
}  // namespace p3a::mod

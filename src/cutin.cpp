// Story cut-ins. A scene stresses a line with a close-up of a character that slides over the screen for a moment:
// a message holding only the BMD function `cut` (table 4, function 17: character, picture, side; 96 in the English
// story scripts, 2026-11-02), or the flowscript's CUTIN_START / CUTIN_START_EX (after-battle remarks, a few field
// scenes). Both show a CutInDraw (an AUIBaseActor), which loads T_UI_CI_<PC|SC><char %04d>_<pic %02d>: +0x2A0 is the
// character id, +0x2A4 the picture (FUN_1413b7390 passes both to the texture name's builder FUN_1413b26f0). One actor
// serves a scene's cut-ins one after another; +0x2C4, the slide-in's progress, starts again from 0 for each (0 to 0.77
// over 0.6 s, then held; sampled 2026-11-02). A cut-in begins when that progress leaves 0 or falls back: its
// description (strings::kCutins) is read then, and the dialogue line after it waits for it (msg::cutin_said).
// The BMD `cut` handler (FUN_14124c250) and CUTIN_START (FUN_1410e2090 -> FUN_1410e21b0) both ask the UI manager for
// UI 0x1D with the same parameters; CUTIN_STOP (FUN_1410e22c0) closes it. /cutin/test plays every picture through
// FUN_1410e21b0 (no script: its VM argument may be null) and checks each is read.
#include "cutin.h"
#include "core/strings.h"
#include "devserver.h"
#include "hooks.h"
#include "log.h"
#include "msg.h"
#include "seh.h"
#include "speech.h"
#include "world.h"
#include "screens/util.h"
#include "ue/scan.h"
#include <format>
#include <string>
#include <utility>
#include <vector>

namespace p3a::cutin {
using namespace ue;

namespace {
screens::Finder g_draw{"CutInDraw"};
constexpr int kChara = 0x2A0, kPic = 0x2A4, kProgress = 0x2C4;
WeakObj g_seen;          // the actor last looked at
float g_progress = 0;    // its slide-in progress then
bool g_primed = false;   // the first tick after loading: a cut-in already up is not new

// The self-test: each kCutins row started in turn; a row passes once tick() reads that picture.
struct Test { bool running = false; size_t row = 0; int wait = 0; std::vector<std::string> results; };
Test g_test;
int g_heard_chara = 0, g_heard_pic = 0;   // the cut-in tick() read last
using Start_t = void (*)(void* vm, int chara, int pic, int a4, int a5, int side, int a7);
using Stop_t = uint64_t (*)();
Start_t start_fn() {
  // CUTIN_START's handler loads its three script arguments and calls the builder: the call's target
  static uintptr_t at = scan::find_unique("C7 44 24 30 00 00 00 00 41 B9 FF FF FF FF 89 44 24 28 44 8B C3 8B D7 C7 44 24 20 FF FF FF FF 48 8B CE E8");
  return at ? (Start_t)scan::rip(at + 35) : nullptr;
}
Stop_t stop_fn() {
  static uintptr_t at = scan::find_unique("48 83 EC 28 E8 ?? ?? ?? ?? 48 85 C0 74 13 45 33 C9 48 8B C8 41 8D 51 1D 45 8D 41 01 E8");
  return (Stop_t)at;
}
constexpr int kTestTicks = 600;   // about 10 s for a picture to load and slide in before the row fails

void test_tick() {
  if (!g_test.running) return;
  auto& rows = strings::kCutins;
  constexpr size_t n = sizeof(rows) / sizeof(rows[0]);
  if (g_test.row >= n) { g_test.running = false; guarded([] { stop_fn()(); }); return; }
  const auto& c = rows[g_test.row];
  if (g_test.wait == 0) {
    g_heard_chara = g_heard_pic = 0;
    guarded([&] { stop_fn()(); start_fn()(nullptr, c.chara, c.pic, -1, -1, 0, 0); });
  }
  bool heard = g_heard_chara == c.chara && g_heard_pic == c.pic;
  if (heard || ++g_test.wait > kTestTicks) {
    g_test.results.push_back(std::format("{} {}/{}: {}", heard ? "ok  " : "FAIL", c.chara, c.pic, c.text));
    ++g_test.row;
    g_test.wait = 0;
  }
}

const char* describe(int chara, int pic) {
  for (auto& c : strings::kCutins) if (c.chara == chara && c.pic == pic) return c.text;
  return nullptr;
}
}  // namespace

void tick() {
  test_tick();
  UObject* d = g_draw.get();
  bool primed = std::exchange(g_primed, true);
  if (!d) return;
  int chara = 0, pic = 0;
  float progress = 0;
  if (!guarded([&] {
        chara = *(int32_t*)((uint8_t*)d + kChara);
        pic = *(int32_t*)((uint8_t*)d + kPic);
        progress = *(float*)((uint8_t*)d + kProgress);
      })) return;
  WeakObj w = WeakObj::of(d);
  bool same = w == g_seen;
  float was = same ? g_progress : primed ? 0 : progress;
  g_seen = w;
  g_progress = progress;
  if (chara <= 0 || progress <= 0 || (was > 0 && progress >= was)) return;   // nothing new sliding in
  const char* text = describe(chara, pic);
  log::writef("cutin: character {} picture {}{}", chara, pic, text ? "" : " (no description)");
  g_heard_chara = chara; g_heard_pic = pic;
  if (!text || world::scene_fast()) return;   // fast-forward reads no dialogue either
  speech::speak(text, false);
  msg::cutin_said();
}

void register_routes() {
  dev::route("/cutin/test", [](const dev::Request& r, int&) {
    bool start = dev::has(r, "start");
    return dev::on_game([start] {
      if (start) {
        if (!start_fn() || !stop_fn()) return std::string("CUTIN_START / CUTIN_STOP not found\n");
        g_test = Test{};
        g_test.running = true;
        return std::string("started\n");
      }
      std::string out = std::format("{} ({} of {})\n", g_test.running ? "running" : "done", g_test.results.size(),
                                    sizeof(strings::kCutins) / sizeof(strings::kCutins[0]));
      for (auto& l : g_test.results) out += l + "\n";
      return out;
    });
  }, "?start=1 play every cut-in picture through the game's CUTIN_START and check each is read; no args: results (dev)");
}
}  // namespace p3a::cutin

// The controller's mod layer: both triggers open it, a lone trigger is held back, a tap is replayed or claimed,
// a held button repeats, and a mod menu takes the whole controller.
#include "doctest.h"
#include <cstdint>
#include <utility>
#include <vector>
#include "core/pad_layer.h"

using namespace p3a::core;

namespace {
constexpr uint16_t kA = 0x1000, kB = 0x2000, kDown = 0x0002;

// One controller: poll() is one read of the pad at a time in milliseconds; `seen` is what the game gets.
struct Pad {
  PadLayer layer;
  PadLayerOptions opt;
  PadLayerOutput out;
  PadState seen;
  const PadState& poll(uint64_t now, uint16_t buttons, uint8_t lt = 0, uint8_t rt = 0) {
    out = PadLayerOutput{};
    seen = PadState{};
    seen.buttons = buttons; seen.lt = lt; seen.rt = rt;
    layer.filter(seen, now, opt, out);
    return seen;
  }
  // The call's events as (button, repeat) pairs.
  std::vector<std::pair<int, bool>> events() const {
    std::vector<std::pair<int, bool>> v;
    for (int i = 0; i < out.count; ++i) v.push_back({out.events[i].button, out.events[i].repeat});
    return v;
  }
};
using Events = std::vector<std::pair<int, bool>>;
}  // namespace

TEST_CASE("pad layer: buttons pass through while no trigger is held") {
  Pad p;
  CHECK(p.poll(0, kA).buttons == kA);
  CHECK(p.out.count == 0);
  CHECK(p.out.tap == 0);
  CHECK_FALSE(p.layer.on);
}

TEST_CASE("pad layer: both triggers open it once and hide themselves from the game") {
  Pad p;
  p.poll(0, 0, 255, 255);
  CHECK(p.layer.on);
  CHECK(p.events() == Events{{0, false}});
  CHECK(p.seen.lt == 0);
  CHECK(p.seen.rt == 0);
  p.poll(10, 0, 255, 255);
  CHECK(p.out.count == 0);   // still open: not announced again
  CHECK(p.layer.on);
}

TEST_CASE("pad layer: a button in the layer is the mod's and repeats while held") {
  Pad p;
  p.poll(0, 0, 255, 255);
  p.poll(10, kDown, 255, 255);
  CHECK(p.events() == Events{{kDown, false}});
  CHECK(p.seen.buttons == 0);
  p.poll(459, kDown, 255, 255);
  CHECK(p.out.count == 0);
  p.poll(460, kDown, 255, 255);   // 450 ms after the press
  CHECK(p.events() == Events{{kDown, true}});
  p.poll(589, kDown, 255, 255);
  CHECK(p.out.count == 0);
  p.poll(590, kDown, 255, 255);   // then every 130 ms
  CHECK(p.events() == Events{{kDown, true}});
  p.poll(600, 0, 255, 255);
  p.poll(2000, 0, 255, 255);
  CHECK(p.out.count == 0);        // released: no more repeats
}

TEST_CASE("pad layer: the sticks still reach the game in the layer") {
  Pad p;
  p.out = PadLayerOutput{};
  p.seen = PadState{};
  p.seen.lt = p.seen.rt = 255; p.seen.lx = 20000; p.seen.ry = -5;
  p.layer.filter(p.seen, 0, p.opt, p.out);
  CHECK(p.seen.lx == 20000);
  CHECK(p.seen.ry == -5);
}

TEST_CASE("pad layer: what is held when the layer closes stays hidden until released") {
  Pad p;
  p.poll(0, 0, 255, 255);
  p.poll(10, kA, 255, 255);
  p.poll(20, kA, 0, 0);            // the triggers let go, A still down
  CHECK_FALSE(p.layer.on);
  CHECK(p.seen.buttons == 0);
  CHECK(p.out.count == 0);
  p.poll(1000, kA, 0, 0);
  CHECK(p.seen.buttons == 0);
  CHECK(p.out.count == 0);         // and it no longer repeats
  p.poll(1010, 0);
  CHECK(p.poll(1020, kA).buttons == kA);
}

TEST_CASE("pad layer: one trigger still held when the layer closes stays hidden too") {
  Pad p;
  p.poll(0, 0, 255, 255);
  p.poll(10, 0, 255, 0);
  CHECK_FALSE(p.layer.on);
  CHECK(p.seen.lt == 0);
  p.poll(500, 0, 255, 0);
  CHECK(p.seen.lt == 0);
  p.poll(510, 0, 0, 0);
  p.poll(520, 0, 255, 0);          // a new press: held back first, then the game's
  CHECK(p.seen.lt == 0);
  CHECK(p.poll(670, 0, 255, 0).lt == 255);
}

TEST_CASE("pad layer: a lone trigger reaches the game after the grace") {
  Pad p;
  CHECK(p.poll(0, 0, 0, 255).rt == 0);
  CHECK(p.poll(149, 0, 0, 255).rt == 0);
  CHECK(p.poll(150, 0, 0, 255).rt == 255);
  CHECK(p.poll(400, 0, 0, 255).rt == 255);
  CHECK(p.out.count == 0);
  CHECK_FALSE(p.layer.on);
}

TEST_CASE("pad layer: the grace is the caller's; none means no hold-back") {
  Pad p;
  p.opt.grace_ms = 0;
  CHECK(p.poll(0, 0, 0, 255).rt == 255);
  Pad q;
  q.opt.grace_ms = 250;
  CHECK(q.poll(0, 0, 255, 0).lt == 0);
  CHECK(q.poll(249, 0, 255, 0).lt == 0);
  CHECK(q.poll(250, 0, 255, 0).lt == 255);
}

TEST_CASE("pad layer: the second trigger within the grace opens the layer and the game never sees the first") {
  Pad p;
  CHECK(p.poll(0, 0, 0, 255).rt == 0);
  p.poll(80, 0, 255, 255);
  CHECK(p.layer.on);
  CHECK(p.events() == Events{{0, false}});
  CHECK(p.seen.lt == 0);
  CHECK(p.seen.rt == 0);
}

TEST_CASE("pad layer: a tap shorter than the grace is replayed for the game") {
  Pad p;
  CHECK(p.poll(0, 0, 0, 255).rt == 0);
  CHECK(p.poll(60, 0, 0, 0).rt == 255);     // let go: the game gets a full press now
  CHECK(p.poll(100, 0, 0, 0).rt == 255);
  CHECK(p.poll(139, 0, 0, 0).rt == 255);
  CHECK(p.poll(140, 0, 0, 0).rt == 0);      // for 80 ms
  CHECK(p.out.tap == 0);
  CHECK(p.poll(60 + 1000, 0, 0, 0).rt == 0);
}

TEST_CASE("pad layer: a claimed lone trigger is a tap once the other had 50 ms to follow, and never the game's") {
  Pad p;
  p.opt.claim_taps = true;
  CHECK(p.poll(0, 0, 255, 0).lt == 0);
  CHECK(p.out.tap == 0);
  p.poll(49, 0, 255, 0);
  CHECK(p.out.tap == 0);
  p.poll(50, 0, 255, 0);
  CHECK(p.out.tap == -1);                   // LT
  CHECK(p.seen.lt == 0);
  p.poll(500, 0, 255, 0);                   // held on: one tap only, still hidden
  CHECK(p.out.tap == 0);
  CHECK(p.seen.lt == 0);
  p.poll(510, 0, 0, 0);
  CHECK(p.out.tap == 0);
  CHECK(p.seen.lt == 0);                    // no replay
}

TEST_CASE("pad layer: a claimed trigger let go before the 50 ms is a tap on its release") {
  Pad p;
  p.opt.claim_taps = true;
  p.poll(0, 0, 0, 255);
  CHECK(p.out.tap == 0);
  p.poll(20, 0, 0, 0);
  CHECK(p.out.tap == 1);                    // RT
  CHECK(p.seen.rt == 0);
  CHECK(p.poll(40, 0, 0, 0).rt == 0);       // not replayed for the game
}

TEST_CASE("pad layer: a claimed trigger with no grace is a tap at once") {
  Pad p;
  p.opt.claim_taps = true;
  p.opt.grace_ms = 0;
  p.poll(0, 0, 255, 0);
  CHECK(p.out.tap == -1);
  CHECK(p.seen.lt == 0);
}

TEST_CASE("pad layer: with taps claimed, the other trigger within 50 ms still opens the layer") {
  Pad p;
  p.opt.claim_taps = true;
  p.poll(0, 0, 255, 0);
  p.poll(30, 0, 255, 255);
  CHECK(p.out.tap == 0);
  CHECK(p.layer.on);
  CHECK(p.events() == Events{{0, false}});
}

TEST_CASE("pad layer: a trigger is held past 100 and until it falls under 40") {
  Pad p;
  CHECK(p.poll(0, 0, 100, 0).lt == 100);    // not a press yet: the game sees the raw value
  p.poll(10, 0, 255, 255);
  CHECK(p.layer.on);
  p.poll(20, 0, 50, 50);                    // easing off: still held
  CHECK(p.layer.on);
  CHECK(p.out.count == 0);
  p.poll(30, 0, 39, 50);
  CHECK_FALSE(p.layer.on);
}

TEST_CASE("pad layer: a mod menu takes every button and the game sees nothing") {
  Pad p;
  p.opt.modal = true;
  p.out = PadLayerOutput{};
  p.seen = PadState{};
  p.seen.buttons = kA | kDown; p.seen.lt = 200; p.seen.lx = 30000; p.seen.ly = -30000; p.seen.rx = 5; p.seen.ry = 7;
  p.layer.filter(p.seen, 0, p.opt, p.out);
  CHECK(p.events() == Events{{kDown, false}, {kA, false}});   // in bit order
  CHECK(p.seen.buttons == 0);
  CHECK(p.seen.lt == 0);
  CHECK(p.seen.lx == 0);
  CHECK(p.seen.ly == 0);
  CHECK(p.seen.rx == 0);
  CHECK(p.seen.ry == 0);
  p.poll(449, kA | kDown, 200, 0);
  CHECK(p.out.count == 0);
  p.poll(450, kA | kDown, 200, 0);          // the last one pressed (the highest bit) is the one that repeats
  CHECK(p.events() == Events{{kA, true}});
}

TEST_CASE("pad layer: triggers held through a mod menu are the layer already open when it closes") {
  Pad p;
  p.opt.modal = true;
  p.poll(0, 0, 255, 255);
  CHECK(p.out.count == 0);                  // no opening event inside the menu
  CHECK(p.layer.on);
  p.opt.modal = false;
  p.poll(10, kB, 255, 255);
  CHECK(p.events() == Events{{kB, false}}); // the layer's button, and no second opening
  CHECK(p.seen.buttons == 0);
  CHECK(p.seen.lt == 0);
  CHECK(p.seen.rt == 0);
}

TEST_CASE("pad layer: a button held when a mod menu closes stays hidden from the game") {
  Pad p;
  p.opt.modal = true;
  p.poll(0, kB);
  p.opt.modal = false;
  CHECK(p.poll(10, kB).buttons == 0);
  CHECK(p.out.count == 0);
  p.poll(20, 0);
  CHECK(p.poll(30, kB).buttons == kB);
}

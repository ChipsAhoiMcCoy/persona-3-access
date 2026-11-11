// Tartarus terminals ("SELECT TERMINAL": the Giant Device at the entrance and on the floors). AUIDungeonTransfer,
// native layout (measured 2026-10-28 on Thebel 5F): +0x340 TArray of 0x10-byte entries, the floors you can go to
// in floor order (+0x4 the floor, +0xC 1 for the furthest floor reached, also from the entrance), +0x300 the list's
// cursor (a ListCursor), +0x390 a copy of the entry under it, for the camera. The floor numbers are sprites; floor 1
// is the entrance.
#include "screens/menu.h"
#include "screens/util.h"
#include "core/strings.h"
#include "msg.h"
#include <format>

namespace p3a::screens {
using namespace ue;

// As a bespoke menu: the floors top to bottom as drawn (highest first). +0x294 is 4 while
// choosing; the cursor is the ListCursor at +0x300 (index = +0x324 + +0x328; +0x390 only copies the entry for the
// camera); up raises the index. A write is honoured (the camera follows +0x390 the next frame).
class TerminalScreen : public MenuScreen {
  Finder ui_{"UIDungeonTransfer"};
  uint8_t* u() { UObject* o = ui_.get(); return o && *(int32_t*)((uint8_t*)o + 0x294) == 4 ? (uint8_t*)o : nullptr; }
  TArray<uint8_t>* floors(uint8_t* p) {
    auto* arr = (TArray<uint8_t>*)(p + 0x340);
    return arr->data && arr->num > 0 && arr->num < 300 ? arr : nullptr;
  }
  Mirror& list_ = mirror("floors", [this] { uint8_t* p = u(); return p && floors(p) ? ListCursor(p + 0x300).index() : -1; });
  std::string floor(uint8_t* p, int i) {
    uint8_t* e = floors(p)->data + i * 0x10;
    int f = *(int32_t*)(e + 4);
    std::string s = f <= 1 ? std::string(strings::kEntrance) : std::format("{} {}", strings::kFloorTitle, f);
    if (*(int32_t*)(e + 0xC) == 1) s += std::string(", ") + strings::kYouAreHere;
    return s;
  }
 public:
  TerminalScreen() {
    list_.reversed = true;
    list_.set = [this](int i) { uint8_t* p = u(); return p && write_cursor(ListCursor(p + 0x300), i); };
  }
  const char* id() const override { return "terminal"; }
  int priority() const override { return 20; }
  bool active() override { uint8_t* p = u(); return p && floors(p) && !msg::current().blocking; }
  std::string title() override { return strings::kTerminal; }
  void on_enter() override { screens::set_keys(strings::kTerminalKeys); }
  void build(ui::GraphBuilder& b) override {
    uint8_t* p = u();
    auto* f = p ? floors(p) : nullptr;
    if (!f) return;
    b.begin_stop("floors");
    for (int i = f->num - 1; i >= 0; --i) {
      std::string l = floor(p, i);
      b.add_item(slot("floors", i), game_item(list_, i, [l] { return l; }));
    }
    if (int c = list_.cursor(); c >= 0) b.set_start(slot("floors", c));
  }
};

std::unique_ptr<Screen> make_terminal() { return std::make_unique<TerminalScreen>(); }
}  // namespace p3a::screens

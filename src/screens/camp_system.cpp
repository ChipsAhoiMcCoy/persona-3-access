// The camp's System submenu as a bespoke menu (menu.h), modelled from the game's own objects (the offsets are this
// build's, measured 2026-10-30). Its list uses the generic ListCursor (util.h): index = row + top.
//
//   UCmpSystem +0x54 == 4, +0x5C the open timer, ListCursor +0x64 (7 entries; locked by flags 0x40000089..8F),
//   +0xA0 the cursor's row copy, +0xA8 the draw (+0x13E4 the help line's fade).
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/camp.h"
#include "core/strings.h"
#include "party.h"

namespace p3a::screens {
using namespace ue;

class CampSystem : public MenuScreen {
  Finder menu_{"CmpSystem"};
  uint8_t* m() { UObject* o = camp_open() ? menu_.get() : nullptr; return o && i32(o, 0x54) == 4 ? (uint8_t*)o : nullptr; }
  Mirror& list_ = mirror("system", [this] { uint8_t* o = m(); return o ? ListCursor(o + 0x64).index() : -1; });
 public:
  // The draw reads the cursor every frame; the move adds its row copy (+0xA0), the help line's fade and the sound.
  CampSystem() {
    // FUN_14135b890 case 4: the open timer +0x5C and the UI layer's input (FUN_1412d5f60), what its confirm waits for.
    list_.ready = [this] {
      uint8_t* o = m();
      return o && open_timer_done(*(float*)(o + 0x5C), *(void**)(o + 0x30)) &&
             fn<bool (*)(void*)>(0x12d5f60)(*(void**)(fn<uint8_t* (*)()>(0x122ac90)() + 0x308A0));
    };
    list_.set = [this](int i) {
      uint8_t* o = m();
      if (!o || !ListCursor(o + 0x64).set(i)) return false;
      *(int32_t*)(o + 0xA0) = row_of(o + 0x64);
      if (uint8_t* d = *(uint8_t**)(o + 0xA8)) *(uint64_t*)(d + 0x13E4) = 0;
      cursor_sound();
      return true;
    };
  }
  const char* id() const override { return "camp_system"; }
  int priority() const override { return 17; }
  bool active() override { return m() != nullptr; }
  std::string title() override { return strings::kCampItems[8]; }
  void on_enter() override { screens::set_keys(strings::kListKeys); }
  void build(ui::GraphBuilder& b) override {
    b.begin_stop("system");
    for (int i = 0; i < 7; ++i) {
      bool locked = party::flag(0x40000089 + i);
      std::string label = strings::kSystemItems[i];
      if (locked) label += std::string(", ") + strings::kUnavailable;
      b.add_item(slot("system", i), game_item(list_, i, [label] { return label; }));
    }
    if (int c = list_.cursor(); c >= 0) b.set_start(slot("system", c));
  }
};

std::unique_ptr<Screen> make_camp_system() { return std::make_unique<CampSystem>(); }
}  // namespace p3a::screens

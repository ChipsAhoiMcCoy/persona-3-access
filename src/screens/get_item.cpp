// The field's item-get notice ("GOT <item> x10", OK to close), drawn by AUIMiscGetItemDraw. Its list of what was
// got is native (FUN_14148db00 draws it, 2026-11-02): +0x2F8 the entries (12 bytes each: +0 the item id as a u16,
// +4 the count, +8 set for money, drawn as a yen amount), +0x300 how many. The item's name is drawn from the id
// (FUN_1411f3980), the count as sprite digits; the mod reads both from the list. Its state +0x294 (the UI actors' task
// state) is 3 while it is up and takes OK, 4 as it closes, 5 once closed (2026-11-03, Yukari's Strap from her rank MAX
// event).
#include "screens/screen.h"
#include "screens/util.h"
#include "core/strings.h"
#include "items.h"
#include "log.h"
#include "seh.h"
#include "speech.h"
#include <format>

namespace p3a::screens {
using namespace ue;

class GetItemScreen : public Screen {
  Finder draw_{"UIMiscGetItemDraw"};
  std::string line_;   // the notice up now, as read last
  std::string said_;   // the notice spoken last while this screen was current
  // "Got Velvet Outfit times 10, 500 yen", or "" when the list is empty.
  std::string read() {
    uint8_t* d = (uint8_t*)draw_.get();
    if (!d || *(int32_t*)(d + 0x294) != 3) return {};   // 3 up; 4 closing, 5 closed (the list is emptied then)
    std::string s;
    guarded([&] {
      uint8_t* e = *(uint8_t**)(d + 0x2F8);
      int n = *(int32_t*)(d + 0x300);
      if (!e || n <= 0 || n > 32) return;
      for (int i = 0; i < n; ++i, e += 12) {
        int id = *(uint16_t*)e, count = *(int32_t*)(e + 4);
        std::string what;
        if (e[8]) what = std::format("{} {}", count, strings::kYen);
        else {
          what = items::name_of(id);
          if (what.empty()) what = std::format("{} {}", strings::kItemNumber, id);
          if (count > 1) what += std::format(" {} {}", strings::kTimes, count);
        }
        s += (s.empty() ? std::string(strings::kGot) + " " : std::string(", ")) + what;
      }
    });
    return s;
  }
 public:
  const char* id() const override { return "get_item"; }
  int priority() const override { return 20; }
  bool active() override {
    std::string l = read();
    if (l.empty()) return false;
    if (l != line_) {
      if (!line_.empty()) log::writef("get_item: {}", l);
      line_ = l;
    }
    return true;
  }
  void enter() override { screens::set_keys(strings::kGetItemClose); said_ = line_; speech::speak(line_, true); }
  // The next notice while this one is up: queued after it.
  void tick() override {
    if (line_ == said_) return;
    said_ = line_;
    speech::speak(line_, false);
  }
  std::string describe() override { return std::format("{}. {}", line_, strings::kGetItemClose()); }
};

std::unique_ptr<Screen> make_get_item() { return std::make_unique<GetItemScreen>(); }
}  // namespace p3a::screens

// The tutorial library (camp System > Tutorial): titles are drawn as text in one colour; AUITutorial +0x280 is the
// list's ListCursor (its row on screen is +0x2A4, measured 2026-10-27; the index is row + top), +0x278 the tab Q and
// E switch (0 Battle .. 4 System; the tab names are pictures; measured 2026-10-29). A tab with nothing in it draws one line, "There are no tutorials to display."
// The tutorial itself opens as a message window (msg reads it).
#include "screens/menu.h"
#include "screens/util.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "msg.h"
#include "ue/scan.h"
#include <format>

namespace p3a::screens {
using namespace ue;

// From memory: +0x27D the list has input, +0x27E a tutorial is open; +0x338 points to
// per-category TArrays (0x10 each) of FTutorialTableData (0x30: Title FString at +0x10); tab t shows category
// {0, 3, 2, 1, 4}[t]. The cursor is the ListCursor at +0x280 (10 rows, wraps); after a move the game calls
// exe+0x1558500(this) (sound, draw push), which a write calls too.
// The table loads asynchronously after the actor is made (FUN_141551550, once the handle at +0x350 has loaded): it
// sizes the category array (+0x340) to 5 and fills every category. Before that every tab reads as empty, and the first
// tab still read empty two frames after +0x340 was set, so the screen waits for +0x27D too ("no tutorials" was said on
// opening, 2026-11-01 and 2026-11-06).
class TutorialListScreen : public MenuScreen {
  Finder ui_{"UITutorial"};
  uint8_t* u() { return (uint8_t*)ui_.get(); }
  int tab() { int t = u() ? *(int32_t*)(u() + 0x278) : -1; return t >= 0 && t < 5 ? t : -1; }
  TArray<uint8_t>* rows() {
    uint8_t* p = u();
    int t = tab();
    uint8_t* cats = p ? *(uint8_t**)(p + 0x338) : nullptr;
    if (!cats || t < 0) return nullptr;
    static constexpr int kCategory[5] = {0, 3, 2, 1, 4};
    auto* a = (TArray<uint8_t>*)(cats + kCategory[t] * 0x10);
    return a->data && a->num > 0 && a->num < 500 ? a : nullptr;
  }
  Mirror& list_ = mirror("tutorials", [this] { uint8_t* p = u(); return p && p[0x27D] && !p[0x27E] ? ListCursor(p + 0x280).index() : -1; });
 public:
  TutorialListScreen() {
    // FUN_1415536c0: its confirm and cancel wait for the open timer +0x334 (frames * 1/20 s), moves only for +0x27D.
    list_.ready = [this] {
      uint8_t* p = u();
      return p && *(float*)(p + 0x334) >= ((int (*)(void*))(scan::exe_base() + 0x12e13b0))(*(void**)(p + 0x3A8)) * 0.05f;
    };
    list_.set = [this](int i) {
      uint8_t* p = u();
      if (!p || !ListCursor(p + 0x280).set(i)) return false;
      ((void (*)(void*))(scan::exe_base() + 0x1558500))(p);
      return true;
    };
  }
  const char* id() const override { return "tutorial_list"; }
  int priority() const override { return 17; }
  // Up while its actor lives (made on open, destroyed on close); a tutorial opened from it is a message window.
  bool active() override { return loaded() && !msg::current().open; }
  bool loaded() { uint8_t* p = u(); return p && p[0x27D] && *(uint8_t**)(p + 0x338) && *(int32_t*)(p + 0x340) == 5; }
  bool holds_keys() override { uint8_t* p = u(); return p && p[0x27D] && !p[0x27E]; }
  std::string title() override { return strings::kTutorials; }
  void on_enter() override { screens::set_keys(strings::kTutorialListKeys); }
  void build(ui::GraphBuilder& b) override {
    int t = tab();
    if (t < 0) return;
    std::string key = std::format("tutorials.{}", t);
    list_.key = key;
    b.begin_stop(key);
    b.push_context(strings::kTutorialTabs[t]);
    auto* r = rows();
    for (int i = 0; r && i < r->num; ++i) {
      std::string title = core::clean_text(to_utf8(*(FString*)(r->data + i * 0x30 + 0x10)));
      b.add_item(slot(key, i), game_item(list_, i, [title] { return title; }));
    }
    if (!r) b.add_item(ui::ControlId::structural(key + ".none"), item([] { return std::string(strings::kNoTutorials); }));
    b.pop_context();
    if (int c = list_.cursor(); c >= 0 && r) b.set_start(slot(key, c));
  }
};

std::unique_ptr<Screen> make_tutorial_list() { return std::make_unique<TutorialListScreen>(); }
}  // namespace p3a::screens

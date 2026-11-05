// The dictionary (camp System > Dictionary). AUIDictionary lives only while it is open; native layout (measured
// 2026-10-28): +0x294 the cursor over the whole list, +0x298 the top row shown, +0x29D 1 while the list has focus,
// +0x2A0 TArray of FDictionaryTableData (0x30 each: Title FString at +0x10) of the terms unlocked so far.
// A term opens as a message window (MsgDictionary: the term as speaker, its text as the page), which msg reads.
#include "screens/menu.h"
#include "screens/util.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "msg.h"
#include <format>

namespace p3a::screens {
using namespace ue;

class DictionaryScreen : public MenuScreen {
  Finder ui_{"UIDictionary"};
  Mirror& list_ = mirror("terms", [this] {
    UObject* u = ui_.get();
    return u && *((uint8_t*)u + 0x29D) ? *(int32_t*)((uint8_t*)u + 0x294) : -1;
  });
  TArray<uint8_t>* rows(UObject* u) {
    auto* r = (TArray<uint8_t>*)((uint8_t*)u + 0x2A0);
    return r->data && r->num > 0 && r->num <= 1000 ? r : nullptr;
  }
 public:
  // The ListCursor (+0x2B0) written, then exe+0x13bff60(ui), the rest of the game's move: the sound, the index copies
  // (+0x290 / +0x294 / +0x298) and the draw's rows. The list takes moves while it has focus (+0x29D).
  DictionaryScreen() {
    list_.ready = [this] { UObject* u = ui_.get(); return u && *((uint8_t*)u + 0x29D); };
    list_.set = [this](int i) {
      auto* u = (uint8_t*)ui_.get();
      if (!u || !u[0x29D] || !ListCursor(u + 0x2B0).set(i)) return false;
      ((void (*)(void*))(scan::exe_base() + 0x13bff60))(u);
      return true;
    };
  }
  const char* id() const override { return "dictionary"; }
  int priority() const override { return 17; }
  bool active() override { return ui_.get() != nullptr; }   // a term open over the list stays this screen
  bool own_messages() override { return ui_.get() != nullptr; }   // a term's page closes straight back to the list
  // Reading a term (its message window over the list): the keys are the page's.
  bool holds_keys() override { UObject* u = ui_.get(); return u && *((uint8_t*)u + 0x29D) && !msg::current().open; }
  std::string title() override { return strings::kDictionary; }
  void on_enter() override { screens::set_keys(strings::kListKeys); }
  void build(ui::GraphBuilder& b) override {
    UObject* u = ui_.get();
    auto* r = u ? rows(u) : nullptr;
    if (!r) return;
    b.begin_stop("terms");
    for (int i = 0; i < r->num; ++i) {
      std::string t = core::clean_text(to_utf8(*(FString*)(r->data + i * 0x30 + 0x10)));
      b.add_item(slot("terms", i), game_item(list_, i, [t] { return t; }));
    }
    if (int c = list_.cursor(); c >= 0) b.set_start(slot("terms", c));
  }
};

std::unique_ptr<Screen> make_dictionary() { return std::make_unique<DictionaryScreen>(); }
}  // namespace p3a::screens

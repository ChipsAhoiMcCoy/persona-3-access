// Mail (Ctrl in the field, and the morning's "I've got a text message..."): BP_MailDraw_C (AMailDraw).
// Reflected: SelectedIndex, MailListItems (0x20 each: +0x10 the title's FBmdPage, +0x18 flags, +0x1C sender id;
// measured 2026-10-27), and the animation flags that tell the list from an open mail (bIsStartListAnim /
// bIsStartMailAnim, each cleared again by its finish). The texts are BMD pages built when the mail opens, all from
// one buffer per mail: dialog 0 the sender's name, the title, then the body. The thought and the reply choice after
// reading are ordinary message windows (msg).
#include "screens/menu.h"
#include "screens/util.h"
#include "core/strings.h"
#include "devserver.h"
#include "hooks.h"
#include "msg.h"
#include "seh.h"
#include "speech.h"
#include "ue/scan.h"
#include <algorithm>
#include <format>
#include <memory>
#include <utility>
#include <vector>

namespace p3a::screens {
using namespace ue;

class MailScreen : public MenuScreen {
  Finder draw_{"MailDraw"}, actor_{"MailActor"};
  bool reading_ = false;
  std::string said_;
  struct Mail { std::string sender, title, body; bool fresh = false; };
  static bool flag(UObject* d, const char* n) { auto* b = field<bool>(d, n); return b && *b; }
  Mail mail(UObject* d, int i) {
    Mail m;
    auto* items = field<TArray<uint8_t>>(d, "MailListItems");
    if (!items || i < 0 || i >= items->num) return m;
    // The title is rendered from the item's own page, never looked up among the pages seen being built: a list
    // opened again builds the same page unchanged at another address, which is not a new page (it read "Mail 1"
    // every other time, 2026-11-11). The sender and the body are the other pages laid out from the title's buffer.
    void* title = nullptr; void* buf = nullptr; void* data = nullptr;
    guarded([&] {
      title = *(void**)(items->data + (size_t)i * 0x20 + 0x10);
      if (title) { buf = *(void**)((uint8_t*)title + 0x18); data = *(void**)((uint8_t*)title + 0x20); }
    });
    if (title) m.title = msg::page_string(title);
    int td = -1;
    auto pages = buf ? msg::pages_of(buf) : std::vector<msg::Created>{};
    for (auto& c : pages) if (c.data == data) td = c.dialog;
    // A sender's file is its name (dialog 0), then a title and its body for each mail: the body is the dialog after
    // the title. It is laid out once, with the list, not again when the mail opens (waiting for a newer layout read
    // no mail at all, 2026-11-11).
    std::vector<std::pair<int, std::string>> body;
    for (auto& c : pages) {
      if (c.dialog == td || c.data == data) continue;
      if (c.dialog == 0) m.sender = c.text;
      else if (td >= 0 && c.dialog == td + 1) body.emplace_back(c.page, c.text);
    }
    std::sort(body.begin(), body.end());
    for (auto& [p, t] : body) m.body += (m.body.empty() ? "" : " ") + t;
    // The sender, when no page of its own was built (the "Unknown Sender" of a mail with none): AMailActor +0x350 is
    // the list (0x30 each: +0x8 the sender's page).
    if (m.sender.empty()) {
      if (UObject* a = actor_.get()) {
        auto* arr = (TArray<uint8_t>*)((uint8_t*)a + 0x350);
        if (arr->data && i < arr->num && arr->num < 500) guarded([&] { m.sender = msg::page_string(*(void**)(arr->data + (size_t)i * 0x30 + 0x8)); });
      }
    }
    if (UFunction* f = find_function(class_of(d), "GetListIsNew")) {
      Params p(f);
      p.set<int32_t>("InIndex", i);
      if (call(d, p)) m.fresh = p.ret<bool>();
    }
    return m;
  }
  static std::string line(const Mail& m) {
    std::string s = m.sender.empty() ? m.title : m.sender + ": " + m.title;
    return m.fresh ? s + ", " + strings::kMailNew : s;
  }
  Mirror& list_ = mirror("mail", [this] {
    UObject* d = draw_.get();
    auto* si = d ? field<int32_t>(d, "SelectedIndex") : nullptr;
    auto* items = d ? field<TArray<uint8_t>>(d, "MailListItems") : nullptr;
    return si && items && *si >= 0 && *si < items->num && !flag(d, "bIsStartMailAnim") ? *si : -1;
  });
  // The actor while its list takes moves (its state +0x294 at 4, bit 0x10 of +0x388 clear), else null.
  uint8_t* list_actor() {
    auto* a = (uint8_t*)actor_.get();
    return a && *(int32_t*)(a + 0x294) == 4 && !(*(uint32_t*)(a + 0x388) & 0x10) ? a : nullptr;
  }
 public:
  MailScreen() {
    // The list's row / top (u16 AMailActor +0x37A / +0x37C, five rows shown over +0x358), then its sound (cue 0x1FB),
    // as the actor's state 4 moves them; the actor copies them to the draw (SelectedIndex) and clears the NEW mark of
    // the mail left every frame. Not the mail's mouse handler: that also opens the mail.
    list_.ready = [this] { return list_actor() != nullptr; };
    list_.set = [this](int i) {
      uint8_t* a = list_actor();
      if (!a) return false;
      int n = *(int32_t*)(a + 0x358);
      if (i < 0 || i >= n) return false;
      int top = core::scroll_top(i, *(uint16_t*)(a + 0x37C), 5, n);
      if (top + *(uint16_t*)(a + 0x37A) == i && top == *(uint16_t*)(a + 0x37C)) return true;
      *(uint16_t*)(a + 0x37C) = (uint16_t)top;
      *(uint16_t*)(a + 0x37A) = (uint16_t)(i - top);
      cursor_sound(0x1FB);
      return true;
    };
    // Dev: open the mail as the game's MAIL script command does (exe+0x1537980, 2026-10-27): the UI manager
    // (exe+0x134dce0) spawns UI type 32 (exe+0x1357240), then +0x382 = mode, bit 5 of +0x388 = flag. Ctrl, which
    // opens it in play, cannot be sent to an unfocused game.
    dev::route("/mailopen", [](const dev::Request& r, int&) {
      int mode = (int)dev::arg_int(r, "mode", 0), on = (int)dev::arg_int(r, "flag", 0);
      auto out = std::make_shared<std::string>("no ui manager\n");
      hooks::run_on_game_thread([=] {
        uintptr_t b = scan::exe_base();
        auto mgr = ((void* (*)())(b + 0x134dce0))();
        if (!mgr) return;
        auto actor = (uint8_t*)((void* (*)(void*, int))(b + 0x1357240))(mgr, 32);
        if (!actor) { *out = "spawn failed\n"; return; }
        *(uint16_t*)(actor + 0x382) = (uint16_t)mode;
        *(uint32_t*)(actor + 0x388) = (*(uint32_t*)(actor + 0x388) & ~0x20u) | (on ? 0x20u : 0);
        *out = std::format("mail actor {}\n", (void*)actor);
      });
      return *out;
    }, "?mode=0&flag=0 open the mail screen (dev)");
  }
  const char* id() const override { return "mail"; }
  int priority() const override { return 15; }
  bool active() override {
    UObject* d = draw_.get();
    // The actor stays after it closes (bIsFinishOutAnim, 2026-10-27).
    return d && flag(d, "bIsFinishInAnim") && !flag(d, "bIsStartOutAnim") && !flag(d, "bIsFinishOutAnim");
  }
  // An open mail is read whole when it opens; its keys (reply, next, scroll) are the game's.
  bool holds_keys() override { UObject* d = draw_.get(); return d && !flag(d, "bIsStartMailAnim"); }
  std::string title() override { return strings::kMail; }
  void on_enter() override { reading_ = false; said_.clear(); screens::set_keys(strings::kMailKeys); }
  void on_tick() override {
    UObject* d = draw_.get();
    auto* si = field<int32_t>(d, "SelectedIndex");
    if (!si) return;
    bool reading = flag(d, "bIsStartMailAnim");
    if (reading && !reading_) {
      Mail m = mail(d, *si);
      // Not laid out yet: wait for it while the list flips to the mail (bIsFlipToMainAnim, cleared by the flip's
      // end; bIsFinishMailAnim is never set while a mail is open). Once it is open, read what there is.
      if (m.body.empty() && flag(d, "bIsFlipToMainAnim")) return;
      std::string s = m.sender.empty() ? m.title : m.sender + ". " + m.title;
      if (!m.body.empty()) s += ". " + m.body;
      speech::speak(s, true); said_ = s; reading_ = true;
      return;
    }
    if (!reading && reading_) reading_ = false;
  }
  void build(ui::GraphBuilder& b) override {
    UObject* d = draw_.get();
    auto* items = field<TArray<uint8_t>>(d, "MailListItems");
    if (!items) return;
    b.begin_stop("mail");
    if (items->num == 0) b.add_item(ui::ControlId::structural("mail.none"), item([] { return std::string(strings::kMailEmpty()); }));
    for (int i = 0; i < items->num; ++i)
      b.add_item(slot("mail", i), game_item(list_, i, [this, i] {
        UObject* dd = draw_.get();
        Mail m = dd ? mail(dd, i) : Mail{};
        return m.title.empty() ? std::format("{} {}", strings::kMail, i + 1) : line(m);
      }));
    if (int c = list_.cursor(); c >= 0) b.set_start(slot("mail", c));
  }
  std::string describe() override { return reading_ && !said_.empty() ? said_ : MenuScreen::describe(); }
};

std::unique_ptr<Screen> make_mail() { return std::make_unique<MailScreen>(); }
}  // namespace p3a::screens

// The "SELECT" list (UIGenericSelect: who to think of at the shrine's fortune, the Velvet Room's Talk, the cafe's
// coffee: UGenericSelectShop, the same layout as UGenericSelectMsg). BP_GenericSelectDraw_C
// lives only while it is open; its PSystem (UGenericSelectMsg, native; measured 2026-10-28) holds +0x28 a TArray of
// TSharedPtr entries (object, refcount) whose object has the entry's BMD page at +0x8, and the heading's pages at
// +0x48 (the activity, "Fortune") and +0x50 (the question, "Who should I think of...?"). The highlighted entry is
// drawn dark on a white bar (the text itself stays white).
// The item variant (UGenericSelectItem: what to plant in the rooftop garden) keeps the same entry array, but each
// entry holds an item id (u16 at +0x8) instead of a page, and the heading is two FStrings at +0x40 (the activity) and
// +0x50 (the question). The game builds the item's description as BMD dialog <id & 0xFFF> when it is highlighted.
#include "screens/menu.h"
#include "screens/util.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "items.h"
#include "msg.h"
#include "textcap.h"
#include <windows.h>
#include <algorithm>
#include <format>
#include <map>
#include <vector>

namespace p3a::screens {
using namespace ue;

class GenericSelectScreen : public MenuScreen {
  Finder draw_{"BP_GenericSelectDraw_C"}, actor_{"UIGenericSelect"};
  Mirror& list_ = mirror("select", [this] { return cursor(); });
  std::vector<std::string> names_;
  std::vector<int> items_;   // the item variant: each entry's item id
  std::string heading_;
  bool item_list(uint8_t* sys) { return is_a((UObject*)sys, "GenericSelectItem"); }
  uint8_t* system() {
    UObject* d = draw_.get();
    return d ? (uint8_t*)get<UObject*>(d, "PSystem") : nullptr;
  }
  std::vector<std::string> entries(uint8_t* sys) {
    std::vector<std::string> out;
    auto* arr = (TArray<void*[2]>*)(sys + 0x28);
    if (!arr->data || arr->num <= 0 || arr->num > 64) return out;
    bool items = item_list(sys);
    for (int i = 0; i < arr->num; ++i) {
      uint8_t* obj = (uint8_t*)arr->data[i][0];
      if (!obj) out.push_back({});
      else if (items) out.push_back(items::name_of(*(uint16_t*)(obj + 0x8)));
      else out.push_back(msg::page_string(*(void**)(obj + 0x8)));
    }
    return out;
  }
  // The cursor: AUIGenericSelect's ListCursor (+0x2A4); the draw's +0x378 is its row on screen (+0x374 the top).
  int cursor() {
    auto* a = (uint8_t*)actor_.get();
    UObject* d = draw_.get();
    int c = a ? ListCursor(a + 0x2A4).index() : d ? *(int32_t*)((uint8_t*)d + 0x374) + *(int32_t*)((uint8_t*)d + 0x378) : -1;
    return c >= 0 && c < (int)names_.size() ? c : -1;
  }
  static std::string sentence(std::string s) {   // ends with a stop ("...?" keeps its own)
    if (!s.empty() && s.back() != '.' && s.back() != '?' && s.back() != '!') s += '.';
    return s;
  }
  int item_id(uint8_t* sys, int i) {
    auto* arr = (TArray<void*[2]>*)(sys + 0x28);
    if (!arr->data || i < 0 || i >= arr->num || !arr->data[i][0]) return -1;
    return *(uint16_t*)((uint8_t*)arr->data[i][0] + 0x8);
  }
  std::string heading(uint8_t* sys) {
    if (item_list(sys)) {
      std::string a = to_utf8(*(FString*)(sys + 0x40)), q = to_utf8(*(FString*)(sys + 0x50));
      return a.empty() ? q : q.empty() ? a : sentence(a) + " " + q;
    }
    std::string a = msg::page_string(*(void**)(sys + 0x48)), q = msg::page_string(*(void**)(sys + 0x50));
    return a.empty() ? q : q.empty() ? a : sentence(a) + " " + q;
  }
  // The item variant's description: the game builds it as BMD dialog <id & 0xFFF> once its cursor is on the item.
  // Other lists show an INFO line for the highlighted entry (the game over list's "Start over from the beginning of
  // the battle."): the page on screen that is neither the heading nor an entry, once the game's cursor is there,
  // kept per entry (the game builds it once: a re-read a moment later found nothing on screen).
  std::map<int, std::string> info_;
  std::string description(int i) {
    if (items_.empty()) {
      if (list_.seen == i && list_.target < 0) {
        std::string best;
        for (auto& p : msg::visible(300)) {
          std::string t = core::clean_text(p.text);
          if (t.empty() || heading_.find(t) != std::string::npos) continue;
          if (std::find(names_.begin(), names_.end(), t) != names_.end()) continue;
          best = t;
        }
        if (!best.empty()) info_[i] = best;
      }
      auto it = info_.find(i);
      return it != info_.end() ? it->second : std::string();
    }
    if (i < 0 || i >= (int)items_.size() || items_[i] < 0) return {};
    return items::help(items_[i]);   // its file's line: the drawn page is built once and matched by dialog alone
  }
 public:
  // The cursor written, then exe+0x1478330(actor, direction, 1), what the actor's own move runs: the sound, the
  // system's notice, the draw's rows, and the item variant's info. The list takes moves in the actor's state 5.
  GenericSelectScreen() {
    list_.ready = [this] { auto* a = (uint8_t*)actor_.get(); return a && *(int32_t*)(a + 0x294) == 5; };
    list_.set = [this](int i) {
      auto* a = (uint8_t*)actor_.get();
      ListCursor lc(a ? a + 0x2A4 : nullptr);
      int old = lc.index();
      if (!a || !lc.set(i)) return false;
      if (i != old) ((void (*)(void*, int, char))(scan::exe_base() + 0x1478330))(a, i > old ? 2 : 1, 1);
      return true;
    };
  }
  const char* id() const override { return "generic_select"; }
  int priority() const override { return 20; }
  bool active() override { return system() != nullptr; }
  std::string title() override { return sentence(heading_); }
  // The list is read as it opens, before its title and first entry are spoken, and forgotten as it closes: the model
  // kept from the last list spoke the Velvet Room's "Talk. What should I ask about?, Fusion Basics, 1 of 7" over the
  // cafe's coffee (2026-10-31).
  void on_enter() override { screens::set_keys(strings::kSelectKeys); read(); }
  void on_tick() override { read(); }
  void on_leave() override { names_.clear(); items_.clear(); heading_.clear(); info_.clear(); }
  void read() {
    uint8_t* sys = system();
    if (!sys) return;
    std::string h = heading(sys);
    if (h != heading_) info_.clear();   // another list
    heading_ = h;
    auto names = entries(sys);
    // Locked entries stay in the array (the Velvet Room's Talk holds 15 topics and shows 7): the list is what is
    // drawn, in array order.
    int shown = 0;
    auto drawn = textcap::snapshot(200);
    for (auto& n : names) {
      bool on = false;
      for (auto& t : drawn) if (t.source == 2 && core::clean_text(t.text) == n) on = true;
      if (!on) break;
      ++shown;
    }
    if (shown > 0) names.resize(shown);
    names_ = names;
    items_.clear();
    if (item_list(sys))
      for (int i = 0; i < (int)names_.size(); ++i) items_.push_back(item_id(sys, i));
  }
  void build(ui::GraphBuilder& b) override {
    b.begin_stop("select");
    int c = cursor();
    for (int i = 0; i < (int)names_.size(); ++i) {
      if (names_[i].empty()) continue;
      std::string name = names_[i];
      if (i < (int)items_.size()) {
        items::Info in;
        if (items::find(name, in)) if (int o = items::owned(in); o >= 0) name += std::format(", {} {}", o, strings::kOwned);
      }
      auto v = game_item(list_, i, [name] { return name; }, [this, i] { return description(i); });
      // The item's description, once the game has built it: read after the name.
      describe_with(v, [this, i] { return description(i); });
      b.add_item(slot("select", i), v);
    }
    if (c >= 0) b.set_start(slot("select", c));
  }
};

std::unique_ptr<Screen> make_generic_select() { return std::make_unique<GenericSelectScreen>(); }
}  // namespace p3a::screens

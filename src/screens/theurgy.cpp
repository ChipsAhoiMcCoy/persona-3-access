// The Theurgy list outside battle as a bespoke menu (menu.h), modelled from the game's own object.
// UITheurgia: +0x34 state (0xF browsing, 0x10 a Theurgy's detail), +0xD8 mode (1 = opened from a Stats member's
// page, FUN_141351c00; offered once a party member knows a Theurgy, UUICmpStatus +0x58), +0xDD closing, +0xEBC busy;
// the carousel +0x140 (count +0x148), 0xA0 each: +0x84 the skill id, +0x9E new. The cursor +0x114 moves with left /
// right and wraps; the carousel is laid out again only on a move (the `set` runs the game's). Q / E switch the character
// (+0xCC the current one's id; +0x1A8 TArray<short> ids).
#include "screens/menu.h"
#include "screens/util.h"
#include "bmdfile.h"
#include "core/bmd.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "party.h"
#include "persona.h"
#include "speech.h"
#include <format>

namespace p3a::screens {
using namespace ue;

class TheurgyMenu : public MenuScreen {
  Finder ui_{"UITheurgia"};
  uint8_t* u() { UObject* o = ui_.get(); return o && (i32(o, 0x34) == 0xF || i32(o, 0x34) == 0x10) ? (uint8_t*)o : nullptr; }
  bool personality() { UObject* o = ui_.get(); return o && i32(o, 0x34) == 0x10; }
  // F: the character's personality (what fills their Theurgy gauge), BMD_TheurgiaFlavorText dialog = character id.
  static std::string personality_text(int who) {
    auto f = bmdfile::load("BMD_TheurgiaFlavorText");
    if (!f || !f->ok || who <= 0 || who >= (int)f->dialogs.size() || f->dialogs[who].pages.empty()) return {};
    return core::clean_text(core::bmd::plain(f->dialogs[who].pages[0]));
  }
  bool personality_said_ = false;
  Mirror& list_ = mirror("theurgy", [this] { uint8_t* p = u(); return p ? i32(p, 0x114) : -1; });
  int who() { uint8_t* p = u(); return p ? i32(p, 0xCC) : 0; }
 public:
  // The game's own select-entry handler (FUN_14151b180, an input delegate): the index, the sound, the carousel laid
  // out again, the entry left marked seen. Not on the index already there (it buzzes) nor while closing (+0xDD).
  TheurgyMenu() {
    list_.keep_edges = true;   // the game's row wraps
    list_.ready = [this] { uint8_t* p = u(); return p && i32(p, 0x34) == 0xF && !p[0xDD]; };
    list_.set = [this](int i) {
      uint8_t* p = u();
      auto* a = p ? arr<uint8_t>(p, 0x140) : nullptr;
      if (!a || i32(p, 0x34) != 0xF || p[0xDD] || i < 0 || i >= a->num) return false;
      if (i32(p, 0x114) == i) return true;
      return fn<bool (*)(void*, void*, int)>(0x151b180)(p, nullptr, i);
    };
  }
  const char* id() const override { return "theurgy"; }
  int priority() const override { return 18; }
  bool active() override { uint8_t* p = u(); return p && arr<uint8_t>(p, 0x140); }
  // States 2 to 0xD: the "Theurgy obtained" scene a Social Link event plays (MASTERY_THEURGIA_DRAW): each card said
  // once, from the carousel's new entry (+0x9E) or the one under the cursor. One scene can show several: the cursor
  // moves to each (Ken's awakening, 10/6: Divine Retribution, then Divine Intervention at +0x114 = 1, the flags
  // cleared; the second went unsaid, 2026-11-03).
  int obtained_said_ = 0;
  void background() override {
    UObject* o = ui_.get();
    int st = o ? i32(o, 0x34) : 0;
    if (st < 2 || st > 0xD) { if (st == 0 || st >= 0x11) obtained_said_ = 0; return; }
    auto* a = arr<uint8_t>(o, 0x140);
    if (!a) return;
    int pick = i32(o, 0x114);
    for (int i = 0; i < a->num; ++i) if (a->data[i * 0xA0 + 0x9E]) pick = i;
    if (pick < 0 || pick >= a->num) return;
    int id = *(uint16_t*)(a->data + pick * 0xA0 + 0x84);
    if (id == obtained_said_) return;
    obtained_said_ = id;
    std::string d = persona::skill_help(id);
    speech::speak(std::format("{}: {}{}", strings::kTheurgyObtained, persona::skill_name(id), d.empty() ? "" : ". " + d), true);
  }
  std::string title() override { return strings::kTheurgy; }
  void on_enter() override { screens::set_keys([this] { return std::string(personality() ? strings::kCBack() : strings::kTheurgyListKeys()); }); }
  bool holds_keys() override { return !personality(); }
  void on_tick() override {
    if (!personality()) { personality_said_ = false; return; }
    if (personality_said_) return;
    personality_said_ = true;
    std::string t = personality_text(who());
    if (!t.empty()) speech::speak(std::format("{}: {}", strings::kTheurgyGauge, t), true);
  }
  void build(ui::GraphBuilder& b) override {
    uint8_t* p = u();
    auto* a = p ? arr<uint8_t>(p, 0x140) : nullptr;
    if (!a) return;
    list_.count = a->num;
    std::string key = std::format("theurgy.{}", who());
    list_.key = key;
    b.begin_stop(key);
    b.push_context(party::name(who()));
    b.start_row(key);
    for (int i = 0; i < a->num; ++i) {
      uint8_t* e = a->data + i * 0xA0;
      int id = *(uint16_t*)(e + 0x84);
      std::string l = persona::skill_name(id);
      if (e[0x9E]) l += std::string(", ") + strings::kMailNew;
      auto v = game_item(list_, i, [l] { return l; });
      describe_with(v, [id] { return persona::skill_help(id); });
      b.add_item(slot(key, i), v);
    }
    b.end_row();
    b.pop_context();
    if (int c = list_.cursor(); c >= 0) b.set_start(slot(key, c));
  }
};

std::unique_ptr<Screen> make_theurgy_menu() { return std::make_unique<TheurgyMenu>(); }
}  // namespace p3a::screens

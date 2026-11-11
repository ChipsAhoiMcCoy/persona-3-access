// The camp's Persona submenu as a bespoke menu (menu.h), modelled from the game's own objects (the offsets are this
// build's, measured 2026-10-30). Its list uses the generic ListCursor (util.h): index = row + top.
//
//   UCmpPersona +0x38 == 3, actor +0x1D0, its PersonaList_: +0x40 mode (0 list, 1 status page), +0x48 a pointer to
//   its ListCursor, +0x88 its host (+0x9D0 the UI layer the open timer asks, +0x9D8 the description), +0x90 the
//   status widget, +0x98 the open timer. Entries: the stock in order; the equipped slot is persona::equipped().
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/camp.h"
#include "screens/persona_status.h"
#include "core/strings.h"
#include "persona.h"

namespace p3a::screens {
using namespace ue;

class CampPersona : public MenuScreen {
  Finder menu_{"CmpPersona"};
  uint8_t* list() {
    UObject* o = camp_open() ? menu_.get() : nullptr;
    if (!o || i32(o, 0x38) != 3) return nullptr;
    UObject* actor = *(UObject**)((uint8_t*)o + 0x1D0);
    return actor ? (uint8_t*)get<UObject*>(actor, "PersonaList_") : nullptr;
  }
  Mirror& list_ = mirror("personas", [this] {
    uint8_t* l = list();
    return l && l[0x40] == 0 ? cursor_at(l, 0x48).index() : -1;
  });
 public:
  // The list's move (FUN_141327b90): the sound, the Persona loaded into the status widget (model, stats, skills:
  // FUN_1414ebc00), the bar (FUN_141323cb0) and the description's fade (FUN_141323730). Moves wait for the list's open
  // timer and for the widget's swap animation to be nearly done (FUN_1414e7b70(widget, 3)), as the game's do.
  CampPersona() {
    list_.ready = [this] {
      uint8_t* l = list();
      if (!l || l[0x40] != 0) return false;
      uint8_t* host = *(uint8_t**)(l + 0x88);
      void* layer = *(void**)(host + 0x9D0);
      return open_timer_done(*(float*)(l + 0x98), layer) && fn<bool (*)(void*, int)>(0x14e7b70)(*(void**)(l + 0x90), 3);
    };
    list_.set = [this](int i) {
      uint8_t* l = list();
      ListCursor lc = cursor_at(l, 0x48);
      if (!l || !lc.set(i)) return false;
      int idx = lc.index();
      cursor_sound();
      fn<void (*)(void*, int, char, char, char, char)>(0x14ebc00)(*(void**)(l + 0x90), idx, 0, 0, 1, 1);
      fn<void (*)(void*, int, char)>(0x1323cb0)(l, idx, 0);
      fn<void (*)(void*)>(0x1323730)(*(void**)(*(uint8_t**)(l + 0x88) + 0x9D8));
      return true;
    };
  }
  const char* id() const override { return "camp_persona"; }
  int priority() const override { return 17; }
  bool active() override { uint8_t* l = list(); return l && l[0x40] == 0; }   // mode 1: the status page's screen
  std::string title() override { return strings::kCampItems[3]; }
  void on_enter() override { screens::set_keys(strings::kCampPersonaKeys); }
  // Q / E (LB / RB): the previous / next Persona, its page from the top (the game's list does not wrap; ours does).
  bool takes_bumpers() override { return true; }
  bool blocks_status() override { return true; }   // the game's status page (F / X) would only repeat this one
  void on_bumper(int dir) override {
    int n = (int)persona::stock().size(), c = list_.cursor();
    if (n < 2 || c < 0 || !list_.ready()) return;
    int to = ((c + dir) % n + n) % n;
    if (list_.set(to)) focus(slot("personas", to));
  }
  // The page of the Persona under the game's cursor (the status page's, F): its name line is the list's row (Enter
  // equips it), then its sections; the triggers jump them.
  void build(ui::GraphBuilder& b) override {
    auto stock = persona::stock();
    int c = list_.cursor();
    if (c < 0 || c >= (int)stock.size()) return;
    int sl = stock[c].slot;
    auto label = [sl] {
      std::string l;
      for (auto& o : persona::stock()) if (o.slot == sl) l = persona::describe(o.id, o.level);
      if (sl == persona::equipped()) l += std::string(", ") + strings::kEquipped;
      return l;
    };
    ui::ControlId id = slot("personas", c);
    build_status_page(b, persona::stock_entry(sl), {}, {}, nullptr, &id, game_item(list_, c, label));
    b.set_start(id);
  }
};

std::unique_ptr<Screen> make_camp_persona() { return std::make_unique<CampPersona>(); }
}  // namespace p3a::screens

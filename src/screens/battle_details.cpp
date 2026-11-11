// The details of whoever is in focus (the Say the Spire model): the right stick up and down, or Ctrl+Up and Ctrl+Down,
// walk the lines of BtlWords::details from the top, the unit's name: down goes on, up goes back (the user's call,
// 2026-11-05); each read is fresh, and landing on someone starts at their top again. Who is in focus is the game's own
// cursor in each state; a list of the acting member's own (items) reads them. A flick is one line. The stick is
// hidden from the game only while something is in focus; no battle Blueprint reads it.
// Since 2026-11-11 the same buffer serves any menu row that declares details (MenuScreen::has_details), in a battle
// or out of one: the skill list's rows, the Stats list's members.
// The game's cursors, by GUI state (battle_state.h has the wheel's and the target manager's):
//   state 4 targets   BP_BtlTargetSelect_C     targetList, TargetIndex
//   state 6 Survey    BP_BtlTacticsCheck_C     CursorList, TargetIndex
//   state 9 Shift     BP_BtlGuiShiftSelect_C   PlayerList, TargetIndex
//   state 10 Tactics  BP_BtlStrategyInstruct_C inputState 1 (choosing a member): PlayerList[CursorIndex[1] - 1]
#include "screens/battle_details.h"
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/battle_state.h"
#include "screens/battle_words.h"
#include "devserver.h"
#include "input.h"
#include "msg.h"
#include "seh.h"
#include "speech.h"
#include "ue/scan.h"
#include <algorithm>
#include <format>

namespace p3a::screens {
using namespace ue;

namespace {
Finder& target_list() { static Finder f{"BP_BtlTargetSelect_C"}; return f; }
Finder& survey_list() { static Finder f{"BP_BtlTacticsCheck_C"}; return f; }
Finder& shift_list() { static Finder f{"BP_BtlGuiShiftSelect_C"}; return f; }
Finder& tactics_list() { static Finder f{"BP_BtlStrategyInstruct_C"}; return f; }
UObject* at(UObject* o, const char* list, const char* index) {
  auto* l = o ? field<TArray<UObject*>>(o, list) : nullptr;
  int32_t* i = o ? field<int32_t>(o, index) : nullptr;
  return l && i && *i >= 0 && *i < l->num ? l->data[*i] : nullptr;
}
UObject* focused_unit() {
  int st = gui_state();
  if (!st) return nullptr;
  UObject* acting = acting_member();
  switch (st) {
    case 1: return wheel_target();   // the wheel: the target left and right change
    case 2: case 3: case 7: return acting;
    case 4: return at(target_list().get(), "targetList", "TargetIndex");
    case 6: return at(survey_list().get(), "CursorList", "TargetIndex");
    case 8: return cursor_target();
    case 9: return at(shift_list().get(), "PlayerList", "TargetIndex");
    case 10: {
      UObject* s = tactics_list().get();
      auto* cur = s ? field<TArray<int32_t>>(s, "CursorIndex") : nullptr;
      if (!s || get<int32_t>(s, "inputState") != 1 || !cur || cur->num < 2) return acting;
      auto* party = field<TArray<UObject*>>(s, "PlayerList");
      int k = cur->data[1] - 1;   // item 0 is everyone
      return party && k >= 0 && k < party->num ? party->data[k] : acting;
    }
    default: return nullptr;
  }
}
// A menu row with details of its own (MenuScreen::has_details: a skill, a member on the Stats list) comes before the
// battle's unit; outside a battle it is the only source.
MenuScreen* detail_menu() {
  MenuScreen* m = menu::active();
  return m && m->has_details() ? m : nullptr;
}
WeakObj g_detail_unit;
std::string g_detail_row;   // the menu row's identity, empty for a unit
int g_detail_line = 0;
void detail_landing(MenuScreen* m, UObject* u) {
  std::string row = m ? m->detail_key() : std::string();
  WeakObj w = m ? WeakObj() : WeakObj::of(u);
  if (!(w == g_detail_unit) || row != g_detail_row) { g_detail_unit = w; g_detail_row = row; g_detail_line = 0; }
}
std::string detail_step(int dir) {
  MenuScreen* m = detail_menu();
  UObject* u = m ? nullptr : focused_unit();
  if (!m && !u) return {};
  detail_landing(m, u);
  auto lines = m ? m->detail_lines() : btl_words().details(u);
  if (lines.empty()) return {};
  g_detail_line = std::clamp(g_detail_line + dir, 0, (int)lines.size() - 1);
  return lines[g_detail_line];
}
}  // namespace

namespace battle_details {
bool in_focus() { bool on = false; guarded([&] { on = detail_menu() || focused_unit(); }); return on && !msg::current().blocking; }
void step(int dir) {
  std::string l;
  guarded([&] { l = detail_step(dir); });
  if (!l.empty()) speech::speak(l, true);
}
void frame() {
  guarded([] { MenuScreen* m = detail_menu(); detail_landing(m, m ? nullptr : focused_unit()); });
  bool on = in_focus() && !input::modal();
  input::claim_right_stick(on);
  if (int d = input::take_right_flick(); d && on) step(-d);   // a flick up is +1: back toward the top
}
void install() {
  dev::route("/battle/details", [](const dev::Request& r, int&) {
    int d = (int)dev::arg_int(r, "step", 0);
    int give = (int)dev::arg_int(r, "ailment", -1);
    return dev::on_game([d, give] {
      std::string out;
      guarded([&] {
        UObject* u = focused_unit();
        // ?ailment=N: type N on the focused unit by the game's own route: its flag (SetBadStatus) and then the turn
        // counters the battle gives it (FUN_140fe2ef0(actor, bit), +0x38E and +0x3A6), for testing what a skill would do.
        // This build's address: on another build nothing is given.
        if (give >= 0 && give < 12 && u && known_build()) {
          UObject* db = btl_words().db();
          UFunction* f = db ? find_function(class_of(db), "SetBadStatus") : nullptr;
          if (f) { Params p(f); p.set<UObject*>("Actor", u); p.set<int32_t>("Type", give); call(db, p); }
          ((void (*)(void*, uint32_t))(scan::exe_base() + 0xfe2ef0))(u, 1u << give);
        }
        if (d) step(d);
        if (u) for (auto& l : btl_words().details(u)) out += l + "\n";
        out += std::format("line {}\n", g_detail_line);
      });
      return out;
    });
  }, "?step=1|-1 ?ailment=N the focused battle unit's detail lines (what the right stick reads)");
}
}  // namespace battle_details
}  // namespace p3a::screens

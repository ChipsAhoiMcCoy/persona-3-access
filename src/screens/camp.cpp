// The camp menu (Tab in the field). Open while the innermost pause is Camp (pause_top); the root list's cursor is
// UCmpRoot +0x3c (0 Skill .. 8 System, measured 2026-10-27). Labels and help lines are sprites: the mod's wording
// for the labels, the help lines as the game shows them (strings::kCampHelp).
#include "screens/menu.h"
#include "screens/util.h"
#include "core/strings.h"
#include "party.h"
#include <format>

namespace p3a::screens {
using namespace ue;

// The innermost pause: AppPauseManagerSubsystem.PauseFlow is a stack of EAppPauseType ({None, TownMap} while the
// town map is up; IsPause(type) answers true for every type once anything pauses, measured 2026-10-27).
int pause_top() {
  static const int id = watch::want("AppPauseManagerSubsystem");
  UObject* m = watch::get(id);
  if (!m) return 0;
  auto* flow = field<TArray<uint8_t>>(m, "PauseFlow");
  if (!flow || flow->num <= 0 || !flow->data) return 0;
  return flow->data[flow->num - 1];
}
bool camp_open() { return pause_top() == 1; }   // EAppPauseType::Camp

// Whether one of the root's submenus is open. Each submenu is one object for the whole session (all ten exist from
// boot, measured 2026-11-01) whose state is 0 while it is closed: Skill +0x3C, Item / Equip / Persona +0x38, Stats
// +0x44, System +0x54 (ints), Quest +0x3C, Social Link +0x58, Calendar +0x3C (byte flags).
bool camp_submenu_open() {
  static Finder skill{"CmpSkill"}, item{"CmpItem"}, equip{"CmpEquip"}, persona{"CmpPersona"}, status{"CmpStatus"},
      quest{"CmpQuest"}, commu{"CmpCommu"}, calendar{"CmpCalendar"}, sys{"CmpSystem"};
  auto i32 = [](Finder& f, int off) { UObject* o = f.get(); return o ? *(int32_t*)((uint8_t*)o + off) : 0; };
  auto u8 = [](Finder& f, int off) { UObject* o = f.get(); return o ? ((uint8_t*)o)[off] : 0; };
  return i32(skill, 0x3C) || i32(item, 0x38) || i32(equip, 0x38) || i32(persona, 0x38) || i32(status, 0x44) ||
         i32(sys, 0x54) || u8(quest, 0x3C) || u8(commu, 0x58) || u8(calendar, 0x3C);
}

class CampScreen : public MenuScreen {
  Finder root_{"CmpRoot"};
  static constexpr int kCount = 9;
  Mirror& list_ = mirror("camp", [this] {
    UObject* r = root_.get();
    int c = r ? *(int32_t*)((uint8_t*)r + 0x3c) : -1;
    return c >= 0 && c < kCount ? c : -1;
  });
 public:
  // The root's ListCursor is +0x48 (+0x3c is its row, copied every frame). The draw reads the cursor each frame; the
  // move (FUN_141328720) adds the row copy, the help line's fade (draw +0x88, +0x7BC) and the sound.
  CampScreen() {
    // Input in states 1-3; a confirm only once the root's timer (+0x40) has run a third of a second.
    list_.ready = [this] {
      auto* r = (uint8_t*)root_.get();
      int st = r ? *(int32_t*)(r + 0x38) : 0;
      return st >= 1 && st <= 3 && *(float*)(r + 0x40) >= 1.f / 3;
    };
    list_.set = [this](int i) {
      auto* r = (uint8_t*)root_.get();
      int st = r ? *(int32_t*)(r + 0x38) : 0;
      if (st < 1 || st > 3 || !ListCursor(r + 0x48).set(i)) return false;
      *(int32_t*)(r + 0x3c) = *(int32_t*)(r + 0x48 + 0x24);
      if (uint8_t* d = *(uint8_t**)(r + 0x88)) *(uint64_t*)(d + 0x7BC) = 0;
      cursor_sound();
      return true;
    };
  }
  const char* id() const override { return "camp"; }
  int priority() const override { return 15; }
  // The save key (H) opens the save list under a camp pause too, without the root menu: UCmpRoot +0x38 is the
  // root's state, 0 unless the menu itself is open (3 while idle on it; measured 2026-10-27).
  // +0x38 is 3 while idle on the list, and stays 3 while a submenu is open: state 4 asks the camp manager for the
  // submenu and goes straight back to 3 (FUN_141328720; 5-8 closing). So the root is in front only while no submenu
  // is open; it read "Equip, 3 of 9" again whenever a submenu passed between its own states (2026-11-01).
  // State 2 is the opening slide, which already takes moves (FUN_141328720 falls through to the list's input; only
  // the confirm waits for the timer): the menu is up from it, else moves there were only the game's sound.
  bool active() override {
    UObject* r = camp_open() ? root_.get() : nullptr;
    int st = r ? *(int32_t*)((uint8_t*)r + 0x38) : 0;
    // 4 for a frame as a confirmed entry opens its submenu (2026-11-03): still the camp, not a gap that read its title
    // again when it came back.
    if (!(st == 2 || st == 3 || st == 4) || camp_submenu_open()) return false;
    // System's Return to Title, once confirmed, fades the screen out, waits for the fade (FUN_14135b890 state 0xF:
    // FUN_141090af0, the fade at its end) and only then closes itself and the camp: the root is on its list for a few
    // more frames behind the black screen, and read "Menu, System, 9 of 9" on the way to the title (2026-11-10).
    return !faded_out();
  }
  std::string title() override { return strings::kCamp; }
  void on_enter() override { screens::set_keys(strings::kCampKeys); }
  void build(ui::GraphBuilder& b) override {
    b.begin_stop("camp");
    // An entry the story has not opened yet is still selectable (the game buzzes): its lock flags, in list order
    // (exe+0x53b8428).
    static constexpr int kLock[kCount] = {0x40000080, 0x40000081, 0x40000082, 0x40000083, 0x40000084, 0x40000086, 0x40000085, 0x40000087, 0x40000088};
    for (int i = 0; i < kCount; ++i) {
      std::string label = strings::kCampItems[i], help = strings::kCampHelp[i];
      if (party::flag(kLock[i])) label += std::string(", ") + strings::kUnavailable;
      auto v = game_item(list_, i, [label] { return label; }, [help] { return help; });
      describe_with(v, [help] { return help; });
      b.add_item(slot("camp", i), v);
    }
    if (int c = list_.cursor(); c >= 0) b.set_start(slot("camp", c));
  }
};

std::unique_ptr<Screen> make_camp() { return std::make_unique<CampScreen>(); }
}  // namespace p3a::screens

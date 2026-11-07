// Save and load lists (ASaveLoadDraw, reflected): CursorIndex, AllSlotNum, OpenType (TYPE_FIELD / TYPE_CAMP = save or load by BootMode,
// TYPE_TITLE = load) and SaveSlotItems: per slot SlotNo, Month, Day, Week, TimeZone, PLV, PTimeHour/Min, PlayerName,
// LocationName, bHasData. The list itself is sprites. System > Rewind shows the same actor with BootMode 3: its items
// are the autosaved days to go back to (AllSlotNum keeps the save list's 15; measured 2026-10-28).
#include "screens/menu.h"
#include "screens/util.h"
#include "core/strings.h"
#include "gametext.h"
#include "speech.h"
#include <format>

namespace p3a::screens {
using namespace ue;

// A bespoke menu (menu.h): the slots in order. The game's cursor is the ListCursor at AUISaveLoad +0x298, copied to the
// draw's CursorIndex / CurrentIndex every frame; a write is followed by exe+0x152a120(actor), the tail of the game's
// own move (the actor's row / top / index copies, the sound, the slot's key help).
class SaveLoadScreen : public MenuScreen {
  Finder draw_{"SaveLoadDraw"}, actor_{"UISaveLoad"};
  Mirror& list_ = mirror("slots", [this] { UObject* d = draw_.get(); int c = d ? get<int32_t>(d, "CurrentIndex") : -1; return c >= 0 && c < count() ? c : -1; });
  std::string slot_text_;
  int slot_seen_ = -1;
 public:
  SaveLoadScreen() {
    // AUISaveLoad +0x348 is its state: 3 once the opening has played; +0x350 the list is set up (FUN_141533eb0). Rewind
    // (BootMode 3) has a state of its own, not read here: it takes moves as it did.
    list_.ready = [this] {
      if (draw_.get() && rewind()) return true;
      auto* a = (uint8_t*)actor_.get();
      return a && *(int32_t*)(a + 0x348) == 3 && a[0x350];
    };
    list_.set = [this](int i) {
      auto* a = (uint8_t*)actor_.get();
      ListCursor lc(a ? a + 0x298 : nullptr);
      if (!a || i >= lc.count() || !lc.set(i)) return false;
      ((void (*)(void*))(scan::exe_base() + 0x152a120))(a);
      return true;
    };
  }
 private:
  bool rewind() { return get<int32_t>(draw_.get(), "BootMode") == 3; }
  std::string title() override {
    if (rewind()) return strings::kRewind;
    if (get<bool>(draw_.get(), "bIsInheritanceLoad")) return strings::kCarryOverSave;   // Episode Aigis's new game
    FField* p = find_prop(class_of(draw_.get()), "OpenType");
    std::string t = p ? enum_name(prop_inner_object(p), *((uint8_t*)draw_.get() + prop_offset(p))) : "";
    // BootMode 1 saves and 2 loads when opened from camp's System menu (measured 2026-10-28).
    return t == "TYPE_TITLE" || get<int32_t>(draw_.get(), "BootMode") == 2 ? strings::kLoadGame : strings::kSaveGame;
  }
  int count() {
    if (rewind()) { auto* items = field<TArray<uint8_t>>(draw_.get(), "SaveSlotItems"); return items ? items->num : 0; }
    return get<int32_t>(draw_.get(), "AllSlotNum");
  }
  std::string slot(int i) {
    UObject* d = draw_.get();
    auto* items = field<TArray<uint8_t>>(d, "SaveSlotItems");
    FField* ip = find_prop(class_of(d), "SaveSlotItems");
    if (!items || !ip || i < 0 || i >= items->num) return std::format("{} {}", strings::kSlot, i + 1);
    UStruct* st = prop_inner_object(prop_inner_prop(ip));
    uint8_t* it = items->data + i * struct_size(st);
    auto i32 = [&](const char* n) { return *(int32_t*)(it + offset_of(st, n)); };
    auto str = [&](const char* n) { return to_utf8(*(FString*)(it + offset_of(st, n))); };
    std::string head = rewind() ? std::string() : std::format("{} {}", strings::kSlot, i + 1);
    if (!*(bool*)(it + offset_of(st, "bHasData"))) return head + ", " + strings::kEmpty;
    calendar::Now n;
    n.month = i32("Month"); n.day = i32("Day"); n.weekday = i32("Week"); n.valid = true;
    // The slot's time of day is DT_SystemTextName row 121 + TimeZone (4 After School, 6 Evening, 8 Late Night;
    // measured 2026-10-27), not ECldTimeZone.
    std::string when = calendar::describe(n);
    std::string tz = gametext::system_text(121 + i32("TimeZone"));
    if (!tz.empty()) when += ", " + tz;
    // Rewind's points are all this game's: the day leads, without the player's name.
    std::string who = head.empty() ? std::string() : head + ": " + str("PlayerName") + ", ";
    return std::format("{}{}, {} {}, {}, {} {} {} {}", who, when, strings::kLevel, i32("PLV"),
                       str("LocationName"), strings::kPlayTime, i32("PTimeHour"), i32("PTimeHour") == 1 ? strings::kHour : strings::kHoursAnd,
                       std::format("{} {}", i32("PTimeMin"), i32("PTimeMin") == 1 ? strings::kMinute : strings::kMinutes));
  }
 public:
  const char* id() const override { return "save_load"; }
  int priority() const override { return 30; }
  bool active() override { UObject* d = draw_.get(); return d && count() > 0; }
  void on_enter() override { screens::set_keys(strings::kListKeys); slot_seen_ = -1; }
  void on_tick() override {
    // The focused slot's contents changed: the save just went in.
    int c = list_.cursor();
    if (c < 0) return;
    std::string t = slot(c);
    if (c == slot_seen_ && t != slot_text_) speech::speak(std::string(strings::kSaved) + ". " + t, true);
    slot_seen_ = c;
    slot_text_ = t;
  }
  void build(ui::GraphBuilder& b) override {
    int n = count();
    b.begin_stop("slots");
    for (int i = 0; i < n; ++i) b.add_item(slot_id(i), game_item(list_, i, [this, i] { return slot(i); }));
    if (int c = list_.cursor(); c >= 0) b.set_start(slot_id(c));
  }
  static ui::ControlId slot_id(int i) { return MenuScreen::slot("slots", i); }
};

std::unique_ptr<Screen> make_save_load() { return std::make_unique<SaveLoadScreen>(); }
}  // namespace p3a::screens

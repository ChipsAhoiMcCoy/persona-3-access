// Elizabeth's requests and the missing persons as a bespoke menu (menu.h), modelled from the game's own objects (the
// offsets are this build's, measured 2026-10-30): the camp's Quest submenu, the request counter in the Velvet Room
// and the Missing Persons board show the same two lists. Every list uses the generic ListCursor (util.h): index =
// row + top.
//
//   UCmpQuest +0x3C open, +0x1C4 tab (0 requests, 1 missing persons).
//   UIRequest: +0x34 state (8 list, 9 info), +0xD0 TArray<{u16 quest, u8 status, u8 new, ...}> (0xC each),
//   ListCursor +0xE0, +0x300 the highlighted request, +0x309 0 at the Velvet Room's counter, +0x5F8 the prompt up
//   (-1 none), +0xD60 / +0xD61 the list leaving.
//   UIMissingPerson: +0x34 state (6 list, 7 detail), +0x40 / +0x44 two floats the bar's slide is given, +0x4C /
//   +0x4D (no move while either is set), +0x531 1 on the board outside the camp, +0x538 a pointer to its ListCursor,
//   +0x548 the bar, +0x728 the highlighted case, +0xB48 TArray<{u16, u16 case}> (count also at +0xB50).
//   DisappearDataAsset +0x30: a TMap, elements of 0x30 (key at +0, the case's flags at +8).
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/camp.h"
#include "bmdfile.h"
#include "core/bmd.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "msg.h"
#include "party.h"
#include <algorithm>
#include <format>
#include <map>

namespace p3a::screens {
using namespace ue;

class CampQuest : public MenuScreen {
 public:
  CampQuest() {
    // A request's move (FUN_1415a9370 state 8): the sound, the NEW mark of the request left cleared, the highlighted
    // request (+0x300), the bar (FUN_141585740) and Elizabeth's reaction to its status (FUN_141587280). Not while
    // a prompt is up (+0x5F8) or the list is leaving (+0xD60 / +0xD61).
    reqs_.ready = [this] { uint8_t* r = req(); return r && !r[0xD61] && r[0xD60] != 1 && i32(r, 0x5F8) == -1; };
    reqs_.set = [this](int i) {
      uint8_t* r = req();
      auto* list = r ? arr<uint8_t>(r, 0xD0) : nullptr;
      ListCursor lc(r ? r + 0xE0 : nullptr);
      int old = lc.index();
      if (!list || i >= list->num || old < 0 || old >= list->num || !lc.set(i)) return false;
      cursor_sound();
      fn<void (*)(uint16_t, char)>(0x1505690)(*(uint16_t*)(list->data + old * 0xC), 0);
      list->data[old * 0xC + 3] = 0;
      *(int32_t*)(r + 0x300) = i;
      fn<void (*)(void*)>(0x1585740)(r);
      fn<void (*)(void*, char, char, char, char)>(0x1587280)(r, r[0x309], list->data[i * 0xC + 2], 1, 0);
      return true;
    };
    // A case's move (FUN_14149c750 state 6): the bar (FUN_141490460, two floats), the sound, the case left marked
    // seen, the highlighted case (+0x728).
    cases_.ready = [this] { uint8_t* r = missing(); return r && !r[0x4C] && !r[0x4D]; };
    cases_.set = [this](int i) {
      uint8_t* r = missing();
      ListCursor lc = cursor_at(r, 0x538);
      if (!r || !lc.set(i)) return false;
      int old = i32(r, 0x728), n = i32(r, 0xB50);
      fn<void (*)(void*, float, float)>(0x1490460)(r + 0x548, *(float*)(r + 0x40), row_of(lc.p) * 60.f + *(float*)(r + 0x44));
      if (n != 0 && old != i) {
        cursor_sound();
        if (old >= 0 && old < n) {
          uint32_t f = *(uint16_t*)(*(uint8_t**)(r + 0xB48) + old * 4 + 2) + 0x20000E95u;
          void* g = fn<void* (*)()>(0x122ac90)();
          if (!fn<bool (*)(void*, uint32_t)>(0x122a080)(g, f)) fn<void (*)(void*, uint32_t, int)>(0x1230410)(g, f, 1);
        }
      }
      *(int32_t*)(r + 0x728) = i;
      return true;
    };
  }
  void on_enter() override {
    screens::set_keys([this] {
      return std::string(velvet() ? strings::kRequestKeys() : board() ? strings::kMissingBoardKeys()
                         : tab() == 1 ? strings::kMissingPersonsKeys() : strings::kCampRequestKeys());
    });
  }
 private:
  Finder menu_{"CmpQuest"}, req_{"UIRequest"}, missing_{"UIMissingPerson"}, velvet_{"UIVelvetRoomRequest"};
  Finder quests_data_{"DisappearDataAsset"};
  bool open() { UObject* q = camp_open() ? menu_.get() : nullptr; return q && ((uint8_t*)q)[0x3C]; }
  // Elizabeth's own counter in the Velvet Room shows the same list (UIRequest +0x309 = 0 there: accept and report).
  bool velvet() { return !open() && velvet_.get() != nullptr; }
  // The Missing Persons board on its own (AUIMissingPersonActor: UIMissingPerson +0x531 = 1), outside the camp.
  bool board() { uint8_t* r = missing(); return !open() && r && r[0x531]; }
  int tab() { if (board()) return 1; if (velvet()) return 0; UObject* q = menu_.get(); return q ? i32(q, 0x1C4) : 0; }
  uint8_t* req() { UObject* r = req_.get(); return r && i32(r, 0x34) == 8 ? (uint8_t*)r : nullptr; }
  uint8_t* missing() { UObject* r = missing_.get(); return r && i32(r, 0x34) == 6 ? (uint8_t*)r : nullptr; }
  Mirror& reqs_ = mirror("requests", [this] { uint8_t* r = req(); return r && tab() == 0 ? ListCursor(r + 0xE0).index() : -1; });
  Mirror& cases_ = mirror("cases", [this] { uint8_t* r = missing(); return r && tab() == 1 ? cursor_at(r, 0x538).index() : -1; });
  std::string quest_title(int id) {
    auto f = bmdfile::load("BMD_Quest");
    if (!f || !f->ok || id < 0 || id >= (int)f->dialogs.size()) return std::format("{} {}", strings::kNumber, id);
    auto& d = f->dialogs[id];
    if (d.speaker >= 0 && d.speaker < (int)f->speakers.size()) return core::clean_text(f->speakers[d.speaker]);
    return std::format("{} {}", strings::kNumber, id);
  }
  std::string quest_page(int id, int page, bool* gap = nullptr) {
    auto f = bmdfile::load("BMD_Quest");
    if (!f || !f->ok || id < 0 || id >= (int)f->dialogs.size() || page >= (int)f->dialogs[id].pages.size()) return {};
    auto& pg = f->dialogs[id].pages[page];
    // Table 4 functions are text the game fills in (an item's name): the file's text has a hole there.
    if (gap) { *gap = false; for (auto& fn : pg.functions) if (fn.table == 4) *gap = true; }
    return core::clean_text(core::bmd::plain(pg));
  }
 public:
  const char* id() const override { return "camp_quest"; }
  int priority() const override { return 21; }
  bool active() override { return (open() || velvet() || board()) && ((tab() == 0 && req()) || (tab() == 1 && missing())); }
  // A request's prompt (accept? / the report) is a message window over the list: the keys are its.
  bool holds_keys() override { uint8_t* r = req(); return tab() == 1 || !r || *(int32_t*)(r + 0x5F8) == -1; }
  // A case's profile is read with each row: the game's details key (F / X) does nothing there. A request's info (F)
  // stays the game's: Elizabeth's text names the item only on the page the game builds for that view.
  bool blocks_status() override { return tab() == 1; }
  std::string title() override { return tab() == 1 ? strings::kMissingPersons : strings::kRequests; }
  void build(ui::GraphBuilder& b) override {
    if (tab() == 0) {
      uint8_t* r = req();
      auto* list = r ? arr<uint8_t>(r, 0xD0) : nullptr;
      b.begin_stop("requests");
      for (int i = 0; list && i < list->num; ++i) {
        uint8_t* e = list->data + i * 0xC;
        int id = *(uint16_t*)e, status = e[2];
        std::string label = status == 5 ? std::string("???") : quest_title(id);
        if (status >= 0 && status < 5) label += std::string(", ") + strings::kRequestStatus[status];
        auto v = game_item(reqs_, i, [label] { return label; }, [this, id, status] {
          return status == 5 ? std::string() : quest_page(id, 1);
        });
        // The task after the title, as the game shows it beside the list. The message file's text leaves out what the
        // game fills in (an item's name: "Show Elizabeth a ."); the page the game built for the highlighted request
        // has it, so that one is read once the game's cursor is there.
        if (status != 5) {
          bool gap = false;
          std::string task = quest_page(id, 1, &gap);
          describe_with(v, [this, i, task, gap] {
            std::string fallback = gap ? std::string() : task;
            if (!reqs_.settled_on(i)) return fallback;
            std::string head = task.substr(0, std::min<size_t>(task.size(), 8));
            for (auto& pg : msg::visible(30)) { std::string t = core::clean_text(pg.text); if (!head.empty() && t.rfind(head, 0) == 0) return t; }
            return fallback;
          });
        }
        b.add_item(slot("requests", i), v);
      }
      if (int c = reqs_.cursor(); c >= 0) b.set_start(slot("requests", c));
      return;
    }
    uint8_t* r = missing();
    auto* list = r ? arr<uint32_t>(r, 0xB48) : nullptr;
    auto f = bmdfile::load("BMD_DisappearHelp");
    // Each case's state from DisappearDataAsset (a TMap: elements of 0x30, key at +0, flags Active, Success,
    // Failed, Npc, InfoOn, Award at +8), and its name only once InfoOn is set.
    std::map<int, const int32_t*> flags;
    if (UObject* d = quests_data_.get()) {
      auto* map = (TArray<uint8_t>*)((uint8_t*)d + 0x30);
      for (int i = 0; map->data && i < map->num && i < 200; ++i) flags[*(uint16_t*)(map->data + i * 0x30)] = (int32_t*)(map->data + i * 0x30 + 8);
    }
    b.begin_stop("cases");
    for (int i = 0; list && i < list->num; ++i) {
      int no = (list->data[i] >> 16) & 0xFFFF;
      std::string name = strings::kUnknown, profile, status;
      const int32_t* fl = flags.count(no) ? flags[no] : nullptr;
      if (fl) status = party::flag(fl[1]) ? strings::kMissingFound : party::flag(fl[2]) ? strings::kMissingDiscontinued : strings::kMissingSearching;
      if ((!fl || party::flag(fl[4])) && f && f->ok && no < (int)f->dialogs.size()) {
        auto& dlg = f->dialogs[no];
        if (dlg.speaker >= 0 && dlg.speaker < (int)f->speakers.size()) name = core::clean_text(f->speakers[dlg.speaker]);
        if (!dlg.pages.empty()) profile = core::clean_text(core::bmd::plain(dlg.pages[0]));
      }
      std::string label = std::format("{} {}, {}", strings::kNumber, no, name);
      if (!status.empty()) label += ", " + status;
      auto v = game_item(cases_, i, [label] { return label; }, [profile] { return profile; });
      describe_with(v, [profile] { return profile; });
      b.add_item(slot("cases", i), v);
    }
    if (int c = cases_.cursor(); c >= 0) b.set_start(slot("cases", c));
  }
};

std::unique_ptr<Screen> make_camp_quest() { return std::make_unique<CampQuest>(); }
}  // namespace p3a::screens

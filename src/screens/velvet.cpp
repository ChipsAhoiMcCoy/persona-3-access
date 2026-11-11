// The Velvet Room's fusion screens as a bespoke menu (menu.h). Everything is drawn as pictures and native text the font
// hooks never see, so the mod reads the state and names the entries from game data. UIVelvetRoomDraw.Scene: 2 MAIN
// (Fusion / Summon / Registration; cursor UIVelvetRoom +0x3C8), 3 COMBINE_SELECT (the method), 4 COMBINE (fusion),
// 5 the compendium, 6 registration (measured 2026-10-27). The room's own state is UIVelvetRoom +0x294 (6 the main
// menu takes input, 9 the compendium runs); the draw animates while its +0x2F9 or +0x2FA is set. The lists, from memory:
//   UICombine (+0x34 step): 2 the method, 3 base, 4 second, 12 Special, 13 Search, 5 result page, 6 inheritance,
//     7 and 8 the fusion's animation, 11 the result's level up, 14 status browse; +0x1E4 a pending dialog (-1 none).
//     Rows +0x138 (count +0x140), 0xA8 each: the Persona entry at +0x18 (id +0x1A, level +0x1C), +0x8C no result;
//     step 4's results +0x148 (row i = base + row i). Special recipes +0x188 (count +0x190), 0xC0 each: the result
//     row at +0, materials TArray at +0xA8 (count +0xB0) of rows with +0x8D "can be had". Search results +0x178
//     (count +0x180), 0x1F8 each: result at +0, material A's id +0xC2, B's +0x16A. The list cursor is the
//     ListCursor at +0x84 for all four; a write is followed by the draw's exe+0x1586b50(draw, top, row) (steps 3
//     and 4) or +0x40A = 1 (12 and 13), as the game's own move does. The method list: ListCursor +0x48.
//   UICompleteBook (+0x34: 1 list, 2 status page): rows +0x48 (count +0x50, 0xA8: +0x88 registered, +0x94 price,
//     sort +0x124), ListCursor +0x58. UICompleteBookRegist (+0x34: 3 list, 5 status): rows +0x58 (count +0x60,
//     +0x8A marked), ListCursor +0xA4.
//   Inheritance (step 6): APersonaStatus +0x470 entries of 8 (skill id u16, +6 chosen), count +0x478, index =
//     +0x484 + +0x488.
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/persona_status.h"
#include "core/strings.h"
#include "msg.h"
#include "persona.h"
#include "social.h"
#include "speech.h"
#include "ue/scan.h"
#include <algorithm>
#include <cstring>
#include <format>

namespace p3a::screens {
using namespace ue;
std::string velvet_result_bonus();

class VelvetScreen : public MenuScreen {
  Finder room_{"UIVelvetRoom"}, draw_{"UIVelvetRoomDraw"}, combine_{"UICombine"}, book_{"UICompleteBook"}, regist_{"UICompleteBookRegist"};
  Finder ps_{"PersonaStatus"};
  int page_ = 0;   // the Persona the status page showed last (0: no page)
  // A fusion's result page opens on the result; Q / E page through its materials, which are not the result.
  int result_id_ = 0;
  int scene() { UObject* d = draw_.get(); return d ? get<uint8_t>(d, "Scene") : 0; }
  static int i32(void* o, int off) { return o ? *(int32_t*)((uint8_t*)o + off) : -1; }
  uint8_t* cb() { return (uint8_t*)combine_.get(); }
  int step() { return i32(cb(), 0x34); }
  // Whether a list is up (a status page has the keys otherwise: Q / E page it, F switches it).
  bool on_list() {
    int sc = scene();
    if (sc == 2 || sc == 3) return true;
    if (sc == 4) { int st = step(); return st == 3 || st == 4 || st == 6 || st == 12 || st == 13; }
    if (sc == 5) return i32(book_.get(), 0x34) == 1;
    if (sc == 6) return i32(regist_.get(), 0x34) == 3;
    return false;
  }
  Mirror& top_ = mirror("velvet.top", [this] { return scene() == 2 ? i32(room_.get(), 0x3C8) : -1; });
  Mirror& method_ = mirror("velvet.method", [this] { return scene() == 3 && cb() ? ListCursor(cb() + 0x48).index() : -1; });
  Mirror& list_ = mirror("velvet.list", [this] {
    int st = step();
    return scene() == 4 && (st == 3 || st == 4 || st == 12 || st == 13) ? ListCursor(cb() + 0x84).index() : -1;
  });
  Mirror& inherit_ = mirror("velvet.inherit", [this] {
    uint8_t* p = (uint8_t*)ps_.get();
    return scene() == 4 && step() == 6 && p ? *(int32_t*)(p + 0x484) + *(int32_t*)(p + 0x488) : -1;
  });
  Mirror& book_list_ = mirror("velvet.book", [this] {
    uint8_t* b = (uint8_t*)book_.get();
    return scene() == 5 && i32(b, 0x34) == 1 ? ListCursor(b + 0x58).index() : -1;
  });
  Mirror& regist_list_ = mirror("velvet.regist", [this] {
    uint8_t* r = (uint8_t*)regist_.get();
    return scene() == 6 && i32(r, 0x34) == 3 ? ListCursor(r + 0xA4).index() : -1;
  });

  static std::string entry_line(const uint8_t* row) {
    int id = *(uint16_t*)(row + 0x1A), level = *(uint16_t*)(row + 0x1C);
    return persona::name(id).empty() ? std::string() : persona::describe(id, level);
  }
  bool write_list(uint8_t* list, int i, bool push_draw) {
    if (!ListCursor(list).set(i)) return false;
    uintptr_t b = scan::exe_base();
    if (push_draw) {
      if (UObject* d = draw_.get()) ((void (*)(void*, int, int))(b + 0x1586b50))(d, *(int32_t*)(list + 0x28), *(int32_t*)(list + 0x24));
      cursor_sound();
    } else {
      cb()[0x40A] = 1;   // steps 12 and 13 refresh themselves (the seen flag, the materials panel, the sound)
    }
    return true;
  }
  void build_lists(ui::GraphBuilder& b, int st);
  void build_inherit(ui::GraphBuilder& b);
  void build_book(ui::GraphBuilder& b);
  void build_regist(ui::GraphBuilder& b);

 public:
  VelvetScreen() {
    // The main menu: the room's own select-entry handler (exe+0x1586a30, its mouse path): the row, the sound (cue
    // 0x25F) and the card's turn. It takes it in the room's state 6 with the draw not animating (+0x2F9 / +0x2FA).
    top_.ready = [this] {
      UObject* r = room_.get();
      auto* d = (uint8_t*)draw_.get();
      return r && i32(r, 0x294) == 6 && d && !d[0x2F9] && !d[0x2FA];
    };
    top_.set = [this](int i) {
      UObject* r = room_.get();
      if (!r || i32(r, 0x294) != 6) return false;
      return ((bool (*)(void*, void*, int))(scan::exe_base() + 0x1586a30))(r, nullptr, i);
    };
    // The method: the cursor, the sound (0x25F) and the card (exe+0x1586b40(draw, row)), as UICombine's step 2 moves
    // it; not while its sub-dialog is pending (+0x1E4), and only with the draw idle, what its confirm waits for (FUN_141574cd0).
    method_.ready = [this] {
      auto* d = (uint8_t*)draw_.get();
      return step() == 2 && i32(cb(), 0x1E4) == -1 && d && !d[0x2F9] && !d[0x2FA];
    };
    method_.set = [this](int i) {
      uint8_t* c = cb();
      UObject* d = draw_.get();
      if (!c || !d || step() != 2 || !ListCursor(c + 0x48).set(i)) return false;
      cursor_sound(0x25F);
      ((void (*)(void*, int))(scan::exe_base() + 0x1586b40))(d, *(int32_t*)(c + 0x48 + 0x24));
      return true;
    };
    // Inheritance: six rows shown over +0x478 entries; exe+0x14c9c70(status, top, row) writes them and restarts the
    // draw's slide, after the sound, as the sequence's own move does. The sequence (status +0x520) reads confirm, cancel
    // and moves only at its +0x30 == 1 (0 builds, 2 Igor's question, 3 done; FUN_141516980).
    inherit_.ready = [this] {
      auto* p = (uint8_t*)ps_.get();
      auto* seq = p ? *(uint8_t**)(p + 0x520) : nullptr;
      return seq && seq[0x30] == 1;
    };
    inherit_.set = [this](int i) {
      uint8_t* p = (uint8_t*)ps_.get();
      if (!p || scene() != 4 || step() != 6) return false;
      int n = *(int32_t*)(p + 0x478);
      if (i < 0 || i >= n) return false;
      int v = n < 6 ? n : 6, top = *(int32_t*)(p + 0x488);
      if (i < top) top = i;
      else if (i >= top + v) top = i - v + 1;
      if (top > n - v) top = n - v;
      if (top < 0) top = 0;
      cursor_sound();
      ((void (*)(void*, int, int))(scan::exe_base() + 0x14c9c70))(p, top, i - top);
      return true;
    };
    // Registration: the same as the fusion lists' move (FUN_1415784f0).
    regist_list_.keep_edges = book_list_.keep_edges = true;   // Left / Right walk a row's details (add_walk)
    regist_list_.set = [this](int i) { uint8_t* r = (uint8_t*)regist_.get(); return r && i32(r, 0x34) == 3 && write_list(r + 0xA4, i, true); };
    list_.keep_edges = true;   // Left / Right walk the row's Persona or result (add_walk), never the game's paging
    // FUN_141574cd0: Special takes nothing while a dialog is pending (+0x1E4), Search only stops for 0x24; the fusion
    // lists have no gate.
    list_.ready = [this] {
      int st = step(), pending = i32(cb(), 0x1E4);
      return st == 12 ? pending == -1 : st == 13 ? pending != 0x24 : true;
    };
    list_.set = [this](int i) { int st = step(); return cb() && write_list(cb() + 0x84, i, st == 3 || st == 4); };
    // The compendium's own move also pushes the row's summon price to the money panel: DAT_1456b6bfc = price, then
    // exe+0x15a4590(draw, 0, -price), the draw being the room's (UIVelvetRoomDraw; the move's code reaches it through
    // the owner at book +0x40, +0x60). Without it the panel kept the old row's price after a jump.
    // The room runs the book's tick only in its state 9 (FUN_1415a8300); a cancel leaves +0x34 at 1 and Scene at 5 until
    // the draw has left, so cursor() alone would keep taking moves as it closes.
    book_list_.ready = [this] { return i32(room_.get(), 0x294) == 9; };
    book_list_.set = [this](int i) {
      uint8_t* b = (uint8_t*)book_.get();
      if (!b || !write_list(b + 0x58, i, true)) return false;
      auto* rows = (TArray<uint8_t>*)(b + 0x48);
      UObject* d = draw_.get();
      if (d && rows->data && i < rows->num) {
        int price = *(int32_t*)(rows->data + (size_t)i * 0xA8 + 0x94);
        uintptr_t base = scan::exe_base();
        *(int32_t*)(base + 0x56b6bfc) = price;
        ((void (*)(void*, uint8_t, int))(base + 0x15a4590))(d, 0, -price);
      }
      return true;
    };
  }
  const char* id() const override { return "velvet"; }
  int priority() const override { return 19; }
  bool active() override { int s = scene(); return (s >= 2 && s <= 6) && room_.get(); }
  bool holds_keys() override { return on_list() || on_page() || levelup(); }
  // Back from skill_learn (a fusion's level up): "Learned Tentarafoo" is not cut off by the page's first line.
  // Only when it is the Velvet Room that the learning covered (a skill learned after a battle says nothing about it).
  bool covered_ = false;
  void on_leave() override { covered_ = active(); }   // still up: something is over it, not closed
  bool announce_entry() override {
    bool learned = take_skill_learned(), quiet = (learned && covered_) || accepted_;
    covered_ = false;
    return !quiet;
  }
  // After the fusion (step 11) the Social Link bonus levels the result up on its page (APersonaStatus.Sequence_ a
  // PersonaStatusLvUpSeq; skills past eight go through skill_learn.cpp first). The page's entry (draw +0x34C) keeps
  // the old values: the new one is built from the sequence (+0x148 the five stat gains, +0x154 the EXP gained,
  // +0x158 the old level) and the status's skill list (+0x310: 8 of {u32 flag, u32 skill}, count +0x354; measured
  // 2026-10-31: Mithras 24 -> 28, St +1 Ma +1 Ag +7 Lu +3 as the page showed). Its "before" is the old entry.
  // Once the level up is done (a skill change offered after it, "Check skills") the entry holds the new values: the
  // old one is the copy kept while it still had them (before_).
  uint8_t levelup_[0x30] = {}, before_[0x30] = {};
  const uint8_t* levelup_before_ = nullptr;
  bool levelup() {
    if (scene() != 4 || step() != 11) return false;
    auto* p = (uint8_t*)ps_.get();
    const uint8_t* e = persona::shown_status();
    UObject* seq = p ? get<UObject*>((UObject*)p, "Sequence_") : nullptr;
    if (!e || !seq || name_of(class_of(seq)) != "PersonaStatusLvUpSeq") return false;
    auto* q = (uint8_t*)seq;
    int gained = *(int32_t*)(q + 0x154), n = *(int32_t*)(p + 0x354), old = *(int32_t*)(q + 0x158);
    if (gained <= 0) return false;
    if (old != *(uint16_t*)(e + 4)) {
      if (*(uint16_t*)(e + 4) <= old) return false;
      memcpy(levelup_, e, sizeof levelup_);
      levelup_before_ = *(uint16_t*)(before_ + 2) == *(uint16_t*)(e + 2) ? before_ : nullptr;
      return true;
    }
    memcpy(before_, e, sizeof before_);
    levelup_before_ = before_;
    memcpy(levelup_, e, sizeof levelup_);
    *(uint16_t*)(levelup_ + 4) = (uint16_t)persona::level_with_exp(e, gained);
    *(uint32_t*)(levelup_ + 8) += gained;
    for (int i = 0; i < 5; ++i) levelup_[0x1C + i] = (uint8_t)(e[0x1C + i] + q[0x148 + i]);
    if (n > 0 && n <= 8)
      for (int i = 0; i < 8; ++i) *(uint16_t*)(levelup_ + 0xC + i * 2) = i < n ? (uint16_t)*(uint32_t*)(p + 0x310 + i * 8 + 4) : 0;
    return true;
  }
  // A status page (a fusion's result, the compendium's or registration's chosen Persona): the page as lines
  // (build_status_page); arrows move over them, Enter, Q / E, F and C are the game's.
  bool on_page() {
    int sc = scene();
    int st = step();   // 5 a fusion's result, 14 a list's Persona (F)
    return (sc == 4 && (st == 5 || st == 14)) || (sc == 5 && i32(book_.get(), 0x34) == 2) || (sc == 6 && i32(regist_.get(), 0x34) == 5);
  }
  // Igor's "Is this Persona acceptable?" after the inheritance (step 6, a choice over it): what it inherits, after
  // the question (APersonaStatus +0x510: TArray<u16> of the chosen skills, measured 2026-10-31).
  bool inherit_said_ = false;
  // Yes to that question: the game stays on step 6 a few frames before the fusion (7), so the list came back as the
  // window closed ("Choose skills to inherit" before "Fusing.", 2026-11-03). The answer, not the step, ends the list.
  int answer_ = -1;
  bool accepted_ = false;
  // Registration's page keeps Tab for the game (registered stats): sections on Page Up / Down. The compendium's page
  // takes no Tab (FUN_1415777d0 step 2).
  bool game_tab() override { return on_page() && scene() == 6; }
  void background() override {
    if (scene() != 4 || step() != 6) { inherit_said_ = accepted_ = false; answer_ = -1; return; }
    msg::Current c = msg::current();
    if (c.choices.empty()) {   // "Hold on" (the second answer) goes back to the list
      if (answer_ == 0) accepted_ = true;
      answer_ = -1;
      inherit_said_ = false;
      return;
    }
    answer_ = c.choice;
    if (inherit_said_) return;
    inherit_said_ = true;
    auto* p = (uint8_t*)ps_.get();
    auto* chosen = p ? (TArray<uint16_t>*)(p + 0x510) : nullptr;
    std::string list;
    for (int i = 0; chosen && chosen->data && i < chosen->num && i < 8; ++i)
      list += (list.empty() ? "" : ", ") + persona::skill_name(chosen->data[i]);
    if (!list.empty()) speech::speak(std::format("{} {}", strings::kInherits, list), false);
  }
  int chosen_ = -1;   // the inheritance's chosen count last tick: a drop is said ("chosen" is the item's own part)
  bool fusing_ = false;   // the fusion's animation (steps 7 and 8: Igor's cards, then the Persona appears) was said
  void on_tick() override {
    // The animation runs 20 seconds with nothing to read until the Persona speaks: say it started (Tab skips it).
    if (scene() == 4 && (step() == 7 || step() == 8)) {
      if (!fusing_) { fusing_ = true; screens::set_keys(strings::kFusingKeys); speech::speak(strings::kFusing, true); }
    } else if (scene() != 4 || step() < 7) {
      fusing_ = false;
      memset(before_, 0, sizeof before_);   // a later fusion of the same Persona compares with its own values
    }
    {
      auto* p = (uint8_t*)ps_.get();
      int n = scene() == 4 && step() == 6 && p ? ((TArray<uint16_t>*)(p + 0x510))->num : -1;
      if (n >= 0 && chosen_ >= 0 && n < chosen_) speech::speak(strings::kNotChosen, true);
      chosen_ = n;
    }
    if (!on_page()) { page_ = 0; result_id_ = 0; return; }
    const uint8_t* e = persona::shown_status();
    int id = e ? *(uint16_t*)(e + 2) : 0;
    if (!id || id == page_) return;
    // A new page, or Q / E paging to another Persona: its name line, read from the top.
    if (!page_ && scene() == 4 && step() == 5) result_id_ = id;
    if (!page_) screens::set_keys(scene() == 4 && step() == 5 ? strings::kFusionConfirmKeys : scene() == 4 ? strings::kPersonaStatusKeys
                               : scene() == 5 ? strings::kCompendiumPageKeys : strings::kStatusConfirmKeys);
    else focus(status_name_id(id));
    page_ = id;
  }
  void build(ui::GraphBuilder& b) override {
    int sc = scene();
    if (sc == 2 || sc == 3) screens::set_keys(strings::kListKeys);
    if (sc == 2) {
      b.begin_stop("velvet.top");
      b.push_context(strings::kVelvetTop);
      for (int i = 0; i < 3; ++i) { std::string l = strings::kVelvetMenu[i]; b.add_item(slot("velvet.top", i), game_item(top_, i, [l] { return l; })); }
      b.pop_context();
      if (int c = top_.cursor(); c >= 0) b.set_start(slot("velvet.top", c));
    } else if (sc == 3) {
      b.begin_stop("velvet.method");
      b.push_context(strings::kFusionMethod);
      for (int i = 0; i < 3; ++i) { std::string l = strings::kFusionMethods[i]; b.add_item(slot("velvet.method", i), game_item(method_, i, [l] { return l; })); }
      b.pop_context();
      if (int c = method_.cursor(); c >= 0) b.set_start(slot("velvet.method", c));
    } else if (on_page()) {   // before the compendium's and registration's lists: their pages are in the same scene
      const uint8_t* e = persona::shown_status();
      bool fusion = sc == 4 && step() == 5;
      bool result = fusion && (!result_id_ || (e && *(uint16_t*)(e + 2) == result_id_));
      // The compendium's and registration's pages are a question: confirming summons or registers the Persona.
      const char* lead = result ? strings::kFusionResult : fusion ? strings::kFusionMaterial
                         : sc == 5 ? strings::kSummonThis : sc == 6 ? strings::kRegisterThis : "";
      build_status_page(b, e, lead, result ? velvet_result_bonus() : "", nullptr);
    } else if (sc == 5) {
      build_book(b);
    } else if (sc == 6) {
      build_regist(b);
    } else if (levelup()) {
      screens::set_keys(strings::kLevelUpKeys);
      build_status_page(b, levelup_, strings::kLevelUp, "", levelup_before_);
    } else if (sc == 4) {
      int st = step();
      if (st == 6) build_inherit(b);
      else if (st == 3 || st == 4 || st == 12 || st == 13) build_lists(b, st);
    }
  }
};

void VelvetScreen::build_lists(ui::GraphBuilder& b, int st) {
  uint8_t* c = cb();
  std::string key = std::format("velvet.list.{}", st);
  list_.key = key;
  b.begin_stop(key);
  // What Right walks from the focused row (add_walk): the row's Persona, or the result it makes.
  int fr = focused_row(key), n = 0;
  std::vector<std::string> walk;
  auto result_lines = [](const uint8_t* entry) {
    std::vector<std::string> l = persona::detail_lines(entry);
    if (!l.empty()) l.insert(l.begin(), std::format("{}, {}", strings::kFusionResult, persona::describe(*(uint16_t*)(entry + 2), *(uint16_t*)(entry + 4))));
    return l;
  };
  if (st == 3 || st == 4) {
    screens::set_keys(strings::kFusionKeys);
    b.push_context(st == 4 ? strings::kFusionSecond : strings::kFusionFirst);
    auto* rows = (TArray<uint8_t>*)(c + 0x138);
    auto* res = (TArray<uint8_t>*)(c + 0x148);
    int hero = persona::unit_level(1);
    auto stock = persona::stock();
    auto owned = [&stock](int id) { for (auto& o : stock) if (o.id == id) return true; return false; };
    for (int i = 0; rows->data && i < rows->num && i < 64; ++i) {
      uint8_t* r = rows->data + i * 0xA8;
      std::string l = entry_line(r);
      if (st == 4) {
        uint8_t* x = res->data && i < res->num ? res->data + i * 0xA8 : nullptr;
        std::string out = x && !r[0x8C] ? entry_line(x) : std::string();
        l += out.empty() ? std::format(", {}", strings::kFusionImpossible) : std::format(", {} {}", strings::kFusionGives, out);
        if (!out.empty() && hero > 0 && *(uint16_t*)(x + 0x1C) > hero) l += std::format(", {}", strings::kAboveYourLevel);
        if (!out.empty() && owned(*(uint16_t*)(x + 0x1A))) l += std::format(", {}", strings::kAlreadyOwned);
        if (i == fr && !out.empty()) walk = result_lines(x + 0x18);
      } else if (i == fr) {
        walk = persona::detail_lines(r + 0x18);
      }
      n = i + 1;
      b.add_item(slot(key, i), game_item(list_, i, [l] { return l; }));
    }
  } else if (st == 12) {
    screens::set_keys(strings::kFusionKeys);
    b.push_context(strings::kSpecialFusion);
    auto* rs = (TArray<uint8_t>*)(c + 0x188);
    for (int i = 0; rs->data && i < rs->num && i < 200; ++i) {
      uint8_t* r = rs->data + i * 0xC0;
      std::string l = entry_line(r), mats;
      auto* m = (TArray<uint8_t>*)(r + 0xA8);
      int need = 0, have = 0;
      for (int k = 0; m->data && k < m->num && k < 8; ++k) {
        uint8_t* x = m->data + k * 0xA8;
        ++need;
        if (x[0x8D]) ++have;
        mats += (mats.empty() ? "" : ", ") + persona::name(*(uint16_t*)(x + 0x1A));
      }
      l += std::format(", {} {}", strings::kFusionFrom, mats);
      l += have == need ? std::format(", {}", strings::kSpecialReady) : std::format(", {} {} {} {}", strings::kSpecialHave, have, strings::kOf, need);
      if (i == fr) walk = persona::detail_lines(r + 0x18);
      n = i + 1;
      b.add_item(slot(key, i), game_item(list_, i, [l] { return l; }));
    }
  } else {
    screens::set_keys(strings::kFusionSearchKeys);
    b.push_context(strings::kFusionSearch);
    auto* rs = (TArray<uint8_t>*)(c + 0x178);
    for (int i = 0; rs->data && i < rs->num && i < 400; ++i) {
      uint8_t* r = rs->data + i * 0x1F8;
      std::string l = std::format("{}, {} {} {} {}", entry_line(r), strings::kFusionFrom, persona::name(*(uint16_t*)(r + 0xC2)),
                                  strings::kFusionAnd, persona::name(*(uint16_t*)(r + 0x16A)));
      if (i == fr) walk = persona::detail_lines(r + 0x18);
      n = i + 1;
      b.add_item(slot(key, i), game_item(list_, i, [l] { return l; }));
    }
    if (rs->num <= 0) b.add_item(ui::ControlId::structural("velvet.none"), item([] { return std::string(strings::kFusionNone); }));
  }
  if (fr >= 0 && fr < n) add_walk(b, key, fr, n, walk);
  b.pop_context();
  if (int cc = list_.cursor(); cc >= 0) b.set_start(slot(key, cc));
}

void VelvetScreen::build_inherit(ui::GraphBuilder& b) {
  uint8_t* p = (uint8_t*)ps_.get();
  if (!p) return;
  auto* e = (TArray<uint8_t>*)(p + 0x470);
  b.begin_stop("velvet.inherit");
  screens::set_keys(strings::kInheritSkill);
  b.push_context(strings::kInheritTitle);
  for (int i = 0; e->data && i < e->num && i < 64; ++i) {
    uint8_t* x = e->data + i * 8;
    int id = *(uint16_t*)x;
    std::string l = persona::skill_name(id), cost = persona::skill_cost(id);
    if (!cost.empty()) l += ", " + cost;
    auto v = game_item(inherit_, i, [l] { return l; });
    // "chosen", said as it changes (Enter toggles it a frame after the press).
    v->announcements.push_back(ui::NodeAnnouncement([this, i] {
      uint8_t* pp = (uint8_t*)ps_.get();
      auto* ee = pp ? (TArray<uint8_t>*)(pp + 0x470) : nullptr;
      bool on = ee && ee->data && i < ee->num && *(uint16_t*)(ee->data + i * 8 + 6) == 1;
      return on ? std::string(strings::kChosen) : std::string();
    }, true, ui::announcement_kinds::kSelected));
    buffer_with(v, [id] { return persona::skill_help(id); });
    b.add_item(slot("velvet.inherit", i), v);
  }
  b.pop_context();
  if (int c = inherit_.cursor(); c >= 0) b.set_start(slot("velvet.inherit", c));
}

void VelvetScreen::build_book(ui::GraphBuilder& b) {
  uint8_t* bk = (uint8_t*)book_.get();
  if (!bk) return;
  screens::set_keys(strings::kCompendiumKeys);
  auto* rows = (TArray<uint8_t>*)(bk + 0x48);
  // The sort (Q / E) and the registered-only filter (Tab) rebuild the list: its count and sort name it.
  std::string key = std::format("velvet.book.{}.{}", rows->num, i32(bk, 0x124));
  book_list_.key = key;
  b.begin_stop(key);
  b.push_context(strings::kCompendium);
  auto stock = persona::stock();
  for (int i = 0; rows->data && i < rows->num && i < 600; ++i) {
    uint8_t* e = rows->data + i * 0xA8;
    int id = *(uint16_t*)(e + 0x1A), level = *(uint16_t*)(e + 0x1C);
    std::string l = e[0x88] ? std::format("{}, {} {}", persona::describe(id, level), *(int32_t*)(e + 0x94), strings::kYen)
                            : std::format("{}, {} {}, {}", social::arcana_name(persona::arcana(id)), strings::kLevel, level, strings::kNotRegistered);
    for (auto& o : stock) if (e[0x88] && o.id == id) { l += std::format(", {}", strings::kAlreadyOwned); break; }
    b.add_item(slot(key, i), game_item(book_list_, i, [l] { return l; }));
  }
  // A registered Persona's details to the right, as registered (the row's entry at +0x18).
  int n = rows->data ? std::min(rows->num, 600) : 0, f = focused_row(key);
  if (f >= 0 && f < n && rows->data[f * 0xA8 + 0x88]) add_walk(b, key, f, n, persona::detail_lines(rows->data + f * 0xA8 + 0x18));
  b.pop_context();
  if (int c = book_list_.cursor(); c >= 0) b.set_start(slot(key, c));
}

void VelvetScreen::build_regist(ui::GraphBuilder& b) {
  uint8_t* rg = (uint8_t*)regist_.get();
  if (!rg) return;
  screens::set_keys(strings::kRegisterKeys);
  auto* rows = (TArray<uint8_t>*)(rg + 0x58);
  std::string key = std::format("velvet.regist.{}", rows->num);   // rebuilt after each registration
  regist_list_.key = key;
  b.begin_stop(key);
  b.push_context(strings::kRegister);
  for (int i = 0; rows->data && i < rows->num && i < 64; ++i) {
    uint8_t* e = rows->data + i * 0xA8;
    std::string l = entry_line(e);
    if (e[0x8A]) l += std::string(", ") + strings::kMarked;
    b.add_item(slot(key, i), game_item(regist_list_, i, [l] { return l; }));
  }
  int n = rows->data ? std::min(rows->num, 64) : 0, f = focused_row(key);
  if (f >= 0 && f < n) add_walk(b, key, f, n, persona::detail_lines(rows->data + f * 0xA8 + 0x18));
  b.pop_context();
  if (int c = regist_list_.cursor(); c >= 0) b.set_start(slot(key, c));
}

// A fusion's result page (scene 4, step 5): the arcana's Social Link rank and the EXP it adds, as the page's bonus
// plate shows them (APersonaStatusDraw +0x858 ArcanaRank, +0x85C ExpBonusPoints; p3rpc.nativetypes).
std::string velvet_result_bonus() {
  static Finder draw{"UIVelvetRoomDraw"}, combine{"UICombine"}, ps{"PersonaStatus"};
  UObject* d = draw.get();
  auto* c = (uint8_t*)combine.get();
  UObject* p = ps.get();
  if (!d || !c || !p || get<uint8_t>(d, "Scene") != 4 || *(int32_t*)(c + 0x34) != 5) return {};
  auto* sd = (uint8_t*)get<UObject*>(p, "pPersonaStatusDraw");
  if (!sd) return {};
  int rank = *(int32_t*)(sd + 0x858), exp = *(int32_t*)(sd + 0x85C);
  if (rank <= 0 && exp <= 0) return {};
  std::string s = std::format("{}: {} {}", strings::kSlBonus, strings::kSlRank, rank);
  if (exp > 0) {
    s += std::format(", {} {}", exp, strings::kExp);
    const uint8_t* e = persona::shown_status();
    int now = e ? *(uint16_t*)(e + 4) : 0, after = persona::level_with_exp(e, exp);
    if (after > now) s += std::format(", {} {}", strings::kRaisesTo, after);
  }
  return s;
}

std::unique_ptr<Screen> make_velvet() { return std::make_unique<VelvetScreen>(); }
}  // namespace p3a::screens

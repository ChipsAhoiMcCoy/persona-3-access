// The camp's Social Link submenu as bespoke menus (menu.h), modelled from the game's own objects (the offsets are
// this build's, measured 2026-10-30): the list of links (CampSocial) and a link's page (CampSocialDetails). Every list
// uses the generic ListCursor (util.h): index = row + top.
//
//   UCmpCommu +0x58 open, Actor_ +0xC50 (CampSocialUi, camp_util.h): +0x2AD the list takes input, +0x2B0 the link
//   changed event, +0x410 the list scene (UCmpCommuList: +0x44 == 3 while it takes input), +0x418 the details scene,
//   +0x428 TArray<entry*> (the link records: +0 the arcana, u16), +0x448 a pointer to the ListCursor, +0x460 UCmpCommu
//   (+0x48 / +0x4C its previous / page entries).
//   The details scene: +0x54 == 1 while up, +0x58 the arcana, +0x68 a pointer to the people's ListCursor, +0x78 TArray
//   of the people (0x20 each: +0x8 the name FString; read 2026-10-30).
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/camp_util.h"
#include "bmdfile.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "msg.h"
#include "social.h"
#include <climits>
#include <format>
#include <map>
#include <tuple>
#include <utility>

namespace p3a::screens {
using namespace ue;

// ---------------------------------------------------------------- Social Link
class CampSocial : public MenuScreen, private CampSocialUi {
  Mirror& list_ = mirror("links", [this] { uint8_t* a = actor(); return a && !details() ? cursor_at(a, 0x448).index() : -1; });
  // The list scene (UCmpCommuList, actor +0x410): +0x44 == 3 while it takes input.
  uint8_t* scene() { uint8_t* a = actor(); return a ? *(uint8_t**)(a + 0x410) : nullptr; }
 public:
  // The link's move (FUN_14130bee0 from the list scene's tick): the menu's previous / page entries, the link changed
  // event (the card in hand, FUN_1412f1800 on actor +0x2B0) and the bar with the sound (FUN_1412f2d40).
  CampSocial() {
    list_.ready = [this] { uint8_t* a = actor(); uint8_t* sc = scene(); return sc && i32(sc, 0x44) == 3 && a[0x2AD]; };
    list_.set = [this](int i) {
      uint8_t* a = actor();
      uint8_t* sc = scene();
      ListCursor lc = cursor_at(a, 0x448);
      int old = lc.index();
      if (!sc || !lc.set(i)) return false;
      uint8_t* c = *(uint8_t**)(a + 0x460);   // UCmpCommu
      *(int32_t*)(c + 0x4C) = fn<int (*)(void*)>(0x1300ba0)(c);
      *(int32_t*)(c + 0x48) = old;
      fn<void (*)(void*, void*, int)>(0x12f1800)(a + 0x2B0, fn<void* (*)(void*)>(0x1300ec0)(a), i);
      fn<void (*)(void*, int)>(0x12f2d40)(sc, row_of(lc.p));
      return true;
    };
  }
  const char* id() const override { return "camp_social"; }
  int priority() const override { return 17; }
  bool active() override { uint8_t* a = actor(); return a && arr<void*>(a, 0x428) && !details(); }
  std::string title() override { return strings::kCampItems[6]; }
  void on_enter() override { screens::set_keys(strings::kListKeys); }
  void build(ui::GraphBuilder& b) override {
    uint8_t* a = actor();
    auto* links = a ? arr<uint8_t*>(a, 0x428) : nullptr;
    if (!links) return;
    auto all = social::read();
    b.begin_stop("links");
    for (int i = 0; i < links->num; ++i) {
      uint8_t* e = links->data[i];
      int arcana = e ? *(uint16_t*)e : 0;
      std::string label = social::describe(link_of(all, arcana));
      b.add_item(slot("links", i), game_item(list_, i, [label] { return label; }));
    }
    if (int c = list_.cursor(); c >= 0) b.set_start(slot("links", c));
  }
};

// ---------------------------------------------------------------- Social Link: a link's page (its people)
// The details scene (Social Link actor +0x418): +0x54 == 1 while up, +0x58 the arcana, +0x68 a pointer to the people's
// ListCursor, +0x78 TArray of the people (0x20 each: +0x8 the name FString; read 2026-10-30). RB / LB (Q / E) switch
// the link. The link's memo is BMD_CmmOutlineHelp dialog arcana * 4 (+1 lover, +2 reversed), page rank - 1; a person's
// profile is the page drawn beside them.
class CampSocialDetails : public MenuScreen, private CampSocialUi {
  Mirror& people_ = mirror("people", [this] { uint8_t* d = details(); return d ? cursor_at(d, 0x68).index() : -1; });
  social::Link link() {
    uint8_t* d = details();
    return link_of(social::read(), d ? i32(d, 0x58) : 0);
  }
  // The pages the game laid out are told apart by their file's buffer: dialog numbers repeat across files (the
  // Magician's memo, dialog 4, is also Yukari's name in the profile file). What a page showed is kept, so a link or
  // person read once keeps the game's variant.
  std::map<std::tuple<int, int, bool>, std::string> memos_;   // (arcana, rank, reversed) -> the memo drawn
  std::map<std::pair<int, std::string>, std::string> profiles_;   // (arcana, name) -> the profile drawn
  // The memo: BMD_CmmOutlineHelp dialog arcana * 4 (+1 lover, +2 reversed), page rank - 1. The page the game laid
  // out for it, the newest of the two variants (only the game knows the lover variant), else the file's.
  std::string memo() {
    social::Link l = link();
    bool rev = social::reversed(l.arcana);
    int dialog = l.arcana * 4 + (rev ? 2 : 0);
    auto key = std::make_tuple(l.arcana, l.rank, rev);
    if (const void* buf = bmdfile::buffer("BMD_CmmOutlineHelp")) {
      uint64_t seq = 0;
      for (auto& p : msg::pages_of(buf))
        if ((p.dialog == dialog || p.dialog == dialog + 1) && p.page == l.rank - 1 && p.seq >= seq) {
          seq = p.seq;
          memos_[key] = core::clean_text(p.text);
        }
    }
    if (auto it = memos_.find(key); it != memos_.end()) return it->second;
    auto f = bmdfile::load("BMD_CmmOutlineHelp");
    int pg = l.rank - 1;
    if (!f || !f->ok || dialog >= (int)f->dialogs.size() || pg < 0 || pg >= (int)f->dialogs[dialog].pages.size()) return {};
    return bmdfile::text(f->dialogs[dialog].pages[pg]);
  }
  // A person's profile: BMD_CmmProfileHelp's dialogs from their MSG_PRF_<NAME>_NAME (the name as listed) up to the
  // next person's (the profile, then _REVERSE, _MISSING, _LOST variants). The one of those the game laid out last
  // (only the game knows which variant), else the file's plain or reversed one.
  std::string profile(int i, const std::string& name) {
    social::Link l = link();
    auto key = std::make_pair(l.arcana, name);
    auto f = bmdfile::load("BMD_CmmProfileHelp");
    if (!f || !f->ok) return profiles_.contains(key) ? profiles_[key] : std::string();
    int first = -1, end = -1;
    for (int d = 0; d < (int)f->dialogs.size(); ++d) {
      auto& dl = f->dialogs[d];
      if (!dl.name.ends_with("_NAME")) continue;
      if (first >= 0) { end = d; break; }
      if (!dl.pages.empty() && bmdfile::text(dl.pages[0]) == name) first = d;
    }
    // A name the file does not list: the profile file's newest page, while the game's cursor rests on them.
    if (first < 0) { first = 0; end = INT_MAX; }
    else if (end < 0) end = (int)f->dialogs.size();
    if (const void* buf = bmdfile::buffer("BMD_CmmProfileHelp"); buf && (end != INT_MAX || people_.settled_on(i))) {
      uint64_t seq = 0;
      for (auto& p : msg::pages_of(buf))
        if (p.dialog > first && p.dialog < end && p.dialog < (int)f->dialogs.size() &&
            !f->dialogs[p.dialog].name.ends_with("_NAME") && p.seq >= seq) {
          seq = p.seq;
          profiles_[key] = core::clean_text(p.text);
        }
    }
    if (auto it = profiles_.find(key); it != profiles_.end()) return it->second;
    if (end == INT_MAX) return {};
    int pick = first + 1;
    if (social::reversed(l.arcana))
      for (int d = first + 1; d < end; ++d) if (f->dialogs[d].name.ends_with("_REVERSE")) pick = d;
    if (pick >= end || f->dialogs[pick].pages.empty()) return {};
    return bmdfile::text(f->dialogs[pick].pages[0]);
  }
 public:
  CampSocialDetails() {
    // The bar's slide and the sound (FUN_1413043b0), what the details scene's move does.
    people_.set = [this](int i) {
      uint8_t* d = details();
      if (!d || !cursor_at(d, 0x68).set(i)) return false;
      fn<void (*)(void*)>(0x13043b0)(d);
      return true;
    };
  }
  const char* id() const override { return "camp_social_details"; }
  int priority() const override { return 17; }
  bool active() override { return details() != nullptr; }
  // No title: the first line, where the page opens, names the link and its rank.
  int arcana_ = -1;
  void on_enter() override { uint8_t* d = details(); arcana_ = d ? i32(d, 0x58) : -1; screens::set_keys(strings::kSocialPageKeys); }
  void on_tick() override {
    uint8_t* d = details();
    int a = d ? i32(d, 0x58) : -1;
    if (a > 0 && a != arcana_ && arcana_ > 0) focus(ui::ControlId::structural(std::format("people.{}.memo", a)));
    if (a > 0) arcana_ = a;
  }
  void build(ui::GraphBuilder& b) override {
    uint8_t* d = details();
    auto* ppl = d ? (TArray<uint8_t>*)(d + 0x78) : nullptr;
    if (!ppl || !ppl->data || ppl->num <= 0 || ppl->num > 16) return;
    std::string key = std::format("people.{}", i32(d, 0x58));
    // Another link (Q / E): its page opens on its own line too (on_tick), not on a member.
    if (people_.key != key) people_.rekey_unfollowed(key);
    b.begin_stop(key);
    // The link itself first: its rank and memo, read on the way in.
    std::string head = social::describe(link());
    b.add_item(ui::ControlId::structural(key + ".memo"), item([this, head] { std::string m = memo(); return m.empty() ? head : head + ". " + m; }));
    for (int i = 0; i < ppl->num; ++i) {
      std::string n = core::clean_text(to_utf8(*(FString*)(ppl->data + i * 0x20 + 8)));
      auto v = game_item(people_, i, [n] { return n; });
      describe_with(v, [this, i, n] { return profile(i, n); });
      b.add_item(slot(key, i), v);
    }
    // The page opens on the link's own line, not the game's cursor on its first member (2026-10-31).
    b.set_start(ui::ControlId::structural(key + ".memo"));
  }
};

std::unique_ptr<Screen> make_camp_social() { return std::make_unique<CampSocial>(); }
std::unique_ptr<Screen> make_camp_social_details() { return std::make_unique<CampSocialDetails>(); }
}  // namespace p3a::screens

// Shops as a bespoke menu (menu.h): the police station's weapon shop, Aohige Pharmacy, Mayoido Antiques and the
// script-called simple shops (AFclShopBase and its subclasses share one state machine). From memory, spec
// 2026-10-30 (this build's offsets):
//   +0x294 state: 5 top menu, 6 party member (weapon shop), 7 buy list, 9 quantity, 10 "Buy / Quit", 0xC "Equip now?",
//          0x13 antique trade list, 0x14 sell list, 0x16 sell quantity, 0x17 / 0x18 sell confirm.
//   Top menu: TArray<i32> modes +0x2B8 (count +0x2C0), cursor u16 +0x2C8 (top +0x2CA). Modes: 1-4 Buy Weapon / Armor /
//          Footwear / Accessory, 0xC Sell, 7 Fuse Equipment, 9 Trade Card, 10 Trade Item.
//   Members: TArray<u16> ids +0x2D0 (count +0x2D8), cursor u16 row +0x2E0, top +0x2E2 (Q / E switch it from the
//          list too). What a member wears: the unit's work +0x28C + slot * 2 (u16 item id, 0x8000 = nothing).
//   Lists (buy, sell, trade): +0x2E8 (count +0x2F0), 0x28 each: +0 price, +4 u16 item id, +6 u16 how many you can
//          take (0 = not now), +8 TArray<{u16 id, u16 count}> what a trade takes; cursor u16 row +0x2F8, top +0x2FA,
//          rows shown u16 +0x2FC; a simple shop's rows also +0x1E u16 what is left of its stock. The draw actor reads
//          these every frame: written directly, with the cursor sound (the mouse path does the same). The sell list's
//          category (Q / E): cursor u16 row +0x350, top +0x352.
//   Antique fusion (state 0x10): the base equipment TArray<u16> ids +0x308 (count +0x310), cursor u16 row +0x318,
//          top +0x31A, rows shown u16 +0x31C (5); state 0x11 lists what it makes in the ordinary list (+0x2E8, with
//          the materials each takes; seen 2026-10-30). State 0x12 is the game's alone.
//   Quantity u16 +0x330 (buy; the item shop's total +0xBA8), +0x354 (sell). Yes / No cursors u16: +0x334 buy,
//          +0x336 equip now, +0x338 sell what they wore before, +0x356 sell (0 = the first choice; left / right, or up
//          / down when +0x3B8 is set).
//   State 0xD (after "Equip it now?", FUN_141400cb0): sell the replaced equipment, its id u16 +0x332, price i32 +0x32C.
//   Input gates, bool virtuals by vtable offset: +0x648 the shop takes input at all, +0x748 / +0x750 a simple shop's
//          buy list is held (state 7), +0x760 its Buy / Quit panel is (state 10).
#include "screens/menu.h"
#include "screens/util.h"
#include "core/battle_text.h"
#include "core/strings.h"
#include "items.h"
#include "msg.h"
#include "party.h"
#include "speech.h"
#include "stats.h"
#include <format>

namespace p3a::screens {
using namespace ue;

class ShopScreen : public MenuScreen {
  // The newest shop. ASimpleShopDraw (the vending machines' and other simple shops' draw, spawned after its
  // AFclSimpleShop) derives from AFclShopBase too, and its +0x294 is not the state: it is skipped (2026-11-01).
  int shops_ = watch::want("FclShopBase", true);
  UObject* shop() {
    auto v = watch::all(shops_);
    for (size_t i = v.size(); i-- > 0;) if (!is_a(v[i], "SimpleShopDraw")) return v[i];
    return nullptr;
  }
  uint8_t* s() { return (uint8_t*)shop(); }
  int state() { return s() ? *(int32_t*)(s() + 0x294) : -1; }
  // The shop's virtual at `off` (a class that does not care answers the neutral value). FUN_14140aa10 runs no state's
  // input until +0x648 says so (the item shop: its five panel animations ended); the simple shop's +0x748 / +0x750 hold
  // state 7 (its opening timer, the Buy panel closing, FUN_1413f2110) and +0x760 state 10 (the panel's timer, FUN_1413f0950).
  bool vcall(int off) { uint8_t* o = s(); return o && (*(bool (***)(void*))o)[off / 8](o); }
  bool antique() { UObject* o = shop(); return o && is_a(o, "FclAntiqueShop"); }
  static bool list_state(int st) { return st == 7 || st == 0x11 || st == 0x13 || st == 0x14; }
  static bool yes_no(int st) { return st == 10 || st == 0xC || st == 0xD || st == 0x17 || st == 0x18; }
  static int yn_off(int st) { return st == 10 ? 0x334 : st == 0xC ? 0x336 : st == 0xD ? 0x338 : 0x356; }
  int u16(int off) { return s() ? *(uint16_t*)(s() + off) : -1; }
  Mirror& top_ = mirror("shop.top", [this] { return state() == 5 ? u16(0x2C8) + u16(0x2CA) : -1; });
  Mirror& who_ = mirror("shop.member", [this] { return state() == 6 ? u16(0x2E0) + u16(0x2E2) : -1; });
  Mirror& list_ = mirror("shop.list", [this] { return list_state(state()) ? u16(0x2F8) + u16(0x2FA) : -1; });
  Mirror& fuse_ = mirror("shop.fuse", [this] { return state() == 0x10 ? u16(0x318) + u16(0x31A) : -1; });
  Mirror& yn_ = mirror("shop.yn", [this] { int st = state(); return yes_no(st) ? u16(yn_off(st)) : -1; });
  int qty_ = -1;

  // Write a u16 row / top pair (the lists' cursors) for index i, then the cursor sound.
  bool write_rows(int row_off, int i, int count, int shown) {
    if (!s() || i < 0 || i >= count) return false;
    int top = core::scroll_top(i, *(uint16_t*)(s() + row_off + 2), shown, count);
    *(uint16_t*)(s() + row_off + 2) = (uint16_t)top;
    *(uint16_t*)(s() + row_off) = (uint16_t)(i - top);
    cursor_sound();
    return true;
  }
  int member() {
    auto* ids = s() ? (TArray<uint16_t>*)(s() + 0x2D0) : nullptr;
    int c = u16(0x2E0) + u16(0x2E2);
    return ids && ids->data && c >= 0 && c < ids->num ? ids->data[c] : 0;
  }
  // The shop's list entry: name, price, owned, stats; what a trade takes; "not now" when it cannot be taken.
  std::string entry(uint8_t* e, int st) {
    int price = *(int32_t*)e, id = *(uint16_t*)(e + 4), can = *(uint16_t*)(e + 6);
    std::string n = items::name_of(id);
    // The sell list opens with "Sell all items" of the category: an entry with no item of its own, the total as its
    // price (the police station's Valuables, 2026-10-30).
    if (n.empty() && st == 0x14 && (id & 0xFFF) == 0) return std::format("{}, {} {}", strings::kSellAll, price, strings::kYen);
    if (n.empty()) n = std::format("{} {:#x}", strings::kItem, id);
    items::Info in;
    bool known = items::find(n, in);
    if (st != 0x13 && st != 0x11 && price > 0) n += std::format(", {} {}", price, strings::kYen);
    if (known) {
      if (int o = items::owned(in); o >= 0) n += std::format(", {} {}", o, strings::kOwned);
      n += items::stats(in);
    }
    auto* need = (TArray<uint16_t[2]>*)(e + 8);
    if ((st == 0x13 || st == 0x11) && need->data && need->num > 0 && need->num < 8) {
      std::string t;
      for (int k = 0; k < need->num; ++k) {
        int nid = need->data[k][0], num = need->data[k][1];
        if (!nid || !num) continue;
        std::string nn = items::name_of(nid);
        items::Info ni;
        int have = items::find(nn, ni) ? items::owned(ni) : -1;
        t += std::format("{} {} {}", t.empty() ? std::string(", ") + strings::kNeeds : ",", nn, num);
        if (have >= 0) t += std::format(", {} {}", strings::kYouHave, have);
      }
      n += t;
    }
    // A buy row's count (+6) is the least of three (exe+0x1401050): its stock (+0x1E), what the money buys and what
    // can still be carried (99). The stock is a simple shop's alone (the week's, from the save; 99 = never runs out,
    // exe+0x1404bd0 does not count it down); the other shops write 99. The row says which of the three stops it.
    if (st == 7) {
      int stock = *(uint16_t*)(e + 0x1E), have = known ? items::owned(in) : -1, yen = stats::money();
      if (stock > 0 && stock < 99) n += std::format(", {} {}", stock, strings::kLeft);
      if (can) return n;
      return n + ", " + core::cannot_buy_reason(stock, have, price, yen);
    }
    if (!can) n += std::string(", ") + (st == 0x14 ? strings::kUnavailable : strings::kCannotBuy);
    return n;
  }
  // What the member wears in that slot now (the weapon shop's lists are for one member and one slot).
  std::string worn_now(int mode) {
    int who = member();
    uint8_t* u = who ? party::unit_work(who) : nullptr;
    if (!u || mode < 1 || mode > 4) return {};
    int id = *(uint16_t*)(u + 0x28C + (mode - 1) * 2);
    if (id == 0x8000) return {};
    std::string n = items::name_of(id);
    items::Info in;
    if (!n.empty() && items::find(n, in)) n += items::stats(in);
    return n.empty() ? std::string() : std::format("{} {}: {}", party::name(who), strings::kEquipped, n);
  }
  // The row's own help line from the game's file (items::help), so a quick move never reads the last row's page.
  // A weapon's help ends "For Yukari.", the member the list is already for: dropped.
  // Without the file, the page the game drew for this row: its dialog is the item's index, as in the file (Band-Ace,
  // 0x4050, is dialog 0x50). The game builds it in the frame the cursor moves or reuses one built earlier, so a page
  // is matched by its dialog, never by when it was built (that read nothing at all, 2026-11-03).
  static std::string drawn_help(int id) {
    std::string d;
    for (auto& p : msg::visible(300)) if (p.dialog == (id & 0xFFF) && p.text.size() > d.size()) d = p.text;
    return d;
  }
  std::string description(int i) {
    if (list_.seen != i || list_.target >= 0) return {};
    auto* rows = (TArray<uint8_t>*)(s() + 0x2E8);
    if (!rows->data || i < 0 || i >= rows->num) return {};
    int id = *(uint16_t*)(rows->data + i * 0x28 + 4);
    std::string d = items::help(id);
    if (d.empty()) d = drawn_help(id);
    if (int who = member()) {
      std::string tail = std::format(" For {}.", party::name(who));   // the game's words (BMD_ItemWeaponHelp)
      if (d.size() > tail.size() && d.ends_with(tail)) d.resize(d.size() - tail.size());
    }
    return d;
  }

 public:
  ShopScreen() {
    top_.ready = [this] { return vcall(0x648); };
    list_.ready = [this] { return vcall(0x648) && (state() != 7 || (!vcall(0x748) && !vcall(0x750))); };
    yn_.ready = [this] { return vcall(0x648) && (state() != 10 || !vcall(0x760)); };
    top_.set = [this](int i) { return write_rows(0x2C8, i, s() ? *(int32_t*)(s() + 0x2C0) : 0, 0); };
    list_.set = [this](int i) { return write_rows(0x2F8, i, s() ? *(int32_t*)(s() + 0x2F0) : 0, u16(0x2FC)); };
    fuse_.set = [this](int i) { return write_rows(0x318, i, s() ? *(int32_t*)(s() + 0x310) : 0, u16(0x31C)); };
    // The member list shows every row; the game's move (exe+0x14037d0) adds only the sound.
    who_.set = [this](int i) { return state() == 6 && write_rows(0x2E0, i, s() ? *(int32_t*)(s() + 0x2D8) : 0, 0); };
    yn_.set = [this](int i) {
      int st = state();
      if (!yes_no(st) || i < 0 || i > 1) return false;
      *(uint16_t*)(s() + yn_off(st)) = (uint16_t)i;
      cursor_sound();
      return true;
    };
  }
  const char* id() const override { return "shop"; }
  int priority() const override { return 18; }
  bool active() override {
    int st = state();
    return st == 5 || st == 6 || list_state(st) || st == 9 || st == 0x16 || yes_no(st) || st == 0x10 || st == 0x11 || st == 0x12;
  }
  // The quantity is the game's (up and down change it): the menu stands aside, its number is read as it changes.
  bool holds_keys() override { int st = state(); return st != 9 && st != 0x16 && st != 0x12; }
  std::string title() override {
    return std::format("{}. {} {}", antique() ? strings::kAntiqueShop : strings::kShop, stats::money(), strings::kYen);
  }
  // F1: the keys of the state in front (the weapon shop's lists switch member with Q / E, the sell list category).
  std::string keys() {
    int st = state();
    if (st == 5) return strings::kShopTopKeys();
    if (st == 7) return member() ? strings::kShopWeaponKeys() : strings::kShopListKeys();
    if (st == 0x14) return strings::kShopSellKeys();
    if (st == 0x11 || st == 0x13) return strings::kShopTradeKeys();
    if (st == 9 || st == 0x16) return strings::kHowMany();
    if (yes_no(st)) return s()[0x3B8] ? strings::kChooseKeys() : strings::kShopYesNoKeys();
    return st == 6 || st == 0x10 ? strings::kListKeys() : "";
  }
  void on_enter() override { screens::set_keys([this] { return keys(); }); }
  void on_tick() override {
    int st = state();
    if (st != 9 && st != 0x16) { qty_ = -1; return; }
    int q = u16(st == 9 ? 0x330 : 0x354);
    if (q == qty_) return;
    std::string line = std::to_string(q);
    // +0xBA8 is the item shop's own (AFclItemShop is 0x1128 bytes; the weapon and simple shops end before it)
    if (st == 9 && is_a(shop(), "FclItemShop")) if (int total = *(int32_t*)(s() + 0xBA8); total > 0) line += std::format(", {} {}", total, strings::kYen);
    // Not through the menu's no-repeat guard: the game has the keys here, so nothing tells the guard of a press, and
    // a number gone back to (2, 3, 2) is said again. The keys are F1's (keys(): kHowMany).
    speech::speak(qty_ < 0 ? std::format("{} {}", strings::kHowManyWords, line) : line, true);
    qty_ = q;
  }
  void build(ui::GraphBuilder& b) override {
    int st = state();
    if (st == 5) {
      auto* modes = (TArray<int32_t>*)(s() + 0x2B8);
      b.begin_stop("shop.top");
      for (int i = 0; modes->data && i < modes->num && i < 16; ++i) {
        std::string l = core::shop_mode_name(modes->data[i]);
        b.add_item(slot("shop.top", i), game_item(top_, i, [l] { return l; }));
      }
      if (int c = top_.cursor(); c >= 0) b.set_start(slot("shop.top", c));
    } else if (st == 6) {
      auto* ids = (TArray<uint16_t>*)(s() + 0x2D0);
      b.begin_stop("shop.member");
      b.push_context(strings::kForWhom);
      for (int i = 0; ids->data && i < ids->num && i < 16; ++i) {
        int id = ids->data[i];
        b.add_item(slot("shop.member", i), game_item(who_, i, [id] { return party::name(id); }));
      }
      b.pop_context();
      if (int c = who_.cursor(); c >= 0) b.set_start(slot("shop.member", c));
    } else if (list_state(st)) {
      auto* rows = (TArray<uint8_t>*)(s() + 0x2E8);
      // Q / E switch the weapon shop's member: a new list. The top menu's mode says what it sells.
      auto* modes = (TArray<int32_t>*)(s() + 0x2B8);
      int top = u16(0x2C8) + u16(0x2CA);
      int mode = modes->data && top >= 0 && top < modes->num ? modes->data[top] : 0;
      // The sell list's category (Q / E; cursor u16 +0x350) is part of its identity: two categories can hold as many.
      int cat = st == 0x14 ? u16(0x350) + u16(0x352) : 0;
      std::string key = std::format("shop.list.{}.{}.{}.{}", st, member(), cat, rows->num);
      list_.key = key;
      b.begin_stop(key);
      std::string ctx = st == 0x14 ? std::string(strings::kWeaponShopMenu[4]) : st == 0x11 ? std::string(strings::kAntiqueMenu[0]) : mode ? core::shop_mode_name(mode) : std::string();
      if (int who = member(); who && mode >= 1 && mode <= 4) ctx += std::format(", {} {}", strings::kFor, party::name(who));
      b.push_context(ctx);
      std::string worn = worn_now(mode);
      for (int i = 0; rows->data && i < rows->num && i < 300; ++i) {
        std::string l = entry(rows->data + i * 0x28, st);
        // The description, then what the member wears now.
        auto v = game_item(list_, i, [l] { return l; }, [worn] { return worn; });
        describe_with(v, [this, i] { return description(i); });
        describe_with(v, [worn] { return worn; });
        b.add_item(slot(key, i), v);
      }
      if (!rows->data || rows->num <= 0) b.add_item(ui::ControlId::structural("shop.none"), item([] { return std::string(strings::kNoItems); }));
      b.pop_context();
      if (int c = list_.cursor(); c >= 0) b.set_start(slot(key, c));
    } else if (st == 0x10) {
      auto* ids = (TArray<uint16_t>*)(s() + 0x308);
      b.begin_stop("shop.fuse");
      b.push_context(strings::kWhichMaterial);
      for (int i = 0; ids->data && i < ids->num && i < 300; ++i) {
        int id = ids->data[i];
        std::string n = items::name_of(id);
        items::Info in;
        if (!n.empty() && items::find(n, in)) {
          if (int o = items::owned(in); o >= 0) n += std::format(", {} {}", o, strings::kOwned);
          n += items::stats(in);
        }
        auto v = game_item(fuse_, i, [n] { return n; });
        std::string help = items::help(id);
        describe_with(v, [this, i, id, help] { return !help.empty() ? help : fuse_.seen == i && fuse_.target < 0 ? drawn_help(id) : std::string(); });
        b.add_item(slot("shop.fuse", i), v);
      }
      b.pop_context();
      if (int c = fuse_.cursor(); c >= 0) b.set_start(slot("shop.fuse", c));
    } else if (yes_no(st)) {
      // The prompt's two choices; left / right in the game (up / down when +0x3B8 is set).
      yn_.horizontal = !s()[0x3B8];
      yn_.keep_edges = yn_.horizontal;   // the game's pair wraps
      std::string key = std::format("shop.yn.{}", st);
      yn_.key = key;
      std::string q = st == 10 ? strings::kBuyQuestion : st == 0xC ? strings::kEquipQuestion : strings::kSellConfirm;
      if (st == 0xD) {
        q = strings::kSellOldQuestion;
        std::string n = items::name_of(u16(0x332));
        if (!n.empty()) q += std::format(" {}, {} {}", n, *(int32_t*)(s() + 0x32C), strings::kYen);
      }
      const char* a = st == 10 ? strings::kBuy : st == 0xC ? strings::kEquip : strings::kSell;
      const char* n = st == 0xC ? strings::kDontEquip : st == 0xD ? strings::kKeep : strings::kQuit;
      b.begin_stop(key);
      b.push_context(q);
      if (yn_.horizontal) b.start_row(key);
      b.add_item(slot(key, 0), game_item(yn_, 0, [a] { return std::string(a); }));
      b.add_item(slot(key, 1), game_item(yn_, 1, [n] { return std::string(n); }));
      if (yn_.horizontal) b.end_row();
      b.pop_context();
      if (int c = yn_.cursor(); c >= 0) b.set_start(slot(key, c));
    }
  }
};

std::unique_ptr<Screen> make_shop() { return std::make_unique<ShopScreen>(); }
}  // namespace p3a::screens

// Shuffle Time after a battle (BtlShuffleMainBase, BP_BtlShuffleUI_C): the cards are art, each card's effect is a
// BMD page of BMD_BtlShuffleMsg ("Obtain the Lovers Persona, Pixie."), its variables filled per card by the game.
// Reflected: ShowCardsMax (cards dealt), EnableSelectNum (cards you may take), SelectCoursorIndex (the card under the
// cursor), _PersonaStatusActor (a Persona card's status page), GetAppearCard (a card's type and id), GetMegNoDetailArcanaSelectCard (the dialog of a card's effect page).
// The info panel (BP Tick part, BP_BtlShuffleUI "preSelectCardIdx"): when SelectCoursorIndex changes it fades
// the old card's page out (selectCardInfoOutStart, SelectCardFadeTimer counting down), and only when the timer runs
// out sets showCardInfoIndex to the new card and builds its page. So the page on screen is showCardInfoIndex's, never
// the cursor's: reading it as the cursor's gave every card the first card's effect (2026-11-01). A timer written to 0
// ends the fade-out on the game's next tick.
// The actor stays up from the deal to the last card's effect; between those the cards spin and no page is drawn
// (2026-10-30: the screen used to drop out then, and the battle's lingering All-Out prompt was read in its place for
// eight minutes). A new, empty actor (ShowCardsMax 0) is made for the next battle during the results.
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/battle.h"
#include "core/strings.h"
#include "log.h"
#include "msg.h"
#include "persona.h"
#include "speech.h"
#include <windows.h>
#include <array>
#include <cstring>
#include <format>
#include <vector>

namespace p3a::screens {
using namespace ue;

void build_status_page(ui::GraphBuilder& b, const uint8_t* e, const std::string& lead, const std::string& bonus,
                       const uint8_t* before, const ui::ControlId* name_id, ui::NodeVtablePtr name_item);

UObject* shuffle_ui() {
  static const int id = watch::want("BtlShuffleMainBase", true);
  for (UObject* u : watch::all(id))
    if (get<int32_t>(u, "ShowCardsMax") > 0) return u;
  return nullptr;
}

// The cards as a row (left and right here, A and D in the game). Every card's effect is read before the row is shown:
// once the cards take input, the mod puts the game's cursor on each card in turn (the fade skipped), reads the page the
// game builds for it, and puts the cursor back. A card whose data changes is read again (no Major Arcana changes the others: CheckChangeCard is the card
// models' spawn; the Star's extra pick and Arcana Burst's row are the in-deal changes, 2026-11-02).
// Arcana Burst (all the day's Major Arcana collected; isShowBonousCard, state 6 SelectCardAllGetBonus): a second row of
// four Minor Arcana cards (+0x2CC, 0x10 each: type, rank, value) after the deal, taken EnableSelectNum times ("Draw 2"),
// C cancelling the last; the same cursor, info page and picks (SelectedCardNum / GetSelectedCardIndex) as the deal.
// An item card (GetItemNumSelectCard >= 0) says how many you own (GetHaveItemNumBySelectCard), as the game shows.
// Persona Overstock (state 4, a Persona card taken with the stock full; isEndPersonaListInFlag once it is in): a list
// of the new Persona ("Gain", row 0) and the stock in order (rows 1..), PersonaSelectCoursorIndex, wrapping, moved
// with Up / Down; Enter releases the Persona under it (row 0: the new one is not taken), F shows its stats. Its rows
// are drawn natively (UBtlShufflePersonaList, unreflected): they follow the card (ActionCardIndex's slot: type 1, the
// Persona's id) and the stock, checked against the screen 2026-11-02 (7 rows over a 6-Persona stock). F opens the
// row's status page (_PersonaStatusActor, +0x528 == 5 while shown, the field cleared on close; Q / E move the list's
// cursor with it).
class ShuffleScreen : public MenuScreen {
  using Param = std::array<uint8_t, 12>;   // FAppPropsCardParam
  struct Card { Param param{}; std::string effect; int owned = -1; bool read = false; };
  std::vector<Card> cards_;
  WeakObj deal_;              // the deal the cards belong to
  bool listed_ = false;       // every card read once: the row is up
  bool reading_ = false;      // the mod has the game's cursor
  int home_ = 0;              // the player's card while the mod reads the others
  int reading_card_ = -1;     // the card the cursor was put on
  uint64_t mark_ = 0;         // pages built after this are the card's
  int waited_ = 0;            // ticks on reading_card_
  std::string persona_;
  bool bonus_ = false;        // the row is Arcana Burst's
  bool overstock_ = false;    // the release list was said
  int stats_ = 0;             // the Persona whose status page is up over the release list
  int picked_ = 0;            // SelectedCardNum as last seen
  Mirror& release_ = mirror("release", [] {
    UObject* ui = shuffle_ui();
    return ui && overstock(ui) ? get<int32_t>(ui, "PersonaSelectCoursorIndex") : -1;
  });
  Mirror& row_ = mirror("cards", [this] {
    UObject* ui = shuffle_ui();
    if (!ui || !listed_) return -1;
    return reading_ ? home_ : get<int32_t>(ui, "SelectCoursorIndex");
  });

  static bool bonus(UObject* ui) { return get_bool(ui, "isShowBonousCard"); }
  // How many cards the row has: the deal's, or Arcana Burst's (its slots with a card in them).
  static int count(UObject* ui) {
    if (!bonus(ui)) return get<int32_t>(ui, "ShowCardsMax");
    int n = 0;
    while (n < 4 && *(int32_t*)((uint8_t*)ui + 0x2CC + n * 0x10) != 0) ++n;
    return n;
  }
  static int card_call(UObject* ui, const char* fn, int i) {
    UFunction* f = find_function(class_of(ui), fn);
    if (!f) return -1;
    Params p(f);
    p.set<int32_t>("idx", i);
    p.set<bool>("isBonusCard", bonus(ui));
    call(ui, p);
    return p.ret<int32_t>();
  }
  static Param param(UObject* ui, int i) {
    Param out{};
    UFunction* f = find_function(class_of(ui), "GetAppearCard");
    if (!f) return out;
    Params p(f);
    p.set<int32_t>("idx", i);
    p.set<bool>("isBonusCard", bonus(ui));
    call(ui, p);
    if (auto* o = p.at<uint8_t>("Output")) std::memcpy(out.data(), o, out.size());
    return out;
  }
  static int detail_dialog(UObject* ui, int i) {
    UFunction* f = find_function(class_of(ui), "GetMegNoDetailArcanaSelectCard");
    if (!f) return -1;
    Params p(f);
    p.set<int32_t>("idx", i);
    p.set<int32_t>("minorId", 0);
    p.set<bool>("isBonusCard", bonus(ui));
    call(ui, p);
    return p.ret<int32_t>();
  }
  static const void* bmd_buffer(UObject* ui) {
    UObject* a = call_ret<UObject*>(ui, "GetBmdAsset");
    auto* buf = a ? field<TArray<uint8_t>>(a, "mBuf") : nullptr;
    return buf && buf->num > 0 ? buf->data : nullptr;
  }
  // The game shows card i's info and has built its page since the cursor was put there: the effect page (its dialog),
  // else the longest new page of the file (the other is the Persona's name alone).
  std::string read_page(UObject* ui, int i) {
    if (get<int32_t>(ui, "showCardInfoIndex") != i || get_bool(ui, "selectCardInfoOutStart")) return {};
    const void* buf = bmd_buffer(ui);
    int want = detail_dialog(ui, i);
    std::string by_dialog, longest;
    for (auto& c : msg::visible(300)) {
      if (c.seq <= mark_ || (buf && c.buf != buf)) continue;
      if (c.dialog == want && c.text.size() > by_dialog.size()) by_dialog = c.text;
      if (c.text.size() > longest.size()) longest = c.text;
    }
    std::string t = by_dialog.empty() ? longest : by_dialog;
    if (!t.empty()) log::writef("shuffle: card {} dialog {} ({}): {}", i, want, by_dialog.empty() ? "longest" : "matched", t);
    if (!t.empty() && t.back() == '.') t.pop_back();   // joined with the mod's own full stops
    return t;
  }
  // Puts the game's cursor on card i with no sound and no fade: its info is built on the game's next tick.
  void show(UObject* ui, int i) {
    if (bool* first = field<bool>(ui, "IsFirstSelect")) *first = false;
    if (int32_t* c = field<int32_t>(ui, "SelectCoursorIndex")) *c = i;
    if (float* t = field<float>(ui, "SelectCardFadeTimer")) *t = 0.f;
    if (const void* buf = bmd_buffer(ui)) msg::forget_pages(buf);
    mark_ = msg::created_seq();
  }
  // One step of reading the cards: the next unread card's page, or the cursor back to the player's card when all are read.
  void read_cards(UObject* ui) {
    int next = -1;
    for (int i = 0; i < (int)cards_.size() && next < 0; ++i)
      if (!cards_[i].read && !taken(ui, i)) next = i;
    if (next < 0) {
      if (reading_) {
        show(ui, home_);
        reading_ = false;
        reading_card_ = -1;
      }
      listed_ = true;
      return;
    }
    if (!reading_) {
      reading_ = true;
      int c = get<int32_t>(ui, "SelectCoursorIndex");
      home_ = c >= 0 && c < (int)cards_.size() ? c : 0;
    }
    if (reading_card_ != next || get<int32_t>(ui, "SelectCoursorIndex") != next) {
      show(ui, next);
      reading_card_ = next;
      waited_ = 0;
      return;
    }
    std::string t = read_page(ui, next);
    if (!t.empty()) {
      cards_[next].effect = t;
      cards_[next].read = true;
      cards_[next].owned = -1;
      if (card_call(ui, "GetItemNumSelectCard", next) >= 0) cards_[next].owned = card_call(ui, "GetHaveItemNumBySelectCard", next);
      return;
    }
    // A card whose page never comes (a state not modelled): it keeps its number instead of holding the row forever.
    if (++waited_ > 120) {
      log::writef("shuffle: no page for card {} (showing {}, fading {})", next, get<int32_t>(ui, "showCardInfoIndex"),
                  get_bool(ui, "selectCardInfoOutStart"));
      cards_[next].read = true;
    }
  }
  static const uint8_t* persona_page(UObject* ui) {
    UObject* ps = get<UObject*>(ui, "_PersonaStatusActor");
    if (!persona::status_page_up(ps)) return nullptr;
    const uint8_t* e = persona::status_entry(ps);
    return e && *(uint16_t*)(e + 2) ? e : nullptr;
  }
  std::string label(int i) {
    if (i < 0 || i >= (int)cards_.size()) return {};
    std::string s = cards_[i].effect.empty() ? std::format("{} {}", strings::kCard, i + 1) : cards_[i].effect;
    if (cards_[i].owned >= 0) s += std::format(", {} {}", strings::kOwned, cards_[i].owned);
    return s;
  }

 public:
  // Choosing (+0x278 == 2, unreflected; 6 in Arcana Burst) once the cards have come in, outside a Major Arcana effect;
  // +0xF71 is the input step's "took input" byte. The first press only shows the cursor
  // (IsFirstSelect).
  static bool choosing(UObject* ui) {
    auto* p = (uint8_t*)ui;
    int st = p ? *(int32_t*)(p + 0x278) : 0;
    return p && (st == 2 || st == 6) && get_bool(ui, "isEndCardInAnime") && !get_bool(ui, "IsStartMajorArcanaEffect") &&
           (get_bool(ui, "IsFirstSelect") || p[0xF71] == 1);
  }
  // The release list's status page (F), or null.
  static const uint8_t* overstock_stats(UObject* ui) {
    UObject* ps = get<UObject*>(ui, "_PersonaStatusActor");
    if (!ps || *(int32_t*)((uint8_t*)ps + 0x528) != 5) return nullptr;
    const uint8_t* e = persona::status_entry(ps);
    return e && *(uint16_t*)(e + 2) ? e : nullptr;
  }
  static bool overstock(UObject* ui) { return *(int32_t*)((uint8_t*)ui + 0x278) == 4 && get_bool(ui, "isEndPersonaListInFlag"); }
  // The Persona the taken card gives (the card being acted on: type 1, value its id).
  static int gained(UObject* ui) {
    int i = get<int32_t>(ui, "ActionCardIndex");
    if (i < 0 || i > 4) return 0;
    auto* c = (int32_t*)((uint8_t*)ui + 0x27C + i * 0x10);
    return c[0] == 1 ? c[2] : 0;
  }
  static bool taken(UObject* ui, int i) {
    UFunction* f = find_function(class_of(ui), "GetSelectedCardIndex");
    int n = get<int32_t>(ui, "SelectedCardNum");
    for (int k = 0; f && k < n; ++k) {
      Params p(f);
      p.set<int32_t>("selectIdx", k);
      call(ui, p);
      if (p.ret<int32_t>() == i) return true;
    }
    return false;
  }
  // The card under the cursor is SelectCoursorIndex, which confirm takes: written as the game's own hover does (the
  // index and cue 4002), never onto a card already taken.
  ShuffleScreen() {
    row_.ready = [this] { return !reading_ && choosing(shuffle_ui()); };
    row_.keep_edges = true;   // the game's row of cards wraps
    row_.set = [this](int i) {
      UObject* ui = shuffle_ui();
      if (reading_ || !choosing(ui) || i < 0 || i >= count(ui) || taken(ui, i)) return false;
      if (bool* first = field<bool>(ui, "IsFirstSelect")) *first = false;
      if (int32_t* c = field<int32_t>(ui, "SelectCoursorIndex")) *c = i;
      cursor_sound(4002);
      return true;
    };
    release_.count = 0;   // set per build: the list wraps over its rows
    release_.ready = [] { UObject* ui = shuffle_ui(); return ui && overstock(ui); };
  }
  const char* id() const override { return "shuffle"; }
  int priority() const override { return 26; }
  // Every card it lets you take is taken: the deal is over once their effects are (a Persona card's status page). The
  // actor outlives it, through the victory screen that always follows (it read the taken card again over the
  // results' end, 2026-11-02).
  static bool done(UObject* ui) {
    int k = get<int32_t>(ui, "EnableSelectNum");
    return k > 0 && get<int32_t>(ui, "SelectedCardNum") >= k;
  }
  bool active() override {
    UObject* ui = shuffle_ui();
    return ui && result_state(result_ui()) < 1 && (!done(ui) || persona_page(ui) || overstock(ui));
  }
  std::string title() override {
    UObject* ui = shuffle_ui();
    if (ui && overstock(ui)) return strings::kPersonaOverstock;
    std::string s = strings::kShuffleTime;
    // How many cards there are is the position on each card ("1 of 4"; the player's call, 2026-10-30): only taking
    // more than one is said here.
    if (int k = ui ? get<int32_t>(ui, "EnableSelectNum") : 1; k > 1) s += std::format(". {} {}", strings::kChoose, k);
    return s;
  }
  void on_enter() override {
    UObject* ui = shuffle_ui();
    overstock_ = ui && overstock(ui);   // said by the title on entry
    screens::set_keys(overstock_ ? strings::kOverstockKeys : strings::kShuffleKeys);
  }
  // The deal is over: the next battle's deal comes on the same actor (ShowCardsMax set again), so its cards are
  // forgotten here (the next deal was said with the last one's first card until its own were read, 2026-11-02).
  void on_leave() override {
    if (active()) return;
    cards_.clear();
    listed_ = reading_ = bonus_ = overstock_ = false;
    stats_ = 0;
    reading_card_ = -1;
    picked_ = 0;
    deal_ = {};
  }
  void on_tick() override {
    UObject* ui = shuffle_ui();
    if (!ui) return;
    if (deal_.resolve() != ui) {
      deal_ = WeakObj::of(ui);
      cards_.clear();
      listed_ = reading_ = false;
      reading_card_ = -1;
    }
    // Arcana Burst's row replaces the deal's: read anew, then said with its title.
    if (bonus(ui) != bonus_) {
      bonus_ = bonus(ui);
      cards_.clear();
      listed_ = false;
      picked_ = 0;
    }
    int n = count(ui);
    if ((int)cards_.size() != n && n > 0) cards_.resize(n);
    // A Persona card's status page: read it when it comes.
    if (const uint8_t* e = persona_page(ui)) {
      std::string p = persona::status_line(e);
      if (!p.empty() && p != persona_) { persona_ = p; speech::speak(std::format("{}. {}", strings::kPersonaStatus, p), true); }
      return;
    }
    persona_.clear();
    if (overstock(ui) != overstock_) {
      overstock_ = !overstock_;
      screens::set_keys(overstock_ ? strings::kOverstockKeys : strings::kShuffleKeys);
      if (overstock_) {
        speech::speak(strings::kPersonaOverstock, true);
        focus(slot("release", get<int32_t>(ui, "PersonaSelectCoursorIndex")), false);
      }
    }
    if (overstock_) {
      const uint8_t* e = overstock_stats(ui);
      int id = e ? *(uint16_t*)(e + 2) : 0;
      if (id != stats_) {
        stats_ = id;
        focus(id ? ui::ControlId::structural(std::format("ps.{}.name", id)) : slot("release", get<int32_t>(ui, "PersonaSelectCoursorIndex")));
      }
    }
    if (!choosing(ui)) return;
    // A card whose data changed is read again.
    for (int i = 0; i < (int)cards_.size(); ++i)
      if (Param p = param(ui, i); p != cards_[i].param) {
        // Its old text stays until the new one is read: a "Card 1" in between was said too (2026-11-02).
        cards_[i].param = p;
        cards_[i].read = false;
      }
    bool was = listed_;
    read_cards(ui);
    if (!listed_) return;
    // Arcana Burst's row has come: its title, then the card under the cursor.
    int k = get<int32_t>(ui, "SelectedCardNum"), left = get<int32_t>(ui, "EnableSelectNum") - k;
    if (!was && bonus_) {
      speech::speak(std::format("{}. {} {}", strings::kArcanaBurst, strings::kChoose, left), true);
      if (int c = row_.cursor(); c >= 0) focus(slot("cards", c), false);
    }
    // A card taken with more still to take, or a pick cancelled (C): how many are left (the game's "Draw 1").
    else if (k > picked_ && left > 0) speech::speak(std::format("{} {} {}", strings::kChoose, left, strings::kMore), false);
    else if (k < picked_) speech::speak(std::format("{} {}", strings::kChoose, left), false);
    picked_ = k;
  }
  void build(ui::GraphBuilder& b) override {
    if (UObject* ui = shuffle_ui(); ui && overstock(ui)) {
      if (const uint8_t* e = overstock_stats(ui)) { build_status_page(b, e, {}, {}, nullptr, nullptr, {}); return; }
      std::vector<std::string> rows;
      int id = gained(ui);
      rows.push_back(std::format("{}: {}", strings::kGain, id ? persona::describe(id, persona::base_level(id)) : std::string()));
      for (auto& o : persona::stock()) rows.push_back(persona::describe(o.id, o.level));
      release_.count = (int)rows.size();
      b.begin_stop("release");
      for (int i = 0; i < (int)rows.size(); ++i) {
        std::string t = rows[i];
        b.add_item(slot("release", i), game_item(release_, i, [t] { return t; }));
      }
      if (int c = release_.cursor(); c >= 0) b.set_start(slot("release", c));
      return;
    }
    if (!listed_ || cards_.empty()) return;
    b.begin_stop("cards");
    b.start_row("cards");
    UObject* ui = shuffle_ui();
    for (int i = 0; i < (int)cards_.size(); ++i) {
      if (ui && taken(ui, i)) continue;   // a card taken stays drawn, face up
      auto v = game_item(row_, i, [this, i] { return label(i); }, [this, i] { return label(i); });
      v->announcements[0].live = true;   // a card the game changed is said again once read
      b.add_item(slot("cards", i), v);
    }
    b.end_row();
    if (int c = row_.cursor(); c >= 0) b.set_start(slot("cards", c));
  }
  std::string describe() override {
    if (!persona_.empty()) return std::format("{}. {}", strings::kShuffleTime, persona_);
    return MenuScreen::describe();
  }
};

std::unique_ptr<Screen> make_shuffle() { return std::make_unique<ShuffleScreen>(); }
}  // namespace p3a::screens

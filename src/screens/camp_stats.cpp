// The camp's Stats submenu as bespoke menus (menu.h), modelled from the game's own objects (the offsets are this
// build's, measured 2026-10-30): the party list with Tactics and its orders (CampStats), and a member's page
// (CampStatsPage). Every list uses the generic ListCursor (util.h): index = row + top.
//
//   UCmpStatus +0x44 != 0, Actor_ +0x58 (UUICmpStatus; CampStatsUi, camp_util.h): +0x28 TArray<u16> ids, +0x60 the
//   party panel, ListCursor +0x68, +0x330 the UI layer (its input gate, the open timer's frames), +0x344 the open
//   timer. The scene on top of SceneFSM_ (+0x2B0): CmpStatusCharacterList (+0x48 mode 1 list, 3 Tactics, 4 order;
//   +0xA0 a pointer to the orders' ListCursor), CmpCharacterStatus or CmpHeroHumanStatus (a member's page).
//   A member's tactic: unit +0x296.
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/camp_util.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "party.h"
#include "persona.h"
#include "stats.h"
#include "textcap.h"
#include <algorithm>
#include <cctype>
#include <format>

namespace p3a::screens {
using namespace ue;

// ---------------------------------------------------------------- Stats (the party list and Tactics)
class CampStats : public MenuScreen, private CampStatsUi {
  int mode() { UObject* sc = scene(); return sc && is_a(sc, "CmpStatusCharacterList") ? i32(sc, 0x48) : -1; }
  Mirror& list_ = mirror("stats", [this] { int md = mode(); return md == 1 || md == 3 ? ListCursor(ui() + 0x68).index() : -1; });
  Mirror& order_ = mirror("stats.order", [this] { return mode() == 4 ? cursor_at(scene(), 0xA0).index() : -1; });
  std::vector<std::string> orders_;   // the tactics' names as the order list draws them (on_tick): known once it was listed
  int tactic(int id) { uint8_t* u = party::unit_work(id); return u ? *(uint8_t*)(u + 0x296) : -1; }
  std::string tactic_name(int t) {
    if (t >= 0 && t < (int)orders_.size()) return orders_[t];
    return t >= 0 && t < 5 ? strings::kTacticNames[t] : std::string();
  }
  // The UI layer has input (FUN_1412d5f90, the gate the scene's own moves pass).
  bool input_on() { uint8_t* u = ui(); return u && fn<bool (*)(void*)>(0x12d5f90)(*(void**)(u + 0x330)); }
 public:
  CampStats() {
    // The party list's move in one call (FUN_141352af0): the sound, the panel, the member shown, the camera.
    // Opening (mode 1) also waits for the scene's open timer +0x344 (FUN_141350110, what its confirm waits for).
    list_.ready = [this] {
      int md = mode();
      if ((md != 1 && md != 3) || !input_on()) return false;
      uint8_t* u = ui();
      return md == 3 || open_timer_done(*(float*)(u + 0x344), *(void**)(u + 0x330));
    };
    list_.set = [this](int i) {
      uint8_t* u = ui();
      int md = mode();
      if ((md != 1 && md != 3) || !ListCursor(u + 0x68).set(i)) return false;
      fn<void (*)(void*, void*, int)>(0x1352af0)(scene(), *(void**)(u + 0x60), row_of(u + 0x68));
      return true;
    };
    // The order list: FUN_141352c40, the sound and the bar.
    order_.ready = [this] { return mode() == 4 && input_on(); };
    order_.set = [this](int i) {
      if (mode() != 4 || !cursor_at(scene(), 0xA0).set(i)) return false;
      fn<void (*)(void*)>(0x1352c40)(scene());
      return true;
    };
  }
  const char* id() const override { return "camp_stats"; }
  int priority() const override { return 17; }
  // The list scene in any mode is this menu (its other modes are the steps between the lists); only 1, 3 and 4 read.
  bool active() override { return mode() >= 1; }
  bool holds_keys() override { int md = mode(); return md == 1 || md == 3 || md == 4; }
  std::string title() override { return strings::kCampItems[4]; }
  void on_enter() override {
    screens::set_keys([this] {
      int md = mode();
      return std::string(md == 1 ? strings::kStatsKeys() : md == 3 ? strings::kTacticsListKeys() : strings::kListKeys());
    });
  }
  // A member on the party list: their line, their Persona, then its stats, skills with costs and affinities.
  int focused_member() {
    int i = mode() == 1 ? focused_row("stats") : -1;
    uint8_t* u = ui();
    auto* ids = u && i >= 0 ? arr<uint16_t>(u, 0x28) : nullptr;
    return ids && i < ids->num ? ids->data[i] : -1;
  }
  bool has_details() override { return focused_member() >= 0; }
  std::vector<std::string> detail_lines() override {
    int id = focused_member();
    if (id < 0) return {};
    std::vector<std::string> out{member_line(id)};
    if (const uint8_t* e = party::persona_entry(id)) {
      if (int pid = *(const uint16_t*)(e + 2)) out.push_back(persona::describe(pid, *(const uint16_t*)(e + 4)));
      for (auto& l : persona::detail_lines(e)) out.push_back(l);
    }
    return out;
  }
  void on_tick() override {
    if (mode() != 4) return;
    // The order list is drawn on the right (x ~1204, from y 310): its names, top to bottom.
    std::vector<textcap::Item> rows;
    for (auto& t : textcap::snapshot(100))
      if (t.source == 2 && t.x > 1100 && t.y >= 290 && core::has_words(t.text)) rows.push_back(t);
    std::sort(rows.begin(), rows.end(), [](auto& a, auto& b) { return a.y < b.y; });
    std::vector<std::string> names;
    for (auto& r : rows) {
      std::string n = core::clean_text(r.text);
      if (std::find(names.begin(), names.end(), n) == names.end()) names.push_back(n);
    }
    if (names.size() >= 2) orders_ = names;
  }
  void build(ui::GraphBuilder& b) override {
    uint8_t* u = ui();
    if (!u || !holds_keys()) return;
    int md = mode();
    auto* ids = arr<uint16_t>(u, 0x28);
    if (md == 4) {
      ListCursor lc = cursor_at(scene(), 0xA0);
      b.begin_stop("stats.order");
      b.push_context(strings::kTacticsWhich);
      for (int i = 0; i < lc.count(); ++i)
        b.add_item(slot("stats.order", i), game_item(order_, i, [this, i] {
          std::string n = tactic_name(i);
          return n.empty() ? std::format("{} {}", strings::kTactic, i + 1) : n;
        }));
      b.pop_context();
      if (int c = order_.cursor(); c >= 0) b.set_start(slot("stats.order", c));
      return;
    }
    // The party list, or (E) the same members with their tactics.
    std::string key = md == 3 ? "stats.tactics" : "stats";
    list_.key = key;
    b.begin_stop(key);
    if (md == 3) b.push_context(strings::kTacticsHeading);
    for (int i = 0; ids && i < ids->num; ++i) {
      int id = ids->data[i];
      if (md == 3) {
        b.add_item(slot(key, i), game_item(list_, i, [this, id] {
          std::string t = tactic_name(tactic(id));
          return t.empty() ? party::name(id) : std::format("{}: {}", party::name(id), t);
        }));
      } else {
        auto persona_line = [id] {
          const uint8_t* e = party::persona_entry(id);
          return e ? persona::status_line(e) : std::string();
        };
        // The row names the Persona; its stats, skills and affinities are the detail buffer's lines (detail_lines), and
        // the review key reads them all at once. They were one 40-word line on every move (the user's call, 2026-11-11).
        auto v = game_item(list_, i, [id] { return member_line(id); }, persona_line);
        describe_with(v, [id] {
          const uint8_t* e = party::persona_entry(id);
          int pid = e ? *(const uint16_t*)(e + 2) : 0;
          return pid ? persona::describe(pid, *(const uint16_t*)(e + 4)) : std::string();
        });
        b.add_item(slot(key, i), v);
      }
    }
    if (md == 3) b.pop_context();
    if (int c = list_.cursor(); c >= 0) b.set_start(slot(key, c));
  }
};

// ---------------------------------------------------------------- Stats: a member's page
// The scene CmpCharacterStatus (or CmpHeroHumanStatus) on top of the Stats UI's scene stack; the member is the party
// list's (UUICmpStatus +0x28 ids, ListCursor +0x68, which RB / LB, Q / E, move while the page is up). A page of lines:
// the member, their Persona's stats and each skill with its description, its affinities; the hero's social stats. In
// sections (menu.h): the member, their Persona, Stats one per line, Skills, Affinities; Tab / RT and LT jump them.
class CampStatsPage : public MenuScreen, private CampStatsUi {
  int member() {
    uint8_t* u = ui();
    auto* ids = u ? arr<uint16_t>(u, 0x28) : nullptr;
    int c = u ? ListCursor(u + 0x68).index() : -1;
    return ids && c >= 0 && c < ids->num ? ids->data[c] : 0;
  }
  int shown_ = 0;
 public:
  const char* id() const override { return "camp_stats_page"; }
  int priority() const override { return 17; }
  bool active() override {
    UObject* sc = scene();
    return sc && (is_a(sc, "CmpCharacterStatus") || is_a(sc, "CmpHeroHumanStatus"));
  }
  std::string title() override { return strings::kCampItems[4]; }
  void on_enter() override { screens::set_keys(strings::kStatsPageKeys); }
  bool game_triggers() override { return true; }
  void on_tick() override {
    int who = member();
    if (who && shown_ && who != shown_) focus(ui::ControlId::structural(std::format("member.{}.line", who)));   // Q / E
    if (who) shown_ = who;
  }
  void build(ui::GraphBuilder& b) override {
    int who = member();
    if (!who) return;
    std::string k = std::format("member.{}", who);
    b.begin_stop(k);
    b.set_region(k + ".member");
    b.add_item(ui::ControlId::structural(k + ".line"), item([who] { return member_line(who); }));
    if (who == 1) b.add_item(ui::ControlId::structural(k + ".social"), item([] { return stats::describe(); }));
    // A Persona: its name with the member's lines, then its sections: Stats (one per line), Skills, Affinities.
    auto persona_sections = [&](int pid, int lv, const uint8_t* st, auto skill_at) {
      b.add_item(ui::ControlId::structural(k + ".persona"), item([pid, lv] { return persona::describe(pid, lv); }));
      b.set_region(k + ".stats");
      b.push_context(strings::kStatsTitle);
      for (int i = 0; i < 5; ++i) {
        std::string l = std::format("{} {}", strings::kStatNames[i], st[i]);
        b.add_item(ui::ControlId::structural(std::format("{}.stat{}", k, i)), item([l] { return l; }));
      }
      b.pop_context();
      b.set_region(k + ".skills");
      b.push_context(strings::kSkills);
      for (int i = 0; i < 8; ++i) if (int sid = skill_at(i)) {
        std::string sl = persona::skill_name(sid), cost = persona::skill_cost(sid);
        if (!cost.empty()) sl += ", " + cost;
        auto v = item([sl] { return sl; }, {}, [sid] { return persona::skill_help(sid); });
        buffer_with(v, [sid] { return persona::skill_help(sid); });
        b.add_item(ui::ControlId::structural(std::format("{}.skill{}", k, i)), v);
      }
      b.pop_context();
      if (auto groups = persona::affinity_groups(pid); !groups.empty()) {
        b.set_region(k + ".aff");
        b.push_context(strings::kAffinitiesTitle);
        for (size_t i = 0; i < groups.size(); ++i) {
          std::string g = groups[i];
          g[0] = (char)toupper((unsigned char)g[0]);
          b.add_item(ui::ControlId::structural(std::format("{}.aff{}", k, i)), item([g] { return g; }));
        }
        b.pop_context();
      }
    };
    const uint8_t* e = party::persona_entry(who);
    if (who == 1) {   // the hero's equipped Persona from the stock
      int eq = persona::equipped();
      for (auto& o : persona::stock()) if (o.slot == eq) persona_sections(o.id, o.level, o.stats, [&o](int i) { return (int)o.skills[i]; });
    } else if (e && *(uint16_t*)(e + 2)) {
      persona_sections(*(uint16_t*)(e + 2), *(uint16_t*)(e + 4), e + 0x1C, [e](int i) { return (int)*(uint16_t*)(e + 0xC + i * 2); });
    }
  }
};

std::unique_ptr<Screen> make_camp_stats() { return std::make_unique<CampStats>(); }
std::unique_ptr<Screen> make_camp_stats_page() { return std::make_unique<CampStatsPage>(); }
}  // namespace p3a::screens

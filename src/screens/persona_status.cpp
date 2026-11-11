// The Persona status page outside battle (F on the camp Persona list): APersonaStatus stays after it closes;
// +0x528 is 1 while it is shown (2 once closed; measured 2026-10-28). The page is sprites: it is read from the entry
// it shows (persona::shown_status) with the game's affinity table. Q and E switch Personas.
#include "screens/menu.h"
#include "bmdfile.h"
#include "core/text_clean.h"
#include "msg.h"
#include "screens/util.h"
#include "core/strings.h"
#include "persona.h"
#include <format>

namespace p3a::screens {
using namespace ue;

// A Persona status page as lines to move through (one stop, "ps.<id>"), in sections (menu.h: the arrows walk every
// line top to bottom, Tab / Shift+Tab or a trigger press jump to a section and say its title): the name (after `lead`,
// "Result") with the `bonus` line if any and the EXP to the next level; the stats, one per line; each skill (its cost;
// its description after the position and on the review key); the skills it learns later; its affinities, one entry
// per kind.
// `before` (a level up): each stat says what it gained, each skill it did not have says "new".
// `name_item` (the camp's list, a game_item at `name_id`) replaces the name line.
void build_status_page(ui::GraphBuilder& b, const uint8_t* e, const std::string& lead, const std::string& bonus,
                       const uint8_t* before, const ui::ControlId* name_id, ui::NodeVtablePtr name_item) {
  using M = MenuScreen;
  int id = e ? *(uint16_t*)(e + 2) : 0;
  if (!id || persona::name(id).empty()) return;
  std::string k = std::format("ps.{}", id);
  int level = *(uint16_t*)(e + 4);
  b.begin_stop(k);
  b.set_region(k + ".top");
  std::string name = persona::describe(id, level);
  if (!lead.empty()) name = lead + (lead.back() == '!' || lead.back() == '?' ? " " : ": ") + name;
  if (name_id && name_item) b.add_item(*name_id, name_item);
  else b.add_item(ui::ControlId::structural(k + ".name"), M::item([name] { return name; }));
  if (!bonus.empty()) b.add_item(ui::ControlId::structural(k + ".bonus"), M::item([bonus] { return bonus; }));
  if (int next = persona::exp_to_next(e); next >= 0) {
    std::string l = std::format("{} {}", next, strings::kExpToNext);
    b.add_item(ui::ControlId::structural(k + ".exp"), M::item([l] { return l; }));
  }
  b.set_region(k + ".stats");
  b.push_context(strings::kStatsTitle);
  for (int i = 0; i < 5; ++i) {
    int v = persona::stat_total(e, i), up = before ? v - persona::stat_total(before, i) : 0;
    std::string l = std::format("{} {}", strings::kStatNames[i], v);
    if (up > 0) l += std::format(", {} {}", strings::kUp, up);
    b.add_item(ui::ControlId::structural(std::format("{}.stat{}", k, i)), M::item([l] { return l; }));
  }
  b.pop_context();
  b.set_region(k + ".skills");
  b.push_context(strings::kSkills);
  for (int i = 0; i < 8; ++i) {
    int sid = *(uint16_t*)(e + 0xC + i * 2);
    if (!sid) continue;
    std::string l = persona::skill_name(sid), cost = persona::skill_cost(sid);
    if (!cost.empty()) l += ", " + cost;
    bool had = !before;
    for (int j = 0; before && j < 8; ++j) had |= *(uint16_t*)(before + 0xC + j * 2) == sid;
    if (!had) l += std::string(", ") + strings::kNew;
    auto v = M::item([l] { return l; }, {}, [sid] { return persona::skill_help(sid); });
    // A skill just learned says what it does; one already known keeps that for the detail buffer.
    if (!had) M::describe_with(v, [sid] { return persona::skill_help(sid); });
    else M::buffer_with(v, [sid] { return persona::skill_help(sid); });
    b.add_item(ui::ControlId::structural(std::format("{}.skill{}", k, i)), v);
  }
  b.pop_context();
  // What it learns by levelling, past its level (the game shows these only as they are learned).
  std::vector<persona::Learn> later;
  for (auto& l : persona::learnset(id)) if (l.level > level) later.push_back(l);
  if (!later.empty()) {
    b.set_region(k + ".learns");
    b.push_context(strings::kLearnsTitle);
    for (auto& l : later) {
      std::string t = std::format("{}, {} {}", persona::skill_name(l.skill), strings::kLearnsAt, l.level);
      int sid = l.skill;
      auto v = M::item([t] { return t; }, {}, [sid] { return persona::skill_help(sid); });
      M::buffer_with(v, [sid] { return persona::skill_help(sid); });
      b.add_item(ui::ControlId::structural(std::format("{}.learn{}", k, sid)), v);
    }
    b.pop_context();
  }
  b.set_region(k + ".aff");
  b.push_context(strings::kAffinitiesTitle);
  auto groups = persona::affinity_groups(id);
  if (groups.empty()) groups.push_back(strings::kNoAffinities);
  for (size_t i = 0; i < groups.size(); ++i) {
    std::string g = groups[i];
    g[0] = (char)toupper((unsigned char)g[0]);
    b.add_item(ui::ControlId::structural(std::format("{}.aff{}", k, i)), M::item([g] { return g; }));
  }
  b.pop_context();
  b.set_region({});
}

// A bespoke page (menu.h, build_status_page): the Persona's name, each stat, each skill, what it learns later and its
// affinities, as lines to move through. Q and E (the game's) switch the Persona;
// Enter and the rest reach the game. Up wherever the plain page is shown: the camp, and in battle (GUI state 12, F on a skill list; a second instance). Skill
// learning has its own screen.
class PersonaStatusScreen : public MenuScreen {
  Finder ps_{"PersonaStatus"};
  Finder gui_{"BtlGuiStateManager"};
  bool battle_;   // the battle's own (GUI state 12: F from a skill list)
  int shown_ = 0;
 public:
  explicit PersonaStatusScreen(bool battle) : battle_(battle) {}
  const char* id() const override { return battle_ ? "battle_persona" : "persona_status"; }
  // Over the camp submenus and the Velvet Room; in battle, over its lists.
  int priority() const override { return battle_ ? 26 : 20; }
  bool active() override {
    if (battle_) { UObject* g = gui_.get(); return g && get<uint8_t>(g, "nowState") == 12 && persona::shown_status(); }
    return persona::status_page_up(ps_.get());
  }
  std::string title() override { return strings::kPersonaStatus; }
  void on_enter() override { screens::set_keys(battle_ ? strings::kPersonaStatusKeys : strings::kCampPersonaStatusKeys); shown_ = 0; }
  void on_tick() override {
    const uint8_t* e = persona::shown_status();
    int id = e ? *(uint16_t*)(e + 2) : 0;
    if (id && shown_ && id != shown_) focus(ui::ControlId::structural(std::format("ps.{}.name", id)));   // Q / E
    if (id) shown_ = id;
  }
  void build(ui::GraphBuilder& b) override { build_status_page(b, persona::shown_status(), {}, {}, nullptr, nullptr, {}); }
};

std::unique_ptr<Screen> make_persona_status() { return std::make_unique<PersonaStatusScreen>(false); }
std::unique_ptr<Screen> make_battle_persona() { return std::make_unique<PersonaStatusScreen>(true); }
}  // namespace p3a::screens

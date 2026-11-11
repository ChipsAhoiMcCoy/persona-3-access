#include "screens/menu.h"

namespace p3a::screens {
// Every screen the mod models, each by the name of its factory (make_<name>, defined in the screen's own file), in the
// order they are added. A new screen is one name here.
#define P3A_SCREENS(X) \
  X(title) \
  X(title_menu) \
  X(difficulty) \
  X(umg_dialog) \
  X(name_entry) \
  X(save_load) \
  X(camp) \
  X(get_item) \
  X(movie) \
  X(credits) \
  X(town_map) \
  X(stats_up) \
  X(battle) \
  X(battle_survey) \
  X(battle_skills) \
  X(battle_items) \
  X(battle_targets) \
  X(battle_theurgy) \
  X(battle_shift) \
  X(battle_tactics) \
  X(rank_up) \
  X(battle_result) \
  X(camp_item) \
  X(camp_skill) \
  X(camp_equip) \
  X(camp_persona) \
  X(camp_stats) \
  X(camp_social) \
  X(camp_quest) \
  X(camp_system) \
  X(camp_calendar) \
  X(camp_social_details) \
  X(theurgy_menu) \
  X(camp_stats_page) \
  X(shuffle) \
  X(shop) \
  X(tutorial_list) \
  X(velvet) \
  X(fast_travel) \
  X(mail) \
  X(area_map) \
  X(config) \
  X(dictionary) \
  X(backlog) \
  X(choice) \
  X(generic_select) \
  X(terminal) \
  X(persona_status) \
  X(battle_persona) \
  X(skill_learn)

#define P3A_DECLARE(name) std::unique_ptr<Screen> make_##name();
P3A_SCREENS(P3A_DECLARE)
#undef P3A_DECLARE

void register_all() {
  menu::install();
#define P3A_ADD(name) add(make_##name());
  P3A_SCREENS(P3A_ADD)
#undef P3A_ADD
}
#undef P3A_SCREENS
}  // namespace p3a::screens

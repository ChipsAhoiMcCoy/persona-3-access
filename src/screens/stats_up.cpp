// The "Stats up! Social stats" screen (Space for OK): AUIGetHeroParameterDraw, made for it and destroyed once it has
// closed, its panel a UCmpHeroHumanStatusDraw (+0xD00 pHumanParamDraw_). Its state +0x294 (previous +0x298, the UI
// actors' common task state) is 6 while it is up and takes OK, then 7, 8 and 9 as it closes (measured 2026-11-03,
// studying at the desk). The stats and their rank names are the save data's (stats::read).
#include "screens/screen.h"
#include "screens/util.h"
#include "core/strings.h"
#include "speech.h"
#include "stats.h"
#include <format>

namespace p3a::screens {
using namespace ue;

class StatsUpScreen : public Screen {
  Finder draw_{"UIGetHeroParameterDraw"};
  // The panel as a line: each social stat with its level and rank name. `rise` names the stat that went up first (the
  // last rise no panel has shown).
  std::string line(bool rise) {
    stats::Stats s;
    if (!stats::read(s)) return {};
    std::string out = strings::kStatsUp;
    if (int up = rise ? stats::last_up() : -1; up >= 0) out += std::format(" {} {}.", strings::kSocialStats[up], strings::kStatUp);
    for (int i = 0; i < 3; ++i) out += std::format(" {} {} {}.", strings::kSocialStats[i], s.level[i], stats::rank_name(s, i));
    return out;
  }
 public:
  const char* id() const override { return "stats_up"; }
  int priority() const override { return 20; }
  bool active() override {
    uint8_t* d = (uint8_t*)draw_.get();
    if (!d || *(int32_t*)(d + 0x294) != 6 || !*(UObject**)(d + 0xD00)) return false;
    stats::Stats s;
    return stats::read(s);
  }
  // The stat that went up was said as its points changed, 2.4 s before this screen (2026-10-29): the review line
  // names it, the entry does not again. The key that closes the panel is the screen's key help (F1), not part of
  // either line.
  void enter() override { screens::set_keys(strings::kSpaceContinues); speech::speak(line(false), true); }
  void leave() override { if (!active()) stats::up_shown(); }   // closed (not covered): its rise has been shown
  std::string describe() override { return line(true); }
};

std::unique_ptr<Screen> make_stats_up() { return std::make_unique<StatsUpScreen>(); }
}  // namespace p3a::screens

// Story movies (BP_MoviePlayer_C, CRI Mana). Enter pauses and resumes a movie and Tab skips it; a paused movie is
// silent and looks like a hang, so the pause is announced. Subtitles are BMD pages: msg speaks them.
#include "screens/screen.h"
#include "screens/util.h"
#include "core/strings.h"
#include "speech.h"
#include <format>

namespace p3a::screens {
using namespace ue;

class MovieScreen : public Screen {
  Finder player_{"BP_MoviePlayer_C"};
  int paused_ = -1;
  bool ask(UObject* mana, const char* fn) { return call_ret<bool>(mana, fn); }
  UObject* mana() {
    UObject* m = player_.get();
    return m ? get<UObject*>(m, "Mana") : nullptr;
  }
 public:
  const char* id() const override { return "movie"; }
  int priority() const override { return 5; }
  bool active() override {
    UObject* m = mana();
    return m && ask(m, "IsPlaying");
  }
  void enter() override {
    paused_ = -1;
    screens::set_keys([] { return std::string(title_opening() ? strings::kOpeningKeys() : strings::kMovieKeys()); });
  }
  void tick() override {
    UObject* m = mana();
    if (!m) return;
    if (opening_line_due()) speech::speak(strings::kTitleOpening, true);
    int p = ask(m, "IsPaused") ? 1 : 0;
    if (paused_ >= 0 && p != paused_) speech::speak(p ? strings::kMoviePaused() : strings::kMovieResumed, true);
    paused_ = p;
  }
  // The opening movie is skipped by a press, not Tab, and Enter skips rather than pauses it (seen live, 2026-10-29).
  std::string describe() override {
    if (paused_ != 1 && title_opening()) return strings::kTitleOpening;
    return paused_ == 1 ? strings::kMoviePaused() : strings::kMoviePlaying();
  }
};

std::unique_ptr<Screen> make_movie() { return std::make_unique<MovieScreen>(); }
}  // namespace p3a::screens

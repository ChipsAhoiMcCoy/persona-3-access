// The ending's staff roll (AStaffRoll: its own draw actor and font over a movie, from the dump; no input). It is
// announced so the silence is not taken for a hang.
// UNTESTED: written from the dump while the game was in use; verify at the ending (slot 15, after Nyx).
#include "screens/screen.h"
#include "screens/util.h"
#include "core/strings.h"
#include "speech.h"

namespace p3a::screens {
using namespace ue;

class CreditsScreen : public Screen {
  Finder roll_{"StaffRoll"};
 public:
  const char* id() const override { return "credits"; }
  int priority() const override { return 6; }   // over the movie it plays on
  bool active() override {
    UObject* r = roll_.get();
    return r && get<UObject*>(r, "pStaffRollDraw") != nullptr;
  }
  void enter() override { speech::speak(strings::kCredits, true); }
  std::string describe() override { return strings::kCredits; }
};

std::unique_ptr<Screen> make_credits() { return std::make_unique<CreditsScreen>(); }
}  // namespace p3a::screens

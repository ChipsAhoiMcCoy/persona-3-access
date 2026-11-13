// The ending's staff roll (AStaffRoll: its own draw actor and font over a movie; no input). It is announced so the
// silence is not taken for a hang (seen at the ending, 2026-11-11: "Credits" as the roll began). The review key reads
// what is rolling past: the roll's rows are UStaffRollDataAsset.Data (AStaffRoll.pStaffRollDataList; up to four
// name columns a row), and the draw (pStaffRollDraw) keeps the first row not yet rolled off at +0x3A730 (it went
// 5, then 10 ten seconds on, of 750). The whole roll is never read unasked: hundreds of names over a song.
#include "screens/screen.h"
#include "screens/util.h"
#include "core/strings.h"
#include "seh.h"
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
  std::string describe() override {
    std::string out = strings::kCredits;
    UObject* r = roll_.get();
    UObject* draw = r ? get<UObject*>(r, "pStaffRollDraw") : nullptr;
    UObject* list = r ? get<UObject*>(r, "pStaffRollDataList") : nullptr;
    if (!draw || !list || !ue::known_build()) return out;
    TableRows rows = table_rows(list);
    static const char* const kColumns[] = {"FirstColumnName", "SecondColumnName", "ThirdColumnName", "ForthColumnName"};
    guarded([&] {
      int at = *(int32_t*)((uint8_t*)draw + 0x3A730);
      if (!rows || at < 0) return;
      // A screenful from the first row still rolling.
      for (int i = at; i < rows.num && i < at + 8; ++i)
        for (const char* col : kColumns) {
          int off = offset_of(rows.row, col);
          if (off < 0) continue;
          std::string t = to_utf8(*(FString*)(rows.at(i) + off));
          if (!t.empty()) out += (out == strings::kCredits ? ". " : ", ") + t;
        }
    });
    return out;
  }
};

std::unique_ptr<Screen> make_credits() { return std::make_unique<CreditsScreen>(); }
}  // namespace p3a::screens

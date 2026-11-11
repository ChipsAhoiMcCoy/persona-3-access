#include "doctest.h"
#include "core/speech_text.h"
#include <string>
#include <vector>

TEST_CASE("position puts a line's place after it") {
  using p3a::core::position;
  CHECK(position("Yes", 1, 2) == "Yes, 1 of 2");
  CHECK(position("Only one", 1, 1) == "Only one, 1 of 1");
  CHECK(position("A page.", 3, 12) == "A page., 3 of 12");   // the stop is the caller's to take off, or tidy_joins'
  CHECK(position("", 2, 5) == ", 2 of 5");
  CHECK(position("Caf\xC3\xA9", 2, 3) == "Caf\xC3\xA9, 2 of 3");
}
TEST_CASE("spoken_line names the speaker when there is one") {
  using p3a::core::spoken_line;
  CHECK(spoken_line("Yukari", "Hello.") == "Yukari: Hello.");
  CHECK(spoken_line("", "The door is locked.") == "The door is locked.");
  CHECK(spoken_line("Thinking", "") == "Thinking: ");
  CHECK(spoken_line("", "") == "");
  CHECK(spoken_line("\xC3\x89lisabeth", "\xE2\x80\xA6Oh.") == "\xC3\x89lisabeth: \xE2\x80\xA6Oh.");
}
TEST_CASE("without_own_stop drops one full stop and keeps an ellipsis") {
  using p3a::core::without_own_stop;
  CHECK(without_own_stop("Nice to meet you.") == "Nice to meet you");
  CHECK(without_own_stop("Nice to meet you") == "Nice to meet you");
  CHECK(without_own_stop("...") == "...");
  CHECK(without_own_stop("Well...") == "Well...");
  CHECK(without_own_stop("Really?") == "Really?");
  CHECK(without_own_stop(".") == ".");
  CHECK(without_own_stop("") == "");
  CHECK(without_own_stop("Caf\xC3\xA9.") == "Caf\xC3\xA9");
  CHECK(without_own_stop("Hm\xE2\x80\xA6") == "Hm\xE2\x80\xA6");
}
TEST_CASE("all_filled wants entries, each with text") {
  using p3a::core::all_filled;
  CHECK(all_filled({"Yes", "No"}));
  CHECK(all_filled({"Yes"}));
  CHECK_FALSE(all_filled({}));
  CHECK_FALSE(all_filled({"Yes", ""}));
  CHECK_FALSE(all_filled({""}));
  CHECK(all_filled({"\xE3\x81\xAF\xE3\x81\x84", "..."}));
}
TEST_CASE("no_digits blanks each run of digits") {
  using p3a::core::no_digits;
  CHECK(no_digits("Settings will revert in 9 seconds.") == "Settings will revert in # seconds.");
  CHECK(no_digits("Settings will revert in 10 seconds.") == no_digits("Settings will revert in 9 seconds."));
  CHECK(no_digits("1,000 yen") == "#,# yen");
  CHECK(no_digits("12") == "#");
  CHECK(no_digits("No numbers") == "No numbers");
  CHECK(no_digits("") == "");
  CHECK(no_digits("a#1") == "a#");   // a '#' of the text's own takes the digits after it
  CHECK(no_digits("\xC2\xA5" "500") == "\xC2\xA5#");
}
TEST_CASE("sentence ends a line with a stop") {
  using p3a::core::sentence;
  CHECK(sentence("Fortune") == "Fortune.");
  CHECK(sentence("Fortune.") == "Fortune.");
  CHECK(sentence("Who should I think of...?") == "Who should I think of...?");
  CHECK(sentence("Go!") == "Go!");
  CHECK(sentence("") == "");
  CHECK(sentence("Caf\xC3\xA9") == "Caf\xC3\xA9.");
  CHECK(sentence("Hm\xE2\x80\xA6") == "Hm\xE2\x80\xA6.");
}

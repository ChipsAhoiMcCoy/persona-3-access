#include "doctest.h"
#include "core/text_clean.h"
using p3a::core::clean_text;

TEST_CASE("clean_text joins lines and drops bullets") {
  CHECK(clean_text("\xE2\x80\xA2 A device was\ndisconnected.\n\nPressing any button") == "A device was disconnected. Pressing any button");
  CHECK(clean_text("  hello   world  ") == "hello world");
  CHECK(clean_text("a\x01" "b") == "ab");
  CHECK(clean_text("") == "");
  CHECK(clean_text("@") == "");
  CHECK(clean_text("Hey @ you") == "Hey you");
}
TEST_CASE("clean_text keeps closing punctuation on its word") {
  CHECK(clean_text("Thanks @.") == "Thanks.");
  CHECK(clean_text("Thanks\n.") == "Thanks.");
  CHECK(clean_text("Wait ... what ?") == "Wait... what?");
  CHECK(clean_text("Yukari: ...Hm? The list.") == "Yukari: ...Hm? The list.");
  CHECK(clean_text("I see @...") == "I see...");
  CHECK(clean_text("He said \"no\" .") == "He said \"no\".");
}
TEST_CASE("has_words") {
  CHECK(p3a::core::has_words("...a"));
  CHECK_FALSE(p3a::core::has_words("... !"));
}
TEST_CASE("first_sentence") {
  CHECK(p3a::core::first_sentence("Low-stress combat. Your choice...") == "Low-stress combat.");
  CHECK(p3a::core::first_sentence("No stop") == "No stop");
  CHECK(p3a::core::first_sentence("Wait... really? Yes.") == "Wait... really?");
}
TEST_CASE("tidy_joins drops a comma or period after a sentence's end") {
  using p3a::core::tidy_joins;
  CHECK(tidy_joins("Then choose a party member., 3 of 4") == "Then choose a party member. 3 of 4");
  CHECK(tidy_joins("2 choices: Kinda., How so?") == "2 choices: Kinda. How so?");
  CHECK(tidy_joins("Running late?, Not feeling well?") == "Running late? Not feeling well?");
  CHECK(tidy_joins("1 ally's HP.. Slightly") == "1 ally's HP. Slightly");
  CHECK(tidy_joins("Junpei Iori. yo!. y'know") == "Junpei Iori. yo! y'know");
  CHECK(tidy_joins("Fine..., Okay.") == "Fine... Okay.");
  CHECK(tidy_joins("member. , 3 of 4") == "member. 3 of 4");
  CHECK(tidy_joins("member. . 3 of 4") == "member. 3 of 4");
  CHECK(tidy_joins("Hm\xE2\x80\xA6. Next") == "Hm\xE2\x80\xA6 Next");
}
TEST_CASE("tidy_joins leaves ellipses, numbers and plain commas") {
  using p3a::core::tidy_joins;
  CHECK(tidy_joins("Thinking: ......") == "Thinking: ......");
  CHECK(tidy_joins("Yukari: ...Hm? The list.") == "Yukari: ...Hm? The list.");
  CHECK(tidy_joins("Wait.... what") == "Wait.... what");
  CHECK(tidy_joins("Dia, 3 SP, 1 of 1. Restores 1,000 HP.") == "Dia, 3 SP, 1 of 1. Restores 1,000 HP.");
  CHECK(tidy_joins("Ver. 1.5, ok") == "Ver. 1.5, ok");
}

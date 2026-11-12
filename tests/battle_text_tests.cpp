#include "doctest.h"
#include "core/battle_text.h"
#include "core/strings.h"
#include <string>
#include <vector>

using namespace p3a::core;
namespace strings = p3a::strings;

TEST_CASE("battle_text: health is a share rounded up") {
  CHECK(health_share(100, 100) == 100);
  CHECK(health_share(50, 100) == 50);
  CHECK(health_share(1, 1000) == 1);     // never 0% while alive
  CHECK(health_share(0, 1000) == 0);
  CHECK(health_share(999, 1000) == 100);
  CHECK(health_share(1, 3) == 34);
}

TEST_CASE("battle_text: turns in the singular and the plural") {
  CHECK(turns_text(1) == "1 turn");
  CHECK(turns_text(3) == "3 turns");
  CHECK(turns_text(0) == "0 turns");
}

TEST_CASE("battle_text: a countdown's time in words") {
  CHECK(countdown_text(1800.0f, true) == "30 minutes");
  CHECK(countdown_text(1752.7f, true) == "29 minutes 12 seconds");
  CHECK(countdown_text(1752.7f, false) == "29 minutes");
  CHECK(countdown_text(61.0f, true) == "1 minute 1 second");
  CHECK(countdown_text(59.9f, false) == "59 seconds");
  CHECK(countdown_text(0.4f, true) == "0 seconds");
  CHECK(countdown_text(-3.0f, true) == "0 seconds");
}

TEST_CASE("battle_text: the marks a countdown passes") {
  CHECK(countdown_mark(1800.0f, 1799.9f) == -1);   // the start is not a mark passed
  CHECK(countdown_mark(1500.1f, 1499.9f) == 1500);
  CHECK(countdown_mark(1500.0f, 1499.9f) == -1);   // said when it reached 1500
  CHECK(countdown_mark(300.05f, 300.0f) == 300);
  CHECK(countdown_mark(60.01f, 59.99f) == 60);
  CHECK(countdown_mark(30.01f, 29.99f) == 30);
  CHECK(countdown_mark(10.01f, 9.99f) == 10);
  CHECK(countdown_mark(45.0f, 44.0f) == -1);
  CHECK(countdown_mark(700.0f, 50.0f) == 60);      // several at once (600, 300, 60): the lowest
  CHECK(countdown_mark(40.0f, 41.0f) == -1);       // going up is no mark
}

TEST_CASE("battle_text: an attack's name loses its last word") {
  CHECK(short_attack("Fire Attack") == "Fire");
  CHECK(short_attack("Almighty Attack") == "Almighty");
  CHECK(short_attack("Fire") == "Fire");
  CHECK(short_attack(" Attack") == " Attack");   // nothing would be left
  CHECK(short_attack("") == "");
}

TEST_CASE("battle_text: enemies that fall together are one line") {
  CHECK(defeated_line({"Cowardly Maya"}) == "Cowardly Maya defeated");
  CHECK(defeated_line({"Cowardly Maya", "Cowardly Maya"}) == "2 Cowardly Maya defeated");
  CHECK(defeated_line({"Cowardly Maya", "Muttering Tiara", "Cowardly Maya"}) == "2 Cowardly Maya, Muttering Tiara defeated");
  CHECK(defeated_line({"Muttering Tiara", "Cowardly Maya"}) == "Muttering Tiara, Cowardly Maya defeated");
}

TEST_CASE("battle_text: the wheel's target keys") {
  CHECK(target_key_step(1, 0x25) == -1);   // Left
  CHECK(target_key_step(1, 'A') == -1);
  CHECK(target_key_step(1, 0x27) == 1);    // Right
  CHECK(target_key_step(1, 'D') == 1);
  CHECK(target_key_step(1, 'Q') == 0);
  CHECK(target_key_step(1, 0x26) == 0);    // Up
}

TEST_CASE("battle_text: Analyze's target keys go the other way") {
  CHECK(target_key_step(8, 'E') == -1);
  CHECK(target_key_step(8, 'D') == -1);
  CHECK(target_key_step(8, 'Q') == 1);
  CHECK(target_key_step(8, 'A') == 1);
  CHECK(target_key_step(8, 0x25) == 0);
  CHECK(target_key_step(2, 'A') == 0);     // no target keys in a list
  CHECK(target_key_step(0, 0x27) == 0);
}

TEST_CASE("battle_text: the skills a level up brought") {
  uint16_t was[8] = {10, 11, 12, 0, 0, 0, 0, 0};
  uint16_t now[8] = {10, 11, 12, 40, 0, 0, 0, 0};
  CHECK(new_skills(now, was) == std::vector<int>{40});
  uint16_t swapped[8] = {41, 11, 12, 10, 42, 0, 0, 0};   // a slot replaced, the list reordered
  CHECK(new_skills(swapped, was) == std::vector<int>{41, 42});
  CHECK(new_skills(was, was).empty());
  CHECK(new_skills(nullptr, was).empty());
}

TEST_CASE("battle_text: a Major Arcana card's face is its arcana") {
  CHECK(card_face(2, 0, 27) == "Fool");
  CHECK(card_face(2, 0, 28) == "Magician");
  CHECK(card_face(2, 0, 39) == "Hanged Man");
  CHECK(card_face(2, 0, 92) == "Temperance");
  CHECK(card_face(2, 0, 98) == "Judgement");
  CHECK(card_face(2, 0, 40) == "");    // not a card's page
  CHECK(card_face(2, 0, 99) == "");
}

TEST_CASE("battle_text: a suit card's face is its suit and number") {
  CHECK(card_face(3, 0, 4) == "Swords 1");
  CHECK(card_face(3, 4, 5) == "Coins 5");
  CHECK(card_face(3, 2, 6) == "Cups 3");
  CHECK(card_face(3, 9, 25) == "Cups 10");
  CHECK(card_face(3, 0, 26) == "Wands 1");
  CHECK(card_face(3, 0, 27) == "");    // a Major Arcana's page under a suit card
  CHECK(card_face(1, 3, 6) == "");     // a Persona card: its page names it
}

TEST_CASE("battle_text: a shop's menu entries") {
  CHECK(shop_mode_name(1) == "Buy Weapon");
  CHECK(shop_mode_name(4) == "Buy Accessory");
  CHECK(shop_mode_name(0xC) == "Sell Item");
  CHECK(shop_mode_name(7) == "Fuse Equipment");
  CHECK(shop_mode_name(9) == "Trade Card");
  CHECK(shop_mode_name(10) == "Trade Item");
  CHECK(shop_mode_name(5) == "Option 5");
}

TEST_CASE("battle_text: why a row cannot be bought") {
  CHECK(std::string(cannot_buy_reason(0, 3, 100, 5000)) == strings::kOutOfStock);     // the stock comes first
  CHECK(std::string(cannot_buy_reason(0, 99, 100, 0)) == strings::kOutOfStock);
  CHECK(std::string(cannot_buy_reason(99, 99, 100, 5000)) == strings::kCarryingMax);
  CHECK(std::string(cannot_buy_reason(99, 3, 100, 50)) == strings::kNotEnoughYen);
  CHECK(std::string(cannot_buy_reason(99, 3, 100, 100)) == strings::kCannotBuy);      // affordable: some other reason
  CHECK(std::string(cannot_buy_reason(99, 3, 100, -1)) == strings::kCannotBuy);       // money not known
  CHECK(std::string(cannot_buy_reason(99, -1, 0, 0)) == strings::kCannotBuy);         // free, owned count not known
}

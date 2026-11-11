#pragma once
// Wording and small rules of the battle screens, Shuffle Time and the shops that need nothing from the game: the
// screens read the numbers, these say them.
#include <cstdint>
#include <string>
#include <vector>

namespace p3a::core {
// Health as a share of the maximum, rounded up (1 HP left is never 0%). `max` is positive.
constexpr int health_share(int hp, int max) { return (hp * 100 + max - 1) / max; }
// "1 turn" / "3 turns".
std::string turns_text(int n);
// The game's attack names are "Fire Attack", "Ice Attack": the word is dropped ("weak to Fire, Ice").
std::string short_attack(std::string name);
// Enemies that fell together, in one line: "2 Cowardly Maya, Muttering Tiara defeated". The names in the order they
// first fell; one that fell several times has its count before it.
std::string defeated_line(const std::vector<std::string>& names);
// A target key in a battle GUI state (1 the command wheel, 8 Analyze): -1 the press lowers the game's target index,
// +1 raises it, 0 not a target key there. The wheel: Left / A lower, Right / D raise. Analyze: E / D lower (its next
// target), Q / A raise.
int target_key_step(int gui_state, uint32_t vk);
// The skills in `now` that `was` does not hold (two lists of eight ids, 0 = an empty slot), in `now`'s order.
std::vector<int> new_skills(const uint16_t* now, const uint16_t* was);
// What a Shuffle Time card's face shows, from its type (2 a Major Arcana, 3 a suit card), its rank and the dialog of
// its effect page: "Fool", "Cups 3" (a suit card's number is its rank + 1). "" for a Persona card, whose page names
// it, and for a dialog that is no card's.
std::string card_face(int type, int rank, int dialog);
// A shop's top menu entry by its mode (1-4 the weapon shop's buy lists, 0xC Sell, 7 / 9 / 10 the antique shop's).
std::string shop_mode_name(int mode);
// Why a buy row cannot be bought now: nothing left of its stock, the player carries the maximum (99), or it costs
// more than the player has (`yen` below 0: not known); else just "not now".
const char* cannot_buy_reason(int stock, int have, int price, int yen);
}  // namespace p3a::core

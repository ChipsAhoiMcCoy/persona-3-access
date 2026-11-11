#include "core/battle_text.h"
#include "core/strings.h"
#include <format>
#include <map>
#include <string_view>

namespace p3a::core {
std::string turns_text(int n) { return std::format("{} {}", n, n == 1 ? strings::kTurn : strings::kTurns); }

std::string short_attack(std::string name) {
  static constexpr std::string_view tail = " Attack";
  if (name.size() > tail.size() && name.ends_with(tail)) name.resize(name.size() - tail.size());
  return name;
}

std::string defeated_line(const std::vector<std::string>& names) {
  std::map<std::string, int> count;
  std::vector<std::string> order;
  for (auto& n : names) if (!count[n]++) order.push_back(n);
  std::string s;
  for (auto& n : order) s += (s.empty() ? "" : ", ") + (count[n] > 1 ? std::format("{} {}", count[n], n) : n);
  return std::format("{} {}", s, strings::kDefeated);
}

int target_key_step(int gui_state, uint32_t vk) {
  constexpr uint32_t kLeft = 0x25, kRight = 0x27;   // VK_LEFT, VK_RIGHT
  if (gui_state == 1) return vk == kLeft || vk == 'A' ? -1 : vk == kRight || vk == 'D' ? 1 : 0;
  if (gui_state == 8) return vk == 'E' || vk == 'D' ? -1 : vk == 'Q' || vk == 'A' ? 1 : 0;   // R1 / L1 on the keyboard
  return 0;
}

std::vector<int> new_skills(const uint16_t* now, const uint16_t* was) {
  std::vector<int> out;
  for (int i = 0; now && i < 8; ++i) {
    uint16_t id = now[i];
    bool had = false;
    for (int j = 0; j < 8; ++j) had |= was[j] == id;
    if (id && !had) out.push_back(id);
  }
  return out;
}

// The effect pages' dialogs in the game's message file, in order: 4 Sword, 5 Coin, 6..25 Cup, 26 Wand, 27..39 Fool to
// Hanged Man, 92..98 Temperance to Judgement (there is no Death card).
std::string card_face(int type, int rank, int dialog) {
  int d = dialog;
  if (type == 2) {
    int arcana = d >= 27 && d <= 39 ? d - 26 : d >= 92 && d <= 98 ? d - 77 : 0;
    return arcana ? strings::kArcana[arcana] : "";
  }
  if (type != 3) return {};
  const char* suit = d == 4 ? strings::kSuitSwords : d == 5 ? strings::kSuitCoins : d >= 6 && d <= 25 ? strings::kSuitCups :
                     d == 26 ? strings::kSuitWands : nullptr;
  return suit ? std::format("{} {}", suit, rank + 1) : std::string();
}

std::string shop_mode_name(int mode) {
  switch (mode) {
    case 1: case 2: case 3: case 4: return strings::kWeaponShopMenu[mode - 1];
    case 0xC: return strings::kWeaponShopMenu[4];
    case 7: return strings::kAntiqueMenu[0];
    case 9: return strings::kAntiqueMenu[1];
    case 10: return strings::kAntiqueMenu[2];
    default: return std::format("{} {}", strings::kOption, mode);
  }
}

const char* cannot_buy_reason(int stock, int have, int price, int yen) {
  return !stock ? strings::kOutOfStock : have >= 99 ? strings::kCarryingMax
       : price > 0 && yen >= 0 && yen < price ? strings::kNotEnoughYen : strings::kCannotBuy;
}
}  // namespace p3a::core

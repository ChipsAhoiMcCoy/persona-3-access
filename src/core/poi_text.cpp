#include "core/poi_text.h"
#include "core/strings.h"
#include <cctype>
#include <cmath>
#include <format>
#include <map>
#include <set>

namespace p3a::core {

int clock_hour(float b) {
  int h = (int)std::lround(b / 30.0f);
  if (h <= 0) h += 12;
  return h;
}

std::vector<std::string> name_parts(const std::string& n) {
  std::vector<std::string> tok;
  for (size_t s = 0; s <= n.size();) { size_t e = n.find('_', s); if (e == std::string::npos) e = n.size(); tok.push_back(n.substr(s, e - s)); s = e + 1; }
  return tok;
}

std::string person_from_actor_name(const std::string& actor_name) {
  static const std::map<std::string, std::string> cast = {
    {"Yukari", "Yukari"}, {"Junpei", "Junpei"}, {"Mituru", "Mitsuru"}, {"Sanada", "Akihiko"}, {"Fuka", "Fuuka"},
    {"Aegis", "Aigis"}, {"Koromaru", "Koromaru"}, {"Ken", "Ken"}, {"Amada", "Ken"}, {"Shinjiro", "Shinjiro"}, {"Aragaki", "Shinjiro"},
    {"Ikutsuki", "Ikutsuki"}, {"Elizabeth", "Elizabeth"}, {"Igor", "Igor"}, {"Pharos", "Pharos"},
  };
  // FldNpcActor_105_102_Yukari_6 -> the first token after the field ids
  std::vector<std::string> tok = name_parts(actor_name);
  for (auto& t : tok) { auto it = cast.find(t); if (it != cast.end()) return it->second; }
  // The first word after the class (FldNpcActor_Jin has no field ids at all: Adamah, 2026-10-28).
  for (size_t i = 1; i < tok.size(); ++i) if (!tok[i].empty() && !isdigit((unsigned char)tok[i][0])) return tok[i];
  return {};
}

std::string story_scene(const std::string& actor_name) {
  std::vector<std::string> tok = name_parts(actor_name);
  if (tok.size() < 4 || tok[0] != "FldHitActor" || tok[1].empty() || !isdigit((unsigned char)tok[1][0])) return {};
  const std::string& w = tok[2];
  if (w.size() < 2 || !isalpha((unsigned char)w[0]) || (w[0] == 'H' && isdigit((unsigned char)w[1]))) return {};
  return w;
}

bool numbered_name(const std::string& flag, std::string& base, int& number) {
  size_t us = flag.find_last_of('_');
  if (us == std::string::npos || us + 1 >= flag.size() || flag.size() - us > 10 ||
      flag.find_first_not_of("0123456789", us + 1) != std::string::npos || (flag[us + 1] == '0' && us + 2 != flag.size()))
    return false;
  base = flag.substr(0, us);
  number = std::stoi(flag.substr(us + 1)) + 1;
  return true;
}

// The game marks nearly every hit with the plain Check icon: a business's door (Be Blue V, the police station, Hagakure
// Ramen) and a classroom's look exactly like a bookshelf; only the Net Cafe and Umiushi carry the Shop icon. Their
// scripts tell them apart (HIT_SKILL_SHOP, HIT_POLICE_ENTER, CALL_PUBLIC_SHOP; FLD_OPEN_ORD_DOOR_FADE for a room), so
// these lists sort the Check hits by the name the game gives them (surveyed from every field level's hits and their
// BF_OBJ flowscripts, 2026-10-31).
CheckKind check_kind(const std::string& name) {
  // Somewhere to buy, eat, work or spend time: a business's entrance, a vendor, a vending machine.
  static const std::set<std::string> shops = {
    "Aohige Pharmacy", "Azuki Arai Sweets", "Be Blue V", "Bookworms", "Used Bookstore", "Caf\xC3\xA9",
    "Chagall Caf\xC3\xA9", "Net Caf\xC3\xA9", "Game Parade", "Hagakure Ramen", "Ramen Shop", "Wakatsu Kitchen", "Umiushi Beef Bowls",
    "Hamburger Shop", "Wilduck Burger", "Kenneth Arnold", "Mahjong Parlor Red Hawk", "Mandragora", "Mayoido",
    "Mayoido Antiques", "Octopia", "Police Station", "Que Sera Sera", "Screen Shot", "Takoyaki Vendor",
    "Vending Machine", "Mystery Vending Machine", "Ticket Booth", "Kyoto Souvenirs", "Souvenir Sweets", "Kyoto Crafts",
  };
  // A room's door you check to go in (or find locked): classrooms, offices, restrooms, the dorm's bedrooms.
  static const std::set<std::string> doors = {
    "Classroom Door", "Classroom 2-E", "Computer Room", "Faculty Office", "Library", "Nurse's Office", "Laboratory",
    "Janitor's Room", "Art Room", "Science Room", "Music Room", "Music Prep Room", "Home Ec Prep Room",
    "Home Economics Room", "PA Room", "Printing Room", "Student Council Room", "Welfare Training Room", "Gym",
    "Indoor Swimming Pool", "Practice Field", "Men's Room", "Women's Room", "Restroom", "Bathroom", "Storage Room",
    "Storeroom", "Rooftop Storage", "Emergency Exit", "Rear Entrance", "Kitchen Door", "Locked Gate", "Metal Doors",
    "Large Door", "Command Room", "Sanada's Room", "Mitsuru's Room", "Yukari's Room", "Vacant Room", "Junpei's Room",
    "Aragaki's Room", "Amada's Room", "Fuuka's Room", "Aigis's Room", "His Room", "Door to Velvet Room", "Fuji Room", "Inn Exit", "Hot Spring",
    "School Building 2F", "School Building 3F",   // the school's stairs (a Check hit, HIT_SECOND_FLOOR_STAIRS)
  };
  if (shops.count(name)) return CheckKind::Shop;
  if (doors.count(name)) return CheckKind::Door;
  return CheckKind::Thing;
}

std::string chest_name(const std::string& type, const std::string& game_name) {
  if (type == "TBoxRare") return strings::kPoiChestRare;
  if (type == "TBoxJewelry") return strings::kPoiChestGold;
  int cost = type == "TBoxMedal1" ? 1 : type == "TBoxMedal2" ? 2 : type == "TBoxMedal3" ? 3 : 0;
  if (cost) return std::format("{}, {} {}", strings::kPoiChestLocked, cost, cost == 1 ? strings::kPoiFragment : strings::kPoiFragments);
  return game_name;
}

bool is_named(const std::string& poi_name, const std::string& label) {
  return poi_name == label || (poi_name.size() > label.size() + 2 && poi_name.starts_with(label) &&
                               poi_name.compare(label.size(), 2, " (") == 0);
}

// Every such first word in the game's hit-name tables ("Proceed Onward", "Enter the Velvet Room", "To furthest
// floor").
bool named_as_action(const std::string& name) {
  for (const char* verb : {"Go ", "Head ", "Return ", "Proceed ", "Enter ", "Exit ", "To "})
    if (name.rfind(verb, 0) == 0) return true;
  return false;
}

int settle_category(int current, int n, int delta, bool move, const std::function<bool(int)>& has_any) {
  for (int step = move ? 1 : 0; step <= n; ++step) {
    int c = ((current + step * delta) % n + n) % n;
    if (has_any(c)) return c;
  }
  return -1;
}

bool same_thing(const std::string& name_a, const Vec3& a, const std::string& name_b, const Vec3& b) {
  return name_a == name_b && std::fabs(a.x - b.x) + std::fabs(a.y - b.y) + std::fabs(a.z - b.z) < 150.0f;
}

std::vector<size_t> near_sides(const std::vector<Place>& sorted) {
  std::vector<size_t> kept;
  for (size_t i = 0; i < sorted.size(); ++i) {
    const Place& p = sorted[i];
    bool far_side = false;
    if (p.exit)
      for (size_t k : kept)
        if (sorted[k].exit && sorted[k].name != p.name &&
            std::hypot(sorted[k].at.x - p.at.x, sorted[k].at.y - p.at.y, sorted[k].at.z - p.at.z) < 200.0f) far_side = true;
    if (!far_side) kept.push_back(i);
  }
  return kept;
}

const char* icon_state(std::string_view kind) {
  if (kind == "Talk_Read") return strings::kIconTalked;
  if (kind == "Listen_Read") return strings::kIconHeard;
  if (kind == "Commu" || kind == "Commu_Field") return strings::kIconLink;
  if (kind == "Commu_Reverse") return strings::kIconLinkReversed;
  if (kind == "DormitoryLife") return strings::kIconHangout;
  if (kind == "MaleQuest" || kind == "MaleQuest_Field") return strings::kIconEpisode;
  if (kind == "Quest") return strings::kIconRequest;
  if (kind == "StudyGroup") return strings::kIconStudy;
  if (kind == "Walking") return strings::kIconWalk;
  return "";
}

std::string icon_kind(std::string_view class_name) {
  constexpr std::string_view head = "BP_UI_AccessIcon_", tail = "_C";
  if (class_name.size() <= head.size() + tail.size() || !class_name.starts_with(head) || !class_name.ends_with(tail)) return {};
  return std::string(class_name.substr(head.size(), class_name.size() - head.size() - tail.size()));
}

}  // namespace p3a::core

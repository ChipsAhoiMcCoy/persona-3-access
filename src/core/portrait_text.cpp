#include "portrait_text.h"
#include <cstdlib>

namespace p3a::core::portrait {
namespace {
struct Outfit { int chara, clothes; const char* text; };
struct Pose { int chara, clothes; char pose; const char* text; };
#include "portrait_entries.inc"

bool digits(std::string_view s, size_t n, int& out) {
  if (s.size() < n) return false;
  int v = 0;
  for (size_t i = 0; i < n; ++i) {
    if (s[i] < '0' || s[i] > '9') return false;
    v = v * 10 + (s[i] - '0');
  }
  out = v;
  return true;
}
}  // namespace

bool parse_texture(std::string_view name, Shown& out) {
  // a path ("/Game/.../T_BU_PC0002_PoseA_C102.T_BU_PC0002_PoseA_C102") or the object name alone
  if (size_t at = name.rfind("T_BU_"); at != std::string_view::npos) name = name.substr(at);
  else return false;
  if (size_t dot = name.find('.'); dot != std::string_view::npos) name = name.substr(0, dot);
  name.remove_prefix(5);
  if (name.size() < 2 || !(name.starts_with("PC") || name.starts_with("SC") || name.starts_with("NC"))) return false;
  name.remove_prefix(2);
  Shown s;
  if (!digits(name, 4, s.chara)) return false;
  name.remove_prefix(4);
  if (!name.starts_with("_Pose") || name.size() < 6) return false;
  s.pose = name[5];
  name.remove_prefix(6);
  if (!name.starts_with("_C")) return false;
  name.remove_prefix(2);
  if (!digits(name, 3, s.clothes) || name.size() != 3) return false;
  out = s;
  return true;
}

std::string describe(const Shown& s) {
  std::string out;
  for (auto& o : kOutfits)
    if (o.chara == s.chara && o.clothes == s.clothes) { out = o.text; break; }
  if (out.empty()) return out;
  for (auto& p : kPoses)
    if (p.chara == s.chara && p.clothes == s.clothes && p.pose == s.pose) { out += " "; out += p.text; break; }
  return out;
}

size_t outfit_count() { return sizeof(kOutfits) / sizeof(kOutfits[0]); }
size_t pose_count() { return sizeof(kPoses) / sizeof(kPoses[0]); }
bool has_outfit(int chara, int clothes) {
  for (auto& o : kOutfits) if (o.chara == chara && o.clothes == clothes && o.text && *o.text) return true;
  return false;
}
bool has_pose(int chara, int clothes, char pose) {
  for (auto& p : kPoses) if (p.chara == chara && p.clothes == clothes && p.pose == pose && p.text && *p.text) return true;
  return false;
}
}  // namespace p3a::core::portrait

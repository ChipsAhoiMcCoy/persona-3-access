#include "core/page_text.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include <cctype>

namespace p3a::core {
const char* icon_key(int id, bool pad, bool battle, std::string_view around) {
  const strings::IconKey* key = nullptr;
  for (auto& k : strings::kIconKeys) if (k.id == id) key = &k;
  if (!key) return nullptr;
  if (pad) return key->pad;
  auto nearby = [&](std::string_view w) {
    if (battle) return false;
    std::string a(around);
    for (auto& ch : a) ch = (char)tolower((unsigned char)ch);
    return a.find(w) != std::string::npos;
  };
  switch (id) {
    case 104: return battle ? strings::kIconUpBattle : key->keyboard;     // battle Tactics; field zoom in
    case 105: return battle ? strings::kIconDownBattle : key->keyboard;   // battle Survey; field camera reset
    case 112: return nearby("menu") ? strings::kIconMenuField : key->keyboard;   // battle skills (Persona); the field menu
    case 114: return nearby("map") ? strings::kIconMapField : key->keyboard;     // previous page; the field map
    case 117: return nearby("exploration") || nearby("recover") ? strings::kIconRecoverField : key->keyboard;   // next page; Auto-Recover
    default: return key->keyboard;
  }
}

std::string finish_icons(std::string out, bool pad, bool battle) {
  // The sentence around a button decides its key: from the last full stop before it to the next one after.
  for (size_t p; (p = out.find(kIconByPlace)) != std::string::npos && p + 1 < out.size();) {
    int id = (unsigned char)out[p + 1];
    size_t from = out.rfind('.', p), to = out.find('.', p);
    if (from == std::string::npos) from = 0;
    std::string around = out.substr(from, (to == std::string::npos ? out.size() : to) - from);
    const char* name = icon_key(id, pad, battle, around);
    out.replace(p, 2, name ? name : "");
  }
  for (size_t p; (p = out.find(kIconGlyph)) != std::string::npos;) {
    bool dropped = p + 1 >= out.size() || isalnum((unsigned char)out[p + 1]);
    out.replace(p, 1, dropped ? "" : strings::kAKey);
  }
  return out;
}

std::string rows_as_list(const std::vector<std::string>& rows, std::string_view joined) {
  std::vector<std::string> kept;
  for (auto& r : rows) if (std::string c = clean_text(r); !c.empty()) kept.push_back(c);
  if (kept.size() < 2) return {};
  std::string spaced, listed;
  for (size_t i = 0; i < kept.size(); ++i) {
    const std::string& r = kept[i];
    if (r.find_first_of(".,!?:;") != std::string::npos) return {};
    if (i && !isupper((unsigned char)r[0]) && !isdigit((unsigned char)r[0])) return {};
    spaced += (i ? " " : "") + r;
    listed += (i ? ", " : "") + r;
  }
  return spaced == joined ? listed : std::string{};
}

bool declares_voice(const uint8_t* d, size_t size, bool any) {
  if (!d) return false;
  for (size_t i = 0; i < 96 && i + 2 < size && d[i] == 0xFE;) {
    uint8_t f = d[i + 1];
    int argc = (f & 0x0F) - 1;
    if ((f & 0xF0) != 0xF0 || argc < 0) return false;
    if (d[i + 2] == 0x61 && argc >= 4) {
      if (i + 10 >= size) return false;
      if (any || (d[i + 3] == 0x02 && d[i + 4] == 0x01)) return d[i + 9] != 0x01 || d[i + 10] != 0x01;
    }
    i += 3 + (size_t)argc * 2;
  }
  return false;
}
}  // namespace p3a::core

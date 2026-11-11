// The game's item database: UDatItem holds, per category, a stats list (TableWeapon.Data: FWeaponItemList ...) and a
// name list in the same order (TableWeaponName.Data: FString). Shops and menus draw prices and stats as sprites; this
// gives them by item name.
#include "items.h"
#include "bmdfile.h"
#include "core/strings.h"
#include "party.h"
#include "seh.h"
#include "watch.h"
#include <windows.h>
#include <format>
#include <map>

namespace p3a::items {
using namespace ue;

static std::map<std::string, Info> g_cache;
static std::map<int, std::string> g_names;   // by item id
static WeakObj g_loaded;   // the DatItem g_cache was read from

void install() { watch::want("DatItem"); }   // before the first tick: that tick's one sweep fills it

static void load() {
  static const int dat_id = watch::want("DatItem");   // the object watcher's: no object scan per lookup
  UObject* dat = watch::get(dat_id);
  if (!dat || (WeakObj::of(dat) == g_loaded && !g_cache.empty())) return;
  // Its tables can be missing at first: read again once the set of tables it has changes (one appears), not on
  // every call.
  static const char* kTables[] = {"TableWeapon", "TableArmor", "TableShoes", "TableAccs", "TableCommon", "TableMaterial",
                                  "TableSkillcard", "TableEvitem", "TableCostume"};
  uint32_t have = 0;
  for (int i = 0; i < 9; ++i) if (get<UObject*>(dat, kTables[i])) have |= 1u << i;
  static uint32_t tried_with = ~0u;
  if (WeakObj::of(dat) == g_loaded && have == tried_with) return;
  tried_with = have;
  g_loaded = WeakObj::of(dat);
  g_cache.clear();
  g_names.clear();
  static const char* cats[] = {"Weapon", "Armor", "Shoes", "Accs", "Common", "Material", "Skillcard", "Evitem", "Costume"};
  // Where each category's counts start in the item bag (one byte per item, by its place in the name list).
  static const int bags[] = {0, 0x400, 0x800, 0xC00, 0xE00, 0x1300, 0x1700, 0x1200, 0x1B00};
  // The id category nibble, by the list above: skill cards are 7 and event items 5 (the bag reader exe+0x11ef210
  // reads nibble 5 at bag +0x1200 and 7 at +0x1700; the camp Item list confirmed it, 2026-10-30).
  static const int codes[] = {0, 1, 2, 3, 4, 6, 7, 5, 8};
  for (int ci = 0; ci < 9; ++ci) {
    const char* c = cats[ci];
    UObject* table = get<UObject*>(dat, std::string("Table") + c);
    UObject* names = get<UObject*>(dat, std::string("Table") + c + "Name");
    if (!table || !names) continue;
    guarded([&] {
      auto* rows = field<TArray<uint8_t>>(table, "Data");
      auto* ns = field<TArray<FString>>(names, "Data");
      FField* dp = find_prop(class_of(table), "Data");
      if (!rows || !ns || !dp) return;
      UStruct* st = prop_inner_object(prop_inner_prop(dp));
      int sz = struct_size(st);
      auto off = [&](const char* f) { return offset_of(st, f); };
      int oPrice = off("Price"), oSell = off("SellPrice"), oAtk = off("attack"), oAcc = off("Accuracy"), oDef = off("Defence"), oEva = off("Evasion");
      for (int i = 0; i < rows->num && i < ns->num; ++i) {
        std::string n = to_utf8(ns->data[i]);
        if (n.empty() || n == "Unused") continue;
        if (!g_names.count(codes[ci] << 12 | i)) g_names[codes[ci] << 12 | i] = n;
        if (g_cache.count(n)) continue;
        uint8_t* r = rows->data + i * sz;
        Info in;
        in.category = c;
        in.bag = bags[ci] + i;
        in.id = codes[ci] << 12 | i;
        if (oPrice >= 0) in.price = *(uint32_t*)(r + oPrice);
        if (oSell >= 0) in.sell = *(uint32_t*)(r + oSell);
        if (oAtk >= 0) in.attack = *(uint16_t*)(r + oAtk);
        if (oAcc >= 0) in.accuracy = *(uint16_t*)(r + oAcc);
        if (oDef >= 0) in.defence = *(uint16_t*)(r + oDef);
        if (oEva >= 0) in.evasion = *(uint16_t*)(r + oEva);
        g_cache[n] = in;
      }
    });
  }
}

std::string name_of(int id) {
  load();
  auto it = g_names.find(id);
  return it == g_names.end() ? std::string() : it->second;
}

bool find(const std::string& name, Info& out) {
  load();
  auto it = g_cache.find(name);
  if (it == g_cache.end()) return false;
  out = it->second;
  return true;
}

// UGlobalWork's item bag (p3rpc.nativetypes' ItemBag, 0x568 further on in this build: its money field, bag + 0x1D78,
// held the wallet at +0x4260, measured 2026-10-27).
int owned(const Info& in) {
  uint8_t* g = party::global_work();
  if (!g || in.bag < 0 || in.bag >= 0x1D00) return -1;
  int n = -1;
  guarded([&] { n = *(g + 0x24E8 + in.bag); });
  return n;
}

std::string stats(const Info& in) {
  std::string s;
  if (in.attack) s += std::format(", {} {}", strings::kAttack, in.attack);
  if (in.accuracy) s += std::format(", {} {}", strings::kAccuracy, in.accuracy);
  if (in.defence) s += std::format(", {} {}", strings::kDefence, in.defence);
  if (in.evasion) s += std::format(", {} {}", strings::kEvasion, in.evasion);
  return s;
}
// The game's help line for an item: BMD_Item<category>Help, dialog = the id's index (the drink 0x4033, Nastea, is
// BMD_ItemCommonHelp dialog 0x33, as the shops draw it; 2026-11-01). The hero's own gear names him through a token
// (function 4 2: "For Makoto.", "Fits Makoto.", "The attire Makoto wears"), filled in here as the game draws it;
// unused rows are Japanese. Empty when unknown.
std::string help(int id) {
  static const char* by_code[] = {"Weapon", "Armor", "Shoes", "Accs", "Common", "Evitem", "Material", "Skillcard", "Costume"};
  int code = id >> 12, i = id & 0xFFF;
  if (id < 0 || code >= (int)std::size(by_code)) return {};
  auto f = bmdfile::load(std::format("BMD_Item{}Help", by_code[code]).c_str());
  if (!f || !f->ok || i >= (int)f->dialogs.size() || f->dialogs[i].pages.empty()) return {};
  std::string t = bmdfile::text(f->dialogs[i].pages[0]);
  return t.starts_with("\xE6\x9C\xAA\xE4\xBD\xBF\xE7\x94\xA8") ? std::string() : t;   // "unused" in Japanese
}
}  // namespace p3a::items

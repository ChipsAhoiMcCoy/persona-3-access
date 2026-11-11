// Persona data and the hero's stock. Names: UDatPersonaNameTable.Data[id].Name (FText); arcana and base level:
// UDatPersonaTable.Data[id] (FDatPersonaDataRecord, 0xE bytes: +2 Race, +3 Level). The stock: UGlobalWork +0x1B0 is
// FDatUnitWork[11] (0x2B4 each; the hero is unit 1, ID 1), +0x48 FDatUnitPersona {u16 equip, entries of 0x30 from +4:
// +0 flags (1 = filled), +2 id, +4 level, +0xC skills u16[8], +0x1C St Ma En Ag Lu} (p3rpc.nativetypes; measured
// 2026-10-27: Orpheus 3 and Apsaras 2).
#include "persona.h"
#include "bmdfile.h"
#include "core/strings.h"
#include "devserver.h"
#include "hooks.h"
#include "party.h"
#include "seh.h"
#include "social.h"
#include "ue/scan.h"
#include "ue/ue.h"
#include "watch.h"
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <format>
#include <memory>

namespace p3a::persona {
using namespace ue;

static UObject* dat() {
  static CachedFind w;
  return w.get("DatPersona");
}

std::string name(int id) {
  UObject* d = dat();
  UObject* t = d ? get<UObject*>(d, "TableName") : nullptr;
  auto* rows = t ? field<TArray<FText>>(t, "Data") : nullptr;   // FDatPersonaName is one FText
  if (!rows || id <= 0 || id >= rows->num) return {};
  std::string s;
  guarded([&] { s = text(rows->data[id]); });
  return s;
}

static const uint8_t* record(int id) {
  UObject* d = dat();
  UObject* t = d ? get<UObject*>(d, "TablePersona") : nullptr;
  auto* rows = t ? field<TArray<uint8_t>>(t, "Data") : nullptr;
  if (!rows || id <= 0 || id >= rows->num) return nullptr;
  return rows->data + id * 0xE;
}
// Affinities: UDatAffinityTable (DatPersonaAffinityDataAsset) Data[id] = u16 attr[19]: 0 Slash .. 6 Wind, 7 Almighty,
// 8 Light, 9 Dark (2026-10-31: Gabriel nulls 8 and is weak to 9; Hell Biker weak to 8, its page marks Light). The
// high byte is the kind (0x01 nulls, 0x02 repels, 0x04 drains, 0x08 weak, 0x10 resists), 0x0014 alone is normal
// (measured 2026-10-28: Nekomata 0x0800 Elec, 0x1000 Wind, as its status shows; 2026-10-31: Jack Frost 0x0100 Ice,
// Mithras 0x0400 Fire, "Drain" on its level-up page, Hell Biker 0x0200 Fire and Wind, the repel shield on its page;
// the table holds no 0x20-0x80 kinds, one 0x2000 on Alice's Dark, not read).
std::string affinities(int id) {
  std::string out;
  for (auto& g : affinity_groups(id)) out += (out.empty() ? "" : ", ") + g;
  return out;
}

std::vector<std::string> affinity_groups(int id) {
  static CachedFind w;
  UObject* t = w.get("DatAffinityTable");
  auto* rows = t ? field<TArray<uint8_t>>(t, "Data") : nullptr;
  if (!rows || id <= 0 || id >= rows->num) return {};
  const uint16_t* a = (const uint16_t*)(rows->data + id * 38);
  static const int kColumn[9] = {0, 1, 2, 3, 4, 5, 6, 8, 9};   // kAttrNames' order in the table (7 is Almighty)
  static const struct { uint16_t bit; int kind; } kinds[] = {{0x0800, 4}, {0x1000, 5}, {0x0100, 1}, {0x0200, 2}, {0x0400, 3}};
  std::vector<std::string> out;
  guarded([&] {
    for (auto& k : kinds) {
      std::string list;
      for (int i = 0; i < 9; ++i) if (a[kColumn[i]] & k.bit) list += (list.empty() ? "" : " ") + std::string(strings::kAttrNames[i]);
      if (!list.empty()) out.push_back(std::string(strings::kAffinity[k.kind]) + " " + list);
    }
  });
  return out;
}

int arcana(int id) { const uint8_t* r = record(id); return r ? r[2] : 0; }   // Race 1 = Fool
int base_level(int id) { const uint8_t* r = record(id); return r ? r[3] : 0; }

static uint8_t* hero_personas() {
  uint8_t* u = party::unit_work(1);
  return u ? u + 0x48 : nullptr;
}

std::vector<Owned> stock() {
  std::vector<Owned> out;
  uint8_t* p = hero_personas();
  if (!p) return out;
  guarded([&] {
    for (int i = 0; i < 12; ++i) {
      uint8_t* e = p + 4 + i * 0x30;
      if (!(*(uint16_t*)e & 1)) continue;
      Owned o{i, *(uint16_t*)(e + 2), *(uint16_t*)(e + 4), {}, {}};
      memcpy(o.skills, e + 0xC, sizeof o.skills);
      memcpy(o.stats, e + 0x1C, sizeof o.stats);
      if (o.id > 0) out.push_back(o);
    }
  });
  return out;
}

int unit_level(int unit_id) {
  uint8_t* u = unit_id == 1 ? party::unit_work(unit_id) : nullptr;   // the other members keep their level elsewhere (+0x1C reads 1 for them)
  if (!u) return 0;
  int lv = 0;
  guarded([&] { lv = *(int16_t*)(u + 0x1C); });
  return lv;
}

int equipped() {
  uint8_t* p = hero_personas();
  int v = -1;
  if (p) guarded([&] { v = *(uint16_t*)p; });
  return v;
}

int dyad_result(int a, int la, int b, int lb) {
  int ra = arcana(a), rb = arcana(b);
  if (ra < 1 || rb < 1 || a == b) return 0;
  // From the object watcher, not a scan: Fusion Search tries every pair in the stock per cursor move (66 for 12
  // Personas), and a scan each was about 1 ms (2026-10-29).
  static const int chart_id = watch::want("NormalSpreadDataAsset");
  UObject* chart = watch::get(chart_id);
  int race = 0;
  if (chart) guarded([&] {
    // Data: TArray<FNormalSpreadItem{TArray<uint8> Races}>
    auto* rows = field<TArray<TArray<uint8_t>>>(chart, "Data");
    int lo = std::min(ra, rb) - 1, hi = std::max(ra, rb) - 1;
    if (rows && lo < rows->num && hi < rows->data[lo].num) race = rows->data[lo].data[hi];
  });
  if (race < 1) return 0;
  int target = (la + lb) / 2 + 1;
  int best = 0, best_level = 0;
  for (int id = 1; id < 600; ++id) {
    const uint8_t* r = record(id);
    if (!r) break;
    if (r[2] != race || *(uint16_t*)r != 0 || id == a || id == b) continue;
    int lv = r[3];
    if (ra != rb) { if (lv >= target && (!best || lv < best_level)) { best = id; best_level = lv; } }
    else if (lv < target && (!best || lv > best_level)) { best = id; best_level = lv; }
  }
  return best;
}

// The game's help line for a skill, from its file (BMD_SkillHelp, dialog = the skill id) rather than the drawn page:
// the game builds that page once per highlight, so a drawn-page read came up empty after a second or on a reused page.
std::string skill_help(int id) {
  auto f = bmdfile::load("BMD_SkillHelp");
  if (!f || !f->ok || id < 0 || id >= (int)f->dialogs.size() || f->dialogs[id].pages.empty()) return {};
  return bmdfile::text(f->dialogs[id].pages[0]);
}
std::string skill_name(int id) {
  static CachedFind w;
  UObject* t = w.get("DatSkillNameTable");
  auto* rows = t ? field<TArray<FText>>(t, "Data") : nullptr;
  if (!rows || id <= 0 || id >= rows->num) return {};
  std::string s;
  guarded([&] { s = text(rows->data[id]); });
  return s;
}

std::string skill_cost(int id) {
  static CachedFind w;
  UObject* t = w.get("DatSkillNormalTable");
  if (!t) return {};
  FField* dp = find_prop(class_of(t), "Data");
  auto* rows = field<TArray<uint8_t>>(t, "Data");
  UStruct* st = dp ? (UStruct*)prop_inner_object(prop_inner_prop(dp)) : nullptr;
  if (!rows || !st || id <= 0 || id >= rows->num) return {};
  int sz = struct_size(st), oType = offset_of(st, "costtype"), oCost = offset_of(st, "cost");
  if (oType < 0 || oCost < 0) return {};
  std::string s;
  guarded([&] {
    const uint8_t* r = rows->data + (size_t)id * sz;
    int type = r[oType], cost = *(uint16_t*)(r + oCost);
    if (type == 1 && cost) s = std::format("{}% {}", cost, strings::kHP);
    else if (type == 2 && cost) s = std::format("{} {}", cost, strings::kSP);
  });
  return s;
}

int stat_total(const uint8_t* e, int i) {
  if (!e || i < 0 || i > 4) return 0;
  int v = 0;
  guarded([&] { v = e[0x1C + i] + e[0x21 + i] + e[0x26 + i]; });
  return v > 99 ? 99 : v;
}

// The total EXP an entry needs for a level: exe+0x1204000(entry, level) (0 below level 2; ally Personas, ids 1..0x22,
// from their unit's table, the rest from a curve; the base of FDatUnitPersona::CalculateBonusExp, exe+0x1563220).
int exp_for_level(const uint8_t* e, int level) {
  if (!e || level < 2 || !ue::known_build()) return 0;
  int v = -1;
  guarded([&] { v = ((int (*)(const void*, uint16_t))(scan::exe_base() + 0x1204000))(e, (uint16_t)level); });
  return v;
}

int exp_to_next(const uint8_t* e) {
  int level = e ? *(const uint16_t*)(e + 4) : 99;
  if (level >= 99) return -1;
  int need = exp_for_level(e, level + 1);
  return need < 0 ? -1 : std::max(0, need - (int)*(const uint32_t*)(e + 8));
}

int level_with_exp(const uint8_t* e, int gain) {
  int level = e ? *(const uint16_t*)(e + 4) : 0;
  if (!e || gain <= 0) return level;
  int64_t total = (int64_t)*(const uint32_t*)(e + 8) + gain;
  while (level < 99) {
    int need = exp_for_level(e, level + 1);
    if (need <= 0 || need > total) break;
    ++level;
  }
  return level;
}

std::vector<std::string> detail_lines(const uint8_t* e) {
  std::vector<std::string> out;
  int id = e ? *(const uint16_t*)(e + 2) : 0;
  if (name(id).empty()) return out;
  for (int i = 0; i < 5; ++i) out.push_back(std::format("{} {}", strings::kStatNames[i], stat_total(e, i)));
  for (int i = 0; i < 8; ++i)
    if (int sid = *(const uint16_t*)(e + 0xC + i * 2)) {
      std::string l = skill_name(sid), cost = skill_cost(sid);
      out.push_back(cost.empty() ? l : l + ", " + cost);
    }
  auto groups = affinity_groups(id);
  if (groups.empty()) groups.push_back(strings::kNoAffinities);
  for (auto g : groups) { g[0] = (char)toupper((unsigned char)g[0]); out.push_back(g); }
  return out;
}

const uint8_t* stock_entry(int slot) {
  uint8_t* p = hero_personas();
  return p && slot >= 0 && slot < 12 ? p + 4 + slot * 0x30 : nullptr;
}

std::vector<Learn> learnset(int id) {
  std::vector<Learn> out;
  UObject* d = dat();
  UObject* t = d ? get<UObject*>(d, "TableGrowth") : nullptr;
  FField* dp = t ? find_prop(class_of(t), "Data") : nullptr;
  auto* rows = t ? field<TArray<uint8_t>>(t, "Data") : nullptr;
  UStruct* st = dp ? (UStruct*)prop_inner_object(prop_inner_prop(dp)) : nullptr;
  if (!rows || !st || id <= 0 || id >= rows->num) return out;
  int sz = struct_size(st), ev = offset_of(st, "skillEvent"), base = base_level(id);
  if (ev < 0) return out;
  guarded([&] {
    const uint8_t* r = rows->data + (size_t)id * sz + ev;
    for (int i = 0; i < 16; ++i, r += 6)
      if (int sk = *(const uint16_t*)(r + 4)) out.push_back({base + r[0], sk});
  });
  return out;
}

std::string status_line(const uint8_t* e) {
  std::string s;
  guarded([&] {
    int id = *(uint16_t*)(e + 2), level = *(uint16_t*)(e + 4);
    if (!id) return;
    s = describe(id, level);
    static const char* stat[5] = {"St", "Ma", "En", "Ag", "Lu"};
    for (int i = 0; i < 5; ++i) s += std::format(", {} {}", stat[i], stat_total(e, i));
    std::string sk;
    for (int i = 0; i < 8; ++i)
      if (int sid = *(uint16_t*)(e + 0xC + i * 2)) sk += (sk.empty() ? "" : ", ") + skill_name(sid);
    if (!sk.empty()) s += std::string(". ") + strings::kSkills + ": " + sk;
    if (std::string af = affinities(id); !af.empty()) { af[0] = (char)toupper((unsigned char)af[0]); s += ". " + af; }
  });
  return s;
}

const uint8_t* status_entry(UObject* ps) {
  UObject* draw = ps ? get<UObject*>(ps, "pPersonaStatusDraw") : nullptr;
  return draw ? (const uint8_t*)draw + 0x34C : nullptr;
}
bool status_page_up(UObject* ps) { return ps && *(int32_t*)((uint8_t*)ps + 0x528) == 1; }   // 2 once closed
const uint8_t* shown_status() {
  static const int status_id = watch::want("PersonaStatus");   // absent outside its screen: no scan per call
  return status_entry(watch::get(status_id));
}

std::string describe(int id, int level) {
  return std::format("{}, {}, {} {}", name(id), social::arcana_name(arcana(id)), strings::kLevel, level);
}

void install() {
  dev::route("/persona", [](const dev::Request& r, int&) {
    // /persona the hero's stock | ?id=N one Persona's data
    auto out = std::make_shared<std::string>("timeout\n");
    int id = (int)dev::arg_int(r, "id", -1);
    std::string find = dev::arg(r, "find");
    bool bench = dev::has(r, "bench"), learn = dev::has(r, "learn");
    hooks::run_on_game_thread([out, id, find, bench, learn] {
      if (bench) {   // ?bench=1: Fusion Search's work for one cursor move (every stock pair), and one object scan
        LARGE_INTEGER f, t0, t1, t2;
        QueryPerformanceFrequency(&f);
        auto st = stock();
        int pairs = 0;
        QueryPerformanceCounter(&t0);
        for (size_t a = 0; a < st.size(); ++a)
          for (size_t b = a + 1; b < st.size(); ++b) { dyad_result(st[a].id, st[a].level, st[b].id, st[b].level); ++pairs; }
        QueryPerformanceCounter(&t1);
        find_first("NormalSpreadDataAsset");
        QueryPerformanceCounter(&t2);
        auto us = [&](LARGE_INTEGER a, LARGE_INTEGER b) { return (b.QuadPart - a.QuadPart) * 1000000 / f.QuadPart; };
        *out = std::format("{} pairs: {} us; one object scan: {} us\n", pairs, us(t0, t1), us(t1, t2));
        return;
      }
      if (!find.empty()) {
        *out = "not found\n";
        for (int i = 1; i < 600; ++i) if (name(i) == find) { *out = std::format("{} {}\n", i, find); break; }
        return;
      }
      if (id == 0) {   // ?id=0: every Persona (id race level flag name)
        std::string s;
        for (int i = 1; i < 600; ++i) {
          const uint8_t* rec = record(i);
          if (!rec) break;
          s += std::format("{} {} {} {:#x} {}\n", i, rec[2], rec[3], *(uint16_t*)rec, name(i));
        }
        *out = s;
        return;
      }
      if (id >= 0) { *out = std::format("{} {} arcana {} base level {}; skill {}\n", id, name(id), arcana(id), base_level(id), skill_name(id)); return; }
      if (learn) {   // ?learn=1: the equipped Persona one EXP short of the level of its next skill (dev: the victory
                     // screen's skill-learned status page)
        auto* e = (uint8_t*)stock_entry(equipped());
        int pid = e ? *(uint16_t*)(e + 2) : 0, level = e ? *(uint16_t*)(e + 4) : 0;
        *out = "no skill left to learn\n";
        for (auto& l : learnset(pid)) {
          if (l.level <= level) continue;
          int need = exp_for_level(e, l.level);
          if (need <= 0) break;
          *(uint32_t*)(e + 8) = (uint32_t)(need - 1);
          *out = std::format("{} EXP {}: learns {} at level {}\n", name(pid), need - 1, skill_name(l.skill), l.level);
          break;
        }
        return;
      }
      std::string s = std::format("equipped slot {}\n", equipped());
      for (auto& o : stock()) s += std::format("  slot {} id {} {}\n", o.slot, o.id, describe(o.id, o.level));
      *out = s;
    });
    return *out;
  }, "the hero's Persona stock | ?id=N one Persona | ?learn=1 the equipped one 1 EXP short of its next skill (dev)");
}
}  // namespace p3a::persona

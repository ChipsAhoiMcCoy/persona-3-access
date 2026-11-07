#pragma once
// The battle's own words and numbers, from its database (UBtlDataBaseComponent on BP_BtlTopUI_C: GetUnitName,
// GetUnitHP, GetSkillName, GetSkillCost, GatAnalizeAttr, GetAttackName, GetPersonaName, CheckSupport). Used by the
// battle screen (battle.cpp).
#include "screens/util.h"
#include "core/strings.h"
#include "party.h"
#include <format>
#include <map>
#include <string>
#include <vector>

namespace p3a::screens {
using namespace ue;   // as in every screen file; this header is included by battle.cpp only
class BtlWords {
  Finder top_{"BP_BtlTopUI_C"};
 public:
  using UObject = ue::UObject;
  UObject* db() { UObject* t = top_.get(); return t ? get<UObject*>(t, "BtlDataBase") : nullptr; }
  // A database call taking one actor, returning an int.
  int unit_int(const char* fn, UObject* a) {
    UObject* d = db();
    UFunction* f = d && a ? find_function(class_of(d), fn) : nullptr;
    if (!f) return -1;
    ue::Params p(f);
    p.set<UObject*>("Actor", a);
    call(d, p);
    return p.ret<int32_t>();
  }
  // The database's FText results are not released: releasing them deadlocked the game thread inside the text's
  // destructor (GetUnitName, 2026-10-27). Each distinct name is fetched once and cached instead, so the few
  // unreleased texts are bounded by the number of units, skills and attributes.
  std::string ftext_call(UObject* d, ue::Params& p) {
    call(d, p);
    FText* t = p.at<FText>("ReturnValue");
    return t ? text(*t) : std::string();
  }
  std::map<WeakObj::Key, std::string> unit_names_;   // by object index + full name
  std::map<int, std::string> skill_names_, attack_names_;
  std::string unit_name(UObject* a) {
    auto key = WeakObj::of(a).key();
    if (auto it = unit_names_.find(key); it != unit_names_.end()) return it->second;
    UObject* d = db();
    UFunction* f = d && a ? find_function(class_of(d), "GetUnitName") : nullptr;
    if (!f) return {};
    ue::Params p(f);
    p.set<UObject*>("Actor", a);
    p.set<uint8_t>("InsertSpace", 1);
    return unit_names_[key] = ftext_call(d, p);
  }
  std::string skill_name(int id) {
    if (auto it = skill_names_.find(id); it != skill_names_.end()) return it->second;
    UObject* d = db();
    UFunction* f = d ? find_function(class_of(d), "GetSkillName") : nullptr;
    if (!f) return {};
    ue::Params p(f);
    p.set<int32_t>("skillId", id);
    return skill_names_[id] = ftext_call(d, p);
  }
  std::string attack_name(int attr) {
    if (auto it = attack_names_.find(attr); it != attack_names_.end()) return it->second;
    UObject* d = db();
    UFunction* f = d ? find_function(class_of(d), "GetAttackName") : nullptr;
    if (!f) return {};
    ue::Params p(f);
    p.set<int32_t>("attr", attr);
    return attack_names_[attr] = ftext_call(d, p);
  }
  // "3 SP" / "8% HP" (physical skills cost a share of HP).
  std::string skill_cost(int id, UObject* who) {
    UObject* d = db();
    UFunction* f = d ? find_function(class_of(d), "GetSkillCost") : nullptr;
    if (!f) return {};
    ue::Params p(f);
    p.set<int32_t>("skillId", id);
    p.set<UObject*>("Action", who);
    call(d, p);
    int v = p.ret<int32_t>();
    uint8_t* type = p.at<uint8_t>("cost");
    if (!type || *type == 0 || v <= 0) return {};
    return *type == 1 ? std::format("{}% {}", v, strings::kHP) : std::format("{} {}", v, strings::kSP);
  }
  std::string hp_sp(UObject* a) {
    return std::format("{} {} {} {}, {} {} {} {}", strings::kHP, unit_int("GetUnitHP", a), strings::kOf, unit_int("GetUnitMaxHP", a),
                       strings::kSP, unit_int("GetUnitSP", a), strings::kOf, unit_int("GetUnitMaxSP", a)) + ailments(a) + buffs(a);
  }
  // BtlDataBase GetBadStatus(EBtlDataBadStatus): the same answer the info panel's icons draw from.
  bool has_ailment(UObject* a, int type) {
    UObject* d = db();
    UFunction* f = d && a ? find_function(class_of(d), "GetBadStatus") : nullptr;
    if (!f) return false;
    ue::Params p(f);
    p.set<UObject*>("Actor", a);
    p.set<int32_t>("Type", type);
    call(d, p);
    return p.ret<bool>();
  }
  // ABtlActor +0x38E: one signed byte per status bit, the turns the ailment can still last; a roll can end it sooner
  // (FUN_140fe2ef0 sets it, the decrement after FUN_140fd91e0 counts it down; checked live on Jin's Panic: 3, 2, 1, 0).
  // Bit i is 1 << i; the enum's first 12 are bits 0-11 (FUN_14102b0a0). Poison stays at 0 until it is cured, and Down
  // has no count: those read without one.
  int ailment_turns(UObject* a, int type) {
    if (!a || type < 0 || type > 11) return 0;
    return *((int8_t*)a + 0x38E + type);
  }
  // ", Downed, Poisoned": what a focused unit is suffering, Down first.
  std::string ailments(UObject* a) {
    std::string s;
    if (has_ailment(a, 13)) s += std::string(", ") + strings::kAilments[13];
    for (int t = 0; t <= 11; ++t)
      if (has_ailment(a, t)) s += std::string(", ") + strings::kAilments[t];
    return s;
  }
  std::string turns_text(int n) { return std::format("{} {}", n, n == 1 ? strings::kTurn : strings::kTurns); }
  // BtlDataBase GetSupportTurn: turns left on a buff or debuff (BP_BtlTargetInfo marks 1 as its last).
  int support_turn(UObject* a, int type) {
    UObject* d = db();
    UFunction* f = d && a ? find_function(class_of(d), "GetSupportTurn") : nullptr;
    if (!f) return 0;
    ue::Params p(f);
    p.set<UObject*>("Actor", a);
    p.set<int32_t>("Type", type);
    call(d, p);
    return p.ret<int32_t>();
  }
  bool has_support(UObject* a, int type) {
    UObject* d = db();
    UFunction* f = d && a ? find_function(class_of(d), "CheckSupport") : nullptr;
    if (!f) return false;
    ue::Params p(f);
    p.set<UObject*>("Actor", a);
    p.set<int32_t>("Type", type);
    call(d, p);
    return p.ret<bool>();
  }
  // Everything known about a unit, one line each: what the right stick (Ctrl+Up / Down) walks through. The first line
  // is who it is; what the game hides from a sighted player about a Shadow stays hidden (its stats, HP as a number).
  std::vector<std::string> details(UObject* a) {
    std::vector<std::string> out;
    if (!a) return out;
    bool foe = get_bool(a, "EnemyFlag");
    // GetUnitLevel is not a member's level ("Junpei, level 22" at level 9, 2026-11-06): the save data's, as on Stats.
    int level = unit_int("GetUnitLevel", a);
    party::Member m;
    if (!foe && party::member(unit_int("GetUnitID", a), m)) level = m.level;
    out.push_back(std::format("{}, {} {}", unit_name(a), strings::kLevel, level));
    int hp = unit_int("GetUnitHP", a), mx = unit_int("GetUnitMaxHP", a);
    if (foe) { if (mx > 0) out.push_back(std::format("{} {}%", strings::kHP, (hp * 100 + mx - 1) / mx)); }
    else {
      out.push_back(std::format("{} {} {} {}", strings::kHP, hp, strings::kOf, mx));
      out.push_back(std::format("{} {} {} {}", strings::kSP, unit_int("GetUnitSP", a), strings::kOf, unit_int("GetUnitMaxSP", a)));
    }
    bool any = false;
    if (has_ailment(a, 13)) { out.push_back(strings::kAilments[13]); any = true; }
    for (int t = 0; t <= 11; ++t) {
      if (!has_ailment(a, t)) continue;
      any = true;
      int n = ailment_turns(a, t);
      out.push_back(n > 0 ? std::format("{}, {} {}", strings::kAilments[t], strings::kUpTo, turns_text(n)) : std::string(strings::kAilments[t]));
    }
    if (!any) out.push_back(strings::kNoAilments);
    if (call_ret<bool>(a, "IsGuard")) out.push_back(strings::kGuarding);
    size_t before = out.size();
    auto effect = [&](bool on, int type, const char* word) {
      if (!on) return;
      int n = support_turn(a, type);
      out.push_back(n > 0 ? std::format("{}, {}", word, turns_text(n)) : std::string(word));
    };
    // The HUD's stats (buffs() above), then what the info panel draws beside them, then the rest of EBtlDataSupportTable.
    effect(has_support(a, 0), 0, strings::kBuffs[0]);
    effect(has_support(a, 1), 1, strings::kBuffs[1]);
    effect(has_support(a, 4), 4, strings::kBuffs[2]);
    effect(has_support(a, 5), 5, strings::kBuffs[3]);
    effect(has_support(a, 2) || has_support(a, 6), has_support(a, 2) ? 2 : 6, strings::kBuffs[4]);
    effect(has_support(a, 3) || has_support(a, 7), has_support(a, 3) ? 3 : 7, strings::kBuffs[5]);
    effect(has_support(a, 21), 21, strings::kCharged);
    effect(has_support(a, 22), 22, strings::kConcentrating);
    effect(has_support(a, 19), 19, strings::kPhysicalShield);
    effect(has_support(a, 20), 20, strings::kMagicShield);
    for (int i = 0; i < 4; ++i) effect(has_support(a, 15 + i), 15 + i, strings::kWalls[i]);
    for (int i = 0; i < 4; ++i) effect(has_support(a, 10 + i), 10 + i, strings::kBreaks[i]);
    effect(has_support(a, 14), 14, strings::kAilmentsDoubled);
    if (out.size() == before) out.push_back(strings::kNoEffects);
    for (auto& l : affinity_groups(a)) out.push_back(l);
    if (!foe) {
      if (int pid = unit_int("GetPersonaEquipID", a); pid > 0) out.push_back(std::format("{} {}", strings::kPersonaWord, persona_name(pid)));
      static const char* stats[] = {"GetStatusStrength", "GetStatusMagic", "GetStatusEndurance", "GetStatusSpeed", "GetStatusLuck"};
      std::string line;
      for (int i = 0; i < 5; ++i) line += std::format("{}{} {}", i ? ", " : "", strings::kStatNames[i], unit_int(stats[i], a));
      out.push_back(line);
      int tp = unit_int("GetUnitTP", a), max_tp = unit_int("GetUnitMaxTP", a);
      if (max_tp > 0) out.push_back(std::format("{} {} {} {}", strings::kTheurgyCharge, tp, strings::kOf, max_tp));
    }
    return out;
  }
  // Stat buffs and debuffs the HUD shows as ATK / DEF / AGI arrows: BtlDataBase CheckSupport(Type) with
  // EBtlDataSupportTable 0/1 attack, 4/5 defense, 2/3 evasion and 6/7 accuracy (agility on the HUD); measured
  // 2026-10-28 after Masukukaja (2 and 6 set). CheckSupportDownUp answered false throughout.
  std::string buffs(UObject* a) {
    UObject* d = db();
    UFunction* f = d && a ? find_function(class_of(d), "CheckSupport") : nullptr;
    if (!f) return {};
    auto on = [&](int type) {
      ue::Params p(f);
      p.set<UObject*>("Actor", a);
      p.set<int32_t>("Type", type);
      call(d, p);
      return p.ret<bool>();
    };
    std::string out;
    bool set[6] = {on(0), on(1), on(4), on(5), on(2) || on(6), on(3) || on(7)};
    for (int i = 0; i < 6; ++i) if (set[i]) out += std::string(", ") + strings::kBuffs[i];
    return out;
  }
  // An enemy: name, health as a share (the game shows a bar), and the affinities the player has found out.
  std::string enemy(UObject* e) {
    std::string s = unit_name(e);
    int hp = unit_int("GetUnitHP", e), mx = unit_int("GetUnitMaxHP", e);
    if (mx > 0) s += std::format(", {}%", (hp * 100 + mx - 1) / mx);
    s += ailments(e);
    s += buffs(e);
    for (auto& l : affinity_groups(e)) s += ", " + l;
    return s;
  }
  // The affinities the player has found out, a line per kind: "weak to Fire Attack, Ice Attack".
  std::vector<std::string> affinity_groups(UObject* e) {
    std::vector<std::string> out;
    UObject* d = db();
    UFunction* f = d ? find_function(class_of(d), "GatAnalizeAttr") : nullptr;
    if (!f) return out;
    std::vector<std::string> by[7];
    for (int attr = 0; attr <= 9; ++attr) {   // Slash .. Dark: the ones the analysis shows
      ue::Params p(f);
      p.set<UObject*>("Actor", e);
      p.set<uint8_t>("attr", (uint8_t)attr);
      call(d, p);
      int a = p.ret<uint8_t>();
      std::string name = a >= 1 && a <= 5 ? attack_name(attr) : std::string();
      if (!name.empty()) by[a].push_back(name);
    }
    static const int order[] = {4, 5, 1, 2, 3};   // weak, resists, nulls, repels, drains
    for (int a : order) {
      if (by[a].empty()) continue;
      std::string s = std::string(strings::kAffinity[a]) + " ";
      for (size_t i = 0; i < by[a].size(); ++i) s += (i ? ", " : "") + by[a][i];
      out.push_back(s);
    }
    return out;
  }
  std::map<int, std::string> persona_names_;
  std::string persona_name(int id) {
    if (auto it = persona_names_.find(id); it != persona_names_.end()) return it->second;
    UObject* d = db();
    UFunction* f = d ? find_function(class_of(d), "GetPersonaName") : nullptr;
    if (!f) return {};
    ue::Params p(f);
    p.set<int32_t>("PersonaID", id);
    return persona_names_[id] = ftext_call(d, p);
  }
};
}  // namespace p3a::screens

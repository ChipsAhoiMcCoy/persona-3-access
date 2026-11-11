#pragma once
// The battle's own words and numbers, from its database (UBtlDataBaseComponent on BP_BtlTopUI_C: GetUnitName,
// GetUnitHP, GetSkillName, GetSkillCost, GatAnalizeAttr, GetAttackName, GetPersonaName, GetBadStatus, CheckSupport,
// GetSupportTurn). One instance (btl_words) serves the battle screen (battle.cpp), the battle's lists
// (battle_menus.cpp) and the detail buffer (battle_details.cpp).
#include "screens/util.h"
#include "core/battle_text.h"
#include "core/strings.h"
#include "party.h"
#include <format>
#include <map>
#include <string>
#include <vector>

namespace p3a::screens {
class BtlWords {
  Finder top_{"BP_BtlTopUI_C"};
 public:
  ue::UObject* db() { ue::UObject* t = top_.get(); return t ? ue::get<ue::UObject*>(t, "BtlDataBase") : nullptr; }
  // A database call taking one actor, returning an int.
  int unit_int(const char* fn, ue::UObject* a) {
    ue::UObject* d = db();
    ue::UFunction* f = d && a ? ue::find_function(ue::class_of(d), fn) : nullptr;
    if (!f) return -1;
    ue::Params p(f);
    p.set<ue::UObject*>("Actor", a);
    ue::call(d, p);
    return p.ret<int32_t>();
  }
  // A database call taking an actor and a type (an ailment, a buff): `none` without the database or the actor.
  template <typename T>
  T typed_call(const char* fn, ue::UObject* a, int type, T none) {
    ue::UObject* d = db();
    ue::UFunction* f = d && a ? ue::find_function(ue::class_of(d), fn) : nullptr;
    if (!f) return none;
    ue::Params p(f);
    p.set<ue::UObject*>("Actor", a);
    p.set<int32_t>("Type", type);
    ue::call(d, p);
    return p.ret<T>();
  }
  // The database's FText results are not released: releasing them deadlocked the game thread inside the text's
  // destructor (GetUnitName, 2026-10-27). Each distinct name is fetched once and cached instead, so the few
  // unreleased texts are bounded by the number of units, skills and attributes.
  std::string ftext_call(ue::UObject* d, ue::Params& p) {
    ue::call(d, p);
    ue::FText* t = p.at<ue::FText>("ReturnValue");
    return t ? ue::text(*t) : std::string();
  }
  std::map<ue::WeakObj::Key, std::string> unit_names_;   // by object index + full name
  std::map<int, std::string> skill_names_, attack_names_, persona_names_;
  // A name the database gives for an id (the function's one int parameter is `param`), fetched once.
  std::string cached_text(std::map<int, std::string>& cache, const char* fn, const char* param, int id) {
    if (auto it = cache.find(id); it != cache.end()) return it->second;
    ue::UObject* d = db();
    ue::UFunction* f = d ? ue::find_function(ue::class_of(d), fn) : nullptr;
    if (!f) return {};
    ue::Params p(f);
    p.set<int32_t>(param, id);
    return cache[id] = ftext_call(d, p);
  }
  std::string unit_name(ue::UObject* a) {
    auto key = ue::WeakObj::of(a).key();
    if (auto it = unit_names_.find(key); it != unit_names_.end()) return it->second;
    ue::UObject* d = db();
    ue::UFunction* f = d && a ? ue::find_function(ue::class_of(d), "GetUnitName") : nullptr;
    if (!f) return {};
    ue::Params p(f);
    p.set<ue::UObject*>("Actor", a);
    p.set<uint8_t>("InsertSpace", 1);
    return unit_names_[key] = ftext_call(d, p);
  }
  std::string skill_name(int id) { return cached_text(skill_names_, "GetSkillName", "skillId", id); }
  std::string attack_name(int attr) { return cached_text(attack_names_, "GetAttackName", "attr", attr); }
  std::string persona_name(int id) { return cached_text(persona_names_, "GetPersonaName", "PersonaID", id); }
  // "3 SP" / "121 HP": what the skill costs this member now (GetSkillCost has already turned a physical skill's
  // share of maximum HP into HP: Aigis at 633 HP read 121 for a 19% skill).
  std::string skill_cost(int id, ue::UObject* who) {
    ue::UObject* d = db();
    ue::UFunction* f = d ? ue::find_function(ue::class_of(d), "GetSkillCost") : nullptr;
    if (!f) return {};
    ue::Params p(f);
    p.set<int32_t>("skillId", id);
    p.set<ue::UObject*>("Action", who);
    ue::call(d, p);
    int v = p.ret<int32_t>();
    uint8_t* type = p.at<uint8_t>("cost");
    if (!type || *type == 0 || v <= 0) return {};
    return *type == 1 ? std::format("{} {}", v, strings::kHP) : std::format("{} {}", v, strings::kSP);
  }
  std::string hp_sp(ue::UObject* a) {
    return std::format("{} {} {} {}, {} {} {} {}", strings::kHP, unit_int("GetUnitHP", a), strings::kOf, unit_int("GetUnitMaxHP", a),
                       strings::kSP, unit_int("GetUnitSP", a), strings::kOf, unit_int("GetUnitMaxSP", a)) + ailments(a) + buffs(a);
  }
  // A turn's opening: "HP 99, SP 17" and what the member is under. The maxima are in their details.
  std::string hp_sp_short(ue::UObject* a) {
    return std::format("{} {}, {} {}", strings::kHP, unit_int("GetUnitHP", a), strings::kSP, unit_int("GetUnitSP", a)) + ailments(a) + buffs(a);
  }
  // BtlDataBase GetBadStatus(EBtlDataBadStatus): the same answer the info panel's icons draw from.
  bool has_ailment(ue::UObject* a, int type) { return typed_call<bool>("GetBadStatus", a, type, false); }
  // ABtlActor +0x38E: one signed byte per status bit, the turns the ailment can still last; a roll can end it sooner
  // (FUN_140fe2ef0 sets it, the decrement after FUN_140fd91e0 counts it down; checked live on Jin's Panic: 3, 2, 1, 0).
  // Bit i is 1 << i; the enum's first 12 are bits 0-11 (FUN_14102b0a0). Poison stays at 0 until it is cured, and Down
  // has no count: those read without one.
  int ailment_turns(ue::UObject* a, int type) {
    if (!ue::known_build()) return 0;   // the offset is this build's: on another the ailment reads without a count
    if (!a || type < 0 || type > 11) return 0;
    return *((int8_t*)a + 0x38E + type);
  }
  // ", Down, Poisoned": what a focused unit is suffering, Down first.
  std::string ailments(ue::UObject* a) {
    std::string s;
    if (has_ailment(a, 13)) s += std::string(", ") + strings::kAilments[13];
    for (int t = 0; t <= 11; ++t)
      if (has_ailment(a, t)) s += std::string(", ") + strings::kAilments[t];
    return s;
  }
  // BtlDataBase GetSupportTurn: turns left on a buff or debuff (BP_BtlTargetInfo marks 1 as its last).
  int support_turn(ue::UObject* a, int type) { return typed_call<int32_t>("GetSupportTurn", a, type, 0); }
  // BtlDataBase CheckSupport(EBtlDataSupportTable): the buff or debuff is on the unit.
  bool has_support(ue::UObject* a, int type) { return typed_call<bool>("CheckSupport", a, type, false); }
  // Stat buffs and debuffs the HUD shows as ATK / DEF / AGI arrows, in strings::kBuffs' order: EBtlDataSupportTable
  // 0/1 attack, 4/5 defense, 2/3 evasion and 6/7 accuracy (agility on the HUD); measured 2026-10-28 after Masukukaja
  // (2 and 6 set). CheckSupportDownUp answered false throughout. The type that holds HUD buff i now, -1 for none.
  int hud_buff(ue::UObject* a, int i) {
    static constexpr int first[6] = {0, 1, 4, 5, 2, 3}, second[6] = {-1, -1, -1, -1, 6, 7};
    if (has_support(a, first[i])) return first[i];
    return second[i] >= 0 && has_support(a, second[i]) ? second[i] : -1;
  }
  // Everything known about a unit, one line each: what the right stick (Ctrl+Up / Down) walks through. The first line
  // is who it is; what the game hides from a sighted player about a Shadow stays hidden (its stats, HP as a number).
  std::vector<std::string> details(ue::UObject* a) {
    std::vector<std::string> out;
    if (!a) return out;
    bool foe = ue::get_bool(a, "EnemyFlag");
    // GetUnitLevel is not a member's level ("Junpei, level 22" at level 9, 2026-11-06): the save data's, as on Stats.
    int level = unit_int("GetUnitLevel", a);
    party::Member m;
    if (!foe && party::member(unit_int("GetUnitID", a), m)) level = m.level;
    out.push_back(std::format("{}, {} {}", unit_name(a), strings::kLevel, level));
    int hp = unit_int("GetUnitHP", a), mx = unit_int("GetUnitMaxHP", a);
    if (foe) { if (mx > 0) out.push_back(std::format("{} {}%", strings::kHP, core::health_share(hp, mx))); }
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
      out.push_back(n > 0 ? std::format("{}, {} {}", strings::kAilments[t], strings::kUpTo, core::turns_text(n)) : std::string(strings::kAilments[t]));
    }
    if (!any) out.push_back(strings::kNoAilments);
    if (ue::call_ret<bool>(a, "IsGuard")) out.push_back(strings::kGuarding);
    size_t before = out.size();
    auto effect = [&](bool on, int type, const char* word) {
      if (!on) return;
      int n = support_turn(a, type);
      out.push_back(n > 0 ? std::format("{}, {}", word, core::turns_text(n)) : std::string(word));
    };
    // The HUD's stats (hud_buff), then what the info panel draws beside them, then the rest of EBtlDataSupportTable.
    for (int i = 0; i < 6; ++i) { int type = hud_buff(a, i); effect(type >= 0, type, strings::kBuffs[i]); }
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
  // ", Attack up, ...": the HUD's six stat buffs and debuffs on a unit.
  std::string buffs(ue::UObject* a) {
    std::string out;
    for (int i = 0; i < 6; ++i) if (hud_buff(a, i) >= 0) out += std::string(", ") + strings::kBuffs[i];
    return out;
  }
  // An enemy: name, health as a share (the game shows a bar), and the affinities the player has found out.
  std::string enemy(ue::UObject* e) {
    std::string s = unit_name(e);
    int hp = unit_int("GetUnitHP", e), mx = unit_int("GetUnitMaxHP", e);
    if (mx > 0) s += std::format(", {}%", core::health_share(hp, mx));
    s += ailments(e);
    s += buffs(e);
    for (auto& l : affinity_groups(e)) s += ", " + l;
    return s;
  }
  // A unit as a list's row: an enemy's line, a member's name with HP and SP.
  std::string unit_line(ue::UObject* u) {
    return ue::get_bool(u, "EnemyFlag") ? enemy(u) : std::format("{}, {}", unit_name(u), hp_sp(u));
  }
  // The affinities the player has found out, a line per kind: "weak to Fire, Ice". The game's names are "Fire
  // Attack", "Ice Attack": the word is dropped (core::short_attack), it was said with every target (the user's call,
  // 2026-11-11).
  std::vector<std::string> affinity_groups(ue::UObject* e) {
    std::vector<std::string> out;
    ue::UObject* d = db();
    ue::UFunction* f = d ? ue::find_function(ue::class_of(d), "GatAnalizeAttr") : nullptr;
    if (!f) return out;
    std::vector<std::string> by[7];
    for (int attr = 0; attr <= 9; ++attr) {   // Slash .. Dark: the ones the analysis shows
      ue::Params p(f);
      p.set<ue::UObject*>("Actor", e);
      p.set<uint8_t>("attr", (uint8_t)attr);
      ue::call(d, p);
      int a = p.ret<uint8_t>();
      std::string name = a >= 1 && a <= 5 ? core::short_attack(attack_name(attr)) : std::string();
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
};
// The one BtlWords: its name caches are shared, so each unreleased text is fetched once for the whole mod.
inline BtlWords& btl_words() { static BtlWords w; return w; }
}  // namespace p3a::screens

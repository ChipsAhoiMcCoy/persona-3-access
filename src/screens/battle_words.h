#pragma once
// The battle's own words and numbers, from its database (UBtlDataBaseComponent on BP_BtlTopUI_C: GetUnitName,
// GetUnitHP, GetSkillName, GetSkillCost, GatAnalizeAttr, GetAttackName, GetPersonaName, CheckSupport). Used by the
// battle screen (battle.cpp).
#include "screens/util.h"
#include "core/strings.h"
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
                       strings::kSP, unit_int("GetUnitSP", a), strings::kOf, unit_int("GetUnitMaxSP", a)) + buffs(a);
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
    s += buffs(e);
    UObject* d = db();
    UFunction* f = d ? find_function(class_of(d), "GatAnalizeAttr") : nullptr;
    if (!f) return s;
    std::vector<std::string> by[7];
    for (int attr = 0; attr <= 9; ++attr) {   // Slash .. Dark: the ones the analysis shows
      ue::Params p(f);
      p.set<UObject*>("Actor", e);
      p.set<uint8_t>("attr", (uint8_t)attr);
      call(d, p);
      int a = p.ret<uint8_t>();
      if (a >= 1 && a <= 5) by[a].push_back(attack_name(attr));
    }
    static const int order[] = {4, 5, 1, 2, 3};   // weak, resists, nulls, repels, drains
    for (int a : order) {
      if (by[a].empty()) continue;
      s += std::string(", ") + strings::kAffinity[a] + " ";
      for (size_t i = 0; i < by[a].size(); ++i) s += (i ? " " : "") + by[a][i];
    }
    return s;
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

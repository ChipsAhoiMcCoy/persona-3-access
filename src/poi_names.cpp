// What a point of interest is called: the game's own names for hit actors, NPCs, chests and Shadows.
// Read here:
//  - AFldHitCore: mNameIndex_ (the row KIN_<8 digits> of DT_FldDailyHitName / DT_FldDungeonHitName: Name, and Name2
//    while the row's story `flag` is on), mCheckIcon_ (the prompt's icon), mOffFlagName_.
//  - AFldNpcActor: mDispName_, else mNameIndex_ in DT_FldNpcName; AFldCmmActor (a Social Link partner): mUniqueId_.
//  - UGlobalWorkBpLib::GWorkFlagChk: the game's story-flag check, by flag name.
//  - UFldEnemyComp.EnemyType (EFldEnemyType) and AFldDungeonObjectActor.ObjType (EDungeonObjType), as value names.
#include "poi_internal.h"
#include "core/poi_text.h"
#include "core/strings.h"
#include "gametext.h"
#include "seh.h"
#include "social.h"
#include <format>

namespace p3a::poi {
using namespace ue;

const char* kind_name(Kind k) {
  switch (k) {
    case Kind::Exit: return strings::kPoiExit;
    case Kind::Person: return strings::kPoiPerson;
    case Kind::Shop: return strings::kPoiShop;
    case Kind::Door: return strings::kPoiDoor;
    case Kind::Enemy: return strings::kPoiShadow;
    case Kind::Event: return strings::kPoiEvent;
    case Kind::Fragment: return strings::kPoiFragment;
    default: return strings::kPoiThing;
  }
}

// The cast's actor names carry the romanised Japanese spelling ("Mituru", "Sanada", "Fuka", "Aegis"): the names the
// English game uses. Everyone else reads by the actor-name token.
static std::string npc_actor_name(UObject* npc) {
  std::string n = core::person_from_actor_name(name_of(npc));
  return n.empty() ? std::string(strings::kPoiPerson) : n;
}

// UGlobalWorkBpLib::GWorkFlagChk(FName, out EGWorkOnOffEnum) -- the game's own story-flag check.
bool flag_on(const std::string& flag) {
  UClass* lib = find_class("GlobalWorkBpLib");
  UFunction* fn = find_function(lib, "GWorkFlagChk");
  if (!fn) return false;
  // "FOO_12" is the FName FOO with number 13 (Unreal splits a numeric suffix without a leading zero off the name).
  FName name{fname_index(flag), 0};
  std::string base_name;
  int number = 0;
  if (core::numbered_name(flag, base_name, number))
    if (int32_t base = fname_index(base_name); base >= 0) name = {base, number};
  if (name.index < 0) return false;
  Params p(fn);
  p.set<FName>("FlagName", name);
  call(cdo_of(lib), p);
  uint8_t* v = p.at<uint8_t>("OnOff");
  return v && enum_name(prop_inner_object(find_prop(fn, "OnOff")), *v) == "On";
}

// Row KIN_<idx, 8 digits> of the first of these name tables that has it (FldNpcNameTableRow and the hit-name rows
// share Name / flag / Name2).
std::string kin_name(std::initializer_list<const char*> tables, int idx) {
  if (idx < 0) return {};
  std::string row = std::format("KIN_{:08}", idx);
  for (const char* tn : tables) {
    UObject* t = gametext::table(tn);
    uint8_t* r = t ? datatable_find(t, row) : nullptr;
    if (!r) continue;
    UStruct* rs = datatable_struct(t);
    std::string n = to_utf8(*(FString*)(r + offset_of(rs, "Name")));
    // A row's Name2 replaces Name while its story flag is on ("Go Elsewhere" -> "Head to the Dorm" with
    // FLD_NOW_STATION_TO_DORMITORY): that is the name the game shows, and it tells the player where the story goes.
    std::string flag = to_utf8(*(FString*)(r + offset_of(rs, "flag")));
    std::string n2 = to_utf8(*(FString*)(r + offset_of(rs, "Name2")));
    if (!flag.empty() && !n2.empty() && flag_on(flag)) return n2;
    if (!n.empty()) return n;
  }
  return {};
}
// A hit actor's own name: mNameIndex_ is the row KIN_<8 digits> of the daily or the dungeon hit-name table
// (10002006 = "Escalator", 10000001 = "Go Elsewhere"; verified at Iwatodai Station 2026-10-27).
std::string hit_name(UObject* hit) {
  return kin_name({"DT_FldDailyHitName", "DT_FldDungeonHitName"}, get<int32_t>(hit, "mNameIndex_", -1));
}
// An NPC's name as the game labels it: mDispName_, else row KIN_<mNameIndex_> of DT_FldNpcName (641000 = "Student",
// 1000 = "Kenji Tomochika"; verified at Gekkoukan 2026-10-27), else a cast name from the actor's own name.
std::string npc_name(UObject* npc) {
  if (auto* dn = field<FString>(npc, "mDispName_")) { std::string s = to_utf8(*dn); if (!s.empty()) return s; }
  // A Social Link partner (FldCmmActor): mUniqueId_ is the arcana counted from the Fool as 0 (6 with the flag
  // CMM_06LOVERS..., 18 with CMM_18MOON..., 2026-10-27).
  if (is_a(npc, "FldCmmActor")) {
    int id = get<int32_t>(npc, "mUniqueId_", -1) + 1;
    if (id >= 1 && id <= 22) {
      std::string n = social::person(id);
      if (!n.empty()) return std::format("{} ({})", n, social::arcana_name(id));
    }
  }
  std::string n = kin_name({"DT_FldNpcName"}, get<int32_t>(npc, "mNameIndex_", -1));
  return n.empty() ? npc_actor_name(npc) : n;
}

// A hit under the plain Check icon, sorted by the name the game gives it (the lists: core::check_kind).
Kind check_kind(const std::string& name) {
  switch (core::check_kind(name)) {
    case core::CheckKind::Shop: return Kind::Shop;
    case core::CheckKind::Door: return Kind::Door;
    default: return Kind::Thing;
  }
}

std::string icon_of(UObject* hit) { return enum_prop_name(hit, "mCheckIcon_"); }

// A Twilight Fragment lying in town: the hit the game names "Something Shining" over a glowing key-item mesh. They
// are switched off by a flag FLD_RESOURCE_SPOT<n> once picked up, and their scripts all give the same item. The two
// in the August full-moon base are the same thing under another flag, FLD_MOON_MILITARY_R_MEDAL_<n>_GOT ("medal" is
// the game's internal word for a Twilight Fragment; their scripts give that item too).
bool fragment_spot(UObject* hit) {
  auto* off = field<FName>(hit, "mOffFlagName_");
  if (!off || off->index <= 0) return false;
  std::string flag = fname(*off);
  return flag.starts_with("FLD_RESOURCE_SPOT") || flag.starts_with("FLD_MOON_MILITARY_R_MEDAL");
}

// A Tartarus chest's kind. Every chest shares one hit and one name ("Treasure Chest"); the kind is on the dungeon
// object the hit belongs to (FldDungeonObjectActor.ObjType, EDungeonObjType; its hits are m_pHitActorList). The
// locked ones cost Twilight Fragments to open: TBoxMedal1/2/3 take 1/2/3 (the chest's script, SUB_TBOX_MEDAL).
// Seen on Adamah floor 240 (2026-11-11): TBoxPrimField is a plain black chest, TBoxRare a silver case under purple
// sparks, TBoxJewelry a gold case under a gold glow (it held a Diamond). A locked one was seen listed on Adamah floors
// 247 and 255 (2026-11-11): "Locked Treasure Chest, 3 Twilight Fragments".
std::string chest_name(const std::string& type, const std::string& game_name) { return core::chest_name(type, game_name); }

// A roaming Shadow's kind, the game's own (UFldEnemyComp.EnemyType, EFldEnemyType, on the component the enemy
// manager holds beside the actor; the actor's class says the same: BP_FldEmNormal / Strong / Rare / Death). "Rare
// Shadow" and "the Reaper" are the game's words for them (Mitsuru's and Fuuka's lines); "Strong" is the type's name.
const char* shadow_name(UObject* actor, UObject* comp) {
  std::string t;
  guarded([&] { t = enum_prop_name(comp, "EnemyType"); });
  std::string cn = name_of(class_of(actor));
  auto is = [&](const char* type, const char* cls) { return t.empty() ? cn.find(cls) != std::string::npos : t == type; };
  if (is("Death", "FldEmDeath")) return strings::kPoiReaper;
  if (is("Rare", "FldEmRare")) return strings::kPoiShadowRare;
  if (is("Strong", "FldEmStrong")) return strings::kPoiShadowStrong;
  return strings::kPoiShadow;
}
}  // namespace p3a::poi

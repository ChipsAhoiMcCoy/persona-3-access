#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "ue/ue.h"

namespace p3a::persona {
// The game's Persona data (UDatPersona: TableName = names by id, TablePersona = arcana and base level by id) and the
// hero's stock (UGlobalWork unit 1, FDatUnitPersona: 12 entries of 0x30 -- p3rpc.nativetypes). Game thread.
struct Owned { int slot; int id; int level; uint16_t skills[8]; uint8_t stats[5]; };
std::string name(int id);                 // "Orpheus", or "" for an unknown id
int arcana(int id);                       // 1 Fool .. 22 Aeon (social::arcana_name), or 0
int base_level(int id);
std::vector<Owned> stock();               // the hero's Personas in stock order
int unit_level(int unit_id);              // the hero's (unit 1) level from the save data (FDatUnitWork +0x1C), else 0
int equipped();                           // the hero's equipped stock slot, or -1
std::string describe(int id, int level);   // "Orpheus, Fool, level 3"
std::string affinities(int id);            // "weak to Elec, resists Wind" (UDatAffinityTable), or ""
std::vector<std::string> affinity_groups(int id);   // the same, one entry per kind: {"weak to Elec", "resists Wind"}
// A dyad fusion's result as the Velvet Room computes it, or 0 when the pair cannot fuse. The arcana comes from the
// game's chart (NormalSpreadDataAsset: Data[low arcana - 1].Races[high arcana - 1]); the level target is
// (level A + level B) / 2 + 1 (BP_CombineCalc GetNormalSpreadLevel). Different arcana: the lowest Persona of the
// result arcana at or above the target; same arcana: the highest below it, never one of the two. Only Personas with
// flag 0 take part (0x2 = special fusion only, 0x200 = DLC, 0x8 = unused; the table, 2026-10-27).
int dyad_result(int a, int level_a, int b, int level_b);
std::string skill_name(int id);           // UDatSkillNameTable.Data[id] ("Agi")
std::string skill_help(int id);           // its help line (BMD_SkillHelp dialog id), empty if unknown
std::string skill_cost(int id);           // "4 SP", "8% HP" or "" (UDatSkillNormalTable.Data[id]: costtype 1 HP%, 2 SP)
// A Persona entry as the status screens hold it (APersonaStatusDraw +0x34C, FDatUnitPersonaEntry):
// "Nekomata, Magician, level 3, St 4, Ma 4, En 2, Ag 5, Lu 2. Skills: Agi, Rakunda"
std::string status_line(const uint8_t* entry);
// A stat as the status page shows it (0 St .. 4 Lu): the base +0x1C, plus +0x21 and +0x26 (incense, the fusion's
// bonus), at most 99 (FUN_1411acc90, p3rpc.nativetypes GetParamTotal).
int stat_total(const uint8_t* entry, int i);
int exp_to_next(const uint8_t* entry);    // EXP still needed for its next level (entry +8 EXP), -1 at 99 or unknown
int level_with_exp(const uint8_t* entry, int gain);   // the level it reaches with `gain` more EXP
// An entry as lines to step through one at a time: each stat ("Strength 12"), each skill with its cost, each kind of
// affinity ("Weak to Elec").
std::vector<std::string> detail_lines(const uint8_t* entry);
const uint8_t* stock_entry(int slot);     // the hero's stock entry in that slot (FDatUnitPersonaEntry), or null
// The skills a Persona learns by levelling (UDatPersonaGrowthTable.Data[id].skillEvent[16]: level above the base
// level (0 = innate), event 1, skill; measured 2026-10-31: Mithras, base 24, Agilao at +1 as its page's "next
// skill: level 25"), in table order (by level). `level` is absolute.
struct Learn { int level; int skill; };
std::vector<Learn> learnset(int id);
const uint8_t* shown_status();            // the entry the Persona status screen shows now, or null
const uint8_t* status_entry(ue::UObject* persona_status);   // that APersonaStatus actor's entry, or null
bool status_page_up(ue::UObject* persona_status);           // its plain status page is shown (+0x528 == 1)
void install();                           // /persona dev route
}  // namespace p3a::persona

#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace p3a::party {
// The party from the save data (UGlobalWork: FDatUnitWork per character, p3rpc.nativetypes): who is in it
// (GlobalWorkBpLib.GWorkGetPartyID), level, HP and SP with their maximums as the game computes them. Game thread.
struct Member { int id = 0; std::string name; int level = 0, hp = 0, max_hp = 0, sp = 0, max_sp = 0, exp = 0; };
std::string name(int id);              // first name ("Yukari"); the hero's own as the player entered it
std::vector<Member> members();         // the active party in order (leader first)
bool member(int id, Member& out);
// A party member by first name (case-insensitive; the camp draws "makoto" as entered), from the active party.
const uint8_t* persona_entry(int id);  // the member's equipped Persona entry (FDatUnitPersonaEntry), or null
uint8_t* global_work();                // the UGlobalWork object (the save data), or null
uint8_t* unit_work(int id);            // its FDatUnitWork for character id 1..10 (+0x1B0 + 0x2B4 * id), or null
bool flag(int id);                     // a story flag (GlobalWorkBpLib.GWorkFlagChkValue)
std::string line(const Member& m);     // "Yukari, level 2, HP 74 of 80, SP 61 of 65"
std::string summary();                 // every member's line
// Outside battle and the camp menu, a member's HP or SP changing is spoken (Auto-Recover, the entrance clock, field
// pickups): battles and the camp's own lists speak theirs.
void tick();
void install();                        // resolves the max HP/SP routines
}  // namespace p3a::party

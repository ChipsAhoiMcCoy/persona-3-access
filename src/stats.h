#pragma once
#include <string>
#include "ue/ue.h"

namespace p3a::stats {
// The hero's social stats, in the game's order: 0 Academics, 1 Charm, 2 Courage (game thread).
struct Stats { int points[3] = {}; int level[3] = {}; ue::UObject* names = nullptr; };
bool read(Stats& out);
std::string rank_name(const Stats& s, int stat);   // "Slacker"
std::string describe();                             // "Academics Slacker, Charm Plain, Courage Timid. 3000 yen"
int money();                                        // the wallet, or -1
void tick();                                        // announces a rank up
int last_up();                                      // the stat whose points rose last and no Stats up! panel has shown, or -1
void up_shown();                                    // a Stats up! panel has shown it
void* flow_command(const char* name);               // a flowscript command's function, by its name (P3R.exe's command table)
int flow_int_arg(uint8_t* ctx);                     // a flowscript command's first argument as an int (0 if none)
void install();                                     // finds GetHeroParameterHandle; /stats dev route
}  // namespace p3a::stats

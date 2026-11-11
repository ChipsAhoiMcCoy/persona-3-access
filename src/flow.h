#pragma once
#include <cstdint>

namespace p3a::flow {
// The game's flowscript commands (the script functions events call: CMM_RANKUP, ADD_GET_ITEM_LIST_MONEY, ...).
// A command is int64 fn(uint8_t* script_context); the game keeps them in a table of {fn, argc, name}.
using Command = int64_t (*)(uint8_t* ctx);
// A command's function, by its name in P3R.exe's command table; null when the name is not there.
void* command(const char* name);
// A command's first argument as an int (a float is truncated), read from the script context the command was called
// with; 0 when there is none or it is a script variable.
int int_arg(uint8_t* ctx);
}  // namespace p3a::flow

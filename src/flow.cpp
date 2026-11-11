// Flowscript commands. An entry of the game's command table is {fn, argc, name}: the name is a string in the exe's
// data, and the entry is found through the pointer to it. The argument reader (FUN_141274950): the script context's
// +0x34 argument count, +0x37+i a type byte (0/4 int, 1 float, 2/3 script variables), +0x60+i*8 the value.
#include "flow.h"
#include "seh.h"
#include "ue/scan.h"

namespace p3a::flow {
void* command(const char* name) {
  uintptr_t str = scan::find_string(name);
  if (!str) return nullptr;
  // The name is the entry's third field: its function is 16 bytes before the pointer to the name.
  uintptr_t slot = scan::find_pointer(str, 16);
  return slot ? *(void**)(slot - 16) : nullptr;
}

int int_arg(uint8_t* ctx) {
  int v = 0;
  guarded([&] {
    int i = *(int32_t*)(ctx + 0x34);   // the count: the first argument's type and value are in slot [count]
    if (i < 1 || i > 0x30) return;
    uint8_t type = ctx[0x37 + i];
    if (type == 0 || type == 4) v = *(int32_t*)(ctx + 0x60 + i * 8);
    else if (type == 1) v = (int)*(float*)(ctx + 0x60 + i * 8);
  });
  return v;
}
}  // namespace p3a::flow

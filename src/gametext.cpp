#include "gametext.h"
#include "devserver.h"
#include "watch.h"
#include "seh.h"
#include <format>
#include <map>

namespace p3a::gametext {
using namespace ue;

UObject* table(const char* name) {
  static std::map<std::string, WeakObj> cache;
  WeakObj& w = cache[name];
  if (UObject* t = w.resolve()) return t;
  // From the object watcher's list: a table not loaded here (the dungeon's, in town) was a full object scan per call.
  static const int tables = watch::want("DataTable", true);
  for (UObject* t : watch::all(tables)) if (name_of(t) == name) { w = WeakObj::of(t); return t; }
  return nullptr;
}

std::string system_text(int row) {
  UObject* t = table("DT_SystemTextName");
  uint8_t* r = t ? datatable_find(t, std::to_string(row)) : nullptr;
  if (!r) return {};
  return text(*(FText*)(r + offset_of(datatable_struct(t), "Name")));   // DatNameTable.Name is an FText
}

std::string time_zone(int tz) {
  // ECldTimeZone 1..9 -> DT_SystemTextName 120..129 (126 is "Daytime", used by no time zone)
  static const int rows[10] = {0, 120, 121, 122, 123, 124, 125, 127, 128, 129};
  if (tz < 1 || tz > 9) return {};
  return system_text(rows[tz]);
}

void install() {
  dev::route("/gametext", [](const dev::Request& r, int&) {
    int row = (int)dev::arg_int(r, "row", 120);
    return dev::on_game([row] {
      UObject* t = table("DT_SystemTextName");
      uint8_t* p = t ? datatable_find(t, std::to_string(row)) : nullptr;
      std::string s = std::format("table #{} {} row {} at {} name off {}\n", t ? index_of(t) : -1, t ? full_name(t) : "", row, (void*)p,
                                  t ? offset_of(datatable_struct(t), "Name") : -1);
      if (p) guarded([&] { s += "text: " + system_text(row) + "\n"; });
      return s;
    });
  }, "?row=N a DT_SystemTextName row, with where it was found");
}
}  // namespace p3a::gametext


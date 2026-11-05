#include "gametext.h"
#include "devserver.h"
#include "watch.h"
#include "hooks.h"
#include "msg.h"
#include "seh.h"
#include "screens/screen.h"
#include "speech.h"
#include "world.h"
#include "core/strings.h"
#include "ue/scan.h"
#include <windows.h>
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
    auto out = std::make_shared<std::string>("timeout\n");
    hooks::run_on_game_thread([out, row] {
      UObject* t = table("DT_SystemTextName");
      uint8_t* p = t ? datatable_find(t, std::to_string(row)) : nullptr;
      std::string s = std::format("table #{} {} row {} at {} name off {}\n", t ? index_of(t) : -1, t ? full_name(t) : "", row, (void*)p,
                                  t ? offset_of(datatable_struct(t), "Name") : -1);
      if (p) guarded([&] { s += "text: " + system_text(row) + "\n"; });
      *out = s;
    });
    return *out;
  }, "?row=N a DT_SystemTextName row, with where it was found");
  // The game's scene change as sleeping makes it: "now" moved first, the change queued on the UCalendar, then the
  // starter (FUN_141053e80: Setted_ -> Type_, opens UI 0x19 DayChange / 0x18 TimeChange; read 2026-10-31).
  dev::route("/calendar/change", [](const dev::Request& r, int&) {
    bool time = dev::arg(r, "type", "day") == std::string("time");
    int day = (int)dev::arg_int(r, "day", -1), tz = (int)dev::arg_int(r, "tz", 0);
    auto out = std::make_shared<std::string>("timeout\n");
    if (dev::arg_int(r, "end", 0)) {
      // The sequence ends it: FUN_141053c10 asks the screen to close (flag 1 -> 2), then FUN_141051360 each frame
      // (true while it closes; then Type_ None).
      hooks::run_on_game_thread([out] {
        UObject* c = watch::get(watch::want("Calendar"));
        if (!c) { *out = "no calendar\n"; return; }
        if (*field<uint32_t>(c, "mChangeFlag_") != 2) ((void (*)())(scan::exe_base() + 0x1053c10))();
        bool closing = ((bool (*)())(scan::exe_base() + 0x1051360))();
        *out = std::format("closing {} type {}\n", closing, *field<int8_t>(c, "mChangeType_"));
      });
      return *out;
    }
    hooks::run_on_game_thread([out, time, day, tz] {
      UClass* lc = find_class("CldBpFuncLib");
      UObject* l = lc ? cdo_of(lc) : nullptr;
      UObject* c = watch::get(watch::want("Calendar"));
      calendar::Now n = calendar::now();
      if (!l || !c || !n.valid) { *out = "no calendar\n"; return; }
      int to_day = time ? n.total : day >= 0 ? day : n.total + 1;
      int to_tz = tz ? tz : time ? n.time_zone + 1 : 2;
      if (UFunction* f = find_function(lc, "SetNowTotalDay")) { Params p(f); *p.at<int32_t>("TotalDay") = to_day; call(l, p); }
      if (UFunction* f = find_function(lc, "SetNowTimeZone")) { Params p(f); *p.at<uint8_t>("TimeZone") = (uint8_t)to_tz; call(l, p); }
      uint8_t* b = (uint8_t*)c;
      *(int32_t*)(b + offset_of(class_of(c), "mChangePrevDay_")) = n.total;
      b[offset_of(class_of(c), "mChangePrevTimeZone_")] = (uint8_t)n.time_zone;
      *(int32_t*)(b + offset_of(class_of(c), "mChangeNextDay_")) = to_day;
      b[offset_of(class_of(c), "mChangeNextTimeZone_")] = (uint8_t)to_tz;
      b[offset_of(class_of(c), "mChangeSetted_")] = time ? 1 : 2;
      ((void (*)())(scan::exe_base() + 0x1053e80))();
      *out = std::format("day {} tz {} -> day {} tz {}, type now {}\n", n.total, n.time_zone, to_day, to_tz,
                         (int)b[offset_of(class_of(c), "mChangeType_")]);
    });
    return *out;
  }, "?type=day|time[&day=N&tz=N] play the game's day / time change (dev: moves the date); ?end=1 (repeat) closes it");
}

const char* weekday(int y) {
  static const char* names[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  return y >= 0 && y < 7 ? names[y] : "";
}
}  // namespace p3a::gametext

namespace p3a::calendar {
using namespace ue;

static UFunction* fn(const char* name) {
  static std::map<std::string, UFunction*> cache;
  UFunction*& f = cache[name];
  if (!f || !is_live(f)) f = find_function(find_class("CldBpFuncLib"), name);
  return f;
}
static UObject* lib() { return cdo_of(find_class("CldBpFuncLib")); }

Now now() {
  Now n;
  UObject* l = lib();
  UFunction* md = fn("GetNowTotalToDayMon");
  if (!l || !md) return n;
  Params p(md); call(l, p);
  n.month = p.ret<int32_t>("Month"); n.day = p.ret<int32_t>("Day");
  if (UFunction* f = fn("GetNowTimeZone")) { Params q(f); call(l, q); n.time_zone = q.ret<uint8_t>(); }
  if (UFunction* f = fn("GetNowYoubi")) { Params q(f); call(l, q); n.weekday = q.ret<int32_t>(); }
  if (UFunction* f = fn("GetNowYasumi")) { Params q(f); call(l, q); n.holiday = q.ret<bool>(); }
  if (UFunction* f = fn("GetNowMoonAge")) { Params q(f); call(l, q); n.moon = q.ret<int32_t>(); }
  if (UFunction* f = fn("GetNowTotalDay")) { Params q(f); call(l, q); n.total = q.ret<int32_t>(); }
  n.valid = n.month >= 1 && n.month <= 12 && n.day >= 1;
  return n;
}

std::string moon_phase(int age) {
  // ECldMoonAge 0..29. Full at 14: April 6 reads 11, April 7 12, and the full moon is April 9 (measured 2026-10-31).
  // The quarters and the new moon are the ages the calendar marks (FUN_1412b9030): 6, 22 and 29.
  if (age < 0 || age > 29) return {};
  int i = age == 29 ? 0 : age < 6 ? 1 : age == 6 ? 2 : age < 14 ? 3 : age == 14 ? 4 : age < 22 ? 5 : age == 22 ? 6 : 7;
  return strings::kMoonPhases[i];
}

std::string moon_mark(int age) {
  return age == 6 || age == 14 || age == 22 || age == 29 ? moon_phase(age) : std::string();
}

std::string describe(const Now& n) {
  static const char* months[13] = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
  if (!n.valid) return {};
  std::string s = std::format("{} {}, {}", months[n.month], n.day, gametext::weekday(n.weekday));
  if (n.holiday) s += ", holiday";
  std::string tz = gametext::time_zone(n.time_zone);
  if (!tz.empty()) s += ", " + tz;
  return s;
}

// The game's scene changes (UCalendar, a transient object): sleeping queues one (mChangeSetted_ DayChange, with
// mChangePrevDay_ / mChangeNextDay_ and the next time zone; the game's "now" is already the new day), then
// mChangeType_ holds DayChange (2) while the moon and the days roll by, ~6 s, and goes back to None. A time of day
// passing (After School -> Evening) is TimeChange (1) the same way. Measured 2026-10-31 (April 6 -> 7: Setted_ with
// days 5 -> 6, EarlyMorning; a frame later Type_ DayChange and the Next fields cleared).
// Each change is spoken as its screen starts; days and times of day reached any other way (a load, a story jump)
// once the field is free again. What was last spoken is kept, so the field never repeats it.
void tick() {
  static const int cal_id = watch::want("Calendar");
  static int last_type = 0, next_tz = 0, said_day = -1, said_tz = -1;
  if (UObject* c = watch::get(cal_id)) {
    int8_t* type = field<int8_t>(c, "mChangeType_");
    int8_t* setted = field<int8_t>(c, "mChangeSetted_");
    int8_t* tz = field<int8_t>(c, "mChangeNextTimeZone_");
    if (setted && tz && *setted) next_tz = *tz;
    int t = type ? *type : 0;
    if (t != last_type && t == 2) {
      Now n = now();
      if (n.valid) {
        std::string line = describe(n);
        if (std::string m = moon_phase(n.moon); !m.empty()) line += ". " + m;
        speech::speak(line, true);
        said_day = n.total; said_tz = n.time_zone;
      }
    } else if (t != last_type && t == 1) {
      Now n = now();
      int to = next_tz ? next_tz : n.time_zone;
      if (std::string s = gametext::time_zone(to); !s.empty()) { speech::speak(s, true); said_tz = to; said_day = n.total; }
    }
    if (t != last_type && t) next_tz = 0;
    last_type = t;
  }
  static ULONGLONG last = 0;
  ULONGLONG now_ms = GetTickCount64();
  if (now_ms - last < 1000) return;
  last = now_ms;
  if (!world::player() || msg::current().blocking || screens::current() || last_type) return;
  Now n = now();
  if (!n.valid) return;
  bool first = said_day < 0;
  if (n.total == said_day && n.time_zone == said_tz) return;
  std::string s = n.total == said_day ? gametext::time_zone(n.time_zone) : describe(n);
  said_day = n.total; said_tz = n.time_zone;
  if (!first && !s.empty()) speech::speak(s, false);
}
}  // namespace p3a::calendar

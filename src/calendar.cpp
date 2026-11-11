#include "calendar.h"
#include "core/calendar_text.h"
#include "devserver.h"
#include "gametext.h"
#include "msg.h"
#include "screens/screen.h"
#include "speech.h"
#include "ue/scan.h"
#include "ue/ue.h"
#include "watch.h"
#include "world.h"
#include <format>

namespace p3a::calendar {
using namespace ue;

// CldBpFuncLib's getters, each run on the library's default object; class and functions are looked up by name per
// call (the lookups are cached). A getter that is missing leaves its field at the default.
Now now() {
  Now n;
  UClass* c = find_class("CldBpFuncLib");
  UObject* l = cdo_of(c);
  UFunction* md = c ? find_function(c, "GetNowTotalToDayMon") : nullptr;
  if (!l || !md) return n;
  Params p(md); call(l, p);
  n.month = p.ret<int32_t>("Month"); n.day = p.ret<int32_t>("Day");
  n.time_zone = call_ret<uint8_t>(l, "GetNowTimeZone", (uint8_t)n.time_zone);
  n.weekday = call_ret<int32_t>(l, "GetNowYoubi", n.weekday);
  n.holiday = call_ret<bool>(l, "GetNowYasumi", n.holiday);
  n.moon = call_ret<int32_t>(l, "GetNowMoonAge", n.moon);
  n.total = call_ret<int32_t>(l, "GetNowTotalDay", n.total);
  n.valid = n.month >= 1 && n.month <= 12 && n.day >= 1;
  return n;
}

std::string moon_phase(int age) { return core::moon_phase(age); }
std::string moon_mark(int age) { return core::moon_mark(age); }

std::string describe(const Now& n) {
  if (!n.valid) return {};
  return core::date_text(n.month, n.day, n.weekday, n.holiday, gametext::time_zone(n.time_zone));
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
  bool queued = false;   // a change is set and its screen has not started yet: the date is that screen's to say
  if (UObject* c = watch::get(cal_id)) {
    int8_t* type = field<int8_t>(c, "mChangeType_");
    int8_t* setted = field<int8_t>(c, "mChangeSetted_");
    int8_t* tz = field<int8_t>(c, "mChangeNextTimeZone_");
    if (setted && tz && *setted) next_tz = *tz;
    queued = setted && *setted;
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
  if (!world::player() || msg::current().blocking || screens::current() || last_type || queued) return;
  // The day and the time of day against what was last said, every tick (two getters); the whole date is read only
  // when one of them moved.
  UObject* l = cdo_of(find_class("CldBpFuncLib"));
  if (call_ret<int32_t>(l, "GetNowTotalDay", -1) == said_day && call_ret<uint8_t>(l, "GetNowTimeZone", 0) == said_tz) return;
  Now n = now();
  if (!n.valid) return;
  bool first = said_day < 0;
  if (n.total == said_day && n.time_zone == said_tz) return;
  std::string s = n.total == said_day ? gametext::time_zone(n.time_zone) : describe(n);
  said_day = n.total; said_tz = n.time_zone;
  if (!first && !s.empty()) speech::speak(s, false);
}

void install() {
  // The game's scene change as sleeping makes it: "now" moved first, the change queued on the UCalendar, then the
  // starter (FUN_141053e80: Setted_ -> Type_, opens UI 0x19 DayChange / 0x18 TimeChange; read 2026-10-31).
  dev::route("/calendar/change", [](const dev::Request& r, int&) {
    // The functions below are at fixed addresses of the build the mod was made for.
    if (!known_build()) return std::string("another build of the game\n");
    bool time = dev::arg(r, "type", "day") == std::string("time");
    int day = (int)dev::arg_int(r, "day", -1), tz = (int)dev::arg_int(r, "tz", 0);
    if (dev::arg_int(r, "end", 0)) {
      // The sequence ends it: FUN_141053c10 asks the screen to close (flag 1 -> 2), then FUN_141051360 each frame
      // (true while it closes; then Type_ None).
      return dev::on_game([] {
        UObject* c = watch::get(watch::want("Calendar"));
        if (!c) return std::string("no calendar\n");
        if (*field<uint32_t>(c, "mChangeFlag_") != 2) ((void (*)())(scan::exe_base() + 0x1053c10))();
        bool closing = ((bool (*)())(scan::exe_base() + 0x1051360))();
        return std::format("closing {} type {}\n", closing, *field<int8_t>(c, "mChangeType_"));
      });
    }
    return dev::on_game([time, day, tz] {
      UClass* lc = find_class("CldBpFuncLib");
      UObject* l = lc ? cdo_of(lc) : nullptr;
      UObject* c = watch::get(watch::want("Calendar"));
      Now n = now();
      if (!l || !c || !n.valid) return std::string("no calendar\n");
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
      return std::format("day {} tz {} -> day {} tz {}, type now {}\n", n.total, n.time_zone, to_day, to_tz,
                         (int)b[offset_of(class_of(c), "mChangeType_")]);
    });
  }, "?type=day|time[&day=N&tz=N] play the game's day / time change (dev: moves the date); ?end=1 (repeat) closes it");
}
}  // namespace p3a::calendar

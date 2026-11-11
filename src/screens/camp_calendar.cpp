// The camp's Calendar submenu as a bespoke menu (menu.h), modelled from the game's own objects.
// UCmpCalendar +0x3C open, its actor +0x48 (AUICmpCalendar): +0x2A8 0 on the day grid, 1 on the day's plans; the day
// cursor (UCmpCldCursor, actor +0x2F8): +0xDC the day as an absolute number, held between +0xE0 and +0xE4. Up / down
// move a week, left / right a day; Q / E a month. The draw (UICmpCalendarDraw) holds the selected date: +0x188 month,
// +0x190 year, +0x1A4 day (measured 2026-10-27). The mod lays the month out as the game does, weeks as rows of seven
// (Sunday first), and steps the game's cursor through the same moves.
// A day's events are the game's data, so any day reads without the game's cursor on it (the panel is drawn from the
// same rows, and while it crossfades the last day's text is still drawn): CldDateMessageDataAsset, the game's common
// resource 0x2B (FUN_140fc85f0 -> FUN_1415695e0). Data (+0x30) a TArray per month (index = month), each of 0x14-byte
// rows: +0 Key, +4 TotalDay (u16, the cursor's absolute day), +6 month, +7 day, +8 Period, +0xC MsgLabel (a dialog of
// BMD_CalendarDateMessages), +0x10 VisibleFlag (shown when 0 or set). As FUN_14104ec60; the day-changed handler
// (FUN_1412b1a30) hands the old and new day's rows to the draw (+0x1F0 / +0x200). No row: the file's dialog 0
// ("Nothing in particular"), as the game shows. (The part-time jobs under them, ArbeitInfoDataAsset at draw +0x1E0,
// are drawn as images.) Measured 2026-10-31: April 29 = TotalDay 28, MsgLabel 7 "Showa Day".
// The grid itself (FUN_1412b9030) marks each day's number red for a holiday and draws a moon on four ages of
// CldDateDataAsset's 6-byte day rows (+2 MoonAge): 6 first quarter, 14 full (animated), 22 last quarter, 29 new.
// The moves' gates: the actor's open timer +0x308 and its draw (+0x300), whose per-move cooldown is +0x250; the
// cursor's events are +0xE8 (the day changed) and +0x100 (the month changed).
#include "screens/menu.h"
#include "screens/util.h"
#include "screens/camp.h"
#include "bmdfile.h"
#include "core/bmd.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "calendar.h"
#include "textcap.h"
#include <algorithm>
#include <chrono>
#include <format>
#include <iterator>
#include <unordered_map>

namespace p3a::screens {
using namespace ue;

class CampCalendar : public MenuScreen {
  Finder menu_{"CmpCalendar"}, draw_{"UICmpCalendarDraw"};
  uint8_t* actor() {
    UObject* o = camp_open() ? menu_.get() : nullptr;
    return o && ((uint8_t*)o)[0x3C] ? *(uint8_t**)((uint8_t*)o + 0x48) : nullptr;
  }
  uint8_t* cursor_obj() { uint8_t* a = actor(); return a ? *(uint8_t**)(a + 0x2F8) : nullptr; }
  Mirror& days_ = mirror("days", [this] { uint8_t* a = actor(); uint8_t* c = cursor_obj(); return a && c && i32(a, 0x2A8) == 0 ? i32(c, 0xDC) : -1; });
  // The cursor's day as a date: (absolute day) -> year, month, day. The month and day are the game's own split of the
  // cursor (FUN_14104c5d0: day % 365 through a month-length table from April), so they never lag it. Only the school
  // year (April to March) comes from the draw, whose selected date follows the cursor a frame or two late; the
  // calendar spans one school year, so that never changes under a move. Reading the draw's date itself labelled the
  // month a day off on the frame of a move: across a month's end the old month was built with the new day in it, and
  // its old date was said before the new month's (April 30, then May 1; 2026-11-01).
  bool anchor(int& abs, std::chrono::sys_days& date) {
    uint8_t* c = cursor_obj();
    uint8_t* b = (uint8_t*)draw_.get();
    if (!c || !b) return false;
    int dm = *(int32_t*)(b + 0x188), dy = *(int32_t*)(b + 0x190);
    if (dm < 1 || dm > 12 || dy < 2000) return false;
    abs = i32(c, 0xDC);
    int m = 0, d = 0;
    fn<void (*)(int, int*, int*)>(0x104c5d0)(abs, &m, &d);
    if (m < 1 || m > 12 || d < 1 || d > 31) return false;
    int year = dy - (dm <= 3) + (m <= 3);
    date = std::chrono::sys_days{std::chrono::year_month_day{std::chrono::year{year}, std::chrono::month{(unsigned)m}, std::chrono::day{(unsigned)d}}};
    return true;
  }
  // A day's events, by absolute day (cached per opening: the rows and their flags hold while the calendar is open).
  std::unordered_map<int, std::string> events_;
  std::string events(int day) {
    if (auto it = events_.find(day); it != events_.end()) return it->second;
    void* res = fn<void* (*)()>(0xfc85f0)();
    UObject* a = res ? fn<UObject* (*)(void*, uint8_t)>(0x15695e0)(res, 0x2B) : nullptr;
    auto f = bmdfile::load("BMD_CalendarDateMessages");
    if (!a || !ue::is_a(a, "CldDateMessageDataAsset") || !f || !f->ok) return {};
    auto text = [&](int label) {
      return label >= 0 && label < (int)f->dialogs.size() && !f->dialogs[label].pages.empty()
                 ? core::clean_text(core::bmd::plain(f->dialogs[label].pages[0])) : std::string();
    };
    void* g = fn<void* (*)()>(0x122ac90)();
    uint8_t* months = *(uint8_t**)((uint8_t*)a + 0x30);
    int n = i32((uint8_t*)a, 0x38);
    std::string out;
    for (int m = 0; months && m < n; m++) {
      uint8_t* rows = *(uint8_t**)(months + m * 0x10);
      int count = i32(months + m * 0x10, 8);
      for (int r = 0; rows && r < count; r++) {
        uint8_t* row = rows + r * 0x14;
        uint32_t flag = *(uint32_t*)(row + 0x10);
        if (*(uint16_t*)(row + 4) != day || (flag && !(g && fn<bool (*)(void*, uint32_t)>(0x122a080)(g, flag)))) continue;
        std::string t = text(i32(row, 0xC));
        if (!t.empty() && out.find(t) == std::string::npos) out += (out.empty() ? "" : ", ") + t;
      }
    }
    if (out.empty()) out = text(0);
    for (auto& j : jobs(day)) out += std::format(", {}: {}", strings::kJob, j);
    return events_[day] = out;
  }
  // The part-time jobs a day takes, as the calendar lights them. The draw's list (+0x1E0, count +0x1E8) holds the
  // unlocked jobs (ArbeitInfoDataAsset rows, 0x18 bytes: +0 HelpMSGLabel, +4 OrderableTimes, +5 OrderableWeeks, +8
  // UnlockFLG); each row is drawn lit when FUN_14104bf20(_, job, 1) holds for the calendar's selected day (calendar
  // FUN_1410511c0(1) +0xC0, which the day-changed handler writes), grey otherwise. The job number is FUN_1415264e0's:
  // UnlockFLG 0x15 -> 0, 0x1B -> 2, else 1. The check runs here with that day written for the call and put back.
  std::vector<std::string> jobs(int day) {
    std::vector<std::string> out;
    UObject* d = draw_.get();
    uint8_t* cal = fn<uint8_t* (*)(char)>(0x10511c0)(1);
    if (!d || !cal) return out;
    uint8_t* rows = *(uint8_t**)((uint8_t*)d + 0x1E0);
    int n = i32((uint8_t*)d, 0x1E8);
    int held = i32(cal, 0xC0);
    for (int r = 0; rows && r < n && r < 8; r++) {
      uint8_t* row = rows + r * 0x18;
      int flag = i32(row, 8), label = i32(row, 0), times = row[4] & 3;
      uint32_t job = flag == 0x15 ? 0 : flag == 0x1B ? 2 : 1;
      *(int32_t*)(cal + 0xC0) = day;
      bool lit = fn<bool (*)(void*, uint32_t, char)>(0x104bf20)(nullptr, job, 1);
      *(int32_t*)(cal + 0xC0) = held;
      if (!lit || label < 0 || label >= (int)std::size(strings::kJobs)) continue;
      out.push_back(times ? std::format("{}, {}", strings::kJobs[label], strings::kJobTimes[times]) : strings::kJobs[label]);
    }
    return out;
  }
  // The day's plans list (mode 1) is read as drawn.
  std::string plans(unsigned max_age = 200) {
    std::vector<textcap::Item> cols;
    for (auto& t : textcap::snapshot(max_age)) if (t.source == 2 && t.x > 1000 && t.y > 550 && (t.color & 0xff) && core::has_words(t.text)) cols.push_back(t);
    std::sort(cols.begin(), cols.end(), [](auto& a, auto& b) { return a.y < b.y; });
    std::string out;
    for (auto& t : cols) if (out.find(core::clean_text(t.text)) == std::string::npos) out += (out.empty() ? "" : ", ") + core::clean_text(t.text);
    return out;
  }
 public:
  CampCalendar() {
    // The cursor's move (FUN_1412cc8d0): the day written, the month-changed event when it crosses one, the sound, the
    // day-changed event (the draw's highlight and selected date follow it). Moves wait for the grid's open timer and
    // the draw's per-move cooldown (draw +0x250 against FUN_1412bfdc0), as the game's do.
    days_.ready = [this] {
      uint8_t* a = actor();
      uint8_t* d = a && i32(a, 0x2A8) == 0 ? *(uint8_t**)(a + 0x300) : nullptr;
      if (!d) return false;
      void* layer = *(void**)(fn<uint8_t* (*)()>(0x122ac90)() + 0x308A0);
      return open_timer_done(*(float*)(a + 0x308), layer) && *(float*)(d + 0x250) > fn<int (*)(void*)>(0x12bfdc0)(d) * (1.f / 30);
    };
    days_.set = [this](int i) {
      uint8_t* c = cursor_obj();
      if (!c || i < i32(c, 0xE0) || i > i32(c, 0xE4)) return false;
      int old = i32(c, 0xDC);
      int m0 = 0, m1 = 0, x = 0;
      auto split = fn<void (*)(int, int*, int*)>(0x104c5d0);
      auto fire = fn<void (*)(void*, int, int)>(0x12b32d0);
      split(old, &m0, &x);
      *(int32_t*)(c + 0xDC) = i;
      split(i, &m1, &x);
      if (m0 != m1) fire(c + 0x100, m0, m1);
      cursor_sound();
      fire(c + 0xE8, old, i);
      return true;
    };
  }
  const char* id() const override { return "camp_calendar"; }
  int priority() const override { return 17; }
  // F on a day with plans opens them as a list on the right (mode 1): the game's keys; the plans are read as they change.
  bool active() override { uint8_t* a = actor(); return a && cursor_obj(); }
  bool holds_keys() override { uint8_t* a = actor(); return a && i32(a, 0x2A8) == 0; }
  std::string plans_said_;
  void on_tick() override {
    uint8_t* a = actor();
    if (!a || i32(a, 0x2A8) != 1) { plans_said_.clear(); return; }
    std::string p = plans();
    if (!p.empty() && p != plans_said_) { plans_said_ = p; say(p, true); }
  }
  // Today's absolute day (GetNowTotalDay counts as the cursor does), read on entry: the date holds while it is open.
  int today_ = -1;
  void on_enter() override {
    events_.clear(); today_ = calendar::now().total;
    screens::set_keys([this] { uint8_t* a = actor(); return std::string(a && i32(a, 0x2A8) == 1 ? strings::kCalendarPlansKeys() : strings::kCalendarKeys()); });
  }
  std::string title() override { return strings::kCampItems[7]; }
  void build(ui::GraphBuilder& b) override {
    int abs; std::chrono::sys_days date;
    uint8_t* c = cursor_obj();
    if (!c || !anchor(abs, date)) return;
    int lo = i32(c, 0xE0), hi = i32(c, 0xE4);
    std::chrono::year_month_day ymd{date};
    auto first = std::chrono::sys_days{ymd.year() / ymd.month() / std::chrono::day{1}};
    auto last = std::chrono::sys_days{ymd.year() / ymd.month() / std::chrono::last};
    std::string key = std::format("days.{}", (unsigned)ymd.month());
    days_.key = key;
    b.begin_stop(key);
    // Raw nodes with the game's own moves as edges: left / right a day, up / down a week.
    std::vector<int> ids;
    for (auto di = first; di <= last; di += std::chrono::days{1}) {
      int i = abs + (int)(di - date).count();
      if (i < lo || i > hi) continue;
      std::chrono::year_month_day d{di};
      calendar::Now n;
      n.month = (int)(unsigned)d.month(); n.day = (int)(unsigned)d.day(); n.weekday = (int)std::chrono::weekday{di}.c_encoding(); n.valid = true;
      // The grid's marks for the day (FUN_1412b9030): the number red for a holiday (FUN_1412b0d50: 2 red, 1 blue for
      // Saturday; a Sunday is always red), and a moon on the quarters, the full moon and the new moon (FUN_141050530,
      // the day's ECldMoonAge from CldDateDataAsset).
      n.holiday = n.weekday != 0 && fn<uint8_t (*)(void*, int)>(0x12b0d50)(nullptr, i) == 2;
      std::string label = calendar::describe(n);
      if (std::string m = calendar::moon_mark(fn<uint8_t (*)(int)>(0x1050530)(i)); !m.empty()) label += ", " + m;
      if (i == today_) label += std::format(", {}", strings::kToday);
      auto v = game_item(days_, i, [label] { return label; }, [this, i] { return events(i); });
      describe_with(v, [this, i] { return events(i); });
      b.add_node(slot(key, i), v);
      ids.push_back(i);
    }
    auto has = [&](int i) { return std::find(ids.begin(), ids.end(), i) != ids.end(); };
    for (int i : ids) {
      if (has(i + 1)) { b.connect(slot(key, i), ui::GraphDir::Right, slot(key, i + 1)); b.connect(slot(key, i + 1), ui::GraphDir::Left, slot(key, i)); }
      if (has(i + 7)) { b.connect(slot(key, i), ui::GraphDir::Down, slot(key, i + 7)); b.connect(slot(key, i + 7), ui::GraphDir::Up, slot(key, i)); }
    }
    if (int cc = days_.cursor(); cc >= 0) b.set_start(slot(key, cc));
  }
};

std::unique_ptr<Screen> make_camp_calendar() { return std::make_unique<CampCalendar>(); }
}  // namespace p3a::screens

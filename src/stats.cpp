// The hero's social stats (Academics, Charm, Courage). Native only: GetHeroParameterHandle() (found by the call-site
// signature p3rpc.socialStatTracker uses) returns a UHeroParameterHandle: +0x28 points[3] ({int, int points}), +0x30
// UHeroParameterDataAsset (+0x30 TArray of per-stat tables, each a TArray<int> of points needed per rank), +0x38 the
// rank-name DataTable.
#include "stats.h"
#include "core/strings.h"
#include "devserver.h"
#include "flow.h"
#include "hooks.h"
#include "log.h"
#include "party.h"
#include "seh.h"
#include "screens/screen.h"
#include "speech.h"
#include <windows.h>
#include "ue/scan.h"
#include <cstring>
#include <format>
#include <memory>

namespace p3a::stats {
using namespace ue;

// The stat whose points rose last, until a Stats up! panel has shown it (screens/stats_up.cpp takes it).
static int g_last_up = -1;
int last_up() { return g_last_up; }
void up_shown() { g_last_up = -1; }


using GetHandle_t = uint8_t* (*)();
static GetHandle_t g_get_handle;

// The flowscript command ADD_GET_ITEM_LIST_MONEY(amount) queues yen on the item-get notice the next OPEN_GET_ITEM
// shows; the wallet only changes when the player confirms it. Hooked to note the amount, so the wallet's change that
// follows is not said again (the notice itself is read by screens/get_item.cpp).
static flow::Command g_money_cmd;
static int g_told_money = 0;   // a money notice's amount, noted until the wallet takes it

static int64_t money_cmd_hook(uint8_t* ctx) {
  int amount = flow::int_arg(ctx);
  if (amount > 0) {   // the notice says it (screens/get_item.cpp reads its list); the wallet change is then not said
    g_told_money = amount;
  }
  return g_money_cmd(ctx);
}

// The money panel: CALL_MONEY_PANEL draws the wallet while a script asks the player to pay, SET_ADD_MONEY_PLANS(n)
// draws a planned change under it (minus the price at the TV shop's "Buy / Don't buy", BF_OBJ_MAIL_ORDER, and the
// other scripts that ask a price), CLOSE_MONEY_PANEL takes it away. The panel itself (AUIMiscMoneyDraw) is native and
// unreflected: what the script asked is noted here, and the choice that follows says it (msg.cpp read_choices).
static flow::Command g_panel_open_cmd, g_panel_close_cmd, g_plan_cmd;
static bool g_panel = false;
static int g_plan = 0;
static int64_t panel_open_hook(uint8_t* ctx) { g_panel = true; g_plan = 0; return g_panel_open_cmd(ctx); }
static int64_t panel_close_hook(uint8_t* ctx) { g_panel = false; g_plan = 0; return g_panel_close_cmd(ctx); }
static int64_t plan_hook(uint8_t* ctx) {
  g_plan = flow::int_arg(ctx);
  return g_plan_cmd(ctx);
}

std::string price_line() {
  if (!g_panel || g_plan >= 0) return {};
  int m = money();
  std::string s = std::format("{} {} {}", strings::kPrice, -g_plan, strings::kYen);
  if (m >= 0) s += std::format(", {} {} {}", strings::kYouHave, m, strings::kYen);
  return s;
}

static uint8_t* handle() {
  uint8_t* h = nullptr;
  if (g_get_handle) guarded([&] { h = g_get_handle(); });
  return h;
}

bool read(Stats& out) {
  uint8_t* h = handle();
  if (!h) return false;
  return guarded([&] {
    auto* pts = *(int32_t**)(h + 0x28);
    uint8_t* asset = *(uint8_t**)(h + 0x30);
    auto* tables = (TArray<TArray<int32_t>>*)(asset + 0x30);
    out.names = *(UObject**)(h + 0x38);
    for (int i = 0; i < 3; ++i) {
      out.points[i] = pts[i * 2 + 1];
      // Rank r (1-based) is reached at the running sum of the table's first r entries.
      int need = 0, level = 0;
      if (i < tables->num) {
        auto& t = tables->data[i];
        for (int r = 0; r < t.num && r < 6; ++r) { need += t.data[r]; if (out.points[i] >= need) level = r + 1; }
      }
      out.level[i] = level;
    }
  });
}

// DT_Hero_Parameter_Name: rows Level1..Level6, one column per stat (Brain, charm, Courage) -- the game's own words.
std::string rank_name(const Stats& s, int stat) {
  static const char* cols[3] = {"Brain", "charm", "Courage"};
  std::string out;
  if (s.names && stat >= 0 && stat < 3)
    guarded([&] {
      uint8_t* row = datatable_find(s.names, std::format("Level{}", s.level[stat]));
      FField* p = row ? find_prop(datatable_struct(s.names), cols[stat]) : nullptr;
      if (!p) return;
      out = value_string(p, row, 1);
      if (out.size() >= 2 && out.front() == '"' && out.back() == '"') out = out.substr(1, out.size() - 2);
    });
  return out.empty() ? std::format("{} {}", strings::kRank, s.level[stat]) : out;
}

// A stat that went up is announced when its points change (the game's "Stats up!" screen is sprites only): with
// the new rank's name when it ranked up.
void tick() {
  static Stats prev;
  static bool have = false;
  // The wallet: money notices ("Got 5,000 yen") are sprites, and shops take money silently.
  static int prev_money = -1;
  int m = money();
  bool load = screens::loading();   // a save loading: take the new values silently
  if (load) { have = false; prev_money = -1; g_panel = false; }
  // A money notice says its own gain (screens/get_item.cpp); the game pays on its confirm.
  bool told = g_told_money > 0 && m - prev_money == g_told_money;
  if (m >= 0 && prev_money >= 0 && m != prev_money) g_told_money = 0;
  // Returning to the title empties the wallet a moment before the title shows: not spending.
  if (m >= 0 && prev_money >= 0 && m != prev_money && !load && m != 0 && !told)
    speech::speak(m > prev_money ? std::format("{} {} {}", strings::kGot, m - prev_money, strings::kYen)
                                 : std::format("{} {} {}", strings::kSpent, prev_money - m, strings::kYen), false);
  if (m >= 0) prev_money = m;
  Stats s;
  if (!read(s)) { have = false; return; }
  if (have && !load)
    for (int i = 0; i < 3; ++i) {
      if (s.points[i] > prev.points[i]) g_last_up = i;
      if (s.level[i] > prev.level[i])
        speech::speak(std::format("{} {} {}, {}", strings::kSocialStats[i], strings::kRankUp, s.level[i], rank_name(s, i)), false);
      else if (s.points[i] > prev.points[i])
        speech::speak(std::format("{} {}", strings::kSocialStats[i], strings::kStatUp), false);
    }
  prev = s;
  have = true;
}

// UGlobalWorkBpLib::GWorkGetMoney -- the wallet.
int money() {
  Params p = party::gwork("GWorkGetMoney");
  return p.fn ? p.ret<int32_t>() : -1;
}

std::string describe() {
  Stats s;
  if (!read(s)) return {};
  std::string out;
  for (int i = 0; i < 3; ++i) out += std::format("{}{} {}", i ? ", " : "", strings::kSocialStats[i], rank_name(s, i));
  if (int m = money(); m >= 0) out += std::format(". {} {}", m, strings::kYen);
  return out;
}

void install() {
  uintptr_t cs = scan::find("E8 ?? ?? ?? ?? 48 89 45 ?? 4C 8B E8 48 85 C0 0F 84 ?? ?? ?? ?? 8B 40 ??");
  if (cs) g_get_handle = (GetHandle_t)scan::rip(cs + 1);
  log::writef("stats: GetHeroParameterHandle={:#x}", g_get_handle ? (uintptr_t)g_get_handle - scan::exe_base() : 0);
  g_money_cmd = (flow::Command)flow::command("ADD_GET_ITEM_LIST_MONEY");
  log::writef("stats: ADD_GET_ITEM_LIST_MONEY={:#x}", g_money_cmd ? (uintptr_t)g_money_cmd - scan::exe_base() : 0);
  if (g_money_cmd) hooks::attach((void**)&g_money_cmd, (void*)&money_cmd_hook, "ADD_GET_ITEM_LIST_MONEY");
  struct { flow::Command* fn; void* hook; const char* name; } panel[] = {
      {&g_panel_open_cmd, (void*)&panel_open_hook, "CALL_MONEY_PANEL"},
      {&g_panel_close_cmd, (void*)&panel_close_hook, "CLOSE_MONEY_PANEL"},
      {&g_plan_cmd, (void*)&plan_hook, "SET_ADD_MONEY_PLANS"}};
  for (auto& p : panel)
    if ((*p.fn = (flow::Command)flow::command(p.name))) hooks::attach((void**)p.fn, p.hook, p.name);
  dev::route("/stats", [](const dev::Request&, int&) {
    return dev::on_game([] {
      Stats s;
      if (!read(s)) return std::string("no hero parameters\n");
      std::string o;
      for (int i = 0; i < 3; ++i) o += std::format("stat {} points {} level {}\n", i, s.points[i], s.level[i]);
      o += std::format("names table: {}\n", s.names ? full_name(s.names) : "null");
      if (s.names) for (auto& r : datatable_rows(s.names)) o += "  " + r.name + ": " + struct_string(datatable_struct(s.names), r.data, 1) + "\n";
      return o;
    });
  }, "the hero's social stats: points, ranks, rank-name table");
}
}  // namespace p3a::stats

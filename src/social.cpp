// Social links. Native only: UCommunityHandler +0x28 holds 25 pointers to per-link records {u16 arcana id, ...,
// progress* at +8: u16 rank, u16 points} (id 1 Fool .. 22 Aeon; measured 2026-10-27, points from rirurin's
// p3rpc.nativetypes CmmEntry). A link is reversed while story flag 0x1000003F + id is on (p3rpc.slplus
// CmmCheckReverse). The rank-up screens ("Thou hast established a new bond", "Rank 1 Fool, S.E.E.S.") are sprites:
// screens/rank_up.cpp reads the card's own rank and link and tells this module, which otherwise announces changes
// from this data. Names: DT_CommunityFormat_Name row "<id 2 digits><Japanese arcana>", CommunityName.
// Ranking up: UCommunityHandler +0xF0 CommunityEventDataAsset, +0x30 Tables[arcana] (0x30 each: NotRankUpTable,
// RankUpTable {s16 rank, s16 points, s16 major, s16 minor, s8 academics, charm, courage, u32 flags[4]} of 0x1C,
// LoverRankUpTable). A RankUpTable row is the event that reaches its rank: rank 1 carries the stats a link needs to
// begin (Empress Academics 6, Lovers Charm 6, Tower Courage 3), and the points (15-55) are per rank, the count
// starting over each time (read 2026-10-30). Story links (Fool, Death, Judgement) have no hangouts (2 NotRankUp rows).
#include "social.h"
#include "watch.h"
#include "core/strings.h"
#include "devserver.h"
#include "gametext.h"
#include "hooks.h"
#include "log.h"
#include "msg.h"
#include "seh.h"
#include "screens/screen.h"
#include "speech.h"
#include "stats.h"
#include "ue/scan.h"
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <format>
#include <memory>

namespace p3a::social {
using namespace ue;

static UObject* handler() {
  static const int id = watch::want("CommunityHandler");
  return watch::get(id);
}

std::vector<Link> read() {
  std::vector<Link> out;
  UObject* h = handler();
  if (!h) return out;
  guarded([&] {
    auto** recs = (uint8_t**)((uint8_t*)h + 0x28);
    for (int i = 0; i < 25; ++i) {
      uint8_t* r = recs[i];
      if (!r) continue;
      // +8 points at the link's progress: u16 rank (Fool went 1 -> 2 after the first Tartarus visit; Magician became
      // 1 while +4 stayed 0 -- +4 is not "established"; 2026-10-27), then u16 points toward the next rank.
      uint16_t* prog = *(uint16_t**)(r + 8);
      int id = r[0], rank = prog ? prog[0] : 0, points = prog ? prog[1] : 0;
      if (id >= 1 && id <= 22) out.push_back(Link{id, rank, points});
    }
  });
  std::sort(out.begin(), out.end(), [](const Link& a, const Link& b) { return a.arcana < b.arcana; });
  return out;
}

std::string arcana_name(int id) { return id >= 1 && id <= 22 ? strings::kArcana[id] : std::to_string(id); }

// GlobalWorkBpLib.GWorkFlagChkValue(Index): one story flag.
static bool flag(int id) {
  static UClass* lib = nullptr;
  static UFunction* fn = nullptr;
  if (!fn || !is_live(fn)) { lib = find_class("GlobalWorkBpLib"); fn = lib ? find_function(lib, "GWorkFlagChkValue") : nullptr; }
  if (!fn) return false;
  Params p(fn);
  p.set<int32_t>("Index", id);
  call(cdo_of(lib), p);
  uint8_t* v = p.at<uint8_t>("OnOff");
  return v && enum_name(prop_inner_object(find_prop(fn, "OnOff")), *v) == "On";
}
bool reversed(int id) { return id >= 1 && id <= 22 && flag(0x1000003F + id); }

// A column of the arcana's row in DT_CommunityFormat_Name (rows "<id 2 digits><Japanese arcana>").
static std::string community_field(int id, const char* column) {
  UObject* t = gametext::table("DT_CommunityFormat_Name");
  if (!t) return {};
  std::string prefix = std::format("{:02}", id), out;
  for (auto& r : datatable_rows(t)) {
    if (r.name.rfind(prefix, 0) != 0) continue;
    FField* p = find_prop(datatable_struct(t), column);
    if (p) guarded([&] { out = value_string(p, r.data, 1); });
    break;
  }
  if (out.size() >= 2 && out.front() == '"') out = out.substr(1, out.size() - 2);
  return out == "None" ? std::string() : out;
}
// The link's partner as the game names it (CommunityName: "SEES", "Classmate", "Fuuka Yamagishi").
std::string partner(int id) { return community_field(id, "CommunityName"); }

// The person as the camp shows them: name A until the link is formed ("Jersey-Wearing Student"), B after
// ("Kazushi Miyamoto").
std::string person(int id) {
  int rank = 0;
  for (auto& l : read()) if (l.arcana == id) rank = l.rank;
  std::string n = community_field(id, rank > 0 ? "CampDispCommunityCharacterNameB" : "CampDispCommunityCharacterNameA");
  return n.empty() ? partner(id) : n;
}

// The field prompt over a Social Link partner shows their arcana and rank (AUIMiscCheckDraw: DT_UILayout_MiscCheck
// ARCANA_NAME, RANK): "Magician, rank 3" for the person of an established link, else "".
std::string partner_rank(const std::string& name) {
  if (name.empty()) return {};
  // The camp names some partners by first name only ("Maiko", "Bebe"): those match the field label's first word. The
  // Old Couple's link names neither of them (their prompt gets nothing).
  auto first = [](const std::string& s) { return s.substr(0, s.find(' ')); };
  for (auto& l : read())
    if (std::string p = l.rank > 0 ? person(l.arcana) : std::string();
        !p.empty() && (p == name || (p.find(' ') == std::string::npos && p == first(name))))
      return std::format("{}, {} {}", arcana_name(l.arcana), strings::kRank, l.rank >= 10 ? std::string(strings::kRankMax) : std::to_string(l.rank));
  return {};
}

static std::string name_rank(int arcana, int rank) {
  std::string p = partner(arcana);
  std::string r = rank >= 10 ? strings::kRankMax : std::to_string(rank);
  return std::format("{}{}, {} {}", arcana_name(arcana), p.empty() ? "" : " (" + p + ")", strings::kRank, r);
}

// The points the link needs for its next rank, or -1 when it ranks up through the story or is at MAX. Of several rows
// for one rank (romance and story variants) the lowest whose flag, if any, is on.
static int points_needed(const Link& l) {
  if (l.rank < 1 || l.rank >= 10) return -1;
  UObject* h = handler();
  int need = -1;
  if (h) guarded([&] {
    UObject* a = *(UObject**)((uint8_t*)h + 0xF0);
    if (!a || !is_live(a)) return;
    auto* tables = (TArray<uint8_t>*)((uint8_t*)a + 0x30);
    if (!tables->data || l.arcana >= tables->num) return;
    uint8_t* t = tables->data + l.arcana * 0x30;
    if (((TArray<uint8_t>*)t)->num <= 2) return;   // a story link
    auto* rows = (TArray<uint8_t>*)(t + 0x10);
    if (!rows->data || rows->num <= 0 || rows->num > 64) return;
    for (int i = 0; i < rows->num; ++i) {
      uint8_t* r = rows->data + i * 0x1C;
      int rank = *(int16_t*)r, pts = *(int16_t*)(r + 2);
      uint32_t f = *(uint32_t*)(r + 0xC);
      if (rank != l.rank + 1 || (f && !flag((int)f))) continue;
      if (need < 0 || pts < need) need = pts;
    }
  });
  return need;
}

bool ready(const Link& l) {
  int need = points_needed(l);
  return need >= 0 && l.points >= need && !reversed(l.arcana);
}

std::string describe(const Link& l) {
  std::string s = name_rank(l.arcana, l.rank);
  if (l.rank > 0 && reversed(l.arcana)) s += std::string(", ") + strings::kReversed;
  else if (ready(l)) s += std::string(", ") + strings::kReadyToRankUp;
  return s;
}

std::string summary() {
  std::string s;
  int n = 0;
  for (auto& l : read()) if (l.rank > 0) { s += (n++ ? ". " : "") + describe(l); }
  return n ? std::format("{}: {}. {}", strings::kSocialLinks, n, s) : strings::kNoSocialLinks;
}

static std::string g_last_change;

// The rank card comes from the flowscript command CMM_RANKUP(arcana, mode, 0) (every Social Link event calls it;
// mode 0 new link, 1 rank up, 2 MAX). Hooked: the link it names has its card coming in this conversation, and a rank
// change of that link (the data may move before or after the command) is the card's to speak.
static bool g_card_due[23] = {};
static int g_rankup_arcana = 0;      // the link of the last CMM_RANKUP: the card's when its own number is unreadable
using FlowCmd_t = int64_t (*)(uint8_t*);
static FlowCmd_t g_rankup_cmd;
static int64_t rankup_hook(uint8_t* ctx) {
  int a = stats::flow_int_arg(ctx);
  // The command runs every frame while its card is up (it waits on it): noted once.
  if (a >= 1 && a <= 22 && !g_card_due[a]) { g_card_due[a] = true; g_rankup_arcana = a; log::writef("social: CMM_RANKUP {}", arcana_name(a)); }
  return g_rankup_cmd(ctx);
}
static int g_prev[23] = {};          // rank per link at the last tick
static bool g_prev_rev[23] = {};

Card card(int arcana, int rank) {
  if (arcana < 1 || arcana > 22) arcana = g_rankup_arcana;
  if (arcana < 1 || arcana > 22)
    return {strings::kSocialLinkUp, rank > 0 ? std::format("{} {}", strings::kRank, rank) : strings::kSocialLinks};
  g_card_due[arcana] = true;
  if (rank <= 0) rank = std::max(g_prev[arcana], 1);
  const char* what = reversed(arcana) ? strings::kSocialLinkReversed
                   : g_prev_rev[arcana] ? strings::kSocialLinkRestored
                   : rank == 1 ? strings::kNewSocialLink
                   : rank >= 10 ? strings::kSocialLinkMaxed : strings::kSocialLinkUp;
  // The card names the arcana and rank; the partner as the camp's list names them ("Kenji Tomochika", not the
  // "Classmate" of the link's own record).
  std::string p = person(arcana);
  std::string r = rank >= 10 ? strings::kRankMax : std::to_string(rank);
  return {what, std::format("{} {}{}, {} {}", arcana_name(arcana), strings::kSocialLink, p.empty() ? "" : ", " + p,
                            strings::kRank, r)};
}

namespace {
struct Pending { int arcana; std::string line; };
}
static std::vector<Pending> g_pending;

// A rank change or reversal waits for the conversation it happens in to end (no message window, no message session
// between windows, no rank card up); one whose card was called for is the card's and is dropped.
static void queue(int arcana, std::string line) {
  g_last_change = line;
  log::writef("social: {}", line);
  if (g_card_due[arcana]) return;
  g_pending.push_back(Pending{arcana, std::move(line)});
}

static void flush() {
  screens::Screen* cur = screens::current();
  bool card_up = cur && !strcmp(cur->id(), "rank_up");
  if (msg::current().blocking || msg::session_open() || card_up) return;
  for (auto& p : g_pending) if (!g_card_due[p.arcana]) speech::speak(p.line, false);
  g_pending.clear();
  for (bool& d : g_card_due) d = false;   // the conversation is over: its cards have been shown
}

void tick() {
  static int prev_points[23] = {};
  static bool have = false;
  // A save loading is not news: the records that follow are a new baseline.
  if (screens::loading()) { have = false; g_pending.clear(); return; }
  auto links = read();
  if (links.empty()) { have = false; g_pending.clear(); return; }
  int cur[23] = {}, points[23] = {};
  bool rev[23] = {};
  for (auto& l : links) {
    cur[l.arcana] = l.rank; points[l.arcana] = l.points;
    if (l.rank > 0) rev[l.arcana] = reversed(l.arcana);
  }
  if (have) {
    for (int i = 1; i <= 22; ++i) {
      if (cur[i] > g_prev[i]) {
        log::writef("social: {} rank {} -> {} with {} of {} points (now {})", arcana_name(i), g_prev[i], cur[i], prev_points[i],
                    points_needed(Link{i, g_prev[i], prev_points[i]}), points[i]);
        queue(i, std::format("{}: {}", g_prev[i] == 0 ? strings::kNewSocialLink : strings::kSocialLinkUp, name_rank(i, cur[i])));
      } else if (cur[i] == g_prev[i] && cur[i] > 0 && points[i] > prev_points[i]) {
        // The notes over the partner's head after an answer: the points it earned. Said at once (it answers the choice).
        std::string line = std::format("{}: {}, {} {}", strings::kBondUp, arcana_name(i), points[i] - prev_points[i], strings::kPoints);
        Link l{i, cur[i], points[i]};
        if (ready(l) && !ready(Link{i, cur[i], prev_points[i]})) line += std::string(", ") + strings::kReadyToRankUp;
        log::writef("social: {} (now {} of {})", line, points[i], points_needed(l));
        speech::speak(line, false);
      }
      if (rev[i] != g_prev_rev[i] && cur[i] > 0 && cur[i] == g_prev[i])
        queue(i, std::format("{}: {}", rev[i] ? strings::kSocialLinkReversed : strings::kSocialLinkRestored, name_rank(i, cur[i])));
    }
  }
  memcpy(g_prev, cur, sizeof g_prev);
  memcpy(prev_points, points, sizeof prev_points);
  memcpy(g_prev_rev, rev, sizeof g_prev_rev);
  have = true;
  flush();
}

void install() {
  g_rankup_cmd = (FlowCmd_t)stats::flow_command("CMM_RANKUP");
  log::writef("social: CMM_RANKUP={:#x}", g_rankup_cmd ? (uintptr_t)g_rankup_cmd - scan::exe_base() : 0);
  if (g_rankup_cmd) hooks::attach((void**)&g_rankup_cmd, (void*)&rankup_hook, "CMM_RANKUP");
  dev::route("/social", [](const dev::Request& r, int&) {
    // ?partner=Kenji%20Tomochika: what the field prompt adds for that person
    auto out = std::make_shared<std::string>("timeout\n");
    std::string who = dev::arg(r, "partner");
    hooks::run_on_game_thread([out, who] {
      std::string s = summary() + "\n";
      if (!who.empty()) s = std::format("partner '{}': '{}'\n", who, partner_rank(who)) + s;
      for (auto& l : read())
        if (l.rank > 0 || l.points > 0)
          s += std::format("{:2} {:<11} rank {:2} points {:3} of {:3}{}{} ({})\n", l.arcana, arcana_name(l.arcana), l.rank, l.points,
                           points_needed(l), reversed(l.arcana) ? " reversed" : "", ready(l) ? " ready" : "", person(l.arcana));
      if (!g_last_change.empty()) s += "last: " + g_last_change + "\n";
      *out = s;
    });
    return *out;
  }, "social links and ranks");
}
}  // namespace p3a::social

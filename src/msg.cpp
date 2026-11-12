// Message windows. Layout (P3R community RE, rirurin/p3rpc.nativetypes Xrd777.cs, verified live 2026-10-27):
//   UMsgManager.ProcList (reflected)  TArray<UMsgProcItem*>
//   UMsgProcItem +0x48 pMsgItem, +0x50 pSelItem (both reflected)
//   UMsgItem     +0x28 FCurrentBmdPage* CurrentPage, +0x30 CurrentSpeaker, +0x88 mpMsgProcWindow (reflected)
//   FCurrentBmdPage +0x28 FCurrentBmdCharacterOverview* CharacterData; overview +0x48 Next = the first line
//   USelItem     +0x40 selIndex (the cursor's row on screen), +0x48 selCount, +0x50 TArray<SelBoxStruct2*> entries,
//                +0x60 its ListCursor (the answer under the cursor: row + top)
//   BmdPageToFString(FString* out, first line) renders a page to plain text (the game allocates out's buffer).
#include "msg.h"
#include "msg_render.h"
#include "core/page_text.h"
#include "core/speech_text.h"
#include "core/text_clean.h"
#include "core/strings.h"
#include "devserver.h"
#include "hooks.h"
#include "screens/util.h"
#include "input.h"
#include "log.h"
#include "seh.h"
#include "settings.h"
#include "speech.h"
#include "ue/scan.h"
#include "ue/ue.h"
#include "voice.h"
#include "world.h"
#include <format>
#include <map>
#include <set>
#include <utility>
#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

namespace p3a::msg {
using namespace ue;
static void** g_gmalloc;   // FMalloc** (GMalloc)
static int g_free_slot = -1;
static std::mutex g_mu;
static Current g_current;
// What the mod has read of one window. `page` is the address of the page read last, only ever compared with the
// window's current one; `sel_obj` is the choice list read last (index -1: none).
struct Seen { void* page = nullptr; WeakObj sel_obj; uint64_t seq = 0; std::string spoken, chosen; std::set<std::string> said;
              int tut_page = -1; };
// What a window said is forgotten when it closes (tick): the game reuses its proc and page for the next notice (the
// camp Persona list's "already equipped", the same text each Enter, was read only the first time, 2026-10-31).
static std::map<WeakObj::Key, Seen> g_seen;   // by UMsgProcItem (its index and name, not its address)
static uint64_t g_pages_spoken = 0;

// A page is compared with its digits blanked (core::no_digits): a countdown rewrites its page every second ("Settings
// will revert in 9 seconds.", the display settings prompt, 2026-10-28) and must be read once, not on every tick.
using core::no_digits;
using core::spoken_line;
// The last line said (digits blanked), until the message session ends: a notice read twice as one window opens is
// said once (the countdown prompt was read twice as it opened); the same notice opened again later is a new one.
static std::string g_last_key;
static bool just_said(const std::string& t) { return no_digits(t) == g_last_key; }
static void mark_said(const std::string& t) { g_last_key = no_digits(t); }

std::string take_fstring(FString& s) {
  std::string out = s.data ? to_utf8(s) : std::string();
  game_free(s.data);
  s.data = nullptr;
  return out;
}
void game_free(void* p) {
  if (!p || !g_gmalloc || !*g_gmalloc || g_free_slot < 0) return;
  void* m = *g_gmalloc;
  using Free_t = void (*)(void*, void*);
  ((Free_t)(*(void***)m)[g_free_slot])(m, p);
}
// FMalloc's vtable: FExec's destructor and Exec, then Malloc(size, alignment) at slot 2 (TryMalloc at slot 3 jumps
// to it; read 2026-10-28).
void* game_malloc(size_t n) {
  if (!g_gmalloc || !*g_gmalloc) return nullptr;
  void* m = *g_gmalloc;
  using Malloc_t = void* (*)(void*, size_t, uint32_t);
  return ((Malloc_t)(*(void***)m)[2])(m, n, 0);
}

static UObject* manager() {
  static const int id = watch::want("MsgManager");
  return watch::get(id);
}

// "Yes, 1 of 2": every list in the mod speaks its position this way. An answer's own full stop goes ("Nice to meet
// you., 1 of 3" read as a dot, 2026-10-29); an ellipsis stays.
static std::string choice_line(const std::vector<std::string>& ch, size_t i) {
  return core::position(core::without_own_stop(ch[i]), i + 1, ch.size());
}

static std::string window_kind(UObject* item) {
  UObject* win = get<UObject*>(item, "mpMsgProcWindow");
  std::string k = win ? name_of(class_of(win)) : name_of(class_of(item));
  if (k.rfind("MsgProcWindow_", 0) == 0) k = k.substr(14);
  return k;
}


// The game's log (B) lists every page of dialogue and thought and each answer chosen. Its storage is native and
// unmapped, so the mod keeps its own record of this session in the same order (game thread).
static std::vector<std::string> g_history;
static void remember(const std::string& kind, const std::string& line) {
  if (kind == "System" || kind == "Tutorial" || line.empty()) return;
  if (!g_history.empty() && g_history.back() == line) return;
  g_history.push_back(line);
  if (g_history.size() > 500) g_history.erase(g_history.begin());
}
const std::vector<std::string>& history() { return g_history; }


// Voiced dialogue (setting VoicedLines): a page that declares a voice (declares_voice) is never read (or only its
// speaker is named) and nothing waits for the voice: there is no fallback reading when none plays (the user's call,
// 2026-11-03). The page stays pending only to claim its voice's start for the end cue. The voice is the page's own by
// its cue: the page's voice function names the cue (core::declared_cue) and the sound manager's voice block holds the
// cue it was asked to play (voice::started). It can begin before the page has its text or some 350 ms after; a start
// counts once, and never one older than the last a page claimed.
struct Pending { std::string line, who; void* page = nullptr; uint64_t shown = 0; bool active = false, cue = false; int voice_cue = 0; };
static Pending g_pending;
static uint64_t g_voice_used = 0;   // the voice start a page already claimed (the next page must not claim it too)
// A page that waits for a key, once its voice (or the mod's reading of an unvoiced line: thoughts, everyday NPC talk)
// has ended: then "Next" or a game sound says so (setting VoiceEndSound).
static void* g_wait_page = nullptr;
static ue::WeakObj g_sel;   // the choice list up now (USelItem), from the last tick
static bool g_wait_speech = false;   // the mod read the page: its reading has to end before an end sound
static bool g_wait_spoke = false;    // the speech backend has reported that reading under way
static void wait_for_key(void* page, bool after_speech) { g_wait_page = page; g_wait_speech = after_speech; g_wait_spoke = false; }
// The page whose voice began (cleared by the next page), and whether that voice has ended since (Current::voice_done).
static void* g_voiced_page = nullptr;
static bool g_voiced_done = false;
// The speaker is named only when the speaker changes: Junpei's second line in a row is read without "Junpei:". A line
// with no speaker (narration, a notice) ends the run, and so does the end of the message session (a new conversation).
static std::string g_last_speaker;
static std::string g_last_who;   // the last named speaker in this session: a window titled with it is that window's title
static bool may_be_voiced(const std::string& kind) {
  return settings::get(settings::VoicedLines) != settings::VoicedRead && kind != "System" && kind != "Tutorial" && kind != "Select";
}
// A scene's description (a story cut-in, a full-screen picture) was read since the last dialogue line: that line
// queues after it.
static bool g_after_cutin = false;
void say_description(const char* text) {
  if (!text || world::scene_fast()) return;   // fast-forward reads no dialogue either
  speech::speak(text, false);
  g_after_cutin = true;
}
static bool may_interrupt() { return !std::exchange(g_after_cutin, false); }
static void voiced(const Pending& p) {
  if (settings::get(settings::VoicedLines) == settings::VoicedName && !p.who.empty()) speech::speak(p.who, may_interrupt());
}
// Fast-forward (Tab, or Menu on a controller) in story scenes: AtlEvtSubsystem.IsHighSpeed. While it is on the
// scene's sequence runs four times faster (WorldSettings.MatineeTimeDilation 4; found 2026-10-29 by copying every
// object around two presses).
// While it is on, dialogue pages go by several a second: each would cut off the one before, so they go to the log
// unspoken. The last one held is read if it still shows when fast-forward ends, and before a choice list that stops it.
static bool g_ffwd = false;
// Auto (UMsgManager +0x34) turns pages on a timer for unvoiced lines: one page interrupting the last could cut a line
// off before the screen reader finished it, so while Auto is on dialogue pages queue instead.
static bool g_auto = false;
static std::string g_ffwd_line, g_ffwd_text;

// cue: the window waits for a key after this page (dialogue and thoughts; not system notices, tutorials or the
// navigator's and subtitles' windows, which go on by themselves).
// The windows that go on by themselves in battle (the navigator's support lines, the party's remarks: BMD_BtlSupport*)
// and the party's voiced chatter in the field (MsgAssist, BMD_FldSupport_DungeonTalk_*) talk over what matters: they go
// to the log unspoken (the player's calls, 2026-11-01 for battle, 2026-11-06 for the field: "Thebel, floor 4" and "Junpei,
// HP 1 of 106" were cut off). A field line the game does not voice is still read. The Reaper's warning is voiced too: the user is content with the voice.
static bool g_hush = false;
// Whether the game will voice a page: its BMD data (FCurrentBmdPage +0x20) opens with message functions, FE then a byte
// F0 | (1 + argument count), the function (table << 5 | index) and two bytes per argument; a voiced page has table 3
// function 1 with its first argument 1 (FE F7 61 02 01 .., the fourth argument the voice's cue: Yukari's 002_020 lines
// 1-3), an unvoiced one none, or the same function with 3 (Akihiko's everyday lines; measured 2026-11-03). Arguments are
// stored plus one per byte (01 01 is 0): a cue of 0 has no voice (the contract's "Did I spell that correctly?",
// FE F7 61 02 01 01 01 01 01 01 01..., where the Mysterious Boy's lines count 02, 03, ...; 2026-11-03).
// The field's support talk (MsgAssist) opens with the same function and another first argument (FE F7 61 03 ..): `any`
// takes any first argument, so that a line of chatter with a cue counts as voiced (2026-11-06).
// The walk itself is core::declares_voice; the data has no length here, and the walk looks at its first 96 bytes'
// worth of functions (each read stays within 128 bytes).
static int declared_cue(void* page, bool any = false) {
  int v = 0;
  guarded([&] {
    uint8_t* d = page ? *(uint8_t**)((uint8_t*)page + 0x20) : nullptr;
    if (d) v = core::declared_cue(d, 128, any);
  });
  return v;
}
static bool declares_voice(void* page, bool any = false) { return declared_cue(page, any) != 0; }

static void say_dialogue(const std::string& line, const std::string& who, const std::string& kind, void* page, bool cue) {
  if (g_hush) return;
  g_wait_page = nullptr;   // the page before moved on
  g_voiced_page = nullptr; g_voiced_done = false;
  g_ffwd_line.clear();
  if (g_ffwd && cue) {   // dialogue and thoughts only: navigator lines and subtitles are read as ever
    g_pending.active = false;
    g_ffwd_line = line;
    // the page's own text, compared with what the window shows when fast-forward ends
    g_ffwd_text = !who.empty() && line.rfind(who + ": ", 0) == 0 ? line.substr(who.size() + 2) : line;
    return;
  }
  // A line the game will not voice (its page declares none) is read at once; only a declared voice is waited for.
  if (!may_be_voiced(kind) || !declares_voice(page)) {
    g_pending.active = false;
    speech::speak(line, may_interrupt() && !g_auto);
    if (cue) wait_for_key(page, true);
    return;
  }
  g_pending = Pending{line, who == strings::kThinking ? std::string() : who, page, GetTickCount64(), true, cue, declared_cue(page)};
  log::writef("msg: page declares voice cue {}", g_pending.voice_cue);
  voiced(g_pending);
}
// Speech that follows the page (its choice list): after the line, or alone if the line is voiced.
static void say_after(const std::string& line) {
  if (!g_ffwd_line.empty()) { speech::speak(g_ffwd_line, true); g_ffwd_line.clear(); }   // the question it answers
  speech::speak(line, false);
}
static void pending_tick() {
  Pending& p = g_pending;
  if (!p.active) return;
  uint64_t now = GetTickCount64();
  uint64_t v = voice::started(p.voice_cue);
  if (v > g_voice_used) {
    g_voice_used = v;
    g_voiced_page = p.page; g_voiced_done = false;
    log::writef("msg: voiced ({} ms after the page): {}", now - p.shown, p.line);
    p.active = false;
    if (p.cue) wait_for_key(p.page, false);
  }
}

static uint32_t g_presses = 0;
uint32_t press_count() { return g_presses; }
void player_pressed() {
  ++g_presses;
  bool blocking;
  { std::lock_guard lk(g_mu); blocking = g_current.blocking; }
  if (blocking) speech::stop();
}

// The controller's confirm and cancel (A 0x1000, B 0x2000) pressed for the game: not in the mod layer or menu.
static void pad_press_tick() {
  static uint16_t pad_was = 0;
  uint16_t pad = input::real_buttons() & 0x3000;
  if ((pad & ~pad_was) && !input::layer_open() && !input::modal()) player_pressed();
  pad_was = pad;
}

// What one tick gathers from the windows: the state others read (msg::current) and what the end cue asks.
struct Tick {
  Current cur;
  bool in_battle = false;
  bool wait_showing = false, pending_showing = false, choosing = false;
  UObject* sel_now = nullptr;
  void* cur_page = nullptr;
};

// A page of a window is said: spoken, held or hushed (say_dialogue), kept as the session's last line, recorded for
// the log and noted in the window's record. `who` is the speaker when the line names one; `log_line` always names
// the speaker; `icons` is the page whose button icons are logged (null: none).
static void say_page(Seen& s, const std::string& line, const std::string& who, const std::string& log_line, const std::string& text,
                     const std::string& kind, void* page, bool cue, void* icons) {
  ++g_pages_spoken;
  say_dialogue(line, who, kind, page, cue);
  mark_said(line);
  remember(kind, log_line);
  if (icons) log_if_icons((uintptr_t)icons, text);
  s.spoken = text;
  s.said.insert(no_digits(text));
}

// One window's page: what it shows goes into the tick's state and what is new in it is said. Returns whether the
// window has text.
static bool read_window(Tick& tk, Seen& s, UObject* item, UObject* sel) {
  Current& cur = tk.cur;
  void* page = *(void**)((uint8_t*)item + 0x28);
  void* speaker = *(void**)((uint8_t*)item + 0x30);
  std::string text = page_string(page), who = page_string(speaker);
  // Inner monologue carries a developer placeholder speaker ("DEV: Internal MES"); the sighted player sees it as
  // a thought bubble, so the mod says "Thinking" instead of reading the placeholder.
  // The battle's game-over prompt has it without the prefix ("Internal MES", 2026-10-27).
  if (who.rfind("DEV:", 0) == 0 || who.rfind("Internal MES", 0) == 0) who = who.find("Internal") != std::string::npos ? strings::kThinking : "";
  if (text.empty()) return false;
  cur.open = true; cur.text = text; cur.kind = window_kind(item);
  tk.cur_page = page;
  std::string ic = name_of(class_of(item));
  bool own_pace = ic == "MsgAssist" || ic == "MsgSubtitles" || ic == "MsgMindSubtitles";
  if (!own_pace) cur.blocking = true;
  // Hushed lines still reach the log. Cutscene subtitles are read only if the player turned them on.
  bool subtitle = ic == "MsgSubtitles" || ic == "MsgMindSubtitles";
  g_hush = (ic == "MsgAssist" && declares_voice(page, true)) || (own_pace && tk.in_battle) || (subtitle && !settings::on(settings::Subtitles));
  bool cue = cur.blocking && cur.kind != "System" && cur.kind != "Tutorial";
  if (who.empty() && cur.kind == "Mind") who = strings::kThinking;   // a thought bubble with no speaker line
  cur.speaker = who;
  // A tutorial's closing frames show its title alone as a page of a new window (no speaker): said already.
  std::string& last_who = g_last_who;
  bool title_echo = page != s.page && who.empty() && text == last_who;
  if (title_echo) s.said.insert(no_digits(text));
  if (!who.empty() && who != strings::kThinking) last_who = who;
  // The next page of a window is often built (and spoken from its buffer, below) a frame before it becomes
  // CurrentPage: then it is not said again (the shrine fortune's thoughts, 2026-10-28).
  bool echo = page != s.page && !s.spoken.empty() && text == s.spoken;
  if (echo) s.page = page;
  // System questions answer with the key-help buttons, not a choice list (TODO: the window's own yes/no flag).
  auto hinted = [&](const std::string& t) {
    // A question may end in a note: "...keep these changes? (Settings will revert in 10 seconds.)".
    size_t q = !t.empty() && t.back() == ')' ? t.rfind(" (") : std::string::npos;
    bool ask = !t.empty() && (t.back() == '?' || (q != std::string::npos && q > 0 && t[q - 1] == '?'));
    return !sel && cur.kind == "System" && ask ? t + " " + strings::kYesNoHint() : t;
  };
  // A tutorial turns its pages (Q and E) without replacing CurrentPage: the window keeps the shown page's index
  // (MsgProcWindow_Tutorial +0x74, the one before at +0x70, the page count at +0xA4; measured 2026-10-29), and
  // every page was built from the window's buffer when it opened, one dialog per page.
  int tut_page = -1, tut_pages = 0;
  if (cur.kind == "Tutorial")
    if (UObject* win = get<UObject*>(item, "mpMsgProcWindow"))
      guarded([&] { tut_page = *(int*)((uint8_t*)win + 0x74); tut_pages = *(int*)((uint8_t*)win + 0xA4); });
  if (page != s.page && !title_echo) {
    bool named = !who.empty() && who != g_last_speaker;
    std::string line = hinted(named ? spoken_line(who, text) : text);
    // A tutorial of several pages says the first one's position too, like the pages turned to.
    if (tut_page >= 0 && tut_pages > 1 && tut_pages < 64 && tut_page < tut_pages) line = core::position(line, (size_t)tut_page + 1, (size_t)tut_pages);
    if (cur.kind != "System" || !just_said(line)) {   // dialogue may repeat a line ("...") on purpose
      // The log always names the speaker (and never holds a System question, so the key hint is not in it).
      say_page(s, line, named ? who : std::string(), spoken_line(who, text), text, cur.kind, page, cue, page);
      if (!g_hush) g_last_speaker = who;   // an unspoken line named nobody: the next line still names its speaker
    } else {
      s.spoken = text;
      s.said.insert(no_digits(text));
    }
    if (!who.empty()) s.said.insert(no_digits(who));   // a tutorial's title is a page of the same buffer: said already
    s.seq = created_seq();
  } else if (!title_echo) {
    // Later pages of a multi-page window are built from the same BMD buffer without replacing CurrentPage
    // (the system notices' page 2, measured 2026-10-27): speak any newer page of this buffer.
    void* buf = nullptr;
    guarded([&] { buf = *(void**)((uint8_t*)page + 0x18); });
    // A tutorial's page turned: the page it shows now, read again even when heard before, with its position.
    if (tut_page >= 0 && s.tut_page >= 0 && tut_page != s.tut_page) {
      std::string t = dialog_text(buf, tut_page);
      if (!t.empty() && t != s.spoken) {
        std::string line = tut_pages > tut_page && tut_pages < 64 ? core::position(t, (size_t)tut_page + 1, (size_t)tut_pages) : t;
        ++g_pages_spoken;
        speech::speak(line, true);
        mark_said(line);
        s.spoken = t;
        s.said.insert(no_digits(t));
      }
    }
    for (auto& c : created_since(s.seq)) {
      s.seq = c.seq;
      // A page laid out again (BmdSetTextSize runs on re-layout too) must not be spoken twice in one window.
      bool title = c.text == last_who;   // the window's own title
      if (buf && c.buf == buf && c.text != s.spoken && !s.said.count(no_digits(c.text)) && !title) {
        // A tutorial's later page: the page the window shows once turned (the DLC notice's came this way, dialogs 7
        // and 27, without their positions, 2026-10-29).
        std::string said = hinted(c.text);
        if (tut_page > 0 && tut_page < tut_pages && tut_pages < 64) said = core::position(said, (size_t)tut_page + 1, (size_t)tut_pages);
        // The same speaker goes on: not named again. The page object the ring recorded may be gone by now: its
        // icons are looked at only when it is the page the window holds.
        say_page(s, said, {}, spoken_line(who, c.text), c.text, cur.kind, page, cue, c.obj == page ? page : nullptr);
        cur.text = c.text;
      } else if (buf && c.buf == buf && no_digits(c.text) == no_digits(s.spoken)) {
        s.spoken = c.text;   // a countdown ticked: keep the current number for a re-read
      }
    }
    // The current page's own text changed in place (a tutorial's page 2 in the Velvet Room, 2026-10-27).
    bool title = text == last_who;
    if (text != s.spoken && !s.said.count(no_digits(text)) && !title) {
      say_page(s, hinted(text), {}, spoken_line(who, text), text, cur.kind, page, cue, nullptr);
    } else if (text != s.spoken && no_digits(text) == no_digits(s.spoken)) {
      s.spoken = text;   // a countdown ticked
    }
    if (!s.spoken.empty()) cur.text = s.spoken;
  }
  // Only a page that had text counts as seen: a fresh page is often empty for a frame or two before its text is
  // laid out, and marking it seen then silenced it for good (the dorm arrival thought, 2026-10-27).
  s.page = page; s.tut_page = tut_page;
  if (page == g_wait_page) tk.wait_showing = true;   // after this page's own say_dialogue, which may arm it
  if (page == g_pending.page) tk.pending_showing = true;
  return true;
}

// The event choice with its two answers side by side (USelAEventTypeB: the ending's "Close your eyes"): Left and
// Right set +0x40, which is the answer Enter takes (seen 2026-11-11: +0x40 = 1 took the second answer, the
// ListCursor still on 0).
bool side_by_side(UObject* sel) { return sel && ue::is_a(sel, "SelAEventTypeB"); }

// A window's choice list: its answers and cursor go into the tick's state, a new list's focused answer is said once,
// and the answer under the cursor when the list closes goes in the log.
static void read_choices(Tick& tk, Seen& s, UObject* sel) {
  Current& cur = tk.cur;
  if (sel && is_live(sel)) {
    tk.choosing = true;
    tk.sel_now = sel;
    // The answer under the cursor: the ListCursor at +0x60 (7 rows shown; +0x40 is only its row on screen). Two answers
    // side by side (side_by_side) are the exception: +0x40 is the answer and the ListCursor stays on 0; it opens at -1,
    // nothing picked, which the choice screen settles on the first answer as soon as the list takes input.
    int32_t row = *(int32_t*)((uint8_t*)sel + 0x40);
    uint32_t idx = side_by_side(sel) ? (uint32_t)(row < 0 ? 0 : row)
                                     : (uint32_t)screens::ListCursor((uint8_t*)sel + 0x60).index();
    auto ch = choices_of(sel);
    cur.open = true; cur.blocking = true; cur.choices = ch; cur.choice = (int)idx;
    WeakObj list = WeakObj::of(sel);
    if (!(s.sel_obj == list)) {
      // A new choice list: the focused entry with its position, once (after the page it answers), when every entry
      // has its text -- the entries are empty for a frame or two after the list appears, like pages.
      if (core::all_filled(ch)) {
        if (idx < ch.size()) say_after(choice_line(ch, idx));
        s.sel_obj = list;
      }
    }
    // Moves are the choice menu's (screens/choice.cpp): it speaks them from its own focus.
    if (idx < ch.size() && !ch[idx].empty()) s.chosen = ch[idx];
  } else if (s.sel_obj.index >= 0) {   // the choice list closed: the answer goes in the log
    remember("Choice", s.chosen);
    s.sel_obj = WeakObj{}; s.chosen.clear();
  }
}

// A switch of the game's that is said when it changes while a conversation is on (Auto, fast-forward); a change with
// no window (the game resetting it) is only logged. `change(was, now)` words the log line. Returns whether it changed.
struct Toggle { uint32_t was = 0; bool window = false; };
static Toggle g_auto_said, g_ffwd_said;
template <class F>
static bool announce_toggle(Toggle& t, uint32_t now, bool talking, const char* on, const char* off, F&& change) {
  bool changed = now != t.was;
  if (changed) {
    bool heard = talking && t.window;
    log::writef("msg: {}{}", change(t.was, now), heard ? "" : " (no window)");
    if (heard) speech::speak(now ? on : off, false);
    t.was = now;
  }
  t.window = talking;
  return changed;
}

// The page's voice, or the mod's reading of it, ended while the page still shows: the game waits for a key.
// "Next" is speech: queued behind the reading, it goes once the voice has ended. A sound has to wait for the reading
// too: once the backend has reported it under way and then done; a backend that cannot tell (NVDA: -1) plays it as
// the voice ends.
static void end_cue_tick(const Tick& tk, uint32_t autov) {
  if (!g_wait_page) return;
  int sp = speech::speaking();
  if (sp == 1) g_wait_spoke = true;
  bool next = settings::get(settings::VoiceEndSound) == settings::EndNext;
  bool reading_done = !g_wait_speech || sp == -1 || (g_wait_spoke && sp == 0);
  if (!tk.wait_showing || tk.choosing || autov) g_wait_page = nullptr;   // a choice list waits for its answer; Auto goes on by itself
  else if (!voice::playing() && (next || reading_done)) {
    if (next) speech::speak(strings::kNext, false);
    else voice::play_end_sound();
    g_wait_page = nullptr;
  }
}

void tick() {
  pad_press_tick();
  pending_tick();
  UObject* m = manager();
  if (!m) return;
  g_ffwd = world::scene_fast();
  guarded([&] { g_auto = *(uint32_t*)((uint8_t*)m + 0x34) != 0; });
  auto* procs = field<TArray<UObject*>>(m, "ProcList");
  if (!procs) return;
  static const int battle = watch::want("BtlCoreComponent");
  Tick tk;
  tk.in_battle = watch::get(battle) != nullptr;
  Current& cur = tk.cur;
  std::map<WeakObj::Key, Seen> next;
  for (int i = 0; i < procs->num; ++i) {
    UObject* proc = procs->data[i];
    if (!is_live(proc)) continue;
    UObject* item = get<UObject*>(proc, "pMsgItem");
    UObject* sel = get<UObject*>(proc, "pSelItem");
    WeakObj::Key key = WeakObj::of(proc).key();
    auto seen = g_seen.find(key);
    Seen s = seen != g_seen.end() ? seen->second : Seen{};
    g_hush = false;
    bool has_text = item && is_live(item) && read_window(tk, s, item, sel);
    // The window closed (its proc's state +0x30 at 7 closing / 8 closed, or no item): what it said is forgotten. A
    // fresh page with no text yet (state 1, a frame or two) keeps it.
    int proc_state = 0;
    guarded([&] { proc_state = *(int32_t*)((uint8_t*)proc + 0x30); });
    if (!has_text && s.page && (!item || !is_live(item) || proc_state >= 7)) {
      s.page = nullptr; s.spoken.clear(); s.said.clear(); s.tut_page = -1;
    }
    read_choices(tk, s, sel);
    next[key] = s;
  }
  for (auto& [proc, old] : g_seen) if (old.sel_obj.index >= 0 && !next.count(proc)) remember("Choice", old.chosen);
  // The page left the screen before a voice began: the next voice (the party's chatter, which replaces no pending page) is
  // not its own.
  if (!tk.pending_showing) g_pending.active = false;
  g_seen.swap(next);
  // The notice or conversation is over (no window, no window to follow): its last line, speaker run and titles end.
  bool session = cur.open || session_open();
  if (!session) { g_last_key.clear(); g_last_speaker.clear(); g_last_who.clear(); }
  // In a conversation: a window shows, another is coming (the message session), or a story scene plays (fast-forward
  // turned on and off between its pages went unsaid, 2026-10-29).
  bool talking = session || world::scene_playing();
  // Auto (Shift, or X / Square): the game turns pages by itself once a voice ends. The flag is native, UMsgManager
  // +0x34 (0 off, 1 on; found 2026-10-29 by copying every object after each press).
  uint32_t autov = 0;
  guarded([&] { autov = *(uint32_t*)((uint8_t*)m + 0x34); });
  announce_toggle(g_auto_said, autov, talking, strings::kAutoOn, strings::kAutoOff,
                  [](uint32_t was, uint32_t now) { return std::format("auto {} -> {}", was, now); });
  // Fast-forward said the same way; when it ends, the page it left showing is read.
  if (announce_toggle(g_ffwd_said, g_ffwd ? 1 : 0, talking, strings::kFastForwardOn, strings::kFastForwardOff,
                      [](uint32_t, uint32_t now) { return std::string(now ? "fast-forward on" : "fast-forward off"); })) {
    if (!g_ffwd && !g_ffwd_line.empty() && cur.open && cur.text == g_ffwd_text) speech::speak(g_ffwd_line, false);
    if (!g_ffwd) g_ffwd_line.clear();
  }
  g_sel = WeakObj::of(tk.sel_now);
  end_cue_tick(tk, autov);
  if (g_voiced_page && !g_voiced_done && !voice::playing()) g_voiced_done = true;
  cur.voice_done = tk.cur_page && tk.cur_page == g_voiced_page && g_voiced_done;
  std::lock_guard lk(g_mu);
  g_current = cur;
}

Current current() { std::lock_guard lk(g_mu); return g_current; }

// Between two windows of one notice or conversation. A window's UMsgProcItem (ProcList) runs its state +0x30 through
// 1 (showing) .. 7 (closing) to 8 (closed, pMsgItem gone) with flag 0x8 in +0x2C; from there it starts the next window
// (state 1, a frame later) or leaves the list. Script-owned procs with no window sit at state 0 (a shop's, the field's
// passers-by), and a menu's prompt proc stays at 8 without the flag once its choice is made (the camp's "Return to the
// title screen?"). Measured 2026-11-03: the 6/8 evening event, leaving a shop, the camp's System prompts.
static bool between_windows(UObject* p) {
  return is_live(p) && *(int32_t*)((uint8_t*)p + 0x30) == 8 && (*(uint32_t*)((uint8_t*)p + 0x2C) & 0x8);
}
static TArray<UObject*>* proc_list() {
  UObject* m = manager();
  return m ? field<TArray<UObject*>>(m, "ProcList") : nullptr;
}
bool session_open() {
  auto* procs = proc_list();
  if (!procs) return false;
  for (int i = 0; i < procs->num; ++i) if (between_windows(procs->data[i])) return true;
  return false;
}
// A proc already between windows when a window opens is another session's: a story line before a battle leaves its proc
// there for the whole fight (seen 2026-11-06: the victory page under a level-up's notice waited for a key).
static std::vector<ue::WeakObj> g_other_sessions;
void window_opened() {
  g_other_sessions.clear();
  auto* procs = proc_list();
  for (int i = 0; procs && i < procs->num; ++i)
    if (between_windows(procs->data[i])) g_other_sessions.push_back(ue::WeakObj::of(procs->data[i]));
}
bool window_session_open() {
  auto* procs = proc_list();
  for (int i = 0; procs && i < procs->num; ++i) {
    UObject* p = procs->data[i];
    if (between_windows(p) && std::find(g_other_sessions.begin(), g_other_sessions.end(), ue::WeakObj::of(p)) == g_other_sessions.end()) return true;
  }
  return false;
}
ue::UObject* sel_item() { return g_sel.resolve(); }
void repeat() {
  Current c = current();
  if (!c.open) { speech::speak(strings::kNoMessage, true); return; }
  std::string line = spoken_line(c.speaker, c.text);
  if (c.choice >= 0 && c.choice < (int)c.choices.size()) line += ". " + choice_line(c.choices, c.choice);
  speech::speak(line, true);
}
std::string status() { return std::format("msg: bmd2str={} gmalloc={} free_slot={} pages_spoken={} speaking={} wait_page={}", renderer(), (void*)g_gmalloc, g_free_slot, g_pages_spoken, speech::speaking(), g_wait_page); }

static std::string raw_dump() {
  std::string out = status() + "\n";
  UObject* m = manager();
  if (!m) return out + "no MsgManager\n";
  auto* procs = field<TArray<UObject*>>(m, "ProcList");
  out += std::format("ProcList {}\n", procs ? procs->num : -1);
  for (int i = 0; procs && i < procs->num; ++i) {
    UObject* proc = procs->data[i];
    UObject* item = get<UObject*>(proc, "pMsgItem");
    UObject* sel = get<UObject*>(proc, "pSelItem");
    out += std::format(" proc #{} item={} sel={}\n", index_of(proc), full_name(item), full_name(sel));
    if (item) {
      void* page = *(void**)((uint8_t*)item + 0x28);
      void* spk = *(void**)((uint8_t*)item + 0x30);
      UObject* win = get<UObject*>(item, "mpMsgProcWindow");
      out += std::format("  window={} status={}\n  page={} '{}'\n  speaker={} '{}'\n", full_name(win), win ? *((uint8_t*)win + 0x88) : -1, page, page_string(page), spk, page_string(spk));
    }
    if (sel) {
      out += std::format("  sel index={} count={}\n", *(uint32_t*)((uint8_t*)sel + 0x40), *(uint32_t*)((uint8_t*)sel + 0x48));
      auto ch = choices_of(sel);
      for (size_t c = 0; c < ch.size(); ++c) out += std::format("   [{}] {}\n", c, ch[c]);
    }
  }
  return out;
}

bool install() {
  uintptr_t b = scan::exe_base();
  uintptr_t a = scan::find_unique("4C 8B DC 55 57 41 54 49 8D AB ?? ?? ?? ?? 48 81 EC C0 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 ?? ?? ?? ?? 45 33 E4 48 89 54 24 ??");
  set_renderer((void*)a);
  uintptr_t gm = scan::find("48 8B 0D ?? ?? ?? ?? 48 85 C9 75 ?? E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B 01 48 8B D3 FF 50 ?? 48 83 C4 20");
  if (gm) { g_gmalloc = (void**)scan::rip(gm + 3); g_free_slot = *(uint8_t*)(gm + 32) / 8; }
  log::writef("msg: BmdPageToFString={:#x} GMalloc={:#x} free slot {}", a ? a - b : 0, gm ? (uintptr_t)g_gmalloc - b : 0, g_free_slot);
  install_pages(a != 0);
  dev::route("/msg", [](const dev::Request&, int&) {
    return dev::on_game([] { return raw_dump(); });
  }, "message windows: proc items, pages, speakers, choices");
  dev::route("/msg/tokens", [](const dev::Request& r, int&) {
    // /msg/tokens?page=0x.. the lines of a page (page+0x28 -> +0x48 first line; line +0x40 next line, +0x28 first
    // token; token: inline UTF-8 text at +0, next token at +0x28) with each token's first 0x60 bytes
    uintptr_t page = (uintptr_t)dev::arg_int(r, "page", 0);
    auto out = std::make_shared<std::string>("timeout\n");
    hooks::run_on_game_thread([out, page] {
      std::string s = dump_tokens(page);
      *out = s.empty() ? "nothing\n" : s;
    });
    return *out;
  }, "?page=0x.. dump a page's line and token records");
  return a != 0;
}
}  // namespace p3a::msg

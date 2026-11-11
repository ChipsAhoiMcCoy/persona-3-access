#pragma once
#include <string>
#include <vector>
#include "ue/ue.h"

namespace p3a::msg {
// The game's BMD message system (UMsgManager -> UMsgProcItem -> UMsgItem pages, USelItem choices): dialogue,
// system messages, tutorials. Each new page is spoken once ("Speaker: text"); choices are read as a list and the
// cursor is followed. Everything is read from the game's own objects (msg.cpp has the layout notes).
bool install();
void tick();   // game thread
// The current message window, for the review keys and generic screens.
// open: any window has text; blocking: one that stops the player (party chatter and subtitles do not).
// voice_done: the page on screen was voiced and its voice has ended (the game's log lists such a page already).
struct Current { bool open = false, blocking = false, voice_done = false; std::string speaker, text; std::vector<std::string> choices; int choice = -1; std::string kind; };
Current current();
bool session_open();   // a notice or conversation is still under way (another window may follow; game thread)
void window_opened();         // a window came up over a screen: procs already between windows are not its session
bool window_session_open();   // session_open for that window's session alone
ue::UObject* sel_item();   // the choice list up now (USelItem), or null (game thread)
void repeat();   // speak the current page again
// The player pressed confirm or cancel (Enter, Space, Escape, Backspace; A, B): over a blocking message window the
// mod's reading stops at once, so a page skipped (or a choice made) feels instant (game thread).
void player_pressed();
uint32_t press_count();   // how many such presses so far: a screen that acts on the player's confirm compares it
// A story cut-in's description was just read (cutin.cpp): the next dialogue line queues after it, not over it.
void cutin_said();

// Every BMD page the game builds (dialogue pages, UI descriptions, help lines), rendered to text, newest last.
struct Created { uint64_t seq; uint64_t ms; void* wrapper; int dialog; int page; std::string text; void* buf = nullptr; void* data = nullptr; void* obj = nullptr; };   // obj: the FBmdPage
std::vector<Created> created_since(uint64_t seq);
// Every page of a BMD buffer the game has laid out, as it reads now; `ms` is when it was last laid out. Unlike the
// ring of created pages, a page built again unchanged (a list reopened) is still here.
std::vector<Created> pages_of(const void* buf);
uint64_t created_seq();
// The seq to count from for "pages built in the last ms": a screen that starts counting on entry takes the pages its
// opening built, not the history before it (a camp submenu's first help line was the title's "Load this save data?").
uint64_t created_seq_before(unsigned ms);
// Pages rebuilt within the last max_age_ms (what BMD text is on screen now), oldest first; seq is when its text was built.
std::vector<Created> visible(unsigned max_age_ms = 200);
// Forget the pages of one message buffer: the next build of each is a new page, read at once (a page object reused
// for new text is otherwise re-read only a few times a second).
void forget_pages(const void* buf);
// The pages of one dialog of a buffer as last built, joined (a tutorial's page turned back to: nothing is rebuilt).
std::string dialog_text(void* buf, int dialog);
void install_pages(bool renderer);   // bmdpages.cpp: the CreateBmdPage / BmdSetTextSize hooks, from install()
std::string status();
// Frees memory the game allocated (an FString a UFunction returned into our buffer) through GMalloc.
void game_free(void* p);
// An FString the game allocated (a UFunction's return, BmdPageToFString's output): its text, and the buffer freed.
std::string take_fstring(ue::FString& s);
void* game_malloc(size_t n);   // GMalloc->Malloc (null before install): memory the engine may free or grow itself
std::string page_string(void* page);   // an FCurrentBmdPage rendered to text ("" on any fault)
// Every line of dialogue and thought spoken this session and each answer chosen, oldest first (what the game's
// log lists; game thread).
const std::vector<std::string>& history();
}  // namespace p3a::msg

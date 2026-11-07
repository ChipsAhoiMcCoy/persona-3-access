// Config (System > Config). Native: AUIConfiguration holds the state (+0x294: 5 on the tab list, 6 in a tab's rows,
// 24 once closed; measured 2026-10-28), its DrawActor (BP_UIConfiguration_C) answers reflected calls:
// GetCursorIndex, GetListNum, IsConfigTypeMode (the tab list) and GetIndexedDrawConfigParam(i) -> FDrawConfigParam
// (Type, Title, Description, Value, bFlag, bEnable, ListType, ListMaxNum, SelectedLabel). The highlight that marks
// a value is a sprite, so values come from that struct, not from the drawn text.
#include "screens/menu.h"
#include "screens/util.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "hooks.h"
#include "input.h"
#include "log.h"
#include "msg.h"
#include "speech.h"
#include "ue/scan.h"
#include <algorithm>
#include <format>
#include <vector>

namespace p3a::screens {
using namespace ue;

namespace {
struct Row { std::string title, desc, value; bool ok = false; };

// Keyboard key codes of the key configuration's Value (measured 2026-10-28 against the defaults: 432 W, 442 S, 441 A,
// 443 D, 469 Space, 452 C, 431 Q, 433 E, 454 B): the letters in keyboard rows from 431.
std::string key_name(int v) {
  static const char* kRows = "QWERTYUIOPASDFGHJKLZXCVBNM";
  if (v >= 431 && v <= 440) return std::string(1, kRows[v - 431]);
  if (v >= 441 && v <= 449) return std::string(1, kRows[10 + v - 441]);
  if (v >= 450 && v <= 456) return std::string(1, kRows[19 + v - 450]);
  if (v == 469) return "Space";
  return std::format("{} {}", strings::kKeyCode, v);
}

std::string take(FString* s) { return s ? core::clean_text(msg::take_fstring(*s)) : std::string(); }
}  // namespace

// A bespoke menu (menu.h): the rows are cached (each read is a reflected call whose strings the game allocates) and
// read again when the list changes, and the focused row every frame (its value changes a frame or two after left /
// right, which reach the game). The cursor is the ListCursor behind +0x3D0 (row + top; the
// next frame copies it into the draw actor), written directly with the cursor sound, as the mouse path does.
class ConfigScreen : public MenuScreen {
  Finder cfg_{"UIConfiguration"};
  bool on_ = false;
  int log_st_ = -1;
  ULONGLONG log_st_ms_ = 0;
  std::vector<Row> rows_;
  bool tabs_ = false;
  std::string list_key_;
  // The BGM playlist (Change Battle / Boss Battle / Dungeon BGM, D on the row; state 11): BgmParams (reflected:
  // Title, bNewFlag, bIsFavoriteFlag) indexed by GetBgmListTopIndex + GetBgmCursorIndex (measured 2026-10-28).
  Mirror& bgm_ = mirror("bgm", [this] {
    UObject* d = draw();
    if (!d || !call_bool(d, "IsBGMMode")) return -1;
    auto* arr = field<TArray<uint8_t>>(d, "BgmParams");
    int i = call_int(d, "GetBgmListTopIndex") + call_int(d, "GetBgmCursorIndex");
    return arr && i >= 0 && i < arr->num ? i : -1;
  });
  Mirror& list_ = mirror("cfg", [this] {
    UObject* d = draw();
    if (!d || call_bool(d, "IsBGMMode")) return -1;
    // The cursor itself, not the draw's copy: entering a tab, the copy still held the tab list's row a frame or two
    // on, and the new list was read from that row first ("Shadow Quality, 3 of 6", then "Graphics Quality, 1 of 6").
    // While the state and the rows disagree (5 the tab list, 6 a tab) the rows are the last list's: no cursor yet.
    auto* a = (uint8_t*)cfg_.get();
    int st = a ? *(int32_t*)(a + 0x294) : 0;
    uint8_t* lc = a ? *(uint8_t**)(a + 0x3D0) : nullptr;
    if (!lc || (st == 5) != tabs_) return -1;
    int c = ListCursor(lc).index();
    return c >= 0 && c < (int)rows_.size() ? c : -1;
  });
  int state_seen_ = -1;          // the state last tick...
  bool state_settled_ = false;   // ...and whether the game has run a tick in it
  uint8_t* bgm_dialog() {
    auto* c = (uint8_t*)cfg_.get();
    int st = c ? *(int32_t*)(c + 0x294) : 0;
    return st >= 11 && st <= 13 ? *(uint8_t**)(c + 0x3F0 + (st == 11 ? 0 : st == 12 ? 8 : 0x10)) : nullptr;
  }
  UObject* draw() {
    UObject* c = cfg_.get();
    return c ? get<UObject*>(c, "DrawActor") : nullptr;
  }
  int call_int(UObject* d, const char* fn) { return call_ret<int32_t>(d, fn, -1); }
  bool call_bool(UObject* d, const char* fn) { return call_ret<bool>(d, fn); }
  std::string call_str(UObject* d, const char* fn, int arg = -1) {
    UFunction* f = find_function(class_of(d), fn);
    if (!f) return {};
    Params p(f);
    if (arg >= 0) p.set<int32_t>("InSelect", arg);
    call(d, p);
    return take(p.at<FString>("ReturnValue"));
  }
  Row row(UObject* d, int i) {
    Row r;
    UFunction* f = find_function(class_of(d), "GetIndexedDrawConfigParam");
    if (!f) return r;
    Params p(f);
    p.set<int32_t>("InIndex", i);
    call(d, p);
    FField* rp = find_prop(f, "ReturnValue");
    UStruct* st = rp ? (UStruct*)prop_inner_object(rp) : nullptr;
    uint8_t* b = p.at<uint8_t>("ReturnValue");
    if (!st || !b) return r;
    auto at = [&](const char* n) { return b + offset_of(st, n); };
    r.title = take((FString*)at("Title"));
    r.desc = take((FString*)at("Description"));
    std::string label = take((FString*)at("SelectedLabel"));
    int type = *at("Type"), list = *at("ListType"), value = *(int32_t*)at("Value"), max = *(int32_t*)at("ListMaxNum");
    bool flag = *at("bFlag") != 0, enabled = *at("bEnable") != 0;
    r.ok = !r.title.empty();
    switch (type) {
      case 0: r.value = flag ? strings::kOn : strings::kOff; break;                        // DCPT_FLAG
      case 1: r.value = max > 0 && max != 100 ? std::format("{} {} {}", value, strings::kOf, max) : std::to_string(value); break;   // DCPT_VALUE
      case 2: r.value = call_str(d, "GetSelLangText", value); break;                       // DCPT_LANGUAGE
      case 4: r.value = call_str(d, "GetBGMText", value); break;                           // DCPT_BGM: 0 "Default"
      default:
        if (list == 10) r.value = key_name(value);                                          // KEY_CONFIG
        else r.value = !label.empty() ? label : std::to_string(value);
    }
    if (!enabled) r.value += (r.value.empty() ? "" : ", ") + std::string(strings::kUnavailable);
    return r;
  }
  // "Camera Speed: 2 of 4" (the tab list: just the title).
  std::string name(const Row& r, bool tabs) { return !tabs && !r.value.empty() ? r.title + ": " + r.value : r.title; }
  std::string desc(const Row& r) {
    std::string d = r.desc;
    while (!d.empty() && (d.back() == '.' || d.back() == ' ')) d.pop_back();
    return d;
  }
  // The BGM playlist (Change Battle / Boss Battle / Dungeon BGM, D on the row; state 11): BgmParams (reflected:
  // Title, bNewFlag, bIsFavoriteFlag) indexed by GetBgmListTopIndex + GetBgmCursorIndex (measured 2026-10-28).
  // The BGM playlist's entry: title, favourite, new.
  std::string bgm_entry(UObject* d, int i) {
    auto* arr = field<TArray<uint8_t>>(d, "BgmParams");
    FField* ap = find_prop(class_of(d), "BgmParams");
    if (!arr || !ap || i < 0 || i >= arr->num) return {};
    UStruct* st = (UStruct*)prop_inner_object(prop_inner_prop(ap));
    uint8_t* e = arr->data + i * struct_size(st);
    std::string t = core::clean_text(to_utf8(*(FString*)(e + offset_of(st, "Title"))));
    if (*(e + offset_of(st, "bIsFavoriteFlag"))) t += std::string(", ") + strings::kFavorite;
    if (*(e + offset_of(st, "bNewFlag"))) t += std::string(", ") + strings::kMailNew;
    return t;
  }
  bool bgm_mode() { UObject* d = draw(); return d && call_bool(d, "IsBGMMode"); }
  void read_all(UObject* d, bool tabs) {
    int n = call_int(d, "GetListNum");
    rows_.clear();
    for (int i = 0; i < n && i < 64; ++i) rows_.push_back(row(d, i));
    tabs_ = tabs;
    list_key_ = tabs ? std::string("cfg.tabs") : "cfg." + (rows_.empty() ? std::string() : rows_[0].title);
    list_.key = list_key_;
  }
 public:
  ConfigScreen() {
    // The BGM playlists are UConfigBGMSelectDialog objects (AUIConfiguration +0x3F0 battle, +0x3F8 boss, +0x400
    // dungeon; states 11, 12, 13), each with its ListCursor at +0x8C. Their step (FUN_1413a2b20) plays the sound on
    // a move and copies row / top / index and tells the draw every frame, so a write and the sound are the move.
    bgm_.ready = [this] { return bgm_dialog() != nullptr; };
    bgm_.set = [this](int i) {
      uint8_t* d = bgm_dialog();
      if (!d || !ListCursor(d + 0x8C).set(i)) return false;
      cursor_sound();
      return true;
    };
    // FUN_1413bc540: states 5 and 6 read keys once the key-help's timer (+0xD58: +0x34 past +0x38, 0.55 s from entering
    // the state) has run, and state 6 once its open timer +0x3B8 has too (or +0x345).
    list_.ready = [this] {
      auto* c = (uint8_t*)cfg_.get();
      int st = c ? *(int32_t*)(c + 0x294) : 0;
      uint8_t* k = c ? *(uint8_t**)(c + 0xD58) : nullptr;
      // The timer is reset by the game's first tick in a state: on the frame a state appears it still holds the last one's.
      if ((st != 5 && st != 6) || !state_settled_ || !k || *(float*)(k + 0x34) <= *(float*)(k + 0x38)) return false;
      uintptr_t base = scan::exe_base();
      uint8_t* game = ((uint8_t * (*)())(base + 0x122ac90))();
      int frames = ((int (*)(void*))(base + 0x12e13b0))(*(void**)(game + 0x308A0));
      return st == 5 || c[0x345] || *(float*)(c + 0x3B8) >= frames * (1.f / 30);
    };
    list_.set = [this](int i) {
      UObject* c = cfg_.get();
      uint8_t* lc = c ? *(uint8_t**)((uint8_t*)c + 0x3D0) : nullptr;
      if (!lc || !ListCursor(lc).set(i)) return false;
      cursor_sound();
      return true;
    };
  }
  const char* id() const override { return "config"; }
  int priority() const override { return 22; }   // over the camp submenus and lists drawn alike
  bool active() override {
    UObject* c = cfg_.get();
    if (!c || !get<UObject*>(c, "DrawActor")) return false;
    // Between the tab list and a tab the state passes through others for a few frames: stay on until it closes.
    int st = *(int32_t*)((uint8_t*)c + 0x294);
    if (st != log_st_) {
      ULONGLONG now = GetTickCount64();
      log::writef("config: state {} -> {} after {} ms", log_st_, st, log_st_ms_ ? now - log_st_ms_ : 0);
      log_st_ = st; log_st_ms_ = now;
    }
    // 19 is the close (the wipe out; read from the cancel paths, 0x13927f4 and 0x1392900), 24 once it has played
    // (OnDrawClosed, 0x1392950): Config held the floor through the camp's close wipe, so the System menu was read late.
    on_ = st == 5 || st == 6 || st == 11 || (on_ && st != 19 && st != 24 && st != 0);   // 11: the BGM playlist
    return on_;
  }
  std::string title() override { return strings::kConfig; }
  void on_enter() override {
    screens::set_keys([this] { return std::string(bgm_mode() ? strings::kBgmKeys() : tabs_ ? strings::kConfigTabKeys() : strings::kConfigKeys()); });
    rows_.clear();
    state_seen_ = -1;
    state_settled_ = false;
  }
  void on_tick() override {
    auto* cs = (uint8_t*)cfg_.get();
    int st = cs ? *(int32_t*)(cs + 0x294) : 0;
    state_settled_ = st == state_seen_;
    state_seen_ = st;
    UObject* d = draw();
    if (!d) return;
    if (call_bool(d, "IsBGMMode")) return;
    bool tabs = call_bool(d, "IsConfigTypeMode");
    int n = call_int(d, "GetListNum");
    if (n <= 0) return;
    // A new list (the tab list, a tab's rows): read it all; its first row names it. The count and the rows lag a
    // switch by a few frames, so a list whose first row changed is read again too.
    if (tabs != tabs_ || n != (int)rows_.size() || (!rows_.empty() && row(d, 0).title != rows_[0].title)) { read_all(d, tabs); return; }
    // The focused row every frame: its value changes with Left / Right and with the game's own refreshes.
    int c = call_int(d, "GetListTopIndex") + call_int(d, "GetCursorIndex");
    if (c >= 0 && c < (int)rows_.size()) { Row r = row(d, c); if (r.ok) rows_[c] = r; }
  }
  void build(ui::GraphBuilder& b) override {
    if (bgm_mode()) {
      UObject* d = draw();
      auto* arr = field<TArray<uint8_t>>(d, "BgmParams");
      b.begin_stop("bgm");
      b.push_context(strings::kPlaylist);
      for (int i = 0; arr && i < arr->num && i < 500; ++i)
        b.add_item(slot("bgm", i), game_item(bgm_, i, [this, i] { UObject* dd = draw(); return dd ? bgm_entry(dd, i) : std::string(); }));
      b.pop_context();
      if (int c = bgm_.cursor(); c >= 0) b.set_start(slot("bgm", c));
      return;
    }
    if (rows_.empty()) return;
    b.begin_stop(list_key_);
    for (int i = 0; i < (int)rows_.size(); ++i) {
      if (!rows_[i].ok) continue;
      // Each line is of this list: once the rows are another's (read before the focus has moved), a row of the list
      // left says nothing, or its live parts read the new rows out ("off", a description) over the landing.
      auto of_list = [this, key = list_key_, i] { return key == list_key_ && i < (int)rows_.size(); };
      auto v = game_item(list_, i, [this, i, of_list] { return of_list() ? rows_[i].title : std::string(); },
                         [this, i, of_list] { return of_list() ? desc(rows_[i]) : std::string(); });
      if (!tabs_) v->announcements.push_back(ui::NodeAnnouncement([this, i, of_list] { return of_list() ? rows_[i].value : std::string(); }, true, ui::announcement_kinds::kValue));
      describe_with(v, [this, i, of_list] { return of_list() ? desc(rows_[i]) : std::string(); });
      b.add_item(slot(list_key_, i), v);
    }
    if (int c = list_.cursor(); c >= 0) b.set_start(slot(list_key_, c));
  }
};

std::unique_ptr<Screen> make_config() { return std::make_unique<ConfigScreen>(); }
}  // namespace p3a::screens

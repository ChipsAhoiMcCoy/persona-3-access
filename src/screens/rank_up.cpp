// The Social Link poem and rank card (then the arcana card "Rank 1"): sprites
// only. BP_UICmmRankUp_C exists only while they are up (destroyed on OK, measured 2026-10-27). It opens inside the
// conversation, over its message window, so it runs over messages (it went unread under the window, 2026-10-29).
// Native fields (rirurin's p3rpc.slplus RankUpHooks): UICmmRankUpDraw +0x2B8 rank shown (10 = MAX), +0xB30 its
// UIRankUpDraw manager, whose rank-up message handler (exe+0x138cf00) stores the link number at +0x2A0 and a mode at
// +0x2A4, and whose state is +0x294. The link number is the arcana for the game's own links.
// Its AnimManager (BP_UICmmRankUPAnim_C) plays the parts (traced 2026-10-31, Kazushi rank 2 and Yuko rank 1): a new
// link opens with the poem (CardInAnime, ~5 s, then its voice), and the rank card turns in with RankUpInAnime ~10 s
// later; a plain rank up plays RankUpInAnime at once, 70 ms after the screen opens. Each part is spoken when its own
// timeline starts, so a plain rank up (no poem) gets what happened and the card in one line.
// The poem is BP_UIPoetryDraw (UIRankUpDraw.PoetryActor -> AUIPoetryActor.pUIPoetryDraw; ArcanaID, ModeID,
// UIPoetryIsRankMaxPoetry): lines drawn as pictures (DT UIPoetryDataAsset rows 0-4 a new link, 5-11 rank MAX, the
// arcana's name a picture in the gap). The game's voice reads it, so the mod does not speak its words.
// A game over's poem is the same screen: the script's EVT_GAMEOVER_POEM sends the rank-up message with link 1 and
// mode 5, and the AnimManager's GameOverAllAnime brings four lines in as pictures (rows 12-15), unvoiced, for some
// 10 s before the list of what to do. Their text is dialog 0 of the event's message file, read out as it starts.
#include "screens/screen.h"
#include "screens/util.h"
#include "bmdfile.h"
#include "core/strings.h"
#include "log.h"
#include "seh.h"
#include "social.h"
#include "speech.h"
#include <format>

namespace p3a::screens {
using namespace ue;

class RankUpScreen : public Screen {
  Finder draw_{"UICmmRankUpDraw"};
  int rank_ = -1, arcana_ = 0, mode_ = -1;
  std::string line_;   // the game over poem as spoken
  social::Card card_;
  WeakObj spoken_;   // the card last read out: a message window closing re-enters it (a later card can reuse its address)
  bool what_said_ = false, card_said_ = false;

  // Reads the card's rank and link (every read tells the social module the card is up).
  void read() {
    UObject* d = draw_.get();
    if (!d) return;
    int rank = 0, arcana = 0, mode = -1, state = -1;
    guarded([&] {
      rank = *(int32_t*)((uint8_t*)d + 0x2B8);
      UObject* m = manager();
      if (m) {
        arcana = *(int32_t*)((uint8_t*)m + 0x2A0);
        mode = *(int32_t*)((uint8_t*)m + 0x2A4);
        state = *(int32_t*)((uint8_t*)m + 0x294);
      }
    });
    mode_ = mode;
    if (mode == 5) return;   // the game over poem: no card
    if (rank < 0 || rank > 10) rank = 0;
    // The card's rank is set late (0 while the poem shows): the rank-up message's mode says what it will be for a new
    // link (0, CMM_RANKUP mode 0) and rank MAX (2); 1 is a plain rank up.
    if (rank == 0) rank = mode == 0 ? 1 : mode == 2 ? 10 : 0;
    card_ = social::card(arcana, rank);
    if (rank == rank_ && arcana == arcana_) return;
    log::writef("rank_up: rank {} link {} mode {} state {}", rank, arcana, mode, state);
    rank_ = rank; arcana_ = arcana;
  }
  // The game over poem: "Game Over" and the poem's four lines from the event's own message file (dialogs 1 and 0),
  // which the game shows as pictures, one line after another, before its list of what to do.
  static std::string game_over() {
    auto f = bmdfile::load("BMD_Scr_Event_Extr_300_010_C");
    auto dialog = [&](size_t i) {
      std::string out;
      if (!f || !f->ok || i >= f->dialogs.size() || f->dialogs[i].kind != 0) return out;
      for (auto& p : f->dialogs[i].pages) {
        std::string t = bmdfile::text(p);
        // The poem's pages have no end of their own: each runs on through the ones after it.
        if (!t.empty() && !out.ends_with(t)) out += (out.empty() ? "" : " ") + t;
      }
      return out;
    };
    std::string head = dialog(1), poem = dialog(0);
    if (head.empty()) head = strings::kGameOver;
    if (head.back() != '.') head += '.';
    return poem.empty() ? head : head + " " + poem;
  }
  UObject* manager() {
    UObject* d = draw_.get();
    UObject* m = d ? *(UObject**)((uint8_t*)d + 0xB30) : nullptr;
    return m && is_live(m) ? m : nullptr;
  }
  UObject* timeline(const char* name) {
    UObject* d = draw_.get();
    UObject* anim = d ? get<UObject*>(d, "AnimManager") : nullptr;
    UObject* tl = anim && is_live(anim) ? get<UObject*>(anim, name) : nullptr;
    return tl && is_live(tl) ? tl : nullptr;
  }
  static bool started(UObject* tl) { return tl && (call_ret<bool>(tl, "IsPlaying") || call_ret<float>(tl, "GetPlaybackPosition") > 0.0f); }
  static bool finished(UObject* tl) { return tl && !call_ret<bool>(tl, "IsPlaying") && call_ret<float>(tl, "GetPlaybackPosition") > 0.0f; }
  // The rank card's timeline once it has begun to turn in (CardInAnime and Start Reverse InAnime are the poem's).
  UObject* card_timeline() {
    for (const char* t : {"RankUpInAnime", "Reverse Rank In Anime", "RecoveryAnime"})
      if (UObject* tl = timeline(t); started(tl)) return tl;
    return nullptr;
  }
  void speak_card() {
    card_said_ = true;
    // No poem before it (a plain rank up goes straight to the card): what happened leads.
    speech::speak(what_said_ ? card_.line : std::format("{}: {}", card_.what, card_.line), true);
    what_said_ = true;
  }
 public:
  const char* id() const override { return "rank_up"; }
  int priority() const override { return 20; }
  bool over_messages() const override { return true; }
  bool active() override { return draw_.get() != nullptr; }
  void enter() override {
    WeakObj d = WeakObj::of(draw_.get());
    screens::set_keys(strings::kSpaceContinues);
    if (d == spoken_) return;
    spoken_ = d;
    rank_ = -1; arcana_ = 0;
    what_said_ = card_said_ = false;
    read();
  }
  void tick() override {
    int was = rank_;
    read();
    if (mode_ == 5) {
      if (!what_said_) {
        what_said_ = true;
        log::writef("rank_up: the game over poem");
        speech::speak(line_ = game_over(), true);
      }
      return;
    }
    UObject* card = card_timeline();
    // The poem: when its own timeline starts and the card has not (a new link, rank MAX).
    if (!what_said_ && !card && started(timeline("CardInAnime"))) {
      speech::speak(card_.what, true);   // the game's voice reads the poem itself
      what_said_ = true;
    }
    if (!card_said_) {
      // The card is built before its rank is set: it is read once the rank is in, or once it has turned in.
      if (card && (rank_ > 0 || finished(card))) speak_card();
    } else if (was <= 0 && rank_ > 0) {
      speak_card();   // the rank arrived after the card was read
    }
  }
  std::string describe() override {
    if (mode_ == 5) return line_;
    if (card_said_) return card_.line;
    return card_.what;
  }
};

std::unique_ptr<Screen> make_rank_up() { return std::make_unique<RankUpScreen>(); }
}  // namespace p3a::screens

// Audio description: a narrator describes the animated story movies between their lines.
// A story movie is a CRI UManaComponent ("Mana") on BP_MoviePlayer_C (anime movies) or
// BP_MoviePlayerEventSequenceTrack_C (movies inside an event's sequence): GetTime returns an FTimespan (int64,
// 100 ns ticks), GetSource the UManaMovie, IsPlaying stays true while paused (Enter). AudioTrack (+0x1A0) picks
// the voice language: two 5.1 tracks, 0 Japanese and 1 English (the gap maps hold for both).
//
// Data: assets/ad/<key>.txt (one cue a line: at, length, clip, text) and assets/ad/<key>/NN.mp3.
// <key> is the source's name less "_VP9" and a trailing "_Movi" (MS_Event_Main_100_030_M). A movie with no file
// gets nothing. Its clips are decoded on a worker thread when the movie starts (well before the first cue).
// core::ad::Schedule decides what plays when (pauses, seeks, a skip); here its actions reach the mixer's
// narrator, and the movie's own volume (Mana SetVolume) dips under a description. Every cue's end is logged
// ("ad: <key> cue N done|cut ... played P of T s") and kept for /ad/status: the in-game tests read those.
#include "ad.h"
#include "audio.h"
#include "core/ad_schedule.h"
#include "devserver.h"
#include "log.h"
#include "settings.h"
#include "watch.h"
#include "ue/ue.h"
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <format>
#include <memory>
#include <mutex>
#include <thread>

namespace p3a::ad {
using namespace ue;
namespace sched = core::ad;

namespace {
constexpr float kDuck = 0.45f;       // the movie's volume under a description (about -7 dB)
constexpr float kDuckStep = 0.08f;   // per frame toward the target: ~7 frames, ~120 ms at 60 fps

struct Scene {
  std::string key;
  std::string file;   // the movie's path under CriData/Stream ("#file" line)
  std::vector<sched::Cue> cues;
  std::vector<std::string> files, texts;
  std::vector<std::shared_ptr<const std::vector<float>>> pcm;
  std::atomic<bool> ready{false};
  std::atomic<int> failed{0};
};

struct Run {   // one cue's last play, for /ad/status and the tests
  double started = -1;   // movie time
  size_t played = 0, total = 0;
};

std::shared_ptr<Scene> g_scene;
sched::Schedule g_sched;
std::vector<Run> g_runs;
std::vector<sched::State> g_seen;   // the states already logged
int g_voice_id = -1;                // the narrator clip that is ours
int g_cur = -1;
bool g_was_playing = false;
std::string g_key;                  // the movie we last set up for (with or without descriptions)
float g_base_volume = -1, g_volume = -1;
// All of the above is the game thread's: the dev routes read it through dev::on_game.

// The clip decoders: one worker thread per described movie, each owned here until it is joined (once its scene is
// ready, at the next movie's load; or at unload, which first tells them to stop). Their code is in this DLL, so none
// may outlive it. The list is never destroyed: a thread still joinable must not meet its destructor at process exit.
struct Decoder { std::thread thread; std::shared_ptr<Scene> scene; };
std::mutex g_decoders_mu;                // the list (the game thread loads, unload runs on another thread)
std::atomic<bool> g_unloading{false};    // set by unload: no new decoder, and the running ones stop after their clip
std::vector<Decoder>& decoders() { static auto* list = new std::vector<Decoder>; return *list; }

int player_id() { static int id = watch::want("BP_MoviePlayer_C"); return id; }
int track_player_id() { static int id = watch::want("BP_MoviePlayerEventSequenceTrack_C"); return id; }
double seconds(int64_t ticks) { return ticks / 1e7; }

// A movie is under way from its header decode to its end (EManaComponentStatus DecHdr 1 .. Playing 5): a seek
// takes it back through DecHdr/Prep for a few frames, when IsPlaying is false but the movie has not ended.
UObject* mana_of(int watch_id) {
  UObject* p = watch::get(watch_id);
  UObject* m = p ? get<UObject*>(p, "Mana") : nullptr;
  if (!m) return nullptr;
  int st = call_ret<uint8_t>(m, "GetStatus");
  return st >= 1 && st <= 5 ? m : nullptr;
}
UObject* playing_mana() {
  if (UObject* m = mana_of(player_id())) return m;
  return mana_of(track_player_id());
}

std::string key_of(std::string name) {
  // the player's source is a PlatformManaMovie wrapping the platform's FileManaMovie (seen live, 2026-11-02)
  if (name.starts_with("PlatformManaMovie_")) name.erase(0, 18);
  if (auto p = name.find("_VP9"); p != std::string::npos) name.erase(p, 4);
  if (name.size() > 5 && name.ends_with("_Movi")) name.resize(name.size() - 5);
  return name;
}

std::string data_dir() { return audio::module_dir() + "assets\\ad\\"; }

std::shared_ptr<Scene> load_scene(const std::string& key) {
  FILE* f = fopen((data_dir() + key + ".txt").c_str(), "rb");
  if (!f) return nullptr;
  auto s = std::make_shared<Scene>();
  s->key = key;
  char line[2048];
  while (fgets(line, sizeof line, f)) {
    if (strncmp(line, "#file ", 6) == 0) {
      s->file = line + 6;
      while (!s->file.empty() && (s->file.back() == '\n' || s->file.back() == '\r')) s->file.pop_back();
      continue;
    }
    double at = 0, dur = 0;
    char file[64] = {};
    int used = 0;
    if (sscanf(line, "%lf\t%lf\t%63[^\t]\t%n", &at, &dur, file, &used) < 3) continue;
    std::string text = line + used;
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
    s->cues.push_back({at, dur});
    s->files.push_back(file);
    s->texts.push_back(text);
  }
  fclose(f);
  s->pcm.resize(s->cues.size());
  std::lock_guard lk(g_decoders_mu);
  if (g_unloading) return nullptr;
  std::vector<Decoder>& list = decoders();
  // The decoders that have finished are joined here (their last statement sets `ready`: no wait to speak of).
  for (auto it = list.begin(); it != list.end();) {
    if (it->scene->ready) { it->thread.join(); it = list.erase(it); }
    else ++it;
  }
  list.push_back(Decoder{std::thread([s] {
    for (size_t i = 0; i < s->files.size() && !g_unloading; ++i) {
      auto v = std::make_shared<std::vector<float>>();
      if (audio::decode_mono(data_dir() + s->key + "\\" + s->files[i], *v)) s->pcm[i] = std::move(v);
      else ++s->failed;
    }
    s->ready = true;
  }), s});
  return s;
}

void set_movie_volume(UObject* m, float v) {
  UFunction* fn = find_function(class_of(m), "SetVolume");
  if (!fn) return;
  Params p(fn);
  p.set<float>("Volume", v);
  call(m, p);
}

void log_endings(const Scene& s) {
  for (size_t i = 0; i < s.cues.size(); ++i) {
    sched::State st = g_sched.state((int)i);
    if (st == g_seen[i]) continue;
    g_seen[i] = st;
    if (st == sched::State::Playing || st == sched::State::Pending) continue;
    const Run& r = g_runs[i];
    log::writef("ad: {} cue {} {} at {:.2f} {}{}played {:.2f} of {:.2f} s", s.key, i, sched::state_name(st),
                s.cues[i].at, g_sched.reason((int)i), g_sched.reason((int)i).empty() ? "" : ", ",
                r.played / 48000.0, r.total / 48000.0);
  }
}

void stop_all() {
  if (g_cur >= 0) audio::narrate_stop();
  g_cur = -1;
  g_voice_id = -1;
}
}  // namespace

void unload() {
  g_unloading = true;
  std::vector<Decoder> list;
  { std::lock_guard lk(g_decoders_mu); list.swap(decoders()); }
  for (Decoder& d : list) if (d.thread.joinable()) d.thread.join();
}

Clock clock() {
  Clock c;
  UObject* m = playing_mana();
  if (!m) return c;
  c.playing = true;
  c.paused = call_ret<bool>(m, "IsPaused");
  if (UObject* src = call_ret<UObject*>(m, "GetSource")) {
    // The player's source is a PlatformManaMovie wrapping the language's FileManaMovie (ManaMovie), which carries
    // its file ("Movie_VP9/Anim/MS_..._VP9.usm"). The file names the movie: one wrapper can stand for two files
    // (the Episode Aigis opening plays MS_Astrea_OP_Movie_B in English, _A otherwise).
    if (UObject* inner = get<UObject*>(src, "ManaMovie")) src = inner;
    FString* fp = field<FString>(src, "FilePath");
    std::string path = fp ? to_utf8(*fp) : std::string();
    if (auto slash = path.find_last_of("/\\"); slash != std::string::npos) path.erase(0, slash + 1);
    if (path.ends_with(".usm")) path.resize(path.size() - 4);
    c.movie = path.empty() ? name_of(src) : path;
  }
  c.time = seconds(call_ret<int64_t>(m, "GetTime"));
  c.frame = call_ret<int32_t>(m, "GetFrameNumber");
  c.audio_track = get<int32_t>(m, "AudioTrack");
  return c;
}

// ---- the in-game test (/ad/test) ----
// Plays movies through a movie player it spawns (BP_MoviePlayer_C via GameplayStatics), each chosen by its playlist
// id (the data file's "#id" line; see arm), and for each description starts a fresh player half a second before it,
// then waits for the narrator to finish it: the real clock, scheduler, decoder, mixer and ducking, with the
// dialogue between descriptions skipped. A description passes when it ends Done with every sample played. Results: /ad/test (progress) and the
// log ("ad test: ..."). mode=full plays a movie start to finish instead (no seeks).
namespace {
struct Test {
  bool on = false, full = false;
  std::vector<std::string> keys;
  size_t movie = 0;
  int cue = -1;
  enum Step { Open, WaitPlay, Seek, WaitCue, Next } step = Open;
  double fps = 0;   // measured from the movie's own clock before the first seek
  double seek_to = 0;
  int status = -1;   // the player's last Mana status (logged as it changes)
  int arm_frame = 0;
  bool armed = true;
  bool cue_started = false;   // the current description has begun (its time limit runs from then)
  double last_clock = -1;
  ULONGLONG clock_moved = 0;   // when the movie clock last changed (the watchdog)
  int id = -1;   // the movie's playlist index (#id line)
  std::vector<double> ats;   // its descriptions' times (the scene is reloaded with each fresh player)
  int cues = 0;   // the movie's description count (the scene object is replaced each time the movie restarts)
  ULONGLONG since = 0;
  WeakObj player;
  int pass = 0, fail = 0, movies_done = 0;
  ULONGLONG paused_at = 0;   // the pause check: the movie paused mid-description (first cue of each movie)
  int paused_checks = 0;
  std::vector<std::string> failures;
  std::string now;
} g_test;

// A Blueprint class by its object path, loaded if it is not (KismetSystemLibrary::LoadClassAsset_Blocking). The
// TSoftClassPtr parameter (0x28): a weak pointer and tag (0x10, zeroed), then the FSoftObjectPath: the path as an
// FName (+0x10, made by KismetStringLibrary::Conv_StringToName) and an empty sub-path FString (+0x18).
// load_soft("LoadAsset_Blocking", "Asset", path) loads any asset the same way (TSoftObjectPtr has the layout).
UObject* load_soft(const char* fn, const char* param, const std::string& path) {
  UObject* ksl = find_object("/Script/Engine.Default__KismetSystemLibrary");
  UObject* kstr = find_object("/Script/Engine.Default__KismetStringLibrary");
  UFunction* conv = kstr ? find_function(class_of(kstr), "Conv_StringToName") : nullptr;
  UFunction* load = ksl ? find_function(class_of(ksl), fn) : nullptr;
  if (!conv || !load) return nullptr;
  std::u16string w = to_u16(path);
  Params n(conv);
  n.set<FString>("inString", make_fstring(w));
  call(kstr, n);
  FName name = n.ret<FName>();
  Params p(load);
  if (uint8_t* soft = p.at<uint8_t>(param)) memcpy(soft + 0x10, &name, sizeof name);
  call(ksl, p);
  return p.ret<UObject*>();
}
UClass* load_class(const char* path) { return (UClass*)load_soft("LoadClassAsset_Blocking", "AssetClass", path); }

UObject* spawn_player() {
  UClass* cls = find_class("BP_MoviePlayer_C");
  if (!cls) cls = load_class("/Game/Xrd777/Blueprints/Movies/BP_MoviePlayer.BP_MoviePlayer_C");
  UObject* gs = find_object("/Script/Engine.Default__GameplayStatics");
  if (!cls || !gs) return nullptr;
  float xf[12] = {0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0};   // FTransform: identity rotation, origin, unit scale
  UFunction* begin = find_function(class_of(gs), "BeginDeferredActorSpawnFromClass");
  UFunction* finish = find_function(class_of(gs), "FinishSpawningActor");
  if (!begin || !finish) return nullptr;
  Params p(begin);
  p.set<UObject*>("WorldContextObject", gworld());
  p.set<UObject*>("ActorClass", (UObject*)cls);
  if (auto* t = p.at<float>("SpawnTransform")) memcpy(t, xf, sizeof xf);
  p.set<uint8_t>("CollisionHandlingOverride", 1);   // AlwaysSpawn
  call(gs, p);
  UObject* a = p.ret<UObject*>();
  if (!a) return nullptr;
  Params q(finish);
  q.set<UObject*>("Actor", a);
  if (auto* t = q.at<float>("SpawnTransform")) memcpy(t, xf, sizeof xf);
  call(gs, q);
  return a;
}

// Close the test's movie player the way the game does at a movie's end (BP Close(ForceClose): Remove, then
// K2_DestroyActor). Stopping a spawned player and playing it again crashed the game inside CRI (twice,
// 2026-11-02): every start gets a fresh player instead.
void close_player(Test& t) {
  if (UObject* pl = t.player.resolve())
    if (UFunction* fn = find_function(class_of(pl), "Close")) { Params p(fn); p.set<uint8_t>("ForceClose", 1); call(pl, p); }
  t.player = WeakObj{};
}

// A fresh player on `path`, starting at `frame` (Mana SeekToPosition before Play sets the start).
UObject* open_movie(Test& t, int frame) {
  close_player(t);
  UObject* pl = spawn_player();
  UObject* m = pl ? get<UObject*>(pl, "Mana") : nullptr;
  if (!m) return nullptr;
  t.player = WeakObj::of(pl);
  // The Blueprint binds the movie's cue commands from Mana.OnStatusChanged (MovieBindCuePointFunction at WaitPrep)
  // and the first, CueCmd_LoadBmdAsset (FUN_14a63f500), reads a message-system object that is gone the second time
  // a test player starts: null + 0x48, a crash. The test needs no subtitles: unbind the handler (the delegate's
  // binding array count, +8) so neither the cue commands nor the end-of-movie teardown run.
  if (uint8_t* d = field<uint8_t>(m, "OnStatusChanged")) *(int32_t*)(d + 8) = 0;
  // with its status handler unbound the Blueprint plays nothing itself: test_tick picks the movie, the start frame,
  // and plays it (arm)
  t.arm_frame = frame;
  t.armed = false;
  return m;
}

// The second half of open_movie, once the fresh player has stopped.
// The movie is chosen the game's way: its ID (its index in the player's playlist, MPL_MoviePlayer[En], what
// MOVIE_PLAY(id) plays) requested on the actor, which opens it (RequestMovie, CallOpenSourceForBP). SetFile,
// SetMovie and PreparePlaylistIndex on the Mana were all ignored.
void arm(Test& t, UObject* m) {
  t.armed = true;
  UObject* pl = t.player.resolve();
  if (!pl) return;
  if (UFunction* fn = find_function(class_of(pl), "RequestMovie")) { Params p(fn); p.set<int32_t>("ID", t.id); call(pl, p); }
  call(pl, "CallOpenSourceForBP");
  if (t.arm_frame > 0)
    if (UFunction* fn = find_function(class_of(m), "SeekToPosition")) { Params p(fn); p.set<int32_t>("FrameNumber", t.arm_frame); call(m, p); }
  call(m, "Play");
}

void test_fail(const std::string& what) {
  ++g_test.fail;
  g_test.failures.push_back(what);
  log::writef("ad test: FAIL {}", what);
}

void test_tick() {
  Test& t = g_test;
  if (!t.on) return;
  ULONGLONG now = GetTickCount64();
  if (t.movie >= t.keys.size()) {
    t.on = false;
    close_player(t);
    log::writef("ad test: finished: {} movies, {} descriptions passed, {} failed, {} pause checks", t.movies_done,
                t.pass, t.fail, t.paused_checks);
    return;
  }
  const std::string& key = t.keys[t.movie];
  UObject* pl = t.player.resolve();
  UObject* m = pl ? get<UObject*>(pl, "Mana") : nullptr;
  if (m) {   // with the Blueprint's status handler unbound, nothing plays the movie once it is prepared: we do
    int st = call_ret<uint8_t>(m, "GetStatus");
    if (st != t.status) { log::writef("ad test: player status {} -> {}", t.status, st); t.status = st; }
    // watchdog: a movie whose clock stands still for 10 s (outside the deliberate pause) is frozen; fail and move
    // on rather than wait (a frozen player once held a run for half an hour)
    double tm = seconds(call_ret<int64_t>(m, "GetTime"));
    if (tm != t.last_clock || t.paused_at > 1 && now - t.paused_at < 1600 || st != 5) { t.last_clock = tm; t.clock_moved = now; }
    else if (t.clock_moved && now - t.clock_moved > 10000 && (t.step == Test::WaitPlay || t.step == Test::WaitCue)) {
      test_fail(std::format("{} cue {}: the movie froze at {:.2f} s", t.keys[t.movie], t.cue, tm));
      t.clock_moved = now;
      t.step = t.full || t.step == Test::WaitPlay ? Test::Next : Test::Seek;
    }
    if (!t.armed && st == 0) arm(t, m);
    else if (t.armed && st == 4) call(m, "Play");   // Ready
  }
  if (!m && t.step != Test::Open && t.step != Test::Seek && t.step != Test::Next) {
    test_fail(key + ": the movie player went away"); t.step = Test::Next; return;
  }
  std::shared_ptr<Scene> s = g_scene;
  switch (t.step) {
    case Test::Open: {
      FILE* f = fopen((data_dir() + key + ".txt").c_str(), "rb");
      std::string path;
      if (f) {
        char line[512] = {};
        if (fgets(line, sizeof line, f) && strncmp(line, "#file ", 6) == 0) path = line + 6;
        // the second line ("#asset", the movie's asset path) is read past: the movie is chosen by its id
        if (fgets(line, sizeof line, f) && fgets(line, sizeof line, f) && strncmp(line, "#id ", 4) == 0) t.id = atoi(line + 4);
        fclose(f);
      }
      while (!path.empty() && (path.back() == '\n' || path.back() == '\r')) path.pop_back();
      if (path.empty()) { test_fail(key + ": no #file line"); t.step = Test::Next; break; }
      if (!open_movie(t, 0)) { test_fail(key + ": no movie player"); t.on = false; break; }
      t.now = key;
      t.cue = -1;
      t.step = Test::WaitPlay;
      t.since = now;
      log::writef("ad test: {} opens {}", key, path);
      break;
    }
    case Test::WaitPlay:
      if (!(s && s->key == key && s->ready)) {
        if (now - t.since > 15000) { test_fail(key + ": the movie did not start (or has no data)"); t.step = Test::Next; }
        break;
      }
      if (!t.full) {   // the frame rate (29.97 or 30), from the movie's own clock once it has run a second
        double tm = seconds(call_ret<int64_t>(m, "GetTime"));
        if (tm < 1.0) break;
        t.fps = call_ret<int32_t>(m, "GetFrameNumber") / tm;
      }
      t.cues = (int)s->cues.size();
      t.ats.clear();
      for (const auto& c : s->cues) t.ats.push_back(c.at);
      t.step = t.full ? Test::WaitCue : Test::Seek;
      t.cue = t.full ? 0 : -1;
      t.since = now;
      break;
    case Test::Seek:
      // Mana's Seek stops a playing movie (Playing -> StopProcessing -> Stop, seen live) and a stopped spawned
      // player cannot be played again safely: a fresh player starts at the frame; the narrator sees the movie start
      // there and skips what is behind it. A description still sounding (one that began while the frame rate was
      // measured) finishes first, so the next check is not confused by it.
      if (g_sched.playing() >= 0 && now - t.since < 8000) break;
      if (++t.cue >= t.cues) { t.step = Test::Next; break; }
      t.seek_to = std::max(0.0, t.ats[t.cue] - 0.5);
      t.cue_started = false;
      if (!open_movie(t, (int)(t.seek_to * t.fps))) { test_fail(key + ": no movie player"); t.step = Test::Next; break; }
      t.step = Test::WaitCue;
      t.since = now;
      break;
    case Test::WaitCue: {
      if (!s || s->key != key || !s->ready || g_sched.size() != (size_t)t.cues) {   // restarting after the seek
        if (now - t.since > 10000) { test_fail(std::format("{} cue {}: the movie did not come back after the seek", key, t.cue)); t.step = Test::Seek; }
        break;
      }
      if (t.full) {   // wait for the whole movie; judge every cue at the end
        if (now - t.since > 1000 * (ULONGLONG)(s->cues.empty() ? 5 : s->cues.back().at + s->cues.back().dur + 5)) t.step = Test::Next;
        break;
      }
      sched::State st = g_sched.state(t.cue);
      const Run& run = g_runs[t.cue];
      // the first description of each movie: pause the movie halfway through it for 1.5 s (the narrator must hold
      // and then finish, not stop or run on)
      if (t.cue == 0 && st == sched::State::Playing && !t.paused_at && run.total && run.played > run.total / 2) {
        if (UFunction* fn = find_function(class_of(m), "Pause")) { Params p(fn); p.set<uint8_t>("bPause", 1); call(m, p); }
        t.paused_at = now;
        log::writef("ad test: {} paused the movie at {} of {} samples", key, run.played, run.total);
      }
      if (t.paused_at > 1 && now - t.paused_at > 1500) {
        if (audio::narration().played > run.played + 4800) test_fail(key + ": the narrator ran on while the movie was paused");
        if (UFunction* fn = find_function(class_of(m), "Pause")) { Params p(fn); p.set<uint8_t>("bPause", 0); call(m, p); }
        t.paused_at = 1;   // done for this movie
        ++t.paused_checks;
        t.since = now;
      }
      bool final = st != sched::State::Pending && st != sched::State::Playing;
      // the time limit runs from the description's start, not the request: event movies take seconds to open
      if (st == sched::State::Playing && !t.cue_started) { t.cue_started = true; t.since = now; }
      ULONGLONG limit = t.cue_started ? (ULONGLONG)(1000 * (s->cues[t.cue].dur + 4)) : 15000;
      if (!final && now - t.since < limit) break;
      const Run& r = g_runs[t.cue];
      if (st == sched::State::Done && r.total > 0 && r.played == r.total) ++t.pass;
      else test_fail(std::format("{} cue {} at {:.2f}: {} {} (played {} of {} samples)", key, t.cue, s->cues[t.cue].at,
                                 sched::state_name(st), g_sched.reason(t.cue), r.played, r.total));
      t.step = Test::Seek;
      break;
    }
    case Test::Next:
      if (t.full && s && s->key == key)
        for (size_t i = 0; i < s->cues.size(); ++i) {
          const Run& r = g_runs[i];
          if (g_sched.state((int)i) == sched::State::Done && r.total > 0 && r.played == r.total) ++t.pass;
          else test_fail(std::format("{} cue {} at {:.2f}: {} {}", key, i, s->cues[i].at, sched::state_name(g_sched.state((int)i)), g_sched.reason((int)i)));
        }
      close_player(t);
      t.paused_at = 0;
      ++t.movies_done;
      ++t.movie;
      t.step = Test::Open;
      break;
  }
}
}  // namespace

void tick() {
  test_tick();
  UObject* m = playing_mana();
  bool playing = m != nullptr;
  if (!playing && !g_was_playing && g_sched.playing() < 0) return;   // nothing on, nothing finishing
  if (!settings::on(settings::AudioDescription)) {
    if (g_was_playing) { stop_all(); g_was_playing = false; g_scene.reset(); }
    return;
  }
  Clock c = playing ? clock() : Clock{};
  if (playing && (!g_was_playing || g_key != key_of(c.movie))) {
    // a movie starts (or the source changed under a running player): load its descriptions, if it has any
    stop_all();
    g_key = key_of(c.movie);
    auto s = load_scene(g_key);
    g_scene = s;
    g_sched.load(s ? s->cues : std::vector<sched::Cue>{});
    g_runs.assign(g_sched.size(), Run{});
    g_seen.assign(g_sched.size(), sched::State::Pending);
    g_base_volume = call_ret<float>(m, "GetVolume", 1.0f);
    g_volume = g_base_volume;
    log::writef("ad: movie {} ({}), {} descriptions", c.movie, key_of(c.movie), s ? s->cues.size() : 0);
  }
  g_was_playing = playing;
  std::shared_ptr<Scene> s = g_scene;
  if (!s || !s->ready) return;

  audio::Narration n = audio::narration();
  bool ours = n.id == g_voice_id && g_voice_id >= 0;
  if (ours && g_cur >= 0) { g_runs[g_cur].played = n.played; g_runs[g_cur].total = n.total; }
  sched::Voice v{ours && n.active, ours && n.ended};
  for (const sched::Action& a : g_sched.step({playing, c.paused, c.time}, v)) {
    switch (a.act) {
      case sched::Act::Play:
        if (!s->pcm[a.cue]) break;   // a clip that would not decode: the cue is logged as cut by the narrator
        g_voice_id = audio::narrate(s->pcm[a.cue]);
        g_cur = a.cue;
        g_runs[a.cue] = Run{c.time, 0, s->pcm[a.cue]->size()};
        log::writef("ad: {} cue {} plays at {:.2f} (due {:.2f}): {}", s->key, a.cue, c.time, s->cues[a.cue].at, s->texts[a.cue]);
        break;
      case sched::Act::Pause: audio::narrate_pause(true); break;
      case sched::Act::Resume: audio::narrate_pause(false); break;
      case sched::Act::Stop: audio::narrate_stop(); g_cur = -1; break;
    }
  }
  if (g_sched.playing() < 0) g_cur = -1;
  log_endings(*s);
  if (!playing) {   // the movie is over: keep the scene until a last description has played out
    if (g_sched.playing() < 0) { g_scene.reset(); g_key.clear(); }
    return;
  }
  // the movie's own sound dips while a description speaks
  float target = g_base_volume * (g_cur >= 0 && !c.paused ? kDuck : 1.0f);
  if (g_volume != target) {
    g_volume = g_volume < target ? std::min(target, g_volume + kDuckStep) : std::max(target, g_volume - kDuckStep);
    set_movie_volume(m, g_volume);
  }
}

void register_routes() {
  player_id();
  track_player_id();
  dev::route("/ad/clock", [](const dev::Request&, int&) {
    return dev::on_game([] {
      Clock c = clock();
      if (!c.playing) return std::string("playing=0\n");
      return std::format("playing=1 paused={} movie={} time={:.3f} frame={} track={}\n", c.paused ? 1 : 0, c.movie,
                         c.time, c.frame, c.audio_track);
    });
  }, "the story movie clock (audio description): movie, time s, frame, pause, voice track");
  dev::route("/ad/status", [](const dev::Request&, int&) {
    return dev::on_game([] {
      std::shared_ptr<Scene> s = g_scene;
      if (!s) return std::string("no described movie\n");
      std::string out = std::format("movie {} ready={} failed_clips={} cues={} playing={}\n", s->key, s->ready.load(),
                                    s->failed.load(), s->cues.size(), g_sched.playing());
      for (size_t i = 0; i < s->cues.size(); ++i)
        out += std::format("{:3} {:8.2f} {:8} played {:.2f}/{:.2f} {}\n", i, s->cues[i].at,
                           sched::state_name(g_sched.state((int)i)), g_runs[i].played / 48000.0, g_runs[i].total / 48000.0,
                           g_sched.reason((int)i));
      return out;
    });
  }, "the described movie: every cue's state and how much of it played");
  dev::route("/ad/test", [](const dev::Request& r, int&) {
    std::string movie = dev::arg(r, "movie");
    bool start = dev::has(r, "start"), full = dev::arg(r, "mode") == "full", stop = dev::has(r, "stop");
    return dev::on_game([movie, start, full, stop] {
      Test& t = g_test;
      if (stop) { t.on = false; close_player(t); return std::string("stopped\n"); }
      if (start) {
        // a player left from an earlier run (a stop before a hot reload) is the one the clock would read: close
        // every movie player first (a scan, once per run)
        for (UObject* old : find_all("BP_MoviePlayer_C"))
          if (UFunction* fn = find_function(class_of(old), "Close")) { Params p(fn); p.set<uint8_t>("ForceClose", 1); call(old, p); }
        t = Test{};
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA((data_dir() + "*.txt").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
          do {
            std::string k = fd.cFileName;
            k.resize(k.size() - 4);
            if (movie.empty() || k.find(movie) != std::string::npos) t.keys.push_back(k);
          } while (FindNextFileA(h, &fd));
          FindClose(h);
        }
        t.full = full;
        t.on = !t.keys.empty();
        log::writef("ad test: starts ({} movies, {})", t.keys.size(), full ? "full playback" : "seek to each description");
      }
      std::string out = std::format("running={} movie {}/{} {} cue {} passed={} failed={} pause_checks={}\n", t.on,
                                    t.movie, t.keys.size(), t.now, t.cue, t.pass, t.fail, t.paused_checks);
      for (const auto& f : t.failures) out += "FAIL " + f + "\n";
      return out;
    });
  }, "?start=1[&movie=filter][&mode=full] play described movies in game and check every description plays in full; no args: progress");
  dev::route("/ad/selftest",[](const dev::Request& r, int&) {
    // Every packed clip of every movie decoded and its length compared with the cue's (a fast check of the shipped
    // files, not of the timing or the mixer). Runs on the server's thread: it touches no game state.
    std::string filter = dev::arg(r, "movie");
    return selftest(filter);
  }, "?movie=filter decode every packed description clip and report frames (shipping check)");
}

std::string selftest(const std::string& filter) {
  WIN32_FIND_DATAA fd;
  HANDLE h = FindFirstFileA((data_dir() + "*.txt").c_str(), &fd);
  if (h == INVALID_HANDLE_VALUE) return "no assets\\ad\n";
  int movies = 0, clips = 0, bad = 0;
  double total = 0;
  std::string out;
  do {
    std::string key = fd.cFileName;
    key.resize(key.size() - 4);
    if (!filter.empty() && key.find(filter) == std::string::npos) continue;
    FILE* f = fopen((data_dir() + fd.cFileName).c_str(), "rb");
    if (!f) continue;
    ++movies;
    char line[2048];
    while (fgets(line, sizeof line, f)) {
      double at = 0, dur = 0;
      char file[64] = {};
      if (sscanf(line, "%lf\t%lf\t%63[^\t]", &at, &dur, file) < 3) continue;
      std::vector<float> v;
      ++clips;
      bool ok = audio::decode_mono(data_dir() + key + "\\" + file, v);
      double got = v.size() / 48000.0;
      total += got;
      // MP3 adds up to ~50 ms of encoder padding; a clip shorter than its voiced length lost its end
      if (!ok || got < dur - 0.03 || got > dur + 0.12) {
        ++bad;
        out += std::format("BAD {} {} at {:.2f}: decoded {:.3f} s, voiced {:.3f} s\n", key, file, at, got, dur);
      }
    }
    fclose(f);
  } while (FindNextFileA(h, &fd));
  FindClose(h);
  return out + std::format("selftest: {} movies, {} clips, {:.1f} min decoded, {} bad\n", movies, clips, total / 60, bad);
}
}  // namespace p3a::ad

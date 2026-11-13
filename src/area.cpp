// The area key: a written description of the place the player stands in (src/core/area_text holds the words).
// What the game shows is read from its own state, never guessed from the clock alone:
// - the area: the field ids of the visible hit actors' levels (poi::area_ids), else the banner's name (Tartarus);
// - the light: the common lighting level the field streams for the time of day (LV_FEnv_Noon/Evening/Night/
//   Shadow_Lighting, /Game/Xrd777/Maps/Field/Common), the area's own BG_<time>_Only set as a fallback;
// - the storey of a multi-storey area: the player's height (core::area::floor_of, measured per building);
// - the season, Christmas and January's decline: the calendar, on the dates DT_FldVariationSubLevelTable streams
//   their sets; the dorm's Christmas tree is its own object (BP_F102_102_Christmas, shown or hidden by the game).
#include "area.h"
#include "flow.h"
#include "core/area_text.h"
#include "core/strings.h"
#include "devserver.h"
#include "calendar.h"
#include "poi.h"
#include "portrait.h"
#include "seh.h"
#include "speech.h"
#include "ue/scan.h"
#include "watch.h"
#include "world.h"
#include <algorithm>
#include <format>
#include <map>

namespace p3a::area {
using namespace ue;
namespace ca = core::area;

static bool has(const std::vector<std::string>& v, std::string_view part) {
  for (auto& s : v) if (s.find(part) != std::string::npos) return true;
  return false;
}

static ca::Light light_of(const std::vector<std::string>& levels) {
  if (has(levels, "Shadow_Lighting") || has(levels, "_Shadow_Only")) return ca::Light::DarkHour;
  if (has(levels, "Night_Lighting")) return ca::Light::Night;
  if (has(levels, "Evening_Lighting")) return ca::Light::Evening;
  if (has(levels, "Noon_Lighting")) return ca::Light::Day;
  if (has(levels, "_Night_Only")) return ca::Light::Night;
  if (has(levels, "_Evening_Only")) return ca::Light::Evening;
  return ca::Light::Day;
}

static bool tree_shown() {
  static const int id = watch::want("BP_F102_102_Christmas_C");
  UObject* t = watch::get(id);
  return t && world::actor_visible_level(t) && !get_bool(t, "bHidden");
}

static ca::Scene scene(std::vector<std::string>* levels_out = nullptr) {
  ca::Scene s;
  poi::area_ids(s.major, s.minor);
  s.name = poi::area_name();
  if (calendar::Now n = calendar::now(); n.valid) { s.month = n.month; s.day = n.day; }
  std::vector<std::string> levels = world::visible_levels();
  s.light = light_of(levels);
  // A field with no hit actors loaded (150_211, 201_103 in the Dark Hour, the sweep of 2026-11-02): its own streamed
  // levels name it (LV_F150_211_001_BG).
  if (!s.major) {
    std::map<std::pair<int, int>, int> votes;
    for (auto& l : levels) {
      int ma = 0, mi = 0;
      if (sscanf(l.c_str(), "LV_F%d_%d", &ma, &mi) == 2 && ma >= 100) ++votes[{ma, mi}];
    }
    if (!votes.empty()) {
      auto best = std::max_element(votes.begin(), votes.end(), [](auto& a, auto& b) { return a.second < b.second; })->first;
      s.major = best.first; s.minor = best.second;
    }
  }
  world::FVector me{};
  if (world::player_location(me)) s.floor = ca::floor_of(s.major, s.minor, me.z);
  s.christmas_tree = s.major == 102 && tree_shown();
  // A Monad door's area is a section of the same floor far above it (z ~7600 on Adamah 250F, 2026-11-02); the banner
  // keeps the block's name there. Its return door (BP_DHIT_FORBIDDEN_DOOR_RET) stands in that section.
  static const int ret_id = watch::want("BP_DHIT_FORBIDDEN_DOOR_RET_C", true);
  for (UObject* d : watch::all(ret_id)) {
    if (!world::actor_visible_level(d)) continue;
    world::FVector at = world::actor_location(d);
    if (std::abs(at.z - me.z) < 1500.0f) {
      // Episode Aigis calls its hidden sections the Monad Chambers (the Abyss of Time's labyrinths, dungeon 499).
      bool abyss = false;
      for (const char* n : {"Malebolge", "Cocytus", "Caina", "Antenora", "Ptolomea", "Judecca", "Empyrean"})
        if (s.name.find(n) != std::string::npos) abyss = true;
      s.name = abyss ? strings::kMonadChambers : strings::kMonad;
      s.major = 0;
      break;
    }
  }
  if (levels_out) *levels_out = std::move(levels);
  return s;
}

void describe() {
  if (portrait::describe()) return;   // talking to someone: the character, not the place
  if (!world::player()) { speech::speak(strings::kNotInField, true); return; }
  std::string d = ca::describe(scene());
  speech::speak(d.empty() ? std::string(strings::kNoAreaDescription) + " " + poi::area_name() : d, true);
}

// Field change state (P3R.exe 0x69ce3439, read 2026-11-12). UFldManagerSubsystem +0x48 holds the field manager's
// current state: an 8-byte object whose vtable's second slot is the step it runs each frame. Idle in a field is the
// state with vtable rva 0x42b7548 (FUN_1411b52d0: waits for a field-change request bit in +0x40). A CALL_FIELD walks
// it through the loading states to 0x42b76c8 (FUN_1411b4bf0), which waits for the floor's init script
// (mInitScrManager_) and only then goes back to idle. Tartarus' init script (BF_FldInitScript_299_all) starts a field
// event there on a gatekeeper floor above the highest one reached: an AFldFieldEventActor (mFieldEventActor_) whose
// last state (vtable 0x42b7480, FUN_14a3c2880; the actor's state is its +0x288) places the followers around
// mPlayerComp_. A CALL_FIELD in that window tore the hero down first, and the follower placement (FUN_14113cf00) read
// +0xA0 of the null player (P3R.exe+0x113cf1b): `jump --set field=299,49` then `field=299,50`, 2026-11-12. The dev
// routes refuse a CALL_FIELD until the field is back in idle with no field event and the hero placed.
std::string field_busy() {
  static const int sub_id = watch::want("FldManagerSubsystem");
  UObject* sub = watch::get(sub_id);
  if (!sub) return "no field manager";
  if (auto ev = field<UObject*>(sub, "mFieldEventActor_"); ev && *ev) return "a field event is running";
  if (auto pc = field<UObject*>(sub, "mPlayerComp_"); !pc || !*pc) return "the hero is not placed";
  if (!ue::known_build()) return "";   // the state's vtable is this build's
  uintptr_t vt = 0;
  if (!guarded([&] { if (auto st = *(uint8_t**)((uint8_t*)sub + 0x48)) vt = *(uintptr_t*)st; })) return "the field state is unreadable";
  uintptr_t rva = vt ? vt - scan::exe_base() : 0;
  if (rva == 0x42b7548) return "";
  // 0x42b76c8 / 0x42b7718 (FUN_1411b4bf0 / FUN_1411b4590): the area's init script runs (a floor's arrival message waits
  // for a press there: Fuuka's on Tartarus 50, 2026-11-12).
  if (rva == 0x42b76c8 || rva == 0x42b7718) return "its init script is running";
  return std::format("the field is changing (state {:#x})", rva);
}

void install() {
  dev::route("/area/ready", [](const dev::Request&, int&) {
    return dev::on_game([] {
      std::string why = field_busy();
      return why.empty() ? std::string("ready\n") : "busy: " + why + "\n";
    });
  }, "whether the field has settled after a field change (a CALL_FIELD before then can crash the game) (dev)");
  dev::route("/area", [](const dev::Request&, int&) {
    return dev::on_game([] {
      std::vector<std::string> levels;
      ca::Scene s = scene(&levels);
      world::FVector me{};
      world::player_location(me);
      static const char* lights[] = {"day", "evening", "night", "dark hour"};
      std::string out = std::format("ids {} {} name '{}' date {}/{} light {} z {:.0f} floor {} tree {}\n", s.major, s.minor, s.name,
                                    s.month, s.day, lights[(int)s.light], me.z, s.floor, s.christmas_tree);
      for (auto& l : levels) if (l.starts_with("LV_F")) out += "  " + l + "\n";
      return out + ca::describe(s) + "\n";
    });
  }, "the area key's scene (ids, light, storey, visible levels) and its description");
  // Dev: load any field area: the field scripts' CALL_FIELD(major, minor, entry) command, run as a script runs it
  // (flow::run). Used to check descriptions in places the dev saves cannot reach by playing (the trips, the later
  // Tartarus blocks).
  dev::route("/area/goto", [](const dev::Request& r, int&) {
    int major = (int)dev::arg_int(r, "major"), minor = (int)dev::arg_int(r, "minor"), entry = (int)dev::arg_int(r, "entry");
    if (!entry) entry = 1;
    return dev::on_game([=] {
      if (!ue::known_build()) return std::string("another build of the game\n");   // the script context is this build's
      return flow::run("CALL_FIELD", std::format("{},{},{}", major, minor, entry));
    });
  }, "?major=&minor=[&entry=1] load a field area as the field scripts' CALL_FIELD does (dev)");
  // Dev: DUNGEON_MOVE_FDOOR + DUNGEON_MOVE_FREEID(id), as the Monad door does (BF_FldShortCut: ids 10-13): move the
  // party to that spot of the floor's layout.
  dev::route("/area/freeid", [](const dev::Request& r, int&) {
    int id = (int)dev::arg_int(r, "id");
    bool door = dev::arg_int(r, "fdoor") != 0;
    return dev::on_game([=] {
      if (!ue::known_build()) return std::string("another build of the game\n");
      return (door ? flow::run("DUNGEON_MOVE_FDOOR", "") : std::string()) + flow::run("DUNGEON_MOVE_FREEID", std::to_string(id));
    });
  }, "?id=[&fdoor=1] move within the dungeon floor as the Monad door does (dev)");
}
}  // namespace p3a::area

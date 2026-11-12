// The points of interest around the hero: the scan every list, sound and prompt reads, and the module's tick.
// Read here:
//  - AFldHitCore (every hit actor on a visible level): mCheckIcon_, mType_ (Normal / AreaChange), mNameIndex_,
//    mKeyPushBluePrint_ / mOverlapBluePrint_ (an unnamed box with only an overlap blueprint is a trigger), and the
//    flags that retire it: mExecutedFlagName_, mOffFlagName_, mIgnoreFlagName_.
//  - AFldNpcActor: bHidden, Children, mNpcBaseClass_ / mCharaBaseClass_ (a placeholder has none of them),
//    mCharaModelList_ (empty: no character stands there).
//  - AFldDungeonObjectActor: ObjType and m_pHitActorList (a chest's kind for its hit), Box (a breakable's body).
//  - UFldDungeonMissingController.m_pMissingPerson, +0x108 / +0x168 (its case: missing_name); AEnemySymbolBaseCore
//    (the roaming Shadows) with the enemy manager's holders (shadow::enemy_holders).
#include "poi_internal.h"
#include "bmdfile.h"
#include "core/poi_text.h"
#include "core/strings.h"
#include "core/text_clean.h"
#include "party.h"
#include "devserver.h"
#include "doors.h"
#include "hooks.h"
#include "msg.h"
#include "screens/screen.h"
#include "seh.h"
#include "settings.h"
#include "shadow.h"
#include "world.h"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <map>
#include <memory>

namespace p3a::poi {
using namespace ue;

// A complete navmesh path ends near the thing. Kept per actor while the hero stays on his side of the map (floors
// are joined by stairs that jump him: doors::side_stamp counts those, doors passed and levels changing).
static bool walkable(UObject* a, const world::FVector& me, const world::FVector& v) {
  static std::map<WeakObj::Key, bool> cache;
  static uint64_t side = 0;
  if (uint64_t s = doors::side_stamp(); s != side) { side = s; cache.clear(); }
  auto key = WeakObj::of(a).key();
  auto it = cache.find(key);
  if (it != cache.end()) return it->second;
  bool partial = true;
  auto path = world::find_path(me, v, &partial);
  bool ok = !partial && !path.empty() && std::fabs(path.back().z - v.z) < 250.0f &&
            std::hypot(path.back().x - v.x, path.back().y - v.y) < 300.0f;
  cache[key] = ok;
  return ok;
}

// The story flag an actor's FName property names is on (false when the property is unset or "None").
static bool flag_prop_on(UObject* a, const char* prop) {
  auto* f = field<FName>(a, prop);
  if (!f || f->index <= 0) return false;
  std::string flag = fname(*f);
  return !flag.empty() && flag != "None" && flag_on(flag);
}

// The name of the missing person on this floor, as the police board's notice gives it, or "" while the board does
// not show it yet. UFldDungeonMissingController: +0x168 a case is placed, +0x108 its data, whose +0x68 is the case
// number (what DUNGEON_GET_MISSING_ID returns, FUN_14117e120). The name is the speaker of that dialog of
// BMD_DisappearHelp, shown once the case's InfoOn flag is set (DisappearDataAsset +0x30: a TMap, elements of 0x30,
// key at +0, the case's flags Active, Success, Failed, Npc, InfoOn, Award at +8), as the Missing Persons list does.
static std::string missing_name(UObject* controller) {
  if (!controller || !ue::known_build()) return {};
  int no = 0;
  if (!guarded([&] {
        uint8_t* c = (uint8_t*)controller;
        uint8_t* data = c[0x168] ? *(uint8_t**)(c + 0x108) : nullptr;
        no = data ? *(int32_t*)(data + 0x68) : 0;
      }) || no <= 0)
    return {};
  static const int asset_id = watch::want("DisappearDataAsset");
  UObject* d = watch::get(asset_id);
  if (!d) return {};
  auto* map = (TArray<uint8_t>*)((uint8_t*)d + 0x30);
  const int32_t* fl = nullptr;
  for (int i = 0; map->data && i < map->num && i < 200; ++i)
    if (*(uint16_t*)(map->data + i * 0x30) == no) fl = (const int32_t*)(map->data + i * 0x30 + 8);
  if (!fl || !party::flag(fl[4])) return {};
  auto f = bmdfile::load("BMD_DisappearHelp");
  if (!f || !f->ok || no >= (int)f->dialogs.size()) return {};
  int sp = f->dialogs[no].speaker;
  return sp >= 0 && sp < (int)f->speakers.size() ? core::clean_text(f->speakers[sp]) : std::string();
}

bool breakable(UObject* a) {
  if (!a) return false;
  std::string cn = name_of(class_of(a));
  if (cn.find("Broken") == std::string::npos || get_bool(a, "bHidden")) return false;
  if (UObject* box = get<UObject*>(a, "Box")) {
    UFunction* f = find_function(class_of(box), "GetCollisionEnabled");
    if (f) { Params p(f); call(box, p); if (p.ret<uint8_t>() == 0) return false; }
  }
  return true;
}

std::vector<Poi> scan(float radius) {
  if (radius < 0) radius = settings::get(settings::PoiRange) * 100.0f;
  std::vector<Poi> out;
  world::FVector me;
  if (!world::player_location(me)) return out;
  UObject* hero = world::player();
  bool dungeon = hero && name_of(class_of(hero)).find("DUNGEON") != std::string::npos;
  // Whether the unnamed triggers are listed (below) is one answer for every caller: it goes by what the list's own
  // range holds, not by `radius`. The sonar reads 10 m around the player, found nothing else that near, and sounded
  // a trigger the list (60 m, a chest and the stairs in it) did not have (a Tartarus floor, 2026-11-11).
  const float list_radius = std::max(radius, settings::get(settings::PoiRange) * 100.0f);
  bool beyond = false;   // something past `radius` that the list has
  // The icon the game floats over a person: an actor of its own, BP_UI_AccessIcon_<Kind>_C, whose Owner is the person
  // (the dorm lounge, 2026-11-11: five Talk icons owned by the five people there; Akihiko's became Talk_Read once he
  // was talked to). Hidden while a conversation runs.
  static const char* const kIconKinds[] = {"Talk_Read", "Listen_Read", "Commu", "Commu_Field", "Commu_Reverse", "DormitoryLife",
                                           "MaleQuest", "MaleQuest_Field", "Quest", "StudyGroup", "Walking"};
  static const std::vector<int> icon_ids = [] {
    std::vector<int> ids;
    for (const char* k : kIconKinds) ids.push_back(watch::want(std::format("BP_UI_AccessIcon_{}_C", k), true));
    return ids;
  }();
  std::map<UObject*, const char*> icons;   // a person -> what their icon says
  for (size_t i = 0; i < icon_ids.size(); ++i)
    for (UObject* icon : watch::all(icon_ids[i])) {
      if (!is_live(icon) || get_bool(icon, "bHidden")) continue;
      if (UObject* owner = get<UObject*>(icon, "Owner")) icons[owner] = core::icon_state(kIconKinds[i]);
    }
  auto add = [&](UObject* a, Kind k, std::string name, bool any_floor = false) {
    world::FVector v{};
    if (!guarded([&] { v = world::actor_location(a); })) return;
    float d = std::sqrt((v.x - me.x) * (v.x - me.x) + (v.y - me.y) * (v.y - me.y) + (v.z - me.z) * (v.z - me.z));
    // Floors of one building share a level, stacked about 3.4 m apart and joined by teleporting stairs (the dorm,
    // 2026-10-27): another floor's things count only when the navmesh walks there (Tartarus' raised entrance does).
    // Tartarus floors are levels of their own with raised and sunken parts: no floor filter there.
    bool other_floor = !dungeon && !any_floor && std::fabs(v.z - (me.z - 90.0f)) > 280.0f;
    if (d > radius) { beyond |= d <= list_radius && !other_floor; return; }
    if (other_floor && !walkable(a, me, v)) return;
    // Another part of the map, past a closed door (the corridor's things from the gym hallway, 2026-11-11): listed
    // once the hero is through it.
    if (doors::behind(a, me, v)) return;
    // The daily and the Dark Hour version of a hit often stand on the same spot: one entry per name and place.
    for (auto& o : out)
      if (core::same_thing(o.name, world::vec3(o.loc), name, world::vec3(v))) return;
    out.push_back(Poi{WeakObj::of(a), k, std::move(name), v, d, world::bearing(me, v)});
    if (auto it = icons.find(a); it != icons.end()) out.back().state = it->second;
  };
  std::vector<UObject*> triggers;
  // A story trigger named for its scene, FldHitActor_<major>_<Scene>_H<n> (FldHitActor_299_Takaya_H0 and _Nyx_H0 at
  // the top of Tartarus, 2026-10-28), is where the story goes next: listed beside everything else, on any floor
  // (Takaya's box floats 7 m up a ramp, with no navmesh path to its centre), until its off flag is set.
  // Seen at Adamah floor 263 (2026-11-11): "Event: Takaya" and "Event: Nyx" listed from the landing, and the walk
  // to each started its fight.
  auto story_beat = [](UObject* a) -> std::string {
    std::string w = core::story_scene(name_of(a));
    if (w.empty() || flag_prop_on(a, "mOffFlagName_")) return {};
    return w;
  };
  // A thing farther off than the list's own range cannot be in the result, nor change it: it is passed over before
  // its name and flags are read (most of a town's hit actors, every scan).
  auto out_of_range = [&](UObject* a) {
    world::FVector v{};
    if (!guarded([&] { v = world::actor_location(a); })) return true;
    return std::sqrt((v.x - me.x) * (v.x - me.x) + (v.y - me.y) * (v.y - me.y) + (v.z - me.z) * (v.z - me.z)) > list_radius;
  };
  std::map<UObject*, std::string> chests;   // a chest's hit -> its dungeon object's type
  for (UObject* o : live("FldDungeonObjectActor")) {
    auto* hits = field<TArray<UObject*>>(o, "m_pHitActorList");
    if (!hits) continue;
    guarded([&] {
      std::string type = enum_prop_name(o, "ObjType");
      if (!type.starts_with("TBox") || !hits->data || hits->num <= 0 || hits->num > 16) return;
      for (int i = 0; i < hits->num; ++i) if (hits->data[i]) chests[hits->data[i]] = type;
    });
  }
  // A missing person on this Tartarus floor (UFldDungeonMissingController.m_pMissingPerson, a BP_FldMisNPC_<n>_C;
  // the rescue scene's script destroys the actor and clears the pointer, DUNGEON_DESTROY_MISSING_NPC).
  static const int missing_id = watch::want("FldDungeonMissingController");
  UObject* missing = nullptr;
  if (UObject* mc = watch::get(missing_id)) missing = get<UObject*>(mc, "m_pMissingPerson");
  if (missing && (!is_live(missing) || get_bool(missing, "bHidden"))) missing = nullptr;
  for (UObject* a : live("FldHitCore")) {
    if (!world::actor_visible_level(a) || is_a(a, "FldNpcActor") || out_of_range(a)) continue;
    std::string icon = icon_of(a);
    // The missing person's only hit has no icon: a sphere that starts the rescue when walked into
    // (BP_DHIT_MISNPC_C, overlap BP_FldHit_MISNPC_01_HERO_C). Named by the game's hit-name table: "Stray Cat",
    // "Old Man in Pain", else "Missing Person" (row 10009013).
    // Seen on floor 50 (2026-11-12): listed under People, the walk into it started the rescue.
    if (icon == "None" && missing && name_of(get<UObject*>(a, "mOverlapBluePrint_")) == "BP_FldHit_MISNPC_01_HERO_C") {
      std::string nm = hit_name(a);
      if (nm.empty()) {
        nm = kin_name({"DT_FldDungeonHitName"}, 10009013);
        if (nm.empty()) nm = strings::kPoiMissingPerson;
        // The person's name from the police board's notice, as the Missing Persons list says it (seen on floor 50,
        // 2026-11-12: "Missing Person, Ayako Yoshimoto"); the hit's own name stays first.
        if (std::string who = missing_name(watch::get(missing_id)); !who.empty()) nm += ", " + who;
      }
      add(a, Kind::Person, nm);
      continue;
    }
    if (icon == "None") continue;
    // Story triggers: unnamed boxes that only run a blueprint when walked through (the first Tartarus floor's
    // tutorial gates, 2026-10-27). Nothing there to use, but where the story leads: kept for an area with nothing
    // else (the monorail tracks, whose way on is a row of them, 2026-10-27).
    bool trigger = get<int32_t>(a, "mNameIndex_", -1) < 0 && !get<UObject*>(a, "mKeyPushBluePrint_") && get<UObject*>(a, "mOverlapBluePrint_");
    if (trigger)
      if (std::string beat = story_beat(a); !beat.empty()) { add(a, Kind::Event, std::format("{}: {}", strings::kPoiEvent, beat), true); continue; }
    // Done already: an opened treasure chest keeps its hit, with its story flag on (FLD_TAR_TBOX_OPENED_04).
    if (flag_prop_on(a, "mExecutedFlagName_")) continue;
    // Turned off: a picked-up resource spot ("Something Shining", a Twilight Fragment) keeps its hit with its off flag
    // on (FLD_RESOURCE_SPOT10, Paulownia Mall 2026-10-31).
    if (flag_prop_on(a, "mOffFlagName_")) continue;
    // The game's own "is this hit on" test fails on a second flag too, mIgnoreFlagName_ (a few unnamed triggers).
    // UNTESTED: from the game's code; none of those triggers was met with its flag on.
    if (flag_prop_on(a, "mIgnoreFlagName_")) continue;
    if (trigger) {
      // A Gatekeeper's floor: the box that starts its fight (BP_FldHit_Tartarus_GateKeeper, the Abyss of Time's too),
      // off once it is beaten (FLD_BATTLE_DEFEAT_GK_*, checked above). Always listed: it is what the floor is for.
      // The other unnamed boxes were never checked against their flags: a finished one is left out now.
      if (name_of(get<UObject*>(a, "mOverlapBluePrint_")) == "BP_FldHit_Tartarus_GateKeeper_C") add(a, Kind::Event, strings::kPoiGatekeeper);
      else triggers.push_back(a);
      continue;
    }
    std::string type = enum_prop_name(a, "mType_");
    // A Social Link partner stands in the world as a hit actor of its own (FldCmmActor), hidden while away.
    bool cmm = is_a(a, "FldCmmActor");
    if (cmm && get_bool(a, "bHidden")) continue;
    std::string nm = cmm ? npc_name(a) : hit_name(a);
    if (type == "AreaChange" || icon == "Goto") add(a, Kind::Exit, nm.empty() ? strings::kPoiExit : nm);
    else if (icon == "Shop") add(a, Kind::Shop, nm.empty() ? strings::kPoiShop : nm);
    else if (icon == "Speak" || icon == "Listen") add(a, Kind::Person, nm.empty() ? strings::kPoiPerson : nm);
    // Tartarus' doors are ways onward under the plain Check icon: the Monad door on a floor ("Enter Door", row
    // 10009006; seen on Adamah floors 231 and 243, 2026-11-11), and in the passage "Exit Door", "Proceed Onward" and
    // the way back (10009007-10009009, 10009012; all three seen listed on Adamah floor 255, 2026-11-11).
    else if (int idx = get<int32_t>(a, "mNameIndex_", -1); ((idx >= 10009006 && idx <= 10009009) || idx == 10009012) && !nm.empty()) add(a, Kind::Exit, nm);
    else if (fragment_spot(a)) add(a, Kind::Fragment, strings::kPoiFragment);
    else if (nm.empty()) add(a, Kind::Thing, strings::kPoiThing);
    else if (auto c = chests.find(a); c != chests.end()) add(a, Kind::Thing, chest_name(c->second, nm));
    else add(a, check_kind(nm), nm);
  }
  for (UObject* a : live("FldNpcActor")) {
    if (!world::actor_visible_level(a) || get_bool(a, "bHidden")) continue;
    // A placeholder with nobody spawned in it (no child actor, no class): not a person (the dorm lounge, 2026-10-27).
    auto* kids = field<TArray<UObject*>>(a, "Children");
    if (kids && kids->num == 0 && !get<UObject*>(a, "mNpcBaseClass_") && !get<UObject*>(a, "mCharaBaseClass_")) continue;
    // Nobody standing in it: no character model loaded (mCharaModelList_, a TMap: its element array's count is 0).
    // Three such spots on the dorm's third floor (FldNpcActor_298_*, a base class but no model and no icon, 5/4
    // evening, 2026-11-12) were listed as "Person" 2 m away, and the auto-walk stalled against their collision.
    auto* models = field<TArray<uint8_t>>(a, "mCharaModelList_");
    if (models && models->num == 0) continue;
    add(a, Kind::Person, npc_name(a));
  }
  // Tartarus' breakable Shadow remains (BP_DOBJ_Broken*_C; strike them, Ctrl, for items). A broken one stays visible
  // but its Box stops colliding (NoCollision, 2026-10-27).
  // One of them is a crystal that always gives a Twilight Fragment (BP_DOBJ_BrokenFragment_C,
  // EDungeonObjType::BrokenFragment; its script is BF_BP_DOBJ_BrokenObject's "Hit_BrokenHakumei_Object"). It is built
  // like the others (the same Box), and was left out of the breakables until 2026-11-11.
  // Seen listed on Adamah floor 240 (2026-11-11).
  for (UObject* a : live("FldDungeonObjectActor")) {
    if (!breakable(a)) continue;
    if (name_of(class_of(a)).find("BrokenFragment") != std::string::npos) add(a, Kind::Fragment, strings::kPoiFragmentCrystal);
    else add(a, Kind::Thing, strings::kPoiBreakable);
  }
  // Shadows roaming a Tartarus floor (EnemySymbolBaseCore; parked or defeated ones are hidden).
  const std::vector<shadow::Holder> holders = shadow::enemy_holders();
  for (UObject* a : live("EnemySymbolBaseCore")) {
    if (get_bool(a, "bHidden")) continue;
    UObject* comp = nullptr;
    for (const shadow::Holder& h : holders) if (h.actor == a) { comp = h.comp; break; }
    add(a, Kind::Enemy, shadow_name(a, comp));
  }
  if (out.empty() && !beyond) for (UObject* a : triggers) add(a, Kind::Event, strings::kPoiEvent);
  std::sort(out.begin(), out.end(), [](const Poi& a, const Poi& b) { return a.dist < b.dist; });
  // A door between two rooms is two exits a step apart, one on each side, each named for where it leads (the
  // monorail's cars, 1.4 m apart, 2026-10-27): only the one on the player's side, the nearer, counts.
  std::vector<core::Place> places;
  for (auto& p : out) places.push_back(core::Place{p.kind == Kind::Exit, p.name, world::vec3(p.loc)});
  std::vector<Poi> kept;
  for (size_t i : core::near_sides(places)) kept.push_back(std::move(out[i]));
  return kept;
}

void tick() {
  if (screens::current() || msg::current().blocking) return;   // a menu, notice or message has the floor
  pickup_tick();
  // The field's sounds only while the player walks it: a battle between turns has no screen up, and a story scene
  // no blocking window, yet neither is the place for them.
  if (world::exploring()) shadow::tick();
  prompt_tick();
  arrival_tick();
}

void install() {
  dev::route("/hits", [](const dev::Request& r, int&) {
    // /hits?idx=10002012 every hit actor with that name index (any level), with its level and visibility
    int want = (int)dev::arg_int(r, "idx", -1);
    auto out = std::make_shared<std::string>("timeout\n");
    hooks::run_on_game_thread([out, want] {
      std::string s;
      for (UObject* a : live("FldHitCore")) {
        int idx = get<int32_t>(a, "mNameIndex_", -1);
        if (want >= 0 && idx != want) continue;
        world::FVector v = world::actor_location(a);
        s += std::format("#{} {} idx {} '{}' level {} visible={} at ({:.0f}, {:.0f}, {:.0f})\n", index_of(a), name_of(a), idx, hit_name(a), name_of(outer_of(outer_of(a))),
                         world::actor_visible_level(a), v.x, v.y, v.z);
      }
      *out = s.empty() ? "none\n" : s;
    });
    return *out;
  }, "?idx=N hit actors by name index");
  dev::route("/poi", [](const dev::Request& r, int&) {
    auto out = std::make_shared<std::string>("timeout\n");
    float radius = (float)dev::arg_num(r, "r", 6000);
    hooks::run_on_game_thread([out, radius] {
      std::string s;
      for (auto& p : scan(radius)) s += std::format("#{} [{}] {}\n", p.obj.index, kind_name(p.kind), describe(p));
      *out = s.empty() ? "none\n" : s;
    });
    return *out;
  }, "?r=cm points of interest near the player");
}
}  // namespace p3a::poi

// UGlobalWork +0x1B0: FDatUnitWork[11] (0x2B4 each, by character id): +0x8 ID, +0xC HP, +0x10 SP, +0x1C level,
// +0x20 EXP (p3rpc.nativetypes). Maximum HP and SP are computed, not stored: BtlDataBaseComponent.GetUnitMaxHP/SP's
// natives load the battle unit's FDatUnitWork (+0x328) and tail-call the routines that compute them from a unit
// work; those are found through the UFunctions' Func pointers (UFunction +0xD8), so a patch that moves them keeps
// working. The hero's entered first name is UTF-8 at UGlobalWork +0xC728 (measured 2026-10-28).
#include "party.h"
#include "core/strings.h"
#include "devserver.h"
#include "log.h"
#include "screens/screen.h"
#include "speech.h"
#include "seh.h"
#include "ue/ue.h"
#include "watch.h"
#include "world.h"
#include <windows.h>
#include <format>

namespace p3a::party {
using namespace ue;

using UnitCalc_t = int (*)(const uint8_t* unit);
static UnitCalc_t g_max_hp, g_max_sp;

// From the object watcher: a class's first lookup is filled by a sweep, so an instance made before then is found.
uint8_t* global_work() {
  static const int id = watch::want("GlobalWork");
  return (uint8_t*)watch::get(id);
}

Params gwork(const char* fn) { return call_static("GlobalWorkBpLib", fn); }
Params gwork(const char* fn, int index) {
  return call_static("GlobalWorkBpLib", fn, [index](Params& p) { p.set<int32_t>("Index", index); });
}

uint8_t* unit_work(int id) {
  uint8_t* g = global_work();
  return g && id >= 1 && id <= 10 ? g + 0x1B0 + 0x2B4 * id : nullptr;
}

// A GetUnitMax* exec thunk calls the native `mov rcx, [rdx+0x328]; jmp <routine>`: follow it to the routine.
static UnitCalc_t resolve(const char* fn_name) {
  UClass* c = find_class("BtlDataBaseComponent");
  UFunction* f = c ? find_function(c, fn_name) : nullptr;
  if (!f) return nullptr;
  uint8_t* thunk = *(uint8_t**)((uint8_t*)f + 0xD8);
  UnitCalc_t out = nullptr;
  guarded([&] {
    for (int i = 0; i < 0xA0 && !out; ++i) {
      if (thunk[i] != 0xE8) continue;
      uint8_t* t = thunk + i + 5 + *(int32_t*)(thunk + i + 1);
      if (t[0] == 0x48 && t[1] == 0x8B && t[2] == 0x8A && *(int32_t*)(t + 3) == 0x328 && t[7] == 0xE9)
        out = (UnitCalc_t)(t + 12 + *(int32_t*)(t + 8));
    }
  });
  return out;
}

std::string name(int id) {
  if (id == 1) {
    uint8_t* g = global_work();
    std::string s;
    if (g) guarded([&] { const char* p = (const char*)g + 0xC728; size_t n = strnlen(p, 32); if (n < 32) s.assign(p, n); });
    if (!s.empty()) { s[0] = (char)toupper((unsigned char)s[0]); return s; }
  }
  static const int table_id = watch::want("DatPlayerFirstNameTable");
  UObject* t = watch::get(table_id);
  auto* data = t ? field<TArray<uint8_t>>(t, "Data") : nullptr;
  FField* dp = t ? find_prop(class_of(t), "Data") : nullptr;
  if (!data || !dp) return {};
  UStruct* st = (UStruct*)prop_inner_object(prop_inner_prop(dp));
  int sz = struct_size(st), id_off = offset_of(st, "mPlayerID"), name_off = offset_of(st, "mName");
  for (int i = 0; i < data->num; ++i) {
    uint8_t* r = data->data + i * sz;
    if (*(int32_t*)(r + id_off) == id) return text(*(FText*)(r + name_off));   // FDatPlayerNameOne.mName is an FText
  }
  return {};
}

bool member(int id, Member& m) {
  uint8_t* u = unit_work(id);
  if (!u) return false;
  bool ok = guarded([&] {
    m.id = id;
    m.hp = *(int32_t*)(u + 0xC);
    m.sp = *(int32_t*)(u + 0x10);
    // The hero's level is the unit's own; the others' is their Persona's (what the max HP routine reads too):
    // FDatUnitPersona at +0x48, u16 equipped index, entries of 0x30 from +4 with the level at +4.
    m.level = *(int16_t*)(u + 0x1C);
    if (id != 1) { uint16_t eq = *(uint16_t*)(u + 0x48); m.level = eq < 12 ? *(uint16_t*)(u + 0x48 + 4 + 0x30 * eq + 4) : 0; }
    m.exp = *(int32_t*)(u + 0x20);
    m.max_hp = g_max_hp ? g_max_hp(u) : 0;
    m.max_sp = g_max_sp ? g_max_sp(u) : 0;
  });
  if (!ok || m.level <= 0) return false;
  m.name = name(id);
  return true;
}

std::vector<Member> members() {
  std::vector<Member> out;
  for (int i = 0; i < 4; ++i) {
    Params p = gwork("GWorkGetPartyID", i);
    if (!p.fn) return out;
    int id = p.ret<int32_t>();
    Member m;
    if (id > 0 && member(id, m)) out.push_back(m);
  }
  return out;
}

const uint8_t* persona_entry(int id) {
  uint8_t* u = unit_work(id);
  const uint8_t* e = nullptr;
  if (u) guarded([&] { uint16_t eq = *(uint16_t*)(u + 0x48); if (eq < 12) e = u + 0x48 + 4 + 0x30 * eq; });
  return e;
}

std::string line(const Member& m, bool level) {
  std::string s = level ? std::format("{}, {} {}", m.name, strings::kLevel, m.level) : m.name;
  s += m.max_hp > 0 ? std::format(", {} {} {} {}", strings::kHP, m.hp, strings::kOf, m.max_hp) : std::format(", {} {}", strings::kHP, m.hp);
  s += m.max_sp > 0 ? std::format(", {} {} {} {}", strings::kSP, m.sp, strings::kOf, m.max_sp) : std::format(", {} {}", strings::kSP, m.sp);
  return s;
}

std::string summary() {
  std::string out;
  for (auto& m : members()) out += (out.empty() ? "" : ". ") + line(m);
  return out.empty() ? std::string(strings::kNoParty) : out;
}

void tick() {
  static std::vector<Member> prev;
  // A battle or the camp speaks its own numbers, and a save loading is not news: take the new values silently. So is
  // the story's own healing: after the first battle the SP it used came back as the scene moved to the infirmary a
  // week later, and was read out between its lines ("Test, SP 41 of 41", 2026-10-29). What is news happens with the
  // hero in the field outside a scene: Auto-Recover in Tartarus, the entrance clock, an item.
  static const int battle = watch::want("BtlCoreComponent");
  bool usual = watch::get(battle) || screens::current() || screens::loading();
  bool story = !world::player() || world::scene_playing();
  auto cur = members();
  // A game over is not news either: with the hero down the game stands the fallen allies up at 1 HP for the retry,
  // and the battle is already gone ("Yukari, HP 1 of 94" before the poem, 2026-11-10).
  bool wiped = false;
  for (auto& m : cur) if (m.id == 1 && m.hp == 0) wiped = true;
  for (auto& p : prev) if (p.id == 1 && p.hp == 0) wiped = true;
  bool quiet = usual || story || wiped;
  if (!quiet)
    for (auto& m : cur)
      for (auto& p : prev)
        if (p.id == m.id && (p.hp != m.hp || p.sp != m.sp) && p.level == m.level) {
          std::string s = m.name;
          if (p.hp != m.hp) s += std::format(", {} {} {} {}", strings::kHP, m.hp, strings::kOf, m.max_hp);
          if (p.sp != m.sp) s += std::format(", {} {} {} {}", strings::kSP, m.sp, strings::kOf, m.max_sp);
          speech::speak(s, false);
        }
  if (story && !usual && !prev.empty())   // logged: a change the story made, not a battle's
    for (auto& m : cur)
      for (auto& p : prev)
        if (p.id == m.id && (p.hp != m.hp || p.sp != m.sp)) {
          log::writef("party: {} HP {} -> {}, SP {} -> {} taken quietly (player {}, scene {})", m.name, p.hp, m.hp, p.sp, m.sp,
                      world::player() != nullptr, world::scene_playing());
        }
  prev = cur;
}

void install() {
  // Wanted before the first tick, so that tick's one sweep fills them with every other class's.
  watch::want("GlobalWork");
  watch::want("DatPlayerFirstNameTable");
  g_max_hp = resolve("GetUnitMaxHP");
  g_max_sp = resolve("GetUnitMaxSP");
  log::writef("party: max HP routine {}, max SP routine {}", (void*)g_max_hp, (void*)g_max_sp);
  dev::route("/party", [](const dev::Request&, int&) {
    return dev::on_game([] { return summary() + "\n"; });
  }, "the party: level, HP and SP of each member");
}
bool flag(int id) {
  if (!id) return false;
  Params p = gwork("GWorkFlagChkValue", id);
  uint8_t* v = p.fn ? p.at<uint8_t>("OnOff") : nullptr;
  return v && enum_name(prop_inner_object(find_prop(p.fn, "OnOff")), *v) == "On";
}
}  // namespace p3a::party

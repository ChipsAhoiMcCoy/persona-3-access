// The portrait key (the area key during a conversation): what the character speaking looks like, in the outfit and
// pose the game draws beside the line (src/core/portrait_text holds the words).
// - The game's BustupDraw (a UObject) holds the portrait shown in BustupObject_ (+0x38). Showing one
//   (FUN_1412b3980: character, face, outfit packed into one id) makes a new BustupObject there; hiding it
//   (FUN_1412b37a0 -> FUN_1412bf400) moves it to PrevBustupObject_ (+0x40) to fade out and sets BustupObject_ to
//   null. So BustupObject_ set = a portrait is up now.
// - A BustupObject's base texture BaseTex_ is T_BU_<PC|SC|NC><char>_Pose<x>_C<outfit> (FUN_1412c3c80 builds the name
//   from its character +0xB8 and outfit +0xC0; +0xBC is the face). The texture loads a frame or so after the object
//   is made: until then the two ints are read (this build's layout only).
#include "portrait.h"
#include "core/portrait_text.h"
#include "core/strings.h"
#include "devserver.h"
#include "log.h"
#include "msg.h"
#include "seh.h"
#include "speech.h"
#include "watch.h"
#include <format>

namespace p3a::portrait {
using namespace ue;
namespace cp = core::portrait;

namespace {
constexpr int kChara = 0xB8, kClothes = 0xC0;

struct Found { cp::Shown shown; std::string texture; };

std::vector<Found> shown_now() {
  static const int id = watch::want("BustupDraw", true);
  std::vector<Found> out;
  for (UObject* d : watch::all(id)) {
    UObject* bo = get<UObject*>(d, "BustupObject_");
    if (!bo) continue;
    Found f;
    if (UObject* tex = get<UObject*>(bo, "BaseTex_")) {
      f.texture = name_of(tex);
      cp::parse_texture(f.texture, f.shown);
    }
    if (f.shown.chara < 0 && known_build())
      guarded([&] { f.shown.chara = *(int32_t*)((uint8_t*)bo + kChara); f.shown.clothes = *(int32_t*)((uint8_t*)bo + kClothes); });
    if (f.shown.chara >= 0) out.push_back(std::move(f));
  }
  return out;
}
}  // namespace

bool describe() {
  // Only while someone is talking: a portrait belongs to a message window (or the pause between two of one scene).
  msg::Current c = msg::current();
  if (!c.open && !msg::session_open()) return false;
  std::vector<Found> found = shown_now();
  if (found.empty()) return false;
  std::string said;
  for (auto& f : found) {
    std::string d = cp::describe(f.shown);
    if (d.empty()) {
      log::writef("portrait: {} (character {} outfit {}) has no description", f.texture, f.shown.chara, f.shown.clothes);
      d = std::format("{} {}", strings::kNoPortraitDescription, c.speaker.empty() ? f.texture : c.speaker);
    }
    if (!said.empty()) said += " ";
    said += d;
  }
  speech::speak(said, true);
  return true;
}

void install() {
  dev::route("/portrait", [](const dev::Request& r, int&) {
    int chara = (int)dev::arg_int(r, "chara"), clothes = (int)dev::arg_int(r, "clothes");
    std::string pose = dev::arg(r, "pose");
    if (chara) {   // a description by id, no game needed
      cp::Shown s{chara, clothes, pose.empty() ? 'A' : pose[0]};
      return cp::describe(s) + "\n";
    }
    return dev::on_game([] {
      std::string out;
      for (auto& f : shown_now())
        out += std::format("{} chara {} clothes {} pose {}\n  {}\n", f.texture, f.shown.chara, f.shown.clothes,
                           f.shown.pose ? std::string(1, f.shown.pose) : "?", cp::describe(f.shown));
      msg::Current c = msg::current();
      out += std::format("window open {} session {} speaker '{}'\n", c.open, msg::session_open(), c.speaker);
      return out.empty() ? std::string("no portrait\n") : out;
    });
  }, "the dialogue portraits shown now and their descriptions; ?chara=&clothes=[&pose=A] one by id");
}
}  // namespace p3a::portrait

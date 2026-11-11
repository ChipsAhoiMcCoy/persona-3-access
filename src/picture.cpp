// Full-screen pictures. The flowscript's OPEN_PICTURE(id, mode) (a class question: Event_Extr_330_020/030 with
// SCHE_GET_PICTID_TEACH_QUESTION / _ANSWER; the story's sketch) shows an AUIMiscPictureDraw, which loads
// T_UI_Picture_<id> into m_pPicture (+0x2D0). The question and its answers are messages, but the picture can be the
// question itself (four spirals labelled A to D), so each picture has a description (strings::kScenePictures), read
// when a new one is up; the dialogue line after it waits for it (msg::cutin_said). The actor's +0x294 is 1-3 while a
// picture opens and shows, 4-5 as it closes, and it keeps the texture after (measured 2026-11-03): only states 1-3 are
// up, and the same picture shown again (a question's picture is closed and opened again with the answers) is not
// read twice.
// OPEN_PICTURE's handler (FUN_141537ab0) asks the UI manager (FUN_14122b890) for UI 0x1B, mode 0, with a request
// (FUN_140fe6f30 builds it, 0x80 bytes) holding the two arguments at +0x20 / +0x24; CLOSE_PICTURE (FUN_141535df0)
// sends the same UI mode 1 with an empty request. /picture?id= does the same (dev).
#include "picture.h"
#include "core/strings.h"
#include "devserver.h"
#include "log.h"
#include "msg.h"
#include "seh.h"
#include "speech.h"
#include "world.h"
#include "screens/util.h"
#include "ue/scan.h"
#include <format>
#include <string>

namespace p3a::picture {
using namespace ue;

namespace {
screens::Finder g_draw{"UIMiscPictureDraw"};
constexpr int kPicture = 0x2D0;
std::string g_said;   // the texture last read (forgotten with the actor)

const char* describe(const std::string& texture) {
  for (auto& p : strings::kScenePictures) if (texture == p.texture) return p.text;
  return nullptr;
}

// OPEN_PICTURE / CLOSE_PICTURE as their handlers do it (see the top of the file).
bool request(int mode, int id, int arg) {
  uintptr_t base = scan::exe_base();
  return guarded([&] {
    void* ui = ((void* (*)())(base + 0x122b890))();
    if (!ui) return;
    alignas(16) uint8_t req[0x80] = {};
    ((void (*)(void*))(base + 0xfe6f30))(req);
    if (mode == 0) { *(int32_t*)(req + 0x20) = id; *(int32_t*)(req + 0x24) = arg; }
    ((void (*)(void*, int, int, void*))(base + 0x1352d20))(ui, 0x1b, mode, req);
    for (int off : {0x60, 0x50}) if (void* p = *(void**)(req + off)) ((void (*)(void*))(base + 0x174d4b0))(p);
  });
}
}  // namespace

void tick() {
  UObject* d = g_draw.get();
  std::string texture;
  int state = -1;
  if (d) guarded([&] {
    state = *(int32_t*)((uint8_t*)d + 0x294);
    UObject* t = *(UObject**)((uint8_t*)d + kPicture);
    if (t && is_live(t)) texture = name_of(t);
  });
  if (!d) { g_said.clear(); return; }
  if (state < 1 || state > 3 || texture.empty() || texture == g_said) return;
  g_said = texture;
  const char* text = describe(texture);
  log::writef("picture: {}{}", texture, text ? "" : " (no description)");
  if (!text || world::scene_fast()) return;
  speech::speak(text, false);
  msg::cutin_said();
}

void register_routes() {
  dev::route("/picture", [](const dev::Request& r, int&) {
    bool close = dev::has(r, "close");
    int id = (int)dev::arg_int(r, "id"), arg = dev::has(r, "mode") ? (int)dev::arg_int(r, "mode") : 1;
    return dev::on_game([=] {
      bool ok = request(close ? 1 : 0, id, arg);
      return std::format("{} {}\n", close ? "CLOSE_PICTURE()" : std::format("OPEN_PICTURE({}, {})", id, arg), ok ? "sent" : "faulted");
    });
  }, "?id=N[&mode=1] | ?close=1 show or close a full-screen picture as OPEN_PICTURE / CLOSE_PICTURE do (dev)");
}
}  // namespace p3a::picture

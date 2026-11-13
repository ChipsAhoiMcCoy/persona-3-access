// Flowscript commands. An entry of the game's command table is {fn, argc, name} in 0x40 bytes: the name is a string in
// the exe's data, and the entry is found through the pointer to it. The script context (one per running script):
//   +0x30 program counter, +0x34 argument count; arguments are 1-based stack slots: +0x37+slot a type byte (0/4 int,
//   1 float, 2/3 script variables, 5 a pointer), +0x60+slot*8 the value. A command reads its argument i from slot
//   (count - i) (FUN_141274950 ints, FUN_1412748d0 floats). +0x1e0 a command's result, +0x67 its type (0 int, 1 float,
//   6 none: set before each call). +0x1e8 the script's header (+0x18 bit 0: big-endian), +0x208 its code, +0x224 a
//   message window id (-1 none), +0x228 / +0x22c how many frames the current instruction has yielded.
// The command-call opcode (FUN_141260510): a command returning 0 has not finished; the VM keeps the program counter and
// calls it again next frame with +0x22c counted up (FUN_141274cb0). Non-zero: done, its arguments are popped.
// Dev: `/flow/run` calls any command as a script would, from a context of its own, until it finishes.
#include "flow.h"
#include "area.h"
#include "devserver.h"
#include "log.h"
#include "seh.h"
#include "ue/scan.h"
#include "ue/ue.h"
#include <format>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace p3a::flow {
namespace {
struct Entry { void* fn = nullptr; int argc = -1; };

// The command's table entry by name; cached (a data scan is tens of milliseconds).
Entry entry(const std::string& name) {
  static std::map<std::string, Entry> cache;
  if (auto it = cache.find(name); it != cache.end()) return it->second;
  Entry e;
  if (uintptr_t str = scan::find_string(name.c_str()))
    // The name is the entry's third field: its function is 16 bytes before the pointer to the name, its count 8.
    if (uintptr_t slot = scan::find_pointer(str, 16)) {
      e.fn = *(void**)(slot - 16);
      e.argc = *(int32_t*)(slot - 8);
      if (e.argc < 0 || e.argc > 0x30) e = {};
    }
  cache[name] = e;
  return e;
}

constexpr size_t kContextSize = 0x300;
struct Job {
  std::string text;                    // NAME(args), for the status
  Command fn = nullptr;
  std::unique_ptr<uint8_t[]> ctx;      // the fake script context
  std::unique_ptr<uint8_t[]> blank;    // a zeroed header / code block the context points at
  uint64_t calls = 0;
  bool done = false, faulted = false;
  std::string result;
};
std::vector<std::unique_ptr<Job>>& jobs() { static std::vector<std::unique_ptr<Job>> j; return j; }
std::vector<std::string>& finished() { static std::vector<std::string> f; return f; }

// One call of the command, as the command-call opcode makes it.
void step(Job& j) {
  uint8_t* c = j.ctx.get();
  c[0x67] = 6;
  int64_t r = 0;
  if (!guarded([&] { r = j.fn(c); })) { j.faulted = j.done = true; j.result = "faulted"; return; }
  ++j.calls;
  if (r == 0) {   // not finished: called again next frame
    ++*(int32_t*)(c + 0x22c);
    ++*(int32_t*)(c + 0x228);
    return;
  }
  j.done = true;
  uint8_t t = c[0x67];
  if (t == 0) j.result = std::format("result {}", *(int32_t*)(c + 0x1e0));
  else if (t == 1) j.result = std::format("result {}", *(float*)(c + 0x1e0));
  else j.result = "no result";
}

std::string line(const Job& j) {
  return std::format("{} {} after {} call{}", j.text, j.done ? j.result : std::string("running"), j.calls, j.calls == 1 ? "" : "s");
}

void retire() {
  auto& js = jobs();
  for (auto it = js.begin(); it != js.end();) {
    if ((*it)->done) {
      log::writef("flow: {}", line(**it));
      finished().push_back(line(**it));
      if (finished().size() > 20) finished().erase(finished().begin());
      it = js.erase(it);
    } else ++it;
  }
}

}  // namespace

std::string run(const std::string& name, const std::string& args) {
  Entry e = entry(name);
  if (!e.fn) return std::format("no command {}\n", name);
  std::vector<std::string> vals;
  for (size_t i = 0; i < args.size();) {
    size_t k = args.find(',', i);
    if (k == std::string::npos) k = args.size();
    if (k > i) vals.push_back(args.substr(i, k - i));
    i = k + 1;
  }
  if ((int)vals.size() != e.argc) return std::format("{} takes {} argument{}, got {}\n", name, e.argc, e.argc == 1 ? "" : "s", vals.size());
  // A field change while the last one is still settling can crash the game in its follower placement (area.cpp,
  // field_busy). A script never does that: its next command waits for the field. Refused, so the caller waits for
  // /area/ready.
  if (name == "CALL_FIELD") {
    for (auto& j : jobs())
      if (j->text.starts_with("CALL_FIELD(") && !j->done) return "refused: a CALL_FIELD is still running\n";
    if (std::string why = area::field_busy(); !why.empty()) return "refused: " + why + "\n";
  }
  auto j = std::make_unique<Job>();
  j->fn = (Command)e.fn;
  j->ctx.reset(new uint8_t[kContextSize]());
  j->blank.reset(new uint8_t[0x400]());
  uint8_t* c = j->ctx.get();
  *(void**)(c + 0x1e8) = j->blank.get();
  *(void**)(c + 0x208) = j->blank.get();
  *(int32_t*)(c + 0x224) = -1;
  int n = (int)vals.size();
  *(int32_t*)(c + 0x34) = n;
  j->text = name + "(";
  for (int i = 0; i < n; ++i) {
    int slot = n - i;   // argument i, as the script pushed them (last argument first)
    const std::string& v = vals[i];
    try {
      if (v.find('.') != std::string::npos) {
        c[0x37 + slot] = 1;
        *(float*)(c + 0x60 + slot * 8) = std::stof(v);
      } else {
        c[0x37 + slot] = 0;
        *(int64_t*)(c + 0x60 + slot * 8) = (int64_t)std::stoll(v, nullptr, 0);
      }
    } catch (...) { return std::format("{}: not a number: {}\n", name, v); }
    j->text += (i ? ", " : "") + v;
  }
  j->text += ")";
  step(*j);
  std::string out = line(*j) + "\n";
  jobs().push_back(std::move(j));
  retire();
  return out;
}

void* command(const char* name) { return entry(name).fn; }

int int_arg(uint8_t* ctx) {
  int v = 0;
  guarded([&] {
    int i = *(int32_t*)(ctx + 0x34);   // the count: the first argument's type and value are in slot [count]
    if (i < 1 || i > 0x30) return;
    uint8_t type = ctx[0x37 + i];
    if (type == 0 || type == 4) v = *(int32_t*)(ctx + 0x60 + i * 8);
    else if (type == 1) v = (int)*(float*)(ctx + 0x60 + i * 8);
  });
  return v;
}

void tick() {
  if (jobs().empty()) return;
  for (auto& j : jobs())
    if (!j->done) step(*j);
  retire();
}

void install() {
  dev::route("/flow/run", [](const dev::Request& r, int&) {
    std::string name = dev::arg(r, "cmd"), args = dev::arg(r, "args");
    bool cancel = dev::has(r, "cancel");
    return dev::on_game([=] {
      // The context layout and the command table are this build's (measured on it).
      if (!ue::known_build()) return std::string("another build of the game\n");
      if (cancel) {
        size_t n = jobs().size();
        jobs().clear();
        return std::format("cancelled {}\n", n);
      }
      if (!name.empty()) return run(name, args);
      std::string out;
      for (auto& j : jobs()) out += line(*j) + "\n";
      for (auto& f : finished()) out += "finished: " + f + "\n";
      return out.empty() ? std::string("nothing run\n") : out;
    });
  }, "?cmd=NAME&args=1,2,3 run a flowscript command as a script does, called each frame until it finishes; no cmd: the "
     "running and last finished ones; ?cancel=1 drops the running ones (dev)");
}
}  // namespace p3a::flow

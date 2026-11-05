// The engine's object-creation listeners (FUObjectArray::UObjectCreateListeners, GUObjectArray +0x68; the delete
// listeners follow at +0x78, their critical section at +0x88). AllocateUObjectIndex calls
// listener->vtable[1](listener, object, index) for every entry after it leaves ObjObjectsCritical, so on whichever
// thread makes the object (the async loader too). Read from the game's own code 2026-10-28: exe+0x1a32967 (the
// call loop), exe+0xe5889d0 (shutdown: vtable[2] = OnUObjectArrayShutdown), exe+0x3fe9050 (the array's destructor
// frees the list's buffer with FMemory::Free, so the buffer must come from GMalloc). The list was empty.
#include "watch.h"
#include "devserver.h"
#include "hooks.h"
#include "log.h"
#include "msg.h"
#include "seh.h"
#include <windows.h>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <format>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace p3a::watch {
using namespace ue;

namespace {
constexpr int kCreateListeners = 0x68, kDeleteListeners = 0x78;

// ---- the listener ----
// The engine calls whatever sits in its list, and a hot reload unloads this DLL: the listener object, its vtable and
// the code they reach live in a page of their own that is never freed. Each thunk counts itself in and out and calls
// its function only while it is set; remove() clears them, then waits for the count to drain.
struct Stub {
  void* vtable;                    // +0x00 -> slots
  void* pad;
  void* slots[3];                  // +0x10 destructor, NotifyUObjectCreated, OnUObjectArrayShutdown
  void* volatile shutdown_fn;      // +0x28 void(void* self)
  void* volatile fn;               // +0x30 void(void* self, UObject* object, int32_t index)
  volatile LONG64 inflight;        // +0x38
  uint8_t ret[16];                 // +0x40
  uint8_t notify[64];              // +0x50
  uint8_t on_shutdown[64];         // +0x90
};
static_assert(offsetof(Stub, fn) == 0x30 && offsetof(Stub, inflight) == 0x38 && offsetof(Stub, notify) == 0x50);
Stub* g_stub;
std::atomic<bool> g_active{false};

void put_rel32(uint8_t* at, const uint8_t* next, const volatile void* target) {
  int32_t r = (int32_t)((const uint8_t*)target - next);
  memcpy(at, &r, 4);
}
// A thunk at `c` that calls through `*target` (arguments untouched) while it is set, counted in s->inflight.
void emit_thunk(Stub* s, uint8_t* c, void* volatile* target) {
  static const uint8_t code[] = {
      0xF0, 0x48, 0xFF, 0x05, 0, 0, 0, 0,   // +0  lock inc qword [inflight]
      0x48, 0x8B, 0x05, 0, 0, 0, 0,         // +8  mov rax, [target]
      0x48, 0x85, 0xC0,                     // +15 test rax, rax
      0x74, 0x0A,                           // +18 jz +30
      0x48, 0x83, 0xEC, 0x28,               // +20 sub rsp, 0x28 (shadow space; rsp 16-aligned at the call)
      0xFF, 0xD0,                           // +24 call rax (rcx, rdx, r8 untouched)
      0x48, 0x83, 0xC4, 0x28,               // +26 add rsp, 0x28
      0xF0, 0x48, 0xFF, 0x0D, 0, 0, 0, 0,   // +30 lock dec qword [inflight]
      0xC3};                                // +38 ret
  memcpy(c, code, sizeof code);
  put_rel32(c + 4, c + 8, &s->inflight);
  put_rel32(c + 11, c + 15, target);
  put_rel32(c + 34, c + 38, &s->inflight);
}
Stub* make_stub(void (*fn)(void*, UObject*, int32_t), void (*shutdown_fn)(void*)) {
  auto* s = (Stub*)VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (!s) return nullptr;
  s->vtable = s->slots;
  s->ret[0] = 0xC3;   // ret
  emit_thunk(s, s->notify, &s->fn);
  emit_thunk(s, s->on_shutdown, &s->shutdown_fn);
  s->slots[0] = s->ret;
  s->slots[1] = s->notify;
  s->slots[2] = s->on_shutdown;
  s->fn = (void*)fn;
  s->shutdown_fn = (void*)shutdown_fn;
  FlushInstructionCache(GetCurrentProcess(), s, sizeof(Stub));
  return s;
}

// Takes the stub out of the engine's list (it stays callable: a thread may have read the list just before).
void leave_list() {
  guarded([&] {
    auto* create = (TArray<void*>*)(object_array() + kCreateListeners);
    int n = create->num;
    for (int i = 0; i < n; ++i) {
      if (create->data[i] != g_stub) continue;
      for (int j = i; j + 1 < n; ++j) create->data[j] = create->data[j + 1];
      InterlockedExchange((volatile LONG*)&create->num, n - 1);
      break;
    }
  });
}
// The game quitting: ShutdownUObjectArray asks each create listener to unregister and then checks the list is empty,
// a fatal error otherwise ("All UObject delete listeners should be unregistered", line 436: the crash on quitting,
// 2026-10-28).
void on_shutdown(void*) {
  g_active = false;
  leave_list();
}

// ---- what the listener hears (any thread) ----
// Indices of new objects, sorted on the game thread next tick: the listener only appends (objects can be half-built
// on the loader thread). A full ring sets `overflow` and the next tick sweeps every object instead.
constexpr uint32_t kRing = 1 << 17;
SRWLOCK g_ring_lock = SRWLOCK_INIT;
int32_t g_ring[kRing];
uint32_t g_ring_n = 0;
bool g_overflow = false;
std::atomic<uint64_t> g_heard{0};

void on_created(void*, UObject*, int32_t index) {
  AcquireSRWLockExclusive(&g_ring_lock);
  if (g_ring_n < kRing) g_ring[g_ring_n++] = index;
  else g_overflow = true;
  ReleaseSRWLockExclusive(&g_ring_lock);
  ++g_heard;
}

// ---- the registry (game thread, but want() may come from the init thread) ----
struct Inst { int32_t index; UObject* obj; UClass* cls; };   // a reused address or slot fails the class check
struct Want { std::string name; bool all = false; std::vector<Inst> live; ULONGLONG last_scan = 0; };
struct Registry {
  std::mutex mu;
  std::vector<Want> wants;
  std::unordered_map<std::string, int> by_name;
  // Per class: the wants it is (itself or through its supers); per class or struct: the want its own name is (-1
  // none). Both keyed by address and checked against the object's identity, as classes come and go with packages.
  std::unordered_map<UObject*, std::pair<WeakObj, std::vector<int>>> class_ids;
  std::unordered_map<UObject*, std::pair<WeakObj, int>> own_id;
};
// Built on first use: a Finder at namespace scope (title.cpp) registers during the DLL's static initialization,
// before this file's globals would exist (a global registry crashed the game at startup, 2026-10-28).
Registry& reg() { static Registry r; return r; }
bool g_need_sweep = true;
uint64_t g_sweeps = 0, g_last_sweep_us = 0, g_sorted = 0;

bool alive(const Inst& x) { return object_at(x.index) == x.obj && is_live(x.obj) && class_of(x.obj) == x.cls; }

int own_id(UStruct* s) {
  WeakObj w = WeakObj::of(s);
  auto it = reg().own_id.find(s);
  if (it != reg().own_id.end() && it->second.first == w) return it->second.second;
  auto n = reg().by_name.find(name_of(s));
  int id = n == reg().by_name.end() ? -1 : n->second;
  reg().own_id[s] = {w, id};
  return id;
}
const std::vector<int>& ids_for(UClass* k) {
  WeakObj w = WeakObj::of(k);
  auto it = reg().class_ids.find(k);
  if (it != reg().class_ids.end() && it->second.first == w) return it->second.second;
  std::vector<int> ids;
  for (UStruct* s = k; s; s = super_of(s))
    if (int id = own_id(s); id >= 0) ids.push_back(id);
  auto& e = reg().class_ids[k];
  e = {w, std::move(ids)};
  return e.second;
}
void add(Want& w, UObject* o, UClass* k) {
  Inst x{index_of(o), o, k};
  // A keep-all list is pruned when read (all()) and whenever it reaches another 1024 (one nobody reads for a while);
  // the others stay short here.
  if (!w.all || (w.live.size() + 1) % 1024 == 0) std::erase_if(w.live, [&](const Inst& y) { return !alive(y); });
  for (auto& y : w.live) if (y.obj == o && y.index == x.index) return;
  w.live.push_back(x);
  if (!w.all && w.live.size() > 16) w.live.erase(w.live.begin());
}
void consider(UObject* o) {
  guarded([&] {
    if (!o || is_cdo(o)) return;   // CDOs and templates are born with their flags
    UClass* k = class_of(o);
    if (!k) return;
    for (int id : ids_for(k)) add(reg().wants[id], o, k);
  });
  ++g_sorted;
}
void sweep() {
  LARGE_INTEGER a, z, f;
  QueryPerformanceCounter(&a);
  int32_t n = num_objects();
  for (int32_t i = 0; i < n; ++i) consider(object_at(i));
  QueryPerformanceCounter(&z);
  QueryPerformanceFrequency(&f);
  g_last_sweep_us = (uint64_t)((z.QuadPart - a.QuadPart) * 1000000 / f.QuadPart);
  ++g_sweeps;
  g_need_sweep = false;
}
}  // namespace

int want(std::string_view class_name, bool all) {
  std::lock_guard lk(reg().mu);
  std::string name(class_name);
  if (auto it = reg().by_name.find(name); it != reg().by_name.end()) {
    Want& w = reg().wants[it->second];
    if (all && !w.all) { w.all = true; g_need_sweep = true; }   // the newest few were kept so far: fill it in
    return it->second;
  }
  int id = (int)reg().wants.size();
  reg().wants.push_back({name, all});
  reg().by_name[name] = id;
  reg().class_ids.clear();
  reg().own_id.clear();
  g_need_sweep = true;   // a class wanted late: its existing instances come from one sweep
  return id;
}

UObject* get(int id) {
  std::lock_guard lk(reg().mu);
  if (id < 0 || id >= (int)reg().wants.size()) return nullptr;
  Want& w = reg().wants[id];
  UObject* found = nullptr;
  if (g_active) {
    if (g_need_sweep) sweep();   // wanted this frame: fill it now, not next tick (see all())
    guarded([&] {
      for (size_t i = w.live.size(); i-- > 0;) {
        if (!alive(w.live[i])) { w.live.erase(w.live.begin() + i); continue; }
        if (!is_cdo(w.live[i].obj)) { found = w.live[i].obj; return; }
      }
    });
    return found;
  }
  // No listener: the old way, one scan per class at most twice a second while it is missing.
  guarded([&] { if (!w.live.empty() && alive(w.live.back())) found = w.live.back().obj; });
  if (found) return found;
  ULONGLONG now = GetTickCount64();
  if (now - w.last_scan < 500) return nullptr;
  w.last_scan = now;
  w.live.clear();
  UObject* o = find_first(w.name);
  if (o) w.live.push_back({index_of(o), o, class_of(o)});
  return o;
}

std::vector<UObject*> all(int id) {
  std::vector<UObject*> out;
  std::lock_guard lk(reg().mu);
  if (id < 0 || id >= (int)reg().wants.size()) return out;
  Want& w = reg().wants[id];
  if (!g_active) {   // no listener: the old full scan
    for (UObject* o : find_all(w.name)) out.push_back(o);
    return out;
  }
  // A class wanted for the first time is filled by a sweep; waiting for the next tick's made the session's first
  // scan of points of interest empty ("Nothing nearby", then the list on the next press; 2026-10-29).
  if (g_need_sweep) sweep();
  guarded([&] {
    std::erase_if(w.live, [&](const Inst& y) { return !alive(y); });
    for (auto& y : w.live) if (!is_cdo(y.obj)) out.push_back(y.obj);
  });
  return out;
}

// Dev: /watch/born logs every object made while on (class and name), to find what a scene spawns.
bool g_born_on = false;
std::vector<std::string> g_born;

void tick() {
  if (!g_active) return;
  static std::vector<int32_t> batch;
  bool overflow;
  AcquireSRWLockExclusive(&g_ring_lock);
  batch.assign(g_ring, g_ring + g_ring_n);
  g_ring_n = 0;
  overflow = g_overflow;
  g_overflow = false;
  ReleaseSRWLockExclusive(&g_ring_lock);
  std::lock_guard lk(reg().mu);
  if (overflow || g_need_sweep) {
    sweep();
    if (g_sweeps == 1 || overflow) log::writef("watch: swept {} objects in {} us{}", num_objects(), g_last_sweep_us, overflow ? " (the ring overflowed)" : "");
    return;
  }
  for (int32_t i : batch) consider(object_at(i));
  if (g_born_on)
    for (int32_t i : batch)
      guarded([&] {
        UObject* o = object_at(i);
        if (!o || g_born.size() >= 20000) return;
        g_born.push_back(std::format("{} {} {} : {}", GetTickCount64(), i, name_of(o), name_of(class_of(o))));
      });
}

void install() {
  dev::route("/watch", [](const dev::Request& r, int&) {
    // /watch?class=BmdAsset what the list holds, and the live instances a scan finds that it lacks
    std::string cls = dev::arg(r, "class");
    auto out = std::make_shared<std::string>("timeout\n");
    hooks::run_on_game_thread([out, cls] {
      std::vector<UObject*> have;
      {
        std::lock_guard lk(reg().mu);
        auto it = reg().by_name.find(cls);
        if (it == reg().by_name.end()) { *out = "not wanted\n"; return; }
        Want& w = reg().wants[it->second];
        *out = std::format("{} entries (all={})\n", w.live.size(), w.all);
        for (auto& x : w.live) {
          have.push_back(x.obj);
          *out += std::format("  #{} {}{}\n", x.index, alive(x) ? name_of(x.obj) : "", alive(x) ? "" : "(dead)");
        }
      }
      for (UObject* o : find_all(cls))
        if (std::find(have.begin(), have.end(), o) == have.end()) *out += std::format("missing #{} {}\n", index_of(o), name_of(o));
    });
    return *out;
  }, "?class=X the watch list for a class, and live instances it lacks");
  dev::route("/watch/born", [](const dev::Request& r, int&) {
    std::string on = dev::arg(r, "on"), match = dev::arg(r, "match");
    auto out = std::make_shared<std::string>("timeout\n");
    hooks::run_on_game_thread([out, on, match] {
      if (!on.empty()) { g_born_on = on == "1"; g_born.clear(); }
      *out = std::format("on={} {} objects\n", g_born_on, g_born.size());
      if (on.empty())
        for (auto& l : g_born) if (match.empty() || dev::contains_ci(l, match)) *out += l + "\n";
    });
    return *out;
  }, "?on=1|0 start/stop logging every object made; no args: the log (?match=) (dev)");
  uint8_t* base = object_array();
  if (!base) { log::write("watch: no object array; scanning instead"); return; }
  // The layout check: both lists hold sane counts, and every delete listener is an object whose vtable lies in the
  // game's image (the two lists are neighbours, so the create list is where it should be too).
  bool sane = false;
  guarded([&] {
    auto* create = (TArray<void*>*)(base + kCreateListeners);
    auto* del = (TArray<void*>*)(base + kDeleteListeners);
    auto ok = [](TArray<void*>* a) { return a->num >= 0 && a->num <= 64 && a->max >= a->num && a->max <= 4096 && (a->num == 0 || a->data); };
    if (!ok(create) || !ok(del) || del->num < 1) return;
    auto* dos = (IMAGE_DOS_HEADER*)GetModuleHandleW(nullptr);
    auto* nt = (IMAGE_NT_HEADERS*)((uint8_t*)dos + dos->e_lfanew);
    uintptr_t lo = (uintptr_t)dos, hi = lo + nt->OptionalHeader.SizeOfImage;
    for (int i = 0; i < del->num; ++i) {
      uintptr_t vt = *(uintptr_t*)del->data[i];
      if (vt < lo || vt >= hi) return;
    }
    sane = true;
  });
  if (!sane) { log::write("watch: the listener lists do not look as expected; scanning instead"); return; }
  g_stub = make_stub(&on_created, &on_shutdown);
  if (!g_stub) { log::write("watch: no stub page; scanning instead"); return; }
  bool added = false;
  guarded([&] {
    auto* create = (TArray<void*>*)(base + kCreateListeners);
    int n = create->num;
    if (n < create->max) {
      create->data[n] = g_stub;
    } else {
      // Grown the engine's way (GMalloc), so its own Add, Remove and destructor can keep managing the buffer. The old
      // buffer (none so far) is left alone: another thread may be walking it.
      int cap = n + 4;
      auto* buf = (void**)msg::game_malloc(sizeof(void*) * cap);
      if (!buf) return;
      if (n) memcpy(buf, create->data, sizeof(void*) * n);
      buf[n] = g_stub;
      InterlockedExchangePointer((void* volatile*)&create->data, buf);
      create->max = cap;
    }
    InterlockedExchange((volatile LONG*)&create->num, n + 1);   // publishes the entry written above
    added = true;
  });
  if (!added) { log::write("watch: could not join the listener list; scanning instead"); return; }
  g_active = true;
  log::writef("watch: listening for new objects (stub {})", (void*)g_stub);
}

void remove() {
  if (!g_active.exchange(false)) return;
  leave_list();
  // A thread that read the list just before can still enter the stub: it finds the pointers cleared, or is counted.
  InterlockedExchangePointer(&g_stub->fn, nullptr);
  InterlockedExchangePointer(&g_stub->shutdown_fn, nullptr);
  for (int i = 0; i < 2000 && g_stub->inflight; ++i) Sleep(1);
  if (g_stub->inflight) log::write("watch: a listener call was still running at unload");
}

std::string status() {
  std::lock_guard lk(reg().mu);
  size_t live = 0;
  for (auto& w : reg().wants) live += w.live.size();
  return std::format("watch: listening={} wants={} instances={} heard={} sorted={} sweeps={} last_sweep_us={}", g_active.load(), reg().wants.size(), live,
                     g_heard.load(), g_sorted, g_sweeps, g_last_sweep_us);
}
}  // namespace p3a::watch

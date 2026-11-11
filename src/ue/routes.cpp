// Dev routes over the reflection layer: the live object browser used in development.
// Objects are named by their GUObjectArray index ("obj=1234"), a class (first live instance) or a path.
#include "ue/routes.h"
#include "ue/ue.h"
#include "ue/scan.h"
#include "devserver.h"
#include "hooks.h"
#include "seh.h"
#include "input.h"
#include "log.h"
#include <windows.h>
#include <vector>
#include <deque>
#include <map>
#include <cstring>
#include <algorithm>
#include <format>

namespace p3a::ue {
static UObject* pick_base(const dev::Request& r);
// &prop=Name follows an object property of the picked object (repeatable as prop=A.B)
static UObject* pick(const dev::Request& r) {
  UObject* o = pick_base(r);
  std::string chain = dev::arg(r, "prop");
  for (size_t s = 0; o && s < chain.size();) {
    size_t e = chain.find('.', s); if (e == std::string::npos) e = chain.size();
    o = get<UObject*>(o, chain.substr(s, e - s));
    s = e + 1;
  }
  return o;
}
static UObject* pick_base(const dev::Request& r) {
  if (dev::has(r, "obj")) return object_at((int32_t)dev::arg_int(r, "obj"));
  if (dev::has(r, "path")) return find_object(dev::arg(r, "path"));
  if (dev::has(r, "class")) {
    auto all = find_all(dev::arg(r, "class"));
    size_t i = (size_t)dev::arg_int(r, "i", 0);
    return i < all.size() ? all[i] : nullptr;
  }
  if (dev::has(r, "cdo")) return cdo_of(find_class(dev::arg(r, "cdo")));
  return nullptr;
}
using dev::on_game;
using dev::contains_ci;

// Key probe (dev): for finding native state a key toggles while the player plays. Registered regions are copied every
// tick; on a press of Shift, Tab or the pad's X/Y/Menu the copy from just before is compared with one 700 ms later, and
// dwords that were steady for the half second before the press and changed by then go to the log.
struct ProbeRegion { WeakObj obj; int len; std::vector<uint8_t> prev, old, before; uint64_t old_ms = 0; };
static std::vector<ProbeRegion> g_probe;
static uint64_t g_probe_due = 0;
static std::string g_probe_key;
static bool copy_region(ProbeRegion& p, std::vector<uint8_t>& out) {
  UObject* o = p.obj.resolve();
  if (!o) return false;
  out.resize(p.len);
  return guarded([&] { memcpy(out.data(), o, p.len); });
}
// ?all=1: after each press, every live instance (its class's size) is copied; ?cmp=key compares the last three copies
// taken after that key (pressed on, off, on): dwords equal in the first and third and different in the second.
struct AllSnap { std::string key; std::map<int, std::pair<UObject*, std::vector<uint8_t>>> objs; };
static std::deque<AllSnap> g_snaps;
static bool g_probe_all = false;
static void snap_all(const std::string& key) {
  AllSnap s{key, {}};
  size_t bytes = 0;
  int n = num_objects();
  for (int i = 0; i < n; ++i) {
    UObject* o = object_at(i);
    if (!o || is_cdo(o)) continue;
    UClass* c = class_of(o);
    std::string cn = name_of(c);
    if (cn == "Function" || cn == "Package" || cn == "ScriptStruct" || cn == "Enum" || cn == "DelegateFunction" || cn.ends_with("Class") ||
        cn == "Texture2D" || cn == "StaticMesh" || cn == "SkeletalMesh" || cn == "AnimSequence" || cn == "DataTable") continue;
    int len = (std::min)(struct_size((UStruct*)c), 0x30000);
    if (len <= 0x28) continue;
    std::vector<uint8_t> b(len);
    if (!guarded([&] { memcpy(b.data(), o, len); })) continue;
    bytes += len;
    s.objs.emplace(i, std::make_pair(o, std::move(b)));
  }
  log::writef("probe: copied {} objects, {} KB after {}", s.objs.size(), bytes / 1024, key);
  g_snaps.push_back(std::move(s));
  // keep the last four copies of each key (a Tab or Shift between presses must not push out the toggles)
  int same = 0;
  for (auto it = g_snaps.rbegin(); it != g_snaps.rend(); ++it) if (it->key == key && ++same > 4) { g_snaps.erase(std::next(it).base()); break; }
}
static std::string cmp_all(const std::string& key) {
  std::vector<AllSnap*> k;
  for (auto& s : g_snaps) if (s.key == key) k.push_back(&s);
  if (k.size() < 3) return std::format("{} copies after {}, need 3\n", k.size(), key);
  AllSnap &a = *k[k.size() - 3], &b = *k[k.size() - 2], &c = *k[k.size() - 1];
  std::string out;
  int shown = 0;
  for (auto& [i, ea] : a.objs) {
    auto ib = b.objs.find(i), ic = c.objs.find(i);
    if (ib == b.objs.end() || ic == c.objs.end() || ib->second.first != ea.first || ic->second.first != ea.first) continue;
    auto &va = ea.second, &vb = ib->second.second, &vc = ic->second.second;
    if (va.size() != vb.size() || va.size() != vc.size()) continue;
    for (size_t off = 0; off + 4 <= va.size() && shown < 300; off += 4) {
      uint32_t x = *(uint32_t*)(va.data() + off), y = *(uint32_t*)(vb.data() + off), z = *(uint32_t*)(vc.data() + off);
      if (x == z && x != y) { out += std::format("#{} {} +{:#x}: {:#x} / {:#x} / {:#x}\n", i, name_of(class_of(ea.first)), off, x, y, z); ++shown; }
    }
  }
  return out.empty() ? std::string("nothing\n") : out;
}

void probe_tick() {
  if (g_probe.empty() && !g_probe_all) return;
  uint64_t now = GetTickCount64();
  static bool was_shift = false, was_tab = false;
  static uint16_t was_pad = 0;
  bool shift = GetAsyncKeyState(VK_SHIFT) & 0x8000, tab = GetAsyncKeyState(VK_TAB) & 0x8000;
  uint16_t pad = input::real_buttons() & 0x4000 | input::real_buttons() & 0x8000 | input::real_buttons() & 0x0010;
  std::string key = shift && !was_shift ? "Shift" : tab && !was_tab ? "Tab" : (pad & ~was_pad) & 0x4000 ? "pad X" : (pad & ~was_pad) & 0x8000 ? "pad Y" : (pad & ~was_pad) & 0x0010 ? "pad Menu" : "";
  was_shift = shift; was_tab = tab; was_pad = pad;
  if (!key.empty() && !g_probe_due) {
    for (auto& p : g_probe) p.before = p.prev;
    g_probe_due = now + 700; g_probe_key = key;
  }
  if (g_probe_due && now >= g_probe_due) {
    g_probe_due = 0;
    if (g_probe_all) snap_all(g_probe_key);
    std::string out = std::format("probe: {} pressed", g_probe_key);
    for (auto& p : g_probe) {
      std::vector<uint8_t> after;
      if (!copy_region(p, after) || p.before.size() != after.size() || p.old.size() != after.size()) continue;
      int shown = 0;
      for (int i = 0; i + 4 <= p.len && shown < 40; i += 4) {
        uint32_t a = *(uint32_t*)(p.old.data() + i), b = *(uint32_t*)(p.before.data() + i), c = *(uint32_t*)(after.data() + i);
        if (a == b && b != c) { out += std::format("\n  #{} +{:#x}: {:#x} -> {:#x}", p.obj.index, i, b, c); ++shown; }
      }
    }
    log::write(out);
  }
  for (auto& p : g_probe) {
    std::vector<uint8_t> cur;
    if (!copy_region(p, cur)) continue;
    if (now - p.old_ms >= 500) { p.old = p.prev.empty() ? cur : p.prev; p.old_ms = now; }
    p.prev.swap(cur);
  }
}

void register_routes() {
  dev::route("/ue/probe", [](const dev::Request& r, int&) {
    // /ue/probe?add=N&len=0x.. watch an object | ?clear=1
    bool clear = dev::has(r, "clear");
    int idx = (int)dev::arg_int(r, "add", -1), len = (int)dev::arg_int(r, "len", 0x200), all = (int)dev::arg_int(r, "all", -1);
    std::string cmp = dev::arg(r, "cmp"), snap = dev::arg(r, "snap");
    return on_game([clear, idx, len, all, cmp, snap] {
      if (!cmp.empty()) return cmp_all(cmp);
      // ?snap=name: a copy now, for keys sent from outside (posted keys never reach GetAsyncKeyState)
      if (!snap.empty()) { snap_all(snap); return std::format("copied after {}\n", snap); }
      if (all >= 0) { g_probe_all = all != 0; if (!g_probe_all) g_snaps.clear(); }
      if (clear) g_probe.clear();
      if (idx >= 0) if (UObject* o = object_at(idx)) g_probe.push_back({WeakObj::of(o), len});
      std::string out = std::format("{} regions\n", g_probe.size());
      for (auto& p : g_probe) out += std::format("#{} len {:#x}\n", p.obj.index, p.len);
      return out;
    });
  }, "?add=N&len= | ?clear=1 | ?all=1 | ?snap=name | ?cmp=key log what a key press changes (dev)");
  dev::route("/ue/status", [](const dev::Request&, int&) { return on_game([] { return status() + "\n"; }); }, "reflection layer status");
  dev::route("/ue/classes", [](const dev::Request& r, int&) {
    std::string m = dev::arg(r, "match");
    return on_game([m] {
      std::string out;
      int n = num_objects();
      for (int i = 0; i < n; ++i) {
        UObject* o = object_at(i);
        if (!o) continue;
        std::string cn = name_of(class_of(o));
        if (cn != "Class" && cn != "BlueprintGeneratedClass" && cn != "WidgetBlueprintGeneratedClass" && cn != "AnimBlueprintGeneratedClass") continue;
        std::string nm = name_of(o);
        if (!m.empty() && !contains_ci(nm, m)) continue;
        out += std::format("{} {} : {}\n", i, nm, name_of(super_of(o)));
      }
      return out;
    });
  }, "?match= loaded classes");
  dev::route("/ue/find", [](const dev::Request& r, int&) {
    std::string cn = dev::arg(r, "class"), m = dev::arg(r, "match");
    return on_game([cn, m] {
      std::string out;
      if (!cn.empty()) {
        for (UObject* o : find_all(cn)) out += std::format("{} {}\n", index_of(o), full_name(o));
        return out.empty() ? std::string("none (class ") + (find_class(cn) ? "loaded" : "not loaded") + ")\n" : out;
      }
      int n = num_objects(), shown = 0;
      for (int i = 0; i < n && shown < 400; ++i) {
        UObject* o = object_at(i);
        if (!o || is_cdo(o)) continue;
        std::string nm = name_of(o);
        if (!contains_ci(nm, m)) continue;
        out += std::format("{} {}\n", i, full_name(o));
        ++shown;
      }
      return out;
    });
  }, "?class=X instances | ?match=substr objects by name");
  dev::route("/ue/obj", [](const dev::Request& r, int&) {
    int depth = (int)dev::arg_int(r, "depth", 1);
    return on_game([r, depth] { UObject* o = pick(r); return o ? std::format("#{} ", index_of(o)) + describe(o, depth) : std::string("no object\n"); });
  }, "?obj=N|class=X[&i=]|path=|cdo=X [&depth=] every property");
  dev::route("/ue/funcs", [](const dev::Request& r, int&) {
    std::string cn = dev::arg(r, "class"), m = dev::arg(r, "match");
    return on_game([cn, m] {
      UClass* c = find_class(cn);
      if (!c) return std::string("class not loaded\n");
      std::string out;
      for (UStruct* k = c; k; k = super_of(k)) {
        std::string kn = name_of(k);
        out += "-- " + kn + "\n";
        for (UObject* fn = first_child(k); fn; fn = next_field(fn)) {
          if (name_of(class_of(fn)) != "Function" && name_of(class_of(fn)) != "DelegateFunction") continue;
          std::string d = describe_function(fn);
          if (m.empty() || contains_ci(d, m)) out += "  " + d + "\n";
        }
        if (kn == "Object") break;
      }
      return out;
    });
  }, "?class=X[&match=] functions with their parameters");
  dev::route("/ue/call", [](const dev::Request& r, int&) {
    // /ue/call?obj=N&fn=Name[&args=Name:int:5,Other:float:1.5,S:str:hi] -> every out/return param after the call
    std::string fn = dev::arg(r, "fn"), args = dev::arg(r, "args");
    return on_game([r, fn, args] {
      UObject* o = pick(r);
      if (!o) return std::string("no object\n");
      UFunction* f = find_function(class_of(o), fn);
      if (!f) return std::string("no such function\n");
      Params p(f);
      std::vector<std::u16string> keep;
      size_t s = 0;
      while (s < args.size()) {
        size_t e = args.find(',', s); if (e == std::string::npos) e = args.size();
        std::string a = args.substr(s, e - s);
        size_t c1 = a.find(':'), c2 = a.find(':', c1 + 1);
        if (c1 != std::string::npos && c2 != std::string::npos) {
          std::string nm = a.substr(0, c1), ty = a.substr(c1 + 1, c2 - c1 - 1), v = a.substr(c2 + 1);
          if (ty == "int") p.set<int32_t>(nm, atoi(v.c_str()));
          else if (ty == "float") p.set<float>(nm, (float)atof(v.c_str()));
          else if (ty == "bool" || ty == "byte") p.set<uint8_t>(nm, (uint8_t)atoi(v.c_str()));
          else if (ty == "obj") p.set<UObject*>(nm, object_at(atoi(v.c_str())));
          else if (ty == "name") p.set<FName>(nm, FName{fname_index(v), 0});
          else if (ty == "str") { keep.push_back(to_u16(v)); p.set<FString>(nm, make_fstring(keep.back())); }
          else if (ty == "vec" || ty == "rot") {   // vec:x;y;z  rot:pitch;yaw;roll
            FVector f3{};
            sscanf_s(v.c_str(), "%f;%f;%f", &f3.x, &f3.y, &f3.z);
            p.set<FVector>(nm, f3);
          }
        }
        s = e + 1;
      }
      call(o, p);
      std::string out = "called " + describe_function(f) + "\n";
      for (FField* q = first_prop(f); q; q = prop_next(q)) out += "  " + prop_name(q) + " = " + value_string(q, p.buf.data(), 1) + "\n";
      return out;
    });
  }, "?obj=N&fn=Name&args=P:int:1,Q:str:x call a UFunction");
  dev::route("/ue/mem", [](const dev::Request& r, int&) {
    // /ue/mem?obj=N|addr=0x..&off=0&len=0x100 hex dump (game thread, guarded)
    int off = (int)dev::arg_int(r, "off", 0), len = (int)dev::arg_int(r, "len", 0x100);
    uintptr_t addr = (uintptr_t)dev::arg_int(r, "addr", 0);
    return on_game([r, off, len, addr] {
      uint8_t* base = addr ? (uint8_t*)addr : (uint8_t*)pick(r);
      if (!base) return std::string("no object\n");
      std::string out;
      bool ok = guarded([&] {
        for (int i = 0; i < len; i += 16) {
          out += std::format("{:#06x}:", off + i);
          for (int j = 0; j < 16 && i + j < len; ++j) out += std::format(" {:02x}", base[off + i + j]);
          out += "  |";
          for (int j = 0; j < 16 && i + j < len; j += 4) {
            uint32_t v = *(uint32_t*)(base + off + i + j);
            float f = *(float*)&v;
            out += (f > -1e6f && f < 1e6f && (v & 0x7f800000) && v) ? std::format(" {:.3g}", f) : std::format(" {}", (int32_t)v);
          }
          out += "\n";
        }
      });
      return ok ? out : out + "<fault>\n";
    });
  }, "?obj=N|addr=&off=&len= hex dump");
  dev::route("/ue/poke", [](const dev::Request& r, int&) {
    // /ue/poke?obj=N&off=0x50c&val=7[&size=4] write a value into an object (dev experiments; game thread, guarded)
    int off = (int)dev::arg_int(r, "off", 0), size = (int)dev::arg_int(r, "size", 4);
    int64_t val = dev::arg_int(r, "val", 0);
    uintptr_t addr = (uintptr_t)dev::arg_int(r, "addr", 0);   // a heap address instead of an object (an array's data)
    return on_game([r, off, size, val, addr] {
      uint8_t* base = addr ? (uint8_t*)addr : (uint8_t*)pick(r);
      if (!base || (size != 1 && size != 2 && size != 4 && size != 8)) return std::string("no object or bad size\n");
      bool ok = guarded([&] { memcpy(base + off, &val, size); });
      return ok ? std::string("ok\n") : std::string("<fault>\n");
    });
  }, "?obj=N|addr=0x..&off=&val=[&size=4] write into an object or at an address (dev)");
  dev::route("/ue/diff", [](const dev::Request& r, int&) {
    // /ue/diff?obj=N&len=0x200 dwords that changed since the previous /ue/diff of the same object
    int len = (int)dev::arg_int(r, "len", 0x200);
    return on_game([r, len] {
      static std::map<UObject*, std::vector<uint8_t>> snaps;
      UObject* o = pick(r);
      if (!o) return std::string("no object\n");
      std::vector<uint8_t> cur(len);
      if (!guarded([&] { memcpy(cur.data(), o, len); })) return std::string("<fault>\n");
      std::string out;
      auto it = snaps.find(o);
      if (it == snaps.end()) out = "snapshot taken\n";
      else for (int i = 0; i + 4 <= len; i += 4) {
        uint32_t a = *(uint32_t*)(it->second.data() + i), b = *(uint32_t*)(cur.data() + i);
        if (a != b) out += std::format("{:#06x}: {} -> {}  ({:#x} -> {:#x})\n", i, (int32_t)a, (int32_t)b, a, b);
      }
      snaps[o] = cur;
      return out.empty() ? std::string("no change\n") : out;
    });
  }, "?obj=N&len= changed dwords since the last call");
  dev::route("/ue/dt", [](const dev::Request& r, int&) {
    // /ue/dt?obj=N|path=...[&match=][&depth=1] rows of a DataTable
    std::string m = dev::arg(r, "match");
    int depth = (int)dev::arg_int(r, "depth", 1);
    return on_game([r, m, depth] {
      UObject* t = pick(r);
      if (!t || !is_a(t, "DataTable")) return std::string("not a DataTable\n");
      UStruct* s = datatable_struct(t);
      std::string out = std::format("{} rows of {}\n", full_name(t), name_of(s));
      for (auto& row : datatable_rows(t)) {
        std::string line = row.name + ": " + struct_string(s, row.data, depth);
        if (m.empty() || contains_ci(line, m)) out += line + "\n";
      }
      return out;
    });
  }, "?obj=N|path=[&match=&depth=] DataTable rows");
  dev::route("/scan", [](const dev::Request& r, int&) {
    // /scan?pat=48 8B ?? ... every match in the exe's code (rva), at most 50
    std::string out;
    int n = 0;
    scan::each_match(dev::arg(r, "pat"), [&](uintptr_t a) { out += std::format("exe+{:#x}\n", a - scan::exe_base()); return ++n < 50; });
    return out.empty() ? std::string("no match\n") : out;
  }, "?pat=hex bytes with ?? every match (rva)");
  dev::route("/memfind", [](const dev::Request& r, int&) {
    // /memfind?text=Nekomata [&ascii=1] | ?ptr=0x... every committed read-write page holding the UTF-16 (or ASCII)
    // text, or an 8-byte pointer value; at most 40 hits (dev: finding where native UI keeps what it draws)
    std::string needle;
    if (dev::has(r, "ptr")) { uintptr_t v = (uintptr_t)dev::arg_int(r, "ptr", 0); needle.assign((const char*)&v, 8); }
    else if (dev::has(r, "ascii")) needle = dev::arg(r, "text");
    else for (char c : dev::arg(r, "text")) { needle += c; needle += '\0'; }
    if (needle.empty()) return std::string("no needle\n");
    std::string out;
    int hits = 0;
    MEMORY_BASIC_INFORMATION mbi;
    for (uint8_t* a = nullptr; hits < 40 && VirtualQuery(a, &mbi, sizeof mbi); a = (uint8_t*)mbi.BaseAddress + mbi.RegionSize) {
      if (mbi.State != MEM_COMMIT || mbi.Protect != PAGE_READWRITE || mbi.RegionSize > (512u << 20)) continue;
      guarded([&] {
        uint8_t* b = (uint8_t*)mbi.BaseAddress;
        size_t n = mbi.RegionSize;
        for (size_t i = 0; i + needle.size() <= n && hits < 40; ++i)
          if (b[i] == (uint8_t)needle[0] && memcmp(b + i, needle.data(), needle.size()) == 0) { out += std::format("{:#x}\n", (uintptr_t)(b + i)); ++hits; }
      });
    }
    return out.empty() ? std::string("not found\n") : out;
  }, "?text=|ptr=0x.. find text or a pointer in the heap (dev)");
  dev::route("/ue/world", [](const dev::Request&, int&) {
    return on_game([] {
      std::string out = "world: " + full_name(gworld()) + "\n";
      out += "game instance: " + full_name(game_instance()) + "\n";
      out += "controller: " + full_name(player_controller()) + "\n";
      out += "pawn: " + full_name(player_pawn()) + "\n";
      return out;
    });
  }, "world, controller, pawn");
}
}  // namespace p3a::ue

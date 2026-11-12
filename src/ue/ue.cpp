#include "ue/ue.h"
#include <cstdlib>
#include "ue/scan.h"
#include "hooks.h"
#include "log.h"
#include "seh.h"
#include <windows.h>
#include <format>
#include <map>
#include <mutex>
#include <unordered_map>

namespace p3a::ue {
// ---- layout (UE 4.27, measured by Dumper-7 on P3R build 0x69ce3439) ----
namespace off {
constexpr int kObjFlags = 0x08, kObjIndex = 0x0C, kObjClass = 0x10, kObjName = 0x18, kObjOuter = 0x20;
constexpr int kFieldNext = 0x28;
constexpr int kStructSuper = 0x40, kStructChildren = 0x48, kStructChildProps = 0x50, kStructSize = 0x58;
constexpr int kClassCastFlags = 0xD0, kClassCDO = 0x118;
constexpr int kFuncFlags = 0xB0, kFuncParmsSize = 0xB6;
constexpr int kFFieldClass = 0x08, kFFieldNext = 0x20, kFFieldName = 0x28;
constexpr int kPropElemSize = 0x3C, kPropOffset = 0x4C, kPropExtra = 0x78;
constexpr int kEnumNames = 0x40;
constexpr int kItemSize = 0x18, kItemFlags = 0x08;
constexpr int kTextDataSource = 0x28;
}  // namespace off
constexpr uint32_t kKnownBuild = 0x69ce3439;
constexpr uintptr_t kGObjects = 0x057B4D20, kGNames = 0x057789C0, kGWorld = 0x0566B440, kProcessEvent = 0x01A410D0;
constexpr int kProcessEventIdx = 0x44;
constexpr uint64_t CASTCLASS_UClass = 0x20;
constexpr uint32_t RF_ClassDefaultObject = 0x10, RF_ArchetypeObject = 0x20;
constexpr int32_t kUnreachable = 1 << 28, kPendingKill = 1 << 29;

static uint8_t* g_objects;   // FChunkedFixedUObjectArray
static uint8_t* g_names;     // FNamePool
static UObject** g_world;
using ProcessEvent_t = void (*)(UObject*, UFunction*, void*);
static ProcessEvent_t g_pe;
static bool g_ready;

template <typename T> static T rd(const void* p, int o) { return *(const T*)((const uint8_t*)p + o); }

// ---- names ----
std::string fname(int32_t index, int32_t number) {
  if (!g_names || index < 0) return {};
  uint32_t block = (uint32_t)index >> 16, offs = ((uint32_t)index & 0xFFFF) * 2;
  if (block >= 8192) return {};
  uint8_t* blk = ((uint8_t**)(g_names + 0x10))[block];
  if (!blk) return {};
  uint8_t* e = blk + offs;
  uint16_t hdr = *(uint16_t*)e;
  bool wide = hdr & 1;
  int len = hdr >> 6;
  std::string s;
  if (wide) s = to_utf8((const char16_t*)(e + 2), len);
  else s.assign((const char*)(e + 2), len);
  if (number > 0) s += "_" + std::to_string(number - 1);
  return s;
}
std::string fname(FName n) { return fname(n.index, n.number); }

std::string to_utf8(const char16_t* s, int len) {
  if (!s) return {};
  return log::utf8(len < 0 ? std::u16string_view(s) : std::u16string_view(s, (size_t)len));
}
std::string to_utf8(const FString& s) {
  if (!s.data || s.num <= 0) return {};
  int len = s.num; if (len > 0 && s.data[len - 1] == 0) --len;
  return to_utf8(s.data, len);
}
std::u16string to_u16(std::string_view u) {
  if (u.empty()) return {};
  int n = MultiByteToWideChar(CP_UTF8, 0, u.data(), (int)u.size(), nullptr, 0);
  std::u16string out(n, u'\0');
  MultiByteToWideChar(CP_UTF8, 0, u.data(), (int)u.size(), (wchar_t*)out.data(), n);
  return out;
}
std::string text(const FText& t) {
  if (!t.data) return {};
  std::string out;
  if (!guarded([&] { out = to_utf8(*(const FString*)((const uint8_t*)t.data + off::kTextDataSource)); })) return "<bad FText>";
  return out;
}
void release_text(FText& t) {
  // FReferenceControllerBase: vtable (0 = virtual dtor, 1 = DestroyObject), int32 SharedReferenceCount, int32 Weak.
  struct Controller { void** vtbl; volatile long shared; volatile long weak; };
  auto* c = (Controller*)t.ref;
  t.data = nullptr; t.ref = nullptr;
  if (!c) return;
  if (InterlockedDecrement(&c->shared) == 0) {
    ((void (*)(Controller*))c->vtbl[1])(c);   // DestroyObject
    if (InterlockedDecrement(&c->weak) == 0) ((void (*)(Controller*, int))c->vtbl[0])(c, 1);   // deleting dtor
  }
}
FString make_fstring(const std::u16string& s) { return FString{(char16_t*)s.c_str(), (int32_t)s.size() + 1, (int32_t)s.size() + 1}; }

int32_t fname_index(std::string_view s) {
  static std::mutex mu;
  static std::unordered_map<std::string, int32_t> cache;
  static std::unordered_map<std::string, ULONGLONG> misses;   // retried every 10 s: names keep being made (levels, tables)
  std::lock_guard lk(mu);
  auto it = cache.find(std::string(s));
  if (it != cache.end()) return it->second;
  if (auto m = misses.find(std::string(s)); m != misses.end() && GetTickCount64() - m->second < 10000) return -1;
  // Walk the pool's blocks entry by entry (entries are 2-aligned: header + chars, padded).
  uint32_t cur_block = *(uint32_t*)(g_names + 0x8), cur_cursor = *(uint32_t*)(g_names + 0xC);
  for (uint32_t b = 0; b <= cur_block && b < 8192; ++b) {
    uint8_t* blk = ((uint8_t**)(g_names + 0x10))[b];
    if (!blk) break;
    uint32_t limit = b == cur_block ? cur_cursor : 0x20000;
    for (uint32_t o = 0; o + 2 <= limit;) {
      uint16_t hdr = *(uint16_t*)(blk + o);
      int len = hdr >> 6; bool wide = hdr & 1;
      if (len == 0) break;
      if (!wide && (size_t)len == s.size() && memcmp(blk + o + 2, s.data(), len) == 0) {
        int32_t idx = (int32_t)((b << 16) | (o / 2));
        cache[std::string(s)] = idx;
        return idx;
      }
      uint32_t bytes = 2 + len * (wide ? 2 : 1);
      o += (bytes + 1) & ~1u;
    }
  }
  misses[std::string(s)] = GetTickCount64();
  return -1;
}

// ---- objects ----
int32_t num_objects() { return g_objects ? *(int32_t*)(g_objects + 0x14) : 0; }
uint8_t* object_array() { return g_objects ? g_objects - 0x10 : nullptr; }
static uint8_t* item_at(int32_t i) {
  if (!g_objects || i < 0 || i >= num_objects()) return nullptr;
  uint8_t** chunks = *(uint8_t***)g_objects;
  uint8_t* chunk = chunks[i / 0x10000];
  return chunk ? chunk + (i % 0x10000) * off::kItemSize : nullptr;
}
UObject* object_at(int32_t i) { uint8_t* it = item_at(i); return it ? *(UObject**)it : nullptr; }
static uint32_t flags_of(UObject* o) { return o ? rd<uint32_t>(o, off::kObjFlags) : 0; }
int32_t index_of(UObject* o) { return o ? rd<int32_t>(o, off::kObjIndex) : -1; }
UClass* class_of(UObject* o) { return o ? rd<UClass*>(o, off::kObjClass) : nullptr; }
UObject* outer_of(UObject* o) { return o ? rd<UObject*>(o, off::kObjOuter) : nullptr; }
std::string name_of(UObject* o) { return o ? fname(rd<FName>(o, off::kObjName)) : "null"; }
std::string path_name(UObject* o) {
  if (!o) return "null";
  std::string s = name_of(o);
  for (UObject* p = outer_of(o); p; p = outer_of(p)) s = name_of(p) + "." + s;
  return s;
}
std::string full_name(UObject* o) { return o ? name_of(class_of(o)) + " " + path_name(o) : "null"; }
bool is_cdo(UObject* o) { return (flags_of(o) & (RF_ClassDefaultObject | RF_ArchetypeObject)) != 0; }
bool is_live(UObject* o) {
  if (!o) return false;
  int32_t i = index_of(o);
  uint8_t* it = item_at(i);
  if (!it || *(UObject**)it != o) return false;
  return (rd<int32_t>(it, off::kItemFlags) & (kUnreachable | kPendingKill)) == 0;
}
UStruct* super_of(UStruct* s) { return s ? rd<UStruct*>(s, off::kStructSuper) : nullptr; }
bool is_a(UObject* o, UClass* c) {
  if (!o || !c) return false;
  for (UStruct* k = class_of(o); k; k = super_of(k)) if (k == c) return true;
  return false;
}
bool is_a(UObject* o, std::string_view cn) { return is_a(o, find_class(cn)); }

WeakObj WeakObj::of(UObject* o) {
  WeakObj w;
  if (!o) return w;
  w.index = index_of(o);
  FName n = rd<FName>(o, off::kObjName);
  w.serial = n.index;
  w.number = n.number;
  return w;
}
UObject* WeakObj::resolve() const {
  if (index < 0) return nullptr;
  UObject* o = object_at(index);
  if (!o || !is_live(o)) return nullptr;
  FName n = rd<FName>(o, off::kObjName);
  if (n.index != serial || n.number != number) return nullptr;
  return o;
}

static std::mutex g_class_mu;
static std::unordered_map<std::string, UClass*> g_classes;
static ULONGLONG g_class_scan_ms;
static std::unordered_map<std::string, UClass*> scan_classes() {
  std::unordered_map<std::string, UClass*> m;
  int32_t n = num_objects();
  for (int32_t i = 0; i < n; ++i) {
    UObject* o = object_at(i);
    if (!o) continue;
    UClass* k = class_of(o);
    if (!k || !(rd<uint64_t>(k, off::kClassCastFlags) & CASTCLASS_UClass) || !is_live(o)) continue;
    m[name_of(o)] = o;
  }
  return m;
}
UClass* find_class(std::string_view cn) {
  {
    std::lock_guard lk(g_class_mu);
    auto it = g_classes.find(std::string(cn));
    if (it != g_classes.end() && is_live(it->second)) return it->second;
    // Blueprint classes load and unload as the game goes: rescan on a miss or a dead entry, at most twice a second.
    if (GetTickCount64() - g_class_scan_ms < 500 && !g_classes.empty()) return nullptr;
    g_class_scan_ms = GetTickCount64();
  }
  // The scan reads game memory outside the lock (a fault unwinds without destructors under /EHsc).
  auto m = scan_classes();
  std::lock_guard lk(g_class_mu);
  g_classes = std::move(m);
  auto it = g_classes.find(std::string(cn));
  return it == g_classes.end() ? nullptr : it->second;
}
UObject* find_object(std::string_view path) {
  int32_t n = num_objects();
  std::string_view leaf = path.substr(path.find_last_of("./:") == std::string_view::npos ? 0 : path.find_last_of("./:") + 1);
  for (int32_t i = 0; i < n; ++i) {
    UObject* o = object_at(i);
    if (!o || name_of(o) != leaf) continue;
    if (path_name(o) == path) return o;
  }
  return nullptr;
}
std::vector<UObject*> find_all(UClass* c) {
  std::vector<UObject*> out;
  if (!c) return out;
  int32_t n = num_objects();
  for (int32_t i = 0; i < n; ++i) {
    UObject* o = object_at(i);
    if (!o || is_cdo(o) || !is_a(o, c)) continue;
    uint8_t* it = item_at(i);
    if (rd<int32_t>(it, off::kItemFlags) & (kUnreachable | kPendingKill)) continue;
    out.push_back(o);
  }
  return out;
}
std::vector<UObject*> find_all(std::string_view cn) { return find_all(find_class(cn)); }
UObject* find_first(UClass* c) {
  if (!c) return nullptr;
  int32_t n = num_objects();
  for (int32_t i = 0; i < n; ++i) {
    UObject* o = object_at(i);
    if (o && !is_cdo(o) && is_a(o, c) && is_live(o)) return o;
  }
  return nullptr;
}
UObject* find_first(std::string_view cn) { return find_first(find_class(cn)); }
UObject* cdo_of(UClass* c) { return c ? rd<UObject*>(c, off::kClassCDO) : nullptr; }

// ---- properties ----
FField* first_prop(UStruct* s) { return s ? rd<FField*>(s, off::kStructChildProps) : nullptr; }
FField* prop_next(FField* p) { return p ? rd<FField*>(p, off::kFFieldNext) : nullptr; }
std::string prop_name(FField* p) { return p ? fname(rd<FName>(p, off::kFFieldName)) : ""; }
std::string prop_type(FField* p) { if (!p) return ""; void* fc = rd<void*>(p, off::kFFieldClass); return fc ? fname(*(FName*)fc) : ""; }
int32_t prop_offset(FField* p) { return p ? rd<int32_t>(p, off::kPropOffset) : -1; }
int32_t prop_size(FField* p) { return p ? rd<int32_t>(p, off::kPropElemSize) : 0; }
bool get_bool(UObject* o, std::string_view prop) {
  FField* p = o ? find_prop(class_of(o), prop) : nullptr;
  return p && prop_bool(p, o);
}
int32_t struct_size(UStruct* s) { return s ? rd<int32_t>(s, off::kStructSize) : 0; }
TableRows table_rows(UObject* table, std::string_view prop) {
  TableRows t;
  FField* dp = table ? find_prop(class_of(table), prop) : nullptr;
  auto* rows = table ? field<TArray<uint8_t>>(table, prop) : nullptr;
  if (!dp || !rows) return t;
  t.row = (UStruct*)prop_inner_object(prop_inner_prop(dp));
  t.stride = struct_size(t.row);
  t.data = rows->data;
  t.num = rows->num;
  return t;
}
// Lookups by name walk every field of a struct and its supers, building a string per field (a Blueprint widget has
// hundreds): screens doing a few per tick cost the frame. Answers are cached by (struct, name) with the struct's weak
// handle, so a class that unloads and whose memory is reused is never answered from the old entry.
template <typename T>
struct LookupCache {
  std::mutex mu;
  std::map<std::pair<UStruct*, std::string>, std::pair<WeakObj, T*>> map;
  template <typename F>
  T* get(UStruct* s, std::string_view name, F&& walk) {
    if (!s) return nullptr;
    auto key = std::make_pair(s, std::string(name));
    {
      std::lock_guard lk(mu);
      auto it = map.find(key);
      if (it != map.end() && it->second.first.resolve() == s) return it->second.second;
    }
    // The walk reads game memory outside the lock: a fault caught by guarded() unwinds without destructors (/EHsc),
    // and a lock held across it would stay locked and freeze the next lookup.
    T* v = walk();
    WeakObj w = WeakObj::of(s);
    std::lock_guard lk(mu);
    map[key] = {w, v};
    return v;
  }
};
static LookupCache<FField> g_prop_cache;
FField* find_prop(UStruct* s, std::string_view name) {
  return g_prop_cache.get(s, name, [&]() -> FField* {
    for (UStruct* k = s; k; k = super_of(k))
      for (FField* p = first_prop(k); p; p = prop_next(p))
        if (prop_name(p) == name) return p;
    return nullptr;
  });
}
UObject* prop_inner_object(FField* p) {
  std::string t = prop_type(p);
  if (t == "EnumProperty") return rd<UObject*>(p, off::kPropExtra + 8);
  if (t == "StructProperty" || t == "ObjectProperty" || t == "ClassProperty" || t == "ByteProperty" || t == "WeakObjectProperty" ||
      t == "SoftObjectProperty" || t == "LazyObjectProperty" || t == "InterfaceProperty")
    return rd<UObject*>(p, off::kPropExtra);
  return nullptr;
}
FField* prop_inner_prop(FField* p) { return p ? rd<FField*>(p, off::kPropExtra) : nullptr; }
bool prop_bool(FField* p, const void* container) {
  uint8_t byte_off = rd<uint8_t>(p, off::kPropExtra + 1), field_mask = rd<uint8_t>(p, off::kPropExtra + 3);
  return (((const uint8_t*)container)[prop_offset(p) + byte_off] & field_mask) != 0;
}
std::string enum_name(UObject* e, int64_t v) {
  if (!e) return std::to_string(v);
  auto& names = *(TArray<uint8_t>*)((uint8_t*)e + off::kEnumNames);   // TPair<FName, int64> = 16 bytes
  for (int i = 0; i < names.num; ++i) {
    uint8_t* pr = names.data + i * 16;
    if (*(int64_t*)(pr + 8) == v) { std::string s = fname(*(FName*)pr); size_t c = s.rfind("::"); return c == std::string::npos ? s : s.substr(c + 2); }
  }
  return std::to_string(v);
}
std::string enum_prop_name(UObject* obj, const char* prop) {
  FField* p = obj ? find_prop(class_of(obj), prop) : nullptr;
  if (!p) return {};
  return enum_name(prop_inner_object(p), *((uint8_t*)obj + prop_offset(p)));
}

// Through find_prop's cache: the old map here, keyed by the raw class pointer, could answer for a different class
// that reused an unloaded one's memory.
int32_t offset_of(UStruct* cls, std::string_view prop) {
  FField* p = find_prop(cls, prop);
  return p ? prop_offset(p) : -1;
}

std::string value_string(FField* p, const void* c, int depth) {
  const uint8_t* v = (const uint8_t*)c + prop_offset(p);
  std::string t = prop_type(p);
  if (t == "BoolProperty") return prop_bool(p, c) ? "true" : "false";
  if (t == "IntProperty") return std::to_string(*(int32_t*)v);
  if (t == "Int8Property") return std::to_string(*(int8_t*)v);
  if (t == "Int16Property") return std::to_string(*(int16_t*)v);
  if (t == "Int64Property") return std::to_string(*(int64_t*)v);
  if (t == "UInt16Property") return std::to_string(*(uint16_t*)v);
  if (t == "UInt32Property") return std::to_string(*(uint32_t*)v);
  if (t == "UInt64Property") return std::to_string(*(uint64_t*)v);
  if (t == "FloatProperty") return std::format("{:.3f}", *(float*)v);
  if (t == "DoubleProperty") return std::format("{:.3f}", *(double*)v);
  if (t == "ByteProperty") { UObject* e = prop_inner_object(p); return e ? enum_name(e, *v) : std::to_string(*v); }
  if (t == "EnumProperty") { int sz = prop_size(p); int64_t x = sz == 1 ? *(uint8_t*)v : sz == 2 ? *(int16_t*)v : sz == 4 ? *(int32_t*)v : *(int64_t*)v; return enum_name(prop_inner_object(p), x); }
  if (t == "NameProperty") return fname(*(FName*)v);
  if (t == "StrProperty") return "\"" + to_utf8(*(FString*)v) + "\"";
  if (t == "TextProperty") return "\"" + text(*(FText*)v) + "\"";
  if (t == "ObjectProperty" || t == "ClassProperty") { UObject* o = *(UObject**)v; return o ? name_of(o) + " (" + name_of(class_of(o)) + ")" : "null"; }
  if (t == "ArrayProperty") {
    auto& a = *(TArray<uint8_t>*)v;
    std::string s = std::format("[{}]", a.num);
    FField* inner = prop_inner_prop(p);
    if (depth > 0 && inner && a.num > 0 && a.data) {
      int esz = prop_size(inner);
      s += " {";
      for (int i = 0; i < a.num && i < (depth >= 4 ? 64 : 16); ++i) {
        // The inner property has offset 0: render each element as a container of its own.
        s += (i ? ", " : "") + value_string(inner, a.data + i * esz, depth - 1);
      }
      s += a.num > (depth >= 4 ? 64 : 16) ? ", ...}" : "}";
    }
    return s;
  }
  if (t == "StructProperty") {
    UStruct* st = prop_inner_object(p);
    std::string sn = name_of(st);
    if (sn == "Vector") return std::format("({:.1f}, {:.1f}, {:.1f})", ((float*)v)[0], ((float*)v)[1], ((float*)v)[2]);
    if (sn == "Rotator") return std::format("(p {:.1f}, y {:.1f}, r {:.1f})", ((float*)v)[0], ((float*)v)[1], ((float*)v)[2]);
    if (sn == "Vector2D") return std::format("({:.1f}, {:.1f})", ((float*)v)[0], ((float*)v)[1]);
    if (depth <= 0) return sn + "{...}";
    std::string s = sn + "{";
    bool first = true;
    for (UStruct* k = st; k; k = super_of(k))
      for (FField* f = first_prop(k); f; f = prop_next(f)) {
        s += (first ? "" : ", ") + prop_name(f) + "=" + value_string(f, v, depth - 1);
        first = false;
        if (s.size() > 300) return s + ", ...}";   // PostProcessSettings and friends run to kilobytes
      }
    return s + "}";
  }
  if (t == "WeakObjectProperty") { int32_t idx = *(int32_t*)v; UObject* o = object_at(idx); return o ? "weak " + name_of(o) : "weak null"; }
  if (t == "MapProperty" || t == "SetProperty") return std::format("{{{} entries}}", *(int32_t*)(v + 8));
  return "<" + t + ">";
}

std::string describe(UObject* o, int depth) {
  if (!o) return "null\n";
  std::string out = full_name(o) + "\n";
  std::vector<UStruct*> chain;
  for (UStruct* k = class_of(o); k; k = super_of(k)) chain.push_back(k);
  for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
    for (FField* p = first_prop(*it); p; p = prop_next(p)) {
      std::string val;
      if (!guarded([&] { val = value_string(p, o, depth); })) val = "<fault>";
      out += std::format("  {:#06x} {}: {} = {}\n", prop_offset(p), prop_name(p), prop_type(p), val);
    }
  }
  return out;
}

// ---- data tables ----
// TMap<FName, uint8*> = TSet of TSetElement{TPair{FName 8, uint8* 8}, HashNextId 4, HashIndex 4} (0x18 each) in a
// TSparseArray: elements TArray at +0, allocation flags (TBitArray, inline 4 dwords) at +0x10, NumBits at +0x20.
UStruct* datatable_struct(UObject* t) { return t ? rd<UStruct*>(t, 0x28) : nullptr; }
std::vector<Row> datatable_rows(UObject* t) {
  std::vector<Row> out;
  if (!t) return out;
  guarded([&] {
    uint8_t* map = (uint8_t*)t + 0x30;
    auto& elems = *(TArray<uint8_t>*)map;
    const uint32_t* inline_bits = (const uint32_t*)(map + 0x10);
    const uint32_t* secondary = *(const uint32_t**)(map + 0x20);
    int32_t num_bits = *(int32_t*)(map + 0x28);
    const uint32_t* bits = secondary ? secondary : inline_bits;
    for (int i = 0; i < elems.num; ++i) {
      bool allocated = i >= num_bits || (bits[i / 32] >> (i % 32)) & 1;
      if (!allocated) continue;
      uint8_t* e = elems.data + i * 0x18;
      out.push_back(Row{fname(*(FName*)e), *(uint8_t**)(e + 8)});
    }
  });
  return out;
}
uint8_t* datatable_find(UObject* t, std::string_view row) {
  for (auto& r : datatable_rows(t)) if (r.name == row) return r.data;
  return nullptr;
}
std::string struct_string(UStruct* s, const void* data, int depth) {
  std::string out;
  bool first = true;
  std::vector<UStruct*> chain;
  for (UStruct* k = s; k; k = super_of(k)) chain.push_back(k);
  for (auto it = chain.rbegin(); it != chain.rend(); ++it)
    for (FField* f = first_prop(*it); f; f = prop_next(f)) {
      std::string v;
      if (!guarded([&] { v = value_string(f, data, depth); })) v = "<fault>";
      out += (first ? "" : ", ") + prop_name(f) + "=" + v;
      first = false;
    }
  return out;
}

// ---- functions ----
UObject* first_child(UStruct* s) { return s ? rd<UObject*>(s, off::kStructChildren) : nullptr; }
UObject* next_field(UObject* f) { return f ? rd<UObject*>(f, off::kFieldNext) : nullptr; }
static LookupCache<UObject> g_func_cache;   // see find_prop
UFunction* find_function(UClass* c, std::string_view name) {
  return g_func_cache.get(c, name, [&]() -> UFunction* {
    for (UStruct* k = c; k; k = super_of(k))
      for (UObject* f = rd<UObject*>(k, off::kStructChildren); f; f = rd<UObject*>(f, off::kFieldNext))
        if (name_of(f) == name) return f;
    return nullptr;
  });
}
uint16_t params_size(UFunction* f) { return f ? rd<uint16_t>(f, off::kFuncParmsSize) : 0; }
static uint32_t function_flags(UFunction* f) { return f ? rd<uint32_t>(f, off::kFuncFlags) : 0; }
std::string describe_function(UFunction* f) {
  if (!f) return "null";
  std::string s = name_of(f) + "(";
  bool first = true;
  for (FField* p = first_prop(f); p; p = prop_next(p)) {
    s += std::format("{}{} {} @{:#x}", first ? "" : ", ", prop_type(p), prop_name(p), prop_offset(p));
    first = false;
  }
  return s + std::format(") flags={:#x} size={}", function_flags(f), params_size(f));
}
static ProcessEvent_t g_pe_orig;
void process_event(UObject* o, UFunction* f, void* params) { (g_pe_orig ? g_pe_orig : g_pe)(o, f, params); }
Params::Params(UFunction* f) : fn(f), buf(f ? params_size(f) : 0, 0) {}
bool call(UObject* o, Params& p) {
  if (!o || !p.fn) return false;
  process_event(o, p.fn, p.buf.empty() ? nullptr : p.buf.data());
  return true;
}
bool call(UObject* o, std::string_view fn) {
  UFunction* f = find_function(class_of(o), fn);
  if (!f) return false;
  Params p(f);
  return call(o, p);
}

// ---- ProcessEvent hook ----
static void pe_hook(UObject* o, UFunction* f, void* params) {
  // The mod's frame is PeekMessageW (hooks::pump at the top of every frame). ProcessEvent only stands in when that
  // pump has gone quiet (a load that stops pumping messages): ticking here as well ran a second tick per frame,
  // nested inside whatever UFunction the game was running.
  if (hooks::on_game_thread() && hooks::ms_since_tick() > 250) hooks::pump();
  g_pe_orig(o, f, params);
}
static const volatile bool* g_exit_flag = nullptr;
bool install_hooks() {
  // RequestEngineExit (its callers pass "Win RequestExit", "ConsoleCtrl RequestExit"): mov byte ptr [GIsRequestingExit], 1
  // after a call. The one match or nothing: a wrong flag would stop the mod.
  if (uintptr_t a = scan::find_unique("48 83 EC 28 B1 01 E8 ?? ?? ?? ?? C6 05 ?? ?? ?? ?? 01 48 83 C4 28 C3"))
    g_exit_flag = (const volatile bool*)(scan::rip(a + 13) + 1);   // the instruction ends one byte after its rel32
  log::writef("ue: GIsRequestingExit={:#x}", g_exit_flag ? (uintptr_t)g_exit_flag - scan::exe_base() : 0);
  g_pe_orig = g_pe;
  return hooks::attach((void**)&g_pe_orig, (void*)&pe_hook, "UObject::ProcessEvent");
}
void remove_hooks() { if (g_pe_orig && g_pe_orig != g_pe) hooks::detach((void**)&g_pe_orig, (void*)&pe_hook); }

// ---- world ----
UObject* gworld() { return g_world ? *g_world : nullptr; }
UObject* game_instance() { return get<UObject*>(gworld(), "OwningGameInstance"); }
UObject* player_controller() {
  UObject* gi = game_instance();
  auto* players = field<TArray<UObject*>>(gi, "LocalPlayers");
  if (!players || players->num < 1) return nullptr;
  return get<UObject*>(players->data[0], "PlayerController");
}
UObject* player_pawn() { return get<UObject*>(player_controller(), "Pawn"); }

// ---- init ----
// GObjects and GNames. Signatures first (they survive a patch); the measured offsets are the fallback and the
// cross-check. The image is searched (~100 ms a signature) on every call until the name pool answers at what was
// found (the code may not be unpacked yet on the first calls); from then on init() only checks. Each line is logged
// once, or again when what it says changes.
static bool g_located;
static void locate(uintptr_t base, uint32_t ts) {
  uintptr_t objs = 0, names = 0;
  if (uintptr_t a = scan::find("48 8B 05 ?? ?? ?? ?? 48 8B 0C ?? 48 8D 04 ?? 48 85 C0 74 ?? 44 39 40 ?? 75 ?? F7 40 ?? 00 00 00 30 75 ?? 48 8B 00")) objs = scan::rip(a + 3);
  if (uintptr_t a = scan::find("4C 8D 05 ?? ?? ?? ?? EB ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 4C 8B C0 C6 05 ?? ?? ?? ?? 01")) names = scan::rip(a + 3);
  static uintptr_t logged_objs = ~(uintptr_t)0, logged_names = ~(uintptr_t)0;
  if (objs != logged_objs || names != logged_names) {
    logged_objs = objs; logged_names = names;
    log::writef("ue: build {:#x}{}; sig GObjects={:#x} GNames={:#x} (dump: {:#x} {:#x})", ts, ts == kKnownBuild ? " (known)" : " (UNKNOWN)",
                objs ? objs - base : 0, names ? names - base : 0, kGObjects, kGNames);
  }
  if (!objs) objs = base + kGObjects;
  if (!names) names = base + kGNames;
  g_objects = (uint8_t*)objs;
  g_names = (uint8_t*)names;
}
bool init() {
  uintptr_t base = scan::exe_base();
  uint32_t ts = scan::exe_timestamp();
  uintptr_t pe = 0, world = 0;
  if (!g_located) locate(base, ts);
  if (fname(0) != "None") {
    static bool said = false;
    if (!said) { said = true; log::writef("ue: name pool check failed ('{}')", fname(0)); }
    g_ready = false;
    return false;
  }
  g_located = true;
  // ProcessEvent: vtable slot 0x44 of any UObject (the first object is /Script/CoreUObject).
  UObject* o0 = object_at(0);
  if (o0) pe = (*(uintptr_t**)o0)[kProcessEventIdx];
  static uintptr_t logged_pe = ~(uintptr_t)0;
  bool news = pe != logged_pe || !g_ready;
  logged_pe = pe;
  if (news && ts == kKnownBuild && pe != base + kProcessEvent) log::writef("ue: ProcessEvent slot {:#x} != measured {:#x}", pe - base, kProcessEvent);
  g_pe = (ProcessEvent_t)pe;
  world = ts == kKnownBuild ? base + kGWorld : 0;
  g_world = (UObject**)world;
  g_ready = true;
  if (news) log::writef("ue: ready, {} objects, first='{}', ProcessEvent=exe+{:#x}", num_objects(), full_name(o0), pe - base);
  return true;
}
bool known_build() {
  static const bool known = scan::exe_timestamp() == kKnownBuild && !getenv("P3A_UNSUPPORTED");   // the env var: dev, to test it
  return known;
}
bool exit_requested() { return g_exit_flag && *g_exit_flag; }

std::string status() { return std::format("ue ready={} objects={} world={}", g_ready, num_objects(), full_name(gworld())); }
}  // namespace p3a::ue

#pragma once
// A small Unreal Engine 4.27 reflection layer for P3R (offsets measured by Dumper-7 on build 0x69ce3439 and
// re-validated at init). Everything is resolved BY NAME at runtime -- classes, properties, functions -- so a game
// patch that only moves code keeps working. Rules (Grim Dark lessons):
//  - Never hold a UObject* across frames: hold ue::WeakObj and resolve it in the frame you use it.
//  - Touch game objects only on the game thread (hooks::run_on_game_thread from other threads).
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace p3a::ue {
struct UObject;  // opaque; read through the helpers below
using UClass = UObject;
using UStruct = UObject;
using UFunction = UObject;
struct FField;   // FProperty and friends (not UObjects in 4.25+)

// ---- engine containers we read ----
template <typename T>
struct TArray { T* data; int32_t num; int32_t max; };
struct FString { char16_t* data; int32_t num; int32_t max; };  // num counts the terminator
struct FName { int32_t index; int32_t number; };
struct FText { void* data; void* ref; uint32_t flags; uint32_t pad; };
struct FVector { float x, y, z; };
struct FRotator { float pitch, yaw, roll; };

// Weak handle: the GUObjectArray index plus the object's full name (base index + number). resolve() returns null once
// the object is gone or its index holds another object (a later battle's BP_BtlShadowBase_C_12 is not the freed _7).
struct WeakObj {
  int32_t index = -1, serial = 0, number = 0;   // serial: the name's comparison index
  using Key = std::tuple<int32_t, int32_t, int32_t>;
  UObject* resolve() const;
  bool valid() const { return resolve() != nullptr; }
  static WeakObj of(UObject* o);
  Key key() const { return {index, serial, number}; }
  bool operator==(const WeakObj& o) const { return key() == o.key(); }
};

bool init();          // locate GObjects / GNames / ProcessEvent; false if this build is not understood
bool ready();
std::string status();

// ---- names and strings ----
std::string fname(FName n);
std::string fname(int32_t index, int32_t number = 0);
int32_t fname_index(std::string_view s);  // -1 if the name was never made (cached lookup by scanning the pool)
std::string to_utf8(const FString& s);
std::string to_utf8(const char16_t* s, int len = -1);
std::u16string to_u16(std::string_view utf8);
std::string text(const FText& t);        // an FText's display string
FString make_fstring(const std::u16string& s);
// An FText a UFunction returned into one of our parameter buffers owns a shared reference: release it, or every call
// leaks one (UEditableText::GetText polled per tick, 2026-10-27). Thread-safe TSharedRef semantics (UE 4.27).
void release_text(FText& t);  // points into `s`: valid while `s` lives (for passing parameters)

// ---- objects ----
int32_t num_objects();
uint8_t* object_array();   // GUObjectArray (FUObjectArray; ObjObjects, the chunked array, at +0x10)
UObject* object_at(int32_t i);
int32_t index_of(UObject* o);
UClass* class_of(UObject* o);
UObject* outer_of(UObject* o);
std::string name_of(UObject* o);
std::string full_name(UObject* o);   // "Class /Script/xrd777.MsgManager" style: class + path
std::string path_name(UObject* o);   // "/Script/xrd777.MsgManager"
bool is_cdo(UObject* o);             // RF_ClassDefaultObject or archetype
bool is_live(UObject* o);            // in the array, not pending kill / unreachable
UStruct* super_of(UStruct* s);
bool is_a(UObject* o, UClass* c);
bool is_a(UObject* o, std::string_view class_name);
UClass* find_class(std::string_view short_name);       // "MsgManager", "BP_BtlTopUI_C" (cached; live classes only)
UObject* find_object(std::string_view path);            // "/Script/xrd777.Default__MsgManager"
UObject* find_first(UClass* c);                         // first live non-CDO instance
UObject* find_first(std::string_view class_name);
// A long-lived object found by class (a data table, GlobalWork): kept as a WeakObj, and while it is missing the
// scan (~1 ms) runs at most once a second, so a per-row or per-tick caller never scans every call.
struct CachedFind {
  WeakObj w;
  unsigned long long miss_ms = 0;
  UObject* get(std::string_view class_name);
};
std::vector<UObject*> find_all(UClass* c);
std::vector<UObject*> find_all(std::string_view class_name);
UObject* cdo_of(UClass* c);

// ---- properties ----
FField* find_prop(UStruct* s, std::string_view name);   // walks supers
int32_t prop_offset(FField* p);
int32_t prop_size(FField* p);
std::string prop_name(FField* p);
std::string prop_type(FField* p);   // "IntProperty", "StructProperty", ...
FField* prop_next(FField* p);
FField* first_prop(UStruct* s);     // this struct's own properties (not supers)
UObject* prop_inner_object(FField* p);   // StructProperty -> UScriptStruct, ObjectProperty -> UClass, EnumProperty/ByteProperty -> UEnum
FField* prop_inner_prop(FField* p);      // ArrayProperty -> inner property
bool prop_bool(FField* p, const void* container);
std::string enum_name(UObject* uenum, int64_t value);
int32_t struct_size(UStruct* s);

// Offset of `prop` in `cls`, cached by (class, name); -1 if absent. Use field<T>() for typed access.
int32_t offset_of(UStruct* cls, std::string_view prop);
template <typename T>
T* field(UObject* o, std::string_view prop) {
  if (!o) return nullptr;
  int32_t off = offset_of(class_of(o), prop);
  return off < 0 ? nullptr : (T*)((uint8_t*)o + off);
}
bool get_bool(UObject* o, std::string_view prop);   // bitfield-aware
template <typename T>
T get(UObject* o, std::string_view prop, T def = T{}) { T* p = field<T>(o, prop); return p ? *p : def; }

// A readable rendering of a property value (dev dumps and generic screens).
std::string value_string(FField* p, const void* container, int depth = 1);
// Every property of an object, one per line ("Name: Type = value").
std::string describe(UObject* o, int depth = 1);

// ---- data tables ----
// UDataTable: RowStruct (UScriptStruct*) at +0x28, RowMap TMap<FName, uint8*> at +0x30. Rows in table order.
struct Row { std::string name; uint8_t* data; };
std::vector<Row> datatable_rows(UObject* table);
UStruct* datatable_struct(UObject* table);
uint8_t* datatable_find(UObject* table, std::string_view row);
// Every field of a struct instance on one line ("A=1, B=x").
std::string struct_string(UStruct* s, const void* data, int depth = 1);

// ---- functions ----
UFunction* find_function(UClass* c, std::string_view name);   // walks supers
UObject* first_child(UStruct* s);   // UField chain (functions) of this struct only
UObject* next_field(UObject* f);
uint16_t params_size(UFunction* f);
void process_event(UObject* o, UFunction* f, void* params);
std::string describe_function(UFunction* f);

// Call a UFunction with a parameter block built by name. `Params` owns a zeroed buffer of the function's ParmsSize.
struct Params {
  UFunction* fn = nullptr;
  std::vector<uint8_t> buf;
  explicit Params(UFunction* f);
  template <typename T> T* at(std::string_view name) { int32_t o = offset_of(fn, name); return o < 0 ? nullptr : (T*)(buf.data() + o); }
  template <typename T> bool set(std::string_view name, const T& v) { T* p = at<T>(name); if (p) *p = v; return p != nullptr; }
  template <typename T> T ret(std::string_view name = "ReturnValue") { T* p = at<T>(name); return p ? *p : T{}; }
};
bool call(UObject* o, Params& p);   // false if fn is null; runs ProcessEvent
bool call(UObject* o, std::string_view fn_name);   // no parameters
// A parameterless UFunction of `o`, by name: its return value, or `missing` when the object or function is absent.
template <typename T>
T call_ret(UObject* o, std::string_view fn, T missing = T{}) {
  UFunction* f = o ? find_function(class_of(o), fn) : nullptr;
  if (!f) return missing;
  Params p(f);
  call(o, p);
  return p.ret<T>();
}

// The ProcessEvent hook: calls hooks::pump on the game thread (the fallback frame before the window exists).
bool install_hooks();
void remove_hooks();

// ---- world ----
UObject* gworld();        // the current UWorld
UObject* game_instance();
UObject* player_controller();
UObject* player_pawn();
}  // namespace p3a::ue

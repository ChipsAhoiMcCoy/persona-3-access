#include "screens/umg.h"
#include "core/text_clean.h"

namespace p3a::screens::umg {
using namespace ue;

bool visible(UObject* w) {
  FField* p = w ? find_prop(class_of(w), "Visibility") : nullptr;
  if (!p) return true;
  std::string v = enum_name(prop_inner_object(p), *((uint8_t*)w + prop_offset(p)));
  return v != "Collapsed" && v != "Hidden";
}

bool in_viewport(UObject* uw) {
  static UFunction* fn = nullptr;
  if (!fn || !is_live(fn)) fn = find_function(find_class("UserWidget"), "IsInViewport");
  if (!fn || !uw) return false;
  Params p(fn);
  call(uw, p);
  return p.ret<bool>();
}

static void walk(UObject* w, std::vector<std::string>& out, int depth) {
  if (!w || depth > 40 || !visible(w)) return;
  if (is_a(w, "TextBlock") || is_a(w, "RichTextBlock")) {
    if (auto* t = field<FText>(w, "Text")) {
      std::string s = core::clean_text(text(*t));
      if (core::has_words(s)) out.push_back(s);
    }
    return;
  }
  if (is_a(w, "UserWidget")) {
    if (UObject* tree = get<UObject*>(w, "WidgetTree")) walk(get<UObject*>(tree, "RootWidget"), out, depth + 1);
    return;
  }
  if (auto* slots = field<TArray<UObject*>>(w, "Slots"))
    for (int i = 0; i < slots->num; ++i) walk(get<UObject*>(slots->data[i], "Content"), out, depth + 1);
}

std::vector<std::string> texts(UObject* uw) {
  std::vector<std::string> out;
  walk(uw, out, 0);
  return out;
}
}  // namespace p3a::screens::umg

#pragma once
#include "ue/ue.h"
#include <string>
#include <string_view>
#include <vector>

namespace p3a::watch {
// Live instances of the classes the mod looks for (screens' draw actors, battle components), kept current by the
// engine itself: a listener in GUObjectArray's create-listener list hears every object the game makes, and each
// tick sorts the new ones by class. No object scans after the first, so a screen's object is known the tick after
// it appears and an absent one costs nothing (each absent screen used to rescan all ~70k objects twice a second, a
// 13-17 ms hitch every 500 ms, measured 2026-10-28). Without the listener (unknown layout), get() falls back to a
// throttled scan per class.
// Registers a class (subclasses count) and returns its handle; any thread. A class wanted with `all` keeps every
// instance (the field's hit boxes, NPCs and shadows), otherwise the newest few.
int want(std::string_view class_name, bool all = false);
ue::UObject* get(int id);                // the newest live, non-CDO instance, or null. Game thread.
std::vector<ue::UObject*> all(int id);   // every live, non-CDO instance, oldest first (want(name, true)). Game thread.
void install();                          // after msg::install (the listener list grows through GMalloc)
void tick();                             // game thread, every mod tick, before the screens
void remove();                           // unload: leaves the list, waits out any call in flight
std::string status();
}  // namespace p3a::watch

// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
// The navigator's view of a screen: a screen declares a
// graph over OUR model of what it shows (immediate mode, rebuilt per operation), and the navigator owns focus,
// movement and every announcement. The game's drawn UI is never the state store: a screen reads the
// game's objects for its model and drives the game (its cursor, confirm) to match the mod's focus.
// Engine-free: the game-side half is screens::MenuScreen (src/screens/menu.h).
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace p3a::ui {
class GraphBuilder;

class Screen {
 public:
  virtual ~Screen() = default;
  virtual std::string_view key() const { return {}; }
  virtual bool is_active() { return true; }
  // Graph declaration, IMMEDIATE MODE: called on every render; declare controls fresh each call (no retained tree);
  // focus persists by ControlId identity. Declare nothing while content does not exist.
  virtual void build(GraphBuilder&) {}
  // The screen's title, for the host to speak on entry (the navigator never says it).
  virtual std::string screen_name() const { return {}; }
  // Whether Tab wraps from the last stop back to the first (the arrows never wrap: an edge repeats the item).
  virtual bool wrap() const { return false; }
  // Keep per-screen nav state (focus, stop memory) when the screen closes. The host reads this
  // and calls GraphNavigator::screen_closed when it is false.
  virtual bool keep_state_on_pop() const { return false; }
};
}  // namespace p3a::ui

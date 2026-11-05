// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
// The navigator's view of a screen, after Grim Dark's core/screen.h (itself from wotr-access): a screen declares a
// graph over OUR model of what it shows (immediate mode, rebuilt per operation), and the navigator owns focus,
// movement, type-ahead and every announcement. The game's drawn UI is never the state store: a screen reads the
// game's objects for its model and drives the game (its cursor, confirm) to match the mod's focus.
// Engine-free: the game-side half is screens::MenuScreen (src/screens/menu.h).
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace p3a::ui {
class GraphBuilder;

// A screen-level command dispatched by string id (Back and the like).
struct ScreenAction {
  std::string id;
  std::function<void()> execute;
};
namespace action_ids { inline constexpr std::string_view Back = "back"; }

class Screen {
 public:
  virtual ~Screen() = default;
  virtual std::string_view key() const { return {}; }
  virtual bool is_active() { return true; }
  // Graph declaration, IMMEDIATE MODE: called on every render; declare controls fresh each call (no retained tree);
  // focus persists by ControlId identity. Declare nothing while content does not exist.
  virtual void build(GraphBuilder&) {}
  // The screen's title (spoken by the host on entry; the navigator uses it for an unfocused screen's Tab).
  virtual std::string screen_name() const { return {}; }
  // Whether Tab wraps from the last stop back to the first, and arrows from the last item to the first.
  virtual bool wrap() const { return false; }
  // Keep per-screen nav state (focus, stop memory, expansion) when the screen closes.
  virtual bool keep_state_on_pop() const { return false; }
  // Start with no focused element: arrows bubble to global handlers and Tab enters the tree.
  virtual bool start_unfocused() const { return false; }
  // Stand down input dispatch entirely (a text field reads raw keys itself).
  virtual bool captures_raw_input() const { return false; }
  // Whether typing letters runs type-ahead over the focused stop.
  virtual bool allows_typeahead() const { return true; }
  // Screen-level actions by id.
  virtual std::vector<ScreenAction> actions() { return {}; }
  bool invoke_action(std::string_view id) {
    for (auto& a : actions())
      if (a.id == id && a.execute) { a.execute(); return true; }
    return false;
  }
};
}  // namespace p3a::ui

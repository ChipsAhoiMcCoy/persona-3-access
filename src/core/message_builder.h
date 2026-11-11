// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace p3a::ui {

// Fluent message accumulator. The value it carries is the separation discipline: consecutive fragments
// are joined with a space, list-item boundaries with a comma. A chain of collaborating functions can
// each append its piece without coordinating spacing. Single use: build() may be called once.
// The navigator's announcements (graph_announcer.cpp, navigator.cpp) are assembled with it.
class MessageBuilder {
 public:
  MessageBuilder() = default;
  MessageBuilder(const MessageBuilder&) = delete;
  MessageBuilder& operator=(const MessageBuilder&) = delete;
  MessageBuilder(MessageBuilder&&) = default;
  MessageBuilder& operator=(MessageBuilder&&) = default;

  // Append a text fragment. Separated from preceding content by a space; the first fragment of a
  // fresh list item is preceded by a comma when anything came before it. Empty fragments are ignored so
  // optional pieces can be appended blindly. A lone " " is rejected: spacing is the builder's job.
  MessageBuilder& fragment(std::string_view text);
  // Start a new list item: the next fragment opens it (comma-separated from what came before).
  MessageBuilder& list_item();
  // Another builder is not a fragment: build it first, then fragment(its string).
  MessageBuilder& fragment(const MessageBuilder& other) = delete;

  // Finish. Throws std::logic_error if called twice.
  std::string build();

 private:
  enum class State { Initial, ListItem, Fragment, FragmentInList, Built };
  void check_not_built() const;
  std::string text_;
  State state_ = State::Initial;
};

}  // namespace p3a::ui

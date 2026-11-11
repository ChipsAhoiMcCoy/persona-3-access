// Altered from Grim Dark (GD Access) by Austin Hicks, itself ported from Dark Vision (WrathAccess) by Bradley Renshaw; both zlib, see LICENSES/. This is not the original software.
#pragma once
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace p3a::ui {

// A value-equatable key, used for structural control keys, Tab-stop keys, region keys and menu row
// keys. It is either
//   - empty      — no key (a node outside any region, a row with no column key), or
//   - a string   — including composites the caller has already flattened ("personas#3").
// An empty key and a key made from "" are different keys.
// Deliberately NOT a general variant: the engine-free core must not depend on game types, and a
// composite key is a string concatenation.
class Key {
 public:
  Key() = default;  // the empty key
  Key(std::string text) : set_(true), text_(std::move(text)) {}
  Key(const char* text) : set_(true), text_(text) {}
  Key(std::string_view text) : set_(true), text_(text) {}

  bool empty() const { return !set_; }
  bool is_string() const { return set_; }
  // The string payload; empty for the empty key.
  const std::string& text() const { return text_; }
  std::string to_string() const { return text_; }

  friend bool operator==(const Key& a, const Key& b) { return a.set_ == b.set_ && a.text_ == b.text_; }
  friend bool operator!=(const Key& a, const Key& b) { return !(a == b); }

  std::size_t hash() const;

 private:
  bool set_ = false;
  std::string text_;
};

/// The identity of a control (graph node): a structural key (see Key), stable across rebuilds, so
/// focus follows the logical control from one render to the next. Never empty. Equality and hashing
/// are the key's, so it is a stable dictionary key (the graph stores nodes and traversal order by it).
class ControlId {
 public:
  static ControlId structural(Key structural_key) { return ControlId(std::move(structural_key)); }

  const Key& structural_key() const { return structural_key_; }

  friend bool operator==(const ControlId& a, const ControlId& b) {
    return a.structural_key_ == b.structural_key_;
  }
  friend bool operator!=(const ControlId& a, const ControlId& b) { return !(a == b); }

  std::string to_string() const;

 private:
  explicit ControlId(Key structural_key);
  Key structural_key_;
};

}  // namespace p3a::ui

template <>
struct std::hash<p3a::ui::Key> {
  std::size_t operator()(const p3a::ui::Key& k) const noexcept { return k.hash(); }
};

template <>
struct std::hash<p3a::ui::ControlId> {
  std::size_t operator()(const p3a::ui::ControlId& id) const noexcept {
    return id.structural_key().hash();
  }
};

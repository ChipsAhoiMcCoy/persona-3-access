#include "core/text_field.h"
#include "core/strings.h"
#include <algorithm>

namespace p3a::core {
namespace {
bool continuation(const std::string& s, size_t i) { return i < s.size() && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80; }
size_t next_char(const std::string& s, size_t i) {
  if (i < s.size()) ++i;
  while (continuation(s, i)) ++i;
  return i;
}
size_t prev_char(const std::string& s, size_t i) {
  if (i > 0) --i;
  while (i > 0 && continuation(s, i)) --i;
  return i;
}
bool one_char(const std::string& s) { return !s.empty() && next_char(s, 0) == s.size(); }
std::string or_blank(std::string s) { return s.empty() ? std::string(strings::kBlank) : s; }
}  // namespace

void TextField::open(std::string text) {
  text_ = std::move(text);
  caret_ = text_.size();
  back_ = false;
}

std::string TextField::at_caret() const { return or_blank(text_.substr(caret_, next_char(text_, caret_) - caret_)); }

std::string TextField::pressed(Key key) {
  back_ = false;
  switch (key) {
    case Key::Left: caret_ = prev_char(text_, caret_); return at_caret();
    case Key::Right: caret_ = next_char(text_, caret_); return at_caret();
    case Key::Home: caret_ = 0; return at_caret();
    case Key::End: caret_ = text_.size(); return at_caret();
    case Key::Line: return or_blank(text_);
    case Key::Backspace: back_ = true; return {};
  }
  return {};
}

std::string TextField::edited(const std::string& now) {
  // What changed lies at the caret: the text before it stays (before the character Backspace takes).
  size_t keep = std::min(back_ ? prev_char(text_, caret_) : caret_, std::min(text_.size(), now.size()));
  size_t start = 0;
  while (start < keep && text_[start] == now[start]) ++start;
  size_t tail = 0;
  while (tail < text_.size() - start && tail < now.size() - start && text_[text_.size() - 1 - tail] == now[now.size() - 1 - tail]) ++tail;
  while (start > 0 && continuation(now, start)) --start;   // text that opens on a stray continuation byte stops at 0
  while (tail > 0 && continuation(now, now.size() - tail)) --tail;
  std::string removed = text_.substr(start, text_.size() - tail - start);
  std::string added = now.substr(start, now.size() - tail - start);
  text_ = now;
  caret_ = now.size() - tail;
  back_ = false;
  if (added.empty() && one_char(removed)) return removed;
  if (removed.empty() && one_char(added)) return added;
  return or_blank(now);
}

}  // namespace p3a::core

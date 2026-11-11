#pragma once
// The few UTF-8 steps the engine-free core needs: walking a string a character at a time, and reading or writing
// one code point. No tables and no validation beyond what the walk needs: a byte that does not fit is passed
// through as itself, so malformed text is never dropped or corrupted.
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace p3a::core::utf8 {

// s[i] is a continuation byte (10xxxxxx): the inside of a character, never its start. False past the end.
inline bool is_continuation(std::string_view s, std::size_t i) {
  return i < s.size() && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80;
}

// The index of the character after the one at i (s.size() at the end).
inline std::size_t next_char(std::string_view s, std::size_t i) {
  if (i < s.size()) ++i;
  while (is_continuation(s, i)) ++i;
  return i;
}

// The index of the character before the one at i (0 at the start).
inline std::size_t prev_char(std::string_view s, std::size_t i) {
  if (i > 0) --i;
  while (i > 0 && is_continuation(s, i)) --i;
  return i;
}

// Decodes the code point starting at `i` (i < s.size()), advancing `i`. A byte that starts no valid sequence
// decodes as itself.
inline std::uint32_t next_codepoint(std::string_view s, std::size_t& i) {
  const auto b0 = static_cast<unsigned char>(s[i]);
  auto byte = [&s](std::size_t k) { return static_cast<std::uint32_t>(static_cast<unsigned char>(s[k])); };
  if (b0 < 0x80) {
    i += 1;
    return b0;
  }
  if ((b0 & 0xE0) == 0xC0 && is_continuation(s, i + 1)) {
    const std::uint32_t cp = ((b0 & 0x1Fu) << 6) | (byte(i + 1) & 0x3Fu);
    i += 2;
    return cp;
  }
  if ((b0 & 0xF0) == 0xE0 && is_continuation(s, i + 1) && is_continuation(s, i + 2)) {
    const std::uint32_t cp = ((b0 & 0x0Fu) << 12) | ((byte(i + 1) & 0x3Fu) << 6) | (byte(i + 2) & 0x3Fu);
    i += 3;
    return cp;
  }
  if ((b0 & 0xF8) == 0xF0 && is_continuation(s, i + 1) && is_continuation(s, i + 2) && is_continuation(s, i + 3)) {
    const std::uint32_t cp = ((b0 & 0x07u) << 18) | ((byte(i + 1) & 0x3Fu) << 12) | ((byte(i + 2) & 0x3Fu) << 6) |
                             (byte(i + 3) & 0x3Fu);
    i += 4;
    return cp;
  }
  i += 1;
  return b0;
}

// Appends one code point, UTF-8 encoded.
inline void append_codepoint(std::string& out, std::uint32_t cp) {
  if (cp < 0x80) {
    out.push_back(static_cast<char>(cp));
  } else if (cp < 0x800) {
    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp < 0x10000) {
    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}

}  // namespace p3a::core::utf8

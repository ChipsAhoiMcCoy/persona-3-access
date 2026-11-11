#pragma once

namespace p3a::core {
// Which of the mod's own records a row of the game's log shows. The game lists `count` entries oldest first (row 0 the
// oldest); the mod keeps `records` lines of this session, oldest first, the newest of which the game may not have
// logged yet (`skip` of them: a page still on screen). Row `row` is the record that many places from the newest one
// the game holds. Returns the record's index, or -1 for a row older than the mod's records (or one that is not there).
constexpr int log_record_index(int row, int count, int skip, int records) {
  int from_end = count - 1 - row + skip;   // 0 = the newest record
  return from_end < 0 || from_end >= records ? -1 : records - 1 - from_end;
}
}  // namespace p3a::core

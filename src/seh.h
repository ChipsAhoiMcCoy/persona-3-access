#pragma once
#include <windows.h>

namespace p3a {
// Runs f() under a structured-exception guard: false if it faulted (a stale or garbage game pointer). The
// function owns no objects, so MSVC allows __try here while callers keep their std::string locals.
template <typename F>
inline bool guarded(F&& f) {
  __try { f(); return true; }
  __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { return false; }
}
}  // namespace p3a

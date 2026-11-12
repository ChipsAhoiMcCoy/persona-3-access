#include "paths.h"

namespace p3a::paths {
HMODULE self() {
  HMODULE m = nullptr;
  GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&self, &m);
  return m;
}
std::wstring dir_w() {
  wchar_t path[MAX_PATH] = {};
  GetModuleFileNameW(self(), path, MAX_PATH);
  std::wstring s = path;
  size_t slash = s.find_last_of(L'\\');
  return slash == std::wstring::npos ? std::wstring() : s.substr(0, slash + 1);
}
std::string dir() {
  char path[MAX_PATH] = {};
  GetModuleFileNameA(self(), path, MAX_PATH);
  std::string s = path;
  size_t slash = s.find_last_of('\\');
  return slash == std::string::npos ? std::string() : s.substr(0, slash + 1);
}
}  // namespace p3a::paths

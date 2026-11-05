// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#include "log.h"
#include <windows.h>
#include <cstdio>
#include <share.h>
#include <mutex>

namespace p3a::log {
static FILE* g_file;
static std::mutex g_mu;
static std::wstring g_path;
static LineRing g_ring(2000);

std::wstring path() { return g_path; }
LineRing& ring() { return g_ring; }

void init() {
  wchar_t base[MAX_PATH];
  DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
  std::wstring dir = n ? std::wstring(base, n) + L"\\P3RAccess" : L"C:\\P3RAccess";
  CreateDirectoryW(dir.c_str(), nullptr);
  g_path = dir + L"\\p3access.log";
  // The log's first line names the process that wrote it. A new process keeps the old log: a player's session as
  // p3access.last-play.log, a dev session as p3access.prev.log (a dev launch wrote over the player's cafe visit,
  // 2026-10-31). A hot reload in the same process starts a dev log again but carries on a player's: the session before
  // the reload is what a report about it needs (a reload wiped a Social Link line the player asked about, 2026-11-01).
  bool dev = GetEnvironmentVariableW(L"P3A_NOFOCUS", nullptr, 0) > 0;
  DWORD pid = GetCurrentProcessId();
  bool carry_on = false;
  if (FILE* old = _wfopen(g_path.c_str(), L"r")) {
    char first[128] = {};
    unsigned long old_pid = 0;
    char mode[8] = {};
    bool known = fgets(first, sizeof first, old) && sscanf_s(first, "p3access log pid=%lu mode=%7s", &old_pid, mode, (unsigned)sizeof mode) == 2;
    fclose(old);
    if (!known || old_pid != pid) {
      std::wstring keep = dir + (known && std::string(mode) == "play" ? L"\\p3access.last-play.log" : L"\\p3access.prev.log");
      MoveFileExW(g_path.c_str(), keep.c_str(), MOVEFILE_REPLACE_EXISTING);
    } else {
      carry_on = !dev && std::string(mode) == "play";
    }
  }
  // Truncate, then append: the crash logger writes through its own handle, and a FILE* opened "w" keeps its own
  // position and wrote over the crash report (2026-10-27). "a" always writes at the end of the file.
  if (!carry_on)
    if (FILE* t = _wfopen(g_path.c_str(), L"w")) fclose(t);
  g_file = _wfsopen(g_path.c_str(), L"a", _SH_DENYNO);
  if (g_file) {
    if (carry_on) fprintf(g_file, "---- hot reload ----\n");
    else fprintf(g_file, "p3access log pid=%lu mode=%s\n", (unsigned long)pid, dev ? "dev" : "play");
    fflush(g_file);
  }
}

void write(std::string_view line) {
  SYSTEMTIME t; GetLocalTime(&t);
  std::string stamped = std::format("{:02}:{:02}:{:02}.{:03} {}", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, line);
  g_ring.push(stamped);
  std::lock_guard lk(g_mu);
  if (!g_file) return;
  fprintf(g_file, "%s\n", stamped.c_str());
  fflush(g_file);
}

std::string utf8(std::u16string_view s) {
  if (s.empty()) return {};
  int n = WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
  std::string out(n, '\0');
  WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)s.data(), (int)s.size(), out.data(), n, nullptr, nullptr);
  return out;
}
std::string utf8(const char16_t* s) { return s ? utf8(std::u16string_view(s)) : std::string(); }
}  // namespace p3a::log

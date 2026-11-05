// dsound.dll proxy. P3R.exe imports DSOUND.dll by ordinal, it is not a KnownDLL, so a dsound.dll next to the exe
// is loaded at process start, before any of the game's own code runs. We forward every export to
// the system copy and load the mod: P3A_DLL (environment, the dev loop) or P3RAccess\p3access.dll beside the exe.
// The proxy itself stays loaded for the life of the process; the mod DLL can be ejected and reloaded (hot reload).
#include <windows.h>

static HMODULE g_real;
static FARPROC g_fn[13];
static const char* const kNames[13] = {nullptr, "DirectSoundCreate", "DirectSoundEnumerateA", "DirectSoundEnumerateW",
                                       "DllCanUnloadNow", "DllGetClassObject", "DirectSoundCaptureCreate",
                                       "DirectSoundCaptureEnumerateA", "DirectSoundCaptureEnumerateW", "GetDeviceID",
                                       "DirectSoundFullDuplexCreate", "DirectSoundCreate8", "DirectSoundCaptureCreate8"};

static FARPROC real(int ord) {
  if (!g_real) {
    wchar_t sys[MAX_PATH];
    GetSystemDirectoryW(sys, MAX_PATH);
    lstrcatW(sys, L"\\dsound.dll");
    g_real = LoadLibraryW(sys);
  }
  if (!g_fn[ord] && g_real) g_fn[ord] = GetProcAddress(g_real, kNames[ord]);
  return g_fn[ord];
}

// Every dsound export returns HRESULT and takes only integer/pointer arguments (at most ten), so one generic
// forwarder per ordinal is exact under the x64 convention: extra stack slots are read but never used by the callee.
typedef HRESULT(WINAPI* Fwd)(void*, void*, void*, void*, void*, void*, void*, void*, void*, void*);
#define FORWARD(ORD, NAME)                                                                                   \
  extern "C" HRESULT WINAPI P_##NAME(void* a, void* b, void* c, void* d, void* e, void* f, void* g, void* h, \
                                     void* i, void* j) {                                                     \
    FARPROC p = real(ORD);                                                                                   \
    return p ? ((Fwd)p)(a, b, c, d, e, f, g, h, i, j) : E_FAIL;                                             \
  }
FORWARD(1, DirectSoundCreate)
FORWARD(2, DirectSoundEnumerateA)
FORWARD(3, DirectSoundEnumerateW)
FORWARD(5, DllGetClassObject)
FORWARD(6, DirectSoundCaptureCreate)
FORWARD(7, DirectSoundCaptureEnumerateA)
FORWARD(8, DirectSoundCaptureEnumerateW)
FORWARD(9, GetDeviceID)
FORWARD(10, DirectSoundFullDuplexCreate)
FORWARD(11, DirectSoundCreate8)
FORWARD(12, DirectSoundCaptureCreate8)
extern "C" HRESULT WINAPI P_DllCanUnloadNow() { return S_FALSE; }

static DWORD WINAPI load_mod(LPVOID) {
  wchar_t path[MAX_PATH * 2];
  DWORD n = GetEnvironmentVariableW(L"P3A_DLL", path, MAX_PATH * 2);
  if (n == 0 || n >= MAX_PATH * 2) {
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    wchar_t* slash = wcsrchr(path, L'\\');
    if (slash) slash[1] = 0;
    lstrcatW(path, L"P3RAccess\\p3access.dll");
  }
  if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) return 0;
  LoadLibraryW(path);
  return 0;
}

BOOL APIENTRY DllMain(HMODULE self, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(self);
    // Loading the mod straight from here would run its DllMain under our loader lock; a thread starts as soon as
    // the loader finishes the static imports, which is still long before the game creates its window.
    HANDLE t = CreateThread(nullptr, 0, load_mod, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
  }
  return TRUE;
}

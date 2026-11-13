// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
// Mute or unmute this process's audio session (WASAPI). Never touches the game's saved volume settings, but Windows
// remembers the mute for P3R.exe across launches (the mixer's per-app setting). A mute the mod sets leaves a marker
// file beside the log; a launch without P3A_MUTE lifts only that mute (release_dev_mute), never one the player set.
// Its own file: the COM audio headers clash with miniaudio's definitions in audio.cpp (CLSCTX_ALL).
#include "audio.h"
#include "log.h"
#include <windows.h>
#include <string>
#include <mmdeviceapi.h>
#include <audiopolicy.h>

namespace p3a::audio {
static std::wstring marker() {
  std::wstring p = log::path();
  return p.substr(0, p.find_last_of(L'\\') + 1) + L"audio-muted-by-mod";
}

bool mute_process(bool mute) {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  bool uninit = SUCCEEDED(hr);
  IMMDeviceEnumerator* en = nullptr;
  IMMDevice* dev = nullptr;
  IAudioSessionManager* mgr = nullptr;
  ISimpleAudioVolume* vol = nullptr;
  bool ok = false;
  BOOL was = FALSE;
  if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&en)) &&
      SUCCEEDED(en->GetDefaultAudioEndpoint(eRender, eConsole, &dev)) &&
      SUCCEEDED(dev->Activate(__uuidof(IAudioSessionManager), CLSCTX_ALL, nullptr, (void**)&mgr)) &&
      SUCCEEDED(mgr->GetSimpleAudioVolume(nullptr, FALSE, &vol))) {
    vol->GetMute(&was);
    ok = SUCCEEDED(vol->SetMute(mute ? TRUE : FALSE, nullptr));
  }
  if (vol) vol->Release();
  if (mgr) mgr->Release();
  if (dev) dev->Release();
  if (en) en->Release();
  if (uninit) CoUninitialize();
  log::writef("audio: process session mute={} (was {}) -> {}", mute, was != FALSE, ok ? "ok" : "failed");
  if (ok && mute) {
    if (HANDLE f = CreateFileW(marker().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr); f != INVALID_HANDLE_VALUE) CloseHandle(f);
  } else if (ok) {
    DeleteFileW(marker().c_str());
  }
  return ok;
}

// The player muted P3R in Windows' mixer: theirs to keep. Only the mod's own mute (a dev launch's) is lifted.
void release_dev_mute() {
  if (GetFileAttributesW(marker().c_str()) == INVALID_FILE_ATTRIBUTES) return;
  log::write("audio: a dev launch left the game muted; unmuting");
  mute_process(false);
}
}  // namespace p3a::audio

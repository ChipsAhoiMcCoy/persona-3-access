// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
// Mute or unmute this process's audio session (WASAPI). Never touches the game's saved volume settings, but Windows
// remembers the mute for P3R.exe across launches (the mixer's per-app setting): every launch sets it (dllmain).
// Its own file: the COM audio headers clash with miniaudio's definitions in audio.cpp (CLSCTX_ALL).
#include "audio.h"
#include "log.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>

namespace p3a::audio {
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
  return ok;
}
}  // namespace p3a::audio

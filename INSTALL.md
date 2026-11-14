# Installing Persona 3 Access

You need the Steam version of Persona 3 Reload on Windows, already installed. The mod includes no game files and doesn't change any.

## Install

1. Download `persona-3-access-0.2.0.zip` from the Releases page: https://github.com/ChipsAhoiMcCoy/persona-3-access/releases
2. Extract it anywhere. You get `dsound.dll`, a `P3RAccess` folder, `README.txt` and `INSTALL.txt`.
3. Find the game's folder. In a default Steam install it is:

   `C:\Program Files (x86)\Steam\steamapps\common\P3R\P3R\Binaries\Win64`

   Elsewhere: in your Steam library, open Persona 3 Reload's properties, then Installed Files, then Browse, then go into `P3R`, `Binaries`, `Win64`. It is the folder with `P3R.exe` in it.
4. Copy `dsound.dll` and the `P3RAccess` folder into it. These two paths should now exist:

   - `Win64\dsound.dll`
   - `Win64\P3RAccess\p3access.dll`

5. Start your screen reader, then the game from Steam. After a while you will hear "Persona 3 Access loaded".

## Update

Quit the game and copy the new `dsound.dll` and `P3RAccess` folder over the old ones. Your settings are kept separately.

## Uninstall

Delete `Win64\dsound.dll` and `Win64\P3RAccess`. To also remove settings and logs, delete `%LOCALAPPDATA%\P3RAccess` (press Windows+R, paste that path, press Enter). Your saves are not touched.

## Troubleshooting

### No speech at all, and no "Persona 3 Access loaded"

- Check the two paths above. The usual mistake is `dsound.dll` in the wrong folder (the top `P3R` folder, or an extra folder left from extracting).
- Check for `%LOCALAPPDATA%\P3RAccess\p3access.log`. If it was not created or updated when you started the game, the mod did not load.
- Antivirus can quarantine `dsound.dll` or `p3access.dll`, because the mod loads itself into the game. Check your antivirus's history; only you can decide whether to allow them. You can also build the mod yourself from source.
- Move `dsound.dll` out and check the game runs without the mod. If it doesn't, the problem is elsewhere.

### Windows speech instead of your screen reader

- Start your screen reader before the game. The mod picks its speech at startup and does not switch later.
- Run your screen reader and the game at the same permission level: if only one runs as administrator, they may not be able to talk.
- The log line starting `speech: using backend` says what was chosen. Other `speech:` lines saying something failed, or hearing "Persona 3 Access loaded, no speech backend", mean no speech could start: please report it.

### The mod says it could not find the game engine

The game has probably been updated. Please report it with the log. The mod stays quiet and the game should carry on.

### Speech works, but no wall tones, sonar or narrator

- Check the mod's volumes in its settings (F2, or both triggers and View).
- Check these exist:
  - `Win64\P3RAccess\assets\audio\walltones\2\north.wav`
  - `Win64\P3RAccess\assets\audio\sonar`
  - `Win64\P3RAccess\assets\ad`
  - `Win64\P3RAccess\phonon.dll` (needed only for the "3D for headphones" sound placement; without it, sounds use the default stereo placement)

### The game crashes

The crash is recorded in the log. Please report it. The crashed session's log is `p3access.log` until you start the game again, then `p3access.last-play.log`.

## Finding the log

The log is `%LOCALAPPDATA%\P3RAccess\p3access.log` (press Windows+R, paste `%LOCALAPPDATA%\P3RAccess`, press Enter). Settings are in the same folder, in `settings.ini`. The log holds what the mod spoke, including story text, and may show your Windows user name in a path, so look it over before sharing it.

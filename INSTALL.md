# Installing Arcana

You need the Steam version of Persona 3 Reload on Windows, already installed. The mod includes no game files and doesn't change any.

## Install

1. Download the newest `arcana-<version>.zip` from the Releases page: https://github.com/ChipsAhoiMcCoy/arcana/releases
2. Extract it anywhere. You get `dsound.dll`, an `Arcana` folder, `README.txt` and `INSTALL.txt`.
3. Find the game's folder. In a default Steam install it is:

   `C:\Program Files (x86)\Steam\steamapps\common\P3R\P3R\Binaries\Win64`

   Elsewhere: in your Steam library, open Persona 3 Reload's properties, then Installed Files, then Browse, then go into `P3R`, `Binaries`, `Win64`. It is the folder with `P3R.exe` in it.
4. Copy `dsound.dll` and the `Arcana` folder into it. These two paths should now exist:

   - `Win64\dsound.dll`
   - `Win64\Arcana\arcana.dll`

5. Start your screen reader, then the game from Steam. After a while you will hear "Arcana loaded".

## Update

Quit the game and copy the new `dsound.dll` and `Arcana` folder over the old ones. Your settings are kept separately.

Coming from a version named Persona 3 Access (0.2.0 or older)? Delete the old `Win64\P3RAccess` folder; the new `Arcana` folder replaces it. Your settings move over by themselves the first time the game starts.

## Uninstall

Delete `Win64\dsound.dll` and `Win64\Arcana`. To also remove settings and logs, delete `%LOCALAPPDATA%\Arcana` (press Windows+R, paste that path, press Enter). Your saves are not touched.

## Troubleshooting

### No speech at all, and no "Arcana loaded"

- Check the two paths above. The usual mistake is `dsound.dll` in the wrong folder (the top `P3R` folder, or an extra folder left from extracting).
- Check for `%LOCALAPPDATA%\Arcana\arcana.log`. If it was not created or updated when you started the game, the mod did not load.
- Antivirus can quarantine `dsound.dll` or `arcana.dll`, because the mod loads itself into the game. Check your antivirus's history; only you can decide whether to allow them. You can also build the mod yourself from source.
- Move `dsound.dll` out and check the game runs without the mod. If it doesn't, the problem is elsewhere.

### Windows speech instead of your screen reader

- Start your screen reader before the game. The mod picks its speech at startup and does not switch later.
- Run your screen reader and the game at the same permission level: if only one runs as administrator, they may not be able to talk.
- The log line starting `speech: using backend` says what was chosen. Other `speech:` lines saying something failed, or hearing "Arcana loaded, no speech backend", mean no speech could start: please report it.

### The mod says it could not find the game engine

The game has probably been updated. Please report it with the log. The mod stays quiet and the game should carry on.

### Speech works, but no wall tones, sonar or narrator

- Check the mod's volumes in its settings (F2, or both triggers and View).
- Check these exist:
  - `Win64\Arcana\assets\audio\walltones\2\north.wav`
  - `Win64\Arcana\assets\audio\sonar`
  - `Win64\Arcana\assets\ad`
  - `Win64\Arcana\phonon.dll` (needed only for the "3D for headphones" sound placement; without it, sounds use the default stereo placement)

### The game crashes

The crash is recorded in the log. Please report it. The crashed session's log is `arcana.log` until you start the game again, then `arcana.last-play.log`.

## Finding the log

The log is `%LOCALAPPDATA%\Arcana\arcana.log` (press Windows+R, paste `%LOCALAPPDATA%\Arcana`, press Enter). Settings are in the same folder, in `settings.ini`. The log holds what the mod spoke, including story text, and may show your Windows user name in a path, so look it over before sharing it.

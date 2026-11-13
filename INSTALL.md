# Installing Persona 3 Access

## Before you start

You need the Steam version of Persona 3 Reload on Windows, already installed. Start your screen reader before the game. The mod includes no game files and doesn't change any.

## Install

1. Download `persona-3-access-0.2.0.zip` from the Releases page: https://github.com/ChipsAhoiMcCoy/persona-3-access/releases
2. Extract the zip anywhere. You will get `dsound.dll`, a folder called `P3RAccess`, and two text files, `README.txt` and `INSTALL.txt`.
3. Find the game's folder. In a default Steam install it is:

   `C:\Program Files (x86)\Steam\steamapps\common\P3R\P3R\Binaries\Win64`

   If you installed the game elsewhere: in your Steam library, open the properties of Persona 3 Reload, then Installed Files, then Browse. Then go into `P3R`, `Binaries`, `Win64`. You are in the right place when you see `P3R.exe`.
4. Copy `dsound.dll` and the `P3RAccess` folder into that folder.

When you are done, these two paths should exist, where `Win64` is the folder above:

- `Win64\dsound.dll`
- `Win64\P3RAccess\p3access.dll`

5. Start your screen reader, then start the game from Steam as usual. You will hear "Persona 3 Access loaded". It can take a little while.

## Update

Quit the game, then copy the new `dsound.dll` and `P3RAccess` folder over the old ones, replacing the files. Your settings are kept separately.

## Uninstall

Delete `Win64\dsound.dll` and the `Win64\P3RAccess` folder. To also remove settings and logs, delete `%LOCALAPPDATA%\P3RAccess`. To go there, press Windows+R, paste that path, and press Enter. Your game saves are not touched.

## Troubleshooting

### No speech at all, and no "Persona 3 Access loaded"

- Check the two paths above. The most common mistake is `dsound.dll` being in the wrong folder, such as the top `P3R` folder or a subfolder, or the files being left inside an extra folder from extracting.
- Check whether the log exists: `%LOCALAPPDATA%\P3RAccess\p3access.log`. If that folder or file was not created or updated when you started the game, the mod did not load.
- Antivirus software can quarantine `dsound.dll` or `p3access.dll`, because a mod like this loads itself into a game. Check your antivirus's history. Only you can decide whether to allow them. You can build the mod yourself from the source if you prefer.
- Check the game runs without the mod, by moving `dsound.dll` out. If it doesn't, the problem is elsewhere.

### Screen reader not detected, or Windows speech instead of your screen reader

- Start your screen reader before the game. The mod picks its speech at startup and does not switch later.
- Run your screen reader and the game at the same permission level. If one runs as administrator and the other does not, they may not be able to talk to each other.
- The log says what was chosen. Open `%LOCALAPPDATA%\P3RAccess\p3access.log` and look for the line starting `speech: using backend`. Lines starting `speech:` that say failed or no usable backend mean no speech could start. If you hear "Persona 3 Access loaded, no speech backend" please report it.

### The mod says it could not find the game engine

The game has likely been updated, or something stopped it from starting properly. Please report it and include the log. The mod keeps quiet after this and the game itself should carry on.

### Speech works, but no wall tones, sonar or narrator

- Check the mod's volume settings with F2, or both triggers and View.
- Check these exist:
  - `Win64\P3RAccess\assets\audio\walltones\2\north.wav`
  - `Win64\P3RAccess\assets\ad`
  - `Win64\P3RAccess\phonon.dll`
- Without `phonon.dll` the sounds still play, but only pan left and right.

### The game crashes

The crash is recorded in the log. Please report it. The log for the session that crashed is `p3access.log` until you start the game again, after which it is `p3access.last-play.log`.

## Finding the log

The log is `%LOCALAPPDATA%\P3RAccess\p3access.log`. To open the folder, press Windows+R, paste `%LOCALAPPDATA%\P3RAccess`, and press Enter. It holds what the mod spoke, which includes story text, and may show your Windows user name in a path, so look it over before sharing it. See "Reporting bugs" in the README.

Settings are in the same folder, in `settings.ini`.

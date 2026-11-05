# Contributing

Bug reports are the most useful contribution at this stage. See "Reporting bugs" in the README. Accessibility feedback from blind players is very welcome, including "this is confusing" or "this says too much".

## Building

You need:

- Windows 10 or 11.
- Visual Studio 2022 or its Build Tools, with the "Desktop development with C++" tools for x64.
- CMake 3.24 or later.
- Ninja. Visual Studio's own copy is found automatically if you installed its CMake tools. Otherwise put Ninja on your PATH.
- Python 3, for the packaging script and the dev client. The standard library is enough to package.

All other dependencies are already in the `third_party` folder.

To build everything, from the repository root in a Command Prompt:

```
tools\build.cmd
```

This builds into `build\ninja`: `p3access.dll`, `dsound.dll` and the unit tests.

## Running the tests

The tests need no game:

```
build\ninja\p3core_tests.exe
```

## Building the player zip

```
python tools\package.py
```

This makes a Release build, runs the unit tests and writes `dist\persona-3-access-<version>.zip` and a `.sha256` file beside it. It never touches your game folder. Zip contents are checked against what the code loads.

## Trying your build in the game

You need your own copy of the game. Install your build the way INSTALL.md describes, from the zip, or copy `build\ninja\dsound.dll` and the rest of the mod's files by hand.

`tools/p3.py` is a development client. It builds, copies only the proxy `dsound.dll` into the game folder, and starts the game with a local debugging server turned on. The server is only on when the `P3A_PORT` environment variable is set, which a normal Steam launch does not do, and it only listens on your own computer. Run `python tools/p3.py` with no arguments, or read the top of the file, for its commands. `tools/days.sh`, `tools/crawl.sh` and `tools/fight.sh` are helpers built on it, and `tools/stack.py` takes a thread's stack.

## How the code is laid out

- `src/proxy`: the `dsound.dll` the game loads, which then loads the mod.
- `src/ue`: reads the game through Unreal Engine's own reflection.
- `src/screens`: one file per screen or menu.
- `src/core`: code with no game or Windows dependency, with unit tests in `tests`. It includes the navigator, which owns menu focus and announcements, and `strings.h`, where all of the mod's wording lives.
- `src/world.cpp`, `src/poi.cpp`, `src/nav.cpp`, `src/walltones.cpp`: getting around.
- `src/msg.cpp` and neighbours: message windows.
- `assets`: the mod's own sounds and audio description clips.
- `data/ad`: the audio description scripts. The tool that voices them is not in this repository, so `assets/ad` holds their finished output.

## Rules for changes

- Speech doesn't interrupt by default. Focus moves and screen entry do.
- Key hints are never spoken by themselves. They go behind F1.
- Every mod key has a controller equivalent, and every player-facing key or setting goes in the README in the same change.
- All mod wording goes in `src/core/strings.h`. Game text is read verbatim.
- No delays, debounces or retries to fix a double announcement or a stale read. Find the state the game is really in.
- Never use a low-level keyboard hook. It breaks NVDA.
- Never steal focus from the player.

## What this repository must not contain

Please don't add game files or anything extracted from them: textures, audio, movies, scripts, message files, data tables, the executable, saves, or encryption keys. Please also don't paste decompiled game code. Short references such as function names and field offsets are fine.

## License

By contributing, you agree your contribution is under this project's zlib license. Mark any file you take from elsewhere, and add its license to `LICENSES` and `THIRD_PARTY_NOTICES.md`.

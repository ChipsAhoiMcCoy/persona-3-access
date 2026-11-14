# Contributing

Bug reports are the most useful contribution right now (see "Reporting bugs" in the README). Feedback from blind players is very welcome, including "this is confusing" or "this says too much".

## Building

You need:

- Windows 10 or 11.
- Visual Studio 2022 or its Build Tools, with "Desktop development with C++" for x64.
- CMake 3.24 or later.
- Ninja (Visual Studio's copy is found automatically if its CMake tools are installed; otherwise put Ninja on your PATH).
- Python 3 (standard library only) for packaging and the dev client.

All other dependencies are in `third_party`. From the repository root in a Command Prompt:

```
tools\build.cmd
```

This builds `p3access.dll`, `dsound.dll` and the unit tests into `build\ninja`. The tests need no game:

```
build\ninja\p3core_tests.exe
```

## Building the player zip

```
python tools\package.py
```

This makes a Release build, runs the tests, and writes `dist\persona-3-access-<version>.zip` with a `.sha256` beside it, checking the zip against what the code loads. It never touches your game folder.

## Trying your build in the game

Install the zip as INSTALL.md describes, or use `tools/p3.py`, the development client: it builds, copies the proxy `dsound.dll` into the game folder, and starts the game with a local debugging server. The server only runs when `P3A_PORT` is set (a normal Steam launch doesn't set it) and only listens on 127.0.0.1. Its commands are listed at the top of the file. `tools/jump.py` and `tools/scenarios.py` reach test situations through it; `days.sh`, `crawl.sh` and `fight.sh` are helpers on top of it, and `stack.py` takes a thread's stack.

## How the code is laid out

- `src/proxy`: the `dsound.dll` the game loads, which then loads the mod.
- `src/ue`: reads the game through Unreal Engine's own reflection.
- `src/screens`: one file per screen or menu.
- `src/core`: code with no game or Windows dependency, unit-tested in `tests`. It includes the navigator, which owns menu focus and announcements, and `strings.h`, which holds all of the mod's wording.
- `src/world.cpp`, `src/poi*.cpp`, `src/nav.cpp`, `src/walltones.cpp`, `src/sonar.cpp`, `src/shadow.cpp`, `src/topdown.cpp`: getting around.
- `src/msg.cpp` and neighbours: message windows.
- `assets`: sounds (see THIRD_PARTY_NOTICES.md for those from Grim Dark and Dark Vision) and the audio description clips.
- `data/ad`: the audio description scripts. The tool that voices them is not in this repository; `assets/ad` holds its output.
- `tools/sfx`: designs the mod's own sonar sounds.

## Rules for changes

- Speech doesn't interrupt by default. Focus moves and screen entry do.
- Key hints are never spoken by themselves. They go behind F1.
- Every mod key has a controller equivalent, and every player-facing key or setting goes in the README in the same change.
- All mod wording goes in `src/core/strings.h`. Game text is read verbatim.
- No delays, debounces or retries to fix a double announcement or a stale read. Find the state the game is really in.
- Never use a low-level keyboard hook. It breaks NVDA.
- Never steal focus from the player.

## What this repository must not contain

No game files or anything extracted from them (textures, audio, movies, scripts, message files, data tables, the executable, saves, encryption keys), and no decompiled game code. Short references such as function names and field offsets are fine.

## License

By contributing, you agree your contribution is under this project's zlib license. Mark any file you take from elsewhere, and add its license to `LICENSES` and `THIRD_PARTY_NOTICES.md`.

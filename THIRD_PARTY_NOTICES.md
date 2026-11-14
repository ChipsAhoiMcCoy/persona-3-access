# Third-party notices

Arcana uses the work of other people. This file says what each piece is, whether it ships in the player zip, its license, and where the license text is. In the player zip the license texts are in `Arcana\licenses`. In the repository they are in the `LICENSES` folder and beside each component in `third_party`.

## Code derived from Grim Dark (GD Access)

Grim Dark, also called GD Access, is a screen-reader mod for Grim Dawn by Austin Hicks: https://github.com/ahicks92/grimdark

- Copyright (c) 2026 Austin Hicks. zlib license. The full text is unchanged in `LICENSES/grim-dark-zlib.txt`.
- Ships in the player zip: yes, compiled into `arcana.dll`.
- These are altered versions and not the original software. Arcana was changed to work with a different game. Each file below carries a comment at its top saying it is altered from Grim Dark.
- Grim Dark's navigator and related parts were ported from Dark Vision, below.

Files derived from Grim Dark:

- The navigator and its parts in `src/core`: `navigator`, `graph_types`, `graph_builder`, `graph_announcer`, `key_graph`, `control_id`, `message_builder`, `ui_screen`, and the sonar's `sonar_field`.
- Their tests in `tests`: `navigator_tests`, `graph_builder_tests`, `graph_announcer_tests`, `key_graph_tests`, `message_builder_tests`, `main`.
- Support code in `src`: `crash`, `log`, `ring`, `speech`, `audio`, `audio_mute`, `walltones`, `hooks`, `dllmain`, `devserver`.
- `tools/vsdev.cmd`.

## Code and sounds from Dark Vision

Dark Vision, also called WrathAccess or wotr-access in comments, is a screen-reader mod for Pathfinder: Wrath of the Righteous by Bradley Renshaw: https://github.com/bradjrenshaw/darkvision-wotr

- Copyright (c) 2026 Bradley Renshaw. zlib license. The full text is unchanged in `LICENSES/dark-vision-zlib.txt`.
- Ships in the player zip: yes, through the code ported to Grim Dark and from there to this project, and the wall tones below.
- Our versions are altered, and the marks at the top of the files say so.
- Ported directly from Dark Vision: the stereo placement model (`src/core/spatializer`, from `Spatializer.cs`).

## Wall tones

The four sounds in `assets/audio/walltones/2`, in the player zip, are byte-for-byte the same as the wall tone files in Dark Vision (assets/audio/walltones/2), by Bradley Renshaw, under its zlib license. Grim Dark copied them from there, and we copied them from Grim Dark. Dark Vision's records do not say how the sounds themselves were made.

## Sonar sounds from Grim Dark

The five sounds in `assets/audio/sonar` and `assets/audio/signals`, in the player zip, are Grim Dark's sonar sounds (its dungeon entrance, interactable, ruined shrine, loot and enemy sounds), by Austin Hicks, under its zlib license. The door, person and shop sounds are made by the mod itself.

## Prism

Prism is the speech library that talks to your screen reader, by Ethin Probst and contributors: https://github.com/ethindp/prism

- Version 0.18.1, the unmodified prebuilt SDK from the project's release page.
- Ships in the player zip: yes, as `prism.dll`.
- License: Mozilla Public License 2.0.
- Source code for this exact version, as MPL-2.0 requires us to tell you: https://github.com/ethindp/prism/tree/v0.18.1
- Prism's own NOTICE and license folder ship unchanged. In the zip they are in `Arcana\licenses\prism`. In the repository they are in `third_party/prism-bin/prism-sdk-v0.18.1`.

That folder covers what Prism itself uses: simdutf (Apache-2.0), Moderncom (MIT), dr_wav (public domain as Prism chose it), Djinni (Apache-2.0), concurrentqueue (simplified BSD), fmt (MIT), Highway, and NVGT (zlib style).

About the LGPL file in that folder: Prism includes NVDA controller client RPC definitions that were originally LGPL-2.1. Prism's NOTICE states the Prism project received permission to license those files, and what is generated from them, under MPL-2.0, and that the LGPL file remains for attribution. It is included here unchanged for the same reason. Arcana also loads `prism.dll` as a separate file that you can replace.

## Steam Audio

Steam Audio provides the 3D sound for headphones, by Valve Corporation: https://github.com/ValveSoftware/steam-audio

- Version 4.8.1, unmodified.
- Ships in the player zip: yes, as `phonon.dll`.
- License: Apache License 2.0, in `third_party/steamaudio/steamaudio-4.8.1/LICENSE.md`.
- `phonon.dll` contains other people's work under their own terms, including Intel IPP, Intel Embree, FFTS, PFFFT, MySOFA, AMD RadeonRays, AMD TrueAudio Next, the CIPIC HRTF Database and Google's Spherical Harmonics library. Those notices are in `third_party/steamaudio/steamaudio-4.8.1/THIRDPARTY.md`. They ship unchanged, and in the zip they are in `Arcana\licenses\steamaudio`.

## Microsoft Detours

Detours is the function hooking library, by Microsoft: https://github.com/microsoft/Detours

- Ships in the player zip: yes, compiled into `arcana.dll`.
- MIT license. Copyright (c) Microsoft Corporation. The text is in `third_party/Detours/LICENSE`, and in the zip in `Arcana\licenses\detours`.

## miniaudio

miniaudio plays the mod's sounds, by David Reid: https://github.com/mackron/miniaudio

- Version 0.11.25.
- Ships in the player zip: yes, compiled into `arcana.dll`.
- Licensed as a choice of public domain (Unlicense) or MIT No Attribution. Neither requires this credit, and we give it anyway. The statement from the header is in `LICENSES/miniaudio.txt`.

## doctest

doctest is the unit test framework, by Viktor Kirilov: https://github.com/doctest/doctest

- Ships in the player zip: no. It is only used to build the tests. Its license text is included in the zip anyway, with the others.
- MIT license. Copyright (c) 2016-2023 Viktor Kirilov. The header has no license text of its own and points to the project's file, copied here as `LICENSES/doctest-MIT.txt`.
- Its header says parts are taken from Catch2 and from lest, which use the Boost Software License 1.0: `LICENSES/boost-1.0-catch2-lest.txt`.

## Kokoro narration

The narrator in `assets/ad`, in the player zip, was generated in advance with the Kokoro text-to-speech model, voice af_heart. The model and the speech tools do not ship, only the finished MP3 clips and their text.

- Kokoro-82M by hexgrad: https://huggingface.co/hexgrad/Kokoro-82M. Its model card gives the license for the weights as Apache-2.0. A copy of that license is in `LICENSES/apache-2.0-kokoro.txt`. The model repository has no separate license file.
- Neither the model card nor its voice list restricts distributing audio made with the model. The voice list marks CC BY sources only for some Japanese and French voices, and not for af_heart.
- kokoro-onnx by thewh1teagle ran the model: https://github.com/thewh1teagle/kokoro-onnx. MIT license, in `LICENSES/kokoro-onnx-MIT.txt`.
- kokoro-onnx uses the espeak-ng phonemizer while generating. It is not part of anything we distribute.

The description text in `assets/ad` and `data/ad` is original writing for this mod.

## Research credits

These projects were used as research references. None of their code is included. The mod's source comments name them where a fact came from them.

- rirurin's p3rpc mods, which documented native layouts and signatures of Persona 3 Reload: p3rpc.nativetypes, p3rpc.slplus, p3rpc.classconstructor and p3rpc.commonmodutils (LGPL-3.0). https://github.com/rirurin
- AnimatedSwine37's p3rpc.essentials and p3rpc.socialStatTracker (GPL-3.0). https://github.com/AnimatedSwine37
- Malurth's p3rpc.socialStatTrackerPlus (GPL-3.0). https://github.com/Malurth/p3rpc.socialStatTrackerPlus
- Dumper-7 by Encryqed: https://github.com/Encryqed/Dumper-7
- Atlus Script Tools by TGE (GPL-3.0): https://github.com/tge-was-taken/Atlus-Script-Tools

Eleven of the thirteen short byte patterns in our source that locate game functions (in `src/bmdpages.cpp`, `src/msg.cpp`, `src/stats.cpp`, `src/textcap.cpp` and `src/ue/ue.cpp`) are the same as patterns published in those mods. They describe the game's own machine code. No other text is shared with those projects: a check for shared lines of 40 or more characters found none.

## The game

This project includes no game files. Persona 3 Reload and related names and characters belong to their owners. See the README.

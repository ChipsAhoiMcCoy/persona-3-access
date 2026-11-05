"""Build the player zip from this checkout, reproducibly: a Release build, then a zip with sorted names and fixed dates.

    python tools/package.py [--version 0.1.0-alpha.1] [--nobuild] [--allow-no-license]

Output: dist/persona-3-access-<version>.zip and a .sha256 file beside it. The script never touches a game folder.

The same commit built in the same folder gives the same zip, byte for byte. Another folder changes p3access.dll
slightly: MSVC derives the names of anonymous namespaces from each source file's path.

Zip layout (the player copies dsound.dll and P3RAccess into P3R\\Binaries\\Win64):

    dsound.dll                       the proxy the game loads at startup (src/proxy)
    README.txt, INSTALL.txt          for reading before installing
    P3RAccess/
      p3access.dll, prism.dll, phonon.dll
      assets/ad/                     audio description cue lists and clips (src/ad.cpp)
      assets/audio/walltones/2/      the wall tones (src/walltones.cpp)
      docs/                          the same readme and install steps, known issues, changelog, credits
      licenses/                      every license and notice

Needs Visual Studio 2022's C++ x64 tools, CMake and Ninja (see CONTRIBUTING.md). Python standard library only.
"""
import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "build", "release")
DIST = os.path.join(ROOT, "dist")
PRISM = "third_party/prism-bin/prism-sdk-v0.18.1"
STEAMAUDIO = "third_party/steamaudio/steamaudio-4.8.1"
STAMP = (2026, 1, 1, 0, 0, 0)   # every entry's date, so two builds of one tree give one zip
TONES = ("north.wav", "east.wav", "south.wav", "west.wav")
FORBIDDEN = re.compile(r"\.(map|pdb|lib|exp|ilk|log|ini|obj|sav|pak|uasset|usm)$|live\d+\.dll$", re.I)


def build():
    vsdev = os.path.join(ROOT, "tools", "vsdev.cmd")
    subprocess.check_call([vsdev, "cmake", "-S", ROOT, "-B", BUILD, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release",
                           "-DP3A_REPRODUCIBLE=ON"])
    subprocess.check_call([vsdev, "cmake", "--build", BUILD, "--target", "p3access", "dsound_proxy", "p3core_tests"])
    subprocess.check_call([os.path.join(BUILD, "p3core_tests.exe")])


def as_text(path):
    """A Markdown file as plain text for readers without a Markdown view: no markers, CRLF line ends."""
    s = open(path, encoding="utf-8", newline="").read().replace("\r\n", "\n")
    s = re.sub(r"^#+ +", "", s, flags=re.M)
    s = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r"\1 (\2)", s)
    s = s.replace("`", "").replace("**", "")
    return s.replace("\n", "\r\n").encode("utf-8")


def repo(path):
    with open(os.path.join(ROOT, *path.split("/")), "rb") as f:
        return f.read()


def collect(allow_no_license):
    """Published path -> bytes."""
    out = {"dsound.dll": open(os.path.join(BUILD, "dsound.dll"), "rb").read()}
    for name in ("p3access.dll", "prism.dll", "phonon.dll"):
        out["P3RAccess/" + name] = open(os.path.join(BUILD, name), "rb").read()

    for d, _, files in os.walk(os.path.join(ROOT, "assets", "ad")):
        for f in files:
            full = os.path.join(d, f)
            rel = os.path.relpath(full, ROOT).replace(os.sep, "/")
            out["P3RAccess/" + rel] = open(full, "rb").read()
    for t in TONES:
        out[f"P3RAccess/assets/audio/walltones/2/{t}"] = repo(f"assets/audio/walltones/2/{t}")

    readme, install = as_text(os.path.join(ROOT, "README.md")), as_text(os.path.join(ROOT, "INSTALL.md"))
    out["README.txt"], out["INSTALL.txt"] = readme, install
    out["P3RAccess/docs/README.txt"], out["P3RAccess/docs/INSTALL.txt"] = readme, install
    for name in ("KNOWN_ISSUES", "CHANGELOG", "CREDITS"):
        out[f"P3RAccess/docs/{name}.txt"] = as_text(os.path.join(ROOT, name + ".md"))

    out["P3RAccess/licenses/THIRD_PARTY_NOTICES.txt"] = as_text(os.path.join(ROOT, "THIRD_PARTY_NOTICES.md"))
    if os.path.exists(os.path.join(ROOT, "LICENSE")):
        out["P3RAccess/licenses/LICENSE.txt"] = repo("LICENSE")
    elif not allow_no_license:
        sys.exit("no LICENSE in the repository root (--allow-no-license for a trial zip)")
    for f in sorted(os.listdir(os.path.join(ROOT, "LICENSES"))):
        out[f"P3RAccess/licenses/{f}"] = repo(f"LICENSES/{f}")
    out["P3RAccess/licenses/prism/NOTICE.txt"] = repo(f"{PRISM}/NOTICE")
    lic = os.path.join(ROOT, *PRISM.split("/"), "LICENSES")
    for d, _, files in os.walk(lic):
        for f in files:
            full = os.path.join(d, f)
            rel = os.path.relpath(full, lic).replace(os.sep, "/")
            out[f"P3RAccess/licenses/prism/{rel}"] = open(full, "rb").read()
    out["P3RAccess/licenses/steamaudio/LICENSE.md"] = repo(f"{STEAMAUDIO}/LICENSE.md")
    out["P3RAccess/licenses/steamaudio/THIRDPARTY.md"] = repo(f"{STEAMAUDIO}/THIRDPARTY.md")
    out["P3RAccess/licenses/detours/LICENSE.txt"] = repo("third_party/Detours/LICENSE")
    return out


def check(files):
    """What the code loads must be there (src/proxy, src/speech.cpp, src/hrtf.cpp, src/walltones.cpp, src/ad.cpp)."""
    problems = [n for n in files if FORBIDDEN.search(n)]
    for need in ["dsound.dll", "P3RAccess/p3access.dll", "P3RAccess/prism.dll", "P3RAccess/phonon.dll"] + \
            [f"P3RAccess/assets/audio/walltones/2/{t}" for t in TONES]:
        if need not in files:
            problems.append("missing " + need)
    movies = clips = 0
    for name, data in files.items():
        m = re.fullmatch(r"P3RAccess/assets/ad/([^/]+)\.txt", name)
        if not m:
            continue
        movies += 1
        for line in data.decode("utf-8").splitlines():
            if not line or line.startswith("#"):
                continue
            clip = line.split("\t")[2]
            clips += 1
            if f"P3RAccess/assets/ad/{m.group(1)}/{clip}" not in files:
                problems.append(f"missing clip {m.group(1)}/{clip}")
    if problems:
        sys.exit("package check failed:\n  " + "\n  ".join(problems))
    print(f"check passed: {len(files)} files, {movies} described movies, {clips} clips")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--version", default="0.1.0-alpha.1")
    ap.add_argument("--nobuild", action="store_true", help="use the existing build/release")
    ap.add_argument("--allow-no-license", action="store_true")
    a = ap.parse_args()
    if not a.nobuild:
        build()
    files = collect(a.allow_no_license)
    check(files)
    os.makedirs(DIST, exist_ok=True)
    zpath = os.path.join(DIST, f"persona-3-access-{a.version}.zip")
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name in sorted(files):
            info = zipfile.ZipInfo(name, STAMP)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.create_system = 0
            z.writestr(info, files[name])
    digest = hashlib.sha256(open(zpath, "rb").read()).hexdigest()
    with open(zpath + ".sha256", "w", newline="\n") as f:
        f.write(f"{digest}  {os.path.basename(zpath)}\n")
    print(zpath)
    print("sha256", digest)


if __name__ == "__main__":
    main()

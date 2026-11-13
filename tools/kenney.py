"""Download Kenney's audio packs (https://kenney.nl/assets, all CC0) into third_party/kenney/<pack>/, to audition.

  python tools/kenney.py            every pack in PACKS (skips one already there)
  python tools/kenney.py ui-audio   just these
  python tools/kenney.py --force    download again

third_party/kenney/ is git-ignored: the packs are for choosing from. A sound the mod ships is copied into
assets/audio/ and listed in THIRD_PARTY_NOTICES.md.

The site hands out a signed link per download: the pack's page carries an id (data-download-id), and
/download-link/<id> answers {"url": ...} for the same session's cookies.
"""
import http.cookiejar
import io
import json
import os
import re
import sys
import urllib.request
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "third_party", "kenney")
PACKS = ["interface-sounds", "ui-audio", "impact-sounds", "digital-audio", "sci-fi-sounds", "rpg-audio",
         "casino-audio", "music-jingles"]
UA = "Mozilla/5.0 (P3RAccess asset fetch)"


def opener():
    o = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
    o.addheaders = [("User-Agent", UA)]
    return o


def fetch(pack, op):
    page_url = f"https://kenney.nl/assets/{pack}"
    page = op.open(page_url).read().decode("utf-8", "replace")
    m = re.search(r"data-download-id='([0-9a-f]+)'", page)
    if not m:
        raise RuntimeError(f"{pack}: no download id on {page_url}")
    req = urllib.request.Request(f"https://kenney.nl/download-link/{m.group(1)}", headers={"Referer": page_url})
    link = json.loads(op.open(req).read())["url"]
    return op.open(urllib.request.Request(link, headers={"Referer": page_url})).read()


def extract(data, dest):
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        root = os.path.realpath(dest)
        for info in z.infolist():
            target = os.path.realpath(os.path.join(dest, info.filename))
            if target != root and not target.startswith(root + os.sep):
                raise RuntimeError(f"unsafe path in zip: {info.filename}")
        z.extractall(dest)


def main(args):
    force = "--force" in args
    packs = [a for a in args if not a.startswith("--")] or PACKS
    op = opener()
    for pack in packs:
        dest = os.path.join(OUT, pack)
        if os.path.isdir(dest) and not force:
            print(f"{pack}: already there")
            continue
        data = fetch(pack, op)
        os.makedirs(dest, exist_ok=True)
        extract(data, dest)
        print(f"{pack}: {len(data) // 1024} KB -> {os.path.relpath(dest, ROOT)}")


if __name__ == "__main__":
    main(sys.argv[1:])

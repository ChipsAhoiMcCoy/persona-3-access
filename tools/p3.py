"""Arcana dev client (stdlib only; `shot` needs Pillow).

The dev loop after Grim Dark's: the game runs visible but never focused, muted, with the mod's loopback HTTP
server on P3A_PORT (default 8793). Claude drives and observes it from here without touching the user's desktop.

  python tools/p3.py launch [--speak] [--sound] [--nobuild]   build, deploy the proxy, start the game unfocused
  python tools/p3.py play [--nobuild]          for the player: build, then start the game normally with the server on
  python tools/p3.py up [--slot N] [--build] [--speak] [--sound]   whatever it takes to reach a working game (relaunch if dead or hung)
  python tools/p3.py status | kill
  (kill, up, reload, keys, advance, goto, flow and scenario refuse a player's session, mode=play, without --force)
  python tools/p3.py reload [--nobuild]        hot reload: build, then swap the loaded mod for a fresh copy
  python tools/p3.py crash                     the last session's crash lines, symbolized
  python tools/p3.py sym 0x14d57b [...] [--map F]   mod offsets (arcana.dll+0x..) to functions via the linker map
  python tools/p3.py build | deploy            build only | copy dsound.dll (the proxy) into the game folder
  python tools/p3.py get /route?a=b            any dev route (prints the text)
  python tools/p3.py state | wait <on> <text> [secs] [--not] | batch "route" "sleep:500" "route"
  python tools/p3.py health | log [--since N] | speech [--since N] [--wait S]
  python tools/p3.py say "text"
  python tools/p3.py keys enter wait:2 down    game keys in order, waits in seconds; prints /screen
  python tools/p3.py advance [secs] [key]      play dialogue, pressing the key on each page; stops at a choice
  python tools/p3.py goto "name" [--noact]     auto-walk to a point of interest (or "#id"), then press Space
  python tools/p3.py jump --list <text> | <row or query> [--bare] [--dry] | --set key=value ...   reach any situation
                                               (tools/jump.py: state, fields, events, battles, gifts), no playing
  python tools/p3.py scenario list | <name>   multi-step test situations (tools/scenarios.py)
  python tools/p3.py flow load [slot] | newgame [--sign] | sleep   title -> save slot | fresh game | home to bed
  python tools/p3.py shot [name.png]           PrintWindow capture of the game window (bare names go to build/shots)

The game never loads build/ninja/arcana.dll itself but a copy beside it (arcana.live<N>.dll + .map), so a build
always links while the game runs and a failed build leaves the running mod alone.
"""
import ctypes
import ctypes.wintypes as wt
import bisect
import glob
import os
import re
import shutil
import subprocess
import sys
import time
import urllib.parse
import urllib.request
import urllib.error

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "build", "ninja")
# A second dev game (its own P3A_PORT) keeps its own log.
LOG_NAME = "arcana.log" if os.environ.get("P3A_PORT", "8793") == "8793" else f"arcana.{os.environ['P3A_PORT']}.log"
LOG = os.path.join(os.environ.get("LOCALAPPDATA", ""), "Arcana", LOG_NAME)
GAME_DIR = r"C:\Program Files (x86)\Steam\steamapps\common\P3R\P3R\Binaries\Win64"
GAME_EXE = os.path.join(GAME_DIR, "P3R.exe")
EXE_NAME = "P3R.exe"
APPID = "2161700"
DEFAULT_PORT = 8793
PORT = int(os.environ.get("P3A_PORT", str(DEFAULT_PORT)))

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
u32 = ctypes.WinDLL("user32", use_last_error=True)
psapi = ctypes.WinDLL("psapi", use_last_error=True)
k32.OpenProcess.restype = wt.HANDLE
k32.VirtualAllocEx.restype = ctypes.c_void_p
k32.VirtualAllocEx.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_size_t, wt.DWORD, wt.DWORD]
k32.WriteProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p]
k32.CreateRemoteThread.restype = wt.HANDLE
k32.CreateRemoteThread.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_void_p, wt.DWORD, ctypes.c_void_p]
k32.GetModuleHandleW.restype = ctypes.c_void_p
k32.GetProcAddress.restype = ctypes.c_void_p
k32.GetProcAddress.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
k32.VirtualFreeEx.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_size_t, wt.DWORD]
psapi.EnumProcessModulesEx.argtypes = [wt.HANDLE, ctypes.POINTER(ctypes.c_void_p), wt.DWORD, ctypes.POINTER(wt.DWORD), wt.DWORD]
psapi.GetModuleFileNameExW.argtypes = [wt.HANDLE, ctypes.c_void_p, wt.LPWSTR, wt.DWORD]

PROCESS_ALL = 0x1F0FFF


def unminimize():
    """The game drops posted keys while its window is minimized (2026-10-28): restore it without focus, at the
    bottom of the z-order, so it stays out of the user's way."""
    if getattr(unminimize, "hwnd", None) is None:
        m = re.search(r"window=0x([0-9a-f]+)", http("/health", retry=0, check=False) or "")
        unminimize.hwnd = int(m.group(1), 16) if m else 0
    h = unminimize.hwnd
    if h and u32.IsIconic(wt.HWND(h)):
        u32.ShowWindow(wt.HWND(h), 4)   # SW_SHOWNOACTIVATE
        u32.SetWindowPos(wt.HWND(h), wt.HWND(1), 0, 0, 0, 0, 0x13)   # HWND_BOTTOM; NOSIZE, NOMOVE, NOACTIVATE
        time.sleep(0.3)


def focus():
    """Bring the dev game to the foreground (the user allows it for the dev game). Unfocused, the game's input object
    never sees the decide action, so a choice list (SEL) ignores posted Enter / Space while message pages still take
    them (2026-11-11). An Alt tap first lets SetForegroundWindow work from a background process."""
    m = re.search(r"window=0x([0-9a-f]+)", http("/health", retry=0, check=False) or "")
    if not m:
        return False
    h = wt.HWND(int(m.group(1), 16))
    u32.keybd_event(0x12, 0, 0, 0)
    u32.SetForegroundWindow(h)
    u32.keybd_event(0x12, 0, 2, 0)
    time.sleep(0.5)
    u32.GetForegroundWindow.restype = ctypes.c_void_p
    return u32.GetForegroundWindow() == h.value


def http(route, timeout=15.0, body=None, retry=8.0, check=True):
    """GET (or POST with body) a dev route. Non-200 answers come back as "HTTP <code>: <text>" (503 = the game
    thread did not run the job: busy, loading or frozen). Connection errors are retried for `retry` seconds while the
    game runs (a hot reload's gap), then exit the script (or raise, with http.raise_errors)."""
    if check and route.lstrip("/").startswith("key"):
        unminimize()
    url = f"http://127.0.0.1:{PORT}{route if route.startswith('/') else '/' + route}"
    url = url.replace(" ", "%20")
    t0 = time.time()
    while True:
        try:
            req = urllib.request.Request(url, data=body.encode("utf-8") if body is not None else None)
            with urllib.request.urlopen(req, timeout=timeout) as r:
                return r.read().decode("utf-8", "replace")
        except urllib.error.HTTPError as e:
            return f"HTTP {e.code}: " + e.read().decode("utf-8", "replace")
        except (urllib.error.URLError, ConnectionError, TimeoutError) as e:
            if time.time() - t0 < retry and pids():
                time.sleep(0.5)
                continue
            if __name__ == "__main__" and not getattr(http, "raise_errors", False):
                sys.exit(f"no dev server on {PORT} ({getattr(e, 'reason', e)}); game {'running' if pids() else 'NOT running'}")
            raise


def q(s):
    return urllib.request.quote(s, safe="")


def pid_file(port):
    return os.path.join(ROOT, "build", f"instance-{port}.pid")


def pids(exe=EXE_NAME):
    """This instance's game process. P3A_PORT picks the instance: several dev games can run side by side, each
    started by a launch / up with its own port and recorded in build/instance-<port>.pid. The default port owns
    every game process no other instance has claimed (the player's own included)."""
    res = all_pids(exe)
    if exe != EXE_NAME:
        return res
    claimed = {}
    for f in glob.glob(os.path.join(ROOT, "build", "instance-*.pid")):
        try:
            port, pid = int(os.path.basename(f)[len("instance-"):-len(".pid")]), int(open(f).read().strip())
        except (OSError, ValueError):
            continue
        if pid in res:
            claimed[port] = pid
        else:
            try:
                os.remove(f)
            except OSError:
                pass
    if PORT in claimed:
        return [claimed[PORT]]
    return [p for p in res if p not in claimed.values()] if PORT == DEFAULT_PORT else []


def all_pids(exe=EXE_NAME):
    out = subprocess.run(["tasklist", "/FI", f"IMAGENAME eq {exe}", "/FO", "CSV", "/NH"], capture_output=True, text=True).stdout
    res = []
    for line in out.splitlines():
        parts = [p.strip('"') for p in line.split('","')]
        if len(parts) > 1 and parts[0].lower() == exe.lower():
            res.append(int(parts[1]))
    return res


def player_game():
    """The running game is the player's (`play` or a Steam launch: the mod's log header says mode=play)."""
    p = pids()
    try:
        with open(LOG, encoding="utf-8", errors="replace") as f:
            m = re.match(r"arcana log pid=(\d+) mode=(\w+)", f.readline())
    except OSError:
        return False
    return bool(p) and bool(m) and int(m.group(1)) in p and m.group(2) == "play"


def kill_all():
    # CrashReportClient keeps the mod DLL open after a crash; it goes too.
    for exe in (EXE_NAME, "CrashReportClient.exe"):
        for p in pids(exe):
            subprocess.run(["taskkill", "/F", "/PID", str(p)], capture_output=True)
    for _ in range(20):
        if not pids():
            return
        time.sleep(0.5)


def shadow():
    """Copy the build's arcana.dll (and .map) to arcana.live<N>.dll beside it, so assets and prism.dll resolve
    as before, and return its path. Copies no longer loaded are removed (a loaded one is locked and stays)."""
    src = os.path.join(BUILD, "arcana.dll")
    for old in glob.glob(os.path.join(BUILD, "arcana.live*.dll")):
        try:
            os.remove(old)
            if os.path.exists(old[:-4] + ".map"):
                os.remove(old[:-4] + ".map")
        except OSError:
            pass
    n = int(time.time())
    while os.path.exists(os.path.join(BUILD, f"arcana.live{n}.dll")):
        n += 1
    dst = os.path.join(BUILD, f"arcana.live{n}.dll")
    shutil.copyfile(src, dst)
    if os.path.exists(src[:-4] + ".map"):
        shutil.copyfile(src[:-4] + ".map", dst[:-4] + ".map")
    return dst


def loaded_mod(pid):
    """Path of the arcana*.dll loaded in the game, or None."""
    h = k32.OpenProcess(PROCESS_ALL, False, pid)
    try:
        for path in modules(h):
            if re.match(r"arcana(\.live\d+)?\.dll$", os.path.basename(path)):
                return path
    finally:
        k32.CloseHandle(h)
    return None


def health_ok(timeout=3):
    """(answered, ticking, text) of /health without exiting on errors."""
    try:
        http.raise_errors = True
        t = http("/health", timeout, retry=0)
    except Exception:
        return False, False, ""
    finally:
        http.raise_errors = False
    return t.startswith("ok"), "ticking=true" in t, t


def wait_health(module=None, secs=90):
    """Wait until /health answers (and names `module` when given); its first lines, or None."""
    t0 = time.time()
    while time.time() - t0 < secs:
        ok, _, t = health_ok()
        if ok and (module is None or os.path.basename(module).lower() in t.lower()):
            return t
        if not pids():
            return None
        time.sleep(1)
    return None


def build():
    r = subprocess.run(["cmd", "/c", os.path.join(ROOT, "tools", "build.cmd")], cwd=ROOT, capture_output=True, text=True)
    out = r.stdout + r.stderr
    errs = [l for l in out.splitlines() if " error " in l or "FAILED" in l or "fatal error" in l]
    if r.returncode != 0:
        print("\n".join(errs[:40]) or out[-4000:])
        sys.exit("build failed")
    print("build ok")


def deploy():
    src = os.path.join(BUILD, "dsound.dll")
    dst = os.path.join(GAME_DIR, "dsound.dll")
    if not os.path.exists(dst) or open(src, "rb").read() != open(dst, "rb").read():
        if all_pids():
            sys.exit("the game is running; cannot replace the proxy")
        shutil.copyfile(src, dst)
        print("deployed proxy", dst)


def remote_call(h, fn_addr, arg):
    t = k32.CreateRemoteThread(h, None, 0, ctypes.c_void_p(fn_addr), ctypes.c_void_p(arg), 0, None)
    if not t:
        raise OSError(ctypes.get_last_error(), "CreateRemoteThread")
    if k32.WaitForSingleObject(t, 20000) != 0:   # still running: freeing its argument or the DLL now would crash the game
        k32.CloseHandle(t)
        raise TimeoutError("the remote call did not finish in 20 s")
    code = wt.DWORD()
    k32.GetExitCodeThread(t, ctypes.byref(code))
    k32.CloseHandle(t)
    return code.value


def modules(h):
    arr = (ctypes.c_void_p * 1024)()
    need = wt.DWORD()
    psapi.EnumProcessModulesEx(h, arr, ctypes.sizeof(arr), ctypes.byref(need), 3)
    res = {}
    for i in range(need.value // 8):
        buf = ctypes.create_unicode_buffer(520)
        psapi.GetModuleFileNameExW(h, arr[i], buf, 520)
        res[buf.value.lower()] = arr[i]
    return res


def export_rva(dll, name):
    # Map the DLL as an image without running it and let the loader resolve the export.
    k32.LoadLibraryExW.restype = ctypes.c_void_p
    k32.LoadLibraryExW.argtypes = [wt.LPCWSTR, wt.HANDLE, wt.DWORD]
    k32.FreeLibrary.argtypes = [ctypes.c_void_p]
    h = k32.LoadLibraryExW(os.path.abspath(dll), None, 0x1)  # DONT_RESOLVE_DLL_REFERENCES
    if not h:
        raise OSError(ctypes.get_last_error(), "LoadLibraryExW " + dll)
    try:
        a = k32.GetProcAddress(ctypes.c_void_p(h), name.encode())
        if not a:
            raise KeyError(name)
        return a - h
    finally:
        k32.FreeLibrary(ctypes.c_void_p(h))


def inject(pid, dll):
    h = k32.OpenProcess(PROCESS_ALL, False, pid)
    path = (os.path.abspath(dll) + "\0").encode("utf-16-le")
    mem = k32.VirtualAllocEx(h, None, len(path), 0x3000, 0x04)
    k32.WriteProcessMemory(h, mem, path, len(path), None)
    ll = k32.GetProcAddress(k32.GetModuleHandleW("kernel32.dll"), b"LoadLibraryW")
    r = remote_call(h, ll, mem)
    k32.VirtualFreeEx(h, mem, 0, 0x8000)
    k32.CloseHandle(h)
    return r


def eject(pid):
    dll = loaded_mod(pid)
    if not dll:
        print("mod not loaded")
        return
    h = k32.OpenProcess(PROCESS_ALL, False, pid)
    base = modules(h).get(dll)
    rva = export_rva(dll, "p3a_unload")
    remote_call(h, base + rva, 0)
    fl = k32.GetProcAddress(k32.GetModuleHandleW("kernel32.dll"), b"FreeLibrary")
    remote_call(h, fl, base)
    k32.CloseHandle(h)
    print("ejected")


class STARTUPINFO(ctypes.Structure):
    _fields_ = [("cb", wt.DWORD), ("lpReserved", wt.LPWSTR), ("lpDesktop", wt.LPWSTR), ("lpTitle", wt.LPWSTR),
                ("dwX", wt.DWORD), ("dwY", wt.DWORD), ("dwXSize", wt.DWORD), ("dwYSize", wt.DWORD),
                ("dwXCountChars", wt.DWORD), ("dwYCountChars", wt.DWORD), ("dwFillAttribute", wt.DWORD),
                ("dwFlags", wt.DWORD), ("wShowWindow", wt.WORD), ("cbReserved2", wt.WORD),
                ("lpReserved2", ctypes.c_void_p), ("hStdInput", wt.HANDLE), ("hStdOutput", wt.HANDLE), ("hStdError", wt.HANDLE)]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [("hProcess", wt.HANDLE), ("hThread", wt.HANDLE), ("dwProcessId", wt.DWORD), ("dwThreadId", wt.DWORD)]


def start_game(env, show):
    block = "".join(f"{k}={v}\0" for k, v in env.items()) + "\0"
    si = STARTUPINFO()
    si.cb = ctypes.sizeof(si)
    si.dwFlags = 1  # STARTF_USESHOWWINDOW
    si.wShowWindow = show
    pi = PROCESS_INFORMATION()
    cmdline = ctypes.create_unicode_buffer(f'"{GAME_EXE}"')
    ok = k32.CreateProcessW(None, cmdline, None, None, False, 0x400, ctypes.create_unicode_buffer(block, len(block)),
                            GAME_DIR, ctypes.byref(si), ctypes.byref(pi))
    if not ok:
        sys.exit(f"CreateProcess failed: {ctypes.get_last_error()}")
    print("started pid", pi.dwProcessId)
    with open(pid_file(PORT), "w") as f:
        f.write(str(pi.dwProcessId))


def play(args):
    # For the player: the dev server on (so Claude can hot reload and look while they play), but focus, game sound,
    # speech and wall tones as in a normal launch. A failed build still launches the last good one.
    if pids():
        sys.exit("the game is already running")
    if "--nobuild" not in args:
        try:
            build()
        except SystemExit:
            if not os.path.exists(os.path.join(BUILD, "arcana.dll")):
                raise
            print("using the last good build")
    deploy()
    env = dict(os.environ)
    for k in ("P3A_NOFOCUS", "P3A_MUTE", "P3A_NOWALLTONES", "P3A_VPAD"):
        env.pop(k, None)
    env.update({
        "P3A_DLL": shadow(),
        "P3A_PORT": str(PORT),
        "SteamAppId": APPID, "SteamGameId": APPID, "SteamOverlayGameId": APPID,
    })
    start_game(env, 1)  # SW_SHOWNORMAL


def launch(args):
    if pids():
        sys.exit("the game is already running (it may be the player's): refusing")
    if "--nobuild" not in args:
        build()
    deploy()
    env = dict(os.environ)
    env.update({
        "P3A_DLL": shadow(),
        "P3A_PORT": str(PORT),
        "P3A_LOG": LOG_NAME,
        "P3A_NOFOCUS": "1",
        "P3A_NOWALLTONES": "1",
        "SteamAppId": APPID, "SteamGameId": APPID, "SteamOverlayGameId": APPID,
    })
    if "--speak" not in args:
        env["P3A_MUTE"] = "1"
    if "--sound" in args:
        env["P3A_GAME_SOUND"] = "1"
    start_game(env, 4)  # SW_SHOWNOACTIVATE
    t = wait_health(secs=120)
    if t is None:
        sys.exit("the game exited" if not pids() else "no /health after 120 s")
    print(t.splitlines()[0])


def reload(args):
    p = pids()
    if not p:
        sys.exit("not running")
    if "--nobuild" not in args:
        build()   # exits on failure, before anything is unloaded: the running mod stays
    new = shadow()
    eject(p[0])
    for attempt in (1, 2):
        inject(p[0], new)
        if loaded_mod(p[0]) == new.lower():
            break
        print("inject attempt", attempt, "did not load the mod")
    else:
        sys.exit("NOT LOADED")
    t = wait_health(new, 60)
    print("loaded", os.path.basename(new), "|", t.splitlines()[0] if t else "but no /health after 60 s")


def flow_load(slot):
    # From a fresh launch through the title menu's Load Game to that save slot.
    t0 = time.time()
    while time.time() - t0 < 240:
        time.sleep(1.2)
        scr = http("/screen")
        m = http("/msg")
        lines = scr.splitlines()
        if http("/world").startswith("player=BP"):
            print("reached the field")
            return True
        if "item=Msg" in m and "page=" in m and "item=MsgSubtitles" not in m:
            # No to online features (Space said yes and turned the shared Network Functions setting on, 2026-10-29);
            # Space for other notices (Enter left them up: the update's song notice, 2026-10-28)
            http("/key?k=c&ms=120" if "online features" in m else "/key?k=space&ms=120")
        elif "screen=movie" in scr:
            http("/key?k=tab&ms=120")   # the title's idle opening movie
        elif "Title screen" in scr or "Press any" in scr:
            http("/key?k=space&ms=120")   # Enter did not pass the press-wait screen (2026-10-28)
        elif "Title menu" in scr:
            http("/key?k=space&ms=120" if len(lines) > 1 and "Load Game" in lines[1] else "/key?k=down&ms=120")   # Enter stopped confirming (2026-10-28)
        elif "save_load" in scr:
            http("/key?k=space&ms=120" if len(lines) > 1 and (f"Slot {slot}," in lines[1] or f"Slot {slot}:" in lines[1]) else "/key?k=down&ms=120")
        print(lines[0] if lines else "", "|", (lines[1] if len(lines) > 1 else "")[:70])
    return False


def load_syms(mapfile):
    syms = []
    for l in open(mapfile, errors="replace"):
        p = l.split()
        if len(p) >= 4 and re.match(r"^[0-9a-f]{4}:[0-9a-f]{8}$", p[0]) and re.match(r"^[0-9a-f]{16}$", p[2]):
            syms.append((int(p[2], 16) - 0x180000000, p[1]))
    syms.sort()
    return syms


def crash_report(path=LOG):
    """The crash lines of a mod log with every arcana*.dll+0x.. resolved through the matching .map."""
    if not os.path.exists(path):
        return "no log at " + path
    lines = [l.rstrip("\n") for l in open(path, encoding="utf-8", errors="replace")]
    hits = [l for l in lines if " crash: exception" in l or l.startswith("  callers:") or "faulted" in l]
    if not hits:
        return "no crash lines in " + path
    maps = {}

    def sym(m):
        dll, off = m.group(1), int(m.group(2), 16)
        mp = os.path.join(BUILD, dll[:-4] + ".map")
        if not os.path.exists(mp):
            mp = os.path.join(BUILD, "arcana.map")
        if mp not in maps:
            maps[mp] = load_syms(mp) if os.path.exists(mp) else []
        syms = maps[mp]
        i = bisect.bisect_right(syms, (off, "~")) - 1
        return f"{m.group(0)} [{syms[i][1][:80]}+{off - syms[i][0]:#x}]" if i >= 0 else m.group(0)
    return "\n".join(re.sub(r"(arcana[\w.]*\.dll)\+0x([0-9a-f]+)", sym, l) for l in hits[-60:])


def up(args):
    """Reach a working game from any state: kill a crash reporter, inject a missing mod, relaunch a dead or hung
    game (keeping the dead session's log under build/crashlogs), then load the save slot if one is given."""
    for p in pids("CrashReportClient.exe"):
        subprocess.run(["taskkill", "/F", "/PID", str(p)], capture_output=True)
    p = pids()
    if p:
        if not loaded_mod(p[0]):
            print("game running without the mod: injecting")
            new = shadow()
            inject(p[0], new)
            wait_health(new, 60)
        t0 = time.time()
        while time.time() - t0 < 30:
            ok, ticking, t = health_ok()
            if ok and ticking:
                print("up:", t.splitlines()[0])
                return
            time.sleep(2)
        print("game not answering or not ticking for 30 s: relaunching")
    if os.path.exists(LOG):
        rep = crash_report()
        if not rep.startswith("no crash"):
            print(rep)
            os.makedirs(os.path.join(ROOT, "build", "crashlogs"), exist_ok=True)
            shutil.copyfile(LOG, os.path.join(ROOT, "build", "crashlogs", time.strftime("%Y%m%d-%H%M%S") + ".log"))
    kill_all()
    launch(([] if "--build" in args else ["--nobuild"]) + [a for a in args if a in ("--speak", "--sound")])
    if "--slot" in args:
        flow_load(args[args.index("--slot") + 1])


def shot(out):
    from PIL import Image
    hwnd = None
    pid_set = set(pids())

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, _):
        nonlocal hwnd
        pid = wt.DWORD()
        u32.GetWindowThreadProcessId(h, ctypes.byref(pid))
        cls = ctypes.create_unicode_buffer(64)
        u32.GetClassNameW(h, cls, 64)
        if pid.value in pid_set and cls.value == "UnrealWindow":
            hwnd = h
            return False
        return True
    u32.EnumWindows(cb, 0)
    if not hwnd:
        sys.exit("no game window")
    r = wt.RECT()
    u32.GetClientRect(hwnd, ctypes.byref(r))
    w, hgt = r.right, r.bottom
    gdi = ctypes.WinDLL("gdi32")
    hdc = u32.GetDC(hwnd)
    mdc = gdi.CreateCompatibleDC(hdc)
    bmp = gdi.CreateCompatibleBitmap(hdc, w, hgt)
    gdi.SelectObject(mdc, bmp)
    u32.PrintWindow(hwnd, mdc, 3)  # PW_CLIENTONLY | PW_RENDERFULLCONTENT
    buf = ctypes.create_string_buffer(w * hgt * 4)
    gdi.GetBitmapBits(bmp, len(buf), buf)
    img = Image.frombuffer("RGBA", (w, hgt), buf, "raw", "BGRA", 0, 1).convert("RGB")
    img.save(out)
    gdi.DeleteObject(bmp)
    gdi.DeleteDC(mdc)
    u32.ReleaseDC(hwnd, hdc)
    print(out, w, hgt)


def goto(want, act=True):
    """Auto-walk to the nearest point of interest whose line contains `want` ("#id" for an id), then press Space
    unless act is False. False when there is no such point."""
    want = want.lower()
    target = None
    for l in http("/poi?r=20000").splitlines():
        if l.startswith("#") and want in l.lower():
            target = l.split()[0][1:]
            print("target:", l)
            break
    if not target:
        return False
    http(f"/nav?walk={target}")
    for _ in range(60):
        time.sleep(1)
        if "walking=false" in http("/nav"):
            break
    print(http("/speech?since=0").splitlines()[-1])
    if act:
        time.sleep(0.8)
        http("/key?k=space&ms=120")
        time.sleep(2)
    return True


def advance(secs=60, key="enter", choose=False, stop_at=None):
    """Play through dialogue, pressing `key` whenever a message is open; prints what the mod speaks. Stops at a
    choice list (unless choose), at a modelled screen, or once a spoken line contains `stop_at`. Returns the reason."""
    cur = int(http("/speech?since=999999999").split()[1])
    t0 = time.time()
    while time.time() - t0 < secs:
        time.sleep(1.5)
        out = http(f"/speech?since={cur}")
        lines = out.splitlines()
        cur = int(lines[0].split()[1])
        for l in lines[1:]:
            said = l.split(": ", 1)[1] if ": " in l else l
            print(said)
            if stop_at and stop_at.lower() in said.lower():
                return "spoken"
        m = http("/msg")
        if "sel index" in m:
            if choose:
                time.sleep(1.0)
                http(f"/key?k={key}&ms=120")
                time.sleep(1.0)
                continue
            print("-- choice open, stopping")
            return "choice"
        scr = http("/screen").splitlines()[0]
        if "screen=movie" in scr:
            continue   # subtitles are message items too, but Enter would pause the movie
        if any(s in scr for s in ("screen=get_item", "screen=stats_up", "screen=rank_up", "screen=battle_result")):
            http("/key?k=space&ms=120")   # notices that only need OK
            continue
        if "screen=none" not in scr and "item=Msg" not in m:
            print("-- " + scr + ", stopping")
            return "screen"
        if "item=Msg" in m and "item=MsgSubtitles" not in m:   # subtitles advance on their own
            http(f"/key?k={key}&ms=120")
    return "time"


def cheat(fn, **args):
    """A UAppCheatManager command (Atlus's own debug code, live in this build) with int arguments."""
    a = ",".join(f"{k}:int:{v}" for k, v in args.items())
    return http(f"/ue/call?cdo=AppCheatManager&fn={fn}" + (f"&args={a}" if a else ""))


def gwork(fn, **args):
    """A GlobalWorkBpLib call (flags by index section<<28|bit, counters, money) with int arguments."""
    a = ",".join(f"{k}:int:{v}" for k, v in args.items())
    return http(f"/ue/call?cdo=GlobalWorkBpLib&fn={fn}" + (f"&args={a}" if a else ""))


def field_wait(ready, secs):
    """Wait (server side) until /area/ready says the field has settled (ready=True: the field manager idle, no field
    event, the hero placed) or has left that state (ready=False: a field change was taken). Returns the last answer."""
    t0 = time.time()
    while True:
        left = secs - (time.time() - t0)
        r = http(f"/wait?on=/area/ready&match=ready&not={0 if ready else 1}&timeout={max(min(left, 10), 0.1):.1f}",
                 check=False)
        last = r.split("\n", 1)[1].strip() if "\n" in r else r.strip()
        if r.startswith("matched") or left <= 0:
            return last


def field_settle(secs):
    """Wait until the field has settled, pressing Enter on the message pages its init script or field event waits on
    (a Tartarus floor's arrival line). A choice list is left to the caller. Returns /area/ready's last answer."""
    t0 = time.time()
    while True:
        left = secs - (time.time() - t0)
        last = field_wait(True, min(left, 3))
        if last == "ready" or left <= 0:
            return last
        m = http("/msg", check=False)
        page = re.search(r"page=0x[0-9a-f]+ '(.*)'\s*$", m, re.M)
        if "sel index" in m or not page or ("init script" not in last and "field event" not in last):
            continue
        print(f"  (advancing: {page.group(1)})")
        http("/key?k=enter&ms=120")
        http(f"/wait?on=/msg&match={urllib.parse.quote(page.group(1)[:40])}&not=1&timeout=5", check=False)


def field(major, minor, entry=1, secs=40):
    """Load a field area (CALL_FIELD) once the current one has settled, and wait until the new one has: a CALL_FIELD
    while the last field's init script or field event still runs crashed the game (/area/ready, src/area.cpp). The
    arrival messages that hold a field's init script are advanced."""
    last = field_settle(secs)
    if last != "ready":
        print(f"  the field has not settled ({last}): finish its scene or choice first")
        return False
    r = http(f"/area/goto?major={major}&minor={minor}&entry={entry}")
    if not r.startswith("CALL_FIELD("):
        print(f"  {r.strip()}")
        return False
    if field_wait(False, secs) == "ready":
        print("  the field change was not taken")
        return False
    last = field_settle(secs)
    a = http("/area", check=False)
    here = a.startswith(f"ids {major} {minor} ") or major == 299 and f", floor {minor}'" in a  # Tartarus blocks: 'Arqa, floor 50'
    if last != "ready":
        print(f"  the new field has not settled ({last})")
    return last == "ready" and here and "HTTP" not in a   # a Tartarus floor reports ids 0 0


def scenario(name):
    """Run a named scenario from tools/scenarios.py: each step is a call above. Prints each step and the outcome."""
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import scenarios
    if name == "list":
        for k, s in scenarios.SCENARIOS.items():
            print(f"{k:16} {s['about']}")
        return
    s = scenarios.SCENARIOS.get(name)
    if not s:
        sys.exit(f"no scenario {name!r}; `p3.py scenario list`")
    t0 = time.time()
    ctx = {"http": http, "cheat": cheat, "gwork": gwork, "field": field, "goto": goto, "advance": advance, "focus": focus,
           "keys": lambda *ks: [time.sleep(float(k[5:])) if k.startswith("wait:") else http(f"/key?k={k}&ms=120")
                                for k in ks]}
    for step in s["steps"]:
        fn, *a = step
        kw = a.pop() if a and isinstance(a[-1], dict) else {}
        print(f"[{time.time() - t0:5.1f}s] {fn} {' '.join(map(str, a))} {kw if kw else ''}".rstrip())
        r = ctx[fn](*a, **kw)
        if r is False:
            sys.exit(f"step {fn} failed")
        if isinstance(r, str) and r.strip():
            print("  " + r.strip().splitlines()[0])
    print(f"[{time.time() - t0:5.1f}s] done: {s['check']}")


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    a = sys.argv[1:]
    if not a:
        print(__doc__)
        return
    cmd, rest = a[0], a[1:]
    # Commands that kill, reload or press keys never touch the player's session unless asked (--force).
    force = "--force" in rest
    rest = [x for x in rest if x != "--force"]
    if cmd in ("kill", "up", "reload", "keys", "advance", "goto", "flow", "scenario", "jump") and not force and player_game():
        sys.exit(f"the running game is the player's (mode=play): `{cmd}` would act on it; pass --force if they asked")
    if cmd == "launch":
        launch(rest)
    elif cmd == "play":
        play(rest)
    elif cmd == "status":
        p = pids()
        print("running" if p else "not running", p)
        if p:
            try:
                print(http("/health", 3))
            except Exception as e:
                print("no /health:", e)
    elif cmd == "kill":
        kill_all()
    elif cmd == "build":
        build()
    elif cmd == "deploy":
        deploy()
    elif cmd == "reload":
        reload(rest)
    elif cmd == "up":
        up(rest)
    elif cmd == "crash":
        print(crash_report(rest[0] if rest else LOG))
    elif cmd == "get":
        print(http(rest[0], float(rest[1]) if len(rest) > 1 else 30), end="")
    elif cmd == "state":
        print(http("/state" + (f"?speech={rest[0]}" if rest else "")), end="")
    elif cmd == "wait":
        # p3.py wait screen "Title menu" [10] [--not]   (on=screen|msg|world|nav|speech|<route>)
        a = [x for x in rest if not x.startswith("--")]
        secs = float(a[2]) if len(a) > 2 else 10
        out = http(f"/wait?on={q(a[0])}&match={q(a[1])}&timeout={secs}" + ("&not=1" if "--not" in rest else ""), secs + 15)
        print(out, end="")
        if out.startswith("HTTP"):
            sys.exit(1)
    elif cmd == "batch":
        print(http("/batch", 120, body="\n".join(rest)), end="")
    elif cmd in ("health",):
        print(http("/health"), end="")
    elif cmd in ("log", "speech"):
        since = rest[rest.index("--since") + 1] if "--since" in rest else "0"
        print(http(f"/{cmd}?since={since}"), end="")
    elif cmd == "keys":
        # p3.py keys enter wait:2 down down space   (a sequence of game keys with waits in seconds)
        for k in rest:
            if k.startswith("wait:"):
                time.sleep(float(k[5:]))
            else:
                http(f"/key?k={q(k)}&ms=120")
                time.sleep(0.35)
        print(http("/screen"), end="")
    elif cmd == "flow" and rest and rest[0] == "load":
        # p3.py flow load [slot=15]: from a fresh launch through the title menu's Load Game to that save slot.
        flow_load(rest[1] if len(rest) > 1 else "15")
    elif cmd == "flow" and rest and rest[0] == "newgame":
        # From a fresh launch to the contract: answers the first-run prompts the way a player would (network off,
        # Peaceful difficulty), driven by what the mod itself reports (/screen, /msg).
        t0 = time.time()
        while time.time() - t0 < 300:
            time.sleep(1.2)
            scr = http("/screen")
            m = http("/msg")
            if "name_entry" in scr and "--sign" in rest:
                # --sign: fill in the contract (test dev) and confirm it; stops at the field after the prologue
                lines = scr.splitlines()
                d = lines[1] if len(lines) > 1 else ""
                if d.strip() in ("First name:", "First name: blank"):
                    http("/key?k=t,e,s,t&ms=60"); time.sleep(0.6); http("/key?k=enter&ms=80")
                elif d.strip() in ("Last name:", "Last name: blank"):
                    http("/key?k=d,e,v&ms=60"); time.sleep(0.6); http("/key?k=enter&ms=80")
                elif "First name test" in d and "Last name dev" in d:
                    http("/key?k=h&ms=80")
                elif "First name blank" in d:
                    http("/key?k=enter&ms=80")
                elif "First name test" in d and "Last name blank" in d:
                    http("/key?k=down&ms=80"); time.sleep(0.3); http("/key?k=enter&ms=80")
                print(scr.splitlines()[0], "|", d[:60])
                continue
            if "name_entry" in scr or "field" in scr:
                print("reached:", scr.splitlines()[0])
                break
            page = ""
            for l in m.splitlines():
                if "page=" in l and "'" in l:
                    page = l.split("'", 1)[1]
            if "item=Msg" in m and page and "item=MsgSubtitles" not in m:   # a key on a subtitle pauses the movie
                key = "c" if "online features" in page else ("enter" if "Mind" in m else "space")
                http(f"/key?k={key}&ms=120")
                continue
            if ("Title screen, OP" in scr or "Press any" in scr or "PressWait" in scr or "Title screen, Logo" in scr
                    or "Caution" in scr or "Photosensitive" in scr):   # the mod holds the boot notices for a press
                http("/key?k=enter&ms=120")
            elif "Title menu" in scr:
                if "New Game" in scr.splitlines()[1]:
                    http("/key?k=enter&ms=120")
                else:
                    http("/key?k=up&ms=120")
            elif "difficulty" in scr:
                if "Peaceful" in scr.splitlines()[1]:
                    http("/key?k=enter&ms=120")
                else:
                    http("/key?k=up&ms=120")
            elif "screen=none" in scr:
                w = http("/world")
                if w.startswith("player=BP"):
                    print("reached the field")
                    break
                pass   # movies and events run on their own; a key here pauses the movie
            print(scr.splitlines()[0], "|", (scr.splitlines()[1] if len(scr.splitlines()) > 1 else "")[:60])
    elif cmd == "goto":
        # p3.py goto "Head to the Dorm" [--noact]: auto-walk to the nearest point of interest whose name contains the
        # text, then press the action key (Space) unless --noact.
        if not goto(rest[0], "--noact" not in rest):
            sys.exit("no such point of interest")
    elif cmd == "advance":
        # p3.py advance [seconds=60] [key=enter]: play through dialogue, pressing the key whenever a message is open;
        # prints what the mod speaks. Stops early when a choice list appears (sel != null) or a modelled screen opens.
        args = [a for a in rest if not a.startswith("--")]
        advance(float(args[0]) if args else 60, args[1] if len(args) > 1 else "enter", "--choose" in rest)
    elif cmd == "jump":
        # p3.py jump ...: tools/jump.py (its docstring is the help).
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import jump
        jump.main(rest, sys.modules[__name__])
    elif cmd == "scenario":
        # p3.py scenario list | <name>: put the game straight into a test situation (tools/scenarios.py).
        scenario(rest[0] if rest else "list")
    elif cmd == "flow" and rest and rest[0] == "sleep":
        # p3.py flow sleep: from free time anywhere in town, travel home (town map -> Student Dormitory), go in,
        # up to your room and to bed, choosing the first option of every choice. Prints what the mod speaks.
        def say_tail():
            return http("/speech").splitlines()[-1].split(": ", 1)[-1]
        def key(k, wait=1.5):
            http(f"/key?k={k}&ms=150"); time.sleep(wait)
        def drain(secs=40):
            t0 = time.time()
            while time.time() - t0 < secs:
                m = http("/msg")
                if "sel index" in m: time.sleep(0.8); key("space", 2.5); continue
                scr = http("/screen").splitlines()[0]
                if any(s in scr for s in ("get_item", "stats_up", "rank_up")): key("space", 2); continue
                if "item=Msg" in m and "item=MsgSubtitles" not in m: key("enter", 2); continue
                return
        def poi(name):
            for l in http("/poi?r=30000").splitlines():   # "#123 [Kind] Name, 5 meters, ..."
                parts = l.split(" ", 1)
                if len(parts) == 2 and parts[1].startswith("["): parts[1] = parts[1].split("] ", 1)[-1]   # "[Object] Bed"
                if len(parts) == 2 and parts[1].lower().startswith(name.lower()): return parts[0].lstrip("#")
            return None
        def walk(name, use=True):
            o = poi(name)
            if not o: return False
            http(f"/nav?walk={o}")
            for _ in range(30):
                time.sleep(2)
                if "walking=false" in http("/nav"): break
            if use: key("space", 3)
            return True
        def fast(dest):   # the map key's "Where should I go?" list
            key("m", 2.5)
            for _ in range(12):
                if say_tail().startswith(dest) or ("? " + dest + ",") in say_tail(): key("space", 4); return True
                key("down", 1.2)
            key("c", 1.5); return False
        drain()
        if not poi("Bed"):
            if not fast("Your Room"):
                if poi("Iwatodai Dorm") is None:
                    fast("Town Map"); time.sleep(1)
                    for _ in range(16):
                        if "Student Dormitory" in say_tail(): break
                        key("up" if _ < 8 else "down", 1.2)
                    key("space", 2); key("space", 6)
                drain()
                walk("Iwatodai Dorm"); time.sleep(2); drain(60)
                if not fast("Your Room"):
                    walk("Hallway 2F", use=False); time.sleep(2); walk("Your Room"); time.sleep(3)
            drain()
        walk("Bed"); drain(10)
        print(http("/speech").splitlines()[-1])
    elif cmd == "say":
        print(http("/say?text=" + q(" ".join(rest))), end="")
    elif cmd == "shot":
        out = rest[0] if rest else "shot.png"
        if not os.path.dirname(out):
            os.makedirs(os.path.join(ROOT, "build", "shots"), exist_ok=True)
            out = os.path.join(ROOT, "build", "shots", out)
        shot(out)
    elif cmd == "sym":
        # p3.py sym 0x14d57b ... [--map build/ninja/arcana.map], through load_syms.
        mp = rest[rest.index("--map") + 1] if "--map" in rest else os.path.join(BUILD, "arcana.map")
        syms = load_syms(mp)
        for a in rest:
            if a.startswith("0x"):
                off = int(a, 16)
                i = bisect.bisect_right(syms, (off, "~")) - 1
                print(a, f"{syms[i][1][:100]}+{off - syms[i][0]:#x}" if i >= 0 else "?")
    else:
        print(__doc__)


if __name__ == "__main__":
    main()

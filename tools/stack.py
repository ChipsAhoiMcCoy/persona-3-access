# Stack snapshot of a live thread (no debugger needed): python tools/stack.py <pid> <tid> <dll_base_hex> [mapfile]
# Prints RIP/RSP and every stack slot that points into the mod DLL or P3R.exe, mod addresses resolved via the map.
import ctypes, ctypes.wintypes as w, sys, struct, bisect, re
pid, tid, base = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3], 16)
mapf = sys.argv[4] if len(sys.argv) > 4 else "build/ninja/p3access.map"
k = ctypes.WinDLL("kernel32", use_last_error=True)
syms = []
for l in open(mapf, errors="replace"):
    p = l.split()
    if len(p) >= 4 and re.match(r"^[0-9a-f]{4}:[0-9a-f]{8}$", p[0]) and re.match(r"^[0-9a-f]{16}$", p[2]):
        syms.append((int(p[2], 16) - 0x180000000, p[1]))
syms.sort()
def sym(off):
    i = bisect.bisect_right(syms, (off, "~")) - 1
    return f"{syms[i][1][:90]}+{off - syms[i][0]:#x}" if i >= 0 else "?"
class CTX(ctypes.Structure):
    _fields_ = [("buf", ctypes.c_ubyte * 1232)]
h = k.OpenThread(0x1FFFFF, False, tid)
k.SuspendThread(h)
ctx = (ctypes.c_ubyte * (1232 + 16))()
addr = (ctypes.addressof(ctx) + 15) & ~15
c = (ctypes.c_ubyte * 1232).from_address(addr)
struct.pack_into("<I", c, 0x30, 0x10000B)  # CONTEXT_FULL
ok = k.GetThreadContext(h, ctypes.byref(c))
rip, rsp = struct.unpack_from("<Q", c, 0xF8)[0], struct.unpack_from("<Q", c, 0x98)[0]
ph = k.OpenProcess(0x10, False, pid)
buf = (ctypes.c_ubyte * 0x4000)()
n = ctypes.c_size_t()
k.ReadProcessMemory(ph, ctypes.c_void_p(rsp), buf, 0x4000, ctypes.byref(n))
k.ResumeThread(h)
print(f"ctx ok={ok} rip={rip:#x} rsp={rsp:#x}", ("mod+" + hex(rip - base) + " " + sym(rip - base)) if base <= rip < base + 0x800000 else "")
for i in range(0, n.value - 8, 8):
    v = struct.unpack_from("<Q", buf, i)[0]
    if base <= v < base + 0x800000: print(f"  [rsp+{i:#x}] mod+{v - base:#x} {sym(v - base)}")
    elif 0x140000000 <= v < 0x146000000: print(f"  [rsp+{i:#x}] P3R+{v - 0x140000000:#x}")

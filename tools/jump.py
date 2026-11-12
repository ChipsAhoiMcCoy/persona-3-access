"""Reach any situation in the game without playing to it (`python tools/p3.py jump ...`).

  jump --list <text>          index rows (events, battles, shops, choices, gifts...) and game data (items, encounters,
                              Social Links, Personas, fields) matching the text; nothing is run
  jump <row | query>          satisfy the row's gates (date, time, flags, counters), load its field, then reach the
                              trigger as the player would (walk to the object or person, press); --bare calls it directly
  jump --set key=value ...    pure state, alone or before a row (the row's own gates are applied after it)
  jump --dry <row>            print what would be set and done, run nothing

State keys (ids are numbers, 0x.. allowed; item / Persona / Social Link / field names are looked up in the data index):
  date=M/D[/T] time=T          T: 0 morning 1 am 2 noon 3 pm 4 afterschool 5 evening 6 darkhour 7 midnight (or a name)
  flag:ID=1|0  count:N=V  money=N  item:ID=N (silent)  give:ID=N (the got-item notice)  party:ID=1|0
  cmm:ARCANA=RANK  stat:N=POINTS (0 academics, 1 charm, 2 courage: points added)  persona:ID=1 (into the stock)
  field=MAJOR,MINOR[,ENTRY]  battle=ENCOUNTER  eventbattle=MAJOR,MINOR,ENCOUNTER  event=MAJOR,MINOR  cmmevent=MAJOR,MINOR
  cmd=NAME(a,b,...)            any flowscript command
Every value set is read back and printed, and every gate says how it was met, so a failed jump shows what it missed.

Flags are written as the scripts write them: BIT_CHK((171 + 0x20000000)) is 0x200000AB (the added constant is the
section). The index is built from a local decompile of the game, which is not published: without it, --set and the
named scenarios (tools/scenarios.py) still work.
"""
import datetime
import os
import re
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INDEX_DIR = os.environ.get("P3A_INDEX") or os.path.join(ROOT, "research", "decomp")
EVENT_INDEX = os.path.join(INDEX_DIR, "event_index.tsv")
DATA_INDEX = os.path.join(INDEX_DIR, "data_index.tsv")

TIMES = {"morning": 0, "am": 1, "noon": 2, "lunch": 2, "pm": 3, "afterschool": 4, "after_school": 4, "evening": 5,
         "night": 5, "darkhour": 6, "dark_hour": 6, "dark": 6, "midnight": 7}
TIME_NAMES = ["morning", "AM", "noon", "PM", "after school", "evening", "Dark Hour", "midnight"]
DOW = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]   # GET_DAYOFWEEK: ECldWeek, Sunday 0
SCHED_TIMES = {"EarlyMorning": 0, "Morning": 0, "AM": 1, "LunchTime": 2, "PM": 3, "AfterSchool": 4, "Daytime": 4,
               "Night": 5, "ShadowTime": 6, "MidNight": 7}
YEAR = 2009                                               # the game's school year starts 2009-04-01

p3 = None   # tools/p3.py, set by main()


# ---- the game -------------------------------------------------------------------------------------------------------
def cmd(name, *args, wait=None, quiet=False):
    """Run a flowscript command through /flow/run. Returns (status line, int result or None). wait: seconds to wait for
    a yielding command to finish (None: do not wait)."""
    a = ",".join(str(x) for x in args)
    line = p3.http(f"/flow/run?cmd={name}" + (f"&args={a}" if a else "")).strip()
    if wait and " running " in line:
        text = line.split(" running ")[0]
        t0 = time.time()
        while time.time() - t0 < wait:
            time.sleep(0.5)
            out = p3.http("/flow/run")
            done = [l for l in out.splitlines() if l.startswith("finished: " + text)]
            if done and not any(l.startswith(text + " running") for l in out.splitlines()):
                line = done[-1][len("finished: "):]
                break
    if not quiet:
        print("  " + line)
    m = re.search(r" result (-?\d+)", line)
    return line, (int(m.group(1)) if m else None)


def get(name, *args):
    return cmd(name, *args, quiet=True)[1]


def now():
    """(month, day, time) from the game."""
    return get("GET_MONTH"), get("GET_DAY"), get("GET_TIME")


def set_date(month, day, t):
    p3.cheat("AtlSetDayTime", Month=month, Day=day, Time=t)
    m, d, tt = now()
    ok = (m, d, tt) == (month, day, t)
    print(f"  date {m}/{d} {TIME_NAMES[tt] if 0 <= (tt or 0) < 8 else tt}" + ("" if ok else f"  (wanted {month}/{day} {TIME_NAMES[t]})"))
    return ok


# ---- data index (names to ids) --------------------------------------------------------------------------------------
_data = None


def data_rows():
    global _data
    if _data is None:
        _data = []
        if os.path.exists(DATA_INDEX):
            with open(DATA_INDEX, encoding="utf-8") as f:
                head = f.readline().rstrip("\n").split("\t")
                for l in f:
                    _data.append(dict(zip(head, l.rstrip("\n").split("\t"))))
    return _data


def lookup(table_words, text):
    """An id from a number or a name in the data index (tables whose name contains one of table_words)."""
    text = text.strip()
    try:
        return int(text, 0)
    except ValueError:
        pass
    want = text.lower()
    hits = [r for r in data_rows() if any(w in r.get("table", "").lower() for w in table_words)
            and r.get("name", "").lower() == want]
    hits = hits or [r for r in data_rows() if any(w in r.get("table", "").lower() for w in table_words)
                    and want in r.get("name", "").lower()]
    if not hits:
        sys.exit(f"no {'/'.join(table_words)} named {text!r}" + ("" if data_rows() else " (no data index here)"))
    r = hits[0]
    print(f"  {text!r} -> {r['table']} {r['id']} {r['name']}")
    return int(r["id"], 0)


# ---- --set ----------------------------------------------------------------------------------------------------------
def parse_time(s):
    s = s.strip().lower().replace(" ", "")
    return TIMES[s] if s in TIMES else int(s)


def apply_set(item):
    """One key=value. Prints what was set and its value read back."""
    key, _, val = item.partition("=")
    kind, _, ident = key.partition(":")
    kind = kind.lower()
    print(f"set {item}")
    if kind == "date":
        parts = val.split("/")
        m, d = int(parts[0]), int(parts[1])
        t = parse_time(parts[2]) if len(parts) > 2 else now()[2]
        return set_date(m, d, t)
    if kind == "time":
        m, d, _ = now()
        return set_date(m, d, parse_time(val))
    if kind == "flag":
        f = int(ident, 0)
        cmd("BIT_ON" if int(val) else "BIT_OFF", f, quiet=True)
        v = get("BIT_CHK", f)
        print(f"  flag {flag_name(f)} = {v}")
        return v == (1 if int(val) else 0)
    if kind == "count":
        n = int(ident, 0)
        cmd("SET_COUNT", n, int(val, 0), quiet=True)
        v = get("GET_COUNT", n)
        print(f"  COUNT {n} = {v}")
        return v == int(val, 0)
    if kind == "money":
        p3.gwork("GWorkSetMoney", SetMoeny=int(val))
        v = get("GET_MONEY")
        print(f"  money {v}")
        return v == int(val)
    if kind in ("item", "give"):
        i = lookup(["item"], ident)
        n = int(val)
        if kind == "item":
            cmd("SET_ITEM_NUM", i, n, quiet=True)
        else:
            cmd("ADD_GET_ITEM_LIST", i, n, quiet=True)
            cmd("OPEN_GET_ITEM")   # the notice stays until Space (the job ends when it closes)
        v = get("GET_ITEM_NUM", i)
        print(f"  item {i:#x} owned {v}")
        return True
    if kind == "party":
        cmd("PARTY_IN" if int(val) else "PARTY_OUT", int(ident, 0))
        print(f"  member {ident} {'in' if get('IS_PARTY_EXISTS', int(ident, 0)) else 'not in'} the party")
        return True
    if kind == "cmm":
        a = lookup(["social", "cmm", "arcana"], ident)
        cmd("CMM_SET_LV", a, int(val), quiet=True)
        v = get("CMM_GET_LV", a)
        print(f"  Social Link {a} rank {v}")
        return v == int(val)
    if kind == "stat":
        n = int(ident)
        cmd("ADD_HERO_PARAM_EXP", n, int(val), quiet=True)
        print(f"  social stat {n} level {get('GET_HERO_PARAM_LV', n)}")
        return True
    if kind == "persona":
        i = lookup(["persona"], ident)
        cmd("ADD_PERSONA_STOCK", i)
        return True
    if kind == "field":
        a = [int(x, 0) for x in val.split(",")]
        ok = p3.field(*a)
        print(f"  {p3.http('/area').splitlines()[0]}")
        return ok
    if kind in ("battle", "eventbattle"):
        # A bare battle has no script holding the field: a press during it can reach a field hit, and its battle
        # objects outlive it (the mod then thinks a battle is on). The first one in a game session has always ended
        # cleanly; a later one crashed the game at its results three times (2026-11-12). Relaunch between them.
        if any("BATTLE(" in l for l in p3.http("/flow/run").splitlines()):
            print("  warning: a bare battle already ran in this game session; a second one has crashed the game at its "
                  "results (relaunch with `p3.py up --slot N` first)")
        print("  keep the hero 2 m from any object or person: a press in the battle can reach the field behind it")
        if kind == "battle":
            cmd("CALL_FIELDBATTLE", lookup(["encount"], val))
        else:
            cmd("CALL_EVENTBATTLE", *[int(x, 0) for x in val.split(",")])
        return True
    if kind in ("event", "cmmevent"):
        cmd("CALL_EVENT" if kind == "event" else "CALL_EVENT_CMM", *[int(x, 0) for x in val.split(",")])
        return True
    if kind == "cmd":
        m = re.match(r"(\w+)\((.*)\)$", val.strip())
        if not m:
            sys.exit("cmd=NAME(a,b)")
        cmd(m.group(1), *[x.strip() for x in m.group(2).split(",") if x.strip()])
        return True
    sys.exit(f"unknown key {kind!r} (see `p3.py jump` help)")


def flag_name(f):
    return f"{f:#x}" if f >= 0x10000000 else str(f)


# ---- the event index ------------------------------------------------------------------------------------------------
_rows = None


def rows():
    global _rows
    if _rows is None:
        _rows = []
        if os.path.exists(EVENT_INDEX):
            with open(EVENT_INDEX, encoding="utf-8") as f:
                head = f.readline().rstrip("\n").split("\t")
                for l in f:
                    _rows.append(dict(zip(head, l.rstrip("\n").split("\t"))))
    return _rows


def no_index():
    print("No event index here. It is built from a local decompile of the game (not published); `jump --set ...` and "
          "the named scenarios (`p3.py scenario list`) work without it.")


def search(text):
    """Index rows for text: a row id; rows whose line holds the whole text (`CALL_EVENT_CMM(3, 10)`); else rows
    holding every word of it (id, command, args, script, procedure, owner, field, gates, message)."""
    words = text.lower().split()
    if len(words) == 1 and words[0].isdigit():
        return [r for r in rows() if r.get("id") == words[0]]
    whole = [r for r in rows() if text.lower() in row_line(r).lower()]
    if whole:
        return whole
    return [r for r in rows() if all(w in " ".join(r.values()).lower() for w in words)]


def row_line(r):
    s = f"{r.get('id', '?'):>6} {r.get('kind', ''):8} {r.get('command', '')}({r.get('args', '')})  {r.get('script', '')}"
    s += f" :{r.get('procedure', '')}"
    if r.get("owner"):
        s += f"  [{r['owner']}]"
    if r.get("gates"):
        s += f"  if {r['gates']}"
    if r.get("msg"):
        s += f"  \"{r['msg'][:60]}\""
    return s


def list_rows(text, limit=60):
    hits = search(text)
    data = [r for r in data_rows() if all(w in " ".join(r.values()).lower() for w in text.lower().split())]
    if not rows() and not data_rows():
        no_index()
    for r in hits[:limit]:
        print(row_line(r))
    if len(hits) > limit:
        print(f"... {len(hits) - limit} more rows (narrow the text)")
    for r in data[:limit]:
        print(f"  data {r.get('table', '')} {r.get('id', '')} {r.get('name', '')} {r.get('extra', '')}")
    if len(data) > limit:
        print(f"... {len(data) - limit} more data rows")
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import scenarios
    for k, s in scenarios.SCENARIOS.items():
        if all(w in (k + " " + s["about"]).lower() for w in text.lower().split()):
            print(f"  scenario {k}: {s['about']}")


# ---- gates ----------------------------------------------------------------------------------------------------------
# The index's gate forms: `BIT 156` / `!BIT 0x200000AB`, `COUNT 123==0`, `DAY 8/21`, `DAYS 10/5-1/31`, `WEEKDAY n`,
# `TIME==5` / `MONTH` / `MDAY`, `CHK_TIME(n)`, `on 5/25 LunchTime` (a scheduler procedure), `SEL(7)==0`, calls such as
# `CMM_GET_LV(3)>=2` / `GET_ITEM_NUM(id)>0` / `IS_PARTY_EXISTS(4)` / `DUNGEON_GET_FLOORNO()==50`, `(a | b)` for an ||.
NUM = r"(-?(?:0x[0-9a-fA-F]+|\d+))"
OPS = {"==": lambda a, b: a == b, "!=": lambda a, b: a != b, ">=": lambda a, b: a >= b, "<=": lambda a, b: a <= b,
       ">": lambda a, b: a > b, "<": lambda a, b: a < b}
NOT = {"==": "!=", "!=": "==", ">=": "<", "<=": ">", ">": "<=", "<": ">="}
# Values jump can set: kind -> (description, read, set)
SETTABLE = {
    "count": ("COUNT {}", lambda n: get("GET_COUNT", n), lambda n, v: apply_set(f"count:{n}={v}")),
    "cmm": ("Social Link {} rank", lambda a: get("CMM_GET_LV", a), lambda a, v: apply_set(f"cmm:{a}={v}")),
    "item": ("item {:#x} owned", lambda i: get("GET_ITEM_NUM", i), lambda i, v: apply_set(f"item:{i}={v}")),
    "money": ("money", lambda _: get("GET_MONEY"), lambda _, v: apply_set(f"money={v}")),
    "party": ("member {} in the party", lambda i: get("IS_PARTY_EXISTS", i), lambda i, v: apply_set(f"party:{i}={v}")),
}


def school_days():
    d = datetime.date(YEAR, 4, 1)
    while d <= datetime.date(YEAR + 1, 3, 31):
        yield d
        d += datetime.timedelta(days=1)


def split_top(text, sep):
    """Split at sep outside parentheses."""
    out, depth, cur, i = [], 0, "", 0
    while i < len(text):
        c = text[i]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        if depth == 0 and text.startswith(sep, i):
            out.append(cur)
            cur = ""
            i += len(sep)
            continue
        cur += c
        i += 1
    out.append(cur)
    return [x.strip() for x in out if x.strip()]


def strip_parens(t):
    """Drop parentheses that wrap the whole term."""
    while t.startswith("(") and t.endswith(")"):
        depth = 0
        for i, c in enumerate(t):
            depth += c == "("
            depth -= c == ")"
            if depth == 0 and i < len(t) - 1:
                return t   # the first "(" closes before the end: not a wrapper
        t = t[1:-1].strip()
    return t


class Gates:
    """A row's conditions sorted into what jump can set (date, time, flags, values) and what it cannot."""

    def __init__(self, text="", procedure=""):
        self.date_tests = []    # (description, fn(date) -> bool)
        self.time_tests = []    # (description, fn(t) -> bool)
        self.flags = {}         # flag -> wanted 0/1
        self.values = []        # (kind, key, op, v)
        self.floor = None       # a Tartarus floor (DUNGEON_GET_FLOORNO)
        self.choices = []       # (SEL id, option) the path takes
        self.unknown = []
        self.alts = []          # disjunctions: [Gates, ...] each
        for term in split_top(text or "", "&"):
            self.add(term)
        # The day schedulers' procedures are named for the day and time they run: sdl05_25_LunchTime_243.
        m = re.match(r"sdl(\d\d)_(\d\d)_([A-Za-z]+)", procedure or "")
        if m and not any(d.startswith("on ") for d, _ in self.date_tests):
            self.add(f"on {int(m.group(1))}/{int(m.group(2))} {m.group(3)}")

    def merge(self, o):
        self.date_tests += o.date_tests
        self.time_tests += o.time_tests
        self.flags.update(o.flags)
        self.values += o.values
        self.floor = o.floor or self.floor
        self.choices += o.choices
        self.unknown += o.unknown
        self.alts += o.alts

    def add(self, term):
        t = strip_parens(term.strip())
        if not t:
            return
        alts = split_top(t, "|")
        if len(alts) > 1:
            self.alts.append([Gates(a) for a in alts])
            return
        neg = t.startswith("!")
        if neg:
            t = strip_parens(t[1:].strip())
        if not self.parse(t, neg):
            self.unknown.append(("!" if neg else "") + t)

    def parse(self, t, neg):
        sign = "!" if neg else ""
        m = re.fullmatch(r"BIT\s+(\S+)", t)
        if m:
            try:
                self.flags[eval_int(m.group(1))] = 0 if neg else 1
            except ValueError:
                return False
            return True
        m = re.fullmatch(r"COUNT\s+(\d+)\s*(==|!=|>=|<=|>|<)\s*" + NUM, t)
        if m:
            op = NOT[m.group(2)] if neg else m.group(2)
            self.values.append(("count", int(m.group(1)), op, int(m.group(3), 0)))
            return True
        m = re.fullmatch(r"on (\d+)/(\d+) (\w+)", t)
        if m:
            mo, dy, zone = int(m.group(1)), int(m.group(2)), m.group(3)
            self.date_tests.append((sign + t, lambda d, mo=mo, dy=dy, n=neg: ((d.month, d.day) == (mo, dy)) != n))
            if zone in SCHED_TIMES and not neg:
                self.time_tests.append((t, lambda tt, z=SCHED_TIMES[zone]: tt == z))
            return True
        m = re.fullmatch(r"DAY (\d+)/(\d+)", t)
        if m:
            mo, dy = int(m.group(1)), int(m.group(2))
            self.date_tests.append((sign + t, lambda d, mo=mo, dy=dy, n=neg: ((d.month, d.day) == (mo, dy)) != n))
            return True
        m = re.fullmatch(r"DAYS (\d+)/(\d+)-(\d+)/(\d+)", t)
        if m:
            lo, hi = to_date(int(m.group(1)), int(m.group(2))), to_date(int(m.group(3)), int(m.group(4)))
            self.date_tests.append((sign + t, lambda d, lo=lo, hi=hi, n=neg: (lo <= d <= hi) != n))
            return True
        m = re.fullmatch(r"(WEEKDAY|MONTH|MDAY|TIME)\s*(==|!=|>=|<=|>|<)?\s*" + NUM, t)
        if m:
            what, op, v = m.group(1), m.group(2) or "==", int(m.group(3), 0)
            if neg:
                op = NOT[op]
            if what == "TIME":
                self.time_tests.append((sign + t, lambda tt, op=op, v=v: OPS[op](tt, v)))
            else:
                get_ = {"WEEKDAY": lambda d: (d.weekday() + 1) % 7, "MONTH": lambda d: d.month,
                        "MDAY": lambda d: d.day}[what]
                self.date_tests.append((sign + t, lambda d, g=get_, op=op, v=v: OPS[op](g(d), v)))
            return True
        m = re.fullmatch(r"(\w+)\((.*?)\)\s*(?:(==|!=|>=|<=|>|<)\s*" + NUM + r")?", t)
        if not m:
            return False
        name, op, v = m.group(1), m.group(3), (int(m.group(4), 0) if m.group(4) else None)
        try:
            args = [eval_int(a) for a in m.group(2).split(",") if a.strip()]
        except ValueError:
            return False
        if op is None:   # a bare call is "not zero"
            op, v = "!=", 0
        if neg:
            op = NOT[op]
        if name == "CHK_TIME":
            self.time_tests.append((sign + t, lambda tt, a=args[0], op=op, v=v: OPS[op](int(tt == a), v)))
        elif name == "CHK_DAYOFWEEK":
            self.date_tests.append((sign + t, lambda d, a=args[0], op=op, v=v: OPS[op](int((d.weekday() + 1) % 7 == a), v)))
        elif name in ("SEL", "MSG_SEL"):
            self.choices.append((args[0], v if op == "==" else f"{op} {v}"))
        elif name == "CMM_GET_LV":
            self.values.append(("cmm", args[0], op, v))
        elif name == "GET_ITEM_NUM":
            self.values.append(("item", args[0], op, v))
        elif name == "GET_MONEY":
            self.values.append(("money", 0, op, v))
        elif name == "IS_PARTY_EXISTS":
            self.values.append(("party", args[0], op, v))
        elif name == "DUNGEON_GET_FLOORNO" and op == "==":
            self.floor = v
        else:
            return False
        return True

    def resolve(self):
        """Pick one alternative of every disjunction: the first that is fully settable and leaves a date and time that
        pass."""
        for alts in self.alts:
            for a in sorted(alts, key=lambda g: len(g.unknown)):
                trial = Gates()
                trial.merge(self)
                trial.alts = []
                trial.merge(a)
                if not a.unknown and trial.pick_date() is not None and trial.pick_time() is not None:
                    self.merge(a)
                    break
            else:
                self.unknown.append("one of (" + " | ".join(describe(a) for a in alts) + ")")
        self.alts = []

    def date_ok(self, d):
        return all(f(d) for _, f in self.date_tests)

    def pick_date(self, prefer=None):
        """The first school day passing the date tests (prefer: a (month, day) to keep when it passes)."""
        if prefer and prefer[0]:
            pd = to_date(*prefer)
            if self.date_ok(pd):
                return pd
        for d in school_days():
            if self.date_ok(d):
                return d
        return None

    def pick_time(self, prefer=None):
        ok = [t for t in range(8) if all(f(t) for _, f in self.time_tests)]
        if prefer in ok:
            return prefer
        for t in (4, 5, 3, 1, 2, 0, 6, 7):   # free time first: after school, evening, then the rest
            if t in ok:
                return t
        return None

    def solve_value(self, kind, key, cur):
        tests = [(o, v) for k, kk, o, v in self.values if (k, kk) == (kind, key)]
        if cur is not None and all(OPS[o](cur, v) for o, v in tests):
            return cur
        base = {v for _, v in tests}
        for c in sorted(base | {v + 1 for v in base} | {max(v - 1, 0) for v in base} | {0, 1}):
            if all(OPS[o](c, v) for o, v in tests):
                return c
        return None


def describe(g):
    parts = [x for x, _ in g.date_tests + g.time_tests]
    parts += [f"{'' if v else '!'}BIT {flag_name(f)}" for f, v in g.flags.items()]
    parts += [f"{k} {kk} {o} {v}" for k, kk, o, v in g.values]
    parts += g.unknown
    return " & ".join(parts)


def eval_int(s):
    s = s.strip().replace(" ", "")
    if not re.fullmatch(r"[0-9a-fA-Fx+\-()]+", s):
        raise ValueError(s)
    return int(eval(s, {"__builtins__": {}}))   # literal sums such as (171+0x20000000)


def to_date(m, d):
    return datetime.date(YEAR if m >= 4 else YEAR + 1, m, d)


# ---- saves ----------------------------------------------------------------------------------------------------------
def school_index(m, d):
    return (to_date(m, d) - datetime.date(YEAR, 4, 1)).days


def nearest_saves(m, d):
    """Dev slots 8-15 and the pack saves at or before M/D (latest first), from the save files themselves."""
    try:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import p3save
    except ImportError:
        return []
    import glob
    import contextlib
    import io
    out = []
    steam = glob.glob(os.path.join(os.environ.get("APPDATA", ""), "SEGA", "P3R", "Steam", "*"))
    files = [(f"dev slot {n}", os.path.join(steam[0], f"SaveData{n:03d}.sav")) for n in range(8, 16)] if steam else []
    files += [(f"pack {os.path.basename(f)}", f) for f in sorted(glob.glob(os.path.join(ROOT, "build", "saves", "nodlc", "*.sav")))]
    for label, f in files:
        if not os.path.exists(f):
            continue
        buf = io.StringIO()
        try:
            with contextlib.redirect_stdout(buf):
                p3save.info(open(f, "rb").read())
        except Exception:
            continue
        text = buf.getvalue()
        mm, dd = re.search(r"Month: (\d+)", text), re.search(r"Day: (\d+)", text)
        if mm and dd:
            out.append((school_index(int(mm.group(1)), int(dd.group(1))), f"{label} ({mm.group(1)}/{dd.group(1)})"))
    target = school_index(m, d)
    return [lbl for i, lbl in sorted(out, reverse=True) if i <= target][:3]


# ---- jump to a row --------------------------------------------------------------------------------------------------
def jump(row, bare=False, dry=False, walk_text=None, play_secs=180):
    print("row " + row_line(row))
    g = Gates(row.get("gates", ""), row.get("procedure", ""))
    g.resolve()
    who, where, when = placement(row)
    if when is not None and not g.time_tests:
        g.time_tests.append((f"{who or 'the trigger'} stands there at {TIME_NAMES[when]}", lambda tt, w=when: tt == w))
    plan = []   # (description, fn)
    m0, d0, t0 = (None, None, None) if dry else now()
    if g.date_tests or g.time_tests:
        day = g.pick_date((m0, d0))
        tt = g.pick_time(t0)
        if day is None or tt is None:
            print("gates: no date / time passes " + " & ".join(x for x, _ in g.date_tests + g.time_tests))
            return False
        why = " & ".join(dict.fromkeys(x for x, _ in g.date_tests + g.time_tests))
        plan.append((f"date {day.month}/{day.day} {TIME_NAMES[tt]}  (meets {why})",
                     lambda: set_date(day.month, day.day, tt)))
    for f, v in g.flags.items():
        plan.append((f"flag {flag_name(f)} = {v}  (meets {'' if v else '!'}BIT {flag_name(f)})",
                     lambda f=f, v=v: apply_set(f"flag:{f}={v}")))
    for kind, key in sorted({(k, kk) for k, kk, _, _ in g.values}):
        desc, read, write = SETTABLE[kind]
        cur = None if dry else read(key)
        v = g.solve_value(kind, key, cur)
        tests = " & ".join(f"{o} {x}" for k, kk, o, x in g.values if (k, kk) == (kind, key))
        name = desc.format(key)
        if v is None:
            print(f"gates: no value of {name} passes {tests}")
            return False
        if v != cur:
            plan.append((f"{name} = {v}  (was {cur}; meets {tests})", lambda w=write, key=key, v=v: w(key, v)))
        else:
            plan.append((f"{name} stays {cur}  (meets {tests})", lambda: True))
    field = (299, g.floor) if g.floor else where
    target = walk_text or who
    if field and not bare:
        plan.append((f"field {field[0]} {field[1]}" + (" (Tartarus floor)" if g.floor else ""),
                     lambda: p3.field(*field) or print("  the field did not come up")))
    plan.append(trigger(row, bare, target))
    for sel, opt in g.choices:
        print(f"the path takes SEL {sel} option {opt}" + (f" (the {ordinal(opt)} answer)" if isinstance(opt, int) else ""))
    for u in g.unknown:
        print(f"gate not set by jump: {u}")
    for desc, _ in plan:
        print(("would: " if dry else "plan: ") + desc)
    if dry:
        return True
    t_start = time.time()
    for desc, fn in plan:
        print(f"[{time.time() - t_start:5.1f}s] {desc}")
        if fn() is False:
            print("  failed")
    if play_secs:
        print(f"[{time.time() - t_start:5.1f}s] play: " + play(g.choices, play_secs))
    if g.date_tests:
        saves = nearest_saves(day.month, day.day)
        if saves:
            print("if the trigger does not appear, the save may lack the story progress; nearest saves at or before "
                  f"{day.month}/{day.day}: " + ", ".join(saves) + " (`p3.py up --slot N`; a pack save goes in with "
                  "`p3save.py install`, which backs the slot up first)")
    return True


def ordinal(n):
    return ["first", "second", "third", "fourth", "fifth"][n] if 0 <= n < 5 else f"#{n + 1}"


ACTORS = []   # the actor names of the row's person or object (placement() fills it)
PLACE_TIMES = {"Night_Only": 5, "Evening_Only": 4, "Noon_Only": 2, "Shadow_Only": 6}


def placement(row):
    """Where the row's person or object stands: (name, (major, minor) or None, time or None). The first placement that
    is not a key-free event's copy (KeyFree_Event_N), else the first; a field from the row's own field column / owner
    when there is no data row."""
    owner, name, places, key = row.get("owner", ""), None, [], None
    m = re.match(r"NPC (\d+) variant (\d+)", owner)
    if m:
        key = ("npc", f"{m.group(1)}_{m.group(2)}")
    m = re.match(r"Social Link NPC (\d+) variant (\d+)", owner)
    if m:
        key = ("npc", f"cmmu {m.group(1)}_{m.group(2)}")
    m = re.match(r"object (\S+)(?: in field (\d+) (\d+))?", owner)
    if m:
        key = ("obj", f"{m.group(2)}_{m.group(3)}_{m.group(1)}" if m.group(2) else m.group(1))
    if key:
        r = data_get(*key)
        if r:
            name = r["name"] or None
            for p_ in r.get("extra", "").split(" ; "):
                fm = re.search(r"field (\d+) (\d+)", p_)
                tm = re.search(r"time (\S+)", p_)
                am = re.search(r"actor (\S+)", p_)
                if am:
                    ACTORS.append(am.group(1))
                if fm:
                    places.append(((int(fm.group(1)), int(fm.group(2))), tm.group(1) if tm else ""))
    places.sort(key=lambda pl: pl[1].startswith("KeyFree"))
    if places:
        field, t = places[0]
        return name, field, PLACE_TIMES.get(t)
    for text in (row.get("field", ""), owner):
        fm = re.search(r"(\d{3})\s+(\d{3})", text)
        if fm:
            return name, (int(fm.group(1)), int(fm.group(2))), None
    return name, None, None


def reach(target):
    """Walk to the point of interest and press. When it is not listed (out of range, or the door model keeps it out),
    teleport beside the script's own actor first, then walk the last steps."""
    if p3.goto(target):
        return True
    for actor in ACTORS:
        for line in p3.http(f"/ue/find?match={actor}").splitlines():
            if line.split(".")[-1] == actor:
                obj = line.split()[0]
                print(f"  {target!r} is not listed from here: teleporting beside {actor} (#{obj})")
                p3.http(f"/tp?obj={obj}")
                time.sleep(2)
                if p3.goto(target):
                    return True
    print(f"  no point of interest {target!r}")
    return False


def data_get(table, id_):
    for r in data_rows():
        if r.get("table") == table and r.get("id") == id_:
            return r
    return None


def trigger(row, bare, target):
    command, args = row.get("command", ""), row.get("args", "")
    try:
        iargs = [eval_int(a) for a in args.split(",") if a.strip()]
    except ValueError:
        iargs = None
    if not bare and target:
        return (f"walk to {target!r} and press", lambda: reach(target))
    if iargs is not None and command:
        how = "" if bare else "  (no person or object to walk to: called directly)"
        return (f"call {command}({', '.join(map(str, iargs))}){how}", lambda: cmd(command, *iargs))
    return (f"nothing to call: {command}({args}) has script values; walk there (--walk TEXT) or use --set",
            lambda: True)


# ---- playing the scene: messages and the path's choices -------------------------------------------------------------
LOCK = os.path.join(ROOT, "build", "focus.lock")


class FocusLock:
    """build/focus.lock: one agent at a time may bring a game to the front (a choice takes Enter only while focused)."""

    def __enter__(self):
        t0 = time.time()
        while True:
            try:
                fd = os.open(LOCK, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
                os.write(fd, f"{os.getpid()} port {p3.PORT}".encode())
                os.close(fd)
                return self
            except FileExistsError:
                if time.time() - os.path.getmtime(LOCK) > 180:   # a holder that died
                    os.remove(LOCK)
                    continue
                if time.time() - t0 > 300:
                    sys.exit("build/focus.lock is held (another game is being driven)")
                time.sleep(1)

    def __exit__(self, *a):
        try:
            os.remove(LOCK)
        except OSError:
            pass


def play(choices, secs=120, quiet_polls=4):
    """Advance message pages (Enter; subtitles and movies run on their own) and answer each choice: the path's option
    for that choice when known (choices: [(sel id, option)] in path order), else the first. Stops after secs, at a
    modelled screen, or when nothing has been on screen for quiet_polls polls after something was."""
    wanted = [o for _, o in choices if isinstance(o, int)]
    cur = int(p3.http("/speech?since=999999999").split()[1])
    t0, quiet, seen = time.time(), 0, False
    while time.time() - t0 < secs:
        time.sleep(1.5)
        out = p3.http(f"/speech?since={cur}").splitlines()
        cur = int(out[0].split()[1])
        for l in out[1:]:
            print("  said: " + (l.split(": ", 1)[1] if ": " in l else l))
        m = p3.http("/msg")
        sel = re.search(r"sel index=(\d+) count=(\d+)", m)
        if sel:
            seen, quiet = True, 0
            opt = wanted.pop(0) if wanted else 0
            print(f"  choice: answering option {opt} ({ordinal(opt)})")
            with FocusLock():
                p3.focus()
                time.sleep(0.8)
                for _ in range(opt):
                    p3.http("/key?k=down&ms=120")
                    time.sleep(0.4)
                p3.http("/key?k=enter&ms=120")
                time.sleep(1.5)
            continue
        scr = p3.http("/screen").splitlines()[0]
        if "screen=movie" in scr or "item=MsgSubtitles" in m:
            seen, quiet = True, 0
            continue
        if any(s in scr for s in ("screen=get_item", "screen=stats_up", "screen=rank_up", "screen=battle_result")):
            seen, quiet = True, 0
            p3.http("/key?k=space&ms=120")
            continue
        if "item=Msg" in m:
            seen, quiet = True, 0
            p3.http("/key?k=enter&ms=120")
            continue
        if "screen=none" not in scr:
            print("  -- " + scr + ": stopping here")
            return "screen"
        quiet += 1
        if any(" running " in l for l in p3.http("/flow/run").splitlines()):
            quiet = 0   # an event (or battle) started through /flow/run has not finished: its quiet stretches count not
            continue
        if seen and quiet >= quiet_polls:
            return "quiet"
        if not seen and quiet >= 3 * quiet_polls:
            return "nothing on screen"
    return "time"


# ---- main -----------------------------------------------------------------------------------------------------------
def main(argv, p3mod):
    global p3
    p3 = p3mod
    if not argv or argv[0] in ("-h", "--help", "help"):
        print(__doc__)
        return
    bare = "--bare" in argv
    dry = "--dry" in argv
    walk = None
    if "--walk" in argv:
        i = argv.index("--walk")
        walk = argv[i + 1]
        del argv[i:i + 2]
    play_secs = 180
    if "--play" in argv:
        i = argv.index("--play")
        play_secs = float(argv[i + 1])
        del argv[i:i + 2]
    if "--noplay" in argv:
        play_secs = 0
    argv = [a for a in argv if a not in ("--bare", "--dry", "--noplay")]
    if argv[0] == "--list":
        list_rows(" ".join(argv[1:]))
        return
    sets, rest = [], argv
    if argv[0] == "--set":
        sets = [a for a in argv[1:] if "=" in a]
        rest = [a for a in argv[1:] if "=" not in a]
    t0 = time.time()
    for s in sets:
        if apply_set(s) is False:
            print("  (not as asked)")
    if sets and not rest and "--play" in sys.argv:
        print(f"[{time.time() - t0:5.1f}s] play: " + play([], play_secs))
    if rest:
        query = " ".join(rest)
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import scenarios
        if query in scenarios.SCENARIOS:
            p3.scenario(query)
            return
        if not rows():
            no_index()
            sys.exit(1)
        hits = search(query)
        if not hits:
            sys.exit(f"no index row matches {query!r} (`p3.py jump --list ...`)")
        if len(hits) > 1:
            print(f"{len(hits)} rows match; jumping to the first (give a row id to choose):")
            for r in hits[:8]:
                print("  " + row_line(r))
        jump(hits[0], bare=bare, dry=dry, walk_text=walk, play_secs=play_secs)
    print(f"[{time.time() - t0:5.1f}s] jump done")

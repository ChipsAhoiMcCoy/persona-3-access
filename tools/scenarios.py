"""Test situations reached directly, without playing to them (`python tools/p3.py scenario <name>`).

Each scenario runs from a dev game already in a daily field (any save; `p3.py up --slot N` first if not). A step is
(call, args..., {kwargs}) on one of p3.py's helpers:
  cheat(fn, **ints)    UAppCheatManager: Atlus's debug commands, live in this build. AtlSetDayTime(Month, Day, Time:
                       0 morning, 1 AM, 2 noon, 3 PM, 4 after school, 5 evening, 6 Dark Hour, 7 midnight);
                       AtlCallEvent / AtlCallEventCmmu / AtlCallEventFild(EventMajorID, EventMinorID);
                       AtlBtlCountdownPlay; AtlAutoPlayExecShuffleTimeTest; AtlAutoPlayExecSkillMutation*.
  gwork(fn, **ints)    GlobalWorkBpLib: GWorkFlagOnValue / GWorkFlagOffValue(Index = section << 28 | bit),
                       GWorkSetCounterValue(CntNo, Value), GWorkSetMoney(SetMoeny).
  field(major, minor)  load a field area (CALL_FIELD) and wait for it: 102 121 Your Room, 102 102 lounge,
                       102 101 dorm entrance, 103 101 Paulownia Mall, 105 102 strip mall, 299 N Tartarus floor N.
  goto(text)           walk to a point of interest and press Space (the game's own check runs, gates included).
  advance(secs, stop_at=...)   play through messages; stops at a choice or a screen.
  keys(k, "wait:2", ...)  game keys.
  focus()              bring the dev game to the front: a choice list (SEL) takes Enter only while it is focused.
  http(route)          any dev route.
A date set live lasts until the game's next day change: test within the day.
Where the game has a script that leads to the thing (a hit, an NPC), prefer walking to it over calling the event
bare: the script sets up what the event expects, and it is what the player meets.
Ids come from the game's flowscripts (the procedure that leads there, its BIT / COUNT gates). A script's
BIT_CHK((156 + 0)) is index 156; BIT_CHK((171 + 0x20000000)) is 0x200000AB: the added constant is the section.
"""

SCENARIOS = {
    "tv-shop": {
        "about": "TV shopping show in Your Room (Sunday 5/3, afternoon), through the TV itself",
        "steps": [
            ("cheat", "AtlSetDayTime", {"Month": 5, "Day": 3, "Time": 3}),
            ("gwork", "GWorkFlagOffValue", {"Index": 156}),   # BIT_CHK(156 + 0): this week's show watched
            ("field", 102, 121),
            ("focus",),
            ("goto", "TV"),
            ("advance", 240, {"key": "space"}),
        ],
        "check": "the 'Buy / Don't buy' choice with the Pay money panel; Enter must buy",
    },
    "ending-choice": {
        "about": "the ending's last choice ('Close your eyes'), event MAIN 220 040",
        "steps": [
            ("gwork", "GWorkFlagOnValue", {"Index": 0x40000095}),
            ("gwork", "GWorkFlagOnValue", {"Index": 0x40000096}),
            ("focus",),
            ("cheat", "AtlCallEvent", {"EventMajorID": 220, "EventMinorID": 40}),
            ("advance", 600),
        ],
        "check": "the two answers side by side; Enter must be taken with no Left / Right first",
    },
    "countdown": {
        "about": "the battle countdown (start a battle first: `p3.py get strike` near a Shadow)",
        "steps": [
            ("cheat", "AtlBtlCountdownSetTime", {"InSecond": 30}),
            ("cheat", "AtlBtlCountdownPlay"),
        ],
        "check": "the countdown is announced",
    },
}

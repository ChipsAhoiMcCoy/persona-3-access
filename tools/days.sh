#!/bin/sh
# Dev helper: pass days until a date (story scenes are played through, first choice each time; free time goes home
# and to bed). Usage: sh tools/days.sh <month> <day>
export MSYS_NO_PATHCONV=1
p() { python tools/p3.py "$@"; }
tm=$1; td=$2
lock=build/days.lock
[ -e "$lock" ] && kill -0 "$(cat "$lock")" 2>/dev/null && { echo "days.sh already running"; exit 1; }
echo $$ > "$lock"; trap 'rm -f "$lock"' EXIT
for i in $(seq 1 40); do
  md=$(p get "ue/call?cdo=CldBpFuncLib&fn=GetNowTotalToDayMon" | grep -E "^  (Month|Day) =" | awk '{print $3}' | tr -d '\r' | tr '\n' ' ')
  set -- $md
  echo "== $1/$2 $(p get 'ue/call?cdo=CldBpFuncLib&fn=GetNowTimeZone' | tail -1 | awk '{print $3}')"
  [ "$1" -gt "$tm" ] || { [ "$1" -eq "$tm" ] && [ "$2" -ge "$td" ]; } && { echo "reached"; break; }
  timeout 500 python tools/p3.py advance 120 --choose space | tail -2
  case "$(p get screen | head -1)" in
    *battle*) sh tools/fight.sh 60 | tail -1;;
    *mail*) p get "key?k=space&ms=150" >/dev/null; sleep 3
            timeout 300 python tools/p3.py advance 90 --choose space | tail -2;;   # a morning text: read, accept
  esac
  # P3_SAVE=N: save to slot N each day before bed (H opens the list on the last slot used; overwrite confirms).
  if [ -n "$P3_SAVE" ] && p get world | grep -q "^player=BP"; then
    p get "key?k=h&ms=150" >/dev/null; sleep 3
    if p get screen | grep -q "Slot $P3_SAVE[:,]"; then
      p get "key?k=space&ms=150" >/dev/null; sleep 2; p get "key?k=space&ms=150" >/dev/null; sleep 3
    fi
    p get "key?k=c&ms=150" >/dev/null; sleep 2
  fi
  timeout 500 python tools/p3.py flow sleep | tail -1
  sleep 3; p get msg | grep -q "Go to bed" && p get "key?k=space&ms=150" >/dev/null
  sleep 5
done
p get speech | tail -5

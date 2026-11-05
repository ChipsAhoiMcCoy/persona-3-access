#!/bin/sh
# Dev helper: play a battle with plain attacks (Space on the command wheel), advancing messages and result pages.
# Usage: sh tools/fight.sh [max_steps=60]
# P3_WEAK=1: every enemy is set to 1 HP when the battle starts (dev saves only: pushes the story past bosses).
export MSYS_NO_PATHCONV=1
p() { python tools/p3.py "$@"; }
n=${1:-60}; idle=0
for i in $(seq 1 $n); do
  sleep 2
  s=$(p get /screen | head -1)
  # a subtitle advances itself: Enter on one pauses the movie
  if p get /msg | grep "item=Msg" | grep -vq "item=MsgSubtitles"; then p get "/key?k=enter&ms=120" >/dev/null; idle=0; continue; fi
  case "$s" in
    *battle_result*) p get "/key?k=space&ms=120" >/dev/null; idle=0;;
    *battle*) if [ -n "$P3_WEAK" ] && [ -z "$weakened" ]; then
                DB=$(p get '/ue/find?class=BtlDataBaseComponent' | head -1 | cut -d' ' -f1)
                for e in $(p get '/ue/find?class=BP_BtlShadowBase_C' | cut -d' ' -f1); do p get "/ue/call?obj=$DB&fn=SetUnitHP&args=Actor:obj:$e,Hp:int:1" >/dev/null; done
                weakened=1
              fi
              G=$(p get '/ue/find?class=BtlGuiStateManager' | head -1 | cut -d' ' -f1)
              p get "/ue/obj?obj=$G" | grep -q "nowState: EnumProperty = BTL_STATE_TOP" && p get "/key?k=space&ms=120" >/dev/null; idle=0;;
    *) idle=$((idle+1)); [ $idle -gt 6 ] && p get /world | grep -q "^player=BP" && break;;
  esac
done

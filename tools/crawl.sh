#!/bin/sh
# Dev helper: clear Tartarus floors unattended. Fights the nearest Shadow (strikes it first for the advantage), breaks
# crystals, opens chests, then takes the stairs up. Battles are played with Rush (Tab), messages are advanced.
# Usage: sh tools/crawl.sh [max_steps=200] [floors=1]
export MSYS_NO_PATHCONV=1
p() { python tools/p3.py "$@"; }
n=${1:-200}; floors=${2:-1}; climbed=0; target=""; skip=""; last=""; tries=0
gui() { G=$(p get 'ue/find?class=BtlGuiStateManager' | head -1 | cut -d' ' -f1); [ -n "$G" ] && p get "ue/obj?obj=$G" | grep -o "nowState: EnumProperty = [A-Z_]*" | sed 's/.*= //'; }
for i in $(seq 1 $n); do
  sleep 1.5
  s=$(p get screen | head -1)
  # a subtitle advances itself: Enter on one pauses the movie
  if p get msg | grep "item=Msg" | grep -vq "item=MsgSubtitles"; then p get "key?k=enter&ms=120" >/dev/null; continue; fi
  case "$s" in
    *battle_result*|*get_item*|*stats_up*|*rank_up*) p get "key?k=space&ms=120" >/dev/null; continue;;
    *battle*) [ "$(gui)" = "BTL_STATE_TOP" ] && p get "key?k=tab&ms=120" >/dev/null; target=""; continue;;
    *none*) ;;
    *movie*) continue;;   # never Enter on a movie (it pauses)
    *) echo "screen: $s"; p get "key?k=enter&ms=120" >/dev/null; continue;;
  esac
  # Field: still walking to something?
  if p get nav | grep -q "walking=true"; then
    # A Shadow within reach: strike it before it turns.
    sh=$(p get "poi?r=350" | grep "Shadow" | head -1)
    [ -n "$sh" ] && { p get "nav?stop=1" >/dev/null; p get strike >/dev/null; }
    continue
  fi
  list=$(p get "poi?r=20000")
  for b in $skip; do list=$(echo "$list" | grep -v "^#$b "); done
  pick=$(echo "$list" | grep "Shadow" | head -1)
  [ -z "$pick" ] && pick=$(echo "$list" | grep -E "Breakable object|Treasure Chest" | head -1)
  if [ -z "$pick" ]; then
    pick=$(echo "$list" | grep -i "Head Up exit" | head -1)
    if [ -z "$pick" ]; then idle=$((${idle:-0}+1)); [ $idle -gt 8 ] && { echo "nothing left"; break; }; continue; fi
  fi
  idle=0
  id=$(echo "$pick" | cut -d' ' -f1 | tr -d '#')
  if [ "$id" = "$target" ]; then
    # Arrived (or stuck) at it: act.
    case "$pick" in
      *Shadow*|*Breakable*) p get strike >/dev/null;;
      *Chest*) p get "key?k=space&ms=120" >/dev/null;;
      *exit*) p get "key?k=space&ms=120" >/dev/null; climbed=$((climbed+1)); sleep 4
              [ $climbed -ge $floors ] && { echo "climbed $climbed"; break; };;
    esac
    target=""
    continue
  fi
  echo "-> $pick"
  # The same target five times: give up on it.
  [ "$id" = "$last" ] && tries=$((tries+1)) || tries=0
  last=$id
  [ $tries -ge 5 ] && { skip="$skip $id"; tries=0; continue; }
  target=$id
  p get "nav?walk=$id" >/dev/null
done
p get speech | tail -15

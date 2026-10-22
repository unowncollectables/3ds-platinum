#!/usr/bin/env bash
# Headless Azahar smoke test for platinum3ds .3dsx
# Usage: emu_smoke.sh <seconds> <outdir>
# Env overrides:
#   SHOTS="5 10 20"        screenshot times (s since launch; default: every SHOT_EVERY s)
#   SHOT_EVERY=5
#   KEYS="8:a 12:Return 15:a" key presses at time:key (3DS buttons below)
#   RENDERER=gl|sw          gl = OpenGL on Mesa llvmpipe (default, faster), sw = Azahar software renderer
#   TDSX=path SAVE=path NDS=path CONFIG=path
# 3DS button -> key name for KEYS: A=a B=s X=z Y=x L=q R=w ZL=1 ZR=2 Start=m Select=n
#   Dpad Up=t Down=g Left=f Right=h  Home=b  CirclePad=Up/Down/Left/Right (arrows)
#   Hold/combos: KEYS="40:Up@2000 50:q+w+n@300" (keys joined by +, hold ms after @)
#   PRESS_BG=1: key holds run in the background so screenshots can be taken during a hold
#   Touch: KEYS="10:touch:160,120" (bottom-screen coords 0..319,0..239); hold: "10:touch:8,8@800"
set -u
SECS=${1:?seconds}; OUT=${2:?outdir}
P=/workspace/platinum3ds
TDSX=${TDSX:-$P/platinum3ds/full/platinum3ds_full.3dsx}
SAVE=${SAVE:-$P/test_saves/run22_platinum.sav}
NDS=${NDS:-$P/baserom/platinum_us_rev1.nds}
CONFIG=${CONFIG:-}
RENDERER=${RENDERER:-gl}
SHOT_EVERY=${SHOT_EVERY:-5}
AZ=/opt/azahar/squashfs-root/AppRun
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
W=$OUT/emu; rm -rf "$W"; mkdir -p "$W/cfg/azahar-emu" "$W/data/azahar-emu/sdmc/3ds/platinum3ds"
SD=$W/data/azahar-emu/sdmc/3ds/platinum3ds
# Read-only snapshot of the build so concurrent rebuilds don't race us
cp "$TDSX" "$W/game.3dsx"
ln -s "$NDS" "$SD/platinum.nds"
[ -f "$SAVE" ] && cp "$SAVE" "$SD/platinum.sav"
FOLBIN=${FOLBIN-$P/platinum3ds/full/followers.bin}; [ -n "$FOLBIN" ] && [ -f "$FOLBIN" ] && ln -s "$FOLBIN" "$SD/followers.bin"
[ -f "$P/platinum3ds/full/emotes.bin" ] && ln -s "$P/platinum3ds/full/emotes.bin" "$SD/emotes.bin"
if [ -n "$CONFIG" ]; then cp "$CONFIG" "$SD/config.txt"
elif [ -f "$P/test_saves/config.txt" ]; then cp "$P/test_saves/config.txt" "$SD/config.txt"; fi
api=1; [ "$RENDERER" = sw ] && api=0
cat > "$W/cfg/azahar-emu/qt-config.ini" <<INI
[Core]
use_cpu_jit=true
[System]
is_new_3ds=true
is_new_3ds\\default=false
[Data%20Storage]
use_virtual_sd=true
sdmc_directory=$W/data/azahar-emu/sdmc/
sdmc_directory\\default=false
[Renderer]
graphics_api=$api
graphics_api\\default=false
use_vsync=false
use_vsync\\default=false
frame_limit=100
resolution_factor=1
resolution_factor\\default=false
${EXTRA_RENDERER:-}
[Layout]
layout_option=0
[Audio]
output_type=1
output_type\\default=false
[Miscellaneous]
check_for_update_on_start=false
check_for_update_on_start\\default=false
[UI]
confirmClose=false
confirmClose\\default=false
firstStart=false
firstStart\\default=false
fullscreen=true
fullscreen\\default=false
[WebService]
enable_telemetry=false
enable_telemetry\\default=false
INI
DN=$(( 90 + RANDOM % 400 ))
while [ -e /tmp/.X11-unix/X$DN ]; do DN=$((DN+1)); done
Xvfb :$DN -screen 0 400x480x24 -nolisten tcp >"$OUT/xvfb.log" 2>&1 & XP=$!
sleep 1
export DISPLAY=:$DN XDG_CONFIG_HOME=$W/cfg XDG_DATA_HOME=$W/data LIBGL_ALWAYS_SOFTWARE=1 QT_QPA_PLATFORM=xcb
"$AZ" -f "$W/game.3dsx" >"$OUT/azahar_stdout.log" 2>&1 & EP=$!
start=$(date +%s%3N)
# No WM under Xvfb: Qt places the fullscreen window off-origin; pin it to 0,0
( for i in $(seq 1 100); do wid=$(xdotool search --name "^Azahar [0-9]" 2>/dev/null | head -1)
    if [ -n "$wid" ]; then for r in 1 2 3 4 5 6; do xdotool windowmove "$wid" 0 0; sleep 1; done; break; fi; sleep 0.2; done ) &
el(){ echo $(( $(date +%s%3N) - start )); }   # ms
shot(){ local t=$1 f="$OUT/t$(printf %06.2f "$1")"
  import -display :$DN -window root "$f.full.png" 2>/dev/null || return
  convert "$f.full.png" -crop 400x240+0+0 +repage "$f.top.png"
  convert "$f.full.png" -crop 320x240+40+240 +repage "$f.bottom.png"
  echo "shot t=$t -> $f.{top,bottom}.png"; }
press(){ local k=$1
  if [[ $k == touch:* ]]; then local tk=${k#touch:} td=150; [[ $tk == *@* ]] && { td=${tk##*@}; tk=${tk%@*}; }; IFS=, read x y <<<"$tk"
    xdotool mousemove $((x+40)) $((y+240)) mousedown 1; sleep "$(awk "BEGIN{print $td/1000}")"; xdotool mouseup 1
  else wid=$(xdotool search --pid $EP 2>/dev/null | tail -1); [ -n "$wid" ] && xdotool windowfocus "$wid" 2>/dev/null
    # hold/combos: "q+w+n" presses together, "@ms" sets hold time (default 150), e.g. 40:Up@2000 or 30:q+w+n@300
    local dur=150 ks=$k; [[ $k == *@* ]] && { dur=${k##*@}; ks=${k%@*}; }
    IFS=+ read -ra arr <<<"$ks"
    for x in "${arr[@]}"; do xdotool keydown "$x"; done; sleep "$(awk "BEGIN{print $dur/1000}")"
    for ((j=${#arr[@]}-1;j>=0;j--)); do xdotool keyup "${arr[j]}"; done; fi
  echo "press t=$(el)ms $k"; }
SHOTS=${SHOTS:-$(seq $SHOT_EVERY $SHOT_EVERY $SECS | tr '\n' ' ')}
events=$( { for s in $SHOTS; do echo "$s shot $s"; done; for k in ${KEYS:-}; do echo "${k%%:*} key ${k#*:}"; done; } | sort -n )
while read -r t typ arg; do
  [ -z "$t" ] && continue
  tm=$(awk "BEGIN{print int($t*1000)}"); while (( $(el) < tm )); do sleep 0.1; kill -0 $EP 2>/dev/null || break 2; done
  if [ "$typ" = shot ]; then shot "$t"; elif [ -n "${PRESS_BG:-}" ]; then press "$arg" & else press "$arg"; fi
done <<<"$events"
while (( $(el) < SECS*1000 )) && kill -0 $EP 2>/dev/null; do sleep 0.2; done
kill -0 $EP 2>/dev/null && alive=yes || alive="no (exited early)"
shot "$SECS" >/dev/null; kill $EP 2>/dev/null; sleep 2; kill -9 $EP 2>/dev/null; pkill -9 -f "$W/game.3dsx" 2>/dev/null
kill $XP 2>/dev/null; [ -d "$SD/seq" ] && mv "$SD/seq" "$OUT/seq"
cp "$SD/log.txt" "$OUT/log.txt" 2>/dev/null && echo "log.txt -> $OUT/log.txt" || echo "no log.txt written"
cp "$SD/platinum.sav" "$OUT/platinum.sav.after" 2>/dev/null
cp "$W/data/azahar-emu/log/azahar_log.txt" "$OUT/azahar_log.txt" 2>/dev/null
echo "emulator alive at end: $alive"; echo "outputs in $OUT"

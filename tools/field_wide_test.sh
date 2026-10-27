#!/usr/bin/env bash
# run 112: overworld wide-view regression test (Run 111 lost the field wide view on hardware with the slider up).
# Usage: tools/field_wide_test.sh <3dsx> <outdir> <config.txt> <save> [extra config lines...]
# Boots the save into the overworld, screenshots the top screen with the 3D slider at 0 and with stereo_fake=100,
# and fails unless both show the wide 400px view: the 38px side strips must be non-black and not the pillarbox
# border colour (Run 111: 48,32,48), and the log must show the 320x256 wide 3D eye targets with no "320x256 targets
# FAILED" line. Needs Azahar via tools/emu_smoke.sh.
set -u
T=$1; O=$2; CFG=$3; SAV=$4; shift 4
cd "$(dirname "$0")/.."
rc=0
for s in 0 100; do
  c=$(mktemp); cp "$CFG" "$c"; [ "$s" != 0 ] && echo "stereo_fake=$s" >> "$c"; for l in "$@"; do echo "$l" >> "$c"; done
  d="$O/slider$s"; rm -rf "$d"
  TDSX=$T SAVE=$SAV CONFIG=$c SHOTS="50 58" KEYS="14:a 18:a 22:a 26:a 30:a 33:a 36:m 38:m" timeout 120 tools/emu_smoke.sh 60 "$d" >/dev/null 2>&1
  python3 - "$d" "$s" <<'P' || rc=1
import sys, statistics
from PIL import Image
d, s = sys.argv[1], sys.argv[2]
ok = True
for t in ("050", "058"):
    im = Image.open(f"{d}/t{t}.00.top.png").convert("RGB"); w, h = im.size
    for name, x0 in (("left", 0), ("right", w - 38)):
        px = [im.getpixel((x, y)) for x in range(x0, x0 + 38, 2) for y in range(20, h, 3)]
        nb = sum(sum(p) > 40 for p in px) / len(px); border = sum(abs(p[0]-48) + abs(p[1]-32) + abs(p[2]-48) < 24 for p in px) / len(px)
        good = nb > 0.6 and border < 0.3
        print(f"slider {s} t{t} {name} strip: non-black {nb:.2f}, border colour {border:.2f} -> {'WIDE' if good else 'NOT WIDE'}")
        ok &= good
log = open(f"{d}/log.txt", errors="ignore").read()
if "320x256 targets FAILED" in log or "eye targets now 320x256" not in log: print(f"slider {s}: wide 3D targets missing/failed in the log -> NOT WIDE"); ok = False
sys.exit(0 if ok else 1)
P
  grep -E "3DRES\] (320|eye)|wide_gpu target" "$d/log.txt" | head -4
done
echo "field_wide_test: $([ $rc = 0 ] && echo PASS || echo FAIL)"; exit $rc

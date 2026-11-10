#!/usr/bin/env bash
# Rebuild libnetwork-probe.a (libntrdwc + libntrwifi, the game's own Wi-Fi Connection code) in an ALREADY STAGED pspoke tree.
#
# Why: pspoke compiles the network libraries once, in the one-shot "libraries" phase. pspoke-patch 0012 (online network
# libraries: socket backend + SoulSilver's GTS fixes) changes port/native-probe/compile-network.py, but a tree staged before
# it keeps the old copy and the old archive. A fresh `p3ds_build.sh all` does not need this script.
#
# Usage (WSL):  P3DS_ROOT=~/3ds-platinum bash ~/3ds-platinum/scripts/rebuild_network.sh
#   then the normal quick build. The script deletes platinum3ds_full.elf so make relinks the new archive.
# Safe to re-run: it does nothing once the staged copy matches pspoke's port/ copy and a rebuild from it succeeded.
set -euo pipefail
ROOT="${P3DS_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
PS="${PSPOKE:-$ROOT/pspoke}"
W="${W:-$PS/.work/tree/test_out}"
SRC="$PS/port/native-probe/compile-network.py"
NW="$W/native-probe/compile-network.py"
die() { echo "rebuild_network: $*" >&2; exit 1; }
[ -f "$SRC" ] || die "pspoke source copy not found: $SRC (set PSPOKE=)"
[ -f "$NW" ] || die "staged copy not found: $NW (tree not staged yet? a full build compiles the network libraries itself)"
grep -q "SDK_BUILD_LINUX" "$SRC" || die "pspoke has no patch 0012 yet (no SDK_BUILD_LINUX in $SRC); re-apply pspoke-patches first"
if cmp -s "$SRC" "$NW.p3ds-built"; then echo "rebuild_network: already up to date"; exit 0; fi
# The staged copy has @PSPDEV@ filled in by staging; take the value from it.
PSPDEV="$(grep -o "'[^']*/bin/psp-gcc'" "$NW" | head -1 | sed -e "s#^'##" -e "s#/bin/psp-gcc'\$##")"
[ -n "$PSPDEV" ] || die "could not read the toolchain path from the staged compile-network.py"
cp -f "$NW" "$NW.p3ds-orig" 2>/dev/null || true
sed -e "s#@PSPDEV@#$PSPDEV#g" "$SRC" > "$NW"
! grep -q '@[A-Z]\+@' "$NW" || die "unfilled placeholder left in $NW"
python3 "$(dirname "$0")/p3ds_nw_idempotent.py" "$NW" || die "could not make the staged compile-network.py re-run safe"
echo "rebuild_network: compiling libntrdwc/libntrwifi with the socket backend (a few minutes)"
( cd "$W/native-probe" && python3 compile-network.py )
cp -f "$SRC" "$NW.p3ds-built"
rm -f "$ROOT/platinum3ds/full/platinum3ds_full.elf"
echo "rebuild_network: done; run the normal build to relink"

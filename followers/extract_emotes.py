#!/usr/bin/env python3
"""run 71: HGSS follower emote bubbles (fuki_*) from the user's OWN SoulSilver ROM, NARC a/1/0/3 -> emotes.bin (SD runtime
file, never committed). Format (LE): 'P3EM' u32 n, then n x {char name[12], 512 B 32x32 4bpp linear texels of frame 1
(low nibble = left pixel, index 0 = transparent), 32 B BGR555 palette}. Order = EMOTES below (indices used by the port)."""
import struct, sys, os, ndspy.rom, ndspy.narc
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import btx
ROM = sys.argv[1] if len(sys.argv) > 1 else '/workspace/platinum3ds/roms/soulsilver.nds'
OUT = sys.argv[2] if len(sys.argv) > 2 else '/workspace/platinum3ds/platinum3ds/full/emotes.bin'
EMOTES = ['heart', 'onpu', 'bikkuri', 'hatena', 'ase', 'chinmoku', 'pikon', 'komari', 'doku', 'kao1', 'kao2', 'kao3', 'kao4', 'kao5']
narc = ndspy.narc.NARC(ndspy.rom.NintendoDSRom(open(ROM, 'rb').read()).getFileByName('a/1/0/3'))
found = {}
for f in narc.files:
    if f[:4] != b'BTX0': continue
    t, p = btx.parse(f)
    n = t[0]['name'].split('.')[0]
    if n.startswith('fuki_') and t[0]['w'] == 32 and t[0]['fmt'] == 3: found.setdefault(n[5:], (t[0]['data'], p[0]['data'][:32].ljust(32, b'\0')))
# run 72: a/1/0/3 has a 2nd, palette-less fuki_heart (file 28) that overwrote the real one (file 2): keep the first
out = struct.pack('<4sI', b'P3EM', len(EMOTES))
for e in EMOTES:
    d, pl = found.get(e, (b'\0' * 512, b'\0' * 32))
    out += e.encode().ljust(12, b'\0') + d + pl
open(OUT, 'wb').write(out); print(OUT, len(out), 'missing:', [e for e in EMOTES if e not in found])

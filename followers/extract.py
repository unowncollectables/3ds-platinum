#!/usr/bin/env python3
"""run 58: HGSS following-Pokemon sprites from the user's OWN SoulSilver ROM (roms/soulsilver.nds) -> followers.bin.
Nothing is downloaded: the sprite data comes only from the ROM; hgss_luts.py holds the species->model code tables
transcribed from the pokeheartgold decomp source.
Source: NARC a/0/8/1 files 297..862 (MMODEL_FOLLOWER_MON_BULBASAUR + FOLLOWER_MON index), each an NSBTX with 8
4bpp textures (2 frames each: up, down, left, right) and 2 palettes (tsure_poke0 normal, tsure_poke1 shiny).

followers.bin (little endian):
  0  'P3FL'  u32 version=1  u32 nSprites  u32 nSpecies(494, index 0 unused)
  16 species table nSpecies x {u16 model, u8 formMax, u8 femaleFlag}
     sprite dir   nSprites x {u32 offset, u8 w, u8 h, u8 frames, u8 0}
     sprite data  frames x (w*h/2) 4bpp linear texels (low nibble = left pixel), 32 B normal pal, 32 B shiny pal
Sprite index = model (+1 if femaleFlag and female, else +form if form <= formMax), clamped to nSprites-1."""
import struct, sys, os, hashlib, ndspy.rom, ndspy.narc
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import btx, hgss_luts as L
ROM = sys.argv[1] if len(sys.argv) > 1 else '/workspace/platinum3ds/roms/soulsilver.nds'
OUT = sys.argv[2] if len(sys.argv) > 2 else '/workspace/platinum3ds/platinum3ds/full/followers.bin'
raw = open(ROM, 'rb').read()
code = raw[0x0C:0x10].decode('latin1')
assert code in ('IPGE', 'IPKE'), 'not a US HeartGold/SoulSilver ROM: ' + code
rom = ndspy.rom.NintendoDSRom(raw)
narc = ndspy.narc.NARC(rom.getFileByName('a/0/8/1'))
BASE, N = 297, 566
assert len(narc.files) >= BASE + N
species = b''.join(struct.pack('<HBB', L.MODEL_INDEX[s], L.FORM_MAX[s - 1] if s else 0, L.FEMALE_FLAG[s - 1] if s else 0)
                   for s in range(494))
hdr = struct.pack('<4sIII', b'P3FL', 1, N, 494)
off = len(hdr) + len(species) + N * 8
dir_, data = b'', b''
for i in range(N):
    texs, pals = btx.parse(narc.files[BASE + i])
    assert len(texs) == 8 and all(t['fmt'] == 3 for t in texs)
    w, h = texs[0]['w'], texs[0]['h']
    pd = {p['name']: p['data'][:32].ljust(32, b'\0') for p in pals}
    blob = b''.join(t['data'] for t in texs) + pd['tsure_poke0'] + pd.get('tsure_poke1', pd['tsure_poke0'])
    dir_ += struct.pack('<IBBBB', off + len(data), w, h, 8, 0)
    data += blob
out = hdr + species + dir_ + data
open(OUT, 'wb').write(out)
print('ROM', code, 'rev', raw[0x1E], '->', OUT, len(out), 'bytes sha1', hashlib.sha1(out).hexdigest())

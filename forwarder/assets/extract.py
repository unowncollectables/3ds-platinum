#!/usr/bin/env python3
"""run 55: forwarder icon/banner/sound from the user's OWN ROM (baserom/platinum_us_rev1.nds); nothing downloaded.
Icon/banner: poketool/pokegra/pl_otherpoke.narc file 153 (Giratina Origin front, Pt-encrypted NCGR) + palette 247
(shiny Origin). Sound: data/sound/pl_sound_data.sdat WAVE_ARC_PV487 wave 0 (Giratina cry, PCM8 13379 Hz) -> 32 kHz WAV."""
import struct, wave, ndspy.rom, ndspy.narc, ndspy.soundArchive
from PIL import Image
rom = ndspy.rom.NintendoDSRom.fromFile('/workspace/platinum3ds/baserom/platinum_us_rev1.nds')
narc = ndspy.narc.NARC(rom.getFileByName('poketool/pokegra/pl_otherpoke.narc'))
d = narc.files[153]; o = d.find(b'RAHC'); dlen = struct.unpack_from('<I', d, o + 0x18)[0]
v = list(struct.unpack_from('<%dH' % (dlen // 2), d, o + 0x20)); s = v[0]; out = []
for x in v: out.append(x ^ (s & 0xFFFF)); s = (s * 0x41C64E6D + 0x6073) & 0xFFFFFFFF
b = struct.pack('<%dH' % len(out), *out)
p = narc.files[247]; po = p.find(b'TTLP') + 0x18
P = [((c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31) for c in struct.unpack_from('<16H', p, po)]
im = Image.new('RGBA', (80, 80), (0, 0, 0, 0))
for i in range(6400):
    x, y = i % 80, i // 80; j = y * 160 + x; ix = (b[j // 2] >> ((j & 1) * 4)) & 15
    if ix: im.putpixel((x, y), P[ix] + (255,))
im.save('giratina_origin_shiny_front.png'); c = im.crop(im.getbbox()); w, h = c.size
sc = 46 / max(w, h); ic = c.resize((round(w * sc), round(h * sc)), Image.LANCZOS)
icon = Image.new('RGBA', (48, 48), (0, 0, 0, 0)); icon.alpha_composite(ic, ((48 - ic.size[0]) // 2, (48 - ic.size[1]) // 2)); icon.save('icon48.png')
ban = Image.new('RGBA', (256, 128))
for y in range(128):
    for x in range(256): ban.putpixel((x, y), (30 + y // 6, 20 + y // 10, 50 + y // 4, 255))
sc2 = 120 / max(w, h); bc = c.resize((round(w * sc2), round(h * sc2)), Image.LANCZOS)
ban.alpha_composite(bc, ((256 - bc.size[0]) // 2, (128 - bc.size[1]) // 2)); ban.save('banner256.png')
sd = ndspy.soundArchive.SDAT(rom.getFileByName('data/sound/pl_sound_data.sdat'))
wv = [x for n, x in sd.waveArchives if n == 'WAVE_ARC_PV487'][0].waves[0]
pcm = [(q - 256 if q > 127 else q) for q in wv.data]; src, dst = wv.sampleRate, 32000; o2 = []
for i in range(int(len(pcm) * dst / src)):
    t = i * src / dst; a = int(t); f = t - a; x0 = pcm[a] if a < len(pcm) else 0; x1 = pcm[a + 1] if a + 1 < len(pcm) else 0
    o2.append(max(-32768, min(32767, int((x0 + (x1 - x0) * f) * 256 * 0.9))))
o2 += [0] * 3200
with wave.open('giratina_cry.wav', 'wb') as f:
    f.setnchannels(1); f.setsampwidth(2); f.setframerate(dst); f.writeframes(struct.pack('<%dh' % len(o2), *o2))

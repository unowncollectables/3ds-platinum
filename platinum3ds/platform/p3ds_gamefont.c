/* run 64: the game's own message font for port UI (splash, follower_talk, settings app ...).
   Read at runtime from the user's ROM (sdmc:/3ds/platinum3ds/platinum.nds). ss3ds: SoulSilver keeps the fonts in
   /a/0/1/6 (NARC_graphic_font: member 1 = message font, member 0 = system font) and the message-box frames in /a/0/3/8;
   Platinum's /graphic/pl_font.narc and pl_winframe.narc are not in this ROM. Nothing ROM-derived is stored in the repo. ~33 KB per font, loaded once.
   Glyphs: 16x16, 2bpp, 4 tiles of 16 bytes (u16 rows, high byte = left 4 px, MSB first); value 1 = text,
   2 = shadow, 3 = background (drawn transparent here). Width table at header.widthTableOffset. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifndef P3DS_ROMPATH
#define P3DS_ROMPATH "sdmc:/3ds/platinum3ds/platinum.nds"
#endif
extern void PSPNativeMemLog(const char *, ...);
static uint8_t *gf[2]; static uint32_t gfLen[2];
static uint32_t rd32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static int rdAt(FILE *f, long off, void *b, size_t n) { return fseek(f, off, SEEK_SET) == 0 && fread(b, 1, n, f) == n; }
/* find "dir/name" in the NDS FNT; returns file id or -1 */
static int fntFind(FILE *f, uint32_t fnt, const char *path) {
  uint32_t dir = 0; /* dir index (0 = root) */
  char part[64]; const char *p = path;
  while (*p) {
    const char *s = strchr(p, '/'); size_t n = s ? (size_t)(s - p) : strlen(p); if (n >= sizeof part) return -1;
    memcpy(part, p, n); part[n] = 0; p = s ? s + 1 : p + n;
    uint8_t e[8]; if (!rdAt(f, fnt + dir * 8, e, 8)) return -1;
    uint32_t sub = rd32(e); int fid = e[4] | e[5] << 8; long pos = fnt + sub; int found = 0;
    for (;;) { uint8_t t; if (!rdAt(f, pos, &t, 1) || t == 0) return -1; pos++;
      int len = t & 0x7F, isDir = t & 0x80; char nm[128]; if (!rdAt(f, pos, nm, len)) return -1; nm[len] = 0; pos += len;
      uint16_t did = 0; if (isDir) { uint8_t d[2]; if (!rdAt(f, pos, d, 2)) return -1; did = d[0] | d[1] << 8; pos += 2; }
      if (len == (int)n && !memcmp(nm, part, n)) { if (*p) { if (!isDir) return -1; dir = did & 0xFFF; } else { if (isDir) return -1; return fid; } found = 1; break; }
      if (!isDir) fid++; }
    if (!found) return -1; }
  return -1; }
/* read NARC member m of ROM file `path` into a malloc'd buffer */
static uint8_t *narcMember(FILE *f, uint32_t fnt, uint32_t fat, const char *path, int m, uint32_t *len) {
  int id = fntFind(f, fnt, path); uint8_t fe[8], nh[0x1C], c[8], me[8]; if (id < 0 || !rdAt(f, fat + id * 8, fe, 8)) return 0;
  uint32_t base = rd32(fe), end = rd32(fe + 4);
  if (end <= base || !rdAt(f, base, nh, sizeof nh) || memcmp(nh, "NARC", 4) || memcmp(nh + 0x10, "BTAF", 4)) return 0;
  uint32_t btafSz = rd32(nh + 0x14), cnt = nh[0x18] | nh[0x19] << 8, btnf = base + 0x10 + btafSz;
  if ((uint32_t)m >= cnt || !rdAt(f, btnf, c, 8) || memcmp(c, "BTNF", 4)) return 0;
  uint32_t gmif = btnf + rd32(c + 4) + 8; if (!rdAt(f, base + 0x1C + m * 8, me, 8)) return 0;
  uint32_t s = rd32(me), e = rd32(me + 4); if (e <= s || e - s > 0x20000) return 0;
  uint8_t *d = malloc(e - s); if (d && rdAt(f, gmif + s, d, e - s)) { *len = e - s; return d; } free(d); return 0; }
/* message-box frame (a/0/3/8: tiles member 2+N, palette 26+N, as LoadUserFrameGfx2): 18 4bpp tiles + 16 colours */
static uint8_t boxTiles[18 * 32]; static uint32_t boxPal[16]; static int boxOk, boxFrame = -1;
extern int P3DS_CfgInt(const char *, int);
static void loadBox(FILE *f, uint32_t fnt, uint32_t fat, int fr) {
  uint32_t lg = 0, lp = 0; uint8_t *g = narcMember(f, fnt, fat, "a/0/3/8", 2 + fr, &lg), *p = narcMember(f, fnt, fat, "a/0/3/8", 26 + fr, &lp);
  if (g && p && lg >= 0x30 + sizeof boxTiles && !memcmp(g + 0x10, "RAHC", 4) && !memcmp(p + 0x10, "TTLP", 4)) {
    uint32_t to = 0x18 + rd32(g + 0x2C), po = 0x18 + rd32(p + 0x24);
    if (to + sizeof boxTiles <= lg && po + 32 <= lp) { memcpy(boxTiles, g + to, sizeof boxTiles);
      for (int i = 0; i < 16; i++) { unsigned c = p[po + i * 2] | p[po + i * 2 + 1] << 8; boxPal[i] = ((c & 31) << 3) | (((c >> 5) & 31) << 11) | (((c >> 10) & 31) << 19); }
      boxOk = 1; boxFrame = fr; } }
  free(g); free(p); }
int P3DS_GameFontLoad(void) {
  if (gf[0] || gf[1]) return 1;
  FILE *f = fopen(P3DS_ROMPATH, "rb"); if (!f) { PSPNativeMemLog("[FONT] no ROM"); return 0; }
  uint8_t h[0x50]; int ok = 0;
  if (rdAt(f, 0, h, sizeof h)) { uint32_t fnt = rd32(h + 0x40), fat = rd32(h + 0x48);
    for (int m = 0; m < 2; m++) if ((gf[m] = narcMember(f, fnt, fat, "a/0/1/6", m, &gfLen[m]))) ok++;
    int fr = P3DS_CfgInt("ui_frame", 0); if (fr < 0 || fr > 19) fr = 0; loadBox(f, fnt, fat, fr); }
  fclose(f); PSPNativeMemLog("[FONT] game font from ROM: %d font(s) (%lu / %lu bytes), message box frame %d: %s", ok, (unsigned long)gfLen[0], (unsigned long)gfLen[1], boxFrame, boxOk ? "ok" : "missing");
  return ok > 0; }
int P3DS_GameBoxReady(void) { return boxOk; }
/* Draw the game's message-box frame around the interior (x,y,w,h in px; w,h rounded up to 8): 2 tiles left,
   3 right, 1 above/below, like DrawMessageBoxFrame. Interior = frame tile 8. */
void P3DS_GameBoxDraw(uint32_t *img, int stride, int cw, int ch, int x, int y, int w, int h) {
  if (!boxOk) return; int tw = (w + 7) / 8, th = (h + 7) / 8;
  for (int ty = -1; ty <= th; ty++) for (int tx = -2; tx < tw + 3; tx++) {
    int row = ty < 0 ? 0 : ty == th ? 2 : 1, col = tx == -2 ? 0 : tx == -1 ? 1 : tx < tw ? 2 : 3 + (tx - tw);
    const uint8_t *t = boxTiles + (row * 6 + col) * 32;
    for (int py = 0; py < 8; py++) for (int px = 0; px < 8; px++) { int v = (t[py * 4 + px / 2] >> ((px & 1) * 4)) & 15; if (!v) continue;
      int X = x + tx * 8 + px, Y = y + ty * 8 + py; if (X >= 0 && X < cw && Y >= 0 && Y < ch) img[Y * stride + X] = boxPal[v]; } } }
int P3DS_GameFontReady(void) { return gf[1] != 0; }
/* ASCII -> game charcode (US). 0 = unsupported */
static int cc(unsigned char c) {
  if (c >= '0' && c <= '9') return 0x121 + c - '0'; if (c >= 'A' && c <= 'Z') return 0x12B + c - 'A'; if (c >= 'a' && c <= 'z') return 0x145 + c - 'a';
  switch (c) { case ' ': return 0x1DE; case '!': return 0x1AB; case '?': return 0x1AC; case ',': return 0x1AD; case '.': return 0x1AE; case '/': return 0x1B1;
    case '\'': return 0x1B3; case '"': return 0x1B5; case '(': return 0x1B9; case ')': return 0x1BA; case '+': return 0x1BD; case '-': return 0x1BE; case '*': return 0x1BF;
    case '#': return 0x1C0; case '=': return 0x1C1; case '&': return 0x1C2; case '~': return 0x1C3; case ':': return 0x1C4; case ';': return 0x1C5; case '%': return 0x1D2; case 0xE9: return 0x188; /* Latin-1 e acute: Pok\xe9mon */ }
  return 0; }
static const uint8_t *glyph(int font, int c, int *w) {
  const uint8_t *d = gf[font & 1]; if (!d) return 0; uint32_t hs = rd32(d), wt = rd32(d + 4), ng = rd32(d + 8);
  int g = cc((unsigned char)c); if (!g || (uint32_t)g > ng) g = 0x1AC; g--;
  if (hs + (uint32_t)(g + 1) * 64 > gfLen[font & 1] || wt + (uint32_t)g >= gfLen[font & 1]) return 0;
  *w = d[wt + g]; return d + hs + g * 64; }
/* 2026-11-18 sharp bottom text (ss3ds_sharptext.h): one glyph as values (0 none, 1 text, 2 shadow, 3 background); returns its width, -1 = no data */
int P3DS_GameFontGlyph(int font, int ch, unsigned char v[16][16]) {
  int w; const uint8_t *g = glyph(font, ch, &w); if (!g) return -1;
  for (int py = 0; py < 16; py++) for (int px = 0; px < 16; px++) { int tile = (py >> 3) * 2 + (px >> 3); const uint8_t *row = g + tile * 16 + (py & 7) * 2; int bx = (px & 7) >> 2;
    v[py][px] = (row[bx ^ 1] >> (6 - 2 * (px & 3))) & 3; }
  return w; }
int P3DS_GameFontWidth(int font, const char *t) { int x = 0, w; for (; *t; t++) if (glyph(font, *t, &w)) x += w; return x; }
/* draw into a u32 0x00BBGGRR image (stride, clip w x h); scale 1 or 2; shadow 0 = none. Returns width drawn. */
int P3DS_GameFontDraw(uint32_t *img, int stride, int cw, int ch, int x, int y, const char *t, uint32_t fg, uint32_t sh, int scale, int font) {
  int x0 = x; if (scale < 1) scale = 1;
  for (; *t; t++) { int w; const uint8_t *g = glyph(font, *t, &w); if (!g) { if (*t == ' ') x += 3 * scale; continue; }
    /* run 118: space charcode 0x1DE is correct per HGSS charmap, but the NFTR bitmap can paint a low bar that looks like '_'. Advance only. */
    if (*t == ' ') { x += (w > 0 ? w : 3) * scale; continue; }
    for (int py = 0; py < 16; py++) for (int px = 0; px < w && px < 16; px++) {
      int tile = (py >> 3) * 2 + (px >> 3); const uint8_t *row = g + tile * 16 + (py & 7) * 2; int bx = (px & 7) >> 2;
      int v = (row[bx ^ 1] >> (6 - 2 * (px & 3))) & 3; if (v != 1 && !(v == 2 && sh)) continue; uint32_t col = v == 1 ? fg : sh;
      for (int sy = 0; sy < scale; sy++) for (int sx = 0; sx < scale; sx++) { int X = x + px * scale + sx, Y = y + py * scale + sy;
        if (X >= 0 && X < cw && Y >= 0 && Y < ch) img[Y * stride + X] = col; } }
    x += w * scale; }
  return x - x0; }
/* 3/4-size text (12 px cell) for tight bars: each output pixel is on when at least half of the 4/3 x 4/3 source area under it is ink; advance = w*3/4 */
static int ovl(int o, int s) { const int a = o * 4, b = a + 4, c = s * 3, d = c + 3; const int lo = a > c ? a : c, hi = b < d ? b : d; return hi > lo ? hi - lo : 0; }
int P3DS_GameFontWidth34(int font, const char *t) { int x = 0, w; for (; *t; t++) if (glyph(font, *t, &w)) x += w; return (x * 3 + 2) / 4; }
int P3DS_GameFontDraw34(uint32_t *img, int stride, int cw, int ch, int x, int y, const char *t, uint32_t fg, int font) {
  int x0 = x, acc = 0;
  for (; *t; t++) { int w; const uint8_t *g = glyph(font, *t, &w); if (!g) { if (*t == ' ') { acc += 3; x = x0 + (acc * 3 + 2) / 4; } continue; }
    if (*t == ' ') { acc += (w > 0 ? w : 3); x = x0 + (acc * 3 + 2) / 4; continue; } /* run 118: space = advance only */
    if (w > 16) w = 16;
    uint8_t ink[16][16]; memset(ink, 0, sizeof ink);
    for (int py = 0; py < 16; py++) for (int px = 0; px < w; px++) { int tile = (py >> 3) * 2 + (px >> 3); const uint8_t *row = g + tile * 16 + (py & 7) * 2; int bx = (px & 7) >> 2;
      ink[py][px] = ((row[bx ^ 1] >> (6 - 2 * (px & 3))) & 3) == 1; }
    const int ow = (w * 3 + 3) / 4;
    for (int oy = 0; oy < 12; oy++) for (int ox = 0; ox < ow; ox++) { int cov = 0;
      for (int sy = oy * 4 / 3; sy <= (oy * 4 + 3) / 3 && sy < 16; sy++) { const int wy = ovl(oy, sy); if (!wy) continue;
        for (int sx = ox * 4 / 3; sx <= (ox * 4 + 3) / 3 && sx < w; sx++) if (ink[sy][sx]) cov += wy * ovl(ox, sx); }
      if (cov * 2 >= 16) { int X = x + ox, Y = y + oy; if (X >= 0 && X < cw && Y >= 0 && Y < ch) img[Y * stride + X] = fg; } }
    acc += w; x = x0 + (acc * 3 + 2) / 4; }
  return x - x0; }
/* run 65 Map tab: Pokemon party icons from the ROM (poketool/icongra/pl_poke_icon.narc). member[i] / pal[i] come from
   the game's own PokeIconSpriteIndex / PokeIconPaletteIndex, palFile from PokeIconPalettesFileIndex. Frame 0 (32x32,
   4x4 tiles, 4bpp) -> out[i*1024], 0x00BBGGRR, index 0 = 0xFF000000 (transparent). Returns icons loaded. */
int P3DS_IconLoad(int n, const int *member, const int *pal, int palFile, uint32_t *out) {
  FILE *f = fopen(P3DS_ROMPATH, "rb"); if (!f) return 0;
  uint8_t h[0x50]; int ok = 0;
  if (rdAt(f, 0, h, sizeof h)) { uint32_t fnt = rd32(h + 0x40), fat = rd32(h + 0x48), lp = 0;
    uint8_t *p = narcMember(f, fnt, fat, "poketool/icongra/pl_poke_icon.narc", palFile, &lp);
    uint32_t pc[3][16]; int havePal = 0;
    if (p && lp > 0x30 && !memcmp(p + 0x10, "TTLP", 4)) { uint32_t po = 0x18 + rd32(p + 0x24);
      if (po + 3 * 32 <= lp) { havePal = 1; for (int b = 0; b < 3; b++) for (int i = 0; i < 16; i++) { unsigned c = p[po + (b * 16 + i) * 2] | p[po + (b * 16 + i) * 2 + 1] << 8;
        pc[b][i] = ((c & 31) << 3 | (c & 31) >> 2) | (((c >> 5) & 31) << 11 | ((c >> 5) & 31) >> 2 << 8) | (((c >> 10) & 31) << 19 | ((c >> 10) & 31) >> 2 << 16); } } }
    free(p);
    for (int i = 0; i < n && havePal; i++) { uint32_t lg = 0; uint8_t *g = narcMember(f, fnt, fat, "poketool/icongra/pl_poke_icon.narc", member[i], &lg);
      uint32_t *o = out + i * 1024; for (int k = 0; k < 1024; k++) o[k] = 0xFF000000u;
      if (g && lg > 0x30 && !memcmp(g + 0x10, "RAHC", 4)) { uint32_t to = 0x18 + rd32(g + 0x2C); int b = pal[i] < 0 || pal[i] > 2 ? 0 : pal[i];
        if (to + 512 <= lg) { for (int t = 0; t < 16; t++) for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
            unsigned v = g[to + t * 32 + y * 4 + x / 2]; v = (x & 1) ? v >> 4 : v & 15;
            if (v) o[((t / 4) * 8 + y) * 32 + (t % 4) * 8 + x] = pc[b][v]; }
          ok++; } }
      free(g); } }
  fclose(f); return ok; }

/* ss3ds 0110: followers.bin (overworld Pokemon sprites for the visible wild Pokemon) built on the 3DS from the user's ROM,
   the same file 3ds-platinum followers/extract.py writes, so no other port or PC step is needed. Source: NARC a/0/8/1
   members 297..862, each an NSBTX with 8 4bpp 32x32/64x64 textures (2 frames each: up, down, left, right) and the
   palettes tsure_poke0 (normal) / tsure_poke1 (shiny). Written to a .tmp file and renamed when complete.
   Format: 'P3FL' u32 1, u32 nSprites, u32 nSpecies; species nSpecies x {u16 model, u8 formMax, u8 female};
   dir nSprites x {u32 offset, u8 w, u8 h, u8 frames, u8 0}; data per sprite: 8 x (w*h/2) texels, 32 B pal, 32 B shiny pal. */
#include "p3ds_follower_luts.h"
static uint16_t rd16(const uint8_t *p) { return p[0] | p[1] << 8; }
/* NSBTX "info 3D" block at t+o: count, entry size, entry i, name i (16 bytes) */
static int bInfo(const uint8_t *t, uint32_t n, uint32_t o, int *cnt, const uint8_t **ent, uint32_t *esz, const uint8_t **names) {
  if (o + 4 > n) return 0; int c = t[o + 1]; uint32_t p = o + 4 + 8 + 4 * c; if (p + 4 > n) return 0;
  *esz = rd16(t + p); p += 4; if (*esz < 2 || p + *esz * c + 16 * c > n) return 0;
  *cnt = c; *ent = t + p; *names = t + p + *esz * c; return 1; }
/* one NSBTX -> blob (8 textures + 2 palettes); returns size, 0 = bad */
static uint32_t btxBlob(const uint8_t *b, uint32_t n, uint8_t *out, uint32_t cap, int *w) {
  if (n < 0x14 || memcmp(b, "BTX0", 4)) return 0; uint32_t off = rd32(b + 16); if (off + 0x3C > n || memcmp(b + off, "TEX0", 4)) return 0;
  const uint8_t *t = b + off; uint32_t tn = n - off;
  uint32_t texInfo = rd16(t + 0x0E), texData = rd32(t + 0x14), palInfo = rd32(t + 0x34), palData = rd32(t + 0x38);
  int c; const uint8_t *e, *nm; uint32_t esz, o = 0;
  if (!bInfo(t, tn, texInfo, &c, &e, &esz, &nm) || c != 8) return 0;
  for (int i = 0; i < 8; i++) {
    uint32_t to = rd16(e + i * esz), par = rd16(e + i * esz + 2); int tw = 8 << ((par >> 4) & 7), th = 8 << ((par >> 7) & 7);
    if (((par >> 10) & 7) != 3 || tw != th || (tw != 32 && tw != 64) || (i && tw != *w)) return 0;
    uint32_t sz = (uint32_t)tw * th / 2, src = texData + (to << 3); if (src + sz > tn || o + sz > cap) return 0;
    memcpy(out + o, t + src, sz); o += sz; *w = tw; }
  if (!bInfo(t, tn, palInfo, &c, &e, &esz, &nm)) return 0;
  int p0 = -1, p1 = -1; for (int i = 0; i < c; i++) { if (!memcmp(nm + i * 16, "tsure_poke0", 11)) p0 = i; else if (!memcmp(nm + i * 16, "tsure_poke1", 11)) p1 = i; }
  if (p0 < 0 || o + 64 > cap) return 0; if (p1 < 0) p1 = p0;
  for (int k = 0; k < 2; k++) { uint32_t src = palData + ((uint32_t)rd16(e + (k ? p1 : p0) * esz) << 3); memset(out + o, 0, 32);
    if (src < tn) memcpy(out + o, t + src, tn - src < 32 ? tn - src : 32); o += 32; }
  return o; }
/* run 41 fix: the build took over 10 s on hardware and the watchdog halted the game on the first field frame.
   Now it runs at boot (main3ds.c), with a 64 KiB write buffer, and the watchdog ignores it while this is set. */
volatile int P3DS_FolBuilding;
static int folBuild(const char *outPath);
int P3DS_FollowersBuild(const char *outPath) { P3DS_FolBuilding = 1; int ok = folBuild(outPath); P3DS_FolBuilding = 0; return ok; }
static int folBuild(const char *outPath) {
  enum { BASE = 297, NSPR = 566 };
  FILE *f = fopen(P3DS_ROMPATH, "rb"); if (!f) { PSPNativeMemLog("[FOL] build: no ROM at %s", P3DS_ROMPATH); return 0; }
  uint8_t h[0x50], nh[0x1C], c[8], me[8], fe[8]; char tmp[160]; snprintf(tmp, sizeof tmp, "%s.tmp", outPath);
  FILE *o = 0; uint8_t *buf = malloc(0x20000), *blob = malloc(8 * 2048 + 64); int ok = 0, done = 0; uint32_t total = 0;
  extern unsigned long long svcGetSystemTick(void); unsigned long long t0 = svcGetSystemTick();
  if (!buf || !blob || !rdAt(f, 0, h, sizeof h) || (memcmp(h + 0x0C, "IPG", 3) && memcmp(h + 0x0C, "IPK", 3))) { PSPNativeMemLog("[FOL] build: not a HeartGold/SoulSilver ROM"); goto out; }
  { uint32_t fnt = rd32(h + 0x40), fat = rd32(h + 0x48); int id = fntFind(f, fnt, "a/0/8/1");
    if (id < 0 || !rdAt(f, fat + id * 8, fe, 8)) goto out;
    uint32_t base = rd32(fe); if (!rdAt(f, base, nh, sizeof nh) || memcmp(nh, "NARC", 4) || memcmp(nh + 0x10, "BTAF", 4)) goto out;
    uint32_t cnt = nh[0x18] | nh[0x19] << 8, btnf = base + 0x10 + rd32(nh + 0x14); if (cnt < BASE + NSPR || !rdAt(f, btnf, c, 8) || memcmp(c, "BTNF", 4)) goto out;
    uint32_t gmif = btnf + rd32(c + 4) + 8;
    if (!(o = fopen(tmp, "wb"))) { PSPNativeMemLog("[FOL] build: cannot write %s", tmp); goto out; } setvbuf(o, NULL, _IOFBF, 0x10000);
    uint8_t hdr[16] = { 'P', '3', 'F', 'L', 1, 0, 0, 0, NSPR & 255, NSPR >> 8, 0, 0, P3DS_FOL_NSPEC & 255, P3DS_FOL_NSPEC >> 8, 0, 0 };
    fwrite(hdr, 1, 16, o);
    for (int s = 0; s < P3DS_FOL_NSPEC; s++) { uint8_t e4[4] = { kFolModel[s] & 255, kFolModel[s] >> 8, kFolForm[s], kFolFemale[s] }; fwrite(e4, 1, 4, o); }
    uint32_t dirAt = 16 + P3DS_FOL_NSPEC * 4, dataAt = dirAt + NSPR * 8; static uint8_t dir[NSPR * 8]; memset(dir, 0, sizeof dir);
    if (fseek(o, dataAt, SEEK_SET)) goto out; total = dataAt;
    for (int i = 0; i < NSPR; i++) {
      if (!rdAt(f, base + 0x1C + (BASE + i) * 8, me, 8)) goto out; uint32_t s = rd32(me), e = rd32(me + 4);
      if (e <= s || e - s > 0x20000 || !rdAt(f, gmif + s, buf, e - s)) goto out;
      int w = 0; uint32_t bl = btxBlob(buf, e - s, blob, 8 * 2048 + 64, &w); if (!bl) { PSPNativeMemLog("[FOL] build: sprite %d not readable", i); goto out; }
      if (fwrite(blob, 1, bl, o) != bl) goto out;
      uint8_t *d = dir + i * 8; d[0] = total; d[1] = total >> 8; d[2] = total >> 16; d[3] = total >> 24; d[4] = w; d[5] = w; d[6] = 8; total += bl; done++; }
    if (fseek(o, dirAt, SEEK_SET) || fwrite(dir, 1, sizeof dir, o) != sizeof dir) goto out;
    ok = 1; }
out:
  if (o && fclose(o)) ok = 0; fclose(f); free(buf); free(blob);
  if (ok) { remove(outPath); ok = rename(tmp, outPath) == 0; } else if (o) remove(tmp);
  PSPNativeMemLog("[FOL] build followers.bin from the ROM: %s (%d/%d sprites, %lu bytes, %lu ms)", ok ? "ok" : "FAILED", done, NSPR, (unsigned long)total, (unsigned long)((svcGetSystemTick() - t0) / 268111));
  return ok; }

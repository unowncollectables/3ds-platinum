/* ss3ds: Bottom menu layout + mini map (bottom screen, 256x192, 0x00BBGGRR). Pure pixel/state code with no 3DS calls, so
   tools/minimap_preview.cpp can render it on a PC. render3ds.cpp owns input, config and logging.

   Layout (everything inside 256x192, 4 px margins):
     y   0..16   header bar: current location name, right side X/Z
     y  18..100  map panel (INFO / WORLD / LOCAL), tap = next mode
     y 104..188  two rows of four tiles: POKEDEX POKEMON BAG POKEGEAR / TRAINER SAVE OPTIONS MAP

   Map data comes from the game thread (hook 0080 in FieldSystem_ProcessStep): map id, mapsec name, player tile x/z, the map
   matrix cell (x/32, z/32 outdoors; the header's town-map cell indoors) and the map type. Nothing is read from the ROM and
   nothing is written to the save. WORLD = every matrix cell the player has stood in, coloured by place type, with the player
   marked; kept across runs in minimap.bin next to config.txt. LOCAL = the path walked on this map, centred on the player. */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ss3ds_settings_ui.h"

enum { kMmGrid = 64, kMmTrail = 400, kMmModes = 3 };
enum { kMmInfo = 0, kMmWorld = 1, kMmLocal = 2 };

struct SMm {
  uint8_t cell[kMmGrid * kMmGrid]; /* 0 = never visited, else the map type (1 city, 2 route, 3 cave, 4/5 inside, 6 underground) */
  int16_t tx[kMmTrail], tz[kMmTrail]; int tn, th; /* trail ring of tiles walked on the current map */
  int map, px, pz, cx, cy, type, onMain; unsigned tick, seen; char name[24];
  bool have, dirty;
  SMm() { memset(this, 0, sizeof *this); map = -1; cx = cy = -1; }
};

/* Called with the game thread's published values when `tick` changed. Returns 1 when a never-visited cell was entered. */
static int mmUpdate(SMm &m, int map, int x, int z, int cx, int cy, int type, int onMain, const char *name, unsigned tick) {
  int fresh = 0;
  const bool mapChanged = map != m.map;
  if (mapChanged) { m.tn = m.th = 0; }
  if (mapChanged || x != m.px || z != m.pz) {
    m.tx[m.th] = (int16_t)x; m.tz[m.th] = (int16_t)z; m.th = (m.th + 1) % kMmTrail; if (m.tn < kMmTrail) m.tn++;
  }
  if (cx >= 0 && cy >= 0 && cx < kMmGrid && cy < kMmGrid && onMain) { /* only the shared outdoor matrix goes in the world map */
    const int t = type < 1 ? 1 : type > 6 ? 6 : type;
    if (!m.cell[cy * kMmGrid + cx]) { fresh = 1; m.dirty = true; }
    m.cell[cy * kMmGrid + cx] = (uint8_t)t; /* 1..6 = map type */
  }
  m.map = map; m.px = x; m.pz = z; m.cx = cx; m.cy = cy; m.type = type; m.onMain = onMain; m.tick = tick; m.have = true;
  snprintf(m.name, sizeof m.name, "%s", name && name[0] ? name : "?");
  return fresh;
}

/* minimap.bin: "MM01" + 64x64 cells. Failure to read/write is harmless (the map just starts empty / is not kept). */
static int mmLoad(SMm &m, const char *path) {
  FILE *f = fopen(path, "rb"); if (!f) return 0; char h[4]; uint8_t buf[kMmGrid * kMmGrid]; int ok = 0;
  if (fread(h, 1, 4, f) == 4 && !memcmp(h, "MM01", 4) && fread(buf, 1, sizeof buf, f) == sizeof buf) { memcpy(m.cell, buf, sizeof buf); ok = 1; }
  fclose(f); return ok; }
static int mmSave(SMm &m, const char *path) {
  FILE *f = fopen(path, "wb"); if (!f) return 0; int ok = fwrite("MM01", 1, 4, f) == 4 && fwrite(m.cell, 1, sizeof m.cell, f) == sizeof m.cell;
  fclose(f); if (ok) m.dirty = false; return ok; }
static int mmCellCount(const SMm &m) { int n = 0; for (int i = 0; i < kMmGrid * kMmGrid; i++) n += m.cell[i] != 0; return n; }

static void mmFrame(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t c) {
  for (int x = x0; x < x1; x++) { suiPx(b, x, y0, c); suiPx(b, x, y1 - 1, c); } for (int y = y0; y < y1; y++) { suiPx(b, x0, y, c); suiPx(b, x1 - 1, y, c); } }
static const char *mmTypeName(int t) { static const char *const n[] = { "?", "TOWN", "ROUTE", "CAVE", "INSIDE", "INSIDE", "UNDERGROUND" }; return n[t < 0 || t > 6 ? 0 : t]; }
static uint32_t mmCellColor(int v) {
  switch (v) { case 1: return SUI_RGB(104, 176, 104); case 2: return SUI_RGB(224, 204, 136); case 3: return SUI_RGB(160, 120, 88);
    case 4: case 5: return SUI_RGB(176, 168, 200); default: return SUI_RGB(144, 152, 176); } }
static const uint32_t kMmSea = SUI_RGB(168, 204, 240), kMmSeaLo = SUI_RGB(148, 188, 232), kMmPlayer = SUI_RGB(232, 48, 40), kMmPlayerIn = SUI_RGB(255, 255, 255);

/* red player marker (blinks between red and white), 5x5 diamond-ish dot centred on (x,y) */
/* 2026-11-15 (fps thread): the player marker is the only blinking part of the Bottom menu picture (besides the phone ring lamp). While the overlay is repainted its
   positions are noted here, so render3ds.cpp can keep ONE cached picture and just redraw the 7x7 marker each frame, instead of repainting the whole overlay (12 ms)
   again at the next blink flip. */
static bool gMkRec; static int gMkN, gMkX[4], gMkY[4];
static void mmPlayer(uint32_t *b, int x, int y, bool blink) {
  if (gMkRec && gMkN < 4) { gMkX[gMkN] = x; gMkY[gMkN] = y; gMkN++; }
  const uint32_t c = blink ? kMmPlayerIn : kMmPlayer, e = blink ? kMmPlayer : kMmPlayerIn;
  for (int dy = -3; dy <= 3; dy++) for (int dx = -3; dx <= 3; dx++) { const int d = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy); if (d == 4) suiPx(b, x + dx, y + dy, e); else if (d < 4) suiPx(b, x + dx, y + dy, c); } }

/* the map panel inside (X0,Y0)-(X1,Y1): layout 0 uses the 248x82 strip under the header, layout 1 the big left panel */
static void mmDrawPanel(uint32_t *b, const SUiText &T, const SMm &m, int mode, bool blink, int X0, int Y0, int X1, int Y1) {
  char buf[48];
  suiPanel(b, X0, Y0, X1, Y1, kSuiCell, kSuiCellLo, kSuiEdge, false);
  const int ix0 = X0 + 2, iy0 = Y0 + 2, ix1 = X1 - 2, iy1 = Y1 - 2; /* 244 x 78 in layout 0 */
  if (mode == kMmInfo || !m.have) {
    suiFill(b, ix0, iy0, ix1, iy1, kSuiCell);
    if (!m.have) { const char *t = "WALK AROUND TO LOCATE YOU"; T.draw(b, (ix0 + ix1) / 2 - T.width(t) / 2, iy0 + (iy1 - iy0) / 2 - 9, t, kSuiNote, kSuiNoteSh); return; }
    const int w = T.width(m.name); T.draw(b, (ix0 + ix1) / 2 - w / 2, iy0 + 8, m.name, kSuiName, kSuiNameSh);
    snprintf(buf, sizeof buf, "X %d   Z %d", m.px, m.pz); T.draw(b, (ix0 + ix1) / 2 - T.width(buf) / 2, iy0 + 30, buf, kSuiVal, kSuiValSh);
    snprintf(buf, sizeof buf, "%s   MAP %d", mmTypeName(m.type), m.map); T.draw(b, (ix0 + ix1) / 2 - T.width(buf) / 2, iy0 + 50, buf, kSuiNote, kSuiNoteSh);
    return; }
  /* map area: sea-blue with faint stripes, clipped to the panel interior */
  for (int y = iy0; y < iy1; y++) for (int x = ix0; x < ix1; x++) suiPx(b, x, y, ((x + y) & 7) < 1 ? kMmSeaLo : kMmSea);
  const int aw = ix1 - ix0, ah = iy1 - iy0;
  if (mode == kMmWorld) {
    int x0 = 99, y0 = 99, x1 = -1, y1 = -1;
    for (int y = 0; y < kMmGrid; y++) for (int x = 0; x < kMmGrid; x++) if (m.cell[y * kMmGrid + x]) { if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y; }
    const bool pOk = m.cx >= 0 && m.cy >= 0 && m.cx < kMmGrid && m.cy < kMmGrid;
    if (pOk) { if (m.cx < x0) x0 = m.cx; if (m.cx > x1) x1 = m.cx; if (m.cy < y0) y0 = m.cy; if (m.cy > y1) y1 = m.cy; }
    if (x1 < 0) { const char *t = "NOTHING EXPLORED YET"; T.draw(b, (ix0 + ix1) / 2 - T.width(t) / 2, iy0 + 30, t, kSuiNote, kSuiNoteSh); return; }
    /* pad to at least 24 x 10 cells so the first steps do not blow up to one giant cell */
    int w = x1 - x0 + 1, h = y1 - y0 + 1; if (w < 24) { x0 -= (24 - w) / 2; w = 24; } if (h < 10) { y0 -= (10 - h) / 2; h = 10; }
    int s = aw / w; const int sy = ah / h; if (sy < s) s = sy; if (s < 1) s = 1; if (s > 8) s = 8;
    const int ox = ix0 + (aw - w * s) / 2, oy = iy0 + (ah - h * s) / 2;
    for (int y = 0; y < kMmGrid; y++) for (int x = 0; x < kMmGrid; x++) { const int v = m.cell[y * kMmGrid + x]; if (!v) continue;
      const int gx = ox + (x - x0) * s, gy = oy + (y - y0) * s; suiFill(b, gx < ix0 ? ix0 : gx, gy < iy0 ? iy0 : gy, gx + s > ix1 ? ix1 : gx + s, gy + s > iy1 ? iy1 : gy + s, mmCellColor(v));
      if (s >= 3) mmFrame(b, gx, gy, gx + s, gy + s, suiMix(mmCellColor(v), kSuiEdge, 1, 3)); }
    if (pOk) { int px = ox + (m.cx - x0) * s + s / 2, py = oy + (m.cy - y0) * s + s / 2;
      if (m.onMain && s >= 4) { px = ox + (m.cx - x0) * s + (m.px % 32) * s / 32; py = oy + (m.cy - y0) * s + (m.pz % 32) * s / 32; }
      if (px < ix0 + 3) px = ix0 + 3; if (px > ix1 - 4) px = ix1 - 4; if (py < iy0 + 3) py = iy0 + 3; if (py > iy1 - 4) py = iy1 - 4;
      mmPlayer(b, px, py, blink); }
    if (!m.onMain) { const char *t = "INDOORS"; T.draw(b, ix1 - 4 - T.width(t), iy1 - 12, t, kSuiNote, kSuiNoteSh); }
    return; }
  /* LOCAL: 3 px per tile, player fixed in the middle, trail fades from old (pale) to new (strong) */
  const int cxp = (ix0 + ix1) / 2, cyp = (iy0 + iy1) / 2, ts = 3;
  for (int gy = -(ah / 2 / ts / 8 + 1) * 8; gy <= (ah / 2 / ts / 8 + 1) * 8; gy += 8) for (int gx = -(aw / 2 / ts / 8 + 1) * 8; gx <= (aw / 2 / ts / 8 + 1) * 8; gx += 8) { /* 8-tile grid dots, scroll with the player */
    const int sx = cxp + ((gx - ((m.px % 8) + 8) % 8)) * ts, sy = cyp + ((gy - ((m.pz % 8) + 8) % 8)) * ts; if (sx >= ix0 && sx < ix1 && sy >= iy0 && sy < iy1) suiPx(b, sx, sy, SUI_RGB(120, 156, 208)); }
  for (int k = 0; k < m.tn; k++) { const int idx = (m.th - m.tn + k + kMmTrail * 2) % kMmTrail;
    const int sx = cxp + (m.tx[idx] - m.px) * ts - 1, sy = cyp + (m.tz[idx] - m.pz) * ts - 1; if (sx < ix0 || sy < iy0 || sx + ts > ix1 || sy + ts > iy1) continue;
    suiFill(b, sx, sy, sx + ts, sy + ts, suiMix(SUI_RGB(232, 240, 255), SUI_RGB(64, 112, 208), k, m.tn)); }
  mmPlayer(b, cxp, cyp, blink);
  const char *t = "PATH WALKED HERE"; T.draw(b, ix0 + 4, iy1 - 12, t, kSuiNote, kSuiNoteSh); }

/* ---- bottom_layout=1 ("JOHTO"): big map panel on the left, a column of icon buttons on the right and a registered-item
   square under it, the arrangement of GabeSud0's pokeplatinum-3ds lower menu (port_lowermenu.c: panel left, eight buttons,
   item squares) redrawn for SoulSilver in silver and Johto blue. Layout and palette only; no code or art is copied, the
   icons below are drawn here (10x10, '#' = ink, '+' = accent, '.' = light). Map: the explored-cell world map from hook 0080
   (procedural, not the Pokegear art). Buttons 0..6 = the same actions as layout 0's tiles, 7 = map mode, 8 = Y item. */
static const int kBlPanX0 = 3, kBlPanY0 = 3, kBlPanX1 = 187, kBlPanY1 = 189, kBlColX0 = 190, kBlColX1 = 253;
/* run 18: bottom_hd=1 draws GabeSud0's column geometry at native 320x240 (ss3ds_blnative.h); touch follows it (native x 4/5) */
static int gBlHd;
enum { kBlItem1 = 8, kBlRun = 9, kBlItem2 = 10, kBlButtonCount = 11, kBlItem2Hit = 12, kBlItemIconSize = 20 };
static inline void blNativeRect(int i, int *x0, int *y0, int *x1, int *y1) {
  if (i < kBlItem1) { *x0 = 250; *x1 = 316; *y0 = 3 + 23 * i; *y1 = *y0 + 21; }
  else {
    *x0 = i == kBlRun ? 285 : 250;
    *x1 = i == kBlRun ? 317 : 282;
    *y0 = i == kBlRun ? 197 : i == kBlItem1 ? 191 : 214;
    *y1 = i == kBlRun ? 229 : *y0 + 21;
  }
}
static inline void blRect(int i, int *x0, int *y0, int *x1, int *y1) {
  if (gBlHd) {
    int a, b, c, d; blNativeRect(i, &a, &b, &c, &d);
    *x0 = a * 4 / 5; *y0 = b * 4 / 5; *x1 = (c + 1) * 4 / 5; *y1 = (d + 1) * 4 / 5; return;
  }
  if (i < kBlItem1) { *x0 = kBlColX0; *x1 = kBlColX1; *y0 = 3 + i * 18; *y1 = *y0 + 17; }
  else {
    *x0 = i == kBlRun ? kBlColX0 + 32 : kBlColX0;
    *x1 = i == kBlRun ? kBlColX1 : *x0 + 31;
    *y0 = i == kBlItem2 ? 170 : 151;
    *y1 = i == kBlItem1 ? 169 : 189;
  }
}
/* 10 = map panel, 11 = tabs, 12 = second registered item. */
static inline int blHit(int x, int y) {
  for (int i = 0; i < kBlButtonCount; i++) {
    int a, b, c, d; blRect(i, &a, &b, &c, &d);
    if (x >= a && x < c && y >= b && y < d) return i == kBlItem2 ? kBlItem2Hit : i;
  }
  if (x >= kBlPanX0 && x < kBlPanX1 && y >= kBlPanY0 && y < kBlPanY0 + 16) return 11; /* the tab bar (ss3ds_pgextra.h; ignored when pg_extras is off) */
  return x >= kBlPanX0 && x < kBlPanX1 && y >= kBlPanY0 + 16 && y < kBlPanY1 ? 10 : -1; }
/* run 10: 12x12 icons, 1-px dark outline, solid fills ('#' ink, '+' accent, 'o' white), drawn 1:1 (no scaling in this code) */
static const char *const kBlIcon[12][12] = {
  { ".##########.", "#++++++++++#", "#+oooooooo+#", "#+o######o+#", "#+o#oooo#o+#", "#+o######o+#", "#+oooooooo+#", "#+o##o##oo+#", "#+oooooooo+#", "#++++++++++#", ".##########.", "............" }, /* dex */
  { "....####....", "..##++++##..", ".#++++++++#.", ".#++++++++#.", "#####oo#####", "#ooo#..#ooo#", "#ooo####ooo#", ".#oooooooo#.", ".#oooooooo#.", "..##oooo##..", "....####....", "............" }, /* ball */
  { "....####....", "...#....#...", "..#......#..", ".##########.", "#++++++++++#", "#+########+#", "#+#oooooo#+#", "#+########+#", "#++++++++++#", "#++++++++++#", ".##########.", "............" }, /* bag */
  { ".##########.", "#oooooooooo#", "#o########o#", "#o#++++++#o#", "#o#+o+++o#o#", "#o#++++++#o#", "#o########o#", "#oo#oo#oo#o#", "#oooooooooo#", "#oo######oo#", ".##########.", "............" }, /* pokegear */
  { "############", "#oooooooooo#", "#o##o+++++o#", "#o##ooooooo#", "#o##o+++++o#", "#oooooooooo#", "#o++++++++o#", "#oooooooooo#", "############", "............", "............", "............" }, /* card */
  { "###########.", "#+#oooooo#+#", "#+#oooooo#+#", "#+#oooooo#+#", "#+########+#", "#++++++++++#", "#+########+#", "#+#oo#ooo#+#", "#+#oo#ooo#+#", "############", "............", "............" }, /* save */
  { ".....##.....", "..#.#++#.#..", ".#+#++++#+#.", "..#++##++#..", ".#++#oo#++#.", "##+#oooo#+##", "##+#oooo#+##", ".#++#oo#++#.", "..#++##++#..", ".#+#++++#+#.", "..#.#++#.#..", ".....##....." }, /* options */
  { "##...##...##", "#+#.#oo#.#+#", "#++#oooo#++#", "#++#oooo#++#", "#++#oooo#++#", "#++#oooo#++#", "#++#oooo#++#", "#++#oooo#++#", "#+#.#oo#.#+#", "##...##...##", "............", "............" }, /* map */
  { ".....#......", "....#+#.....", "...#+o+#....", "..#+ooo+#...", ".#+ooooo+#..", "#++ooooo++#.", ".#+ooooo+#..", "..#+ooo+#...", "...#+o+#....", "....#+#.....", ".....#......", "............" }, /* item */
  { "............", "....####....", "...#++++#...", "...#++++#...", "...#+++##...", "...#++#oo#..", "..#+++#ooo#.", ".#++++#oooo#", "#+++++######", "############", "#oo#oo#oo#o#", "############" }, /* running shoe */
  { "........#...", ".......#....", "......#.....", "############", "#ooooooooo+#", "#o###oo##o+#", "#o#+#oo##o+#", "#o###oo##o+#", "#ooooooooo+#", "############", "............", "............" }, /* radio (panel tab, run 147) */
  { "....####....", "...#oooo#...", "..#ooooo+#..", "..#oooo++#..", ".#ooooo+++#.", ".#oooo++++#.", ".#ooo+++++#.", ".#oo++++++#.", "..#++++++#..", "..##++++##..", "....####....", "............" }, /* egg (panel tab, run 147) */
};
/* run 151 (Nathan: one style everywhere): the native column's colours (ss3ds_blnative.h bnFrame): BTN / BTN_SHD buttons,
   NAVY outline, SEL green for the open / pressed state, grey frame behind */
static const uint32_t kBlSilverHi = SUI_RGB(244, 247, 254), kBlSilverLo = SUI_RGB(189, 201, 222), kBlNavy = SUI_RGB(7, 33, 54),
  kBlBlue = SUI_RGB(24, 153, 115), kBlPress = SUI_RGB(46, 208, 155), kBlBg0 = SUI_RGB(200, 208, 211), kBlBg1 = SUI_RGB(186, 196, 200),
  kBlText = SUI_RGB(18, 26, 38), kBlSelO = SUI_RGB(0, 84, 64), kBlIcInk = SUI_RGB(24, 120, 72), kBlIcAcc = SUI_RGB(104, 200, 140);
/* the native column's button box in DS pixels: outline without its corners, top highlight row, two shade rows */
static void blBox(uint32_t *b, int x0, int y0, int x1, int y1, bool sel) {
  const uint32_t fill = sel ? kBlPress : kBlSilverHi, top = sel ? kBlPress : SUI_RGB(252, 253, 255), shd = sel ? kBlBlue : kBlSilverLo;
  for (int y = y0 + 1; y < y1 - 1; y++)
    suiFill(b, x0 + 1, y, x1 - 1, y + 1, y == y0 + 1 ? top : y >= y1 - 3 ? shd : fill);
  for (int x = x0 + 1; x < x1 - 1; x++) { suiPx(b, x, y0, kBlNavy); suiPx(b, x, y1 - 1, kBlNavy); }
  for (int y = y0 + 1; y < y1 - 1; y++) { suiPx(b, x0, y, kBlNavy); suiPx(b, x1 - 1, y, kBlNavy); } }
static void blButtonDetails(uint32_t *b, int x0, int y0, int y1, int divider, int accent, bool selected) {
  suiFill(b, x0 + 1, y0 + 1, divider, y1 - 1, selected ? SUI_RGB(40, 190, 142) : SUI_RGB(232, 237, 246));
  suiFill(b, divider, y0 + 1, divider + 1, y1 - 1, selected ? SUI_RGB(26, 108, 95) : SUI_RGB(164, 176, 182));
  suiFill(b, accent, y0 + 3, accent + 1, y1 - 3, SUI_RGB(246, 93, 77));
  suiFill(b, accent + 1, y0 + 3, accent + 2, y1 - 3, SUI_RGB(212, 59, 60));
}
static void blIcon(uint32_t *b, int x, int y, int i, uint32_t ink, uint32_t acc) {
  for (int r = 0; r < 12; r++) for (int c = 0; c < 12 && kBlIcon[i][r][c]; c++) { const char ch = kBlIcon[i][r][c];
    if (ch == '#') suiPx(b, x + c, y + r, ink); else if (ch == '+') suiPx(b, x + c, y + r, acc); else if (ch == 'o') suiPx(b, x + c, y + r, SUI_RGB(255, 255, 255)); } }
/* run 11: bit i set = button i's icon comes from the game (drawn at native resolution by gpuscale's overlay), skip ours */
static unsigned blGameIconMask;
static uint32_t blItemIcons[2][kBlItemIconSize * kBlItemIconSize];
static unsigned blItemIconMask;
static void blButton(uint32_t *b, const SUiText &T, int i, const char *label, const char *alt, bool on, bool pressed) {
  /* run 151: the same look as the sharp column (icon cell, divider, name, red tab at the right edge); only seen at SCALE 1:1 */
  int x0, y0, x1, y1; blRect(i, &x0, &y0, &x1, &y1); blBox(b, x0, y0, x1, y1, pressed);
  const bool game = (blGameIconMask >> i) & 1; const uint32_t ink = pressed ? kBlSelO : kBlIcInk, acc = pressed ? SUI_RGB(255, 255, 255) : kBlIcAcc;
  if (i == kBlItem1 || i == kBlItem2) {
    const int slot = i == kBlItem1 ? 0 : 1;
    if (!game && (blItemIconMask & (1u << slot))) {
      const int size = y1 - y0 - 2;
      const int x = (x0 + x1 - size) / 2, y = y0 + 1;
      for (int r = 0; r < size; r++) for (int c = 0; c < size; c++) {
        const uint32_t color = blItemIcons[slot][(r * kBlItemIconSize / size) * kBlItemIconSize + c * kBlItemIconSize / size];
        if (color >> 24) suiPx(b, x + c, y + r, color);
      }
    } else if (!game) {
      const char *n = slot ? "2" : "1";
      T.draw(b, (x0 + x1 - T.width(n)) / 2, (y0 + y1) / 2 - 5, n, kBlText, kBlSilverHi);
    }
    return;
  }
  if (i < 8) {
    blButtonDetails(b, x0, y0, y1, x0 + 15, x1 - 3, pressed);
    if (!game) blIcon(b, x0 + 2, (y0 + y1) / 2 - 6, i, ink, acc);
    const char *t = T.width(label) <= x1 - x0 - 20 ? label : alt; const int tx = x0 + 16 + (x1 - x0 - 19 - T.width(t)) / 2;
    T.draw(b, tx, (y0 + y1) / 2 - 5, t, pressed ? SUI_RGB(255, 255, 255) : on ? kBlText : SUI_RGB(150, 160, 170), pressed ? kBlSelO : kBlSilverHi); }
  else { if (!game) blIcon(b, (x0 + x1) / 2 - 6, y0 + 5, i, ink, acc); const char *t = T.width(label) <= x1 - x0 - 2 ? label : alt; T.draw(b, (x0 + x1) / 2 - T.width(t) / 2, game ? y0 + 28 : y0 + 22, t, pressed ? SUI_RGB(255, 255, 255) : kBlText, pressed ? kBlSelO : kBlSilverHi); } }
/* whole screen for layout 1; `labels`/`alts`/`on` for buttons 0..8, `press` = button held (-1 none) */
static void ctCardDraw(uint32_t *b, const SUiText &T, int X0, int Y0, int X1); /* ss3ds_pgmap.h: Bug-Catching Contest card (hook 0229) */
struct SPg; static bool blPgDraw(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1, bool blink); /* ss3ds_pgmap.h, via render3ds.cpp */
static bool pxOn(); static inline int pxPane(); static void pxNameStrip(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, const char *name); static void pxRingDraw(uint32_t *b, const SUiText &T, bool blink); static void pxBar(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, const char *name); static bool pxPaneDraw(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1); /* ss3ds_pgextra.h */
/* run 142: on the MAP pane the area name has its own full-width strip under the tab row, and the map panel starts below it */
enum { kBlNameH = 12 };
static inline int blNameH() { return pxOn() && pxPane() == 0 ? kBlNameH : 0; }
/* mode -1 = the Pokegear map (run 10), 0..2 = bottom_map views */
static void blDraw(uint32_t *b, const SUiText &T, const SMm &m, int mode, bool blink, const char *const *labels, const char *const *alts, const bool *on, int press) {
  for (int y = 0; y < 192; ++y) {
    uint32_t *row = b + y * 256;
    std::fill(row, row + kBlColX0 - 1, suiMix(kBlBg0, kBlBg1, y, 191));
    std::fill(row + kBlColX0 - 1, row + 256, SUI_RGB(77, 100, 114));
  }
  /* location bar on top of the map panel */
  /* run 147: with the tabs on, the row has no silver backing bar (it made silver buttons sit in a silver box); the tabs float on the screen background like the side column's buttons */
  if (!pxOn()) { for (int y = kBlPanY0; y < kBlPanY0 + 16; y++) { const uint32_t c = suiMix(kBlSilverHi, kBlSilverLo, y - kBlPanY0, 16); suiFill(b, kBlPanX0, y, kBlPanX1, y + 1, c); }
  mmFrame(b, kBlPanX0, kBlPanY0, kBlPanX1, kBlPanY0 + 16, kBlNavy); }
  /* 2026-11-18 speed: with the native column/tabs on (gBlHd) their DS-size copies are hidden under the overlay, so they are not painted:
     a repaint (tab swap, scroll, pan) skips 11 buttons and 6 tabs of text and icons */
  if (pxOn()) { if (!gBlHd) pxBar(b, T, kBlPanX0, kBlPanY0, kBlPanX1, m.have ? m.name : "LOCATING..."); } else suiSDraw(T, b, kBlPanX0 + 5, kBlPanY0 + 2, m.have ? m.name : "LOCATING...", kBlNavy, kBlSilverHi);
  const int nh = blNameH(); if (nh) pxNameStrip(b, T, kBlPanX0, kBlPanY0, kBlPanX1, m.have ? m.name : "LOCATING...");
  if (mode < 0 && pxPaneDraw(b, T, kBlPanX0, kBlPanY0 + 17, kBlPanX1, kBlPanY1)) {}
  else if (mode >= 0 || !blPgDraw(b, T, kBlPanX0, kBlPanY0 + 15 + nh, kBlPanX1, kBlPanY1, blink)) /* panel rim shares the tab bar's (or name strip's) bottom row (no gap, no double border) */ mmDrawPanel(b, T, m, mode < 0 ? kMmWorld : mode, blink, kBlPanX0, kBlPanY0 + 17 + nh, kBlPanX1, kBlPanY1);
  if (mode < 0 && pxPane() == 0) ctCardDraw(b, T, kBlPanX0, kBlPanY0 + 17 + nh, kBlPanX1);
  if (mode < 0) pxRingDraw(b, T, blink);
  if (!gBlHd) for (int i = 0; i < kBlButtonCount; i++) blButton(b, T, i, labels[i], alts[i], on[i], press == i);
}

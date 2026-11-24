/* Parts of this file are adapted from GabeSud0/pokeplatinum-3ds 3ds_port/nitro/src/port_lowermenu.c (GPL-3.0): frame drawing, palette, geometry, glyphs, MAP icon. */
/* ss3ds run 18: the JOHTO layout's button column drawn at the 3DS's own 320x240 resolution into the native overlay atlas
   (gpuscale.c), so its frames and names are not blurred by the 256x192 -> 320x240 stretch. Style after GabeSud0's
   pokeplatinum-3ds lower menu (3ds_port/nitro/src/port_lowermenu.c, GPL-3.0): 1 px outline with cut corners, a highlight
   row under the top, two shading rows over the bottom, an icon cell + divider + name per button, a coloured tab at the
   right edge, and its small 9-row capital font (glyphs A..X from that file; F J L U W Y - and the space added here).
   Colours are ours (SoulSilver silver and Johto blue). Hit areas are unchanged: every native rect is the DS rect x 1.25. */
#pragma once
#include <stdint.h>
#include <string.h>
struct SBnGlyph { char ch; const char *rows[9]; };
static const SBnGlyph kBnGlyphs[] = {
  { '1', { ".#.", "##.", ".#.", ".#.", ".#.", ".#.", ".#.", ".#.", "###" } },
  { '2', { ".##.", "#..#", "...#", "...#", "..#.", ".#..", "#...", "#...", "####" } },
  { 'A', { ".##.", "#..#", "#..#", "#..#", "####", "#..#", "#..#", "#..#", "#..#" } },
  { 'B', { "###.", "#..#", "#..#", "#..#", "###.", "#..#", "#..#", "#..#", "###." } },
  { 'C', { ".##.", "#..#", "#...", "#...", "#...", "#...", "#...", "#..#", ".##." } },
  { 'D', { "###.", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "###." } },
  { 'E', { "####", "#...", "#...", "#...", "###.", "#...", "#...", "#...", "####" } },
  /* 'e' = É: Gabe's small accented e read as '&' at this size, so a capital E (7 rows) under the accent */
  { 'e', { "..#.", ".#..", "####", "#...", "###.", "#...", "#...", "#...", "####" } },
  { 'F', { "####", "#...", "#...", "#...", "###.", "#...", "#...", "#...", "#..." } },
  { 'G', { ".##.", "#..#", "#...", "#...", "#.##", "#..#", "#..#", "#..#", ".###" } },
  { 'H', { "#..#", "#..#", "#..#", "#..#", "####", "#..#", "#..#", "#..#", "#..#" } },
  { 'I', { "###", ".#.", ".#.", ".#.", ".#.", ".#.", ".#.", ".#.", "###" } },
  { 'J', { "..##", "...#", "...#", "...#", "...#", "...#", "#..#", "#..#", ".##." } },
  { 'K', { "#..#", "#..#", "#.#.", "#.#.", "##..", "#.#.", "#.#.", "#..#", "#..#" } },
  { 'L', { "#...", "#...", "#...", "#...", "#...", "#...", "#...", "#...", "####" } },
  { 'M', { "#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#", "#...#", "#...#" } },
  { 'N', { "#..#", "#..#", "##.#", "##.#", "#.##", "#.##", "#..#", "#..#", "#..#" } },
  { 'O', { ".##.", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", ".##." } },
  { 'P', { "###.", "#..#", "#..#", "#..#", "###.", "#...", "#...", "#...", "#..." } },
  { 'R', { "###.", "#..#", "#..#", "#..#", "###.", "#.#.", "#..#", "#..#", "#..#" } },
  { 'S', { ".##.", "#..#", "#...", "#...", ".##.", "...#", "...#", "#..#", ".##." } },
  { 'T', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." } },
  { 'U', { "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", ".##." } },
  { 'V', { "#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", ".#.#.", "..#..", "..#.." } },
  { 'W', { "#...#", "#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", "##.##", "#...#" } },
  { 'X', { "#..#", "#..#", "#..#", ".##.", ".##.", ".##.", "#..#", "#..#", "#..#" } },
  { 'Y', { "#...#", "#...#", ".#.#.", ".#.#.", "..#..", "..#..", "..#..", "..#..", "..#.." } },
  { '-', { "...", "...", "...", "...", "###", "...", "...", "...", "..." } },
  { ' ', { "..", "..", "..", "..", "..", "..", "..", "..", ".." } },
};
static const SBnGlyph *bnGlyph(char c) { for (const SBnGlyph &g : kBnGlyphs) if (g.ch == c) return &g; if (c >= 'a' && c <= 'z') c -= 32; for (const SBnGlyph &g : kBnGlyphs) if (g.ch == c) return &g; return 0; }
static int bnTextW(const char *s) { int w = 0, n = 0; for (; *s; s++, n++) { const SBnGlyph *g = bnGlyph(*s); w += g ? (int)strlen(g->rows[0]) : 4; } return n ? w + n - 1 : 0; }
/* canvas: the atlas (stride 512, words R<<24|G<<16|B<<8|A), origin at atlas (ox,oy) = native (nx,ny) */
struct SBnCanvas { uint32_t *a; int ox, oy, nx, ny, w, h; };
/* run 20: the game's Sprint Button for the RUN square (31x32, [0] off, [1] on), decoded from the ROM by blBuildIcons */
static uint32_t gBnSprint[2][31 * 32]; static bool gBnSprintOk;
static inline uint32_t bnRGB(int r, int g, int b) { return (uint32_t)r << 24 | (uint32_t)g << 16 | (uint32_t)b << 8 | 0xFFu; }
static inline void bnPut(const SBnCanvas &c, int x, int y, uint32_t v) { x -= c.nx; y -= c.ny; if (x >= 0 && y >= 0 && x < c.w && y < c.h) c.a[(c.oy + y) * 512 + c.ox + x] = v; }
static void bnRect(const SBnCanvas &c, int x0, int y0, int x1, int y1, uint32_t v) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) bnPut(c, x, y, v); }
static void bnText(const SBnCanvas &c, int x, int y, const char *s, uint32_t v) {
  for (; *s; s++) { const SBnGlyph *g = bnGlyph(*s); if (!g) { x += 5; continue; } const int gw = (int)strlen(g->rows[0]);
    for (int r = 0; r < 9; r++) for (int k = 0; k < gw; k++) if (g->rows[r][k] == '#') bnPut(c, x + k, y + r, v); x += gw + 1; } }
/* the box: outline with its corners left out, fill, highlight row, `shade` rows */
static void bnBox(const SBnCanvas &c, int x0, int y0, int x1, int y1, uint32_t ol, uint32_t fill, uint32_t top, uint32_t shade, int shadeRows) {
  for (int x = x0 + 1; x < x1; x++) { bnPut(c, x, y0, ol); bnPut(c, x, y1, ol); } for (int y = y0 + 1; y < y1; y++) { bnPut(c, x0, y, ol); bnPut(c, x1, y, ol); }
  bnRect(c, x0 + 1, y0 + 1, x1 - 1, y1 - 1, fill); for (int x = x0 + 1; x < x1; x++) bnPut(c, x, y0 + 1, top);
  for (int r = 0; r < shadeRows; r++) for (int x = x0 + 1; x < x1; x++) bnPut(c, x, y1 - 1 - r, shade); }
/* one of our 12x12 icons (ss3ds_minimap.h kBlIcon) at scale sc (1 = native pixels; 2x looked heavy), rows [r0, r1) */
static void bnIcon2x(const SBnCanvas &c, int x, int y, int i, uint32_t ink, uint32_t acc, int r0 = 0, int r1 = 12, int sc = 1) {
  for (int r = r0; r < r1; r++) for (int k = 0; k < 12 && kBlIcon[i][r][k]; k++) { const char ch = kBlIcon[i][r][k]; uint32_t v;
    if (ch == '#') v = ink; else if (ch == '+') v = acc; else if (ch == 'o') v = bnRGB(255, 255, 255); else continue;
    for (int dy = 0; dy < sc; dy++) for (int dx = 0; dx < sc; dx++) bnPut(c, x + k * sc + dx, y + (r - r0) * sc + dy, v); } }
/* GabeSud0's MAP icon (port_lowermenu.c sMapIconRows/sMapIconPalette, his own pixel art, GPL-3.0): our JOHTO map button. 21 x 17. */
static const uint32_t kBnMapPal[] = { 0x4A525AFFu, 0xFFFFF6FFu, 0xCDDEE6FFu, 0x78C8FFFFu, 0x1870D0FFu, 0x90E468FFu, 0x288838FFu, 0x309CF0FFu, 0xE83828FFu, 0x40B840FFu, 0xDEB440FFu, 0xFFE260FFu };
static const char *const kBnMapRows[17] = {
    "......aaa..........aa", "..aaaabccaaaa...aaaba", "aabbbbdeeccccaaabbbba", "abdddddeeeeeecbbdddba",
    "abddddfggeeeeeddffdba", "abhhfffggeeeeedfiffba", "abhfffjggkkkkefilffba", "abjjjjjggkgeeklljjhba",
    "abjjjjlkkkgeeehjhhhba", "abjllljggkgeeehhhhhba", "abjjjjjggggeeehhhhhba", "abjjjjjggggeeehhhhhba",
    "abhhjjheeeeeeehhhhhba", "abhhhhbcceeeeehhhhhba", "abbbbbaaaccccehhbbbaa", "abaaaa...aaaacbbaaa..",
    "aa...........aaa....." };
static inline uint32_t bnFade(uint32_t p, uint32_t bg, int a) { uint32_t o = 0xFF; for (int s = 24; s >= 8; s -= 8) o |= ((((p >> s) & 255) * a + ((bg >> s) & 255) * (255 - a) + 127) / 255) << s; return o; }
static void bnMapIcon(const SBnCanvas &c, int x0, int y0, int x1, int y1, bool on, uint32_t bg) {
  const int x = x0 + (x1 - x0 + 1 - 21) / 2, y = y0 + (y1 - y0 + 1 - 17) / 2;
  for (int r = 0; r < 17; r++) for (int k = 0; k < 21; k++) { const char ch = kBnMapRows[r][k]; if (ch == '.') continue; const uint32_t p = kBnMapPal[ch - 'a']; bnPut(c, x + k, y + r, on ? p : bnFade(p, bg, 96)); } }
/* text with a 1 px outline (port_lowermenu.c text_outlined) */
static void bnTextO(const SBnCanvas &c, int x, int y, const char *s, uint32_t v, uint32_t o) { bnText(c, x - 1, y, s, o); bnText(c, x + 1, y, s, o); bnText(c, x, y - 1, s, o); bnText(c, x, y + 1, s, o); bnText(c, x, y, s, v); }
/* Gabe's geometry (port_lowermenu.c): buttons x 250-316, rows 3 + 23 i, 22 tall; icon cell 251-274, divider 275, name
   277-314, red tab 317-318; squares {250,197,282,229} {285,197,317,229}. */
static void bnRectOf(int i, int *x0, int *y0, int *x1, int *y1) {
  blNativeRect(i, x0, y0, x1, y1);
}
/* the same in DS pixels (touch), x 4/5 */
static void bnDsRect(int i, int *x0, int *y0, int *x1, int *y1) { bnRectOf(i, x0, y0, x1, y1); *x0 = *x0 * 4 / 5; *y0 = *y0 * 4 / 5; *x1 = (*x1 + 1) * 4 / 5; *y1 = (*y1 + 1) * 4 / 5; }
/* DS pixel span [ds0, ds1) on the 256x192 picture, as the native pixels that cover it when that picture is stretched to 320x240 (x5/4). */
static void bnDsSpan(int ds0, int ds1, int *n0, int *n1) {
  *n0 = ds0 * 5 / 4; *n1 = (ds1 * 5 + 3) / 4; if (*n1 < *n0) *n1 = *n0; }
// The right column and the top tabs share the same inset icon cell and red edge accent.
static void bnButtonDetails(const SBnCanvas &c, int x0, int y0, int y1, int divider, int accent, bool selected) {
  const uint32_t cell = selected ? bnRGB(40, 190, 142) : bnRGB(232, 237, 246);
  const uint32_t line = selected ? bnRGB(26, 108, 95) : bnRGB(164, 176, 182);
  bnRect(c, x0 + 1, y0 + 1, divider - 1, y1 - 1, cell);
  bnRect(c, divider, y0 + 1, divider, y1 - 1, line);
  bnRect(c, accent, y0 + 3, accent, y1 - 3, bnRGB(246, 93, 77));
  bnRect(c, accent + 1, y0 + 3, accent + 1, y1 - 3, bnRGB(212, 59, 60));
}
/* One native tab button, placed in the atlas by bnTabStrip. Screen rect is 320x240; source is 1:1 in the atlas. */
struct SBnTabDst { short dx, dy, w, h, sx, sy; };
/* JOHTO top tabs at native resolution, same box and green pixel icons as the column (bnBox / bnIcon2x).
   ds* are the DS hit rects from pxChips. Drawn into atlas row y=180 (the frame windows stop at y=168). */
static int bnTabStrip(uint32_t *atlas, int pane, int n, const int *dsX0, const int *dsX1, int dsY0, int dsY1, const int *icons, const char *const *labels, const char *const *shortLabels, SBnTabDst *dst, int maxDst) {
  if (!atlas || n < 1) return 0;
  const uint32_t NAVY = bnRGB(7, 33, 54), BTN = bnRGB(244, 247, 254), BTN_TOP = bnRGB(252, 253, 255), BTN_SHD = bnRGB(189, 201, 222),
    SEL = bnRGB(46, 208, 155), SEL_SHD = bnRGB(24, 153, 115), BG = bnRGB(200, 208, 211),
    INK = bnRGB(24, 120, 72), ACC = bnRGB(104, 200, 140), SEL_INK = bnRGB(0, 84, 64), WHITE = bnRGB(255, 255, 255);
  int nd = 0, ax = 0;
  for (int i = 0; i < n && nd < maxDst; i++) {
    int x0, x1, y0, y1; bnDsSpan(dsX0[i], dsX1[i], &x0, &x1); bnDsSpan(dsY0, dsY1, &y0, &y1);
    const int w = x1 - x0, h = y1 - y0; if (w < 8 || h < 8 || ax + w > 250) continue;
    const SBnCanvas c = { atlas, ax, 180, 0, 0, w, h };
    bnRect(c, 0, 0, w - 1, h - 1, BG); /* opaque corners: bnBox leaves them out, same cut as the column */
    const bool on = i == pane;
    bnBox(c, 0, 0, w - 1, h - 1, NAVY, on ? SEL : BTN, on ? SEL : BTN_TOP, on ? SEL_SHD : BTN_SHD, 2);
    const int divider = 13, textLeft = divider + 2, textRight = w - 4, room = textRight - textLeft;
    const char *label = bnTextW(labels[i]) <= room ? labels[i] : shortLabels[i];
    const int tw = bnTextW(label);
    const bool showLabel = tw <= room;
    bnButtonDetails(c, 0, 0, h - 1, showLabel ? divider : w - 5, w - 3, on);
    bnIcon2x(c, showLabel ? 1 : (w - 5 - 12) / 2, (h - 12) / 2,
             icons[i], on ? SEL_INK : INK, on ? WHITE : ACC);
    if (showLabel) {
      const int x = textLeft + (room - tw) / 2, y = (h - 9) / 2;
      if (on) bnTextO(c, x, y, label, WHITE, SEL_INK);
      else bnText(c, x, y, label, bnRGB(18, 26, 38));
    }
    dst[nd++] = { (short)x0, (short)y0, (short)w, (short)h, (short)ax, 180 }; ax += w; }
  return nd; }
/* Atlas areas (512x256): column = native (235,0) 85x240 at atlas (256,0); top = native (0,0) 235x4 at (0,160);
   bottom = native (0,236) 235x4 at (0,164); left = native (0,4) 4x232 at (344,0). The DS-drawn map panel shows between. */
static void bnFrame(uint32_t *atlas, const char *const *lb, const char *const *al, const bool *on, int press, unsigned gameIco, int iconX[kBlButtonCount], int iconY[kBlButtonCount]) {
  const uint32_t BG = bnRGB(200, 208, 211), EDGE_HI = bnRGB(252, 253, 253), NAVY = bnRGB(7, 33, 54), WHITE = bnRGB(246, 247, 250), COL = bnRGB(77, 100, 114),
    COLDK = bnRGB(60, 80, 94), DIV_L1 = bnRGB(216, 223, 226), DIV_L2 = bnRGB(230, 234, 237), BTN = bnRGB(244, 247, 254), BTN_TOP = bnRGB(252, 253, 255),
    BTN_SHD = bnRGB(189, 201, 222), CELL = bnRGB(232, 237, 246),
    TEXT = bnRGB(18, 26, 38), SEL = bnRGB(46, 208, 155), SEL_SHD = bnRGB(24, 153, 115), SEL_CELL = bnRGB(40, 190, 142),
    SEL_TXT = bnRGB(255, 255, 255), SEL_TXT_O = bnRGB(0, 84, 64), TEXT_OFF = bnRGB(150, 160, 170);
  /* the frame pieces: drawn as one 320x240 picture through four windows */
  const SBnCanvas parts[4] = { { atlas, 256, 0, 235, 0, 85, 240 }, { atlas, 0, 160, 0, 0, 235, 4 }, { atlas, 0, 164, 0, 236, 235, 4 }, { atlas, 344, 0, 0, 4, 4, 232 } };
  for (const SBnCanvas &c : parts) {
    bnRect(c, 0, 0, 319, 239, BG); for (int x = 0; x < 246; x++) bnPut(c, x, 0, EDGE_HI); for (int y = 0; y < 240; y++) bnPut(c, 0, y, EDGE_HI);
    bnBox(c, 1, 2, 236, 237, NAVY, BG, BG, BG, 0);
    for (int x = 2; x < 236; x++) { bnPut(c, x, 3, WHITE); bnPut(c, x, 236, WHITE); } for (int y = 3; y < 237; y++) { bnPut(c, 2, y, WHITE); bnPut(c, 235, y, WHITE); }
    for (int y = 0; y < 240; y++) { bnPut(c, 246, y, DIV_L1); bnPut(c, 247, y, NAVY); bnPut(c, 248, y, DIV_L2); bnPut(c, 319, y, NAVY); }
    bnRect(c, 249, 0, 318, 239, COL); for (int x = 249; x < 319; x++) { bnPut(c, x, 188, NAVY); bnPut(c, x, 189, COLDK); } }
  const SBnCanvas &c = parts[0];
  for (int i = 0; i < kBlButtonCount; i++) { int x0, y0, x1, y1; bnRectOf(i, &x0, &y0, &x1, &y1); const bool sel = press == i;
    bnBox(c, x0, y0, x1, y1, NAVY, sel ? SEL : BTN, sel ? SEL : BTN_TOP, sel ? SEL_SHD : BTN_SHD, 2);
    if (i == 9 && gBnSprintOk && strcmp(lb[9], "NO SHOES")) { /* run 20: Sprint Button, its square outlined in the tab orange */
      const uint32_t *sp = gBnSprint[on[9] ? 1 : 0]; const uint32_t OR = bnRGB(184, 104, 16);
      for (int x = x0 + 1; x < x1; x++) bnPut(c, x, y0, OR); for (int y = y0 + 1; y < y1; y++) { bnPut(c, x0, y, OR); bnPut(c, x1, y, OR); }
      for (int y = 0; y < 32; y++) for (int x = 0; x < 31; x++) bnPut(c, x0 + 1 + x, y0 + 1 + y, sp[y * 31 + x]);
      iconX[i] = x0 + 1; iconY[i] = y0 + 1; continue; }
    if (i < 8) {
      bnButtonDetails(c, x0, y0, y1, 275, 317, sel);
      const char *t = bnTextW(lb[i]) <= 38 ? lb[i] : al[i]; if (!strcmp(t, "POKEDEX")) t = "POKeDEX"; else if (!strcmp(t, "POKEMON")) t = "POKeMON"; const int tx = 277 + (38 - bnTextW(t) + 1) / 2, ty = y0 + 1 + (18 - 9 + 1) / 2;
      if (sel) bnTextO(c, tx, ty, t, SEL_TXT, SEL_TXT_O); else bnText(c, tx, ty, t, on[i] ? TEXT : TEXT_OFF);
      iconX[i] = 253; iconY[i] = y0;   /* the game's 20x22 icon, centred in the cell */
      if (i == 7) bnMapIcon(c, 251, y0 + 1, 274, y0 + 20, on[i] || sel, sel ? SEL_CELL : CELL);
      /* run 22: no drawn fallback icon; the ROM icon is always there once the column is shown */ }
    else if (i == kBlItem1 || i == kBlItem2) {
      iconX[i] = (x0 + x1 + 1 - kBlItemIconSize) / 2;
      iconY[i] = (y0 + y1 + 1 - kBlItemIconSize) / 2;
      if (!(gameIco & (1u << i))) {
        const char *n = i == kBlItem1 ? "1" : "2";
        bnText(c, (x0 + x1 + 1 - bnTextW(n)) / 2, (y0 + y1 + 1 - 9) / 2, n, TEXT_OFF);
      }
    } else { iconX[i] = x0 + 1; iconY[i] = y0 + 1; }
  }
}

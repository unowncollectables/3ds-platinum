/* ss3ds: drawing for the SoulSilver settings app (bottom screen, 256x192, 0x00BBGGRR). Pure pixel code with no
   3DS calls, so tools/settings_preview.cpp can render it on a PC. render3ds.cpp's runApp owns input and state.
   Look: SoulSilver's own menus (summary / Pokegear): light steel-blue striped background, navy header bar,
   white panels with a blue-grey rim, the selected panel in a bright blue rim, values in the game's blue. */
#pragma once
#include <algorithm>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define SUI_RGB(r, g, b) ((uint32_t)(r) | (uint32_t)(g) << 8 | (uint32_t)(b) << 16)
struct SUiText { int (*draw)(uint32_t *img, int x, int y, const char *t, uint32_t fg, uint32_t sh); int (*width)(const char *t);
  int (*drawS)(uint32_t *img, int x, int y, const char *t, uint32_t fg, uint32_t sh); int (*widthS)(const char *t); /* optional 3/4-size text for tight bars; null = same as draw/width */ };
static inline int suiSDraw(const SUiText &T, uint32_t *b, int x, int y, const char *t, uint32_t fg, uint32_t sh) { return T.drawS ? T.drawS(b, x, y, t, fg, sh) : T.draw(b, x, y, t, fg, sh); }
static inline int suiSWidth(const SUiText &T, const char *t) { return T.widthS ? T.widthS(t) : T.width(t); }
struct SUiCell { const char *name, *value; bool sel, changed; };
static const uint32_t kSuiBgTop = SUI_RGB(208, 224, 248), kSuiBgBot = SUI_RGB(160, 188, 232), kSuiStripe = SUI_RGB(232, 240, 255),
  kSuiHead = SUI_RGB(48, 64, 112), kSuiHeadLine = SUI_RGB(200, 212, 236), kSuiTabTx = SUI_RGB(184, 200, 232), kSuiTabTxSh = SUI_RGB(32, 40, 80),
  kSuiTabOn = SUI_RGB(240, 244, 252), kSuiTabOnEdge = SUI_RGB(120, 144, 200), kSuiTabOnTx = SUI_RGB(40, 48, 88), kSuiTabOnSh = SUI_RGB(176, 188, 216),
  kSuiCell = SUI_RGB(248, 248, 252), kSuiCellLo = SUI_RGB(228, 236, 248), kSuiEdge = SUI_RGB(104, 120, 168),
  kSuiSelEdge = SUI_RGB(56, 128, 232), kSuiSelCell = SUI_RGB(232, 244, 255), kSuiSelLo = SUI_RGB(208, 228, 255),
  kSuiName = SUI_RGB(72, 72, 80), kSuiNameSh = SUI_RGB(208, 208, 216), kSuiVal = SUI_RGB(40, 88, 208), kSuiValSh = SUI_RGB(176, 196, 240),
  kSuiArrow = SUI_RGB(120, 136, 184), kSuiChanged = SUI_RGB(232, 72, 56), kSuiNote = SUI_RGB(112, 120, 144), kSuiNoteSh = SUI_RGB(216, 220, 232);
/* optional clip, in addition to the 256x192 screen. Defaults to the whole screen; the achievement list narrows it while a row slides across the header. */
static int suiClipX0 = 0, suiClipY0 = 0, suiClipX1 = 256, suiClipY1 = 192;
static inline void suiPx(uint32_t *b, int x, int y, uint32_t c) {
  if (x < suiClipX0 || y < suiClipY0 || x >= suiClipX1 || y >= suiClipY1) return;
  if ((unsigned)x < 256u && (unsigned)y < 192u) b[y * 256 + x] = c; }
static void suiFill(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t c) {
  x0 = std::max({x0, suiClipX0, 0});
  y0 = std::max({y0, suiClipY0, 0});
  x1 = std::min({x1, suiClipX1, 256});
  y1 = std::min({y1, suiClipY1, 192});
  if (x0 >= x1) return;
  for (int y = y0; y < y1; ++y) {
    uint32_t *row = b + y * 256;
    std::fill(row + x0, row + x1, c);
  }
}

static inline uint32_t suiMix(uint32_t a, uint32_t c, int t, int n) { /* a..c, t of n */
  uint32_t o = 0; for (int s = 0; s < 24; s += 8) { int x = (a >> s) & 255, y = (c >> s) & 255; o |= (uint32_t)(x + (y - x) * t / n) << s; } return o; }
/* rounded panel: 1 px rim (2 px when thick), top half `fill`, bottom half shaded towards `lo` like the game's panels */
static void suiPanel(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t fill, uint32_t lo, uint32_t edge, bool thick) {
  const int h = y1 - y0;
  for (int y = y0; y < y1; ++y) {
    const int dy = std::min(y - y0, y1 - 1 - y);
    const int cut = std::max(0, 2 - dy);
    const int left = x0 + cut, right = x1 - cut;
    if (left >= right) continue;

    // The diagonal rim reaches farther inward on rows near the corners.
    const int rim = std::max(thick ? 2 : 1, 3 - dy);
    if (dy == 0 || (thick && dy == 1) || x0 + rim >= x1 - rim) {
      suiFill(b, left, y, right, y + 1, edge);
      continue;
    }

    const uint32_t c = y - y0 < h / 2 ? fill : suiMix(fill, lo, y - y0 - h / 2, h - h / 2);
    suiFill(b, left, y, x0 + rim, y + 1, edge);
    suiFill(b, x0 + rim, y, x1 - rim, y + 1, c);
    suiFill(b, x1 - rim, y, right, y + 1, edge);
  }
}
static void suiArrow(uint32_t *b, int x, int y, int dir, uint32_t c) {
  for (int k = 0; k < 4; k++) for (int yy = k; yy < 7 - k; yy++) suiPx(b, dir > 0 ? x + k : x + 3 - k, y + yy, c); }
static void suiBackground(uint32_t *b) {
  for (int y = 0; y < 192; y++) { const uint32_t c = suiMix(kSuiBgTop, kSuiBgBot, y, 191);
    for (int x = 0; x < 256; x++) b[y * 256 + x] = ((x + y) & 15) < 2 ? suiMix(c, kSuiStripe, 1, 2) : c; } }
/* header bar with the tabs; returns nothing, tab hit-test is x * nt / 256 for y < 22 */
static void suiHeader(uint32_t *b, const SUiText &T, const char *const *tabs, int nt, int tab, const char *tabLabel) {
  for (int y = 0; y < 21; y++) for (int x = 0; x < 256; x++) b[y * 256 + x] = suiMix(kSuiHead, SUI_RGB(32, 44, 80), y, 20);
  for (int x = 0; x < 256; x++) b[21 * 256 + x] = kSuiHeadLine;
  for (int t = 0; t < nt; t++) { const int x0 = t * 256 / nt + 2, x1 = (t + 1) * 256 / nt - 2; const bool on = t == tab;
    const char *nm = on && tabLabel ? tabLabel : tabs[t];
    if (on) suiPanel(b, x0, 2, x1, 20, kSuiTabOn, kSuiCellLo, kSuiTabOnEdge, false);
    T.draw(b, (x0 + x1) / 2 - T.width(nm) / 2, 2, nm, on ? kSuiTabOnTx : kSuiTabTx, on ? kSuiTabOnSh : kSuiTabTxSh); } }
/* scrolling list of setting rows (DS y 24..143): name on the left, value between < > arrows on the right (arrows only on the
   selected row), a red dot when the value differs from when the app opened, a scroll bar when there are more than kSuiRows. */
enum { kSuiRows = 7, kSuiRowH = 16, kSuiRowPitch = 17, kSuiListY = 24, kSuiValX = 150 };
static void suiList(uint32_t *b, const SUiText &T, const SUiCell *c, int n, int top) {
  for (int k = 0; k < kSuiRows && top + k < n; k++) { const SUiCell &e = c[top + k];
    const int x0 = 4, x1 = n > kSuiRows ? 244 : 252, y0 = kSuiListY + k * kSuiRowPitch, y1 = y0 + kSuiRowH;
    suiPanel(b, x0, y0, x1, y1, e.sel ? kSuiSelCell : kSuiCell, e.sel ? kSuiSelLo : kSuiCellLo, e.sel ? kSuiSelEdge : kSuiEdge, e.sel);
    T.draw(b, x0 + 7, y0, e.name, kSuiName, kSuiNameSh);
    const int vc = (kSuiValX + x1 - 6) / 2; /* value centre */
    T.draw(b, vc - T.width(e.value) / 2, y0, e.value, kSuiVal, kSuiValSh);
    if (e.sel) { suiArrow(b, kSuiValX, y0 + 4, -1, kSuiSelEdge); suiArrow(b, x1 - 10, y0 + 4, 1, kSuiSelEdge); }
    if (e.changed) for (int q = 0; q < 3; q++) for (int r = 0; r < 3; r++) suiPx(b, x0 + 2 + q, y0 + 2 + r, kSuiChanged); }
  if (n > kSuiRows) { const int h = kSuiRows * kSuiRowPitch - 1, th = h * kSuiRows / n < 8 ? 8 : h * kSuiRows / n, ty = kSuiListY + (h - th) * top / (n - kSuiRows);
    for (int y = kSuiListY; y < kSuiListY + h; y++) for (int x = 247; x < 252; x++) suiPx(b, x, y, kSuiCellLo);
    for (int y = ty; y < ty + th; y++) for (int x = 247; x < 252; x++) suiPx(b, x, y, (x == 247 || x == 251) ? kSuiEdge : kSuiSelEdge); } }
/* word-wrap `t` into at most `maxLines` lines of at most `maxW` px (last line ends with ".." if cut); returns the line count */
enum { kSuiTipY = 146, kSuiTipLines = 2, kSuiTipLen = 96 };
static int suiWrap(const SUiText &T, const char *t, int maxW, char (*ln)[kSuiTipLen], int maxLines) {
  int nl = 0, len = 0; ln[0][0] = 0;
  for (const char *p = t; *p;) {
    while (*p == ' ') p++; if (!*p) break;
    int wl = 0; while (p[wl] && p[wl] != ' ') wl++;
    char cand[kSuiTipLen]; const int sp = len ? 1 : 0, cl = len + sp + wl;
    if (cl >= kSuiTipLen - 3) break;
    for (int i = 0; i < len; i++) cand[i] = ln[nl][i];
    if (sp) cand[len] = ' ';
    for (int i = 0; i < wl; i++) cand[len + sp + i] = p[i];
    cand[cl] = 0; p += wl;
    if (len && T.width(cand) > maxW) {
      if (nl + 1 >= maxLines) { ln[nl][len] = '.'; ln[nl][len + 1] = '.'; ln[nl][len + 2] = 0; return nl + 1; }
      nl++; for (int i = 0; i < wl; i++) ln[nl][i] = cand[len + sp + i]; ln[nl][wl] = 0; len = wl; }
    else { for (int i = 0; i <= cl; i++) ln[nl][i] = cand[i]; len = cl; } }
  return len ? nl + 1 : 0; }
/* tooltip panel under the list: the selected setting's description, up to two wrapped lines */
static void suiTip(uint32_t *b, const SUiText &T, const char *tip) {
  suiPanel(b, 4, kSuiTipY, 252, 190, kSuiCell, kSuiCellLo, kSuiEdge, false);
  char ln[kSuiTipLines][kSuiTipLen]; const int n = suiWrap(T, tip ? tip : "", 232, ln, kSuiTipLines);
  for (int i = 0; i < n; i++) T.draw(b, 10, kSuiTipY + 3 + i * 18, ln[i], kSuiNote, kSuiNoteSh); }
/* quick-open button drawn over the game's bottom screen in the overworld: 26 x 14 panel in the corner (side 0 = top-left,
   1 = top-right) with three bars like a menu icon; pressed = the bright selected look. Hit-test with suiBtnHit. */
enum { kSuiBtnW = 26, kSuiBtnH = 14 };
static inline int suiBtnX(int side) { return side ? 256 - 2 - kSuiBtnW : 2; }
static inline bool suiBtnHit(int side, int x, int y) { const int x0 = suiBtnX(side); return x >= x0 - 3 && x < x0 + kSuiBtnW + 3 && y < 2 + kSuiBtnH + 3; }
static void suiMenuButton(uint32_t *b, int side, bool pressed) {
  const int x0 = suiBtnX(side), y0 = 2;
  suiPanel(b, x0, y0, x0 + kSuiBtnW, y0 + kSuiBtnH, pressed ? kSuiSelCell : kSuiCell, pressed ? kSuiSelLo : kSuiCellLo, pressed ? kSuiSelEdge : kSuiEdge, pressed);
  for (int k = 0; k < 3; k++) for (int x = x0 + 7; x < x0 + kSuiBtnW - 7; x++) { suiPx(b, x, y0 + 3 + k * 3, kSuiVal); suiPx(b, x, y0 + 4 + k * 3, pressed ? kSuiVal : kSuiValSh); } }

/* 2026-11-19 restyle (Nathan: "match the main bottom menu, almost feel built in"): the settings app in the Johto panel's look
   (ss3ds_blnative.h / ss3ds_pgmap.h colours): mint background, white tab boxes with a navy rim and the selected one green, a green
   band with the tab's name, white row cards with ON / OFF pills, a green footer with the buttons. The description moved to the top
   screen (suiAppTop), so the list has the room. Same 256x192 canvas; text goes through the sharp text recorder like before. */
static const uint32_t kSaInk = SUI_RGB(7, 33, 54), kSaText = SUI_RGB(18, 26, 38), kSaPaper = SUI_RGB(248, 252, 250), kSaPaperLo = SUI_RGB(232, 244, 238),
  kSaEdge = SUI_RGB(190, 214, 206), kSaGreen = SUI_RGB(46, 208, 155), kSaGreenD = SUI_RGB(24, 153, 115), kSaGreenO = SUI_RGB(0, 84, 64),
  kSaBandA = SUI_RGB(40, 170, 128), kSaBandB = SUI_RGB(20, 140, 104), kSaMintA = SUI_RGB(214, 240, 230), kSaMintB = SUI_RGB(168, 218, 196),
  kSaSelFill = SUI_RGB(232, 250, 242), kSaOffFill = SUI_RGB(214, 222, 220), kSaDim = SUI_RGB(104, 122, 118), kSaWhite = SUI_RGB(255, 255, 255),
  kSaGold = SUI_RGB(255, 229, 155), kSaMint = SUI_RGB(218, 238, 228), kSaChanged = SUI_RGB(232, 72, 56);
enum { kSaTabH = 19, kSaBandY = 21, kSaBandH = 13, kSaListY = 37, kSaRowH = 17, kSaRowPitch = 19, kSaRows = 7, kSaFootY = 174, kSaValX = 150 };
static void saBox(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t fill, uint32_t edge, int rim) { /* cut-corner box, `rim` px edge */
  for (int y = y0; y < y1; y++) { const int dy = std::min(y - y0, y1 - 1 - y), cut = std::max(0, 2 - dy);
    for (int x = x0 + cut; x < x1 - cut; x++) { const int dx = std::min(x - x0, x1 - 1 - x); suiPx(b, x, y, (dy < rim || dx < rim || dx + dy < 2 + rim) ? edge : fill); } } }
static void saGrad(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t a, uint32_t c) { for (int y = y0; y < y1; y++) suiFill(b, x0, y, x1, y + 1, suiMix(a, c, y - y0, std::max(1, y1 - y0 - 1))); }
/* a small rounded pill with centred small text; returns its left x */
static int saPill(uint32_t *b, const SUiText &T, int xr, int y0, int h, const char *t, uint32_t fill, uint32_t ink, int minW) {
  const int w = std::max(minW, suiSWidth(T, t) + 10), x0 = xr - w; saBox(b, x0, y0, xr, y0 + h, fill, fill, 1);
  suiSDraw(T, b, x0 + (w - suiSWidth(T, t)) / 2, y0 + (h - 12) / 2, t, ink, fill); return x0; }
static bool saIsOff(const char *v) { return v && (!strcmp(v, "OFF") || !strcmp(v, "Off")); }
static bool saIsOn(const char *v) { return v && (!strcmp(v, "ON") || !strcmp(v, "On")); }
/* whole bottom screen. tabs[nt], `tab` selected; cells[n] rows from `top`; tab hit test: y < kSaTabH, tab = x * nt / 256. */
static void suiAppBottom(uint32_t *b, const SUiText &T, const char *const *tabs, int nt, int tab, const SUiCell *c, int n, int top) {
  for (int y = 0; y < 192; y++) suiFill(b, 0, y, 256, y + 1, suiMix(kSaMintA, kSaMintB, y, 191));
  for (int t = 0; t < nt; t++) { const int x0 = t * 256 / nt + 2, x1 = (t + 1) * 256 / nt - 2; const bool on = t == tab;
    saBox(b, x0, 2, x1, kSaTabH, on ? kSaGreen : SUI_RGB(252, 253, 255), kSaInk, 1);
    if (on) suiFill(b, x0 + 2, kSaTabH - 3, x1 - 2, kSaTabH - 1, kSaGreenD);
    suiSDraw(T, b, (x0 + x1) / 2 - suiSWidth(T, tabs[t]) / 2, 4, tabs[t], on ? kSaGreenO : kSaInk, on ? kSaGreen : kSaWhite); }
  saGrad(b, 2, kSaBandY, 254, kSaBandY + kSaBandH, kSaBandA, kSaBandB);
  suiSDraw(T, b, 7, kSaBandY, tabs[tab], kSaWhite, kSaGreenO);
  { char cn[24]; snprintf(cn, sizeof cn, n == 1 ? "1 setting" : "%d settings", n); suiSDraw(T, b, 249 - suiSWidth(T, cn), kSaBandY, cn, SUI_RGB(225, 250, 240), kSaGreenO); }
  const bool scroll = n > kSaRows;
  for (int k = 0; k < kSaRows && top + k < n; k++) { const SUiCell &e = c[top + k];
    const int x0 = 3, x1 = scroll ? 246 : 253, y0 = kSaListY + k * kSaRowPitch, y1 = y0 + kSaRowH;
    saBox(b, x0, y0, x1, y1, e.sel ? kSaSelFill : kSaPaper, e.sel ? kSaGreen : kSaEdge, e.sel ? 2 : 1);
    T.draw(b, x0 + 6, y0, e.name, kSaText, e.sel ? kSaSelFill : kSaPaper);
    const bool off = saIsOff(e.value), on = saIsOn(e.value);
    const int xr = x1 - (e.sel ? 11 : 4);
    const int px0 = saPill(b, T, xr, y0 + 3, 12, e.value, on ? kSaGreenD : off ? kSaOffFill : kSaInk, off ? kSaDim : kSaWhite, 30);
    if (e.sel) { suiArrow(b, px0 - 6, y0 + 5, -1, kSaGreenD); suiArrow(b, xr + 3, y0 + 5, 1, kSaGreenD); }
    if (e.changed) for (int q = 0; q < 3; q++) for (int r = 0; r < 3; r++) suiPx(b, x0 + 2 + q, y0 + 2 + r, kSaChanged); }
  if (scroll) { const int h = kSaRows * kSaRowPitch - 2, th = std::max(10, h * kSaRows / n), ty = kSaListY + (h - th) * top / (n - kSaRows);
    saBox(b, 248, kSaListY, 254, kSaListY + h, SUI_RGB(196, 226, 214), SUI_RGB(196, 226, 214), 1); saBox(b, 248, ty, 254, ty + th, kSaGreenD, kSaGreenD, 1); }
  saGrad(b, 0, kSaFootY, 256, 192, kSaBandB, kSaGreenO);
  int x = 5; static const char *const kKey[3][2] = { { "L R", "tab" }, { "A", "change" }, { "B", "close" } };
  for (auto &k : kKey) { const int w = suiSWidth(T, k[0]) + 10; saBox(b, x, kSaFootY + 3, x + w, 189, kSaWhite, kSaWhite, 1);
    suiSDraw(T, b, x + 5, kSaFootY + 3, k[0], kSaGreenO, kSaWhite); x += w + 3; suiSDraw(T, b, x, kSaFootY + 3, k[1], SUI_RGB(230, 255, 245), kSaBandB); x += suiSWidth(T, k[1]) + 9; } }
/* top screen card over the dimmed game: title band, build chips, then the selected setting's name and description.
   `draw`/`width` = the game font at normal size, `drawS`/`widthS` = 3/4 size (T as above). ver = "run N (hash)", date = __DATE__. */
static void suiAppTop(uint32_t *b, const SUiText &T, const char *ver, const char *date, const char *tabName, const char *name, const char *tip) {
  const int X0 = 12, X1 = 244, Y0 = 10, Y1 = 182;
  saBox(b, X0, Y0, X1, Y1, kSaPaper, kSaInk, 1);
  saGrad(b, X0 + 1, Y0 + 1, X1 - 1, Y0 + 18, kSaBandA, kSaBandB);
  { const int cx = X0 + 11, cy = Y0 + 9; for (int y = -6; y <= 6; y++) for (int x = -6; x <= 6; x++) { const int d = x * x + y * y; if (d <= 36) suiPx(b, cx + x, cy + y, d <= 6 ? kSaGreenD : kSaWhite); } }
  T.draw(b, X0 + 21, Y0 + 1, "SETTINGS", kSaWhite, kSaGreenO);
  suiSDraw(T, b, X1 - 6 - suiSWidth(T, "GAME PAUSED"), Y0 + 3, "GAME PAUSED", kSaGold, kSaBandB);
  T.draw(b, X0 + 8, Y0 + 20, "Pok\xe9mon Platinum for New 3DS", kSaText, kSaPaper);
  int run = 0; char hash[16] = "", c1[24], c2[24]; sscanf(ver ? ver : "", "run %d (%15[^)])", &run, hash);
  char mon[4] = "", yr[8] = ""; int day = 0; sscanf(date ? date : "", "%3s %d %7s", mon, &day, yr);
  snprintf(c1, sizeof c1, "Build %d", run); snprintf(c2, sizeof c2, "%s %d, %s", mon, day, yr);
  int x = X0 + 8; for (const char *t : { (const char *)c1, (const char *)c2, (const char *)hash }) { if (!*t) continue; const int w = suiSWidth(T, t) + 10;
    saBox(b, x, Y0 + 38, x + w, Y0 + 52, kSaMint, kSaEdge, 1); suiSDraw(T, b, x + 5, Y0 + 39, t, kSaGreenO, kSaMint); x += w + 4; }
  suiFill(b, X0 + 8, Y0 + 58, X1 - 8, Y0 + 59, kSaEdge);
  { char crumb[64]; snprintf(crumb, sizeof crumb, "%s  >  %s", tabName ? tabName : "", name ? name : ""); suiSDraw(T, b, X0 + 8, Y0 + 61, crumb, kSaDim, kSaPaper); }
  T.draw(b, X0 + 8, Y0 + 74, name ? name : "", kSaGreenO, kSaPaper);
  char ln[3][kSuiTipLen]; const int nl = suiWrap(T, tip ? tip : "", X1 - X0 - 16, ln, 3);
  for (int i = 0; i < nl; i++) T.draw(b, X0 + 8, Y0 + 92 + i * 14, ln[i], SUI_RGB(60, 74, 80), kSaPaper);
  saBox(b, X0 + 8, Y1 - 22, X1 - 8, Y1 - 6, kSaMint, kSaEdge, 1);
  suiSDraw(T, b, X0 + 14, Y1 - 21, "Changes are saved when you close.", kSaGreenO, kSaMint); }

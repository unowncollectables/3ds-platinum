/* ss3ds 2026-10-28 (Nathan: "polish Pokegear map, phone list and radio"): the JOHTO bottom screen's left panel gets three
   tabs on its top bar (MAP / PHONE / RADIO / PARTY / BAG; run 122 restyles them as even rounded JOHTO chips). Setting `pg_extras` (Settings > Input,
   defaults to ORIGINAL). OFF hides the tabs. ORIGINAL embeds native Phone and Radio. The lists below describe PREVIEWS.
   MAP    unchanged map plus pins on places where a phone contact lives (blue = number registered, amber = wants a rematch)
          and a "Phone:" line on the info card (pgDraw calls pxMapPins / pxCardLine).
   PHONE  the Pokegear's registered numbers (hook 0122 reads the save's phone book, the ROM's tel/pmtel_book.dat places and
          the game's own rematch/gift flags). Drag to scroll with a fling, drag the right edge to scrub, SORT chip cycles
          A-Z / PLACE / REMATCH, tap a row = a real outgoing call (hook 0128 opens the game's Pokegear on the phone app and
          starts the game's own call, scripts and all), tap the MAP chip at the right of a row = jump to it on the map.
   RADIO  tap a station = the Pokegear opens on its radio already tuned to it (hook 0129; needs the Radio Card, the game's gate).
          The stations you can tune where you stand right now (the game's Radio_GetAvailableChannels, published by the same
          hook) with what is on the air this hour: the real rotations from pokegear_radio.c / pokemon_music.c, from the
          3DS clock.
   Read-only: nothing here writes the save. Logs: [PHONE] and [RADIO]. */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "ss3ds_settings_ui.h"

extern "C" {
/* published by hook 0122 on the game thread (field_control.c) */
volatile int P3DS_PhReady, P3DS_RadSel, P3DS_RadNat, P3DS_RadGb, P3DS_RadCard, P3DS_PhRing, P3DS_PhCallReq, P3DS_RadTuneReq; /* Requests: -1 opens the app in the panel without calling/tuning. RadTuneReq: station id + 1 to tune from the RADIO tab (hook 0129), 0 none; PhCallReq: contact id + 1 to call from the PHONE tab (hook 0128), 0 none; RadCard: Radio card registered; PhRing: 0 quiet, 1 ringing (caller unknown), 2+id = contact id ringing (hook 0125) */ volatile uint32_t P3DS_PhTick;
/* published by hook 0167 (field_control.c / field_system.c) while a PARTY or BAG tab is open: party slots {species, level, hp, max hp, held item, status bits, egg}, every bag slot of the eight pockets in the game's pocket order (ITEMS, MEDICINE, BALLS, TM/HM, BERRIES, MAIL, BATTLE, KEY), item names read from the game's own text */
volatile int P3DS_VwOn, P3DS_VwReady; volatile unsigned P3DS_VwTick; int P3DS_VwPartyN; uint16_t P3DS_VwParty[6][7]; uint16_t P3DS_VwBagId[8][165], P3DS_VwBagQty[8][165]; char P3DS_VwItem[540][20]; /* run 119 / hook 0200: row tap selects this mon/item when opening Party/Bag */ volatile int P3DS_VwPartySlot; /* 0 none, 1..6 = party index+1 */ volatile int P3DS_VwBagPocket; /* -1 none, 0..7 pocket */ volatile int P3DS_VwBagSlot; /* -1 none, pocket slot index */
/* run 145 / hook 0207 (panel_live): one live action for the game thread. Req 1 = swap party slots A and B, 2 = use item A on party slot B;
   the game clears Req, sets Res (1 swapped, 2 used, -2 lead change, -3 no effect, -4 needs the full menu, -5 not in the bag, -6 bad) and bumps Done */
volatile int P3DS_LvReq, P3DS_LvA, P3DS_LvB, P3DS_LvRes; volatile unsigned P3DS_LvDone;
/* published by hook 0210 (field_control.c / field_system.c) while the BREED tab is open: layout in pspoke-patches/0210 (d[0..32]) */
volatile int P3DS_BrOn, P3DS_BrReady; volatile unsigned P3DS_BrTick; int P3DS_BrD[40];
uint8_t P3DS_PhFlag[75], P3DS_PhLoc[75]; /* flag bit0 registered, bit1 rematch, bit2 gift; loc = Pokegear location 0..99 or 255 */
}

static const char *const kPxName[75] = {
  "Mom", "Prof. Elm", "Prof. Oak", "Ethan", "Lyra", "Kurt", "Day-Care Man", "Day-Care Lady", "Buena", "Bill",
  "Joey", "Ralph", "Liz", "Wade", "Anthony", "Bike Shop", "Kenji", "Whitney", "Falkner", "Jack",
  "Chad", "Brent", "Todd", "Arnie", "Baoba", "Irwin", "Janine", "Clair", "Erika", "Misty",
  "Blaine", "Blue", "Chuck", "Brock", "Bugsy", "Sabrina", "Lt. Surge", "Morty", "Jasmine", "Pryce",
  "Huey", "Gaven", "Jamie", "Reena", "Vance", "Parry", "Erin", "Beverly", "Jose", "Gina",
  "Alan", "Dana", "Derek", "Tully", "Tiffany", "Wilton", "Krise", "Ian", "Walt", "Alfred",
  "Doug", "Rob", "Kyle", "Kyler", "Tim & Sue", "Kenny", "Tanner", "Josh", "Torin", "Hillary",
  "Billy", "Kay & Tia", "Reese", "Aiden", "Ernest" };

enum { kPxRowH = 17, kPxSubH = 14 };
struct SPx {
  int pane;                       /* 0 MAP, 1 PHONE, 2 RADIO, 3 PARTY, 4 BAG, 5 EGGS = the breeding tab (breed_tab) */
  unsigned brSeen; bool brLogged; int brLast[8]; /* BREED: last data tick, first-open logged, last logged facts */
  int bagLX0, bagLX1, bagRX0, bagRX1; /* BAG pocket arrows, set while drawing */
  int bag, bagN, bagList[165]; unsigned vwSeen; int vwWait; bool vwLogged; /* BAG pocket (0..7), its non-empty slots, last data tick, frames waiting for data */
  int sort;                       /* 0 A-Z, 1 PLACE, 2 REMATCH first */
  int ord[75], n; uint32_t tick; bool built;
  float scroll, vel; int lastY; bool scrub, touching;
  int sortX0, sortX1;             /* SORT chip, set while drawing */
  int listY0, listY1, listX1;     /* list geometry, set while drawing */
  int locN[100]; uint8_t locRe[100]; /* contacts per Pokegear location, any wants a rematch */
  int minute;                     /* radio pane redraws when the minute changes */
  bool loggedPhone, loggedRadio;
  int radN, radSt[5], radY0[5], radY1[5], radX0, radX1; /* RADIO pane rows (game station ids and rects), set while drawing */
  int pressId; bool moved;        /* contact under the finger (-1 none) for the pressed highlight */
  int lvSel, lvItem, lvPocket, lvSlot; /* run 145 panel_live: picked party slot + 1 (0 none); picked medicine id (0 none), its pocket and slot */
  int lvSheet, lvSheetSlot, lvBtn[3][4], lvBtnN; /* run 147: action card over the live pane (1 party Pokemon, 2 bag item), its target, button rects x0 y0 x1 y1 */
  int lvWait, lvKind, lvB, lvOpen, lvOpenSlot, lvMenuX0; unsigned lvSeen; /* frames waiting for the game, last request, open-the-real-menu request (1 party, 2 bag), header chip x */
};
static SPx gPx;
/* pxOn() / pxOnV (the pg_extras setting) live in render3ds.cpp next to the settings rows */
static inline int pxPane() { return pxOn() ? gPx.pane : 0; }

/* game-style button: vertical gradient plus a navy frame; `hot` = pressed look */
static void pxBtn(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t hi, uint32_t lo, uint32_t edge) {
  for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) suiPx(b, x, y, suiMix(hi, lo, y - y0, y1 - y0));
  mmFrame(b, x0, y0, x1, y1, edge); }
/* pane backdrop: the Pokegear's blue, with a navy title strip of height h that shows white text */
static void pxBack(uint32_t *b, int X0, int Y0, int X1, int Y1, int h) {
  for (int y = Y0; y < Y1; y++) for (int x = X0; x < X1; x++) suiPx(b, x, y, suiMix(SUI_RGB(214, 240, 230), SUI_RGB(150, 210, 186), y - Y0, Y1 - Y0));
  for (int y = Y0; y < Y0 + h; y++) for (int x = X0; x < X1; x++) suiPx(b, x, y, suiMix(kBlPress, kBlBlue, y - Y0, h));
  for (int x = X0; x < X1; x++) suiPx(b, x, Y0 + h, kBlNavy); }
static void pxFit(const SUiText &T, char *out, int sz, const char *s, int maxW) {
  snprintf(out, sz, "%s", s); int k = (int)strlen(out);
  if (maxW <= 0) { out[0] = 0; return; }
  if (T.width(out) <= maxW) return;
  /* run 118: hard-chop then ".." ellipsis so names are not silently truncated mid-word */
  while (k > 0) {
    if (k + 2 < sz) { out[k] = '.'; out[k + 1] = '.'; out[k + 2] = 0; } else { out[k] = 0; }
    if (T.width(out) <= maxW) return;
    k--; out[k] = 0;
  }
  out[0] = 0; }

/* 2026-11-18: empty / waiting messages as a card (the pane's icon, then the lines), the same box as the MAP tab's cards,
   instead of loose white text on the backdrop. The icon is left out when a line would not fit beside it. */
static void pxNote(uint32_t *b, const SUiText &T, int X0, int Y, int X1, int icon, const char *l0, const char *l1 = 0, const char *l2 = 0, const char *l3 = 0) {
  const char *ln[4] = { l0, l1, l2, l3 }; int n = 0, w = 0; while (n < 4 && ln[n]) { if (T.width(ln[n]) > w) w = T.width(ln[n]); n++; }
  const int x0 = X0 + 3, x1 = X1 - 3, y1 = Y + 8 + n * 10 + 4; pgCard(b, x0, Y, x1, y1 < Y + 26 ? Y + 26 : y1);
  int tx = x0 + 6; if (icon >= 0 && tx + 17 + w <= x1 - 4) { blIcon(b, x0 + 5, Y + 6, icon, kBlIcInk, kBlIcAcc); tx += 17; }
  for (int i = 0; i < n; i++) T.draw(b, tx, Y + 4 + i * 10, ln[i], i ? kPgDim : kPgInk, kPgPaper); }

/* ---- phone list ---- */
static int pxCmpMode;
static int pxCmp(const void *pa, const void *pb) {
  const int a = *(const int *)pa, b = *(const int *)pb;
  if (pxCmpMode == 2) { const int ra = (P3DS_PhFlag[a] >> 1) & 1, rb = (P3DS_PhFlag[b] >> 1) & 1; if (ra != rb) return rb - ra; }
  if (pxCmpMode == 1) { const int la = P3DS_PhLoc[a], lb = P3DS_PhLoc[b]; if ((la == 255) != (lb == 255)) return la == 255 ? 1 : -1;
    if (la != 255) { const int c = strcmp(P3DS_PgName[la], P3DS_PgName[lb]); if (c) return c; } }
  return strcmp(kPxName[a], kPxName[b]); }
static void pxBuild() {
  gPx.tick = P3DS_PhTick; gPx.n = 0; memset(gPx.locN, 0, sizeof gPx.locN); memset(gPx.locRe, 0, sizeof gPx.locRe);
  for (int i = 0; i < 75; i++) if (P3DS_PhFlag[i] & 1) { gPx.ord[gPx.n++] = i; const int l = P3DS_PhLoc[i];
    if (l >= 0 && l < 100) { gPx.locN[l]++; if (P3DS_PhFlag[i] & 2) gPx.locRe[l] = 1; } }
  pxCmpMode = gPx.sort; qsort(gPx.ord, gPx.n, sizeof(int), pxCmp); gPx.built = true; }
static inline bool pxScrolls() { return (pxPane() == 1 && !pxNativeGear(1)) || pxPane() == 4; } /* panes with a drag-to-scroll list */
static inline int pxMaxScroll() { const int h = gPx.listY1 - gPx.listY0; const int m = (gPx.pane == 4 ? gPx.bagN : gPx.n) * kPxRowH - h; return m > 0 ? m : 0; }

/* ---- tab bar (run 122: even-width rounded JOHTO tabs; hit rects match draw) ---- */
enum { kPxMaxTab = 6, kPxTabGap = 3, kPxTabPad = 3 };
static bool brCfg();
static inline int pxNTab() { return brCfg() ? 6 : 5; } /* the BREED tab only exists with breed_tab on */
static const char *const kPxTabs[kPxMaxTab] = { "MAP", "PHONE", "RADIO", "PARTY", "BAG", "EGGS" };
static const char *const kPxTabShort[kPxMaxTab] = { "MAP", "TEL", "FM", "PKM", "BAG", "EGG" };
static void pxChips(const SUiText &T, int X1, int Y0, int *x0, int *x1, int *y0, int *y1) {
  /* run 142: the tabs span the whole bar in five even buttons (the name has its own strip below, pxNameStrip), and use the side column's silver button look. X1 is the panel's right edge, the bar starts at kBlPanX0. */
  (void)T; const int L = kBlPanX0 + 2, R = X1 - 2, N = pxNTab(), W = R - L - (N - 1) * kPxTabGap;
  for (int i = 0; i < N; i++) { x0[i] = L + i * kPxTabGap + W * i / N; x1[i] = L + i * kPxTabGap + W * (i + 1) / N; }
  *y0 = Y0 + 1; *y1 = Y0 + 15; }
/* run 142: the area name on its own full-width line under the tab row (MAP pane only; the map panel starts below it, see blNameH).
   The selected tab's word sits in a green chip on the left; the place name keeps the rest of the line. */
static void pxNameStrip(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, const char *name) {
  const int y0 = Y0 + 15, y1 = y0 + kBlNameH + 1;
  /* run 147: a navy header (not a third silver button): the place name in white on a deep blue gradient, like the panel's other title strips */
  for (int y = y0; y < y1; y++) { const uint32_t c = suiMix(kBlPress, kBlBlue, y - y0, y1 - y0); for (int x = X0; x < X1; x++) suiPx(b, x, y, c); }
  mmFrame(b, X0, y0, X1, y1, kBlNavy);
  const int ty = y0 + (y1 - y0 - 9) / 2;
  const char *cap = (gPx.pane >= 0 && gPx.pane < pxNTab()) ? kPxTabs[gPx.pane] : "MAP";
  const int cw = suiSWidth(T, cap) + 8, cx0 = X0 + 2, cx1 = cx0 + cw;
  int nx = X0 + 4;
  if (cx1 + 16 < X1) {
    for (int y = y0 + 1; y < y1 - 1; y++) for (int x = cx0; x < cx1; x++) suiPx(b, x, y, (x == cx0 || x == cx1 - 1 || y == y0 + 1 || y == y1 - 2) ? kBlSelO : kBlPress);
    suiSDraw(T, b, cx0 + 4, ty, cap, SUI_RGB(255, 255, 255), kBlSelO); nx = cx1 + 4; }
  char nm[48]; pxFit(T, nm, sizeof nm, name, X1 - nx - 4);
  suiSDraw(T, b, nx, ty, nm, SUI_RGB(255, 255, 255), kBlSelO); }
/* tab icons: same art as the column. When the bottom picture is scaled, render3ds draws these again at native size (bnTabStrip). */
static const int kPxTabIcon[kPxMaxTab] = { 7, 3, 10, 1, 2, 11 }; /* map, pokegear, radio, ball, bag, egg */
/* The native strip adds labels where they fit. This DS-size fallback retains the same button details. */
static void pxBar(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, const char *name) {
  (void)X0; (void)name; int cx0[kPxMaxTab], cx1[kPxMaxTab], cy0, cy1; pxChips(T, X1, Y0, cx0, cx1, &cy0, &cy1);
  for (int i = 0; i < pxNTab(); i++) {
    const bool on = gPx.pane == i;
    const int x0 = cx0[i], x1 = cx1[i], divider = x1 - 5;
    blBox(b, x0, cy0, x1, cy1, on);
    blButtonDetails(b, x0, cy0, cy1, divider, x1 - 3, on);
    blIcon(b, (x0 + 1 + divider - 12) / 2, (cy0 + cy1 - 12) / 2,
           kPxTabIcon[i], on ? kBlSelO : kBlIcInk, on ? SUI_RGB(255, 255, 255) : kBlIcAcc);
  }
}
/* tab index under bar x, -1 none (same layout as pxBar) */
static int pxChipHit(const SUiText &T, int X1, int Y0, int x) {
  int cx0[kPxMaxTab], cx1[kPxMaxTab], cy0, cy1; pxChips(T, X1, Y0, cx0, cx1, &cy0, &cy1);
  for (int i = 0; i < pxNTab(); i++) if (x >= cx0[i] - 1 && x < cx1[i] + 1) return i; return -1; }

/* ---- map extras (called from pgDraw) ---- */
static void pxMapPins(uint32_t *b, const SPg &p, int X0, int Y0, int X1, int oy) {
  if (!pxOn() || !P3DS_PhReady) return;
  if (!gPx.built || gPx.tick != P3DS_PhTick) pxBuild();
  for (int l = 0; l < 100; l++) if (gPx.locN[l]) { int x, y, w, h; pgSpecRect(l, &x, &y, &w, &h);
    const int mx = x * 8 + w * 8 - 2, my = y * 8 + 2; if (pgHiddenAt(p, x * 8 + w * 4, y * 8 + h * 4)) continue;
    int px, py; pgToDs(p, X0, Y0, oy, mx, my, &px, &py); if (px < X0 + 3 || px >= X1 - 3) continue;
    const bool re = gPx.locRe[l]; const uint32_t fill = re ? SUI_RGB(255, 200, 40) : SUI_RGB(46, 208, 155);
    /* 2026-11-18: a smaller pin (was 7 px across) so a town with several contacts is not buried under diamonds */
    for (int yy = -2; yy <= 2; yy++) for (int xx = -2; xx <= 2; xx++) { const int d = abs(xx) + abs(yy); if (d <= 2) suiPx(b, px + xx, py + yy, d == 2 ? kBlNavy : fill); } } }
/* "Phone: Joey, Wade" for the info card of location `loc` into out (empty if none); true if there is something to show */
static bool pxCardText(const SUiText &T, char *out, int sz, int loc, int maxW) {
  out[0] = 0; if (!pxOn() || !P3DS_PhReady || loc < 0 || loc >= 100) return false;
  char t[100] = "Phone: "; int n = 0;
  for (int i = 0; i < 75; i++) if ((P3DS_PhFlag[i] & 1) && P3DS_PhLoc[i] == loc) { const size_t L = strlen(t); if (L + 20 >= sizeof t) { strcat(t, ".."); break; } snprintf(t + L, sizeof t - L, "%s%s%s", n ? ", " : "", kPxName[i], (P3DS_PhFlag[i] & 2) ? "!" : ""); n++; }
  if (!n) return false; pxFit(T, out, sz, t, maxW); return true; }

static inline int pxMapChipW(const SUiText &T) { return T.width("MAP") + 7; }
/* ---- the PHONE pane ---- */
static void pxPhone(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const uint32_t ink = SUI_RGB(7, 33, 54), sh = SUI_RGB(244, 247, 254), fg = SUI_RGB(18, 26, 38);
  if (!gPx.built || gPx.tick != P3DS_PhTick) pxBuild();
  const uint32_t wh = SUI_RGB(255, 255, 255);
  pxBack(b, X0, Y0, X1, Y1, kPxSubH - 1);
  char t[48]; snprintf(t, sizeof t, "TAP TO CALL (%d)", gPx.n); T.draw(b, X0 + 4, Y0 + 2, t, wh, kBlNavy);
  static const char *const kSort[3] = { "A-Z", "PLACE", "REMATCH" }; const int sw = T.width(kSort[gPx.sort]) + 8;
  gPx.sortX1 = X1 - 3; gPx.sortX0 = gPx.sortX1 - sw;
  pxBtn(b, gPx.sortX0, Y0 + 1, gPx.sortX1, Y0 + kPxSubH - 2, SUI_RGB(250, 252, 255), kBlSilverLo, kBlNavy); T.draw(b, gPx.sortX0 + 4, Y0 + 1, kSort[gPx.sort], ink, sh);
  gPx.listY0 = Y0 + kPxSubH; gPx.listY1 = Y1; gPx.listX1 = X1;
  if (!P3DS_PhReady) { pxNote(b, T, X0, gPx.listY0 + 4, X1, 3, "Contacts load after", "your first step in the field."); return; }
  if (!gPx.n) { pxNote(b, T, X0, gPx.listY0 + 4, X1, 3, "No phone numbers yet.", "Trainers and friends you", "register show up here."); return; }
  const int ms = pxMaxScroll(); if (gPx.scroll > ms) { gPx.scroll = (float)ms; gPx.vel = 0; } if (gPx.scroll < 0) { gPx.scroll = 0; gPx.vel = 0; }
  const int sc = (int)gPx.scroll, nr = (gPx.listY1 - gPx.listY0) / kPxRowH + 2, r0 = sc / kPxRowH, listR = X1 - (ms > 0 ? 13 : 3);
  for (int r = r0; r < r0 + nr && r < gPx.n; r++) { const int id = gPx.ord[r]; const int y0 = gPx.listY0 + r * kPxRowH - sc, y1 = y0 + kPxRowH; if (y1 <= gPx.listY0 || y0 >= gPx.listY1) continue;
    const bool hot = gPx.touching && !gPx.moved && gPx.pressId == id, inside = y0 + 1 >= gPx.listY0 && y1 - 1 <= gPx.listY1;
    if (inside) pxBtn(b, X0 + 2, y0 + 1, listR, y1 - 1, hot ? kBlPress : SUI_RGB(252, 253, 255), hot ? kBlPress : (r & 1) ? SUI_RGB(220, 238, 232) : SUI_RGB(236, 246, 242), kBlNavy);
    else for (int y = y0; y < y1; y++) { if (y < gPx.listY0 || y >= gPx.listY1) continue; for (int x = X0 + 2; x < listR; x++) suiPx(b, x, y, SUI_RGB(236, 246, 242)); }
    const int ty = y0 + 4; if (!inside) continue; /* text rows only when fully inside the list */
    const uint8_t f = P3DS_PhFlag[id]; int bx = X0 + 5;
    if (f & 2) { for (int yy = 0; yy < 10; yy++) for (int xx = 0; xx < 10; xx++) { const int d = (xx - 5) * (xx - 5) + (yy - 5) * (yy - 5); if (d <= 25) suiPx(b, bx + xx, y0 + 3 + yy, d > 16 ? kBlNavy : SUI_RGB(255, 200, 40)); } T.draw(b, bx + 3, y0 + 2, "!", kBlNavy, SUI_RGB(255, 200, 40)); }
    bx += 12;
    if (f & 4) { for (int yy = 0; yy < 10; yy++) for (int xx = 0; xx < 10; xx++) suiPx(b, bx + xx, y0 + 3 + yy, (xx == 0 || yy == 0 || xx == 9 || yy == 9) ? kBlNavy : SUI_RGB(120, 190, 120)); T.draw(b, bx + 2, y0 + 2, "G", kBlNavy, SUI_RGB(120, 190, 120)); }
    const int nx = X0 + 32; T.draw(b, nx, ty - 3, kPxName[id], ink, sh);
    const int loc = P3DS_PhLoc[id]; if (loc >= 0 && loc < 100 && P3DS_PgName[loc][0]) { char pl[24]; const int cw = pxMapChipW(T), room = listR - 4 - cw - 4 - (nx + T.width(kPxName[id]) + 6);
      pxBtn(b, listR - 2 - cw, y0 + 3, listR - 3, y1 - 3, SUI_RGB(250, 252, 255), kBlSilverLo, kBlNavy); T.draw(b, listR - 2 - cw + 3, ty - 3, "MAP", ink, sh); /* MAP chip = jump to the place; the rest of the row calls */
      if (room > 18) { pxFit(T, pl, sizeof pl, P3DS_PgName[loc], room); T.draw(b, listR - 2 - cw - 4 - T.width(pl), ty - 3, pl, fg, sh); } } }
  if (ms > 0) { const int h = gPx.listY1 - gPx.listY0, bh = h * h / (h + ms) < 8 ? 8 : h * h / (h + ms), by = gPx.listY0 + (h - bh) * sc / ms;
    for (int y = gPx.listY0; y < gPx.listY1; y++) for (int x = X1 - 10; x < X1 - 3; x++) suiPx(b, x, y, SUI_RGB(70, 150, 120));
    for (int y = by; y < by + bh; y++) for (int x = X1 - 10; x < X1 - 3; x++) suiPx(b, x, y, (gPx.scrub || gPx.touching) ? SUI_RGB(24, 153, 115) : SUI_RGB(70, 150, 120)); } }

/* ---- the RADIO pane: the game's rotations (pokegear_radio.c RadioShow_TranslateStationID, pokemon_music.c) ---- */
static const char *pxMusicToday(int wday) {
  switch (wday) {
  case 0: return P3DS_RadGb ? "GB Sounds" : "Pokemon March";
  case 1: case 5: return "Pokemon March";
  case 2: case 6: return "Pokemon Lullaby";
  case 3: return P3DS_RadNat ? "Hoenn Sound" : "Pokemon March";
  case 4: return P3DS_RadNat ? "Sinnoh Sound" : "Pokemon Lullaby";
  } return "Pokemon March"; }
/* station index s of the game (0..7) -> name on the air at hour h, weekday w; `until` = hour it changes (-1 = all day) */
static void pxOnAir(int s, int h, int w, const char **what, const char **sub, int *until) {
  *until = -1; *sub = "";
  switch (s) {
  case 0: *what = "POKEMON MUSIC"; *sub = pxMusicToday(w); break;
  case 1: *what = "POKEMON TALK"; break;
  case 2: *what = (h % 2) ? "THAT TOWN, THESE PEOPLE" : "TRAINER PROFILES"; *until = (h + 1) % 24; break;
  case 3: { static const char *const k[3] = { "POKEMON SEARCH PARTY", "SERIAL RADIO DRAMA", "BUENA'S PASSWORD" }; *what = k[h % 3]; *until = (h + 1) % 24; break; }
  case 4: *what = "POKE FLUTE"; break;
  case 5: *what = "UNOWN RADIO"; break;
  case 6: *what = "ROCKET RADIO"; break;
  default: *what = "MAHOGANY SIGNAL"; break; } }
static void pxRadio(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const uint32_t ink = SUI_RGB(7, 33, 54), sh = SUI_RGB(244, 247, 254), fg = SUI_RGB(18, 26, 38);
  const uint32_t wh = SUI_RGB(255, 255, 255);
  pxBack(b, X0, Y0, X1, Y1, kPxSubH - 1); gPx.radN = 0; gPx.radX0 = X0 + 2; gPx.radX1 = X1 - 2;
  const time_t now = time(0); struct tm tmv; const struct tm *lt = localtime_r(&now, &tmv); const int hh = lt ? lt->tm_hour : 0, mi = lt ? lt->tm_min : 0, wd = lt ? lt->tm_wday : 0;
  static const char *const kDay[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" }; char clk[24]; snprintf(clk, sizeof clk, "%s %02d:%02d", kDay[wd % 7], hh, mi);
  const int sel = P3DS_RadSel;
  static const char *const kArea[7] = { "JOHTO RADIO", "KANTO RADIO", "KANTO + EXPN CARD", "NO SIGNAL HERE", "RUINS OF ALPH", "ROCKET TAKEOVER", "MAHOGANY SIGNAL" };
  T.draw(b, X0 + 4, Y0 + 2, kArea[sel < 0 || sel > 6 ? 0 : sel], wh, kBlNavy); T.draw(b, X1 - 4 - T.width(clk), Y0 + 2, clk, SUI_RGB(200, 236, 222), kBlNavy);
  if (!P3DS_PhReady) { pxNote(b, T, X0, Y0 + kPxSubH + 4, X1, 10, "Radio info loads after", "your first step in the field."); return; }
  if (!P3DS_RadCard) { pxNote(b, T, X0, Y0 + kPxSubH + 4, X1, 10, "No Radio Card yet.", "The Pokegear radio unlocks", "when you get it."); return; }
  int st[5], n = 0;
  switch (sel) { case 0: st[n++] = 0; st[n++] = 1; st[n++] = 2; st[n++] = 3; break; case 1: st[n++] = 0; st[n++] = 1; st[n++] = 2; break;
  case 2: st[n++] = 0; st[n++] = 1; st[n++] = 2; st[n++] = 3; st[n++] = 4; break; case 4: st[n++] = 5; break; case 5: st[n++] = 6; break; case 6: st[n++] = 7; break; default: break; }
  if (!n) { pxNote(b, T, X0, Y0 + kPxSubH + 4, X1, 10, "No radio signal here.", "The Pokegear cannot pick up", "a station in this place."); return; }
  for (int i = 0; i < n; i++) { const int y0 = Y0 + kPxSubH + 2 + i * 28, y1 = y0 + 26; if (y1 > Y1) break; const char *what, *sub; int until; pxOnAir(st[i], hh, wd, &what, &sub, &until);
    gPx.radSt[i] = st[i]; gPx.radY0[i] = y0; gPx.radY1[i] = y1; gPx.radN = i + 1;
    pxBtn(b, X0 + 2, y0, X1 - 2, y1, SUI_RGB(252, 253, 255), SUI_RGB(220, 238, 232), kBlNavy);
    for (int y = y0 + 1; y < y1 - 1; y++) for (int x = X0 + 3; x < X0 + 5; x++) suiPx(b, x, y, kBlBlue); /* accent stripe */
    /* the "on air" lamp */
    for (int yy = 0; yy < 7; yy++) for (int xx = 0; xx < 7; xx++) { const int d = (xx - 3) * (xx - 3) + (yy - 3) * (yy - 3); if (d <= 10) suiPx(b, X0 + 7 + xx, y0 + 4 + yy, d > 6 ? SUI_RGB(30, 120, 60) : SUI_RGB(90, 220, 120)); }
    char nm[40]; pxFit(T, nm, sizeof nm, what, X1 - X0 - 24 - 6); T.draw(b, X0 + 18, y0 + 1, nm, ink, sh);
    char ln[64]; if (sub[0]) snprintf(ln, sizeof ln, "Today: %s", sub); else if (until >= 0) snprintf(ln, sizeof ln, "Changes at %02d:00", until); else snprintf(ln, sizeof ln, "On the air all day");
    suiSDraw(T, b, X0 + 18, y0 + 12, ln, fg, sh); } }

/* ---- the PARTY and BAG panes (hook 0167 data; run 118: tap opens the real Pokemon / Bag Start-menu apps) ---- */
/* the game's item name with its accent restored ("Pok Ball" -> "Poke Ball"; the text pipe drops the e-acute) */
static void pxItemName(int id, char *out, int sz) {
  const char *n = id > 0 && id < 540 && P3DS_VwItem[id][0] ? P3DS_VwItem[id] : "?"; int o = 0;
  for (int i = 0; n[i] && o < sz - 2; i++) { out[o++] = n[i]; if (i >= 2 && n[i - 2] == 'P' && n[i - 1] == 'o' && n[i] == 'k' && n[i + 1] == ' ' && (i == 2 || n[i - 3] == ' ')) out[o++] = 'e'; }
  out[o] = 0; }
static const char *const kVwPocket[8] = { "ITEMS", "MEDICINE", "POKE BALLS", "TMs & HMs", "BERRIES", "MAIL", "BATTLE ITEMS", "KEY ITEMS" };
static void pxVwWait(uint32_t *b, const SUiText &T, int X0, int Y, int X1, int icon) {
  if (gPx.vwWait < 90) { pxNote(b, T, X0, Y, X1, icon, "Reading your save..."); return; }
  pxNote(b, T, X0, Y, X1, -1, "No data from the game yet.", "Hook 0167 is missing from", "this build, or the field", "is not running."); }
/* ---- run 145 panel_live: PARTY / BAG actions with the world running (hook 0207 does them on the game thread) ---- */
static void pxSetPane(int p, const char *why);
static void pxLivePost(int kind, int a, int b) {
  P3DS_LvA = a; P3DS_LvB = b; P3DS_LvReq = kind; gPx.lvWait = 1; gPx.lvKind = kind; gPx.lvB = b; BLINV;
  PSPNativeMemLog("[LIVE] request %s a %d b %d", kind == 1 ? "swap" : "use item", a, b); }
static void pxLiveClear() { gPx.lvSel = 0; gPx.lvItem = 0; gPx.lvSheet = 0; BLINV; }
static void pxLiveResult(int r) {
  char it[24]; pxItemName(gPx.lvItem, it, sizeof it);
  PSPNativeMemLog("[LIVE] result %d for %s (item %d '%s', slot %d)", r, gPx.lvKind == 1 ? "swap" : "use item", gPx.lvItem, it, gPx.lvB);
  if (r == 1) { pxLiveClear(); return; }
  if (r == 2) { char t[40]; snprintf(t, sizeof t, "USED %s", it); for (char *c = t; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32; P3DS_Toast(t); pxLiveClear(); pxSetPane(4, "live: item used"); return; }
  if (r == -2) { P3DS_Toast("NEW LEAD: OPENING MENU"); gPx.lvOpen = gPx.lvKind == 1 ? 1 : 2; gPx.lvOpenSlot = gPx.lvB; pxLiveClear(); return; }
  if (r == -3) { P3DS_Toast("IT WON'T HAVE ANY EFFECT"); return; } /* keep the medicine picked: tap another Pokemon */
  if (r == -4) { gPx.lvOpen = 2; gPx.lvOpenSlot = gPx.lvB; pxLiveClear(); return; }
  if (r == -5) { P3DS_Toast("NONE LEFT"); pxLiveClear(); pxSetPane(4, "live: none left"); return; }
  P3DS_Toast("CAN'T DO THAT"); pxLiveClear(); }
/* every frame (from pxFrame) */
static void pxLiveFrame() {
  if (gPx.pane != 3 && !gPx.lvWait && (gPx.lvSel || gPx.lvItem)) pxLiveClear();
  if (gPx.lvSheet && gPx.pane != (gPx.lvSheet == 1 ? 3 : 4)) { gPx.lvSheet = 0; BLINV; }
  if (P3DS_LvDone != gPx.lvSeen) { gPx.lvSeen = P3DS_LvDone; if (gPx.lvWait) { gPx.lvWait = 0; pxLiveResult(P3DS_LvRes); } }
  else if (gPx.lvWait && ++gPx.lvWait > 150) { P3DS_LvReq = 0; gPx.lvWait = 0; pxLiveClear(); P3DS_Toast("NOT RIGHT NOW");
    PSPNativeMemLog("[LIVE] no answer from the game in 150 frames (hook 0207 missing, or a script/menu kept the player busy)"); } }
/* PARTY title strip in live mode: title + MENU / CANCEL chip; false = not live (draw the normal strip) */
static bool pxLiveHeader(uint32_t *b, const SUiText &T, int X0, int Y0, int X1) {
  extern volatile int P3DS_PanelPartyWant, P3DS_PanelPartyOn;
  if (!panelLiveCfg() || P3DS_PanelPartyWant || P3DS_PanelPartyOn) return false;
  const uint32_t ink = SUI_RGB(7, 33, 54), sh = SUI_RGB(244, 247, 254), wh = SUI_RGB(255, 255, 255);
  const char *c = (gPx.lvSel || gPx.lvItem) ? "CANCEL" : "MENU"; const int w = T.width(c) + 8, x1 = X1 - 3, x0 = x1 - w; gPx.lvMenuX0 = x0;
  pxBtn(b, x0, Y0 + 1, x1, Y0 + kPxSubH - 2, SUI_RGB(250, 252, 255), kBlSilverLo, kBlNavy); T.draw(b, x0 + 4, Y0 + 1, c, ink, sh);
  char t[40], fit[40];
  if (gPx.lvWait) snprintf(t, sizeof t, "...");
  else if (gPx.lvItem) { char it[24]; pxItemName(gPx.lvItem, it, sizeof it); snprintf(t, sizeof t, "USE %s ON:", it); }
  else if (gPx.lvSel) snprintf(t, sizeof t, "SWAP WITH:");
  else snprintf(t, sizeof t, "PARTY");
  pxFit(T, fit, sizeof fit, t, x0 - X0 - 8); T.draw(b, X0 + 4, Y0 + 2, fit, wh, kBlNavy); return true; }
static int pxPartyAt(int y);
/* tap on the PARTY pane in live mode */
static void pxLiveTapParty(int x, int y) {
  if (gPx.lvWait) { PSPNativeMemLog("[LIVE] tap ignored: waiting for the game"); return; }
  if (y >= gPx.listY0 - kPxSubH && y < gPx.listY0) {
    if (x < gPx.lvMenuX0 - 2) return;
    if (gPx.lvItem) { pxLiveClear(); pxSetPane(4, "live: cancel item"); PSPNativeMemLog("[LIVE] item cancelled"); }
    else if (gPx.lvSel) { pxLiveClear(); PSPNativeMemLog("[LIVE] swap cancelled"); }
    else { gPx.lvOpen = 1; gPx.lvOpenSlot = -1; PSPNativeMemLog("[LIVE] MENU chip: open the real Pokemon menu"); }
    return; }
  const int s = pxPartyAt(y); if (s < 0) return;
  if (gPx.lvItem) { pxLivePost(2, gPx.lvItem, s); return; }
  if (!gPx.lvSel) { gPx.lvSheet = 1; gPx.lvSheetSlot = s; BLINV; PSPNativeMemLog("[LIVE] party slot %d: action card", s); return; }
  if (gPx.lvSel == s + 1) { pxLiveClear(); PSPNativeMemLog("[LIVE] swap cancelled (same slot)"); return; }
  pxLivePost(1, gPx.lvSel - 1, s); }
static int pxBagRowAt(int x, int y);
/* tap on the BAG list in live mode: a MEDICINE row picks it and shows the party; true = handled (other pockets open the real bag as before) */
static bool pxLiveTapBag(int x, int y) {
  if (gPx.lvWait) return false;
  const int r = pxBagRowAt(x, y); if (r < 0) return false;
  const int slot = gPx.bagList[r], id = P3DS_VwBagId[gPx.bag][slot]; if (!id) return false;
  gPx.lvSheet = 2; gPx.lvSheetSlot = slot; BLINV; PSPNativeMemLog("[LIVE] bag pocket %d slot %d item %d: action card", gPx.bag, slot, id); return true; }

/* run 147: styled action card over the live PARTY / BAG pane (same navy/silver look as the rest of the panel) */
static void pxLiveSheet(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  gPx.lvBtnN = 0; if (!gPx.lvSheet || !panelLiveCfg()) return;
  const uint32_t ink = SUI_RGB(7, 33, 54), sh = SUI_RGB(244, 247, 254), wh = SUI_RGB(255, 255, 255);
  char title[40], sub[40] = "", l0[16], l1[16] = "";
  if (gPx.lvSheet == 1) { const uint16_t *m = P3DS_VwParty[gPx.lvSheetSlot]; snprintf(title, sizeof title, "%s", m[6] ? "EGG" : m[0] > 0 && m[0] < 494 ? kP3dsSpecies[m[0]] : "?");
    if (!m[6]) snprintf(sub, sizeof sub, "Lv%d  HP %d/%d", (int)m[1], (int)m[2], (int)m[3]); snprintf(l0, sizeof l0, "SWITCH"); snprintf(l1, sizeof l1, "OPEN MENU"); }
  else { const int id = P3DS_VwBagId[gPx.bag][gPx.lvSheetSlot]; pxItemName(id, title, sizeof title); snprintf(sub, sizeof sub, "x%d", P3DS_VwBagQty[gPx.bag][gPx.lvSheetSlot]);
    snprintf(l0, sizeof l0, gPx.bag == 1 ? "USE ON..." : "OPEN BAG"); }
  const int nb = gPx.lvSheet == 1 ? 3 : 2, w = 130, h = 20 + nb * 19, x0 = (X0 + X1 - w) / 2, y0 = Y0 + kPxSubH + 10;
  for (int y = Y0 + kPxSubH; y < Y1; y++) for (int x = X0; x < X1; x++) if (((x + y) & 1) == 0) suiPx(b, x, y, SUI_RGB(0, 60, 46)); /* dim the list behind */
  pxBtn(b, x0, y0, x0 + w, y0 + h, SUI_RGB(252, 253, 255), SUI_RGB(220, 238, 232), kBlNavy);
  for (int y = y0 + 1; y < y0 + 13; y++) for (int x = x0 + 1; x < x0 + w; x++) suiPx(b, x, y, kBlNavy);
  char fit[40]; pxFit(T, fit, sizeof fit, title, w - 8); T.draw(b, x0 + 4, y0 + 1, fit, wh, kBlNavy);
  if (sub[0]) T.draw(b, x0 + w - 4 - T.width(sub), y0 + 1, sub, SUI_RGB(200, 236, 222), kBlNavy);
  const char *lab[3] = { l0, gPx.lvSheet == 1 ? l1 : "CANCEL", "CANCEL" };
  for (int i = 0; i < nb; i++) { const int by0 = y0 + 16 + i * 19, by1 = by0 + 17, bx0 = x0 + 6, bx1 = x0 + w - 6;
    gPx.lvBtn[i][0] = bx0; gPx.lvBtn[i][1] = by0; gPx.lvBtn[i][2] = bx1; gPx.lvBtn[i][3] = by1;
    pxBtn(b, bx0, by0, bx1, by1, SUI_RGB(250, 252, 255), kBlSilverLo, kBlNavy); T.draw(b, (bx0 + bx1 - T.width(lab[i])) / 2, by0 + 3, lab[i], ink, sh); }
  gPx.lvBtnN = nb; }
/* tap on the card: true = consumed */
static bool pxLiveSheetTap(int x, int y) {
  if (!gPx.lvSheet) return false;
  int hit = -1; for (int i = 0; i < gPx.lvBtnN; i++) if (x >= gPx.lvBtn[i][0] && x < gPx.lvBtn[i][2] && y >= gPx.lvBtn[i][1] && y < gPx.lvBtn[i][3]) hit = i;
  const int kind = gPx.lvSheet, slot = gPx.lvSheetSlot; gPx.lvSheet = 0; BLINV;
  if (hit < 0) { PSPNativeMemLog("[LIVE] card closed (tap outside)"); return true; }
  if (kind == 1) {
    if (hit == 0) { gPx.lvSel = slot + 1; PSPNativeMemLog("[LIVE] card: SWITCH slot %d, pick the other Pokemon", slot); }
    else if (hit == 1) { gPx.lvOpen = 1; gPx.lvOpenSlot = slot; PSPNativeMemLog("[LIVE] card: open the real Pokemon menu at slot %d", slot); }
    else PSPNativeMemLog("[LIVE] card: cancel"); }
  else {
    if (hit == 0) { const int id = P3DS_VwBagId[gPx.bag][slot];
      if (gPx.bag == 1) { gPx.lvSel = 0; gPx.lvItem = id; gPx.lvPocket = 1; gPx.lvSlot = slot; PSPNativeMemLog("[LIVE] card: USE item %d, pick a Pokemon", id); pxSetPane(3, "live: pick a Pokemon"); }
      else { gPx.lvOpen = 2; gPx.lvPocket = gPx.bag; gPx.lvSlot = slot; gPx.lvOpenSlot = -1; PSPNativeMemLog("[LIVE] card: open the real bag at pocket %d slot %d", gPx.bag, slot); } }
    else PSPNativeMemLog("[LIVE] card: cancel"); }
  return true; }

static void pxParty(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const uint32_t ink = SUI_RGB(7, 33, 54), sh = SUI_RGB(244, 247, 254), fg = SUI_RGB(18, 26, 38), wh = SUI_RGB(255, 255, 255);
  pxBack(b, X0, Y0, X1, Y1, kPxSubH - 1);
  gPx.listY0 = Y0 + kPxSubH; gPx.listY1 = Y1; gPx.listX1 = X1;
  if (P3DS_PanelPartyWant || P3DS_PanelPartyOn || (panelPartyCfg() && !panelLiveCfg())) {
    // The live game picture will replace this pane once the app is ready.
    gPx.listY0 = gPx.listY1;
    T.draw(b, X0 + 4, Y0 + 2, "PARTY", wh, kBlNavy);
    return;
  }
  if (!pxLiveHeader(b, T, X0, Y0, X1)) {
    T.draw(b, X0 + 4, Y0 + 2, "PARTY", wh, kBlNavy);
    const char *hint = "TAP TO OPEN";
    T.draw(b, X1 - 4 - T.width(hint), Y0 + 2, hint, SUI_RGB(200, 236, 222), kBlNavy);
  }
  if (!P3DS_VwReady) { pxVwWait(b, T, X0, gPx.listY0 + 4, X1, gPx.pane == 4 ? 2 : 1); return; }
  if (!P3DS_VwPartyN) { pxNote(b, T, X0, Y0 + kPxSubH + 4, X1, 1, "No Pokemon in the party."); return; }
  for (int i = 0; i < P3DS_VwPartyN && i < 6; i++) { const uint16_t *m = P3DS_VwParty[i]; const int y0 = Y0 + kPxSubH + 1 + i * 25, y1 = y0 + 24; if (y1 > Y1) break;
    const bool lvS = gPx.lvSel == i + 1; /* run 145: the Pokemon picked to swap */
    pxBtn(b, X0 + 2, y0, X1 - 2, y1, lvS ? SUI_RGB(255, 244, 196) : SUI_RGB(252, 253, 255), lvS ? SUI_RGB(240, 200, 96) : (i & 1) ? SUI_RGB(220, 238, 232) : SUI_RGB(236, 246, 242), kBlNavy);
    char nm[32]; if (m[6]) snprintf(nm, sizeof nm, "EGG"); else snprintf(nm, sizeof nm, "%s", m[0] > 0 && m[0] < 494 ? kP3dsSpecies[m[0]] : "?");
    T.draw(b, X0 + 5, y0 + 2, nm, ink, sh);
    if (m[6]) { T.draw(b, X0 + 5, y0 + 13, "It has not hatched yet.", fg, sh); continue; }
    char lv[12]; snprintf(lv, sizeof lv, "Lv%d", (int)m[1]); T.draw(b, X1 - 5 - T.width(lv), y0 + 2, lv, ink, sh);
    const int hp = m[2] > m[3] ? m[3] : m[2], mx = m[3] ? m[3] : 1; const int bw = 64, bx = X0 + 5, by = y0 + 15;
    for (int y = by; y < by + 5; y++) for (int x = bx; x < bx + bw; x++) suiPx(b, x, y, SUI_RGB(40, 56, 64));
    const int fw = hp * (bw - 2) / mx; const uint32_t col = hp == 0 ? SUI_RGB(120, 120, 130) : hp * 2 > mx ? SUI_RGB(64, 200, 96) : hp * 5 > mx ? SUI_RGB(240, 200, 48) : SUI_RGB(224, 64, 56);
    for (int y = by + 1; y < by + 4; y++) for (int x = bx + 1; x < bx + 1 + fw; x++) suiPx(b, x, y, col);
    char hs[20]; snprintf(hs, sizeof hs, "%d/%d", (int)m[2], (int)m[3]); T.draw(b, bx + bw + 4, y0 + 12, hs, fg, sh);
    const uint16_t st = m[5]; const char *tag = hp == 0 ? "FNT" : (st & 7) ? "SLP" : (st & 0x88) ? "PSN" : (st & 0x10) ? "BRN" : (st & 0x20) ? "FRZ" : (st & 0x40) ? "PAR" : nullptr;
    int rx = X1 - 5;
    if (tag) { const int w = suiSWidth(T, tag) + 6; pxBtn(b, rx - w, y0 + 13, rx, y0 + 22, hp == 0 ? SUI_RGB(200, 200, 208) : SUI_RGB(250, 224, 150), hp == 0 ? SUI_RGB(150, 150, 160) : SUI_RGB(226, 170, 60), kBlNavy); suiSDraw(T, b, rx - w + 3, y0 + 13, tag, kBlNavy, SUI_RGB(240, 230, 200)); rx -= w + 3; }
    if (m[4]) { char it[24], fit[24]; pxItemName(m[4], it, sizeof it); const int room = rx - (bx + bw + 4 + T.width(hs) + 6); pxFit(T, fit, sizeof fit, it, room); if (room > 20) T.draw(b, rx - T.width(fit), y0 + 12, fit, fg, sh); } } }
static void pxBagBuild() {
  gPx.bagN = 0; for (int i = 0; i < 165; i++) if (P3DS_VwBagId[gPx.bag][i]) gPx.bagList[gPx.bagN++] = i;
  const int ms = pxMaxScroll(); if (gPx.scroll > ms) gPx.scroll = (float)ms; }
static void pxBag(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const uint32_t ink = SUI_RGB(7, 33, 54), sh = SUI_RGB(244, 247, 254), fg = SUI_RGB(18, 26, 38), wh = SUI_RGB(255, 255, 255);
  pxBack(b, X0, Y0, X1, Y1, kPxSubH - 1);
  gPx.listY0 = Y0 + kPxSubH; gPx.listY1 = Y1; gPx.listX1 = X1;
  if (P3DS_PanelPartyWant || P3DS_PanelPartyOn || (panelPartyCfg() && !panelLiveCfg())) {
    gPx.listY0 = gPx.listY1;
    gPx.bagLX0 = gPx.bagLX1 = gPx.bagRX0 = gPx.bagRX1 = 0;
    T.draw(b, X0 + 4, Y0 + 2, "BAG", wh, kBlNavy);
    return;
  }
  pxBagBuild();
  /* < pocket name (count) >: tap the arrows to change pocket */
  gPx.bagLX0 = X0 + 2; gPx.bagLX1 = X0 + 16; gPx.bagRX1 = X1 - 3; gPx.bagRX0 = X1 - 17;
  pxBtn(b, gPx.bagLX0, Y0 + 1, gPx.bagLX1, Y0 + kPxSubH - 2, SUI_RGB(250, 252, 255), kBlSilverLo, kBlNavy); T.draw(b, gPx.bagLX0 + 4, Y0 + 1, "<", ink, sh);
  pxBtn(b, gPx.bagRX0, Y0 + 1, gPx.bagRX1, Y0 + kPxSubH - 2, SUI_RGB(250, 252, 255), kBlSilverLo, kBlNavy); T.draw(b, gPx.bagRX0 + 5, Y0 + 1, ">", ink, sh);
  { char t[40]; snprintf(t, sizeof t, "%s (%d)", kVwPocket[gPx.bag], gPx.bagN); T.draw(b, X0 + (X1 - X0 - T.width(t)) / 2, Y0 + 2, t, wh, kBlNavy); }
  if (!P3DS_VwReady) { pxVwWait(b, T, X0, gPx.listY0 + 4, X1, gPx.pane == 4 ? 2 : 1); return; }
  if (!gPx.bagN) { pxNote(b, T, X0, gPx.listY0 + 4, X1, 2, "Nothing in this pocket."); return; }
  const int ms = pxMaxScroll(); if (gPx.scroll > ms) { gPx.scroll = (float)ms; gPx.vel = 0; } if (gPx.scroll < 0) { gPx.scroll = 0; gPx.vel = 0; }
  const int sc = (int)gPx.scroll, nr = (gPx.listY1 - gPx.listY0) / kPxRowH + 2, r0 = sc / kPxRowH, listR = X1 - (ms > 0 ? 13 : 3);
  for (int r = r0; r < r0 + nr && r < gPx.bagN; r++) { const int slot = gPx.bagList[r]; const int y0 = gPx.listY0 + r * kPxRowH - sc, y1 = y0 + kPxRowH; if (y1 <= gPx.listY0 || y0 >= gPx.listY1) continue;
    const bool inside = y0 + 1 >= gPx.listY0 && y1 - 1 <= gPx.listY1;
    if (inside) pxBtn(b, X0 + 2, y0 + 1, listR, y1 - 1, SUI_RGB(252, 253, 255), (r & 1) ? SUI_RGB(220, 238, 232) : SUI_RGB(236, 246, 242), kBlNavy);
    else for (int y = y0; y < y1; y++) { if (y < gPx.listY0 || y >= gPx.listY1) continue; for (int x = X0 + 2; x < listR; x++) suiPx(b, x, y, SUI_RGB(236, 246, 242)); }
    if (!inside) continue;
    const int id = P3DS_VwBagId[gPx.bag][slot], q = P3DS_VwBagQty[gPx.bag][slot]; char it[24], fit[24], qs[12] = ""; pxItemName(id, it, sizeof it);
    if (gPx.bag != 7 && !(gPx.bag == 3 && q == 1)) snprintf(qs, sizeof qs, "x%d", q);
    const int qw = qs[0] ? T.width(qs) + 4 : 0; pxFit(T, fit, sizeof fit, it, listR - 4 - (X0 + 6) - qw);
    T.draw(b, X0 + 6, y0 + 3, fit, ink, sh); if (qw) T.draw(b, listR - 4 - T.width(qs), y0 + 3, qs, fg, sh); }
  if (ms > 0) { const int h = gPx.listY1 - gPx.listY0, bh = h * h / (h + ms) < 8 ? 8 : h * h / (h + ms), by = gPx.listY0 + (h - bh) * sc / ms;
    for (int y = gPx.listY0; y < gPx.listY1; y++) for (int x = X1 - 10; x < X1 - 3; x++) suiPx(b, x, y, SUI_RGB(70, 150, 120));
    for (int y = by; y < by + bh; y++) for (int x = X1 - 10; x < X1 - 3; x++) suiPx(b, x, y, (gPx.scrub || gPx.touching) ? SUI_RGB(24, 153, 115) : SUI_RGB(70, 150, 120)); } }

/* ---- the BREED pane (hook 0210 data; read-only) ---- */
static void pxBreed(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const uint32_t paper = SUI_RGB(244, 250, 246), mint = SUI_RGB(218, 238, 228);
  const uint32_t green = SUI_RGB(25, 112, 76), gold = SUI_RGB(255, 229, 155);
  pxBack(b, X0, Y0, X1, Y1, kPxSubH - 1);
  T.draw(b, X0 + 5, Y0 + 2, "BREEDING", kBlSilverHi, kBlNavy);

  const int left = X0 + 4, right = X1 - 4, top = Y0 + kPxSubH + 4;
  if (!P3DS_BrReady) {
    pxBtn(b, left, top, right, top + 33, paper, mint, kBlNavy);
    blIcon(b, left + 6, top + 9, 11, kBlIcInk, kBlIcAcc);
    suiSDraw(T, b, left + 24, top + 5, "Waiting for breeding data", kBlNavy, paper);
    suiSDraw(T, b, left + 24, top + 17, "Take a step in the field.", green, paper);
    return;
  }

  const int *d = P3DS_BrD;
  const int dayH = d[0] == 0 ? 31 : d[0] == 1 ? 43 : 66;
  pxBtn(b, left, top, right, top + dayH, paper, mint, kBlNavy);
  pxBtn(b, left, top, right, top + 15, kBlPress, kBlBlue, kBlNavy);
  blIcon(b, left + 4, top + 2, 11, kBlNavy, gold);
  suiSDraw(T, b, left + 21, top + 2, "DAY-CARE", kBlSilverHi, kBlNavy);

  char line[64];
  int y = top + 17;
  for (int i = 0; i < 2 && i < d[0]; i++) {
    if (d[1 + i] <= 0 || d[1 + i] > 493) continue;
    snprintf(line, sizeof line, "%s  Lv%d", kP3dsSpecies[d[1 + i]], d[3 + i]);
    suiSDraw(T, b, left + 6, y, line, kBlNavy, paper);
    snprintf(line, sizeof line, "+%d levels", d[5 + i]);
    suiSDraw(T, b, right - 6 - suiSWidth(T, line), y, line, green, paper);
    y += 11;
  }
  const char *status = "Empty";
  if (d[0] == 1) status = "Needs a partner";
  else if (d[0] >= 2 && d[7] <= 0) status = "These two cannot breed";
  else if (d[0] >= 2 && d[9]) status = d[32] >= 6 ? "Egg ready | Party full" : "Egg ready to pick up!";
  else if (d[0] >= 2) {
    snprintf(line, sizeof line, "%d%% egg chance | %d steps to roll", d[7], d[10] < 0 ? 0 : d[10]);
    status = line;
  }
  pxBtn(b, left + 4, y, right - 4, y + 12, d[9] ? gold : paper, d[9] ? gold : mint, kBlNavy);
  suiSDraw(T, b, left + 8, y + 1, status, kBlNavy, paper);
  if (d[0] >= 2 && d[7] > 0)
    suiSDraw(T, b, left + 6, y + 14, d[8] ? "Masuda bonus | 5x shiny odds" : "Masuda bonus inactive", green, paper);

  y = top + dayH + 4;
  const int eggs = d[13] > 6 ? 6 : d[13];
  pxBtn(b, left, y, right, y + 13, kBlPress, kBlBlue, kBlNavy);
  snprintf(line, sizeof line, "PARTY EGGS  %d", eggs);
  suiSDraw(T, b, left + 5, y + 1, line, kBlSilverHi, kBlNavy);
  if (eggs && d[12] >= 2) {
    const char *boost = "2x HATCH SPEED";
    suiSDraw(T, b, right - 5 - suiSWidth(T, boost), y + 1, boost, gold, kBlNavy);
  }
  y += 15;
  if (!eggs) {
    pxBtn(b, left, y, right, y + 20, paper, mint, kBlNavy);
    suiSDraw(T, b, left + 6, y + 4, "No eggs in your party", kBlNavy, paper);
  }
  const int tileW = (right - left - 4) / 2;
  for (int i = 0; i < eggs; i++) {
    const int x = left + (i % 2) * (tileW + 4), row = y + (i / 2) * 22;
    if (row + 21 > Y1) break;
    pxBtn(b, x, row, x + tileW, row + 21, paper, mint, kBlNavy);
    blIcon(b, x + 4, row + 5, 11, kBlIcInk, d[20 + i] <= 0 ? gold : kBlIcAcc);
    if (d[20 + i] <= 0) snprintf(line, sizeof line, "Slot %d", d[14 + i] + 1);
    else snprintf(line, sizeof line, "#%d  %d cycles", d[14 + i] + 1, d[20 + i]);
    suiSDraw(T, b, x + 19, row + 1, line, kBlNavy, paper);
    snprintf(line, sizeof line, "~%d steps", d[26 + i]);
    suiSDraw(T, b, x + 19, row + 11, d[20 + i] <= 0 ? "Hatches soon" : line, green, paper);
  }
}

/* the pane under the tab bar; false = draw the map */
static bool pxPaneDraw(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const int p = pxPane(); if (!p) return false;
  if (pxNativeGear(p)) {
    pxBack(b, X0, Y0, X1, Y1, kPxSubH - 1);
    T.draw(b, X0 + 4, Y0 + 2, kPxTabs[p], kBlSilverHi, kBlNavy);
    if (!P3DS_PanelPartyOn)
      T.draw(b, X0 + 4, Y0 + kPxSubH + 6, "Opening Pokegear...", kBlNavy, kBlSilverHi);
    return true;
  }
  if (p == 1) { pxPhone(b, T, X0, Y0, X1, Y1); if (!gPx.loggedPhone) { gPx.loggedPhone = true; PSPNativeMemLog("[PHONE] phone pane open: %d contacts%s", gPx.n, P3DS_PhReady ? "" : " (no data from the game yet: hook 0122 not in this build, or no field step yet)"); } }
  else if (p == 3 || p == 4) { if (p == 3) pxParty(b, T, X0, Y0, X1, Y1); else pxBag(b, T, X0, Y0, X1, Y1); pxLiveSheet(b, T, X0, Y0, X1, Y1);
    if (!gPx.vwLogged) { gPx.vwLogged = true; PSPNativeMemLog("[VIEW] %s pane open: %s", p == 3 ? "party" : "bag", P3DS_VwReady ? "data already here" : "waiting for hook 0167 data"); } }
  else if (p == 5) { pxBreed(b, T, X0, Y0, X1, Y1);
    if (!gPx.brLogged) { gPx.brLogged = true; PSPNativeMemLog("[BREED] pane open: %s", P3DS_BrReady ? "data already here" : "waiting for hook 0210 data"); } }
  else { pxRadio(b, T, X0, Y0, X1, Y1); if (!gPx.loggedRadio) { gPx.loggedRadio = true; PSPNativeMemLog("[RADIO] radio pane open: area %d, national dex %d, GB Sounds %d%s", (int)P3DS_RadSel, (int)P3DS_RadNat, (int)P3DS_RadGb, P3DS_PhReady ? "" : " (no data from the game yet: hook 0122 not in this build, or no field step yet)"); } }
  return true; }

/* ---- touch ---- */
static void pxSetPane(int p, const char *why) { if (p < 0 || p >= pxNTab() || p == gPx.pane) return; gPx.pane = p; gPx.vel = 0; gPx.scroll = 0; gPx.touching = gPx.scrub = false; gPx.vwWait = 0; BLINV; PSPNativeMemLog("[PHONE] panel -> %s (%s)", kPxTabs[p], why); }
/* every frame: fling after the finger lifts, and the radio's clock */
static void pxFrame(bool touching) {
  if (!pxOn()) return;
  { static int vwSelInit; if (!vwSelInit) { vwSelInit = 1; P3DS_VwPartySlot = 0; P3DS_VwBagPocket = -1; P3DS_VwBagSlot = -1; } }
  gPx.touching = touching;
  if (pxScrolls() && !touching && (gPx.vel > 0.5f || gPx.vel < -0.5f)) { gPx.scroll += gPx.vel; gPx.vel *= 0.9f; const int ms = pxMaxScroll(); if (gPx.scroll < 0 || gPx.scroll > ms) { gPx.scroll = gPx.scroll < 0 ? 0.f : (float)ms; gPx.vel = 0; } BLINV; }
  else if (!touching) gPx.vel = 0;
  { const bool vw = gPx.pane == 3 || gPx.pane == 4; P3DS_VwOn = vw ? 1 : 0;
    if (vw) { if (!P3DS_VwReady) gPx.vwWait++; if (P3DS_VwTick != gPx.vwSeen) { gPx.vwSeen = P3DS_VwTick; BLINV; if (gPx.vwWait) { PSPNativeMemLog("[VIEW] data from the game after %d frames: %d party Pokemon", gPx.vwWait, P3DS_VwPartyN); gPx.vwWait = 0; } } } }
  if (gPx.pane >= pxNTab()) { gPx.pane = 0; BLINV; } /* breed_tab turned off while BREED was open */
  { const bool br = gPx.pane == 5; P3DS_BrOn = br ? 1 : 0;
    if (br) {
      if (P3DS_BrTick != gPx.brSeen) { gPx.brSeen = P3DS_BrTick; BLINV; const int *d = P3DS_BrD; const int now[8] = { d[0], d[1], d[2], d[5], d[6], d[9], d[13], d[12] };
        if (memcmp(now, gPx.brLast, sizeof now)) { memcpy(gPx.brLast, now, sizeof now); PSPNativeMemLog("[BREED] daycare %d (%d Lv%d +%d, %d Lv%d +%d), egg waiting %d, chance %d%%, masuda %d, next roll %d steps; party eggs %d%s", d[0], d[1], d[3], d[5], d[2], d[4], d[6], d[9], d[7], d[8], d[10], d[13], d[13] ? "" : " (none)");
          for (int e = 0; e < d[13] && e < 6; e++) PSPNativeMemLog("[BREED] egg slot %d: %d cycles, about %d steps (x%d)", d[14 + e] + 1, d[20 + e], d[26 + e], d[12]); } } } }
  pxLiveFrame();
  if (gPx.pane == 2) { const time_t now = time(0); const int m = (int)(now / 60); if (m != gPx.minute) { gPx.minute = m; BLINV; } } }
/* finger down / held on the panel (x,y in DS pixels); first = this frame the touch started */
static int pxRowAt(int x, int y);
/* run 118: scroll=false until the finger has moved past the tap threshold (render3ds tDrag), so jitter does not steal taps */
static void pxDrag(int x, int y, bool first, bool scroll) {
  if (!(gPx.pane == 1 || gPx.pane == 4)) return;
  if (first) { gPx.lastY = y; gPx.vel = 0; gPx.moved = false; gPx.pressId = pxRowAt(x, y); BLINV; gPx.scrub = pxMaxScroll() > 0 && x >= gPx.listX1 - 14 && y >= gPx.listY0; }
  if (!scroll && !gPx.scrub) return; /* 2026-11-18: lastY stays at the press point until the drag gate opens, so the list follows the whole stroke (it used to drop the first 10-16 px) */
  if (gPx.scrub) { const int h = gPx.listY1 - gPx.listY0, ms = pxMaxScroll(); float s = (float)(y - gPx.listY0) * ms / (h > 0 ? h : 1); if (s < 0) s = 0; if (s > ms) s = (float)ms; if (s != gPx.scroll) { gPx.scroll = s; BLINV; } return; }
  const int dy = y - gPx.lastY; if (dy) { gPx.moved = true; gPx.scroll -= dy; gPx.vel = -(float)dy; const int ms = pxMaxScroll(); if (gPx.scroll < 0) gPx.scroll = 0; if (gPx.scroll > ms) gPx.scroll = (float)ms; gPx.lastY = y; BLINV; } }
/* tap on the tab bar */
static void pxChipTap(const SUiText &T, int X1, int Y0, int x) { const int i = pxChipHit(T, X1, Y0, x); if (i >= 0) pxSetPane(i, "tab tapped"); }
/* game station id (0..7) of the RADIO row under (x,y), -1 none */
static int pxRadioAt(int x, int y) { if (gPx.pane != 2) return -1; for (int i = 0; i < gPx.radN; i++) if (x >= gPx.radX0 && x < gPx.radX1 && y >= gPx.radY0[i] && y < gPx.radY1[i]) return gPx.radSt[i]; return -1; }
/* true if x is on the MAP chip at the right of contact id's row (that part jumps to the map, the rest of the row calls) */
static bool pxTapIsPlace(const SUiText &T, int id, int x) {
  const int loc = P3DS_PhLoc[id]; if (loc < 0 || loc >= 100 || !P3DS_PgName[loc][0]) return false;
  const int listR = gPx.listX1 - (pxMaxScroll() > 0 ? 13 : 3); return x >= listR - 2 - pxMapChipW(T) - 3; }
/* contact id under list y (-1 none), and the SORT chip */
static int pxRowAt(int x, int y) { if (gPx.pane != 1 || y < gPx.listY0 || y >= gPx.listY1) return -1; if (pxMaxScroll() > 0 && x >= gPx.listX1 - 14) return -1;
  const int r = (y - gPx.listY0 + (int)gPx.scroll) / kPxRowH; return r >= 0 && r < gPx.n ? gPx.ord[r] : -1; }
/* BAG pane: the < and > chips in the title strip change the pocket */
static bool pxBagTap(int x, int y) {
  if (gPx.pane != 4 || y < gPx.listY0 - kPxSubH || y >= gPx.listY0) return false;
  int d = 0; if (x >= gPx.bagLX0 - 2 && x < gPx.bagLX1 + 2) d = -1; else if (x >= gPx.bagRX0 - 2 && x < gPx.bagRX1 + 2) d = 1; if (!d) return false;
  gPx.bag = (gPx.bag + d + 8) % 8; gPx.scroll = 0; gPx.vel = 0; BLINV; PSPNativeMemLog("[VIEW] bag pocket -> %s", kVwPocket[gPx.bag]); return true; }
/* run 118: true if (x,y) is on the PARTY/BAG list body (open the real menu), not the title strip */
static bool pxViewerListHit(int y) {
  if (gPx.pane == 3) return y >= gPx.listY0 - kPxSubH && y < gPx.listY1; /* party: whole pane incl. title */
  if (gPx.pane == 4) return y >= gPx.listY0 && y < gPx.listY1; /* bag: list rows only; title arrows = pxBagTap */
  return false; }
/* run 119: party slot under y (-1 none); bag list-row under (x,y) (-1 none) */
static int pxPartyAt(int y) {
  if (gPx.pane != 3 || !P3DS_VwReady || y < gPx.listY0 || y >= gPx.listY1) return -1;
  const int i = (y - gPx.listY0 - 1) / 25;
  return (i >= 0 && i < P3DS_VwPartyN && i < 6) ? i : -1; }
static int pxBagRowAt(int x, int y) {
  if (gPx.pane != 4 || y < gPx.listY0 || y >= gPx.listY1) return -1;
  if (pxMaxScroll() > 0 && x >= gPx.listX1 - 14) return -1;
  const int r = (y - gPx.listY0 + (int)gPx.scroll) / kPxRowH;
  return (r >= 0 && r < gPx.bagN) ? r : -1; }

static bool pxSortTap(int x, int y) {
  if (gPx.pane != 1 || y < gPx.listY0 - kPxSubH || y >= gPx.listY0 || x < gPx.sortX0 - 2 || x >= gPx.sortX1 + 2) return false;
  gPx.sort = (gPx.sort + 1) % 3; gPx.built = false; gPx.scroll = 0; gPx.vel = 0; BLINV; static const char *const k[3] = { "A-Z", "PLACE", "REMATCH first" };
  PSPNativeMemLog("[PHONE] sort -> %s", k[gPx.sort]); return true; }

/* ---- incoming call banner (hook 0125 publishes whether the Pokegear is ringing). Tapping it = the POKEGEAR button: the game's
   Pokegear opens on the phone and answers (FieldSystem_InitPokegearArgs_Phone). Drawn over any pane, bottom of the panel. ---- */
enum { kPxRingX0 = kBlPanX0 + 2, kPxRingX1 = kBlPanX1 - 2, kPxRingY0 = kBlPanY1 - 21, kPxRingY1 = kBlPanY1 - 2 };
/* run 118: GearPhoneRingManager_IsRinging can flap between ring beeps; hold the banner ~20 quiet frames so it does not flicker off */
static int pxRingHold, pxRingVal;
static int pxRingShown() { return pxOn() ? pxRingVal : 0; }
static inline bool pxRinging() { return pxRingShown() > 0; }
static bool pxRingHit(int x, int y) { return pxRinging() && x >= kPxRingX0 && x < kPxRingX1 && y >= kPxRingY0 && y < kPxRingY1; }
static void pxRingDraw(uint32_t *b, const SUiText &T, bool blink) {
  const int ring = pxRingShown(); if (!ring) return;
  const uint32_t paper = SUI_RGB(255, 251, 231), gold = SUI_RGB(248, 217, 139);
  const uint32_t lamp = blink ? SUI_RGB(255, 213, 74) : SUI_RGB(218, 151, 44);
  pxBtn(b, kPxRingX0, kPxRingY0, kPxRingX1, kPxRingY1, paper, gold, kBlNavy);
  suiFill(b, kPxRingX0 + 1, kPxRingY0 + 1, kPxRingX0 + 4, kPxRingY1 - 1, lamp);
  blIcon(b, kPxRingX0 + 8, kPxRingY0 + 4, 3, kBlNavy, lamp);

  const int id = ring - 2;
  const char *caller = id >= 0 && id < 75 ? kPxName[id] : "Unknown caller";
  const int nameX = kPxRingX0 + 24, answerR = kPxRingX1 - 3;
  const int answerX = answerR - T.width("ANSWER") - 12;
  char fit[40];
  pxFit(T, fit, sizeof fit, caller, answerX - nameX - 4);
  T.draw(b, nameX, kPxRingY0 + 4, fit, kBlNavy, paper);
  pxBtn(b, answerX, kPxRingY0 + 2, answerR, kPxRingY1 - 2,
        SUI_RGB(33, 137, 102), SUI_RGB(17, 93, 74), kBlNavy);
  T.draw(b, answerX + 6, kPxRingY0 + 4, "ANSWER", kBlSilverHi, kBlNavy);
}

/* every frame: redraw and log when the (sticky) ringing display changes */
static void pxRingFrame() {
  static int last;
  const int active = P3DS_PhRing;
  if (active > 0) {
    pxRingVal = active;
    pxRingHold = 20;
  } else if (pxRingHold > 0 && --pxRingHold == 0) {
    pxRingVal = 0;
  }
  const int r = pxRingShown();
  if (r != last) { last = r; BLINV; if (r > 0) PSPNativeMemLog("[PHONE] Pokegear ringing: caller %d '%s' (tap ANSWER or POKEGEAR)", r - 2, r >= 2 && r - 2 < 75 ? kPxName[r - 2] : "?"); else PSPNativeMemLog("[PHONE] ringing stopped"); } }

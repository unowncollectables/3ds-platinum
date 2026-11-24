/* ss3ds run 10: the Pokegear's Johto/Kanto map in the JOHTO bottom layout (bottom_layout=1).
   The art is the user's own ROM data, read on the game thread by pspoke hook 0081 (pgmap_gra NARC members 10 NCGR 8bpp
   tiles, 11 NSCR 64x64 tile sheet whose top-left 47x20 tiles are the whole map, 20 NCLR palette) and decoded here; nothing
   from the ROM is ever written to the SD card or committed. Locations are the game's own sLocationSpecs (x/y/size in map
   tiles). Visited locations are kept in sdmc:/3ds/platinum3ds/visited.bin ("VB01", trainer ID, 128-bit set), never in
   the save; a different trainer ID (new game) starts empty. Pure pixel/state code except the file I/O. */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss3ds_settings_ui.h"
#include "ss3ds_sharptext.h"
#include "p3ds_speciesnames.h"
#include "tracker.h"

extern "C" {
struct SS3DS_PgSpec { uint16_t mapId; uint8_t x, y; uint16_t bits; uint8_t flavorText, block, u8_, u9, uA, uB, srcX, srcY, dw, dh; };
extern const SS3DS_PgSpec sLocationSpecs[100];
/* 0259: 0 clear, 1 rain, 2 snow, 3 mist. The game thread publishes a complete forecast before bumping the tick. */
volatile int P3DS_PgWeatherOn;
volatile uint32_t P3DS_PgWeatherTick;
int P3DS_PgWeatherSeason = -1;
uint8_t P3DS_PgWeather[100];
int P3DS_PgWeatherMapId(int loc) { return sLocationSpecs[loc].mapId; }
int P3DS_AmbientWeatherCfg(void);
extern volatile int P3DS_FlyReq, P3DS_FlyRes, P3DS_FlyN, P3DS_FlyHere; extern volatile unsigned short P3DS_FlyList[32]; /* hook 0228, ss3ds_glue.c */
volatile int P3DS_PgReady, P3DS_PgLoc = -1, P3DS_PgShoes = -1; volatile uint32_t P3DS_PgTid;
char P3DS_PgName[100][20]; char P3DS_PgText[100][100];
/* buffers the game thread fills (hook 0081). Keys: pgmap_gra member (0..63), 100 + a/0/1/4 member, 200–203 = registered
   item icon chars/palette (fixed 16 KB slots, refilled in place when the registered item changes). */
static uint8_t *pgRaw[256]; static uint32_t pgRawSz[256];
volatile int P3DS_PgGender = -1, P3DS_PgItem = -1, P3DS_PgItem2 = -1, P3DS_PgItemReq;
/* run 13: progress per Pokegear location from pspoke hook 0083 (read-only flag checks on the game thread). Row i < 100:
   [0] species available, [1] caught, [2] seen, [3] trainers, [4] beaten, [5] visible items, [6] taken, [7] hidden items,
   [8] found. Row 100 = whole map (species counted once). */
/* run 16: hook 0083's ROM tables, kept here because the field overlay's .bss is cleared on every reload (each battle) */
uint16_t P3DS_PsKId[2400]; uint8_t P3DS_PsKLoc[2400], P3DS_PsKKind[2400], P3DS_PsKSp[100][64]; int P3DS_PsKN, P3DS_PsKBuilt;
volatile int P3DS_PsReady; volatile uint32_t P3DS_PsTick; uint16_t P3DS_PsStat[101][9];
/* run 16: hook 0084 publishes the wild species set per location (bit = species 1..493) and which of them are caught */
uint8_t P3DS_PsSp[100][64], P3DS_PsCg[100][64];
/* run 146 / hook 0209 (pg_find): when a species is found per location, [t]: 0 morning, 1 day, 2 night (land), 3 any time (water, rock smash); kept here like P3DS_PsK* */
uint8_t P3DS_PsTm[100][4][64];
/* 2026-11-18 hook 0287: sub-areas (map sections inside a Pokegear place, e.g. Safari Zone areas, cave floors); ROM table, kept here for the same reason */
volatile int P3DS_SubReady; int P3DS_SubN; uint16_t P3DS_SubSec[160]; uint8_t P3DS_SubLoc[160]; char P3DS_SubName[160][20];
/* hook 0286: how likely each species is per location and method, kept here like P3DS_PsTm. Method: 0 morning, 1 day, 2 night (grass), 3 surf, 4 rock smash, 5 old rod, 6 good rod, 7 super rod; pct = the game's slot chance given an encounter. */
#define kPgRtMax 6000
uint16_t P3DS_PsRtSp[kPgRtMax]; uint8_t P3DS_PsRtLoc[kPgRtMax], P3DS_PsRtMth[kPgRtMax], P3DS_PsRtPct[kPgRtMax]; int P3DS_PsRtN;
int P3DS_PsRtAdd(int loc, int sp, int mth, int pct) {
  if (loc < 0 || loc > 99 || sp <= 0 || sp > 493 || mth < 0 || mth > 7 || pct <= 0) return 0;
  for (int i = 0; i < P3DS_PsRtN; i++) if (P3DS_PsRtSp[i] == sp && P3DS_PsRtLoc[i] == loc && P3DS_PsRtMth[i] == mth) { int v = P3DS_PsRtPct[i] + pct; P3DS_PsRtPct[i] = v > 100 ? 100 : v; return 1; }
  if (P3DS_PsRtN >= kPgRtMax) return 0;
  const int n = P3DS_PsRtN++; P3DS_PsRtSp[n] = sp; P3DS_PsRtLoc[n] = loc; P3DS_PsRtMth[n] = mth; P3DS_PsRtPct[n] = pct > 100 ? 100 : pct; return 1; }
void *P3DS_PgAlloc(int key, uint32_t size) {
  if (key < 0 || key > 255 || size < 16 || size > 64 * 1024) return 0;
  if (key >= 200 && key <= 203) { if (size > 0x4000) return 0; if (!pgRaw[key]) pgRaw[key] = (uint8_t *)malloc(0x4000); pgRawSz[key] = pgRaw[key] ? size : 0; return pgRaw[key]; }
  free(pgRaw[key]); pgRaw[key] = (uint8_t *)malloc(size); pgRawSz[key] = pgRaw[key] ? size : 0; return pgRaw[key]; }
}
/* compatibility for the run 10 decode (members 10/11/20) */
#define pgRawM(m) pgRaw[m]
#define pgRawSzM(m) pgRawSz[m]

enum { kPgW = 47 * 8, kPgH = 20 * 8, kPgN = 100 };
struct SPg {
  uint32_t img[kPgW * kPgH]; uint8_t lit[47 * 20]; bool decoded, failed;
  uint8_t visited[16]; uint32_t tid; bool tidKnown, dirty;
  int sel = -1; int viewX = -1; int lastLoc = -2;
  int zoom = 1; int viewY = -1; int panAcc = 0; float accX = 0, accY = 0, velX = 0, velY = 0; /* 2026-11-18: sub-pixel pen pan + fling (map px) */ /* zoom 1 = the normal map; viewY is the map row at the top when zoom is 2 */
  int zmX0 = 0, zmY0 = 0, zmX1 = 0, zmY1 = 0, zpX0 = 0, zpY0 = 0, zpX1 = 0, zpY1 = 0; /* minus / plus, DS px; x1==x0 means hidden */
  bool weather; int wxX0, wxY0, wxX1;
  int markX, markY; bool markOk, gameMarker; /* run 11: marker position (DS px) for the native overlay, and whether the game's head is used */
  bool ov; int ovScroll; int ovRows[kPgN]; int ovN, ovListY0, ovListY1, ovTop; /* run 13: progress overview (menu-dots button) */
  struct { short x, y; uint8_t k; } ico[48]; int nIco; bool gameIco; /* small stat icons drawn by the native overlay */
  int dotsX0, dotsY0, dotsX1, dotsY1;
  uint8_t subVis[24]; bool subDirty; int subX0, subY0, subX1, subY1; /* 2026-11-18 sub-areas: visited bits (by table index), AREAS chip rect (DS px, x1==x0 none) */
  bool achOpen; int achScroll, achX0, achY0, achX1, achY1; /* achievements list (achievements.c), button left of the dots */
  int achUpX0, achUpY0, achUpX1, achUpY1, achDnX0, achDnY0, achDnX1, achDnY1, achPage, achMax; /* UP/DN page the list; achMax is the clamp */
  int flyX0, flyY0, flyX1, flyY1; /* hook 0228 (pg_flytap): the FLY chip on the open info card, 0 size = none (screen px) */
  int pressX = -1, pressY = -1; /* 2026-11-18: finger down on the map panel (DS px), so the chip under it shows pressed at once; -1 none */
  int findList, findPage; /* where-to-catch list over the map (finder LIST chip) and its page */
  int find, findTm; /* run 146 pg_find: species being looked for (0 none) and the time filter (0 any, 1 morning, 2 day, 3 night) */
  int page; /* run 16: info card page, 0 = info, 1 = wild Pokemon list (tap the card to flip) */
  bool glide, moving; /* run 16: view slides to the player's new location instead of jumping; moving = still sliding (redraw) */
  int kantoX = -2, kantoAlways = -1; bool kantoOpen; uint8_t hide[47 * 20]; /* run 16: Kanto hidden until a Kanto place is visited */
};
static inline bool pgVis(const SPg &p, int i) { return i >= 0 && i < kPgN && (p.visited[i >> 3] >> (i & 7)) & 1; }
static inline void pgSpecRect(int i, int *x, int *y, int *w, int *h) {
  const SS3DS_PgSpec &l = sLocationSpecs[i]; *x = l.x; *y = l.y; *w = l.bits & 15; *h = (l.bits >> 4) & 15; if (*w < 1) *w = 1; if (*h < 1) *h = 1; }
/* Screen pixel whose centre samples DS pixel ds. The bottom picture is nearest-doubled into a 512-wide image
   (DS pixel ds fills texels [2*ds, 2*ds+2)) and then drawn linearly across 320 px. Truncating ds*5/4 matches
   that when ds%4 is 0 or 1. When ds%4 is 2 the sample sits on the texel edge, and when it is 3 the sample lands
   a full screen pixel into the previous pixel, so the head crawls against the town while the view recentres.
   n = round((2*ds+1)*5/8 - 1/2) = (2*ds+1)*5/8. */
static int pgDsToNative(int ds) { int n = (2 * ds + 1) * 5 / 8; return n < 0 ? 0 : n; }
static int pgMapZoom(); /* 1 or 2 */
static bool pgRoamSpr();
static int pgMapLimit(const SPg &p, int visW) { return !p.kantoOpen && p.kantoX > 0 && p.kantoX * 8 > visW ? p.kantoX * 8 : kPgW; }
static int pgClampLeft(const SPg &p, int left, int visW) { const int lim = pgMapLimit(p, visW); if (left > lim - visW) left = lim - visW; if (left < 0) left = 0; return left; }
/* panel interior pgDraw actually paints (the old tap math was 1 or 2 px off this, and the sign flipped with the name strip) */
static void pgInner(int *X0, int *Y0, int *X1, int *Y1) {
  const int nh = blNameH(); *X0 = kBlPanX0 + 2; *Y0 = kBlPanY0 + 15 + nh + 2; *X1 = kBlPanX1 - 2; *Y1 = kBlPanY1 - 2; }
/* 2026-11-18: smooth pen pan. dx/dy are map px (fractions carry over); zoom 1 only moves sideways. Edges clamp and kill the fling. */
static void pgPanBy(SPg &p, float dx, float dy) {
  int X0, Y0, X1, Y1; pgInner(&X0, &Y0, &X1, &Y1); const int z = p.zoom < 2 ? 1 : 2; int visW = (X1 - X0) / z, visH = (Y1 - Y0) / z; if (visW < 1) visW = 1; if (visH < 1) visH = 1;
  if (z < 2) dy = 0;
  p.accX += dx; p.accY += dy; const int ix = (int)p.accX, iy = (int)p.accY; p.accX -= ix; p.accY -= iy;
  if (p.viewX >= 0) { const int w = p.viewX + ix, n = pgClampLeft(p, w, visW); if (n != w) { p.velX = 0; p.accX = 0; } p.viewX = n; }
  if (z > 1 && p.viewY >= 0) { const int w = p.viewY + iy; int n = w; if (n > kPgH - visH) n = kPgH - visH; if (n < 0) n = 0; if (n != w) { p.velY = 0; p.accY = 0; } p.viewY = n; } }
/* every frame with no finger down: keep gliding after a flick and slow down; true = view moved (redraw) */
static bool pgPanTick(SPg &p) {
  if (p.velX > -0.05f && p.velX < 0.05f && p.velY > -0.05f && p.velY < 0.05f) { p.velX = p.velY = 0; return false; }
  pgPanBy(p, p.velX, p.velY); p.velX *= 0.92f; p.velY *= 0.92f; return true; }
static void pgTouchMap(const SPg &p, int tx, int ty, int *mx, int *my) {
  int X0, Y0, X1, Y1; pgInner(&X0, &Y0, &X1, &Y1); const int z = p.zoom < 2 ? 1 : 2; const int ah = Y1 - Y0;
  if (z <= 1) { const int oy = Y0 + (ah - kPgH) / 2; *mx = tx - X0 + p.viewX; *my = ty - oy; }
  else { *mx = p.viewX + (tx - X0) / z; *my = p.viewY + (ty - Y0) / z; } }
/* point marker: at 2x the +z/2 lands on the centre of that map pixel's block. At 1x it is the old DS pixel. */
static void pgToDs(const SPg &p, int X0, int Y0, int oy, int mx, int my, int *sx, int *sy) {
  const int z = p.zoom < 2 ? 1 : 2;
  if (z <= 1) { *sx = X0 + mx - p.viewX; *sy = oy + my; }
  else { *sx = X0 + (mx - p.viewX) * z + z / 2; *sy = Y0 + (my - p.viewY) * z + z / 2; } }
static void pgToDsCorner(const SPg &p, int X0, int Y0, int oy, int mx, int my, int *sx, int *sy) {
  const int z = p.zoom < 2 ? 1 : 2;
  if (z <= 1) { *sx = X0 + mx - p.viewX; *sy = oy + my; }
  else { *sx = X0 + (mx - p.viewX) * z; *sy = Y0 + (my - p.viewY) * z; } }
static void pgCenterOn(SPg &p, int mapCx, int mapCy, int aw, int ah) {
  p.zoom = pgMapZoom(); const int z = p.zoom < 2 ? 1 : 2; int visW = aw / z; if (visW < 1) visW = 1;
  p.viewX = pgClampLeft(p, mapCx - visW / 2, visW); p.glide = false;
  if (z > 1) { int visH = ah / z; if (visH < 1) visH = 1; int top = mapCy - visH / 2; if (top > kPgH - visH) top = kPgH - visH; if (top < 0) top = 0; p.viewY = top; } }

/* find a Nitro section by its 4-byte magic (stored reversed in the file) */
static const uint8_t *pgSect(const uint8_t *d, uint32_t n, const char *m) {
  for (uint32_t i = 16; i + 8 <= n; i += 4) if (!memcmp(d + i, m, 4)) return d + i; return 0; }
static inline uint32_t pgU32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static inline uint16_t pgU16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }

/* decode once the hook has delivered the three members; returns true when the picture is ready */
static bool pgDecode(SPg &p) {
  if (p.decoded) return true; if (p.failed || !P3DS_PgReady) return false;
  const uint8_t *chr = pgSect(pgRaw[10], pgRawSz[10], "RAHC"), *scr = pgSect(pgRaw[11], pgRawSz[11], "NRCS"), *pal = pgSect(pgRaw[20], pgRawSz[20], "TTLP");
  if (!chr || !scr || !pal) { p.failed = true; return false; }
  const uint8_t *C = chr + 8 + pgU32(chr + 28); const uint32_t cn = pgU32(chr + 24);
  const int sw = pgU16(scr + 8) / 8; const uint8_t *S = scr + 20;
  const uint8_t *P = pal + 8 + pgU32(pal + 20); const uint32_t pn = pgU32(pal + 16) / 2;
  if (sw < 47 || cn < 64 || pn < 16 || C + cn > pgRaw[10] + pgRawSz[10]) { p.failed = true; return false; }
  uint32_t colors[256];
  colors[0] = SUI_RGB(0, 60, 46); /* index 0 = transparent map backdrop */
  for (unsigned i = 1; i < 256; ++i) {
    if (i < pn) {
      const uint16_t c = pgU16(P + 2 * i);
      colors[i] = SUI_RGB((c & 31) * 255 / 31, (c >> 5 & 31) * 255 / 31, (c >> 10 & 31) * 255 / 31);
    } else colors[i] = colors[0];
  }
  for (int ty = 0; ty < 20; ty++) for (int tx = 0; tx < 47; tx++) {
    const uint16_t e = pgU16(S + 2 * (ty * sw + tx)); const uint32_t t = e & 1023; const bool fx = e >> 10 & 1, fy = e >> 11 & 1;
    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
      const int sx = fx ? 7 - x : x, sy = fy ? 7 - y : y; const uint32_t o = t * 64 + sy * 8 + sx; const int v = o < cn ? C[o] : 0;
      p.img[(ty * 8 + y) * kPgW + tx * 8 + x] = colors[v]; } }
  p.decoded = true; return true; }

static void pgRelight(SPg &p) {
  memset(p.lit, 0, sizeof p.lit);
  for (int i = 0; i < kPgN; i++) if (pgVis(p, i)) { int x, y, w, h; pgSpecRect(i, &x, &y, &w, &h);
    for (int yy = y; yy < y + h && yy < 20; yy++) for (int xx = x; xx < x + w && xx < 47; xx++) if (xx >= 0 && yy >= 0) p.lit[yy * 47 + xx] = 1; } }

static void pgLoadVisited(SPg &p, uint32_t tid, const char *path) {
  memset(p.visited, 0, sizeof p.visited); p.tid = tid; p.tidKnown = true;
  FILE *f = fopen(path, "rb"); uint8_t b[24];
  if (f) { if (fread(b, 1, 24, f) == 24 && !memcmp(b, "VB01", 4) && pgU32(b + 4) == tid) memcpy(p.visited, b + 8, 16); fclose(f); }
  pgRelight(p); }
static bool pgSaveVisited(SPg &p, const char *path) {
  FILE *f = fopen(path, "wb"); if (!f) return false; uint8_t b[24]; memcpy(b, "VB01", 4);
  for (int i = 0; i < 4; i++) b[4 + i] = (uint8_t)(p.tid >> (8 * i)); memcpy(b + 8, p.visited, 16);
  const bool ok = fwrite(b, 1, 24, f) == 24; fclose(f); if (ok) p.dirty = false; return ok; }
/* returns 1 when a location was lit for the first time */
static int pgMark(SPg &p, int loc) {
  if (loc < 0 || loc >= kPgN || pgVis(p, loc)) return 0; p.visited[loc >> 3] |= 1 << (loc & 7); p.dirty = true; pgRelight(p); return 1; }
static int pgCount(const SPg &p) { int n = 0; for (int i = 0; i < kPgN; i++) n += pgVis(p, i); return n; }

/* location under map pixel (mx,my), smallest rect wins */
static int pgHitLoc(int mx, int my) {
  int best = -1, ba = 999; const int tx = mx / 8, ty = my / 8;
  for (int i = 0; i < kPgN; i++) { int x, y, w, h; pgSpecRect(i, &x, &y, &w, &h);
    if (tx >= x && tx < x + w && ty >= y && ty < y + h && w * h < ba) { ba = w * h; best = i; } }
  return best; }
/* 2026-11-18 pen: a tap that lands just beside a small town (1 tile = 8 px at 1x) still picks it. Exact hits win; otherwise
   the nearest place whose box is within r map px. */
static int pgHitLocNear(int mx, int my, int r) {
  const int exact = pgHitLoc(mx, my); if (exact >= 0 || r <= 0) return exact;
  int best = -1, bd = r + 1, ba = 999;
  for (int i = 0; i < kPgN; i++) { int x, y, w, h; pgSpecRect(i, &x, &y, &w, &h); if (w <= 0 || h <= 0) continue;
    const int dx = mx < x * 8 ? x * 8 - mx : mx >= (x + w) * 8 ? mx - (x + w) * 8 + 1 : 0;
    const int dy = my < y * 8 ? y * 8 - my : my >= (y + h) * 8 ? my - (y + h) * 8 + 1 : 0;
    const int d = dx > dy ? dx : dy;
    if (d < bd || (d == bd && w * h < ba)) { bd = d; ba = w * h; best = i; } }
  return bd <= r ? best : -1; }

/* simple word wrap into lines of at most `room` pixels */
static int pgWrap(const SUiText &T, const char *s, int room, char out[][64], int maxLines) {
  int n = 0; char line[64] = ""; const char *p = s;
  while (*p && n < maxLines) { char word[40]; int k = 0; while (*p == ' ') p++; while (*p && *p != ' ' && k < 39) word[k++] = *p++; word[k] = 0; if (!k) break;
    char tryl[100]; snprintf(tryl, sizeof tryl, "%s%s%s", line, line[0] ? " " : "", word);
    if (T.width(tryl) <= room || !line[0]) snprintf(line, sizeof line, "%s", tryl);
    else { snprintf(out[n++], 64, "%s", line); snprintf(line, sizeof line, "%s", word); } }
  if (line[0] && n < maxLines) snprintf(out[n++], 64, "%s", line); return n; }
/* shorten `s` into `out` so it fits maxW; a cut string ends with ".." */
static void pgFit(const SUiText &T, char *out, int sz, const char *s, int maxW) {
  if (sz < 1) return;
  snprintf(out, (size_t)sz, "%s", s ? s : "");
  if (maxW <= 0) { out[0] = 0; return; }
  if (T.width(out) <= maxW) return;
  int k = (int)strlen(out);
  while (k > 0) {
    k--;
    if (k + 3 <= sz) { out[k] = '.'; out[k + 1] = '.'; out[k + 2] = 0; }
    else out[k] = 0;
    if (out[0] && T.width(out) <= maxW) return;
    out[k] = 0;
  }
}

/* 2026-11-18 polish (Nathan: "still looks early in dev"): one look for every MAP page, matching the PHONE / RADIO / EGG panes
   and the side column. Small buttons are pgChip (navy outline with cut corners, white-to-silver, green when on, a soft shadow);
   titles sit in a green band; boxes are pgCard (paper-to-mint with a navy edge); counts are an icon, "a/n" and a thin bar. */
static const uint32_t kPgInk = SUI_RGB(7, 33, 54), kPgText = SUI_RGB(18, 26, 38), kPgPaper = SUI_RGB(248, 252, 250), kPgMint = SUI_RGB(218, 238, 228),
  kPgDone = SUI_RGB(28, 132, 84), kPgDim = SUI_RGB(104, 122, 118), kPgGold = SUI_RGB(255, 229, 155);
static inline void pgShade(uint32_t *b, int x, int y) {
  if (x < suiClipX0 || y < suiClipY0 || x >= suiClipX1 || y >= suiClipY1 || x >= 256 || y >= 192 || x < 0 || y < 0) return;
  b[y * 256 + x] = suiMix(b[y * 256 + x], kBlNavy, 2, 5); }
/* 2026-11-18 pen: chips are small on the 3DS screen, so every tap target reaches a little past its picture (dx left/right,
   dy up/down; the gaps between neighbours are 4 px, so dx stays 2) */
static inline bool pgIn(int x, int y, int x0, int y0, int x1, int y1, int dx = 2, int dy = 4) { return x1 > x0 && x >= x0 - dx && x < x1 + dx && y >= y0 - dy && y < y1 + dy; }
static SPg *gPgPress; /* the panel being drawn, for pgPressed (set by pgDraw) */
static inline bool pgPressed(int x0, int y0, int x1, int y1) { return gPgPress && gPgPress->pressX >= 0 && pgIn(gPgPress->pressX, gPgPress->pressY, x0, y0, x1, y1); }
static void pgChip(uint32_t *b, int x0, int y0, int x1, int y1, bool on) {
  on = on || pgPressed(x0, y0, x1, y1);
  const uint32_t hi = on ? kBlPress : SUI_RGB(252, 253, 255), lo = on ? kBlBlue : kBlSilverLo;
  for (int y = y0 + 1; y < y1 - 1; y++) suiFill(b, x0 + 1, y, x1 - 1, y + 1, suiMix(hi, lo, y - y0, y1 - y0));
  for (int x = x0 + 1; x < x1 - 1; x++) { suiPx(b, x, y0, kBlNavy); suiPx(b, x, y1 - 1, kBlNavy); }
  for (int y = y0 + 1; y < y1 - 1; y++) { suiPx(b, x0, y, kBlNavy); suiPx(b, x1 - 1, y, kBlNavy); }
  for (int x = x0 + 1; x < x1; x++) pgShade(b, x, y1); for (int y = y0 + 1; y < y1; y++) pgShade(b, x1, y); }
/* green title band (same colours as the tab strip's selected button and the PHONE / RADIO titles) */
static void pgBand(uint32_t *b, int x0, int y0, int x1, int y1) {
  for (int y = y0; y < y1; y++) suiFill(b, x0, y, x1, y + 1, suiMix(kBlPress, kBlBlue, y - y0, y1 - y0));
  suiFill(b, x0, y1, x1, y1 + 1, kBlNavy); }
static void pgCard(uint32_t *b, int x0, int y0, int x1, int y1) {
  for (int y = y0 + 1; y < y1 - 1; y++) suiFill(b, x0 + 1, y, x1 - 1, y + 1, suiMix(kPgPaper, kPgMint, y - y0, y1 - y0));
  for (int x = x0 + 1; x < x1 - 1; x++) { suiPx(b, x, y0, kBlNavy); suiPx(b, x, y1 - 1, kBlNavy); }
  for (int y = y0 + 1; y < y1 - 1; y++) { suiPx(b, x0, y, kBlNavy); suiPx(b, x1 - 1, y, kBlNavy); } }
/* 9x9 count icons: '#' navy, '.' clear, 'w' white, others a colour from kPgIcoPal */
enum { kPgIcCaught, kPgIcSeen, kPgIcTrainer, kPgIcItem, kPgIcHidden, kPgIcPlace, kPgIcN };
static const char *const kPgIco[kPgIcN][9] = {
  { "..#####..", ".#rrrrr#.", "#rrrrrrr#", "#rr###rr#", "####w####", "#ww###ww#", "#wwwwwww#", ".#wwwww#.", "..#####.." },
  { ".........", "..#####..", ".#wwwww#.", "#ww#b#ww#", "#w##b##w#", "#ww#b#ww#", ".#wwwww#.", "..#####..", "........." },
  { "..####...", ".#rrrr#..", ".######..", "..#ss#...", "..#ss#...", ".######..", "#bbbbbb#.", "#bbbbbb#.", "########." },
  { "...###...", "...#w#...", "..##w##..", ".#ppppp#.", "#pwppppp#", "#pwppppp#", "#ppppppp#", ".#ppppp#.", "..#####.." },
  { "....#....", "....y....", "...#y#...", "..#yyy#..", "#yyywyyy#", "..#yyy#..", "...#y#...", "....y....", "....#...." },
  { "..#####..", ".#ggggg#.", "#gggwggg#", "#ggwwwgg#", ".#ggwgg#.", "..#ggg#..", "...#g#...", "....#....", "........." } };
static void pgIco(uint32_t *b, int x, int y, int k) {
  uint32_t px[81];
  for (int r = 0; r < 9; r++) for (int c = 0; c < 9; c++) { uint32_t v; switch (kPgIco[k][r][c]) {
    case '#': v = kBlNavy; break; case 'w': v = SUI_RGB(255, 255, 255); break; case 'r': v = SUI_RGB(226, 64, 52); break;
    case 'b': v = SUI_RGB(56, 112, 216); break; case 's': v = SUI_RGB(248, 208, 168); break; case 'p': v = SUI_RGB(168, 96, 208); break;
    case 'y': v = SUI_RGB(246, 196, 40); break; case 'g': v = kBlPress; break; default: v = 0; }
    px[r * 9 + c] = v; }
  btxIcon(b, x, y, k, px); } /* 2026-11-18: drawn at 1:1 on the 320x240 layer when nothing covers it (ss3ds_sharptext.h) */
/* 3 px progress bar: light track, green fill, brighter green once complete */
static void pgBar(uint32_t *b, int x0, int y, int x1, int a, int n) {
  if (x1 - x0 < 4) return; const int w = x1 - x0, f = n > 0 ? (a >= n ? w : w * a / n) : 0;
  for (int k = 0; k < 3; k++) { suiFill(b, x0, y + k, x1, y + k + 1, SUI_RGB(196, 214, 208)); if (f) suiFill(b, x0, y + k, x0 + f, y + k + 1, a >= n ? (k ? kBlBlue : kBlPress) : (k ? SUI_RGB(40, 128, 92) : SUI_RGB(64, 176, 124))); }
  suiPx(b, x0, y, kPgMint); suiPx(b, x1 - 1, y, kPgMint); suiPx(b, x0, y + 2, kPgMint); suiPx(b, x1 - 1, y + 2, kPgMint); }
/* a location's (or the whole map's, loc 100) counts as up to five {icon, done, total} */
struct SPgStat { int ico, a, n; const char *name; };
static int pgStatList(int loc, SPgStat *out, bool seen) {
  const uint16_t *s = P3DS_PsStat[loc]; int k = 0;
  if (s[0]) out[k++] = { kPgIcCaught, s[1], s[0], "Caught" }; if (seen && s[0]) out[k++] = { kPgIcSeen, s[2], s[0], "Seen" };
  if (s[3]) out[k++] = { kPgIcTrainer, s[4], s[3], "Trainers" }; if (s[5]) out[k++] = { kPgIcItem, s[6], s[5], "Items" }; if (s[7]) out[k++] = { kPgIcHidden, s[8], s[7], "Hidden" };
  return k; }
/* one row of count cells (icon, "a/n", bar under) across [x0, x1); 13 px tall. Returns the number of cells (0: a quiet note instead). */
static int pgStatCells(uint32_t *b, const SUiText &T, int x0, int y, int x1, int loc, uint32_t sh) {
  SPgStat st[5]; const int k = pgStatList(loc, st, false);
  if (!k) { suiSDraw(T, b, x0, y + 1, "Nothing to collect here", kPgDim, sh); return 0; }
  const int cw = (x1 - x0 + 4) / k;
  for (int i = 0; i < k; i++) { const int cx = x0 + i * cw, done = st[i].a >= st[i].n; char t[16]; snprintf(t, sizeof t, "%d/%d", st[i].a, st[i].n);
    /* the bar first: it sits under the digits' lowest rows, and anything drawn over a recorded string pulls it back into the stretched
       DS picture (that was the garbled "5/12" on the PROGRESS page) */
    pgBar(b, cx, y + 11, cx + cw - 5, st[i].a, st[i].n);
    pgIco(b, cx, y, st[i].ico); const int room = cw - 15;
    if (T.width(t) <= room) T.draw(b, cx + 11, y - 2, t, done ? kPgDone : kPgText, sh); else suiSDraw(T, b, cx + 11, y, t, done ? kPgDone : kPgText, sh); }
  return k; }
#include "achievements.h"
static void pgDots(uint32_t *b, SPg &p, int X1, int Y0) {
  p.dotsX0 = X1 - 20; p.dotsY0 = Y0 + 2; p.dotsX1 = X1 - 2; p.dotsY1 = Y0 + 14;
  p.achX1 = p.dotsX0 - 4; p.achX0 = p.achX1 - 16; p.achY0 = p.dotsY0; p.achY1 = p.dotsY1;
  /* achievements: a gold folder on a chip, red corner dot while something new is unread */
  pgChip(b, p.achX0, p.achY0, p.achX1, p.achY1, p.achOpen);
  { const int fx = p.achX0 + 3, fy = p.achY0 + 3, fw = 10, fh = 7;
    for (int y = 0; y < fh; y++) for (int x = 0; x < fw; x++) { const bool edge = !y || !x || y == fh - 1 || x == fw - 1; suiPx(b, fx + x, fy + y, edge ? kBlNavy : (y < 2 ? SUI_RGB(255, 236, 140) : SUI_RGB(246, 200, 72))); }
    for (int x = 1; x < 5; x++) suiPx(b, fx + x, fy - 1, kBlNavy); }
  if (P3DS_AchUnviewed()) for (int y = p.achY0 - 1; y < p.achY0 + 3; y++) for (int x = p.achX1 - 4; x < p.achX1; x++) suiPx(b, x, y, (y == p.achY0 - 1 || x == p.achX1 - 4) ? kBlNavy : SUI_RGB(226, 48, 40));
  /* overview: three dots */
  pgChip(b, p.dotsX0, p.dotsY0, p.dotsX1, p.dotsY1, p.ov);
  for (int d = 0; d < 3; d++) for (int yy = 0; yy < 2; yy++) for (int xx = 0; xx < 2; xx++) suiPx(b, p.dotsX0 + 4 + d * 4 + xx, p.dotsY0 + 5 + yy, p.ov ? SUI_RGB(255, 255, 255) : kBlNavy); }
/* contest_helper (hook 0229): Bug-Catching Contest card over the top of the panel body while a contest runs. Time is the game's own whole-minute counter
   (the contest ends when 20 minutes are used), so "min left" can be up to a minute generous. Drawn only for blMode < 0 and no Pokegear pane. */
static void ctCardDraw(uint32_t *b, const SUiText &T, int X0, int Y0, int X1) {
  if (!P3DS_ContestOn() || !P3DS_Ct.active) return;
  const P3DS_ContestInfo &c = P3DS_Ct; char t[64]; const int left = 20 - c.elapsed, x0 = X0 + 2, x1 = X1 - 2, y0 = Y0 + 2;
  /* 2026-11-18: same card and green title band as the MAP pages */
  pgCard(b, x0, y0, x1, y0 + 40); pgBand(b, x0 + 1, y0 + 1, x1 - 1, y0 + 12);
  suiSDraw(T, b, x0 + 5, y0 + 2, "BUG-CATCHING CONTEST", SUI_RGB(255, 255, 255), kBlSelO);
  const uint32_t warn = SUI_RGB(176, 40, 24);
  if (left > 0) snprintf(t, sizeof t, "TIME  %d min left", left); else snprintf(t, sizeof t, "TIME UP");
  suiSDraw(T, b, x0 + 5, y0 + 15, t, left <= 3 ? warn : kPgText, kPgPaper);
  pgBar(b, x0 + 96, y0 + 18, x1 - 6, left > 0 ? left : 0, 20);
  snprintf(t, sizeof t, "BALLS  %d / 20", c.balls); suiSDraw(T, b, x0 + 5, y0 + 26, t, c.balls <= 3 ? warn : kPgText, kPgPaper);
  if (c.species > 0) { snprintf(t, sizeof t, "%s Lv%d", kP3dsSpecies[c.species], c.level); suiSDraw(T, b, x0 + 96, y0 + 26, t, kPgDone, kPgPaper); }
  else suiSDraw(T, b, x0 + 96, y0 + 26, "none yet", kPgDim, kPgPaper);
}
/* run 45: roamer colours (map squares and the overview's ROAMING card) */
static const uint32_t kTrkCol[4] = { SUI_RGB(232, 184, 16), SUI_RGB(224, 72, 40), SUI_RGB(240, 96, 144), SUI_RGB(46, 208, 155) };
/* card title in the polish style: small dark-green capitals, optional right-hand note */
static void pgCardTitle(uint32_t *b, const SUiText &T, int x0, int y, int x1, const char *title, const char *right, uint32_t rightInk) {
  suiSDraw(T, b, x0 + 5, y + 2, title, kBlSelO, kPgPaper);
  if (right && right[0]) suiSDraw(T, b, x1 - 5 - suiSWidth(T, right), y + 2, right, rightInk, kPgPaper);
  for (int x = x0 + 4; x < x1 - 4; x++) suiPx(b, x, y + 12, SUI_RGB(176, 214, 196)); }
/* run 45 tracker as cards (roaming Pokemon, then the Day-Care) at content y; returns the height used, 0 when off */
static int pgTracker(uint32_t *b, const SUiText &T, int x0, int Y, int x1) {
  if (!P3DS_TrkOn()) return 0; int y = Y; char t[64];
  int rows = 0; if (P3DS_TrkReady) for (int i = 0; i < 4; i++) if (P3DS_TrkRoam[i].active && P3DS_TrkRoam[i].species) rows++;
  int h = 16 + (rows ? rows : 1) * 12 + 2; pgCard(b, x0, y, x1, y + h); pgCardTitle(b, T, x0, y, x1, "ROAMING NOW", 0, 0);
  int ry = y + 15;
  if (!P3DS_TrkReady) suiSDraw(T, b, x0 + 6, ry + 1, "Take a step to look.", kPgDim, kPgPaper);
  else if (!rows) suiSDraw(T, b, x0 + 6, ry + 1, "None out yet.", kPgDim, kPgPaper);
  else for (int i = 0; i < 4; i++) { const P3DS_TrkRoamer &r = P3DS_TrkRoam[i]; if (!r.active || !r.species) continue;
    const char *l = r.loc >= 0 && r.loc < kPgN && P3DS_PgName[r.loc][0] ? P3DS_PgName[r.loc] : "somewhere";
    for (int yy = 0; yy < 8; yy++) for (int xx = 0; xx < 8; xx++) suiPx(b, x0 + 6 + xx, ry + 2 + yy, xx == 0 || yy == 0 || xx == 7 || yy == 7 ? kBlNavy : kTrkCol[i]);
    snprintf(t, sizeof t, "%s Lv%d", kP3dsSpecies[r.species], r.level); T.draw(b, x0 + 18, ry, t, kPgText, kPgPaper);
    char lc[40]; pgFit(T, lc, sizeof lc, l, x1 - 6 - (x0 + 18 + T.width(t) + 6)); suiSDraw(T, b, x1 - 6 - suiSWidth(T, lc), ry + 2, lc, kPgDone, kPgPaper); ry += 12; }
  y += h + 4;
  const P3DS_TrkDaycare &d = P3DS_TrkDc; const char *st = d.n == 0 ? "EMPTY" : d.n == 1 ? "1 POKEMON" : d.egg ? "EGG READY" : d.compat > 0 ? "2 POKEMON" : "NO MATCH";
  const int lines = d.n >= 2 ? (d.compat > 0 ? 3 : 2) : 1; h = 16 + lines * 11 + 3;
  pgCard(b, x0, y, x1, y + h); pgCardTitle(b, T, x0, y, x1, "DAY-CARE", st, d.egg ? kPgDone : kPgDim);
  ry = y + 15;
  if (d.n == 0) suiSDraw(T, b, x0 + 6, ry + 1, "Leave two Pokemon to find eggs.", kPgDim, kPgPaper);
  else if (d.n == 1) suiSDraw(T, b, x0 + 6, ry + 1, "One Pokemon: needs a partner.", kPgText, kPgPaper);
  else {
    snprintf(t, sizeof t, "%s + %s", kP3dsSpecies[d.sp[0]], kP3dsSpecies[d.sp[1]]); suiSDraw(T, b, x0 + 6, ry + 1, t, kPgText, kPgPaper); ry += 11;
    if (d.compat <= 0) suiSDraw(T, b, x0 + 6, ry + 1, "These two cannot breed.", SUI_RGB(160, 90, 50), kPgPaper);
    else {
      if (d.egg) snprintf(t, sizeof t, "Egg waiting! %d%% per roll", d.compat); else snprintf(t, sizeof t, "Egg %d%% per roll, next in %d steps", d.compat, d.toCheck < 0 ? 0 : d.toCheck);
      suiSDraw(T, b, x0 + 6, ry + 1, t, d.egg ? kPgDone : kPgText, kPgPaper); ry += 11;
      suiSDraw(T, b, x0 + 6, ry + 1, d.masuda ? "Masuda: YES, shiny 1/1638" : "Masuda: no, shiny 1/8192", d.masuda ? kPgDone : kPgDim, kPgPaper); } }
  y += h + 4;
  return y - Y; }
/* the overview: a green title band (the corner buttons sit on it), then one scrolling column of cards: Johto totals with
   bars, the tracker cards, and one card per visited place (tap opens it on the map). */
enum { kPgOvBand = 16, kPgOvRow = 32 };
static void pgOverview(uint32_t *b, const SUiText &T, SPg &p, int X0, int Y0, int X1, int Y1) {
  for (int y = Y0; y < Y1; ++y) suiFill(b, X0, y, X1, y + 1, suiMix(SUI_RGB(214, 240, 230), SUI_RGB(168, 218, 196), y - Y0, Y1 - Y0));
  pgBand(b, X0, Y0, X1, Y0 + kPgOvBand);
  char t[64]; int n = 0; for (int i = 0; i < kPgN; i++) n += pgVis(p, i);
  T.draw(b, X0 + 4, Y0 + 1, "PROGRESS", SUI_RGB(255, 255, 255), kBlSelO);
  snprintf(t, sizeof t, "%d/%d places", n, kPgN);
  { const int r = X1 - 42 - 4; /* 42 = kPgAchCtrl, the corner buttons */ if (r - suiSWidth(T, t) > X0 + 4 + T.width("PROGRESS") + 6) suiSDraw(T, b, r - suiSWidth(T, t), Y0 + 3, t, SUI_RGB(255, 255, 255), kBlSelO); }
  const int V0 = Y0 + kPgOvBand + 1, x0 = X0 + 3, x1 = X1 - 6;
  p.ovListY0 = V0; p.ovListY1 = Y1; p.ovN = 0; p.ovTop = 1 << 20;
  const int clipY0 = suiClipY0, clipY1 = suiClipY1; suiClipY0 = V0; suiClipY1 = Y1;
  int y = V0 + 3 - p.ovScroll;
  if (!P3DS_PsReady) { pgCard(b, x0, y, x1, y + 22); suiSDraw(T, b, x0 + 6, y + 6, "Take a step to count.", kPgText, kPgPaper); y += 26; }
  else {
    SPgStat st[5]; const int k = pgStatList(100, st, true); const int rh = 12, h = 16 + (k + 1) * rh + 2;
    pgCard(b, x0, y, x1, y + h); pgCardTitle(b, T, x0, y, x1, "JOHTO TOTALS", 0, 0);
    for (int i = -1; i < k; i++) { const int ry = y + 15 + (i + 1) * rh; const SPgStat s = i < 0 ? SPgStat{ kPgIcPlace, n, kPgN, "Visited" } : st[i];
      char c[16]; snprintf(c, sizeof c, "%d/%d", s.a, s.n); const int cw = T.width(c), done = s.a >= s.n;
      pgBar(b, x0 + 66, ry + 4, x1 - 9 - cw, s.a, s.n);
      pgIco(b, x0 + 5, ry + 1, s.ico); T.draw(b, x0 + 17, ry - 1, s.name, kPgText, kPgPaper);
      T.draw(b, x1 - 5 - cw, ry - 1, c, done ? kPgDone : kPgText, kPgPaper); }
    y += h + 4;
    y += pgTracker(b, T, x0, y, x1);
    for (int i = 0; i < kPgN; i++) if (pgVis(p, i)) { bool dup = false; for (int j = 0; j < p.ovN; j++) if (!strcmp(P3DS_PgName[p.ovRows[j]], P3DS_PgName[i])) dup = true; if (!dup) p.ovRows[p.ovN++] = i; }
    snprintf(t, sizeof t, "PLACES VISITED  %d", p.ovN); suiSDraw(T, b, x0 + 2, y + 1, t, kBlSelO, SUI_RGB(200, 232, 218)); y += 13;
    p.ovTop = y + p.ovScroll - V0;
    if (!p.ovN) { pgCard(b, x0, y, x1, y + 22); suiSDraw(T, b, x0 + 6, y + 6, "Visit places to list them here.", kPgText, kPgPaper); y += 26; }
    for (int r = 0; r < p.ovN; r++, y += kPgOvRow) { if (y + kPgOvRow < V0 || y >= Y1) continue; const int l = p.ovRows[r];
      pgCard(b, x0, y, x1, y + kPgOvRow - 2);
      char nm[40]; pgFit(T, nm, sizeof nm, P3DS_PgName[l][0] ? P3DS_PgName[l] : "?", x1 - x0 - 60); T.draw(b, x0 + 5, y + 1, nm, kPgInk, kPgPaper);
      SPgStat ls[5]; const int lk = pgStatList(l, ls, false); bool all = lk > 0; for (int i = 0; i < lk; i++) all = all && ls[i].a >= ls[i].n;
      if (all) suiSDraw(T, b, x1 - 5 - suiSWidth(T, "COMPLETE"), y + 3, "COMPLETE", kPgDone, kPgPaper);
      pgStatCells(b, T, x0 + 5, y + 15, x1 - 3, l, kPgPaper); }
  }
  const int total = y + p.ovScroll - V0 + 2, view = Y1 - V0, maxS = total > view ? total - view : 0;
  suiClipY0 = clipY0; suiClipY1 = clipY1;
  if (p.ovScroll > maxS) p.ovScroll = maxS; if (p.ovScroll < 0) p.ovScroll = 0;
  if (maxS > 0) { const int bh = view * view / (view + maxS), by = V0 + (view - bh) * p.ovScroll / maxS;
    for (int yy = V0 + 1; yy < Y1 - 1; yy++) { suiPx(b, X1 - 3, yy, SUI_RGB(176, 214, 196)); suiPx(b, X1 - 2, yy, SUI_RGB(176, 214, 196)); }
    for (int yy = by; yy < by + bh; yy++) { suiPx(b, X1 - 3, yy, kBlBlue); suiPx(b, X1 - 2, yy, kBlBlue); } } }
/* overview place under DS y, -1 none */
static int pgOvHit(const SPg &p, int y) {
  if (!p.ov || y < p.ovListY0 || y >= p.ovListY1 || p.ovTop >= (1 << 20)) return -1;
  const int c = y - p.ovListY0 + p.ovScroll - p.ovTop; if (c < 0) return -1;
  const int r = c / kPgOvRow; return r < p.ovN && c % kPgOvRow < kPgOvRow - 2 ? p.ovRows[r] : -1; }

/* ---- run 16: Kanto on the map only after you have been there (the real Pokegear shows it after the S.S. Aqua).
   Kanto places are recognised by their names (from the ROM via hook 0081); the hidden area starts at the westmost Kanto
   town's column, but places that are not Kanto (Route 26/27, Victory Road, Indigo Plateau, Mt. Silver...) stay visible even
   when they lie further east. The gate opens for good when a Kanto place is in visited.bin or you are standing in one. */
static bool pgIsKantoName(int i) {
  if (i < 0 || i >= kPgN) return false; const char *n = P3DS_PgName[i]; char u[24]; int k = 0;
  for (; n[k] && k < 23; k++) u[k] = (char)(n[k] >= 'a' && n[k] <= 'z' ? n[k] - 32 : n[k]); u[k] = 0;
  static const char *const town[] = { "PALLET", "VIRIDIAN", "PEWTER", "CERULEAN", "VERMILION", "LAVENDER", "CELADON", "FUCHSIA", "SAFFRON", "CINNABAR",
    "SEAFOAM", "DIGLETT", "MT. MOON", "MT.MOON", "ROCK TUNNEL", "POWER PLANT" };
  for (const char *t : town) if (!strncmp(u, t, strlen(t))) return true;
  if (!strncmp(u, "ROUTE ", 6)) { const int r = atoi(u + 6); return r >= 1 && r <= 25; }
  return false; }
/* 2026-11-17 cloud profile: the gate check below runs every frame and upper-cased and compared all 100 names each time
   (~3% of busy CPU time). The names are fixed once the ROM hook has filled them (P3DS_PgReady), so the answer is kept. */
static bool pgIsKanto(int i) {
  static signed char known[kPgN]; static bool init;
  if (i < 0 || i >= kPgN) return false; if (!P3DS_PgReady) return pgIsKantoName(i);
  if (!init) { init = true; memset(known, -1, sizeof known); }
  if (known[i] < 0) known[i] = pgIsKantoName(i) ? 1 : 0; return known[i] != 0; }
static bool pgIsKantoTown(int i) { return pgIsKanto(i) && strncmp(P3DS_PgName[i], "Route", 5) && strncmp(P3DS_PgName[i], "ROUTE", 5); }
/* recompute the gate; returns true when it changed (the caller logs and redraws) */
static bool pgKantoUpdate(SPg &p, int always) {
  if (p.kantoX == -2) { p.kantoX = -1; for (int i = 0; i < kPgN; i++) if (pgIsKantoTown(i)) { int x, y, w, h; pgSpecRect(i, &x, &y, &w, &h); if (p.kantoX < 0 || x < p.kantoX) p.kantoX = x; } }
  bool open = always || p.kantoX < 0 || pgIsKanto(P3DS_PgLoc); for (int i = 0; i < kPgN && !open; i++) if (pgIsKanto(i) && pgVis(p, i)) open = true;
  const bool changed = open != p.kantoOpen || always != p.kantoAlways || p.kantoX == -1; p.kantoOpen = open; p.kantoAlways = always;
  if (!changed) return false;
  memset(p.hide, 0, sizeof p.hide);
  if (!open) { for (int ty = 0; ty < 20; ty++) for (int tx = p.kantoX; tx < 47; tx++) p.hide[ty * 47 + tx] = 1;
    for (int pass = 0; pass < 2; pass++) for (int i = 0; i < kPgN; i++) { if (pgIsKanto(i) != (pass == 0)) continue; int x, y, w, h; pgSpecRect(i, &x, &y, &w, &h); /* Kanto rects hidden first, then non-Kanto rects shown */
      for (int yy = y; yy < y + h && yy < 20; yy++) for (int xx = x; xx < x + w && xx < 47; xx++) if (xx >= 0 && yy >= 0) p.hide[yy * 47 + xx] = pass == 0; } }
  if (p.kantoX == -1) p.kantoX = -3; /* "no Kanto town found" reported once */
  return true; }
static inline bool pgHiddenAt(const SPg &p, int mx, int my) { return mx >= 0 && my >= 0 && mx < kPgW && my < kPgH && p.hide[(my / 8) * 47 + mx / 8]; }
/* hook 0228: is this location a Fly point the player has unlocked (the game's own flag, published by the game thread)? */
static bool pgFlyUnlocked(int loc) { if (loc < 0 || loc >= kPgN) return false; const int m = sLocationSpecs[loc].mapId, n = P3DS_FlyN < 32 ? P3DS_FlyN : 32;
  for (int i = 0; i < n; i++) if (P3DS_FlyList[i] == m) return true; return false; }
static inline int pgCardH(const SPg &p) { return p.page ? 110 : 74; }
/* ---- 2026-11-18 sub-areas: card page 2 lists the map sections inside the place (hook 0287); the ones you have stood in turn green ---- */
static int pgSubCount(const SPg &p, int loc, int *seen) { int n = 0; if (seen) *seen = 0; for (int i = 0; i < P3DS_SubN && i < 160; i++) if (P3DS_SubLoc[i] == loc) { n++; if (seen && (p.subVis[i >> 3] >> (i & 7) & 1)) (*seen)++; } return n; }
static int pgSubMark(SPg &p, int sec) { int n = 0; for (int i = 0; i < P3DS_SubN && i < 160; i++) if (P3DS_SubSec[i] == sec && !(p.subVis[i >> 3] >> (i & 7) & 1)) { p.subVis[i >> 3] |= 1 << (i & 7); p.subDirty = true; n++; } return n; }
static void pgSubLoad(SPg &p, uint32_t tid, const char *path) {
  memset(p.subVis, 0, sizeof p.subVis); FILE *f = fopen(path, "rb"); uint8_t b[32];
  if (f) { if (fread(b, 1, 32, f) == 32 && !memcmp(b, "SA01", 4) && pgU32(b + 4) == tid) memcpy(p.subVis, b + 8, 24); fclose(f); } }
static bool pgSubSave(SPg &p, const char *path) {
  FILE *f = fopen(path, "wb"); if (!f) return false; uint8_t b[32]; memcpy(b, "SA01", 4);
  for (int i = 0; i < 4; i++) b[4 + i] = (uint8_t)(p.tid >> (8 * i)); memcpy(b + 8, p.subVis, 24);
  const bool ok = fwrite(b, 1, 32, f) == 32; fclose(f); if (ok) p.subDirty = false; return ok; }
/* species in a location's wild set; fills `out` (up to max), returns the count, `*caught` = how many are caught */
static int pgWild(int loc, int *out, int max, int *caught) {
  int n = 0; *caught = 0; if (loc < 0 || loc >= kPgN) return 0;
  for (int sp = 1; sp <= 493; sp++) if (P3DS_PsSp[loc][sp >> 3] >> (sp & 7) & 1) { if (n < max) out[n] = sp; n++; if (P3DS_PsCg[loc][sp >> 3] >> (sp & 7) & 1) (*caught)++; }
  return n; }

/* ---- run 146: Pokedex encounter finder (pg_find). Tap a name on the wild list and the map pins every place that species lives, with a morning/day/night filter ---- */
static bool pgFindCfg(); /* render3ds.cpp */
static bool pgFlyCfg(); /* render3ds.cpp (hook 0228) */
static const char *const kPgFindTm[4] = { "ANY", "MORN", "DAY", "NITE" };
static int pgTimeNow() { const time_t now = time(0); struct tm tmv; const struct tm *lt = localtime_r(&now, &tmv); const int h = lt ? lt->tm_hour : 12; return h >= 4 && h < 10 ? 1 : h >= 10 && h < 20 ? 2 : 3; }
static bool pgTmHave() { for (int l = 0; l < 100; l++) for (int t = 0; t < 4; t++) for (int i = 0; i < 64; i++) if (P3DS_PsTm[l][t][i]) return true; return false; }
static bool pgFindAt(int loc, int sp, int tm, bool tmOk) {
  const uint8_t m = (uint8_t)(1 << (sp & 7)); const int o = sp >> 3;
  if (tm == 0 || !tmOk) return (P3DS_PsSp[loc][o] & m) != 0;
  return ((P3DS_PsTm[loc][tm - 1][o] | P3DS_PsTm[loc][3][o]) & m) != 0; }
static int pgFindCount(const SPg &p) { if (p.find <= 0 || p.find > 493) return 0; const bool ok = pgTmHave(); int n = 0; for (int l = 0; l < kPgN && l < 100; l++) n += pgFindAt(l, p.find, p.findTm, ok); return n; }
static void pgWildLayout(int aw, int ch, int wn, int *cw, int *rows, int *shown) { *cw = (aw - 8) / 3; *rows = (ch - 18) / 10; const int slots = *rows * 3; *shown = wn > slots ? slots - 1 : wn; }
/* species under a tap on the wild list page (card spans the panel's bottom), 0 = none */
static int pgWildHit(const SPg &p, int x, int y) {
  if (p.sel < 0 || p.page != 1) return 0; const int X0 = kBlPanX0 + 2, aw = (kBlPanX1 - 2) - X0, ch = pgCardH(p), cy0 = (kBlPanY1 - 2) - ch;
  int wl[64], cg = 0; const int wn = pgWild(p.sel, wl, 64, &cg); if (!wn) return 0; int cw, rows, shown; pgWildLayout(aw, ch, wn, &cw, &rows, &shown);
  const int col = (x - (X0 + 4)) / cw, row = (y - (cy0 + 16)) / 10; if (x < X0 + 4 || y < cy0 + 16 || col < 0 || col > 2 || row < 0 || row >= rows) return 0;
  const int i = col * rows + row; return i < shown ? wl[i] : 0; }
/* Leave room for the native font's full ink height before painting the next row. */
enum { kPgFindH = 48, kPgFindTimeY = 31, kPgFindTimeH = 16, kPgFindRowH = 17, kPgFindFooterH = 18 };
/* finder strip hit: 1 = clear (X), 2..5 = time filter 0..3 */
static int pgFindHit(const SPg &p, int Y0, int x, int y) {
  if (p.find <= 0) return 0; const int X0 = kBlPanX0 + 2, X1 = kBlPanX1 - 2, tx = X0 + 42, cw = (X1 - tx - 2) / 4;
  if (x >= X1 - 17 && x < X1 && y >= Y0 && y < Y0 + 16) return 1;
  if (x >= X1 - 49 && x < X1 - 18 && y >= Y0 && y < Y0 + 16) return 6;
  if (p.findList && y >= (kBlPanY1 - 2) - kPgFindFooterH && y < kBlPanY1 - 2) { if (x < X0 + 52) return 7; if (x >= X1 - 52) return 8; }
  if (p.findList && y >= Y0 + kPgFindH) return 9;
  if (y >= Y0 + kPgFindTimeY - 1 && y < Y0 + kPgFindTimeY + kPgFindTimeH) { const int i = (x - tx) / cw; if (x >= tx && i >= 0 && i < 4) return 2 + i; }
  return 0; }
static void pgFindPins(uint32_t *b, const SPg &p, int X0, int Y0, int X1, int oy) {
  const bool ok = pgTmHave();
  for (int l = 0; l < 100 && l < kPgN; l++) { if (!pgFindAt(l, p.find, p.findTm, ok)) continue; int x, y, w, h; pgSpecRect(l, &x, &y, &w, &h);
    const int mx = x * 8 + w * 4, my = y * 8 + h * 4; if (pgHiddenAt(p, mx, my)) continue; int px, py; pgToDs(p, X0, Y0, oy, mx, my, &px, &py); if (px < X0 + 4 || px >= X1 - 4) continue;
    for (int yy = -4; yy <= 4; yy++) for (int xx = -4; xx <= 4; xx++) { const int d = abs(xx) + abs(yy); if (d <= 4) suiPx(b, px + xx, py + yy, d == 4 ? SUI_RGB(255, 255, 255) : d == 3 ? SUI_RGB(120, 16, 16) : SUI_RGB(236, 56, 48)); } } }
/* HGSS menu icons use tiled 4bpp graphics and a separate bank of 16-colour palettes. */
extern "C" int P3DS_RomNarcMember(const char *path, int member, unsigned char *dst, unsigned cap);
extern "C" uint32_t GetMonIconNaixEx(uint32_t species, int isEgg, uint32_t form);
extern "C" uint8_t GetMonIconPaletteEx(uint32_t species, uint32_t form, uint32_t isEgg);

static bool pgMonIcon(int sp, uint32_t *dst, bool speciesColours = false) {
  memset(dst, 0, 32 * 32 * sizeof(*dst));
  if (sp <= 0 || sp > 493) return false;

  static uint8_t graphics[2048], palettes[2048];
  const int member = GetMonIconNaixEx(sp, 0, 0);
  const unsigned bank = speciesColours ? GetMonIconPaletteEx(sp, 0, 0) : 0;
  const int graphicsSize = P3DS_RomNarcMember("a/0/2/0", member, graphics, sizeof graphics);
  const int paletteSize = P3DS_RomNarcMember("a/0/2/0", 0, palettes, sizeof palettes);
  if (graphicsSize <= 0 || paletteSize <= 0) return false;

  const uint8_t *chr = pgSect(graphics, graphicsSize, "RAHC");
  const uint8_t *pal = pgSect(palettes, paletteSize, "TTLP");
  if (!chr || !pal || graphics + graphicsSize - chr < 32 || palettes + paletteSize - pal < 24)
    return false;

  const uint64_t pixelsOffset = (chr - graphics) + 8ull + pgU32(chr + 28);
  const unsigned bankOffset = bank * 32;
  const uint64_t coloursOffset = (pal - palettes) + 8ull + pgU32(pal + 20) + bankOffset;
  if (pgU32(chr + 24) < 512 || pixelsOffset + 512 > (unsigned)graphicsSize ||
      bankOffset + 32 > pgU32(pal + 16) || coloursOffset + 32 > (unsigned)paletteSize)
    return false;

  uint32_t colours[16] = {};
  for (int i = 1; i < 16; i++) {
    const uint16_t colour = pgU16(palettes + coloursOffset + i * 2);
    const unsigned r = colour & 31, g = (colour >> 5) & 31, b = (colour >> 10) & 31;
    if (speciesColours) {
      colours[i] = SUI_RGB((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2));
    } else {
      const unsigned brightness = (r * 30 + g * 59 + b * 11) / 100;
      colours[i] = suiMix(SUI_RGB(0, 70, 54), SUI_RGB(250, 250, 255), brightness, 31);
    }
    colours[i] |= 0xFF000000u;
  }

  const uint8_t *pixels = graphics + pixelsOffset;
  for (int tile = 0; tile < 16; tile++) {
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        const unsigned index = (pixels[tile * 32 + y * 4 + x / 2] >> (4 * (x & 1))) & 15;
        dst[((tile / 4) * 8 + y) * 32 + (tile % 4) * 8 + x] = colours[index];
      }
    }
  }
  return true;
}
static uint32_t pgFindIco[32 * 32]; static int pgFindIcoSp = -1; static bool pgFindIcoOk;
static void pgFindIconLoad(int sp) {
  if (pgFindIcoSp == sp) return; pgFindIcoSp = sp; pgFindIcoOk = pgMonIcon(sp, pgFindIco, true);
  PSPNativeMemLog("[FIND] icon sp=%d -> %s", sp, pgFindIcoOk ? "ok" : "missing (card has no sprite)"); }
/* one 16x16 roamer icon, cached per slot. False = caller draws the letter. Loaded only when the species changes. */
static uint32_t pgRoamPx[4][16 * 16]; static int pgRoamSp[4] = { -1, -1, -1, -1 }; static int pgRoamOk[4];
static bool pgRoamBlit(uint32_t *b, int slot, int sp, int cx, int cy, int X0, int Y0, int X1, int Y1) {
  if (slot < 0 || slot > 3 || sp <= 0 || sp > 493) return false;
  if (pgRoamSp[slot] != sp) { pgRoamSp[slot] = sp; uint32_t big[32 * 32]; pgRoamOk[slot] = pgMonIcon(sp, big, true); memset(pgRoamPx[slot], 0, sizeof pgRoamPx[slot]);
    if (pgRoamOk[slot]) for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) pgRoamPx[slot][y * 16 + x] = big[(y * 2) * 32 + x * 2];
    PSPNativeMemLog("[TRACK] roamer sprite slot %d species %d %s", slot, sp, pgRoamOk[slot] ? "ok" : "missing, letter kept"); }
  if (!pgRoamOk[slot]) return false;
  for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { const uint32_t c = pgRoamPx[slot][y * 16 + x]; if (!(c >> 24)) continue;
    const int px = cx - 8 + x, py = cy - 8 + y; if (px < X0 || py < Y0 || px >= X1 || py >= Y1) continue; suiPx(b, px, py, c & 0xFFFFFF); }
  return true; }
/* card: sprite box left, name + count, X top right, time chips below (hit: pgFindHit) */
static void pgFindBanner(uint32_t *b, const SUiText &T, const SPg &p, int X0, int Y0, int X1) {
  const uint32_t ink = SUI_RGB(255, 255, 255), bar = SUI_RGB(7, 33, 54), edge = SUI_RGB(190, 214, 206), chip = SUI_RGB(40, 130, 100), gold = SUI_RGB(246, 214, 96);
  for (int y = Y0; y < Y0 + kPgFindH; y++) for (int x = X0; x < X1; x++) suiPx(b, x, y, y == Y0 + kPgFindH - 1 || y == Y0 ? edge : suiMix(SUI_RGB(14, 120, 90), bar, y - Y0, kPgFindH));
  pgFindIconLoad(p.find);
  for (int y = 0; y < 34; y++) for (int x = 0; x < 34; x++) suiPx(b, X0 + 3 + x, Y0 + 3 + y, x == 0 || y == 0 || x == 33 || y == 33 ? edge : SUI_RGB(0, 60, 46));
  if (pgFindIcoOk) for (int y = 0; y < 32; y++) for (int x = 0; x < 32; x++) { const uint32_t c = pgFindIco[y * 32 + x]; if (c >> 24) suiPx(b, X0 + 4 + x, Y0 + 4 + y, c & 0xFFFFFF); }
  const int tx = X0 + 42; const int n = pgFindCount(p);
  for (int y = Y0 + 3; y < Y0 + 14; y++) for (int x = X1 - 15; x < X1 - 3; x++) suiPx(b, x, y, SUI_RGB(200, 60, 52));
  T.draw(b, X1 - 12, Y0 + 4, "X", ink, SUI_RGB(200, 60, 52));
  { const uint32_t lc = p.findList ? gold : chip; for (int y = Y0 + 3; y < Y0 + 14; y++) for (int x = X1 - 48; x < X1 - 19; x++) suiPx(b, x, y, lc); T.draw(b, X1 - 45, Y0 + 4, p.findList ? "MAP" : "LIST", p.findList ? bar : ink, lc); }
  char t[48]; snprintf(t, sizeof t, "%s", kP3dsSpecies[p.find]); int k = (int)strlen(t); const int room = X1 - 50 - tx; while (k > 3 && T.width(t) > room) t[--k] = 0;
  T.draw(b, tx, Y0 + 4, t, ink, SUI_RGB(14, 120, 90));
  char cn[32]; snprintf(cn, sizeof cn, n == 1 ? "1 place" : "%d places", n); T.draw(b, tx, Y0 + 16, cn, n ? gold : SUI_RGB(200, 160, 160), SUI_RGB(12, 110, 84));
  const int now = pgTimeNow(), have = pgTmHave(), cw = (X1 - tx - 2) / 4;
  for (int i = 0; i < 4; i++) { const int x0 = tx + i * cw; const bool on = p.findTm == i, dim = i > 0 && !have;
    for (int y = Y0 + kPgFindTimeY; y < Y0 + kPgFindTimeY + kPgFindTimeH; y++) for (int x = x0; x < x0 + cw - 2; x++) suiPx(b, x, y, on ? gold : chip);
    T.draw(b, x0 + 3, Y0 + kPgFindTimeY + 1, kPgFindTm[i], on ? bar : dim ? SUI_RGB(140, 170, 160) : ink, on ? gold : chip);
    if (i == now) for (int x = x0 + 2; x < x0 + cw - 4; x++) suiPx(b, x, Y0 + kPgFindTimeY + kPgFindTimeH - 1, SUI_RGB(120, 230, 140)); } }

/* where to catch: one row per place (name + method chances), paged. Chances are the game's slot odds once an encounter starts. */
static int pgFindRows(const SPg &p, int *out, int max) {
  int n = 0; const bool ok = pgTmHave(); if (p.find <= 0 || p.find > 493) return 0;
  for (int l = 0; l < kPgN && l < 100 && n < max; l++) if (pgFindAt(l, p.find, p.findTm, ok)) out[n++] = l;
  return n; }
static void pgFindDetail(int loc, int sp, int tm, char *out, int sz) {
  int m[8] = {}; bool any = false;
  for (int i = 0; i < P3DS_PsRtN; i++) if (P3DS_PsRtSp[i] == sp && P3DS_PsRtLoc[i] == loc) { m[P3DS_PsRtMth[i]] = P3DS_PsRtPct[i]; any = true; }
  if (!any) { snprintf(out, sz, "%s", P3DS_PsRtN ? "chance not known" : "chances need hook 0286"); return; }
  static const char *const nm[8] = { "", "", "", "Surf", "Smash", "Old rod", "Good rod", "Super rod" }; out[0] = 0; int o = 0;
  if (m[0] | m[1] | m[2]) {
    if (tm >= 1) o += snprintf(out + o, sz - o, "Grass %d%%", m[tm - 1]);
    else if (m[0] == m[1] && m[1] == m[2]) o += snprintf(out + o, sz - o, "Grass %d%%", m[0]);
    else o += snprintf(out + o, sz - o, "Grass M%d D%d N%d%%", m[0], m[1], m[2]); }
  for (int k = 3; k < 8 && o < sz - 1; k++) if (m[k]) o += snprintf(out + o, sz - o, "%s%s %d%%", o ? "  " : "", nm[k], m[k]); }
static void pgFindListDraw(uint32_t *b, const SUiText &T, SPg &p, int X0, int Y0, int X1, int Y1) {
  const uint32_t paper = SUI_RGB(236, 244, 240), ink = SUI_RGB(8, 40, 56), dim = SUI_RGB(70, 110, 100), edge = SUI_RGB(190, 214, 206), gold = SUI_RGB(246, 214, 96), chip = SUI_RGB(40, 130, 100);
  const int top = Y0 + kPgFindH, bot = Y1 - kPgFindFooterH; suiFill(b, X0, top, X1, Y1, paper);
  /* One line per place, with enough room for the full font above the separator. */
  int rows[100]; const int n = pgFindRows(p, rows, 100); const int pitch = kPgFindRowH; const int per = (bot - top - 2) / pitch > 0 ? (bot - top - 2) / pitch : 1; const int pages = n ? (n + per - 1) / per : 1;
  if (p.findPage >= pages) p.findPage = pages - 1; if (p.findPage < 0) p.findPage = 0;
  if (!n) T.draw(b, X0 + 6, top + 8, "Not found in the wild here", dim, paper);
  for (int r = 0; r < per; r++) { const int i = p.findPage * per + r; if (i >= n) break; const int y = top + 2 + r * pitch, l = rows[i];
    char dt[80], fit[80]; pgFindDetail(l, p.find, p.findTm, dt, sizeof dt); const int room = X1 - X0 - 12; pgFit(T, fit, sizeof fit, dt, room * 11 / 20); const int dw = T.width(fit);
    char nmb[40]; pgFit(T, nmb, sizeof nmb, P3DS_PgName[l][0] ? P3DS_PgName[l] : "?", room - dw - 8); T.draw(b, X0 + 5, y, nmb, ink, paper);
    T.draw(b, X1 - 6 - dw, y, fit, dim, paper);
    suiFill(b, X0 + 3, y + pitch - 2, X1 - 3, y + pitch - 1, edge); }
  suiFill(b, X0, bot, X1, Y1, chip);
  for (int k = 0; k < 2; k++) { const bool en = k ? p.findPage + 1 < pages : p.findPage > 0; const int x0 = k ? X1 - 50 : X0 + 2; pgChip(b, x0, bot + 1, x0 + 48, Y1 - 1, en); T.draw(b, x0 + 8, bot + 2, k ? "NEXT >" : "< PREV", en ? SUI_RGB(8, 40, 56) : dim, en ? SUI_RGB(236, 244, 240) : paper); }
  char pg[24]; snprintf(pg, sizeof pg, "%d/%d  (%d places)", p.findPage + 1, pages, n); T.draw(b, (X0 + X1) / 2 - T.width(pg) / 2, bot + 2, pg, gold, chip); }

/* draw the map into the panel interior (X0,Y0)-(X1,Y1); the view follows the player's location unless panned */
static void pxMapPins(uint32_t *b, const SPg &p, int X0, int Y0, int X1, int oy); /* ss3ds_pgextra.h: phone contact pins */
static bool pxCardText(const SUiText &T, char *out, int sz, int loc, int maxW);
/* achievements list. The trophy and the dots buttons sit on the title row (pgDots, drawn after this), so the point total
   has its own band under them. Each entry is name + points, then a wrapped description, then the earned date on a smaller line.
   Scroll is in pixels. A line that crosses the list edge is still drawn: pixels outside the list are clipped, so a tall
   row cannot vanish and leave a blank gap before the next one. */
enum { kPgAchCtrl = 42, kPgAchLine = 16, kPgAchDate = 16 }; /* date is drawn with the 3/4 font; the slot stays a full line so the fallback font cannot run into the next row */
/* One sample of a finger that went down on the map panel. *drag is 0 until the finger has moved more than 8px
   (the tap gate; 16 until 2026-11-18, which felt sticky). *prevY stays at the press y until that happens, so the sample that crosses the gate applies the
   whole stroke, not only the last step. Return value is added to achScroll: finger up (y smaller) is positive. */
static int pgAchDragDelta(int *drag, int *prevY, int y, int dist) {
  if (!*drag) { if (dist <= 8) return 0; *drag = 1; }
  int dy = y - *prevY; *prevY = y; return -dy; }
/* 1 = UP (toward the start of the list), 2 = DN (later rows), 0 = not on those chips */
static int pgAchBtnHit(const SPg &p, int x, int y) {
  if (!p.achOpen) return 0;
  if (pgIn(x, y, p.achUpX0, p.achUpY0, p.achUpX1, p.achUpY1, 2, 4)) return 1;
  if (pgIn(x, y, p.achDnX0, p.achDnY0, p.achDnX1, p.achDnY1, 2, 4)) return 2;
  return 0; }
static int pgAchWrap(const SUiText &T, const char *s, int room, char out[][64]) {
  int n = pgWrap(T, s, room, out, 2); if (n < 1) { out[0][0] = 0; return 0; }
  char probe[3][64]; if (n >= 2 && pgWrap(T, s, room, probe, 3) > 2) { char cut[68]; snprintf(cut, sizeof cut, "%s..", out[1]); pgFit(T, out[1], 64, cut, room); }
  for (int i = 0; i < n; i++) { char cut[64]; snprintf(cut, sizeof cut, "%s", out[i]); pgFit(T, out[i], 64, cut, room); }
  return n; }
static void pgAchPanel(uint32_t *b, const SUiText &T, SPg &p, int X0, int Y0, int X1, int Y1) {
  const uint32_t ink = SUI_RGB(7, 33, 54), sh = SUI_RGB(236, 240, 248), dim = SUI_RGB(100, 116, 112), gold = SUI_RGB(226, 168, 36), green = SUI_RGB(60, 170, 80);
  const uint32_t wh = SUI_RGB(255, 255, 255);
  for (int y = Y0; y < Y1; ++y)
    suiFill(b, X0, y, X1, y + 1, suiMix(SUI_RGB(252, 250, 240), SUI_RGB(226, 218, 190), y - Y0, Y1 - Y0));
  int mx = 0; const int tot = P3DS_AchTotal(-1, &mx); char t[64], nm[64];
  /* title row: words stop before the corner buttons. The glyph is 16px, so the totals band starts under it. */
  for (int y = Y0; y < Y0 + 17; ++y)
    suiFill(b, X0, y, X1, y + 1, suiMix(SUI_RGB(40, 170, 128), SUI_RGB(20, 140, 104), y - Y0, 17));
  pgFit(T, nm, sizeof nm, "ACHIEVEMENTS", (X1 - kPgAchCtrl) - (X0 + 4) - 4);
  T.draw(b, X0 + 4, Y0 + 1, nm, wh, SUI_RGB(7, 33, 54));
  /* totals band, full width, under the buttons */
  const int band0 = Y0 + 18, band1 = Y0 + 38;
  for (int y = band0; y < band1 && y < Y1; y++) for (int x = X0; x < X1; x++) suiPx(b, x, y, y == band0 ? SUI_RGB(12, 90, 70) : SUI_RGB(232, 244, 226));
  snprintf(t, sizeof t, "%d / %d G", tot, mx);
  p.achUpX0 = p.achUpX1 = p.achDnX0 = p.achDnX1 = 0;
  { const int bw = suiSWidth(T, "DN") + 10, gap = 3, dnX1 = X1 - 3, dnX0 = dnX1 - bw, upX1 = dnX0 - gap, upX0 = upX1 - bw;
    const int by0 = band0 + 2, by1 = band0 + 16;
    pgFit(T, nm, sizeof nm, t, upX0 - (X0 + 4) - 6);
    if (upX0 > X0 + 24 && by1 <= band1) {
      p.achUpX0 = upX0; p.achUpY0 = by0; p.achUpX1 = upX1; p.achUpY1 = by1;
      p.achDnX0 = dnX0; p.achDnY0 = by0; p.achDnX1 = dnX1; p.achDnY1 = by1;
      for (int y = by0; y < by1; y++) for (int x = upX0; x < upX1; x++) suiPx(b, x, y, (x == upX0 || x == upX1 - 1 || y == by0 || y == by1 - 1) ? kBlNavy : kBlPress);
      for (int y = by0; y < by1; y++) for (int x = dnX0; x < dnX1; x++) suiPx(b, x, y, (x == dnX0 || x == dnX1 - 1 || y == by0 || y == by1 - 1) ? kBlNavy : kBlPress);
      suiSDraw(T, b, upX0 + 5, by0 + 1, "UP", wh, kBlSelO); suiSDraw(T, b, dnX0 + 5, by0 + 1, "DN", wh, kBlSelO); } }
  if (band0 + 1 + kPgAchLine <= Y1) T.draw(b, X0 + 4, band0 + 1, nm, ink, SUI_RGB(232, 244, 226));
  { const int bx = X0 + 4, bw = X1 - X0 - 8, by = band0 + 17, fill = mx > 0 ? bw * tot / mx : 0;
    if (by + 2 < Y1) for (int x = 0; x < bw; x++) for (int yy = 0; yy < 3; yy++) suiPx(b, bx + x, by + yy, x < fill ? green : SUI_RGB(190, 204, 186)); }
  const int top = band1 + 1; int off = 0; const int textX = X0 + 22, textR = X1 - 6;
  const int clipX0 = suiClipX0, clipY0 = suiClipY0, clipX1 = suiClipX1, clipY1 = suiClipY1;
  suiClipX0 = X0; suiClipX1 = X1; suiClipY0 = top; suiClipY1 = Y1;
  for (int c = 0; c < ACH_CATS; c++) {
    int cm = 0; const int cs = P3DS_AchTotal(c, &cm); int cnt = 0, got = 0;
    for (int id = 0; id < ACH_COUNT; id++) if (P3DS_AchGet(id)->cat == c) { cnt++; if (P3DS_AchDate(id)) got++; }
    char left[48], right[32]; snprintf(left, sizeof left, "%s  %d/%d", P3DS_AchCatName(c), got, cnt); snprintf(right, sizeof right, "%dG of %dG", cs, cm);
    const int hy = top + off - p.achScroll; off += kPgAchLine;
    if (hy + kPgAchLine > top && hy < Y1) {
      for (int y = hy; y < hy + kPgAchLine; ++y)
        suiFill(b, X0 + 2, y, X1 - 2, y + 1, suiMix(SUI_RGB(218, 240, 232), SUI_RGB(190, 226, 212), y - hy, kPgAchLine));
      const int rw = T.width(right), room = textR - (X0 + 5) - 8;
      if (rw + 8 < room) { pgFit(T, nm, sizeof nm, left, room - rw - 8); T.draw(b, X0 + 5, hy, nm, ink, SUI_RGB(218, 240, 232)); T.draw(b, textR - rw, hy, right, dim, SUI_RGB(206, 232, 222)); }
      else { pgFit(T, nm, sizeof nm, left, room); T.draw(b, X0 + 5, hy, nm, ink, SUI_RGB(218, 240, 232)); } }
    for (int id = 0; id < ACH_COUNT; id++) { const P3DS_AchInfo *a = P3DS_AchGet(id); if (a->cat != c) continue;
      const int d = P3DS_AchDate(id); unsigned cu = 0, go = 0; const int prog = !d && P3DS_AchProgress(id, &cu, &go) && go > 1;
      char desc[2][64]; const int dl = pgAchWrap(T, a->desc, textR - textX, desc); const int lines = dl > 0 ? dl : 1;
      const int extra = (d || prog) ? kPgAchDate + 1 : 0;
      const int rh = 2 + kPgAchLine + lines * kPgAchLine + extra + 2;
      const int y = top + off - p.achScroll; off += rh;
      if (y + rh <= top || y >= Y1) continue;
      auto glyph = [&](int gy, int gh) { return gy + gh > top && gy < Y1; };
      if (glyph(y + 2, 12)) {
        const int ix = X0 + 5, iy = y + 2;
        for (int dy = 0; dy < 12; dy++) for (int dx = 0; dx < 12; dx++) { const int ex = dx - 6, ey = dy - 6; if (ex * ex + ey * ey > 33) continue;
          suiPx(b, ix + dx, iy + dy, d ? (ex * ex + ey * ey > 22 ? SUI_RGB(190, 138, 24) : (dy < 6 ? SUI_RGB(250, 214, 90) : gold)) : (ex * ex + ey * ey > 22 ? SUI_RGB(150, 150, 160) : SUI_RGB(190, 190, 198))); }
        if (d) { const int ck[7][2] = { {3,6},{4,7},{5,8},{6,7},{7,6},{8,5},{9,4} }; for (auto &q : ck) if (glyph(iy, 12)) { suiPx(b, ix + q[0] - 1, iy + q[1] - 1, SUI_RGB(70, 50, 8)); suiPx(b, ix + q[0] - 1, iy + q[1], SUI_RGB(70, 50, 8)); } }
        else { for (int dy = 5; dy < 9; dy++) for (int dx = 3; dx < 9; dx++) suiPx(b, ix + dx, iy + dy, SUI_RGB(110, 112, 124)); suiPx(b, ix + 4, iy + 4, SUI_RGB(110, 112, 124)); suiPx(b, ix + 7, iy + 4, SUI_RGB(110, 112, 124)); suiPx(b, ix + 5, iy + 3, SUI_RGB(110, 112, 124)); suiPx(b, ix + 6, iy + 3, SUI_RGB(110, 112, 124)); } }
      snprintf(t, sizeof t, "%dG", a->pts); const int pw = T.width(t), nameRoom = (textR - pw - 6) - textX;
      if (glyph(y + 1, kPgAchLine)) { pgFit(T, nm, sizeof nm, a->name, nameRoom > 8 ? nameRoom : 8); T.draw(b, textX, y + 1, nm, d ? ink : dim, sh); T.draw(b, textR - pw, y + 1, t, d ? gold : dim, sh); }
      for (int i = 0; i < lines; i++) { const int ly = y + 1 + kPgAchLine + i * kPgAchLine; if (glyph(ly, kPgAchLine) && desc[i][0]) T.draw(b, textX, ly, desc[i], dim, sh); }
      if (d && glyph(y + 1 + kPgAchLine + lines * kPgAchLine, kPgAchDate)) {
        snprintf(t, sizeof t, "%04d-%02d-%02d", d / 10000, d / 100 % 100, d % 100);
        suiSDraw(T, b, textX, y + 1 + kPgAchLine + lines * kPgAchLine, t, SUI_RGB(90, 110, 104), sh); }
      else if (prog && glyph(y + 1 + kPgAchLine + lines * kPgAchLine, kPgAchDate)) {
        snprintf(t, sizeof t, "%u/%u", cu > go ? go : cu, go); const int lw = suiSWidth(T, t), ly = y + 1 + kPgAchLine + lines * kPgAchLine;
        const int bw2 = 36, bx2 = textR - lw - bw2 - 4, f2 = (int)((unsigned long long)bw2 * (cu > go ? go : cu) / go);
        if (bx2 > textX) for (int x = 0; x < bw2; x++) for (int yy = 0; yy < 3; yy++) suiPx(b, bx2 + x, ly + 3 + yy, x < f2 ? green : SUI_RGB(200, 192, 168));
        suiSDraw(T, b, textR - lw, ly, t, dim, sh); } }
  }
  suiClipX0 = clipX0; suiClipY0 = clipY0; suiClipX1 = clipX1; suiClipY1 = clipY1;
  const int view = Y1 - top; int maxS = off - view; if (maxS < 0) maxS = 0;
  p.achMax = maxS; p.achPage = view > 24 ? view - 8 : 48;
  if (p.achScroll > maxS) p.achScroll = maxS; if (p.achScroll < 0) p.achScroll = 0; }

static void pgZoomChip(uint32_t *b, int x0, int y0, bool plus) {
  pgChip(b, x0, y0, x0 + 12, y0 + 12, false);
  suiFill(b, x0 + 3, y0 + 5, x0 + 9, y0 + 7, kBlNavy);
  if (plus) suiFill(b, x0 + 5, y0 + 3, x0 + 7, y0 + 9, kBlNavy); }
static const uint32_t kPgWeatherColor[] = {
  0, SUI_RGB(48, 154, 255), SUI_RGB(225, 252, 255), SUI_RGB(183, 155, 240)
};
static uint8_t pgWeatherMask[kPgW * kPgH];

static void pgWeatherRefresh(bool kantoOpen) {
  static uint32_t seen;
  static bool kantoWasOpen;
  if (seen == P3DS_PgWeatherTick && kantoWasOpen == kantoOpen) return;
  seen = P3DS_PgWeatherTick;
  kantoWasOpen = kantoOpen;
  memset(pgWeatherMask, 0, sizeof pgWeatherMask);
  for (int i = 0; i < kPgN; i++) {
    const int kind = P3DS_PgWeather[i];
    if (kind < 1 || kind > 3 || (!kantoOpen && pgIsKanto(i))) continue;
    int x, y, w, h;
    pgSpecRect(i, &x, &y, &w, &h);
    const int x0 = x * 8, y0 = y * 8, x1 = x0 + w * 8, y1 = y0 + h * 8;
    // A short feathered edge joins nearby cells while keeping the forecast anchored to mapped locations.
    for (int yy = y0 - 4; yy < y1 + 4; yy++) for (int xx = x0 - 4; xx < x1 + 4; xx++) {
      if (xx < 0 || xx >= kPgW || yy < 0 || yy >= kPgH) continue;
      const int dx = xx < x0 ? x0 - xx : xx >= x1 ? xx - x1 + 1 : 0;
      const int dy = yy < y0 ? y0 - yy : yy >= y1 ? yy - y1 + 1 : 0;
      const int opacity = 5 - dx - dy;
      uint8_t &cell = pgWeatherMask[yy * kPgW + xx];
      if (opacity > (cell >> 2)) cell = (opacity << 2) | kind;
    }
  }
}

static void pgWeatherButton(uint32_t *b, const SUiText &T, SPg &p, int x, int y, bool active) {
  p.wxX0 = x; p.wxX1 = x + 22; p.wxY0 = y;
  pgChip(b, x, y, p.wxX1, y + 12, active);
  suiSDraw(T, b, x + (22 - suiSWidth(T, "WX")) / 2, y + 2, "WX", active ? SUI_RGB(255, 255, 255) : kBlNavy, active ? kBlBlue : kBlSilverHi);
}

static const char *pgWeatherStatus(unsigned visibleKinds) {
  if (!P3DS_PgWeatherTick) return "Waiting for forecast";
  if (!visibleKinds) return "Clear in this view";
  return "Seasonal radar";
}

static const char *pgWeatherSeasonName() {
  static const char *const seasons[] = { "Spring", "Summer", "Autumn", "Winter" };
  const int season = P3DS_PgWeatherSeason;
  return P3DS_PgWeatherTick && season >= 0 && season < 4 ? seasons[season] : nullptr;
}

static void pgWeatherStatusBar(uint32_t *b, const SUiText &T, int x, int y, int right, unsigned visibleKinds) {
  suiFill(b, x, y, right, y + 12, kBlNavy);
  const char *status = pgWeatherStatus(visibleKinds);
  const char *season = pgWeatherSeasonName();
  char label[48];
  if (season) {
    snprintf(label, sizeof label, "%s | %s", season, visibleKinds ? "Forecast" : "Clear in this view");
    status = suiSWidth(T, label) <= right - x - 8 ? label : season;
  }
  suiSDraw(T, b, x + 4, y + 1, status, kBlSilverHi, kBlNavy);
}

static void pgWeatherLegend(uint32_t *b, const SUiText &T, int x, int y, int right, unsigned visibleKinds, bool showSeason) {
  suiFill(b, x, y, right, y + 12, kBlNavy);
  static const char *const names[] = { "Rain", "Snow", "Mist" };
  x += 4;
  const char *season = showSeason ? pgWeatherSeasonName() : nullptr;
  if (season) {
    suiSDraw(T, b, x, y + 1, season, kBlSilverHi, kBlNavy);
    x += suiSWidth(T, season) + 10;
  }
  for (int i = 0; i < 3; i++) {
    if (!(visibleKinds & (1u << (i + 1)))) continue;
    if (x + 8 + suiSWidth(T, names[i]) > right) break;
    suiFill(b, x, y + 4, x + 5, y + 9, kPgWeatherColor[i + 1]);
    suiSDraw(T, b, x + 8, y + 1, names[i], kBlSilverHi, kBlNavy);
    x += 13 + suiSWidth(T, names[i]);
  }
}

static bool pgWeatherHit(const SPg &p, int x, int y) {
  return pgIn(x, y, p.wxX0, p.wxY0, p.wxX1, p.wxY0 + 12);
}

/* places not visited yet: a quarter of the way to grey, then 1/4 toward a cool slate. The old 5/8 mix toward dark green
   turned most of the map olive and muddy on the hardware screen; this keeps the art readable and still clearly "not yet". */
static inline uint32_t pgFade(uint32_t c) {
  const int r = c & 255, g = c >> 8 & 255, bl = c >> 16 & 255, l = (r * 77 + g * 150 + bl * 29) >> 8;
  return SUI_RGB((((r * 3 + l) >> 2) * 3 + 92) >> 2, (((g * 3 + l) >> 2) * 3 + 112) >> 2, (((bl * 3 + l) >> 2) * 3 + 120) >> 2); }
static void pgDraw(uint32_t *b, const SUiText &T, SPg &p, int X0, int Y0, int X1, int Y1, bool blink) {
  const int aw = X1 - X0, ah = Y1 - Y0; p.nIco = 0; p.zmX1 = p.zpX1 = p.wxX1 = p.achX1 = p.dotsX1 = 0; p.zoom = pgMapZoom(); gPgPress = &p;
  if (p.achOpen) { p.markOk = false; pgAchPanel(b, T, p, X0, Y0, X1, Y1); pgDots(b, p, X1, Y0); return; }
  if (p.ov) { p.markOk = false; pgOverview(b, T, p, X0, Y0, X1, Y1); pgDots(b, p, X1, Y0); return; }
  const int z = p.zoom < 2 ? 1 : 2; int visW = aw / z; if (visW < 1) visW = 1;
  int cx = kPgW / 2, cy = kPgH / 2; if (P3DS_PgLoc >= 0) { int x, y, w, h; pgSpecRect(P3DS_PgLoc, &x, &y, &w, &h); cx = x * 8 + w * 4; cy = y * 8 + h * 4; }
  { static int prevZ = 1; if (z != prevZ && p.viewX >= 0) { const int oldW = aw / prevZ < 1 ? 1 : aw / prevZ; p.viewX = pgClampLeft(p, p.viewX + oldW / 2 - visW / 2, visW); if (z > 1) p.viewY = -1; p.glide = false; prevZ = z; } else prevZ = z; }
  const int mapW = pgMapLimit(p, visW); /* Kanto hidden: do not scroll into the empty east */
  int goal = cx - visW / 2; if (goal > mapW - visW) goal = mapW - visW; if (goal < 0) goal = 0;
  int goalY = 0; if (z > 1) { int visH = ah / z; if (visH < 1) visH = 1; goalY = cy - visH / 2; if (goalY > kPgH - visH) goalY = kPgH - visH; if (goalY < 0) goalY = 0; }
  const int q = P3DS_FpsTweaks() >= 5 ? 2 : 4; /* level 5: half the repaints, double the step */
  if (p.viewX < 0) { p.viewX = goal; p.glide = false; }
  else if (p.glide) { int d = goal - p.viewX; if (d >= -1 && d <= 1) p.viewX = goal; else p.viewX += d / q ? d / q : (d > 0 ? 1 : -1);
    if (z > 1 && p.viewY >= 0) { d = goalY - p.viewY; if (d >= -1 && d <= 1) p.viewY = goalY; else p.viewY += d / q ? d / q : (d > 0 ? 1 : -1); } /* the view eases up/down too, not only sideways */
    if (p.viewX == goal && (z <= 1 || p.viewY < 0 || p.viewY == goalY)) p.glide = false; }
  p.moving = p.glide;
  if (p.viewX > mapW - visW) p.viewX = mapW - visW; if (p.viewX < 0) p.viewX = 0;
  if (z <= 1) { p.panAcc = 0; p.accX = p.accY = p.velX = p.velY = 0; }
  const int oy = Y0 + (ah - kPgH) / 2;
  if (z > 1) { int visH = ah / z; if (visH < 1) visH = 1; if (p.viewY < 0) p.viewY = goalY; if (p.viewY > kPgH - visH) p.viewY = kPgH - visH; if (p.viewY < 0) p.viewY = 0; }
  const bool weatherEnabled = P3DS_AmbientWeatherCfg();
  const bool weather = weatherEnabled && p.weather;
  const bool finder = p.find > 0 && pgFindCfg();
  const int weatherTop = Y0 + (finder ? kPgFindH : 0);
  const int weatherBottom = p.sel >= 0 ? Y1 - pgCardH(p) : Y1;
  unsigned visibleWeather = 0;
  if (weather) pgWeatherRefresh(p.kantoOpen);
  for (int y = Y0; y < Y1; y++) {
    const int my = z == 1 ? y - oy : std::clamp(p.viewY + ((y - Y0) >> 1), 0, kPgH - 1);
    const uint32_t sea = SUI_RGB(0, 60, 46);
    if (my < 0 || my >= kPgH) {
      suiFill(b, X0, y, X1, y + 1, sea);
      continue;
    }

    uint32_t *const fastRow = (X0 >= suiClipX0 && X1 <= suiClipX1 && y >= suiClipY0 && y < suiClipY1 && X0 >= 0 && X1 <= 256 && y >= 0 && y < 192) ? b + y * 256 : nullptr;
    const int tileRow = (my / 8) * 47;
    const int imageRow = my * kPgW;
    const bool countWeather = y >= weatherTop && y < weatherBottom;
    int mx = p.viewX;
    for (int x = X0; x < X1; x += z, ++mx) {
      const int sourceX = z == 1 ? mx : std::clamp(mx, 0, kPgW - 1);
      uint32_t c = sea;
      if (sourceX >= 0 && sourceX < kPgW && !p.hide[tileRow + sourceX / 8]) {
        c = p.img[imageRow + sourceX];
        if (!p.lit[tileRow + sourceX / 8]) c = pgFade(c);
        if (weather) {
          c = suiMix(c, SUI_RGB(19, 50, 79), 1, 5);
          const int cell = pgWeatherMask[imageRow + sourceX];
          if (cell) {
            const int kind = cell & 3;
            c = suiMix(c, kPgWeatherColor[kind], cell >> 2, 7);
            if (countWeather) visibleWeather |= 1u << kind;
          }
        }
      }
      if (fastRow) { fastRow[x] = c; if (z == 2 && x + 1 < X1) fastRow[x + 1] = c; } /* 2026-11-18: no per-pixel clip test when the panel is fully inside the clip */
      else { suiPx(b, x, y, c); if (z == 2 && x + 1 < X1) suiPx(b, x + 1, y, c); }
    }
  }
  pxMapPins(b, p, X0, Y0, X1, oy);
  if (p.find > 0 && pgFindCfg()) pgFindPins(b, p, X0, Y0, X1, oy);
  /* selected location outline, then the player marker */
  if (p.sel >= 0) { int x, y, w, h; pgSpecRect(p.sel, &x, &y, &w, &h); int sx, sy, ex, ey; pgToDsCorner(p, X0, Y0, oy, x * 8, y * 8, &sx, &sy); pgToDsCorner(p, X0, Y0, oy, x * 8 + w * 8, y * 8 + h * 8, &ex, &ey);
    /* two gold rows and a navy rim outside them, so the frame reads on light and dark map art alike */
    auto px = [&](int x, int y, uint32_t c) { if (x >= X0 && x < X1 && y >= Y0 && y < Y1) suiPx(b, x, y, c); };
    for (int k = 0; k < 3; k++) { const uint32_t c = k == 2 ? kBlNavy : SUI_RGB(255, 224, 64);
      for (int xx = sx - 1 - k; xx <= ex + k; xx++) { px(xx, sy - 1 - k, c); px(xx, ey + k, c); }
      for (int yy = sy - 1 - k; yy <= ey + k; yy++) { px(sx - 1 - k, yy, c); px(ex + k, yy, c); } } }
  if (P3DS_PgLoc >= 0) { int x, y, w, h; pgSpecRect(P3DS_PgLoc, &x, &y, &w, &h); int px, py; pgToDs(p, X0, Y0, oy, x * 8 + w * 4, y * 8 + h * 4, &px, &py);
    p.markX = px; p.markY = py; p.markOk = px >= X0 + 3 && px < X1 - 3 && (p.sel < 0 || py < Y1 - pgCardH(p) - 6);
    if (px >= X0 + 3 && px < X1 - 3 && !p.gameMarker) mmPlayer(b, px, py, blink); } else p.markOk = false;
  /* roamers: letter square, or the species icon from the ROM when roamer_sprites is on. Same place, same hide rules. */
  if (P3DS_TrkOn() && P3DS_TrkReady) for (int i = 0; i < 4; i++) { const P3DS_TrkRoamer &r = P3DS_TrkRoam[i]; if (!r.active || r.loc < 0 || r.loc >= kPgN) continue;
    int x, y, w, h; pgSpecRect(r.loc, &x, &y, &w, &h); const int mx = x * 8 + w * 4, my = y * 8 + h * 4; if (pgHiddenAt(p, mx, my)) continue;
    int rx, ry; pgToDs(p, X0, Y0, oy, mx, my, &rx, &ry); const int inset = pgRoamSpr() ? 9 : 5;
    if (rx < X0 + inset || rx >= X1 - inset || ry < Y0 + inset || ry >= Y1 - inset) continue;
    if (!(pgRoamSpr() && pgRoamBlit(b, i, r.species, rx, ry, X0, Y0, X1, Y1))) {
      for (int yy = -4; yy <= 4; yy++) for (int xx = -4; xx <= 4; xx++) suiPx(b, rx + xx, ry + yy, xx == -4 || xx == 4 || yy == -4 || yy == 4 ? SUI_RGB(7, 33, 54) : kTrkCol[i]);
      static const char *const kIni[4] = { "R", "E", "L", "L" }; const char *ini = kIni[i]; T.draw(b, rx - T.width(ini) / 2, ry - 4, ini, SUI_RGB(30, 30, 40), kTrkCol[i]); }
    else { for (int k = 0; k < 3; k++) { suiPx(b, rx - 8 + k, ry - 8, kTrkCol[i]); suiPx(b, rx - 8, ry - 8 + k, kTrkCol[i]); } } }
  /* info card for the tapped location (2026-11-18 polish): green title band with the name, FLY and VISITED; count cells
     with bars; up to two lines of description; then the phone line and a WILD POKEMON button. Tapping anywhere on the
     card still flips it to the wild list (render3ds.cpp), so the button is only the visible way in. */
  p.flyX0 = p.flyY0 = p.flyX1 = p.flyY1 = 0; p.subX0 = p.subX1 = 0;
  if (p.sel >= 0) { const int ch = pgCardH(p), cy0 = Y1 - ch; char ln[4][64];
    for (int y = cy0 + 1; y < Y1; ++y) suiFill(b, X0, y, X1, y + 1, suiMix(kPgPaper, kPgMint, y - cy0, ch));
    suiFill(b, X0, cy0, X1, cy0 + 1, kBlNavy); pgBand(b, X0, cy0 + 1, X1, cy0 + 16);
    const uint32_t sh = kPgPaper, fg = kPgText, wh = SUI_RGB(255, 255, 255);
    const char *name = P3DS_PgName[p.sel][0] ? P3DS_PgName[p.sel] : "?"; char nm[48]; int right = X1 - 4;
    int wl[64], cg = 0; const int wn = pgWild(p.sel, wl, 64, &cg); const bool hasWild = P3DS_PsReady && P3DS_PsStat[p.sel][0];
    if (p.page == 2) { /* sub-areas: two columns, visited ones in green, the rest grey */
      int sn = 0, ss = 0; int idx[160]; for (int i = 0; i < P3DS_SubN && i < 160; i++) if (P3DS_SubLoc[i] == p.sel) idx[sn++] = i; for (int i = 0; i < sn; i++) ss += p.subVis[idx[i] >> 3] >> (idx[i] & 7) & 1;
      char hd[40]; snprintf(hd, sizeof hd, "AREAS %d/%d", ss, sn);
      suiSDraw(T, b, right - suiSWidth(T, hd), cy0 + 4, hd, ss >= sn && sn ? kPgGold : wh, kBlSelO); right -= suiSWidth(T, hd) + 6;
      pgFit(T, nm, sizeof nm, name, right - (X0 + 4)); T.draw(b, X0 + 4, cy0 + 1, nm, wh, kBlSelO);
      if (!sn) T.draw(b, X0 + 4, cy0 + 18, "No sub-areas here.", fg, sh);
      else { const int cw = (aw - 8) / 2, rows = (ch - 18) / 10, slots = rows * 2, shown = sn > slots ? slots - 1 : sn;
        for (int i = 0; i < shown; i++) { const int k = idx[i]; const bool seen = p.subVis[k >> 3] >> (k & 7) & 1; char sn2[24]; pgFit(T, sn2, sizeof sn2, P3DS_SubName[k], cw - 4);
          T.draw(b, X0 + 4 + (i / rows) * cw, cy0 + 16 + (i % rows) * 10, sn2, seen ? kPgDone : kPgDim, sh); }
        if (shown < sn) { char m[16]; snprintf(m, sizeof m, "+%d more", sn - shown); T.draw(b, X0 + 4 + cw, cy0 + 16 + (rows - 1) * 10, m, kPgDim, sh); } } }
    else if (p.page) { /* wild Pokemon list: three columns, caught ones in green */
      char hd[40]; if (wn) snprintf(hd, sizeof hd, "CAUGHT %d/%d", cg, wn); else snprintf(hd, sizeof hd, "WILD");
      if (wn && pgFindCfg()) { char h2[48]; snprintf(h2, sizeof h2, "TAP=FIND  %s", hd); if (suiSWidth(T, h2) + T.width(name) + 14 < aw) snprintf(hd, sizeof hd, "%s", h2); }
      suiSDraw(T, b, right - suiSWidth(T, hd), cy0 + 4, hd, cg >= wn && wn ? kPgGold : wh, kBlSelO); right -= suiSWidth(T, hd) + 6;
      pgFit(T, nm, sizeof nm, name, right - (X0 + 4)); T.draw(b, X0 + 4, cy0 + 1, nm, wh, kBlSelO);
      if (!P3DS_PsReady) T.draw(b, X0 + 4, cy0 + 18, "Take a step to count.", fg, sh);
      else if (!hasWild) T.draw(b, X0 + 4, cy0 + 18, "No wild Pokemon here.", fg, sh);
      else if (!wn) { T.draw(b, X0 + 4, cy0 + 18, "Wild list needs hook 0084,", fg, sh); T.draw(b, X0 + 4, cy0 + 28, "not in this build.", fg, sh); }
      else { int cw, rows, shown; pgWildLayout(aw, ch, wn, &cw, &rows, &shown);
        for (int i = 0; i < shown; i++) { const int sp = wl[i], cx0 = X0 + 4 + (i / rows) * cw, cy = cy0 + 16 + (i % rows) * 10; const bool c = P3DS_PsCg[p.sel][sp >> 3] >> (sp & 7) & 1;
          char sn[16]; snprintf(sn, sizeof sn, "%s", kP3dsSpecies[sp]); int k = (int)strlen(sn); while (k > 1 && T.width(sn) > cw - 3) sn[--k] = 0;
          T.draw(b, cx0, cy, sn, c ? kPgDone : fg, sh); }
        if (shown < wn) { char m[16]; snprintf(m, sizeof m, "+%d more", wn - shown); T.draw(b, X0 + 4 + 2 * cw, cy0 + 16 + (rows - 1) * 10, m, kPgDim, sh); } } }
    else {
      const bool vis = pgVis(p, p.sel); const char *st = vis ? "VISITED" : "NOT YET"; const int sw = suiSWidth(T, st);
      suiSDraw(T, b, right - sw, cy0 + 4, st, vis ? wh : kPgGold, kBlSelO); right -= sw + 6;
      if (pgFlyCfg() && pgFlyUnlocked(p.sel)) { /* hook 0228: FLY chip, green text when Fly works here now, grey otherwise (the tap then says why) */
        const int cw = T.width("FLY") + 10, cx1 = right, cx0 = cx1 - cw, cyy = cy0 + 2;
        if (cx0 > X0 + 40) { const bool rdy = P3DS_FlyHere == 0; pgChip(b, cx0, cyy, cx1, cyy + 12, false);
          T.draw(b, cx0 + 5, cyy, "FLY", rdy ? kBlSelO : SUI_RGB(150, 160, 164), SUI_RGB(244, 247, 254)); p.flyX0 = cx0; p.flyY0 = cyy; p.flyX1 = cx1; p.flyY1 = cyy + 12; right = cx0 - 5; } }
      pgFit(T, nm, sizeof nm, name, right - (X0 + 4)); T.draw(b, X0 + 4, cy0 + 1, nm, wh, kBlSelO);
      int ty = cy0 + 19;
      if (P3DS_PsReady) { pgStatCells(b, T, X0 + 4, ty + 2, X1 - 4, p.sel, sh); ty += 16; }
      /* footer: phone line on the left, WILD POKEMON button on the right */
      const char *wb = "WILD POKEMON"; const int bw = hasWild ? suiSWidth(T, wb) + 12 : 0, by0 = Y1 - 15;
      int sbn = 0, sbs = 0; sbn = pgSubCount(p, p.sel, &sbs); char sbl[24]; snprintf(sbl, sizeof sbl, "AREAS %d/%d", sbs, sbn); const int sbw = sbn ? suiSWidth(T, sbl) + 12 : 0;
      char pcl[64]; const int pcAvail = aw - 10 - (bw ? bw + 6 : 0) - (sbw ? sbw + 6 : 0); const bool hasPc = pcAvail > 30 && pxCardText(T, pcl, sizeof pcl, p.sel, pcAvail);
      const int fl = (hasWild || hasPc) ? (by0 - 1 - ty) / 10 : (Y1 - 2 - ty) / 10;
      int n = 0; if (fl > 0) { char more[4][64]; const int all = pgWrap(T, P3DS_PgText[p.sel], aw - 8, more, fl + 1 > 4 ? 4 : fl + 1); n = all < fl ? all : fl;
        for (int i = 0; i < n; i++) snprintf(ln[i], 64, "%s", more[i]);
        if (all > n && n > 0) { char cut[68]; snprintf(cut, sizeof cut, "%s..", ln[n - 1]); pgFit(T, ln[n - 1], 64, cut, aw - 8); } }
      for (int i = 0; i < n; i++) T.draw(b, X0 + 4, ty + i * 10, ln[i], fg, sh);
      if (hasWild) { const int bx1 = X1 - 4, bx0 = bx1 - bw; pgChip(b, bx0, by0, bx1, by0 + 12, true); suiSDraw(T, b, bx0 + 6, by0 + 2, wb, wh, kBlBlue); }
      if (sbw) { const int sx1 = (hasWild ? X1 - 4 - bw - 4 : X1 - 4), sx0 = sx1 - sbw; pgChip(b, sx0, by0, sx1, by0 + 12, true); suiSDraw(T, b, sx0 + 6, by0 + 2, sbl, wh, kBlBlue); p.subX0 = sx0; p.subY0 = by0; p.subX1 = sx1; p.subY1 = by0 + 12; }
      if (hasPc) suiSDraw(T, b, X0 + 4, by0 + 2, pcl, kBlSelO, sh); } }
  if (p.find > 0 && pgFindCfg() && p.findList) { pgFindListDraw(b, T, p, X0, Y0, X1, Y1); p.markOk = false; p.nIco = 0; }
  if (p.find > 0 && pgFindCfg()) pgFindBanner(b, T, p, X0, Y0, X1);
  /* 2026-11-18 tool row: zoom on the left, WX / achievements / overview on the right, all one chip style, on one line at
     the top of the map (under the finder banner when it is open), so nothing floats over the bottom of the map or the card. */
  if (!finder || !p.findList) { const int ty = finder ? Y0 + kPgFindH : Y0;
    pgZoomChip(b, X0 + 2, ty + 2, false); pgZoomChip(b, X0 + 17, ty + 2, true);
    p.zmX0 = X0 + 2; p.zmY0 = ty + 2; p.zmX1 = X0 + 14; p.zmY1 = ty + 14; p.zpX0 = X0 + 17; p.zpY0 = ty + 2; p.zpX1 = X0 + 29; p.zpY1 = ty + 14;
    pgDots(b, p, X1, ty);
    if (weatherEnabled) pgWeatherButton(b, T, p, p.achX0 - 26, ty + 2, weather);
    /* radar key: one strip along the bottom of the map, above the card */
    if (weather) { const int by = (p.sel >= 0 ? Y1 - pgCardH(p) : Y1) - 13;
      if (by > ty + 16) { if (visibleWeather) pgWeatherLegend(b, T, X0, by, X1, visibleWeather, true); else pgWeatherStatusBar(b, T, X0, by, X1, visibleWeather); } } }
}
static int pgZoomHit(const SPg &p, int x, int y) {
  if (p.zmX1 > p.zmX0 && x < (p.zmX1 + p.zpX0) / 2 && pgIn(x, y, p.zmX0, p.zmY0, p.zmX1, p.zmY1, 3, 4)) return 1;
  if (p.zpX1 > p.zpX0 && x >= (p.zmX1 + p.zpX0) / 2 && pgIn(x, y, p.zpX0, p.zpY0, p.zpX1, p.zpY1, 3, 4)) return 2;
  return 0; }

/* ---- run 11: the game's own icons, decoded from the members above (nothing is written anywhere) ---- */
/* LZ10 (BIOS LZ77, first byte 0x10) into a malloc'd buffer; returns 0 if the data is not LZ10 or broken */
static uint8_t *pgLz10(const uint8_t *s, uint32_t n, uint32_t *outN) {
  if (n < 4 || s[0] != 0x10) return 0; const uint32_t len = s[1] | s[2] << 8 | s[3] << 16; if (!len || len > 256 * 1024) return 0;
  uint8_t *d = (uint8_t *)malloc(len); if (!d) return 0; uint32_t i = 4, o = 0;
  while (o < len && i < n) { const uint8_t f = s[i++];
    for (int b = 0; b < 8 && o < len; b++) {
      if (f & (0x80 >> b)) { if (i + 1 >= n) { free(d); return 0; } const int l = (s[i] >> 4) + 3, dist = ((s[i] & 15) << 8 | s[i + 1]) + 1; i += 2;
        if ((uint32_t)dist > o) { free(d); return 0; } for (int k = 0; k < l && o < len; k++, o++) d[o] = d[o - dist]; }
      else { if (i >= n) { free(d); return 0; } d[o++] = s[i++]; } } }
  *outN = len; return d; }
/* member as plain bytes (LZ10 unpacked); caller frees when *own */
static const uint8_t *pgPlain(int key, uint32_t *n, bool *own) {
  *own = false; if (!pgRaw[key]) return 0; if (pgRaw[key][0] == 0x10) { uint8_t *u = pgLz10(pgRaw[key], pgRawSz[key], n); *own = u != 0; return u; }
  *n = pgRawSz[key]; return pgRaw[key]; }
/* palette bank -> 16 colours as 0x00BBGGRR (index 0 = clear) */
static bool pgPal16(int key, int bank, uint32_t *out) {
  const uint8_t *t = pgSect(pgRaw[key], pgRawSz[key], "TTLP"); if (!t) return false; const uint8_t *P = t + 8 + pgU32(t + 20); const uint32_t pn = pgU32(t + 16) / 2;
  if ((uint32_t)(bank * 16 + 16) > pn) return false;
  for (int i = 0; i < 16; i++) { const uint16_t k = pgU16(P + 2 * (bank * 16 + i)); out[i] = SUI_RGB((k & 31) * 255 / 31, (k >> 5 & 31) * 255 / 31, (k >> 10 & 31) * 255 / 31); } return true; }
/* RGBA icon (alpha 0/255), 0x00BBGGRR | A<<24 */
struct SPgIcon { int w, h; uint32_t px[32 * 32]; bool ok; };
/* a 32x32 OBJ in 1D mapping (tiles 0..15 row-major 4x4) */
static bool pgIcon32(SPgIcon &ic, int chrKey, int palKey, int bank) {
  uint32_t n; bool own; const uint8_t *d = pgPlain(chrKey, &n, &own); uint32_t pal[16]; ic.ok = false;
  if (!d) return false; const uint8_t *c = pgSect(d, n, "RAHC");
  if (c && pgPal16(palKey, bank, pal)) { const uint8_t *C = c + 8 + pgU32(c + 28); const uint32_t cn = pgU32(c + 24);
    if (cn >= 16 * 32 && C + 16 * 32 <= d + n) { ic.w = ic.h = 32;
      for (int t = 0; t < 16; t++) for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) { const int v = C[t * 32 + y * 4 + x / 2] >> (4 * (x & 1)) & 15;
        ic.px[((t / 4) * 8 + y) * 32 + (t % 4) * 8 + x] = v ? pal[v] | 0xFF000000u : 0; }
      ic.ok = true; } }
  if (own) free((void *)d); return ic.ok; }
/* run 20: one NCER cell with each OAM's own 16-colour bank of a 256-colour palette, drawn into a w x h RGBA box with the
   cell origin at (ox, oy); only opaque pixels are written. Used for the touch menu's Sprint Button (a/0/1/4 70/68/7). */
static bool pgCellBanks(uint32_t *out, int w, int h, int ox, int oy, int chrKey, int cellKey, int palKey, int cell) {
  uint32_t n, en; bool own, eown; const uint8_t *d = pgPlain(chrKey, &n, &own), *e = pgPlain(cellKey, &en, &eown); bool ok = false;
  const uint8_t *c = d ? pgSect(d, n, "RAHC") : 0, *k = e ? pgSect(e, en, "KBEC") : 0;
  if (c && k) {
    const uint8_t *C = c + 8 + pgU32(c + 28); const uint32_t cn = pgU32(c + 24); const int nc = pgU16(k + 8), ext = pgU16(k + 10) == 1, csz = ext ? 16 : 8;
    const uint32_t mapping = pgU32(k + 16); const uint8_t *base = k + 8 + pgU32(k + 12), *oam = base + nc * csz;
    static const uint8_t SW[3][4] = { { 8, 16, 32, 64 }, { 16, 32, 32, 64 }, { 8, 8, 16, 32 } }, SH[3][4] = { { 8, 16, 32, 64 }, { 8, 8, 16, 32 }, { 16, 32, 32, 64 } };
    if (cell < nc && oam < e + en) { ok = true;
      const int no = pgU16(base + cell * csz); const uint32_t oo = pgU32(base + cell * csz + 4);
      for (int q = 0; q < no && ok; q++) { const uint8_t *a = oam + oo + q * 6; if (a + 6 > e + en) break;
        const uint16_t a0 = pgU16(a), a1 = pgU16(a + 2), a2 = pgU16(a + 4); int y = a0 & 255; if (y > 127) y -= 256; int x = a1 & 511; if (x > 255) x -= 512;
        const int sh = a0 >> 14, sz = a1 >> 14; if (sh > 2) continue; const int ow = SW[sh][sz], oh = SH[sh][sz], hf = a1 >> 12 & 1, vf = a1 >> 13 & 1;
        const int t0 = (a2 & 1023) << (mapping < 16 ? (mapping & 3) : 0); uint32_t pal[16]; if (!pgPal16(palKey, a2 >> 12, pal)) { ok = false; break; }
        for (int ty = 0; ty < oh / 8; ty++) for (int tx = 0; tx < ow / 8; tx++) { const int t = t0 + ty * (ow / 8) + tx; if ((uint32_t)(t * 32 + 32) > cn) continue;
          for (int yy = 0; yy < 8; yy++) for (int xx = 0; xx < 8; xx++) { const int v = C[t * 32 + yy * 4 + xx / 2] >> (4 * (xx & 1)) & 15; if (!v) continue;
            int lx = tx * 8 + xx, ly = ty * 8 + yy; if (hf) lx = ow - 1 - lx; if (vf) ly = oh - 1 - ly; const int px = ox + x + lx, py = oy + y + ly;
            if (px >= 0 && px < w && py >= 0 && py < h) out[py * w + px] = pal[v] | 0xFF000000u; } } } } }
  if (own) free((void *)d); if (eown) free((void *)e); return ok; }
/* run 20: the Sprint Button cropped to our square RUN slot (31x32: cell x 0..30, y 3..34), off = cells 4+12, on = 5+8.
   Recolours (user-approved mockup): the tab's dark-grey edge, any dark pixel, and the uncovered corner -> the tab orange. */
static bool pgSprint(uint32_t *out, bool on) {
  enum { W = 31, H = 32 }; memset(out, 0, W * H * 4);
  if (!pgCellBanks(out, W, H, 0, -3, 170, 168, 107, on ? 5 : 4) || !pgCellBanks(out, W, H, 0, -3, 170, 168, 107, on ? 8 : 12)) return false;
  const uint32_t OR = SUI_RGB(184, 104, 16) | 0xFF000000u;
  for (int i = 0; i < W * H; i++) { const uint32_t c = out[i]; /* DS 5-bit -> 8-bit as c << 3 (184,104,16 is the tab orange) */
    const int r = ((c & 255) * 31 + 127) / 255 << 3, g = ((c >> 8 & 255) * 31 + 127) / 255 << 3, b = ((c >> 16 & 255) * 31 + 127) / 255 << 3; out[i] = SUI_RGB(r, g, b) | (c & 0xFF000000u);
    if (!(c >> 24) || r + g + b < 140 || (r == 72 && g == 72 && b == 72)) out[i] = OR; }
  return true; }
/* one NCER cell (pgmap player head) into a 32x32 box centred on the cell origin */
static bool pgCell(SPgIcon &ic, int chrKey, int cellKey, int palKey, int cell) {
  uint32_t n, en; bool own, eown; const uint8_t *d = pgPlain(chrKey, &n, &own), *e = pgPlain(cellKey, &en, &eown); uint32_t pal[16]; ic.ok = false;
  const uint8_t *c = d ? pgSect(d, n, "RAHC") : 0, *k = e ? pgSect(e, en, "KBEC") : 0;
  if (c && k && pgPal16(palKey, 0, pal)) {
    const uint8_t *C = c + 8 + pgU32(c + 28); const uint32_t cn = pgU32(c + 24); const int nc = pgU16(k + 8), ext = pgU16(k + 10) == 1, csz = ext ? 16 : 8;
    const uint32_t mapping = pgU32(k + 16); const uint8_t *base = k + 8 + pgU32(k + 12), *oam = base + nc * csz;
    static const uint8_t SW[3][4] = { { 8, 16, 32, 64 }, { 16, 32, 32, 64 }, { 8, 8, 16, 32 } }, SH[3][4] = { { 8, 16, 32, 64 }, { 8, 8, 16, 32 }, { 16, 32, 32, 64 } };
    if (cell < nc && oam < e + en) { ic.w = ic.h = 32; memset(ic.px, 0, sizeof ic.px);
      const int no = pgU16(base + cell * csz); const uint32_t oo = pgU32(base + cell * csz + 4);
      for (int q = 0; q < no; q++) { const uint8_t *a = oam + oo + q * 6; if (a + 6 > e + en) break;
        const uint16_t a0 = pgU16(a), a1 = pgU16(a + 2), a2 = pgU16(a + 4); int y = a0 & 255; if (y > 127) y -= 256; int x = a1 & 511; if (x > 255) x -= 512;
        const int sh = a0 >> 14, sz = a1 >> 14; if (sh > 2) continue; const int w = SW[sh][sz], h = SH[sh][sz]; const int t0 = (a2 & 1023) << (mapping < 16 ? (mapping & 3) : 0);
        for (int ty = 0; ty < h / 8; ty++) for (int tx = 0; tx < w / 8; tx++) { const int t = t0 + ty * (w / 8) + tx; if ((uint32_t)(t * 32 + 32) > cn) continue;
          for (int yy = 0; yy < 8; yy++) for (int xx = 0; xx < 8; xx++) { const int v = C[t * 32 + yy * 4 + xx / 2] >> (4 * (xx & 1)) & 15; const int px = 16 + x + tx * 8 + xx, py = 16 + y + ty * 8 + yy;
            if (v && px >= 0 && px < 32 && py >= 0 && py < 32) ic.px[py * 32 + px] = pal[v] | 0xFF000000u; } } }
      ic.ok = true; } }
  if (own) free((void *)d); if (eown) free((void *)e); return ic.ok; }

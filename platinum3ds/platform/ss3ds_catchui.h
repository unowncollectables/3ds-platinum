/* ss3ds: battle Y screen (wild catch info, trainer foe, foe party, damage check, hunt), reworked 2026-11-19 (Nathan: "the y menu in
   battle needs total reworking, its ugly, it doesn't work well, and it has old text" / "think Ultra Sun and Moon levels of pretty").
   Pure pixel code over the 256x192 bottom picture (no 3DS calls). Every string goes through SUiText, so with the recorder on
   (ss3ds_sharptext.h) the text is the sharp native 320x240 text, the same path as the settings app and the Johto panel. Shapes are
   anti-aliased rounded cards, glossy bars and pill chips so they hold up when the GPU stretches the picture x1.25.
   render3ds.cpp owns input and presenting (the screen is an overlay: the battle keeps running underneath); tools/catch_ui_preview.cpp
   renders every page on a PC.
   Layout: title bar 0..21 (ball icon, title, badge, X) / content 25..168 / tab strip 172..190 (tap, swipe, Left/Right, L/R). */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ss3ds_settings_ui.h"

namespace cc {
#define CC_RGB(r, g, b) SUI_RGB(r, g, b)
static const uint32_t kBg0 = CC_RGB(22, 34, 70), kBg1 = CC_RGB(8, 12, 30), kBar0 = CC_RGB(236, 70, 80), kBar1 = CC_RGB(172, 30, 52), kBarSh = CC_RGB(96, 12, 30), kRule = CC_RGB(252, 214, 104),
  kTx = CC_RGB(246, 250, 255), kSh = CC_RGB(6, 10, 26), kDim = CC_RGB(150, 170, 214), kGold = CC_RGB(252, 220, 100), kRed = CC_RGB(255, 124, 100), kGreen = CC_RGB(110, 236, 140),
  kCard0 = CC_RGB(52, 72, 128), kCard1 = CC_RGB(34, 48, 92), kCardEdge = CC_RGB(122, 150, 214), kTrack = CC_RGB(20, 28, 56), kBlue = CC_RGB(124, 182, 255);
enum { kTabY0 = 172, kTabY1 = 190, kCloseX0 = 229, kCloseX1 = 251 };
enum Page { P_INFO, P_CATCH, P_HUNT, P_DMG, P_FOE, P_PARTY };
static const char *const kPageName[6] = { "INFO", "CATCH", "HUNT", "DAMAGE", "FOE", "PARTY" };

static inline uint32_t mixc(uint32_t a, uint32_t b, int t) { /* t 0..255 of b */
  const int r = ((a & 255) * (255 - t) + (b & 255) * t) / 255, g = (((a >> 8) & 255) * (255 - t) + ((b >> 8) & 255) * t) / 255, bl = (((a >> 16) & 255) * (255 - t) + ((b >> 16) & 255) * t) / 255;
  return CC_RGB(r, g, bl); }
static inline void blend(uint32_t *b, int x, int y, uint32_t c, int a) { if (x < suiClipX0 || y < suiClipY0 || x >= suiClipX1 || y >= suiClipY1 || x < 0 || y < 0 || x >= 256 || y >= 192 || a <= 0) return; uint32_t &d = b[y * 256 + x]; d = a >= 255 ? c : mixc(d, c, a); }
static inline void fill(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t c) { for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) suiPx(b, x, y, c); }
/* rounded rectangle with a vertical gradient c0 -> c1 and anti-aliased corners (radius r), alpha a 0..255 */
static void rrect(uint32_t *b, int x0, int y0, int x1, int y1, int r, uint32_t c0, uint32_t c1, int a = 255) {
  const int h = y1 - y0; if (h <= 0 || x1 <= x0) return; if (r * 2 > h) r = h / 2; if (r * 2 > x1 - x0) r = (x1 - x0) / 2;
  for (int y = y0; y < y1; y++) { const uint32_t c = mixc(c0, c1, h > 1 ? (y - y0) * 255 / (h - 1) : 0);
    for (int x = x0; x < x1; x++) { int cov = 255;
      const int dx = x < x0 + r ? x0 + r - 1 - x : x >= x1 - r ? x - (x1 - r) : -1, dy = y < y0 + r ? y0 + r - 1 - y : y >= y1 - r ? y - (y1 - r) : -1;
      if (dx >= 0 && dy >= 0) { const float d = sqrtf((float)(dx * dx + dy * dy)) + 0.5f - ((float)r - 0.5f); cov = d <= 0 ? 255 : d >= 1 ? 0 : (int)((1 - d) * 255); }
      if (cov > 0) blend(b, x, y, c, cov * a / 255); } } }
/* the standard card: soft drop shadow, light rim, gradient body, top gloss line */
static void card(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t t0 = kCard0, uint32_t t1 = kCard1) {
  rrect(b, x0 + 1, y0 + 2, x1 + 1, y1 + 2, 5, kSh, kSh, 90);
  rrect(b, x0, y0, x1, y1, 5, kCardEdge, mixc(kCardEdge, kCard1, 150));
  rrect(b, x0 + 1, y0 + 1, x1 - 1, y1 - 1, 4, t0, t1);
  for (int x = x0 + 5; x < x1 - 5; x++) blend(b, x, y0 + 1, CC_RGB(255, 255, 255), 38); }
static inline void right(const SUiText &T, uint32_t *b, int xr, int y, const char *t, uint32_t fg, uint32_t sh) { T.draw(b, xr - T.width(t), y, t, fg, sh); }
static inline void centre(const SUiText &T, uint32_t *b, int x0, int x1, int y, const char *t, uint32_t fg, uint32_t sh) { T.draw(b, (x0 + x1) / 2 - T.width(t) / 2, y, t, fg, sh); }
static void upper(char *q) { for (; *q; q++) if (*q >= 'a' && *q <= 'z') *q -= 32; }
/* glossy bar: dark track, gradient fill, light top line; fill colour c */
static void bar(uint32_t *b, int x, int y, int w, int h, int v, int mx, uint32_t c) {
  rrect(b, x - 1, y - 1, x + w + 1, y + h + 1, h / 2 + 1, CC_RGB(4, 8, 22), CC_RGB(4, 8, 22), 200); rrect(b, x, y, x + w, y + h, h / 2, CC_RGB(10, 16, 40), kTrack);
  int f = mx > 0 ? v * w / mx : 0; if (v > 0 && f < h) f = h; if (f > w) f = w; if (f <= 0) return;
  rrect(b, x, y, x + f, y + h, h / 2, mixc(c, CC_RGB(255, 255, 255), 90), mixc(c, CC_RGB(0, 0, 0), 60));
  for (int xx = x + 2; xx < x + f - 2; xx++) blend(b, xx, y + 1, CC_RGB(255, 255, 255), 70); }
static uint32_t hpCol(int hp, int mx) { const int pc = mx > 0 ? hp * 100 / mx : 0; return pc > 50 ? CC_RGB(70, 220, 100) : pc > 20 ? CC_RGB(244, 204, 52) : CC_RGB(238, 68, 60); }
static void hpBar(uint32_t *b, int x, int y, int w, int hp, int mx) { bar(b, x, y, w, 7, hp > 0 ? hp : 0, mx, hpCol(hp, mx)); }
/* small pill chip with text */
static void pillBase(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t c) {
  rrect(b, x0, y0 + 1, x1, y1 + 1, (y1 - y0) / 2, kSh, kSh, 110);
  rrect(b, x0, y0, x1, y1, (y1 - y0) / 2, mixc(c, CC_RGB(255, 255, 255), 80), mixc(c, CC_RGB(0, 0, 0), 70)); }
static void pill(const SUiText &T, uint32_t *b, int x0, int y0, int x1, int y1, uint32_t c, const char *t, uint32_t tc = CC_RGB(255, 255, 255)) {
  pillBase(b, x0, y0, x1, y1, c); centre(T, b, x0, x1, y0 + (y1 - y0 - 10) / 2 - 1, t, tc, mixc(c, CC_RGB(0, 0, 0), 150)); }
/* little arrow / cross glyphs, 7 px wide */
static void arrow(uint32_t *b, int x, int y, bool up, uint32_t c) { for (int k = 0; k < 4; k++) { const int w = up ? k : 3 - k; for (int xx = 3 - w; xx <= 3 + w; xx++) suiPx(b, x + xx, y + k, c); } for (int yy = 4; yy < 7; yy++) for (int xx = 2; xx <= 4; xx++) suiPx(b, x + xx, up ? y + yy : y + yy - 4 + (up ? 0 : 0), c); }
static void triUp(uint32_t *b, int x, int y, uint32_t c) { for (int k = 0; k < 5; k++) for (int xx = 4 - k; xx <= 4 + k; xx++) suiPx(b, x + xx, y + k, c); }
static void triDn(uint32_t *b, int x, int y, uint32_t c) { for (int k = 0; k < 5; k++) for (int xx = k; xx <= 8 - k; xx++) suiPx(b, x + xx, y + k, c); }
static void cross(uint32_t *b, int x, int y, uint32_t c) { for (int k = 0; k < 7; k++) { suiPx(b, x + k, y + k, c); suiPx(b, x + 6 - k, y + k, c); suiPx(b, x + k + 1, y + k, c); suiPx(b, x + 5 - k, y + k, c); } }
static void ball(uint32_t *b, int cx, int cy, int r) { /* tiny Poke Ball icon */
  for (int y = -r; y <= r; y++) for (int x = -r; x <= r; x++) { const float d = sqrtf((float)(x * x + y * y)); if (d > r + 0.5f) continue; const int a = d > r - 0.5f ? 255 : 255;
    uint32_t c = d > r - 1.2f ? CC_RGB(24, 16, 24) : y < -1 ? CC_RGB(240, 60, 60) : y > 1 ? CC_RGB(248, 248, 252) : CC_RGB(24, 16, 24);
    if (d < 2.6f) c = d < 1.6f ? CC_RGB(255, 255, 255) : CC_RGB(24, 16, 24); blend(b, cx + x, cy + y, c, a); } }

/* the frame every page shares (frameT: tab captions given directly, used by the field hunt stats screen in ss3ds_huntui.h) */
static void frameT(uint32_t *b, const SUiText &T, const char *title, const char *badge, uint32_t badgeCol, int nTab, const char *const *tabName, int cur) {
  for (int y = 0; y < 192; y++) { const uint32_t c = suiMix(kBg0, kBg1, y, 191); for (int x = 0; x < 256; x++) b[y * 256 + x] = c; }
  for (int y = 0; y < 21; y++) { const uint32_t c = suiMix(kBar0, kBar1, y, 20); for (int x = 0; x < 256; x++) b[y * 256 + x] = c; }
  for (int x = 0; x < 256; x++) { blend(b, x, 1, CC_RGB(255, 255, 255), 60); suiPx(b, x, 21, kRule); blend(b, x, 22, kSh, 120); blend(b, x, 23, kSh, 50); }
  ball(b, 12, 10, 6); T.draw(b, 24, 1, title, CC_RGB(255, 255, 255), kBarSh);
  rrect(b, kCloseX0, 3, kCloseX1, 18, 7, CC_RGB(255, 120, 120), CC_RGB(150, 24, 40)); cross(b, (kCloseX0 + kCloseX1) / 2 - 3, 7, CC_RGB(255, 255, 255));
  if (badge && *badge) { const int w = T.width(badge) + 14; pill(T, b, kCloseX0 - 6 - w, 3, kCloseX0 - 6, 18, badgeCol, badge, CC_RGB(255, 255, 255)); }
  if (nTab > 0) { const int w = (248 - 3 * (nTab - 1)) / nTab;
    for (int i = 0; i < nTab; i++) { const int x0 = 4 + i * (w + 3), on = i == cur;
      if (on) { rrect(b, x0, kTabY0, x0 + w, kTabY1, 8, CC_RGB(255, 236, 140), CC_RGB(238, 176, 52)); centre(T, b, x0, x0 + w, kTabY0 + 3, tabName[i], CC_RGB(70, 40, 6), CC_RGB(255, 244, 190)); }
      else { rrect(b, x0, kTabY0, x0 + w, kTabY1, 8, CC_RGB(64, 86, 148), CC_RGB(36, 50, 100)); centre(T, b, x0, x0 + w, kTabY0 + 3, tabName[i], kDim, kSh); } } } }
static void frame(uint32_t *b, const SUiText &T, const char *title, const char *badge, uint32_t badgeCol, int nTab, const int *tabs, int cur) {
  const char *nm[6]; for (int i = 0; i < nTab && i < 6; i++) nm[i] = kPageName[tabs[i]]; frameT(b, T, title, badge, badgeCol, nTab > 6 ? 6 : nTab, nm, cur); }
/* touch: -2 = close, 0..n-1 = a tab, -1 = nothing */
static int hit(int nTab, int x, int y) {
  if (y < 24 && x >= kCloseX0 - 6) return -2;
  if (nTab > 0 && y >= kTabY0 - 3 && y < kTabY1 + 3) { const int w = (248 - 3 * (nTab - 1)) / nTab; int i = (x - 4) / (w + 3); if (x >= 4 && i >= 0 && i < nTab) return i; }
  return -1; }

/* ---- types (Gen 4 order: 0 Normal .. 8 Steel, 9 ???, 10 Fire .. 17 Dark) ---- */
static const char *const kTypeName[18] = { "NORMAL", "FIGHT", "FLYING", "POISON", "GROUND", "ROCK", "BUG", "GHOST", "STEEL", "???", "FIRE", "WATER", "GRASS", "ELECTRIC", "PSYCHIC", "ICE", "DRAGON", "DARK" };
static const uint32_t kTypeCol[18] = { CC_RGB(160, 160, 128), CC_RGB(200, 56, 48), CC_RGB(150, 130, 238), CC_RGB(164, 62, 164), CC_RGB(218, 180, 84), CC_RGB(184, 160, 56), CC_RGB(160, 180, 28), CC_RGB(110, 86, 152), CC_RGB(160, 160, 190),
  CC_RGB(96, 150, 134), CC_RGB(240, 128, 48), CC_RGB(100, 144, 240), CC_RGB(112, 196, 72), CC_RGB(246, 204, 36), CC_RGB(248, 88, 136), CC_RGB(122, 204, 204), CC_RGB(112, 56, 248), CC_RGB(112, 88, 72) };
static void typeChip(const SUiText &T, uint32_t *b, int x, int y, int ty, int w = 44) { if (ty < 0 || ty > 17) ty = 9; pill(T, b, x, y, x + w, y + 12, kTypeCol[ty], kTypeName[ty]); }

/* ---- wild pages ---- */
struct Wild { int c[20]; const char *name, *ability, *nature; int met; bool metOn; };
static const char *ivRating(int tot) { return tot >= 151 ? "Outstanding" : tot >= 121 ? "Relatively superior" : tot >= 91 ? "Above average" : "Decent"; }
static const char *statusName(int st) { return (st & 7) ? "Asleep" : (st & 0x20) ? "Frozen" : (st & 0x40) ? "Paralyzed" : (st & 0x10) ? "Burned" : (st & 0x88) ? "Poisoned" : "Healthy"; }
static uint32_t statusCol(int st) { return (st & 0xFF) ? CC_RGB(255, 150, 110) : kGreen; }
static uint32_t ivCol(int v) { return v >= 31 ? CC_RGB(255, 214, 70) : v >= 26 ? CC_RGB(84, 220, 120) : v >= 16 ? CC_RGB(110, 170, 255) : v >= 1 ? CC_RGB(232, 150, 110) : CC_RGB(240, 90, 90); }
static void wildBadge(const Wild &w, const char **t, uint32_t *col) {
  if (w.c[18]) { *t = "SHINY!"; *col = CC_RGB(238, 170, 30); } else if (w.c[16]) { *t = "CAUGHT"; *col = CC_RGB(70, 140, 220); } else { *t = "NEW"; *col = CC_RGB(52, 170, 90); } }
/* the header strip shared by INFO and CATCH: level / nature or catch rate, status, HP bar */
static void wildTop(uint32_t *b, const SUiText &T, const Wild &w, bool catchPg) {
  const int *c = w.c; char l[96]; card(b, 4, 25, 252, 64);
  if (catchPg) { snprintf(l, sizeof l, "Catch rate %d", c[15]); T.draw(b, 12, 28, l, kGold, kSh);
    const int cr = c[15]; right(T, b, 244, 28, cr >= 200 ? "Very easy" : cr >= 120 ? "Easy" : cr >= 60 ? "Medium" : cr >= 30 ? "Hard" : "Very hard", kTx, kSh); }
  else { snprintf(l, sizeof l, "Lv%d  %s", c[2], w.nature); T.draw(b, 12, 28, l, kGold, kSh); right(T, b, 244, 28, statusName(c[5]), statusCol(c[5]), kSh); }
  T.draw(b, 12, 43, "HP", kDim, kSh); hpBar(b, 32, 46, 134, c[3], c[4]);
  if (catchPg) snprintf(l, sizeof l, "%d%%  %s", c[4] ? c[3] * 100 / c[4] : 0, statusName(c[5])); else snprintf(l, sizeof l, "%d / %d", c[3], c[4]);
  right(T, b, 244, 43, l, kTx, kSh); }
static void wildInfo(uint32_t *b, const SUiText &T, const Wild &w, int nTab, const int *tabs, int cur) {
  const int *c = w.c; char l[96]; const char *bt; uint32_t bc; wildBadge(w, &bt, &bc);
  snprintf(l, sizeof l, "WILD %s", w.name); upper(l); frame(b, T, l, bt, bc, nTab, tabs, cur); wildTop(b, T, w, false);
  T.draw(b, 12, 68, "Ability", kDim, kSh); T.draw(b, 60, 68, w.ability, kTx, kSh);
  snprintf(l, sizeof l, "Pokerus %s", c[8] ? ((c[8] & 15) ? "yes" : "cured") : "no"); right(T, b, 246, 68, l, kDim, kSh);
  card(b, 4, 84, 252, 168);
  T.draw(b, 12, 86, "Individual values", kDim, kSh);
  if (w.metOn) { snprintf(l, sizeof l, "Met %d times", w.met); right(T, b, 244, 86, l, kDim, kSh); }
  static const char *const nm[6] = { "HP", "Attack", "Defense", "Sp. Atk", "Sp. Def", "Speed" }; int tot = 0;
  for (int k = 0; k < 6; k++) { const int v = c[9 + k]; tot += v; const int x = 12 + (k % 3) * 80, y = 101 + (k / 3) * 24;
    T.draw(b, x, y, nm[k], kDim, kSh); snprintf(l, sizeof l, "%d", v); right(T, b, x + 70, y, l, ivCol(v), kSh); bar(b, x, y + 15, 70, 4, v, 31, ivCol(v)); }
  snprintf(l, sizeof l, "Total %d / 186", tot); T.draw(b, 12, 148, l, kTx, kSh);
  right(T, b, 244, 148, ivRating(tot), tot >= 121 ? kGold : kDim, kSh); bar(b, 12, 162, 232, 3, tot, 186, tot >= 151 ? CC_RGB(255, 214, 70) : tot >= 121 ? CC_RGB(84, 220, 120) : CC_RGB(110, 170, 255)); }

/* Gen 4 capture chance (percent) for a ball multiplier in tenths; the same formula the old overlay used */
static int catchPct(const int *c, int ball) {
  if (c[4] <= 0) return 0; const int st = c[5]; const double sm = (st & 0x27) ? 2.0 : (st & 0xD8) ? 1.5 : 1.0;
  double a = (double)(c[15] * ball / 10) * (3.0 * c[4] - 2.0 * c[3]) / (3.0 * c[4]); a = (double)(int)a * sm;
  if (a >= 255) return 100; if (a < 1) a = 1;
  double bb = 1048560.0 / sqrt(sqrt(16711680.0 / a)); double p = bb / 65536.0; p = p * p * p * p; if (p > 1) p = 1; return (int)(p * 100 + 0.5); }
static uint32_t pctCol(int p) { return p >= 75 ? CC_RGB(70, 220, 100) : p >= 40 ? CC_RGB(244, 204, 52) : CC_RGB(238, 80, 70); }
static void wildCatch(uint32_t *b, const SUiText &T, const Wild &w, int nTab, const int *tabs, int cur) {
  const int *c = w.c; char l[96]; const char *bt; uint32_t bc; wildBadge(w, &bt, &bc);
  snprintf(l, sizeof l, "WILD %s", w.name); upper(l); frame(b, T, l, bt, bc, nTab, tabs, cur); wildTop(b, T, w, true);
  T.draw(b, 12, 68, "Chance per throw, this turn", kDim, kSh);
  card(b, 4, 84, 252, 168);
  const int tm = c[17] + 10 > 40 ? 40 : c[17] + 10;
  struct Ball { const char *n, *cap; int mult; bool on; } balls[8] = {
    { "Poke Ball", "", 10, true }, { "Great Ball", "", 15, true }, { "Ultra Ball", "", 20, true }, { "Quick Ball", "turn 1", c[17] < 1 ? 40 : 10, c[17] < 1 },
    { "Timer Ball", "late", tm, c[17] > 0 }, { "Nest Ball", "low Lv", c[2] < 30 ? 40 - c[2] : 10, c[2] < 30 }, { "Dusk Ball", "night/cave", 35, true }, { "Net Ball", "Water/Bug", 30, true } };
  for (int k = 0; k < 8; k++) { const int col = k / 4, row = k % 4, x = 12 + col * 122, y = 88 + row * 19; const Ball &bl = balls[k];
    const int p = catchPct(c, bl.mult); const uint32_t nc = bl.on ? kTx : kDim; T.draw(b, x, y, bl.n, nc, kSh);
    snprintf(l, sizeof l, "%d%%", p); right(T, b, x + 108, y, l, bl.on ? pctCol(p) : kDim, kSh);
    bar(b, x, y + 15, 108, 3, p, 100, bl.on ? pctCol(p) : CC_RGB(100, 112, 150)); (void)bl.cap; } }

/* ---- trainer pages ---- */
struct Foe { int f[40]; const char *name, *ability; const char *const *moves; int nMoves; const char *const *spec; int nSpec; const int *moveType; };
static const char *specName(const Foe &o, int s) { return s > 0 && s < o.nSpec ? o.spec[s] : "?"; }
static void foePage(uint32_t *b, const SUiText &T, const Foe &o, int nTab, const int *tabs, int cur) {
  const int *f = o.f; char l[96]; snprintf(l, sizeof l, "TRAINER'S %s", o.name); upper(l);
  frame(b, T, l, f[19] ? "SHINY!" : "", CC_RGB(238, 170, 30), nTab, tabs, cur);
  card(b, 4, 25, 252, 64);
  snprintf(l, sizeof l, "Lv%d", f[5]); T.draw(b, 12, 28, l, kGold, kSh); right(T, b, 244, 28, statusName(f[8]), statusCol(f[8]), kSh);
  T.draw(b, 12, 43, "HP", kDim, kSh); hpBar(b, 32, 46, 134, f[6], f[7]); snprintf(l, sizeof l, "%d / %d", f[6], f[7]); right(T, b, 244, 43, l, kTx, kSh);
  T.draw(b, 12, 68, "Ability", kDim, kSh); T.draw(b, 60, 68, o.ability, kTx, kSh); if (f[10]) right(T, b, 246, 68, "Holds an item", kDim, kSh);
  for (int k = 0; k < 4; k++) { const int x0 = k & 1 ? 130 : 4, y0 = 88 + (k >> 1) * 41, id = f[11 + k]; card(b, x0, y0, x0 + 122, y0 + 37);
    if (id > 0 && id < o.nMoves) { T.draw(b, x0 + 8, y0 + 4, o.moves[id], kTx, kSh); if (o.moveType) typeChip(T, b, x0 + 8, y0 + 21, o.moveType[id], 44);
      snprintf(l, sizeof l, "PP %d", f[15 + k]); right(T, b, x0 + 114, y0 + 20, l, f[15 + k] ? kGold : kRed, kSh); }
    else T.draw(b, x0 + 8, y0 + 4, "-", kDim, kSh); } }
static void partyPage(uint32_t *b, const SUiText &T, const Foe &o, int nTab, const int *tabs, int cur) {
  const int *f = o.f; frame(b, T, "FOE PARTY", "", 0, nTab, tabs, cur); char l[48];
  for (int i = 0; i < 6; i++) { const int y0 = 26 + i * 24, sp = f[20 + 3 * i], hp = f[21 + 3 * i], mx = f[22 + 3 * i]; card(b, 4, y0, 252, y0 + 22);
    if (sp <= 0) { T.draw(b, 12, y0 + 4, "-", kDim, kSh); continue; }
    T.draw(b, 12, y0 + 4, specName(o, sp), hp > 0 ? kTx : kDim, kSh); if (i == f[3]) pill(T, b, 112, y0 + 5, 138, y0 + 17, CC_RGB(230, 150, 40), "OUT");
    hpBar(b, 144, y0 + 8, 62, hp, mx); if (hp <= 0) snprintf(l, sizeof l, "FNT"); else snprintf(l, sizeof l, "%d", hp);
    right(T, b, 246, y0 + 4, l, hp > 0 ? kTx : kRed, kSh); } }

/* ---- damage check ---- */
struct Dmg { int d[32]; const char *foeName; const char *const *moves; int nMoves; int (*mult)(int, int); };
struct DmgRow { int id, ty, lo, hi, pctLo, pctHi, mult; bool valid, noDmg, varies; };
/* the per-move numbers (same rough Gen 4 formula as before: no crits, items, abilities, weather, stages, priority; burn halves physical) */
static DmgRow dmgRow(const Dmg &o, int k) {
  const int *d = o.d; DmgRow r{}; r.id = d[18 + k]; r.ty = d[22 + k]; const int pw = d[26 + k]; r.valid = r.id > 0 && r.id < o.nMoves; if (!r.valid) return r;
  if (pw <= 0 || r.ty >= 18) { r.noDmg = true; return r; } if (pw == 1) { r.varies = true; return r; }
  const bool phys = r.ty < 9; const int A = phys ? d[3] / ((d[6] & 0x10) ? 2 : 1) : d[4]; int D = phys ? d[10] : d[11]; const int t1 = d[14], t2 = d[15];
  int m = o.mult(r.ty, t1) * (t2 < 18 && t2 != t1 ? o.mult(r.ty, t2) : 2); r.mult = m; if (m == 0) return r; if (D < 1) D = 1;
  int base = ((2 * d[2] / 5 + 2) * pw * A / D) / 50 + 2; if (r.ty == d[7] || r.ty == d[8]) base = base * 3 / 2;
  r.hi = base * m / 4; r.lo = r.hi * 85 / 100; const int hp = d[13]; r.pctLo = hp > 0 ? r.lo * 100 / hp : 0; r.pctHi = hp > 0 ? r.hi * 100 / hp : 0; return r; }
/* effectiveness badge for an x4-unit multiplier: 0 immune, 1 = 1/4, 2 = 1/2, 4 = neutral, 8 = x2, 16 = x4 */
static void effBadge(const SUiText &T, uint32_t *b, int xr, int y, int m) {
  const char *t = 0; uint32_t c = 0; int kind = 0;
  if (m == 0) { t = "No effect"; c = CC_RGB(110, 116, 132); kind = 0; } else if (m == 1) { t = "Resist x1/4"; c = CC_RGB(206, 78, 62); kind = -1; } else if (m == 2) { t = "Resist x1/2"; c = CC_RGB(206, 98, 62); kind = -1; }
  else if (m == 8) { t = "Super x2"; c = CC_RGB(48, 168, 84); kind = 1; } else if (m >= 16) { t = "Super x4"; c = CC_RGB(34, 190, 70); kind = 1; } else return;
  const int w = T.width(t) + 22; pillBase(b, xr - w, y, xr, y + 13, c); if (kind > 0) triUp(b, xr - w + 5, y + 4, CC_RGB(255, 255, 255)); else if (kind < 0) triDn(b, xr - w + 5, y + 4, CC_RGB(255, 255, 255)); else cross(b, xr - w + 5, y + 3, CC_RGB(255, 255, 255));
  T.draw(b, xr - w + 16, y + 1, t, CC_RGB(255, 255, 255), mixc(c, CC_RGB(0, 0, 0), 150)); }
static void dmgPage(uint32_t *b, const SUiText &T, const Dmg &o, int nTab, const int *tabs, int cur) {
  const int *d = o.d; char l[96]; snprintf(l, sizeof l, "YOUR MOVES VS %s", o.foeName); upper(l); frame(b, T, l, "", 0, nTab, tabs, cur);
  const int ms = (d[6] & 0x40) ? d[5] / 4 : d[5], fsp = (d[16] & 0x40) ? d[12] / 4 : d[12];
  card(b, 4, 25, 252, 53); snprintf(l, sizeof l, "Speed  you %d   foe %d", ms, fsp); T.draw(b, 12, 28, l, kTx, kSh); T.draw(b, 12, 41, "Rough: no crits, items, abilities, weather", kDim, kSh);
  const char *who = ms > fsp ? "You move first" : ms < fsp ? "Foe moves first" : "Speed tie"; const uint32_t wc = ms > fsp ? CC_RGB(52, 170, 90) : ms < fsp ? CC_RGB(206, 78, 62) : CC_RGB(200, 160, 40);
  pill(T, b, 244 - T.width(who) - 14, 28, 244, 41, wc, who);
  for (int k = 0; k < 4; k++) { const int y0 = 56 + k * 28; card(b, 4, y0, 252, y0 + 26); const DmgRow r = dmgRow(o, k);
    if (!r.valid) { T.draw(b, 12, y0 + 8, "-", kDim, kSh); continue; }
    if (r.ty >= 0 && r.ty < 18) typeChip(T, b, 10, y0 + 3, r.ty, 44); T.draw(b, 60, y0 + 2, o.moves[r.id], kTx, kSh);
    if (r.noDmg) { right(T, b, 244, y0 + 13, "Status move", kDim, kSh); continue; } if (r.varies) { right(T, b, 244, y0 + 13, "Damage varies", kDim, kSh); continue; }
    if (r.mult == 0) { effBadge(T, b, 246, y0 + 2, 0); continue; } effBadge(T, b, 246, y0 + 2, r.mult);
    const int hp = d[13]; const int lo = r.pctLo > 100 ? 100 : r.pctLo, hi = r.pctHi > 100 ? 100 : r.pctHi; const bool ko = r.lo >= hp, mko = !ko && r.hi >= hp;
    const uint32_t c = ko ? CC_RGB(238, 68, 60) : mko ? CC_RGB(244, 150, 52) : CC_RGB(250, 206, 60);
    bar(b, 60, y0 + 17, 118, 5, 0, 100, c); /* track */
    const int x0 = 60 + lo * 118 / 100, x1 = 60 + hi * 118 / 100; /* solid up to the minimum roll, lighter to the maximum roll */
    rrect(b, 60, y0 + 17, 60 + (x0 - 60 > 4 ? x0 - 60 : 4), y0 + 22, 2, mixc(c, CC_RGB(255, 255, 255), 90), mixc(c, CC_RGB(0, 0, 0), 60));
    if (x1 > x0) rrect(b, x0, y0 + 17, x1 > x0 + 3 ? x1 : x0 + 3, y0 + 22, 2, mixc(c, CC_RGB(255, 255, 255), 150), mixc(c, CC_RGB(255, 255, 255), 90), 170);
    snprintf(l, sizeof l, "%d-%d%%", r.pctLo > 999 ? 999 : r.pctLo, r.pctHi > 999 ? 999 : r.pctHi); T.draw(b, 184, y0 + 12, l, kTx, kSh);
    if (ko || mko) right(T, b, 246, y0 + 12, ko ? "KO!" : "may KO", ko ? kRed : CC_RGB(255, 190, 100), kSh); }
}

/* ---- hunt (shiny suite) ---- */
struct Hunt { const char *name; int enc, streak, resets, shinies, secs, start, lastShinyAt, odds, enc50, enc90; double chance; bool cheat; };
static void huntPage(uint32_t *b, const SUiText &T, const Hunt &h, int nTab, const int *tabs, int cur) {
  char l[96]; snprintf(l, sizeof l, "HUNT: %s", h.name); upper(l); frame(b, T, l, "", 0, nTab, tabs, cur);
  card(b, 4, 25, 252, 69);
  snprintf(l, sizeof l, "%d", h.enc); T.draw(b, 12, 28, "Encounters", kDim, kSh); right(T, b, 120, 28, l, kTx, kSh);
  snprintf(l, sizeof l, "%d", h.streak); T.draw(b, 134, 28, "Since shiny", kDim, kSh); right(T, b, 244, 28, l, kGold, kSh);
  snprintf(l, sizeof l, "%d", h.resets); T.draw(b, 12, 46, "Soft resets", kDim, kSh); right(T, b, 120, 46, l, kTx, kSh);
  snprintf(l, sizeof l, "%d", h.shinies); T.draw(b, 134, 46, "Shinies", kDim, kSh); right(T, b, 244, 46, l, kTx, kSh);
  card(b, 4, 74, 252, 168);
  snprintf(l, sizeof l, "Odds 1 in %d per wild encounter", h.odds); T.draw(b, 12, 78, l, kGold, kSh);
  T.draw(b, 12, 96, "Chance of a shiny by now", kDim, kSh); snprintf(l, sizeof l, "%.1f%%", h.chance * 100.0); right(T, b, 244, 96, l, kTx, kSh);
  bar(b, 12, 111, 232, 6, (int)(h.chance * 1000), 1000, CC_RGB(250, 206, 60));
  if (h.shinies) snprintf(l, sizeof l, "Last shiny came after %d encounters", h.lastShinyAt); else snprintf(l, sizeof l, "50%% by %d, 90%% by %d encounters", h.enc50, h.enc90); T.draw(b, 12, 124, l, kTx, kSh);
  if (h.start) snprintf(l, sizeof l, "Time %dh %02dm   Started %04d-%02d-%02d", h.secs / 3600, h.secs / 60 % 60, h.start / 10000, h.start / 100 % 100, h.start % 100); else snprintf(l, sizeof l, "Time %dh %02dm", h.secs / 3600, h.secs / 60 % 60);
  T.draw(b, 12, 142, l, kDim, kSh); if (h.cheat) T.draw(b, 12, 152, "Shiny wild cheat is on.", kRed, kSh); }
} /* namespace cc */

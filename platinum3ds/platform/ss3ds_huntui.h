/* ss3ds: field HUNT STATS screen (ZR in the overworld, hunt_stats=1), reworked 2026-11-19 (Nathan: "my hunt stats page with the
   trigger push needs the same overhaul as the y button in battle ... actually make it useful not kill the game ... same pretty layout").
   Same look as the battle Y screen (ss3ds_catchui.h: cards, glossy bars, pills, tab strip, sharp native text through SUiText).
   Pure pixel code over the 256x192 bottom picture, no 3DS calls: render3ds.cpp gathers the numbers (hunt.txt counters, hunt.c) and
   runs it as an OVERLAY (the game keeps running; it only loses the buttons while the screen is up). tools/catch_ui_preview.cpp
   renders every page on a PC.
   Pages: HUNT (one species: streak, odds, chance so far with 50% / 90% marks, pace; Up/Down = other species; buttons X pin/unpin the
   hunt, Y every species, RESET twice clears it), TOTALS (all species),
   TOP (most met), RECENT (first met, newest first), SHINY (fewest encounters before a shiny). */
#pragma once
#include "ss3ds_catchui.h"

namespace hs {
using namespace cc;
enum { H_HUNT, H_TOTALS, H_TOP, H_RECENT, H_SHINY, H_N };
static const char *const kTab[H_N] = { "HUNT", "TOTALS", "TOP", "RECENT", "SHINY" };
struct View { char name[24]; int sp, enc, streak, resets, shinies, secs, start, lastShinyAt, odds, enc50, enc90, idx, nIdx; double chance; bool cheat, target, pinned, met, all, armed; };
struct Tot { int met, enc, resets, shinies, secs, start, luckAt, longN, curStreak; bool pinned; char luck[24], lng[24], cur[24]; };
struct List { int n, a[6], b[6], c[6]; char name[6][24]; };
struct Data { View v; Tot t; List top, recent, shiny; };

/* 12345 -> "12,345" */
static const char *num(char *o, int n, int v) {
  char t[16]; snprintf(t, sizeof t, "%d", v < 0 ? -v : v); const int l = (int)strlen(t); int k = 0; if (v < 0 && k < n - 1) o[k++] = '-';
  for (int i = 0; i < l && k < n - 1; i++) { if (i && (l - i) % 3 == 0 && k < n - 1) o[k++] = ','; o[k++] = t[i]; } o[k] = 0; return o; }
static const char *hm(char *o, int n, int secs) { if (secs >= 3600) snprintf(o, n, "%dh %02dm", secs / 3600, secs / 60 % 60); else snprintf(o, n, "%dm", secs / 60); return o; }
static const char *date(char *o, int n, int d) { if (d > 0) snprintf(o, n, "%04d-%02d-%02d", d / 10000, d / 100 % 100, d % 100); else snprintf(o, n, "-"); return o; }
/* four-point sparkle for the title bar */
static void star(uint32_t *b, int cx, int cy, int r, uint32_t c) {
  for (int y = -r; y <= r; y++) for (int x = -r; x <= r; x++) { const int ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
    const float d = (float)ax * ay; const float e = (float)(ax + ay) - r; if (e > 0.5f || d > 1.6f * r * 0.5f) continue; blend(b, cx + x, cy + y, c, e > -0.5f ? 140 : 255); } }
static void title(uint32_t *b, const SUiText &T, const char *t, const char *badge, uint32_t badgeCol, int cur) {
  frameT(b, T, t, badge, badgeCol, H_N, kTab, cur);
  for (int y = 2; y < 19; y++) for (int x = 4; x < 21; x++) { const uint32_t c = suiMix(kBar0, kBar1, y, 20); suiPx(b, x, y, c); } /* the ball icon spot */
  star(b, 12, 10, 7, CC_RGB(255, 236, 140)); star(b, 12, 10, 3, CC_RGB(255, 255, 255)); }
/* a small stat tile: caption on top, value below */
static void tile(uint32_t *b, const SUiText &T, int x0, int y0, int x1, int y1, const char *cap, const char *val, uint32_t vc) {
  card(b, x0, y0, x1, y1); T.draw(b, x0 + 7, y0 + 2, cap, kDim, kSh); right(T, b, x1 - 7, y0 + 16, val, vc, kSh); }
static void empty(uint32_t *b, const SUiText &T, const char *a, const char *bb) {
  card(b, 4, 25, 252, 168); centre(T, b, 4, 252, 80, a, kGold, kSh); if (bb) centre(T, b, 4, 252, 98, bb, kDim, kSh); }

/* ---- HUNT: one species ---- */
/* the three buttons along the bottom of the HUNT page (render3ds.cpp hsInput hits them with btnHit) */
enum { kBtnY0 = 151, kBtnY1 = 166 };
static const int kBtnX[3][2] = { { 8, 100 }, { 104, 172 }, { 176, 248 } };
static int btnHit(int x, int y) { if (y < kBtnY0 - 3 || y > kBtnY1 + 3) return -1; for (int i = 0; i < 3; i++) if (x >= kBtnX[i][0] && x < kBtnX[i][1]) return i; return -1; }
static void buttons(uint32_t *b, const SUiText &T, const View &v) {
  if (v.sp) pill(T, b, kBtnX[0][0], kBtnY0, kBtnX[0][1], kBtnY1, v.pinned ? CC_RGB(150, 110, 40) : CC_RGB(52, 160, 90), v.pinned ? "X: UNPIN" : "X: SET HUNT");
  else pill(T, b, kBtnX[0][0], kBtnY0, kBtnX[0][1], kBtnY1, CC_RGB(70, 80, 110), "X: SET HUNT", kDim);
  pill(T, b, kBtnX[1][0], kBtnY0, kBtnX[1][1], kBtnY1, v.all ? CC_RGB(140, 90, 210) : CC_RGB(70, 100, 170), v.all ? "Y: ALL ON" : "Y: ALL");
  if (v.sp && (v.met || v.resets)) pill(T, b, kBtnX[2][0], kBtnY0, kBtnX[2][1], kBtnY1, v.armed ? CC_RGB(230, 50, 50) : CC_RGB(150, 60, 70), v.armed ? "TAP AGAIN" : "RESET");
  else pill(T, b, kBtnX[2][0], kBtnY0, kBtnX[2][1], kBtnY1, CC_RGB(70, 80, 110), "RESET", kDim); }
static void huntPg(uint32_t *b, const SUiText &T, const View &v) {
  char l[96], n1[16], n2[16];
  if (!v.sp) { title(b, T, "HUNT", "", 0, H_HUNT); card(b, 4, 25, 252, 168); centre(T, b, 4, 252, 70, "No hunts recorded yet", kGold, kSh);
    centre(T, b, 4, 252, 88, "Meet a wild Pokemon, or press Y", kDim, kSh); centre(T, b, 4, 252, 102, "to pick any species to hunt.", kDim, kSh); buttons(b, T, v); return; }
  char badge[16]; uint32_t bc;
  if (v.pinned) { snprintf(badge, sizeof badge, "PINNED"); bc = CC_RGB(214, 150, 30); }
  else if (v.target) { snprintf(badge, sizeof badge, "CURRENT"); bc = CC_RGB(52, 170, 90); }
  else if (v.all) { snprintf(badge, sizeof badge, "#%d", v.sp); bc = CC_RGB(140, 90, 210); }
  else { snprintf(badge, sizeof badge, "%d/%d", v.idx, v.nIdx); bc = CC_RGB(70, 120, 210); }
  snprintf(l, sizeof l, "HUNT: %s", v.name); upper(l); title(b, T, l, badge, bc, H_HUNT);
  card(b, 4, 25, 252, 76);
  T.draw(b, 12, 28, "Since the last shiny", kDim, kSh); num(n1, sizeof n1, v.streak); right(T, b, 244, 28, n1, kGold, kSh);
  T.draw(b, 12, 44, "Met", kDim, kSh); right(T, b, 80, 44, num(n1, sizeof n1, v.enc), kTx, kSh);
  T.draw(b, 92, 44, "Resets", kDim, kSh); right(T, b, 162, 44, num(n1, sizeof n1, v.resets), kTx, kSh);
  T.draw(b, 174, 44, "Shiny", kDim, kSh); right(T, b, 244, 44, num(n1, sizeof n1, v.shinies), v.shinies ? kGold : kTx, kSh);
  snprintf(l, sizeof l, "Odds 1 in %s", num(n1, sizeof n1, v.odds)); T.draw(b, 12, 60, l, kBlue, kSh);
  if (v.start) { snprintf(l, sizeof l, "since %s", date(n2, sizeof n2, v.start)); right(T, b, 244, 60, l, kDim, kSh); }
  else if (!v.met) right(T, b, 244, 60, "not met yet", kDim, kSh);
  card(b, 4, 80, 252, 170);
  T.draw(b, 12, 83, "Chance of a shiny by now", kDim, kSh); snprintf(l, sizeof l, "%.1f%%", v.chance * 100.0); right(T, b, 244, 83, l, kTx, kSh);
  bar(b, 12, 99, 232, 7, (int)(v.chance * 1000), 1000, v.chance >= 0.9 ? CC_RGB(238, 110, 60) : v.chance >= 0.5 ? CC_RGB(244, 170, 52) : CC_RGB(250, 206, 60));
  for (int m = 0; m < 2; m++) { const int x = 12 + (m ? 232 * 9 / 10 : 232 / 2); for (int y = 97; y < 108; y++) blend(b, x, y, CC_RGB(255, 255, 255), y == 97 || y == 107 ? 120 : 200); }
  for (int m = 0; m < 2; m++) { const int at = m ? v.enc90 : v.enc50, y = 110 + m * 13; snprintf(l, sizeof l, "%d%% mark", m ? 90 : 50); T.draw(b, 12, y, l, kDim, kSh);
    num(n1, sizeof n1, at); if (v.streak >= at) snprintf(l, sizeof l, "%s  passed", n1); else snprintf(l, sizeof l, "%s  (%s to go)", n1, num(n2, sizeof n2, at - v.streak));
    right(T, b, 244, y, l, v.streak >= at ? CC_RGB(255, 170, 120) : kTx, kSh); }
  if (v.cheat) T.draw(b, 12, 136, "Shiny wild cheat is on", kRed, kSh);
  else if (v.secs >= 300 && v.enc > 0) { const int ph = (int)((long long)v.enc * 3600 / v.secs); snprintf(l, sizeof l, "Pace %d an hour", ph); T.draw(b, 12, 136, l, kDim, kSh);
    if (ph > 0 && v.streak < v.enc50) { const int s = (int)((long long)(v.enc50 - v.streak) * 3600 / ph); snprintf(l, sizeof l, "~%s to 50%%", hm(n1, sizeof n1, s)); right(T, b, 244, 136, l, kTx, kSh); }
    else { snprintf(l, sizeof l, "Hunted %s", hm(n1, sizeof n1, v.secs)); right(T, b, 244, 136, l, kTx, kSh); } }
  else if (v.lastShinyAt) { snprintf(l, sizeof l, "Last shiny came after %s", num(n1, sizeof n1, v.lastShinyAt)); T.draw(b, 12, 136, l, kGold, kSh); }
  else { snprintf(l, sizeof l, "Hunted %s", hm(n1, sizeof n1, v.secs)); T.draw(b, 12, 136, l, kDim, kSh); right(T, b, 244, 136, v.all ? "Up/Down: any species" : "Up/Down: species", kDim, kSh); }
  buttons(b, T, v); }

/* ---- TOTALS: every species ---- */
static void totalsPg(uint32_t *b, const SUiText &T, const Tot &t) {
  char n2[16], l[96]; title(b, T, "HUNT STATS", "", 0, H_TOTALS);
  static const char *const cap[6] = { "Species met", "Encounters", "Shinies", "Soft resets", "Hunt time", "Met per shiny" };
  const uint32_t vc[6] = { kTx, kTx, kGold, kTx, kTx, kBlue };
  char v[6][16]; num(v[0], 16, t.met); num(v[1], 16, t.enc); num(v[2], 16, t.shinies); num(v[3], 16, t.resets); hm(v[4], 16, t.secs);
  if (t.shinies) num(v[5], 16, t.enc / t.shinies); else snprintf(v[5], 16, "-");
  for (int i = 0; i < 6; i++) { const int x0 = 4 + (i % 3) * 83, y0 = 25 + (i / 3) * 35; tile(b, T, x0, y0, x0 + 80, y0 + 32, cap[i], v[i], vc[i]); }
  card(b, 4, 96, 252, 168);
  T.draw(b, 12, 99, "Luckiest shiny", kDim, kSh);
  if (t.luck[0]) { snprintf(l, sizeof l, "%s after %s", t.luck, num(n2, sizeof n2, t.luckAt)); right(T, b, 244, 99, l, kGold, kSh); } else right(T, b, 244, 99, "none yet", kDim, kSh);
  T.draw(b, 12, 116, "Longest dry run", kDim, kSh);
  if (t.lng[0]) { snprintf(l, sizeof l, "%s, %s", t.lng, num(n2, sizeof n2, t.longN)); right(T, b, 244, 116, l, kTx, kSh); } else right(T, b, 244, 116, "-", kDim, kSh);
  T.draw(b, 12, 133, t.pinned ? "Pinned hunt" : "Current hunt", kDim, kSh);
  if (t.cur[0]) { snprintf(l, sizeof l, "%s, %s since shiny", t.cur, num(n2, sizeof n2, t.curStreak)); right(T, b, 244, 133, l, kTx, kSh); } else right(T, b, 244, 133, "-", kDim, kSh);
  T.draw(b, 12, 150, "Hunting since", kDim, kSh); right(T, b, 244, 150, date(n2, sizeof n2, t.start), kTx, kSh); }

/* ---- lists ---- */
static void rowCard(uint32_t *b, int i) { const int y0 = 26 + i * 24; card(b, 4, y0, 252, y0 + 22); }
static void topPg(uint32_t *b, const SUiText &T, const List &L) {
  char n1[16]; title(b, T, "MOST MET", "", 0, H_TOP);
  if (!L.n) { empty(b, T, "No wild encounters yet", nullptr); return; }
  const int mx = L.a[0] > 0 ? L.a[0] : 1;
  for (int i = 0; i < L.n; i++) { const int y0 = 26 + i * 24; rowCard(b, i);
    snprintf(n1, sizeof n1, "%d", i + 1); pill(T, b, 9, y0 + 5, 25, y0 + 17, i == 0 ? CC_RGB(230, 160, 40) : CC_RGB(70, 100, 170), n1);
    T.draw(b, 31, y0 + 4, L.name[i], kTx, kSh); bar(b, 112, y0 + 9, 62, 5, L.a[i], mx, CC_RGB(110, 170, 255));
    right(T, b, 214, y0 + 4, num(n1, sizeof n1, L.a[i]), kTx, kSh);
    if (L.b[i]) { snprintf(n1, sizeof n1, "x%d", L.b[i]); pill(T, b, 220, y0 + 5, 247, y0 + 17, CC_RGB(238, 170, 30), n1); } else centre(T, b, 220, 247, y0 + 4, "-", kDim, kSh); } }
static void recentPg(uint32_t *b, const SUiText &T, const List &L) {
  char n1[16], n2[24]; title(b, T, "FIRST MET, NEWEST", "", 0, H_RECENT);
  if (!L.n) { empty(b, T, "No hunts recorded yet", nullptr); return; }
  for (int i = 0; i < L.n; i++) { const int y0 = 26 + i * 24; rowCard(b, i);
    T.draw(b, 12, y0 + 4, date(n2, sizeof n2, L.a[i]), kDim, kSh); T.draw(b, 88, y0 + 4, L.name[i], kTx, kSh);
    snprintf(n2, sizeof n2, "%s met", num(n1, sizeof n1, L.b[i])); right(T, b, 244, y0 + 4, n2, L.c[i] ? kGold : kTx, kSh); } }
static void shinyPg(uint32_t *b, const SUiText &T, const List &L) {
  char n1[16], l[32]; title(b, T, "LUCKIEST SHINIES", "", 0, H_SHINY);
  if (!L.n) { empty(b, T, "No shiny yet. Keep hunting!", "Shinies show here, fewest encounters first."); return; }
  for (int i = 0; i < L.n; i++) { const int y0 = 26 + i * 24; rowCard(b, i);
    star(b, 15, y0 + 11, 5, CC_RGB(255, 220, 90)); T.draw(b, 26, y0 + 4, L.name[i], kTx, kSh);
    snprintf(l, sizeof l, "after %s", num(n1, sizeof n1, L.a[i])); right(T, b, 208, y0 + 4, l, kGold, kSh);
    snprintf(n1, sizeof n1, "x%d", L.b[i]); pill(T, b, 216, y0 + 5, 247, y0 + 17, CC_RGB(238, 170, 30), n1); } }

static void page(uint32_t *b, const SUiText &T, const Data &D, int pg) {
  switch (pg) { case H_HUNT: huntPg(b, T, D.v); break; case H_TOTALS: totalsPg(b, T, D.t); break; case H_TOP: topPg(b, T, D.top); break;
    case H_RECENT: recentPg(b, T, D.recent); break; default: shinyPg(b, T, D.shiny); } }
} /* namespace hs */

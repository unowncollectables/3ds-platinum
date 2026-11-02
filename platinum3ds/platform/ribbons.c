/* ribbon_check=1 (default off; settings app Battle tab "Ribbon checklist"): read-only ribbon checklist for the Pokemon on the
 * summary screen. Port overlay only: it never writes to the Pokemon or the save and cannot grant, remove or edit a ribbon.
 *
 * Data: the summary screen's gather function ends with ReleaseMonLock(mon, lock) while the Pokemon is decrypted (pspoke
 * patch 0051 retargets it to SS3DS_SummaryRelease in ss3ds_glue.c). There P3DS_RibbonCapture asks the game's own
 * GetMonData for every ribbon id (the same call the summary screen's ribbon page uses), so no bit position is guessed.
 * Ids are pret/pokeheartgold include/constants/pokemon.h MON_DATA_*_RIBBON: 25..52 (ribbonsDS1: Sinnoh Champ, Battle Tower
 * "Ability" set, daily set, Footprint, Legend, event ribbons), 78..109 (ribbonsGBA: Hoenn contest / Battle Tower / Effort / commemorative) and
 * 123..142 (ribbonsDS2: Sinnoh Super Contest, 5 categories x 4 ranks). 53 / 143 are unused slots.
 *
 * Access: press X (without L / R) on the summary screen. X again turns the page: Earned in this game -> still Available
 * -> From earlier games -> closed. The summary screen's Y IV/EV box is untouched. Leaving the summary closes the panel.
 * "Obtainable in SoulSilver" (verified list, 15): the 7 Week Sibling dailies (Smile, Alert, Shock, Downcast, Careless, Relax,
 * Snooze), Effort, Legend, and the 6 Battle Tower ribbons (Ability, Great Ability, Double Ability, Multi Ability, Pair Ability,
 * World Ability). World Ability needs the Wi-Fi Battle Room, which Nintendo shut down, so it is shown as "no longer obtainable"
 * and never counted as missing. HGSS has no contests, so no contest ribbon is obtainable; they show under earlier games.
 * Log: [RIBBON] lines (first capture per summary open, page changes). */
#include <stdio.h>
#include <string.h>
#include "ribbons.h"

extern void PSPNativeMemLog(const char *, ...);
extern int P3DS_CfgInt(const char *, int);
extern void P3DS_CfgWrite(const char *, int);
extern int P3DS_GameBoxReady(void);
extern void P3DS_GameBoxDraw(unsigned *img, int stride, int cw, int ch, int x, int y, int w, int h);
extern int P3DS_GameFontDraw(unsigned *img, int stride, int cw, int ch, int x, int y, const char *t, unsigned fg, unsigned sh, int scale, int font);
extern volatile int P3DS_SumOpen;

enum { G_NOW = 0, G_OLD = 1, G_GONE = 2 }; /* obtainable in SoulSilver / only from earlier games / obtainable but no longer possible */
typedef struct { short id; unsigned char grp; const char *name; } Rib;
static const Rib kRib[] = {
  /* obtainable in SoulSilver */
  { 38, G_NOW, "Smile" }, { 32, G_NOW, "Alert" }, { 33, G_NOW, "Shock" }, { 34, G_NOW, "Downcast" }, { 35, G_NOW, "Careless" },
  { 36, G_NOW, "Relax" }, { 37, G_NOW, "Snooze" }, { 102, G_NOW, "Effort" }, { 45, G_NOW, "Legend" },
  { 26, G_NOW, "Ability" }, { 27, G_NOW, "Great Ability" }, { 28, G_NOW, "Double Ability" }, { 29, G_NOW, "Multi Ability" }, { 30, G_NOW, "Pair Ability" },
  { 31, G_GONE, "World Ability" },
  /* earlier games (Platinum / Diamond / Pearl, Hoenn, events) */
  { 25, G_OLD, "Sinnoh Champ" }, { 42, G_OLD, "Footprint" },
  { 39, G_OLD, "Gorgeous" }, { 40, G_OLD, "Royal" }, { 41, G_OLD, "Gorgeous Royal" },
  { 43, G_OLD, "Record" }, { 44, G_OLD, "History" }, { 46, G_OLD, "Red" }, { 47, G_OLD, "Green" }, { 48, G_OLD, "Blue" },
  { 49, G_OLD, "Festival" }, { 50, G_OLD, "Carnival" }, { 51, G_OLD, "Classic" }, { 52, G_OLD, "Premier" },
  { 78, G_OLD, "Cool" }, { 79, G_OLD, "Cool Super" }, { 80, G_OLD, "Cool Hyper" }, { 81, G_OLD, "Cool Master" },
  { 82, G_OLD, "Beauty" }, { 83, G_OLD, "Beauty Super" }, { 84, G_OLD, "Beauty Hyper" }, { 85, G_OLD, "Beauty Master" },
  { 86, G_OLD, "Cute" }, { 87, G_OLD, "Cute Super" }, { 88, G_OLD, "Cute Hyper" }, { 89, G_OLD, "Cute Master" },
  { 90, G_OLD, "Smart" }, { 91, G_OLD, "Smart Super" }, { 92, G_OLD, "Smart Hyper" }, { 93, G_OLD, "Smart Master" },
  { 94, G_OLD, "Tough" }, { 95, G_OLD, "Tough Super" }, { 96, G_OLD, "Tough Hyper" }, { 97, G_OLD, "Tough Master" },
  { 98, G_OLD, "Champion" }, { 99, G_OLD, "Winning" }, { 100, G_OLD, "Victory" }, { 101, G_OLD, "Artist" },
  { 103, G_OLD, "Marine" }, { 104, G_OLD, "Land" }, { 105, G_OLD, "Sky" }, { 106, G_OLD, "Country" },
  { 107, G_OLD, "National" }, { 108, G_OLD, "Earth" }, { 109, G_OLD, "World" },
  { 123, G_OLD, "S.Cool" }, { 124, G_OLD, "S.Cool Great" }, { 125, G_OLD, "S.Cool Ultra" }, { 126, G_OLD, "S.Cool Master" },
  { 127, G_OLD, "S.Beauty" }, { 128, G_OLD, "S.Beauty Great" }, { 129, G_OLD, "S.Beauty Ultra" }, { 130, G_OLD, "S.Beauty Master" },
  { 131, G_OLD, "S.Cute" }, { 132, G_OLD, "S.Cute Great" }, { 133, G_OLD, "S.Cute Ultra" }, { 134, G_OLD, "S.Cute Master" },
  { 135, G_OLD, "S.Smart" }, { 136, G_OLD, "S.Smart Great" }, { 137, G_OLD, "S.Smart Ultra" }, { 138, G_OLD, "S.Smart Master" },
  { 139, G_OLD, "S.Tough" }, { 140, G_OLD, "S.Tough Great" }, { 141, G_OLD, "S.Tough Ultra" }, { 142, G_OLD, "S.Tough Master" },
};
#define NRIB ((int)(sizeof kRib / sizeof kRib[0]))

static volatile unsigned have[5];      /* bit (id & 31) of word (id >> 5): the Pokemon holds this ribbon */
static volatile int haveValid;
static int onV = -1;
static int sec = -1, page;             /* -1 = closed */

int P3DS_RibbonOn(void) { if (onV < 0) { onV = P3DS_CfgInt("ribbon_check", 0) != 0; PSPNativeMemLog("[RIBBON] ribbon_check=%d (X on the summary screen; read-only)", onV); } return onV; }
void P3DS_RibbonSetOn(int v) { onV = v != 0; if (!onV) sec = -1; P3DS_CfgWrite("ribbon_check", onV); PSPNativeMemLog("[RIBBON] ribbon_check=%d from settings", onV); }

void P3DS_RibbonCapture(unsigned mon, unsigned (*get)(unsigned, int, void *)) {
  unsigned w[5] = { 0, 0, 0, 0, 0 }; int n = 0, nNow = 0;
  for (int i = 0; i < NRIB; i++) if (get(mon, kRib[i].id, 0)) { w[kRib[i].id >> 5] |= 1u << (kRib[i].id & 31); n++; if (kRib[i].grp != G_OLD) nNow++; }
  for (int k = 0; k < 5; k++) have[k] = w[k];
  haveValid = 1;
  PSPNativeMemLog("[RIBBON] capture: %d held (%d from this game's list, %d from earlier games)", n, nNow, n - nNow);
}
static int held(int id) { return (have[id >> 5] >> (id & 31)) & 1; }

/* one frame of input (call once per frame with the keys held): X turns the page, leaving the summary closes the panel */
void P3DS_RibbonFrame(unsigned held_keys) {
  static unsigned prev; unsigned kd = held_keys & ~prev; prev = held_keys;
  if (!onV || !P3DS_SumOpen || !haveValid) { sec = -1; return; }
  if (!(kd & RIBBON_KEY_X) || (held_keys & (RIBBON_KEY_L | RIBBON_KEY_R))) return;
  /* pages per section */
  int cnt[3] = { 0, 0, 0 };
  for (int i = 0; i < NRIB; i++) { int h = held(kRib[i].id); if (kRib[i].grp == G_OLD) cnt[2] += h; else if (h) cnt[0]++; else cnt[1]++; }
  if (sec < 0) { sec = 0; page = 0; }
  else { int pages = (cnt[sec] + 13) / 14; if (pages < 1) pages = 1; if (++page >= pages) { page = 0; sec++; } if (sec > 2) sec = -1; }
  PSPNativeMemLog("[RIBBON] checklist opened/page: section %d page %d", sec, page);
}

enum { FG = 0x00505858, SH = 0x00B8B8A8, OKC = 0x00206020, NOC = 0x00404090 };
void P3DS_RibbonDraw(unsigned *img) {
  if (sec < 0 || !onV || !P3DS_SumOpen || !haveValid || !P3DS_GameBoxReady()) return;
  const char *names[RIBBON_MAX]; int n = 0, nowTot = 0, nowHave = 0, gone = 0; unsigned char mark[RIBBON_MAX];
  for (int i = 0; i < NRIB; i++) { const Rib *r = &kRib[i]; int h = held(r->id);
    if (r->grp != G_OLD && (h || r->grp == G_NOW)) { nowTot++; nowHave += h; }
    if (r->grp == G_GONE && !h) gone++;
    int want = sec == 0 ? (r->grp != G_OLD && h) : sec == 1 ? (r->grp != G_OLD && !h) : (r->grp == G_OLD && h);
    if (want && n < RIBBON_MAX) { names[n] = r->name; mark[n] = (unsigned char)(r->grp == G_GONE && !h); n++; } }
  int pages = (n + 13) / 14; if (pages < 1) pages = 1; if (page >= pages) page = 0;
  char head[48], foot[48];
  if (sec == 0) snprintf(head, sizeof head, "Earned in this game  %d/%d", nowHave, nowTot);
  else if (sec == 1) snprintf(head, sizeof head, "Still available  %d", n - gone);
  else snprintf(head, sizeof head, "From earlier games  %d", n);
  snprintf(foot, sizeof foot, gone && sec == 1 ? "X: next  %d/%d  *Wi-Fi ended" : "X: next   %d/%d", page + 1, pages);
  P3DS_GameBoxDraw(img, 256, 256, 192, 24, 16, 192, 152);
  P3DS_GameFontDraw(img, 256, 256, 192, 26, 16, head, FG, SH, 1, 1);
  if (n == 0) P3DS_GameFontDraw(img, 256, 256, 192, 26, 40, sec == 1 ? "Nothing left!" : "None yet", FG, SH, 1, 1);
  for (int k = 0; k < 14; k++) { int idx = page * 14 + k; if (idx >= n) break; int col = k / 7, row = k % 7; char b[40];
    if (mark[idx]) snprintf(b, sizeof b, "%s*", names[idx]); else snprintf(b, sizeof b, "%s", names[idx]);
    P3DS_GameFontDraw(img, 256, 256, 192, 26 + col * 98, 32 + row * 16, b, mark[idx] ? NOC : FG, SH, 1, 1); }
  P3DS_GameFontDraw(img, 256, 256, 192, 26, 152, foot, FG, SH, 1, 1);
}

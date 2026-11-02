/* Achievements: see achievements.h. File sdmc:/3ds/platinum3ds/achievements.txt, one "key=yyyymmdd" line per unlocked
 * achievement plus "carried=1" once the first save scan has run. Log: [ACH] lines (unlock, carry-over, file writes). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#ifndef ACH_HOST_TEST
#include <3ds.h>
#endif
#include "achievements.h"

extern void PSPNativeMemLog(const char *, ...);
extern int P3DS_CfgInt(const char *, int);
extern void P3DS_CfgWrite(const char *, int);
extern void P3DS_HuntChime(void);
#ifndef ACH_DIR
#define ACH_DIR "sdmc:/3ds/platinum3ds/"
#endif

static const struct { const char *key; P3DS_AchInfo i; } kAch[ACH_COUNT] = {
  { "zephyr", { "Zephyr Badge", "Beat Falkner of Violet City.", 20, 0 } }, { "hive", { "Hive Badge", "Beat Bugsy of Azalea Town.", 20, 0 } },
  { "plain", { "Plain Badge", "Beat Whitney of Goldenrod City.", 20, 0 } }, { "fog", { "Fog Badge", "Beat Morty of Ecruteak City.", 20, 0 } },
  { "storm", { "Storm Badge", "Beat Chuck of Cianwood City.", 20, 0 } }, { "mineral", { "Mineral Badge", "Beat Jasmine of Olivine City.", 20, 0 } },
  { "glacier", { "Glacier Badge", "Beat Pryce of Mahogany Town.", 20, 0 } }, { "rising", { "Rising Badge", "Beat Clair of Blackthorn City.", 20, 0 } },
  { "boulder", { "Boulder Badge", "Beat Brock of Pewter City.", 30, 0 } }, { "cascade", { "Cascade Badge", "Beat Misty of Cerulean City.", 30, 0 } },
  { "thunder", { "Thunder Badge", "Beat Lt. Surge of Vermilion City.", 30, 0 } }, { "rainbow", { "Rainbow Badge", "Beat Erika of Celadon City.", 30, 0 } },
  { "soul", { "Soul Badge", "Beat Janine of Fuchsia City.", 30, 0 } }, { "marsh", { "Marsh Badge", "Beat Sabrina of Saffron City.", 30, 0 } },
  { "volcano", { "Volcano Badge", "Beat Blaine of Cinnabar Island.", 30, 0 } }, { "earth", { "Earth Badge", "Beat Blue of Viridian City.", 30, 0 } },
  { "champion", { "Johto League Champion", "Enter the Hall of Fame.", 100, 1 } }, { "red", { "Mt. Silver Showdown", "Defeat Red on Mt. Silver.", 200, 1 } },
  { "trade", { "First Trade", "Complete a link trade.", 25, 1 } }, { "shiny", { "First Shiny", "Meet a shiny Pokemon.", 100, 2 } },
  { "bday", { "Happy Birthday!", "See the birthday sparkle at Sprout Tower.", 25, 3 } }, { "newyear", { "New Year's", "Watch the New Year fireworks.", 25, 3 } },
  { "valentine", { "Valentine's", "See the Valentine's hearts.", 25, 3 } }, { "stpat", { "St. Patrick's", "See the St. Patrick's sparkles.", 25, 3 } },
  { "canada", { "Canada Day", "See the Canada Day sparkles.", 25, 3 } }, { "halloween", { "Halloween", "See the Halloween wisps.", 25, 3 } },
  { "xmas", { "Christmas", "See the Christmas snow.", 25, 3 } } };
static const char *const kCat[ACH_CATS] = { "Gyms", "Milestones", "Shiny", "Special Dates" };

#ifdef ACH_HOST_TEST
#define LOCK()
#define UNLOCK()
static unsigned long long nowTick(void) { return (unsigned long long)clock(); }
#define TICKS_MS (CLOCKS_PER_SEC / 1000)
#else
static LightLock lk; static int lkInit;
#define LOCK() do { if (!lkInit) { LightLock_Init(&lk); lkInit = 1; } LightLock_Lock(&lk); } while (0)
#define UNLOCK() LightLock_Unlock(&lk)
static unsigned long long nowTick(void) { return svcGetSystemTick(); }
#define TICKS_MS CPU_TICKS_PER_MSEC
#endif

static int date[ACH_COUNT], carried, loaded, unviewed, onV = -1, bootScan = 1;
enum { QN = 12 };
static int q[QN], qn;                  /* queued toasts: achievement id, or -(n) for "n carried over" */
static int cur = -1000; static unsigned long long curAt;

static int today(void) { time_t t = time(NULL); struct tm *lt = localtime(&t); return lt ? (lt->tm_year + 1900) * 10000 + (lt->tm_mon + 1) * 100 + lt->tm_mday : 1; }
static void save(void) {
  mkdir(ACH_DIR, 0777); FILE *f = fopen(ACH_DIR "achievements.tmp", "w"); if (!f) { PSPNativeMemLog("[ACH] cannot write achievements.tmp"); return; }
  fprintf(f, "carried=%d\n", carried); for (int i = 0; i < ACH_COUNT; i++) if (date[i]) fprintf(f, "%s=%d\n", kAch[i].key, date[i]);
  fclose(f); remove(ACH_DIR "achievements.txt"); rename(ACH_DIR "achievements.tmp", ACH_DIR "achievements.txt"); }
static void load(void) {
  if (loaded) return;
  loaded = 1;
  FILE *f = fopen(ACH_DIR "achievements.txt", "r");
  if (!f) f = fopen(ACH_DIR "achievements.tmp", "r");
  if (!f) { PSPNativeMemLog("[ACH] no achievements.txt yet"); return; }
  char ln[64]; int n = 0;
  while (fgets(ln, sizeof ln, f)) { char *e = strchr(ln, '='); if (!e) continue; *e = 0; const int v = atoi(e + 1);
    if (!strcmp(ln, "carried")) { carried = v != 0; continue; }
    for (int i = 0; i < ACH_COUNT; i++) if (!strcmp(ln, kAch[i].key)) { date[i] = v ? v : 1; n++; } }
  fclose(f); PSPNativeMemLog("[ACH] loaded %d unlocked (carried=%d)", n, carried); }

const P3DS_AchInfo *P3DS_AchGet(int id) { return id >= 0 && id < ACH_COUNT ? &kAch[id].i : &kAch[0].i; }
const char *P3DS_AchCatName(int cat) { return cat >= 0 && cat < ACH_CATS ? kCat[cat] : ""; }
int P3DS_AchDate(int id) { LOCK(); load(); const int d = id >= 0 && id < ACH_COUNT ? date[id] : 0; UNLOCK(); return d; }
int P3DS_AchTotal(int cat, int *max) {
  LOCK(); load(); int s = 0, m = 0;
  for (int i = 0; i < ACH_COUNT; i++) if (cat < 0 || kAch[i].i.cat == cat) { m += kAch[i].i.pts; if (date[i]) s += kAch[i].i.pts; }
  UNLOCK(); if (max) *max = m; return s; }
int P3DS_AchToastsOn(void) { if (onV < 0) { onV = P3DS_CfgInt("ach_toasts", 1) != 0; PSPNativeMemLog("[ACH] ach_toasts=%d (list always works; informational only, never touches the save)", onV); } return onV; }
void P3DS_AchSetToasts(int v) { onV = v != 0; P3DS_CfgWrite("ach_toasts", onV); PSPNativeMemLog("[ACH] ach_toasts=%d from settings", onV); }
int P3DS_AchUnviewed(void) { return unviewed; }
void P3DS_AchViewed(void) { unviewed = 0; }

static void push(int v) { if (qn < QN) q[qn++] = v; }
static int unlockLocked(int id, int silent) {
  if (id < 0 || id >= ACH_COUNT || date[id]) return 0;
  date[id] = today(); unviewed++;
  PSPNativeMemLog("[ACH] unlocked %s (%d pts)%s", kAch[id].key, kAch[id].i.pts, silent ? " [carried over]" : "");
  if (!silent && P3DS_AchToastsOn()) push(id);
  return 1; }
void P3DS_AchUnlock(int id) { LOCK(); load(); if (unlockLocked(id, 0)) save(); UNLOCK(); }

void P3DS_AchPublish(int badges, int clear, int red) {
  LOCK(); load(); const int first = bootScan; bootScan = 0; int n = 0, ch = 0;
  for (int i = 0; i < 16; i++) if ((badges >> i) & 1) ch += unlockLocked(i, first);
  if (clear) ch += unlockLocked(ACH_CHAMPION, first);
  if (red) ch += unlockLocked(ACH_RED, first);
  n = ch;
  if (first) { if (n > 0 && P3DS_AchToastsOn()) push(-n); carried = 1; PSPNativeMemLog("[ACH] save scan: badges %04x, hall of fame %d, red %d -> %d carried over", badges & 0xFFFF, clear, red, n); save(); }
  else if (ch) save();
  UNLOCK(); }

/* once per eye per frame; eye 0 advances the queue. Slides up from the bottom edge for 0.4 s, holds until 4.4 s, then goes */
int P3DS_AchToastNow(char *l1, int n1, char *l2, int n2, int *slide, int eye) {
  LOCK();
  const unsigned long long now = nowTick(), tot = 4400ull * TICKS_MS;
  if (eye == 0) {
    if (cur != -1000 && now - curAt > tot) cur = -1000;
    if (cur == -1000 && qn > 0) { cur = q[0]; memmove(q, q + 1, (size_t)(qn - 1) * sizeof q[0]); qn--; curAt = now; if (cur >= 0) P3DS_HuntChime(); } }
  if (cur == -1000) { UNLOCK(); return 0; }
  const int ms = (int)((now - curAt) / TICKS_MS);
  *slide = ms < 400 ? ms * 16 / 400 : 16;
  if (cur >= 0) { snprintf(l1, (size_t)n1, "Achievement Unlocked  %dG", kAch[cur].i.pts); snprintf(l2, (size_t)n2, "%s", kAch[cur].i.name); }
  else { snprintf(l1, (size_t)n1, "Achievements"); snprintf(l2, (size_t)n2, "%d carried over from your save", -cur); }
  UNLOCK(); return 1; }

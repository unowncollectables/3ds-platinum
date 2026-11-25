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
  { "xmas", { "Christmas", "See the Christmas snow.", 25, 3 } },
  { "all16", { "Master of Two Regions", "Collect all 16 badges.", 100, 1 } },
  { "wifitrade", { "Global Trader", "Complete a Wi-Fi or GTS trade.", 25, 1 } },
  { "catch1", { "Gotcha!", "Catch your first Pokemon.", 10, 4 } }, { "catch50", { "Rising Trainer", "Catch 50 Pokemon.", 25, 4 } },
  { "catch200", { "Pokemon Hoarder", "Catch 200 Pokemon.", 50, 4 } },
  { "hatch1", { "Egg-cellent", "Hatch your first egg.", 15, 4 } }, { "hatch25", { "Day Care Hero", "Hatch 25 eggs.", 40, 4 } },
  { "steps10k", { "Out for a Stroll", "Walk 10,000 steps.", 15, 4 } }, { "steps100k", { "Marathon Runner", "Walk 100,000 steps.", 50, 4 } },
  { "bike10k", { "Cycle Champ", "Ride 10,000 steps on the Bicycle.", 20, 4 } },
  { "battle50", { "Battle Ready", "Win 50 trainer battles.", 20, 4 } }, { "battle250", { "Battle Hardened", "Win 250 trainer battles.", 50, 4 } },
  { "wild500", { "Tall Grass Regular", "Meet 500 wild Pokemon.", 25, 4 } },
  { "fish25", { "Gone Fishin'", "Reel in 25 fish.", 20, 4 } }, { "berry10", { "Green Thumb", "Plant 10 berries.", 15, 4 } },
  { "fossil", { "Back from the Dead", "Revive a fossil.", 20, 4 } }, { "nick5", { "Best Buddies", "Give 5 Pokemon a nickname.", 15, 4 } },
  { "mail", { "Pen Pal", "Write a Pokemon mail.", 10, 4 } }, { "splash", { "Splashing Around", "Use Splash in battle.", 15, 4 } },
  { "boom", { "Going Out with a Bang", "Use Explosion or Self-Destruct.", 15, 4 } },
  { "contest", { "Contest Star", "Win a Pokemon Contest.", 40, 4 } }, { "tower", { "Tower Victor", "Win a Battle Frontier battle.", 40, 4 } },
  { "ribbon", { "Ribbon Collector", "Earn a ribbon.", 25, 4 } }, { "signed", { "Autograph Hound", "Have your Trainer Card signed.", 15, 4 } },
  { "skylegend", { "Legend in the Sky", "See a legendary Pokemon fly overhead.", 100, 4 } },
  { "skyshiny", { "Shiny on the Wing", "Spot a shiny Pokemon flying past.", 50, 2 } },
  { "hugewild", { "Something Big", "Meet a rare huge wild Pokemon.", 50, 4 } } };
static const char *const kCat[ACH_CATS] = { "Gyms", "Milestones", "Shiny", "Special Dates", "Adventure" };

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

/* counter-based achievements: GAME_STAT_* index (read from the save by hook 0213) and the goal; trade ones accept several counters (sum) */
static const struct { unsigned short stat; unsigned goal; short id; } kStat[] = {
  { 20, 1, ACH_TRADE }, { 25, 1, ACH_TRADE }, { 25, 1, ACH_WIFITRADE }, { 10, 1, ACH_CATCH1 }, { 10, 50, ACH_CATCH50 }, { 10, 200, ACH_CATCH200 },
  { 12, 1, ACH_HATCH1 }, { 12, 25, ACH_HATCH25 }, { 0, 10000, ACH_STEPS10K }, { 0, 100000, ACH_STEPS100K }, { 1, 10000, ACH_BIKE10K },
  { 9, 50, ACH_BATTLE50 }, { 9, 250, ACH_BATTLE250 }, { 8, 500, ACH_WILD500 }, { 11, 25, ACH_FISH25 }, { 5, 10, ACH_BERRY10 },
  { 116, 1, ACH_FOSSIL }, { 50, 5, ACH_NICK5 }, { 46, 1, ACH_MAIL }, { 77, 1, ACH_SPLASH }, { 79, 1, ACH_BOOM }, { 80, 1, ACH_BOOM },
  { 93, 1, ACH_CONTEST }, { 94, 1, ACH_CONTEST }, { 30, 1, ACH_TOWER }, { 95, 1, ACH_RIBBON }, { 115, 1, ACH_SIGNED }, { 114, 1, ACH_WIFITRADE } };
enum { NSTAT = (int)(sizeof kStat / sizeof kStat[0]) };
static unsigned statNow[NSTAT]; static unsigned char statKnown;
static int date[ACH_COUNT], carried, loaded, unviewed, onV = -1, bootScan = 1, bootStats = 1;
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
void P3DS_AchSky(int what) { P3DS_AchUnlock(what == 2 ? ACH_HUGEWILD : what ? ACH_SKYLEGEND : ACH_SKYSHINY); }

void P3DS_AchPublish(int badges, int clear, int red) {
  LOCK(); load(); const int first = bootScan; bootScan = 0; int n = 0, ch = 0;
  for (int i = 0; i < 16; i++) if ((badges >> i) & 1) ch += unlockLocked(i, first);
  if (clear) ch += unlockLocked(ACH_CHAMPION, first);
  if ((badges & 0xFFFF) == 0xFFFF) ch += unlockLocked(ACH_ALL16, first);
  if (red) ch += unlockLocked(ACH_RED, first);
  n = ch;
  if (first) { if (n > 0 && P3DS_AchToastsOn()) push(-n); carried = 1; PSPNativeMemLog("[ACH] save scan: badges %04x, hall of fame %d, red %d -> %d carried over", badges & 0xFFFF, clear, red, n); save(); }
  else if (ch) save();
  UNLOCK(); }

void P3DS_AchStats(const unsigned short *idx, const unsigned *val, int n) {
  LOCK(); load(); const int first = bootStats; bootStats = 0; int ch = 0, carry = 0;
  for (int k = 0; k < NSTAT; k++) { statNow[k] = 0;
    for (int j = 0; j < n; j++) if (idx[j] == kStat[k].stat) { statNow[k] = val[j]; break; }
    if (statNow[k] >= kStat[k].goal) { const int u = unlockLocked(kStat[k].id, first); ch += u; carry += u; } }
  statKnown = 1;
  if (first) { if (carry > 0 && P3DS_AchToastsOn()) push(-carry); PSPNativeMemLog("[ACH] stats scan: %d counters read, %d carried over", n, carry); }
  if (ch) save();
  UNLOCK(); }
int P3DS_AchProgress(int id, unsigned *cur, unsigned *goal) {
  LOCK(); int ok = 0; unsigned best = 0, g = 0;
  if (statKnown) for (int k = 0; k < NSTAT; k++) if (kStat[k].id == id) { ok = 1; g = kStat[k].goal; if (statNow[k] > best) best = statNow[k]; }
  UNLOCK(); if (ok) { if (cur) *cur = best; if (goal) *goal = g; } return ok; }

/* once per eye per frame; eye 0 advances the queue. Slides up from the bottom edge for 0.4 s, holds, slides back down 4.0-4.4 s */
int P3DS_AchToastNow(char *l1, int n1, char *l2, int n2, int *slide, int eye) {
  LOCK();
  const unsigned long long now = nowTick(), tot = 4400ull * TICKS_MS;
  if (eye == 0) {
    if (cur != -1000 && now - curAt > tot) cur = -1000;
    if (cur == -1000 && qn > 0) { cur = q[0]; memmove(q, q + 1, (size_t)(qn - 1) * sizeof q[0]); qn--; curAt = now; if (cur >= 0) P3DS_HuntChime(); } }
  if (cur == -1000) { UNLOCK(); return 0; }
  const int ms = (int)((now - curAt) / TICKS_MS);
  *slide = ms < 400 ? ms * 16 / 400 : ms > 4000 ? (ms < 4400 ? (4400 - ms) * 16 / 400 : 0) : 16;
  if (cur >= 0) { snprintf(l1, (size_t)n1, "Achievement unlocked\t%dG", kAch[cur].i.pts); snprintf(l2, (size_t)n2, "%s", kAch[cur].i.name); }
  else { snprintf(l1, (size_t)n1, "Achievements"); snprintf(l2, (size_t)n2, "%d found in your save", -cur); }
  UNLOCK(); return 1; }

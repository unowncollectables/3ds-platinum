/* achievements (Settings > Play "Achievement toasts"): Xbox-360-style achievements, port-side and informational only.
 * Never writes the save, never changes gameplay. State lives in sdmc:/3ds/platinum3ds/achievements.txt (player-level). */
#ifndef P3DS_ACHIEVEMENTS_H
#define P3DS_ACHIEVEMENTS_H
#ifdef __cplusplus
extern "C" {
#endif
enum { ACH_ZEPHYR, ACH_HIVE, ACH_PLAIN, ACH_FOG, ACH_STORM, ACH_MINERAL, ACH_GLACIER, ACH_RISING, /* Johto badges = bit 0..7 of the badge mask */
       ACH_BOULDER, ACH_CASCADE, ACH_THUNDER, ACH_RAINBOW, ACH_SOUL, ACH_MARSH, ACH_VOLCANO, ACH_EARTH, /* Kanto badges = bit 8..15 */
       ACH_CHAMPION, ACH_RED, ACH_TRADE, ACH_SHINY,
       ACH_BDAY, ACH_NEWYEAR, ACH_VALENTINE, ACH_STPAT, ACH_CANADA, ACH_HALLOWEEN, ACH_XMAS, ACH_COUNT };
enum { ACH_CAT_GYMS, ACH_CAT_MILESTONES, ACH_CAT_SHINY, ACH_CAT_DATES, ACH_CATS };
typedef struct { const char *name, *desc; int pts, cat; } P3DS_AchInfo;
const P3DS_AchInfo *P3DS_AchGet(int id);
int P3DS_AchDate(int id);                 /* yyyymmdd, 0 = locked */
int P3DS_AchTotal(int cat, int *max);      /* points earned in a category (cat < 0 = all), max = points available */
const char *P3DS_AchCatName(int cat);
int P3DS_AchToastsOn(void);               /* Settings > Play, default ON; the list works either way */
void P3DS_AchSetToasts(int v);
void P3DS_AchUnlock(int id);              /* idempotent; saves the file, queues a toast + chime */
void P3DS_AchPublish(int badges, int clear, int red); /* game thread (hook 0145): save flags; the first call per boot carries over silently */
int P3DS_AchUnviewed(void);               /* unlocks not yet seen in the list */
void P3DS_AchViewed(void);
/* top screen: the toast, call once per eye per frame (eye 0 advances the queue); fills the two lines, returns 0 when nothing is showing.
 * slide = 0..16 (16 = fully in) */
int P3DS_AchToastNow(char *l1, int n1, char *l2, int n2, int *slide, int eye);
#ifdef __cplusplus
}
#endif
#endif

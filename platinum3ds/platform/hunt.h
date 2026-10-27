/* run 88 shiny_suite: shiny hunt counters kept in sdmc:/3ds/platinum3ds/hunt.txt (never the save). See hunt.c. */
#ifndef P3DS_HUNT_H
#define P3DS_HUNT_H
#ifdef __cplusplus
extern "C" {
#endif
#define P3DS_HUNT_SPECIES 494
#define P3DS_HUNT_ODDS 8192 /* Gen 4 random/wild shiny odds */
typedef struct P3DS_HuntRec { int enc, streak, resets, shinies, secs, start, lastShinyAt; } P3DS_HuntRec;
int P3DS_HuntOn(void);                 /* shiny_suite (cached) */
void P3DS_HuntSetOn(int on);           /* settings app */
int P3DS_HuntCounting(void);           /* shiny_suite or the older hunt_counter */
const P3DS_HuntRec *P3DS_HuntGet(int sp);
int P3DS_HuntTarget(void);             /* last species met = the current hunt */
int P3DS_HuntNext(int sp, int dir);    /* next species with a record (dir +1/-1), 0 if none */
void P3DS_HuntEncounter(int sp, int shiny);
void P3DS_HuntSoftReset(void);
void P3DS_HuntFrame(int paused);       /* every frame: time spent + periodic write */
void P3DS_HuntFlush(void);
void P3DS_HuntClear(int sp);
double P3DS_HuntChanceBy(int n, int odds); /* chance to have seen at least one shiny in n encounters */
int P3DS_HuntExportCsv(const char *const *names, int nNames, char *path, int pathN);
void P3DS_HuntChime(void);
/* run 119 shiny suite extras (all default off; see hunt.c) */
extern volatile int P3DS_HuntGame[5];  /* field side, shiny_suite only: [0] valid, [1] party shinies, [2] party size,
                                          [3] Poke Radar chain (0 = none), [4] Day Care 0 = <2 parents, 1 same language, 2 Masuda */
extern volatile int P3DS_ShinyForce;   /* wild_vis_shiny: 1 = next wild Pokemon shiny, 2 = decided not shiny (no shiny_rate roll) */
int P3DS_HuntRateOdds(void);           /* shiny_rate as odds: 8192 (original), 4096, 2048, 1024 */
void P3DS_HuntSetRate(int idx);        /* settings app */
int P3DS_ShinyRoll(void);              /* overlay006 CreateWildMon: 1 = take the game's own shiny path */
int P3DS_WildShinyRoll(int always);    /* wild_vis_shiny: does a visible wild Pokemon spawn shiny */
int P3DS_RadarPatchOdds(int chain);    /* the game's CheckPatchShiny rate: 1 in max(200, 8200 - 200 * chain) per patch */
int P3DS_MasudaOdds(void);             /* the game's Egg_SetInitialData: 5 personality tries -> 1 in 1639 */
int P3DS_HuntEncFor(double chance, int odds); /* encounters for that chance of at least one shiny */
unsigned P3DS_RngInitial(int *seeds);  /* first LCRNG_SetSeed value (the boot seed) and how many seedings */
#ifdef __cplusplus
}
#endif
#endif

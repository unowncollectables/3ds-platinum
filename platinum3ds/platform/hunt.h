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
#ifdef __cplusplus
}
#endif
#endif

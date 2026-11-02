/* tracker=1 (Settings > Play "Roamer + breeding tracker", default OFF): roaming Pokemon and Day-Care breeding preview, shown on the
 * JOHTO bottom map (tracker.c, hook 0120). Read-only. */
#ifndef P3DS_TRACKER_H
#define P3DS_TRACKER_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { int active, species, level, hp, mapId, loc; } P3DS_TrkRoamer; /* loc = Pokegear location index or -1 */
typedef struct { int n, sp[2], compat, masuda, egg, toCheck; } P3DS_TrkDaycare; /* compat = egg chance in % per check, toCheck = steps to the next check or -1 */
extern volatile unsigned P3DS_TrkStepTick; /* Day-Care step counter changed (card text only) */
extern volatile unsigned P3DS_TrkTick;  /* bumps whenever the published data changed (the bottom screen repaints) */
extern volatile int P3DS_TrkReady;
extern P3DS_TrkRoamer P3DS_TrkRoam[4];  /* 0 Raikou, 1 Entei, 2 Latias, 3 Latios (the game's roamer slots) */
extern P3DS_TrkDaycare P3DS_TrkDc;
int P3DS_TrkOn(void);
void P3DS_TrkSetOn(int v);
/* game thread (hook 0120): r = 4 x {active, species, level, hp, mapId, loc}, d = {n, speciesA, speciesB, compat%, masuda, egg, toCheck} */
void P3DS_TrkPublish(const int *r, const int *d);
#ifdef __cplusplus
}
#endif
#endif

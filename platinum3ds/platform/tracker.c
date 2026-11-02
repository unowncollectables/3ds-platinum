/* tracker=1 (default off; Settings > Play "Roamer + breeding tracker"): where the roaming Pokemon are and what the Day-Care
 * will do, drawn on the JOHTO bottom map (ss3ds_pgmap.h: coloured marks on the map, a block at the top of the progress
 * overview). Read-only: pspoke hook 0120 (field_control.c, every 16 steps and on a map change) copies the game's own roamer
 * save data and Day-Care state here through P3DS_TrkPublish; nothing is written to the save.
 * Slots are the game's four roamers (Raikou, Entei, Latias, Latios). Suicune does not roam in HeartGold/SoulSilver.
 * Egg odds are the game's own numbers: Save_Daycare_CalcCompatibility (70 / 50 / 20 / 0 %) is rolled every 256 steps, and
 * a language mismatch (Masuda) gives the egg 4 extra shiny rerolls (5/8192 instead of 1/8192).
 * Log: [TRACK] lines (setting, first publish, every change). */
#include <stdio.h>
#include <string.h>
#include "tracker.h"

extern void PSPNativeMemLog(const char *, ...);
extern int P3DS_CfgInt(const char *, int);
extern void P3DS_CfgWrite(const char *, int);

volatile unsigned P3DS_TrkTick, P3DS_TrkStepTick;
volatile int P3DS_TrkReady;
P3DS_TrkRoamer P3DS_TrkRoam[4];
P3DS_TrkDaycare P3DS_TrkDc;
static int onV = -1;

int P3DS_TrkOn(void) { if (onV < 0) { onV = P3DS_CfgInt("tracker", 0) != 0; PSPNativeMemLog("[TRACK] tracker=%d (roamers + Day-Care on the JOHTO map; read-only)", onV); } return onV; }
void P3DS_TrkSetOn(int v) { onV = v != 0; P3DS_CfgWrite("tracker", onV); P3DS_TrkTick++; PSPNativeMemLog("[TRACK] tracker=%d from settings", onV); }

void P3DS_TrkPublish(const int *r, const int *d) {
  P3DS_TrkRoamer nr[4]; P3DS_TrkDaycare nd; int changed = !P3DS_TrkReady;
  for (int i = 0; i < 4; i++) { nr[i].active = r[i * 6] != 0; nr[i].species = r[i * 6 + 1]; nr[i].level = r[i * 6 + 2]; nr[i].hp = r[i * 6 + 3]; nr[i].mapId = r[i * 6 + 4]; nr[i].loc = r[i * 6 + 5];
    if (nr[i].species < 0 || nr[i].species > 493) nr[i].species = 0; }
  nd.n = d[0]; nd.sp[0] = d[1]; nd.sp[1] = d[2]; nd.compat = d[3]; nd.masuda = d[4]; nd.egg = d[5]; nd.toCheck = d[6];
  for (int k = 0; k < 2; k++) if (nd.sp[k] < 0 || nd.sp[k] > 493) nd.sp[k] = 0;
  for (int i = 0; i < 4; i++) {
    if (memcmp(&nr[i], &P3DS_TrkRoam[i], sizeof nr[i])) { changed = 1;
      if (nr[i].active) PSPNativeMemLog("[TRACK] roamer %d species %d Lv%d hp %d now on map %d (Pokegear location %d)", i, nr[i].species, nr[i].level, nr[i].hp, nr[i].mapId, nr[i].loc);
      else if (P3DS_TrkRoam[i].active) PSPNativeMemLog("[TRACK] roamer %d no longer roaming", i); } }
  if (nd.n != P3DS_TrkDc.n || nd.egg != P3DS_TrkDc.egg || nd.compat != P3DS_TrkDc.compat || nd.masuda != P3DS_TrkDc.masuda || nd.sp[0] != P3DS_TrkDc.sp[0] || nd.sp[1] != P3DS_TrkDc.sp[1] || !P3DS_TrkReady) {
    changed = 1; PSPNativeMemLog("[TRACK] daycare %d Pokemon (%d + %d), egg chance %d%%, masuda %d, egg waiting %d, steps to check %d", nd.n, nd.sp[0], nd.sp[1], nd.compat, nd.masuda, nd.egg, nd.toCheck); }
  if (nd.toCheck != P3DS_TrkDc.toCheck) P3DS_TrkStepTick++; /* run 62: Day-Care step counter ticks every step; only the overview card shows it, so it must not repaint the bottom screen on its own */
  memcpy(P3DS_TrkRoam, nr, sizeof nr); P3DS_TrkDc = nd;
  if (changed) { P3DS_TrkReady = 1; P3DS_TrkTick++; }
}

/* run 88 shiny_suite=0 (default off; settings app Extra tab "Shiny hunt suite"): shiny hunt counters that survive
 * soft resets and power-offs. Port file only: sdmc:/3ds/platinum3ds/hunt.txt, never platinum.sav.
 *
 * hunt.txt, one line per species (the run 76 hunt_counter format "species=encounters" is the first field, so old
 * builds still read the count):
 *   target=<species>                       the current hunt = the last species met (soft resets are tagged to it)
 *   <species>=enc,streak,resets,shinies,secs,start,lastShinyAt
 *     enc         wild battles against it (all time)       streak   encounters since the last shiny
 *     resets      soft resets while it was the target       shinies  shinies met
 *     secs        play time while it was the target         start    first encounter, YYYYMMDD
 *     lastShinyAt streak length when the last shiny appeared
 * Written on every encounter, soft reset, lid close, exit and every 2 minutes of hunting; via hunt.tmp so a power cut
 * mid-write keeps the old file. Odds are the game's own Gen 4 rule for random wild Pokemon: 1 in 8192 (Masuda eggs
 * and Poke Radar chains are not detected here, so they are not claimed).
 * Shiny alert (shiny_suite=1): a two-note chime on DSP channel 1 (the game's audio uses channel 0 only), a toast and
 * a short flash/sparkle on the top screen (render3ds.cpp drawShinyFlash). The shiny flag is the game's own
 * Pokemon_IsShiny on the wild foe (battle_controller_player.c, P3DS_CatchInfo[18]).
 * Host test: tools/hunt_test.c (compiles this file with P3DS_HUNT_HOST). */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include "hunt.h"
#ifdef P3DS_HUNT_HOST
#include <stdint.h>
typedef uint64_t u64; typedef int16_t s16;
extern u64 HuntHostTicks(void); extern int P3DS_CfgInt(const char *, int); extern void PSPNativeMemLog(const char *, ...);
#define svcGetSystemTick HuntHostTicks
#define CPU_TICKS_PER_MSEC 1000ull
#define HUNT_LOCK()
#define HUNT_UNLOCK()
#else
#include <3ds.h>
extern int P3DS_CfgInt(const char *, int); extern void PSPNativeMemLog(const char *, ...);
static LightLock huntLk; static int huntLkInit;
#define HUNT_LOCK() do { if (!huntLkInit) { LightLock_Init(&huntLk); huntLkInit = 1; } LightLock_Lock(&huntLk); } while (0)
#define HUNT_UNLOCK() LightLock_Unlock(&huntLk)
#endif
#ifndef HUNT_DIR
#define HUNT_DIR "sdmc:/3ds/platinum3ds/"
#endif

static P3DS_HuntRec rec[P3DS_HUNT_SPECIES];
static int target, loaded, dirty, onV = -1;
static u64 lastTick, msAcc, lastWrite;

int P3DS_HuntOn(void) { if (onV < 0) { onV = P3DS_CfgInt("shiny_suite", 0) != 0; PSPNativeMemLog("[HUNT] shiny_suite=%d", onV); } return onV; }
void P3DS_HuntSetOn(int on) { onV = on != 0; }
int P3DS_HuntCounting(void) { return P3DS_HuntOn() || P3DS_CfgInt("hunt_counter", 0); }

static int today(void) { time_t t = time(NULL); struct tm *lt = localtime(&t); return lt ? (lt->tm_year + 1900) * 10000 + (lt->tm_mon + 1) * 100 + lt->tm_mday : 0; }
static int parse(FILE *f) {
    char l[128]; int n = 0;
    while (fgets(l, sizeof l, f)) {
        int sp, v[7] = {0}, t;
        if (sscanf(l, "target=%d", &t) == 1) { if (t > 0 && t < P3DS_HUNT_SPECIES) target = t; continue; }
        int k = sscanf(l, "%d=%d,%d,%d,%d,%d,%d,%d", &sp, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6]);
        if (k < 2 || sp <= 0 || sp >= P3DS_HUNT_SPECIES) continue;
        P3DS_HuntRec *r = &rec[sp]; r->enc = v[0]; r->streak = k >= 3 ? v[1] : v[0]; /* old line: no shiny seen yet */
        r->resets = v[2]; r->shinies = v[3]; r->secs = v[4]; r->start = v[5]; r->lastShinyAt = v[6]; n++;
    }
    return n;
}
static void load(void) {
    if (loaded) return;
    loaded = 1;
    FILE *f = fopen(HUNT_DIR "hunt.txt", "r"); const char *src = "hunt.txt";
    if (!f) { f = fopen(HUNT_DIR "hunt.tmp", "r"); src = "hunt.tmp"; } /* power cut between remove and rename */
    if (!f) return;
    int n = parse(f); fclose(f); PSPNativeMemLog("[HUNT] %s: %d species, target %d", src, n, target);
}
/* run 104 hunt_async_save (default 1): the hunt file is formatted in memory (fast) and written by a low-priority
   thread. The synchronous fopen/write/remove/rename took ~70 ms on the game thread at every wild battle start (and
   every 2 min), which was the recurring battle-start drop on hardware. Same file contents; 0 = write in place. */
static char hbuf[48 * P3DS_HUNT_SPECIES + 32];
static void writeFile(const char *t, int n) {
    FILE *f = fopen(HUNT_DIR "hunt.tmp", "w"); if (!f) { PSPNativeMemLog("[HUNT] cannot write hunt.tmp"); return; }
    fwrite(t, 1, n, f); fclose(f); remove(HUNT_DIR "hunt.txt"); rename(HUNT_DIR "hunt.tmp", HUNT_DIR "hunt.txt");
}
#ifndef P3DS_HUNT_HOST
static char hpend[sizeof hbuf], hwr[sizeof hbuf]; static volatile int hpendN = -1; static LightLock hpLk; static LightEvent hpEv; static Thread hThr;
static void writer(void *u) { (void)u; for (;;) { LightEvent_Wait(&hpEv); LightLock_Lock(&hpLk); int n = hpendN; if (n >= 0) memcpy(hwr, hpend, n); hpendN = -1; LightLock_Unlock(&hpLk); if (n >= 0) writeFile(hwr, n); } }
#endif
static void save(void) {
    int n = 0;
    if (target) n += snprintf(hbuf + n, sizeof hbuf - n, "target=%d\n", target);
    for (int k = 1; k < P3DS_HUNT_SPECIES && n < (int)sizeof hbuf - 64; k++) { const P3DS_HuntRec *r = &rec[k];
        if (r->enc || r->resets || r->shinies) n += snprintf(hbuf + n, sizeof hbuf - n, "%d=%d,%d,%d,%d,%d,%d,%d\n", k, r->enc, r->streak, r->resets, r->shinies, r->secs, r->start, r->lastShinyAt); }
    dirty = 0; lastWrite = svcGetSystemTick();
#ifndef P3DS_HUNT_HOST
    static int as = -1; if (as < 0) { as = P3DS_CfgInt("hunt_async_save", 1) != 0;
        if (as) { LightLock_Init(&hpLk); LightEvent_Init(&hpEv, RESET_ONESHOT); hThr = threadCreate(writer, 0, 16 * 1024, 0x3F, 0, true); if (!hThr) as = 0; }
        PSPNativeMemLog("[HUNT] hunt_async_save=%d", as); }
    if (as) { LightLock_Lock(&hpLk); memcpy(hpend, hbuf, n); hpendN = n; LightLock_Unlock(&hpLk); LightEvent_Signal(&hpEv); return; }
#endif
    writeFile(hbuf, n);
}

const P3DS_HuntRec *P3DS_HuntGet(int sp) { HUNT_LOCK(); load(); HUNT_UNLOCK(); return sp > 0 && sp < P3DS_HUNT_SPECIES ? &rec[sp] : &rec[0]; }
int P3DS_HuntTarget(void) { HUNT_LOCK(); load(); int t = target; HUNT_UNLOCK(); return t; }
int P3DS_HuntNext(int sp, int dir) {
    HUNT_LOCK(); load(); int r = 0;
    for (int i = 1; i < P3DS_HUNT_SPECIES && !r; i++) { int k = ((sp + dir * i) % (P3DS_HUNT_SPECIES - 1) + (P3DS_HUNT_SPECIES - 1)) % (P3DS_HUNT_SPECIES - 1);
        if (!k) k = P3DS_HUNT_SPECIES - 1;
        if (rec[k].enc || rec[k].resets) r = k; }
    if (!r && sp > 0 && sp < P3DS_HUNT_SPECIES && (rec[sp].enc || rec[sp].resets)) r = sp;
    HUNT_UNLOCK(); return r;
}
void P3DS_HuntEncounter(int sp, int shiny) {
    if (sp <= 0 || sp >= P3DS_HUNT_SPECIES) return;
    HUNT_LOCK(); load(); P3DS_HuntRec *r = &rec[sp];
    if (!r->start) r->start = today();
    if (target != sp) msAcc = 0;
    target = sp; r->enc++; r->streak++;
    if (shiny) { r->shinies++; r->lastShinyAt = r->streak; r->streak = 0; }
    save(); HUNT_UNLOCK();
    if (shiny) PSPNativeMemLog("[HUNT] SHINY species %d after %d encounters (%d soft resets, total met %d)", sp, r->lastShinyAt, r->resets, r->enc);
    else PSPNativeMemLog("[HUNT] species %d met %d times (streak %d)", sp, r->enc, r->streak);
}
void P3DS_HuntSoftReset(void) {
    if (!P3DS_HuntOn()) return;
    HUNT_LOCK(); load(); if (target) { rec[target].resets++; save(); PSPNativeMemLog("[HUNT] soft reset %d for species %d", rec[target].resets, target); } HUNT_UNLOCK();
}
void P3DS_HuntFrame(int paused) {
    const u64 now = svcGetSystemTick(); const u64 prev = lastTick; lastTick = now;
    if (!P3DS_HuntOn() || paused || !prev) return;
    const u64 ms = (now - prev) / CPU_TICKS_PER_MSEC; if (ms > 1000) return; /* sleep, settings app, loading stall */
    HUNT_LOCK(); load();
    if (target) { msAcc += ms; if (msAcc >= 1000) { rec[target].secs += (int)(msAcc / 1000); msAcc %= 1000; dirty = 1; } }
    if (dirty && now - lastWrite > 120000ull * CPU_TICKS_PER_MSEC) save();
    HUNT_UNLOCK();
}
void P3DS_HuntFlush(void) { HUNT_LOCK(); if (loaded && dirty) save(); HUNT_UNLOCK(); }
void P3DS_HuntClear(int sp) {
    if (sp <= 0 || sp >= P3DS_HUNT_SPECIES) return;
    HUNT_LOCK(); load(); memset(&rec[sp], 0, sizeof rec[sp]); save(); HUNT_UNLOCK(); PSPNativeMemLog("[HUNT] counter cleared for species %d", sp);
}
double P3DS_HuntChanceBy(int n, int odds) { return n <= 0 || odds <= 1 ? 0.0 : 1.0 - exp((double)n * log1p(-1.0 / odds)); }
int P3DS_HuntExportCsv(const char *const *names, int nNames, char *path, int pathN) {
    time_t t = time(NULL); struct tm *lt = localtime(&t);
    mkdir(HUNT_DIR "hunts", 0777);
    snprintf(path, pathN, HUNT_DIR "hunts/hunt_%04d%02d%02d_%02d%02d.csv", lt ? lt->tm_year + 1900 : 0, lt ? lt->tm_mon + 1 : 0, lt ? lt->tm_mday : 0, lt ? lt->tm_hour : 0, lt ? lt->tm_min : 0);
    FILE *f = fopen(path, "w"); if (!f) return -1;
    fprintf(f, "species_id,species,encounters,since_last_shiny,soft_resets,shinies,minutes,started,last_shiny_after,chance_by_now_pct,current\n");
    int n = 0; HUNT_LOCK(); load();
    for (int k = 1; k < P3DS_HUNT_SPECIES; k++) { const P3DS_HuntRec *r = &rec[k]; if (!r->enc && !r->resets && !r->shinies) continue;
        fprintf(f, "%d,%s,%d,%d,%d,%d,%d,%d,%d,%.1f,%d\n", k, k < nNames && names ? names[k] : "", r->enc, r->streak, r->resets, r->shinies, r->secs / 60, r->start, r->lastShinyAt,
                100.0 * P3DS_HuntChanceBy(r->streak, P3DS_HUNT_ODDS), k == target); n++; }
    HUNT_UNLOCK(); fclose(f); PSPNativeMemLog("[HUNT] exported %d species to %s", n, path); return n;
}
#ifdef P3DS_HUNT_HOST
void P3DS_HuntChime(void) {}
#else
void P3DS_HuntChime(void) {
    enum { RATE = 32000, N = RATE * 50 / 100, SPLIT = RATE * 15 / 100 };
    static s16 *buf; static ndspWaveBuf wb;
    if (!buf) { buf = (s16 *)linearAlloc(N * 2); if (!buf) return;
        for (int i = 0; i < N; i++) { const int j = i < SPLIT ? i : i - SPLIT; const float f = i < SPLIT ? 1567.98f : 2093.0f; /* G6, then C7 */
            const float tt = (float)j / RATE, env = (tt < 0.004f ? tt / 0.004f : 1.0f) * expf(-tt * (i < SPLIT ? 12.0f : 7.0f));
            buf[i] = (s16)(sinf(6.2831853f * f * tt) * env * 9000.0f); }
        DSP_FlushDataCache(buf, N * 2); }
    if (wb.status == NDSP_WBUF_QUEUED || wb.status == NDSP_WBUF_PLAYING) return;
    ndspChnReset(1); ndspChnSetInterp(1, NDSP_INTERP_LINEAR); ndspChnSetRate(1, (float)RATE); ndspChnSetFormat(1, NDSP_FORMAT_MONO_PCM16);
    float mix[12] = {0.8f, 0.8f}; ndspChnSetMix(1, mix);
    memset(&wb, 0, sizeof wb); wb.data_vaddr = buf; wb.nsamples = N; ndspChnWaveBufAdd(1, &wb);
}
#endif

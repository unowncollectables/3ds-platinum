/* Startup check: is the New 3DS 804 MHz / L2 speed-up really on, and how fast is each core?
 *
 * Idea and job design adapted from GabeSud0/pokeplatinum-3ds (GPL-3.0, (c) its authors),
 * 3ds_port/game/app/main_3ds.c: EnableSpeedup / LogSpeedup / TimeCore / CheckCoreSpeed.
 * The three timing jobs (SpeedJobMain) follow theirs closely (same loops and constants, so the ms figures
 * are comparable); the surrounding code is rewritten for this project's log (PSPNativeMemLog) and config
 * (P3DS_CfgInt). Credited in THIRD_PARTY.md.
 *
 * Why: osSetSpeedupEnable() does nothing, silently, if the program can't use the ptm:sysm service, and the
 * exheader flags (CpuSpeed 804MHz / EnableL2Cache) only apply to an installed CIA. The log said "Speedup is on"
 * (PROGRESS.md run 29) but nothing ever measured it. This logs each step's result and times three small jobs on
 * every core the program can use, so log.txt says whether the clock is really 804 MHz and whether core 1
 * (shared with the system) or core 2 is the slow one.
 *
 * Config: speedcheck=1 (default) runs it once at boot (~0.3 s at 804 MHz, ~1 s at 268 MHz); 0 = skip.
 * It only observes: it never changes the speed-up setting beyond asking for it again, and the PTMSYSM
 * request is the same one libctru's osSetSpeedupEnable makes. Old 3DS: does nothing.
 */
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

extern void PSPNativeMemLog(const char *, ...);
extern int P3DS_CfgInt(const char *, int);

typedef struct SpeedJob {
    int kind;                /* 0 integer, 1 64-bit multiply, 2 double divide */
    u64 ticks;
    volatile u32 sink;
    volatile double start;   /* 1.0, read at run time so the compiler can't fold the loop */
} SpeedJob;

static void SpeedJobMain(void *arg) {
    SpeedJob *job = (SpeedJob *)arg;
    u64 t0 = svcGetSystemTick();
    u32 i;
    if (job->kind == 0) {
        u32 x = 12345;
        for (i = 0; i < 4000000u; i++) { x = x * 1103515245u + 12345u; x ^= x >> 7; }
        job->sink = x;
    } else if (job->kind == 1) {
        s64 acc = 1; s32 a = 4096, b = -3000;
        for (i = 0; i < 2000000u; i++) { acc += (s64)a * b; a = (s32)(acc >> 12) | 1; b += 7; }
        job->sink = (u32)acc;
    } else {
        double d = job->start;
        for (i = 0; i < 300000u; i++) d = (d + 1.5) / (d * 0.5 + 2.0);
        job->sink = (u32)(d * 1000.0);
    }
    job->ticks = svcGetSystemTick() - t0;
    threadExit(0);
}

/* The three jobs on one core; ms[kind] = milliseconds, or -1 if no thread could start there. */
static void TimeCore(int core, s32 priority, double ms[3]) {
    for (int kind = 0; kind < 3; kind++) {
        static SpeedJob job;
        memset(&job, 0, sizeof job);
        job.kind = kind;
        job.start = 1.0;
        Thread t = threadCreate(SpeedJobMain, &job, 16 * 1024, priority, core, false);
        if (!t) { ms[kind] = -1.0; continue; }
        threadJoin(t, UINT64_MAX);
        threadFree(t);
        ms[kind] = (double)job.ticks * 1000.0 / SYSCLOCK_ARM11;
    }
}

void P3DS_SpeedCheck(void) {
    bool n3ds = false;
    APT_CheckNew3DS(&n3ds);
    if (!n3ds) return;   /* Old 3DS: nothing to check */
    if (!P3DS_CfgInt("speedcheck", 1)) { PSPNativeMemLog("[perf] speedcheck=0: skipped"); return; }

    /* The request itself, each step's result kept for the log. Same call chain as osSetSpeedupEnable. */
    osSetSpeedupEnable(true);
    Result svc = ptmSysmInit(), set = -1;
    if (R_SUCCEEDED(svc)) { set = PTMSYSM_ConfigureNew3DSCPU(3); ptmSysmExit(); }
    if (R_FAILED(svc))
        PSPNativeMemLog("[perf] speed-up request: ptm:sysm not available to this program (%08lx); an installed CIA's exheader flags may still apply, see the verdict below", (unsigned long)svc);
    else if (R_FAILED(set))
        PSPNativeMemLog("[perf] speed-up request: ptm:sysm refused it (%08lx); an installed CIA's exheader flags may still apply, see the verdict below", (unsigned long)set);
    else
        PSPNativeMemLog("[perf] speed-up (804 MHz + L2): asked for and accepted");

    /* Timing: integer job is 4M rounds of ~7 cycles: ~35 ms at 804 MHz, ~105 ms at 268 MHz.
     * Core 0 first (the game's own core), then core 2 (workers), then core 1 (system's share; skipped if
     * the program has no share of it yet, since a thread can only be created there after
     * APT_SetAppCpuTimeLimit, which the renderer asks for itself). */
    s32 prio = 0x2F;
    double ms[3][3] = {{0}};
    int used = 0;
    char line[256];
    static const int cores[3] = {0, 2, 1};
    for (int i = 0; i < 3; i++) {
        int core = cores[i];
        TimeCore(core, prio, ms[i]);
        used += snprintf(line + used, sizeof line - (size_t)used, "%score %d: %.1f / %.1f / %.1f", i ? "; " : "", core, ms[i][0], ms[i][1], ms[i][2]);
        if (used >= (int)sizeof line) break;
    }
    PSPNativeMemLog("[perf] core speed, ms for the same jobs (integer / 64-bit mul / double div; integer ~35 ms at 804 MHz, ~105 at 268; -1 = no thread; core 1 is -1 here because its share is only requested later, by the renderer): %s", line);

    /* One-line verdict from the game core's integer job. */
    if (ms[0][0] > 0) {
        const char *v = ms[0][0] < 55.0 ? "804 MHz (speed-up is ON)" : (ms[0][0] > 80.0 ? "~268 MHz (speed-up is NOT working)" : "in between (check the lines above)");
        PSPNativeMemLog("[perf] verdict: core 0 runs at %s", v);
    }
}

/* fps_tweaks 7 (+PARALLEL), 2026-10-29: a tiny fork/join helper for the per-frame pixel passes.
 *
 * Run 62 log, 3D slider up (sharp FAST 3D stereo): 36-41 ms per frame, 24-27 fps, against a steady 30 with the slider at 0.
 * About 10 ms of the stereo frame is single-core pixel work done in a row on the game core: the readback convert in g3gpu.c
 * (rbConvert, ~3.5 ms for both eyes), the 3D layer swizzle in gpucomp.c (conv3dRaw, ~2 ms per eye) and the compose output
 * byte swap (gcOut, ~1.5 ms per eye). Each is a plain loop over 192 rows with no dependency between rows, and core 2 sits idle
 * during all three (the engine B worker has nothing to do under the Bottom menu).
 *
 * P3DS_ParFor(fn, ctx, n) runs fn(ctx, 0..n-1). The calling thread and one helper thread on core 2 both take the next chunk
 * from a shared counter, so the work is split by speed and the caller never depends on the helper being awake: if the helper
 * is late or busy the caller simply does every chunk itself (the only wait is for a chunk the helper already took).
 * Off (level < 7), on an old 3DS, or if the thread cannot start: plain serial loop, bit-identical output.
 * Not re-entrant: a call that finds the helper busy runs serially. */
#include <3ds.h>
#include <string.h>

extern void PSPNativeMemLog(const char *, ...);
extern int P3DS_FpsTweaks(void);

typedef void (*P3DSParFn)(void *ctx, unsigned chunk);

static LightLock lk, jobLk;
static LightEvent ev;
static Thread th;
static int state;                       /* 0 not tried, 1 helper running, -1 no helper */
static P3DSParFn jfn; static void *jctx; static unsigned jn, jnext, jdone;
static unsigned long long nJobs, nSerial, chM, chW, usTot;

static int claim(P3DSParFn *f, void **c, unsigned *i) {
    int got = 0;
    LightLock_Lock(&lk);
    if (jnext < jn) { *i = jnext++; *f = jfn; *c = jctx; got = 1; }
    LightLock_Unlock(&lk);
    return got;
}
static void finish(int helper) { LightLock_Lock(&lk); jdone++; if (helper) chW++; else chM++; LightLock_Unlock(&lk); }

static void helper(void *arg) {
    (void)arg;
    for (;;) {
        LightEvent_Wait(&ev);
        P3DSParFn f; void *c; unsigned i;
        while (claim(&f, &c, &i)) { f(c, i); finish(1); }
    }
}

static int ensure(void) {
    if (state) return state > 0;
    state = -1;
    bool n3ds = false; APT_CheckNew3DS(&n3ds);
    if (n3ds) {
        LightLock_Init(&lk); LightLock_Init(&jobLk); LightEvent_Init(&ev, RESET_ONESHOT);
        th = threadCreate(helper, NULL, 0x4000, 0x2E, 2, false);   /* a notch above the engine B worker (0x2F) on the same core, so a 1 ms chunk is not stuck behind a compose */
        if (th) state = 1;
    }
    PSPNativeMemLog("[PAR] helper thread on core 2: %s (fps_tweaks 7 +PARALLEL runs the 3D convert / texture swizzle / compose swap passes on two cores)", state > 0 ? "yes" : "no, serial");
    return state > 0;
}

void P3DS_ParFor(P3DSParFn fn, void *ctx, unsigned n) {
    if (n < 2 || P3DS_FpsTweaks() < 7 || !ensure() || LightLock_TryLock(&jobLk)) { for (unsigned i = 0; i < n; i++) fn(ctx, i); nSerial++; return; }
    const u64 t0 = svcGetSystemTick();
    LightLock_Lock(&lk); jfn = fn; jctx = ctx; jn = n; jnext = 0; jdone = 0; LightLock_Unlock(&lk);
    LightEvent_Signal(&ev);
    { P3DSParFn f; void *c; unsigned i; while (claim(&f, &c, &i)) { f(c, i); finish(0); } }
    for (;;) { LightLock_Lock(&lk); const unsigned d = jdone; LightLock_Unlock(&lk); if (d >= n) break; svcSleepThread(0); }
    LightLock_Unlock(&jobLk);
    usTot += (svcGetSystemTick() - t0) / 268;
    if (++nJobs % 600 == 0) {
        PSPNativeMemLog("[PAR] last 600 parallel passes: helper took %u%% of the chunks, avg %u us per pass (%u run serial)",
            (unsigned)(chW * 100 / (chW + chM ? chW + chM : 1)), (unsigned)(usTot / 600), (unsigned)nSerial);
        chW = chM = usTot = 0; nSerial = 0;
    }
}

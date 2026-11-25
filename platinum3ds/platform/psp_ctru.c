#include <stdio.h>
/* PSP kernel subset on libctru: threads (with PSP wakeup counts), semaphores, alarms, time, critical sections. */
#include <3ds.h>
#include <pspkernel.h>
#include "crumbs.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#define MAXT 32

typedef struct { Thread th; SceKernelThreadEntry entry; void *arg; SceSize len; u8 argbuf[64]; LightSemaphore wake; int prio, stack, used, started, core; char name[24]; } PT;
static PT pts[MAXT + 1]; /* slot 0 = main thread */
static __thread int tlsId;
volatile int p3ds_quit; /* set by P3DS_ShutdownThreads: every worker leaves at its next blocking point */
static void quitCheck(void) { if (p3ds_quit && tlsId != 0) threadExit(0); }
/* 2026-11-22 thread-limit diagnostics. Hardware boot 15/16 (sound dead after ~15 soft resets): no thread could be created at all,
   not even on core 0 (speedcheck "-1 = no thread" on every core, g3thr "could not start"), so the sound thread (sas_out.c
   native_sas_out) never started, sceAudioOutputPannedBlocking was never called and ndspInit never ran ("ndsp was unstarted").
   The application resource-limit tally and the current process's live thread count are separate measurements. */
void PSPNativeMemLog(const char *, ...);
static void resLimitStr(char *b, int n) {
    Handle rl = 0; static const ResourceLimitType ty[] = { RESLIMIT_THREAD, RESLIMIT_EVENT, RESLIMIT_MUTEX, RESLIMIT_SEMAPHORE, RESLIMIT_TIMER, RESLIMIT_SHAREDMEMORY, RESLIMIT_ADDRESSARBITER, RESLIMIT_CPUTIME };
    static const char *nm[] = { "threads", "events", "mutexes", "semaphores", "timers", "sharedmem", "arbiters", "cputime" };
    s64 cur[8], mx[8]; ResourceLimitType t[8]; for (int i = 0; i < 8; i++) { t[i] = ty[i]; cur[i] = mx[i] = -1; }
    Result r = svcGetResourceLimit(&rl, CUR_PROCESS_HANDLE);
    if (R_SUCCEEDED(r)) { if (R_FAILED(svcGetResourceLimitCurrentValues(cur, rl, t, 8))) for (int i = 0; i < 8; i++) cur[i] = -1; if (R_FAILED(svcGetResourceLimitLimitValues(mx, rl, t, 8))) for (int i = 0; i < 8; i++) mx[i] = -1; svcCloseHandle(rl); }
    int w = snprintf(b, n, "reslimit%s", R_FAILED(r) ? " (unreadable)" : "");
    for (int i = 0; i < 8 && w > 0 && w < n; i++) w += snprintf(b + w, n - w, " %s %lld/%lld", nm[i], (long long)cur[i], (long long)mx[i]);
}
static s64 procInfo(u32 type) { s64 v = -1; if (R_FAILED(svcGetProcessInfo(&v, CUR_PROCESS_HANDLE, type))) v = -1; return v; }
void P3DS_ThreadDiag(const char *when) {
    char b[300]; resLimitStr(b, sizeof b);
    PSPNativeMemLog("[3DS] threads at %s: this process %lld threads, %lld handles | %s", when, (long long)procInfo(7), (long long)procInfo(4), b);
}
static RecursiveLock big; static int bigInit;
static void initBig(void) { if (!bigInit) { RecursiveLock_Init(&big); bigInit = 1; LightSemaphore_Init(&pts[0].wake, 0, 0x7fff); pts[0].used = 1; } }
/* ss3ds run 74: who holds the big "interrupts off" lock? Run 73 (GTS) deadlocked with main stuck in intrlock-wait and nothing said which
   thread owned it. Remember the last acquirer (thread slot + caller) so the watchdog can print it; two plain stores per acquire. */
static volatile unsigned bigLr; static volatile int bigTid;
/* run 79: slot 0 is also what every non-psp thread (libntrdwc/CPS network threads) reports, so "main" in run 74-78 may really be one of
   those. Keep the real thread pointer too, and snapshot who held the lock at the moment someone had to wait (the last wait = the deadlock). */
static volatile unsigned bigThr, cLr, cThr, cMe, cCnt;
/* Run 883: g3_thread waits must not run while this thread holds the big lock (worker / ROM / alarm may need it). */
static __thread int bigDepth;
unsigned int sceKernelCpuSuspendIntr(void) { initBig(); if (!RecursiveLock_TryLock(&big)) { cLr = bigLr; cThr = bigThr; cMe = (unsigned)threadGetCurrent(); cCnt++; CRUMB("intrlock-wait"); RecursiveLock_Lock(&big); } bigLr = (unsigned)__builtin_return_address(0); bigTid = tlsId; bigThr = (unsigned)threadGetCurrent(); bigDepth++; return 1; }
void P3DS_BigLockInfo(char *b, int n) {
    int t = bigTid; if (t < 0 || t > MAXT) t = 0;
    int w = snprintf(b, n, "big lock last taken by thread slot %d (%s) from %p; threads:", t, pts[t].name[0] ? pts[t].name : (t ? "?" : "main"), (void *)bigLr);
    if (w > 0 && w < n - 120) w += snprintf(b + w, n - w, " | last take thread %08x | last WAIT: waiter thread %08x, holder was thread %08x taken from %08x (%u waits);", bigThr, cMe, cThr, cLr, cCnt);
    for (int i = 1; i <= MAXT && w > 0 && w < n - 40; i++) if (pts[i].used) w += snprintf(b + w, n - w, " [%d %s%s]", i, pts[i].name, pts[i].started ? "" : " (not started)");
}
void sceKernelCpuResumeIntr(unsigned int f) { (void)f; if (bigDepth > 0) bigDepth--; RecursiveLock_Unlock(&big); }
/* Drop every recursive hold on this thread before a long wait; restore afterward. Returns depth released. */
int P3DS_BigLockDropForWait(void) {
    int d = 0; initBig();
    while (bigDepth > 0) { sceKernelCpuResumeIntr(1); d++; }
    return d;
}
void P3DS_BigLockRestoreAfterWait(int depth) {
    while (depth-- > 0) (void)sceKernelCpuSuspendIntr();
}
int P3DS_BigLockDepth(void) { return bigDepth; }
int sceKernelSuspendDispatchThread(void) { return (int)sceKernelCpuSuspendIntr(); }
int sceKernelResumeDispatchThread(int s) { sceKernelCpuResumeIntr((unsigned)s); return 0; }
/* PSP prio 0x20 (= PSP main thread) maps to 0x30 (3DS main thread); DS worker threads (0x20+p) land at or below
   the game thread so a spinning worker can never starve it (run-5 overworld freeze). */
static int toCtrPrio(int psp) { int p = 0x30 + (psp - 0x20) / 4; if (p < 0x28) p = 0x28; if (p > 0x3F) p = 0x3F; return p; }
SceUID sceKernelCreateThread(const char *n, SceKernelThreadEntry e, int prio, int stack, SceUInt a, void *o) {
    (void)a; (void)o; initBig();
    for (int i = 1; i <= MAXT; i++) if (!pts[i].used) { memset(&pts[i], 0, sizeof pts[i]); pts[i].used = 1; snprintf(pts[i].name, sizeof pts[i].name, "psp:%s", n ? n : "?"); pts[i].entry = e; pts[i].prio = prio; pts[i].core = (n && (strstr(n, "audio") || strstr(n, "sas"))) ? 2 : -2; pts[i].stack = stack < 0x8000 ? 0x8000 : stack; LightSemaphore_Init(&pts[i].wake, 0, 0x7fff); return i + 1; }
    return -1;
}
static void trampoline(void *p) { PT *t = p; P3DS_ThreadGuard(t->name); tlsId = (int)(t - pts); t->entry(t->len, t->len ? t->argbuf : NULL); }
int sceKernelStartThread(SceUID id, SceSize len, void *argp) {
    PT *t = &pts[id - 1]; if (len > sizeof t->argbuf) return -1; t->len = len; if (len) memcpy(t->argbuf, argp, len);
    t->th = threadCreate(trampoline, t, t->stack, toCtrPrio(t->prio), t->core, false); if (!t->th && t->core != -2) { t->th = threadCreate(trampoline, t, t->stack, toCtrPrio(t->prio), -2, false); } t->started = 1;
    if (!t->th) { char b[300]; resLimitStr(b, sizeof b);
        PSPNativeMemLog("[3DS] thread start FAILED: %s (core %d, prio 0x%x, stack %d) | this process %lld threads | %s", t->name, t->core, toCtrPrio(t->prio), t->stack, (long long)procInfo(7), b);
        if (strstr(t->name, "sas") || strstr(t->name, "audio")) PSPNativeMemLog("[AUDIO] sound thread %s could not start: no sound this boot (ndspInit will never run)", t->name); }
    return t->th ? 0 : -1;
}
int sceKernelExitThread(int s) { (void)s; threadExit(0); return 0; }
int sceKernelWaitThreadEnd(SceUID id, SceUInt *to) { (void)to; PT *t = &pts[id - 1]; if (t->th) threadJoin(t->th, U64_MAX); return 0; }
int sceKernelDeleteThread(SceUID id) {
    PT *t = &pts[id - 1];
    if (t->th) {
        if (threadJoin(t->th, 0) != 0) return -1;
        threadFree(t->th);
        t->th = NULL;
    }
    t->used = 0;
    return 0;
}
SceUID sceKernelGetThreadId(void) { return tlsId + 1; }
int sceKernelSleepThread(void) { CRUMB("sleep"); initBig(); quitCheck(); LightSemaphore_Acquire(&pts[tlsId].wake, 1); quitCheck(); return 0; }
int sceKernelWakeupThread(SceUID id) { LightSemaphore_Release(&pts[id - 1].wake, 1); return 0; }
int sceKernelDelayThread(SceUInt us) { CRUMB("delay"); while (us) { SceUInt part = us > 10000 ? 10000 : us; svcSleepThread((s64)part * 1000); us -= part; quitCheck(); } return 0; }
int sceKernelChangeThreadPriority(SceUID id, int p) { pts[id - 1].prio = p; if (pts[id - 1].th) svcSetThreadPriority(threadGetHandle(pts[id - 1].th), toCtrPrio(p)); return 0; }
int sceKernelRotateThreadReadyQueue(int p) { CRUMB("yield"); (void)p; svcSleepThread(50000); quitCheck(); return 0; } /* real yield: 3DS sleep(0) never runs lower-priority threads */
/* 2026-11-17 cloud profile: the double divide here (u64 -> double -> divide -> u64, all soft-float helpers) was ~2% of busy
   CPU time. Same value from integers: ticks * 1e6 / 268111856 = ticks * 62500 / 16756991, done as the high 64 bits of
   ticks * K (K = 2^64 * 62500 / 16756991, rounded up). Monotonic; at most 1 us off the old result. */
static inline u64 mulhi64(u64 a, u64 b) {
  const u64 al = (u32)a, ah = a >> 32, bl = (u32)b, bh = b >> 32, ll = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
  const u64 mid = (ll >> 32) + (u32)lh + (u32)hl; return hh + (lh >> 32) + (hl >> 32) + (mid >> 32); }
SceInt64 sceKernelGetSystemTimeWide(void) { return (SceInt64)mulhi64(svcGetSystemTick(), 0xf46f6f57cb8727ULL); }
unsigned int sceKernelGetSystemTimeLow(void) { return (unsigned)sceKernelGetSystemTimeWide(); }
/* Semaphores */
typedef struct { LightSemaphore s; int used; } PS; static PS sems[64];
SceUID sceKernelCreateSema(const char *n, SceUInt a, int init, int max, void *o) { (void)n; (void)a; (void)o; for (int i = 0; i < 64; i++) if (!sems[i].used) { sems[i].used = 1; LightSemaphore_Init(&sems[i].s, init, max); return i + 1; } return -1; }
int sceKernelDeleteSema(SceUID id) { sems[id - 1].used = 0; return 0; }
int sceKernelWaitSema(SceUID id, int n, SceUInt *to) { CRUMB("waitsema");
    quitCheck(); if (!to) { LightSemaphore_Acquire(&sems[id - 1].s, n); quitCheck(); return 0; }
    u64 end = svcGetSystemTick() + (u64)*to * CPU_TICKS_PER_USEC;
    while (LightSemaphore_TryAcquire(&sems[id - 1].s, n)) { if (svcGetSystemTick() >= end) return -1; svcSleepThread(100000); }
    return 0;
}
int sceKernelSignalSema(SceUID id, int n) { LightSemaphore_Release(&sems[id - 1].s, n); return 0; }
/* Alarms: one 1 ms-tick service thread. */
typedef struct { SceKernelAlarmHandler h; void *c; u64 due; int used; } PA; static PA alarms[32]; static Thread alarmTh;
static void alarmLoop(void *p) { (void)p; P3DS_ThreadGuard("alarms"); while (!p3ds_quit) { svcSleepThread(1000000); u64 now = svcGetSystemTick();
    for (int i = 0; i < 32; i++) { sceKernelCpuSuspendIntr(); PA a = alarms[i]; int fire = a.used && now >= a.due; sceKernelCpuResumeIntr(1);
        if (!fire) continue; SceUInt next = a.h(a.c); sceKernelCpuSuspendIntr();
        if (alarms[i].used && alarms[i].h == a.h && alarms[i].due == a.due) { if (next) alarms[i].due = svcGetSystemTick() + (u64)next * CPU_TICKS_PER_USEC; else alarms[i].used = 0; }
        sceKernelCpuResumeIntr(1); } } }
SceUID sceKernelSetAlarm(SceUInt us, SceKernelAlarmHandler h, void *c) {
    sceKernelCpuSuspendIntr(); if (!alarmTh) alarmTh = threadCreate(alarmLoop, NULL, 0x4000, 0x18, -2, false);
    for (int i = 0; i < 32; i++) if (!alarms[i].used) { alarms[i] = (PA){ h, c, svcGetSystemTick() + (u64)us * CPU_TICKS_PER_USEC, 1 }; sceKernelCpuResumeIntr(1); return i + 1; }
    sceKernelCpuResumeIntr(1); return -1;
}
int sceKernelCancelAlarm(SceUID id) { sceKernelCpuSuspendIntr(); alarms[id - 1].used = 0; sceKernelCpuResumeIntr(1); return 0; }
void sceKernelDcacheWritebackAll(void) {}
void sceKernelDcacheWritebackRange(const void *p, unsigned s) { GSPGPU_FlushDataCache(p, s); }
void sceKernelDcacheWritebackInvalidateRange(const void *p, unsigned s) { GSPGPU_FlushDataCache(p, s); }
void sceKernelDcacheInvalidateRange(const void *p, unsigned s) { GSPGPU_InvalidateDataCache(p, s); }
void sceKernelExitGame(void) { exit(0); }
/* Partition memory -> linear heap; audio stubs. */
#include <pspaudio.h>
#include <pspsascore.h>
#include <psputility.h>
/* run 33: was 64 blocks. The SAS sample cache (384 slots, 3 MiB) allocates one block per converted sample, so after
   64 distinct samples every new one failed -> voice skipped -> missing battle SEs. */
#define P3DS_BLOCKS 1024
static void *blocks[P3DS_BLOCKS]; unsigned P3DS_BlockFailFull, P3DS_BlockFailOom, P3DS_BlockHigh;
SceUID sceKernelAllocPartitionMemory(SceUID part, const char *n, int t, SceSize size, void *a) { (void)part; (void)n; (void)t; (void)a; for (int i = 0; i < P3DS_BLOCKS; i++) if (!blocks[i]) { blocks[i] = memalign(64, size); if (!blocks[i]) { P3DS_BlockFailOom++; return -1; } if ((unsigned)i + 1 > P3DS_BlockHigh) P3DS_BlockHigh = i + 1; return i + 1; } P3DS_BlockFailFull++; return -1; }
void *sceKernelGetBlockHeadAddr(SceUID id) { return blocks[id - 1]; }
int sceKernelFreePartitionMemory(SceUID id) { free(blocks[id - 1]); blocks[id - 1] = NULL; return 0; }
int sceKernelPollSema(SceUID id, int n) { return LightSemaphore_TryAcquire(&sems[id - 1].s, n) ? -1 : 0; }
void sceKernelDcacheWritebackInvalidateAll(void) {}
int sceUtilityLoadModule(int id) { (void)id; return 0; }
int sceUtilityUnloadModule(int id) { (void)id; return 0; }
void PSPNativeMemLog(const char *, ...);
/* ---- sceSasCore on 3DS: 16 PCM voices (s16 mono, loop start or -1, pitch 0x1000 = 1.0, vol 0..0x1000),
   mixed per grain; sceAudioOutputPannedBlocking queues grains to ndsp (needs sdmc:/3ds/dspfirm.cdc). ---- */
typedef struct { const s16 *d; u32 n; int loop; u32 pos, frac; int pitch, vl, vr, on, pl, pr; } SasV; /* pl/pr: volume at end of last grain (run 39 ramp) */
static SasV sasv[16]; static int sasGrain = 256;
/* run 40: mixer/output rate. 32728 = 3DS DSP native rate (no ndsp resample); audio_rate=44100 = old. Read at first use. */
int P3DS_SasRate = 32728; static int sasRateRead;
static void sasRateInit(void) { if (sasRateRead) return; sasRateRead = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) if (sscanf(l, "audio_rate=%d", &x) == 1 && x >= 16000 && x <= 48000) P3DS_SasRate = x; fclose(f); } PSPNativeMemLog("[AUDIO] audio_rate=%d", P3DS_SasRate); }
int __sceSasInit(void *c, int g, int v, int m, int r) { (void)c; sasRateInit(); (void)v; (void)m; (void)r; sasGrain = g > 0 && g <= 2048 ? g : 256; memset(sasv, 0, sizeof sasv); return 0; }
int __sceSasSetVoicePCM(void *c, int v, void *b, int s, int l) { (void)c; if (v < 0 || v > 15) return -1; SasV *x = &sasv[v]; x->on = 0; x->d = b; x->n = s > 0 ? (u32)s : 0; x->loop = l; x->pos = x->frac = 0; return 0; }
int __sceSasSetPitch(void *c, int v, int p) { (void)c; if (v >= 0 && v < 16) sasv[v].pitch = p; return 0; }
int __sceSasSetVolume(void *c, int v, int l, int r, int el, int er) { (void)c; (void)el; (void)er; if (v >= 0 && v < 16) { sasv[v].vl = l; sasv[v].vr = r; } return 0; }
int __sceSasSetKeyOn(void *c, int v) { (void)c; if (v >= 0 && v < 16) { sasv[v].pos = sasv[v].frac = 0; sasv[v].pl = -1; sasv[v].on = sasv[v].d && sasv[v].n; } return 0; }
int __sceSasSetKeyOff(void *c, int v) { (void)c; if (v >= 0 && v < 16) sasv[v].on = 0; return 0; }
int __sceSasSetADSR(void *c, int v, int f, unsigned a, unsigned d, unsigned s, unsigned r) { return 0; }
int __sceSasSetADSRmode(void *c, int v, int f, int a, int d, int s, int r) { return 0; }
int __sceSasSetSL(void *c, int v, int l) { return 0; }
/* run 121 (from SoulSilver): live audio settings + counters. audio_limiter 0..3 = OFF / CLEAN / LIGHT / DS RAW. */
int P3DS_CfgInt(const char *, int);
volatile int P3DS_AudioGainV = -1, P3DS_AudioLimiterV = -1, P3DS_AudioStallFadeV = -1;
volatile unsigned P3DS_SndPumps, P3DS_AudioStallFades, P3DS_AudioDspStalls, P3DS_AudioMaxWaitMs, P3DS_AudioClipTotal;
volatile int P3DS_AudioAptPaused, P3DS_AudioWoke, P3DS_AudioWokeDsp;
void __real_PSPNativeSasSnapshot(u32 cycles);
void __wrap_PSPNativeSasSnapshot(u32 cycles) { P3DS_SndPumps++; __real_PSPNativeSasSnapshot(cycles); }
void P3DS_SetAudioGain(int g) { if (g > 0 && g <= 64) { P3DS_AudioGainV = g; PSPNativeMemLog("[AUDIO] audio_gain=%d (from settings)", g); } }
void P3DS_SetAudioLimiter(int on) { P3DS_AudioLimiterV = on < 0 ? 0 : on > 3 ? 3 : on; PSPNativeMemLog("[AUDIO] audio_limiter=%d (0 off, 1 clean, 2 light, 3 DS raw)", P3DS_AudioLimiterV); }
void P3DS_SetAudioStallFade(int on) { P3DS_AudioStallFadeV = on != 0; PSPNativeMemLog("[AUDIO] audio_stallfade=%d (from settings)", on != 0); }
int __sceSasCore(void *c, void *out) {
    (void)c; static int acc[2 * 2048]; memset(acc, 0, sasGrain * 2 * sizeof(int));
    if (P3DS_AudioGainV < 0) { int g0 = 10; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) if (sscanf(l, "audio_gain=%d", &x) == 1 && x > 0 && x <= 64) g0 = x; fclose(f); } P3DS_AudioGainV = g0; PSPNativeMemLog("[AUDIO] audio_gain=%d", g0); }
    const int g = P3DS_AudioGainV;
    static int vr_ = -1, ip_ = -1; if (vr_ < 0) { vr_ = 1; ip_ = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) { if (sscanf(l, "audio_volramp=%d", &x) == 1) vr_ = x != 0; if (sscanf(l, "audio_interp=%d", &x) == 1) ip_ = x != 0; } fclose(f); } }
    if (P3DS_AudioLimiterV < 0) { int l0 = 3; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) if (sscanf(l, "audio_limiter=%d", &x) == 1) l0 = x < 0 ? 0 : x > 3 ? 3 : x; fclose(f); } P3DS_AudioLimiterV = l0; PSPNativeMemLog("[AUDIO] audio_limiter=%d (0 off, 1 clean, 2 light, 3 DS raw)", l0); }
    const int ipOn = ip_ && P3DS_AudioLimiterV != 3;
    const int G = sasGrain;
    for (int v = 0; v < 16; v++) { SasV *x = &sasv[v]; if (!x->on) continue;
        u32 step = (u32)x->pitch;
        if (x->pl < 0 || !vr_) { x->pl = x->vl; x->pr = x->vr; }
        int l0 = x->pl, r0 = x->pr, sl = ((x->vl - l0) << 16) / G, sr = ((x->vr - r0) << 16) / G;
        for (int i = 0; i < G; i++) {
            if (x->pos >= x->n) { if (x->loop >= 0 && (u32)x->loop < x->n) x->pos = (u32)x->loop + (x->pos - x->n) % (x->n - (u32)x->loop); else { x->on = 0; break; } }
            int smp = x->d[x->pos];
            if (ipOn && x->frac) { u32 q = x->pos + 1; if (q >= x->n) q = (x->loop >= 0 && (u32)x->loop < x->n) ? (u32)x->loop : x->pos; smp += ((x->d[q] - smp) * (int)x->frac) >> 12; }
            int vl = l0 + ((sl * (i + 1)) >> 16), vrr = r0 + ((sr * (i + 1)) >> 16);
            acc[2 * i] += (smp * vl * g) >> 13; acc[2 * i + 1] += (smp * vrr * g) >> 13;
            x->frac += step; x->pos += x->frac >> 12; x->frac &= 0xFFF;
        }
        x->pl = x->vl; x->pr = x->vr; }
    static int lg = 65536; const int lim_ = P3DS_AudioLimiterV == 3 ? 0 : P3DS_AudioLimiterV; if (!lim_) lg = 65536;
    static unsigned lastPump, holdRun; static int sf = 0;
    if (P3DS_AudioStallFadeV < 0) { P3DS_AudioStallFadeV = P3DS_CfgInt("audio_stallfade", 1) != 0; PSPNativeMemLog("[AUDIO] audio_stallfade=%d", P3DS_AudioStallFadeV); }
    { const unsigned pn = P3DS_SndPumps; if (pn != lastPump) { if (holdRun >= 64 && P3DS_AudioStallFadeV) PSPNativeMemLog("[AUDIO] stall fade: game sound resumed after %u ms", holdRun * (unsigned)G * 1000u / (unsigned)P3DS_SasRate); lastPump = pn; holdRun = 0; } else if (holdRun < 0x7fffffff) holdRun++;
      if (holdRun == 64 && P3DS_AudioStallFadeV) { P3DS_AudioStallFades++; PSPNativeMemLog("[AUDIO] stall fade: no pump for %u ms, fading out", 64u * (unsigned)G * 1000u / (unsigned)P3DS_SasRate); } }
    if (P3DS_AudioWoke) { P3DS_AudioWoke = 0; sf = 0; PSPNativeMemLog("[AUDIO] wake/restore: fading in, gain %d limiter %d", g, P3DS_AudioLimiterV); }
    const int sf0 = sf; if (P3DS_AudioStallFadeV && holdRun >= 64) { sf -= 65536 / 16; if (sf < 0) sf = 0; } else if (sf < 65536) { sf += 65536 / 32; if (sf > 65536) sf = 65536; }
    s16 *o = out; int pk = 0; for (int i = 0; i < G * 2; i++) { int s = acc[i]; if (s > pk) pk = s; if (-s > pk) pk = -s; }
    const int lg0 = lim_ ? lg : 65536;
    if (lim_) { static int ceil_ = -1; if (ceil_ < 0) { ceil_ = P3DS_CfgInt("audio_ceiling", 14000); if (ceil_ < 4000 || ceil_ > 24576) ceil_ = 14000; PSPNativeMemLog("[AUDIO] audio_ceiling=%d", ceil_); }
      const int cl_ = lim_ == 1 ? ceil_ : 24576;
      int need = pk > cl_ ? (int)((long long)cl_ * 65536 / pk) : 65536; if (need < lg) lg = need; else { lg += (65536 - lg) / 16; if (lg > need) lg = need; } }
    int opk = 0; unsigned kneeS = 0, hardS = 0;
    const int kn_ = P3DS_AudioLimiterV == 3 ? 32767 : 24576;
    for (int i = 0; i < G * 2; i++) { int s = acc[i];
        if (lg0 != 65536 || lg != 65536) { int gg = lg0 + (int)(((long long)(lg - lg0) * (i >> 1)) / G); s = (int)(((long long)s * gg) >> 16); }
        if (sf0 != 65536 || sf != 65536) { int ff = sf0 + (int)(((long long)(sf - sf0) * (i >> 1)) / G); s = (int)(((long long)s * ff) >> 16); }
        int m = s < 0 ? -s : s; if (m > kn_) { kneeS++; int ex = m - kn_; m = kn_ + ex * 8192 / (ex + 8192); if (m > 32767) { m = 32767; hardS++; } s = s < 0 ? -m : m; } if (m > opk) opk = m; o[i] = (s16)s; }
    { static int gN, mpk, mon, mvl, mopk, minLg = 65536; static unsigned clipG, kneeG, kneeT, hardT, fadeG; gN++; if (pk > 24576) clipG++; if (pk > mpk) mpk = pk; if (opk > mopk) mopk = opk; if (kneeS) kneeG++; kneeT += kneeS; hardT += hardS; if (lg < minLg) minLg = lg; if (sf0 < 65536 || sf < 65536) fadeG++;
      int on = 0; for (int v = 0; v < 16; v++) if (sasv[v].on) { on++; if (sasv[v].vl > mvl) mvl = sasv[v].vl; } if (on > mon) mon = on;
      if (gN % 1720 == 0) { PSPNativeMemLog("[AUDIO] 10s: peak in %d out %d | over 24576 before %u after %u (%u samples, %u rail) | lim min %d%% (style %d gain %d) | fades %u voices %d vl %d | dsp stalls %u max wait %u ms",
          mpk, mopk, clipG, kneeG, kneeT, hardT, minLg * 100 / 65536, lim_, g, fadeG, mon, mvl, P3DS_AudioDspStalls, P3DS_AudioMaxWaitMs);
        P3DS_AudioClipTotal += kneeG; mpk = mon = mvl = mopk = 0; minLg = 65536; clipG = kneeG = kneeT = hardT = fadeG = 0; P3DS_AudioMaxWaitMs = 0; } }
    return 0;
}
int __sceSasGetAllEnvelopeHeights(void *c, int *h) { (void)c; for (int v = 0; v < 32; v++) h[v] = v < 16 && sasv[v].on ? 0x40000000 : 0; return 0; }
#define NWB 16  /* 16 x 256 = 4096 samples (93 ms) of DSP lead */
static ndspWaveBuf wb[NWB]; static s16 *wbData; static int ndspOk = -1, wbNext;
/* 2026-11-05 (Nathan: "when waves sound I hear the music dim"): libctru starts the DSP in NDSP_CLIP_SOFT, which is the DSP's
   own output LIMITER (gain riding on the whole mix; Azahar does not emulate it, so only real hardware pumped). A loud SE such as
   the sea waves raised the mix level and the limiter pulled everything down, music included. The DS has no such stage: its
   mixer just clamps to s16. Audio style DS RAW (audio_limiter=3, the default) now runs the DSP in NDSP_CLIP_NORMAL (plain clamp);
   our mixer already clamps to s16 before the DSP, so the samples reaching the speaker are now exactly what we mixed.
   Styles OFF/CLEAN/LIGHT keep the old soft (limiter) mode for an A/B in Settings > Debug > Audio style. */
static void ndspApplyClip(const char *why) {
    if (ndspOk != 1) return; int st = P3DS_AudioLimiterV; if (st < 0) st = P3DS_CfgInt("audio_limiter", 3);
    ndspClippingMode m = st == 3 ? NDSP_CLIP_NORMAL : NDSP_CLIP_SOFT; static int last = -1;
    ndspSetClippingMode(m); if ((int)m != last) { last = (int)m; PSPNativeMemLog("[AUDIO] dsp clipping %s (%s, audio style %d)", m == NDSP_CLIP_NORMAL ? "NORMAL: plain clamp like the DS, no DSP limiter" : "SOFT: DSP output limiter (rides the whole mix)", why, st); } }
/* 2026-11-20 (Nathan: sound gone after a soft reset on his other 3DS): the relaunched process can start while the old one still
   holds the DSP (or exited without ndspExit), so one failed ndspInit used to disable audio for the whole session. Now a failure is
   retried (0.25 s, 0.5 s, 0.75 s, then about every 1.5 s, up to 20 tries) and every try is logged. ndspOk: -1 not started, 0 failed
   (retry later), 1 ready, 2 shut down.
   2026-11-22 (sound still died after 7 starter soft resets with #577): #577's "reclaim" called ndspExit() after a FAILED ndspInit().
   libctru's ndspInit already undoes its own reference on failure (ndsp.c _fail0: AtomicDecrement(&ndspRefCount)), so that ndspExit
   drove ndspRefCount to -1 and the retry's ndspInit saw a non-zero count (AtomicPostIncrement) and returned 0 WITHOUT starting the
   DSP: "ndsp ready" was logged, every buffer queued, nothing played, for the whole boot. Never call ndspExit unless OUR ndspInit
   succeeded; a failed init is simply retried (libctru's dspInit force-unloads any component a dead process left behind). */
static volatile u64 ndspInitT0; static volatile int ndspInitBusy; static u32 ndspFrames0; static u64 ndspReadyTick; static int ndspAlive = -1;
static volatile int audioOutCalls; /* sceAudioOutputPannedBlocking calls (the sound thread is alive and feeding grains) */
void P3DS_AudioDiag(const char *when);
static void ndspSetup(void) {
    static int tries; static u64 nextTry;
    if (ndspOk == 1 || ndspOk == 2) return;
    if (ndspOk == 0) { if (tries >= 20 || svcGetSystemTick() < nextTry) return; }
    ndspInitT0 = svcGetSystemTick(); ndspInitBusy = 1;
    Result r = ndspInit(); tries++;
    ndspInitBusy = 0;
    const unsigned initMs = (unsigned)((svcGetSystemTick() - ndspInitT0) / CPU_TICKS_PER_MSEC);
    if (R_FAILED(r)) { ndspOk = 0; nextTry = svcGetSystemTick() + (tries <= 3 ? (u64)SYSCLOCK_ARM11 * tries / 4 : (u64)SYSCLOCK_ARM11 * 3 / 2);
        PSPNativeMemLog("[AUDIO] ndspInit failed %08lx after %u ms (try %d/20%s): copy your console's DSP firmware to sdmc:/3ds/dspfirm.cdc (run 'DSP1' homebrew once) if this never succeeds.", r, initMs, tries, tries < 20 ? ", retrying" : ", giving up, audio disabled");
        return; }
    if (tries > 1) PSPNativeMemLog("[AUDIO] ndspInit ok on try %d in %u ms (the DSP was still busy right after the reset)", tries, initMs);
    ndspFrames0 = ndspGetFrameCount(); ndspReadyTick = svcGetSystemTick(); ndspAlive = -1;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO); sasRateInit(); ndspChnSetInterp(0, P3DS_SasRate == 32728 ? NDSP_INTERP_NONE : NDSP_INTERP_LINEAR); ndspChnSetRate(0, P3DS_SasRate == 32728 ? 32728.498f : (float)P3DS_SasRate); ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    float mix[12] = {1.0f, 1.0f}; ndspChnSetMix(0, mix);
    if (wbData) { linearFree(wbData); wbData = NULL; } /* defensive: never leak a prior linearAlloc if init is re-entered */
    wbData = linearAlloc(NWB * sasGrain * 4); memset(wb, 0, sizeof wb);
    for (int i = 0; i < NWB; i++) { wb[i].data_vaddr = wbData + i * sasGrain * 2; wb[i].nsamples = sasGrain; wb[i].status = NDSP_WBUF_DONE; }
    ndspOk = 1; wbNext = 0; ndspApplyClip("init"); PSPNativeMemLog("[AUDIO] ndsp ready: %d Hz stereo", P3DS_SasRate); PSPNativeMemLog("[AUDIO] buffers, %d x %d-sample buffers", NWB, sasGrain);
}
int sceAudioChReserve(int ch, int s, int f) { (void)s; (void)f; return ch < 0 ? 0 : ch; }
int sceAudioChRelease(int ch) { (void)ch; return 0; }
/* 2026-11-22: prove the DSP really runs after ndspInit. libctru's ndsp thread only counts a frame when the DSP raises its
   interrupt, so a frame count that has not moved 2 s after a successful init means the DSP never started (sound dead for this
   boot). Log only: nothing is restarted or reset. */
static void ndspLiveCheck(void) {
    if (ndspAlive >= 0 || P3DS_AudioAptPaused) return;
    const u64 now = svcGetSystemTick(), age = now - ndspReadyTick; const u32 f = ndspGetFrameCount();
    if (age < (u64)SYSCLOCK_ARM11 / 2) return;
    if (f != ndspFrames0) { ndspAlive = 1; return; }
    if (age > (u64)SYSCLOCK_ARM11 * 2) { ndspAlive = 0; PSPNativeMemLog("[AUDIO] DSP NOT RUNNING: 0 DSP frames %u ms after a successful ndspInit (no sound this boot)", (unsigned)(age / CPU_TICKS_PER_MSEC)); P3DS_AudioDiag("dsp dead"); }
}
static int audioOutImpl(void *b);
int sceAudioOutputPannedBlocking(int ch, int l, int r, void *b) {
    (void)ch; (void)l; (void)r; quitCheck();
    audioOutCalls++;
    return audioOutImpl(b);
}
static int audioOutImpl(void *b) {
    ndspSetup();
    if (ndspOk != 1) { svcSleepThread((s64)sasGrain * 1000000000LL / P3DS_SasRate); return 0; }
    ndspLiveCheck();
    /* run 46 audio_lead=N (default 8 = 62 ms, was 16 = 125 ms of DSP lead): with the 83 ms snapshot latency the total
       sound delay drops from ~250 ms to ~145 ms (Poketch / menu taps felt late). 16 = old. */
    static int lead = -1; if (lead < 0) { lead = 8; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) if (sscanf(l, "audio_lead=%d", &x) == 1 && x >= 4 && x <= NWB) lead = x; fclose(f); } PSPNativeMemLog("[AUDIO] audio_lead=%d buffers (%d ms)", lead, lead * sasGrain * 1000 / P3DS_SasRate); }
    if (P3DS_AudioWokeDsp) { P3DS_AudioWokeDsp = 0; float mx[12] = {1.0f, 1.0f}; ndspSetOutputMode(NDSP_OUTPUT_STEREO); ndspSetMasterVol(1.0f); ndspChnSetMix(0, mx); ndspChnSetRate(0, P3DS_SasRate == 32728 ? 32728.498f : (float)P3DS_SasRate); ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16); ndspApplyClip("wake"); PSPNativeMemLog("[AUDIO] wake: DSP master volume, mix, rate re-applied"); }
    if (wbNext >= lead) wbNext = 0;
    ndspWaveBuf *w = &wb[wbNext];
    if (P3DS_AudioWokeDsp) { P3DS_AudioWokeDsp = 0; float mx[12] = {1.0f, 1.0f}; ndspSetOutputMode(NDSP_OUTPUT_STEREO); ndspSetMasterVol(1.0f); ndspChnSetMix(0, mx); ndspChnSetRate(0, P3DS_SasRate == 32728 ? 32728.498f : (float)P3DS_SasRate); ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16); PSPNativeMemLog("[AUDIO] wake: DSP master volume, mix, rate re-applied"); }
    /* 2026-10-26 DSP watchdog: this wait used to be unbounded. If the DSP stops consuming (it did after some HOME /
       sleep round trips on other homebrew), the audio thread spun here forever and every sound froze. Now a buffer
       that stays queued for 1 s while no HOME/sleep is in progress clears the channel queue and restarts output
       (logged as a "dsp stall"; one short gap instead of permanent silence). */
    { u64 t0 = svcGetSystemTick(); u32 lastPos = ndspChnGetSamplePos(0); u64 tPos = t0;
      while (w->status != NDSP_WBUF_DONE && w->status != NDSP_WBUF_FREE) { if (p3ds_quit) { quitCheck(); return 0; } svcSleepThread(1000000);
        u64 now = svcGetSystemTick(); u32 pos = ndspChnGetSamplePos(0); if (pos != lastPos || P3DS_AudioAptPaused) { lastPos = pos; tPos = now; }
        if (now - tPos > (u64)SYSCLOCK_ARM11) { P3DS_AudioDspStalls++;
            static u32 stallFrames; const u32 fc = ndspGetFrameCount();
            PSPNativeMemLog("[AUDIO] dsp stall: buffer %d still queued after %u ms (sample pos %lu frozen), clearing the channel (stalls %u, DSP frames %lu, +%lu since the last stall)", wbNext, (unsigned)((now - t0) / CPU_TICKS_PER_MSEC), (unsigned long)pos, P3DS_AudioDspStalls, (unsigned long)fc, (unsigned long)(fc - stallFrames));
            stallFrames = fc; ndspLiveCheck();
            ndspChnWaveBufClear(0); for (int i = 0; i < NWB; i++) wb[i].status = NDSP_WBUF_DONE; wbNext = 0; w = &wb[0]; break; } }
      unsigned ms = (unsigned)((svcGetSystemTick() - t0) / CPU_TICKS_PER_MSEC); if (ms > P3DS_AudioMaxWaitMs) P3DS_AudioMaxWaitMs = ms; }
    memcpy((void *)w->data_vaddr, b, sasGrain * 4); DSP_FlushDataCache(w->data_vaddr, sasGrain * 4);
    ndspChnWaveBufAdd(0, w); wbNext = (wbNext + 1) % lead; return 0;
}
volatile int P3DS_SumOpen; /* ribbons: summary screen open (needs glue hook; stays 0 until ported) */
int P3DS_InBattle; /* Platinum: set from the catch snapshot in render3ds.cpp */

/* Release DSP state only after the game workers have left their audio calls.
   A failed ndspInit balances its own refcount in libctru. */
void P3DS_AudioShutdown(int gameThreadsStopped) {
    if (!gameThreadsStopped || ndspInitBusy) {
        PSPNativeMemLog("[AUDIO] shutdown: game worker did not stop; keeping DSP state for process exit");
        return;
    }
    const int was = ndspOk;
    ndspOk = 2;
    if (was == 1) {
        ndspChnWaveBufClear(0);
        ndspChnReset(0);
        ndspChnReset(1);
        ndspExit();
    }
    if (wbData) { linearFree(wbData); wbData = NULL; }
    memset(wb, 0, sizeof wb); wbNext = 0;
    if (was == 1) {
        PSPNativeMemLog("[AUDIO] shutdown: DSP released before exit (buffers freed)");
        svcSleepThread(100000000LL); /* allow DSP release before APT chainload */
    }
}
/* 2026-11-22 audio diagnostics (one line per call, cheap): which boot this is since the console was powered on (counted in
   resetchain.tmp: the console's power-on time = now - uptime, so a power cycle starts again at 1), uptime, memory left in each
   region (BASE is where system modules such as dsp allocate, so a per-reset leak there shows up as a falling number), this
   process's handle and thread counts, and the DSP frame / dropped-frame counters. */
void P3DS_AudioDiag(const char *when) {
    static int bootN = -1;
    const u64 upS = svcGetSystemTick() / SYSCLOCK_ARM11;
    if (bootN < 0) { bootN = 1; long long on = (long long)time(NULL) - (long long)upS, prevOn = 0; int prevN = 0;
        FILE *f = fopen("sdmc:/3ds/platinum3ds/resetchain.tmp", "r"); if (f) { if (fscanf(f, "%lld %d", &prevOn, &prevN) != 2) prevN = 0; fclose(f); }
        if (prevN > 0 && llabs(on - prevOn) <= 5) bootN = prevN + 1;
        if ((f = fopen("sdmc:/3ds/platinum3ds/resetchain.tmp", "w"))) { fprintf(f, "%lld %d\n", bootN > 1 ? prevOn : on, bootN); fclose(f); } }
    s64 hc = -1, hp = -1, tc = -1; if (R_FAILED(svcGetProcessInfo(&hc, CUR_PROCESS_HANDLE, 4))) hc = -1; if (R_FAILED(svcGetProcessInfo(&hp, CUR_PROCESS_HANDLE, 5))) hp = -1; if (R_FAILED(svcGetProcessInfo(&tc, CUR_PROCESS_HANDLE, 7))) tc = -1;
    PSPNativeMemLog("[AUDIO] diag %s: boot %d since console power-on, uptime %llu s | linear free %u KiB | free KiB app %u sys %u base %u | handles %lld (peak %lld) threads %lld | ndsp ok %d, DSP frames %lu, dropped %lu",
        when, bootN, (unsigned long long)upS, (unsigned)(linearSpaceFree() >> 10), (unsigned)(osGetMemRegionFree(MEMREGION_APPLICATION) >> 10), (unsigned)(osGetMemRegionFree(MEMREGION_SYSTEM) >> 10), (unsigned)(osGetMemRegionFree(MEMREGION_BASE) >> 10),
        (long long)hc, (long long)hp, (long long)tc, ndspOk, (unsigned long)(ndspOk == 1 ? ndspGetFrameCount() : 0), (unsigned long)(ndspOk == 1 ? ndspGetDroppedFrames() : 0));
}
/* Game thread (input poll): log once if ndspInit has not returned after 4 s (DSP not answering). Log only: never resets. */
void P3DS_AudioInitWatch(void) {
    { static u64 t0; static int said; if (!t0) t0 = svcGetSystemTick();
      if (!said && !audioOutCalls && svcGetSystemTick() - t0 > (u64)SYSCLOCK_ARM11 * 10) { said = 1;
          PSPNativeMemLog("[AUDIO] no sound grain in 10 s: the sound thread never reached the audio output, so ndspInit was never tried"); P3DS_ThreadDiag("no sound grain"); P3DS_AudioDiag("no sound grain"); } }
    if (ndspInitBusy && svcGetSystemTick() - ndspInitT0 > (u64)SYSCLOCK_ARM11 * 4) { static int said; if (!said) { said = 1; PSPNativeMemLog("[AUDIO] ndspInit has not returned after 4 s (the DSP is not answering)"); P3DS_AudioDiag("ndspInit hung"); } }
}
/* Pad, RTC, MAC, power callbacks, sysmem, file I/O. */
#include <pspctrl.h>
#include <psprtc.h>
#include <pspwlan.h>
#include <psppower.h>
#include <pspsysmem.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
int sceCtrlSetSamplingCycle(int c) { return 0; }
int sceCtrlSetSamplingMode(int m) { return 0; }
/* run 53: tiny config helpers (read once per key) */
/* run 66 [CFG] guard: every P3DS_CfgInt call opens config.txt on the SD (several ms on hardware). Callers must cache;
   count calls + time and log once per ~10 s whenever any happened so a per-frame caller shows up in log.txt. */
static unsigned cfgCalls, cfgUs; static u64 cfgT;
static void cfgAcct(u64 t0) { u64 now = svcGetSystemTick(); cfgCalls++; cfgUs += (unsigned)((now - t0) / CPU_TICKS_PER_USEC);
    if (now - cfgT > 268111856ull * 10) { if (cfgT) PSPNativeMemLog("[CFG] %u config.txt reads in the last 10 s (%u us total)", cfgCalls, cfgUs); cfgT = now; cfgCalls = cfgUs = 0; } }
/* run 67: config.txt is cached in RAM. Overlay code keeps its "read once" statics in overlay data, which is reset every
   time the overlay is reloaded (map change, battle...), so those keys were re-read from the SD after every load
   (~13 ms each on hardware, 9-23 reads per 10 s in log_run66). Now only the first call (and the first call after
   P3DS_CfgWrite / saveScale changed the file) touches the SD; [CFG] counts those real file reads. */
static char cfgBuf[16384]; static int cfgLen = -1; static LightLock cfgLk; static int cfgLkInit;
/* Per-key result cache (perf pass 2026-11-14): P3DS_CfgInt is called many times a frame and used to scan the whole config text with strchr/strncmp
   every time (libc was ~40% of guest CPU in the cloud profile). Key -> {found value, table default, or absent}; cleared with the RAM copy. */
#define CFGC_N 256
static struct { char key[28]; unsigned char st; int v; } cfgc[CFGC_N]; /* st 0 empty, 1 value (file or table), 2 absent */
void P3DS_CfgInvalidate(void) { if (!cfgLkInit) { LightLock_Init(&cfgLk); cfgLkInit = 1; } LightLock_Lock(&cfgLk); cfgLen = -1; memset(cfgc, 0, sizeof cfgc); LightLock_Unlock(&cfgLk); }
/* Defaults for keys that are absent from config.txt (2026-10-28). They override the `def` a caller passes, so game-side hooks
   that read P3DS_CfgInt(key, 0) get them too. A key that IS in config.txt always wins. logring.c reads this table through P3DS_CfgDefault. */
static const struct { const char *key; int def; } kCfgDefaults[] = {
    /* Platinum sync (2026-11-21): the SoulSilver table minus its game-feature rows. Only platform / speed / display defaults are
       carried (the tuned 60 fps setup, GPU compose, sharp text); anything that changes Platinum's gameplay keeps the default its
       own code passes (OFF), so an existing Platinum config.txt behaves as before. A key that IS in config.txt always wins. */
    { "fps_counter", 1 },
    { "g3_wmode", 2 }, { "gpu_direct3d", 1 }, { "fps_tweaks", 7 }, { "dc_flush_skip", 1 },
    { "g3_packets", 1 }, { "g3_fast_bounds", 1 }, { "gpu_compose_b", 1 }, { "gpu_obj_affine", 1 }, { "gpu_b_fades", 1 },
    { "g3_sharp", 3 }, { "g3_smooth_dbg", 0 },
    { "fps60", 1 }, { "g3_thread", 1 }, { "fps60_pipe", 1 }, { "fps60_hold", 1 }, { "fps60_strips", 1 }, { "fps60_vram", 1 }, { "g3_xfcache", 1 },
    { "g3_wake", 1 }, { "g3_fastxform", 1 }, { "g3_thr_texfence", 2 }, { "g3_thr_texsafe", 1 }, { "g3_stereo_share", 1 }, { "present_pace", 1 },
    { "g3_front_ov", 1 }, { "g3_stereo_gpu", 1 }, { "g3_front_bg", 1 },
    { "sharp_text", 1 }, { "sharp_game_text", 1 }, { "wide_layouts", 1 },
    /* SoulSilver features that must stay OFF when the key is absent (mobile PC, Slot 2, Black 2 battle sprites) */
    { "mobile_pc", 0 }, { "mobile_pc_dev", 0 }, { "slot2", 0 }, { "b2_sprites", 0 },
    { "shiny_suite", 1 }, /* SoulSilver default */
    { "catch_overlay", 1 }, /* SoulSilver default */
    { "wild_vis", 1 }, /* SoulSilver default */
    { "wild_vis_alive", 1 }, /* SoulSilver default */
    { "wild_vis_cries", 1 }, /* SoulSilver default */
    { "wild_vis_shiny", 1 }, /* SoulSilver default */
    { "wild_vis_max", 4 }, /* SoulSilver default */
    { "ribbon_check", 1 }, /* SoulSilver default */
    { "celebrations", 1 }, /* SoulSilver default */
    { "summary_ivs", 1 }, /* SoulSilver default */
    { "hm_free", 1 }, /* SoulSilver default */
    { "repel_prompt", 1 }, /* SoulSilver default */
    { "select_reset", 1 }, /* SoulSilver default */
    { "fast_text", 2 }, /* SoulSilver default */
    { "stereo_strength", 0 }, /* SoulSilver default */
    { "zl_run", 1 }, /* SoulSilver default */
    { "hunt_counter", 1 }, /* SoulSilver default */
    { "hunt_stats", 1 }, /* SoulSilver default */
    { "reset_skip_intro", 1 }, /* SoulSilver default */
    { "wide_menus", 1 }, /* SoulSilver default */
    { "battery_warn", 1 }, /* SoulSilver default */
    { "tracker", 1 }, /* SoulSilver default */
    { "type_hint", 1 }, /* SoulSilver default */
    { "pm_shortcuts", 1 }, /* SoulSilver default */
    { "zr_item", 0 }, /* SoulSilver default */
    { "log_level", 1 }, /* SoulSilver default */
    { "battle_anim_speed", 0 }, /* SoulSilver default */
    { "dmg_check", 1 }, /* SoulSilver default */
    { "reusable_tms", 1 }, /* SoulSilver default */
    { "stereo_2d", 4 }, /* SoulSilver default */
    { "menu_audio", 1 }, /* SoulSilver default */
    { "gts_picker", 1 }, /* SoulSilver default */
    { "ambient_weather", 1 }, /* SoulSilver default */
    { "seasonal_trees", 1 }, /* SoulSilver default */
};
/* The default a missing key gets: this table if it lists the key, else the caller's `def`. logring.c reports with it, so the two cannot drift. */
int P3DS_CfgDefault(const char *key, int def) { for (unsigned i = 0; i < sizeof kCfgDefaults / sizeof kCfgDefaults[0]; i++) if (!strcmp(key, kCfgDefaults[i].key)) return kCfgDefaults[i].def; return def; }
int P3DS_CfgInt(const char *key, int def) {
    if (!cfgLkInit) { LightLock_Init(&cfgLk); cfgLkInit = 1; }
    LightLock_Lock(&cfgLk);
    if (cfgLen < 0) { u64 t0 = svcGetSystemTick(); FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); cfgLen = 0;
        if (f) { cfgLen = (int)fread(cfgBuf, 1, sizeof cfgBuf - 1, f); if (cfgLen < 0) cfgLen = 0; fclose(f); } cfgBuf[cfgLen] = 0; cfgAcct(t0);
        PSPNativeMemLog("[CFG] config.txt loaded into RAM (%d bytes, %u us)", cfgLen, (unsigned)((svcGetSystemTick() - t0) / CPU_TICKS_PER_USEC)); }
    size_t n = strlen(key); unsigned h = 2166136261u; for (size_t i = 0; i < n; i++) h = (h ^ (unsigned char)key[i]) * 16777619u;
    int cacheable = n < sizeof cfgc[0].key; unsigned slot = h & (CFGC_N - 1), probe = 0;
    if (cacheable) { while (probe < 8 && cfgc[(slot + probe) & (CFGC_N - 1)].st && strcmp(cfgc[(slot + probe) & (CFGC_N - 1)].key, key)) probe++;
        if (probe < 8) { slot = (slot + probe) & (CFGC_N - 1); if (cfgc[slot].st) { int r = cfgc[slot].st == 1 ? cfgc[slot].v : def; LightLock_Unlock(&cfgLk); return r; } } else cacheable = 0; }
    int v = def; const char *p = cfgBuf; int found = 0;
    while (*p) { const char *e = strchr(p, '\n'); if (!strncmp(p, key, n) && p[n] == '=') { v = atoi(p + n + 1); found = 1; } if (!e) break; p = e + 1; }
    int tbl = 0; if (!found) for (unsigned i = 0; i < sizeof kCfgDefaults / sizeof kCfgDefaults[0]; i++) if (!strcmp(key, kCfgDefaults[i].key)) { v = kCfgDefaults[i].def; tbl = 1; break; }
    if (cacheable) { memcpy(cfgc[slot].key, key, n + 1); cfgc[slot].st = (found || tbl) ? 1 : 2; cfgc[slot].v = v; }
    LightLock_Unlock(&cfgLk); return v; }
/* ss3ds: string value of one config.txt key (e.g. birthdays=10-27,03-14), from the same RAM cache as P3DS_CfgInt. Returns 1 if found. */
int P3DS_CfgStr(const char *key, char *out, int outN) {
    if (outN > 0) out[0] = 0; (void)P3DS_CfgInt(key, 0); /* makes sure the cache is loaded */
    LightLock_Lock(&cfgLk); int found = 0; size_t n = strlen(key); const char *p = cfgBuf;
    while (*p) { const char *e = strchr(p, '\n'); if (!strncmp(p, key, n) && p[n] == '=') { const char *v = p + n + 1; int i = 0;
            while (v[i] && v[i] != '\n' && v[i] != '\r' && i < outN - 1) { out[i] = v[i]; i++; } out[i] = 0; found = 1; }
        if (!e) break; p = e + 1; }
    LightLock_Unlock(&cfgLk); return found; }
/* run 54 cstick_mode: 0 off, 1 = flick taps the Poketch button (run 53), 2 = C-stick works like the D-pad (default) */
static int cstickV = -1;
int P3DS_CstickMode(void) { if (cstickV < 0) { cstickV = P3DS_CfgInt("cstick_mode", 2); if (cstickV < 0 || cstickV > 2) cstickV = 2; } return cstickV; }
void P3DS_SetCstickMode(int v) { cstickV = v < 0 || v > 2 ? 2 : v; }
/* run 55: write one key=value into config.txt (other lines kept, order kept; key appended if missing). */
void P3DS_CfgWrite(const char *key, int val) {
    static char buf[16384]; size_t n = 0, kn = strlen(key); int done = 0, full = 0; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r");
    if (f) { char line[256]; while (fgets(line, sizeof line, f)) { size_t l = strlen(line);
            if (!strncmp(line, key, kn) && line[kn] == '=') { if (done) continue; l = (size_t)snprintf(line, sizeof line, "%s=%d\n", key, val); done = 1; }
            if (n + l + 2 < sizeof buf) { memcpy(buf + n, line, l); n += l; if (l && line[l - 1] != '\n') buf[n++] = '\n'; } else full = 1; } fclose(f); }
    if (!done) { int w = snprintf(buf + n, sizeof buf - n, "%s=%d\n", key, val); if (w < 0 || (size_t)w >= sizeof buf - n) full = 1; else n += (size_t)w; }
    f = full ? NULL : fopen("sdmc:/3ds/platinum3ds/config.txt", "w"); int ok = f != NULL; if (f) { ok = fwrite(buf, 1, n, f) == n; if (fclose(f)) ok = 0; }
    P3DS_CfgInvalidate();
    if (ok) PSPNativeMemLog("[MENU] config.txt: %s=%d (saved)", key, val); else PSPNativeMemLog("[MENU] config.txt: %s=%d WRITE FAILED (setting lost on restart)", key, val); }
/* SS3DS mobile PC: config (default OFF) and unlock sidecar. Never writes the .sav. */
#include "ss3ds_mobilepc.h"
volatile int P3DS_MobilePcReq, P3DS_MobilePcPreviewReq, P3DS_MobilePcUnlockPlay, P3DS_GameClear;
static int sMpOn = -1, sMpDev = -1, sMpAnim = -1, sMpPending, sMpUnlockSeen, sMpZoom = -1;
volatile int P3DS_InPartyMenu, P3DS_MobilePcCloseThenLaunch;
int P3DS_MobilePcCfg(void) {
    if (sMpOn < 0) { sMpOn = P3DS_CfgInt("mobile_pc", 0) != 0; PSPNativeMemLog("[MPC] mobile_pc=%d (party BOXES chip, hook 0293, default OFF)", sMpOn); }
    return sMpOn;
}
int P3DS_MobilePcDevCfg(void) {
    if (sMpDev < 0) { sMpDev = P3DS_CfgInt("mobile_pc_dev", 0) != 0; PSPNativeMemLog("[MPC] mobile_pc_dev=%d (DEBUG: skip HoF gate at runtime only, never writes flags)", sMpDev); }
    return sMpDev;
}
void P3DS_MobilePcSetCfg(int on) { sMpOn = on != 0; P3DS_CfgWrite("mobile_pc", sMpOn); PSPNativeMemLog("[MPC] mobile_pc=%d from settings", sMpOn); }
void P3DS_MobilePcSetDev(int on) { sMpDev = on != 0; P3DS_CfgWrite("mobile_pc_dev", sMpDev); PSPNativeMemLog("[MPC] mobile_pc_dev=%d from settings (runtime gate only)", sMpDev); }
int P3DS_MobilePcOn(void) { return P3DS_MobilePcCfg() && (P3DS_GameClear || P3DS_MobilePcDevCfg()); }
int P3DS_MobilePcChipOn(void) { return P3DS_MobilePcCfg() && (P3DS_MobilePcOn() || P3DS_MobilePcUnlockPlay || sMpAnim >= 0 || sMpPending); }
#define MPC_SIDECAR "sdmc:/3ds/platinum3ds/mobile_pc.txt"
#define MPC_SIDECAR_TMP "sdmc:/3ds/platinum3ds/mobile_pc.tmp"
int P3DS_MobilePcUnlockShown(unsigned tid) {
    FILE *f; char line[128]; unsigned v;
    if (!tid) return 0;
    f = fopen(MPC_SIDECAR, "r");
    if (!f) return 0;
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "shown_tid=%u", &v) == 1 && v == tid) { fclose(f); return 1; }
    }
    fclose(f);
    return 0;
}
void P3DS_MobilePcUnlockMarkShown(unsigned tid) {
    FILE *f; char buf[4096]; size_t n = 0; unsigned v; int have = 0; char line[128];
    if (!tid) return;
    f = fopen(MPC_SIDECAR, "r");
    if (f) {
        while (fgets(line, sizeof line, f)) {
            if (sscanf(line, "shown_tid=%u", &v) == 1 && v == tid) have = 1;
            size_t l = strlen(line);
            if (n + l + 32 < sizeof buf) { memcpy(buf + n, line, l); n += l; if (l && line[l - 1] != '\n') buf[n++] = '\n'; }
        }
        fclose(f);
    } else {
        const char *hdr = "# soulsilver3ds mobile PC unlock (port file, never written to the .sav)\n";
        size_t l = strlen(hdr); memcpy(buf, hdr, l); n = l;
    }
    if (!have) {
        int w = snprintf(buf + n, sizeof buf - n, "shown_tid=%u\n", tid);
        if (w > 0) n += (size_t)w;
    }
    f = fopen(MPC_SIDECAR_TMP, "w");
    if (!f) { PSPNativeMemLog("[MPC] could not write %s", MPC_SIDECAR); return; }
    if (fwrite(buf, 1, n, f) != n) { fclose(f); remove(MPC_SIDECAR_TMP); PSPNativeMemLog("[MPC] write failed for %s", MPC_SIDECAR); return; }
    if (fclose(f)) { remove(MPC_SIDECAR_TMP); return; }
    remove(MPC_SIDECAR);
    if (rename(MPC_SIDECAR_TMP, MPC_SIDECAR) != 0) PSPNativeMemLog("[MPC] rename failed for %s", MPC_SIDECAR);
    else PSPNativeMemLog("[MPC] marked unlock shown for tid %u (sidecar only)", tid);
}
void P3DS_MobilePcRequestOpen(void) {
    if (!P3DS_MobilePcCfg()) return;
    if (!P3DS_MobilePcOn()) { extern void P3DS_Toast(const char *); P3DS_Toast("NOT UNLOCKED YET"); return; }
    if (P3DS_MobilePcReq) return;
    P3DS_MobilePcReq = 1;
    PSPNativeMemLog("[MPC] open requested");
}
void P3DS_MobilePcRequestPreview(void) {
    P3DS_MobilePcPreviewReq = 1;
    PSPNativeMemLog("[MPC] preview unlock requested (no sidecar write, no save flags)");
}
int P3DS_MobilePcAnimFrame(void) { return sMpAnim; }
int P3DS_MobilePcZoomFrame(void) { return sMpZoom; }

void P3DS_MobilePcNotePartyVisible(void) {
    if (sMpPending && sMpAnim < 0) {
        sMpPending = 0;
        sMpAnim = 0;
        PSPNativeMemLog("[MPC] BOXES chip pop-in (first party view after unlock)");
    }
}

static void mpcMapChip(int *dx0, int *dy0, int *dx1, int *dy1, int *nx0, int *ny0, int *nx1, int *ny1) {
    extern volatile int P3DS_PanelPartyOn, P3DS_PanelDest[4], P3DS_PanelSrcHeight;
    const int sx0 = kMobilePcChipSX0, sy0 = kMobilePcChipSY0, sx1 = kMobilePcChipSX1, sy1 = kMobilePcChipSY1;
    if (P3DS_PanelPartyOn && P3DS_PanelDest[2] > P3DS_PanelDest[0] && P3DS_PanelDest[3] > P3DS_PanelDest[1]) {
        const int x0 = P3DS_PanelDest[0], y0 = P3DS_PanelDest[1], x1 = P3DS_PanelDest[2], y1 = P3DS_PanelDest[3];
        const int sh = P3DS_PanelSrcHeight > 0 ? P3DS_PanelSrcHeight : 192;
        *dx0 = x0 + sx0 * (x1 - x0) / 256;
        *dx1 = x0 + sx1 * (x1 - x0) / 256;
        *dy0 = y0 + sy0 * (y1 - y0) / sh;
        *dy1 = y0 + sy1 * (y1 - y0) / sh;
    } else {
        *dx0 = sx0; *dy0 = sy0; *dx1 = sx1; *dy1 = sy1;
    }
    *nx0 = *dx0 * 5 / 4; *ny0 = *dy0 * 5 / 4;
    *nx1 = (*dx1 * 5 + 3) / 4; *ny1 = (*dy1 * 5 + 3) / 4;
}

void P3DS_MobilePcChipRectDS(int *x0, int *y0, int *x1, int *y1) {
    int nx0, ny0, nx1, ny1;
    mpcMapChip(x0, y0, x1, y1, &nx0, &ny0, &nx1, &ny1);
}

void P3DS_MobilePcChipRectNative(int *x0, int *y0, int *x1, int *y1) {
    int dx0, dy0, dx1, dy1;
    mpcMapChip(&dx0, &dy0, &dx1, &dy1, x0, y0, x1, y1);
}

int P3DS_MobilePcChipFooterShown(void) {
    extern int P3DS_InBattle;
    return P3DS_MobilePcChipOn() && P3DS_InPartyMenu && !P3DS_InBattle;
}

int P3DS_MobilePcChipHitDS(int x, int y) {
    int x0, y0, x1, y1;
    if (!P3DS_MobilePcChipFooterShown()) return 0;
    P3DS_MobilePcChipRectDS(&x0, &y0, &x1, &y1);
    return x >= x0 && x < x1 && y >= y0 && y < y1;
}

void P3DS_MobilePcOnChipTap(void) {
    if (sMpZoom >= 0 || P3DS_MobilePcCloseThenLaunch || P3DS_MobilePcReq) return;
    if (!P3DS_MobilePcOn()) { extern void P3DS_Toast(const char *); P3DS_Toast("NOT UNLOCKED YET"); return; }
    sMpZoom = 0;
    PSPNativeMemLog("[MPC] BOXES chip tapped: open zoom");
}

void P3DS_MobilePcAnimTick(void) {
    /* Fanfare + DialogBox still play in the overworld. The chip pop-in waits until the party is first viewed. */
    if (P3DS_MobilePcUnlockPlay) {
        if (!sMpUnlockSeen) { sMpUnlockSeen = 1; sMpPending = 1; PSPNativeMemLog("[MPC] unlock ceremony started; chip pop-in waits for the party screen"); }
    } else sMpUnlockSeen = 0;
    if (sMpAnim >= 0 && sMpAnim < kMobilePcAnimLast) sMpAnim++;
    else if (sMpAnim >= kMobilePcAnimLast) sMpAnim = -1;
    if (sMpZoom >= 0) {
        if (sMpZoom < kMobilePcZoomLast) sMpZoom++;
        else {
            sMpZoom = -1;
            extern int P3DS_InField;
            extern volatile int P3DS_PanelPartyOn;
            if (P3DS_InPartyMenu || P3DS_PanelPartyOn || !P3DS_InField) {
                P3DS_MobilePcCloseThenLaunch = 1;
                PSPNativeMemLog("[MPC] zoom done: closing the party, then launching boxes");
            } else {
                P3DS_MobilePcRequestOpen();
                PSPNativeMemLog("[MPC] zoom done: already in the field, requesting launch");
            }
        }
    }
}

int P3DS_CtlZr = -1, P3DS_CtlZl, P3DS_CtlCp;   /* run 55: file scope so the settings menu can change them live */
int P3DS_CfgSoftReset(void) { static int v = -1; if (v < 0) v = P3DS_CfgInt("soft_reset", 1) != 0; return v; }
volatile int P3DS_RunToggle;
volatile int P3DS_SkipIntro, P3DS_BootStage; /* run 95 reset_skip_intro */
volatile int P3DS_TileOn, P3DS_TileCur = -1; volatile unsigned P3DS_TileKeys; extern volatile int P3DS_BmOn;
volatile unsigned P3DS_InjectKeys; volatile int P3DS_InjectFrames; volatile int P3DS_StartReq = -1, P3DS_StartReqFound, P3DS_StartClose; volatile int P3DS_StartPrompt, P3DS_StartIdle; /* hook 0116 */
/* run 120 panel_party: Want set by PARTY tab when panel_party=1; On set by party_menu Init (hook 0201), cleared on Exit */
volatile int P3DS_PanelPartyWant, P3DS_PanelPartyOn;
/* run 130: DS-space rect (x0,y0,x1,y1) where the party screen is drawn inside the JOHTO panel; main3ds.c maps stylus taps inside it back to the party's 256x192 touch space */
volatile int P3DS_PanelDest[4], P3DS_PanelSrcHeight = 192, P3DS_PanelGearTooltip;
/* ss3ds 0060 bottom_menu: ticks while the game's Start menu task runs; bit n set = StartMenuAction n is in the menu (0xFFFF until it has been opened once) */
volatile unsigned P3DS_StartMenuTick, P3DS_StartAvail = 0xFFFF;
/* ss3ds 0080 mini map: written by the game thread once per player step (hook in field_control.c), read by the Bottom menu in render3ds.cpp */
volatile int P3DS_MmMap = -1, P3DS_MmX, P3DS_MmZ, P3DS_MmCx = -1, P3DS_MmCy = -1, P3DS_MmType, P3DS_MmMain, P3DS_MmSec = -1;
volatile unsigned P3DS_MmTick; char P3DS_MmName[24];
volatile int P3DS_SumIV[6], P3DS_SumEV[6], P3DS_SumAlive;
/* run 64 repel_prompt: port Yes/No (0 closed, 1 asking, 2 answered; result 1 yes 0 no). Keys handled here on the game
   thread; while asking the game sees no buttons. */
volatile int P3DS_RepelAsk, P3DS_RepelLast, P3DS_PromptOpen, P3DS_PromptResult = -1, P3DS_PromptSel; char P3DS_PromptText[64];
volatile int P3DS_MapId = -1; char P3DS_MapWild[3][200];
/* run 65 Map tab grid (written by the game thread snapshot) */
typedef struct { unsigned short species; unsigned char lo, hi, pct, sec; } P3DSMapEnt;
P3DSMapEnt P3DS_MapEnt[48]; volatile int P3DS_MapEntN, P3DS_MapTod; unsigned *P3DS_MapIcons;
int P3DS_MapIconMem[48], P3DS_MapIconPal[48], P3DS_MapIconPalFile; volatile int P3DS_MapGen; /* run 64 Map tab snapshot (game thread) */ /* run 64 summary_ivs (written by the summary screen) */   /* run 53 ZL: read by PlayerAvatar_IsRunButtonHeld (run = B XOR toggle) */
int sceCtrlPeekBufferPositive(SceCtrlData *d, int n) {
    hidScanInput(); { extern void P3DS_FrameHook(void); P3DS_FrameHook(); } { extern void P3DS_AudioInitWatch(void); P3DS_AudioInitWatch(); } u32 k = hidKeysHeld(); { extern int lmYPress; if (lmYPress > 0) { lmYPress--; k |= KEY_Y; } } /* run 100 lower_menu item squares */ unsigned b = 0;
    if (P3DS_PromptOpen == 1) { u32 dn = hidKeysDown(); if (dn & (KEY_DUP | KEY_DDOWN)) P3DS_PromptSel ^= 1;
        if (dn & KEY_A) { P3DS_PromptResult = P3DS_PromptSel == 0; P3DS_PromptOpen = 2; } else if (dn & KEY_B) { P3DS_PromptResult = 0; P3DS_PromptOpen = 2; }
        k = 0; }
    static const u32 map[][2] = { {KEY_SELECT, PSP_CTRL_SELECT}, {KEY_START, PSP_CTRL_START}, {KEY_DUP, PSP_CTRL_UP}, {KEY_DRIGHT, PSP_CTRL_RIGHT},
        {KEY_DDOWN, PSP_CTRL_DOWN}, {KEY_DLEFT, PSP_CTRL_LEFT}, {KEY_L, PSP_CTRL_LTRIGGER}, {KEY_R, PSP_CTRL_RTRIGGER},
        {KEY_A, PSP_CTRL_CIRCLE}, {KEY_B, PSP_CTRL_CROSS}, {KEY_X, PSP_CTRL_TRIANGLE}, {KEY_Y, PSP_CTRL_SQUARE} };
    { /* run 44 hotkey_swallow=1 (default): port combos (L+R+SELECT/Y/X/A/START, L+R+X+Y) must never reach the game
         (run 43: L+R+Y also pressed Y = registered item -> bike). While L and R are both held and any combo key is down,
         latch: hide L, R, A, X, Y, SELECT, START from the game until L and R are both released. 0 = old pass-through. */
      static int sw = -1, latch; if (sw < 0) { sw = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "hotkey_swallow=%d", &x) == 1) sw = x != 0; fclose(f); } PSPNativeMemLog("[3DS] hotkey_swallow=%d", sw); }
      const u32 combo = KEY_A | KEY_X | KEY_Y | KEY_SELECT | KEY_START, reset4 = KEY_L | KEY_R | KEY_START | KEY_SELECT;
      if (sw) { if ((k & (KEY_L | KEY_R)) == (KEY_L | KEY_R) && (k & combo)) latch = 1; else if (!(k & (KEY_L | KEY_R))) latch = 0;
        /* run 53 soft_reset=1 (default): L+R+START+SELECT all held is the game's own soft-reset combo -> pass exactly
           those four through (the game then resets via OS_ResetSystem -> P3DS_SoftReset). */
        if (latch) { const u32 keep = (P3DS_CfgSoftReset() && (k & reset4) == reset4) ? reset4 : 0; k &= ~((KEY_L | KEY_R | combo) & ~keep); } } }
    /* run 53 extra controls (all config, default on): ZR = Y (registered key item, e.g. the bike), ZL = run toggle
       (handled in the game's PlayerAvatar_IsRunButtonHeld via P3DS_RunToggle), circle pad = D-pad (4-way, dead zone). */
    {
#define zr P3DS_CtlZr
#define zl P3DS_CtlZl
#define cp P3DS_CtlCp
      if (zr < 0) { zr = P3DS_CfgInt("zr_item", 1); zl = P3DS_CfgInt("zl_run", 0); /* ss3ds: default off, SoulSilver has no run-toggle hook yet (it only showed a toast) */ cp = P3DS_CfgInt("cpad_dpad", 1);
          P3DS_RunToggle = P3DS_CfgInt("auto_run", 0) != 0; if (P3DS_RunToggle) PSPNativeMemLog("[RUN] auto_run=1: run starts ON (B walks; ZL / RUN tile flip it)");
          PSPNativeMemLog("[3DS] controls: zr_item=%d zl_run=%d cpad_dpad=%d soft_reset=%d cstick_mode=%d", zr, zl, cp, P3DS_CfgSoftReset(), P3DS_CstickMode()); }
      const u32 kh = hidKeysHeld(); u32 kd = hidKeysDown() & ~(KEY_ZL | KEY_ZR);
      if (zr && (kh & KEY_ZR) && !P3DS_CfgInt("hunt_stats", 0)) { /* hunt_stats=1 gives ZR to the hunt page */ k |= KEY_Y; if (kd & KEY_ZR) PSPNativeMemLog("[QUICK] ZR -> Y (item shortcut)"); }
      /* run 17: ZL/ZR edges from our own held history, not hidKeysDown(): any other hidScanInput() between two game polls
         (settings app, boot prompts) clears the "down" bit, and run 7/15 hardware logs never showed a ZL/ZR press. The
         first ZL or ZR seen at all is logged once so a device log tells "never pressed" from "not delivered". */
      { static u32 seen; const u32 zz = kh & (KEY_ZL | KEY_ZR); if (zz & ~seen) { seen |= zz; PSPNativeMemLog("[QUICK] first %s press seen (zl_run=%d zr_item=%d)", (zz & KEY_ZL) ? "ZL" : "ZR", zl, zr); } }
      { static u32 zprev; const u32 zdn = kh & ~zprev & (KEY_ZL | KEY_ZR); zprev = kh; kd |= zdn; }
      if (zl && (kd & KEY_ZL)) { extern void P3DS_Toast(const char *); P3DS_RunToggle = !P3DS_RunToggle; P3DS_Toast(P3DS_RunToggle ? "RUN ON" : "RUN OFF"); PSPNativeMemLog("[QUICK] ZL run toggle %d", P3DS_RunToggle); }
      if (cp && !(k & (KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT))) { static int axis; /* 0 none, 1 horizontal, 2 vertical */
        circlePosition c; hidCircleRead(&c); const int ax = c.dx < 0 ? -c.dx : c.dx, ay = c.dy < 0 ? -c.dy : c.dy, dz = 40;
        if (ax < dz && ay < dz) axis = 0;                          /* dead zone 40 of ~156 */
        else if (axis == 1) { if (ay * 4 > ax * 5) axis = 2; }     /* hysteresis: switch only past 1.25x */
        else if (axis == 2) { if (ax * 4 > ay * 5) axis = 1; }
        else axis = ax >= ay ? 1 : 2;
        if (axis == 1) k |= c.dx > 0 ? KEY_DRIGHT : KEY_DLEFT; else if (axis == 2) k |= c.dy > 0 ? KEY_DUP : KEY_DDOWN; }
      if (P3DS_CstickMode() == 2 && !(k & (KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT))) { static int axis; /* run 54: same 4-way logic */
        circlePosition c; hidCstickRead(&c); const int ax = c.dx < 0 ? -c.dx : c.dx, ay = c.dy < 0 ? -c.dy : c.dy, dz = 40;
        if (ax < dz && ay < dz) axis = 0;
        else if (axis == 1) { if (ay * 4 > ax * 5) axis = 2; }
        else if (axis == 2) { if (ax * 4 > ay * 5) axis = 1; }
        else axis = ax >= ay ? 1 : 2;
        if (axis == 1) k |= c.dx > 0 ? KEY_DRIGHT : KEY_DLEFT; else if (axis == 2) k |= c.dy > 0 ? KEY_DUP : KEY_DDOWN; } }
#undef zr
#undef zl
#undef cp
    /* run 95 reset_skip_intro (Platinum): after a soft reset the new process taps START (2 polls on, 4 off) until the game enters its
       Continue / New Game menu (MainMenu_Init sets P3DS_BootStage = 2) or the field, 3000 polls at most. Only START is sent. */
    /* ss3ds 0070 select_reset: tap SELECT (held 6 polls, about 0.1 s) for the same soft reset as L+R+START+SELECT.
       Wild battle: catch snapshot live (P3DS_CatchInfo[0]), in battle, outside the field. Starter screen (Elm's lab
       ChooseStarter app): WideScreenName "starter" (run 996: was gated on inField/battle only). Never with L/R/START
       held (normal chord), and never when the focused Pokemon is shiny. Re-read twice a second at most. */
    { static int sr = -1; static unsigned sh, sc; extern volatile int P3DS_CatchInfo[20]; extern int P3DS_InField, P3DS_InBattle; extern const char *P3DS_WideScreenName; extern void P3DS_SoftReset(unsigned);
      if (!(sc++ % 120) || sr < 0) { const int was = sr; sr = P3DS_CfgInt("select_reset", 0) != 0; if (sr != was) PSPNativeMemLog("[BOOT] select_reset=%d", sr); }
      const u32 kh = hidKeysHeld();
      const int wild = P3DS_CatchInfo[0] && P3DS_InBattle && !P3DS_InField;
      const int starter = P3DS_WideScreenName && P3DS_WideScreenName[0] && !strcmp(P3DS_WideScreenName, "starter");
      if (sr && (wild || starter) && (kh & KEY_SELECT) && !(kh & (KEY_L | KEY_R | KEY_START))) {
          if (++sh == 6) {
              if (P3DS_CatchInfo[0] && P3DS_CatchInfo[18]) PSPNativeMemLog("[BOOT] select_reset: SELECT ignored, the %s Pokemon (species %d) is SHINY", starter ? "starter" : "wild", (int)P3DS_CatchInfo[1]);
              else { PSPNativeMemLog("[BOOT] select_reset: SELECT held on %s (species %d) -> soft reset", starter ? "starter screen" : "wild battle", (int)P3DS_CatchInfo[1]); P3DS_SoftReset(0); } } }
      else sh = 0;
      { static int lg; if ((hidKeysDown() & KEY_SELECT) && lg < 6) { lg++; PSPNativeMemLog("[BOOT] SELECT pressed: select_reset=%d catchInfo=%d inBattle=%d inField=%d starter=%d shiny=%d", sr, (int)P3DS_CatchInfo[0], P3DS_InBattle, P3DS_InField, starter, (int)P3DS_CatchInfo[18]); } } }
    { /* run 49 X tile cursor: while the Bottom menu tiles are up, X never reaches the game (it only steps the tile cursor, render3ds.cpp);
         with the cursor up A/B/D-pad are ours too. The presses are queued for the render thread. Not while L or R is held (port combos). */
      extern volatile int P3DS_TileOn, P3DS_TileCur; extern volatile unsigned P3DS_TileKeys; static u32 pk; const u32 h = hidKeysHeld(), dn = h & ~pk; pk = h;
      const u32 nav = KEY_X | KEY_A | KEY_B | KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT;
      { static int xl; if ((dn & KEY_X) && xl < 12) { xl++; PSPNativeMemLog("[BMENU] X down: tile_on=%d tiles_up=%d cursor=%d L/R held=%d -> %s", P3DS_TileOn, P3DS_BmOn, P3DS_TileCur, (h & (KEY_L | KEY_R)) != 0, (P3DS_TileOn && P3DS_BmOn && !(h & (KEY_L | KEY_R))) ? "ours" : "passed to the game"); } }
      /* run 52: X reached the hidden Start menu when the tile flags flickered, so X stays ours for 45 polls (~0.75 s) after the tiles were last up, in the field */
      extern int P3DS_InField; static int grace; const int up = P3DS_TileOn && P3DS_BmOn; if (up) grace = 45; else if (grace) grace--;
      if ((up || (grace && P3DS_InField)) && !(h & (KEY_L | KEY_R))) {
          { extern volatile int P3DS_CatchMask; /* 2026-11-19: the ZR hunt stats overlay sits over the panel tiles: its X/Y/D-pad never reach the tiles */
          if (up && !P3DS_CatchMask && (dn & KEY_X || (P3DS_TileCur >= 0 && (dn & nav)))) P3DS_TileKeys |= dn & nav; }
          k &= ~KEY_X; if (P3DS_TileCur >= 0) k &= ~(KEY_A | KEY_B | KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT); } }
    { extern volatile int P3DS_CatchMask; if (P3DS_CatchMask) k &= ~(KEY_A | KEY_B | KEY_X | KEY_Y | KEY_L | KEY_R | KEY_START | KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT); } /* 2026-11-19: the battle Y screen is up (or just closed and keys are still held): the battle underneath gets none of these */
    { extern volatile int P3DS_BmBlock; if (P3DS_BmBlock) k &= ~(KEY_A | KEY_B | KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT); } /* run 142: the game's Start menu is alive under our panel: its cursor is not reachable (injected B below still gets through) */
    if (P3DS_SkipIntro) { static unsigned sn; static u64 t0; if (!t0) t0 = svcGetSystemTick(); if (P3DS_BootStage >= 2 || ++sn > 3000) { P3DS_SkipIntro = 0; PSPNativeMemLog("[BOOT] skip intro done: stage %d after %u polls, %u ms from the first poll", P3DS_BootStage, sn, (unsigned)((svcGetSystemTick() - t0) / CPU_TICKS_PER_MSEC)); } else if (sn % 6 < 2) k |= KEY_START; }
    if (P3DS_InjectFrames > 0) { k |= P3DS_InjectKeys; P3DS_InjectFrames--; } /* run 64: touch Start menu app (one press) */
    for (unsigned i = 0; i < sizeof map / sizeof map[0]; i++) if (k & map[i][0]) b |= map[i][1];
    circlePosition c; hidCircleRead(&c);
    memset(d, 0, sizeof *d); d->Buttons = b; d->Lx = (unsigned char)(128 + c.dx * 127 / 156); d->Ly = (unsigned char)(128 - c.dy * 127 / 156); return 1;
}
int sceCtrlReadBufferPositive(SceCtrlData *d, int n) { return sceCtrlPeekBufferPositive(d, n); }
int sceRtcGetCurrentClockLocalTime(ScePspDateTime *t) { time_t now = time(NULL); struct tm *m = gmtime(&now); /* 3DS time() is local */
    t->year = m->tm_year + 1900; t->month = m->tm_mon + 1; t->day = m->tm_mday; t->hour = m->tm_hour; t->minute = m->tm_min; t->second = m->tm_sec; t->microsecond = 0; return 0; }
int sceRtcGetDayOfWeek(int y, int mo, int d) { struct tm t = {0}; t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d; mktime(&t); return t.tm_wday; }
int sceWlanGetEtherAddr(unsigned char *mac) { static const unsigned char m[6] = {0x00, 0x09, 0xBF, 0x3D, 0x5A, 0x01}; memcpy(mac, m, 6); return 0; }
int scePowerRegisterCallback(int s, SceUID cb) { return 0; }
int scePowerSetClockFrequency(int a, int b, int c) { return 0; }
int sceKernelCreateCallback(const char *n, SceKernelCallbackFunction f, void *a) { return 1; }
int sceKernelRegisterExitCallback(int cb) { return 0; }
int sceKernelSleepThreadCB(void) { for (;;) svcSleepThread(1000000000LL); }
int sceKernelReferThreadStatus(SceUID id, SceKernelThreadInfo *i) { memset(i, 0, sizeof *i); return -1; }
int sceKernelCheckThreadStack(void) { return 0; }
SceSize sceKernelMaxFreeMemSize(void) { return (SceSize)osGetMemRegionFree(MEMREGION_APPLICATION); }
SceSize sceKernelTotalFreeMemSize(void) { return (SceSize)osGetMemRegionFree(MEMREGION_APPLICATION); }
SceUID sceIoOpen(const char *path, int flags, int mode) {
    int f = ((flags & 3) == PSP_O_RDONLY ? O_RDONLY : (flags & 3) == PSP_O_WRONLY ? O_WRONLY : O_RDWR) | (flags & PSP_O_APPEND ? O_APPEND : 0) | (flags & PSP_O_CREAT ? O_CREAT : 0) | (flags & PSP_O_TRUNC ? O_TRUNC : 0);
    char full[256]; if (!strchr(path, ':')) { snprintf(full, sizeof full, "sdmc:/3ds/platinum3ds/%s", path); path = full; }
    return open(path, f, 0666);
}
int sceIoWrite(SceUID fd, const void *b, SceSize n) { return write(fd, b, n); }
int sceIoRead(SceUID fd, void *b, SceSize n) { return read(fd, b, n); }
int sceIoClose(SceUID fd) { return close(fd); }

/* Clean shutdown: wake every sleeper, let each worker exit at its next blocking point, join with timeout. */
int P3DS_ShutdownThreads(int workersStopped) {
    p3ds_quit = 1;
    for (int r = 0; r < 3; r++) {
        for (int i = 1; i <= MAXT; i++) if (pts[i].used && pts[i].started) LightSemaphore_Release(&pts[i].wake, 1);
        for (int i = 0; i < 64; i++) if (sems[i].used) LightSemaphore_Release(&sems[i].s, 4);
        svcSleepThread(5000000);
    }
    /* Soft-reset crash (2026-11-06, hunt reset 12: prefetch abort in a worker, pc into a heap address): a game thread that had not
       reached a quit point within the join timeout kept running while the loop below freed the game's partition blocks. The
       allocator then overwrote a callback pointer inside a freed block and the thread jumped through it. The process is about to
       exit and the kernel reclaims everything, so the blocks (and ndsp) are only released when EVERY thread has really ended. */
    int stopped = 1;
    for (int i = 1; i <= MAXT; i++) {
        Thread thread = pts[i].th;
        if (!thread) continue;
        /* The kernel timeout result is positive, so only zero confirms a join. */
        Result rc = thread == threadGetCurrent() ? -1 : threadJoin(thread, 300000000ULL);
        if (rc != 0) {
            stopped = 0;
            PSPNativeMemLog("[3DS] exit: thread slot %d (%s) did not stop (join %08lx)", i, pts[i].name, (unsigned long)rc);
            continue;
        }
        threadFree(thread);
        pts[i].th = NULL;
    }
    if (alarmTh) {
        if (threadJoin(alarmTh, 300000000ULL) == 0) { threadFree(alarmTh); alarmTh = NULL; }
        else { stopped = 0; PSPNativeMemLog("[3DS] exit: alarm thread did not stop in 300 ms"); }
    }
    if (stopped && workersStopped) {
        for (int i = 0; i < P3DS_BLOCKS; i++) if (blocks[i]) { free(blocks[i]); blocks[i] = NULL; }
    }
    return stopped;
}
volatile int P3DS_FoeInfo[40]; /* hook 0031 trainer foe snapshot (battle -> foe info screen), read-only */
volatile int P3DS_DmgInfo[32]; /* hook 0208 damage/speed check snapshot (battle -> foe info screen), read-only */
volatile int P3DS_CatchInfo[20]; /* run 75 catch info snapshot (battle -> settings app), read-only */
volatile int P3DS_MoveHint[24]; /* hook 0121 fight-menu snapshot (moves, types, foe types), read-only */

/* SoulSilver requests 128 when restoring field music. libntr's decibel
   table only covers 0..127: an unchecked 128 reads the next object in rodata.
   In the tested port that word is +256, turning Goldenrod's -49 attenuation
   into a +207 boost. Saturate the fader target before it reaches the lookup;
   valid targets and fade durations retain the library's normal behavior. */
extern void __real_NNS_SndPlayerMoveVolume(void *handle, int target, int frames);
void __wrap_NNS_SndPlayerMoveVolume(void *handle, int target, int frames)
{
    int bounded = target < 0 ? 0 : target > 127 ? 127 : target;
    if (target != bounded) {
        static unsigned logged;
        if (logged++ < 8)
            PSPNativeMemLog("[AUDIO] fader target %d -> %d frames %d (decibel table bounds)", target, bounded, frames);
    }
    __real_NNS_SndPlayerMoveVolume(handle, bounded, frames);
}

/* Upstream pspoke 41005bc (black Battle Frontier rooms): libntr's GX_BeginLoadBGExtPltt uses LCDC offset 0 for
   GX_VRAM_BGEXTPLTT_23_G under SDK_PORT, where the DS uses 0x4000 (slots 2-3 start at bank G itself). The data then
   landed 16 KB too far, in bank H: the Frontier's between-battle rooms (mode 5 extended affine BG) drew with an empty
   palette, and the sub screen's BG VRAM (bank H) got overwritten. render3ds.cpp mapExtPalettes reads slots 2-3 from the
   start of bank G, as on hardware. libntr is staged and built once, so fix it at the call instead: remember the bank in
   Begin and shift the destination for bank G. Other banks are untouched. */
extern int GX_GetBankForBGExtPltt(void);
extern void __real_GX_BeginLoadBGExtPltt(void);
extern void __real_GX_LoadBGExtPltt(const void *src, u32 destSlotAddr, u32 szByte);
static int sBgExtPlttBankG;
void __wrap_GX_BeginLoadBGExtPltt(void)
{
    sBgExtPlttBankG = (GX_GetBankForBGExtPltt() & 0xFF) == 0x40; /* GX_VRAM_BGEXTPLTT_23_G == GX_VRAM_G */
    __real_GX_BeginLoadBGExtPltt();
}
void __wrap_GX_LoadBGExtPltt(const void *src, u32 destSlotAddr, u32 szByte)
{
    if (sBgExtPlttBankG && destSlotAddr >= 0x4000) {
        static unsigned logged;
        if (logged++ < 4)
            PSPNativeMemLog("[GX] BG ext palette bank G slot 0x%lx +0x%lx -> bank G offset 0x%lx (upstream 41005bc)",
                            (unsigned long)destSlotAddr, (unsigned long)szByte, (unsigned long)(destSlotAddr - 0x4000));
        destSlotAddr -= 0x4000;
    }
    __real_GX_LoadBGExtPltt(src, destSlotAddr, szByte);
}

#ifdef SS3DS /* SoulSilver Pal Pad heap fix (--wrap=Heap_Destroy, SoulSilver link only) */
/* Upstream pspoke 66da681 (Pal Pad > Register Friend Code crashed on OK): the friend-code entry app (unk_020850F4,
   translated asm) installs sub_020855CC as the VBlank callback and its exit destroys heap 0x6C without clearing it, so
   the next VBlank ran on a freed BgConfig. Clear exactly that callback when heap 0x6C goes away; every other
   Heap_Destroy is passed straight through. gSystem.vBlankIntr is the first member of struct System (system.h). */
extern void *gSystem[];
extern void sub_020855CC(void *);
extern void Main_SetVBlankIntrCB(void *cb, void *arg);
extern void __real_Heap_Destroy(u32 heapId);
void __wrap_Heap_Destroy(u32 heapId)
{
    if (heapId == 0x6C && gSystem[0] == (void *)sub_020855CC) {
        PSPNativeMemLog("[HEAP] heap 0x6C destroyed with the friend-code VBlank still set; cleared it (upstream 66da681)");
        Main_SetVBlankIntrCB(NULL, NULL);
    }
    __real_Heap_Destroy(heapId);
}
#endif

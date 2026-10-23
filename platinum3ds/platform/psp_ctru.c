#include <stdio.h>
/* PSP kernel subset on libctru: threads (with PSP wakeup counts), semaphores, alarms, time, critical sections. */
#include <3ds.h>
#include <pspkernel.h>
#include "crumbs.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#define MAXT 32
typedef struct { Thread th; SceKernelThreadEntry entry; void *arg; SceSize len; u8 argbuf[64]; LightSemaphore wake; int prio, stack, used, started, core; } PT;
static PT pts[MAXT + 1]; /* slot 0 = main thread */
static __thread int tlsId;
volatile int p3ds_quit; /* set by P3DS_ShutdownThreads: every worker leaves at its next blocking point */
static void quitCheck(void) { if (p3ds_quit && tlsId != 0) threadExit(0); }
static RecursiveLock big; static int bigInit;
static void initBig(void) { if (!bigInit) { RecursiveLock_Init(&big); bigInit = 1; LightSemaphore_Init(&pts[0].wake, 0, 0x7fff); pts[0].used = 1; } }
unsigned int sceKernelCpuSuspendIntr(void) { initBig(); if (!RecursiveLock_TryLock(&big)) { CRUMB("intrlock-wait"); RecursiveLock_Lock(&big); } return 1; }
void sceKernelCpuResumeIntr(unsigned int f) { (void)f; RecursiveLock_Unlock(&big); }
int sceKernelSuspendDispatchThread(void) { return (int)sceKernelCpuSuspendIntr(); }
int sceKernelResumeDispatchThread(int s) { sceKernelCpuResumeIntr((unsigned)s); return 0; }
/* PSP prio 0x20 (= PSP main thread) maps to 0x30 (3DS main thread); DS worker threads (0x20+p) land at or below
   the game thread so a spinning worker can never starve it (run-5 overworld freeze). */
static int toCtrPrio(int psp) { int p = 0x30 + (psp - 0x20) / 4; if (p < 0x28) p = 0x28; if (p > 0x3F) p = 0x3F; return p; }
SceUID sceKernelCreateThread(const char *n, SceKernelThreadEntry e, int prio, int stack, SceUInt a, void *o) {
    (void)a; (void)o; initBig();
    for (int i = 1; i <= MAXT; i++) if (!pts[i].used) { memset(&pts[i], 0, sizeof pts[i]); pts[i].used = 1; pts[i].entry = e; pts[i].prio = prio; pts[i].core = (n && (strstr(n, "audio") || strstr(n, "sas"))) ? 2 : -2; pts[i].stack = stack < 0x8000 ? 0x8000 : stack; LightSemaphore_Init(&pts[i].wake, 0, 0x7fff); return i + 1; }
    return -1;
}
static void trampoline(void *p) { PT *t = p; tlsId = (int)(t - pts); t->entry(t->len, t->len ? t->argbuf : NULL); }
int sceKernelStartThread(SceUID id, SceSize len, void *argp) {
    PT *t = &pts[id - 1]; if (len > sizeof t->argbuf) return -1; t->len = len; if (len) memcpy(t->argbuf, argp, len);
    t->th = threadCreate(trampoline, t, t->stack, toCtrPrio(t->prio), t->core, false); if (!t->th && t->core != -2) t->th = threadCreate(trampoline, t, t->stack, toCtrPrio(t->prio), -2, false); t->started = 1; return t->th ? 0 : -1;
}
int sceKernelExitThread(int s) { (void)s; threadExit(0); return 0; }
int sceKernelWaitThreadEnd(SceUID id, SceUInt *to) { (void)to; PT *t = &pts[id - 1]; if (t->th) threadJoin(t->th, U64_MAX); return 0; }
int sceKernelDeleteThread(SceUID id) { PT *t = &pts[id - 1]; if (t->th) threadFree(t->th); t->used = 0; return 0; }
SceUID sceKernelGetThreadId(void) { return tlsId + 1; }
int sceKernelSleepThread(void) { CRUMB("sleep"); initBig(); quitCheck(); LightSemaphore_Acquire(&pts[tlsId].wake, 1); quitCheck(); return 0; }
int sceKernelWakeupThread(SceUID id) { LightSemaphore_Release(&pts[id - 1].wake, 1); return 0; }
int sceKernelDelayThread(SceUInt us) { CRUMB("delay"); while (us) { SceUInt part = us > 10000 ? 10000 : us; svcSleepThread((s64)part * 1000); us -= part; quitCheck(); } return 0; }
int sceKernelChangeThreadPriority(SceUID id, int p) { pts[id - 1].prio = p; if (pts[id - 1].th) svcSetThreadPriority(threadGetHandle(pts[id - 1].th), toCtrPrio(p)); return 0; }
int sceKernelRotateThreadReadyQueue(int p) { CRUMB("yield"); (void)p; svcSleepThread(50000); quitCheck(); return 0; } /* real yield: 3DS sleep(0) never runs lower-priority threads */
SceInt64 sceKernelGetSystemTimeWide(void) { return (SceInt64)(svcGetSystemTick() / CPU_TICKS_PER_USEC); }
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
static void alarmLoop(void *p) { (void)p; while (!p3ds_quit) { svcSleepThread(1000000); u64 now = svcGetSystemTick();
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
int __sceSasCore(void *c, void *out) {
    (void)c; static int acc[2 * 2048]; memset(acc, 0, sasGrain * 2 * sizeof(int));
    /* run 34: audio_gain (default 10; old build = 24). vl 1016 = full-volume centred DS channel; DS itself outputs
       ~0.5x sample for that. The old 12x/channel gain drove battle mixes to peaks ~73000 -> heavy soft-clip
       compression (distortion, SE buried under BGM). 10 -> ~5x/channel, battle peaks ~30000. */
    static int g = -1; if (g < 0) { g = 10; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) if (sscanf(l, "audio_gain=%d", &x) == 1 && x > 0 && x <= 64) g = x; fclose(f); } PSPNativeMemLog("[AUDIO] audio_gain=%d (old 24)", g); }
    /* run 39 (battle rain crackle): rain = looping ADPCM voice at ~20 kHz with constant CC7 volume / CC10 pan ramps.
       (a) audio_volramp=1: volume/pan changes were applied as a step once per 256-sample grain (zipper noise on every
       ramp step); now ramped linearly across the grain (keyon still starts at full target: attacks stay crisp).
       (b) audio_interp=1: nearest-sample resampling -> linear interpolation (less aliasing grit on high-pitched noise). */
    static int vr_ = -1, ip_ = -1; if (vr_ < 0) { vr_ = 1; ip_ = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) { if (sscanf(l, "audio_volramp=%d", &x) == 1) vr_ = x != 0; if (sscanf(l, "audio_interp=%d", &x) == 1) ip_ = x != 0; } fclose(f); } PSPNativeMemLog("[AUDIO] audio_volramp=%d audio_interp=%d", vr_, ip_); }
    const int G = sasGrain;
    for (int v = 0; v < 16; v++) { SasV *x = &sasv[v]; if (!x->on) continue;
        u32 step = (u32)x->pitch;   /* 0x1000 = 1 sample per output sample */
        if (x->pl < 0 || !vr_) { x->pl = x->vl; x->pr = x->vr; }
        int l0 = x->pl, r0 = x->pr, sl = ((x->vl - l0) << 16) / G, sr = ((x->vr - r0) << 16) / G; /* 16.16 per-sample step */
        for (int i = 0; i < G; i++) {
            if (x->pos >= x->n) { if (x->loop >= 0 && (u32)x->loop < x->n) x->pos = (u32)x->loop + (x->pos - x->n) % (x->n - (u32)x->loop); else { x->on = 0; break; } }
            int smp = x->d[x->pos];
            if (ip_ && x->frac) { u32 q = x->pos + 1; if (q >= x->n) q = (x->loop >= 0 && (u32)x->loop < x->n) ? (u32)x->loop : x->pos; smp += ((x->d[q] - smp) * (int)x->frac) >> 12; }
            int vl = l0 + ((sl * (i + 1)) >> 16), vrr = r0 + ((sr * (i + 1)) >> 16);
            acc[2 * i] += (smp * vl * g) >> 13; acc[2 * i + 1] += (smp * vrr * g) >> 13;  /* run 34: gain g/8192 per vl (old fixed 3/1024 = g 24) */
            x->frac += step; x->pos += x->frac >> 12; x->frac &= 0xFFF;
        }
        x->pl = x->vl; x->pr = x->vr; }
    s16 *o = out; int pk = 0; for (int i = 0; i < sasGrain * 2; i++) { int s = acc[i]; if (s > pk) pk = s; if (-s > pk) pk = -s; int m = s < 0 ? -s : s; if (m > 24576) { int ex = m - 24576; m = 24576 + ex * 8192 / (ex + 8192); s = s < 0 ? -m : m; } o[i] = (s16)s; }
    { static int gN, mpk, mon, mvl; static unsigned clipG; gN++; if (pk > 24576) clipG++; if (pk > mpk) mpk = pk; int on = 0; for (int v = 0; v < 16; v++) if (sasv[v].on) { on++; if (sasv[v].vl > mvl) mvl = sasv[v].vl; } if (on > mon) mon = on;
      if (gN % 1720 == 0) { PSPNativeMemLog("[AUDIO] 10s: peak out %d (pre-clip; >24576 = soft-clipped, grains clipped %u), max voices on %d, max vl %d", mpk, clipG, mon, mvl); mpk = mon = mvl = 0; clipG = 0; } }
    return 0;
}
int __sceSasGetAllEnvelopeHeights(void *c, int *h) { (void)c; for (int v = 0; v < 32; v++) h[v] = v < 16 && sasv[v].on ? 0x40000000 : 0; return 0; }
#define NWB 16  /* 16 x 256 = 4096 samples (93 ms) of DSP lead */
static ndspWaveBuf wb[NWB]; static s16 *wbData; static int ndspOk = -1, wbNext;
static void ndspSetup(void) {
    if (ndspOk >= 0) return;
    Result r = ndspInit();
    if (R_FAILED(r)) { ndspOk = 0; PSPNativeMemLog("[AUDIO] ndspInit failed %08lx: copy your console's DSP firmware to sdmc:/3ds/dspfirm.cdc (run 'DSP1' homebrew once). Audio disabled.", r); return; }
    ndspSetOutputMode(NDSP_OUTPUT_STEREO); sasRateInit(); ndspChnSetInterp(0, P3DS_SasRate == 32728 ? NDSP_INTERP_NONE : NDSP_INTERP_LINEAR); ndspChnSetRate(0, P3DS_SasRate == 32728 ? 32728.498f : (float)P3DS_SasRate); ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    float mix[12] = {1.0f, 1.0f}; ndspChnSetMix(0, mix);
    wbData = linearAlloc(NWB * sasGrain * 4); memset(wb, 0, sizeof wb);
    for (int i = 0; i < NWB; i++) { wb[i].data_vaddr = wbData + i * sasGrain * 2; wb[i].nsamples = sasGrain; wb[i].status = NDSP_WBUF_DONE; }
    ndspOk = 1; PSPNativeMemLog("[AUDIO] ndsp ready: %d Hz stereo", P3DS_SasRate); PSPNativeMemLog("[AUDIO] buffers, %d x %d-sample buffers", NWB, sasGrain);
}
int sceAudioChReserve(int ch, int s, int f) { (void)s; (void)f; return ch < 0 ? 0 : ch; }
int sceAudioChRelease(int ch) { (void)ch; return 0; }
int sceAudioOutputPannedBlocking(int ch, int l, int r, void *b) {
    (void)ch; (void)l; (void)r; quitCheck(); ndspSetup();
    if (ndspOk != 1) { svcSleepThread((s64)sasGrain * 1000000000LL / P3DS_SasRate); return 0; }
    /* run 46 audio_lead=N (default 8 = 62 ms, was 16 = 125 ms of DSP lead): with the 83 ms snapshot latency the total
       sound delay drops from ~250 ms to ~145 ms (Poketch / menu taps felt late). 16 = old. */
    static int lead = -1; if (lead < 0) { lead = 8; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) if (sscanf(l, "audio_lead=%d", &x) == 1 && x >= 4 && x <= NWB) lead = x; fclose(f); } PSPNativeMemLog("[AUDIO] audio_lead=%d buffers (%d ms)", lead, lead * sasGrain * 1000 / P3DS_SasRate); }
    if (wbNext >= lead) wbNext = 0;
    ndspWaveBuf *w = &wb[wbNext];
    while (w->status != NDSP_WBUF_DONE && w->status != NDSP_WBUF_FREE) { if (p3ds_quit) { quitCheck(); return 0; } svcSleepThread(1000000); }
    memcpy((void *)w->data_vaddr, b, sasGrain * 4); DSP_FlushDataCache(w->data_vaddr, sasGrain * 4);
    ndspChnWaveBufAdd(0, w); wbNext = (wbNext + 1) % lead; return 0;
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
void P3DS_CfgInvalidate(void) { if (!cfgLkInit) { LightLock_Init(&cfgLk); cfgLkInit = 1; } LightLock_Lock(&cfgLk); cfgLen = -1; LightLock_Unlock(&cfgLk); }
int P3DS_CfgInt(const char *key, int def) {
    if (!cfgLkInit) { LightLock_Init(&cfgLk); cfgLkInit = 1; }
    LightLock_Lock(&cfgLk);
    if (cfgLen < 0) { u64 t0 = svcGetSystemTick(); FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); cfgLen = 0;
        if (f) { cfgLen = (int)fread(cfgBuf, 1, sizeof cfgBuf - 1, f); if (cfgLen < 0) cfgLen = 0; fclose(f); } cfgBuf[cfgLen] = 0; cfgAcct(t0);
        PSPNativeMemLog("[CFG] config.txt loaded into RAM (%d bytes, %u us)", cfgLen, (unsigned)((svcGetSystemTick() - t0) / CPU_TICKS_PER_USEC)); }
    int v = def; size_t n = strlen(key); const char *p = cfgBuf;
    while (*p) { const char *e = strchr(p, '\n'); if (!strncmp(p, key, n) && p[n] == '=') v = atoi(p + n + 1); if (!e) break; p = e + 1; }
    LightLock_Unlock(&cfgLk); return v; }
/* run 54 cstick_mode: 0 off, 1 = flick taps the Poketch button (run 53), 2 = C-stick works like the D-pad (default) */
static int cstickV = -1;
int P3DS_CstickMode(void) { if (cstickV < 0) { cstickV = P3DS_CfgInt("cstick_mode", 2); if (cstickV < 0 || cstickV > 2) cstickV = 2; } return cstickV; }
void P3DS_SetCstickMode(int v) { cstickV = v < 0 || v > 2 ? 2 : v; }
/* run 55: write one key=value into config.txt (other lines kept, order kept; key appended if missing). */
void P3DS_CfgWrite(const char *key, int val) {
    static char buf[16384]; size_t n = 0, kn = strlen(key); int done = 0; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r");
    if (f) { char line[256]; while (fgets(line, sizeof line, f)) { size_t l = strlen(line);
            if (!strncmp(line, key, kn) && line[kn] == '=') { if (done) continue; l = (size_t)snprintf(line, sizeof line, "%s=%d\n", key, val); done = 1; }
            if (n + l + 2 < sizeof buf) { memcpy(buf + n, line, l); n += l; if (l && line[l - 1] != '\n') buf[n++] = '\n'; } } fclose(f); }
    if (!done) n += (size_t)snprintf(buf + n, sizeof buf - n, "%s=%d\n", key, val);
    f = fopen("sdmc:/3ds/platinum3ds/config.txt", "w"); if (f) { fwrite(buf, 1, n, f); fclose(f); }
    P3DS_CfgInvalidate(); PSPNativeMemLog("[MENU] config.txt: %s=%d", key, val); }
int P3DS_CtlZr = -1, P3DS_CtlZl, P3DS_CtlCp;   /* run 55: file scope so the settings menu can change them live */
int P3DS_CfgSoftReset(void) { static int v = -1; if (v < 0) v = P3DS_CfgInt("soft_reset", 1) != 0; return v; }
volatile int P3DS_RunToggle;
volatile unsigned P3DS_InjectKeys; volatile int P3DS_InjectFrames; volatile int P3DS_StartReq = -1, P3DS_StartReqFound;
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
    hidScanInput(); { extern void P3DS_FrameHook(void); P3DS_FrameHook(); } u32 k = hidKeysHeld(); unsigned b = 0;
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
      if (zr < 0) { zr = P3DS_CfgInt("zr_item", 1); zl = P3DS_CfgInt("zl_run", 1); cp = P3DS_CfgInt("cpad_dpad", 1);
          PSPNativeMemLog("[3DS] controls: zr_item=%d zl_run=%d cpad_dpad=%d soft_reset=%d cstick_mode=%d", zr, zl, cp, P3DS_CfgSoftReset(), P3DS_CstickMode()); }
      const u32 kh = hidKeysHeld(), kd = hidKeysDown();
      if (zr && (kh & KEY_ZR)) k |= KEY_Y;
      if (zl && (kd & KEY_ZL)) { extern void P3DS_Toast(const char *); P3DS_RunToggle = !P3DS_RunToggle; P3DS_Toast(P3DS_RunToggle ? "RUN ON" : "RUN OFF"); PSPNativeMemLog("[3DS] ZL: run toggle %d", P3DS_RunToggle); }
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
void P3DS_ShutdownThreads(void) {
    p3ds_quit = 1;
    for (int r = 0; r < 3; r++) {
        for (int i = 1; i <= MAXT; i++) if (pts[i].used && pts[i].started) LightSemaphore_Release(&pts[i].wake, 1);
        for (int i = 0; i < 64; i++) if (sems[i].used) LightSemaphore_Release(&sems[i].s, 4);
        svcSleepThread(5000000);
    }
    for (int i = 1; i <= MAXT; i++) if (pts[i].th) { if (R_SUCCEEDED(threadJoin(pts[i].th, 300000000ULL))) threadFree(pts[i].th); pts[i].th = NULL; }
    if (alarmTh) { if (R_SUCCEEDED(threadJoin(alarmTh, 300000000ULL))) threadFree(alarmTh); alarmTh = NULL; }
    for (int i = 0; i < P3DS_BLOCKS; i++) if (blocks[i]) { free(blocks[i]); blocks[i] = NULL; }
    if (ndspOk == 1) { ndspExit(); ndspOk = -1; }
}
volatile int P3DS_CatchInfo[20]; /* run 75 catch info snapshot (battle -> settings app), read-only */

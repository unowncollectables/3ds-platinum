#include "ss3ds_version.h"
#include <unistd.h>
/* platinum3ds: 3DS entry for the full pspoke game build (replaces native-audio-app/main.c). */
#include <3ds.h>
#include <nitro.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <malloc.h>
extern void NitroMain(void);
extern void WIN_Init_sTexStartAddrTable(void);
extern void WIN_Init_sTexPlttStartAddrTable(void);
extern void PSPNativeFrameInit(void);
extern BOOL PSPNativeOverlay_Init(void);
extern BOOL PSPNativeRomFS_SetPath(const char *path);
extern BOOL PSPNative_OpenBackup(const char *path);
extern void PSPNativeMemLog(const char *, ...);
extern void P3DS_LogLine(const char *);
extern void PSPNativeStackProbeInit(void);
static PrintConsole con; static int conOnBottom = 1;
static void hold(const char *why); static void hardExit(int code);
int __real_printf(const char *, ...);
/* Bottom-screen status UI: title, boot checklist, red error box. Verbose output lives in log.txt only. */
static int uiRow = 4;
/* run 36: quiet boot (default): black screen with a small "Loading..." line; checklist/details stay in log.txt.
   verbose_boot=1 brings the checklist console back. Errors/crashes always switch to the full checklist + red box. */
static int quietBoot = -1;
static int isQuiet(void) { if (quietBoot < 0) { quietBoot = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int v; while (fgets(l, sizeof l, f)) if (sscanf(l, "verbose_boot=%d", &v) == 1) quietBoot = !v; fclose(f); } } return quietBoot; }
static void uiInit(void) { consoleSelect(&con); consoleClear(); if (isQuiet()) return; /* run 64 clean launch: quiet boot = plain black (the credits splash is drawn by render3ds) */ __real_printf("\x1b[1;1H\x1b[36;1m  Platinum 3DS\x1b[0m  \x1b[2m" SS3DS_VERSION "\x1b[0m\n  ------------------------------\n"); }
static const char *stepNames[16]; static int stepOk[16], nSteps;
static void uiStepDraw(const char *name, int ok);
static void uiStep(const char *name, int ok) { if (nSteps < 16) { stepNames[nSteps] = name; stepOk[nSteps++] = ok; } uiStepDraw(name, ok); }
static void uiStepDraw(const char *name, int ok) { if (isQuiet()) return; __real_printf("\x1b[%d;3H%-24s %s\x1b[0m", uiRow++, name, ok ? "\x1b[32;1m OK " : "\x1b[31;1mFAIL"); }
static void uiShowChecklist(void) { consoleInit(GFX_BOTTOM, &con); conOnBottom = 1; uiInit(); uiRow = 4; for (int i = 0; i < nSteps; i++) uiStepDraw(stepNames[i], stepOk[i]); }
static void uiError(const char *why) {
    /* run 64: quiet boot -> plain error screen (title + red box), no checklist; verbose_boot=1 keeps the old one */
    if (isQuiet()) { consoleInit(GFX_BOTTOM, &con); conOnBottom = 1; consoleSelect(&con); consoleClear(); __real_printf("\x1b[2;3H\x1b[37;1mPlatinum 3DS\x1b[0m"); uiRow = 4; }
    else if (!conOnBottom) uiShowChecklist();
    int r = uiRow + 2 > 20 ? 20 : uiRow + 2; char line[37];
    __real_printf("\x1b[%d;2H\x1b[41;37;1m%-38s", r++, " ERROR");
    for (const char *p = why; *p && r < 28; r++) { int n = 0; while (p[n] && p[n] != '\n' && n < 36) n++; if (p[n] == '\n') { snprintf(line, sizeof line, "%.*s", n, p); __real_printf("\x1b[%d;2H %-37s", r, line); p += n + 1; continue; } if (p[n] && n == 36) { int k = n; while (k > 8 && p[k] != ' ') k--; if (p[k] == ' ') n = k; } /* run 64: wrap at spaces */ snprintf(line, sizeof line, "%.*s", n, p); __real_printf("\x1b[%d;2H %-37s", r, line); p += n; while (*p == ' ') p++; }
    __real_printf("\x1b[%d;2H%-38s\x1b[0m\x1b[%d;2H  START: exit   SELECT: power off", r, "", r + 2);
}
/* Boot crash fix: libctru gives the main thread a 32 KB stack unless this is defined, and SoulSilver's boot path (main()
   locals + vsnprintf in PSPNativeMemLog) ran off the bottom of it at 0x08000000. Platinum fits; SoulSilver's main is bigger. */
u32 __stacksize__ = 0x100000;
#define STEP(name, cond, failmsg) do { int ok_ = (cond); uiStep(name, ok_); PSPNativeMemLog("[3DS] step %s: %s", name, ok_ ? "ok" : "FAIL"); if (!ok_) hold(failmsg); } while (0)

int P3DS_ConsoleOnBottom(void) { return conOnBottom; }
/* Clean console power-off from the error box (log is written line by line; sync the SD card first). */
static void powerOff(void) {
    PSPNativeMemLog("[3DS] power off requested"); svcSleepThread(200000000LL);
    if (R_SUCCEEDED(ptmSysmInit())) { PTMSYSM_ShutdownAsync(0); ptmSysmExit(); svcSleepThread(5000000000LL); }
    APT_HardwareResetAsync(); for (;;) svcSleepThread(1000000000LL);
}
extern void P3DS_LogFlushSync(void);
static char crashScreen[1024]; /* onCrash fills this (version, registers, last log lines); empty = show `why` */
static volatile int quitting;
static void hold(const char *why) { P3DS_LogFlushSync();
    PSPNativeMemLog("[3DS] halted: %s  (press START to exit)", why);
    uiError(crashScreen[0] ? crashScreen : why);
    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_START) break; if (hidKeysDown() & KEY_SELECT) powerOff(); gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank(); }
    hardExit(1);
}

static aptHookCookie hookCookie;
static Thread wdTh;
extern int P3DS_ShutdownThreads(int workersStopped);
extern int P3DS_ShutdownSDL(void);
extern int P3DS_RomCacheShutdown(void);
extern void P3DS_AudioShutdown(int gameThreadsStopped);
extern void P3DS_ThreadDiag(const char *);
extern void P3DS_LogFinish(int clean);
/* Clean exit: stop workers, join them, close files, gfxExit, then exit() normally (returns to HBL / HOME). */
static void p3SaveBackup(const char *why); /* run 122 */
static void hardExit(int code) {
    static int once; if (once++) exit(code);
    quitting = 1; PSPNativeMemLog("[3DS] exit: stopping threads");
    { extern void P3DS_HuntFlush(void); extern void P3DS_HuntWait(void); extern int P3DS_HuntStopWriter(void); P3DS_HuntFlush(); P3DS_HuntWait(); if (!P3DS_HuntStopWriter()) PSPNativeMemLog("[3DS] exit: hunt writer thread did not stop in 1 s"); } /* shiny_suite: hunt time since the last write */
    /* run 122 (SS 09f7427): save first: ShutdownThreads frees the game's partition blocks, which the backup close may still touch */
    extern BOOL PSPNative_CloseBackup(void); PSPNative_CloseBackup(); p3SaveBackup("exit");
    /* SoulSilver run 934 soft-reset crash: join every platform worker BEFORE freeing game heap / GPU state.
       Order: render (engine B / c1) → g3thr → par helper → SCE/game threads → SDL timers → gfxExit. */
    extern int P3DS_RenderShutdown(void), P3DS_G3ThrShutdown(void), P3DS_ParShutdown(void);
    int stopped = P3DS_RomCacheShutdown();
    stopped &= P3DS_RenderShutdown();
    stopped &= P3DS_G3ThrShutdown();
    stopped &= P3DS_ParShutdown();
    stopped &= P3DS_ShutdownSDL();
    const int gameThreadsStopped = P3DS_ShutdownThreads(stopped);
    { extern void P3U_ClearPending(void); P3U_ClearPending(); }
    { extern void P3DS_UseSummary(const char *); P3DS_UseSummary("clean exit"); }
    { extern void P3DS_FpsGuardClean(void); P3DS_FpsGuardClean(); }
    P3DS_AudioShutdown(gameThreadsStopped);
    if (wdTh && threadGetCurrent() != wdTh) {
        if (threadJoin(wdTh, 1000000000ULL) == 0) { threadFree(wdTh); wdTh = NULL; }
        else PSPNativeMemLog("[3DS] exit: watchdog thread did not stop in 1 s");
    }
    extern int P3DS_LogStopWriter(void);
    const int logStopped = P3DS_LogStopWriter();
    aptUnhook(&hookCookie);
    if (stopped && gameThreadsStopped) gfxExit();
    if (logStopped) P3DS_LogFinish(code == 0);
    exit(code);
}
static char lastMsg[200];
static char missingName[96]; /* name from the last "[SS-MISSING] name" log line (a Missing() trap logs it, then abort()s) */
#include "crumbs.h"
typedef struct { const char *tag; void *caller; unsigned n; int th; unsigned frame; } Crumb;
/* ring 0 = main game thread, ring 1 = every other thread (VBlank/alarm/audio work must not hide the main thread). */
static Crumb crumbs[2][64]; static volatile unsigned crumbPos[2];
extern unsigned PSPNativeRenderFrameCount(void);

/* run 122 (from SoulSilver 2f995e5): rotating validated backups (8 kept) at launch, 2 s after each in-game save,
   and at clean exit. save_backup=0 disables. Export: Settings > Export save -> export/platinum_*.sav (read-back verified).
   The live platinum.sav is never modified by backups/export (byte copy only). */
#define P3_BAK_KEEP 8
#define P3_SAVE_DIR "sdmc:/3ds/platinum3ds/"
#define P3_SAVE_BYTES 524288u
static u16 p3Crc16(const u8 *p, u32 n) { u32 c = 0xFFFF; while (n--) { c ^= (u32)*p++ << 8; for (int i = 0; i < 8; i++) c = (c & 0x8000) ? ((c << 1) ^ 0x1021) & 0xFFFF : (c << 1) & 0xFFFF; } return (u16)c; }
static u32 p3SaveValid(const u8 *d) {
    u32 best = 0;
    for (u32 base = 0; base < P3_SAVE_BYTES; base += 0x40000) {
        u32 cnt = 0, n = 0; int bad = 0;
        for (u32 off = base; off + 16 <= base + 0x40000; off += 4) {
            u32 f[3]; u16 h[2]; memcpy(f, d + off, 12); memcpy(h, d + off + 12, 4);
            if (f[2] != 0x20060623u) continue;
            u32 start = off + 16 - f[1];
            if (f[1] < 16 || f[1] > off + 16 - base || p3Crc16(d + start, off - start) != h[1]) { bad = 1; break; }
            if (n && f[0] != cnt) { bad = 1; break; }
            cnt = f[0]; n++;
        }
        if (!bad && n >= 2 && cnt > best) best = cnt;
    }
    return best;
}
static u8 *p3Buf(int i) { static u8 b[2][P3_SAVE_BYTES]; return b[i]; }
static size_t p3ReadAll(const char *path, u8 *buf) { FILE *f = fopen(path, "rb"); if (!f) return 0; size_t n = fread(buf, 1, P3_SAVE_BYTES, f); int extra = fgetc(f) != EOF; fclose(f); return extra ? 0 : n; }
static int p3WriteAtomic(const char *path, const u8 *buf) {
    char tmp[160]; snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *o = fopen(tmp, "wb"); if (!o) return 0;
    size_t w = fwrite(buf, 1, P3_SAVE_BYTES, o); int cl = fclose(o);
    if (w != P3_SAVE_BYTES || cl) { remove(tmp); return 0; }
    remove(path); return rename(tmp, path) == 0;
}
static LightLock p3SaveLock; static int p3SaveLockInit;
static u32 p3Crc32(const u8 *p, u32 n) { u32 c = 0xFFFFFFFFu; while (n--) { c ^= *p++; for (int i = 0; i < 8; i++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1))); } return ~c; }
static void p3SaveBackupLocked(const char *why) {
    extern int P3DS_CfgInt(const char *, int);
    if (!P3DS_CfgInt("save_backup", 1)) return;
    u8 *live = p3Buf(0), *old = p3Buf(1);
    if (p3ReadAll(P3_SAVE_DIR "platinum.sav", live) != P3_SAVE_BYTES) { PSPNativeMemLog("[SAVE] backup (%s): platinum.sav missing or not 512 KiB", why); return; }
    u32 cnt = p3SaveValid(live);
    if (!cnt) { PSPNativeMemLog("[SAVE] backup (%s): live save has no valid slot (blank or damaged), backups left untouched", why); return; }
    if (p3ReadAll(P3_SAVE_DIR "platinum.sav.bak1", old) == P3_SAVE_BYTES && !memcmp(live, old, P3_SAVE_BYTES)) { PSPNativeMemLog("[SAVE] backup (%s): save count %u already in bak1", why, cnt); return; }
    char a[160], b[160];
    snprintf(a, sizeof a, P3_SAVE_DIR "platinum.sav.bak%d", P3_BAK_KEEP); remove(a);
    for (int i = P3_BAK_KEEP - 1; i >= 1; i--) { snprintf(a, sizeof a, P3_SAVE_DIR "platinum.sav.bak%d", i); snprintf(b, sizeof b, P3_SAVE_DIR "platinum.sav.bak%d", i + 1); rename(a, b); }
    int ok = p3WriteAtomic(P3_SAVE_DIR "platinum.sav.bak1", live);
    PSPNativeMemLog("[SAVE] backup (%s): save count %u crc %08x -> bak1 (%s, %d kept)", why, cnt, p3Crc32(live, P3_SAVE_BYTES), ok ? "ok" : "FAILED", P3_BAK_KEEP);
}
static void p3Lock(void) { if (!p3SaveLockInit) { LightLock_Init(&p3SaveLock); p3SaveLockInit = 1; } LightLock_Lock(&p3SaveLock); }
static void p3SaveBackup(const char *why) { p3Lock(); p3SaveBackupLocked(why); LightLock_Unlock(&p3SaveLock); }
static volatile u64 p3DoneAt;
void P3DS_SaveDone(void) { p3DoneAt = svcGetSystemTick() | 1; }
static void p3SaveTick(void) {
    u64 t = p3DoneAt; if (!t || svcGetSystemTick() - t < 2000ull * CPU_TICKS_PER_MSEC) return;
    p3DoneAt = 0; p3SaveBackup("after save");
}
int P3DS_SaveExport(void) {
    extern void P3DS_Toast(const char *);
    p3Lock();
    u8 *live = p3Buf(0), *chk = p3Buf(1); int ok = 0; char path[160], path2[160];
    if (p3ReadAll(P3_SAVE_DIR "platinum.sav", live) != P3_SAVE_BYTES) { PSPNativeMemLog("[SAVE] export: platinum.sav missing or not 512 KiB"); goto done; }
    u32 cnt = p3SaveValid(live);
    if (!cnt) { PSPNativeMemLog("[SAVE] export: live save has no valid slot, nothing exported"); goto done; }
    mkdir(P3_SAVE_DIR "export", 0777);
    time_t now = time(NULL); struct tm *tm = localtime(&now); char st[24] = "undated";
    if (tm && tm->tm_year >= 120) strftime(st, sizeof st, "%Y%m%d_%H%M%S", tm);
    snprintf(path, sizeof path, P3_SAVE_DIR "export/platinum_%s.sav", st); snprintf(path2, sizeof path2, P3_SAVE_DIR "export/platinum.sav");
    ok = p3WriteAtomic(path, live) && p3ReadAll(path, chk) == P3_SAVE_BYTES && !memcmp(live, chk, P3_SAVE_BYTES)
      && p3WriteAtomic(path2, live) && p3ReadAll(path2, chk) == P3_SAVE_BYTES && !memcmp(live, chk, P3_SAVE_BYTES);
    PSPNativeMemLog("[SAVE] export: save count %u crc %08x -> export/platinum_%s.sav + export/platinum.sav (%s)", cnt, p3Crc32(live, P3_SAVE_BYTES), st, ok ? "ok" : "FAILED");
done:
    LightLock_Unlock(&p3SaveLock);
    P3DS_Toast(ok ? "SAVE EXPORTED" : "SAVE EXPORT FAILED");
    return ok;
}
/* --wrap=CARD_UnlockBackup: game finished a save transaction */
extern void __real_CARD_UnlockBackup(u16 lock_id);
void __wrap_CARD_UnlockBackup(u16 lock_id) { __real_CARD_UnlockBackup(lock_id); P3DS_SaveDone(); }

void P3DS_Crumb(const char *tag, void *caller) {
    int th = (int)(uintptr_t)threadGetCurrent(); int r = th ? 1 : 0;
    unsigned p = crumbPos[r]; Crumb *c = &crumbs[r][(p - 1) & 63];
    if (p && c->tag == tag && c->caller == caller && c->th == th) { c->n++; return; }
    c = &crumbs[r][p & 63]; c->tag = tag; c->caller = caller; c->n = 1; c->th = th; c->frame = PSPNativeRenderFrameCount(); crumbPos[r] = p + 1;
}
void P3DS_CrumbDump(void) {
    for (int r = 0; r < 2; r++) { unsigned p = crumbPos[r]; PSPNativeMemLog("[CRUMBS] %s thread: last %u (oldest first)", r ? "other" : "MAIN", p < 64 ? p : 64);
        for (unsigned i = p < 64 ? 0 : p - 64; i < p; i++) { Crumb *c = &crumbs[r][i & 63];
            PSPNativeMemLog("[CRUMB] %-14s from %p x%u th=%x frame=%u", c->tag, c->caller, c->n, c->th, c->frame); } }
}
/* Heap.c AllocFail() calls this on every failed Heap_Alloc: log it (the game may retry forever). */
extern BOOL __real_CommManager_IsInitialized(void);
BOOL __wrap_CommManager_IsInitialized(void) { static unsigned n; CRUMB("heap-allocfail"); if (n++ < 20) PSPNativeMemLog("[HEAP] Heap_Alloc FAILED (#%u)", n); return __real_CommManager_IsInitialized(); }

extern void __real_PSPNativeFatal(const char *);
void __wrap_PSPNativeFatal(const char *w) { snprintf(lastMsg, sizeof lastMsg, "%s", w); __real_PSPNativeFatal(w); }
void __wrap_abort(void) {
    char b[300];
    if (missingName[0]) snprintf(b, sizeof b, "This game screen is not in the 3DS port yet\n(%s).\nYour last in-game save is safe.\nPlease tell the developer which screen you opened.", missingName);
    else snprintf(b, sizeof b, "abort() from %p. last msg: %s", __builtin_return_address(0), lastMsg[0] ? lastMsg : "(none)");
    hold(b); for (;;) {}
}
/* printf/puts from the port go to the card log too (console alone scrolls away). */
#include <stdarg.h>
/* verbose=1 in config.txt keeps the per-grain/key-on audio spam in log.txt */
static int verboseLog = -1;
static int isVerbose(void) { if (verboseLog < 0) { verboseLog = 0; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int v; while (fgets(l, sizeof l, f)) if (sscanf(l, "verbose=%d", &v) == 1) verboseLog = v; fclose(f); } } return verboseLog; }
int __wrap_printf(const char *fmt, ...) { if (!strncmp(fmt, "[AUDIO-SAS] grain", 17) || !strncmp(fmt, "[AUDIO-SAS] key", 15)) { if (!isVerbose()) return 0; } char b[400]; va_list ap; va_start(ap, fmt); int n = vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    size_t l = strlen(b); while (l && b[l - 1] == '\n') b[--l] = 0; if (l) { P3DS_LogLine(b); snprintf(lastMsg, sizeof lastMsg, "%s", b); } return n; }
int __wrap_puts(const char *s) { P3DS_LogLine(s); snprintf(lastMsg, sizeof lastMsg, "%s", s); return 1; }
/* Boot crash fix: pspoke's SoulSilver PSPNativeMemLog (ss_memlog.c) ends in printf("%s\n", line), which the compiler turns into puts(),
   and the wrapped puts above used to call PSPNativeMemLog again: endless recursion on the first log line. Platinum's memprobe.c was
   patched (memlog-ring.patch) to hand the line straight to the log ring; this does the same for SoulSilver without touching pspoke,
   and skips the per-line open/write/close of native-memlog.txt. */
void __wrap_PSPNativeMemLog(const char *fmt, ...) { char line[400]; va_list ap; va_start(ap, fmt); vsnprintf(line, sizeof line, fmt, ap); va_end(ap); P3DS_LogLine(line); if (!strncmp(line, "[SS-MISSING] ", 13)) snprintf(missingName, sizeof missingName, "%.90s", line + 13); }
static volatile int lidEvt; /* run 64 lid_sound: 1 = closed (sleep), 2 = opened (wake); consumed on the game thread */
/* HOME / power button / lid: commit the save and the log to the SD card now, in case the app never comes back */
static void syncOnApt(const char *why) { extern int PSPNative_SyncBackupIdle(const char *); extern void P3DS_LogRequestSync(void); PSPNative_SyncBackupIdle(why); P3DS_LogRequestSync(); }
static void aptHookCb(APT_HookType t, void *p) { { extern volatile int P3DS_AudioAptPaused; if (t == APTHOOK_ONSUSPEND || t == APTHOOK_ONSLEEP) P3DS_AudioAptPaused = 1; else if (t == APTHOOK_ONRESTORE || t == APTHOOK_ONWAKEUP) P3DS_AudioAptPaused = 0; if (t == APTHOOK_ONRESTORE || t == APTHOOK_ONWAKEUP) { extern volatile int P3DS_AudioWoke, P3DS_AudioWokeDsp; P3DS_AudioWoke = P3DS_AudioWokeDsp = 1; } } /* audio DSP watchdog: no stall while HOME/sleep holds the DSP */
    if (t == APTHOOK_ONSUSPEND) syncOnApt("home/suspend"); else if (t == APTHOOK_ONSLEEP) syncOnApt("sleep");
    if (t == APTHOOK_ONEXIT) { syncOnApt("apt exit"); quitting = 1; } else if (t == APTHOOK_ONSLEEP) lidEvt = 1; else if (t == APTHOOK_ONWAKEUP) lidEvt = 2; }

/* Bottom screen: checklist until the first game frame, then the DS touch screen.
   Hold SELECT+START ~1 s to toggle the checklist view (debug). */
/* Game view: console output goes to an off-screen scratch buffer, both bottom framebuffers (incl. the
   32/24 px border) are cleared, and the renderer blits the DS sub screen into the back buffer every frame. */
static u16 nullFb[320 * 240]; static PrintConsole nullCon;
static void bottomToGame(void) {
    nullCon = con; nullCon.frameBuffer = nullFb; consoleSelect(&nullCon);
    conOnBottom = 0; gfxSetScreenFormat(GFX_BOTTOM, GSP_BGR8_OES); gfxSetDoubleBuffering(GFX_BOTTOM, true);
    for (int i = 0; i < 2; i++) { u16 w, h; u8 *fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &w, &h); memset(fb, 0, 320 * 240 * 3); gfxFlushBuffers(); gfxSwapBuffers(); }
}
void P3DS_GameFramePresented(void) { static int done; if (!done) { done = 1; PSPNativeMemLog("[3DS] first game frame presented; bottom screen -> DS touch screen"); bottomToGame(); } }
/* Screen modes: 0 = 1:1 (CPU), 1 = GPU 1.25x (default; top pillarboxed 320x240), 2 = GPU aspect fill 400x300 (crops 30 rows top/bottom).
   Hold L+R and press SELECT to cycle; saved as "gpuscale=N" in config.txt (old "scale=" entries are ignored). */

#define CFG_PATH "sdmc:/3ds/platinum3ds/config.txt"
static int scaleMode = -1;
/* 2026-11-05 (Nathan: "I don't even need the scale 1:1 and fill or zoom options ... no option at all"): the game is always the
   wide 1.25x picture. gpuscale= in config.txt and the L+R+SELECT cycle are gone; 1:1 is only the automatic fallback when the
   GPU scaler cannot start (render3ds.cpp gpuOk). */
int P3DS_ScaleMode(void) {
    if (scaleMode < 0) { scaleMode = 1; PSPNativeMemLog("[3DS] screen mode fixed at 1.25x wide (scale options removed)"); }
    return scaleMode;
}
/* Rewrites only the gpuscale= line. The old version opened config.txt with "w" and wrote just gpuscale=N,
   wiping every other setting (gpu3d_*, cull3d, audio_*, ...) on each L+R+SELECT. */
void P3DS_SetScaleMode(int m) { void saveScale(void); scaleMode = m < 0 || m > 2 ? 1 : m; saveScale(); }
void saveScale(void) {
    static char buf[8192]; size_t n = 0; int full = 0; FILE *f = fopen(CFG_PATH, "r");
    if (f) { char line[256]; while (fgets(line, sizeof line, f)) { if (!strncmp(line, "gpuscale=", 9) || !strncmp(line, "scale=", 6)) continue; size_t l = strlen(line); if (n + l + 2 < sizeof buf) { memcpy(buf + n, line, l); n += l; if (line[l - 1] != '\n') buf[n++] = '\n'; } else full = 1; } fclose(f); }
    int w = snprintf(buf + n, sizeof buf - n, "gpuscale=%d\n", scaleMode); if (w < 0 || (size_t)w >= sizeof buf - n) full = 1; else n += (size_t)w;
    f = full ? NULL : fopen(CFG_PATH, "w"); if (f) { fwrite(buf, 1, n, f); fclose(f); } { extern void P3DS_CfgInvalidate(void); P3DS_CfgInvalidate(); }
    static const char *names[3] = { "0: 1:1 pixel-perfect 256x192 (CPU blit)", "1: GPU 1.25x 320x240, top pillarboxed (x 40..360)", "2: GPU fill, 4:3 zoom gpuscale_zoom (default 1.40625 = 360x270), crops top+bottom" };
    PSPNativeMemLog("[3DS] screen mode %s; bottom 320x240 in GPU modes", names[scaleMode]);
}
volatile int P3DS_ResetSeen;
/* run 53 diag: bike mount marker (with frame number) so a [SPIKE] / glitch report can be lined up with it */
void P3DS_BikeEvent(int on) { extern unsigned PSPNativeRenderFrameCount(void); PSPNativeMemLog("[BIKE] %s at f=%u", on ? "mount" : "dismount", PSPNativeRenderFrameCount()); }
/* run 53 soft_reset: the game's OS_ResetSystem (L+R+START+SELECT, or its own error-reset paths) lands here instead of
   the old fatal "native restart unsupported". Saves are closed cleanly by hardExit. Relaunch: CIA -> APT relaunches
   this title; 3dsx under Luma's homebrew loader -> hb:ldr target = this 3dsx (argv[0]) + relaunch of the loader title
   (the same way hbmenu starts us). If that isn't available we simply exit cleanly (back to the homebrew menu). */
extern int __system_argc; extern char **__system_argv;
void P3DS_SoftReset(unsigned param) {
    /* Run 934: OS_ResetSystem logs then calls SoftReset, then sceKernelLoadExec also calls SoftReset
       (0061 + psp_ss_ctru). A second entry must not re-run hunt/marker/sleep/hardExit while the first
       teardown is in flight (worker jump through freed state). */
    static volatile int entered;
    if (__atomic_exchange_n(&entered, 1, __ATOMIC_ACQ_REL)) {
        PSPNativeMemLog("[3DS] soft reset: already in progress (param %u ignored)", param);
        return;
    }
    extern int P3DS_CfgSoftReset(void);
    int how = 0;
    { extern void P3DS_HuntSoftReset(void), P3DS_HuntFlush(void); P3DS_HuntSoftReset(); P3DS_HuntFlush(); } /* shiny_suite: count it before the relaunch */
    { extern int P3DS_CfgInt(const char *, int); if (P3DS_CfgInt("reset_skip_intro", 0)) { /* ss3ds 0070 (platinum3ds run 95): tell the relaunched process this was a soft reset */
          FILE *f = fopen("sdmc:/3ds/platinum3ds/skipintro.tmp", "w"); if (f) { fprintf(f, "%lld\n", (long long)time(NULL)); fclose(f); }
          PSPNativeMemLog("[BOOT] reset_skip_intro=1: marker written, the next boot skips the logos and title"); } }
    if (!envIsHomebrew()) { aptSetChainloaderToSelf(); how = 1; }
    else if (__system_argc > 0 && __system_argv && __system_argv[0] && !strncmp(__system_argv[0], "sdmc:/", 6)) {
        Handle h = 0;
        if (R_SUCCEEDED(svcConnectToPort(&h, "hb:ldr"))) {
            const char *path = __system_argv[0] + 5; u32 *cmd = getThreadCommandBuffer();
            cmd[0] = IPC_MakeHeader(2, 0, 2); cmd[1] = IPC_Desc_StaticBuffer(strlen(path) + 1, 0); cmd[2] = (u32)path;
            Result rc = svcSendSyncRequest(h); if (R_SUCCEEDED(rc)) rc = (Result)cmd[1];
            if (R_SUCCEEDED(rc)) { static u32 argBuf[0x100]; char *p = (char *)&argBuf[1]; size_t l = strlen(__system_argv[0]) + 1;
                if (l < sizeof argBuf - 8) { argBuf[0] = 1; memcpy(p, __system_argv[0], l); }
                cmd = getThreadCommandBuffer(); cmd[0] = IPC_MakeHeader(3, 0, 2); cmd[1] = IPC_Desc_StaticBuffer(sizeof argBuf, 1); cmd[2] = (u32)argBuf;
                svcSendSyncRequest(h); aptSetChainloaderToSelf(); how = 2; }
            svcCloseHandle(h); } }
    PSPNativeMemLog("[3DS] soft reset (param %u): %s", param, how == 1 ? "relaunch title" : how == 2 ? "relaunch 3dsx via hb:ldr" : "no relauncher, clean exit");
    /* friend crash (log-4 + Luma dump in the system loader): the relaunch can start before this process has given its
       memory back ("[GPUC] init: out of memory" on the next boot, then a stall or a Loader fault). Log what is still held
       and give the kernel a moment to reclaim before the new process is loaded. */
    PSPNativeMemLog("[3DS] soft reset: linear free %u KiB, app mem free %u KiB; pausing 400 ms before exit", (unsigned)(linearSpaceFree() >> 10), (unsigned)(osGetMemRegionFree(MEMREGION_APPLICATION) >> 10));
    svcSleepThread(400000000LL);
    hardExit(0);
}
/* ss3ds 0070 reset_skip_intro (default off). main() sets P3DS_SkipIntro when the marker P3DS_SoftReset wrote is < 30 s
   old. The hook in the game's NitroMain (pspoke-patches/0071, scripts/ss3ds_hooks.d/0070_skipintro.py) asks here once
   at boot; on 1 it starts the game's own save check -> Continue / New Game menu, exactly where the title screen goes on
   START, instead of the logos, intro movie and title. If the hook was not applied nothing asks, the boot is the normal
   one and P3DS_FrameHook logs that once. */
volatile int P3DS_SkipIntro; static int skipAsked; static u64 skipT0;
int P3DS_SkipIntroBoot(void) {
    skipAsked = 1;
    PSPNativeMemLog("[BOOT] skip intro: game hook asked, answer %d (%u ms after the marker check)", (int)P3DS_SkipIntro,
                    skipT0 ? (unsigned)((svcGetSystemTick() - skipT0) / CPU_TICKS_PER_MSEC) : 0u);
    return P3DS_SkipIntro;
}
void P3DS_FrameHook(void) {
    p3SaveTick();
    static int held;
    u32 k = hidKeysHeld();
    if (P3DS_SkipIntro && !skipAsked) { static unsigned polls; if (++polls == 600) { P3DS_SkipIntro = 0;
            PSPNativeMemLog("[BOOT] skip intro hook NOT applied (the game never asked after 600 polls); normal boot, the toggle does nothing in this build"); } }
    /* run 53: L+R+START+SELECT = soft reset. Remember that the combo was seen until L and R are released, so the
       L+R+SELECT (scale) and L+R+START (FPS, render3ds) toggles fire on RELEASE and only if the reset combo wasn't hit. */
    { const u32 r4 = KEY_L | KEY_R | KEY_START | KEY_SELECT; if ((k & r4) == r4) P3DS_ResetSeen = 1; else if (!(k & (KEY_L | KEY_R))) P3DS_ResetSeen = 0; }
    /* 2026-11-05: L+R+SELECT no longer cycles the screen scale (fixed 1.25x wide) */
    /* run 45: L+R+Y (GPU/CPU 3D toggle) retired; the CPU 3D renderer is only an automatic fallback if GPU init fails, or config gpu3d=0 */
    if ((k & (KEY_L | KEY_R)) == (KEY_L | KEY_R) && (hidKeysDown() & KEY_A)) { /* run 52: sharp 3D disabled -> L+R+A does nothing (still swallowed) */ } /* run 47: L+R+A = res3d 0/2 (was the zinv debug toggle; zinv stays config-only gpu3d_zinv) */
    if ((k & (KEY_SELECT | KEY_START)) == (KEY_SELECT | KEY_START) && !(k & (KEY_L | KEY_R))) { if (++held == 60) { if (conOnBottom) bottomToGame(); else uiShowChecklist(); } } else held = 0;
    if (quitting || !aptMainLoop()) hardExit(0);
    /* run 64 lid_sound=1 (default; run 65: Poke Ball recall SE on close, send-out SE on open, see P3DS_PlayLidSE).
       Only in the overworld (sound system up, no battle/cutscene music to interrupt). Played on the game thread. */
    { static int lt = -1, n; extern int P3DS_CfgInt(const char *, int); extern int P3DS_InField; if (lt < 0) lt = 0 /* lid_test: debug option removed */ != 0;
      if (lt && P3DS_InField) { ++n; if (n == 300) lidEvt = 1; else if (n == 420) lidEvt = 2; } } /* run 65 debug: fake a lid close/open in the overworld */
    if (lidEvt) { int e = lidEvt; lidEvt = 0; if (e == 1) { extern void P3DS_HuntFlush(void); P3DS_HuntFlush(); } /* shiny_suite: lid closed -> hunt.txt up to date */ static int ls = -1; extern int P3DS_CfgInt(const char *, int); extern int P3DS_InField;
        if (ls < 0) ls = P3DS_CfgInt("lid_sound", 1) != 0;
        if (ls && P3DS_InField) { extern void P3DS_PlayLidSE(int); P3DS_PlayLidSE(e == 2); }
        if (e == 1) { extern void P3DS_HuntFlush(void); P3DS_HuntFlush(); } /* run 88: lid closed -> hunt.txt up to date */
        PSPNativeMemLog("[3DS] lid %s (sound %d)", e == 1 ? "closed" : "opened", ls && P3DS_InField); }
}
static int touchPrev;
/* 3DS bottom touch -> DS touch screen (256x192 centred at +32,+24 on the 320x240 panel). */
int P3DS_TouchRead(int *x, int *y) {
    { static int polled; if (!polled) { polled = 1; PSPNativeMemLog("[3DS] touch: game is polling the touch screen (bottom=%s)", conOnBottom ? "console" : "game"); } }
    if (conOnBottom) return 0;
    { extern volatile int P3DS_PromptOpen; if (P3DS_PromptOpen == 1) return 0; } /* run 64 repel_prompt */
    { /* run 148 (Nathan: stray Trainer card): the game must never see the finger that was on our panel/tiles. While our tiles, the panel
         or a menu request own the touch screen, remember it; once they let go, the game gets no touch until the finger has lifted and
         12 more polls (~0.4 s) have passed, so a tap that ends one of our screens cannot land on the game's Start menu / Trainer card. */
      extern volatile int P3DS_BmOn, P3DS_BmBlock, P3DS_PanelPartyOn, P3DS_StartReq; static int cool, logged;
      const int ours = (P3DS_BmOn && !P3DS_PanelPartyOn) || P3DS_BmBlock || P3DS_StartReq >= 0;
      if (ours) cool = 12;
      else if (cool > 0 && !P3DS_PanelPartyOn) { if (!(hidKeysHeld() & KEY_TOUCH)) cool--;
          if ((hidKeysHeld() & KEY_TOUCH) && logged < 20) { logged++; PSPNativeMemLog("[3DS] touch hand-off guard: touch held back from the game (%d polls left)", cool); }
          touchPrev = 1; return 0; } }
    touchPosition t; int dx, dy;
    { /* run 53 cstick_poketch=1 (default): C-stick up/down = tap the Poketch's side button (next / previous app),
         only in the overworld and only when the stylus isn't down. One short tap per flick (stick must return). */
      static int cfg = -1, armed = 1, inj, injY; extern int P3DS_CfgInt(const char *, int); extern int P3DS_InField;
      { extern int P3DS_CstickMode(void); cfg = P3DS_CstickMode() == 1; }  /* run 54: cstick_mode=1 (run 55: live) */
      if (cfg && !(hidKeysHeld() & KEY_TOUCH)) { circlePosition cs; hidCstickRead(&cs);
          if (cs.dy > -40 && cs.dy < 40) armed = 1;
          if (!inj && armed && P3DS_InField && (cs.dy >= 80 || cs.dy <= -80)) { armed = 0; inj = 4; injY = cs.dy > 0 ? 64 : 128; }
          if (inj) { inj--; *x = 240; *y = injY; touchPrev = 1; return 1; } } }
    { extern int P3DS_LowerMenuDraw; extern int NitroPort_LowerMenuTouch(int, int, int, int *, int *);
      { extern int lmTouchInj; if (P3DS_LowerMenuDraw && lmTouchInj > 0) { lmTouchInj--; *x = 240; *y = 64; touchPrev = 1; return 1; } } /* run 102 POKETCH button */
      if (P3DS_LowerMenuDraw) { /* run 100 lower_menu: the menu owns the screen; only touches that began in its panel reach the game */
        int down = (hidKeysHeld() & KEY_TOUCH) != 0, mx = 0, my = 0; if (down) hidTouchRead(&t);
        int ok = NitroPort_LowerMenuTouch(down, down ? t.px : 0, down ? t.py : 0, &mx, &my);
        touchPrev = down; if (!down || !ok || mx < 0 || mx > 255 || my < 0 || my > 191) return 0;
        *x = mx; *y = my; return 1; } }
    if (!(hidKeysHeld() & KEY_TOUCH)) {
        /* run 47 touch_latch=1 (default): the game samples touch once per game frame (~35-45 ms); a quick tap that
           starts and ends between two samples was lost (felt like taps registering late / needing a 2nd tap). Look at
           HID's touch ring (8 most recent ~4 ms samples) and report one frame of touch at the newest pressed sample if
           the game saw no touch last frame. 0 = old behaviour. */
        static int latch = -1; if (latch < 0) { latch = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "touch_latch=%d", &x) == 1) latch = x != 0; fclose(f); } PSPNativeMemLog("[3DS] touch_latch=%d", latch); }
        static int lastIdx = -1; vu32 *sec = hidSharedMem ? hidSharedMem + 42 : NULL;
        int idx = sec ? (int)(sec[4] & 7) : 0, nNew = lastIdx < 0 ? 0 : (idx - lastIdx) & 7, found = 0; lastIdx = idx;
        if (!latch || !sec || touchPrev) { touchPrev = 0; return 0; }   /* only samples taken since the last game poll */
        for (int i = 0; i < nNew && !found; i++) { int j = (idx - i) & 7; u32 a = sec[8 + j * 2], b = sec[8 + j * 2 + 1];
            if (b & 1) { t.px = a & 0xFFFF; t.py = a >> 16; found = 1; } }
        if (!found) return 0;
        static unsigned latched; if (++latched % 50 == 1) PSPNativeMemLog("[3DS] touch_latch: caught a short tap (%u so far)", latched);
    } else { hidTouchRead(&t); }
    if (P3DS_ScaleMode() == 0) { dx = (int)t.px - 32; dy = (int)t.py - 24; } else { dx = (int)t.px * 256 / 320; dy = (int)t.py * 192 / 240; }
    /* touch log: every stylus-down (first 200, then every 50th) with the 3DS panel point and the DS point it maps to */
    static unsigned downs; static int logHeld; int edge = !logHeld; logHeld = (hidKeysHeld() & KEY_TOUCH) != 0;
    if (edge && (++downs <= 200 || downs % 50 == 0))
        PSPNativeMemLog("[3DS] touch #%u raw=(%d,%d) ds=(%d,%d) scale=%d%s", downs, t.px, t.py, dx, dy, P3DS_ScaleMode(), (dx < 0 || dx > 255 || dy < 0 || dy > 191) ? " OUTSIDE DS area, ignored" : "");
    if (dx < 0 || dx > 255 || dy < 0 || dy > 191) return 0;
    { /* Mobile PC BOXES chip: consume the tap so it does not hit CANCEL or the party prompt. */
      extern int P3DS_MobilePcChipHitDS(int, int);
      extern void P3DS_MobilePcOnChipTap(void);
      extern int P3DS_MobilePcZoomFrame(void);
      extern volatile int P3DS_MobilePcCloseThenLaunch;
      if (P3DS_MobilePcZoomFrame() >= 0 || P3DS_MobilePcCloseThenLaunch) { touchPrev = 1; return 0; }
      if (P3DS_MobilePcChipHitDS(dx, dy)) { P3DS_MobilePcOnChipTap(); touchPrev = 1; return 0; }
    }
    { extern volatile int P3DS_PanelPartyOn, P3DS_PanelDest[4], P3DS_PanelSrcHeight;   /* run 130: party-in-panel: a tap inside the party picture is mapped back to the party's own 256x192 touch space; taps elsewhere are ignored (panel chrome handles them) */
      if (P3DS_PanelPartyOn) { const int x0 = P3DS_PanelDest[0], y0 = P3DS_PanelDest[1], x1 = P3DS_PanelDest[2], y1 = P3DS_PanelDest[3];
        if (x1 > x0 && y1 > y0 && dx >= x0 && dx < x1 && dy >= y0 && dy < y1) { *x = (dx - x0) * 256 / (x1 - x0); *y = (dy - y0) * P3DS_PanelSrcHeight / (y1 - y0); touchPrev = 1; return 1; }
        touchPrev = 1; return 0; } }
    { extern int P3DS_StripOn; if (P3DS_StripOn && dy >= 176 && dx >= 8 && dx < 208) { touchPrev = 1; return 0; } } /* run 78 bottom strip owns these taps */
    { extern volatile int P3DS_CatchMask; if (P3DS_CatchMask) { touchPrev = 1; return 0; } } /* 2026-11-19: battle Y screen owns the touch screen */
    { extern volatile int P3DS_BmOn, P3DS_BmBlock; if (P3DS_BmOn || P3DS_BmBlock) { touchPrev = 1; return 0; } } /* ss3ds 0060: bottom-screen Start-menu tiles own the touch screen */
    *x = dx; *y = dy; touchPrev = 1; return 1;
}
/* Watchdog: if no game frame is presented for ~3 s after the game started, log the breadcrumb and show the
   red box from this (higher-priority) thread so a stuck game thread cannot hard-lock the console. */
extern unsigned PSPNativeRenderFrameCount(void);
static volatile int gameStarted;
static void watchdog(void *p) {
    P3DS_ThreadGuard("watchdog");
    unsigned last = 0; int still = 0;
    while (!quitting) {
        svcSleepThread(500000000LL);
        if (quitting) break;
        if (!gameStarted) continue;
        { extern volatile int P3DS_MenuOpen; if (P3DS_MenuOpen) { still = 0; continue; } }  /* run 55: settings menu pauses on purpose */
        { extern volatile int P3DS_FolBuilding; if (P3DS_FolBuilding) { still = 0; continue; } }  /* run 41: one-time followers.bin build */
        unsigned f = PSPNativeRenderFrameCount();
        if (f != last) { last = f; still = 0; continue; }
        { extern volatile int P3DS_NetBusy; if (P3DS_NetBusy && still < 179) { still++; continue; } }  /* GTS login blocks the game thread on purpose (up to ~90 s here); only a real hang gets the red box */
        ++still;
        if (still == 3) {
            const unsigned pos = crumbPos[0];
            const Crumb *c = &crumbs[0][(pos - 1) & 63];
            PSPNativeMemLog("[STALL] no frame since f=%u | main last %s from %p", f,
                pos && c->tag ? c->tag : "?", pos ? c->caller : NULL);
        }
        if (still >= 20) { P3DS_CrumbDump();
            char b[300]; snprintf(b, sizeof b, "game stalled: no frame for 10 s at frame %u (crumbs in log.txt). main last: %s %p. msg: %s", f, crumbs[0][(crumbPos[0]-1)&63].tag ? crumbs[0][(crumbPos[0]-1)&63].tag : "(none)", crumbs[0][(crumbPos[0]-1)&63].caller, lastMsg[0] ? lastMsg : "(none)");
            PSPNativeMemLog("[WATCHDOG] %s", b);
            { extern void P3DS_BigLockInfo(char *b, int n); char lb[400]; P3DS_BigLockInfo(lb, sizeof lb); PSPNativeMemLog("[WATCHDOG] %s", lb); { extern void P3DS_RomLockInfo(char *b, int n); P3DS_RomLockInfo(lb, sizeof lb); PSPNativeMemLog("[WATCHDOG] %s", lb); { extern void P3DS_RomCallerInfo(char *b, int n); char cb[800]; P3DS_RomCallerInfo(cb, sizeof cb); PSPNativeMemLog("[WATCHDOG] %s", cb); } } }
            hold(b);
        }
    }
}
static u8 excStack[0x8000] __attribute__((aligned(8)));
static u64 bootMs;
static void onCrash(ERRF_ExceptionInfo *e, CpuRegisters *r) {
    PSPNativeMemLog("[CRASH] type=%d pc=%08lx lr=%08lx sp=%08lx far=%08lx fsr=%08lx r0=%08lx r4=%08lx", e->type, r->pc, r->lr, r->sp, e->far, e->fsr, r->r[0], r->r[4]);
    { extern int P3DS_InField, P3DS_InBattle; extern int P3DS_CfgInt(const char *, int); extern void P3DS_UseSummary(const char *);
      PSPNativeMemLog("[CRASH] Platinum 3DS %s | up %lu s | field %d battle %d | linear free %lu app free %lu | g3_wmode=%d | last msg: %s",
                      SS3DS_VERSION, (unsigned long)((osGetTime() - bootMs) / 1000), P3DS_InField, P3DS_InBattle, (unsigned long)linearSpaceFree(),
                      (unsigned long)osGetMemRegionFree(MEMREGION_APPLICATION), P3DS_CfgInt("g3_wmode", 1), lastMsg);
      P3DS_UseSummary("crash"); }
    P3DS_CrumbDump(); P3DS_LogFlushSync();
    { extern unsigned P3DS_LogTailCount(void); extern const char *P3DS_LogTailLine(unsigned); extern int P3DS_InField, P3DS_InBattle;
      int n = snprintf(crashScreen, sizeof crashScreen, "Platinum 3DS\n%s\nGAME CRASHED (type %d)\npc=%08lx lr=%08lx\nsp=%08lx far=%08lx\nup %lu s  field %d  battle %d\nLast log lines:\n",
                       SS3DS_VERSION, e->type, r->pc, r->lr, r->sp, e->far, (unsigned long)((osGetTime() - bootMs) / 1000), P3DS_InField, P3DS_InBattle);
      const unsigned tc = P3DS_LogTailCount();
      for (unsigned i = tc > 3 ? tc - 3 : 0; i < tc && n > 0 && n < (int)sizeof crashScreen - 100; i++) n += snprintf(crashScreen + n, sizeof crashScreen - n, "%.68s\n", P3DS_LogTailLine(i));
      if (n > 0 && n < (int)sizeof crashScreen - 40) snprintf(crashScreen + n, sizeof crashScreen - n, "Full details: log.txt, boot.txt"); }
    char b[200]; snprintf(b, sizeof b, "game crashed (type %d) pc=%08lx lr=%08lx - details in log.txt", e->type, r->pc, r->lr); hold(b);
    svcExitProcess();
}
/* Fault handler output: a crash on any thread writes its registers synchronously to boot.txt (the log ring is flushed asynchronously). boot.txt only exists after a crash. */
/* Raw open/write/close (no stdio, no malloc) so it works from a fault handler, and is capped so a chatty caller can't slow boot. */
#include <fcntl.h>
#include <stdarg.h>
static void bcRaw(const char *s, size_t n) { static volatile int cnt; if (cnt++ > 1200) return; int fd = open("sdmc:/3ds/platinum3ds/boot.txt", O_WRONLY | O_CREAT | O_APPEND, 0666); if (fd >= 0) { write(fd, s, n); close(fd); } }
/* Fault handler for every thread (libctru keeps the handler per thread, so each thread entry calls P3DS_ThreadGuard).
   Allocation-free: a fault inside malloc/stdio must still get its registers onto the card before the process dies. */
#define NGUARD 16
#define GUARD_STK 0x1000
static u8 guardStk[NGUARD][GUARD_STK] __attribute__((aligned(8))); static const char *guardName[NGUARD]; static volatile int guardN;
static char *hx(char *p, u32 v) { for (int i = 28; i >= 0; i -= 4) *p++ = "0123456789abcdef"[(v >> i) & 15]; return p; }
static char *tag(char *p, const char *k, u32 v) { while (*k) *p++ = *k++; *p++ = '='; p = hx(p, v); *p++ = ' '; return p; }
static void onBootFault(ERRF_ExceptionInfo *e, CpuRegisters *r) {
    static volatile int again; if (again++) svcExitProcess(); /* a fault inside this handler (e.g. the stack sample below) must not recurse */
    int g = (int)(((u8 *)e - guardStk[0]) / GUARD_STK); /* WRITE_DATA_TO_HANDLER_STACK: e and r sit in this thread's guardStk slot */
    const char *nm = g >= 0 && g < NGUARD && guardName[g] ? guardName[g] : "?"; char b[200], *p = b; u32 tid = 0; svcGetThreadId(&tid, CUR_THREAD_HANDLE);
    bcRaw("FAULT thread=", 13); bcRaw(nm, strlen(nm)); p = b; *p++ = ' '; p = tag(p, "tid", tid); p = tag(p, "type", e->type); p = tag(p, "fsr", e->fsr); p = tag(p, "far", e->far); *p++ = '\n'; bcRaw(b, p - b);
    p = b; p = tag(p, "pc", r->pc); p = tag(p, "lr", r->lr); p = tag(p, "sp", r->sp); p = tag(p, "cpsr", r->cpsr); *p++ = '\n'; bcRaw(b, p - b);
    p = b; p = tag(p, "fpexc", e->fpexc); p = tag(p, "fpinst", e->fpinst); p = tag(p, "fpinst2", e->fpinst2); *p++ = '\n'; bcRaw(b, p - b);
    for (int i = 0; i < 13; i += 4) { p = b; for (int k = i; k < i + 4 && k < 13; k++) { *p++ = 'r'; *p++ = '0' + k / 10; *p++ = '0' + k % 10; *p++ = '='; p = hx(p, r->r[k]); *p++ = ' '; } *p++ = '\n'; bcRaw(b, p - b); }
    if ((r->sp & 3) == 0 && r->sp >= 0x00100000 && r->sp < 0x40000000) { /* stack sample last: if sp itself is bad this read faults, after the lines above are saved */
        u32 *sk = (u32 *)r->sp; p = b; *p++ = 'S'; *p++ = 'T'; *p++ = 'K'; *p++ = ' '; for (int k = 0; k < 8; k++) { p = hx(p, sk[k]); *p++ = ' '; } *p++ = '\n'; bcRaw(b, p - b); }
    svcExitProcess();
}
void P3DS_ThreadGuard(const char *name) {
    int g = __sync_fetch_and_add(&guardN, 1); if (g >= NGUARD) return; guardName[g] = name;
    threadOnException(onBootFault, guardStk[g] + GUARD_STK, WRITE_DATA_TO_HANDLER_STACK);
}
static void __attribute__((constructor(101))) bcCtor(void) { mkdir("sdmc:/3ds", 0777); mkdir("sdmc:/3ds/platinum3ds", 0777); remove("sdmc:/3ds/platinum3ds/boot.txt"); }
int main(void) {
    P3DS_ThreadGuard("main");
    gfxInitDefault(); consoleInit(GFX_BOTTOM, &con); aptHook(&hookCookie, aptHookCb, NULL); uiInit();
    bool n3ds = false; APT_CheckNew3DS(&n3ds); if (n3ds) osSetSpeedupEnable(true);
    mkdir("sdmc:/3ds", 0777); mkdir("sdmc:/3ds/platinum3ds", 0777);
    { extern void P3DS_SpeedCheck(void); P3DS_SpeedCheck(); } /* perf pass 2026-11-14: logs whether 804 MHz / L2 is really on and each core's speed (speedcheck=0 skips); built since run 81 but never called here */
    mkdir("sdmc:/3ds", 0777); mkdir("sdmc:/3ds/platinum3ds", 0777);
    PSPNativeMemLog("[3DS] booted"); 
    { extern int P3DS_CfgInt(const char *, int); /* ss3ds 0070 reset_skip_intro: only right after a soft reset (marker < 30 s old) */
      FILE *f = fopen("sdmc:/3ds/platinum3ds/skipintro.tmp", "r");
      if (f) { long long t = 0; if (fscanf(f, "%lld", &t) != 1) t = 0; fclose(f); remove("sdmc:/3ds/platinum3ds/skipintro.tmp");
          const long long age = (long long)time(NULL) - t; const int cfg = P3DS_CfgInt("reset_skip_intro", 0);
          P3DS_SkipIntro = cfg && t > 0 && age >= 0 && age < 30; skipT0 = svcGetSystemTick();
          PSPNativeMemLog("[BOOT] soft reset marker, age %lld s, reset_skip_intro=%d -> skip %d", age, cfg, (int)P3DS_SkipIntro); } }
    PSPNativeMemLog("Platinum 3DS " SS3DS_VERSION " (%s %s) New3DS=%d linear free=%lu app free=%lu", __DATE__, __TIME__, n3ds,
                    (unsigned long)linearSpaceFree(), (unsigned long)osGetMemRegionFree(MEMREGION_APPLICATION));
    { struct mallinfo mi = mallinfo(); extern char *fake_heap_start, *fake_heap_end; PSPNativeMemLog("[3DS] malloc heap %lu KiB (used %lu)", (unsigned long)((fake_heap_end - fake_heap_start) >> 10), (unsigned long)mi.uordblks); }
    { extern u32 P3DS_HeapWaitMs, P3DS_HeapFreeFirst, P3DS_HeapFreeFinal; if (P3DS_HeapWaitMs) PSPNativeMemLog("[3DS] memory at start: %lu KiB free when loaded, %lu KiB after waiting %lu ms for the last run to give its memory back", (unsigned long)(P3DS_HeapFreeFirst >> 10), (unsigned long)(P3DS_HeapFreeFinal >> 10), (unsigned long)P3DS_HeapWaitMs); }
    bootMs = osGetTime(); { extern void P3DS_LogConfigHeader(const char *); P3DS_LogConfigHeader(SS3DS_VERSION); }
    { extern u32 P3DS_HeapFreeFinal; if (P3DS_HeapFreeFinal < (64u << 20)) { /* heapwait3ds.c: the last run never gave its memory back within 30 s */
        PSPNativeMemLog("[3DS] NOT ENOUGH MEMORY after the reset (%lu KiB, the game needs about 64 MiB): the previous run had not finished closing", (unsigned long)(P3DS_HeapFreeFinal >> 10));
        hold("Not enough memory after the reset:\nthe last run had not finished closing.\nPress START, then open the game again\nfrom the HOME Menu. Your save is safe."); } }
    uiStep("System", 1);
    STEP("Overlays", PSPNativeOverlay_Init(), "overlay init failed");
    { FILE *rf = fopen("sdmc:/3ds/platinum3ds/platinum.nds", "rb"); char hd[16] = {0}; int ok = rf && fread(hd, 1, 12, rf) == 12; if (rf) fclose(rf); /* run 64: plain error when the ROM is missing */
      if (!ok) { uiStep("ROM (platinum.nds)", 0); PSPNativeMemLog("[3DS] step ROM: FAIL (missing)"); hold("ROM not found. Put your Pokemon Platinum (US) ROM at sdmc:/3ds/platinum3ds/platinum.nds"); } }
    STEP("ROM (platinum.nds)", PSPNativeRomFS_SetPath("sdmc:/3ds/platinum3ds/platinum.nds"), "ROM not found. Put your Pokemon Platinum (US) ROM at sdmc:/3ds/platinum3ds/platinum.nds");
    { /* create a blank save only if none exists; never touch an existing one */
      FILE *f = fopen("sdmc:/3ds/platinum3ds/platinum.sav", "rb");
      if (f) fclose(f); else if ((f = fopen("sdmc:/3ds/platinum3ds/platinum.sav", "wb"))) { static u8 blk[4096]; memset(blk, 0xFF, sizeof blk); for (int i = 0; i < 128; i++) fwrite(blk, 1, sizeof blk, f); fclose(f); PSPNativeMemLog("[3DS] created blank platinum.sav"); } }
    p3SaveBackup("launch"); /* run 122: validated rotate */
    STEP("Save (platinum.sav)", PSPNative_OpenBackup("sdmc:/3ds/platinum3ds/platinum.sav"), "platinum.sav invalid: must be exactly 512 KiB");
    { extern int P3DS_GameFontLoad(void); P3DS_GameFontLoad(); } /* run 64: game font from the ROM for port UI */
    PSPNativeStackProbeInit(); STEP("Stack probe", 1, "");
    WIN_Init_sTexStartAddrTable(); WIN_Init_sTexPlttStartAddrTable(); STEP("Texture tables", 1, "");
    CTRDG_Init(); STEP("Cartridge slot", 1, "");
    { /* run 41 fix: build followers.bin here (first run only, can take 10+ s) instead of on the first field frame,
         where the watchdog halted the game. A failed build is not fatal: visible wild Pokemon just stay off. */
      extern int P3DS_WildVisCfg(int); extern int P3DS_FollowerOpen(void);
      if (P3DS_WildVisCfg(0)) { FILE *ff = fopen("sdmc:/3ds/platinum3ds/followers.bin", "rb");
        if (ff) fclose(ff); else { PSPNativeMemLog("[FOL] followers.bin missing: building it from the ROM before the game starts"); uiStep("Wild sprites (first run)", P3DS_FollowerOpen()); } } }
    PSPNativeFrameInit(); STEP("Renderer + audio", 1, "");
    { extern int P3DS_CfgInt(const char *, int); /* online trading (0137): 3DS sockets for the game's own Wi-Fi code; the game may never call WM init on that path, so start them here */
      { extern int P3DS_NetInit(void); PSPNativeMemLog("[NET] auto wireless at boot: sockets %s", P3DS_NetInit() ? "up" : "unavailable"); } }
    STEP("Starting game", 1, ""); PSPNativeMemLog("[3DS] entering NitroMain");
    threadOnException(onCrash, excStack + sizeof excStack, WRITE_DATA_TO_HANDLER_STACK); gameStarted = 1; wdTh = threadCreate(watchdog, NULL, 0x4000, 0x20, -2, false); /* joinable: hardExit joins it (it leaves its loop on quitting) */
    NitroMain(); 
    hold("NitroMain returned");
    return 0;
}
/* Polling/wait points the game can spin on: breadcrumbed, and the box test can never report "busy". */
extern int __real_G3X_GetBoxTestResult(s32 *in);
unsigned p3dsBoxVisW, p3dsBoxHidW;
static int boxForce(void) { static int v = -1; if (v < 0) { v = 0; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "g3_boxtest_force=%d", &x) == 1) v = x != 0; fclose(f); } PSPNativeMemLog("[G3] g3_boxtest_force=%d (1 = every BoxTest reports visible)", v); } return v; }
int __wrap_G3X_GetBoxTestResult(s32 *in) { int r = __real_G3X_GetBoxTestResult(in);
    if (r) { CRUMB("boxtest-busy"); *in = 1; r = 0; }
    if (*in) p3dsBoxVisW++; else p3dsBoxHidW++;
    if (boxForce()) *in = 1; return r; }
#ifdef SS3DS
/* wireless (wm_uds.c): WM callbacks run between frames like the DS FIFO interrupt, as in pspoke's Platinum frame.c */
extern void PSPNativeWMPump(void); extern void __real_PSPNativeFrameComplete(void);
void __wrap_PSPNativeFrameComplete(void) { PSPNativeWMPump(); __real_PSPNativeFrameComplete(); }
#endif
/* run 95 [PERF4]: ticks the game thread spends blocked in its own V-blank / IRQ waits (idle, not work) */
volatile unsigned long long P3DS_WaitTicks; volatile unsigned P3DS_WaitCalls;
extern void __real_OS_WaitVBlankIntr(void); void __wrap_OS_WaitVBlankIntr(void) { CRUMB("waitvblank");
    { extern void P3DS_Fps60Poll(void); P3DS_Fps60Poll(); }   /* fps60: Platinum's main loop waits here (SoulSilver waits in OS_WaitIrq) */
    u64 t = svcGetSystemTick(); __real_OS_WaitVBlankIntr(); P3DS_WaitTicks += svcGetSystemTick() - t; P3DS_WaitCalls++; }
/* [PERF4] (2026-10-27): time the game thread spends idle in its VBlank waits. The game paces itself at 30 per second with
   these waits, and they fall inside the [PERF] "g3soft" share, which therefore is not all CPU work. */
volatile unsigned long long P3DS_VblIdleTicks;
extern void __real_OS_WaitIrq(int c, u32 m);
static void waitIrqBody(int c, u32 m) {
    { extern void P3DS_Fps60Poll(void); P3DS_Fps60Poll(); }   /* fps60: hand the waiting real picture to the LCD if it is time (never waits) */
#ifdef SS3DS
    if (m & 1u) { static int on = -1; static unsigned n; extern int P3DS_CfgInt(const char *, int);
        if (on < 0) { on = P3DS_CfgInt("vram_frame_gen", 1) != 0; PSPNativeMemLog("[G3] vram_frame_gen=%d (texture cache rechecks VRAM every VBlank wait)", on); }
        if (on) { extern void SS3DS_VramFrame(void); SS3DS_VramFrame(); if (++n == 600) { n = 0; extern unsigned PSPNativeVramGen; PSPNativeMemLog("[G3] vram_frame_gen: 600 VBlank waits, VRAM gen %u", PSPNativeVramGen); } } }
#endif
    /* fps60 / FpsTweaks >= 5: how much of each 33 ms game tick in the overworld is spent working vs idle in the VBlank waits. */
    if (m & 1u) { static int pr = -1; static u64 t0, idle; static unsigned waits; extern int P3DS_InField; extern int P3DS_FpsTweaks(void);
        if (pr < 0) { pr = 0; PSPNativeMemLog("[FPS] fps60_probe=%d (busy/idle time per second in the overworld, triangle order stability)", pr); }
        if ((pr || P3DS_FpsTweaks() >= 5) && P3DS_InField) { u64 a = svcGetSystemTick(); if (!t0) t0 = a; __real_OS_WaitIrq(c, m); u64 b = svcGetSystemTick(); idle += b - a; waits++; P3DS_VblIdleTicks += b - a;
            if (b - t0 >= 10 * 268111856ull) { double el = (double)(b - t0) / 268111.856, id = (double)idle / 268111.856;
                PSPNativeMemLog("[FPS] field 10 s: %u VBlank waits (%.1f/s), idle %.0f ms, busy %.0f ms = %.1f ms per 33.3 ms tick (%.0f%% busy)", waits, waits * 1000.0 / el, id, el - id, (el - id) * 2.0 / (waits ? waits : 1), 100.0 * (el - id) / el);
                t0 = b; idle = 0; waits = 0; }
            return; }
        t0 = 0; idle = 0; waits = 0; }
    if (m & 1u) { const u64 a = svcGetSystemTick(); __real_OS_WaitIrq(c, m); P3DS_VblIdleTicks += svcGetSystemTick() - a; return; }
    __real_OS_WaitIrq(c, m); }
void __wrap_OS_WaitIrq(int c, u32 m) { CRUMB("waitirq"); u64 t = svcGetSystemTick(); waitIrqBody(c, m); P3DS_WaitTicks += svcGetSystemTick() - t; P3DS_WaitCalls++; }
extern void __real_MI_WaitDma(u32 ch); void __wrap_MI_WaitDma(u32 ch) { CRUMB("waitdma"); __real_MI_WaitDma(ch); }
/* H-blank DMA (unk_02014A08, sub_02014AB0): the game's per-line scroll wave aims MI_HBlankDmaCopy32/16 at a DS BG
   register (0x04000010 = BG0HOFS/VOFS, one value per screen line). It runs for the water/surf wild battle intro (ov120
   effects 2/3) and other wavy effects. libntr's port is a plain memcpy, so on 3DS it wrote to a raw DS IO address and
   crashed (runs 939/940: [CRASH] pc in memcpy, r0=04000010, right after "native load id=120").
   An engine A BG/window register target is recorded instead (source table, register, bytes a line) and render3ds.cpp
   (hbSnap/hbLine) writes each line's value into engine A before drawing that line, as the DMA would: the wave plays.
   The game re-arms it every VBlank (MI_StopDma, then this) and MI_StopDma ends it. Any other DS IO target (engine B)
   is skipped and logged; a non-IO target passes through. */
volatile const void *P3DS_HbSrc; volatile unsigned P3DS_HbReg, P3DS_HbUnit, P3DS_HbSeq; static volatile int sHbCh = -1;
static int SS3DS_IsDsIoAddr(const void *p) { const unsigned v = (unsigned)(uintptr_t)p; return v >= 0x04000000u && v < 0x04800000u; }
static int SS3DS_HBlankTake(const char *who, u32 dmaNo, const void *src, void *dest, u32 size, unsigned unit) {
    const unsigned v = (unsigned)(uintptr_t)dest;
    if (!SS3DS_IsDsIoAddr(dest)) return 0;
    if (v >= 0x04000008u && v + unit <= 0x04000058u && !(v & 1u) && size == unit && src) {
        static unsigned n; if (n < 3) { n++; PSPNativeMemLog("[HBLANK] %s ch %u -> engine A reg %03x, %u bytes a line: line wave drawn by the renderer", who, (unsigned)dmaNo, v - 0x04000000u, unit); }
        P3DS_HbSrc = 0; P3DS_HbReg = v - 0x04000000u; P3DS_HbUnit = unit; sHbCh = (int)dmaNo; P3DS_HbSrc = src; P3DS_HbSeq++;
        return 1; }
    { static unsigned n; if (n < 4) { n++; PSPNativeMemLog("[HBLANK] %s ch %u -> DS register %08x (%u bytes a line) skipped: not an engine A BG register", who, (unsigned)dmaNo, v, (unsigned)size); } }
    return 1; }
extern void __real_MI_HBlankDmaCopy32(u32 dmaNo, const void *src, void *dest, u32 size);
void __wrap_MI_HBlankDmaCopy32(u32 dmaNo, const void *src, void *dest, u32 size) {
    if (SS3DS_HBlankTake("MI_HBlankDmaCopy32", dmaNo, src, dest, size, 4)) return;
    __real_MI_HBlankDmaCopy32(dmaNo, src, dest, size); }
extern void __real_MI_HBlankDmaCopy16(u32 dmaNo, const void *src, void *dest, u32 size);
void __wrap_MI_HBlankDmaCopy16(u32 dmaNo, const void *src, void *dest, u32 size) {
    if (SS3DS_HBlankTake("MI_HBlankDmaCopy16", dmaNo, src, dest, size, 2)) return;
    __real_MI_HBlankDmaCopy16(dmaNo, src, dest, size); }
extern void __real_MI_StopDma(u32 dmaNo);
void __wrap_MI_StopDma(u32 dmaNo) { if (sHbCh == (int)dmaNo) { P3DS_HbSrc = 0; sHbCh = -1; } __real_MI_StopDma(dmaNo); }
/* Texture-load call chain breadcrumbs (run-8 stall: endless GX_EndLoadTex/Pltt on the main thread). */
extern int __real_Easy3D_UploadTextureToVRAM(void *t);
int __wrap_Easy3D_UploadTextureToVRAM(void *t) { static unsigned n; if ((n++ & 255) == 0) PSPNativeMemLog("[TEX] Easy3D_Upload #%u tex=%p from %p", n, t, __builtin_return_address(0)); CRUMB("easy3d-upload"); return __real_Easy3D_UploadTextureToVRAM(t); }
extern int __real_NNS_G3dResDefaultSetup(void *r);
int __wrap_NNS_G3dResDefaultSetup(void *r) { static unsigned n; if ((n++ & 255) == 0) PSPNativeMemLog("[TEX] ResDefaultSetup #%u res=%p from %p", n, r, __builtin_return_address(0)); CRUMB("resdefsetup"); return __real_NNS_G3dResDefaultSetup(r); }
extern void __real_TextureResource_UploadToVRam(void *r);
void __wrap_TextureResource_UploadToVRam(void *r) { CRUMB("texres-upload"); __real_TextureResource_UploadToVRam(r); }
extern void __real_NNS_GfdDoVramTransfer(void);
void __wrap_NNS_GfdDoVramTransfer(void) { CRUMB("vramxfer"); __real_NNS_GfdDoVramTransfer(); }
extern int P3DS_CfgInt(const char *, int);
/* run 63 vram_trace (debug, log_level 2): VRAM transfer task requests / failures / process calls per 2 s. */
extern int __real_VramTransfer_Request(int type, unsigned dest, void *buf, unsigned size); extern void __real_VramTransfer_Process(void);
static unsigned vtReq, vtFail, vtProc, vtMaxPer, vtPer; static u64 vtT;
static void vtLog(void) { u64 now = svcGetSystemTick(); if (now - vtT > 268111856ull * 2) { vtT = now; static int vt = -1; if (vt < 0) vt = 0 /* vram_trace: debug option removed */; /* run 66: cached */ if (vt) PSPNativeMemLog("[VXFER] 2s: requests %u failed %u process %u max/frame %u", vtReq, vtFail, vtProc, vtMaxPer); vtReq = vtFail = vtProc = vtMaxPer = 0; } }
int __wrap_VramTransfer_Request(int type, unsigned dest, void *buf, unsigned size) { int r = __real_VramTransfer_Request(type, dest, buf, size); vtReq++; vtPer++; if (!r) vtFail++; return r; }
/* run 63 heap_watch=<decimal address> (debug, default 0): log every game heap alloc covering that address and every
   free of a block starting up to 64 KiB below it, with the caller (finds use-after-free / reuse of a watched object). */
extern void *__real_Heap_Alloc(u32 h, u32 n); extern void *__real_Heap_AllocAtEnd(u32 h, u32 n); extern void __real_Heap_Free(void *p); extern void __real_Heap_FreeExplicit(u32 h, void *p);
static unsigned hwAddr = 1; static unsigned hwA(void) { if (hwAddr == 1) hwAddr = (unsigned)0 /* heap_watch: debug option removed */; return hwAddr; }
static void hwChk(const char *k, void *p, u32 h, u32 n, void *ra) { unsigned a = hwA(), q = (unsigned)p; if (!a || !p) return;
  if (n ? (q <= a && a < q + n) : (q <= a && a - q < 0x10000)) PSPNativeMemLog("[HEAPW] %s %p heap %lu size %lu from %p (watch %x)", k, p, (unsigned long)h, (unsigned long)n, ra, a); }
/* run 681 crash (Heap_Alloc -> NNS_FndAllocFromExpHeapEx, NULL heap handle): the game allocated from a heap id whose heap is not
   created (destroyed or never made). The game's GF_ASSERT does not stop here, so it dereferenced NULL. sHeapInfo sits at DS RAM
   0x020d10c0 (heapHandles @0, heapIdxs @16, totalNumHeaps @20, as the compiled Heap_Alloc reads it). Use the default heap instead. */
/* run 688 fix: the address of sHeapInfo moves with every build (run 687 crashed in this guard reading the old fixed 0x020d10c0). Read it
   from the compiled Heap_Alloc itself instead: its first literal load is the sHeapInfo address and it reads +0x14 as a halfword. If that
   pattern is not found the guard does nothing (never dereferences a guessed address). */
static const u32 *heapInfoAddr(void) {
    static const u32 *cached; static int done; if (done) return cached; done = 1;
    const u32 *fn = (const u32 *)(uintptr_t)__real_Heap_Alloc;
    for (int i = 0; i < 10; i++) { const u32 w = fn[i];
        if ((w & 0x0f7f0000u) != 0x051f0000u || !(w & 0x00800000u)) continue;               /* ldr rd, [pc, #+imm] */
        const u32 rd = (w >> 12) & 15, lit = *(const u32 *)((const u8 *)&fn[i] + 8 + (w & 0xfff)); const u32 *hi = (const u32 *)(uintptr_t)lit;
        for (int j = i + 1; j < i + 6; j++) { const u32 x = fn[j];                           /* ldrh rt, [rd, #0x14] */
            if ((x & 0xfff00000u) == 0xe1d00000u && ((x >> 16) & 15) == rd && (x & 0xf0u) == 0xb0u && ((((x >> 4) & 0xf0u) | (x & 0xfu)) == 0x14)) {
                const unsigned total = *(const u16 *)((const u8 *)hi + 20);
                if (total >= 1 && total <= 256 && hi[0] && hi[4]) { cached = hi; PSPNativeMemLog("[HEAP] heap guard: sHeapInfo at %p (read from Heap_Alloc), %u heap ids", (const void *)hi, total); return cached; } } } }
    PSPNativeMemLog("[HEAP] heap guard: sHeapInfo not found in Heap_Alloc, guard off"); return cached; }
static u32 heapGuard(u32 h, const char *k, u32 n, void *ra) {
    const u32 *hi = heapInfoAddr(); if (!hi) return h;
    const void *const *handles = (const void *const *)hi[0]; const u8 *idx = (const u8 *)hi[4];
    unsigned total = *(const u16 *)((const u8 *)hi + 20);
    if (h < total && handles && idx && !handles[idx[h]]) { static unsigned c; if (c++ < 20) PSPNativeMemLog("[HEAP] %s on heap id %lu with no heap (size %lu, from %p): using the default heap (#%u)", k, (unsigned long)h, (unsigned long)n, ra, c); return 0; }
    return h; }
void *__wrap_Heap_Alloc(u32 h, u32 n) { h = heapGuard(h, "Heap_Alloc", n, __builtin_return_address(0)); void *p = __real_Heap_Alloc(h, n); hwChk("alloc", p, h, n, __builtin_return_address(0)); return p; }
void *__wrap_Heap_AllocAtEnd(u32 h, u32 n) { h = heapGuard(h, "Heap_AllocAtEnd", n, __builtin_return_address(0)); void *p = __real_Heap_AllocAtEnd(h, n); hwChk("allocEnd", p, h, n, __builtin_return_address(0)); return p; }
void __wrap_Heap_Free(void *p) { hwChk("free", p, 0, 0, __builtin_return_address(0)); __real_Heap_Free(p); }
void __wrap_Heap_FreeExplicit(u32 h, void *p) { hwChk("freeX", p, h, 0, __builtin_return_address(0)); __real_Heap_FreeExplicit(h, p); }
void __wrap_VramTransfer_Process(void) { __real_VramTransfer_Process(); vtProc++; if (vtPer > vtMaxPer) vtMaxPer = vtPer; vtPer = 0; vtLog(); }
extern void __real_NNS_G3dTexLoad(void *t, int e);
void __wrap_NNS_G3dTexLoad(void *t, int e) { static unsigned n; if ((n++ & 255) == 0) PSPNativeMemLog("[TEX] G3dTexLoad #%u tex=%p from %p", n, t, __builtin_return_address(0)); CRUMB("g3d-texload"); __real_NNS_G3dTexLoad(t, e); }
extern void __real_NNS_G3dPlttLoad(void *t, int e);
void __wrap_NNS_G3dPlttLoad(void *t, int e) { CRUMB("g3d-plttload"); __real_NNS_G3dPlttLoad(t, e); }

/* run 34: sound start diagnostics ([AUDIO-SEQ]): every NNS seq/seqarc start with its result. */
typedef struct NNSSndHandle NNSSndHandle;
extern int __real_NNS_SndArcPlayerStartSeq(NNSSndHandle *h, int seq);
extern int __real_NNS_SndArcPlayerStartSeqEx(NNSSndHandle *h, int pl, int bank, int prio, int seq);
extern int __real_NNS_SndArcPlayerStartSeqArc(NNSSndHandle *h, int arc, int idx);
extern int __real_NNS_SndArcPlayerStartSeqArcEx(NNSSndHandle *h, int pl, int bank, int prio, int arc, int idx);
static unsigned seqLogN, seqFail;
static void seqLog(const char *k, int a, int b, int c, int d, int r) { if (!r) seqFail++; if (seqLogN < 400 || !r) { seqLogN++; PSPNativeMemLog("[AUDIO-SEQ] %s %d %d pl=%d bank=%d -> %s (fails %u)", k, a, b, c, d, r ? "ok" : "FAIL", seqFail); } }
int __wrap_NNS_SndArcPlayerStartSeq(NNSSndHandle *h, int seq) { int r = __real_NNS_SndArcPlayerStartSeq(h, seq); seqLog("seq", seq, -1, -1, -1, r); return r; }
int __wrap_NNS_SndArcPlayerStartSeqEx(NNSSndHandle *h, int pl, int bank, int prio, int seq) { int r = __real_NNS_SndArcPlayerStartSeqEx(h, pl, bank, prio, seq); seqLog("seqEx", seq, prio, pl, bank, r); return r; }
int __wrap_NNS_SndArcPlayerStartSeqArc(NNSSndHandle *h, int arc, int idx) { int r = __real_NNS_SndArcPlayerStartSeqArc(h, arc, idx); seqLog("arc", arc, idx, -1, -1, r); return r; }
int __wrap_NNS_SndArcPlayerStartSeqArcEx(NNSSndHandle *h, int pl, int bank, int prio, int arc, int idx) { int r = __real_NNS_SndArcPlayerStartSeqArcEx(h, pl, bank, prio, arc, idx); seqLog("arcEx", arc, idx, pl, bank, r); return r; }
/* run 57 cam_zoom: overworld camera distance step -2..+2 (0 = original). Applied by FieldCamera_Create (overlay 5
   patch) and live from the settings menu on the current field camera (relative to the step it was created with). */
extern int P3DS_CfgInt(const char *, int);
extern int Camera_GetDistance(const void *camera); extern void Camera_SetDistance(int distance, void *camera);
void *P3DS_FieldCam; int P3DS_FieldCamStep; static int camZoom = -99;
int P3DS_CamZoomPct(int step) { static const int pct[5] = { 86, 93, 100, 108, 116 }; if (step < -2) step = -2; if (step > 2) step = 2; return pct[step + 2]; }
int P3DS_CamZoom(void) { if (camZoom == -99) { camZoom = P3DS_CfgInt("cam_zoom", 0); if (camZoom < -2) camZoom = -2; if (camZoom > 2) camZoom = 2; PSPNativeMemLog("[CAM] cam_zoom=%d", camZoom); } return camZoom; }
void P3DS_SetCamZoom(int z) { if (z < -2) z = -2; if (z > 2) z = 2; P3DS_CamZoom(); camZoom = z;
    if (P3DS_FieldCam && z != P3DS_FieldCamStep) { int d = Camera_GetDistance(P3DS_FieldCam); long long base = (long long)d * 100 / P3DS_CamZoomPct(P3DS_FieldCamStep);
        Camera_SetDistance((int)(base * P3DS_CamZoomPct(z) / 100), P3DS_FieldCam); P3DS_FieldCamStep = z; PSPNativeMemLog("[CAM] live zoom %d (distance %d -> %d)", z, d, (int)(base * P3DS_CamZoomPct(z) / 100)); } }

/* run 68: GF_ASSERT carries no file/line, so log WHO failed: the caller's return address (resolve with
   arm-none-eabi-addr2line -f -e soulsilver3ds.elf <addr>) and the caller's caller, always (log_level 1). */
extern void __real_ErrorHandling_AssertFail(void);
void __wrap_ErrorHandling_AssertFail(void) {
    void *a = __builtin_return_address(0);
    PSPNativeMemLog("[ASSERT] GF_ASSERT failed: caller %p (addr2line -f -e soulsilver3ds.elf %p)", a, (char *)a - 4);
    P3DS_CrumbDump();
    __real_ErrorHandling_AssertFail(); }

#include <unistd.h>
/* platinum3ds: 3DS entry for the full pspoke game build (replaces native-audio-app/main.c). */
#include <3ds.h>
#include <nitro.h>
#include <stdio.h>
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
static void uiInit(void) { consoleSelect(&con); consoleClear(); if (isQuiet()) return; /* run 64 clean launch: quiet boot = plain black (the credits splash is drawn by render3ds) */ __real_printf("\x1b[1;1H\x1b[36;1m  Platinum 3DS\x1b[0m  \x1b[2mdev build\x1b[0m\n  ------------------------------\n"); }
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
    for (const char *p = why; *p && r < 28; r++) { int n = 0; while (p[n] && n < 36) n++; if (p[n] && n == 36) { int k = n; while (k > 8 && p[k] != ' ') k--; if (p[k] == ' ') n = k; } /* run 64: wrap at spaces */ snprintf(line, sizeof line, "%.*s", n, p); __real_printf("\x1b[%d;2H %-37s", r, line); p += n; while (*p == ' ') p++; }
    __real_printf("\x1b[%d;2H%-38s\x1b[0m\x1b[%d;2H  START: exit   SELECT: power off", r, "", r + 2);
}
#define STEP(name, cond, failmsg) do { int ok_ = (cond); uiStep(name, ok_); PSPNativeMemLog("[3DS] step %s: %s", name, ok_ ? "ok" : "FAIL"); if (!ok_) hold(failmsg); } while (0)

int P3DS_ConsoleOnBottom(void) { return conOnBottom; }
/* Clean console power-off from the error box (log is written line by line; sync the SD card first). */
static void powerOff(void) {
    PSPNativeMemLog("[3DS] power off requested"); svcSleepThread(200000000LL);
    if (R_SUCCEEDED(ptmSysmInit())) { PTMSYSM_ShutdownAsync(0); ptmSysmExit(); svcSleepThread(5000000000LL); }
    APT_HardwareResetAsync(); for (;;) svcSleepThread(1000000000LL);
}
extern void P3DS_LogFlushSync(void);
static void hold(const char *why) { P3DS_LogFlushSync();
    PSPNativeMemLog("[3DS] halted: %s  (press START to exit)", why);
    uiError(why);
    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_START) break; if (hidKeysDown() & KEY_SELECT) powerOff(); gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank(); }
    hardExit(1);
}
static volatile int quitting;
static aptHookCookie hookCookie;
extern void P3DS_ShutdownThreads(void); extern void P3DS_ShutdownSDL(void);
/* Clean exit: stop workers, join them, close files, gfxExit, then exit() normally (returns to HBL / HOME). */
static void hardExit(int code) {
    static int once; if (once++) exit(code);
    quitting = 1; PSPNativeMemLog("[3DS] exit: stopping threads");
    P3DS_ShutdownThreads(); P3DS_ShutdownSDL();
    extern BOOL PSPNative_CloseBackup(void); PSPNative_CloseBackup();
    PSPNativeMemLog("[3DS] exit: clean");
    P3DS_LogFlushSync(); aptUnhook(&hookCookie); gfxExit(); exit(code);
}
static char lastMsg[200];
#include "crumbs.h"
typedef struct { const char *tag; void *caller; unsigned n; int th; unsigned frame; } Crumb;
/* ring 0 = main game thread, ring 1 = every other thread (VBlank/alarm/audio work must not hide the main thread). */
static Crumb crumbs[2][64]; static volatile unsigned crumbPos[2];
extern unsigned PSPNativeRenderFrameCount(void);
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
    char b[300]; snprintf(b, sizeof b, "abort() from %p. last msg: %s", __builtin_return_address(0), lastMsg[0] ? lastMsg : "(none)");
    hold(b); for (;;) {}
}
/* printf/puts from the port go to the card log too (console alone scrolls away). */
#include <stdarg.h>
/* verbose=1 in config.txt keeps the per-grain/key-on audio spam in log.txt */
static int verboseLog = -1;
static int isVerbose(void) { if (verboseLog < 0) { verboseLog = 0; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int v; while (fgets(l, sizeof l, f)) if (sscanf(l, "verbose=%d", &v) == 1) verboseLog = v; fclose(f); } } return verboseLog; }
int __wrap_printf(const char *fmt, ...) { if (!strncmp(fmt, "[AUDIO-SAS] grain", 17) || !strncmp(fmt, "[AUDIO-SAS] key", 15)) { if (!isVerbose()) return 0; } char b[400]; va_list ap; va_start(ap, fmt); int n = vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    size_t l = strlen(b); while (l && b[l - 1] == '\n') b[--l] = 0; if (l) { PSPNativeMemLog("%s", b); snprintf(lastMsg, sizeof lastMsg, "%s", b); } return n; }
int __wrap_puts(const char *s) { PSPNativeMemLog("%s", s); snprintf(lastMsg, sizeof lastMsg, "%s", s); return 1; }
static volatile int lidEvt; /* run 64 lid_sound: 1 = closed (sleep), 2 = opened (wake); consumed on the game thread */
static void aptHookCb(APT_HookType t, void *p) { if (t == APTHOOK_ONEXIT) { quitting = 1; } else if (t == APTHOOK_ONSLEEP) lidEvt = 1; else if (t == APTHOOK_ONWAKEUP) lidEvt = 2; }

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
int P3DS_ScaleMode(void) {
    if (scaleMode < 0) { scaleMode = 1; FILE *f = fopen(CFG_PATH, "r"); if (f) { char line[64]; int v; while (fgets(line, sizeof line, f)) if (sscanf(line, "gpuscale=%d", &v) == 1 && v >= 0 && v <= 2) scaleMode = v; fclose(f); } }
    return scaleMode;
}
/* Rewrites only the gpuscale= line. The old version opened config.txt with "w" and wrote just gpuscale=N,
   wiping every other setting (gpu3d_*, cull3d, audio_*, ...) on each L+R+SELECT. */
void P3DS_SetScaleMode(int m) { void saveScale(void); scaleMode = m < 0 || m > 2 ? 1 : m; saveScale(); }
void saveScale(void) {
    static char buf[8192]; size_t n = 0; FILE *f = fopen(CFG_PATH, "r");
    if (f) { char line[256]; while (fgets(line, sizeof line, f)) { if (!strncmp(line, "gpuscale=", 9) || !strncmp(line, "scale=", 6)) continue; size_t l = strlen(line); if (n + l + 2 < sizeof buf) { memcpy(buf + n, line, l); n += l; if (line[l - 1] != '\n') buf[n++] = '\n'; } } fclose(f); }
    n += snprintf(buf + n, sizeof buf - n, "gpuscale=%d\n", scaleMode);
    f = fopen(CFG_PATH, "w"); if (f) { fwrite(buf, 1, n, f); fclose(f); } { extern void P3DS_CfgInvalidate(void); P3DS_CfgInvalidate(); }
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
    extern int P3DS_CfgSoftReset(void);
    int how = 0;
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
    hardExit(0);
}
void P3DS_FrameHook(void) {
    static int held;
    u32 k = hidKeysHeld();
    /* run 53: L+R+START+SELECT = soft reset. Remember that the combo was seen until L and R are released, so the
       L+R+SELECT (scale) and L+R+START (FPS, render3ds) toggles fire on RELEASE and only if the reset combo wasn't hit. */
    { const u32 r4 = KEY_L | KEY_R | KEY_START | KEY_SELECT; if ((k & r4) == r4) P3DS_ResetSeen = 1; else if (!(k & (KEY_L | KEY_R))) P3DS_ResetSeen = 0; }
    if ((k & (KEY_L | KEY_R)) == (KEY_L | KEY_R) && (hidKeysUp() & KEY_SELECT) && !P3DS_ResetSeen) { scaleMode = (P3DS_ScaleMode() + 1) % 3; saveScale(); { extern void P3DS_Toast(const char *); static const char *const nm[3] = { "SCALE 1:1", "SCALE 1.25X", "SCALE FILL" }; P3DS_Toast(nm[scaleMode % 3]); } }
    /* run 45: L+R+Y (GPU/CPU 3D toggle) retired; the CPU 3D renderer is only an automatic fallback if GPU init fails, or config gpu3d=0 */
    if ((k & (KEY_L | KEY_R)) == (KEY_L | KEY_R) && (hidKeysDown() & KEY_A)) { /* run 52: sharp 3D disabled -> L+R+A does nothing (still swallowed) */ } /* run 47: L+R+A = res3d 0/2 (was the zinv debug toggle; zinv stays config-only gpu3d_zinv) */
    if ((k & (KEY_SELECT | KEY_START)) == (KEY_SELECT | KEY_START) && !(k & (KEY_L | KEY_R))) { if (++held == 60) { if (conOnBottom) bottomToGame(); else uiShowChecklist(); } } else held = 0;
    if (quitting || !aptMainLoop()) hardExit(0);
    /* run 64 lid_sound=1 (default; run 65: Poke Ball recall SE on close, send-out SE on open, see P3DS_PlayLidSE).
       Only in the overworld (sound system up, no battle/cutscene music to interrupt). Played on the game thread. */
    { static int lt = -1, n; extern int P3DS_CfgInt(const char *, int); extern int P3DS_InField; if (lt < 0) lt = P3DS_CfgInt("lid_test", 0) != 0;
      if (lt && P3DS_InField) { ++n; if (n == 300) lidEvt = 1; else if (n == 420) lidEvt = 2; } } /* run 65 debug: fake a lid close/open in the overworld */
    if (lidEvt) { int e = lidEvt; lidEvt = 0; static int ls = -1; extern int P3DS_CfgInt(const char *, int); extern int P3DS_InField;
        if (ls < 0) ls = P3DS_CfgInt("lid_sound", 1) != 0;
        if (ls && P3DS_InField) { extern void P3DS_PlayLidSE(int); P3DS_PlayLidSE(e == 2); }
        PSPNativeMemLog("[3DS] lid %s (sound %d)", e == 1 ? "closed" : "opened", ls && P3DS_InField); }
}
static int touchPrev;
/* 3DS bottom touch -> DS touch screen (256x192 centred at +32,+24 on the 320x240 panel). */
int P3DS_TouchRead(int *x, int *y) {
    if (conOnBottom) return 0;
    { extern volatile int P3DS_PromptOpen; if (P3DS_PromptOpen == 1) return 0; } /* run 64 repel_prompt */
    touchPosition t; int dx, dy;
    { /* run 53 cstick_poketch=1 (default): C-stick up/down = tap the Poketch's side button (next / previous app),
         only in the overworld and only when the stylus isn't down. One short tap per flick (stick must return). */
      static int cfg = -1, armed = 1, inj, injY; extern int P3DS_CfgInt(const char *, int); extern int P3DS_InField;
      { extern int P3DS_CstickMode(void); cfg = P3DS_CstickMode() == 1; }  /* run 54: cstick_mode=1 (run 55: live) */
      if (cfg && !(hidKeysHeld() & KEY_TOUCH)) { circlePosition cs; hidCstickRead(&cs);
          if (cs.dy > -40 && cs.dy < 40) armed = 1;
          if (!inj && armed && P3DS_InField && (cs.dy >= 80 || cs.dy <= -80)) { armed = 0; inj = 4; injY = cs.dy > 0 ? 64 : 128; }
          if (inj) { inj--; *x = 240; *y = injY; touchPrev = 1; return 1; } } }
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
    if (dx < 0 || dx > 255 || dy < 0 || dy > 191) return 0;
    { extern int P3DS_StripOn; if (P3DS_StripOn && dy >= 176 && dx >= 8 && dx < 208) { touchPrev = 1; return 0; } } /* run 78 bottom strip owns these taps */
    *x = dx; *y = dy; touchPrev = 1; return 1;
}
/* Watchdog: if no game frame is presented for ~3 s after the game started, log the breadcrumb and show the
   red box from this (higher-priority) thread so a stuck game thread cannot hard-lock the console. */
extern unsigned PSPNativeRenderFrameCount(void);
static volatile int gameStarted; static Thread wdTh;
static void watchdog(void *p) {
    unsigned last = 0; int still = 0;
    while (!quitting) {
        svcSleepThread(500000000LL);
        if (!gameStarted) continue;
        { extern volatile int P3DS_MenuOpen; if (P3DS_MenuOpen) { still = 0; continue; } }  /* run 55: settings menu pauses on purpose */
        unsigned f = PSPNativeRenderFrameCount();
        if (f != last) { last = f; still = 0; continue; }
        if (++still == 20) { P3DS_CrumbDump();
            char b[300]; snprintf(b, sizeof b, "game stalled: no frame for 10 s at frame %u (crumbs in log.txt). main last: %s %p. msg: %s", f, crumbs[0][(crumbPos[0]-1)&63].tag ? crumbs[0][(crumbPos[0]-1)&63].tag : "(none)", crumbs[0][(crumbPos[0]-1)&63].caller, lastMsg[0] ? lastMsg : "(none)");
            PSPNativeMemLog("[WATCHDOG] %s", b); hold(b);
        }
    }
}
static u8 excStack[0x8000] __attribute__((aligned(8)));
static void onCrash(ERRF_ExceptionInfo *e, CpuRegisters *r) {
    PSPNativeMemLog("[CRASH] type=%d pc=%08lx lr=%08lx sp=%08lx far=%08lx fsr=%08lx r0=%08lx r4=%08lx", e->type, r->pc, r->lr, r->sp, e->far, e->fsr, r->r[0], r->r[4]);
    P3DS_CrumbDump(); P3DS_LogFlushSync();
    char b[200]; snprintf(b, sizeof b, "game crashed (type %d) pc=%08lx lr=%08lx - details in log.txt", e->type, r->pc, r->lr); hold(b);
    svcExitProcess();
}
int main(void) {
    gfxInitDefault(); consoleInit(GFX_BOTTOM, &con); aptHook(&hookCookie, aptHookCb, NULL); uiInit();
    bool n3ds = false; APT_CheckNew3DS(&n3ds); if (n3ds) osSetSpeedupEnable(true);
    mkdir("sdmc:/3ds", 0777); mkdir("sdmc:/3ds/platinum3ds", 0777);
    PSPNativeMemLog("[3DS] booted");
    PSPNativeMemLog("platinum3ds full build (%s %s) New3DS=%d linear free=%lu app free=%lu", __DATE__, __TIME__, n3ds,
                    (unsigned long)linearSpaceFree(), (unsigned long)osGetMemRegionFree(MEMREGION_APPLICATION));
    { struct mallinfo mi = mallinfo(); extern char *fake_heap_start, *fake_heap_end; PSPNativeMemLog("[3DS] malloc heap %lu KiB (used %lu)", (unsigned long)((fake_heap_end - fake_heap_start) >> 10), (unsigned long)mi.uordblks); }
    uiStep("System", 1);
    STEP("Overlays", PSPNativeOverlay_Init(), "overlay init failed");
    { FILE *rf = fopen("sdmc:/3ds/platinum3ds/platinum.nds", "rb"); char hd[16] = {0}; int ok = rf && fread(hd, 1, 12, rf) == 12; if (rf) fclose(rf); /* run 64: plain error when the ROM is missing */
      if (!ok) { uiStep("ROM (platinum.nds)", 0); PSPNativeMemLog("[3DS] step ROM: FAIL (missing)"); hold("ROM not found. Put your Pokemon Platinum (US) ROM at sdmc:/3ds/platinum3ds/platinum.nds"); } }
    STEP("ROM (platinum.nds)", PSPNativeRomFS_SetPath("sdmc:/3ds/platinum3ds/platinum.nds"), "ROM not found. Put your Pokemon Platinum (US) ROM at sdmc:/3ds/platinum3ds/platinum.nds");
    { /* create a blank save only if none exists; never touch an existing one */
      FILE *f = fopen("sdmc:/3ds/platinum3ds/platinum.sav", "rb");
      if (f) fclose(f); else if ((f = fopen("sdmc:/3ds/platinum3ds/platinum.sav", "wb"))) { static u8 blk[4096]; memset(blk, 0xFF, sizeof blk); for (int i = 0; i < 128; i++) fwrite(blk, 1, sizeof blk, f); fclose(f); PSPNativeMemLog("[3DS] created blank platinum.sav"); } }
    { /* run 74 save_backup=1 (default ON, safety): at every launch, before the game opens it, platinum.sav is COPIED to
         platinum.sav.bak1 (older copies rotate to .bak2 / .bak3). Byte copy only; the save itself is never touched. */
      extern int P3DS_CfgInt(const char *, int); int nb = P3DS_CfgInt("save_backup", 1);
      if (nb) { static u8 sb[524288]; FILE *f = fopen("sdmc:/3ds/platinum3ds/platinum.sav", "rb"); size_t n = f ? fread(sb, 1, sizeof sb, f) : 0; if (f) fclose(f);
        if (n == sizeof sb) { remove("sdmc:/3ds/platinum3ds/platinum.sav.bak3"); rename("sdmc:/3ds/platinum3ds/platinum.sav.bak2", "sdmc:/3ds/platinum3ds/platinum.sav.bak3");
          rename("sdmc:/3ds/platinum3ds/platinum.sav.bak1", "sdmc:/3ds/platinum3ds/platinum.sav.bak2");
          FILE *o = fopen("sdmc:/3ds/platinum3ds/platinum.sav.bak1", "wb"); size_t w = o ? fwrite(sb, 1, n, o) : 0; if (o) fclose(o);
          PSPNativeMemLog("[SAVE] backup platinum.sav -> .bak1 (%s, 3 kept)", w == n ? "ok" : "FAILED"); } } }
    STEP("Save (platinum.sav)", PSPNative_OpenBackup("sdmc:/3ds/platinum3ds/platinum.sav"), "platinum.sav invalid: must be exactly 512 KiB");
    { extern int P3DS_GameFontLoad(void); P3DS_GameFontLoad(); } /* run 64: game font from the ROM for port UI */
    PSPNativeStackProbeInit(); STEP("Stack probe", 1, "");
    WIN_Init_sTexStartAddrTable(); WIN_Init_sTexPlttStartAddrTable(); STEP("Texture tables", 1, "");
    CTRDG_Init(); STEP("Cartridge slot", 1, "");
    PSPNativeFrameInit(); STEP("Renderer + audio", 1, "");
    STEP("Starting game", 1, ""); PSPNativeMemLog("[3DS] entering NitroMain");
    threadOnException(onCrash, excStack + sizeof excStack, WRITE_DATA_TO_HANDLER_STACK); gameStarted = 1; wdTh = threadCreate(watchdog, NULL, 0x4000, 0x20, -2, true);
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
extern void __real_OS_WaitVBlankIntr(void); void __wrap_OS_WaitVBlankIntr(void) { CRUMB("waitvblank"); __real_OS_WaitVBlankIntr(); }
extern void __real_OS_WaitIrq(int c, u32 m); void __wrap_OS_WaitIrq(int c, u32 m) { CRUMB("waitirq"); __real_OS_WaitIrq(c, m); }
extern void __real_MI_WaitDma(u32 ch); void __wrap_MI_WaitDma(u32 ch) { CRUMB("waitdma"); __real_MI_WaitDma(ch); }
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
static void vtLog(void) { u64 now = svcGetSystemTick(); if (now - vtT > 268111856ull * 2) { vtT = now; static int vt = -1; if (vt < 0) vt = P3DS_CfgInt("vram_trace", 0); /* run 66: cached */ if (vt) PSPNativeMemLog("[VXFER] 2s: requests %u failed %u process %u max/frame %u", vtReq, vtFail, vtProc, vtMaxPer); vtReq = vtFail = vtProc = vtMaxPer = 0; } }
int __wrap_VramTransfer_Request(int type, unsigned dest, void *buf, unsigned size) { int r = __real_VramTransfer_Request(type, dest, buf, size); vtReq++; vtPer++; if (!r) vtFail++; return r; }
/* run 63 heap_watch=<decimal address> (debug, default 0): log every game heap alloc covering that address and every
   free of a block starting up to 64 KiB below it, with the caller (finds use-after-free / reuse of a watched object). */
extern void *__real_Heap_Alloc(u32 h, u32 n); extern void *__real_Heap_AllocAtEnd(u32 h, u32 n); extern void __real_Heap_Free(void *p); extern void __real_Heap_FreeExplicit(u32 h, void *p);
static unsigned hwAddr = 1; static unsigned hwA(void) { if (hwAddr == 1) hwAddr = (unsigned)P3DS_CfgInt("heap_watch", 0); return hwAddr; }
static void hwChk(const char *k, void *p, u32 h, u32 n, void *ra) { unsigned a = hwA(), q = (unsigned)p; if (!a || !p) return;
  if (n ? (q <= a && a < q + n) : (q <= a && a - q < 0x10000)) PSPNativeMemLog("[HEAPW] %s %p heap %lu size %lu from %p (watch %x)", k, p, (unsigned long)h, (unsigned long)n, ra, a); }
void *__wrap_Heap_Alloc(u32 h, u32 n) { void *p = __real_Heap_Alloc(h, n); hwChk("alloc", p, h, n, __builtin_return_address(0)); return p; }
void *__wrap_Heap_AllocAtEnd(u32 h, u32 n) { void *p = __real_Heap_AllocAtEnd(h, n); hwChk("allocEnd", p, h, n, __builtin_return_address(0)); return p; }
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
   arm-none-eabi-addr2line -f -e platinum3ds_full.elf <addr>) and the caller's caller, always (log_level 1). */
extern void __real_ErrorHandling_AssertFail(void);
void __wrap_ErrorHandling_AssertFail(void) {
    void *a = __builtin_return_address(0);
    PSPNativeMemLog("[ASSERT] GF_ASSERT failed: caller %p (addr2line -f -e platinum3ds_full.elf %p)", a, (char *)a - 4);
    P3DS_CrumbDump();
    __real_ErrorHandling_AssertFail(); }

/* p3ds comm stack, stage 1 link fix (2026-10-28, run 53 link: 104 undefined symbols). Patch 0127 translates the real comm drivers
   (unk_02037C94 ... ), which also reach the Nintendo Wi-Fi Connection (DWC) glue: ARM-mode functions in lib/NitroDWC/asm/overlay_00_arm.s
   and overlay_45_arm.s (the translator only handles Thumb) and Thumb pointer-table targets in overlay_45_thumb.s / unk_0208F814.s
   that nothing translates. Local wireless never needs them (online WFC is a separate project, docs/WIRELESS.md Option B), so each is a
   weak stand-in that logs its first calls as "[COMM] DWC stub <name>" and returns 0 (= "not available / failed"). If one of these
   lines shows up in log.txt, that is the next function to port. Plain C, no game headers. */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern void PSPNativeMemLog(const char *fmt, ...);
#define DWC_STUB(name) __attribute__((weak)) unsigned int name(void) { static int n; if (n++ < 4) PSPNativeMemLog("[COMM] DWC stub " #name " called (#%d) -> 0", n); return 0; }

/* GTS connect fix (run 0176 follow-up): the five functions below are the DWC
   connection driver the game polls every frame while connecting. As 0-returning
   stubs, CheckInet never flipped TRUE and GetLastErrorEx never reported an error,
   so the game waited forever. These call the real libntrdwc functions instead. */
extern void DWC_ConnectInetAsync(void);
extern int DWC_CheckInet(void);
extern void DWC_ProcessInet(void);
extern int DWC_GetInetStatus(void);
extern int DWC_GetLastErrorEx(int *errorCode, int *errorType);

unsigned int ov00_021EC11C(int *errorCode, int *errorType) { return (unsigned)DWC_GetLastErrorEx(errorCode, errorType); }
/* p3ds 0180: DWC_Printf does not reach log.txt, so the 0179 marker can never show in a game log. These wrappers log what the
   game really sees: each ConnectInetAsync call (capped), and CheckInet / GetInetStatus only when their value changes, so one run
   shows whether the AC bypass got the driver to CONNECTED. Platform file: rebuilt every build, no Clean needed. */
/* p3ds run 71: the -50199 comes from DWC_ConnectInetAsync's "stpInetCntl == NULL" branch: DWC_InitInet[Ex] was never run,
   because the game's own call reaches a 0-returning stub (in dwc_connectinet.c file order the two functions just before
   ConnectInetAsync are InitInetEx and SetAuthServer, i.e. most likely ov00_021EC3F0 / ov00_021EC454, both logged as stubs right
   before the connect). Initialise the connection control here before every connect. DWC_InitInetEx is a no-op while one is
   already set, and re-initialises after DWC_CleanupInet cleared it, so a second GTS visit works too. 512 bytes is far larger
   than DWCInetControl, which only holds a few state fields. */
extern void DWC_InitInet(void *inetCntl);
static unsigned char s_p3ds_inet[16384] __attribute__((aligned(32)));
/* p3ds run 72: run 71 got past -50199 and crashed in DWC_Alloc (pc=0): libntrdwc's DWC_MEMFUNC calls its registered allocator
   s_alloc(name, size, align) / s_free(name, ptr, size), and the game's DWC_SetMemFunc call is one of the stubs, so s_alloc was NULL.
   Register a malloc-backed pair here (before the first DWC_Alloc, which DWC_AC_Create makes). DWC_AllocEx adds a 32-byte header and
   returns base+32, so a 32-aligned base keeps the 32-byte alignment DWC_Alloc asks for. */
extern void *memalign(unsigned int boundary, unsigned int size);
extern void free(void *ptr);
extern void DWC_SetMemFunc(void *(*alloc)(int name, unsigned int size, int align), void (*freef)(int name, void *ptr, unsigned int size));
static void *ss3ds_dwc_alloc(int name, unsigned int size, int align) { (void)name; (void)align; return memalign(32, size); }
static void ss3ds_dwc_free(int name, void *ptr, unsigned int size) { (void)name; (void)size; free(ptr); }
unsigned int ov00_021EC4A4(void) { static int n = 0; static int memset_done = 0; if (!memset_done) { memset_done = 1; DWC_SetMemFunc(ss3ds_dwc_alloc, ss3ds_dwc_free); } if (n++ < 4) PSPNativeMemLog("[COMM] DWC ConnectInetAsync called (#%d), SetMemFunc + InitInet done first", n); DWC_InitInet(s_p3ds_inet); DWC_ConnectInetAsync(); return 0; }
unsigned int ov00_021EC5B4(void) { int r = DWC_CheckInet(); static int last = -99; if (r != last) { PSPNativeMemLog("[COMM] DWC CheckInet -> %d", r); last = r; } return (unsigned)r; }
unsigned int ov00_021EC60C(void) { DWC_ProcessInet(); return 0; }
unsigned int ov00_021EC724(void) { int r = DWC_GetInetStatus(); static int last = -99; if (r != last) { PSPNativeMemLog("[COMM] DWC GetInetStatus -> %d", r); last = r; } return (unsigned)r; }

/* p3ds run 73: run 72 reached CONNECTED (CheckInet 1, GetInetStatus 4). The game's next steps (overlay_75 states 0xE/0xF, read from the
   NitroDWC asm) were still 0-returning stubs, so it saw failure, tore the connection down and retried:
   - ov00_021EC9E0 = DWC_GetApInfo(DWCApInfo *out, 0x3C bytes): aptype s32 @0, spot area s32 @4 (0xFF = none), spotinfo[9] @8,
     essid @0x12, bssid[6] @0x33. The 3DS OS owns the connection, so report a plain user AP and success (1).
   - ov00_021ECD04 = start of DWC's netcheck (internet reachability test: a worker thread and a result word); returns 1 if started.
   - ov00_021ECDC8 = its per-frame poll: 1/2 still running, 3 = success (the flow then goes on to the profile and login steps),
     0/4/5 = failure. The 3DS already proved it is online (soc + a successful AC connect), so report success (3) at once. */
unsigned int ov00_021EC9E0(unsigned char *apInfo) { static int n = 0; if (n++ < 2) PSPNativeMemLog("[COMM] DWC GetApInfo (ss3ds) -> 1 (#%d)", n); if (apInfo) { int i; for (i = 0; i < 0x3C; i++) apInfo[i] = 0; *(int *)(apInfo + 4) = 0xFF; } return 1; }
extern void P3DS_LoginBegin(void); extern int P3DS_LoginPoll(void);
unsigned int ov00_021ECD04(void) { static int n = 0; P3DS_LoginBegin(); if (n++ < 2) PSPNativeMemLog("[COMM] DWC netcheck start (ss3ds) -> 1 (#%d)", n); return 1; }
/* GHTTP init/release refcount (ov00_021FA0B4 / ov00_021FA0D8); both return 1, callers ignore it. Real GHTTP is not used yet. */
static int s_p3ds_ghttp_ref = 0;
unsigned int ov00_021FA0B4(void) { s_p3ds_ghttp_ref++; return 1; }
unsigned int ov00_021FA0D8(void) { if (s_p3ds_ghttp_ref > 0) s_p3ds_ghttp_ref--; return 1; }
/* DWC_Alloc(name,size) / DWC_Free(name,ptr,size): the game calls these directly after the netcheck; a NULL from the stub made it hang. */
void *ov00_021EC2A8(int name, unsigned int size) { static int n = 0; void *p; (void)name; p = memalign(32, size ? size : 1); if (n++ < 4) PSPNativeMemLog("[COMM] DWC_Alloc(%u) -> %p (#%d)", size, p, n); return p; }
void ov00_021EC2EC(int name, void *ptr, unsigned int size) { (void)name; (void)size; free(ptr); }
/* ghttp wrappers (GTS login): real HTTP GET in p3ds_net.c (P3DS_GhttpStart/Think). FA21C(url,cb,ud) -> request id or -5; FA118() -> 0 means DWC error. */
extern int P3DS_GhttpStart(const char *url, void *cb, void *ud);
extern int P3DS_GhttpThink(void);
int ov00_021FA21C(const char *url, void *cb, void *ud) { return P3DS_GhttpStart(url, cb, ud); }
unsigned int ov00_021FA118(void) { return (unsigned int)P3DS_GhttpThink(); }
/* DWC_Base64Encode(src, dst, srclen, mode) = ov00_021FFC18. mode 1: "[]_", mode 2 (URL-safe, used by the GTS data= field): "-_=", else "+/=". Writes dst, NUL-terminated;
   returns the number of characters written (the GTS caller ignores it). */
unsigned int ov00_021FFC18(const unsigned char *src, char *dst, int srclen, int mode)
{
    static const char a[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const char *x = mode == 1 ? "[]_" : mode == 2 ? "-_=" : "+/=";
    int o = 0;
    for (int i = 0; i < srclen; i += 3) {
        unsigned v = (unsigned)src[i] << 16; int n = srclen - i;
        if (n > 1) v |= (unsigned)src[i + 1] << 8;
        if (n > 2) v |= src[i + 2];
        for (int k = 0; k < 4; k++) {
            int idx = (v >> (18 - 6 * k)) & 63;
            if (k > n) dst[o++] = x[2];
            else dst[o++] = idx < 62 ? a[idx] : x[idx - 62];
        }
    }
    dst[o] = 0;
    return (unsigned)o;
}
unsigned int ov00_021ECDC8(void) { static int n = 0; int r = P3DS_LoginPoll(); if (n++ < 2) PSPNativeMemLog("[COMM] DWC netcheck poll (p3ds) -> %d (login-driven) (#%d)", r, n); return (unsigned)r; }

/* overlay 0 and overlay 45 Wi-Fi glue (ARM asm in lib/NitroDWC, plus overlay 45 Thumb pointer-table targets) */
/* GTS error capture (0177 follow-up): ClearError used to be a 0-returning stub, so the
   game cleared an error it never reported and the reset that followed carried no
   diagnosis. Read and log the pending error first, then really clear it. Every call
   is logged, capped: the first 8 calls, then only when the values change (the game
   calls ClearError every frame in its error loop, so uncapped logging spams the log). */
extern void DWC_ClearError(void);
unsigned int ov00_021EC210(void) { int code = 0, type = 0; int err = DWC_GetLastErrorEx(&code, &type); static int n = 0, lastErr = -1, lastCode = 0, lastType = 0; if (n++ < 8 || err != lastErr || code != lastCode || type != lastType) { PSPNativeMemLog("[COMM] DWC ClearError: last error %d code %d type %d", err, code, type); lastErr = err; lastCode = code; lastType = type; } DWC_ClearError(); return 0; }

/* ss3ds 0193: Mystery Gift WFC download (ov74_02236AE0). Overlay 0 ARM trampolines that
   the maploader cannot translate - map them onto libntrdwc's DWC_Nd* (already in
   libnetwork-probe.a) the same way GTS mapped ConnectInet/CheckInet/ghttp. Requires
   Settings > Play > Wireless = ONLINE (wireless=2). Local wireless MG receive is unchanged.
   Gamecode: keep the ROM header code. US SoulSilver is IPGE; US HeartGold is IPKE.
   Run 113 forced IPGE->IPKE and DLS count returned 0 (Wiimmfi/AltWFC list IPGE gifts
   under dlc/IPGE and treat IPGE as Gen4 random-file; no IPKE folder). */
typedef void (*SS3DS_NdCallback)(int reason, int error, int servererror);
typedef void (*SS3DS_NdCleanupCallback)(void);
extern int DWC_NdInitAsync(SS3DS_NdCallback cb, char *gamecd, char *passwd);
extern int DWC_NdCleanupAsync(SS3DS_NdCleanupCallback cb);
extern int DWC_NdSetAttr(const char *a, const char *b, const char *c);
extern int DWC_NdGetFileListNumAsync(int *entrynum);
extern int DWCi_Ndi_error;
extern int DWCi_Ndi_isOpened;
extern int DWC_NdGetFileListAsync(void *filelist, unsigned offset, unsigned num);
extern int DWC_NdGetFileAsync(void *fileinfo, char *buf, unsigned size);
extern int DWC_NdCancelAsync(void);
extern int DWC_NdGetProgress(unsigned *received, unsigned *contentlen);
extern int DWC_CleanupInetAsync(void);
extern void DWC_NASLoginAbort(void);
extern int DWC_UpdateConnection(void);

/* run 114: do not rewrite IPGE->IPKE. Pass the cart/ROM gamecd through unchanged. */
static char *ss3ds_mg_gamecd(char *gamecd)
{
    return gamecd;
}

/* run 110: DWCauthhttpparam.alloc/free are filled only by DWC_Auth_Create (NAS login).
   Our MG path skips NAS ([LOGIN] failed or skipped), so they stay NULL. NdInit then
   calls DWC_SVLGetTokenAsync -> DWC_Svl_GetTokenAsync -> DWC_Http_Create(&DWCauthhttpparam)
   which does blx param->alloc: run 109 crash pc=0 lr=DWC_Http_Create (0x61d010),
   r0="ALLOC http->lowrecvbuf". Install the same DWC_Alloc/DWC_Free Auth_Create would
   have set. MG Nd wrappers only; GTS-shared EC294 / ConnectInet / NAS login untouched. */
typedef void *(*SS3DS_DWCAuthAlloc)(unsigned long name, long size);
typedef void (*SS3DS_DWCAuthFree)(unsigned long name, void *ptr, long size);
typedef struct {
    const char *url;
    int action;
    unsigned long len_recvbuf;
    SS3DS_DWCAuthAlloc alloc;
    SS3DS_DWCAuthFree free;
    int ignoreca;
    int timeout;
} SS3DS_DWCHttpParam;
extern SS3DS_DWCHttpParam DWCauthhttpparam;
extern void *DWC_Alloc(int name, unsigned int size);
extern void DWC_Free(int name, void *ptr, unsigned int size);
extern void DWC_Auth_SetCustomNas(const char *url);
/* Set by NdInit so MG-only NAS cal / soctr / body logs can gate on the Nd path. */
int gSS_MG_NdSvlActive;

/* run 113: log Nd callbacks (gift count / errors). Run 112 had HTTP/1.1 200 then
   NdCleanup with no callback line, so the DLS gift count was unknown. */
static SS3DS_NdCallback s_mg_nd_cb;
static int *s_mg_entrynum;

static void ss3ds_mg_nd_cb(int reason, int error, int servererror)
{
    int n = s_mg_entrynum ? *s_mg_entrynum : -999;
    PSPNativeMemLog("[SS-MG] NdCallback reason=%d err=%d server=%d entrynum=%d",
                    reason, error, servererror, n);
    if (s_mg_nd_cb)
        s_mg_nd_cb(reason, error, servererror);
}

static void ss3ds_mg_ensure_http_alloc(void)
{
    if (DWCauthhttpparam.alloc == 0 || DWCauthhttpparam.free == 0) {
        DWCauthhttpparam.alloc = (SS3DS_DWCAuthAlloc)DWC_Alloc;
        DWCauthhttpparam.free = (SS3DS_DWCAuthFree)DWC_Free;
        PSPNativeMemLog("[SS-MG] DWCauthhttpparam.alloc/free set (NAS login skipped)");
    }
    /* Auth_Create sets ignoreca when the NAS URL is not the stock https:// one. Login was
       skipped so do the same here once SetCustomNas pointed us at plain http://. */
    if (DWCauthhttpparam.ignoreca == 0)
        DWCauthhttpparam.ignoreca = 1;
}

/* run 111: SVL used Nitro priority 0x68; CreateThreadReal aborts if prio>31. Clamp here so
   MG works even before a CLEAN network rebuild picks up pspoke-patch 0195. GTS Auth calls
   StartThread with prio 0, so this wrap is a no-op on the GTS path. */
extern void __real_DWC_Http_StartThread(void *http, unsigned prio);
void __wrap_DWC_Http_StartThread(void *http, unsigned prio)
{
    if (prio > 31u) {
        PSPNativeMemLog("[SS-MG] Http_StartThread prio %u clamped to 16 (OS max 31)", prio);
        prio = 16u;
    }
    __real_DWC_Http_StartThread(http, prio);
}

extern int P3DS_PrepareStarMeIdentity(void);
unsigned int ov00_021ED1F0(SS3DS_NdCallback cb, char *gamecd, char *passwd)
{
    if (!P3DS_PrepareStarMeIdentity()) return 0;
    char *gc = ss3ds_mg_gamecd(gamecd);
    int r;
    /* Keep s_alloc registered for DWCi_NdAlloc/DWC_AllocEx; ConnectInet already did
       this, but re-set here so the Nd path does not depend on GTS-shared EC294. */
    DWC_SetMemFunc(ss3ds_dwc_alloc, ss3ds_dwc_free);
    /* LoginStart (which calls SetCustomNas) was skipped when userdata was null; SVL still
       posts SVCLOC through DWCauthhttpparam and needs the plain-http revival NAS URL. */
    DWC_Auth_SetCustomNas("http://nas.nintendowifi.net/ac");
    gSS_MG_NdSvlActive = 1;
    ss3ds_mg_ensure_http_alloc();
    s_mg_nd_cb = cb;
    s_mg_entrynum = 0;
    PSPNativeMemLog("[SS-MG] NdInitAsync gamecd=%.4s (ROM=%.4s) passwd=%.4s... (NAS http + SVL)",
                    gc ? gc : "????", gamecd ? gamecd : "????", passwd ? passwd : "????");
    r = DWC_NdInitAsync(ss3ds_mg_nd_cb, gc, passwd);
    PSPNativeMemLog("[SS-MG] NdInitAsync -> %d", r);
    return (unsigned)r;
}
extern unsigned char g_CleanupThread[]; /* NHTTP cleanup OSThread (nhttp_bgnend.c) */
extern void OS_JoinThread(void *thread);
unsigned int ov00_021ED308(SS3DS_NdCleanupCallback cb)
{
    gSS_MG_NdSvlActive = 0;
    /* 2nd Wi-Fi download in one run aborted here (run 150 log: 1st gift ok, 2nd crashed right after
       NdCallback reason=3, no NdCleanupAsync line). NHTTP_CleanupAsync re-creates the static
       g_CleanupThread every time, but the thread shim (OS_CreateThreadReal) abort()s while that OSThread
       is still registered, and nobody joins it. The previous cleanup thread has long exited: reap it. */
    OS_JoinThread(&g_CleanupThread);
    int r = DWC_NdCleanupAsync(cb);
    PSPNativeMemLog("[SS-MG] NdCleanupAsync -> %d", r);
    return (unsigned)r;
}
unsigned int ov00_021ED354(const char *a, const char *b, const char *c)
{
    /* HGSS passes ov74_0223D054 = empty string x3; empty attrs stay NULL (not posted). */
    return (unsigned)DWC_NdSetAttr(a ? a : "", b ? b : "", c ? c : "");
}
unsigned int ov00_021ED388(int *entrynum)
{
    if (!entrynum) {
        PSPNativeMemLog("[SS-MG] NdGetFileListNumAsync NULL entrynum -> 0");
        return 0;
    }
    ss3ds_mg_ensure_http_alloc();
    s_mg_entrynum = entrynum;
    {
        int r = DWC_NdGetFileListNumAsync(entrynum);
        /* Keep a short result line; gift count arrives in NdCallback entrynum. */
        PSPNativeMemLog("[SS-MG] NdGetFileListNumAsync -> %d opened=%d ndi_err=%d",
                        r, DWCi_Ndi_isOpened, DWCi_Ndi_error);
        return (unsigned)r;
    }
}

/* run 114: log DLS response body (count is a short ASCII integer). MG-gated. */
extern int __real_NHTTP_GetBodyAll(void *res, char **value);
int __wrap_NHTTP_GetBodyAll(void *res, char **value)
{
    int r = __real_NHTTP_GetBodyAll(res, value);
    if (gSS_MG_NdSvlActive && r >= 0 && value && *value) {
        PSPNativeMemLog("[SS-MG] DLS response body len=%d: %.80s", r, *value);
    }
    return r;
}
unsigned int ov00_021ED3AC(void *filelist, unsigned offset, unsigned num)
{
    if (!filelist && num != 0) {
        PSPNativeMemLog("[SS-MG] NdGetFileListAsync NULL filelist num=%u -> 0", num);
        return 0;
    }
    ss3ds_mg_ensure_http_alloc();
    {
        int r = DWC_NdGetFileListAsync(filelist, offset, num);
        if (r == 0 || DWCi_Ndi_error)
            PSPNativeMemLog("[SS-MG] NdGetFileListAsync off=%u num=%u -> %d ndi_err=%d",
                            offset, num, r, DWCi_Ndi_error);
        return (unsigned)r;
    }
}
unsigned int ov00_021ED3F4(void *fileinfo, char *buf, unsigned size)
{
    if (!fileinfo || !buf) {
        PSPNativeMemLog("[SS-MG] NdGetFileAsync NULL fileinfo=%p buf=%p -> 0",
                        (void *)fileinfo, (void *)buf);
        return 0;
    }
    ss3ds_mg_ensure_http_alloc();
    {
        int r = DWC_NdGetFileAsync(fileinfo, buf, size);
        if (r == 0 || DWCi_Ndi_error)
            PSPNativeMemLog("[SS-MG] NdGetFileAsync size=%u -> %d ndi_err=%d", size, r, DWCi_Ndi_error);
        return (unsigned)r;
    }
}
unsigned int ov00_021ED444(unsigned *received, unsigned *contentlen)
{
    return (unsigned)DWC_NdGetProgress(received, contentlen);
}
unsigned int ov00_021ED428(void)
{
    int r = DWC_NdCancelAsync();
    PSPNativeMemLog("[SS-MG] NdCancelAsync -> %d", r);
    return (unsigned)r;
}
unsigned int ov00_021ECEC0(void)
{
    /* B during netcheck: abort NAS/login wait (Platinum: DWC_NASLoginAbort). */
    PSPNativeMemLog("[SS-MG] netcheck abort (NASLoginAbort)");
    DWC_NASLoginAbort();
    return 0;
}
unsigned int ov00_021EC938(void)
{
    int r = DWC_CleanupInetAsync();
    static int n, last = -99;
    if (n++ < 8 || r != last) {
        PSPNativeMemLog("[SS-MG] CleanupInetAsync -> %d (#%d)", r, n);
        last = r;
    }
    return (unsigned)r;
}
/* Per-frame connection keep-alive while MG (and GTS) hold the inet session: ProcessInet + UpdateConnection.
   Returns 1 when the connection dropped (ov74 error path). Replaces the 0-returning DWC_STUB. */
unsigned int ov00_021ECB40(void)
{
    DWC_ProcessInet();
    return (unsigned)DWC_UpdateConnection();
}

/* run 109: back to the pre-108 stub so GTS stays exactly as it was (ConnectInet sets the allocators itself). */
DWC_STUB(ov00_021EC294)

/* ss3ds 0194 / run 108: Mystery Gift receive path discrimination.
   Run 107 log showed WM_Initialize + wireless-icon redraw and NO [SS-MG] Nd / ConnectInet
   lines: that is ov74_0222CEE0 (local "GET VIA WIRELESS" search on the WM stand-in), not
   ov74_02236AE0 (WFC download). Receive submenu order is Friend, Wireless, WFC, Cancel;
   itemsAbove=1 + cursorPos=0 selects Wireless. With wireless=2 (ONLINE) local DS parent
   search can never succeed, so redirect Wireless confirm to the WFC download entry
   (same as GET VIA NINTENDO WFC). wireless=0/1 keep the translated local path. */
extern int P3DS_CfgInt(const char *key, int def);
extern unsigned int __real_ov74_0222A560(void *man);
extern unsigned int __real_ov74_0222A4CC(void *man);
unsigned int __wrap_ov74_0222A560(void *man)
{
    PSPNativeMemLog("[SS-MG] path=WFC-download entry (ov74_0222A560) -> state 0x1b / ov74_02236AE0");
    return __real_ov74_0222A560(man);
}
unsigned int __wrap_ov74_0222A4CC(void *man)
{
    /* Auto wireless: with the 3DS online (Wi-Fi up, no cheat, no local session) GET VIA WIRELESS goes to the WFC download, since
       no real DS can be found over 3DS local wireless; otherwise the translated local path runs. */
    extern int P3DS_NetInit(void); extern int P3DS_OnlineCheatBlocked(void); extern int P3DS_LocalWirelessActive(void);
    if (!P3DS_LocalWirelessActive() && !P3DS_OnlineCheatBlocked() && P3DS_NetInit()) {
        PSPNativeMemLog("[SS-MG] path=WIRELESS confirm, auto wireless (online): redirect to WFC-download (local WM search cannot find a real DS parent)");
        return __wrap_ov74_0222A560(man);
    }
    PSPNativeMemLog("[SS-MG] path=local-wireless entry (ov74_0222A4CC), auto wireless (offline) -> state 0x20 / ov74_0222CEE0");
    return __real_ov74_0222A4CC(man);
}

DWC_STUB(ov00_021EC3F0)
/* The Global Terminal calls the synchronous cleanup entry on exit. */
extern void DWC_CleanupInet(void);
extern void P3DS_LoginEnd(void);
unsigned int ov00_021EC8D8(void)
{
    PSPNativeMemLog("[COMM] GTS synchronous WFC cleanup");
    P3DS_LoginEnd();
    DWC_CleanupInet();
    return 0;
}
DWC_STUB(ov00_021EC9D4)
/* ov00_021ECB40: real keep-alive above (ss3ds 0193) */
DWC_STUB(ov00_021ED730)
DWC_STUB(ov00_021ED9B4)
DWC_STUB(ov00_021EDD6C)
DWC_STUB(ov00_021EDE74)
DWC_STUB(ov00_021EDFBC)
DWC_STUB(ov00_021EE054)
DWC_STUB(ov00_021EE24C)
DWC_STUB(ov00_021EE278)
DWC_STUB(ov00_021EE384)
DWC_STUB(ov00_021EE448)
DWC_STUB(ov00_021EE530)
DWC_STUB(ov00_021EFDCC)
DWC_STUB(ov00_021EFF04)
DWC_STUB(ov00_021F1284)
DWC_STUB(ov00_021F9580)
DWC_STUB(ov00_021F9750)
DWC_STUB(ov00_021F9850)
DWC_STUB(ov00_021F989C)
DWC_STUB(ov00_021F98BC)
DWC_STUB(ov00_021F98DC)
DWC_STUB(ov00_021F9988)
DWC_STUB(ov00_021E5CBC)
DWC_STUB(ov00_021E6A70)
DWC_STUB(ov00_021E6CCC)
DWC_STUB(ov00_021E6D60)
DWC_STUB(ov00_021E6D68)
DWC_STUB(ov00_021E6EBC)
DWC_STUB(ov00_021E7078)
DWC_STUB(ov00_021E7080)
DWC_STUB(ov00_021E709C)
DWC_STUB(ov00_021E70B8)
DWC_STUB(ov00_021E70F4)
DWC_STUB(ov00_021E714C)
DWC_STUB(ov00_021E7220)
DWC_STUB(ov00_021E7328)
DWC_STUB(ov00_021EC0FC)
DWC_STUB(ov00_021EC454)
DWC_STUB(ov00_021ECB94)
DWC_STUB(ov00_021EDB1C)
DWC_STUB(ov00_021EE490)
DWC_STUB(ov00_021EE4FC)
DWC_STUB(ov45_0222CF00)
DWC_STUB(ov45_0222CF24)
DWC_STUB(ov45_0222CF40)
DWC_STUB(ov45_0222CF68)
DWC_STUB(ov45_0222CFF4)
DWC_STUB(ov45_0222D01C)
DWC_STUB(ov45_0222D028)
DWC_STUB(ov45_0222D054)
DWC_STUB(ov45_0222D078)
DWC_STUB(ov45_0222D0BC)
DWC_STUB(ov45_0222D0C8)
DWC_STUB(ov45_0222D0D8)
DWC_STUB(ov45_0222D0FC)
DWC_STUB(ov45_0222D130)
DWC_STUB(ov45_0222D13C)
DWC_STUB(ov45_0222D14C)
DWC_STUB(ov45_0222D164)
DWC_STUB(ov45_0222D19C)
DWC_STUB(ov45_0222D1B0)
DWC_STUB(ov45_0222D1DC)
DWC_STUB(ov45_0222D20C)
DWC_STUB(ov45_0222D23C)
DWC_STUB(ov45_0222D250)
DWC_STUB(ov45_0222D27C)
DWC_STUB(ov45_0222D2AC)
DWC_STUB(ov45_0222D2E4)
DWC_STUB(ov45_0222D2F8)
DWC_STUB(ov45_0222D324)
DWC_STUB(ov45_0222D354)
DWC_STUB(ov45_0222D380)
DWC_STUB(ov45_0222D38C)
DWC_STUB(ov45_0222D3B0)
DWC_STUB(ov45_0222D3C4)
DWC_STUB(ov45_0222D3D8)
DWC_STUB(ov45_0222D410)
DWC_STUB(ov45_0222D41C)
DWC_STUB(ov45_0222D428)
DWC_STUB(ov45_0222D448)
DWC_STUB(ov45_0222D44C)
DWC_STUB(ov45_0222D484)
DWC_STUB(ov45_0222D4C0)
DWC_STUB(ov45_0222D4CC)
DWC_STUB(ov45_0222D4DC)
DWC_STUB(ov45_02230B64)
DWC_STUB(ov45_02230B8C)
DWC_STUB(ov45_02230BFC)
DWC_STUB(ov45_02230C40)
DWC_STUB(ov45_02230CD8)
DWC_STUB(ov45_02230D20)
DWC_STUB(ov45_02230D5C)
DWC_STUB(ov45_022310C0)
DWC_STUB(ov45_02231514)
DWC_STUB(ov45_02231EC0)
DWC_STUB(ov45_022320C4)
DWC_STUB(ov45_02232170)
DWC_STUB(ov45_022321E0)
DWC_STUB(ov45_02232304)
DWC_STUB(ov45_02232580)
DWC_STUB(ov45_02232BB0)
DWC_STUB(ov45_02232CA4)

/* unk_0208F814.s pointer-table targets (Thumb, not in the translator closure) */
DWC_STUB(sub_0208F828)
DWC_STUB(sub_0208F8BC)
DWC_STUB(sub_0208F8E4)
DWC_STUB(sub_0208F918)
DWC_STUB(sub_0208F960)
DWC_STUB(sub_0208FA50)
DWC_STUB(sub_0208FA54)
DWC_STUB(sub_0208FA88)
DWC_STUB(sub_0208FA8C)
DWC_STUB(sub_0208FA90)
DWC_STUB(sub_0208FAA8)
DWC_STUB(sub_0208FAEC)
DWC_STUB(sub_0208FB4C)
DWC_STUB(sub_0208FB5C)
DWC_STUB(sub_0208FB60)

/* Run 55 (two consoles, Union Room, Local wireless ON): both crashed in the translated sub_0203A700 (unk_0203A3B0.s), the wireless-icon
   per-frame redraw: it reads DISPCNT/POWCNT through raw DS addresses and writes DS OBJ VRAM through a bank pointer that was NULL here
   (str r1,[r6], far=0). The icon is cosmetic, so patch 0132 lists it in native-provided.json and this strong no-op replaces it. */
unsigned int sub_0203A700(void *task) {
    static int n;
    (void)task;
    if (n++ < 2) PSPNativeMemLog("[COMM] wireless icon redraw (sub_0203A700) skipped (#%d)", n);
    return 0;
}

/* Run 56 (two consoles talked in the Union Room): the crash at the end of the conversation was the wireless-icon teardown sub_0203A444
   (inlined into the translated sub_0203A914): after destroying the icon task it hides the sprite by writing DS OAM directly at
   0x07000000 / 0x07000400, which is unmapped here. Patch 0133 lists it in native-provided.json; this keeps the task destroy and the free
   (the icon object: +0x18 = its SysTask) and drops the raw OAM writes (the redraw above never draws it anyway). */
extern void SysTask_Destroy(void *task);
extern void Heap_Free(void *p);
unsigned int sub_0203A444(void *icon) {
    static int n;
    if (n++ < 4) PSPNativeMemLog("[COMM] wireless icon teardown (sub_0203A444) %p (#%d)", icon, n);
    if (icon) { SysTask_Destroy(*(void **)((unsigned char *)icon + 0x18)); Heap_Free(icon); }
    return 0;
}

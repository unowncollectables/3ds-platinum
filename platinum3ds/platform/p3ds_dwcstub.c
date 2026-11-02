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
static void *p3ds_dwc_alloc(int name, unsigned int size, int align) { (void)name; (void)align; return memalign(32, size); }
static void p3ds_dwc_free(int name, void *ptr, unsigned int size) { (void)name; (void)size; free(ptr); }
unsigned int ov00_021EC4A4(void) { static int n = 0; static int memset_done = 0; if (!memset_done) { memset_done = 1; DWC_SetMemFunc(p3ds_dwc_alloc, p3ds_dwc_free); } if (n++ < 4) PSPNativeMemLog("[COMM] DWC ConnectInetAsync called (#%d), SetMemFunc + InitInet done first [p3ds run 72 built " __DATE__ " " __TIME__ "]", n); DWC_InitInet(s_p3ds_inet); DWC_ConnectInetAsync(); return 0; }
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
unsigned int ov00_021EC9E0(unsigned char *apInfo) { static int n = 0; if (n++ < 2) PSPNativeMemLog("[COMM] DWC GetApInfo (p3ds) -> 1 (#%d)", n); if (apInfo) { int i; for (i = 0; i < 0x3C; i++) apInfo[i] = 0; *(int *)(apInfo + 4) = 0xFF; } return 1; }
extern void P3DS_LoginStart(void); extern int P3DS_LoginPoll(void);
unsigned int ov00_021ECD04(void) { static int n = 0; P3DS_LoginStart(); if (n++ < 2) PSPNativeMemLog("[COMM] DWC netcheck start (p3ds) -> 1 (#%d)", n); return 1; }
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
DWC_STUB(ov00_021EC294)
DWC_STUB(ov00_021EC3F0)
DWC_STUB(ov00_021EC8D8)
DWC_STUB(ov00_021EC9D4)
DWC_STUB(ov00_021ECB40)
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

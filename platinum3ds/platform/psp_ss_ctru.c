/* ss3ds: the PSP kernel calls SoulSilver's pspoke app uses on top of what psp_ctru.c (from Platinum) provides. */
#include <3ds.h>
#include <stdio.h>
#include <pspkernel.h>
#include <psploadexec.h>
_Static_assert(sizeof(RecursiveLock) <= sizeof(SceLwMutexWorkarea), "SceLwMutexWorkarea too small");
/* run 80: the ROM lock (romfs_lock.c SSRomLock) froze GTS: its state word `created` sits just before its mutex workarea. Remember when/where the
   "SS readonly ROM" mutex was created so the watchdog can print whether it ever was, and the words around it (created = the word before). */
extern void PSPNativeMemLog(const char *fmt, ...);
static volatile unsigned romWork, romLr, romMade;
int sceKernelCreateLwMutex(SceLwMutexWorkarea *work, const char *name, SceUInt attr, int count, void *opt)
{ (void)attr; (void)count; (void)opt; RecursiveLock_Init((RecursiveLock *)work);
  if (name && name[0] == 'S' && name[1] == 'S' && name[3] == 'r') { romWork = (unsigned)work; romLr = (unsigned)__builtin_return_address(0); romMade++; PSPNativeMemLog("[ROMLOCK] mutex created #%u work=%p from %p", romMade, (void *)work, __builtin_return_address(0)); }
  return 0; }
/* run 81: --wrap=SSRomLock. Run 80 showed the ROM lock itself is fine (ready, free), so main is looping through ROM file calls; remember which
   FS functions call it (return address of each call) and how often, so addr2line can name the loop. */
extern void __real_SSRomLock(void);
static volatile unsigned rlChain[40], rlChainN;
static volatile unsigned rlCaller[8], rlCount[8], rlTotal, rlLast[16], rlPos;
void __wrap_SSRomLock(void) {
    unsigned c = (unsigned)__builtin_return_address(0); int k;
    rlTotal++; rlLast[rlPos++ & 15] = c;
    for (k = 0; k < 8; k++) { if (rlCaller[k] == c) { rlCount[k]++; break; } if (!rlCaller[k]) { rlCaller[k] = c; rlCount[k] = 1; break; } }
    /* run 82: once it is clearly looping, keep the call chain of the latest call: code-looking words on the stack above us, nearest frame first. */
    if (rlTotal > 3000 && (rlTotal & 255) == 0) {
        volatile unsigned *sp = (volatile unsigned *)__builtin_frame_address(0); int m = 0;
        for (int i = 0; i < 400 && m < 40; i++) { unsigned v = sp[i]; if (v >= 0x100000 && v < 0x738000 && (v & 3) != 2) rlChain[m++] = v; }
        rlChainN = m;
    }
    __real_SSRomLock();
}
void P3DS_RomCallerInfo(char *b, int n) {
    int w = snprintf(b, n, "ROM lock calls %u; by caller:", rlTotal);
    for (int k = 0; k < 8 && rlCaller[k] && w < n - 30; k++) w += snprintf(b + w, n - w, " %08x x%u", rlCaller[k], rlCount[k]);
    w += snprintf(b + w, n - w, " | last 16:");
    for (int k = 0; k < 16 && w < n - 12; k++) w += snprintf(b + w, n - w, " %08x", rlLast[(rlPos + k) & 15]);
    w += snprintf(b + w, n - w, " | stack chain:");
    for (unsigned k = 0; k < rlChainN && w < n - 12; k++) w += snprintf(b + w, n - w, " %08x", rlChain[k]);
}
void P3DS_RomLockInfo(char *b, int n) {
    if (!romWork) { snprintf(b, n, "ROM lock mutex was NEVER created (SSRomLock stuck before init finished)"); return; }
    volatile unsigned *w = (volatile unsigned *)romWork;
    snprintf(b, n, "ROM lock: created %u time(s) from %08x; created-flag %d; mutex words %08x %08x %08x", romMade, romLr, (int)w[-1], w[0], w[1], w[2]);
}
int sceKernelLockLwMutex(SceLwMutexWorkarea *work, int count, SceUInt *timeout)
{ (void)timeout; for (int i = 0; i < count; i++) RecursiveLock_Lock((RecursiveLock *)work); return 0; }
int sceKernelUnlockLwMutex(SceLwMutexWorkarea *work, int count)
{ for (int i = 0; i < count; i++) RecursiveLock_Unlock((RecursiveLock *)work); return 0; }
/* run 3: soft reset via P3DS_SoftReset (clean exit + relaunch). OS_ResetSystem (0061) already calls SoftReset
   first; LoadExec is the fall-through if SoftReset returned. SoftReset is once-guarded (run 934), so a second
   entry is a no-op and we must not abort afterward. */
int sceKernelLoadExec(const char *file, struct SceKernelLoadExecParam *param) {
    extern void P3DS_SoftReset(unsigned); (void)file; (void)param;
    P3DS_SoftReset(0);
    /* SoftReset either exits the process or was already in progress — never reach abort(). */
    for (;;) svcSleepThread(1000000000LL);
}

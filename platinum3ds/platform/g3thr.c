/* g3_thread plumbing (g3soft.cpp includes pspoke/nitro headers that clash with <3ds.h>, so the libctru calls live here). */
#include <3ds.h>
static LightSemaphore sem;
static LightSemaphore sem2;
static LightLock vbLk;
static int vbLkInit, splitOn, splitStarted;
static Thread th1, th2;
static volatile int g3tQuit;
int G3T_Start(void (*fn)(void *)) {
    LightSemaphore_Init(&sem, 0, 32767);
    if (!vbLkInit) { LightLock_Init(&vbLk); vbLkInit = 1; }
    g3tQuit = 0;
    th1 = threadCreate(fn, NULL, 0x8000, 0x30, 2, false);   /* core 2 */
    return th1 != NULL;
}
/* g3_raw_split: second raw worker on core 3 (New 3DS). Idempotent — live Settings toggle may call again. */
int G3T_Start2(void (*fn)(void *)) {
    if (splitStarted) { splitOn = 1; return 1; } /* already running: just re-enable wake/lock */
    LightSemaphore_Init(&sem2, 0, 32767);
    if (!vbLkInit) { LightLock_Init(&vbLk); vbLkInit = 1; }
    th2 = threadCreate(fn, NULL, 0x8000, 0x30, 3, false);   /* core 3 */
    if (th2 == NULL) return 0;
    splitStarted = 1; splitOn = 1; return 1;
}
/* Park worker 2: stop waking it / locking vb. Thread stays asleep on Wait2 (no join — 3DS threads are sticky). */
void G3T_Stop2(void) { splitOn = 0; }
void G3T_Wait(void) { LightSemaphore_Acquire(&sem, 1); }
void G3T_Signal(void) { LightSemaphore_Release(&sem, 1); }
void G3T_Wait2(void) { LightSemaphore_Acquire(&sem2, 1); }
void G3T_Signal2(void) { LightSemaphore_Release(&sem2, 1); }
void G3T_SleepNs(long long ns) { svcSleepThread(ns); }
/* Serialise TriClip2 / batch appends when two raw workers run (g3_raw_split). No-op cost when split OFF and uncontended. */
void G3T_VbLock(void) { if (splitOn) LightLock_Lock(&vbLk); }
void G3T_VbUnlock(void) { if (splitOn) LightLock_Unlock(&vbLk); }
int G3T_SplitOn(void) { return splitOn; }
int G3T_Quiting(void) { return g3tQuit; }
/* Soft-reset / exit: wake sleepers so they see quit and exit their loops, then join. */
void G3T_RequestQuit(void) {
    g3tQuit = 1;
    LightSemaphore_Release(&sem, 1);
    if (splitStarted) LightSemaphore_Release(&sem2, 1);
}
void G3T_Join(void) {
    if (th1) {
        if (R_SUCCEEDED(threadJoin(th1, 500000000ULL))) threadFree(th1);
        else { extern void PSPNativeMemLog(const char *, ...); PSPNativeMemLog("[3DS] exit: g3thr worker 1 did not stop in 500 ms"); }
        th1 = NULL;
    }
    if (th2) {
        if (R_SUCCEEDED(threadJoin(th2, 500000000ULL))) threadFree(th2);
        else { extern void PSPNativeMemLog(const char *, ...); PSPNativeMemLog("[3DS] exit: g3thr worker 2 did not stop in 500 ms"); }
        th2 = NULL;
    }
    splitOn = 0; splitStarted = 0;
}

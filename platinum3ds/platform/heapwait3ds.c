/* 2026-11-24 soft-reset crash (run 1004, boot 2): the relaunched process started while the old one still held about
   39 MiB of the APPLICATION memory budget (boot 1: 85 MiB free -> 32 MiB linear + 53 MiB malloc; boot 2: 46 MiB ->
   22 + 22). The game needs about 40 MiB of malloc, so boot 2 ran out ("[GPUC] init: out of memory", then a NULL
   malloc result written to in NNS_G3dGeBufferOP_N 2 s in). The 400 ms pause before exit did not cover it, and the
   new process cannot fix its heap sizes later: libctru carves both heaps once, before main().
   This replaces libctru 2.7.0's weak __system_allocateHeaps with the same code plus a wait. Split and mappings are
   byte-for-byte libctru 2.7.0's.
   Run 1005 (2026-11-24): the missing 39 MiB is the old process's whole code+data image (7.2 + 0.6 + 32.8 MB), so the old
   process had not finished dying, and nothing came back within 1.5 s. Now: under HEAP_LOW the wait goes on for up to 30 s
   (the screen is still black here: nothing is set up yet); between HEAP_LOW and HEAP_WANT it stops once the memory has not
   grown for 1.5 s (a setup that simply has less). main() refuses to start the game under HEAP_LOW (it would crash). */
#include <3ds.h>

extern char *fake_heap_start, *fake_heap_end;
extern u32 __ctru_heap, __ctru_heap_size, __ctru_linear_heap, __ctru_linear_heap_size;
extern void mappableInit(u32 addrMin, u32 addrMax);

#define HEAP_WANT (80u << 20)
#define HEAP_LOW  (64u << 20)
u32 P3DS_HeapWaitMs, P3DS_HeapFreeFirst, P3DS_HeapFreeFinal;

static u32 freeCommit(void) {
    Handle rl = 0; if (R_FAILED(svcGetResourceLimit(&rl, CUR_PROCESS_HANDLE))) svcBreak(USERBREAK_PANIC);
    ResourceLimitType t = RESLIMIT_COMMIT; s64 mx = 0, cur = 0;
    svcGetResourceLimitLimitValues(&mx, rl, &t, 1); svcGetResourceLimitCurrentValues(&cur, rl, &t, 1); svcCloseHandle(rl);
    return (u32)(mx - cur) & ~0xFFFu;
}

void __system_allocateHeaps(void) {
    const u64 t0 = svcGetSystemTick(); u64 lastGrow = t0;
    u32 remaining = freeCommit(); P3DS_HeapFreeFirst = remaining;
    if (__ctru_heap_size == 0 && __ctru_linear_heap_size == 0)   /* only the automatic split can grow by waiting */
        while (remaining < HEAP_WANT) {
            const u64 now = svcGetSystemTick();
            if (now - t0 > 30000ull * CPU_TICKS_PER_MSEC) break;
            if (remaining >= HEAP_LOW && now - lastGrow > 1500ull * CPU_TICKS_PER_MSEC) break;
            svcSleepThread(20 * 1000000LL);
            const u32 r = freeCommit(); if (r > remaining) lastGrow = svcGetSystemTick(); remaining = r;
        }
    P3DS_HeapFreeFinal = remaining; P3DS_HeapWaitMs = (u32)((svcGetSystemTick() - t0) / CPU_TICKS_PER_MSEC);

    if (__ctru_heap_size + __ctru_linear_heap_size > remaining) svcBreak(USERBREAK_PANIC);
    if (__ctru_heap_size == 0 && __ctru_linear_heap_size == 0) {
        __ctru_linear_heap_size = (remaining / 2) & ~0xFFFu;
        __ctru_heap_size = remaining - __ctru_linear_heap_size;
        if (__ctru_heap_size > (24u << 20)) {
            __ctru_heap_size = 24u << 20; __ctru_linear_heap_size = remaining - (24u << 20);
            if (__ctru_linear_heap_size > (32u << 20)) { __ctru_linear_heap_size = 32u << 20; __ctru_heap_size = remaining - (32u << 20); }
        }
    } else if (__ctru_heap_size == 0) __ctru_heap_size = remaining - __ctru_linear_heap_size;
    else if (__ctru_linear_heap_size == 0) __ctru_linear_heap_size = remaining - __ctru_heap_size;

    if (R_FAILED(svcControlMemory(&__ctru_heap, OS_HEAP_AREA_BEGIN, 0x0, __ctru_heap_size, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE))) svcBreak(USERBREAK_PANIC);
    if (R_FAILED(svcControlMemory(&__ctru_linear_heap, 0x0, 0x0, __ctru_linear_heap_size, MEMOP_ALLOC_LINEAR, MEMPERM_READ | MEMPERM_WRITE))) svcBreak(USERBREAK_PANIC);
    mappableInit(OS_MAP_AREA_BEGIN, OS_MAP_AREA_END);
    fake_heap_start = (char *)__ctru_heap; fake_heap_end = fake_heap_start + __ctru_heap_size;
}

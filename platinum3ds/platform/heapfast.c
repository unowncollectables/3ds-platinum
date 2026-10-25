/* run 101 heap_fast (Debug tab "Fast heap free", default 1): every game heap free (NNS_FndFreeToExpHeap) runs pspoke's
   three SDK_PORT checks WIN_CheckAndFree{AnimBank,CellBank,CharData}, each scanning its whole static table (2048 + 2048
   + 4096 entries, 64-bit compares) for an unpacked G2D bank whose owner pointer lies in the freed block: ~8k compares per
   free, found by the field_prof profiler as most of NNS_FndFreeToExpHeap's time during map loads. Here every
   registration path (NNS_G2dGetUnpacked*) also records the owner address in a short live list; a free whose block holds
   none of them skips the scan (the real scan could not find anything either). If one is inside, the real function runs
   exactly as before. The live list is a superset of the real tables, so the result is identical. 0 = always scan. */
#include <stdint.h>
extern int P3DS_CfgInt(const char *, int); extern void PSPNativeMemLog(const char *, ...);
typedef int BOOL;
#define LMAX 8192
static uintptr_t live[3][LMAX]; static int nLive[3]; static int overflow[3]; static int cfg = -1; static unsigned skips, scans;
static int on(void) { if (cfg < 0) { cfg = P3DS_CfgInt("heap_fast", 1) != 0; PSPNativeMemLog("[HEAP] heap_fast=%d", cfg); } return cfg; }
static void add(int k, void *pp) { if (nLive[k] < LMAX) live[k][nLive[k]++] = (uintptr_t)pp; else overflow[k] = 1; }
/* any live owner strictly inside (start, end)? (same test as the real scan); drops one match, as the real scan frees one */
static int hit(int k, void *s, void *e) {
    if (overflow[k] || !on()) return 1;
    const uintptr_t a = (uintptr_t)s, b = (uintptr_t)e;
    for (int i = 0; i < nLive[k]; i++) if (live[k][i] > a && live[k][i] < b) { live[k][i] = live[k][--nLive[k]]; return 1; }
    return 0;
}
#define REG(k, name) extern BOOL __real_##name(void *, void *); BOOL __wrap_##name(void *f, void *pp) { add(k, pp); return __real_##name(f, pp); }
REG(0, NNS_G2dGetUnpackedAnimBank) REG(0, NNS_G2dGetUnpackedMCAnimBank)
REG(1, NNS_G2dGetUnpackedCellBank)
REG(2, NNS_G2dGetUnpackedCharacterData) REG(2, NNS_G2dGetUnpackedBGCharacterData)
#define CHK(k, name) extern void __real_##name(void *, void *); void __wrap_##name(void *s, void *e) { if (hit(k, s, e)) { scans++; __real_##name(s, e); } else skips++; }
CHK(0, WIN_CheckAndFreeAnimBank) CHK(1, WIN_CheckAndFreeCellBank) CHK(2, WIN_CheckAndFreeCharData)
void P3DS_HeapFastStats(void) { static unsigned n; if (++n % 1800 == 0 && cfg > 0) PSPNativeMemLog("[HEAP] heap_fast: %u table scans skipped, %u run | live %d/%d/%d", skips, scans, nLive[0], nLive[1], nLive[2]); }

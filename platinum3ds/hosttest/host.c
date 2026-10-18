#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "heap.h"
#include "narc.h"
void P_Log(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); }
void P_Fatal(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf(" FATAL\n"); exit(1); }
void p3ds_assert_fail(const char *f, int l) { P_Fatal("assert %s:%d", f, l); }
BOOL CommManager_IsInitialized(void) { return FALSE; }
void ErrorMessageReset_PrintErrorAndReset(void) { P_Fatal("oom"); }
static u8 *lo, *hi;
void *OS_AllocFromMainArenaLo(u32 s, u32 a) { u8 *p = (u8 *)(((uintptr_t)lo + a - 1) & ~(uintptr_t)(a - 1)); lo = p + s; return p; }
void *OS_AllocFromMainExArenaHi(u32 s, u32 a) { hi -= s; return hi; }
static const HeapParam t[] = { { HEAP_SIZE_SYSTEM, OS_ARENA_MAIN }, { HEAP_SIZE_SAVE, OS_ARENA_MAIN }, { HEAP_SIZE_DEBUG, OS_ARENA_MAIN }, { HEAP_SIZE_APPLICATION, OS_ARENA_MAIN } };
int main(int c, char **v) {
    u8 h[512]; lo = malloc(4 << 20); hi = lo + (4 << 20);
    if (!Rom_Open(v[1], h)) return 1;
    Heap_InitSystem(t, 4, HEAP_ID_MAX, 0);
    P_Log("app free %u", HeapExp_FndGetTotalFreeSize(HEAP_ID_APPLICATION));
    Heap_Create(HEAP_ID_APPLICATION, HEAP_ID_FIELD1, HEAP_SIZE_FIELD1);
    NARC *n = NARC_ctor(NARC_INDEX_POKETOOL__PERSONAL__PL_PERSONAL, HEAP_ID_FIELD1);
    P_Log("members %u", NARC_GetFileCount(n));
    u8 *m = NARC_AllocAndReadWholeMember(n, 1, HEAP_ID_FIELD1);
    P_Log("stats %u/%u/%u/%u/%u/%u", m[0], m[1], m[2], m[3], m[4], m[5]);
    Heap_Free(m); NARC_dtor(n);
    void *f = NARC_AllocAndReadWholeMemberByIndexPair(NARC_INDEX_GRAPHIC__PL_FONT, 0, HEAP_ID_APPLICATION);
    P_Log("font %u bytes %08x", NARC_GetMemberSizeByIndexPair(NARC_INDEX_GRAPHIC__PL_FONT, 0), *(u32 *)f);
    Heap_Free(f); Heap_Destroy(HEAP_ID_FIELD1);
    P_Log("app free after %u", HeapExp_FndGetTotalFreeSize(HEAP_ID_APPLICATION));
}

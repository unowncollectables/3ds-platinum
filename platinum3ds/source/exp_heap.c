/* Region-backed expanded heap (first-fit, alloc from head for align>0, from tail for align<0),
 * standing in for NitroSystem NNS_FndExpHeap so the game's heap.c runs unmodified. */
#include <nnsys.h>
#include <string.h>
typedef struct Blk { u32 size; u32 used; struct Blk *next; void *raw; } Blk; /* size = payload bytes */
struct P3dsExpHeap { u8 *start, *end; Blk *head; };
#define HDR ((u32)sizeof(Blk))
#define PS ((u32)sizeof(void *))
NNSFndHeapHandle NNS_FndCreateExpHeap(void *start, u32 size) {
    uintptr_t s = ((uintptr_t)start + 7) & ~(uintptr_t)7;
    struct P3dsExpHeap *h = (struct P3dsExpHeap *)s;
    h->start = (u8 *)s + ((sizeof(*h) + 7) & ~(uintptr_t)7); h->end = (u8 *)start + size;
    h->head = (Blk *)h->start; h->head->size = (u32)(h->end - h->start) - HDR; h->head->used = 0; h->head->next = NULL;
    return h;
}
void NNS_FndDestroyExpHeap(NNSFndHeapHandle h) { (void)h; }
static Blk *blk_of(const void *p) { return ((Blk *const *)p)[-1]; }
void *NNS_FndAllocFromExpHeapEx(NNSFndHeapHandle h, u32 size, int align) {
    u32 a = (u32)(align < 0 ? -align : align); if (a < 4) a = 4;
    size = (size + 3) & ~3u;
    Blk *pick = NULL;
    for (Blk *b = h->head; b; b = b->next) {
        if (b->used || b->size < size + a + PS) continue;
        pick = b; if (align >= 0) break; /* tail alloc: take the last fitting block */
    }
    if (!pick) return NULL;
    u8 *pay = (u8 *)pick + HDR, *lim = pay + pick->size;
    u8 *p;
    if (align >= 0) p = (u8 *)(((uintptr_t)pay + PS + a - 1) & ~(uintptr_t)(a - 1));
    else p = (u8 *)(((uintptr_t)lim - size) & ~(uintptr_t)(a - 1));
    u8 *used_end = (u8 *)(((uintptr_t)p + size + 7) & ~(uintptr_t)7);
    if (align < 0 && p - PS >= pay + HDR + 16) { /* split front: keep free block, new block for tail */
        Blk *nb = (Blk *)(((uintptr_t)p - PS - HDR) & ~(uintptr_t)7);
        nb->size = (u32)(lim - ((u8 *)nb + HDR)); nb->used = 0; nb->next = pick->next;
        pick->size = (u32)((u8 *)nb - pay); pick->next = nb; pick = nb; pay = (u8 *)nb + HDR;
        p = (u8 *)(((uintptr_t)lim - size) & ~(uintptr_t)(a - 1));
        if (p - PS < pay) p = (u8 *)(((uintptr_t)pay + PS + a - 1) & ~(uintptr_t)(a - 1));
        used_end = (u8 *)(((uintptr_t)p + size + 7) & ~(uintptr_t)7);
    }
    if (align >= 0 && lim >= used_end + HDR + 16) { /* split tail */
        Blk *nb = (Blk *)used_end; nb->size = (u32)(lim - used_end) - HDR; nb->used = 0; nb->next = pick->next;
        pick->size = (u32)(used_end - pay); pick->next = nb;
    }
    pick->used = size; ((Blk **)p)[-1] = pick; pick->raw = p;
    return p;
}
void NNS_FndFreeToExpHeap(NNSFndHeapHandle h, void *p) {
    if (!p) return; Blk *b = blk_of(p); b->used = 0;
    for (Blk *c = h->head; c && c->next;) { /* coalesce */
        if (!c->used && !c->next->used && (u8 *)c + HDR + c->size == (u8 *)c->next) { c->size += HDR + c->next->size; c->next = c->next->next; }
        else c = c->next;
    }
}
u32 NNS_FndGetSizeForMBlockExpHeap(const void *p) { Blk *b = blk_of(p); return b->size - (u32)((u8 *)p - ((u8 *)b + HDR)); }
u32 NNS_FndGetTotalFreeSizeForExpHeap(NNSFndHeapHandle h) { u32 t = 0; for (Blk *b = h->head; b; b = b->next) if (!b->used) t += b->size; return t; }
u32 NNS_FndResizeForMBlockExpHeap(NNSFndHeapHandle h, void *p, u32 size) { (void)h; u32 cap = NNS_FndGetSizeForMBlockExpHeap(p); if (size <= cap) { blk_of(p)->used = size; return size; } return 0; }
void NNS_FndInitAllocatorForExpHeap(NNSFndAllocator *a, NNSFndHeapHandle h, int align) { memset(a, 0, sizeof(*a)); a->pHeap = h; a->heapParam1 = (u32)align; }

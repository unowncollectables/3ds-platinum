#ifndef P3DS_NNSYS_H
#define P3DS_NNSYS_H
#include "nitro.h"
typedef struct P3dsExpHeap *NNSFndHeapHandle;
#define NNS_FND_HEAP_INVALID_HANDLE NULL
typedef struct NNSFndAllocator { void *func; void *pHeap; u32 heapParam1, heapParam2; } NNSFndAllocator;
NNSFndHeapHandle NNS_FndCreateExpHeap(void *start, u32 size);
#define NNS_FndCreateExpHeapEx(s, z, o) NNS_FndCreateExpHeap(s, z)
void NNS_FndDestroyExpHeap(NNSFndHeapHandle h);
void *NNS_FndAllocFromExpHeapEx(NNSFndHeapHandle h, u32 size, int align);
void NNS_FndFreeToExpHeap(NNSFndHeapHandle h, void *p);
u32 NNS_FndGetSizeForMBlockExpHeap(const void *p);
u32 NNS_FndGetTotalFreeSizeForExpHeap(NNSFndHeapHandle h);
u32 NNS_FndResizeForMBlockExpHeap(NNSFndHeapHandle h, void *p, u32 size);
void NNS_FndInitAllocatorForExpHeap(NNSFndAllocator *a, NNSFndHeapHandle h, int align);
#endif

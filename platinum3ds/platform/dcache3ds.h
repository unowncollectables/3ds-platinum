/* 2026-11-17: data cache flush/invalidate for GPU buffers without the GSP round trip.
   GSPGPU_FlushDataCache / GSPGPU_InvalidateDataCache are IPC calls to the GSP sysmodule, which then runs the same
   svc on our process (and has to wait its turn on the system core). Calling the svc directly does the same cache
   work without that hop. The CIA's rsf allows both svcs; if one ever fails, the GSP call runs as before. */
#ifndef DCACHE3DS_H
#define DCACHE3DS_H
#include <3ds.h>
static inline void P3DS_DcFlush(const void *p, u32 n) {
    if (!p || !n) return;
    if (R_FAILED(svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)p, n))) GSPGPU_FlushDataCache(p, n);
}
static inline void P3DS_DcInval(const void *p, u32 n) {
    if (!p || !n) return;
    if (R_FAILED(svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, (u32)p, n))) GSPGPU_InvalidateDataCache(p, n);
}
#endif

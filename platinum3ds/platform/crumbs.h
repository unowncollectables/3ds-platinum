/* Breadcrumb ring (cheap): tag + caller address; repeats of the same entry only bump a counter. */
#ifndef P3DS_CRUMBS_H
#define P3DS_CRUMBS_H
#ifdef __cplusplus
extern "C" {
#endif
void P3DS_Crumb(const char *tag, void *caller);
void P3DS_CrumbDump(void);
/* Crash handler (main3ds.c): a per-thread fault handler that writes thread name/id, registers, FPU state and a stack sample
   to sdmc:/3ds/platinum3ds/boot.txt. Call P3DS_ThreadGuard first in every thread. */
void P3DS_ThreadGuard(const char *name);
#ifdef __cplusplus
}
#endif
#define CRUMB(t) P3DS_Crumb(t, __builtin_return_address(0))
#endif

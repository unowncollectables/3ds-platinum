/* Breadcrumb ring (cheap): tag + caller address; repeats of the same entry only bump a counter. */
#ifndef P3DS_CRUMBS_H
#define P3DS_CRUMBS_H
#ifdef __cplusplus
extern "C" {
#endif
void P3DS_Crumb(const char *tag, void *caller);
void P3DS_CrumbDump(void);
#ifdef __cplusplus
}
#endif
#define CRUMB(t) P3DS_Crumb(t, __builtin_return_address(0))
#endif

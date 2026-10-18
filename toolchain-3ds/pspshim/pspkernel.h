/* platinum3ds: the PSP kernel API subset pspoke's services use, implemented on libctru (psp_ctru.c). */
#ifndef P3DS_PSPKERNEL_H
#define P3DS_PSPKERNEL_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int SceUID; typedef unsigned int SceSize; typedef unsigned int SceUInt; typedef int SceInt32;
typedef uint32_t SceUInt32; typedef int64_t SceInt64; typedef uint64_t SceUInt64; typedef void *SceKernelThreadOptParam;
typedef int (*SceKernelThreadEntry)(SceSize args, void *argp);
typedef SceUInt (*SceKernelAlarmHandler)(void *common);
#define PSP_THREAD_ATTR_USER 0x80000000u
#define PSP_THREAD_ATTR_VFPU 0x00004000u
#define THREAD_ATTR_USER PSP_THREAD_ATTR_USER
#define THREAD_ATTR_VFPU PSP_THREAD_ATTR_VFPU
#define PSP_MODULE_INFO(...)
#define PSP_MAIN_THREAD_ATTR(...)
#define PSP_HEAP_SIZE_KB(...)
SceUID sceKernelCreateThread(const char *name, SceKernelThreadEntry entry, int prio, int stack, SceUInt attr, void *opt);
int sceKernelStartThread(SceUID id, SceSize len, void *argp);
int sceKernelExitThread(int status);
int sceKernelDeleteThread(SceUID id);
int sceKernelWaitThreadEnd(SceUID id, SceUInt *timeout);
SceUID sceKernelGetThreadId(void);
int sceKernelSleepThread(void);
int sceKernelWakeupThread(SceUID id);
int sceKernelDelayThread(SceUInt us);
int sceKernelChangeThreadPriority(SceUID id, int prio);
int sceKernelRotateThreadReadyQueue(int prio);
int sceKernelSuspendDispatchThread(void);
int sceKernelResumeDispatchThread(int state);
SceInt64 sceKernelGetSystemTimeWide(void);
unsigned int sceKernelGetSystemTimeLow(void);
SceUID sceKernelSetAlarm(SceUInt us, SceKernelAlarmHandler h, void *common);
int sceKernelCancelAlarm(SceUID id);
SceUID sceKernelCreateSema(const char *name, SceUInt attr, int init, int max, void *opt);
int sceKernelDeleteSema(SceUID id);
int sceKernelWaitSema(SceUID id, int n, SceUInt *timeout);
int sceKernelSignalSema(SceUID id, int n);
void sceKernelDcacheWritebackAll(void);
void sceKernelDcacheWritebackRange(const void *p, unsigned size);
void sceKernelDcacheWritebackInvalidateRange(const void *p, unsigned size);
void sceKernelDcacheInvalidateRange(const void *p, unsigned size);
void sceKernelExitGame(void);
typedef int (*SceKernelCallbackFunction)(int a, int b, void *c);
int sceKernelCreateCallback(const char *n, SceKernelCallbackFunction f, void *a);
int sceKernelRegisterExitCallback(int cb);
int sceKernelSleepThreadCB(void);
typedef struct SceKernelThreadInfo { SceSize size; char name[32]; SceUInt attr; int status; void *entry; void *stack; int stackSize; int rest[16]; } SceKernelThreadInfo;
int sceKernelReferThreadStatus(SceUID id, SceKernelThreadInfo *i);
int sceKernelCheckThreadStack(void);
unsigned int sceKernelCpuSuspendIntr(void);
enum { PSP_SMEM_Low = 0, PSP_SMEM_High = 1, PSP_SMEM_Addr = 2 };
#define PSP_SMEM_L PSP_SMEM_Low
SceUID sceKernelAllocPartitionMemory(SceUID part, const char *name, int type, SceSize size, void *addr);
void *sceKernelGetBlockHeadAddr(SceUID id);
int sceKernelFreePartitionMemory(SceUID id);
int sceKernelPollSema(SceUID id, int n);
void sceKernelDcacheWritebackInvalidateAll(void);
void sceKernelCpuResumeIntr(unsigned int flags);
#ifdef __cplusplus
}
#endif
#endif
#include <pspiofilemgr.h>

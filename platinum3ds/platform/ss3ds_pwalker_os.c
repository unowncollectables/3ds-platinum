#include <nitro/os/common/mutex.h>
#include <nitro/os/common/message.h>

// The translated overlay's buffers use DS layouts. Native SDK objects may
// be larger, so only this bridge owns and passes them to the host SDK.
static OSMutex workerMutex;
static OSMessageQueue workerQueue;
static OSMessage workerMessage;
static OSThread overlayWorker;
static BOOL mutexReady;
static BOOL queueReady;

u32 SS3DS_Pwalker_InitMutex(void)
{
    if (!mutexReady) {
        OS_InitMutex(&workerMutex);
        mutexReady = TRUE;
    }
    return 0;
}

u32 SS3DS_Pwalker_LockMutex(void)
{
    OS_LockMutex(&workerMutex);
    return 0;
}

u32 SS3DS_Pwalker_UnlockMutex(void)
{
    OS_UnlockMutex(&workerMutex);
    return 0;
}

u32 SS3DS_Pwalker_InitQueue(void)
{
    if (!queueReady) {
        OS_InitMessageQueue(&workerQueue, &workerMessage, 1);
        queueReady = TRUE;
    }
    return 0;
}

u32 SS3DS_Pwalker_CreateWorker(void *dsThread, void (*entry)(void *), void *arg,
                              void *stack, u32 size, u32 priority)
{
    (void)dsThread;
    OS_CreateThreadReal(&overlayWorker, entry, arg, stack, size, priority);
    return 0;
}

u32 SS3DS_Pwalker_StartWorker(void)
{
    OS_WakeupThreadDirect(&overlayWorker);
    return 0;
}

void SS3DS_Pwalker_JoinWorker(void)
{
    OS_JoinThread(&overlayWorker);
}

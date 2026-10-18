/* SDL2 subset used by libntr (threads/mutex/sem/timers) on libctru. */
#include <3ds.h>
#include <SDL2/SDL.h>
#include "crumbs.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
extern volatile int p3ds_quit;
struct SDL_mutex { RecursiveLock l; };
struct SDL_semaphore { LightSemaphore s; };
struct SDL_Thread { Thread t; SDL_ThreadFunction fn; void *arg; int result; };
SDL_mutex *SDL_CreateMutex(void) { SDL_mutex *m = malloc(sizeof *m); RecursiveLock_Init(&m->l); return m; }
int SDL_LockMutex(SDL_mutex *m) { if (RecursiveLock_TryLock(&m->l)) { CRUMB("mutex-wait"); RecursiveLock_Lock(&m->l); } return 0; }
int SDL_TryLockMutex(SDL_mutex *m) { return RecursiveLock_TryLock(&m->l) ? SDL_MUTEX_TIMEDOUT : 0; }
int SDL_UnlockMutex(SDL_mutex *m) { RecursiveLock_Unlock(&m->l); return 0; }
void SDL_DestroyMutex(SDL_mutex *m) { free(m); }
SDL_sem *SDL_CreateSemaphore(Uint32 v) { SDL_sem *s = malloc(sizeof *s); LightSemaphore_Init(&s->s, v, 0x7fff); return s; }
void SDL_DestroySemaphore(SDL_sem *s) { free(s); }
int SDL_SemWait(SDL_sem *s) { CRUMB("semwait"); while (LightSemaphore_TryAcquire(&s->s, 1)) { if (p3ds_quit && threadGetCurrent()) threadExit(0); svcSleepThread(1000000); } return 0; }
int SDL_SemWaitTimeout(SDL_sem *s, Uint32 ms) { CRUMB("semwaitto");
    if (ms == SDL_MUTEX_MAXWAIT) return SDL_SemWait(s);
    u64 end = svcGetSystemTick() + (u64)ms * (SYSCLOCK_ARM11 / 1000);
    while (LightSemaphore_TryAcquire(&s->s, 1)) { if (svcGetSystemTick() >= end) return SDL_MUTEX_TIMEDOUT; svcSleepThread(200000); }
    return 0;
}
int SDL_SemPost(SDL_sem *s) { LightSemaphore_Release(&s->s, 1); return 0; }
static void tramp(void *p) { SDL_Thread *t = p; t->result = t->fn(t->arg); }
SDL_Thread *SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data) {
    (void)name; SDL_Thread *t = calloc(1, sizeof *t); t->fn = fn; t->arg = data;
    t->t = threadCreate(tramp, t, 0x10000, 0x2F, -2, false); if (!t->t) { free(t); return NULL; } return t;
}
void SDL_WaitThread(SDL_Thread *t, int *st) { if (!t) return; threadJoin(t->t, U64_MAX); if (st) *st = t->result; threadFree(t->t); free(t); }
SDL_threadID SDL_GetThreadID(SDL_Thread *t) { return t ? (SDL_threadID)(uintptr_t)t->t : SDL_ThreadID(); }
SDL_threadID SDL_ThreadID(void) { return (SDL_threadID)(uintptr_t)threadGetCurrent(); }
void SDL_Delay(Uint32 ms) { CRUMB("sdldelay"); svcSleepThread((s64)ms * 1000000); }
Uint64 SDL_GetTicks64(void) { return svcGetSystemTick() / (SYSCLOCK_ARM11 / 1000); }
Uint32 SDL_GetTicks(void) { return (Uint32)SDL_GetTicks64(); }
/* Timers: one thread per timer (libntr uses at most a couple). */
typedef struct { Thread t; volatile int live; Uint32 iv; SDL_TimerCallback cb; void *p; } Tm; static Tm timers[8];
static void tloop(void *a) { Tm *t = a; while (t->live && !p3ds_quit) { svcSleepThread((s64)t->iv * 1000000); if (!t->live) break; Uint32 n = t->cb(t->iv, t->p); if (!n) { t->live = 0; break; } t->iv = n; } }
SDL_TimerID SDL_AddTimer(Uint32 iv, SDL_TimerCallback cb, void *p) { for (int i = 0; i < 8; i++) if (!timers[i].live) { timers[i] = (Tm){0, 1, iv, cb, p}; timers[i].t = threadCreate(tloop, &timers[i], 0x4000, 0x28, -2, false); return i + 1; } return 0; }
SDL_bool SDL_RemoveTimer(SDL_TimerID id) { if (id < 1 || id > 8) return SDL_FALSE; timers[id - 1].live = 0; return SDL_TRUE; }
int SDL_ShowSimpleMessageBox(Uint32 f, const char *t, const char *m, SDL_Window *w) { printf("[SDL] %s: %s\n", t, m); return 0; }
int SDL_OpenAudio(SDL_AudioSpec *w, SDL_AudioSpec *h) { if (h) *h = *w; return 0; } /* step 2a: audio muted */
void SDL_PauseAudio(int p) {} void SDL_CloseAudio(void) {} void SDL_LockAudio(void) {} void SDL_UnlockAudio(void) {}
const char *SDL_GetError(void) { return "3DS SDL shim"; }
void P3DS_ShutdownSDL(void) { for (int i = 0; i < 8; i++) { timers[i].live = 0; if (timers[i].t) { if (R_SUCCEEDED(threadJoin(timers[i].t, 300000000ULL))) threadFree(timers[i].t); timers[i].t = NULL; } } }

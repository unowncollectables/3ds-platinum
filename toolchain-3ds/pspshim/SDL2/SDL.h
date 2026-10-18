/* platinum3ds: minimal SDL2 API subset (types/threads/mutex/sem/timers) implemented on libctru in sdl_ctru.c. */
#ifndef P3DS_SDL_H
#define P3DS_SDL_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef uint8_t Uint8; typedef uint16_t Uint16; typedef uint32_t Uint32; typedef uint64_t Uint64;
typedef int8_t Sint8; typedef int16_t Sint16; typedef int32_t Sint32; typedef int64_t Sint64;
typedef enum { SDL_FALSE = 0, SDL_TRUE = 1 } SDL_bool;
typedef struct SDL_mutex SDL_mutex; typedef struct SDL_semaphore SDL_sem; typedef struct SDL_Thread SDL_Thread;
typedef struct SDL_Window SDL_Window; typedef struct SDL_Renderer SDL_Renderer; typedef void *SDL_GLContext;
typedef struct SDL_AudioStream SDL_AudioStream; typedef struct SDL_Gamepad SDL_Gamepad;
typedef union SDL_Event { Uint32 type; Uint8 padding[56]; } SDL_Event;
typedef unsigned long SDL_threadID; typedef int SDL_TimerID;
typedef int (*SDL_ThreadFunction)(void *data);
typedef Uint32 (*SDL_TimerCallback)(Uint32 interval, void *param);
#define SDL_MUTEX_TIMEDOUT 1
#define SDL_MUTEX_MAXWAIT (~(Uint32)0)
#define SDL_MESSAGEBOX_ERROR 0x10
SDL_mutex *SDL_CreateMutex(void); int SDL_LockMutex(SDL_mutex *m); int SDL_TryLockMutex(SDL_mutex *m);
int SDL_UnlockMutex(SDL_mutex *m); void SDL_DestroyMutex(SDL_mutex *m);
SDL_sem *SDL_CreateSemaphore(Uint32 v); void SDL_DestroySemaphore(SDL_sem *s); int SDL_SemWait(SDL_sem *s);
int SDL_SemWaitTimeout(SDL_sem *s, Uint32 ms); int SDL_SemPost(SDL_sem *s);
SDL_Thread *SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data);
void SDL_WaitThread(SDL_Thread *t, int *status); SDL_threadID SDL_GetThreadID(SDL_Thread *t); SDL_threadID SDL_ThreadID(void);
void SDL_Delay(Uint32 ms); Uint32 SDL_GetTicks(void); Uint64 SDL_GetTicks64(void);
SDL_TimerID SDL_AddTimer(Uint32 interval, SDL_TimerCallback cb, void *param); SDL_bool SDL_RemoveTimer(SDL_TimerID id);
typedef Uint16 SDL_AudioFormat; typedef Uint32 SDL_AudioDeviceID;
typedef void (*SDL_AudioCallback)(void *userdata, Uint8 *stream, int len);
typedef struct SDL_AudioSpec { int freq; SDL_AudioFormat format; Uint8 channels, silence; Uint16 samples, padding; Uint32 size; SDL_AudioCallback callback; void *userdata; } SDL_AudioSpec;
#define AUDIO_S16LSB 0x8010
int SDL_OpenAudio(SDL_AudioSpec *want, SDL_AudioSpec *have); void SDL_PauseAudio(int pause); void SDL_CloseAudio(void);
void SDL_LockAudio(void); void SDL_UnlockAudio(void); const char *SDL_GetError(void);
int SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *msg, SDL_Window *w);
#ifdef __cplusplus
}
#endif
#endif

/* stand-in <3ds.h> for tools/romcache_spike_test.c (x86) */
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64; typedef int16_t s16; typedef int32_t s32; typedef void* Thread;
static inline u64 svcGetSystemTick(void){return 0;} static inline void svcSleepThread(long long x){(void)x;}
static inline unsigned long linearSpaceFree(void){return 0;}
static inline Thread threadCreate(void(*f)(void*),void*a,int s,int p,int c,int d){(void)f;(void)a;(void)s;(void)p;(void)c;(void)d;return 0;}

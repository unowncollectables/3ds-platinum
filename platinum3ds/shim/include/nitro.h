/* platinum3ds: minimal NitroSDK shim for ARM11/libctru (step 1). Only what heap.c/narc.c need. */
#ifndef P3DS_NITRO_H
#define P3DS_NITRO_H
#include <stdint.h>
#include <stddef.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32; typedef int64_t s64;
typedef volatile u8 vu8; typedef volatile u16 vu16; typedef volatile u32 vu32;
typedef int BOOL;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define NELEMS(a) (sizeof(a) / sizeof((a)[0]))
void p3ds_assert_fail(const char *file, int line);
#ifndef GF_ASSERT
#define GF_ASSERT(e) ((e) ? (void)0 : p3ds_assert_fail(__FILE__, __LINE__))
#endif
typedef enum { OS_ARENA_MAIN = 0, OS_ARENA_MAIN_SUBPRIV, OS_ARENA_MAINEX, OS_ARENA_ITCM, OS_ARENA_DTCM, OS_ARENA_SHARED, OS_ARENA_WRAM_MAIN, OS_ARENA_WRAM_SUB, OS_ARENA_WRAM_SUBPRIV, OS_ARENA_MAX } OSArenaId;
typedef u32 OSIntrMode;
typedef enum { OS_PROCMODE_USER = 16, OS_PROCMODE_IRQ = 18, OS_PROCMODE_SYS = 31 } OSProcMode;
void *OS_AllocFromMainArenaLo(u32 size, u32 align);
void *OS_AllocFromMainExArenaHi(u32 size, u32 align);
static inline OSIntrMode OS_DisableInterrupts(void) { return 0; }
static inline OSIntrMode OS_RestoreInterrupts(OSIntrMode m) { return m; }
static inline OSProcMode OS_GetProcMode(void) { return OS_PROCMODE_SYS; }
/* FS */
typedef struct FSFile { u32 top, bottom, pos, id; BOOL open; } FSFile;
typedef enum { FS_SEEK_SET = 0, FS_SEEK_CUR, FS_SEEK_END } FSSeekFileMode;
void FS_InitFile(FSFile *f);
BOOL FS_OpenFile(FSFile *f, const char *path);
BOOL FS_CloseFile(FSFile *f);
BOOL FS_SeekFile(FSFile *f, s32 off, FSSeekFileMode whence);
s32 FS_ReadFile(FSFile *f, void *dst, s32 len);
static inline u32 FS_GetLength(FSFile *f) { return f->bottom - f->top; }
static inline u32 FS_GetPosition(FSFile *f) { return f->pos - f->top; }
#include "nnsys.h"
#endif

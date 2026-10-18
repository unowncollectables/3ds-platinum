/* NDS ROM file system over one shared SD handle (pspoke NOTES 1.1/1.14: shared handle, case-insensitive paths). */
#include <nitro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "platform.h"
static FILE *rom; static u8 *fnt; static u32 *fat; static u32 fatCount;
BOOL Rom_Open(const char *path, u8 hdr[512]) {
    rom = fopen(path, "rb"); if (!rom) return FALSE;
    if (fread(hdr, 1, 512, rom) != 512) return FALSE;
    u32 fntOff, fntSize, fatOff, fatSize;
    memcpy(&fntOff, hdr + 0x40, 4); memcpy(&fntSize, hdr + 0x44, 4); memcpy(&fatOff, hdr + 0x48, 4); memcpy(&fatSize, hdr + 0x4C, 4);
    fnt = malloc(fntSize); fat = malloc(fatSize); fatCount = fatSize / 8;
    fseek(rom, fntOff, SEEK_SET); fread(fnt, 1, fntSize, rom);
    fseek(rom, fatOff, SEEK_SET); fread(fat, 1, fatSize, rom);
    P_Log("ROM FS: FNT %lu bytes, FAT %lu files", (unsigned long)fntSize, (unsigned long)fatCount);
    return TRUE;
}
FILE *Rom_Handle(void) { return rom; }
static int lookup(const char *path) {
    u32 dir = 0xF000;
    while (*path == '/') path++;
    while (*path) {
        const char *seg = path; size_t n = 0; while (seg[n] && seg[n] != '/') n++;
        int last = seg[n] == 0;
        const u8 *e = fnt + *(u32 *)(fnt + (dir & 0xFFF) * 8); u16 id = *(u16 *)(fnt + (dir & 0xFFF) * 8 + 4);
        int found = -1;
        while (*e) {
            u8 len = *e & 0x7F, isdir = *e & 0x80; const char *nm = (const char *)e + 1;
            int eq = len == n; for (size_t i = 0; eq && i < n; i++) eq = tolower((u8)nm[i]) == tolower((u8)seg[i]);
            if (isdir) { if (eq && !last) { found = *(u16 *)(e + 1 + len); break; } e += 3 + len; }
            else { if (eq && last) return id; id++; e += 1 + len; }
        }
        if (found < 0) return -1;
        dir = (u32)found; path = seg + n + 1;
    }
    return -1;
}
void FS_InitFile(FSFile *f) { memset(f, 0, sizeof(*f)); }
BOOL FS_OpenFile(FSFile *f, const char *path) {
    int id = lookup(path);
    if (id < 0 || (u32)id >= fatCount) { P_Log("[FS] open FAILED: %s", path); return FALSE; }
    f->id = id; f->top = fat[id * 2]; f->bottom = fat[id * 2 + 1]; f->pos = f->top; f->open = TRUE; return TRUE;
}
BOOL FS_CloseFile(FSFile *f) { f->open = FALSE; return TRUE; }
BOOL FS_SeekFile(FSFile *f, s32 off, FSSeekFileMode w) {
    s64 b = w == FS_SEEK_SET ? f->top : w == FS_SEEK_CUR ? f->pos : f->bottom; b += off;
    if (b < f->top) b = f->top; if (b > f->bottom) b = f->bottom; f->pos = (u32)b; return TRUE;
}
s32 FS_ReadFile(FSFile *f, void *dst, s32 len) {
    if (!f->open) return -1;
    if ((u32)len > f->bottom - f->pos) len = (s32)(f->bottom - f->pos);
    fseek(rom, f->pos, SEEK_SET); s32 got = (s32)fread(dst, 1, len, rom); clearerr(rom);
    f->pos += got; return got;
}

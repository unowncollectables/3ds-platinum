#pragma once
#include <pspkernel.h>
#define PSP_O_RDONLY 0x1
#define PSP_O_WRONLY 0x2
#define PSP_O_RDWR 0x3
#define PSP_O_APPEND 0x100
#define PSP_O_CREAT 0x200
#define PSP_O_TRUNC 0x400
SceUID sceIoOpen(const char *path, int flags, int mode); int sceIoWrite(SceUID fd, const void *b, SceSize n);
int sceIoRead(SceUID fd, void *b, SceSize n); int sceIoClose(SceUID fd);

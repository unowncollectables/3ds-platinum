#pragma once
#include <pspkernel.h>
enum { PSP_CTRL_SELECT = 0x1, PSP_CTRL_START = 0x8, PSP_CTRL_UP = 0x10, PSP_CTRL_RIGHT = 0x20, PSP_CTRL_DOWN = 0x40, PSP_CTRL_LEFT = 0x80,
       PSP_CTRL_LTRIGGER = 0x100, PSP_CTRL_RTRIGGER = 0x200, PSP_CTRL_TRIANGLE = 0x1000, PSP_CTRL_CIRCLE = 0x2000, PSP_CTRL_CROSS = 0x4000, PSP_CTRL_SQUARE = 0x8000 };
#define PSP_CTRL_MODE_DIGITAL 0
#define PSP_CTRL_MODE_ANALOG 1
typedef struct SceCtrlData { unsigned int TimeStamp, Buttons; unsigned char Lx, Ly, Rsrv[6]; } SceCtrlData;
int sceCtrlSetSamplingCycle(int c); int sceCtrlSetSamplingMode(int m);
int sceCtrlPeekBufferPositive(SceCtrlData *d, int n); int sceCtrlReadBufferPositive(SceCtrlData *d, int n);

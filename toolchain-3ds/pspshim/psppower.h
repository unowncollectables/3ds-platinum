#pragma once
#include <pspkernel.h>
#define PSP_POWER_CB_SUSPENDING 0x00010000
#define PSP_POWER_CB_RESUMING 0x00020000
#define PSP_POWER_CB_RESUME_COMPLETE 0x00040000
int scePowerRegisterCallback(int slot, SceUID cb);
int scePowerSetClockFrequency(int a, int b, int c);

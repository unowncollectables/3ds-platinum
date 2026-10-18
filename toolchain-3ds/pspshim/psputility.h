#pragma once
#include <pspkernel.h>
#define PSP_MODULE_AV_AVCODEC 0x300
#define PSP_MODULE_AV_SASCORE 0x301
int sceUtilityLoadModule(int id);
int sceUtilityUnloadModule(int id);

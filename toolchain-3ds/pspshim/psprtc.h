#pragma once
#include <pspkernel.h>
typedef struct ScePspDateTime { unsigned short year, month, day, hour, minute, second; unsigned int microsecond; } ScePspDateTime;
int sceRtcGetCurrentClockLocalTime(ScePspDateTime *t); int sceRtcGetDayOfWeek(int y, int m, int d);

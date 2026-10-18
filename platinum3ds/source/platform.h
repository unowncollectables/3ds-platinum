#pragma once
#include <stdio.h>
void P_Log(const char *fmt, ...);
void P_Fatal(const char *fmt, ...);
BOOL Rom_Open(const char *path, u8 hdr[512]);
FILE *Rom_Handle(void);

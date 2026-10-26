/* wireless M1: plain-C interface between wm_uds.c (DS WM API, NitroSDK types) and uds_ctru.c (libctru UDS). */
#pragma once

#define P3U_APP_MAX 200 /* UDS beacon application data limit */
#define P3U_NETS_MAX 8

enum { P3U_OFF, P3U_IDLE, P3U_BUSY, P3U_HOST, P3U_CLIENT, P3U_FAILED, P3U_LOST };

typedef struct {
	unsigned char mac[6];
	unsigned char channel, nodes, maxNodes, pad;
	int appLen;
	unsigned char app[P3U_APP_MAX];
} P3UNet;

unsigned long long P3U_NowUs(void);
int P3U_Init(void);           /* udsInit + worker thread; 0 when wireless is off or the service fails */
void P3U_Exit(void);
void P3U_Host(const void *app, int len, int maxNodes); /* async: state BUSY -> HOST or FAILED */
void P3U_SetAppData(const void *app, int len);
void P3U_Scan(int on);        /* worker scans while idle */
int P3U_ScanResults(P3UNet *out, int max);
void P3U_Connect(const unsigned char mac[6]); /* async: BUSY -> CLIENT or FAILED */
void P3U_Leave(void);
void P3U_Eject(unsigned short node);
int P3U_State(void);
unsigned short P3U_Bitmask(void); /* bit (node - 1) per connected node */
unsigned short P3U_MyNode(void);
int P3U_Send(unsigned short node, const void *buf, int len); /* 1 ok, 0 dropped, -1 fatal */
int P3U_Pull(void *buf, int max, unsigned short *src);      /* bytes, 0 = nothing waiting */

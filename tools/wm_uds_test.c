/* Host test for platform/wm_uds.c (wireless M1): two processes, a parent and a child, each run the real wm_uds.c
 * against a mock 3DS UDS layer (P3U_*) that carries frames over a socketpair and can lose, duplicate and reorder them.
 * A small fake "game" drives the WM API the way pret/pokeplatinum src/wireless_manager.c does (initialize, parent
 * parameters, start parent / scan + connect with a child SSID, start MP, port sends one at a time from the send
 * callback, port receive callback) and checks that every message arrives once, in order, intact, both ways.
 * Needs the libntr headers (pspoke fetches cybervisi0n/libntr) and SDL2 headers (libntr's x86 thread.h):
 *   gcc -O1 -w -DSDK_PORT -DSDK_X86 -DSDK_TS -DSDK_4M -DSDK_FINALROM -DNNS_FINALROM -DSDK_VERSION_MAJOR=4 \
 *       -DuPtr=uintptr_t -include stdint.h -include errno.h -I<dir with nitro/fx/fx_const.h> -I<libntr>/include \
 *       -I platinum3ds/platform tools/wm_uds_test.c -o wt && ./wt
 * (fx_const.h: python3 <libntr>/gen/nitro/fx/gen_fx_const.py <libntr>/gen/nitro/fx/fx_const.csv <dir>/nitro/fx/fx_const.h) */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>

/* ---------------------------------------------------------------- mock platform */
static int gRole; /* 1 parent, 2 child */
static int gVerbose;
void PSPNativeMemLog(const char *fmt, ...)
{
	if (!gVerbose) return;
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "%s ", gRole == 1 ? "P:" : "C:");
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
}
int P3DS_CfgInt(const char *k, int def) { return !strcmp(k, "wireless") ? 1 : def; }

/* ---------------------------------------------------------------- mock UDS (uds_ctru.h) over a socketpair */
#include "uds_ctru.h"
enum { M_DATA = 1, M_BEACON, M_JOIN, M_LEAVE };
static int gFd;
static double gLoss, gDup, gReorder;
static int gState = P3U_OFF, gScan, gPeerHosting, gPeerJoined;
static unsigned char gBeacon[P3U_APP_MAX];
static int gBeaconLen;
static unsigned char gHeld[2048];
static int gHeldLen;
static unsigned long gSent, gDropped, gDuped, gReordered;

unsigned long long P3U_NowUs(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long long)ts.tv_sec * 1000000ull + (unsigned long long)ts.tv_nsec / 1000;
}
static double rnd(void) { return rand() / (RAND_MAX + 1.0); }
static void ctl(int type, const void *d, int len)
{
	unsigned char b[2048];
	b[0] = (unsigned char)type;
	if (len) memcpy(b + 1, d, (size_t)len);
	send(gFd, b, (size_t)len + 1, 0);
}
static void pollCtl(void); /* control frames are read together with data in P3U_Pull */
int P3U_Init(void) { gState = P3U_IDLE; return 1; }
void P3U_Exit(void) { gState = P3U_OFF; }
void P3U_Host(const void *app, int len, int maxNodes)
{
	(void)maxNodes;
	gState = P3U_HOST;
	ctl(M_BEACON, app, len);
}
void P3U_SetAppData(const void *app, int len) { ctl(M_BEACON, app, len); }
void P3U_Scan(int on) { gScan = on; }
int P3U_ScanResults(P3UNet *out, int max)
{
	pollCtl();
	if (!gPeerHosting || max < 1) return 0;
	memset(out, 0, sizeof *out);
	memcpy(out->mac, "\x02\x11\x22\x33\x44\x55", 6);
	out->channel = 6; out->nodes = 1; out->maxNodes = 8;
	out->appLen = gBeaconLen; memcpy(out->app, gBeacon, (size_t)gBeaconLen);
	return 1;
}
void P3U_Connect(const unsigned char mac[6]) { (void)mac; gState = gPeerHosting ? P3U_CLIENT : P3U_FAILED; if (gState == P3U_CLIENT) ctl(M_JOIN, NULL, 0); }
void P3U_Leave(void) { if (gState == P3U_HOST || gState == P3U_CLIENT) ctl(M_LEAVE, NULL, 0); gState = P3U_IDLE; }
void P3U_Eject(unsigned short node) { (void)node; ctl(M_LEAVE, NULL, 0); gPeerJoined = 0; }
int P3U_State(void) { pollCtl(); return gState; }
unsigned short P3U_Bitmask(void) { pollCtl(); return (unsigned short)(gState == P3U_HOST ? 1 | (gPeerJoined ? 2 : 0) : gState == P3U_CLIENT ? 3 : 0); }
unsigned short P3U_MyNode(void) { return (unsigned short)(gState == P3U_HOST ? 1 : 2); }
int P3U_Send(unsigned short node, const void *buf, int len)
{
	(void)node;
	if (gState != P3U_HOST && gState != P3U_CLIENT) return 0;
	gSent++;
	if (rnd() < gLoss) { gDropped++; return 1; }      /* lost in the air: the sender can't tell */
	if (gHeldLen == 0 && rnd() < gReorder) { gHeld[0] = M_DATA; memcpy(gHeld + 1, buf, (size_t)len); gHeldLen = len + 1; gReordered++; return 1; }
	ctl(M_DATA, buf, len);
	if (rnd() < gDup) { ctl(M_DATA, buf, len); gDuped++; }
	if (gHeldLen) { send(gFd, gHeld, (size_t)gHeldLen, 0); gHeldLen = 0; }
	return 1;
}
/* Data waiting for P3U_Pull (control frames are applied as they are read). */
static unsigned char gQ[256][2048];
static int gQLen[256], gQHead, gQCount;
static void pollCtl(void)
{
	unsigned char b[2048];
	ssize_t n;
	while ((n = recv(gFd, b, sizeof b, MSG_DONTWAIT)) > 0) {
		switch (b[0]) {
		case M_BEACON: gPeerHosting = 1; gBeaconLen = (int)n - 1; memcpy(gBeacon, b + 1, (size_t)gBeaconLen); break;
		case M_JOIN: gPeerJoined = 1; break;
		case M_LEAVE:
			if (gState == P3U_CLIENT) gState = P3U_LOST;
			gPeerJoined = 0; gPeerHosting = 0;
			break;
		case M_DATA:
			if (gQCount < 256 && (gState == P3U_HOST || gState == P3U_CLIENT)) {
				int i = (gQHead + gQCount++) % 256;
				gQLen[i] = (int)n - 1; memcpy(gQ[i], b + 1, (size_t)gQLen[i]);
			}
			break;
		}
	}
}
int P3U_Pull(void *buf, int max, unsigned short *src)
{
	pollCtl();
	if (!gQCount || (gState != P3U_HOST && gState != P3U_CLIENT)) return 0;
	int len = gQLen[gQHead] < max ? gQLen[gQHead] : max;
	memcpy(buf, gQ[gQHead], (size_t)len);
	gQHead = (gQHead + 1) % 256; gQCount--;
	*src = (unsigned short)(gState == P3U_HOST ? 2 : 1);
	return len;
}

/* ---------------------------------------------------------------- the code under test */
#include "../platinum3ds/platform/wm_uds.c"

/* ---------------------------------------------------------------- fake game (wireless_manager.c shape) */
#define GGID 0x00000333u
#define PORT 14
static int N_MSG = 600;
static u16 userInfo[8] = { 0x1234, 0x5678 };
static WMParentParam param;
static WMScanParam scanParam;
static WMBssDesc bss;
static int inited, connected, mpStarted, sendBusy, disconnects, failures, wrongSsid;
static int nextSend, nextRecv, badRecv;
static u16 myAidGame;

static void onInit(void *a) { (void)a; inited = 1; }
static void onParam(void *a);
static void onParent(void *a)
{
	WMStartParentCallback *c = a;
	if (c->errcode) { failures++; return; }
	if (c->state == WM_STATECODE_PARENT_START) WM_StartMP(NULL, NULL, 0, NULL, 0, 1), mpStarted = 1;
	if (c->state == WM_STATECODE_CONNECTED) {
		if (c->ssid[0] != 'X' || memcmp(c->ssid + 1, "DP", 3)) { wrongSsid++; WM_Disconnect(NULL, c->aid); return; }
		connected = 1;
	}
	if (c->state == WM_STATECODE_DISCONNECTED) { connected = 0; disconnects++; }
}
static void onParam(void *a) { (void)a; WM_StartParent(onParent); }
static void onConnect(void *a)
{
	WMStartConnectCallback *c = a;
	if (c->errcode) { failures++; return; }
	if (c->state == WM_STATECODE_CONNECTED) { connected = 1; myAidGame = c->aid; WM_StartMP(NULL, NULL, 0, NULL, 0, 1); mpStarted = 1; }
	if (c->state == WM_STATECODE_DISCONNECTED) { connected = 0; disconnects++; }
}
static int scanning, foundParent;
static void onScan(void *a)
{
	WMStartScanCallback *c = a;
	scanning = 0;
	if (c->state == WM_STATECODE_PARENT_FOUND && c->gameInfoLength >= 8 && c->gameInfo.ggid == GGID &&
	    (c->gameInfo.attribute & (WM_ATTR_FLAG_ENTRY | WM_ATTR_FLAG_MB)) == WM_ATTR_FLAG_ENTRY &&
	    c->gameInfo.userGameInfo[0] == 0x1234 && !memcmp(bss.bssid, c->macAddress, 6))
		foundParent = 1;
}
static void onSend(void *a) { WMPortSendCallback *c = a; if (c->errcode) failures++; sendBusy = 0; }
static void onRecv(void *a)
{
	WMPortRecvCallback *c = a;
	if (c->state == WM_STATECODE_DISCONNECTED) return;
	if (c->state != WM_STATECODE_PORT_RECV) return;
	const u8 *d = (const u8 *)c->data;
	int seq = d[0] | d[1] << 8;
	int ok = seq == nextRecv && c->length == (gRole == 1 ? 38 : 192);
	for (int i = 2; ok && i < c->length; i++) ok = d[i] == (u8)(seq * 7 + i);
	if (!ok) { badRecv++; if (badRecv < 5) fprintf(stderr, "%s bad message: got seq %d len %u, want %d\n", gRole == 1 ? "P:" : "C:", seq, c->length, nextRecv); }
	nextRecv = seq + 1;
}
static void sendNext(void)
{
	static u8 buf[192];
	const int len = gRole == 1 ? 192 : 38; /* parent / child MP sizes */
	buf[0] = (u8)nextSend; buf[1] = (u8)(nextSend >> 8);
	for (int i = 2; i < len; i++) buf[i] = (u8)(nextSend * 7 + i);
	if (WM_SetMPDataToPortEx(onSend, NULL, (u16 *)buf, (u16)len, 0xFFFF, PORT, 0) == WM_ERRCODE_OPERATING) { sendBusy = 1; nextSend++; }
}

static int run(int role, int killAfter)
{
	gRole = role;
	WM_Initialize(NULL, onInit, 2);
	WM_SetPortCallback(PORT, onRecv, NULL);
	const unsigned long long start = P3U_NowUs();
	int frames = 0, phase = 0;
	while (P3U_NowUs() - start < 60000000ull) {
		PSPNativeWMPump();
		frames++;
		if (role == 1) {
			if (inited && phase == 0) {
				memset(&param, 0, sizeof param);
				param.ggid = GGID; param.tgid = 1; param.entryFlag = 1; param.maxEntry = 1; param.beaconPeriod = 200;
				param.userGameInfo = userInfo; param.userGameInfoLength = sizeof userInfo; param.parentMaxSize = 192; param.childMaxSize = 38;
				WM_SetParentParameter(onParam, &param); phase = 1;
			}
		} else {
			if (inited && phase == 0 && !scanning && !foundParent) {
				memset(&scanParam, 0, sizeof scanParam);
				scanParam.scanBuf = &bss; scanParam.channel = 1; scanParam.maxChannelTime = 13;
				scanning = 1; WM_StartScan(onScan, &scanParam);
			}
			if (phase == 0 && foundParent) {
				u8 ssid[24] = { 'X', 'D', 'P', 0 };
				WM_EndScan(NULL); WM_StartConnectEx(onConnect, &bss, ssid, 1, WM_AUTHMODE_OPEN_SYSTEM); phase = 1;
			}
		}
		if (connected && mpStarted && !sendBusy && nextSend < N_MSG) sendNext();
		if (killAfter && nextRecv >= killAfter && role == 1) { fprintf(stderr, "P: vanishing after %d messages\n", nextRecv); fflush(stdout); _exit(0); }
		if (nextSend >= N_MSG && nextRecv >= N_MSG) {
			/* both directions done: the child hangs up, the parent waits to hear it */
			/* (the real game agrees on the end with its own sync commands first; here: wait until our sends are acked) */
			if (role == 2 && peers[0].txCount) { usleep(2000); continue; }
			if (role == 2) { WM_Disconnect(NULL, 0); for (int i = 0; i < 20; i++) { PSPNativeWMPump(); usleep(2000); } break; }
			if (disconnects) break;
		}
		if (killAfter && role == 2 && disconnects) break;
		usleep(2000);
	}
	const double s = (P3U_NowUs() - start) / 1e6;
	printf("%s %s: sent %d recv %d bad %d disconnects %d failures %d wrongSsid %d | udp sent %lu lost %lu dup %lu reordered %lu | %.1f s\n",
	       role == 1 ? "parent" : "child ", killAfter ? "(host vanishes)" : "", nextSend, nextRecv, badRecv, disconnects, failures,
	       wrongSsid, gSent, gDropped, gDuped, gReordered, s);
	fflush(stdout);
	if (killAfter) return role == 2 && disconnects == 1 && badRecv == 0 ? 0 : 1;
	return nextSend == N_MSG && nextRecv == N_MSG && badRecv == 0 && failures == 0 && wrongSsid == 0 && disconnects == (role == 1) ? 0 : 1;
}

static int scenario(const char *name, double loss, double dup, double reorder, int killAfter)
{
	int sv[2];
	socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
	printf("== %s (loss %.0f%%, dup %.0f%%, reorder %.0f%%)\n", name, loss * 100, dup * 100, reorder * 100);
	fflush(stdout);
	pid_t p = fork();
	if (p == 0) { close(sv[1]); gFd = sv[0]; gLoss = loss; gDup = dup; gReorder = reorder; srand(1); _exit(run(1, killAfter)); }
	pid_t c = fork();
	if (c == 0) { close(sv[0]); gFd = sv[1]; gLoss = loss; gDup = dup; gReorder = reorder; srand(2); _exit(run(2, killAfter)); }
	close(sv[0]); close(sv[1]);
	int s1, s2;
	waitpid(p, &s1, 0); waitpid(c, &s2, 0);
	const int ok = WIFEXITED(s1) && WIFEXITED(s2) && WEXITSTATUS(s2) == 0 && (killAfter || WEXITSTATUS(s1) == 0);
	printf("   %s\n", ok ? "PASS" : "FAIL");
	return ok;
}

int main(int argc, char **argv)
{
	gVerbose = argc > 1 && !strcmp(argv[1], "-v");
	int ok = 1;
	ok &= scenario("clean link", 0, 0, 0, 0);
	if (gVerbose) return 0;
	ok &= scenario("M0 broadcast-like loss", 0.025, 0, 0, 0);
	ok &= scenario("bad link", 0.05, 0.01, 0.02, 0);
	ok &= scenario("very bad link", 0.15, 0.03, 0.05, 0);
	ok &= scenario("host vanishes mid-trade", 0.02, 0, 0, 200);
	printf(ok ? "ALL PASS\n" : "SOME FAILED\n");
	return ok ? 0 : 1;
}

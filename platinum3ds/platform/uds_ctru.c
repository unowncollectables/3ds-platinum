/* wireless M1 (docs/WIRELESS.md): thin libctru UDS (3DS local wireless) layer for wm_uds.c, which speaks the DS WM
 * API to the game. Plain C types only, so wm_uds.c can include the NitroSDK headers without clashing with <3ds.h>.
 * Slow calls (create, scan, connect, destroy) run on a worker thread so the game thread never stalls; sends and
 * receives are non-blocking and are made from the game thread (wm_uds.c's pump, between frames).
 * Measured with tools/uds_probe (2026-10-26, two New 3DS): rtt p50 4-6 ms, p99 13-20 ms; 0.5-2.5% of BROADCAST frames
 * lost, unicast replies never lost, so wm_uds.c sends unicast only and resends what is not acknowledged. */
#include <3ds.h>
#include <stdlib.h>
#include <string.h>
#include "uds_ctru.h"

extern void PSPNativeMemLog(const char *, ...);

#define WLANCOMMID 0x50334453u /* 'P3DS', same as tools/uds_probe; shared by the Platinum and SoulSilver ports */
#define DATA_CHANNEL 1
static const char kPass[] = "p3ds-wireless-v1";

/* Role commands: the latest one wins (never blocks the game thread, even during a scan). */
enum { CMD_NONE, CMD_HOST, CMD_CONNECT, CMD_LEAVE };

static volatile int sState = P3U_OFF, sScanOn;
static int sCmd = CMD_NONE; /* handed over with __atomic ops */
static volatile u16 sBitmask, sMyNode;
static volatile int sQuit;
static int sRole; /* worker only: 1 = a network is hosted, 2 = joined (what leave() must tear down) */
static Thread sThread;
static LightLock sLock; /* guards the scan list, the pending app data and the connect target */
static udsBindContext sBind;
static u8 sApp[P3U_APP_MAX], sAppPending[P3U_APP_MAX];
static int sAppLen, sAppPendingLen, sMaxNodes;
static u8 sTarget[6];
static int sEjectMask, sAppDirty; /* handed over with __atomic ops */
static P3UNet sNets[P3U_NETS_MAX];
static int sNetCount;
static udsNetworkStruct sNetStruct[P3U_NETS_MAX];
static u8 sScanBuf[0x4000] __attribute__((aligned(4)));

u64 P3U_NowUs(void) { return svcGetSystemTick() * 1000000ull / SYSCLOCK_ARM11; }

static void doScan(void)
{
	udsNetworkScanInfo *nets = NULL;
	size_t total = 0;
	Result r = udsScanBeacons(sScanBuf, sizeof sScanBuf, &nets, &total, WLANCOMMID, 0, NULL, false);
	if (R_FAILED(r)) { if (nets) free(nets); return; }
	LightLock_Lock(&sLock);
	sNetCount = 0;
	for (size_t i = 0; i < total && sNetCount < P3U_NETS_MAX; i++) {
		P3UNet *n = &sNets[sNetCount];
		size_t asz = 0;
		memset(n, 0, sizeof *n);
		if (R_FAILED(udsGetNetworkStructApplicationData(&nets[i].network, n->app, sizeof n->app, &asz))) continue;
		n->appLen = (int)asz;
		memcpy(n->mac, nets[i].network.host_macaddress, 6);
		n->channel = nets[i].network.channel;
		n->nodes = nets[i].network.total_nodes;
		n->maxNodes = nets[i].network.max_nodes;
		sNetStruct[sNetCount] = nets[i].network;
		sNetCount++;
	}
	LightLock_Unlock(&sLock);
	free(nets);
}

static void pollStatus(void)
{
	udsConnectionStatus st;
	if (R_FAILED(udsGetConnectionStatus(&st))) return;
	sBitmask = st.node_bitmask;
	sMyNode = st.cur_NetworkNodeID;
	/* A client whose host vanished (or the console slept) drops to a non-connected status with no host bit. */
	if (sState == P3U_CLIENT && !(st.node_bitmask & 1)) {
		PSPNativeMemLog("[WIRELESS] uds: host gone (status %lu, bitmask %04x)", st.status, st.node_bitmask);
		udsDisconnectNetwork(); udsUnbind(&sBind); sRole = 0; sState = P3U_LOST;
	}
}

static void leave(void)
{
	if (sRole == 1) { udsDestroyNetwork(); udsUnbind(&sBind); }
	else if (sRole == 2) { udsDisconnectNetwork(); udsUnbind(&sBind); }
	sRole = 0; sState = P3U_IDLE; sBitmask = 0; sMyNode = 0;
}

static void worker(void *arg)
{
	(void)arg;
	u64 lastStatus = 0;
	while (!sQuit) {
		int cmd = __atomic_exchange_n(&sCmd, CMD_NONE, __ATOMIC_SEQ_CST);
		switch (cmd) {
		case CMD_HOST: {
			leave();
			udsNetworkStruct net;
			udsGenerateDefaultNetworkStruct(&net, WLANCOMMID, 0, (u8)sMaxNodes);
			Result r = udsCreateNetwork(&net, kPass, sizeof kPass, &sBind, DATA_CHANNEL, UDS_DEFAULT_RECVBUFSIZE);
			if (R_FAILED(r)) { PSPNativeMemLog("[WIRELESS] uds: create network failed %08lx", r); sState = P3U_FAILED; break; }
			LightLock_Lock(&sLock);
			r = udsSetApplicationData(sApp, sAppLen);
			LightLock_Unlock(&sLock);
			u8 ch = 0; udsGetChannel(&ch);
			PSPNativeMemLog("[WIRELESS] uds: hosting, channel %u, %d nodes max, app data %d B%s", ch, sMaxNodes, sAppLen,
			                R_FAILED(r) ? " (set FAILED)" : "");
			sRole = 1; sState = P3U_HOST;
			break;
		}
		case CMD_CONNECT: {
			leave();
			udsNetworkStruct net;
			int found = 0;
			LightLock_Lock(&sLock);
			for (int i = 0; i < sNetCount; i++) if (!memcmp(sNets[i].mac, sTarget, 6)) { net = sNetStruct[i]; found = 1; break; }
			LightLock_Unlock(&sLock);
			if (!found) { PSPNativeMemLog("[WIRELESS] uds: connect target not in the scan list"); sState = P3U_FAILED; break; }
			Result r = 0;
			for (int t = 0; t < 5; t++) {
				r = udsConnectNetwork(&net, kPass, sizeof kPass, &sBind, UDS_BROADCAST_NETWORKNODEID, UDSCONTYPE_Client,
				                      DATA_CHANNEL, UDS_DEFAULT_RECVBUFSIZE);
				if (R_SUCCEEDED(r)) break;
				svcSleepThread(100000000);
			}
			if (R_FAILED(r)) { PSPNativeMemLog("[WIRELESS] uds: connect failed %08lx", r); sState = P3U_FAILED; break; }
			sRole = 2; sState = P3U_CLIENT;
			pollStatus();
			PSPNativeMemLog("[WIRELESS] uds: connected as node %u", sMyNode);
			break;
		}
		case CMD_LEAVE:
			leave();
			break;
		default:
			break;
		}
		if (__atomic_exchange_n(&sAppDirty, 0, __ATOMIC_SEQ_CST) && sRole == 1) {
			LightLock_Lock(&sLock);
			memcpy(sApp, sAppPending, sAppPendingLen); sAppLen = sAppPendingLen;
			udsSetApplicationData(sApp, sAppLen);
			LightLock_Unlock(&sLock);
		}
		int ej = __atomic_exchange_n(&sEjectMask, 0, __ATOMIC_SEQ_CST);
		for (int n = 2; ej && sRole == 1 && n <= UDS_MAXNODES; n++) if (ej & (1 << (n - 1))) udsEjectClient((u16)n);
		if (sScanOn && (sState == P3U_IDLE || sState == P3U_FAILED || sState == P3U_LOST)) doScan();
		u64 t = P3U_NowUs();
		if ((sState == P3U_HOST || sState == P3U_CLIENT) && t - lastStatus > 100000) { pollStatus(); lastStatus = t; }
		if (__atomic_load_n(&sCmd, __ATOMIC_SEQ_CST) == CMD_NONE) svcSleepThread(sScanOn ? 20000000 : 10000000);
	}
	leave();
}

int P3U_Init(void)
{
	if (sState != P3U_OFF) return 1;
	Result r = udsInit(0x3000, NULL);
	if (R_FAILED(r)) { PSPNativeMemLog("[WIRELESS] udsInit failed %08lx (wireless switched off?)", r); return 0; }
	LightLock_Init(&sLock);
	sQuit = 0; sCmd = CMD_NONE; sScanOn = 0; sNetCount = 0; sState = P3U_IDLE;
	sThread = threadCreate(worker, NULL, 0x4000, 0x31, -2, false);
	if (!sThread) { udsExit(); sState = P3U_OFF; PSPNativeMemLog("[WIRELESS] uds worker thread failed"); return 0; }
	PSPNativeMemLog("[WIRELESS] udsInit ok");
	return 1;
}

void P3U_Exit(void)
{
	if (sState == P3U_OFF) return;
	sQuit = 1;
	threadJoin(sThread, U64_MAX);
	threadFree(sThread);
	udsExit();
	sState = P3U_OFF;
	PSPNativeMemLog("[WIRELESS] udsExit");
}

static void post(int cmd) { __atomic_store_n(&sCmd, cmd, __ATOMIC_SEQ_CST); }

void P3U_Host(const void *app, int len, int maxNodes)
{
	if (len > P3U_APP_MAX) len = P3U_APP_MAX;
	LightLock_Lock(&sLock); memcpy(sApp, app, len); sAppLen = len; LightLock_Unlock(&sLock);
	sMaxNodes = maxNodes < 2 ? 2 : maxNodes > UDS_MAXNODES ? UDS_MAXNODES : maxNodes;
	sState = P3U_BUSY; post(CMD_HOST);
}

void P3U_SetAppData(const void *app, int len)
{
	if (len > P3U_APP_MAX) len = P3U_APP_MAX;
	LightLock_Lock(&sLock); memcpy(sAppPending, app, len); sAppPendingLen = len; LightLock_Unlock(&sLock);
	__atomic_store_n(&sAppDirty, 1, __ATOMIC_SEQ_CST);
}

void P3U_Scan(int on) { sScanOn = on; }

int P3U_ScanResults(P3UNet *out, int max)
{
	LightLock_Lock(&sLock);
	int n = sNetCount < max ? sNetCount : max;
	memcpy(out, sNets, (size_t)n * sizeof *out);
	LightLock_Unlock(&sLock);
	return n;
}

void P3U_Connect(const unsigned char mac[6]) { memcpy(sTarget, mac, 6); sState = P3U_BUSY; post(CMD_CONNECT); }
void P3U_Leave(void) { post(CMD_LEAVE); }
void P3U_Eject(unsigned short node) { if (node >= 2 && node <= UDS_MAXNODES) __atomic_or_fetch(&sEjectMask, 1 << (node - 1), __ATOMIC_SEQ_CST); }
int P3U_State(void) { return sState; }
unsigned short P3U_Bitmask(void) { return sBitmask; }
unsigned short P3U_MyNode(void) { return sMyNode; }

int P3U_Send(unsigned short node, const void *buf, int len)
{
	if (sState != P3U_HOST && sState != P3U_CLIENT) return 0;
	Result r = udsSendTo(node, DATA_CHANNEL, UDS_SENDFLAG_Default, buf, (size_t)len);
	return R_SUCCEEDED(r) ? 1 : UDS_CHECK_SENDTO_FATALERROR(r) ? -1 : 0;
}

int P3U_Pull(void *buf, int max, unsigned short *src)
{
	if (sState != P3U_HOST && sState != P3U_CLIENT) return 0;
	size_t got = 0;
	u16 s = 0;
	if (R_FAILED(udsPullPacket(&sBind, buf, (size_t)max, &got, &s))) return 0;
	*src = s;
	return (int)got;
}

/* uds_probe: M0 of docs/WIRELESS.md. Measures 3DS local wireless (UDS) between two (or more) 3DSes with the
 * packet shape the Gen 4 games use for DS MP: the host sends one 192-byte packet per cycle (60 or 30 Hz), every
 * client answers with 38 bytes (the Underground's child size). The host logs round-trip time, loss and late replies
 * per client; clients log how evenly the host's packets arrive. Nothing here touches the game, its save or the SD card
 * beyond appending results to sdmc:/3ds/uds_probe.txt.
 *
 * A = host, X = join, UP/DOWN = rate (before starting), START = quit. */
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#define WLANCOMMID 0x50334453u /* 'P3DS': shared by both ports' future wm_uds.c, nothing else uses it */
#define DATA_CHANNEL 1
#define MAX_CLIENTS 7          /* DS MP limit the games use (1 parent + 7 children) */
#define HOST_SIZE 192          /* parent MP packet size in wireless_manager.c */
#define CLIENT_SIZE 38         /* Underground child MP size; trades use 12 */
#define CYCLES_PER_RUN_S 60    /* run length in seconds */
#define HIST_BUCKETS 51        /* 1 ms buckets, last = 50 ms and more */
#define LOG_PATH "sdmc:/3ds/uds_probe.txt"

static const char passphrase[] = "p3ds-wireless-probe-v1";

enum { MAGIC_PING = 0x50444E55u, MAGIC_REPLY = 0x52444E55u, MAGIC_END = 0x45444E55u };

typedef struct { u32 magic, seq; u64 t0; u32 hz, pad; u8 fill[HOST_SIZE - 24]; } Ping;
typedef struct { u32 magic, seq; u64 t0; u32 gapMaxUs, missed; u8 fill[CLIENT_SIZE - 24]; } Reply;

typedef struct {
	u32 replies, late; u64 rttSum; u32 rttMin, rttMax; u32 hist[HIST_BUCKETS]; u32 lastGapMaxUs, lastMissed;
} NodeStats;

static FILE *logf;
static int hz = 60;

static u64 nowUs(void) { return svcGetSystemTick() * 1000000ull / SYSCLOCK_ARM11; }

/* Print to the screen and append to the log file. */
static void say(const char *fmt, ...)
{
	char line[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(line, sizeof line, fmt, ap);
	va_end(ap);
	printf("%s\n", line);
	if (logf) { fprintf(logf, "%s\n", line); fflush(logf); }
}

static void openLog(const char *role)
{
	logf = fopen(LOG_PATH, "a");
	time_t t = time(NULL);
	bool n3ds = false;
	APT_CheckNew3DS(&n3ds);
	say("[UDS] ---- %s, %s, %d Hz, host %d B, client %d B, %s", role, n3ds ? "New 3DS" : "Old 3DS", hz, HOST_SIZE,
	    CLIENT_SIZE, ctime(&t));
}

static bool quitPressed(void)
{
	hidScanInput();
	return (hidKeysDown() & KEY_START) != 0;
}

static u32 pct(u32 hist[], u32 total, u32 p)
{
	u32 want = (u32)((u64)total * p / 100), acc = 0;
	for (u32 i = 0; i < HIST_BUCKETS; i++) {
		acc += hist[i];
		if (acc >= want && want) return i;
	}
	return HIST_BUCKETS - 1;
}

static void hostReport(NodeStats *ns, u32 sent)
{
	for (int n = 0; n < MAX_CLIENTS; n++) {
		NodeStats *s = &ns[n];
		if (!s->replies) continue;
		say("[UDS] client node %d: %lu/%lu replies (%lu lost), late (> 1 cycle) %lu", n + 2, s->replies, sent,
		    sent - s->replies, s->late);
		say("[UDS]   rtt us min %lu avg %llu max %lu, p50 %lu ms p95 %lu ms p99 %lu ms", s->rttMin,
		    s->rttSum / s->replies, s->rttMax, pct(s->hist, s->replies, 50), pct(s->hist, s->replies, 95),
		    pct(s->hist, s->replies, 99));
		say("[UDS]   client saw: worst gap %lu us, missed pings %lu", s->lastGapMaxUs, s->lastMissed);
	}
}

static void runHost(void)
{
	static udsBindContext bind;
	static NodeStats ns[MAX_CLIENTS];
	static Ping ping;
	static u8 rx[UDS_DATAFRAME_MAXSIZE];
	udsNetworkStruct net;
	udsConnectionStatus st;
	u8 appdata[112], channel = 0;
	Result r;

	openLog("HOST");
	udsGenerateDefaultNetworkStruct(&net, WLANCOMMID, 0, 1 + MAX_CLIENTS);
	r = udsCreateNetwork(&net, passphrase, sizeof passphrase, &bind, DATA_CHANNEL, UDS_DEFAULT_RECVBUFSIZE);
	if (R_FAILED(r)) { say("[UDS] udsCreateNetwork failed 0x%08lx", r); return; }
	/* Same size as a DS beacon's user game info, so M1 knows it fits. */
	for (int i = 0; i < (int)sizeof appdata; i++) appdata[i] = (u8)(i * 7 + 1);
	r = udsSetApplicationData(appdata, sizeof appdata);
	if (R_FAILED(r)) say("[UDS] udsSetApplicationData failed 0x%08lx", r);
	udsGetChannel(&channel);
	say("[UDS] hosting on channel %u. Start the other 3DS with X (join). START = stop", channel);

	/* Wait for the first client, then give others 3 s to join. */
	u64 firstJoin = 0;
	for (;;) {
		if (!aptMainLoop() || quitPressed()) goto out;
		if (R_SUCCEEDED(udsGetConnectionStatus(&st)) && st.total_nodes > 1) {
			if (!firstJoin) { firstJoin = nowUs(); say("[UDS] client joined (%u nodes), starting in 3 s", st.total_nodes); }
			else if (nowUs() - firstJoin > 3000000) break;
		}
		svcSleepThread(10000000);
	}
	say("[UDS] %u nodes, bitmask 0x%04x. Running %d s...", st.total_nodes, st.node_bitmask, CYCLES_PER_RUN_S);

	memset(ns, 0, sizeof ns);
	for (int n = 0; n < MAX_CLIENTS; n++) ns[n].rttMin = 0xFFFFFFFF;
	const u32 periodUs = 1000000 / hz, cycles = (u32)hz * CYCLES_PER_RUN_S;
	u32 sent = 0, sendErr = 0, oddPackets = 0;
	u64 next = nowUs();
	for (u32 seq = 0; seq < cycles; seq++) {
		if (!aptMainLoop() || quitPressed()) break;
		memset(&ping, 0, sizeof ping);
		ping.magic = MAGIC_PING; ping.seq = seq; ping.hz = hz; ping.t0 = nowUs();
		r = udsSendTo(UDS_BROADCAST_NETWORKNODEID, DATA_CHANNEL, UDS_SENDFLAG_Default, &ping, sizeof ping);
		if (UDS_CHECK_SENDTO_FATALERROR(r)) { say("[UDS] udsSendTo fatal 0x%08lx", r); break; }
		if (R_FAILED(r)) sendErr++; else sent++;
		next += periodUs;
		/* Collect replies until the next cycle is due (the DS would be idle there too). */
		do {
			size_t got = 0;
			u16 src = 0;
			while (R_SUCCEEDED(udsPullPacket(&bind, rx, sizeof rx, &got, &src)) && got) {
				const Reply *rp = (const Reply *)rx;
				int n = (int)src - 2;
				if (got < 24 || rp->magic != MAGIC_REPLY || n < 0 || n >= MAX_CLIENTS) { oddPackets++; got = 0; continue; }
				u32 rtt = (u32)(nowUs() - rp->t0);
				NodeStats *s = &ns[n];
				s->replies++; s->rttSum += rtt;
				if (rtt < s->rttMin) s->rttMin = rtt;
				if (rtt > s->rttMax) s->rttMax = rtt;
				if (rtt > periodUs) s->late++;
				s->hist[rtt / 1000 < HIST_BUCKETS ? rtt / 1000 : HIST_BUCKETS - 1]++;
				s->lastGapMaxUs = rp->gapMaxUs; s->lastMissed = rp->missed;
				got = 0;
			}
			svcSleepThread(200000);
		} while (nowUs() < next);
		if (seq % (u32)hz == 0) printf("\x1b[29;0H%lu s  sent %lu\x1b[K", seq / hz, sent);
	}
	printf("\n");
	for (int i = 0; i < 3; i++) {
		memset(&ping, 0, sizeof ping); ping.magic = MAGIC_END;
		udsSendTo(UDS_BROADCAST_NETWORKNODEID, DATA_CHANNEL, UDS_SENDFLAG_Default, &ping, sizeof ping);
		svcSleepThread(50000000);
	}
	say("[UDS] host done: sent %lu, send errors %lu, unexpected packets %lu", sent, sendErr, oddPackets);
	hostReport(ns, sent);
out:
	udsDestroyNetwork();
	udsUnbind(&bind);
}

static void runClient(void)
{
	static udsBindContext bind;
	static u8 scanbuf[0x4000] __attribute__((aligned(4)));
	static u8 rx[UDS_DATAFRAME_MAXSIZE];
	static Reply reply;
	udsNetworkScanInfo *nets = NULL;
	size_t total = 0;
	Result r;

	openLog("CLIENT");
	say("[UDS] scanning... START = stop");
	for (;;) {
		if (!aptMainLoop() || quitPressed()) return;
		total = 0;
		r = udsScanBeacons(scanbuf, sizeof scanbuf, &nets, &total, WLANCOMMID, 0, NULL, false);
		if (R_SUCCEEDED(r) && total) break;
		if (nets) { free(nets); nets = NULL; }
		svcSleepThread(100000000);
	}
	u8 appdata[200];
	size_t asz = 0;
	bool appOk = R_SUCCEEDED(udsGetNetworkStructApplicationData(&nets[0].network, appdata, sizeof appdata, &asz)) && asz == 112;
	for (size_t i = 0; appOk && i < asz; i++) appOk = appdata[i] == (u8)(i * 7 + 1);
	say("[UDS] found %u host(s), channel %u, app data %u bytes %s", (unsigned)total, nets[0].network.channel,
	    (unsigned)asz, appOk ? "ok" : "MISMATCH");

	for (int tries = 0;; tries++) {
		r = udsConnectNetwork(&nets[0].network, passphrase, sizeof passphrase, &bind, UDS_BROADCAST_NETWORKNODEID,
		                      UDSCONTYPE_Client, DATA_CHANNEL, UDS_DEFAULT_RECVBUFSIZE);
		if (R_SUCCEEDED(r)) break;
		if (tries == 9) { say("[UDS] udsConnectNetwork failed 0x%08lx", r); free(nets); return; }
		svcSleepThread(200000000);
	}
	free(nets);
	udsConnectionStatus st;
	udsGetConnectionStatus(&st);
	say("[UDS] connected as node %u. Waiting for the host to start...", st.cur_NetworkNodeID);

	u32 pings = 0, missed = 0, gapMax = 0, lastSeq = 0, gapsOver2 = 0, hostHz = 0;
	u64 lastRx = 0, gapSum = 0, idleSince = nowUs();
	bool ended = false;
	while (!ended) {
		if (!aptMainLoop() || quitPressed()) break;
		size_t got = 0;
		u16 src = 0;
		while (R_SUCCEEDED(udsPullPacket(&bind, rx, sizeof rx, &got, &src)) && got) {
			const Ping *p = (const Ping *)rx;
			got = 0;
			if (p->magic == MAGIC_END) { ended = true; break; }
			if (p->magic != MAGIC_PING) continue;
			u64 t = nowUs();
			if (pings) {
				u32 gap = (u32)(t - lastRx);
				gapSum += gap;
				if (gap > gapMax) gapMax = gap;
				if (hostHz && gap > 2000000 / hostHz) gapsOver2++;
				if (p->seq > lastSeq + 1) missed += p->seq - lastSeq - 1;
			}
			hostHz = p->hz; lastRx = t; lastSeq = p->seq; pings++; idleSince = t;
			memset(&reply, 0, sizeof reply);
			reply.magic = MAGIC_REPLY; reply.seq = p->seq; reply.t0 = p->t0; reply.gapMaxUs = gapMax; reply.missed = missed;
			r = udsSendTo(UDS_HOST_NETWORKNODEID, DATA_CHANNEL, UDS_SENDFLAG_Default, &reply, sizeof reply);
			if (UDS_CHECK_SENDTO_FATALERROR(r)) { say("[UDS] udsSendTo fatal 0x%08lx", r); ended = true; break; }
			if (pings % 60 == 1) printf("\x1b[29;0Hpings %lu  missed %lu\x1b[K", pings, missed);
		}
		if (pings && nowUs() - idleSince > 10000000) { say("[UDS] no packets for 10 s, host gone"); break; }
		svcSleepThread(200000);
	}
	printf("\n");
	say("[UDS] client done: pings %lu, missed %lu, host rate %lu Hz", pings, missed, hostHz);
	if (pings > 1)
		say("[UDS]   arrival gap us avg %llu max %lu, gaps > 2 cycles %lu", gapSum / (pings - 1), gapMax, gapsOver2);
	udsDisconnectNetwork();
	udsUnbind(&bind);
}

int main(void)
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	Result r = udsInit(0x3000, NULL);
	printf("uds_probe (docs/WIRELESS.md M0)\n");
	if (R_FAILED(r)) {
		printf("udsInit failed 0x%08lx. Is wireless on?\nSTART = quit\n", r);
		while (aptMainLoop() && !quitPressed()) gspWaitForVBlank();
		gfxExit();
		return 0;
	}
	bool ran = false;
	while (aptMainLoop()) {
		if (!ran) {
			printf("\x1b[3;0HA = host   X = join   START = quit\nUP/DOWN = rate: %d Hz \x1b[K\n", hz);
		}
		hidScanInput();
		u32 k = hidKeysDown();
		if (k & KEY_START) break;
		if (k & (KEY_UP | KEY_DOWN)) hz = hz == 60 ? 30 : 60;
		if (k & (KEY_A | KEY_X)) {
			consoleClear();
			if (k & KEY_A) runHost(); else runClient();
			if (logf) { fclose(logf); logf = NULL; }
			printf("\nResults appended to %s\nA/X = run again, START = quit\n", LOG_PATH);
			ran = true;
		}
		gspWaitForVBlank();
	}
	udsExit();
	gfxExit();
	return 0;
}

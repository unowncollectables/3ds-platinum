/* wireless M1 (docs/WIRELESS.md): the DS WM API for the game, replacing pspoke's services/wm_absent.c in the link.
 *
 * wireless=0 (default, Settings > Game > Local wireless): exactly pspoke's wm_absent.c behaviour, a radio with nobody
 * in range (the "absent" functions below are that file's code, GPL-3.0, IbrahimIrfan/pspoke). The Underground runs
 * solo, scans find nobody.
 * wireless=1: the same API on 3DS local wireless (UDS, uds_ctru.c), 3DS to 3DS only. A parent hosts a UDS network whose
 * beacon carries the DS game info (GGID, TGID, entry flag, user game info), so the game's own scan/GGID filter and
 * Union Room lists work unchanged. Children join, say hello with their child SSID (the game checks it), and get an aid
 * (UDS node - 1). Port data (WM_SetMPDataToPortEx -> port callbacks) travels as unicast UDS frames through a small
 * resend layer: tools/uds_probe showed 0.5-2.5% of broadcast frames lost but no unicast loss, and the DS's own MP
 * protocol never loses data, which CommSys relies on.
 * Every callback runs from PSPNativeWMPump (frame.c, game thread, between frames), like the DS FIFO interrupt.
 * The mode is chosen at WM_Initialize and kept until WM_End/WM_Finish. Nothing here touches the save; trades write it
 * through the game's own code. Log tag [WIRELESS]. */
#include <nitro.h>
#include <nitro/wm.h>
#include <stdio.h>
#include <string.h>
#include "uds_ctru.h"

extern int P3DS_CfgInt(const char *, int);
extern void PSPNativeMemLog(const char *, ...);

typedef union{WMCallback base;WMStartParentCallback parent;WMStartScanCallback scan;WMStartConnectCallback connect;WMStartMPCallback mp;WMMeasureChannelCallback measure;WMPortSendCallback send;WMPortRecvCallback recv;}WMResult;
typedef struct{WMCallbackFunc fn;unsigned long long due;WMResult r;}Pending;
static Pending queue[48];static int queued;
static WMStatus status;
static BOOL initialized,parent;
static WMCallbackFunc parentCallback;
static unsigned long long nextBeacon;
static u16 beaconMs=200,tgid;
static unsigned long long now(void){return P3U_NowUs();}
/* Time since x, never negative (x may be stamped after the caller read the clock). */
static unsigned long long since(unsigned long long t,unsigned long long x){return t>x?t-x:0;}
/* Queue a result for fn, delivered no sooner than ms (and at least one frame) from now. NULL fn: nothing to deliver. */
static WMResult*post(WMCallbackFunc fn,u16 apiid,unsigned ms){
 static WMResult unused;
 if(!fn){memset(&unused,0,sizeof unused);return &unused;}
 if(queued==(int)(sizeof queue/sizeof queue[0])){PSPNativeMemLog("[WIRELESS] WM queue full");memset(&unused,0,sizeof unused);return &unused;}
 Pending*p=&queue[queued++];memset(p,0,sizeof *p);p->fn=fn;p->due=now()+(unsigned long long)(ms?ms:1)*1000;
 p->r.base.apiid=apiid;p->r.base.errcode=WM_ERRCODE_SUCCESS;return &p->r;
}
static WMErrCode done(WMCallbackFunc fn,u16 apiid){post(fn,apiid,0);return WM_ERRCODE_OPERATING;}
static void drop(u16 apiid){int j=0;for(int i=0;i<queued;i++)if(queue[i].r.base.apiid!=apiid)queue[j++]=queue[i];queued=j;}
static void runQueue(void){
 unsigned long long t=now();
 for(int i=0;i<queued;){
  if(queue[i].due>t){i++;continue;}
  Pending p=queue[i];memmove(&queue[i],&queue[i+1],(size_t)(queued-i-1)*sizeof queue[0]);queued--;
  p.fn(&p.r);   /* may post more; those are due a frame later at the earliest */
 }
}

/* ---------------------------------------------------------------- local wireless (wireless=1) state */
#define MAX_AID 7
#define PEERS (MAX_AID + 1)        /* index = aid; 0 = parent */
#define MSG_MAX 512
#define TXQ 64
#define RESEND_US 50000
#define GIVEUP_US 5000000
#define PEER_TIMEOUT_US 6000000
#define PING_US 250000
enum { PK_DATA = 1, PK_ACK, PK_BYE, PK_PING };
enum { PORT_HELLO = 0xFF01, PORT_WELCOME = 0xFF02 }; /* internal, carried on the reliable stream */
#define PK_MAGIC 0x5033 /* 'P3' */
typedef struct { u16 magic; u8 type, flags; u16 seq, ack, port, len; u32 rsv; } PkHdr; /* 16 bytes */
typedef struct { u16 seq, port, len; u64 sentAt, firstAt; u8 data[MSG_MAX]; } TxMsg;
typedef struct {
	BOOL up, hello;            /* UDS link present / hello+welcome done (game-visible connection) */
	u16 txSeq, txAcked;        /* next seq to assign, oldest unacknowledged */
	u16 rxNext;                /* next seq expected */
	int txHead, txCount;       /* ring of unacknowledged messages */
	TxMsg tx[TXQ];
	u64 lastRx, lastTx;
	BOOL ackDue;
	u8 ssid[WM_SIZE_CHILD_SSID];
	u32 resent, rxDup;
} Peer;

static int udsMode;            /* 1 while this WM session runs on UDS */
static Peer peers[PEERS];
static int role;               /* 0 none, 1 parent, 2 child */
static u16 myAid;
static WMParentParam pp;
static u16 gameInfoBuf[WM_SIZE_USER_GAMEINFO / 2];
static u16 gameInfoLen;
static u8 attrEntry = 1;
static struct { WMCallbackFunc fn; void *arg; } ports[16];
static WMCallbackFunc connectCallback, mpCallback;
static int connecting;         /* child: 1 = waiting for UDS, 2 = waiting for welcome */
static u64 connectSince;
static u8 childSsid[WM_SIZE_CHILD_SSID];
static WMBssDesc *scanBuf;
static P3UNet nets[P3U_NETS_MAX];
static int netCount, scanNext;
static int parentUp;           /* parent: PARENT_START delivered */
static u32 stTx, stRx, stResent, stConnects;
static u64 stSince;

/* Beacon application data: what a DS beacon's WMGameInfo carries, so a child can rebuild it byte for byte. */
typedef struct { u32 magic; u32 ggid; u16 tgid; u8 attribute, userLen; u16 parentMaxSize, childMaxSize; u16 user[WM_SIZE_USER_GAMEINFO / 2]; } BeaconApp;
#define BEACON_MAGIC 0x31573350u /* 'P3W1' */
static void beaconApp(BeaconApp *b) {
	memset(b, 0, sizeof *b);
	b->magic = BEACON_MAGIC; b->ggid = pp.ggid; b->tgid = pp.tgid;
	b->attribute = (u8)((attrEntry && pp.entryFlag) ? WM_ATTR_FLAG_ENTRY : 0);
	b->userLen = (u8)gameInfoLen; b->parentMaxSize = pp.parentMaxSize; b->childMaxSize = pp.childMaxSize;
	memcpy(b->user, gameInfoBuf, gameInfoLen);
}
static void pushBeacon(void) { if (role == 1 && P3U_State() == P3U_HOST) { BeaconApp b; beaconApp(&b); P3U_SetAppData(&b, sizeof b); } }

static u16 connectedBitmap(void) {
	u16 m = 1u << myAid;
	for (int a = 0; a < PEERS; a++) if (a != myAid && peers[a].hello) m |= 1u << a;
	return m;
}

static void rawSend(int aid, u8 type, u16 seq, u16 port, const void *data, u16 len) {
	u8 buf[sizeof(PkHdr) + MSG_MAX];
	PkHdr *h = (PkHdr *)buf;
	Peer *p = &peers[aid];
	h->magic = PK_MAGIC; h->type = type; h->flags = 0; h->seq = seq; h->ack = p->rxNext; h->port = port; h->len = len; h->rsv = 0;
	if (len) memcpy(buf + sizeof *h, data, len);
	if (P3U_Send((u16)(aid + 1), buf, (int)(sizeof *h + len)) > 0) { stTx++; p->lastTx = now(); p->ackDue = FALSE; }
}

static BOOL enqueue(int aid, u16 port, const void *data, u16 len) {
	Peer *p = &peers[aid];
	if (!p->up || p->txCount == TXQ || len > MSG_MAX) return FALSE;
	TxMsg *m = &p->tx[(p->txHead + p->txCount) % TXQ];
	m->seq = p->txSeq++; m->port = port; m->len = len; memcpy(m->data, data, len);
	m->firstAt = m->sentAt = now();
	p->txCount++;
	rawSend(aid, PK_DATA, m->seq, port, data, len);
	return TRUE;
}

static void portEvent(u16 state, int fromAid, u16 port, u16 *data, u16 len) {
	for (int i = 0; i < 16; i++) {
		if (!ports[i].fn || (state == WM_STATECODE_PORT_RECV && i != port)) continue;
		WMPortRecvCallback c; memset(&c, 0, sizeof c);
		c.apiid = WM_APIID_PORT_RECV; c.errcode = WM_ERRCODE_SUCCESS; c.state = state; c.port = (u16)i;
		c.data = data; c.length = len; c.aid = (u16)fromAid; c.arg = ports[i].arg; c.myAid = myAid;
		c.connectedAidBitmap = connectedBitmap(); memcpy(c.ssid, peers[fromAid].ssid, sizeof c.ssid);
		c.maxSendDataSize = role == 1 ? pp.parentMaxSize : pp.childMaxSize; c.maxRecvDataSize = role == 1 ? pp.childMaxSize : pp.parentMaxSize;
		ports[i].fn(&c);
	}
}

/* A game-visible connection ends: tell the parent/connect callback and the ports, forget the peer. */
static void peerDown(int aid, const char *why) {
	Peer *p = &peers[aid];
	BOOL was = p->hello;
	if (p->up || was) PSPNativeMemLog("[WIRELESS] aid %d down (%s): resent %lu, dup %lu", aid, why, (unsigned long)p->resent, (unsigned long)p->rxDup);
	memset(p, 0, sizeof *p);
	if (!was) return;
	if (role == 1 && parentCallback) {
		WMResult *r = post(parentCallback, WM_APIID_START_PARENT, 0);
		r->parent.state = WM_STATECODE_DISCONNECTED; r->parent.aid = (u16)aid;
	} else if (role == 2 && connectCallback) {
		WMResult *r = post(connectCallback, WM_APIID_START_CONNECT, 0);
		r->connect.state = WM_STATECODE_DISCONNECTED; r->connect.aid = myAid;
	}
	portEvent(WM_STATECODE_DISCONNECTED, aid, 0, NULL, 0);
}

static void deliver(int aid, u16 port, u8 *data, u16 len) {
	Peer *p = &peers[aid];
	if (port == PORT_HELLO && role == 1) {             /* child -> parent: its child SSID */
		memcpy(p->ssid, data, len < sizeof p->ssid ? len : sizeof p->ssid);
		u16 a = (u16)aid;
		enqueue(aid, PORT_WELCOME, &a, sizeof a);
		p->hello = TRUE; stConnects++;
		PSPNativeMemLog("[WIRELESS] child aid %d joined (ssid type %u)", aid, p->ssid[0]);
		if (parentCallback) {
			WMResult *r = post(parentCallback, WM_APIID_START_PARENT, 0);
			r->parent.state = WM_STATECODE_CONNECTED; r->parent.aid = (u16)aid; memcpy(r->parent.ssid, p->ssid, sizeof p->ssid);
			r->parent.parentSize = pp.parentMaxSize; r->parent.childSize = pp.childMaxSize;
		}
		return;
	}
	if (port == PORT_WELCOME && role == 2) {           /* parent -> child: our aid */
		u16 a; memcpy(&a, data, sizeof a);
		/* Our link is to the parent (aid 0); peers[0] keeps it, myAid is the game's id for us. */
		myAid = a; p->hello = TRUE; connecting = 0; stConnects++;
		PSPNativeMemLog("[WIRELESS] joined parent as aid %u", a);
		if (connectCallback) {
			WMResult *r = post(connectCallback, WM_APIID_START_CONNECT, 0);
			r->connect.state = WM_STATECODE_CONNECTED; r->connect.aid = a;
			r->connect.parentSize = pp.parentMaxSize; r->connect.childSize = pp.childMaxSize;
		}
		return;
	}
	if (port < 16 && p->hello) { stRx++; portEvent(WM_STATECODE_PORT_RECV, aid, port, (u16 *)data, len); }
}

static void handlePacket(int aid, u8 *buf, int got) {
	PkHdr *h = (PkHdr *)buf;
	if (got < (int)sizeof *h || h->magic != PK_MAGIC || h->len > got - (int)sizeof *h) return;
	Peer *p = &peers[aid];
	if (!p->up) { memset(p, 0, sizeof *p); p->up = TRUE; }
	p->lastRx = now();
	/* Acknowledgement: everything before h->ack arrived. */
	while (p->txCount && (s16)(h->ack - p->tx[p->txHead].seq) > 0) { p->txHead = (p->txHead + 1) % TXQ; p->txCount--; }
	switch (h->type) {
	case PK_DATA:
		if (h->seq == p->rxNext) { p->rxNext++; deliver(aid, h->port, buf + sizeof *h, h->len); }
		else p->rxDup++;          /* old copy (resend) or ahead of a gap: drop, the sender resends in order */
		p->ackDue = TRUE;
		break;
	case PK_BYE:
		peerDown(aid, "bye");
		break;
	default:
		break;
	}
}

static void netPump(void) {
	static u8 buf[sizeof(PkHdr) + MSG_MAX + 64];
	unsigned short src;
	int got, budget = 64;
	if (!stSince) stSince = now();
	while (budget-- && (got = P3U_Pull(buf, sizeof buf, &src)) > 0) {
		int aid = (int)src - 1;
		if (aid < 0 || aid >= PEERS) continue;
		if (role == 2 && aid != 0) continue;          /* a child only talks to the parent */
		handlePacket(aid, buf, got);
	}
	u64 t = now();
	int st = P3U_State();
	/* Child: UDS link up -> say hello once. */
	if (role == 2 && connecting == 1 && st == P3U_CLIENT) {
		Peer *p = &peers[0];
		memset(p, 0, sizeof *p); p->up = TRUE; p->lastRx = t;
		enqueue(0, PORT_HELLO, childSsid, sizeof childSsid);
		connecting = 2;
	}
	if (role == 2 && connecting && (st == P3U_FAILED || since(t, connectSince) > 8000000)) {
		PSPNativeMemLog("[WIRELESS] connect %s", st == P3U_FAILED ? "failed" : "timed out");
		connecting = 0; P3U_Leave();
		if (connectCallback) post(connectCallback, WM_APIID_START_CONNECT, 0)->base.errcode = WM_ERRCODE_FAILED;
	}
	if (role == 2 && peers[0].hello && (st == P3U_LOST || st == P3U_IDLE)) peerDown(0, "host gone");
	/* Parent: a UDS node that left. */
	if (role == 1 && st == P3U_HOST) {
		u16 bm = P3U_Bitmask();
		for (int a = 1; a < PEERS; a++) if (peers[a].up && !(bm & (1u << a))) peerDown(a, "left");
	}
	for (int a = 0; a < PEERS; a++) {
		Peer *p = &peers[a];
		if (!p->up) continue;
		if (since(t, p->lastRx) > PEER_TIMEOUT_US) { peerDown(a, "timeout"); continue; }
		/* Resend everything unacknowledged, oldest first, when the oldest is overdue (go-back-N). */
		if (p->txCount) {
			TxMsg *m0 = &p->tx[p->txHead];
			if (since(t, m0->firstAt) > GIVEUP_US) { peerDown(a, "no ack"); if (role == 1) P3U_Eject((u16)(a + 1)); continue; }
			if (since(t, m0->sentAt) > RESEND_US)
				for (int i = 0; i < p->txCount; i++) {
					TxMsg *m = &p->tx[(p->txHead + i) % TXQ];
					rawSend(a, PK_DATA, m->seq, m->port, m->data, m->len); m->sentAt = t; p->resent++; stResent++;
				}
		}
		if (p->ackDue || since(t, p->lastTx) > PING_US) rawSend(a, p->ackDue ? PK_ACK : PK_PING, 0, 0, NULL, 0);
	}
	if (since(t, stSince) > 10000000 && (stTx || stRx)) {
		PSPNativeMemLog("[WIRELESS] 10 s: role %d aid %u peers %04x tx %lu rx %lu resent %lu", role, myAid, connectedBitmap(),
		                (unsigned long)stTx, (unsigned long)stRx, (unsigned long)stResent);
		stTx = stRx = stResent = 0; stSince = t;
	}
}

static void udsTeardown(const char *why) {
	for (int a = 0; a < PEERS; a++) if (peers[a].up) rawSend(a, PK_BYE, 0, 0, NULL, 0);
	memset(peers, 0, sizeof peers);
	role = 0; myAid = 0; connecting = 0; parentUp = 0; netCount = 0;
	P3U_Scan(0); P3U_Leave();
	PSPNativeMemLog("[WIRELESS] %s", why);
}

void PSPNativeWMPump(void){
 if(udsMode){
  netPump();
  /* Parent start completes when the UDS network is up. */
  if(role==1&&!parentUp&&P3U_State()==P3U_HOST&&parentCallback){parentUp=1;WMResult*r=post(parentCallback,WM_APIID_START_PARENT,0);r->parent.state=WM_STATECODE_PARENT_START;}
  if(role==1&&!parentUp&&P3U_State()==P3U_FAILED&&parentCallback){parentUp=1;post(parentCallback,WM_APIID_START_PARENT,0)->base.errcode=WM_ERRCODE_FAILED;}
 }
 runQueue();
 unsigned long long t=now();
 if(parent&&parentCallback&&t>=nextBeacon&&(!udsMode||parentUp)){
  WMStartParentCallback b;memset(&b,0,sizeof b);b.apiid=WM_APIID_START_PARENT;b.errcode=WM_ERRCODE_SUCCESS;b.state=WM_STATECODE_BEACON_SENT;
  nextBeacon=t+(unsigned long long)beaconMs*1000;parentCallback(&b);
 }
}
static void stop(void){parent=FALSE;parentCallback=NULL;drop(WM_APIID_START_SCAN);}

/* ---------------------------------------------------------------- WM API */
static void chooseMode(void){
 if(initialized)return;
 udsMode=0;
 if(P3DS_CfgInt("wireless",0)){
  if(P3U_Init()){udsMode=1;PSPNativeMemLog("[WIRELESS] WM on 3DS local wireless (wireless=1)");}
  else PSPNativeMemLog("[WIRELESS] wireless=1 but UDS is unavailable: radio with nobody in range");
 }
 if(!udsMode){static BOOL told;if(!told){told=TRUE;PSPNativeMemLog("[WIRELESS] WM stand-in: radio on, no other DS in range");}}
}
WMErrCode WM_Init(void*buf,u16 dma){(void)buf;(void)dma;chooseMode();initialized=TRUE;return WM_ERRCODE_SUCCESS;}
WMErrCode WM_Initialize(void*buf,WMCallbackFunc fn,u16 dma){
 (void)buf;(void)dma;chooseMode();initialized=TRUE;return done(fn,WM_APIID_INITIALIZE);
}
WMErrCode WM_InitializeForListening(void*buf,WMCallbackFunc fn,u16 dma,BOOL blink){(void)blink;return WM_Initialize(buf,fn,dma);}
static void finishUds(void){if(udsMode){udsTeardown("WM end");P3U_Exit();udsMode=0;}memset(ports,0,sizeof ports);connectCallback=mpCallback=NULL;}
WMErrCode WM_Finish(void){stop();queued=0;finishUds();initialized=FALSE;return WM_ERRCODE_SUCCESS;}
WMErrCode WM_Enable(WMCallbackFunc fn){return done(fn,WM_APIID_ENABLE);}
WMErrCode WM_Disable(WMCallbackFunc fn){return done(fn,WM_APIID_DISABLE);}
WMErrCode WM_PowerOn(WMCallbackFunc fn){return done(fn,WM_APIID_POWER_ON);}
WMErrCode WM_PowerOff(WMCallbackFunc fn){return done(fn,WM_APIID_POWER_OFF);}
WMErrCode WM_Reset(WMCallbackFunc fn){stop();if(udsMode)udsTeardown("WM reset");return done(fn,WM_APIID_RESET);}
WMErrCode WM_End(WMCallbackFunc fn){stop();finishUds();initialized=FALSE;return done(fn,WM_APIID_END);}
WMErrCode WM_SetIndCallback(WMCallbackFunc fn){(void)fn;return WM_ERRCODE_SUCCESS;}
WMErrCode WM_SetPortCallback(u16 port,WMCallbackFunc fn,void*arg){if(port<16){ports[port].fn=fn;ports[port].arg=arg;}return WM_ERRCODE_SUCCESS;}
WMErrCode WM_SetLifeTime(WMCallbackFunc fn,u16 table,u16 cam,u16 frame,u16 mp){(void)table;(void)cam;(void)frame;(void)mp;return done(fn,WM_APIID_SET_LIFETIME);}
WMErrCode WM_SetParentParameter(WMCallbackFunc fn,const WMParentParam*p){
 if(p&&p->beaconPeriod)beaconMs=p->beaconPeriod;
 if(p){pp=*p;gameInfoLen=p->userGameInfo&&p->userGameInfoLength<=WM_SIZE_USER_GAMEINFO?p->userGameInfoLength:0;if(gameInfoLen)memcpy(gameInfoBuf,p->userGameInfo,gameInfoLen);}
 return done(fn,WM_APIID_SET_P_PARAM);
}
WMErrCode WM_SetGameInfo(WMCallbackFunc fn,const u16*info,u16 size,u32 ggid,u16 id,u8 attr){
 /* Kept even before WM_StartParent (the game may set it first); the beacon follows while hosting. */
 if(udsMode){if(info&&size<=WM_SIZE_USER_GAMEINFO){memcpy(gameInfoBuf,info,size);gameInfoLen=size;}pp.ggid=ggid;pp.tgid=id;attrEntry=(attr&WM_ATTR_FLAG_ENTRY)!=0;pushBeacon();}
 return done(fn,WM_APIID_SET_GAMEINFO);
}
WMErrCode WM_SetEntry(WMCallbackFunc fn,BOOL enabled){if(udsMode&&role==1){attrEntry=enabled!=0;pushBeacon();}return done(fn,WM_APIID_SET_ENTRY);}
WMErrCode WM_SetBeaconIndication(WMCallbackFunc fn,u16 flag){(void)flag;return done(fn,WM_APIID_SET_BEACON_IND);}
WMErrCode WM_StartParent(WMCallbackFunc fn){
 if(udsMode){
  /* Parent start is reported from the pump once the UDS network is up (PARENT_START), then beacons as before. */
  udsTeardown("start parent");role=1;myAid=0;parentUp=0;
  BeaconApp b;beaconApp(&b);P3U_Host(&b,sizeof b,1+(pp.maxEntry&&pp.maxEntry<=MAX_AID?pp.maxEntry:MAX_AID));
  parent=TRUE;parentCallback=fn;nextBeacon=now()+(unsigned long long)beaconMs*1000;
  PSPNativeMemLog("[WIRELESS] parent: ggid %08lx tgid %u max %u, game info %u B",(unsigned long)pp.ggid,pp.tgid,pp.maxEntry,gameInfoLen);
  return WM_ERRCODE_OPERATING;
 }
 WMResult*r=post(fn,WM_APIID_START_PARENT,0);r->parent.state=WM_STATECODE_PARENT_START;
 parent=TRUE;parentCallback=fn;nextBeacon=now()+(unsigned long long)beaconMs*1000;return WM_ERRCODE_OPERATING;
}
WMErrCode WM_EndParent(WMCallbackFunc fn){stop();if(udsMode)udsTeardown("end parent");return done(fn,WM_APIID_END_PARENT);}
WMErrCode WM_StartMP(WMCallbackFunc fn,u16*recv,u16 recvSize,u16*send,u16 sendSize,u16 freq){
 (void)recvSize;(void)send;(void)sendSize;(void)freq;mpCallback=fn;
 WMResult*r=post(fn,WM_APIID_START_MP,0);r->mp.state=WM_STATECODE_MP_START;r->mp.recvBuf=(WMMpRecvBuf*)recv;return WM_ERRCODE_OPERATING;
}
WMErrCode WM_StartMPEx(WMCallbackFunc fn,u16*recv,u16 recvSize,u16*send,u16 sendSize,u16 freq,u16 retry,BOOL minPoll,BOOL single,BOOL fixFreq,BOOL ignoreFatal){
 (void)retry;(void)minPoll;(void)single;(void)fixFreq;(void)ignoreFatal;return WM_StartMP(fn,recv,recvSize,send,sendSize,freq);
}
WMErrCode WM_EndMP(WMCallbackFunc fn){mpCallback=NULL;return done(fn,WM_APIID_END_MP);}
/* wireless=0: nobody is connected, so the data reaches no one; report the send itself as done.
   wireless=1: queued on the resend layer for every connected destination; reported done a frame later (delivery is
   then guaranteed while the peer stays connected, which is what a DS MP send means to the game). */
WMErrCode WM_SetMPDataToPortEx(WMCallbackFunc fn,void*arg,const u16*data,u16 size,u16 dest,u16 port,u16 prio){
 (void)prio;u16 sent=0;
 if(udsMode&&role){
  for(int a=0;a<PEERS;a++){
   if(a==myAid||!peers[a].hello||!(dest&(1u<<a)))continue;
   if(!enqueue(a,port,data,size)){PSPNativeMemLog("[WIRELESS] send queue full to aid %d",a);return WM_ERRCODE_SEND_QUEUE_FULL;}
   sent|=1u<<a;
  }
 }
 WMResult*r=post(fn,WM_APIID_PORT_SEND,0);
 r->send.state=WM_STATECODE_PORT_SEND;r->send.port=port;r->send.destBitmap=dest;r->send.sentBitmap=sent;r->send.data=data;r->send.size=size;r->send.callback=fn;r->send.arg=arg;
 return WM_ERRCODE_OPERATING;
}
WMMpRecvData*WM_ReadMPData(const WMMpRecvHeader*header,u16 aid){(void)header;(void)aid;return NULL;}
static void fillScan(WMResult*r,const P3UNet*n,u16 channel){
 const BeaconApp*b=(const BeaconApp*)n->app;
 WMGameInfo gi;memset(&gi,0,sizeof gi);
 gi.magicNumber=WM_GAMEINFO_MAGIC_NUMBER;gi.ggid=b->ggid;gi.tgid=b->tgid;gi.userGameInfoLength=b->userLen;gi.attribute=b->attribute;
 gi.parentMaxSize=b->parentMaxSize;gi.childMaxSize=b->childMaxSize;memcpy(gi.userGameInfo,b->user,b->userLen<=WM_SIZE_USER_GAMEINFO?b->userLen:0);
 const u16 giLen=(u16)(offsetof(WMGameInfo,userGameInfo)+b->userLen);
 r->scan.state=WM_STATECODE_PARENT_FOUND;memcpy(r->scan.macAddress,n->mac,6);r->scan.channel=channel;r->scan.linkLevel=WM_LINK_LEVEL_3;
 r->scan.gameInfoLength=giLen;r->scan.gameInfo=gi;
 if(scanBuf){memset(scanBuf,0,sizeof *scanBuf);scanBuf->length=(u16)(sizeof *scanBuf/2);scanBuf->rssi=40;memcpy(scanBuf->bssid,n->mac,6);
  scanBuf->beaconPeriod=200;scanBuf->channel=channel;scanBuf->gameInfoLength=giLen;scanBuf->gameInfo=gi;}
}
WMErrCode WM_StartScan(WMCallbackFunc fn,const WMScanParam*param){
 const unsigned ms=param&&param->maxChannelTime?param->maxChannelTime:20;const u16 ch=param?param->channel:1;
 if(udsMode&&role!=1){
  if(role==0)role=2;
  scanBuf=param?param->scanBuf:NULL;P3U_Scan(1);
  /* One parent per scan call, round robin, like a DS hopping channels and finding one beacon at a time. */
  if(scanNext==0)netCount=P3U_ScanResults(nets,P3U_NETS_MAX);
  int found=-1;
  for(int k=0;k<netCount;k++){const P3UNet*n=&nets[(scanNext+k)%netCount];if(n->appLen>=(int)sizeof(u32)*2&&((const BeaconApp*)n->app)->magic==BEACON_MAGIC){found=(scanNext+k)%netCount;break;}}
  WMResult*r=post(fn,WM_APIID_START_SCAN,ms);
  if(found>=0){fillScan(r,&nets[found],ch);scanNext=found+1<netCount?found+1:0;}
  else{r->scan.state=WM_STATECODE_PARENT_NOT_FOUND;r->scan.channel=ch;scanNext=0;}
  return WM_ERRCODE_OPERATING;
 }
 WMResult*r=post(fn,WM_APIID_START_SCAN,ms);
 r->scan.state=WM_STATECODE_PARENT_NOT_FOUND;r->scan.channel=ch;return WM_ERRCODE_OPERATING;
}
WMErrCode WM_EndScan(WMCallbackFunc fn){drop(WM_APIID_START_SCAN);if(udsMode)P3U_Scan(0);return done(fn,WM_APIID_END_SCAN);}
WMErrCode WM_StartConnectEx(WMCallbackFunc fn,const WMBssDesc*bss,const u8*ssid,BOOL powerSave,u16 auth){
 (void)powerSave;(void)auth;
 if(udsMode&&bss){
  /* UDS join runs on the worker; hello/welcome follow in the pump, then CONNECTED with our aid. */
  memset(peers,0,sizeof peers);role=2;myAid=0;connectCallback=fn;connecting=1;connectSince=now();
  memset(childSsid,0,sizeof childSsid);if(ssid)memcpy(childSsid,ssid,sizeof childSsid);
  pp.parentMaxSize=bss->gameInfo.parentMaxSize;pp.childMaxSize=bss->gameInfo.childMaxSize;
  P3U_Scan(0);P3U_Connect(bss->bssid);
  PSPNativeMemLog("[WIRELESS] connecting to %02x:%02x:%02x:%02x:%02x:%02x ggid %08lx",bss->bssid[0],bss->bssid[1],bss->bssid[2],bss->bssid[3],bss->bssid[4],bss->bssid[5],(unsigned long)bss->gameInfo.ggid);
  return WM_ERRCODE_OPERATING;
 }
 /* Never reached with wireless=0 (scans find nobody); fail the way a vanished parent does. */
 (void)bss;(void)ssid;post(fn,WM_APIID_START_CONNECT,0)->base.errcode=WM_ERRCODE_FAILED;return WM_ERRCODE_OPERATING;
}
WMErrCode WM_Disconnect(WMCallbackFunc fn,u16 aid){
 if(udsMode){
  if(role==1&&aid>0&&aid<PEERS&&peers[aid].up){rawSend(aid,PK_BYE,0,0,NULL,0);BOOL was=peers[aid].hello;memset(&peers[aid],0,sizeof peers[aid]);P3U_Eject((u16)(aid+1));
   if(was&&parentCallback){WMResult*r=post(parentCallback,WM_APIID_START_PARENT,0);r->parent.state=WM_STATECODE_DISCONNECTED_FROM_MYSELF;r->parent.aid=aid;}
   PSPNativeMemLog("[WIRELESS] parent dropped aid %u",aid);}
  else if(role==2){if(connectCallback&&peers[0].hello){WMResult*r=post(connectCallback,WM_APIID_START_CONNECT,0);r->connect.state=WM_STATECODE_DISCONNECTED_FROM_MYSELF;}
   udsTeardown("child disconnect");}
 }
 return done(fn,WM_APIID_DISCONNECT);
}
WMErrCode WM_MeasureChannel(WMCallbackFunc fn,u16 cca,u16 ed,u16 channel,u16 ms){
 (void)cca;(void)ed;WMResult*r=post(fn,WM_APIID_MEASURE_CHANNEL,ms);r->measure.channel=channel;r->measure.ccaBusyRatio=0;return WM_ERRCODE_OPERATING;
}
/* Channels 1, 7 and 13, as a DS reports them; 0x8000 is the SDK's "not initialized". */
u16 WM_GetAllowedChannel(void){return initialized?0x1041:0x8000;}
WMLinkLevel WM_GetLinkLevel(void){return udsMode&&role&&connectedBitmap()!=(1u<<myAid)?WM_LINK_LEVEL_3:WM_LINK_LEVEL_0;}
u16 WM_GetDispersionScanPeriod(void){return 40;}
u16 WM_GetDispersionBeaconPeriod(void){return 200;}
u16 WM_GetNextTgid(void){return ++tgid;}
const WMStatus*WMi_GetStatusAddress(void){return &status;}
/* Wi-Fi (access points) only: online play is not ported. */
WMErrCode WM_StartScanEx(WMCallbackFunc fn,const WMScanExParam*param){(void)fn;(void)param;return WM_ERRCODE_FAILED;}
WMErrCode WM_StartDCF(WMCallbackFunc fn,WMDcfRecvBuf*recv,u16 size){(void)fn;(void)recv;(void)size;return WM_ERRCODE_FAILED;}
WMErrCode WM_SetWEPKeyEx(WMCallbackFunc fn,u16 mode,u16 id,const u8*key){(void)fn;(void)mode;(void)id;(void)key;return WM_ERRCODE_FAILED;}

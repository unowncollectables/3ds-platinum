/* online trading M4 (docs/WIRELESS.md Option B; GTS first): 3DS sockets for the game's own Wi-Fi Connection code.
 * pspoke already links cybervisi0n's libntrwifi (CPS TCP/IP) and libntrdwc (login, GTS); patch 0137 builds them with the
 * Linux socket backend (real BSD sockets plus a select thread). That backend needs three things from the platform:
 *   1. sockets: socInit with a 0x100000 byte aligned buffer (P3DS_NetInit, called from wm_uds.c when wireless=2),
 *   2. the local IP: getifaddrs() below (the 3DS's own Wi-Fi connection, so WPA2 works; the IP is read by connecting a UDP
 *      socket towards the DNS server and asking getsockname, which needs no extra libctru calls),
 *   3. the DNS server: the community Wi-Fi Connection DNS answers nas/gamestats2/gpcm with the revival servers' addresses,
 *      so CPSDnsIp (host-order IPv4, like CPSMyIp) is set to it. Default 178.62.43.212 (Poke Classic Network);
 *      config.txt key wifi_dns=<ipv4> overrides.
 * 4. the console identity: the game starts the connection only when DWC_CheckValidConsole() says the save's Wi-Fi profile
 *      belongs to this console. A real DS keeps the attested Wi-Fi ID in flash; the port has none, so (online mode only)
 *      the port adopts the user id already stored in the save (read only, nothing is written to the save), see
 *      __wrap_DWC_CheckValidConsole / __wrap_DWC_Auth_GetId (link wraps, platinum3ds/full/Makefile).
 * 5. the cheat gate: online mode is refused while any cheat_* option is on (no sockets, no identity, no traffic).
 * Nothing here touches the save. Log tag [NET]; every step is flushed to the SD card at once. */
#include <stdarg.h>
#include <3ds.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern void PSPNativeMemLog(const char *, ...);
extern void P3DS_LogFlushSync(void);
extern int P3DS_CfgStr(const char *, char *, int);
extern int P3DS_CfgInt(const char *, int);
extern int P3DS_Cheat(int);        /* render3ds.cpp: cheat_money .. cheat_candy, k = 0..5 */
extern int P3DS_CheatNoEncCfg(void);
extern unsigned long CPSDnsIp[2]; /* libntrwifi cps; u32 there, 4 bytes on this ABI */
#define NLOG(...) do { PSPNativeMemLog(__VA_ARGS__); P3DS_LogFlushSync(); } while (0)

#define DEFAULT_DNS "178.62.43.212"
#define SOC_BUF_SIZE 0x100000

static int sState; /* 0 untried, 1 sockets up, -1 failed */
static void *sSocBuf;

static void netExit(void)
{
    if (sState == 1) socExit();
    free(sSocBuf);
    sSocBuf = NULL;
    sState = 0;
}

/* nonzero when a cheat is on; the first refusal is logged once per run */
int P3DS_OnlineCheatBlocked(void)
{
    static int told;
    int on = P3DS_CheatNoEncCfg() || P3DS_CfgInt("cheat_time", 0) != 0;
    for (int k = 0; k < 6; k++) on |= P3DS_Cheat(k) != 0;
    if (on && !told) {
        told = 1;
        NLOG("[NET] online mode refused: a cheat_* option is on (cheat_money/catch/shiny/eggs/exp/candy, cheat_no_encounters, cheat_time). No sockets, no identity, no traffic");
    }
    return on;
}

int P3DS_NetInit(void)
{
    if (sState) return sState > 0;
    if (P3DS_OnlineCheatBlocked()) return 0; /* sState stays 0: re-checked next call, so turning the cheat off works after a restart of the game */
    sState = -1;
    u32 wifi = 0;
    Result rc = acInit();
    if (R_SUCCEEDED(rc)) {
        rc = ACU_GetWifiStatus(&wifi);
        acExit();
    }
    NLOG("[NET] Wi-Fi status rc=0x%08lx status=%lu", (unsigned long)rc, (unsigned long)wifi);
    if (R_FAILED(rc) || wifi == 0) {
        NLOG("[NET] no Wi-Fi connection: online mode stays off");
        return 0;
    }
    sSocBuf = memalign(0x1000, SOC_BUF_SIZE);
    rc = sSocBuf ? socInit(sSocBuf, SOC_BUF_SIZE) : -1;
    NLOG("[NET] socInit rc=0x%08lx", (unsigned long)rc);
    if (R_FAILED(rc)) {
        free(sSocBuf);
        sSocBuf = NULL;
        return 0;
    }
    sState = 1;
    atexit(netExit);
    return 1;
}

/* struct layout must match toolchain-3ds/pspshim/ifaddrs.h (network.c is compiled with that header) */
struct ifaddrs {
    struct ifaddrs *ifa_next;
    char *ifa_name;
    unsigned int ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
};

static in_addr_t gDns; /* network order */

static void setDns(void)
{
    char dns[64];
    if (!P3DS_CfgStr("wifi_dns", dns, sizeof dns) || !dns[0]) strcpy(dns, DEFAULT_DNS);
    in_addr_t a = inet_addr(dns);
    if (a == (in_addr_t)-1) {
        NLOG("[NET] wifi_dns=%s is not an IPv4 address, using %s", dns, DEFAULT_DNS);
        a = inet_addr(DEFAULT_DNS);
    }
    gDns = a;
    CPSDnsIp[0] = CPSDnsIp[1] = ntohl(a);
    NLOG("[NET] DNS server %s", dns);
}

int getifaddrs(struct ifaddrs **ifap)
{
    static struct ifaddrs ifa;
    static struct sockaddr_in sa;
    static char name[] = "wlan0";
    *ifap = NULL;
    if (!P3DS_NetInit()) return 0;
    setDns();
    in_addr_t ip = 0;
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd >= 0) {
        struct sockaddr_in to, me;
        socklen_t len = sizeof me;
        memset(&to, 0, sizeof to);
        to.sin_family = AF_INET;
        to.sin_port = htons(53);
        to.sin_addr.s_addr = gDns;
        if (connect(fd, (struct sockaddr *)&to, sizeof to) == 0 && getsockname(fd, (struct sockaddr *)&me, &len) == 0) ip = me.sin_addr.s_addr;
        close(fd);
    }
    NLOG("[NET] local IP %u.%u.%u.%u", (unsigned)(ip & 255), (unsigned)((ip >> 8) & 255), (unsigned)((ip >> 16) & 255), (unsigned)(ip >> 24));
    if (!ip) return 0;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = ip;
    ifa.ifa_next = NULL;
    ifa.ifa_name = name;
    ifa.ifa_addr = (struct sockaddr *)&sa;
    *ifap = &ifa;
    return 0;
}

void freeifaddrs(struct ifaddrs *list) { (void)list; }

/* ---- console identity (link wraps) ---------------------------------------------------------------------------------
 * DWCAccUserData (libntrdwc include/base/dwc_account.h): int size; DWCAccLoginId pseudo; DWCAccLoginId authentic; int gs_profile_id;
 * each DWCAccLoginId is 12 bytes, so `authentic` is at offset 16. DWCAuthWiFiId is { u64 uId; u64 notAttestedId; u32 flg; }. */
extern unsigned long long DWCi_Acc_GetUserId(const void *loginId);
extern int __real_DWC_CheckHasProfile(const void *userData);
static const void *sGameUd; /* the save's DWCUserData, seen by the game's own CheckHasProfile call (run 86 login driver) */
extern int __real_DWC_CheckValidConsole(const void *userData);
extern void __real_DWC_Auth_GetId(void *id);
struct AuthWiFiId { unsigned long long uId; unsigned long long notAttestedId; unsigned int flg; };
static unsigned long long sAdoptedUid;
static int sAdopted;

static int onlineMode(void) { return P3DS_CfgInt("wireless", 0) == 2; }

int __wrap_DWC_CheckValidConsole(const void *userData)
{
    static int told;
    if (userData && onlineMode() && !P3DS_OnlineCheatBlocked() && P3DS_NetInit() && __real_DWC_CheckHasProfile(userData)) {
        unsigned long long uid = DWCi_Acc_GetUserId((const char *)userData + 16);
        if (!sAdopted || sAdoptedUid != uid) {
            sAdopted = 1;
            sAdoptedUid = uid;
            NLOG("[NET] adopting the save's console id %08lx%08lx (the save has a Wi-Fi profile): the connection task may run", (unsigned long)(uid >> 32), (unsigned long)uid);
        }
        return 1;
    }
    if (!told) {
        told = 1;
        NLOG("[NET] console check: real answer (online %s)", onlineMode() ? "on, but not usable (cheat, no sockets or no profile)" : "off");
    }
    return __real_DWC_CheckValidConsole(userData);
}

void __wrap_DWC_Auth_GetId(void *id)
{
    __real_DWC_Auth_GetId(id);
    if (sAdopted && onlineMode()) {
        struct AuthWiFiId *a = id;
        a->uId = sAdoptedUid;
        a->flg = 1;
    }
}

/* The save needs a GameSpy profile (gs_profile_id > 0) before the game runs its connection task; otherwise it launches the
 * Wi-Fi setup app (untranslated) instead. A real DS creates the profile on its first login. Online mode lets the task run
 * without one; the DWC login then creates the account (the profile lands in the save only if that succeeds). */
int __wrap_DWC_CheckHasProfile(const void *userData)
{
    static int told;
    int real = __real_DWC_CheckHasProfile(userData);
    if (userData) sGameUd = userData;
    if (!told) {
        told = 1;
        NLOG("[NET] save has a Wi-Fi profile: %s%s", real ? "yes" : "no", (!real && onlineMode() && !P3DS_OnlineCheatBlocked()) ? " (online mode: connection task allowed anyway, the login will create one)" : "");
    }
    if (!real && onlineMode() && !P3DS_OnlineCheatBlocked() && P3DS_NetInit()) return 1;
    return real;
}

/* ---- run 83: the game's ghttp (GameSpy HTTP) for the GTS login --------------------------------------------------------------
 * After the netcheck the GTS fetches http://gamestats2.gs.nintendowifi.net/pokemondpds/worldexchange/info.asp?pid=..  (a 32 byte
 * challenge) and then a second URL with a SHA1 of it (a 22 byte answer). The game starts a request with ov00_021FA21C(url, cb, ud)
 * and polls ov00_021FA118() every frame; cb(buf, len, result, ud) is called from that poll (result 0 = ok).
 * Here: a plain HTTP/1.0 GET over the same soc sockets, run in a worker thread (the game thread never blocks); the name is resolved
 * by asking the community DNS (wifi_dns, default as above) directly over UDP, because the system DNS knows nothing of these hosts.
 * Online mode only, never with a cheat on. Log tag [GHTTP]. Plain GET only, nothing is POSTed, nothing touches the save. */
#include <poll.h>
#include <errno.h>
#include <fcntl.h>

static in_addr_t ghDnsAddr(void)
{
    if (!gDns) setDns();
    return gDns;
}

static int ghResolve(const char *host, in_addr_t *out)
{
    unsigned char q[300], r[600];
    int n = 0, fd;
    memset(q, 0, 12); q[0] = 0x53; q[1] = 0x31; q[2] = 1; q[5] = 1; n = 12; /* id, RD, 1 question */
    for (const char *p = host; *p;) {
        const char *d = strchr(p, '.'); int l = d ? (int)(d - p) : (int)strlen(p);
        if (l <= 0 || l > 63 || n + l + 6 > (int)sizeof q) return -1;
        q[n++] = (unsigned char)l; memcpy(q + n, p, l); n += l; p += l; if (d) p++;
    }
    q[n++] = 0; q[n++] = 0; q[n++] = 1; q[n++] = 0; q[n++] = 1; /* type A, class IN */
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd < 0) return -2;
    struct sockaddr_in to; memset(&to, 0, sizeof to); to.sin_family = AF_INET; to.sin_port = htons(53); to.sin_addr.s_addr = ghDnsAddr();
    int got = -3;
    for (int tries = 0; tries < 3 && got < 0; tries++) {
        if (sendto(fd, q, n, 0, (struct sockaddr *)&to, sizeof to) < 0) { got = -4; break; }
        struct pollfd pf = { fd, POLLIN, 0 };
        if (poll(&pf, 1, 2000) > 0) { int m = recvfrom(fd, r, sizeof r, 0, NULL, NULL); if (m > 12) got = m; }
    }
    close(fd);
    if (got < 12) return -5;
    int an = (r[6] << 8) | r[7], pos = 12;
    while (pos < got && r[pos]) pos += (r[pos] >= 0xC0) ? 2 : r[pos] + 1; /* question name */
    pos += 5;
    for (int i = 0; i < an && pos + 12 <= got; i++) {
        while (pos < got && r[pos] && !(r[pos] >= 0xC0)) pos += r[pos] + 1;
        pos += (pos < got && r[pos] >= 0xC0) ? 2 : 1;
        if (pos + 10 > got) break;
        int type = (r[pos] << 8) | r[pos + 1], rl = (r[pos + 8] << 8) | r[pos + 9]; pos += 10;
        if (type == 1 && rl == 4 && pos + 4 <= got) { memcpy(out, r + pos, 4); return 0; }
        pos += rl;
    }
    return -6;
}

#define GH_MAX 16384 /* run 100: was 1024, which cut the GTS search listing short */
static struct { volatile int state; /* 0 idle, 1 running, 2 done */ char url[2048]; char body[GH_MAX]; int len, result; void *cb, *ud; Thread th; } gh;

static void ghWork(void *arg)
{
    (void)arg;
    char host[128], path[1990]; const char *u = gh.url; int result = 5, len = 0;
    if (strncmp(u, "http://", 7) == 0) u += 7;
    const char *sl = strchr(u, '/');
    int hl = sl ? (int)(sl - u) : (int)strlen(u);
    if (hl > 0 && hl < (int)sizeof host) {
        memcpy(host, u, hl); host[hl] = 0; snprintf(path, sizeof path, "%s", sl ? sl : "/");
        in_addr_t ip = 0; int rr = ghResolve(host, &ip);
        NLOG("[GHTTP] %s -> %s (%s)", host, rr == 0 ? inet_ntoa(*(struct in_addr *)&ip) : "lookup failed", rr == 0 ? "ok" : "err");
        if (rr == 0) {
            int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            struct sockaddr_in sa; memset(&sa, 0, sizeof sa); sa.sin_family = AF_INET; sa.sin_port = htons(80); sa.sin_addr.s_addr = ip;
            if (fd >= 0 && connect(fd, (struct sockaddr *)&sa, sizeof sa) == 0) {
                char req[2300]; int rl = snprintf(req, sizeof req, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: GameSpyHTTP/1.0\r\nConnection: close\r\n\r\n", path, host);
                static char resp[GH_MAX + 2048]; int tot = 0; /* run 100: static, the worker stack is 32 KiB */
                if (send(fd, req, rl, 0) == rl) {
                    for (;;) {
                        struct pollfd pf = { fd, POLLIN, 0 };
                        int pr = poll(&pf, 1, 20000);
                        if (pr <= 0) { NLOG("[GHTTP] no reply within 20 s (poll %d, errno %d), %d bytes so far", pr, errno, tot); break; }
                        int m = recv(fd, resp + tot, (int)sizeof resp - 1 - tot, 0);
                        if (m < 0) NLOG("[GHTTP] recv error errno %d after %d bytes", errno, tot);
                        if (m <= 0) { if (m == 0 && tot == 0) NLOG("[GHTTP] server closed the connection without a reply"); break; }
                        tot += m; if (tot >= (int)sizeof resp - 1) break;
                    }
                }
                resp[tot] = 0;
                NLOG("[GHTTP] got %d bytes, status line: %.40s", tot, tot > 0 ? resp : "(none)");
                char *body = strstr(resp, "\r\n\r\n");
                if (tot > 12 && strncmp(resp, "HTTP/", 5) == 0 && strstr(resp, " 200") && strstr(resp, " 200") < resp + 16 && body) {
                    body += 4; len = tot - (int)(body - resp); if (len > GH_MAX - 1) len = GH_MAX - 1;
                    memcpy(gh.body, body, len); gh.body[len] = 0; result = 0;
                    NLOG("[GHTTP] ok, body %d bytes", len);
                }
            } else NLOG("[GHTTP] connect failed (errno %d)", errno);
            if (fd >= 0) close(fd);
        }
    }
    gh.len = len; gh.result = result; gh.state = 2;
}

/* returns a request id (>= 0) or -5 when it cannot start */
int P3DS_GhttpStart(const char *url, void *cb, void *ud)
{
    if (!url || !onlineMode() || P3DS_OnlineCheatBlocked() || !P3DS_NetInit()) { NLOG("[GHTTP] not started (online mode off, cheat on, or sockets down): %s", url ? url : "(null)"); return -5; }
    if (gh.state != 0) { NLOG("[GHTTP] busy, refused: %s", url); return -5; }
    snprintf(gh.url, sizeof gh.url, "%s", url); gh.cb = cb; gh.ud = ud; gh.len = 0; gh.result = 5; gh.state = 1;
    NLOG("[GHTTP] GET %s", gh.url);
    gh.th = threadCreate(ghWork, NULL, 0x8000, 0x30, -2, false);
    if (!gh.th) { gh.state = 0; NLOG("[GHTTP] thread create failed"); return -5; }
    return 1;
}

/* per-frame: finish a request on the game thread (the callback is game code) */
int P3DS_GhttpThink(void)
{
    if (gh.state == 2) {
        int (*cb)(char *, int, int, void *) = (int (*)(char *, int, int, void *))gh.cb;
        void *ud = gh.ud; int len = gh.len, res = gh.result;
        threadJoin(gh.th, 2000000000ULL); threadFree(gh.th);
        gh.th = NULL; gh.state = 0; /* idle first: the callback starts the next request */
        NLOG("[GHTTP] callback result %d len %d", res, len);
        if (cb) cb(res == 0 ? gh.body : NULL, len, res, ud);
    }
    return 1;
}

/* ---- run 86: the real DWC account login (NAS web step + GameSpy GP) so the GTS gets a real profile id instead of -1 ---------------
 * The game only learns its profile id from its own login, whose overlay-0 pieces are unfinished stubs. Here libntrdwc's own C login is
 * driven directly on the SAVE's DWCUserData (seen via the game's DWC_CheckHasProfile call): SetMemFunc, DWC_Init, InitFriendsMatch with
 * HGSS's constants (productID 0x29E7, "pokemondpds", secret "1vTlwb", gamecode 0x4144414A), DWC_LoginAsync, then DWC_ProcessFriendsMatch
 * once per frame. The game's netcheck poll (ov00_021ECDC8, polled every frame right before it reads the profile id) is held at "running"
 * until the login finishes or fails (30 s), so the game's next step (state 0x10) sees the real gs_profile_id written by the library.
 * The NAS step uses plain http://nas.nintendowifi.net/ac (libntrdwc skips the cert check for a custom NAS URL). Online mode only, no cheats.
 * The login writes the profile into the save's userdata (that is how a real DS stores it); this is a test save, back it up first.
 * Log tag [LOGIN]. */
extern void DWC_SetMemFunc(void *(*)(int, unsigned, int), void (*)(int, void *, unsigned));
extern void DWC_Init(void *work);
extern int DWC_CheckUserData(const void *ud);
extern void DWC_CreateUserData(void *ud, unsigned gamecode);
extern int DWC_InitFriendsMatch(void *ctl, void *ud, int productID, const char *gameName, const char *secretKey, int sendBufSize, int recvBufSize, void *friendList, int friendListLen);
extern int DWC_LoginAsync(const unsigned short *name, const char *reserved, void (*cb)(int, int, void *), void *param);
extern void DWC_ProcessFriendsMatch(void);
extern int DWC_GetState(void);
extern int DWC_GetLastError(int *code);
extern void DWC_Auth_SetCustomNas(const char *url);
extern int DWC_GetInetStatus(void);

static unsigned char sLoginWork[16384] __attribute__((aligned(32)));
static unsigned char sLoginCtl[65536] __attribute__((aligned(32)));
static unsigned char sLoginFriends[2048] __attribute__((aligned(32)));
static volatile int sLoginDone; /* 0 not started, 1 running, 2 ok, 3 failed */
static volatile int sLoginPid, sLoginErr;
static unsigned sLoginFrames;

static void loginCb(int err, int pid, void *param)
{
    (void)param;
    sLoginErr = err; sLoginPid = pid;
    NLOG("[LOGIN] callback: error %d, profile id %d", err, pid);
    if (err == 0 && pid > 0) sLoginDone = 2; else sLoginDone = 3;
}

/* netcheck start (ov00_021ECD04) */
void P3DS_LoginStart(void)
{
    if (sLoginDone != 0) return;
    if (!sGameUd || !onlineMode() || P3DS_OnlineCheatBlocked() || DWC_GetInetStatus() != 4) {
        NLOG("[LOGIN] not started (userdata %p, online %d, inet %d)", sGameUd, onlineMode(), DWC_GetInetStatus());
        sLoginDone = 3; return;
    }
    void *ud = (void *)sGameUd;
    int okud = DWC_CheckUserData(ud);
    unsigned gid = *(const unsigned *)((const unsigned char *)ud + 0x1c);
    NLOG("[LOGIN] starting on the save's userdata %p (valid %d, gs profile id %u)", ud, okud, gid);
    if (!okud) { DWC_CreateUserData(ud, 0x4144414Au); NLOG("[LOGIN] userdata was invalid: created a fresh one"); }
    DWC_Auth_SetCustomNas("http://nas.nintendowifi.net/ac");
    DWC_Init(sLoginWork);
    int r = DWC_InitFriendsMatch(sLoginCtl, ud, 0x29E7, "pokemondpds", "1vTlwb", 0, 0, sLoginFriends, 0x20);
    NLOG("[LOGIN] InitFriendsMatch -> %d", r);
    static const unsigned short nm[] = { 'T', 'e', 's', 't', 0 };
    r = DWC_LoginAsync(nm, NULL, loginCb, NULL);
    NLOG("[LOGIN] LoginAsync -> %d (state %d)", r, DWC_GetState());
    sLoginDone = r ? 1 : 3;
}

/* netcheck poll (ov00_021ECDC8): returns the game's netcheck result: 1 running, 3 ok (also after a failed login: the game then goes on without a profile id, as in run 85) */
int P3DS_LoginPoll(void)
{
    static int lastState = -99;
    if (sLoginDone == 0) P3DS_LoginStart();
    if (sLoginDone == 1) {
        DWC_ProcessFriendsMatch();
        int st = DWC_GetState();
        if (st != lastState) { int code = 0; int e = DWC_GetLastError(&code); NLOG("[LOGIN] state %d (last error %d code %d)", st, e, code); lastState = st; }
        if (++sLoginFrames > 30 * 30) { NLOG("[LOGIN] gave up after 30 s"); sLoginDone = 3; }
    }
    if (sLoginDone == 1) return 1;
    if (sLoginDone == 2) { static int told; if (!told) { told = 1; NLOG("[LOGIN] done, profile id %d is now in the save's userdata", sLoginPid); } return 3; }
    { static int told; if (!told) { told = 1; NLOG("[LOGIN] failed or skipped: continuing without a profile id (the GTS will use -1 as before)"); } }
    return 3;
}

/* ---- run 88: host name lookups for the library's NAS login and the GameSpy GP connection ----------------------------------------
 * Run 87 crashed in SOCL_Resolve (SOC stack never started on the 3DS). The NAS HTTP step (NHTTPi_resolveHostname -> SOCL_Resolve) and
 * GameSpy GP (gethostbyname("gpcm.gs.nintendowifi.net")) both need names that only the community DNS knows, so all three go through the
 * same direct UDP query to wifi_dns used for ghttp (ghResolve). Strong definitions here win over the library's (platform objects link
 * first, and the link allows duplicates). Online mode only. */
typedef struct { char *h_name; char **h_aliases; int h_addrtype; int h_length; char **h_addr_list; } P3DSHostent; /* same layout as struct hostent; not including <netdb.h> */

/* returns the IPv4 address in HOST order, 0 when it cannot be resolved (libntrwifi/CPS convention) */
unsigned int SOCL_Resolve(const char *host)
{
    in_addr_t a = 0;
    if (!host || !host[0]) return 0;
    a = inet_addr(host);
    if (a == (in_addr_t)-1) {
        if (!onlineMode() || P3DS_OnlineCheatBlocked() || !P3DS_NetInit() || ghResolve(host, &a) != 0) { NLOG("[DNS] %s: not resolved", host); return 0; }
        NLOG("[DNS] %s -> %s", host, inet_ntoa(*(struct in_addr *)&a));
    }
    return ntohl(a);
}

typedef struct { char *name; char **aliases; short addrType; short length; unsigned char **addrList; } P3DSSocHostEnt;
P3DSSocHostEnt *SOC_GetHostByName(const char *name)
{
    static P3DSSocHostEnt he; static char hn[128]; static char *al[1]; static unsigned char ip[4]; static unsigned char *list[2];
    unsigned int h = SOCL_Resolve(name);
    if (!h) return NULL;
    snprintf(hn, sizeof hn, "%s", name);
    ip[0] = h >> 24; ip[1] = h >> 16; ip[2] = h >> 8; ip[3] = h; list[0] = ip; list[1] = NULL; al[0] = NULL;
    he.name = hn; he.aliases = al; he.addrType = 2; he.length = 4; he.addrList = list;
    return &he;
}

P3DSHostent *gethostbyname(const char *name)
{
    static P3DSHostent he; static char hn[128]; static char *al[1]; static unsigned char ip[4]; static char *list[2];
    unsigned int h = SOCL_Resolve(name);
    if (!h) return NULL;
    snprintf(hn, sizeof hn, "%s", name);
    ip[0] = h >> 24; ip[1] = h >> 16; ip[2] = h >> 8; ip[3] = h; list[0] = (char *)ip; list[1] = NULL; al[0] = NULL;
    he.h_name = hn; he.h_aliases = al; he.h_addrtype = AF_INET; he.h_length = 4; he.h_addr_list = list;
    return &he;
}

/* ---- run 89: the library's DS socket API (SOC_*) on the 3DS's own BSD sockets -------------------------------------------------
 * GameSpy's availability check and GP connection (nonport.c) and the NAS HTTP step call SOC_Socket/Connect/Send/... which sit on the DS
 * SOC/CPS stack that never starts here (run 88 crashed in SOCLi_StartupSocket, SOCLiConfigPtr NULL). These strong definitions replace
 * them one for one with libctru sockets (soc:u is already up). Conventions of the DS API: >= 0 on success, NEGATIVE SOC_E* code on error
 * (not errno); sockaddrs are SOCSockAddrIn {u8 len; u8 family; u16 port; u32 addr} with network order port/addr (not the BSD layout);
 * SOC_Poll/Fcntl use the DS flag values. Log tag [SOC] (first calls and every failure of connect only). */
typedef struct { unsigned char len, family; unsigned short port; unsigned int addr; } P3DSSockAddrIn;
typedef struct { int fd; short events; short revents; } P3DSPollFD;

static int socErr(void)
{
    switch (errno) {
    case EAGAIN: return -6;
    case EINPROGRESS: return -26;
    case EALREADY: return -7;
    case ECONNREFUSED: return -14;
    case ECONNRESET: return -15;
    case ECONNABORTED: return -13;
    case ETIMEDOUT: return -76;
    case EINTR: return -27;
    case EISCONN: return -30;
    case ENOTCONN: return -56;
    case EBADF: return -8;
    case EHOSTUNREACH: return -23;
    case ENETUNREACH: return -40;
    case EADDRINUSE: return -3;
    case EINVAL: return -28;
    case EPIPE: return -66;
    case ENOMEM: return -49;
    case ENOBUFS: return -42;
    default: return -29;
    }
}

static void toBsd(const P3DSSockAddrIn *a, struct sockaddr_in *b)
{
    memset(b, 0, sizeof *b);
    b->sin_family = AF_INET; b->sin_port = a->port; b->sin_addr.s_addr = a->addr;
}
static void fromBsd(const struct sockaddr_in *b, P3DSSockAddrIn *a)
{
    a->len = 8; a->family = 2; a->port = b->sin_port; a->addr = b->sin_addr.s_addr;
}

static void soctr(const char *fmt, ...)
{
    static int n; char b[160]; va_list ap;
    if (n >= 60) return;
    n++;
    va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    NLOG("[SOCT] %s", b);
    { extern void P3DS_LogRequestSync(void); P3DS_LogRequestSync(); }
    svcSleepThread(150000000LL); /* let the log writer commit it: run 89 died without a crash report, so the last call matters */
}

int SOC_Socket(int pf, int type, int protocol)
{
    (void)pf; (void)protocol;
    if (!onlineMode() || P3DS_OnlineCheatBlocked() || !P3DS_NetInit()) { NLOG("[SOC] socket refused (online mode off, cheat on, or sockets down)"); return -38; }
    int fd = socket(AF_INET, type == 2 ? SOCK_DGRAM : SOCK_STREAM, 0);
    static int n; if (n++ < 6) NLOG("[SOC] socket(%s) -> %d", type == 2 ? "udp" : "tcp", fd);
    return fd < 0 ? socErr() : fd;
}
int SOC_Bind(int s, const void *sa) { soctr("bind %d", s); struct sockaddr_in b; toBsd(sa, &b); return bind(s, (struct sockaddr *)&b, sizeof b) < 0 ? socErr() : 0; }
int SOC_Connect(int s, const void *sa)
{
    struct sockaddr_in b; toBsd(sa, &b);
    soctr("connect %d", s);
    int r = connect(s, (struct sockaddr *)&b, sizeof b);
    int e = r < 0 ? socErr() : 0;
    static int n; if (n++ < 8) NLOG("[SOC] connect(%d, %s:%u) -> %d", s, inet_ntoa(b.sin_addr), (unsigned)ntohs(b.sin_port), e);
    return e;
}
int SOC_Listen(int s, int backlog) { return listen(s, backlog) < 0 ? socErr() : 0; }
int SOC_Accept(int s, void *sa)
{
    struct sockaddr_in b; socklen_t l = sizeof b;
    int fd = accept(s, (struct sockaddr *)&b, &l);
    if (fd < 0) return socErr();
    if (sa) fromBsd(&b, sa);
    return fd;
}
static void cpsDump(const char *tag, const unsigned char *b, unsigned n);
static void tcpPeek(const char *tag, const void *b, int r, unsigned maxn) { static int n; if (r > 0 && n < 14) { n++; cpsDump(tag, b, (unsigned)r < maxn ? (unsigned)r : maxn); } }
int SOC_Read(int s, void *buf, int len) { soctr("read %d len %d", s, len); int r = read(s, buf, len); return r < 0 ? socErr() : r; }
int SOC_Recv(int s, void *buf, int len, int flags) { soctr("recv %d len %d", s, len); (void)flags; int r = recv(s, buf, len, 0); tcpPeek("GP recv", buf, r, 600); return r < 0 ? socErr() : r; }
int SOC_RecvFrom(int s, void *buf, int len, int flags, void *sa)
{ soctr("recvfrom %d len %d", s, len);
    struct sockaddr_in b; socklen_t l = sizeof b; (void)flags;
    memset(&b, 0, sizeof b);
    int r = recvfrom(s, buf, len, 0, (struct sockaddr *)&b, &l);
    if (r < 0) return socErr();
    if (sa) fromBsd(&b, sa);
    return r;
}
int SOC_Write(int s, const void *buf, int len) { soctr("write %d len %d", s, len); int r = write(s, buf, len); return r < 0 ? socErr() : r; }
int SOC_Send(int s, const void *buf, int len, int flags) { soctr("send %d len %d", s, len); (void)flags; int r = send(s, buf, len, 0); tcpPeek("GP send", buf, r, 150); return r < 0 ? socErr() : r; }
int SOC_SendTo(int s, const void *buf, int len, int flags, const void *sa)
{ soctr("sendto %d len %d %s", s, len, sa ? "addr" : "noaddr");
    struct sockaddr_in b; (void)flags;
    int r;
    if (sa) { toBsd(sa, &b); r = sendto(s, buf, len, 0, (struct sockaddr *)&b, sizeof b); }
    else r = send(s, buf, len, 0);
    return r < 0 ? socErr() : r;
}
int SOC_Close(int s) { soctr("close %d", s); return close(s) < 0 ? socErr() : 0; }
int SOC_Shutdown(int s, int how) { return shutdown(s, how) < 0 ? socErr() : 0; }
int SOC_GetSockName(int s, void *sa)
{
    struct sockaddr_in b; socklen_t l = sizeof b;
    memset(&b, 0, sizeof b);
    if (getsockname(s, (struct sockaddr *)&b, &l) < 0) return socErr();
    if (sa) fromBsd(&b, sa);
    return 0;
}
long SOC_GetHostID(void) { return (long)gethostid(); }
int SOC_Fcntl(int s, int cmd, int mode)
{ soctr("fcntl %d cmd %d mode %d", s, cmd, mode);
    int fl = fcntl(s, F_GETFL, 0);
    if (fl < 0) return socErr();
    if (cmd == 3) return (fl & O_NONBLOCK) ? 0x04 : 0;           /* SOC_F_GETFL -> SOC_O_NONBLOCK bit */
    if (cmd == 4) { fl = (mode & 0x04) ? (fl | O_NONBLOCK) : (fl & ~O_NONBLOCK); return fcntl(s, F_SETFL, fl) < 0 ? socErr() : 0; }
    return -28;
}
int SOC_Poll(P3DSPollFD fds[], unsigned nfds, long long timeout)
{ soctr("poll n=%u t=%lld", nfds, timeout);
    struct pollfd p[8]; unsigned i;
    if (nfds > 8) nfds = 8;
    for (i = 0; i < nfds; i++) {
        p[i].fd = fds[i].fd; p[i].revents = 0; p[i].events = 0;
        if (fds[i].events & 0x03) p[i].events |= POLLIN;
        if (fds[i].events & 0x18) p[i].events |= POLLOUT;
    }
    int r = poll(p, nfds, timeout > 0 ? 1 : 0);
    if (r < 0) return socErr();
    for (i = 0; i < nfds; i++) {
        short rv = 0;
        if (p[i].revents & POLLIN) rv |= 0x01;
        if (p[i].revents & POLLOUT) rv |= 0x08;
        if (p[i].revents & POLLERR) rv |= 0x20;
        if (p[i].revents & POLLHUP) rv |= 0x40;
        if (p[i].revents & POLLNVAL) rv |= 0x80;
        fds[i].revents = rv;
    }
    return r;
}

/* ---- run 91: the library's CPS socket layer (used by the NAS/auth HTTP client dwc_http.c) on 3DS sockets --------------------------
 * Run 90 fault: DWCi_Http_Thread -> CPS_SocBind -> SDL_LockMutex(NULL). CPS is the DS TCP/IP stack; its mutexes only exist after
 * CPS_Startup, which never runs here. DWCi_Http_Thread calls exactly these functions, in this order: SocRegister, Resolve, SocUse,
 * (SetRootCa/SetSsl when ssl), SocBind, TcpConnect, SocWrite, SocFlush, then SocGetLength/SocRead/SocConsume until done, then
 * TcpShutdown, TcpClose, SocRelease, SocUnRegister. One request at a time, so one static slot. Addresses/ports are host order.
 * GetLength returns bytes waiting, 0 = nothing yet, -1 = closed (empty) or failed. The library's receive loop has no timeout on this
 * port, so GetLength gives up after 30 s of silence. Plain http only (the custom NAS URL is http). Online mode only. */
extern unsigned int CPSMyIp; /* library global: the receive loop treats 0 as "no connection" */
static struct { int fd; unsigned short rport; unsigned int rip; unsigned char rx[8192]; unsigned rxn; unsigned rxPos; int eof; unsigned idle; } sCps = { -1 };

static void cpsClose(void) { if (sCps.fd >= 0) { close(sCps.fd); sCps.fd = -1; } sCps.rxn = sCps.rxPos = 0; sCps.eof = 0; sCps.idle = 0; }

void CPS_SocRegister(void *soc) { (void)soc; NLOG("[CPS] SocRegister"); cpsClose(); }
void CPS_SocUnRegister(void) { cpsClose(); }
void CPS_SocUse(void) { cpsClose(); }
void CPS_SocRelease(void) { cpsClose(); }
unsigned int CPS_Resolve(const char *host) { return SOCL_Resolve(host); }
void CPS_SetRootCa(const void *a, int n) { (void)a; (void)n; }
void CPS_SetSsl(int on) { if (on) NLOG("[CPS] ssl requested: not supported on 3DS, request will fail"); }
void CPS_SocBind(unsigned short lport, unsigned short rport, unsigned int rip) { (void)lport; sCps.rport = rport; sCps.rip = rip; NLOG("[CPS] bind -> %s:%u", inet_ntoa((struct in_addr){ htonl(rip) }), (unsigned)rport); }

unsigned int CPS_TcpConnect(void)
{
    struct sockaddr_in a; int fl, r, e;
    if (!onlineMode() || P3DS_OnlineCheatBlocked() || !P3DS_NetInit()) { NLOG("[CPS] connect refused (online mode off, cheat on, or sockets down)"); return 1; }
    cpsClose();
    sCps.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sCps.fd < 0) { NLOG("[CPS] socket failed errno %d", errno); return 1; }
    memset(&a, 0, sizeof a); a.sin_family = AF_INET; a.sin_port = htons(sCps.rport); a.sin_addr.s_addr = htonl(sCps.rip);
    /* blocking connect, like the ghttp worker (the non-blocking poll/SO_ERROR route failed on the 3DS with errno -26 in run 91) */
    r = connect(sCps.fd, (struct sockaddr *)&a, sizeof a); e = errno;
    if (r == 0) { fl = fcntl(sCps.fd, F_GETFL, 0); fcntl(sCps.fd, F_SETFL, fl | O_NONBLOCK); }
    if (r < 0) { NLOG("[CPS] connect failed errno %d (r %d)", e, r); cpsClose(); return 1; }
    CPSMyIp = ntohl((unsigned)gethostid()); if (!CPSMyIp) CPSMyIp = 0x0a000002;
    NLOG("[CPS] connected");
    return 0;
}

static void cpsDump(const char *tag, const unsigned char *b, unsigned n)
{
    char t[900]; unsigned i, o = 0;
    for (i = 0; i < n && o < sizeof t - 6; i++) {
        unsigned char c = b[i];
        if (c == '\r') continue;
        if (c == '\n') { t[o++] = ' '; t[o++] = '|'; t[o++] = ' '; }
        else t[o++] = (c >= 32 && c < 127) ? (char)c : '.';
    }
    t[o] = 0;
    for (i = 0; i == 0 || (i < o && i < 900); i += 300) { char c2 = t[i + 300 > o ? o : i + 300]; t[i + 300 > o ? o : i + 300] = 0; NLOG("[CPS] %s (%u bytes) part %u: %s", tag, n, i / 300, t + i); t[i + 300 > o ? o : i + 300] = c2; }
}

unsigned int CPS_SocWrite(const unsigned char *buf, unsigned len)
{
    unsigned off = 0, tries = 0;
    if (sCps.fd < 0) return 0;
    while (off < len) {
        int r = send(sCps.fd, buf + off, len - off, 0);
        if (r > 0) { off += r; tries = 0; continue; }
        if (r < 0 && errno == EAGAIN && ++tries < 2000) { svcSleepThread(5000000LL); continue; }
        NLOG("[CPS] send failed errno %d", errno); return off;
    }
    NLOG("[CPS] sent %u bytes", off); { static int d; if (d++ < 2) cpsDump("request", buf, len); }
    return off;
}
void CPS_SocFlush(void) {}

int CPS_SocGetLength(void)
{
    if (sCps.fd < 0) return -1;
    if (sCps.rxn - sCps.rxPos == 0) { sCps.rxn = sCps.rxPos = 0; }
    if (!sCps.eof && sCps.rxn < sizeof sCps.rx) {
        int r = recv(sCps.fd, sCps.rx + sCps.rxn, sizeof sCps.rx - sCps.rxn, 0);
        if (r > 0) { sCps.rxn += r; sCps.idle = 0; static int told; if (told++ < 4) { NLOG("[CPS] received %d bytes", r); if (told <= 2) cpsDump("reply", sCps.rx + sCps.rxn - r, (unsigned)r); } }
        else if (r == 0) { sCps.eof = 1; NLOG("[CPS] server closed the connection"); }
        else if (errno != EAGAIN) { NLOG("[CPS] recv failed errno %d", errno); sCps.eof = 1; }
    }
    if (sCps.rxn - sCps.rxPos > 0) return (int)(sCps.rxn - sCps.rxPos);
    if (sCps.eof) return -1;
    if (++sCps.idle > 30 * 200) { NLOG("[CPS] no reply for 30 s: giving up"); return -1; }
    svcSleepThread(5000000LL); /* 5 ms per empty poll */
    return 0;
}
unsigned char *CPS_SocRead(unsigned *len)
{
    unsigned n = sCps.rxn - sCps.rxPos;
    if (sCps.fd < 0 || n == 0) { *len = 0; return NULL; }
    *len = n;
    return sCps.rx + sCps.rxPos;
}
void CPS_SocConsume(unsigned len) { unsigned n = sCps.rxn - sCps.rxPos; sCps.rxPos += len < n ? len : n; }
unsigned int CPS_TcpShutdown(void) { if (sCps.fd >= 0) shutdown(sCps.fd, 2); return 0; }
unsigned int CPS_TcpClose(void) { cpsClose(); return 0; }

/* ---- run 95: the NAS login request carries the cartridge's game and maker code (HW_ROM_HEADER_BUF+0xC / +0x10). That buffer is empty in this
 * port, so Wiimmfi refused the login (error #23934, "Invalid login parameter", empty gamecd=/makercd=). Fill them from the ROM file's header
 * (read only) right before the request is built. cal layout (DWCCalInfo): gamecd[5] at +0x15, makercd[3] at +0x1a. */
extern void __real_DWC_Auth_SetCalInfoToHttp(void *aHttp, char *cal, int withSsid);
void __wrap_DWC_Auth_SetCalInfoToHttp(void *aHttp, char *cal, int withSsid)
{
    if (cal && cal[0x15] == 0) {
        unsigned char h[0x14]; FILE *f = fopen("sdmc:/3ds/platinum3ds/platinum.nds", "rb");
        if (f && fread(h, 1, sizeof h, f) == sizeof h && h[0x0C] >= 'A' && h[0x0C] <= 'Z') {
            memcpy(cal + 0x15, h + 0x0C, 4); cal[0x19] = 0;
            memcpy(cal + 0x1a, h + 0x10, 2); cal[0x1c] = 0;
            NLOG("[LOGIN] NAS request: game code %.4s, maker code %.2s from the ROM header", cal + 0x15, cal + 0x1a);
        } else NLOG("[LOGIN] NAS request: could not read the ROM header for the game code");
        if (f) fclose(f);
    }
    __real_DWC_Auth_SetCalInfoToHttp(aHttp, cal, withSsid);
}

/* ---- run 98: real MD5 for GameSpy GP --------------------------------------------------------------------------------------------
 * md5c.c is built with _NITRO, which compiles the RSA MD5 out and leaves MD5Init/Update/Final as one-instruction jumps into the SDK's
 * DGT_Hash1* functions (a different hash and context). The GP login proof (response = MD5(MD5(partnerchallenge) + ... )) came out as
 * zero bytes and GameSpy answered "\err\804 Response is wrong" (run 97 log). These definitions win over the library's (platform
 * objects link first, duplicates allowed). Same MD5_CTX layout as include/gs/md5.h: u32 state[4], u32 count[2], u8 buffer[64]. */
typedef struct { unsigned int state[4]; unsigned int count[2]; unsigned char buffer[64]; } P3DSMd5Ctx;

static void md5Block(unsigned int st[4], const unsigned char *p)
{
    static const unsigned int K[64] = {
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
        0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
        0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
        0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391 };
    static const unsigned char S[64] = { 7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22, 5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
                                        4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23, 6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21 };
    unsigned int w[16], a = st[0], b = st[1], c = st[2], d = st[3], i;
    for (i = 0; i < 16; i++) w[i] = (unsigned)p[i*4] | ((unsigned)p[i*4+1] << 8) | ((unsigned)p[i*4+2] << 16) | ((unsigned)p[i*4+3] << 24);
    for (i = 0; i < 64; i++) {
        unsigned int f, g, t;
        if (i < 16) { f = (b & c) | (~b & d); g = i; }
        else if (i < 32) { f = (d & b) | (~d & c); g = (5 * i + 1) & 15; }
        else if (i < 48) { f = b ^ c ^ d; g = (3 * i + 5) & 15; }
        else { f = c ^ (b | ~d); g = (7 * i) & 15; }
        t = d; d = c; c = b;
        f = f + a + K[i] + w[g];
        b = b + ((f << S[i]) | (f >> (32 - S[i])));
        a = t;
    }
    st[0] += a; st[1] += b; st[2] += c; st[3] += d;
}
void MD5Init(void *vc)
{
    P3DSMd5Ctx *c = vc;
    c->count[0] = c->count[1] = 0;
    c->state[0] = 0x67452301; c->state[1] = 0xefcdab89; c->state[2] = 0x98badcfe; c->state[3] = 0x10325476;
}
void MD5Update(void *vc, const unsigned char *in, unsigned int len)
{
    P3DSMd5Ctx *c = vc;
    unsigned int idx = (c->count[0] >> 3) & 63, i = 0;
    if ((c->count[0] += len << 3) < (len << 3)) c->count[1]++;
    c->count[1] += len >> 29;
    if (idx) {
        unsigned int part = 64 - idx;
        if (len < part) { memcpy(c->buffer + idx, in, len); return; }
        memcpy(c->buffer + idx, in, part); md5Block(c->state, c->buffer); i = part;
    }
    for (; i + 64 <= len; i += 64) md5Block(c->state, in + i);
    memcpy(c->buffer, in + i, len - i);
}
void MD5Final(unsigned char digest[16], void *vc)
{
    P3DSMd5Ctx *c = vc;
    static const unsigned char pad[64] = { 0x80 };
    unsigned char bits[8]; unsigned int i, idx = (c->count[0] >> 3) & 63;
    for (i = 0; i < 4; i++) { bits[i] = (unsigned char)(c->count[0] >> (8 * i)); bits[4 + i] = (unsigned char)(c->count[1] >> (8 * i)); }
    MD5Update(c, pad, idx < 56 ? 56 - idx : 120 - idx);
    MD5Update(c, bits, 8);
    for (i = 0; i < 16; i++) digest[i] = (unsigned char)(c->state[i >> 2] >> (8 * (i & 3)));
}
void MD5Digest(unsigned char *input, unsigned int len, char output[33])
{
    P3DSMd5Ctx c; unsigned char d[16]; unsigned i;
    static int tested;
    MD5Init(&c); MD5Update(&c, input, len); MD5Final(d, &c);
    for (i = 0; i < 16; i++) sprintf(output + i * 2, "%02x", d[i]);
    if (!tested) {
        char t[33]; tested = 1;
        MD5Init(&c); MD5Update(&c, (const unsigned char *)"abc", 3); MD5Final(d, &c);
        for (i = 0; i < 16; i++) sprintf(t + i * 2, "%02x", d[i]);
        NLOG("[LOGIN] MD5 self-test: %s", !strcmp(t, "900150983cd24fb0d6963f7d28e17f72") ? "ok" : "FAILED");
    }
}

/* ---- run 99: use the real GameSpy profile id in the GTS ------------------------------------------------------------------------
 * Run 98: the login worked (profile id 603803353) but every GTS request still said pid=-1. The game keeps the id in SysInfo
 * (SystemData_GetDWCProfileId); earlier test runs stored -1 there, SystemData_SetDWCProfileId only writes when the field is 0, and
 * the game reads the field instead of asking DWC again. So: while this run's login succeeded, a stored 0 or -1 reads as the real id,
 * and a -1 that is about to be stored becomes the real id. Never touches the save except through the game's own setter. */
extern int __real_SystemData_GetDWCProfileId(void *sysInfo);
extern void __real_SystemData_SetDWCProfileId(void *sysInfo, int profileId);
int __wrap_SystemData_GetDWCProfileId(void *sysInfo)
{
    int v = __real_SystemData_GetDWCProfileId(sysInfo);
    if ((v == -1 || v == 0) && sLoginDone == 2 && sLoginPid > 0) {
        static int told; if (!told) { told = 1; NLOG("[LOGIN] GTS profile id: save held %d, using the login's %d", v, sLoginPid); }
        return sLoginPid;
    }
    return v;
}
void __wrap_SystemData_SetDWCProfileId(void *sysInfo, int profileId)
{
    if (profileId == -1 && sLoginDone == 2 && sLoginPid > 0) { NLOG("[LOGIN] storing the login's profile id %d instead of -1", sLoginPid); profileId = sLoginPid; }
    __real_SystemData_SetDWCProfileId(sysInfo, profileId);
}

/* ---- run 101: say which GTS state machine ends in the comm-error screen -------------------------------------------------------------
 * Run 100 log: after a finished deposit the game froze on its comm-error screen (error 4 = the ov70 state machines' 3600-frame wait
 * timeout, then a hard spin). The callers are lifted functions; the return address tells which one and the code says why it waited. */
extern void __real_NetworkError_DisplayNetworkError(unsigned heapID, unsigned error, unsigned errorCode);
void __wrap_NetworkError_DisplayNetworkError(unsigned heapID, unsigned error, unsigned errorCode)
{
    extern int DWC_GetLastError(int *code); int dcode = 0, derr = DWC_GetLastError(&dcode);
    NLOG("[COMM] NetworkError_DisplayNetworkError(heap %u, error %u, code %u) called from %p; DWC last error %d code %d; login %d pid %d",
         heapID, error, errorCode, __builtin_return_address(0), derr, dcode, sLoginDone, sLoginPid);
    __real_NetworkError_DisplayNetworkError(heapID, error, errorCode);
}

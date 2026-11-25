/* platinum3ds run 45: big ROM read cache (rom_cache_mb, default 16; 0 = off -> romfs.c's own 512 KiB cache).
 * run 44 [SPIKE]: overworld entry still spent 533 ms in ROM reads (208 x 16 KiB misses, ~2.5 ms each): many small
 * sequential reads. This cache keeps 64 KiB blocks (LRU, direct block->slot table) in a malloc'd arena of up to
 * 16 MiB (falls back 8/4/2) and reads misses with a sequential read-ahead that doubles on each consecutive miss
 * (64 KiB .. rom_ra_kb, default 1024) in ONE fread. Re-entering an area (after a battle / building) hits the cache.
 * Game thread only (romfs.c is only called there), so no locking. ROM is read-only: never stale. */
#include "crumbs.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void PSPNativeMemLog(const char *, ...);
extern unsigned long long P3DS_SpkRomTicks; extern unsigned P3DS_SpkRomBlocks;
#define BLK 0x10000u
/* run 86 [SPIKE]: ROM positions of this frame's cache misses (game thread), named at log time (P3DS_RomSpikeFiles) */
#define SPKN 32
static u32 spkPos[SPKN]; static unsigned spkN, spkAll;
static void spkNote(u32 p) { if (spkN < SPKN) spkPos[spkN++] = p; spkAll++; }
static int state = -1;               /* -1 uninit, 0 off, 1 on */
static u8 *arena; static unsigned nSlots; static s16 *slotOf; static u32 *slotBlk, *slotLru; static u32 lruClock, nBlocks;
static u8 *raBuf; static unsigned raMaxBlk = 16, raCur = 1; static u32 lastMissEnd = 0xFFFFFFFFu;
static unsigned stHit, stMiss, stReads; static unsigned long long stBytes;
static u8 *slotPf; static u32 romSizeG;            /* run 51: slot filled by zone_prefetch and not yet used */
static void learnFill(u32 b, u32 k, u64 ticks);
static void zoneStatHit(void);
static void init(u32 romSize) {
    int mb = 32, ra = 1024;   /* run 51: default 32 (falls back 24/16/8/4/2 while keeping 12 MiB heap headroom) */ FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x;
    if (f) { while (fgets(l, sizeof l, f)) { if (sscanf(l, "rom_cache_mb=%d", &x) == 1) mb = x; if (sscanf(l, "rom_ra_kb=%d", &x) == 1) ra = x; } fclose(f); }
    if (ra < 64) ra = 64; if (ra > 2048) ra = 2048; raMaxBlk = (unsigned)ra / 64;
    state = 0; if (mb <= 0 || !romSize) { PSPNativeMemLog("[ROMCACHE] off (rom_cache_mb=%d)", mb); return; }
    if (mb > 32) mb = 32;
    nBlocks = (romSize + BLK - 1) / BLK;
    slotOf = malloc(nBlocks * sizeof *slotOf); raBuf = malloc(raMaxBlk * BLK);
    if (!slotOf || !raBuf) { free(slotOf); free(raBuf); slotOf = NULL; raBuf = NULL; PSPNativeMemLog("[ROMCACHE] off: table alloc failed"); return; }
    /* keep >= 12 MiB of heap headroom for the game/port (peak malloc use seen: ~4 MiB) */
    for (; mb >= 2; mb = mb > 16 ? mb - 8 : mb / 2) { void *probe = malloc(((size_t)mb + 12) << 20); if (!probe) continue; free(probe); arena = malloc((size_t)mb << 20); if (arena) break; }
    if (!arena) { free(slotOf); free(raBuf); PSPNativeMemLog("[ROMCACHE] off: no arena"); return; }
    nSlots = ((unsigned)mb << 20) / BLK; slotBlk = malloc(nSlots * 4); slotLru = calloc(nSlots, 4);
    if (!slotBlk || !slotLru) { free(arena); free(slotOf); free(raBuf); PSPNativeMemLog("[ROMCACHE] off: slot alloc failed"); return; }
    slotPf = calloc(nSlots, 1); romSizeG = romSize;
    for (u32 i = 0; i < nBlocks; i++) slotOf[i] = -1; for (unsigned i = 0; i < nSlots; i++) slotBlk[i] = 0xFFFFFFFFu;
    state = 1; PSPNativeMemLog("[ROMCACHE] %d MiB (%u x 64 KiB) read-ahead up to %u KiB, linear free %lu", mb, nSlots, raMaxBlk * 64, (unsigned long)linearSpaceFree());
}
static int victim(void) { int v = 0; for (unsigned i = 1; i < nSlots; i++) if (slotLru[i] < slotLru[v]) v = (int)i; return v; }
static int fill(FILE *fp, u32 romSize, u32 b, u32 needEnd) {
    /* sequential detection: a miss right where the previous miss run ended doubles the read-ahead */
    raCur = b == lastMissEnd ? (raCur * 2 > raMaxBlk ? raMaxBlk : raCur * 2) : 1;
    u32 n = raCur; if (needEnd - b > n) n = needEnd - b; if (n > raMaxBlk) n = raMaxBlk;
    u32 k = 0; while (k < n && b + k < nBlocks && slotOf[b + k] < 0) k++;   /* stop at the first cached block */
    if (!k) return 0;
    u32 off = b * BLK, len = k * BLK; if (off + len > romSize) len = romSize - off;
    u64 t0 = svcGetSystemTick(); clearerr(fp);
    if (fseek(fp, (long)off, SEEK_SET) || fread(raBuf, 1, len, fp) != len) { clearerr(fp); return -1; }
    P3DS_SpkRomTicks += svcGetSystemTick() - t0; P3DS_SpkRomBlocks += (len + 0x3FFF) / 0x4000; stReads++; stBytes += len;
    for (u32 i = 0; i < k; i++) { u32 bl = off + i * BLK; if (bl >= romSize) break; int s = victim();
        if (slotBlk[s] != 0xFFFFFFFFu) slotOf[slotBlk[s]] = -1;
        u32 cl = romSize - bl < BLK ? romSize - bl : BLK; memcpy(arena + (size_t)s * BLK, raBuf + i * BLK, cl);
        slotBlk[s] = b + i; slotOf[b + i] = (s16)s; slotLru[s] = ++lruClock; if (slotPf) slotPf[s] = 0; }
    lastMissEnd = b + k; stMiss++; learnFill(b, k, svcGetSystemTick() - t0); return 1;
}
/* returns bytes read (== len) or -1 = not handled (caller uses its own path) */
static int rcRead(FILE *fp, u32 romSize, u32 pos, void *dst, u32 len) {
    if (state < 0) init(romSize);
    if (state != 1 || !fp || pos >= romSize) return -1;
    if (len > romSize - pos) len = romSize - pos;
    u8 *out = dst; u32 done = 0; const u32 needEnd = (pos + len + BLK - 1) / BLK;
    while (done < len) {
        u32 p = pos + done, b = p / BLK; int s = slotOf[b];
        if (s < 0) { spkNote(p); if (fill(fp, romSize, b, needEnd) < 0) return done ? -1 : -1; s = slotOf[b]; if (s < 0) return -1; } else { stHit++; if (slotPf && slotPf[s]) { slotPf[s] = 0; zoneStatHit(); } }
        slotLru[s] = ++lruClock;
        u32 o = p - b * BLK, c = BLK - o; if (c > len - done) c = len - done;
        memcpy(out + done, arena + (size_t)s * BLK + o, c); done += c;
    }
    { static unsigned calls; if (++calls % 4000 == 0) PSPNativeMemLog("[ROMCACHE] block hits %u misses %u (freads %u, %llu KiB read)", stHit, stMiss, stReads, stBytes >> 10); }
    return (s32)len;
}

/* ======================= run 51: zone_prefetch (config, default 1) =======================
 * Learns, per destination zone (map header ID), which ROM blocks the game had to read from SD in the first 4 s after
 * entering it (hooks: FieldMapChange_UpdateGameData for warps, FieldMap zone change while walking), and which zones
 * are entered from which (up to 8 neighbours per zone). While the player is in zone A and the game has not touched the
 * ROM for 150 ms, a lowest-priority thread on core 2 reads the learned blocks of A's neighbours that are not cached,
 * ONE 64 KiB block per step, into a staging slot (its own FILE handle). It never touches the cache: the game thread
 * adopts ready staging slots itself (frame hook / its next ROM read), so there is no shared lock and no deadlock.
 * A game ROM read can wait behind at most one in-flight 64 KiB SD read (~10 ms). Paused while the game is reading,
 * while a save is in progress (save box open or a save write in the last 10 s) and while the log is written.
 * Per-zone budget: a quarter of the cache, so the current zone's data is not pushed out.
 * The learned table persists in sdmc:/3ds/platinum3ds/prefetch.bin (small; written at most once a minute when idle and
 * not saving, via a temp file + rename; a bad/foreign file is ignored), so the NEXT session prefetches too. */
#define MAXZ 1024
#define NR 32
#define NN 8
#define NSTG 8
typedef struct { u16 r[NR][2]; u16 nb[NN]; u8 nr, nn; } Zone;
static Zone *zt; static int pfCfg = -1; static volatile int curZone = -1, recZone = -1; static volatile u64 recUntil;
/* perf pass 2026-11-14 (boot prefetch): the first map entry after boot cost ~430 ms of SD reads (run 812) because the prefetch thread only fetches the NEIGHBOURS of the
 * current zone and there is no current zone until the map loads. lastzone.bin (2 bytes) remembers the zone the last session ended in; until the first zone change the thread
 * fetches that zone's own learned blocks (the title screen's idle time). boot_prefetch=0 in config.txt turns it off. */
static volatile int startZone = -1, lzCur = -1, lzSaved = -1; static int bootPfCfg = -1;
static const char *LZPATH = "sdmc:/3ds/platinum3ds/lastzone.bin", *LZTMP = "sdmc:/3ds/platinum3ds/lastzone.tmp";
static volatile int pfReady; static volatile u64 gameLastRead; static volatile int gameInRead, pfDirty; static volatile unsigned pfBudget;
static struct { volatile int st; u32 blk, len; u8 *buf; } stg[NSTG];  /* st: 0 empty, 1 filling (thread), 2 ready */
static unsigned pfFetched, pfAdopted, pfUsed, zcN, zcWant, zcHave; static u64 spikeMax;
extern int P3DS_SaveActive(void); extern volatile int P3DS_LogWriting;
static u64 msT(unsigned ms) { return (u64)ms * 268111; }
static void zoneStatHit(void) { pfUsed++; }
/* battle_prefetch: pseudo-zone BZ holds the ROM blocks read from SD around the start of a battle. Misses of the last 3 s
 * are kept in a small ring (always, also outside the 4 s zone window) and filed under BZ when P3DS_InBattle turns on, so the
 * prefetch thread reads them ahead of the next encounter (the first battle after boot read ~100 ms from SD). */
#define BZ (MAXZ - 1)
#define NRING 64
static struct { u32 b, k; u64 t; } mring[NRING]; static unsigned mringN;
static void addRange(Zone *z, u32 b, u32 k) {
    for (unsigned i = 0; i < z->nr; i++) { u32 s0 = z->r[i][0], c0 = z->r[i][1];
        if (b >= s0 && b + k <= s0 + c0) return;                                  /* already known */
        if (b == s0 + c0 && c0 + k <= 0xFFFF) { z->r[i][1] = (u16)(c0 + k); pfDirty = 1; return; } }  /* extends a range */
    if (z->nr < NR) { z->r[z->nr][0] = (u16)b; z->r[z->nr][1] = (u16)k; z->nr++; pfDirty = 1; }
}
static void learnFill(u32 b, u32 k, u64 ticks) {
    const u64 now = svcGetSystemTick(); const unsigned i = mringN++ % NRING; mring[i].b = b; mring[i].k = k; mring[i].t = now;
    if (!zt || recZone < 0 || now > recUntil) return;
    if (ticks > spikeMax) spikeMax = ticks;
    addRange(&zt[recZone], b, k);
}
static void battleLearn(void) {   /* game thread, at battle start */
    if (!zt) return; const u64 now = svcGetSystemTick(); unsigned n = 0;
    for (unsigned i = 0; i < NRING && i < mringN; i++) if (now - mring[i].t < msT(3000)) { addRange(&zt[BZ], mring[i].b, mring[i].k); n++; }
    PSPNativeMemLog("[PREFETCH] battle start: %u recent ROM misses filed, battle zone %u ranges", n, (unsigned)zt[BZ].nr);
}
static void adopt(void) {   /* at most 4 blocks (256 KiB memcpy, < 1 ms) per call */
    if (state != 1) return; int n = 0;
    for (int i = 0; i < NSTG && n < 4; i++) if (stg[i].st == 2) { n++;
        __sync_synchronize(); u32 b = stg[i].blk;
        if (b < nBlocks && slotOf[b] < 0 && stg[i].len) { int s = victim(); if (slotBlk[s] != 0xFFFFFFFFu) slotOf[slotBlk[s]] = -1;
            memcpy(arena + (size_t)s * BLK, stg[i].buf, stg[i].len); slotBlk[s] = b; slotOf[b] = (s16)s; slotLru[s] = ++lruClock; if (slotPf) slotPf[s] = 1; pfAdopted++; }
        __sync_synchronize(); stg[i].st = 0; }
}
/* run 86: current/previous zone for [ZONE] / [SPIKE] / [PERF3] (also when zone_prefetch is off) */
static int znCur = -1, znPrev = -1, znLearned = -1; static u64 znTick;
extern const char *P3DS_AreaName(int id);
void P3DS_ZoneInfo(int *cur, int *prev, int *learned, unsigned *msSince) {
    *cur = znCur; *prev = znPrev; *learned = znLearned; *msSince = znTick ? (unsigned)((svcGetSystemTick() - znTick) / 268111) : 0;
}
void P3DS_ZoneEnter(int zone) {   /* game thread */
    if (zone != znCur) { znPrev = znCur; znCur = zone; znTick = svcGetSystemTick(); znLearned = -1; PSPNativeMemLog("[ZONE] %d %s -> %d %s", znPrev, P3DS_AreaName(znPrev), zone, P3DS_AreaName(zone)); }
    if (pfCfg != 1 || !zt || zone < 0 || zone >= MAXZ) return;
    const int prev = curZone; if (zone == prev) return;
    if (prev >= 0) { Zone *p = &zt[prev]; int have = 0; for (unsigned i = 0; i < p->nn; i++) if (p->nb[i] == zone) { have = 1;
            if (i) { u16 t = p->nb[i]; for (unsigned j = i; j > 0; j--) p->nb[j] = p->nb[j - 1]; p->nb[0] = t; } break; }  /* most recent first */
        if (!have) { if (p->nn < NN) p->nn++; for (unsigned j = p->nn - 1; j > 0; j--) p->nb[j] = p->nb[j - 1]; p->nb[0] = (u16)zone; pfDirty = 1; } }
    unsigned want = 0, have = 0; if (state == 1) { Zone *z = &zt[zone];
        for (unsigned i = 0; i < z->nr; i++) for (u32 b = z->r[i][0]; b < (u32)z->r[i][0] + z->r[i][1] && b < nBlocks; b++) { want++; if (slotOf[b] >= 0) have++; } }
    zcN++; zcWant += want; zcHave += have; znLearned = want > 0;
    PSPNativeMemLog("[PREFETCH] zone %d -> %d: learned %u blocks, %u already in RAM | totals: fetched %u adopted %u used %u | zone changes %u, ready %u%% | worst learn-window SD read %u ms",
        prev, zone, want, have, pfFetched, pfAdopted, pfUsed, zcN, zcWant ? (unsigned)(100ull * zcHave / zcWant) : 0, (unsigned)(spikeMax / 268111));
    spikeMax = 0; curZone = zone; lzCur = zone; recZone = zone; recUntil = svcGetSystemTick() + msT(4000); pfBudget = nSlots / 4;
}
static int inStaging(u32 b) { for (int i = 0; i < NSTG; i++) if (stg[i].st && stg[i].blk == b) return 1; return 0; }
static int idleOk(void) { return !gameInRead && svcGetSystemTick() - gameLastRead > msT(150) && !P3DS_SaveActive() && !P3DS_LogWriting; }
static const char *PFPATH = "sdmc:/3ds/platinum3ds/prefetch.bin", *PFTMP = "sdmc:/3ds/platinum3ds/prefetch.tmp";
static void pfLoad(void) {
    FILE *f = fopen(PFPATH, "rb"); if (!f) return; u32 h[3]; unsigned nz = 0;
    if (fread(h, 4, 3, f) == 3 && h[0] == 0x31304650u && h[1] == romSizeG && h[2] <= MAXZ) {
        for (u32 i = 0; i < h[2]; i++) { u16 id; u8 nr, nn; Zone z; memset(&z, 0, sizeof z);
            if (fread(&id, 2, 1, f) != 1 || fread(&nr, 1, 1, f) != 1 || fread(&nn, 1, 1, f) != 1 || id >= MAXZ || nr > NR || nn > NN) break;
            if (fread(z.r, 4, nr, f) != nr || fread(z.nb, 2, nn, f) != nn) break;
            int bad = 0; for (unsigned k = 0; k < nr; k++) if ((u32)z.r[k][0] + z.r[k][1] > nBlocks) bad = 1; for (unsigned k = 0; k < nn; k++) if (z.nb[k] >= MAXZ) bad = 1;
            if (bad) break; z.nr = nr; z.nn = nn; zt[id] = z; nz++; } }
    fclose(f); PSPNativeMemLog("[PREFETCH] learned table loaded: %u zones", nz);
}
static void pfSave(void) {
    FILE *f = fopen(PFTMP, "wb"); if (!f) return; u32 cnt = 0; for (int i = 0; i < MAXZ; i++) if (zt[i].nr || zt[i].nn) cnt++;
    u32 h[3] = { 0x31304650u, romSizeG, cnt }; int ok = fwrite(h, 4, 3, f) == 3;
    for (int i = 0; i < MAXZ && ok; i++) { Zone *z = &zt[i]; if (!z->nr && !z->nn) continue; u16 id = (u16)i; u8 nr = z->nr, nn = z->nn;
        ok = fwrite(&id, 2, 1, f) == 1 && fwrite(&nr, 1, 1, f) == 1 && fwrite(&nn, 1, 1, f) == 1 && fwrite(z->r, 4, nr, f) == nr && fwrite(z->nb, 2, nn, f) == nn; }
    if (fclose(f) != 0) ok = 0;
    if (ok) { remove(PFPATH); ok = rename(PFTMP, PFPATH) == 0; } else remove(PFTMP);
    PSPNativeMemLog("[PREFETCH] persist %s (%u zones)", ok ? "ok" : "FAILED", cnt);
}
static Thread pfWorker;
static int pfStop;

static void pfThread(void *arg) {
    P3DS_ThreadGuard("prefetch");
    (void)arg; FILE *fp = NULL; u64 lastPersist = svcGetSystemTick(), lastLz = svcGetSystemTick(); int busy = 0;
    while (!__atomic_load_n(&pfStop, __ATOMIC_ACQUIRE)) {
        svcSleepThread(busy ? 2000000LL : 30000000LL); busy = 0;
        if (__atomic_load_n(&pfStop, __ATOMIC_ACQUIRE)) break;
        if (state != 1 || !pfReady) continue;
        if (!idleOk()) continue;
        if (pfDirty && svcGetSystemTick() - lastPersist > msT(60000)) { pfDirty = 0; lastPersist = svcGetSystemTick(); pfSave(); continue; }
        if (lzCur >= 0 && lzCur != lzSaved && svcGetSystemTick() - lastLz > msT(60000)) { lastLz = svcGetSystemTick(); const int z = lzCur; FILE *lf = fopen(LZTMP, "wb");
            if (lf) { u16 v = (u16)z; int ok = fwrite(&v, 2, 1, lf) == 1; if (fclose(lf)) ok = 0; if (ok) { remove(LZPATH); ok = rename(LZTMP, LZPATH) == 0; } else remove(LZTMP); if (ok) lzSaved = z; } continue; }
        const int boot = curZone < 0 && startZone >= 0 && bootPfCfg == 1;
        const int a = curZone >= 0 ? curZone : (boot ? startZone : -1); if (a < 0 || !pfBudget) continue;
        int slot = -1; for (int i = 0; i < NSTG; i++) if (!stg[i].st) { slot = i; break; }
        if (slot < 0) continue;   /* wait for the game thread to adopt */
        u32 want = 0xFFFFFFFFu; Zone *za = &zt[a];
        for (int n = boot ? -1 : 0; n <= (int)za->nn && want == 0xFFFFFFFFu; n++) { Zone *z = n < 0 ? za : &zt[n ? za->nb[n - 1] : BZ];   /* boot: the zone's own blocks first; then battle blocks */
            for (unsigned i = 0; i < z->nr && want == 0xFFFFFFFFu; i++) for (u32 b = z->r[i][0]; b < (u32)z->r[i][0] + z->r[i][1] && b < nBlocks; b++)
                if (slotOf[b] < 0 && !inStaging(b)) { want = b; break; } }
        if (want == 0xFFFFFFFFu) continue;
        if (!fp) { fp = fopen("sdmc:/3ds/platinum3ds/platinum.nds", "rb"); if (!fp) continue; }
        if (!idleOk() || (curZone >= 0 && curZone != a)) continue;
        stg[slot].blk = want; stg[slot].st = 1;
        u32 off = want * BLK, len = romSizeG - off < BLK ? romSizeG - off : BLK;
        clearerr(fp);
        if (fseek(fp, (long)off, SEEK_SET) || fread(stg[slot].buf, 1, len, fp) != len) { fclose(fp); fp = NULL; stg[slot].st = 0; continue; }  /* e.g. after sleep: reopen next time */
        stg[slot].len = len; pfFetched++; if (pfBudget) pfBudget--;
        __sync_synchronize(); stg[slot].st = 2; busy = 1;
    }
    if (fp) fclose(fp);
}

int P3DS_RomCacheShutdown(void) {
    __atomic_store_n(&pfStop, 1, __ATOMIC_RELEASE);
    if (!pfWorker) return 1;
    Result rc = threadJoin(pfWorker, 500000000ULL);
    if (rc != 0) {
        PSPNativeMemLog("[3DS] exit: prefetch worker did not stop (join %08lx)", (unsigned long)rc);
        return 0;
    }
    threadFree(pfWorker);
    pfWorker = NULL;
    return 1;
}

void P3DS_RomCachePump(void) {   /* game thread, once per frame (render3ds Present) */
    if (__atomic_load_n(&pfStop, __ATOMIC_ACQUIRE)) return;
    if (pfCfg < 0) { pfCfg = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "zone_prefetch=%d", &x) == 1) pfCfg = x != 0; fclose(f); }
        if (pfCfg) { zt = calloc(MAXZ, sizeof *zt); for (int i = 0; i < NSTG && zt; i++) if (!(stg[i].buf = malloc(BLK))) { free(zt); zt = NULL; }
            if (zt) {
                pfWorker = threadCreate(pfThread, NULL, 0x4000, 0x3F, 2, false);
                if (!pfWorker) PSPNativeMemLog("[PREFETCH] thread start FAILED");
            }
            if (!pfWorker) {
                for (int i = 0; i < NSTG; i++) { free(stg[i].buf); stg[i].buf = NULL; }
                free(zt); zt = NULL; pfCfg = 0;
            }
        }
        PSPNativeMemLog("[PREFETCH] zone_prefetch=%d", pfCfg); }
    { extern int P3DS_InBattle; static int wasB; if (P3DS_InBattle && !wasB && pfCfg == 1) battleLearn(); wasB = P3DS_InBattle != 0; }
    if (pfCfg == 1 && state == 1) { static int loaded; if (!loaded) { loaded = 1; pfLoad();
            if (bootPfCfg < 0) { bootPfCfg = 1; FILE *cf = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (cf) { while (fgets(l, sizeof l, cf)) if (sscanf(l, "boot_prefetch=%d", &x) == 1) bootPfCfg = x != 0; fclose(cf); } }
            { FILE *lf = fopen(LZPATH, "rb"); u16 v; if (lf) { if (fread(&v, 2, 1, lf) == 1 && v < MAXZ && (zt[v].nr || zt[v].nn)) { startZone = v; lzSaved = v; pfBudget = nSlots / 4; } fclose(lf); } }
            PSPNativeMemLog("[PREFETCH] boot_prefetch=%d, last session ended in zone %d%s", bootPfCfg, startZone, startZone >= 0 ? " (its learned blocks are read during the title screen)" : " (none / nothing learned)");
            pfReady = 1; }  /* game thread, once: no race with learning */
        if (!gameInRead) adopt(); }
}
int P3DS_RomCacheRead(FILE *fp, u32 romSize, u32 pos, void *dst, u32 len) {
    gameInRead = 1; gameLastRead = svcGetSystemTick();
    if (pfCfg == 1 && state == 1) adopt();
    int r = rcRead(fp, romSize, pos, dst, len);
    gameLastRead = svcGetSystemTick(); gameInRead = 0;
    return r;
}

/* ======================= run 86: name the files behind a [SPIKE]'s ROM misses =======================
 * At the first spike with misses, reads the NDS header's FAT + FNT from the user's ROM (own FILE handle, a few tens of
 * KiB, once) and keeps them in RAM. Each recorded miss position is mapped to its file (path from the FNT, or
 * "file N" for unnamed files such as overlays) and the offset inside it; same file = one entry. Nothing is written. */
static u32 *fat; static u32 fatN; static u8 *fnt; static u32 fntLen; static int fatSt;   /* 0 not tried, 1 ok, -1 failed */
static void fatLoad(void) {
    fatSt = -1; FILE *f = fopen("sdmc:/3ds/platinum3ds/platinum.nds", "rb"); if (!f) return; u32 h[4];
    if (!fseek(f, 0x40, SEEK_SET) && fread(h, 4, 4, f) == 4 && h[1] && h[1] < 0x100000 && h[3] && h[3] < 0x40000 && (h[3] & 7) == 0) {
        fnt = malloc(h[1]); fat = malloc(h[3]);
        if (fnt && fat && !fseek(f, (long)h[0], SEEK_SET) && fread(fnt, 1, h[1], f) == h[1] && !fseek(f, (long)h[2], SEEK_SET) && fread(fat, 1, h[3], f) == h[3]) {
            fntLen = h[1]; fatN = h[3] / 8; fatSt = 1; }
        else { free(fnt); free(fat); fnt = NULL; fat = NULL; } }
    fclose(f); PSPNativeMemLog("[SPIKE] ROM file table %s (%u files)", fatSt == 1 ? "loaded" : "not readable", (unsigned)fatN);
}
static u32 rd32(u32 o) { u32 v; memcpy(&v, fnt + o, 4); return v; }
static u16 rd16(u32 o) { u16 v; memcpy(&v, fnt + o, 2); return v; }
/* walks directory d's sub-table; returns the entry name of file id `want` (or of sub-directory 0xF000|want if isDir) */
static int fntFind(unsigned d, u32 want, int isDir, const u8 **nm) {
    u32 o = rd32(d * 8); u32 id = rd16(d * 8 + 4);
    while (o < fntLen) { u8 t = fnt[o++]; if (!t) break; unsigned l = t & 0x7F; if (o + l > fntLen) break;
        if (t & 0x80) { if (o + l + 2 > fntLen) break; u16 sub = rd16(o + l); if (isDir && sub == (0xF000 | want)) { *nm = fnt + o; return (int)l; } o += l + 2; }
        else { if (!isDir && id == want) { *nm = fnt + o; return (int)l; } id++; o += l; } }
    return -1;
}
static void fileName(u32 id, char *out, unsigned n) {
    snprintf(out, n, "file %u", (unsigned)id); if (fntLen < 8) return;
    unsigned nd = rd16(6); if (nd == 0 || nd * 8 > fntLen) return;
    for (unsigned d = 0; d < nd; d++) { const u8 *nm; int l = fntFind(d, id, 0, &nm); if (l < 0) continue;
        char path[128]; unsigned pl = sizeof path - 1; path[pl] = 0;
        #define PUT(p_, l_) do { unsigned L = (unsigned)(l_); if (L > pl) L = pl; pl -= L; memcpy(path + pl, (p_), L); } while (0)
        PUT(nm, l); unsigned cur = d;
        for (int depth = 0; cur && depth < 8; depth++) { unsigned par = rd16(cur * 8 + 6) & 0xFFF; if (par >= nd) break;
            const u8 *dn; int dl = fntFind(par, cur, 1, &dn); if (dl < 0) break; if (pl) path[--pl] = '/'; PUT(dn, dl); cur = par; }
        #undef PUT
        snprintf(out, n, "%s", path + pl); return; }
}
/* writes " | missed in: fielddata/x.narc +812K 6x, ..." (up to 4 files) for this frame's misses; then clears the record */
void P3DS_RomSpikeFiles(char *out, unsigned n) {
    out[0] = 0;
    if (spkN) { if (!fatSt) fatLoad();
        u32 ids[4], lo[4]; unsigned cnt[4], nf = 0, other = 0;
        for (unsigned i = 0; i < spkN; i++) { u32 p = spkPos[i], id = 0xFFFFFFFFu;
            if (fatSt == 1) for (u32 k = 0; k < fatN; k++) if (p >= fat[k * 2] && p < fat[k * 2 + 1]) { id = k; break; }
            unsigned j = 0; while (j < nf && ids[j] != id) j++;
            if (j < nf) { cnt[j]++; u32 o = id == 0xFFFFFFFFu ? p : p - fat[id * 2]; if (o < lo[j]) lo[j] = o; }
            else if (nf < 4) { ids[nf] = id; lo[nf] = id == 0xFFFFFFFFu ? p : p - fat[id * 2]; cnt[nf++] = 1; } else other++; }
        unsigned w = (unsigned)snprintf(out, n, " | missed in:");
        for (unsigned j = 0; j < nf && w < n; j++) { char nm[128];
            if (ids[j] == 0xFFFFFFFFu) snprintf(nm, sizeof nm, "rom 0x%X", (unsigned)lo[j]); else fileName(ids[j], nm, sizeof nm);
            w += (unsigned)snprintf(out + w, n - w, "%s %s +%uK %ux", j ? "," : "", nm, (unsigned)(ids[j] == 0xFFFFFFFFu ? 0 : lo[j] >> 10), cnt[j]); }
        if (w < n && (other || spkAll > spkN)) snprintf(out + w, n - w, " (+%u more)", other + spkAll - spkN); }
    spkN = 0; spkAll = 0;
}
void P3DS_RomSpikeReset(void) { spkN = 0; spkAll = 0; }

/* run 148 (Pokedex finder sprite): read one member of a NARC in the ROM by path (e.g. "a/0/2/0") straight from the SD ROM copy.
   Read-only, small (<= cap bytes), render thread, called once per species and cached by the caller. Returns the size or 0. */
static int fntDirFind(unsigned d, const char *nm, unsigned nl, int wantDir, u32 *out) {
    if (d * 8 + 8 > fntLen) return 0; u32 o = rd32(d * 8); u32 id = rd16(d * 8 + 4);
    while (o < fntLen) { u8 t = fnt[o++]; if (!t) break; unsigned l = t & 0x7F; if (o + l > fntLen) break; int isD = (t & 0x80) != 0; u32 sub = 0;
        if (isD) { if (o + l + 2 > fntLen) break; sub = rd16(o + l); }
        if (l == nl && !memcmp(fnt + o, nm, nl) && isD == wantDir) { *out = isD ? (sub & 0xFFF) : id; return 1; }
        if (isD) o += l + 2; else { id++; o += l; } }
    return 0;
}
int P3DS_RomNarcMember(const char *path, int member, unsigned char *dst, unsigned cap) {
    if (!fatSt) fatLoad(); if (fatSt != 1 || !path || !dst) return 0;
    unsigned d = 0; u32 fid = 0; const char *p = path;
    for (;;) { const char *e = strchr(p, '/'); unsigned l = e ? (unsigned)(e - p) : (unsigned)strlen(p); u32 r;
        if (!l || !fntDirFind(d, p, l, e != NULL, &r)) return 0; if (!e) { fid = r; break; } d = r; p = e + 1; }
    if (fid >= fatN) return 0; const u32 fs = fat[fid * 2], fe = fat[fid * 2 + 1]; if (fe <= fs + 0x30) return 0;
    FILE *f = fopen("sdmc:/3ds/platinum3ds/platinum.nds", "rb"); if (!f) return 0; int ret = 0;
    u32 h[8]; u32 fb[2]; u32 gm;
    do { if (fseek(f, (long)fs, SEEK_SET) || fread(h, 4, 8, f) != 8 || memcmp(h, "NARC", 4) || memcmp(&h[4], "BTAF", 4)) break;
         const u32 fatbSz = h[5]; const u32 cnt = (u32)(h[6] & 0xFFFF); if (member < 0 || (u32)member >= cnt) break;
         if (fseek(f, (long)(fs + 0x10 + 12 + (u32)member * 8), SEEK_SET) || fread(fb, 4, 2, f) != 2) break;
         u32 nh[2]; if (fseek(f, (long)(fs + 0x10 + fatbSz), SEEK_SET) || fread(nh, 4, 2, f) != 2 || memcmp(nh, "BTNF", 4)) break;
         gm = fs + 0x10 + fatbSz + nh[1] + 8; const u32 n = fb[1] - fb[0]; if (!n || n > cap) break;
         if (fseek(f, (long)(gm + fb[0]), SEEK_SET) || fread(dst, 1, n, f) != n) break; ret = (int)n; } while (0);
    fclose(f); return ret;
}

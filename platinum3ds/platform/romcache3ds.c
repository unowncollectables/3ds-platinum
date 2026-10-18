/* platinum3ds run 45: big ROM read cache (rom_cache_mb, default 16; 0 = off -> romfs.c's own 512 KiB cache).
 * run 44 [SPIKE]: overworld entry still spent 533 ms in ROM reads (208 x 16 KiB misses, ~2.5 ms each): many small
 * sequential reads. This cache keeps 64 KiB blocks (LRU, direct block->slot table) in a malloc'd arena of up to
 * 16 MiB (falls back 8/4/2) and reads misses with a sequential read-ahead that doubles on each consecutive miss
 * (64 KiB .. rom_ra_kb, default 1024) in ONE fread. Re-entering an area (after a battle / building) hits the cache.
 * Game thread only (romfs.c is only called there), so no locking. ROM is read-only: never stale. */
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void PSPNativeMemLog(const char *, ...);
extern unsigned long long P3DS_SpkRomTicks; extern unsigned P3DS_SpkRomBlocks;
#define BLK 0x10000u
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
        if (s < 0) { if (fill(fp, romSize, b, needEnd) < 0) return done ? -1 : -1; s = slotOf[b]; if (s < 0) return -1; } else { stHit++; if (slotPf && slotPf[s]) { slotPf[s] = 0; zoneStatHit(); } }
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
static volatile int pfReady; static volatile u64 gameLastRead; static volatile int gameInRead, pfDirty; static volatile unsigned pfBudget;
static struct { volatile int st; u32 blk, len; u8 *buf; } stg[NSTG];  /* st: 0 empty, 1 filling (thread), 2 ready */
static unsigned pfFetched, pfAdopted, pfUsed, zcN, zcWant, zcHave; static u64 spikeMax;
extern int P3DS_SaveActive(void); extern volatile int P3DS_LogWriting;
static u64 msT(unsigned ms) { return (u64)ms * 268111; }
static void zoneStatHit(void) { pfUsed++; }
static void learnFill(u32 b, u32 k, u64 ticks) {
    if (!zt || recZone < 0 || svcGetSystemTick() > recUntil) return;
    if (ticks > spikeMax) spikeMax = ticks;
    Zone *z = &zt[recZone];
    for (unsigned i = 0; i < z->nr; i++) { u32 s0 = z->r[i][0], c0 = z->r[i][1];
        if (b >= s0 && b + k <= s0 + c0) return;                                  /* already known */
        if (b == s0 + c0 && c0 + k <= 0xFFFF) { z->r[i][1] = (u16)(c0 + k); pfDirty = 1; return; } }  /* extends a range */
    if (z->nr < NR) { z->r[z->nr][0] = (u16)b; z->r[z->nr][1] = (u16)k; z->nr++; pfDirty = 1; }
}
static void adopt(void) {   /* at most 4 blocks (256 KiB memcpy, < 1 ms) per call */
    if (state != 1) return; int n = 0;
    for (int i = 0; i < NSTG && n < 4; i++) if (stg[i].st == 2) { n++;
        __sync_synchronize(); u32 b = stg[i].blk;
        if (b < nBlocks && slotOf[b] < 0 && stg[i].len) { int s = victim(); if (slotBlk[s] != 0xFFFFFFFFu) slotOf[slotBlk[s]] = -1;
            memcpy(arena + (size_t)s * BLK, stg[i].buf, stg[i].len); slotBlk[s] = b; slotOf[b] = (s16)s; slotLru[s] = ++lruClock; if (slotPf) slotPf[s] = 1; pfAdopted++; }
        __sync_synchronize(); stg[i].st = 0; }
}
void P3DS_ZoneEnter(int zone) {   /* game thread */
    if (pfCfg != 1 || !zt || zone < 0 || zone >= MAXZ) return;
    const int prev = curZone; if (zone == prev) return;
    if (prev >= 0) { Zone *p = &zt[prev]; int have = 0; for (unsigned i = 0; i < p->nn; i++) if (p->nb[i] == zone) { have = 1;
            if (i) { u16 t = p->nb[i]; for (unsigned j = i; j > 0; j--) p->nb[j] = p->nb[j - 1]; p->nb[0] = t; } break; }  /* most recent first */
        if (!have) { if (p->nn < NN) p->nn++; for (unsigned j = p->nn - 1; j > 0; j--) p->nb[j] = p->nb[j - 1]; p->nb[0] = (u16)zone; pfDirty = 1; } }
    unsigned want = 0, have = 0; if (state == 1) { Zone *z = &zt[zone];
        for (unsigned i = 0; i < z->nr; i++) for (u32 b = z->r[i][0]; b < (u32)z->r[i][0] + z->r[i][1] && b < nBlocks; b++) { want++; if (slotOf[b] >= 0) have++; } }
    zcN++; zcWant += want; zcHave += have;
    PSPNativeMemLog("[PREFETCH] zone %d -> %d: learned %u blocks, %u already in RAM | totals: fetched %u adopted %u used %u | zone changes %u, ready %u%% | worst learn-window SD read %u ms",
        prev, zone, want, have, pfFetched, pfAdopted, pfUsed, zcN, zcWant ? (unsigned)(100ull * zcHave / zcWant) : 0, (unsigned)(spikeMax / 268111));
    spikeMax = 0; curZone = zone; recZone = zone; recUntil = svcGetSystemTick() + msT(4000); pfBudget = nSlots / 4;
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
static void pfThread(void *arg) {
    (void)arg; FILE *fp = NULL; u64 lastPersist = svcGetSystemTick(); int busy = 0;
    for (;;) {
        svcSleepThread(busy ? 2000000LL : 30000000LL); busy = 0;
        if (state != 1 || !pfReady) continue;
        if (!idleOk()) continue;
        if (pfDirty && svcGetSystemTick() - lastPersist > msT(60000)) { pfDirty = 0; lastPersist = svcGetSystemTick(); pfSave(); continue; }
        const int a = curZone; if (a < 0 || !pfBudget) continue;
        int slot = -1; for (int i = 0; i < NSTG; i++) if (!stg[i].st) { slot = i; break; }
        if (slot < 0) continue;   /* wait for the game thread to adopt */
        u32 want = 0xFFFFFFFFu; Zone *za = &zt[a];
        for (unsigned n = 0; n < za->nn && want == 0xFFFFFFFFu; n++) { Zone *z = &zt[za->nb[n]];
            for (unsigned i = 0; i < z->nr && want == 0xFFFFFFFFu; i++) for (u32 b = z->r[i][0]; b < (u32)z->r[i][0] + z->r[i][1] && b < nBlocks; b++)
                if (slotOf[b] < 0 && !inStaging(b)) { want = b; break; } }
        if (want == 0xFFFFFFFFu) continue;
        if (!fp) { fp = fopen("sdmc:/3ds/platinum3ds/platinum.nds", "rb"); if (!fp) continue; }
        if (!idleOk() || curZone != a) continue;
        stg[slot].blk = want; stg[slot].st = 1;
        u32 off = want * BLK, len = romSizeG - off < BLK ? romSizeG - off : BLK;
        clearerr(fp);
        if (fseek(fp, (long)off, SEEK_SET) || fread(stg[slot].buf, 1, len, fp) != len) { fclose(fp); fp = NULL; stg[slot].st = 0; continue; }  /* e.g. after sleep: reopen next time */
        stg[slot].len = len; pfFetched++; if (pfBudget) pfBudget--;
        __sync_synchronize(); stg[slot].st = 2; busy = 1;
    }
}
void P3DS_RomCachePump(void) {   /* game thread, once per frame (render3ds Present) */
    if (pfCfg < 0) { pfCfg = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "zone_prefetch=%d", &x) == 1) pfCfg = x != 0; fclose(f); }
        if (pfCfg) { zt = calloc(MAXZ, sizeof *zt); for (int i = 0; i < NSTG && zt; i++) if (!(stg[i].buf = malloc(BLK))) { free(zt); zt = NULL; }
            Thread t = zt ? threadCreate(pfThread, NULL, 0x4000, 0x3F, 2, true) : NULL; if (!t) { pfCfg = 0; } }
        PSPNativeMemLog("[PREFETCH] zone_prefetch=%d", pfCfg); }
    if (pfCfg == 1 && state == 1) { static int loaded; if (!loaded) { loaded = 1; pfLoad(); pfReady = 1; }  /* game thread, once: no race with learning */
        if (!gameInRead) adopt(); }
}
int P3DS_RomCacheRead(FILE *fp, u32 romSize, u32 pos, void *dst, u32 len) {
    gameInRead = 1; gameLastRead = svcGetSystemTick();
    if (pfCfg == 1 && state == 1) adopt();
    int r = rcRead(fp, romSize, pos, dst, len);
    gameLastRead = svcGetSystemTick(); gameInRead = 0;
    return r;
}

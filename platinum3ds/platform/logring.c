/* run 35: non-blocking log. Callers (any thread, incl. audio/game) only format + memcpy into a RAM ring under a
   short LightLock; a lowest-priority thread appends to sdmc:/3ds/platinum3ds/log.txt every 100 ms through one
   persistent FILE. Whole lines are copied atomically, so lines never interleave. Full ring = line dropped (counted). */
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#define RING (512 * 1024)
static char ring[RING]; static u32 head, tail; static LightLock lk; static int init; static Thread th; static unsigned dropped;
static FILE *lf; static char out[64 * 1024];
/* run 45 log_level (config, default 1): 1 = boot info, errors/failures, [SPIKE], [WIDE]/[SAVE]/[MEM]/[OVERLAY] events,
   [PERF] every 10th (~10 s), [AUDIO-CLK] every 4th (~11 s); the per-frame/per-second detail lines are NOT written but
   kept in a 128 KiB in-RAM "detail" ring that is appended to log.txt on every synchronous flush (crash handler,
   hold, exit), so a crash still comes with the full recent diagnostics. 2 = everything (old behaviour). Any line
   containing fail/error/abort/crash/fatal/exception/assert is always written. */
static int logLevel = -1;
void P3DS_SetLogLevel(int v) { logLevel = v; }
int P3DS_LogLevel(void) { return logLevel < 0 ? 1 : logLevel; }
#define DRING (128 * 1024)
static char dring[DRING]; static u32 dhead;
static void ensureInit(void) { if (!init) { LightLock_Init(&lk); init = 1; }
    if (logLevel < 0) { logLevel = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) if (sscanf(l, "log_level=%d", &x) == 1) logLevel = x; fclose(f); } } }
static int hasCI(const char *s, const char *w) { size_t n = strlen(w); for (; *s; s++) { size_t k = 0; while (k < n && s[k] && ((s[k] | 32) == w[k])) k++; if (k == n) return 1; } return 0; }
static int isDetail(const char *line) {
    if (logLevel >= 2) return 0;
    static const char *const keep[] = { "fail", "error", "abort", "crash", "fatal", "exception", "assert", "overflow" };
    for (unsigned i = 0; i < sizeof keep / sizeof *keep; i++) if (hasCI(line, keep[i])) return 0;
    if (!strncmp(line, "[PERF]", 6)) { static unsigned n; return (n++ % 10) != 0; }
    if (!strncmp(line, "[PERF3]", 7)) { static unsigned n; return (n++ % 10) != 0; }
    if (!strncmp(line, "[AUDIO-CLK]", 11)) { static unsigned n; return (n++ % 4) != 0; }
    static const char *const det[] = { "[AUDIO-KEY]", "[AUDIO-OFF]", "[AUDIO-PIT]", "[AUDIO-SE]", "[AUDIO-SAS]", "[G3DIAG]",
        "[GPU3D] f=", "[GPU3D] frame:", "[GPU3D] cpu-clipped", "[G3PROF]", "[STEREO]", "[3DLAYER]", "[VDUMP]", "[2D] game state",
        "[2D] bottom_skip", "[3DS] present_skipbot:", "[3DS] tearfix=2: waited", "[TEX]", "[ROMCACHE] block hits", "[ROMFS-CACHE]", "[PREFETCH] persist" };
    for (unsigned i = 0; i < sizeof det / sizeof *det; i++) if (!strncmp(line, det[i], strlen(det[i]))) return 1;
    return 0;
}
volatile int P3DS_LogWriting; /* run 51: zone_prefetch pauses while the log is written to SD */
static void flushOnce(int lock) {
    P3DS_LogWriting = 1;
    for (;;) {
        u32 n = 0;
        if (lock) LightLock_Lock(&lk);
        u32 avail = head - tail; n = avail < sizeof out ? avail : sizeof out;
        for (u32 k = 0; k < n; k++) out[k] = ring[(tail + k) % RING];
        tail += n; unsigned d = dropped; dropped = 0;
        if (lock) LightLock_Unlock(&lk);
        if (!lf) lf = fopen("sdmc:/3ds/platinum3ds/log.txt", "w");
        if (!lf) return;
        if (d) fprintf(lf, "[LOG] %u lines dropped (ring full)\n", d);
        if (!n) break;
        fwrite(out, 1, n, lf);
        { /* run 74 log_max_kb (default 1024, 0 = no limit): past the limit log.txt becomes log_old.txt and a new one starts */
          static long wr; static int mx = -1; extern int P3DS_CfgInt(const char *, int); if (mx < 0) mx = P3DS_CfgInt("log_max_kb", 1024);
          wr += n; if (mx > 0 && wr > (long)mx * 1024) { fclose(lf); remove("sdmc:/3ds/platinum3ds/log_old.txt"); rename("sdmc:/3ds/platinum3ds/log.txt", "sdmc:/3ds/platinum3ds/log_old.txt");
            lf = fopen("sdmc:/3ds/platinum3ds/log.txt", "w"); wr = 0; if (!lf) return; fprintf(lf, "[LOG] log_max_kb=%d reached: older lines in log_old.txt\n", mx); } }
    }
    if (lf) fflush(lf);
    P3DS_LogWriting = 0;
}
static void writer(void *p) { (void)p; for (;;) { svcSleepThread(100000000LL); flushOnce(1); } }
void P3DS_LogLine(const char *line) {
    ensureInit(); u32 len = (u32)strlen(line);
    const int detail = isDetail(line);
    LightLock_Lock(&lk);
    if (detail) { for (u32 k = 0; k < len; k++) dring[(dhead + k) % DRING] = line[k]; dring[(dhead + len) % DRING] = '\n'; dhead += len + 1; LightLock_Unlock(&lk); return; }
    if (head - tail + len + 1 > RING) dropped++;
    else { for (u32 k = 0; k < len; k++) ring[(head + k) % RING] = line[k]; ring[(head + len) % RING] = '\n'; head += len + 1; }
    int start = !th; if (start) th = (Thread)1;
    LightLock_Unlock(&lk);
    if (start) { th = threadCreate(writer, NULL, 0x4000, 0x3F, -2, true); if (!th) th = (Thread)1; }
}
/* Synchronous flush for crash/exit/hold paths (lock attempted; written even if it can't be taken). */
void P3DS_LogFlushSync(void) { ensureInit(); int got = !LightLock_TryLock(&lk); if (got) LightLock_Unlock(&lk); flushOnce(got);
    if (lf && dhead) { /* log_level 1: append the recent suppressed detail lines (oldest first, from a line start) */
        u32 n = dhead < DRING ? dhead : DRING, start = dhead - n; u32 k = 0;
        if (dhead > DRING) while (k < n && dring[(start + k) % DRING] != '\n') k++;
        fprintf(lf, "[LOG] ---- last %u bytes of suppressed detail lines (log_level=%d) ----\n", n - k, logLevel);
        for (; k < n; k++) fputc(dring[(start + k) % DRING], lf);
        fprintf(lf, "[LOG] ---- end of detail ----\n"); fflush(lf); dhead = 0; } }

/* run 35: non-blocking log. Callers (any thread, incl. audio/game) only format + memcpy into a RAM ring under a
   short LightLock; a lowest-priority thread appends to sdmc:/3ds/platinum3ds/log.txt every 100 ms through one
   persistent FILE. Whole lines are copied atomically, so lines never interleave. Full ring = line dropped (counted). */
#include <3ds.h>
#include "crumbs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define RING (512 * 1024)
static char ring[RING]; static u32 head, tail; static LightLock lk; static int init; static Thread th; static unsigned dropped;
static FILE *lf; static char out[64 * 1024];
static int keepRun;
#define PRESERVE_RUN "[LOG] preserve reset diagnostics"
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
    if (!strncmp(line, "[PERF5]", 7)) { static unsigned n; return (n++ % 10) != 0; }
    if (!strncmp(line, "[PERF4]", 7)) { static unsigned n; return (n++ % 20) >= 2; }  /* two lines per second: keep 1 pair in 10 */
    if (!strncmp(line, "[AUDIO-CLK]", 11)) { static unsigned n; return (n++ % 4) != 0; }
    static const char *const det[] = { "[AUDIO-KEY]", "[AUDIO-OFF]", "[AUDIO-PIT]", "[AUDIO-SE]", "[AUDIO-SAS]", "[G3DIAG]",
        "[GPU3D] f=", "[GPU3D] frame:", "[GPU3D] cpu-clipped", "[G3PROF]", "[STEREO]", "[3DLAYER]", "[VDUMP]", "[2D] game state",
        "[2D] bottom_skip", "[3DS] present_skipbot:", "[3DS] tearfix=2: waited", "[TEX]", "[ROMCACHE] block hits", "[ROMFS-CACHE]", "[PREFETCH] persist" };
    for (unsigned i = 0; i < sizeof det / sizeof *det; i++) if (!strncmp(line, det[i], strlen(det[i]))) return 1;
    return 0;
}
volatile int P3DS_LogWriting; /* run 51: zone_prefetch pauses while the log is written to SD */
/* fsync batching: fflush alone leaves log.txt in the OS layer, so a HOME close or power loss could lose the last lines.
   The writer fsyncs at most every 2 s, right away after a [SAVE] line, and on every synchronous flush (crash/hold/exit). */
static volatile int logSyncNow; static int logPending; static u64 logSyncTick; static unsigned logSyncs, logSyncFails;
void P3DS_LogRequestSync(void) { logSyncNow = 1; } /* HOME/sleep: the writer thread fsyncs within 100 ms (no FILE use from the caller's thread) */
static void logSync(int force) {
    if (!lf) return; u64 now = svcGetSystemTick();
    if (!force && !logSyncNow && now - logSyncTick < 2ULL * SYSCLOCK_ARM11) return;
    logSyncNow = 0; logSyncTick = now; logPending = 0;
    if (fsync(fileno(lf))) { if (!logSyncFails++) fprintf(lf, "[LOG] fsync of log.txt FAILED (further failures counted silently)\n"); }
    else if (!logSyncs++) fprintf(lf, "[LOG] log.txt fsync active (every 2 s, after [SAVE] lines, on crash/exit)\n");
}
/* Preserve the last crash or flagged run across later clean boots. A clean footer and
   preservation marker are written after the suppressed details, even if the log rotated. */
static void keepCrashedLog(void) {
    FILE *f = fopen("sdmc:/3ds/platinum3ds/log.txt", "rb"); if (!f) return;
    char tail[4096]; size_t n = 0;
    if (!fseek(f, 0, SEEK_END)) { long sz = ftell(f); long from = sz > (long)sizeof tail - 1 ? sz - (long)sizeof tail + 1 : 0; if (sz > 0 && !fseek(f, from, SEEK_SET)) n = fread(tail, 1, sizeof tail - 1, f); }
    fclose(f); tail[n] = 0;
    /* 2026-10-29: a hang closed from HOME exits "clean", so also keep every previous run as log_prev.txt. */
    if (n && strstr(tail, "[3DS] exit: clean") && !strstr(tail, PRESERVE_RUN)) { remove("sdmc:/3ds/platinum3ds/log_prev.txt"); rename("sdmc:/3ds/platinum3ds/log.txt", "sdmc:/3ds/platinum3ds/log_prev.txt"); return; }
    if (!n) return;
    remove("sdmc:/3ds/platinum3ds/log_crash.txt"); remove("sdmc:/3ds/platinum3ds/log_crash_old.txt");
    rename("sdmc:/3ds/platinum3ds/log.txt", "sdmc:/3ds/platinum3ds/log_crash.txt");
    rename("sdmc:/3ds/platinum3ds/log_old.txt", "sdmc:/3ds/platinum3ds/log_crash_old.txt");
}
static void flushOnce(int lock) {
    P3DS_LogWriting = 1; int wrote = 0;
    for (;;) {
        u32 n = 0;
        if (lock) LightLock_Lock(&lk);
        u32 avail = head - tail; n = avail < sizeof out ? avail : sizeof out;
        for (u32 k = 0; k < n; k++) out[k] = ring[(tail + k) % RING];
        tail += n; unsigned d = dropped; dropped = 0;
        if (lock) LightLock_Unlock(&lk);
        if (!lf) { keepCrashedLog(); lf = fopen("sdmc:/3ds/platinum3ds/log.txt", "w"); }
        if (!lf) return;
        if (d) fprintf(lf, "[LOG] %u lines dropped (ring full)\n", d);
        if (!n) break;
        fwrite(out, 1, n, lf); wrote = 1;
        { /* run 74 log_max_kb (default 1024, 0 = no limit): past the limit log.txt becomes log_old.txt and a new one starts */
          static long wr; static int mx = -1; extern int P3DS_CfgInt(const char *, int); if (mx < 0) mx = P3DS_CfgInt("log_max_kb", 1024);
          wr += n; if (mx > 0 && wr > (long)mx * 1024) { fclose(lf); remove("sdmc:/3ds/platinum3ds/log_old.txt"); rename("sdmc:/3ds/platinum3ds/log.txt", "sdmc:/3ds/platinum3ds/log_old.txt");
            lf = fopen("sdmc:/3ds/platinum3ds/log.txt", "w"); wr = 0; if (!lf) return; fprintf(lf, "[LOG] log_max_kb=%d reached: older lines in log_old.txt\n", mx); } }
    }
    if (lf) { fflush(lf); if (wrote) logPending = 1; if (logPending || logSyncNow) logSync(!lock); }
    P3DS_LogWriting = 0;
}
/* 2026-11-22 (sound dead after ~15 soft resets): the writer used to be a detached thread that never ended, so it was still alive
   when the process exited on every soft reset. It now leaves its loop on logStop and hardExit joins it (P3DS_LogStopWriter). */
static volatile int logStop;
static void writer(void *p) { (void)p; P3DS_ThreadGuard("logwriter"); while (!logStop) { svcSleepThread(100000000LL); flushOnce(1); } }
static void useCount(const char *line);
static char tailBuf[8][112]; static unsigned tailN; /* last 8 written (non-detail) lines, shown on the crash screen */
volatile unsigned P3DS_LogLines, P3DS_LogBytes; /* run 113: every line, shown or hidden detail ([BATT] reports them per battle) */
static int preserveRunLine(const char *line) {
    return !strncmp(line, "[STALL]", 7) || !strncmp(line, "[WATCHDOG]", 10)
        || strstr(line, "thread start FAILED") || strstr(line, "did not stop")
        || strstr(line, "worker could not start") || strstr(line, "no sound grain")
        || strstr(line, "DSP NOT RUNNING") || strstr(line, "ndspInit has not returned");
}

void P3DS_LogLine(const char *line) {
    ensureInit(); u32 len = (u32)strlen(line);
    P3DS_LogLines++; P3DS_LogBytes += len + 1;
    useCount(line);
    const int detail = isDetail(line);
    LightLock_Lock(&lk);
    if (!detail) { char *t = tailBuf[tailN++ & 7]; strncpy(t, line, sizeof tailBuf[0] - 1); t[sizeof tailBuf[0] - 1] = 0; }
    if (detail) { for (u32 k = 0; k < len; k++) dring[(dhead + k) % DRING] = line[k]; dring[(dhead + len) % DRING] = '\n'; dhead += len + 1; LightLock_Unlock(&lk); return; }
    if (!__atomic_load_n(&keepRun, __ATOMIC_RELAXED) && preserveRunLine(line)) {
        __atomic_store_n(&keepRun, 1, __ATOMIC_RELAXED);
        logSyncNow = 1;
    }
    if (!strncmp(line, "[SAVE]", 6)) logSyncNow = 1;
    if (head - tail + len + 1 > RING) dropped++;
    else { for (u32 k = 0; k < len; k++) ring[(head + k) % RING] = line[k]; ring[(head + len) % RING] = '\n'; head += len + 1; }
    if (!th) {
        th = threadCreate(writer, NULL, 0x4000, 0x3F, -2, false);
        if (!th) { th = (Thread)1; __atomic_store_n(&keepRun, 1, __ATOMIC_RELAXED); }
    }
    LightLock_Unlock(&lk);
}
/* hardExit: stop and join the writer thread (up to 1 s). Later lines stay in the ring and P3DS_LogFlushSync writes them. */
int P3DS_LogStopWriter(void) {
    LightLock_Lock(&lk);
    Thread t = th;
    th = (Thread)1;
    logStop = 1;
    LightLock_Unlock(&lk);
    if (!t || t == (Thread)1) return 1;
    if (threadJoin(t, 1000000000ULL) != 0) return 0;
    threadFree(t);
    return 1;
}
/* Synchronous flush for crash/exit/hold paths (lock attempted; written even if it can't be taken). */
static void flushSync(int finish, int clean) { ensureInit(); int got = !LightLock_TryLock(&lk); if (got) LightLock_Unlock(&lk); flushOnce(got);
    if (lf && dhead) { /* log_level 1: append the recent suppressed detail lines (oldest first, from a line start) */
        u32 n = dhead < DRING ? dhead : DRING, start = dhead - n; u32 k = 0;
        if (dhead > DRING) while (k < n && dring[(start + k) % DRING] != '\n') k++;
        fprintf(lf, "[LOG] ---- last %u bytes of suppressed detail lines (log_level=%d) ----\n", n - k, logLevel);
        for (; k < n; k++) fputc(dring[(start + k) % DRING], lf);
        fprintf(lf, "[LOG] ---- end of detail ----\n"); fflush(lf); dhead = 0; }
    if (lf && finish) {
        if (__atomic_load_n(&keepRun, __ATOMIC_RELAXED)) fprintf(lf, PRESERVE_RUN "\n");
        if (clean) fprintf(lf, "[3DS] exit: clean\n");
    }
    if (lf) { fflush(lf); logSync(1); } }
void P3DS_LogFlushSync(void) { flushSync(0, 0); }
void P3DS_LogFinish(int clean) { flushSync(1, clean); }

/* ---- [USE] feature-usage counters, [CFG] header, crash-screen tail --------------------------------------------------
   Every feature that should leave evidence in log.txt writes one tagged line per EVENT (not per frame). Those lines are
   also counted here by prefix (useTab), so the clean-exit "[USE]" summary shows which features were exercised this run
   and which were never touched. Counting is by line prefix: keep the prefixes in step with the log lines (docs/HANDOFF.md). */
extern int P3DS_CfgInt(const char *, int);
extern void PSPNativeMemLog(const char *, ...);
typedef struct { const char *name, *prefix; volatile unsigned n; } UseEnt;
static UseEnt useTab[] = {
    { "g3_command_shortcut", "[G3PACK] direct=", 0 }, { "g3_bounds_config_events", "[G3BOUNDS] ", 0 }, { "second_screen_gpu_batches", "[GCB] frames=", 0 },
    { "battles", "[BATTLE] start", 0 }, { "battle_foe_deeper", NULL, 0 }, { "battle_anim_skips", "[BATTLE] move ", 0 },
    { "catch_info_panel", "[CATCH] panel opened", 0 }, { "shiny_alerts", "[CATCH] shiny alert", 0 },
    { "summary_iv_ev_pages", "[SUMMARY] IV", 0 }, { "summary_iv_ev_box_toggles", "[UI] summary IV/EV box", 0 },
    { "repel_checks", "[REPEL] repel check", 0 }, { "reusable_tm_uses", "[ITEM] reusable TM:", 0 },
    { "field_moves_without_hm", "[HM] move ", 0 }, { "no_wild_battle_skips", NULL, 0 }, { "pm_relearn", NULL, 0 }, { "pm_tutor", NULL, 0 }, /* counted by P3DS_Use (log is sampled) */
    { "tracker_publishes", "[TRACK] daycare", 0 }, { "contest_updates", "[CONTEST] ", 0 }, { "breed_tab_opened", "[BREED] pane open", 0 }, { "ribbon_checklist_opened", "[RIBBON] checklist", 0 }, { "celebration_effects", "[CELEB] effect", 0 }, { "hunt_counter_increments", "[HUNT] species", 0 }, { "shiny_found", "[HUNT] SHINY species", 0 }, { "shiny_rate_rolls", "[HUNT] shiny_rate roll", 0 }, { "hunt_soft_resets", "[HUNT] soft reset", 0 }, { "hunt_page_opened", "[CATCH] hunt page", 0 },
    { "wide_battle_entries", "[WIDE] battle entered", 0 }, { "wide_menu_screens", "[WIDE] menu screen", 0 },
    { "bottom_menu_taps", "[BMENU] tap ", 0 }, { "bottom_menu_game_touch", "[BMENU] game touch screen mode", 0 }, { "bottom_menu_actions", "[BMENU] request handled", 0 },
    { "minimap_mode_changes", "[MINIMAP] mode ", 0 }, { "minimap_new_cells", "[MINIMAP] new cell", 0 }, { "pgmap_wild_lists", "[PGMAP] card page wild", 0 }, { "pgmap_zoom", "[PGMAP] zoom ", 0 }, { "roamer_sprites", "[TRACK] roamer sprite", 0 }, { "phone_taps", "[PHONE] tap ", 0 }, { "pokegear_tab_switches", "[PHONE] panel ->", 0 },
    { "menu_button_taps", "[BTN] opened by menu button tap", 0 }, { "sprite_depth_quads", "[SPRDEPTH] sprite quads", 0 },
    { "sharp_text_lines", "[TEXT] sharp ", 0 }, { "gtext_lines", "[GTEXT] ", 0 }, { "quick_actions", "[QUICK] ", 0 }, { "settings_opened", "[APP] settings open", 0 },
    { "settings_changes", "[APP] saved ", 0 }, { "soft_resets", "[3DS] soft reset", 0 },
    { "saves_synced", "[SAVE] sync", 0 }, { "save_autobackups", "[SAVE] backup (after save)", 0 }, { "save_exports", "[SAVE] export:", 0 },
    { "audio_stall_fades", "[AUDIO] stall fade: no game", 0 }, { "audio_dsp_stalls", "[AUDIO] dsp stall", 0 },
    { "dc_flush_log_lines", "[HEAP] dc_flush", 0 },
    { "wireless_sessions", "[WIRELESS] auto -> LOCAL: 3DS local wireless up", 0 }, { "wireless_links", "[WIRELESS] joined parent", 0 },
    { "wireless_children", "[WIRELESS] child aid", 0 },
    { "ambient_weather", "[WEATHER] map ", 0 },
    { "seasonal_trees", "[WEATHER] tree canopy ", 0 },
    { "seasonal_particles", "[WEATHER] seasonal particles ", 0 },
    { "visible_wild_spawns", "[WILD] spawn ", 0 }, { "visible_wild_battles", "[WILD] contact ", 0 },
    { "fps60_pipe", "[FPS60] pipe ", 0 }, { "fps60_hold", "[FPS60] hold ", 0 }, { "fps60_strips", "[FPS60] fps60_strips=", 0 }, { "fps60_vram", "[FPS60] vram reserve ", 0 }, { "fps60_area", "[FPS60] area ", 0 },
    { "b2_sprites", 0, 0 }, /* ss3ds 0298: battles drawn with Black 2 sprites (b2sprites.c) */
    { "g3_xfcache", "[G3XF] ", 0 }, { "g3_prof", "[G3PROF] 60f ", 0 }, { "g3_wake", "[G3THR] g3_wake=", 0 }, { "g3_thr_check", "[G3THR] snap mismatch", 0 }, { "g3_fastxform", "[G3XF] g3_fastxform=", 0 }, { "g3_thr_texfence", "[G3THR] texfence:", 0 }, { "g3_thr_texsafe", "[G3THR] tex flash", 0 }, { "g3_stereo_share", "[G3STER] ", 0 }, { "present_pace", "[PACE] ", 0 }, { "g3_front_ov", "[FRONT] ", 0 }, { "g3_stereo_gpu", "[G3STER] g3_stereo_gpu=", 0 }, { "g3_front_bg", "[FRONT] g3_front_bg=", 0 },
};
#define NUSE ((int)(sizeof useTab / sizeof useTab[0]))
void P3DS_Use(const char *name) { for (int i = 0; i < NUSE; i++) if (!strcmp(useTab[i].name, name)) { __sync_fetch_and_add(&useTab[i].n, 1); return; } }
static void useCount(const char *line) {
    if (line[0] != '[') return;
    for (int i = 0; i < NUSE; i++) if (useTab[i].prefix && !strncmp(line, useTab[i].prefix, strlen(useTab[i].prefix))) { __sync_fetch_and_add(&useTab[i].n, 1); return; }
}
/* Clean exit (and crash): which features were used, and which never were. */
void P3DS_UseSummary(const char *why) {
    static char used[900], unused[700]; used[0] = unused[0] = 0; size_t u = 0, z = 0; int nu = 0;
    for (int i = 0; i < NUSE; i++) {
        if (useTab[i].n) { if (u < sizeof used - 40) u += (size_t)snprintf(used + u, sizeof used - u, " %s=%u", useTab[i].name, useTab[i].n); nu++; }
        else if (z < sizeof unused - 40) z += (size_t)snprintf(unused + z, sizeof unused - z, " %s", useTab[i].name);
    }
    static char b[1100];
    snprintf(b, sizeof b, "[USE] feature usage (%s): %d of %d features used:%s", why, nu, NUSE, nu ? used : " none"); P3DS_LogLine(b);
    snprintf(b, sizeof b, "[USE] not used this run:%s", z ? unused : " (all used)"); P3DS_LogLine(b);
}
unsigned P3DS_LogTailCount(void) { return tailN < 8 ? tailN : 8; }
const char *P3DS_LogTailLine(unsigned idx) { const unsigned n = P3DS_LogTailCount(); return idx < n ? tailBuf[(tailN - n + idx) & 7] : ""; }

/* Header: every key found in config.txt with its default and a DEAD / UNLISTED flag, then the effective value of every
   setting this build reads with P3DS_CfgInt. Written once at boot (main3ds.c) so a log always says what it ran with. */
extern int P3DS_CfgDefault(const char *key, int def); /* psp_ctru.c kCfgDefaults: the one table of real defaults; `def` below is only the fallback for keys it does not list */
typedef struct { const char *key; int def; const char *dead; } CfgEnt; /* def == INT_MIN: no single default / read elsewhere */
#define NODEF (-2147483647 - 1)
static const CfgEnt cfgTab[] = {
    { "g3_packets", 1, 0 }, { "g3_fast_bounds", 1, 0 }, { "gpu_compose_b", 1, 0 }, { "gpu_obj_affine", 1, 0 }, { "gpu_b_fades", 1, 0 }, { "title_clean_sides", 2, 0 },
    { "battle_monobj_hide", 1, 0 }, { "fps_tweaks", 7, 0 }, { "dc_flush_skip", 1, 0 }, { "fog_hole", 1, 0 }, { "sharp_text", 1, 0 }, { "sharp_game_text", 1, 0 }, { "pg_extras", 1, 0 }, { "wide_pokedex", 1, 0 }, { "wide_pc", 1, 0 },  { "bottom_menu", 1, 0 }, { "bottom_vramgen", 1, 0 }, { "cam_zoom", 0, 0 }, { "catch_overlay", 1, 0 },
    { "cheat_no_encounters", 0, 0 }, { "cheat_time", 0, 0 }, { "cpad_dpad", 1, 0 }, { "credits_splash", 1, 0 }, { "cstick_mode", 2, 0 },
    { "display_capture", 1, 0 }, { "fast_text", 2, 0 }, { "follower", 0, 0 }, { "follower_slot", 0, 0 }, { "follower_talk", 0, 0 },
    { "fps_counter", 1, 0 }, { "fps60_pipe", 1, 0 }, { "fps60_hold", 1, 0 }, { "fps60_strips", 1, 0 }, { "fps60_vram", 1, 0 }, { "fps60_area_dbg", 0, 0 }, { "fps60", 1, 0 }, { "g3_thread", 1, 0 }, { "g3_gputime_dbg", 0, 0 }, { "g3_xfcache", 1, 0 }, { "g3_prof", 0, 0 }, { "g3_wake", 1, 0 }, { "g3_fastxform", 1, 0 }, { "g3_thr_texfence", 2, 0 }, { "g3_thr_texsafe", 1, 0 }, { "g3_stereo_share", 1, 0 }, { "present_pace", 1, 0 }, { "g3_front_ov", 1, 0 }, { "g3_stereo_gpu", 1, 0 }, { "g3_front_bg", 1, 0 }, { "g3_wmode", 2, 0 }, { "g3_sharp", 3, 0 }, { "g3_smooth_dbg", 0, 0 },
    { "gpu_blend", 1, 0 }, { "gpu_compose", 2, 0 }, { "gpu_compose_stereo", 1, 0 }, { "gpu_compose_stereo_nf", 1, 0 }, { "gpu_direct3d", 1, 0 }, { "gpu_direct3d_wide", 1, 0 }, 
    { "ribbon_check", 1, 0 }, { "celebrations", 1, 0 }, { "birthdays", 0, 0 }, { "hunt_counter", 1, 0 }, { "hunt_stats", 1, 0 }, { "shiny_suite", 1, 0 }, { "shiny_rate", 0, 0 }, { "battery_warn", 1, 0 }, { "type_hint", 1, 0 }, { "dmg_check", 1, 0 }, { "hunt_async_save", 1, 0 }, { "lid_sound", 1, 0 }, { "log_max_kb", 1024, 0 }, { "menu_audio", 1, 0 }, { "menu_button", 0, 0 },
    { "menu_combo", 1, 0 }, { "repel_prompt", 1, 0 }, { "reset_skip_intro", 1, 0 }, { "reusable_tms", 1, 0 }, { "save_backup", 1, 0 },
    { "select_reset", 1, 0 }, { "settings_touch", 1, 0 }, { "soft_reset", 1, 0 }, { "speedcheck", 1, 0 }, { "splash_ms", 3000, 0 },
    { "stereo_2d", 3, 0 }, { "stereo_blobdiag", 1, 0 }, { "stereo_edgefix", 1, 0 }, { "stereo_fake", 0, 0 }, { "stereo_strength", 0, 0 },
    { "summary_ivs", 1, 0 }, { "tod_tint", 0, 0 }, { "ui_frame", 0, 0 }, { "ui_gamefont", 1, 0 }, { "vram_frame_gen", 1, 0 }, { "map_zoom", 0, 0 }, { "roamer_sprites", 1, 0 },
    { "wide_battle", 1, 0 }, { "wide_boxes", 1, 0 }, { "wide_edgefill", 1, 0 }, { "wide_fill", 0, 0 }, { "wide_layouts", 1, 0 }, { "wide_menu_screens", 1, 0 },
    { "b2_sprites", 0, 0 },
    { "wide_menus", 1, 0 }, { "wide_weather", 1, 0 }, { "ambient_weather", 0, 0 }, { "seasonal_trees", 1, 0 }, { "zl_run", 1, 0 }, { "auto_run", 0, 0 }, { "sprite_depth", 1, 0 }, { "bottom_bump", 1, 0 }, { "pm_shortcuts", 1, 0 }, { "panel_party", 1, 0 }, { "tracker", 1, 0 }, { "pg_flytap", 1, 0 }, { "pg_find", 1, 0 }, { "breed_tab", 1, 0 }, { "mobile_pc", 0, 0 }, { "mobile_pc_dev", 0, 0 }, { "pokewalker", 1, 0 }, { "gts_picker", 1, 0 }, { "gts_server", 0, 0 }, { "wifi_dns", NODEF, 0 }, { "wireless", 2, 0 }, { "wild_vis", 1, 0 }, { "wild_vis_count", 6, 0 }, { "wild_vis_shiny", 1, 0 }, { "wild_vis_replace", 0, 0 }, { "wild_vis_cries", 1, 0 }, { "wild_vis_alive", 2, 0 }, { "event_any_mon", 1, 0 }, { "contest_helper", 1, 0 }, { "steady_music", 1, 0 }, { "audio_even", 1, 0 }, { "audio_even_level", NODEF, 0 }, { "zr_item", 0, 0 }, { "log_level", 1, 0 }, { "pg_kanto", 0, 0 }, { "fast_tabs", 1, 0 },
    /* read with their own parsers: listed so they are not flagged, no single default shown */
    { "async2d", NODEF, 0 }, { "audio_gain", NODEF, 0 }, { "audio_interp", NODEF, 0 }, { "audio_lead", NODEF, 0 }, { "audio_limiter", NODEF, 0 },
    { "audio_rate", NODEF, 0 }, { "audio_stallfade", 0, 0 }, { "audio_volramp", NODEF, 0 }, { "bottom_refresh", NODEF, 0 }, { "bottom_skip", NODEF, 0 }, { "comp_excl", NODEF, 0 },
    { "core1_limit", NODEF, 0 }, { "cull3d", NODEF, 0 }, { "dump_frame", NODEF, 0 }, { "early_b", NODEF, 0 }, { "fast_conv", NODEF, 0 },
    { "fog3d", NODEF, 0 }, { "g3_boxtest_force", NODEF, 0 }, { "g3_wbuf", NODEF, 0 }, { "g3_wmax", NODEF, 0 }, { "gpu3d", NODEF, 0 }, { "gpu3d_async", NODEF, 0 },
    { "gpu3d_cpuclip", NODEF, 0 }, { "gpu3d_flipy", NODEF, 0 }, { "gpu3d_texflip", NODEF, 0 }, { "gpu3d_zinv", NODEF, 0 }, { "gpu_swizzle", NODEF, 0 },
    { "gpuscale", NODEF, 0 }, { "gpuscale_zoom", NODEF, 0 }, { "hotkey_swallow", NODEF, 0 }, { "land_fastload", NODEF, 0 }, { "mode0_border", NODEF, 0 },
    { "present_skipbot", NODEF, 0 }, { "rb_sync", NODEF, 0 }, { "res3d_lindepth", NODEF, 0 }, { "rom_cache_mb", NODEF, 0 }, { "rom_ra_kb", NODEF, 0 },
    { "save_anim", NODEF, 0 }, { "save_minms", NODEF, 0 }, { "save_seq", NODEF, 0 }, { "scalesrc_dbl", NODEF, 0 }, { "spike_ms", NODEF, 0 },
    { "split_top", NODEF, 0 }, { "steal_b", NODEF, 0 }, { "stereo2d_par", NODEF, 0 }, { "stereo_c1", NODEF, 0 }, { "stereo_clamp", NODEF, 0 },
    { "stereo_depth", NODEF, 0 }, { "tearfix", NODEF, 0 }, { "tex_pagecheck", NODEF, 0 }, { "tex_postvblank", NODEF, 0 }, { "touch_latch", NODEF, 0 },
    { "verbose", NODEF, 0 }, { "verbose_boot", NODEF, 0 }, { "vsync", NODEF, 0 }, { "wide_cull", NODEF, 0 }, { "wide_fadefix", NODEF, 0 },
    { "wide_fx", NODEF, 0 }, { "wide_haze", NODEF, 0 }, { "wideview", NODEF, 0 }, { "zone_prefetch", NODEF, 0 },
    /* generic settings rows read through the menu table (render3ds.cpp genGet) */
    { "hm_free", 1, 0 }, { "battle_anim_speed", 0, 0 }, { "text_hold_b", NODEF, 0 }, { "text_auto", NODEF, 0 }, { "g3_reuse", NODEF, 0 },
    { "cheat_money", NODEF, 0 }, { "cheat_catch", NODEF, 0 }, { "cheat_shiny", NODEF, 0 }, { "cheat_eggs", NODEF, 0 }, { "cheat_exp", NODEF, 0 }, { "cheat_candy", NODEF, 0 },
    { "follower_emotes", NODEF, 0 }, { "follower_sidefix", NODEF, 0 }, { "follower_debug", NODEF, 0 }, { "dw_depth", NODEF, "Platinum Distortion World only" },
    /* dead / ignored keys: still accepted in config.txt, nothing in this build reads them */
    { "settings_app", NODEF, "DEAD: the settings app is the only settings screen (ignored)" }, { "bottom_layout", NODEF, "DEAD: the 4x2 Bottom menu is deleted, JOHTO is the only layout (ignored)" }, { "bottom_map", NODEF, "DEAD: the INFO/WORLD/LOCAL panels are deleted (ignored)" }, { "g3_wcpu", NODEF, "DEAD: CPU 3D W-frames removed (ignored)" }, { "g3_wpersp", NODEF, "DEAD: CPU W raster is always perspective-correct (ignored)" },
    { "g3_wdepth", NODEF, "DEAD: replaced by g3_wmode (ignored since run 3)" }, { "scale", NODEF, "DEAD: old key, use gpuscale (ignored)" },
    { "cstick_poketch", NODEF, "DEAD: use cstick_mode (ignored)" },
    { "g3_shot", NODEF, "DEAD: debug option removed 2026-11-12" }, { "tex_trace", NODEF, "DEAD: debug option removed 2026-11-12" }, { "dump_eyes", NODEF, "DEAD: debug option removed 2026-11-12" }, { "vram_trace", NODEF, "DEAD: debug option removed 2026-11-12" }, { "heap_watch", NODEF, "DEAD: debug option removed 2026-11-12" }, { "lid_test", NODEF, "DEAD: debug option removed 2026-11-12" }, { "celeb_test", NODEF, "DEAD: debug option removed 2026-11-12" }, { "fps60_probe", NODEF, "DEAD: debug option removed 2026-11-12" }, { "g3_reuse_verify", NODEF, "DEAD: debug option removed 2026-11-12" },
    { "g3_gputime", NODEF, "DEAD: renamed g3_gputime_dbg 2026-11-18 (GPU pass timing is off for normal play)" }, { "fps60_area", NODEF, "DEAD: renamed fps60_area_dbg 2026-11-18" },
    { "g3_thr_check", NODEF, "DEAD: no reader" }, { "g3_slim_strips", NODEF, "DEAD: always on since #520" }, { "g3_slim_centre", NODEF, "DEAD: always on since #520" }, { "g3_statecache", NODEF, "DEAD: always on since #522" },
    { "g3_scissor", NODEF, "DEAD: always on (ignored)" }, { "g3_rowtrim", NODEF, "DEAD: always on since #523" }, { "fps60_noskip", NODEF, "DEAD: SKIP kept (#529)" }, { "present_defer", NODEF, "DEAD: always on (ignored)" },
    { "g3_overlap", NODEF, "DEAD: removed (#540)" }, { "g3_smooth", NODEF, "DEAD: renamed g3_smooth_dbg 2026-11-18 (title Lugia stays 1x for 30 fps)" }, { "scale_sharp", NODEF, "DEAD: no reader" },
    { "wild_vis_max", NODEF, 0 }, { "wild_vis_test", NODEF, 0 }, { "wild_vis_species", NODEF, "DEAD: Platinum only" },
    { "dw_debris", NODEF, 0 }, { "dw_unease", NODEF, 0 },
    { "dw_cry", NODEF, 0 }, { "dw_roll", NODEF, 0 },
    { "item_ping", NODEF, 0 }, { "touch_menu", NODEF, 0 }, { "bottom_strip", NODEF, 0 },
    { "cheat_pp", NODEF, 0 }, { "cheat_speed", NODEF, 0 },
    { "cheat_walk_walls", NODEF, 0 },
    { "follower_area_talk", NODEF, 0 }, { "follower_cry", NODEF, 0 }, { "follower_items", NODEF, 0 },
    { "follower_ballpop", NODEF, 0 }, { "follower_friend_talk", NODEF, 0 }, { "follower_idle", NODEF, 0 },
    { "follower_gap", NODEF, 0 }, { "follower_behind", NODEF, 0 },
    /* Platinum keys that have readers here but were never listed (they showed as UNLISTED in the log) */
    { "lower_menu", NODEF, 0 }, { "frame_pacing", NODEF, 0 }, { "gc_atlas16", NODEF, 0 }, { "rng_info", NODEF, 0 }, { "wide_gpu", NODEF, 0 }, { "wide_early", NODEF, 0 },
    { "wide_stereo", NODEF, 0 }, { "wide_split", NODEF, 0 }, { "battle_log", NODEF, 0 }, { "text_hold_b", NODEF, 0 }, { "text_auto", NODEF, 0 }, { "exp_share_all", NODEF, 0 },
    { "catch_swap", NODEF, 0 }, { "ach_toasts", NODEF, 0 }, { "stereo_depth", NODEF, 0 }, { "save_export", NODEF, 0 }, { "slot2", NODEF, 0 }, { "slot2_pak", NODEF, 0 },
    { "mobile_pc", NODEF, 0 }, { "mobile_pc_dev", NODEF, 0 }, { "b2_sprites", NODEF, 0 }, { "event_any_mon", NODEF, 0 }, { "gts_picker", NODEF, 0 },
};
#define NCFG ((int)(sizeof cfgTab / sizeof cfgTab[0]))
void P3DS_LogConfigHeader(const char *version) {
    PSPNativeMemLog("[CFG] ==== Platinum 3DS %s | built %s %s ====", version, __DATE__, __TIME__);
    FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); int nkeys = 0, ndead = 0, nunl = 0;
    if (!f) PSPNativeMemLog("[CFG] config.txt: none (all defaults)");
    else {
        char l[128];
        while (fgets(l, sizeof l, f)) {
            char *eq = strchr(l, '='); if (!eq || l[0] == '#' || l[0] == '\n') continue;
            *eq = 0; char *v = eq + 1; for (char *q = v; *q; q++) if (*q == '\n' || *q == '\r') *q = 0;
            const CfgEnt *e = NULL; for (int i = 0; i < NCFG; i++) if (!strcmp(cfgTab[i].key, l)) { e = &cfgTab[i]; break; }
            nkeys++;
            if (!e) { nunl++; PSPNativeMemLog("[CFG] %s=%s  UNLISTED: not a key this build is known to read (ignored or new)", l, v); }
            else if (e->dead) { ndead++; PSPNativeMemLog("[CFG] %s=%s  %s", l, v, e->dead); }
            else if (e->def != NODEF && atoi(v) != P3DS_CfgDefault(l, e->def)) PSPNativeMemLog("[CFG] %s=%s  (default %d) NON-DEFAULT", l, v, P3DS_CfgDefault(l, e->def));
            else PSPNativeMemLog("[CFG] %s=%s", l, v);
        }
        fclose(f); PSPNativeMemLog("[CFG] config.txt: %d keys, %d dead/ignored, %d unlisted", nkeys, ndead, nunl);
    }
    char b[300]; int n = 0, cnt = 0;
    for (int i = 0; i < NCFG; i++) {
        if (cfgTab[i].def == NODEF || cfgTab[i].dead) continue;
        n += snprintf(b + n, sizeof b - n, " %s=%d", cfgTab[i].key, P3DS_CfgInt(cfgTab[i].key, cfgTab[i].def));
        if (++cnt % 8 == 0 || n > 240) { PSPNativeMemLog("[CFG] effective:%s", b); n = 0; }
    }
    if (n) PSPNativeMemLog("[CFG] effective:%s", b);
}

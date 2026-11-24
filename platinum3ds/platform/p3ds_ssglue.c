/* p3ds_ssglue.c (2026-11 SoulSilver platform sync): Platinum's side of the hooks the shared platform layer expects.
 * The platform layer (render3ds.cpp, g3gpu.c, main3ds.c, wm_uds.c ...) now comes from the SoulSilver port. Where SoulSilver
 * answers one of these from its own game glue (ss3ds_glue.c, pspoke patches), Platinum answers here. A value that stays 0
 * means "that SoulSilver-only screen/state never happens in Platinum" (Pokewalker, Johto Pokegear panel, the HGSS title
 * layout), so the code that reads it keeps its pre-sync Platinum behaviour. Each one is listed in docs/SOULSILVER_SYNC.md. */
#include <stdio.h>
#include <string.h>
extern void PSPNativeMemLog(const char *, ...);
extern int P3DS_CfgInt(const char *, int);

/* screens SoulSilver detects from its overlay manager; none of them exist in Platinum */
int P3DS_InTitle;          /* HGSS title screen (Lugia title layers, title_clean_sides) */
int P3DS_InIntroMovie;     /* HGSS intro movie (no edge fill there) */
int P3DS_InPokewalker;     /* Pokewalker app */
int P3DS_InWideKind;       /* HGSS Pokedex / PC kinds for wide strips (Platinum uses its own wide layouts) */
volatile int P3DS_PanelBagOpening; /* Johto Pokegear panel */
int P3DS_WideEdgeClamp;    /* ss3ds_widescreens.c per-screen edge clamp table */

/* SoulSilver's ListMenu_ProcessInput retarget (patch 0227) publishes the running list so a missing cursor can be drawn.
   Platinum draws its own list cursors; frame 0 = no list, drawListCursor() returns at once. */
volatile struct { unsigned frame; unsigned char bgId, left, top, width, height, cursorX, upY, kind; unsigned short pos, above, total, shown; unsigned char font, padding; } P3DS_ListMenuNow;

/* g3gpu.c dark-cave fog: SoulSilver asks the field weather (12 = Flash). Platinum's dark caves use the same fog bit; without
   a game hook the answer is "not Flash", which is the pre-sync Platinum behaviour (fog always drawn). */
int SS3DS_FieldWeather(void) { return -1; }

/* g3gpu.c g3_reuse (3D FRAME REUSE): skip the draw+readback when a small 3D frame equals the previous one. SoulSilver default ON. */
int P3DS_G3ReuseCfg(void) { static int v = -1; if (v < 0) { v = P3DS_CfgInt("g3_reuse", 1) != 0; PSPNativeMemLog("[G3REUSE] g3_reuse=%d", v); } return v; }

/* main3ds.c builds followers.bin at boot when visible wild Pokemon are on (SoulSilver's wild_vis rows). Platinum's followers
   come from its own follower system, so the SoulSilver boot path is not used. */
int P3DS_WildVisCfg(int k) { (void)k; return 0; }

/* main3ds.c watchdog lines: SoulSilver wraps its ROM lock (SSRomLock) to name the holder. Platinum has no such lock. */
void P3DS_RomLockInfo(char *b, int n) { if (b && n > 0) snprintf(b, (size_t)n, "rom lock: not tracked on Platinum"); }
void P3DS_RomCallerInfo(char *b, int n) { if (b && n > 0) b[0] = 0; }

/* main3ds.c aptHookCb: SoulSilver's pspoke patch 0070 fsyncs the save file on HOME / sleep / exit. Platinum's backup.c already
   writes through on every save transaction (run 44 save notify); nothing is pending here. */
int PSPNative_SyncBackupIdle(const char *why) { (void)why; return 0; }

/* ss3ds_widescreens.c names the HGSS screen that has a wide layout (sharp game text asks for "naming"); Platinum has none */
const char *P3DS_WideScreenName;

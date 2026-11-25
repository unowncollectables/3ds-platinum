/* ss3ds wide_layouts (2026-11-12): which 2D screen is running, and its wide layout.
 *
 * The game runs every screen as an "application" (OverlayManager: Init / Main / Exit). OverlayManager_Run is wrapped
 * (link --wrap): the deepest manager run in a frame is the screen the player sees (a child app runs inside its parent's
 * Main). When that changes, the screen's layout parts (the Platinum port's WIDE_MOVE / WIDE_FILL / WIDE_OBJS rows, see
 * wideapi.c and widecomp.c) are handed to NitroPort_WideScreenParts, per engine. A screen with no layout has no parts:
 * it is drawn as the DS draws it, with the plain side strips.
 *
 * Layouts live in ss3ds_widelayouts.h (built in). For tuning without a rebuild, sdmc:/3ds/platinum3ds/wide_layouts.txt
 * overrides them when present:
 *     screen summary
 *     A move 0xF 0 192 0 120 -32      (engine A|B, kind, bgs, top, bottom, x0, x1, dx-or-col)
 *     A fill 0xF 0 192 88 152 119
 *     A objs 0 192 128 256 32         (objs / objstretch: top bottom x0 x1 dx)
 *     A shift3d 32
 * Display only: nothing here touches game state or the save. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void PSPNativeMemLog(const char *, ...);
extern void NitroPort_WideScreenParts(int engine, const short *rows, int count);

typedef int (*AppFn)(void *, int *);
typedef struct { AppFn init, exec, exit; int ovy; } AppTemplate; /* OverlayManagerTemplate; the manager starts with one */

#define WL_MOVE(bgs, top, bottom, x0, x1, dx)  { 1, (bgs), (top), (bottom), (x0), (x1), (dx), 0 }
#define WL_FILL(bgs, top, bottom, x0, x1, col) { 2, (bgs), (top), (bottom), (x0), (x1), 0, (col) }
#define WL_OBJS(top, bottom, x0, x1, dx)       { 3, 0, (top), (bottom), (x0), (x1), (dx), 0 }
#define WL_OBJS_STRETCH(top, bottom, x0, x1, dx) { 3, 0, (top), (bottom), (x0), (x1), (dx), 1 }
#define WL_COPY(bgs, top, bottom, x0, x1, col) { 5, (bgs), (top), (bottom), (x0), (x1), 0, (col) }
#define WL_WRAP(bgs, top, bottom)              { 6, (bgs), (top), (bottom), 0, 0, 0, 0 }
#define WL_SHIFT3D(dx)                         { 4, 0, 0, 0, 0, 0, (dx), 0 }
#define WL_SCREEN 0x10 /* with a part's bgs: x0..x1 are screen columns, not columns of the BG's picture */
typedef struct { const char *name; const short (*a)[8]; int na; const short (*b)[8]; int nb; } WlScreen;
#define WL_N(t) ((int)(sizeof(t) / sizeof((t)[0])))
#include "ss3ds_widelayouts.h"

/* Init functions that name a screen. Weak: a symbol the build does not have just never matches. */
#define APPS(X) \
    X(PokemonSummary_Init, "summary") X(Bag_Init, "bag") X(Pokedex_Init, "pokedex") X(PCBox_Init, "pcbox") \
    X(TrainerCard_Init, "trainercard") X(TrainerCardSignature_Init, "signature") X(OptionsMenu_Init, "options") \
    X(Pokegear_Init, "pokegear") X(PokegearMap_Init, "gearmap") X(PokegearPhone_Init, "gearphone") X(PokegearRadio_Init, "gearradio") \
    X(PokegearConfigure_Init, "gearconfig") X(FlyMap_Init, "flymap") X(OakSpeech_Init, "oakspeech") X(IntroMovie_Init, "intromovie") \
    X(TitleScreen_Init, "title") X(ChooseStarter_Init, "starter") X(Credits_Init, "credits") X(EasyChat_Init, "easychat") \
    X(HallOfFameShowcase_Init, "halloffame") X(RegisterHallOfFame_Init, "hofregister") X(MoveRelearner_Init, "relearner") \
    X(ApricornBox_Init, "apricorn") X(BerryPots_Init, "berrypots") X(BerryPotsApp_Initialize, "berrypots") X(ov59_02237D40, "apricorn") X(AlphPuzzle_Init, "alphpuzzle") X(UnownReport_Init, "unownreport") \
    X(VoltorbFlip_Init, "voltorbflip") X(PhotoAlbum_Init, "photoalbum") X(AccessoryPortrait_Init, "portrait") \
    X(Certificates_Init, "certificates") X(ViewRankingsApp_Init, "rankings") X(PalPad_Init, "palpad") X(MicTest_Init, "mictest") \
    X(TradeSequence_Init, "trade") X(WirelessTradeSelectMon_Init, "tradeselect") X(BugContestSwapMon_Init, "contestswap") \
    X(PokeathlonCourse_Init, "athloncourse") X(PokeathlonMedals_Init, "athlonmedals") X(PokeathlonCourseRecord_Init, "athlonrecord") \
    X(PokeathlonEventRecord_Init, "athlonevent") X(ScratchOffCards_Init, "scratch") X(GeonetGlobe_Init, "geonet") \
    X(LegendaryCinematic_Init, "cinematic") X(SafariAreaCustomizer_Init, "safaricustom") X(SafariDecoration_Init, "safarideco") \
    X(NamingScreenApp_Init, "naming") X(TrainerCardMainApp_Init, "trainercardmain") X(PartyMenu_Init, "party") X(Battle_Init, "battle")
#define X(fn, nm) extern int fn() __attribute__((weak));
APPS(X)
#undef X
extern const char gOverlayTemplate_PartyMenu[] __attribute__((weak));
extern const char gOverlayTemplate_PokemonSummary[] __attribute__((weak));
extern const char ov01_02206378[] __attribute__((weak)); /* the field */

static const char *nameOf(AppFn init) {
#define X(fn, nm) if (fn && (void *)init == (void *)fn) return nm;
    APPS(X)
#undef X
    if (gOverlayTemplate_PartyMenu && (void *)init == *(void *const *)gOverlayTemplate_PartyMenu) return "party";
    if (gOverlayTemplate_PokemonSummary && (void *)init == *(void *const *)gOverlayTemplate_PokemonSummary) return "summary";
    if (ov01_02206378 && (void *)init == *(void *const *)ov01_02206378) return "field";
    return 0;
}

/* wide_layouts.txt: the rows of one screen, read when that screen opens */
int P3DS_WideShow[2] = { 0x1F, 0x1F };
#define FILE_ROWS 64
static short fileRows[2][FILE_ROWS][8]; static int fileN[2];
static int fromFile(const char *name) {
    FILE *f = fopen("sdmc:/3ds/platinum3ds/wide_layouts.txt", "r"); if (!f) return 0;
    char l[160], cur[32] = ""; int found = 0; fileN[0] = fileN[1] = 0;
    while (fgets(l, sizeof l, f)) { long v[7] = { 0 };
        if (sscanf(l, " screen %31s", cur) == 1) { if (!strcmp(cur, name)) found = 1; continue; }
        if (strcmp(cur, name)) continue;
        char eng, kind[16]; char *p = l; int used = 0; if (sscanf(p, " %c %15s%n", &eng, kind, &used) < 2) continue; p += used;
        int nv = 0; while (nv < 7) { char *e; const long x = strtol(p, &e, 0); if (e == p) break; v[nv++] = x; p = e; }
        const int g = (eng == 'B' || eng == 'b') ? 1 : 0; if (fileN[g] >= FILE_ROWS) continue; short *r = fileRows[g][fileN[g]]; memset(r, 0, 8 * sizeof(short));
        if (!strcmp(kind, "show") && nv >= 1) { P3DS_WideShow[g] = (int)v[0]; continue; } /* tuning: draw only these layers (bit n = BG n, 16 = sprites) */
        if (!strcmp(kind, "move") && nv >= 6) { r[0] = 1; r[1] = (short)v[0]; r[2] = (short)v[1]; r[3] = (short)v[2]; r[4] = (short)v[3]; r[5] = (short)v[4]; r[6] = (short)v[5]; }
        else if (!strcmp(kind, "fill") && nv >= 6) { r[0] = 2; r[1] = (short)v[0]; r[2] = (short)v[1]; r[3] = (short)v[2]; r[4] = (short)v[3]; r[5] = (short)v[4]; r[7] = (short)v[5]; }
        else if ((!strcmp(kind, "objs") || !strcmp(kind, "objstretch")) && nv >= 5) { r[0] = 3; r[2] = (short)v[0]; r[3] = (short)v[1]; r[4] = (short)v[2]; r[5] = (short)v[3]; r[6] = (short)v[4]; r[7] = kind[4] == 's'; }
        else if (!strcmp(kind, "copy") && nv >= 6) { r[0] = 5; r[1] = (short)v[0]; r[2] = (short)v[1]; r[3] = (short)v[2]; r[4] = (short)v[3]; r[5] = (short)v[4]; r[7] = (short)v[5]; }
        else if (!strcmp(kind, "wrap") && nv >= 3) { r[0] = 6; r[1] = (short)v[0]; r[2] = (short)v[1]; r[3] = (short)v[2]; }
        else if (!strcmp(kind, "shift3d") && nv >= 1) { r[0] = 4; r[6] = (short)v[0]; }
        else continue;
        fileN[g]++; }
    fclose(f); return found;
}

const char *P3DS_WideScreenName = "";
/* 1 on screens whose picture reaches the screen edge in plain colour: the wide sides simply continue the edge column of each line
   (render3ds.cpp wideFill), no mirror and no blur, so nothing is doubled. Photo Album (not composable), Pokeathlon medals and
   course records, Hall of Fame showcase. */
int P3DS_WideEdgeClamp;
static void screenChanged(AppFn init) {
    const char *nm = nameOf(init);
    static unsigned nlog;
    if (nlog < 200) { nlog++; PSPNativeMemLog("[WIDE] screen: %s (init %p)", nm ? nm : "unnamed", (void *)init); }
    P3DS_WideScreenName = nm ? nm : "";
    P3DS_WideEdgeClamp = nm && (!strcmp(nm, "photoalbum") || !strcmp(nm, "athlonmedals") || !strcmp(nm, "athlonrecord") || !strcmp(nm, "halloffame"));
    NitroPort_WideScreenParts(0, 0, 0); NitroPort_WideScreenParts(1, 0, 0); P3DS_WideShow[0] = P3DS_WideShow[1] = 0x1F;
    if (!nm) return; /* the parts are kept whether or not the toggle is on; the renderer reads the toggle */
    if (fromFile(nm)) { PSPNativeMemLog("[WIDE] layout for %s from wide_layouts.txt: A %d rows, B %d rows", nm, fileN[0], fileN[1]);
        if (fileN[0]) NitroPort_WideScreenParts(0, &fileRows[0][0][0], fileN[0]); if (fileN[1]) NitroPort_WideScreenParts(1, &fileRows[1][0][0], fileN[1]); return; }
    for (int i = 0; i < WL_N(kWlScreens); i++) if (!strcmp(kWlScreens[i].name, nm)) { const WlScreen *s = &kWlScreens[i];
        if (s->na) NitroPort_WideScreenParts(0, &s->a[0][0], s->na); if (s->nb) NitroPort_WideScreenParts(1, &s->b[0][0], s->nb);
        PSPNativeMemLog("[WIDE] layout for %s: A %d rows, B %d rows", nm, s->na, s->nb); return; }
}

extern int __real_OverlayManager_Run(void *man);
int __wrap_OverlayManager_Run(void *man) {
    static int depth; static AppFn deepest, shown; static int deepestAt;
    if (!depth) { deepest = 0; deepestAt = -1; }
    if (man && depth > deepestAt) { deepestAt = depth; deepest = ((const AppTemplate *)man)->init; }
    depth++; const int r = __real_OverlayManager_Run(man); depth--;
    if (!depth && deepest != shown) { shown = deepest; screenChanged(deepest); }
    return r;
}

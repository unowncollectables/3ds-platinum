/* ss3ds: definitions the Platinum platform layer expects from Platinum's own game/native patches, and the
   game-facing pieces of pspoke's SoulSilver PSP main.c (dropped from the link, see full/Makefile). */
#include <3ds.h>
#include <string.h>

/* VRAM write tracking. Platinum stamps 4 KB VRAM pages from redirected game writes (pspoke vram_dirty.c +
   opttex_redirect.py); SoulSilver's objects are not redirected, so every page counts as written once per
   frame (called from main3ds.c's OS_WaitIrq wrap on each VBlank wait). g3soft's texture cache then always falls
   back to its byte compare: correct, a little slower. */
unsigned PSPNativeVramGen = 1, PSPNativeVramPageGen[0xA4], PSPNativeVramDirtyCalls;
void SS3DS_VramFrame(void)
{
    unsigned g = ++PSPNativeVramGen;
    for (unsigned i = 0; i < sizeof PSPNativeVramPageGen / sizeof PSPNativeVramPageGen[0]; i++) PSPNativeVramPageGen[i] = g;
}

/* ROM read timing for the [SPIKE] log (Platinum: romfs.c patch). */
unsigned long long P3DS_SpkRomTicks;
unsigned P3DS_SpkRomBlocks;

/* Platinum plays a Poke Ball sound on lid close/open from its game patch; no SoulSilver hook yet. */
void P3DS_PlayLidSE(int open) { (void)open; }

/* Platinum links pspoke's stackprobe.o; SoulSilver's app does not build it. */
void PSPNativeStackProbeInit(void) {}

/* From pspoke's SoulSilver main.c. */
char gPSPNativeSelfPath[256];
/* The original routine: does the communication manager exist. */
extern void *ssdata_unk_02037C94__021D4150;
__attribute__((weak)) int sub_02037D78(void) { return ssdata_unk_02037C94__021D4150 != 0; }

/* Wide view (and the other overworld-only extras) key on P3DS_InField. Platinum sets it from pspoke's "enter app"
   memprobe hook (last app = gFieldMapTemplate); SoulSilver's app has no such hook, so watch the overlay managers
   instead. ov01_02206378 is the field map's template (field_system.c FieldSystem_LoadFieldOverlayInternal); every
   other manager (battle, bag, title, ...) replaces it while it runs and clears the flag. Weak: if a pspoke bump
   renames it, the link still works and wide view simply stays off in the overworld. */
extern int P3DS_InField;
extern const char ov01_02206378[] __attribute__((weak));
extern const char gOverlayTemplate_Battle[] __attribute__((weak));
int P3DS_InBattle;
/* Bag_Main state 0 waits for its entrance fade. Expose readiness so the
 * in-panel layout can wait for the complete Bag screen. */
volatile int P3DS_PanelBagOpening;
static int bagPaletteActive;
extern int __real_Bag_Main(void *manager, int *state);
extern const unsigned char *OverlayManager_GetData(void *manager);
extern void __real_GXS_LoadBGPltt(const void *source, u32 offset, u32 size);
extern void PSPNativeMemLog(const char *, ...);
extern unsigned char s_HW_DB_BG_PLTT[];
extern void __real_GXS_LoadOBJPltt(const void *source, u32 offset, u32 size);
// The game retains these raw NCLR pointers, but their bytes can be
// overwritten while the Bag is open. Keep a clean copy for pocket uploads.
static const unsigned char *bagPaletteSource[3];
static unsigned char bagPalette[3][512] __attribute__((aligned(4)));

void __wrap_GXS_LoadBGPltt(const void *source, u32 offset, u32 size) {
    // ov15_021FD404 uploads one of eight pocket palettes to BG banks 0–3.
    if (bagPaletteActive && (offset == 0 || offset == 32 || offset == 96) &&
        size == (offset == 32 ? 64u : 32u)) {
        for (unsigned i = 0; i < 2; i++) {
            if (!bagPaletteSource[i]) continue;
            const uintptr_t pocket = (uintptr_t)source - (uintptr_t)bagPaletteSource[i];
            if (pocket > 7 * 32 || (pocket & 31)) continue;
            const unsigned char *expected = bagPalette[i] + pocket;
            if (memcmp(expected, source, size)) {
                static unsigned mismatches;
                if (mismatches++ < 24)
                    PSPNativeMemLog("[BAGVRAM] pocket palette source changed: set=%u pocket=%u bank=%u",
                        i, (unsigned)pocket / 32, (unsigned)offset / 32);
            }
            memcpy(s_HW_DB_BG_PLTT + offset, expected, size);
            return;
        }
    }
    __real_GXS_LoadBGPltt(source, offset, size);
}

void __wrap_GXS_LoadOBJPltt(const void *source, u32 offset, u32 size) {
    // ov15_02200030 uploads the selected pocket colour and a common colour.
    if (bagPaletteActive && bagPaletteSource[2]) {
        const uintptr_t bank = (uintptr_t)source - (uintptr_t)bagPaletteSource[2];
        if ((bank == 256 && offset == 0 && size == 256) ||
            (bank <= 7 * 32 && !(bank & 31) && bank == offset && size == 32))
            source = bagPalette[2] + bank;
    }
    __real_GXS_LoadOBJPltt(source, offset, size);
}

int __wrap_Bag_Main(void *manager, int *state) {
    if (!bagPaletteActive) {
        const unsigned char *data = OverlayManager_GetData(manager);
        // Bag_Init retains the two background and one object NCLR descriptors.
        static const unsigned descriptorOffsets[] = {0x694, 0x698, 0x6a0};
        for (unsigned i = 0; i < sizeof descriptorOffsets / sizeof *descriptorOffsets; i++) {
            const unsigned char *descriptor;
            u32 size;
            memcpy(&descriptor, data + descriptorOffsets[i], sizeof descriptor);
            bagPaletteSource[i] = NULL;
            if (!descriptor) continue;
            memcpy(&size, descriptor + 8, sizeof size);
            const unsigned bytes = i == 2 ? 512 : 9 * 32;
            if (size >= bytes) {
                memcpy(&bagPaletteSource[i], descriptor + 12, sizeof bagPaletteSource[i]);
                memcpy(bagPalette[i], bagPaletteSource[i], bytes);
            }
        }
    }
    bagPaletteActive = 1;
    const int finished = __real_Bag_Main(manager, state);
    if (finished) bagPaletteActive = 0;
    if (P3DS_PanelBagOpening && (*state != 0 || finished))
        P3DS_PanelBagOpening = 0;
    return finished;
}

int P3DS_InTitle;
int P3DS_InPokewalker;
extern const char ov112_App_MainMenu_SelectOption_ConnectToPokewalker[];
extern const char gApplication_TitleScreen[] __attribute__((weak));
/* 2026-11-18: the start-up intro movie (Ho-Oh/Lugia scenes before the title). render3ds.cpp leaves its wide side strips dark
   instead of the mirrored, blurred picture edge (wideFill), which looked like smeared edges there. */
int P3DS_InIntroMovie;
extern const char gApplication_IntroMovie[] __attribute__((weak));
/* wide_menu_screens: party menu, bag and Pokemon summary. Weak: a renamed symbol just leaves the toggle without effect;
   the first template changes are logged so the log shows which pointers were seen. */
int P3DS_InWideMenu;
extern const char gOverlayTemplate_PartyMenu[] __attribute__((weak));
extern const char gOverlayManagerTemplate_Bag[] __attribute__((weak));
extern const char gOverlayTemplate_PokemonSummary[] __attribute__((weak));
/* run 18: wide_pokedex / wide_pc. HGSS launches both from static templates in launch_application.c
   (sOverlayTemplate_Pokedex = {Pokedex_Init,..,OVY_18}, sOverlayTemplate_PCBox = {PCBox_Init,..,OVY_14}), so they are
   recognised by their Init function. P3DS_InWideKind: 1 party/bag/summary, 2 Pokedex, 3 PC boxes, 0 other. */
int P3DS_InWideKind;
extern int Pokedex_Init() __attribute__((weak));
extern int Bag_Init() __attribute__((weak));
extern int PokemonSummary_Init() __attribute__((weak));
extern int PCBox_Init() __attribute__((weak));
extern void *__real_OverlayManager_New(const void *template, void *parentWork, int heapID);
void *__wrap_OverlayManager_New(const void *template, void *parentWork, int heapID)
{
    const int field = template && template == (const void *)ov01_02206378;
    {int title=gApplication_TitleScreen&&template==(const void*)gApplication_TitleScreen;if(title!=P3DS_InTitle){extern void PSPNativeMemLog(const char*,...);PSPNativeMemLog("[TITLE] overlay active=%d",title);}P3DS_InTitle=title;}
    {const int im=gApplication_IntroMovie&&template==(const void*)gApplication_IntroMovie;if(im!=P3DS_InIntroMovie){extern void PSPNativeMemLog(const char*,...);PSPNativeMemLog("[TITLE] intro movie active=%d (wide sides dark, no edge blur)",im);}P3DS_InIntroMovie=im;}
    if (field != P3DS_InField) { extern void PSPNativeMemLog(const char *, ...); PSPNativeMemLog("[3DS] field %d", field); }
    P3DS_InField = field;
    const int pokewalker = template == (const void *)ov112_App_MainMenu_SelectOption_ConnectToPokewalker;
    if (pokewalker != P3DS_InPokewalker) {
        P3DS_InPokewalker = pokewalker;
        PSPNativeMemLog("[PWALKER] CPU screen composition %s", pokewalker ? "enabled" : "ended");
    }
    /* wide_battle: the battle overlay manager (gOverlayTemplate_Battle, overlay 12) marks the battle; the field
       manager returning clears it. Sub-screens opened from the battle (bag, party) keep the flag. */
    if (template) {
        extern int P3DS_InBattle;
        const int battle = template == (const void *)gOverlayTemplate_Battle ||
                           (!gOverlayTemplate_Battle && ((const int *)template)[3] == 12);
        static unsigned seen;
        if (!field && seen < 24) { seen++; extern void PSPNativeMemLog(const char *, ...); PSPNativeMemLog("[3DS] overlay manager %p ovy %d battle %d", template, ((const int *)template)[3], battle); }
        {
            const char *nm = (gOverlayTemplate_PartyMenu && template == (const void *)gOverlayTemplate_PartyMenu) ? "party" :
                             (gOverlayManagerTemplate_Bag && template == (const void *)gOverlayManagerTemplate_Bag) ? "bag" :
                             (Bag_Init && ((void *const *)template)[0] == (void *)Bag_Init) ? "bag" : /* Bag_LaunchApp copies a static template, so match its Init */
                             (gOverlayTemplate_PokemonSummary && template == (const void *)gOverlayTemplate_PokemonSummary) ? "summary" :
                             (PokemonSummary_Init && ((void *const *)template)[0] == (void *)PokemonSummary_Init) ? "summary" :
                             (Pokedex_Init && ((void *const *)template)[0] == (void *)Pokedex_Init) ? "pokedex" :
                             (PCBox_Init && ((void *const *)template)[0] == (void *)PCBox_Init) ? "pcbox" : 0;
            static const char *lastNm; static unsigned logged;
            if (nm != lastNm && logged < 64) {
                extern void PSPNativeMemLog(const char *, ...);
                logged++;
                PSPNativeMemLog("[WIDE] menu screen %s -> %s (template %p, battle %d, field %d; symbols party %p bag %p summary %p)",
                                lastNm ? lastNm : "none", nm ? nm : "none", template, battle, field,
                                (const void *)gOverlayTemplate_PartyMenu, (const void *)gOverlayManagerTemplate_Bag,
                                (const void *)gOverlayTemplate_PokemonSummary);
            }
            lastNm = nm;
            /* run 132 panel_party: keep the panel up across party/summary/bag handoffs. Hook 0201/0203 (party_menu.c) sets On from Want and no
               longer clears it; bag and summary are lifted code with no hook point, so they claim a pending Want here. The field or a battle
               starting ends the panel (the render also clears On once InField is set). */
            if (nm && nm[0] != 'p') {
                extern volatile int P3DS_PanelPartyWant, P3DS_PanelPartyOn; extern void PSPNativeMemLog(const char *, ...);
                if (P3DS_PanelPartyWant && !P3DS_PanelPartyOn) { P3DS_PanelPartyOn = 1; PSPNativeMemLog("[PANEL] %s open: On=1 (in JOHTO panel)", nm); }
                else if (P3DS_PanelPartyOn) PSPNativeMemLog("[PANEL] %s open: On kept (chained from the panel)", nm);
                P3DS_PanelPartyWant = 0;
            }
            {
                extern volatile int P3DS_PanelPartyOn;
                P3DS_PanelBagOpening = P3DS_PanelPartyOn && nm && !strcmp(nm, "bag");
            }
            if (field || battle) { extern volatile int P3DS_PanelPartyOn; P3DS_PanelPartyOn = 0; }
            P3DS_InWideKind = !nm ? 0 : nm[0] == 'p' && nm[1] == 'o' ? 2 : nm[0] == 'p' && nm[1] == 'c' ? 3 : 1;
            P3DS_InWideMenu = P3DS_InWideKind == 1;
        }
        /* [BATTLE] start / end: one line each per battle (counted in the clean-exit [USE] summary) */
        static unsigned battleN; static unsigned long long battleT0;
        if (battle) { if (!P3DS_InBattle) { extern void PSPNativeMemLog(const char *, ...); battleT0 = svcGetSystemTick(); PSPNativeMemLog("[BATTLE] start #%u", ++battleN); } P3DS_InBattle = 1; }
        else if (field) { if (P3DS_InBattle) { extern void PSPNativeMemLog(const char *, ...); PSPNativeMemLog("[BATTLE] end #%u after %u ms", battleN, (unsigned)((svcGetSystemTick() - battleT0) / CPU_TICKS_PER_USEC / 1000)); } P3DS_InBattle = 0; }
    }
    return __real_OverlayManager_New(template, parentWork, heapID);
}

/* Counted by Platinum's snd_bank.c patch (notes whose wave is missing) and logged by sas_out.c's [AUDIO-SE] line;
   SoulSilver's snd_bank is unpatched, so it stays 0. */
unsigned P3DS_NoteNoWave;

/* Battle start crash: ov01_021EFF90 (the encounter transition's brightness SysTask, overlay_01_021EFB38.s) passes the
   raw DS addresses 0x0400006C / 0x0400106C as the register pointer to libntr's GXx_{Set,Get}MasterBrightness_.
   pspoke's asm translator only rewrites loads and stores (ssio_*), not pointer arguments, so the C function stored
   straight to 0x0400106C, which is unmapped on 3DS (data abort). Map those addresses to the port's register
   variables here; any other DS I/O address is logged and ignored instead of faulting. */
extern volatile u16 s_reg_GX_MASTER_BRIGHT, s_reg_GXS_DB_MASTER_BRIGHT;
static volatile u16 *SS3DS_BrightReg(volatile u16 *reg, const char *fn, void *ra)
{
    static unsigned logged;
    const u32 a = (u32)reg;
    if ((a >> 24) != 0x04u) return reg;
    volatile u16 *mapped = a == 0x0400006Cu ? &s_reg_GX_MASTER_BRIGHT : a == 0x0400106Cu ? &s_reg_GXS_DB_MASTER_BRIGHT : 0;
    if (logged < 8) {
        extern void PSPNativeMemLog(const char *, ...);
        logged++;
        PSPNativeMemLog("[3DS] %s: raw DS register %08lx from %p -> %s", fn, (unsigned long)a, ra,
                        mapped ? "mapped" : "UNMAPPED, ignored");
    }
    return mapped;
}
extern void __real_GXx_SetMasterBrightness_(volatile u16 *reg, int brightness);
void __wrap_GXx_SetMasterBrightness_(volatile u16 *reg, int brightness)
{
    if ((reg = SS3DS_BrightReg(reg, "SetMasterBrightness", __builtin_return_address(0)))) __real_GXx_SetMasterBrightness_(reg, brightness);
}
extern int __real_GXx_GetMasterBrightness_(volatile u16 *reg);
int __wrap_GXx_GetMasterBrightness_(volatile u16 *reg)
{
    return (reg = SS3DS_BrightReg(reg, "GetMasterBrightness", __builtin_return_address(0))) ? __real_GXx_GetMasterBrightness_(reg) : 0;
}

/* Doubled battle Pokemon in 3D (Platinum 3DS run 55, same game code in SoulSilver): during a send-out, the encounter
   start and the ball effect the game makes 2D OBJ copies of every visible battle Pokemon, only so the ball-seal
   particles can layer with them. On the DS they sit pixel-exactly over the 3D sprite quads; on the 3DS the 3D sprite
   gets stereo depth while the 2D copy stays at screen depth, so the Pokemon already out shows twice until the ball
   opens. ov07_0221FDFC is the "copies for all battlers" call (BattleMonOBJData_NewAll in pokeplatinum; it tail-calls
   ov07_0221FB90 with battler 0xFF); its result holds the four ManagedSprite pointers at +0x24. The translated asm
   inlines it into every caller, so a linker --wrap never saw it (run: nm/map had no such symbol). Instead
   pspoke-patches/0017 retargets those calls to this function, which does what FDFC did and then hides the copies;
   the 3D sprites stay. battle_monobj_hide=0 = old behaviour. */
extern unsigned int ov07_0221FB90(unsigned int battleSystem, unsigned int heapID, unsigned int battler, unsigned int unused);
extern void ManagedSprite_SetDrawFlag(void *managedSprite, int draw);
extern int P3DS_CfgInt(const char *key, int def);
unsigned int SS3DS_BattleMonOBJ_NewAll(unsigned int battleSystem, unsigned int heapID)
{
    extern void PSPNativeMemLog(const char *, ...);
    static int hide = -1;
    static unsigned calls;
    unsigned int d = ov07_0221FB90(battleSystem, heapID, 0xFF, 0);
    if (hide < 0) {
        hide = P3DS_CfgInt("battle_monobj_hide", 1) != 0;
        PSPNativeMemLog("[BATTLE] battle_monobj_hide=%d (hide the 2D copies of battle Pokemon in send-outs)", hide);
    }
    int hidden = 0;
    if (hide && d) {
        void **sprites = (void **)((char *)(uintptr_t)d + 0x24);
        for (int i = 0; i < 4; i++)
            if (sprites[i]) { ManagedSprite_SetDrawFlag(sprites[i], 0); hidden++; }
    }
    if (calls++ < 16) PSPNativeMemLog("[BATTLE] mon OBJ copies made (call %u): result %08x, %d hidden", calls, d, hidden);
    return d;
}

/* 2026-11-04 (Nathan: "when you enter a battle you can see 2 enemy sprites while they do their animations"): the wild /
   trainer opponent intro (ov12_02260CDC, battle_controller_opponent) makes the same kind of 2D OBJ copy for ONE battler
   (ov07_0221FB90 with that battler) while the 3D sprite is drawn too, so the foe showed twice during its intro animation in
   3D (the copy has no stereo depth). call-retargets.json (pspoke patch 0216) routes that call here; same rule as NewAll above. */
unsigned int SS3DS_BattleMonOBJ_One(unsigned int battleSystem, unsigned int heapID, unsigned int battler, unsigned int unused)
{
    extern void PSPNativeMemLog(const char *, ...);
    static int hide = -1; static unsigned calls;
    unsigned int d = ov07_0221FB90(battleSystem, heapID, battler, unused);
    if (hide < 0) hide = P3DS_CfgInt("battle_monobj_hide", 1) != 0;
    int hidden = 0;
    if (hide && d) {
        void **sprites = (void **)((char *)(uintptr_t)d + 0x24);
        for (int i = 0; i < 4; i++)
            if (sprites[i]) { ManagedSprite_SetDrawFlag(sprites[i], 0); hidden++; }
    }
    if (calls++ < 16) PSPNativeMemLog("[BATTLE] foe intro OBJ copy (battler %u, call %u): result %08x, %d hidden", battler, calls, d, hidden);
    return d;
}

/* Zone prefetch (romcache3ds.c) learns which ROM blocks each map needs and reads the neighbours' blocks ahead of time
   on an idle core, but only if told when the map changes. Platinum hooks its map change; in SoulSilver the field map
   loader (fieldmap.c: AreaDataManager_Alloc(MapHeader_GetAreaDataBank(location->mapId))) is the one place every
   map load goes through, warps and walking across a map border alike. Without this hook the [PREFETCH] lines never
   appeared in log.txt and every map entry read its data from the SD card cold (100-400 ms frame spikes, audio holds). */
extern void P3DS_ZoneEnter(int zone);
extern unsigned char __real_MapHeader_GetAreaDataBank(unsigned mapId);
unsigned char __wrap_MapHeader_GetAreaDataBank(unsigned mapId)
{
    static unsigned last = 0xFFFFFFFFu;
    if (mapId != last) { last = mapId; P3DS_ZoneEnter((int)mapId); }
    return __real_MapHeader_GetAreaDataBank(mapId);
}

/* Play time (the TIME on the Continue screen and the trainer card) stayed 0:00. The game adds seconds from DS timer 3
   (src/timer3.c: sub_02025488 reads the count, sub_02025504 turns ticks into seconds, sub_020183B0 feeds
   AddIGTSeconds every frame). That version counts on the timer's IRQ and register, which the port does not emulate, so
   the count never moved. pspoke's PSP build swaps in timer3_psp.c (the PSP system clock); this is the same idea on the
   3DS system tick. DS timer 3 runs at the 33513982 Hz bus clock / 64, and sub_02025504 divides by that rate. */
extern void PSPNativeMemLog(const char *, ...);
static u64 ss3dsT3Start;
u64 __wrap_sub_02025488(void)
{
    static u64 nextLog;
    const u64 now = svcGetSystemTick();
    if (!ss3dsT3Start) ss3dsT3Start = now;
    const u64 e = now - ss3dsT3Start, sec = e / SYSCLOCK_ARM11, rem = e % SYSCLOCK_ARM11;
    const u64 ticks = (sec * 33513982u + rem * 33513982u / SYSCLOCK_ARM11) / 64u;
    if (sec >= nextLog) {
        PSPNativeMemLog("[IGT] timer3 clock %llu s since start, %llu DS timer ticks (play time counts from this)", sec, ticks);
        nextLog = sec + 60;
    }
    return ticks;
}
u64 __wrap_sub_020254FC(void) { return __wrap_sub_02025488(); }

/* ZL run toggle (zl_run, Settings > Controls, default off). In SoulSilver the "is B held" test that decides running
   (sub_0205DE88(avatar, keys): keys & PAD_BUTTON_B) lives in translated asm, called from the walk start and the
   movement-state pick in unk_0205CB48. pspoke-patches/0050 retargets both calls here. With the toggle on (ZL flips
   P3DS_RunToggle in psp_ctru.c), holding B walks instead of runs. The running shoes check stays in the game, and
   nothing is written to the save. */
extern volatile int P3DS_RunToggle;
extern int P3DS_CtlZl;
volatile int P3DS_RunLast; /* 0218: the last run answer (1 = running), read by the alive wild Pokemon to tell sneaking from running */
unsigned int SS3DS_RunHeld(unsigned int avatar, unsigned int keys)
{
    extern void PSPNativeMemLog(const char *, ...);
    static int lastTog = -1, lastHeld = -1, calls;
    (void)avatar;
    unsigned int held = (keys & 2u) ? 1u : 0u; /* B bit (sub_0205DE88 is not a linkable symbol: run 53 link error) */
    unsigned int out = P3DS_RunToggle ? !held : held; /* run 10: the JOHTO layout's RUN button sets it too, so no zl_run gate here (ZL itself still needs zl_run=1) */
    P3DS_RunLast = (int)out;
    calls++;
    /* [INPUT] first 12 calls, then every change of toggle or B: setting, raw keys and the run answer (run 52: "run stuck on while it says off") */
    if (calls <= 12 || P3DS_RunToggle != lastTog || (int)held != lastHeld) {
        lastTog = P3DS_RunToggle;
        lastHeld = (int)held;
        PSPNativeMemLog("[INPUT] run test #%d: auto_run toggle %d, keys 0x%x (B %u) -> run %u", calls, P3DS_RunToggle, keys, held, out);
    }
    return out;
}

/* IV / EV on the summary screen (summary_ivs, Settings > Controls, default off; Y shows / hides the box). The summary
   gathers a Pokemon's data in sub_0208981C, which ends with ReleaseMonLock(mon, lock) while the mon is still
   decrypted. pspoke-patches/0051 retargets that call here: copy the 12 values for the port's read-only overlay
   (render3ds.cpp drawSumIV), then release as the game would. PokemonSummary_Main's per-frame sub_0208B278 is
   retargeted to SS3DS_SummaryTick, which tells the overlay the summary is open. Nothing is written to the Pokemon. */
extern unsigned int GetMonData(unsigned int mon, int param, void *unused);
extern unsigned int ReleaseMonLock(unsigned int mon, unsigned int locked);
extern int P3DS_SummaryIvsCfg(void);
extern volatile int P3DS_SumIV[6], P3DS_SumEV[6], P3DS_SumAlive;
#include "ribbons.h"
unsigned int SS3DS_SummaryRelease(unsigned int mon, unsigned int locked)
{
    extern void PSPNativeMemLog(const char *, ...);
    static const int ivId[6] = { 70, 71, 72, 74, 75, 73 };   /* HP Atk Def SpA SpD Spe (MON_DATA_*_IV) */
    static const int evId[6] = { 13, 14, 15, 17, 18, 16 };   /* MON_DATA_*_EV */
    if (P3DS_SummaryIvsCfg()) {
        for (int i = 0; i < 6; i++) {
            P3DS_SumIV[i] = (int)GetMonData(mon, ivId[i], 0);
            P3DS_SumEV[i] = (int)GetMonData(mon, evId[i], 0);
        }
        PSPNativeMemLog("[SUMMARY] IV %d/%d/%d/%d/%d/%d EV %d/%d/%d/%d/%d/%d", P3DS_SumIV[0], P3DS_SumIV[1], P3DS_SumIV[2],
                        P3DS_SumIV[3], P3DS_SumIV[4], P3DS_SumIV[5], P3DS_SumEV[0], P3DS_SumEV[1], P3DS_SumEV[2],
                        P3DS_SumEV[3], P3DS_SumEV[4], P3DS_SumEV[5]);
    }
    if (P3DS_RibbonOn()) P3DS_RibbonCapture(mon, (unsigned (*)(unsigned, int, void *))GetMonData); /* ribbon_check: read-only, ribbons.c */
    return ReleaseMonLock(mon, locked);
}

extern unsigned int sub_0208B278(unsigned int data);
unsigned int SS3DS_SummaryTick(unsigned int data)
{
    P3DS_SumAlive = 4;
    return sub_0208B278(data);
}

/* run 3: sub_02038D90 (comm "safe to cancel save / stop comm" check, unk_02037C94.s) was only an [SS-MISSING] trap, so a
   soft reset (L+R+START+SELECT -> DoSoftReset) or a communication-error screen aborted (hardware run 2: abort() from the
   trap at 0x12ed10, map 33). Hand port. With no comm session (_021D4150 == NULL, always the case offline) the DS returns
   TRUE after sub_02037988 sets the flag at +0x6B9 of the _021D4140+8 object. The comm-active branch (link/wireless, not
   supported in this port) only tears down the comm task; here it logs and returns TRUE so the save is cancelled safely. */
extern unsigned char ssdata_unk_02035900__021D4140[];
extern void *ssdata_unk_02037C94__021D4150;
extern void PSPNativeMemLog(const char *, ...);
__attribute__((weak)) int sub_02038D90(void) {
    unsigned char *p = *(unsigned char **)(ssdata_unk_02035900__021D4140 + 8);
    if (p) p[0x6B9] = 1;
    if (ssdata_unk_02037C94__021D4150 != 0) PSPNativeMemLog("[SS3DS] sub_02038D90 with a comm session (unsupported) -> TRUE");
    return 1;
}
/* run 3: sub_02039AA4 ("comm is idle, OK to reset", unk_02037C94.s) was also only a trap; DoSoftReset polls it every
   frame before OS_ResetSystem. Offline (_021D4150 == NULL, sub_02037D78() false) the DS returns TRUE. */
__attribute__((weak)) int sub_02039AA4(void) { return 1; }

/* 2026-11-05: the field weather for the renderer (g3gpu.c dark-cave fog), -1 outside the field or before a save is loaded */
extern void *SaveData_Get(void);
extern void *Save_LocalFieldData_Get(void *saveData);
extern unsigned LocalFieldData_GetWeatherType(void *localFieldData);
int SS3DS_FieldWeather(void) {
    if (!P3DS_InField) return -1;
    void *save = SaveData_Get();
    return save ? (int)LocalFieldData_GetWeatherType(Save_LocalFieldData_Get(save)) : -1;
}

/* 2026-11-06 (Nathan: Mystery Gift shows no cursor): every translated caller of ListMenu_ProcessInput (call-retargets "*", patch 0227)
   goes through here. It runs the real function and remembers the list the game is processing, so the port can draw a cursor where
   the game's own printed one is missing. Layout: ListMenu / Window in the decomp (list_menu.h, bg_window.h). */
extern int ListMenu_ProcessInput(void *list);
volatile struct { unsigned frame; unsigned char bgId, left, top, width, height, cursorX, upY, kind; unsigned short pos, above, total, shown; unsigned char font, padding; } P3DS_ListMenuNow;
int SS3DS_ListMenuProcessInput(void *list) {
    const int r = ListMenu_ProcessInput(list);
    extern unsigned PSPNativeRenderFrameCount(void);
    const unsigned char *l = (const unsigned char *)list; const unsigned char *w = *(const unsigned char *const *)(l + 0x0C);
    if (w) {
        P3DS_ListMenuNow.bgId = w[4]; P3DS_ListMenuNow.left = w[5]; P3DS_ListMenuNow.top = w[6]; P3DS_ListMenuNow.width = w[7]; P3DS_ListMenuNow.height = w[8];
        P3DS_ListMenuNow.cursorX = l[0x16]; P3DS_ListMenuNow.upY = l[0x17] & 15;
        { const unsigned short b = *(const unsigned short *)(l + 0x1A); P3DS_ListMenuNow.padding = (b >> 3) & 15; P3DS_ListMenuNow.font = (b >> 9) & 63; P3DS_ListMenuNow.kind = (b >> 15) & 1; }
        P3DS_ListMenuNow.total = *(const unsigned short *)(l + 0x10); P3DS_ListMenuNow.shown = *(const unsigned short *)(l + 0x12);
        P3DS_ListMenuNow.pos = *(const unsigned short *)(l + 0x2C); P3DS_ListMenuNow.above = *(const unsigned short *)(l + 0x2E);
        P3DS_ListMenuNow.frame = PSPNativeRenderFrameCount() + 1;
        { static unsigned short lp, la; static int ln; if ((P3DS_ListMenuNow.pos != lp || P3DS_ListMenuNow.above != la || ln++ < 3)) { lp = P3DS_ListMenuNow.pos; la = P3DS_ListMenuNow.above;
            PSPNativeMemLog("[LISTMENU] bg %u win %u,%u %ux%u cursor_X %u upY %u kind %u font %u pad %u total %u shown %u pos %u above %u", P3DS_ListMenuNow.bgId, P3DS_ListMenuNow.left, P3DS_ListMenuNow.top,
                P3DS_ListMenuNow.width, P3DS_ListMenuNow.height, P3DS_ListMenuNow.cursorX, P3DS_ListMenuNow.upY, P3DS_ListMenuNow.kind, P3DS_ListMenuNow.font, P3DS_ListMenuNow.padding,
                P3DS_ListMenuNow.total, P3DS_ListMenuNow.shown, P3DS_ListMenuNow.pos, P3DS_ListMenuNow.above); } }
    }
    return r;
}

/* 2026-11-07 (hook 0228, pg_flytap): tap-to-Fly on the JOHTO map. The panel sets P3DS_FlyReq to a town's map id; the game thread (FieldInput_Process,
   player at rest) takes it, re-checks everything and starts the game's own Fly. It also publishes which Fly points are unlocked (P3DS_FlyList, map ids)
   and whether Fly works here now (P3DS_FlyHere: -1 not known, 0 yes, 1 not here, 2 no Storm Badge, 3 not now, 4 no Pokemon with Fly). Results in P3DS_FlyRes. */
volatile int P3DS_FlyReq, P3DS_FlyRes, P3DS_FlyN, P3DS_FlyHere = -1;
volatile unsigned short P3DS_FlyList[32];

/* Starter-ball fix (2026-11-12): the SDK-port C versions of MTX_RotX43_/RotY43_/RotZ43_ (libntr fx_mtx43.c, SDK_PORT) only write the 3x3 part, so the matrix
   translation (_30.._32) stays whatever the caller left on its stack. The real DS routines write zero there. Elm's starter-select app builds tmpRotMtx on the stack, then
   MTX_Concat43 folds that garbage translation into every ball matrix (rawM translation about -30000 in the log), so the balls land far behind the camera and are rejected
   as off screen. This copy comes earlier in the link (--allow-multiple-definition keeps the first) and zeroes the translation like the hardware routine. */
/* MtxFx43 is 12 consecutive fx32 (_00 _01 _02 _10 _11 _12 _20 _21 _22 _30 _31 _32); FX32_ONE = 0x1000. */
void MTX_RotX43_(int *d,int s,int c){d[0]=0x1000;d[1]=0;d[2]=0;d[3]=0;d[4]=c;d[5]=s;d[6]=0;d[7]=-s;d[8]=c;d[9]=0;d[10]=0;d[11]=0;}
void MTX_RotY43_(int *d,int s,int c){d[0]=c;d[1]=0;d[2]=-s;d[3]=0;d[4]=0x1000;d[5]=0;d[6]=s;d[7]=0;d[8]=c;d[9]=0;d[10]=0;d[11]=0;}
void MTX_RotZ43_(int *d,int s,int c){d[0]=c;d[1]=s;d[2]=0;d[3]=-s;d[4]=c;d[5]=0;d[6]=0;d[7]=0;d[8]=0x1000;d[9]=0;d[10]=0;d[11]=0;}

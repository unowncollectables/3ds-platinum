/* run 58 (PLATINUM3DS): following Pokemon, HGSS style, RENDER-ONLY.
   Not a MapObject: nothing is allocated in the map object manager, so it is never serialised into the save and can
   never be found by scripts (Cheryl/Mira/partner lookups use MapObjMan_* which cannot see it), and it has no
   collision. State lives in the port (P3DS_FolMem) so it survives the overlay reload around battles.
   Drawn as one textured quad right after BillboardLists_Draw() inside the same projection depth bias as the
   overworld sprites, through the normal G3 path -> g3soft/g3gpu -> stereo depth and wide view like every sprite.
   The texture is port-owned (followers.bin, see platform/g3soft.cpp folTex), selected by a magic TEXIMAGE address. */
#include <stdio.h>
#include "billboard.h"
#include "billboard_gfx_sequence.h"
#include "party.h"
#include "system.h"
#include "player_avatar.h"
#include "pokemon.h"
#include "system_flags.h"
#include "vars_flags.h"
#include "generated/movement_types.h"
#include "constants/player_avatar.h"
#include "bag.h"
#include "sound_playback.h"
#include "special_encounter.h"
#include "message.h"
#include "string_gf.h"
#include "constants/narc.h"
#include "map_header_data.h"
#include "overlay006/wild_encounters.h"
#include "pokemon_icon.h"
#include "item.h"
#include "map_header.h"
#include "field_overworld_state.h"
#include "rtc.h"

extern int P3DS_FollowerCfg(void);
extern int P3DS_FollowerOpen(void);
extern int P3DS_FollowerSprite(int species, int form, int female);
extern int P3DS_FollowerLoad(int spr, int shiny);
extern unsigned char P3DS_FolMem[256];
extern int P3DS_FollowerSlotCfg(void);   /* run 61: 0 = lead, 1..6 = party slot (config follower_slot, never the save) */
extern int P3DS_FollowerTalkCfg(void);   /* run 61: follower_talk=1: A facing the follower shows a port overlay text box */
extern void P3DS_FolSay(const char *text, int ms);
extern void PSPNativeMemLog(const char *fmt, ...);

typedef struct P3DSFol {
    int valid;        /* 1 = follower placed on the trail */
    int map;          /* mapHeaderID the trail belongs to */
    int px, pz;       /* player tile at last update */
    VecFx32 ppos;     /* player position at last update */
    VecFx32 lpos;     /* player position when it last stood still on (px,pz) */
    int lx, lz;
    VecFx32 from, to; /* follower moves from -> to while the player's current step runs */
    VecFx32 cur;
    int steps;        /* tiles of the player's current step (1, 2 for a ledge) */
    int dir;          /* 0 up 1 down 2 left 3 right */
    int sprite, shiny, w, monKey, monCheck;
    unsigned anim;
    int moving;
    int taskSteps; /* tile changes while a field task/script runs */
    Pokemon *mon;  /* run 61: the party mon shown (refreshed with monCheck) */
    int talkPend, talkN; /* run 61 follower_talk */
    int species;   /* run 64 follower_gap / talk lines */
    int tileSteps, lastFind, finds; /* run 64 follower_items cooldown (port memory only) */
    int popT, hiddenFrames; /* run 64 follower_ballpop */
} P3DSFol;
typedef char p3dsFolSizeChk[sizeof(P3DSFol) <= 256 ? 1 : -1];

extern int P3DS_CfgInt(const char *, int);
static int p3dsFolDbg(void) { static int v = -1; if (v < 0) v = P3DS_CfgInt("follower_debug", 0); return v; }
static int p3dsFolBehind(void) { static int v = -1; if (v < 0) v = P3DS_CfgInt("follower_behind", 1) != 0; return v; }
/* run 61 follower_sidefix=1 (default): the HGSS NSBTX frame order is up 0-1, down 2-3, LEFT 4-5, RIGHT 6-7 (Pikachu /
   Girafarig previews: frames 4-5 face left). Run 58 drew frames 4-5 unmirrored for right and mirrored for left, so both
   side views faced backwards. 1 = frame dir*2 (+anim), no mirroring; 0 = old run-58 mapping. */
static int p3dsFolSideFix(void) { static int v = -1; if (v < 0) v = P3DS_CfgInt("follower_sidefix", 1) != 0; return v; }
static fx32 p3dsFolAbs(fx32 v) { return v < 0 ? -v : v; }
/* run 64 follower_gap=1 (default): the follower stands one tile behind, but a 64-px sprite (or a long/tall species) still
   reached over the player's head. Draw it pushed further away from the player along the follow direction: 64-px
   sprites: 32 px +4 (+8 when north of the player, where the sprite rises over the head), 64 px +8 (+12 north);
   big/tall species (Girafarig, Onix, Steelix, Wailord, Gyarados, Lapras, legendaries ...) +8 more (1 tile = 16 units).
   Render-only (the trail / logic position is unchanged); follower_behind depth order is kept. 0 = old placement. */
static int p3dsFolGap(void) { static int v = -1; if (v < 0) { v = P3DS_CfgInt("follower_gap", 1) != 0; PSPNativeMemLog("[FOL] follower_gap=%d", v); } return v; }
static int p3dsFolBig(int sp) {
    static const short big[] = { 203, 95, 130, 131, 143, 149, 208, 248, 249, 250, 306, 319, 321, 334, 350, 373, 376, 382, 383, 384,
        389, 445, 450, 464, 473, 483, 484, 485, 486, 487, 493, 6, 3, 9, 68, 76, 112, 142, 144, 145, 146, 150, 230, 242, 245, 260, 330, 365, 381, 380, 409, 411, 426, 448, 466, 468, 472, 475, 477, 484 };
    for (unsigned i = 0; i < sizeof big / sizeof big[0]; i++) if (big[i] == sp) return 1;
    return 0; }

/* run 64 lid_sound: called from platform/main3ds.c on the game thread (overworld only) */
/* run 65: Poke Ball sounds. Close = recall (SEQ_SE_DP_SUIKOMU, decomp id SEQ_SE_DP_SLOT02_sseq_1 = 1797), open = send-out
   (SEQ_SE_DP_BOWA2, decomp id SEQ_SE_DP_SLOT01_sseq_1 = 1798, the battle send-out sound in battle_display.c). Both live in
   BANK_SE_BATTLE; if the SDK can't start them in the overworld (bank/wave arc not resident) fall back to the game's
   field ball sound SEQ_SE_DP_BOWA (BANK_BASIC, the Poke Center ball sound). SE players only: the BGM keeps playing. */
void P3DS_PlayLidSE(int open) {
    if (!Sound_PlayEffect(open ? SEQ_SE_DP_SLOT01_sseq_1 : SEQ_SE_DP_SLOT02_sseq_1)) Sound_PlayEffect(SEQ_SE_DP_BOWA_sseq); }
static void P3DS_FollowerHide(P3DSFol *f) { f->valid = 0; }

static int P3DS_FollowerMon(FieldSystem *fieldSystem, P3DSFol *f)
{
    if (--f->monCheck > 0 && f->w) return f->w;
    f->monCheck = 30;
    Party *party = SaveData_GetParty(fieldSystem->saveData);
    if (!party || Party_GetCurrentCount(party) < 1) return 0;
    /* run 61 follower_slot: 1..6 = that party slot; empty slot or an egg -> the lead (slot 0) */
    int slot = P3DS_FollowerSlotCfg(), cnt = Party_GetCurrentCount(party);
    Pokemon *mon = NULL;
    if (slot >= 1 && slot <= cnt) { mon = Party_GetPokemonBySlotIndex(party, slot - 1); if (mon && Pokemon_GetValue(mon, MON_DATA_IS_EGG, NULL)) mon = NULL; }
    if (!mon) mon = Party_GetPokemonBySlotIndex(party, 0);
    if (!mon || Pokemon_GetValue(mon, MON_DATA_IS_EGG, NULL)) return 0;
    f->mon = mon;
    f->species = Pokemon_GetValue(mon, MON_DATA_SPECIES, NULL);
    int species = Pokemon_GetValue(mon, MON_DATA_SPECIES, NULL), form = Pokemon_GetValue(mon, MON_DATA_FORM, NULL);
    int female = Pokemon_GetGender(mon) == 1, shiny = Pokemon_IsShiny(mon) ? 1 : 0;
    int key = species | form << 10 | female << 16 | shiny << 17 | (slot & 7) << 20;
    if (key == f->monKey && f->w) return f->w;
    int spr = P3DS_FollowerSprite(species, form, female);
    f->w = spr >= 0 ? P3DS_FollowerLoad(spr, shiny) : 0;
    f->monKey = key;
    PSPNativeMemLog("[FOL] lead species %d form %d female %d shiny %d -> sprite %d w %d", species, form, female, shiny, spr, f->w);
    return f->w;
}


/* run 61 follower_talk=1: A while standing still and facing the follower -> a short port-drawn text box
   (P3DS_FolSay, platform overlay on the top screen; nothing in the game's message system, no script, no save data).
   Shown only if the game itself starts nothing for 2 frames after the press (hidden item, sign, ...); hidden at once
   when a field task/script starts or the player moves. */
static void P3DS_FolName(Pokemon *mon, char *o, int n)
{
    u16 b[16];
    int k = 0;
    memset(b, 0xFF, sizeof b);
    Pokemon_GetValue(mon, MON_DATA_NICKNAME, b);
    for (int i = 0; i < 11 && k < n - 1; i++) {
        u16 c = b[i];
        if (c == 0xFFFF) break;
        o[k++] = c >= 0x121 && c <= 0x12A ? '0' + (c - 0x121) : c >= 0x12B && c <= 0x144 ? 'A' + (c - 0x12B)
            : c >= 0x145 && c <= 0x15E ? 'a' + (c - 0x145) : c == 0x1DE ? ' ' : '-'; /* run 64: keep lower case (game font) */
    }
    o[k] = 0;
    if (!k) snprintf(o, n, "Your Pokemon");
}
static void P3DS_FollowerTalk(FieldSystem *fieldSystem, P3DSFol *f, VecFx32 pos, int task)
{
    static const char *const lines[] = {
        /* run 64: mixed case (drawn in the game's font; the port-font fallback upper-cases) */
        "%s is happy to be with you!", "%s is looking around curiously.", "%s stretched and gave a big yawn.",
        "%s gave you a cheerful look!", "%s is stepping along to a happy rhythm.", "%s seems to be enjoying the walk.",
        "%s is sniffing the air. Something smells nice?", "%s nuzzled up to you.",
    };
    if (!P3DS_FollowerTalkCfg()) return;
    if (task || f->moving) { if (f->talkPend || f->talkN) P3DS_FolSay("", 0); f->talkPend = 0; f->talkN = 0; return; }
    if (f->talkPend) {
        if (--f->talkPend == 0 && f->mon) {
            char nm[16], t[128];
            P3DS_FolName(f->mon, nm, sizeof nm);
            static unsigned rng = 0x2468ACE1u; rng ^= f->anim * 2654435761u + (unsigned)f->talkN * 40503u; rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            /* run 64 follower_area_talk=1 (default): lines by area / weather / time of day / species / type (platform
               p3ds_foltalk.c table); 0 = the run 61 generic lines */
            static int areaCfg = -1; if (areaCfg < 0) { areaCfg = P3DS_CfgInt("follower_area_talk", 1) != 0; PSPNativeMemLog("[FOL] follower_area_talk=%d", areaCfg); }
            int map = fieldSystem->location->mapHeaderID, weather = FieldOverworldState_GetWeather(SaveData_GetFieldOverworldState(fieldSystem->saveData));
            if (areaCfg) {
                extern int P3DS_FolLine(char *, int, const char *, int, int, int, int, int, int, int, int, unsigned);
                P3DS_FolLine(t, sizeof t, nm, map, MapHeader_IsCave(map), MapHeader_IsOutdoors(map), weather, GetTimeOfDay(),
                    Pokemon_GetValue(f->mon, MON_DATA_TYPE_1, NULL), Pokemon_GetValue(f->mon, MON_DATA_TYPE_2, NULL), f->species, rng);
            } else snprintf(t, sizeof t, lines[(f->anim / 7 + f->talkN * 3) % (int)(sizeof lines / sizeof lines[0])], nm);
            f->talkN++;
            /* run 64 follower_cry=1 (default): the species' cry through the game's own Pokemon_PlayCry (species + form,
               the same call the summary screen / scripts use; the sound system ducks and restores the music itself) */
            { static int cry = -1; if (cry < 0) { cry = P3DS_CfgInt("follower_cry", 1) != 0; PSPNativeMemLog("[FOL] follower_cry=%d", cry); }
              if (cry) { Pokemon_PlayCry(f->mon); PSPNativeMemLog("[FOL] cry species %d form %d", f->species, Pokemon_GetValue(f->mon, MON_DATA_FORM, NULL)); } }
            /* run 64 follower_items=1 (default 0): sometimes the follower finds a common item. Added ONLY through the
               game's own Bag_TryAddItem on the save's bag (the same call as the script command additem / hidden items /
               berry picking), so the save stays a normal Platinum save. Cooldown: 120 player steps since the last find
               (port memory, not saved), then a 1 in 3 chance per talk. Full bag -> nothing is found. */
            { static int it = -1; if (it < 0) { it = P3DS_CfgInt("follower_items", 0) != 0; PSPNativeMemLog("[FOL] follower_items=%d", it); }
              static const struct { u16 id; const char *nm; } pool[] = { { 17, "a Potion" }, { 18, "an Antidote" }, { 22, "a Paralyze Heal" },
                  { 21, "an Awakening" }, { 4, "a Pok\xe9 Ball" }, { 3, "a Great Ball" }, { 26, "a Super Potion" }, { 78, "an Escape Rope" },
                  { 79, "a Repel" }, { 155, "an Oran Berry" }, { 151, "a Pecha Berry" }, { 149, "a Cheri Berry" }, { 94, "some Honey" } };
              int force = P3DS_CfgInt("follower_items_test", 0); /* debug: 1 = no cooldown/chance (emu tests) */
              if (it && (force || (f->tileSteps - f->lastFind >= 120 && (rng >> 8) % 3 == 0))) {
                  int k = (int)((rng >> 12) % (sizeof pool / sizeof pool[0]));
                  if (Bag_TryAddItem(SaveData_GetBag(fieldSystem->saveData), pool[k].id, 1, HEAP_ID_FIELD1)) {
                      f->lastFind = f->tileSteps; f->finds++;
                      snprintf(t, sizeof t, "%s found %s! You put it in your Bag.", nm, pool[k].nm);
                      PSPNativeMemLog("[FOL] item: %s found item %d via Bag_TryAddItem (find #%d, step %d)", nm, pool[k].id, f->finds, f->tileSteps);
                  } else PSPNativeMemLog("[FOL] item: bag full for item %d, nothing found", pool[k].id);
              } }
            P3DS_FolSay(t, 3200);
            PSPNativeMemLog("[FOL] talk (map %d weather %d): %s", map, weather, t);
        }
        return;
    }
    if (!(gSystem.pressedKeys & PAD_BUTTON_A)) return;
    fx32 dx = f->cur.x - pos.x, dz = f->cur.z - pos.z;
    int d = PlayerAvatar_GetFacingDir(fieldSystem->playerAvatar), want;
    if (p3dsFolAbs(dx) > 20 * FX32_ONE || p3dsFolAbs(dz) > 20 * FX32_ONE || (p3dsFolAbs(dx) < 8 * FX32_ONE && p3dsFolAbs(dz) < 8 * FX32_ONE)) return;
    want = p3dsFolAbs(dx) >= p3dsFolAbs(dz) ? (dx < 0 ? 2 : 3) : (dz < 0 ? 0 : 1);
    if (d != want) return;
    f->talkPend = 2;
}

/* run 64 Map tab (settings app): snapshot of the current map's wild Pokemon from the game's own encounter data
   (MapHeaderData_GetWildEncounters) and species names from the ROM's message bank, taken on the game thread when the
   map header changes. Base tables only (no swarm / time-of-day / radar swaps). Read only. */
static void p3dsCcToLatin(const charcode_t *c, char *o, int n)
{
    int k = 0;
    for (int i = 0; c[i] != 0xFFFF && i < 24 && k < n - 1; i++) {
        unsigned v = c[i]; char ch = 0;
        if (v >= 0x121 && v <= 0x12A) ch = '0' + (v - 0x121); else if (v >= 0x12B && v <= 0x144) ch = 'A' + (v - 0x12B);
        else if (v >= 0x145 && v <= 0x15E) ch = 'a' + (v - 0x145); else if (v == 0x1DE) ch = ' '; else if (v == 0x188) ch = (char)0xE9;
        else if (v == 0x1AE) ch = '.'; else if (v == 0x1BE) ch = '-'; else if (v == 0x1B3) ch = 39;
        if (ch >= 'A' && ch <= 'Z' && k > 0 && ((o[k - 1] >= 'A' && o[k - 1] <= 'Z') || (o[k - 1] >= 'a' && o[k - 1] <= 'z'))) ch += 32; /* SHELLOS -> Shellos */
        if (ch) o[k++] = ch;
    }
    o[k] = 0;
}
static void p3dsMapSnap(FieldSystem *fieldSystem)
{
    /* run 65 Map tab grid: per section (grass for the current time of day, surf, old / good / super rod) every species
       with its level range and encounter chance = sum of its slots' rates (the game's slot tables), plus its party
       icon read from the ROM (pl_poke_icon.narc via the game's own icon index functions). Built once per map while
       the player stands still; nothing is written to the game. */
    typedef struct { unsigned short species; unsigned char lo, hi, pct, sec; } Ent;
    extern volatile int P3DS_MapId, P3DS_MapEntN, P3DS_MapTod; extern Ent P3DS_MapEnt[48]; extern int P3DS_MapIconMem[48], P3DS_MapIconPal[48], P3DS_MapIconPalFile; extern volatile int P3DS_MapGen;
    static int last = -1, still, sx = -1, sz = -1, lastTod = -1;
    if (!fieldSystem->location || !fieldSystem->playerAvatar) return;
    const int tod = GetTimeOfDay() >= 3 ? 1 : 0;
    if (fieldSystem->location->mapHeaderID == last && tod == lastTod) return;
    { MapObject *pl = PlayerAvatar_GetMapObject(fieldSystem->playerAvatar); if (!pl) return; int x = MapObject_GetX(pl), z = MapObject_GetZ(pl);
      if (x != sx || z != sz || FieldSystem_IsRunningTask(fieldSystem)) { sx = x; sz = z; still = 0; return; } if (++still < 30) return; }
    last = fieldSystem->location->mapHeaderID; lastTod = tod;
    u64 t0 = OS_GetTick();
    static const u8 kGrass[12] = { 20, 20, 10, 10, 10, 10, 5, 5, 4, 4, 1, 1 }, kSurf[5] = { 60, 30, 5, 4, 1 }, kRod[5] = { 40, 40, 15, 4, 1 };
    int n = 0;
    const WildEncounters *w = MapHeaderData_GetWildEncounters(fieldSystem);
    if (w) for (int sec = 0; sec < 5; sec++) {
        const WaterEncounters *we = sec == 1 ? &w->surfEncounters : sec == 2 ? &w->oldRodEncounters : sec == 3 ? &w->goodRodEncounters : sec == 4 ? &w->superRodEncounters : NULL;
        int rate = sec == 0 ? w->grassEncounters.encounterRate : we->encounterRate; if (!rate) continue;
        const int cnt = sec == 0 ? MAX_GRASS_ENCOUNTERS : MAX_WATER_ENCOUNTERS; const int first = n;
        for (int i = 0; i < cnt; i++) {
            int sp, lo, hi, pc;
            if (sec == 0) { sp = w->grassEncounters.encounters[i].species; lo = hi = w->grassEncounters.encounters[i].level; pc = i < 12 ? kGrass[i] : 0;
                if (i == 2 || i == 3) { int t = tod ? w->nightEncounters[i - 2] : w->dayEncounters[i - 2]; if (t) sp = t; } }
            else { sp = we->encounters[i].species; lo = we->encounters[i].minLevel; hi = we->encounters[i].maxLevel; pc = sec <= 2 ? kSurf[i] : kRod[i]; }
            sp &= 0x7FF; if (!sp || !pc) continue;
            int j; for (j = first; j < n; j++) if (P3DS_MapEnt[j].species == sp) break;
            if (j == n) { if (n == 48) continue; P3DS_MapEnt[n].species = sp; P3DS_MapEnt[n].lo = lo; P3DS_MapEnt[n].hi = hi; P3DS_MapEnt[n].pct = pc; P3DS_MapEnt[n].sec = sec; n++; }
            else { if (lo < P3DS_MapEnt[j].lo) P3DS_MapEnt[j].lo = lo; if (hi > P3DS_MapEnt[j].hi) P3DS_MapEnt[j].hi = hi; P3DS_MapEnt[j].pct += pc; }
        }
        /* highest chance first within the section */
        for (int a = first; a < n; a++) for (int b = a + 1; b < n; b++) if (P3DS_MapEnt[b].pct > P3DS_MapEnt[a].pct) { Ent t = P3DS_MapEnt[a]; P3DS_MapEnt[a] = P3DS_MapEnt[b]; P3DS_MapEnt[b] = t; }
    }
    /* icons are only looked up here (cheap); the ROM read happens in the settings app (game paused) on first view */
    for (int k = 0; k < n; k++) { P3DS_MapIconMem[k] = PokeIconSpriteIndex(P3DS_MapEnt[k].species, 0, 0); P3DS_MapIconPal[k] = PokeIconPaletteIndex(P3DS_MapEnt[k].species, 0, 0); }
    P3DS_MapIconPalFile = PokeIconPalettesFileIndex(); P3DS_MapGen++;
    P3DS_MapTod = tod; P3DS_MapEntN = n; P3DS_MapId = last;
    PSPNativeMemLog("[MAP] snapshot map %d (%s): %d entries, %u us", last, tod ? "night" : "day", n, (unsigned)(OS_TicksToMicroSeconds(OS_GetTick() - t0)));
}
static void P3DS_FollowerFrame(FieldSystem *fieldSystem)
{
    if (fieldSystem) p3dsMapSnap(fieldSystem);
    /* run 64 repel_prompt=1 (default): when a Repel wears off and the bag still has one, after the game's own
       "wore off" message a port Yes/No asks "Use another?". Yes uses it exactly like the Bag does: Bag_TryRemoveItem
       + repel steps = the item's own effect param (Item_LoadParam) + the Bag's SE. Game input is held while asking. */
    if (fieldSystem && fieldSystem->saveData) {
        extern volatile int P3DS_RepelAsk, P3DS_RepelLast, P3DS_PromptOpen, P3DS_PromptResult; extern char P3DS_PromptText[64];
        static int wait, item; extern int P3DS_RepelPromptCfg(void); int cfg = P3DS_RepelPromptCfg();
        static int rpt = -1; if (rpt < 0) rpt = P3DS_CfgInt("repel_prompt_test", 0); /* run 66: cached (was an SD fopen every frame = the run 64/65 FPS drop) */
        if (rpt > 0) { static int once; if (!once) { once = 1; u8 *rs = SpecialEncounter_GetRepelSteps(SaveData_GetSpecialEncounters(fieldSystem->saveData)); if (*rs == 0) *rs = 2; PSPNativeMemLog("[REPEL] test: repel steps set to %d", *rs); } }
        if (P3DS_RepelAsk == 1) { if (!cfg) P3DS_RepelAsk = 0; else if (FieldSystem_IsRunningTask(fieldSystem)) wait = 0; else if (++wait >= 8) {
            P3DS_RepelAsk = 0; wait = 0; item = 0;
            Bag *bag = SaveData_GetBag(fieldSystem->saveData);
            if ((P3DS_RepelLast == 76 || P3DS_RepelLast == 77 || P3DS_RepelLast == 79) && Bag_CanRemoveItem(bag, P3DS_RepelLast, 1, HEAP_ID_FIELD1)) item = P3DS_RepelLast;
            else { static const u16 kR[3] = { 79, 76, 77 }; for (int i = 0; i < 3 && !item; i++) if (Bag_CanRemoveItem(bag, kR[i], 1, HEAP_ID_FIELD1)) item = kR[i]; }
            PSPNativeMemLog("[REPEL] wore off; bag item %d", item);
            if (item) { snprintf(P3DS_PromptText, 64, "Use another %s?", item == 77 ? "Max Repel" : item == 76 ? "Super Repel" : "Repel"); P3DS_PromptResult = -1; P3DS_PromptOpen = 1; } } }
        if (P3DS_PromptOpen == 2) {
            P3DS_PromptOpen = 0;
            SpecialEncounter *se = SaveData_GetSpecialEncounters(fieldSystem->saveData);
            if (P3DS_PromptResult == 1 && item && SpecialEncounter_RepelStepsEmpty(se) && Bag_TryRemoveItem(SaveData_GetBag(fieldSystem->saveData), item, 1, HEAP_ID_FIELD1)) {
                *SpecialEncounter_GetRepelSteps(se) = (u8)Item_LoadParam(item, ITEM_PARAM_EFFECT_PARAM, HEAP_ID_FIELD1);
                Sound_PlayEffect(SEQ_SE_DP_CARD2_sseq); P3DS_RepelLast = item;
                PSPNativeMemLog("[REPEL] used item %d -> %d steps", item, *SpecialEncounter_GetRepelSteps(se));
                { extern void P3DS_Toast(const char *); P3DS_Toast(item == 77 ? "Used a Max Repel" : item == 76 ? "Used a Super Repel" : "Used a Repel"); }
            } else PSPNativeMemLog("[REPEL] declined (%d)", P3DS_PromptResult);
        }
    }
    P3DSFol *f = (P3DSFol *)P3DS_FolMem;
    /* run 64 (Big build 5b) touch Start menu: the settings app's "Menu" tab sets P3DS_StartReq = START_MENU_OPTION_*.
       When the field is idle the game's own Start menu is opened (X pressed for 2 polls) with the cursor preset to that
       option (fieldSystem->menuCursorPos, the game's own "last used" memory); A is only sent if start_menu.c reports
       the option exists in this menu (P3DS_StartReqFound), else the menu simply stays open. Real menus, real flow. */
    {
        extern volatile int P3DS_StartReq, P3DS_StartReqFound; extern volatile unsigned P3DS_InjectKeys; extern volatile int P3DS_InjectFrames;
        static int st, wait;
        if (st == 0 && P3DS_StartReq >= 0) {
            if (!FieldSystem_IsRunningTask(fieldSystem) && P3DS_InjectFrames <= 0) {
                fieldSystem->menuCursorPos = P3DS_StartReq; P3DS_StartReqFound = 0; P3DS_InjectKeys = 1u << 10; P3DS_InjectFrames = 2; st = 1; wait = 0;
                PSPNativeMemLog("[STARTAPP] open start menu for option %d", P3DS_StartReq);
            } else if (++wait > 180) { PSPNativeMemLog("[STARTAPP] field busy, request %d dropped", P3DS_StartReq); P3DS_StartReq = -1; wait = 0; }
        } else if (st == 1) {
            if (FieldSystem_IsRunningTask(fieldSystem) && P3DS_StartReqFound) {
                if (++wait >= 10) { P3DS_InjectKeys = 1u; P3DS_InjectFrames = 2; PSPNativeMemLog("[STARTAPP] select option %d", P3DS_StartReq); st = 0; P3DS_StartReq = -1; wait = 0; }
            } else if (++wait > 90) { PSPNativeMemLog("[STARTAPP] option %d not in the menu (found %d), left open", P3DS_StartReq, P3DS_StartReqFound); st = 0; P3DS_StartReq = -1; wait = 0; }
        }
    }
    /* run 63 player_trace=1 (debug, default 0): log the player object's status/facing/moving/state on every change */
    {
        static int pt = -1; static u32 last = 0xFFFFFFFF;
        if (pt < 0) pt = P3DS_CfgInt("player_trace", 0);
        if (pt > 0 && fieldSystem && fieldSystem->playerAvatar) {
            MapObject *pl = PlayerAvatar_GetMapObject(fieldSystem->playerAvatar);
            if (pl) {
                u32 st = MapObject_GetStatus(pl);
                u32 key = (st & 0x1FFC0) ^ (u32)MapObject_GetFacingDir(pl) << 20 ^ (u32)MapObject_GetMovingDir(pl) << 24 ^ (u32)PlayerAvatar_GetPlayerState(fieldSystem->playerAvatar) << 28;
                { static unsigned fc; if (++fc % 10 == 0) { u8 *dd = (u8 *)sub_02062AF0(pl); Billboard *bb = dd ? *(Billboard **)(dd + 4) : NULL;
                    if (bb && fc % 60 == 0) { const u8 *ti = bb->gfxSequence.textureIdx; const u16 *sf = bb->gfxSequence.startFrame; PSPNativeMemLog("[PLTR3] seq sf %p ti %p pi %p | sf %u %u %u %u %u %u %u %u | ti %u %u %u %u %u %u %u %u %u %u %u %u", (void *)sf, (void *)ti, (void *)bb->gfxSequence.plttIdx, sf[0], sf[1], sf[2], sf[3], sf[4], sf[5], sf[6], sf[7], ti[0], ti[1], ti[2], ti[3], ti[4], ti[5], ti[6], ti[7], ti[8], ti[9], ti[10], ti[11]); }
                    PSPNativeMemLog("[PLTR2] gfx %d anim %lu last %d/%d bb %p anim# %d frame %ld mgr4 %lu mpause %d f7c %d st %08lx bbst %d draw %d mdl %p tex %p atex %p key %08lx | absf %d tidx %d pidx %d seq %lu mat0 %08lx texd %08lx", MapObject_GetGraphicsID(pl), (unsigned long)sub_02062A14(pl), dd ? (s8)dd[2] : -9, dd ? (s8)dd[3] : -9, (void *)bb,
                        bb ? Billboard_GetAnimNum(bb) : -1, bb ? (long)Billboard_GetAnimFrameNum(bb) : -1L, (unsigned long)MapObjectMan_CheckStatus(MapObject_MapObjectManager(pl), 1 << 2), MapObject_IsMovementPaused(pl), sub_02062F7C(pl), (unsigned long)st, bb ? bb->state : -1, bb ? bb->draw : -1, bb ? (void *)bb->model : NULL, bb ? (void *)bb->texture : NULL, bb ? (void *)bb->animTexture : NULL, bb ? (unsigned long)bb->texKey : 0UL, bb ? (int)(bb->frameNum >> 12) : -1, bb ? BillboardGfxSequence_GetTexPlttIndexAt(&bb->gfxSequence, bb->frameNum >> 12).textureIdx : -1, bb ? BillboardGfxSequence_GetTexPlttIndexAt(&bb->gfxSequence, bb->frameNum >> 12).plttIdx : -1, bb ? (unsigned long)bb->gfxSequence.seqCount : 0UL, bb && bb->model ? (unsigned long)NNS_G3dGetMatDataByIdx(NNS_G3dGetMat(bb->model), 0)->texImageParam : 0UL, bb && bb->animTexture ? (unsigned long)NNS_G3dGetTexDataByIdx(bb->animTexture, BillboardGfxSequence_GetTexPlttIndexAt(&bb->gfxSequence, bb->frameNum >> 12).textureIdx)->texImageParam : 0UL); } }
                if (key != last) { last = key; PSPNativeMemLog("[PLTR] status %08lx (40 pausemove 80 lockdir 100 pauseanim) face %d move %d state %d tile %d,%d task %d", (unsigned long)st, MapObject_GetFacingDir(pl), MapObject_GetMovingDir(pl), PlayerAvatar_GetPlayerState(fieldSystem->playerAvatar), MapObject_GetX(pl), MapObject_GetZ(pl), FieldSystem_IsRunningTask(fieldSystem)); }
            }
        }
    }
    if (!P3DS_FollowerCfg() || !P3DS_FollowerOpen()) { f->valid = 0; return; }
    if (!fieldSystem || !fieldSystem->playerAvatar || !fieldSystem->mapObjMan || !fieldSystem->location || !fieldSystem->saveData) return;
    MapObject *pl = PlayerAvatar_GetMapObject(fieldSystem->playerAvatar);
    if (!pl) return;
    int map = fieldSystem->location->mapHeaderID;
    int x = MapObject_GetX(pl), z = MapObject_GetZ(pl);
    VecFx32 pos;
    MapObject_GetPosPtr(pl, &pos);
    int state = PlayerAvatar_GetPlayerState(fieldSystem->playerAvatar);
    int task = FieldSystem_IsRunningTask(fieldSystem);
    int hide = state != PLAYER_AVATAR_WALKING || FieldMap_InDistortionWorld(fieldSystem) == TRUE
        || MapObjMan_GetLocalMapObjByMovementType(fieldSystem->mapObjMan, MOVEMENT_TYPE_FOLLOW_PLAYER) != NULL
        || SystemFlag_CheckHasPartner(SaveData_GetVarsFlags(fieldSystem->saveData))
        || MapObject_IsHidden(pl);
    /* a map header change alone is not a warp (Route 212 north/south etc. are seamless); warps are caught by the
       tile jump / field task tests below */
    f->map = map;
    {
        static int lastWhy = -1;
        int why = (state != PLAYER_AVATAR_WALKING) | (FieldMap_InDistortionWorld(fieldSystem) == TRUE) << 1
            | (MapObjMan_GetLocalMapObjByMovementType(fieldSystem->mapObjMan, MOVEMENT_TYPE_FOLLOW_PLAYER) != NULL) << 2
            | (SystemFlag_CheckHasPartner(SaveData_GetVarsFlags(fieldSystem->saveData)) != 0) << 3 | (MapObject_IsHidden(pl) != 0) << 4
            | (task != 0) << 5 | f->valid << 6;
        if (why != lastWhy) { PSPNativeMemLog("[FOL] state %d hide-bits %x (1 notwalk 2 dw 4 partnerobj 8 partnerflag 10 hidden 20 task 40 valid) map %d tile %d,%d", state, why, map, x, z); lastWhy = why; }
    }
    if (hide) { if (f->hiddenFrames < 100000) f->hiddenFrames++; f->valid = 0; f->map = map; f->px = x; f->pz = z; f->ppos = pos; f->lpos = pos; f->lx = x; f->lz = z; return; }
    if (x != f->px || z != f->pz) {
        int dx = x - f->px, dz = z - f->pz, ad = (dx < 0 ? -dx : dx) + (dz < 0 ? -dz : dz);
        /* a step already under way when a script/task starts (a trainer's sight, a sign) is still followed; a
           second one inside the same task is scripted movement (cutscene, rock climb, door exit): hide */
        if (task) f->taskSteps++;
        if (f->taskSteps > 1 || ad > 2 || (dx && dz)) {
            /* scripted / warped / rock climb etc: hide until the next normal step */
            f->valid = 0;
        } else {
            VecFx32 L = f->ppos; /* the player stood on the tile it just left */
            if (!f->valid) { f->cur = L; f->valid = 1;
                /* run 64 follower_ballpop=1 (default): after a warp / door / scripted scene (hidden >= 20 frames) the
                   follower comes out of its ball: ball sound (the game's own SE) + it grows in over 12 frames. Render only. */
                { static int bp = -1; if (bp < 0) { bp = P3DS_CfgInt("follower_ballpop", 1) != 0; PSPNativeMemLog("[FOL] follower_ballpop=%d", bp); }
                  if (bp && f->hiddenFrames >= 20) { f->popT = 12; Sound_PlayEffect(SEQ_SE_DP_BOWA_sseq); PSPNativeMemLog("[FOL] ball pop after %d hidden frames", f->hiddenFrames); } } }
            f->from = f->cur;
            f->to = L;
            f->steps = ad;
            fx32 mdx = f->to.x - f->from.x, mdz = f->to.z - f->from.z;
            if (p3dsFolAbs(mdx) > 3 * 16 * FX32_ONE || p3dsFolAbs(mdz) > 3 * 16 * FX32_ONE) f->from = f->to; /* out of reach: snap */
            mdx = f->to.x - f->from.x; mdz = f->to.z - f->from.z;
            if (mdx || mdz) f->dir = p3dsFolAbs(mdx) >= p3dsFolAbs(mdz) ? (mdx < 0 ? 2 : 3) : (mdz < 0 ? 0 : 1);
            else f->dir = dz < 0 ? 0 : dz > 0 ? 1 : dx < 0 ? 2 : 3;
            f->lpos = L;
            PSPNativeMemLog("[FOL] step %d,%d->%d,%d dir %d mdx %d mdz %d", f->px, f->pz, x, z, f->dir, mdx >> 12, mdz >> 12);
            f->tileSteps++;
        }
        f->px = x; f->pz = z;
    }
    if (!task) f->taskSteps = 0;
    f->ppos = pos;
    if (!f->valid) { if (f->hiddenFrames < 100000) f->hiddenFrames++; return; }
    f->hiddenFrames = 0;
    if (!P3DS_FollowerMon(fieldSystem, f)) return;
    /* progress of the player's current step from lpos towards its new tile */
    fx32 done = p3dsFolAbs(pos.x - f->lpos.x) + p3dsFolAbs(pos.z - f->lpos.z), len = f->steps * 16 * FX32_ONE;
    fx32 t = len > 0 ? (fx32)(((s64)done << 12) / len) : FX32_ONE;
    if (t > FX32_ONE) t = FX32_ONE;
    if (t < 0) t = 0;
    f->moving = t < FX32_ONE && (f->from.x != f->to.x || f->from.z != f->to.z);
    f->cur.x = f->from.x + FX_Mul(f->to.x - f->from.x, t);
    f->cur.y = f->from.y + FX_Mul(f->to.y - f->from.y, t);
    f->cur.z = f->from.z + FX_Mul(f->to.z - f->from.z, t);
    if (!f->moving) f->from = f->cur;
    /* don't stand on the player (first frame after a reversal etc.) */
    if (p3dsFolAbs(f->cur.x - pos.x) < 4 * FX32_ONE && p3dsFolAbs(f->cur.z - pos.z) < 4 * FX32_ONE && !f->moving && !p3dsFolDbg()) return;
    f->anim++;
    P3DS_FollowerTalk(fieldSystem, f, pos, task);

    /* draw: view-space quad at the follower's feet, bottom-centred, w x w world units (1 texel = 1 unit) */
    NNS_G3dGeFlushBuffer();
    /* anchored at the sprite centre (w/2 above the feet) like the overworld billboards, so the depth order against
       the player/NPC sprites matches the game's: south of the player = in front, north = behind */
    VecFx32 v, c = f->cur;
    if (p3dsFolGap() && !p3dsFolDbg()) {
        fx32 ddx = f->cur.x - pos.x, ddz = f->cur.z - pos.z;
        /* behind the player on screen (north) the sprite rises over the player's head: more room there */
        int north = ddz < -2 * FX32_ONE && p3dsFolAbs(ddx) <= p3dsFolAbs(ddz);
        fx32 g = ((f->w == 64 ? (north ? 12 : 8) : (north ? 8 : 4)) + (p3dsFolBig(f->species) ? 8 : 0)) * FX32_ONE;
        if (g) { if (ddx > 2 * FX32_ONE) c.x += g; else if (ddx < -2 * FX32_ONE) c.x -= g; if (ddz > 2 * FX32_ONE) c.z += g; else if (ddz < -2 * FX32_ONE) c.z -= g; }
    }
    c.y += (f->w / 2) * FX32_ONE;
    if (p3dsFolDbg()) c = pos, c.y += (f->w / 2) * FX32_ONE, c.x += 32 * FX32_ONE; /* calibration: beside the player */
    MTX_MultVec43(&c, NNS_G3dGlbGetCameraMtx(), &v);
    v.y += 3 * FX32_ONE; /* HGSS frames sit ~3 px lower in their 32x32 cell than Platinum's (emu-measured) */
    /* run 59: never in front of the player. A follower on the tile south of the player (walking up) is nearer the
       camera, and its 32-px sprite then covered the player's legs ("sitting on" the player). Within 1.5 tiles of the
       player its quad is pushed to just behind the player's sprite depth (view z, camera looks down -z), so the player
       always overlaps it, like HGSS. Off = follower_behind=0. */
    if (p3dsFolBehind() && !p3dsFolDbg() && p3dsFolAbs(f->cur.x - pos.x) < 24 * FX32_ONE && p3dsFolAbs(f->cur.z - pos.z) < 24 * FX32_ONE) {
        VecFx32 pc = pos, pv;
        pc.y += 16 * FX32_ONE;
        MTX_MultVec43(&pc, NNS_G3dGlbGetCameraMtx(), &pv);
        if (v.z > pv.z - 2 * FX32_ONE) v.z = pv.z - 2 * FX32_ONE;
    }
    int w = f->w, fr = f->dir * 2 + ((f->anim >> (f->moving ? 3 : 4)) & 1), flip = 0;
    if (!p3dsFolSideFix()) { /* run 58 mapping (side views backwards) */
        if (f->dir == 2) flip = 1;
        if (f->dir == 3) fr = 4 + ((f->anim >> (f->moving ? 3 : 4)) & 1); }
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_PushMtx();
    G3_Identity();
    G3_Translate(v.x, v.y, v.z);
    G3_Scale(16 * FX32_ONE, 16 * FX32_ONE, 16 * FX32_ONE);
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 0, 31, GX_POLYGON_ATTR_MISC_FOG);
    G3_TexImageParam(GX_TEXFMT_PLTT16, GX_TEXGEN_NONE, w == 64 ? GX_TEXSIZE_S64 : GX_TEXSIZE_S32, w == 64 ? GX_TEXSIZE_T64 : GX_TEXSIZE_T32,
        GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, 0x7FFF8);
    G3_TexPlttBase(0x1FF00 + fr * 16, GX_TEXFMT_PLTT16);
    G3_Color(GX_RGB(31, 31, 31));
    fx16 hw = (fx16)((w / 2) * FX16_ONE / 16);
    if (f->popT > 0) { hw = (fx16)(hw * (12 - f->popT + 1) / 13); f->popT--; }
    fx32 s0 = flip ? w * FX32_ONE : 0, s1 = flip ? 0 : w * FX32_ONE;
    G3_Begin(GX_BEGIN_QUADS);
    G3_TexCoord(s0, 0); G3_Vtx(-hw, hw, 0);
    G3_TexCoord(s0, w * FX32_ONE); G3_Vtx(-hw, -hw, 0);
    G3_TexCoord(s1, w * FX32_ONE); G3_Vtx(hw, -hw, 0);
    G3_TexCoord(s1, 0); G3_Vtx(hw, hw, 0);
    G3_End();
    G3_PopMtx(1);
}

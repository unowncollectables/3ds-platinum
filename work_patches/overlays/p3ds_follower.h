/* run 58 (PLATINUM3DS): following Pokemon, HGSS style, RENDER-ONLY.
   Not a MapObject: nothing is allocated in the map object manager, so it is never serialised into the save and can
   never be found by scripts (Cheryl/Mira/partner lookups use MapObjMan_* which cannot see it), and it has no
   collision. State lives in the port (P3DS_FolMem) so it survives the overlay reload around battles.
   Drawn as one textured quad right after BillboardLists_Draw() inside the same projection depth bias as the
   overworld sprites, through the normal G3 path -> g3soft/g3gpu -> stereo depth and wide view like every sprite.
   The texture is port-owned (followers.bin, see platform/g3soft.cpp folTex), selected by a magic TEXIMAGE address. */
#include "party.h"
#include "pokemon.h"
#include "system_flags.h"
#include "vars_flags.h"
#include "generated/movement_types.h"
#include "constants/player_avatar.h"

extern int P3DS_FollowerCfg(void);
extern int P3DS_FollowerOpen(void);
extern int P3DS_FollowerSprite(int species, int form, int female);
extern int P3DS_FollowerLoad(int spr, int shiny);
extern unsigned char P3DS_FolMem[256];
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
} P3DSFol;

extern int P3DS_CfgInt(const char *, int);
static int p3dsFolDbg(void) { static int v = -1; if (v < 0) v = P3DS_CfgInt("follower_debug", 0); return v; }
static int p3dsFolBehind(void) { static int v = -1; if (v < 0) v = P3DS_CfgInt("follower_behind", 1) != 0; return v; }
static fx32 p3dsFolAbs(fx32 v) { return v < 0 ? -v : v; }

static void P3DS_FollowerHide(P3DSFol *f) { f->valid = 0; }

static int P3DS_FollowerMon(FieldSystem *fieldSystem, P3DSFol *f)
{
    if (--f->monCheck > 0 && f->w) return f->w;
    f->monCheck = 30;
    Party *party = SaveData_GetParty(fieldSystem->saveData);
    if (!party || Party_GetCurrentCount(party) < 1) return 0;
    Pokemon *mon = Party_GetPokemonBySlotIndex(party, 0);
    if (!mon || Pokemon_GetValue(mon, MON_DATA_IS_EGG, NULL)) return 0;
    int species = Pokemon_GetValue(mon, MON_DATA_SPECIES, NULL), form = Pokemon_GetValue(mon, MON_DATA_FORM, NULL);
    int female = Pokemon_GetGender(mon) == 1, shiny = Pokemon_IsShiny(mon) ? 1 : 0;
    int key = species | form << 10 | female << 16 | shiny << 17;
    if (key == f->monKey && f->w) return f->w;
    int spr = P3DS_FollowerSprite(species, form, female);
    f->w = spr >= 0 ? P3DS_FollowerLoad(spr, shiny) : 0;
    f->monKey = key;
    PSPNativeMemLog("[FOL] lead species %d form %d female %d shiny %d -> sprite %d w %d", species, form, female, shiny, spr, f->w);
    return f->w;
}

static void P3DS_FollowerFrame(FieldSystem *fieldSystem)
{
    P3DSFol *f = (P3DSFol *)P3DS_FolMem;
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
    if (hide) { f->valid = 0; f->map = map; f->px = x; f->pz = z; f->ppos = pos; f->lpos = pos; f->lx = x; f->lz = z; return; }
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
            if (!f->valid) { f->cur = L; f->valid = 1; }
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
        }
        f->px = x; f->pz = z;
    }
    if (!task) f->taskSteps = 0;
    f->ppos = pos;
    if (!f->valid) return;
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

    /* draw: view-space quad at the follower's feet, bottom-centred, w x w world units (1 texel = 1 unit) */
    NNS_G3dGeFlushBuffer();
    /* anchored at the sprite centre (w/2 above the feet) like the overworld billboards, so the depth order against
       the player/NPC sprites matches the game's: south of the player = in front, north = behind */
    VecFx32 v, c = f->cur;
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
    if (f->dir == 2) flip = 1; /* the HGSS side frames face right; left = mirrored */
    if (f->dir == 3) fr = 4 + ((f->anim >> (f->moving ? 3 : 4)) & 1);
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
    fx32 s0 = flip ? w * FX32_ONE : 0, s1 = flip ? 0 : w * FX32_ONE;
    G3_Begin(GX_BEGIN_QUADS);
    G3_TexCoord(s0, 0); G3_Vtx(-hw, hw, 0);
    G3_TexCoord(s0, w * FX32_ONE); G3_Vtx(-hw, -hw, 0);
    G3_TexCoord(s1, w * FX32_ONE); G3_Vtx(hw, -hw, 0);
    G3_TexCoord(s1, 0); G3_Vtx(hw, hw, 0);
    G3_End();
    G3_PopMtx(1);
}

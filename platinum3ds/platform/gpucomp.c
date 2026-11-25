/* platinum3ds run 60: GPU 2D compositor stage 1 (gpu_compose=2, default 0 = off, never called).
 * Engine A only, and only for frames using the simple subset of the DS 2D engine:
 *   mode 0 text BGs (4bpp / 8bpp without ext palettes), BG0 = 3D layer at HOFS 0, plain OBJs (no affine / semi / window /
 *   bitmap), no windows, no colour special effects, no master brightness, no mosaic (or mosaic size 0), no capture,
 *   display mode 1. Everything else returns 0 and the caller uses the melonDS path unchanged.
 * How: every DS 8x8 tile actually visible this frame is looked up in a cache keyed by (VRAM address, bpp, palette
 *   bank). An entry is reused when its 32/64 source bytes and its palette-bank hash are unchanged (validated by content,
 *   so the cache is exact); otherwise the tile is re-decoded into its slot of a 512x1024 RGBA8 atlas (PICA tiled/Morton
 *   order, texel = final 8-bit colour, alpha 0 = transparent). BG tiles and OBJ tiles are then drawn as GPU quads in DS
 *   priority order (prio 3..0: BG3..BG0 of that prio, then OBJs of that prio from OAM 127 down to 0), nearest
 *   sampling, alpha test > 0, onto a 256x256 render target cleared to the backdrop; the 3D layer is a texture made
 *   from NativeFrame and drawn at BG0's slot. The result is read back with a display transfer (like g3gpu rb_sync)
 *   and byte-swapped into the compositor's 0xFFBBGGRR format.
 * Known CPU passes (flagged): 3D layer -> texture (256x192) and the readback byte swap (256x192). */
#include <3ds.h>
#include <citro3d.h>
#include "dcache3ds.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "blit_shbin.h"
static unsigned gcEngineB;
int P3DS_TitleWideRequested,P3DS_GcTitleWideValid;
u32 P3DS_GcTitleStrips[64*192];
static int gcTitleWide,gcTitleScale,titleReady;
static C3D_Tex titleTex;static C3D_RenderTarget*titleTarget;static u32*titleReadback;
static int titleInit(void){if(titleReady)return titleReady>0;
 if(!(C3D_TexInitVRAM(&titleTex,512,256,GPU_RGBA8)||C3D_TexInit(&titleTex,512,256,GPU_RGBA8)))return titleReady=-1,0;
 titleTarget=C3D_RenderTargetCreateFromTex(&titleTex,GPU_TEXFACE_2D,0,-1);titleReadback=linearAlloc(512*256*4);
 if(!titleTarget||!titleReadback){if(titleTarget)C3D_RenderTargetDelete(titleTarget);if(titleReadback)linearFree(titleReadback);C3D_TexDelete(&titleTex);return titleReady=-1,0;}titleReady=1;return 1;}

extern void PSPNativeMemLog(const char *, ...);
extern int P3DS_CfgInt(const char *, int);
#include "g3_affine_sample.h"
static int fadeV=-1,gcTileFx;
int P3DS_GcBFadesCfg(void){if(fadeV<0)fadeV=P3DS_CfgInt("gpu_b_fades",1)!=0;return fadeV;}
void P3DS_GcBFadesSet(int v){fadeV=v!=0;PSPNativeMemLog("[GCBFADE] gpu_b_fades=%d",fadeV);}
static u32 exactBrightness(u32 c,unsigned eff,unsigned f){
 u32 out=0xFF;for(int sh=8;sh<=24;sh+=8){unsigned v=(c>>sh)&255;v>>=2;
  v=eff==2?v+(((63-v)*f+8)>>4):v-((v*f+7)>>4);
  out|=((v<<2)|(v>>4))<<sh;}return out;
}
static int affineV=-1;
int P3DS_GcAffineCfg(void){if(affineV<0)affineV=P3DS_CfgInt("gpu_obj_affine",1)!=0;return affineV;}
void P3DS_GcAffineSet(int v){affineV=v!=0;PSPNativeMemLog("[GCAFF] gpu_obj_affine=%d",affineV);}

extern void P3DS_Crumb(const char *tag, void *caller);  /* watchdog crumbs (only on gpu_compose>=2 frames) */

#define AW 512
#define AH 1024
#define SLOTS ((AW / 8) * (AH / 8))
#define HSIZE 16384
#define MAXQ 12288
typedef struct { float x, y, u, v; } GV;
typedef struct { u32 key; u32 stamp; u32 palv; u16 slot; u8 empty, n; u8 raw[64]; } GE;
static GE *tab; static unsigned slotsUsed, frameNo;
static C3D_Tex atlas, tex3d, tex3dB, rtTex; static C3D_Tex *t3cur = &tex3d;
extern int P3DS_InField; static C3D_RenderTarget *rt;
static GV *vb; static u32 *rb;
static DVLB_s *dvlb; static shaderProgram_s prog; static int uProj; static int ready = -1;
static u8 mort[64];
static unsigned dirtyLo, dirtyHi;
unsigned P3DS_GcBlendOn, P3DS_GcFxFrames, P3DS_GcBlendFrames, P3DS_GcBlendRej;  /* gpu_blend stats, logged by render3ds.cpp */
static int blendSet = -1;  /* ss3ds: set from the settings app (System > GPU blend) before or after init */
int P3DS_GpuBlendCfg(void) { if (blendSet < 0) blendSet = P3DS_CfgInt("gpu_blend", 1) != 0; return blendSet; }
void P3DS_SetGpuBlendCfg(int v) { blendSet = v != 0; P3DS_GcBlendOn = (unsigned)blendSet; PSPNativeMemLog("[GPUC] gpu_blend=%u (set from settings)", P3DS_GcBlendOn); }
unsigned P3DS_GcWalkUs, P3DS_GcConvUs, P3DS_GcOutUs, P3DS_GcQuads, P3DS_GcDecoded, P3DS_GcResets, P3DS_GcCpuUs, P3DS_GcGpuUs;
/* [PERF4] (2026-10-27): the gpu+wait part of the last call split into FrameBegin wait (earlier GPU work, e.g. the direct 3D
   draw), FrameEnd (submit; with fps_tweaks 0 this includes citro3d's whole-linear-heap cache flush) and the wait for the
   compose + readback itself */
unsigned P3DS_GcBeginUs, P3DS_GcEndUs, P3DS_GcSyncUs;
extern int P3DS_FpsTweaks(void); extern void P3DS_FrameEnd(void);
unsigned P3DS_GcB3Frames;  /* fps_tweaks 4: frames whose 3D translucency was blended on the GPU */
extern C3D_Tex *G3G_DirectTex(const u32 *f);
unsigned P3DS_GcMbFrames;  /* fps_tweaks 3: frames composed with a master-brightness fade */

#define RB_FLAGS (GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

/* fps_tweaks 6 (+OVERLAP), ported from the Platinum port's run 118 frame_pacing (its overlap-only part: no frame of lag, which
 * showed a seam with the wide view on and Nathan always has it on). Stereo GPU compose: the left eye's draw + readback is queued
 * and NOT waited for; the CPU walks the right eye meanwhile, and the left pixels are converted while the GPU draws the right eye.
 * While one of our draws is queued, anything that writes what it reads (atlas tile decode, 3D texture convert) waits first
 * (gcIdle); vertices alternate between two buffers and the right eye has its own readback buffer. Platinum: ~1.2 ms per compose. */
static int gpuBusy, fpEye, fpDefer; static unsigned fpSub, fpOvN, fpWaitN, fpFinN;
static GV *vbA, *vbB; static u32 *rbR;
typedef struct { u32 *rbuf, *out; u16 mbr; } GJ; static GJ fpPend;
/* fps60: C3D_FrameBegin that does not leave the real picture of the last tick waiting for its hand-over to the LCD. The GPU wait here
   (the 3D frame, both pictures: 4 ms, with the CPU idle) is exactly when the LCD has taken the in-between picture (about one V-blank
   after the last present), so the hand-over is tried before and during it (P3DS_Fps60Poll never waits). Without a waiting hand-over
   this is plain C3D_FrameBegin(0). Run 840 log: the hand-over was missed (polls only at 3D draw calls, which come before the LCD had
   taken the in-between picture) in about half the ticks and the next present then idled 5-9 ms (tearfix) for it. */
extern int P3DS_Fps60Pending(void); extern void P3DS_Fps60Poll(void);
static void gcFrameBegin(void) {
    if (!P3DS_Fps60Pending()) { C3D_FrameBegin(0); return; }
    P3DS_Fps60Poll();
    while (!C3D_FrameBegin(C3D_FRAME_NONBLOCK)) { P3DS_Fps60Poll(); svcSleepThread(100000); }
}
static void gcIdle(void) { if (gpuBusy) { C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH); gpuBusy = 0; } }
static int fpOn(void) {
    if (P3DS_FpsTweaks() < 6) return 0;
    static int ok = -1; if (ok >= 0) return ok; if (!vb) return 0;  /* compositor not initialised yet: decide on a later call */
    vbA = vb; vbB = (GV *)linearAlloc(MAXQ * 6 * sizeof(GV)); rbR = (u32 *)linearAlloc(256 * 256 * 4);
    ok = vbA && vbB && rbR; PSPNativeMemLog("[GPUC] overlap (Speed tweaks 6) buffers: %s", ok ? "ready" : "out of linear memory, overlap off"); return ok; }
static void gcOut(const u32 *rbuf, u16 mbr, u32 *out);
/* eye 0/1 of the next compose; pair = the right eye is composed after it this frame (the left may then return unconverted) */
void P3DS_GcEye(int eye, int pair) { fpEye = eye & 1; fpDefer = !eye && pair && fpOn(); }
/* converts a left eye still pending (the right eye was not composed); the caller then owns its out buffer again */
/* 2026-11-04 direct screen (gc_direct, Display tab "Direct screen", default ON; SPEEDUPS.md stage 2, mono first): when the caller sets
   P3DS_GcDeferOut before a compose, the composed picture stays in the GPU target: no readback transfer, no wait for it, no CPU
   colour pass. gpuscale.c samples the target directly at present (P3DS_GcPendTex), so the upload is skipped too. Anything on the CPU
   that reads or draws on the picture first calls P3DS_GcResolve(), which does exactly what the compose used to do at its end. */
int P3DS_GcDeferOut; static int dfPend[2]; static u32 *dfOut[2]; static u16 dfMbr[2]; unsigned P3DS_GcDeferN, P3DS_GcResolveN;
static C3D_Tex rtTexR; static C3D_RenderTarget *rtR; static int rtROk = -1;   /* the right eye's own target (3D slider up): both eyes stay on the GPU */
void P3DS_GcRetryRightTarget(void) {
    if (rtROk == 0) rtROk = -1;
}
static C3D_Tex *dfTex(int e) { return e ? &rtTexR : &rtTex; }
static void dfFetch(int e, u32 *dst) {   /* exactly the end of a non-deferred compose: transfer, wait, colour pass */
    C3D_FrameBegin(0); C3D_SyncDisplayTransfer((u32 *)dfTex(e)->data, GX_BUFFER_DIM(256, 256), rb, GX_BUFFER_DIM(256, 256), RB_FLAGS); P3DS_FrameEnd();
    C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH); gpuBusy = 0;
    gcOut(rb, dfMbr[e], dst); P3DS_GcResolveN++; }
void P3DS_GcResolve(void) { for (int e = 0; e < 2; e++) if (dfPend[e]) { dfPend[e] = 0; P3DS_Crumb("gc-resolve", 0); dfFetch(e, dfOut[e]); } }
/* resolves both eyes; returns the left (or only) eye's buffer, NULL when nothing was pending */
u32 *P3DS_GcResolveOut(void) { if (!dfPend[0] && !dfPend[1]) return NULL; u32 *o = dfPend[0] ? dfOut[0] : NULL; P3DS_GcResolve(); return o; }
/* a CPU copy of the pending left-eye picture in dst; the picture stays pending (the present still samples the GPU target) */
int P3DS_GcResolveTo(u32 *dst) { if (!dfPend[0] || !dst) return 0; P3DS_Crumb("gc-resolve-to", 0); dfFetch(0, dst); return 1; }
/* the composed GPU picture for buf, or NULL when buf holds (or must hold) CPU pixels */
C3D_Tex *P3DS_GcPendTex(const u32 *buf) { for (int e = 0; e < 2; e++) if (dfPend[e] && buf && buf == dfOut[e]) return dfTex(e); return NULL; }
/* called by the present after its FrameBegin (which waited for the compose draws): the atlas may be written again */
/* a frame the compositor will NOT compose (ineligible: rotating/scaling OBJ such as the thrown Poke Ball, windows, ...) is drawn by the CPU into the same buffer, so a
   picture still pending from the last GPU-composed frame is stale: drop it, or the present (P3DS_GcPendTex) and topTouch() show / write back that old picture. */
void P3DS_GcDrop(void) { if (dfPend[0] || dfPend[1]) { gcIdle(); dfPend[0] = dfPend[1] = 0; } }
void P3DS_GcPresented(void) { if (dfPend[0] || dfPend[1]) gpuBusy = 0; }
void P3DS_GcFinish(void) { if (fpPend.out) { gcIdle(); gcOut(fpPend.rbuf, fpPend.mbr, fpPend.out); fpPend.out = 0; fpFinN++; } fpDefer = 0; fpEye = 0; }

static int init(void) {
    if (ready >= 0) return ready;
    ready = 0;
    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++)
        mort[y * 8 + x] = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
    tab = (GE *)calloc(HSIZE, sizeof(GE));
    vb = (GV *)linearAlloc(MAXQ * 6 * sizeof(GV));
    rb = (u32 *)linearAlloc(256 * 256 * 4);
    if (!tab || !vb || !rb) { PSPNativeMemLog("[GPUC] init: out of memory"); return 0; }
    if (!C3D_TexInit(&atlas, AW, AH, GPU_RGBA8) || !C3D_TexInit(&tex3d, 256, 256, GPU_RGBA8) || !C3D_TexInit(&tex3dB, 256, 256, GPU_RGBA8)) { PSPNativeMemLog("[GPUC] init: tex alloc failed"); return 0; }
    C3D_TexSetFilter(&atlas, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&atlas, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    C3D_TexSetFilter(&tex3dB, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&tex3dB, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); memset(tex3dB.data, 0, 256 * 256 * 4); P3DS_DcFlush(tex3dB.data, 256 * 256 * 4);
    C3D_TexSetFilter(&tex3d, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&tex3d, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    memset(atlas.data, 0, AW * AH * 4); memset(tex3d.data, 0, 256 * 256 * 4);
    P3DS_DcFlush(atlas.data, AW * AH * 4); P3DS_DcFlush(tex3d.data, 256 * 256 * 4);
    if (!C3D_TexInitVRAM(&rtTex, 256, 256, GPU_RGBA8)) { PSPNativeMemLog("[GPUC] init: VRAM target failed"); return 0; }
    rt = C3D_RenderTargetCreateFromTex(&rtTex, GPU_TEXFACE_2D, 0, -1); if (!rt) return 0;
    dvlb = DVLB_ParseFile((u32 *)blit_shbin, blit_shbin_size);
    shaderProgramInit(&prog); shaderProgramSetVsh(&prog, &dvlb->DVLE[0]);
    uProj = shaderInstanceGetUniformLocation(prog.vertexShader, "projection");
    P3DS_GcBlendOn = (unsigned)P3DS_GpuBlendCfg();
    PSPNativeMemLog("[GPUC] gpu_blend=%u (default 1 = GPU path; gpu_blend=0 in config.txt = alpha blend / brightness fade frames use the melonDS path)", P3DS_GcBlendOn);
    PSPNativeMemLog("[GPUC] GPU compositor ready: atlas %ux%u (%u tiles), %u quads max", AW, AH, SLOTS, MAXQ);
    ready = 1; return 1;
}

static inline u32 rg(const u8 *r, unsigned o) { return *(const u32 *)(r + o); }
static inline u16 rh(const u8 *r, unsigned o) { return *(const u16 *)(r + o); }
static const s32 SW[16] = { 8,16,8,8, 16,32,8,8, 32,32,16,8, 64,64,32,8 };
static const s32 SH[16] = { 8,8,16,8, 16,8,32,8, 32,16,32,8, 64,32,64,8 };

/* [GPUC] reject detail (2026-10-27 fps thread): why alpha-blend frames stay on the CPU compositor (battles: 140 of 300 frames).
   Clause: 1 = any second target set while BG0 is the 3D layer (P3DS_GcEligible), 2 = a 2D layer drawn above something that is not a
   second target, 3 = ... above a layer that is itself a first target, 4 = backdrop is not a second target (GPU would blend where DS
   draws opaque), 5 = the 3D layer itself is a blend first target. Keyed by BLDCNT, BLDALPHA, which BGs/OBJ are on and BG0=3D. */
typedef struct { u16 bl, alpha; u16 on; u8 clause, d3; u32 n; } RejE;
static RejE rejT[12]; static unsigned rejOther;
static void rejNote(u16 bl, u16 alpha, u32 dc, int clause) {
    const u16 on = (u16)(((dc >> 8) & 31) | (((dc & 8) ? 1u : 0u) << 5)); const u8 d3 = (dc & 8) != 0;
    for (int i = 0; i < 12; i++) { RejE *e = &rejT[i];
        if (e->n && e->bl == bl && e->alpha == alpha && e->on == on && e->clause == clause) { e->n++; return; }
        if (!e->n) { e->bl = bl; e->alpha = alpha; e->on = on; e->clause = (u8)clause; e->d3 = d3; e->n = 1; return; } }
    rejOther++;
}
void P3DS_GcRejLog(void) {
    for (int i = 0; i < 12; i++) { RejE *e = &rejT[i]; if (!e->n) continue;
        PSPNativeMemLog("[GPUC] blend reject x%u: clause %d | BLDCNT %04x (1st %02x eff %u 2nd %02x) BLDALPHA %04x | layers on BG0-3,OBJ %02x 3D-on-BG0 %u", e->n, e->clause, e->bl, e->bl & 0x3F, (e->bl >> 6) & 3, (e->bl >> 8) & 0x3F, e->alpha, e->on & 31, e->d3); }
    if (rejOther) PSPNativeMemLog("[GPUC] blend reject: %u more with other combinations", rejOther);
    memset(rejT, 0, sizeof rejT); rejOther = 0;
}
static int rejects3DBlend(const u8 *regs, u32 dc, u16 bl) {
    if (!(bl & 0x3F00) || (dc & 0x108) != 0x108) return 0;
    /* This battle mode blends the premultiplied 3D target against any lower layer.
       Other second-target masks with BG0=3D require the CPU blend rules. */
    return !(P3DS_FpsTweaks() >= 4 && (bl & 0x3F) == 0 &&
             ((bl >> 6) & 3) == 1 && (bl & 0x3F00) == 0x3F00 &&
             (rg(regs, 0x350) & 0x7FFF) == 0);
}
int P3DS_Gc3DBlendRejects(const u8 *regs) {
    const u16 bl = rh(regs, 0x50);
    if (!P3DS_GpuBlendCfg()) return (bl & 0x3F00) != 0 || ((bl & 0xC0) && (bl & 0x3F));
    return rejects3DBlend(regs, rg(regs, 0), bl);
}
/* 0 = this frame can be composed on the GPU; else a reason bit */
int P3DS_GcEligible(const u8 *regs, const u16 *oam) {
    if (ready < 0) init();  /* reads gpu_blend */
    const u32 dc = rg(regs, 0);
    if (((dc >> 16) & 3) != 1) return 1;
    if (dc & 0x80) return 2;
    if (dc & 0xE000) return 4;
    const unsigned mode = dc & 7;
    const u16 mos = rh(regs, 0x4C);
    for (int i = 0; i < 4; i++) { if (!(dc & (0x100u << i))) continue;
        if (i == 0 && (dc & 8)) { if (rh(regs, 0x10) & 0x1FF) return 8; continue; }
        const bool text = i < 2 || mode == 0 || (i == 2 && mode == 1); if (!text || mode > 5) return 16;
        const u16 c = rh(regs, 8 + i * 2);
        if ((c & 0x40) && (mos & 0xFF)) return 32;
        if ((c & 0x80) && (dc & (1u << 30))) return 64; }
    { const u16 bl = rh(regs, 0x50);
      if (P3DS_GcBlendOn) {
          if (rejects3DBlend(regs, dc, bl)) { rejNote(bl, rh(regs, 0x52), dc, 1); return 128; }
      }
      else if ((bl & 0x3F00) || ((bl & 0xC0) && (bl & 0x3F))) return 128; }
    { const u16 mb = rh(regs, 0x6C); const unsigned m = mb >> 14; if ((m == 1 || m == 2) && (mb & 0x1F) && P3DS_FpsTweaks() < 3) return 256; }  /* fps_tweaks 3: applied in the readback pass of P3DS_GcComposeA */
    if (rg(regs, 0x64) & 0x80000000u) return 512;
    unsigned affineArea=0;
    if (dc & 0x1000) for (int i = 0; i < 128; i++) { const u16 a0 = oam[i * 4]; if ((a0 & 0x300) == 0x200) continue;
        if (a0 & 0x100) {
            if(!P3DS_GcAffineCfg())return 1024;
            unsigned a1=oam[i*4+1],sp=(a0>>14)|((a1&0xC000)>>12);
            affineArea+=SW[sp]*SH[sp]*((a0&0x200)?4:1);
            if(affineArea>65536)return 1024;
        } if ((a0 >> 10) & 3) return 2048;
        if ((a0 & 0x1000) && (mos >> 8)) return 4096;
        if ((a0 & 0x2000) && (dc & 0x80000000u)) return 8192; }
    return 0;
}

static inline u32 col8(u16 c) { /* DS 15-bit -> melonDS final 8-bit, as PICA RGBA8 R<<24|G<<16|B<<8|A */
    const u32 r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    return (((r << 3) | (r >> 3)) << 24) | (((g << 3) | (g >> 3)) << 16) | (((b << 3) | (b >> 3)) << 8) | 0xFF; }
static u32 palHash[2][2][17];  /* [bg/obj][bank 0-15, 16 = 256-colour] */
static u32 lut[2][2][256];
static void palPrep(const u16 *pal, int k, unsigned eff, unsigned f) {
    for (int b = 0; b < 16; b++) { u32 h = 2166136261u; for (int i = 0; i < 16; i++) h = (h ^ pal[b * 16 + i]) * 16777619u; palHash[k][0][b] = h; }
    { u32 h = 2166136261u; for (int i = 0; i < 256; i++) h = (h ^ pal[i]) * 16777619u; palHash[k][0][16] = h; }
    for (int i = 0; i < 256; i++) lut[k][0][i] = col8(pal[i]);
    if(eff>=2){
      for(int i=0;i<256;i++)lut[k][1][i]=exactBrightness(lut[k][0][i],eff,f);
      for(int b=0;b<16;b++){u32 h=2166136261u;for(int i=0;i<16;i++)h=(h^lut[k][1][b*16+i])*16777619u;palHash[k][1][b]=h;}
      u32 h=2166136261u;for(int i=0;i<256;i++)h=(h^lut[k][1][i])*16777619u;palHash[k][1][16]=h;
    }
}
/* key: bit 24 obj, bit 23 8bpp, bits 19-22 bank, bits 0-18 address (32-byte aligned) */
static int fail;
static int tile(int obj, int bpp8, unsigned bank, const u8 *src, unsigned addr) {
    const u32 key = (1u << 31) | (gcEngineB << 25) | ((u32)gcTileFx << 27) | ((u32)obj << 24) | ((u32)bpp8 << 23) | ((bpp8 ? 0 : bank) << 19) | addr;
    unsigned h = ((key * 2654435761u) >> 18) & (HSIZE - 1);
    GE *e;
    for (;;) { e = &tab[h]; if (e->key == key || !e->key) break; h = (h + 1) & (HSIZE - 1); }
    const unsigned n = bpp8 ? 64 : 32; const u32 pv = palHash[obj][gcTileFx][bpp8 ? 16 : bank];
    if (e->key == key) {
        if (e->stamp == frameNo) return e->empty ? -1 : e->slot;
        e->stamp = frameNo;
        if (e->palv == pv && !memcmp(e->raw, src, n)) return e->empty ? -1 : e->slot;
    } else {
        if (slotsUsed >= SLOTS || slotsUsed * 2 >= HSIZE) { fail = 1; return -1; }
        e->key = key; e->slot = slotsUsed++; e->stamp = frameNo;
    }
    gcIdle();  /* overlap: the queued left-eye draw may still sample this atlas slot */
    memcpy(e->raw, src, n); e->palv = pv; P3DS_GcDecoded++;
    unsigned any = 0; for (unsigned i = 0; i < n; i++) any |= src[i];
    e->empty = !any; if (!any) return -1;
    const unsigned s = e->slot, tx = s % (AW / 8), ty = s / (AW / 8);
    u32 *d = (u32 *)atlas.data + (ty * (AW / 8) + tx) * 64;
    const u32 *L = lut[obj][gcTileFx];
    for (int j = 0; j < 8; j++) { const u8 *m = mort + (7 - j) * 8;  /* texture memory row r <-> t = 1 - r/H (measured): DS row j -> tile row 7-j */
        if (bpp8) { const u8 *p = src + j * 8; for (int x = 0; x < 8; x++) { const unsigned c = p[x]; d[m[x]] = c ? L[c] : 0; } }
        else { const u8 *p = src + j * 4; const unsigned pb = bank * 16;
            for (int x = 0; x < 8; x++) { const unsigned c = (p[x >> 1] >> ((x & 1) * 4)) & 15; d[m[x]] = c ? L[pb + c] : 0; } } }
    const unsigned lo = (ty * (AW / 8) + tx) * 256; if (lo < dirtyLo) dirtyLo = lo; if (lo + 256 > dirtyHi) dirtyHi = lo + 256;
    return s;
}
static unsigned nq;
static u32 blendA, blendB, blendY;  /* clamped BLDALPHA EVA/EVB and BLDY EVY of the frame being composed */
static void quad(int slot, float x, float y, int hf, int vf) {
    if (nq >= MAXQ) { fail = 1; return; }
    const unsigned tx = slot % (AW / 8), ty = slot / (AW / 8);
    float u0 = tx * 8.0f / AW, u1 = (tx * 8 + 8.0f) / AW, vt = 1.0f - (ty * 8 + 8.0f) / AH, vb_ = 1.0f - ty * 8.0f / AH;
    if (hf) { float t = u0; u0 = u1; u1 = t; }
    if (vf) { float t = vt; vt = vb_; vb_ = t; }
    float x1=x+8;
    if(gcTitleWide){
      if(gcTitleScale){x=x*1.25f;x1=x1*1.25f;const float bias=(hf?-1.0f:1.0f)/(128.0f*AW);u0+=bias;u1+=bias;}
      else{if(x1<=0||x>=256)return;float oldU0=u0,du=(u1-u0)/8;
        if(x<0){u0=oldU0-x*du;x=0;}if(x1>256){u1-=du*(x1-256);x1=256;}x+=32;x1+=32;}
    }
    GV *v = vb + nq * 6; nq++;
    v[0] = (GV){ x, y, u0, vt }; v[1] = (GV){ x, y + 8, u0, vb_ }; v[2] = (GV){ x1, y, u1, vt };
    v[3] = (GV){ x1, y, u1, vt }; v[4] = (GV){ x, y + 8, u0, vb_ }; v[5] = (GV){ x1, y + 8, u1, vb_ };
}
typedef struct { unsigned first, count; int is3d; int fx; } GB;  /* fx: 0 none, 1 alpha blend, 2 brighten, 3 darken (BLDCNT effect of this layer) */
static GB bat[32]; static unsigned nb;
static void batchBegin(int is3d, int fx) { if (nb && bat[nb - 1].is3d == is3d && !is3d && bat[nb - 1].fx == fx) return; bat[nb].first = nq; bat[nb].is3d = is3d; bat[nb].fx = fx; bat[nb].count = 0; nb++; }
static void batchEnd(void) { bat[nb - 1].count = nq - bat[nb - 1].first; }

/* per-frame memo per BG: map entry -> tile result (most map entries repeat, e.g. the blank tile) */
static struct { u32 stamp; u16 me; s16 slot; } memo[4][1024];
/* #589 (stereo_2d=4 FOE DEEPER): set by render3ds only around the right-eye battle compose. Layers with P3DS_GcRowOn[i] set get a
 * per tile row shift P3DS_BattleRowExtra(row centre y, P3DS_GcRowSd) px to the right (ground plane: sky deepest, ground forward);
 * P3DS_GcRowSd -1 = off. */
int P3DS_GcRowSd = -1; int P3DS_GcRowOn[4];
extern int P3DS_BattleRowExtra(int y, int sd);
static void drawBG(const u8 *regs, const u8 *vram, u32 dc, int i) {
    gcTitleScale=gcTitleWide&&i==3;
    const u16 cnt = rh(regs, 8 + i * 2);
    const unsigned charb = ((cnt >> 2) & 15) * 0x4000 + ((dc >> 24) & 7) * 0x10000;
    const unsigned mapb = ((cnt >> 8) & 31) * 0x800 + ((dc >> 27) & 7) * 0x10000;
    const unsigned sz = cnt >> 14, w = (sz & 1) ? 64 : 32, h = (sz & 2) ? 64 : 32;
    const int bpp8 = (cnt & 0x80) != 0;
    const unsigned hofs = rh(regs, 0x10 + i * 4) & 0x1FF, vofs = rh(regs, 0x12 + i * 4) & 0x1FF;
    const int rowOn = (P3DS_GcRowSd >= 0 && !gcEngineB && P3DS_GcRowOn[i]);
    for (unsigned r = 0; r < 25; r++) { const unsigned wy = (vofs >> 3) + r, ty = wy & (h - 1); const float sy = r * 8.0f - (vofs & 7);
        if (sy >= 192) break;
        const int rowDx = rowOn ? -P3DS_BattleRowExtra((int)(sy + 4.0f), P3DS_GcRowSd) : 0;  /* one row = one 8 px tile row; + moves left */
        for (int k = rowOn ? -1 : 0; k < (rowOn ? 35 : 33); k++) { const unsigned wx = (hofs >> 3) + (unsigned)k, tx = wx & (w - 1); const float sx = k * 8.0f - (hofs & 7) - rowDx;
            const unsigned ma = (mapb + ((tx >> 5) + (ty >> 5) * (w >> 5)) * 0x800 + ((ty & 31) * 32 + (tx & 31)) * 2) & (gcEngineB ? 0x1FFFF : 0x7FFFF);
            const u16 me = *(const u16 *)(vram + ma);
            const unsigned mi = (me ^ (me >> 10)) & 1023; const u16 mk = me & 0xF3FF;
            int s;
            if (memo[i][mi].stamp == frameNo && memo[i][mi].me == mk) s = memo[i][mi].slot;
            else { const unsigned ta = (charb + (me & 0x3FF) * (bpp8 ? 64 : 32)) & (gcEngineB ? 0x1FFFF : 0x7FFFF);
                s = tile(0, bpp8, me >> 12, vram + ta, ta); memo[i][mi].stamp = frameNo; memo[i][mi].me = mk; memo[i][mi].slot = (s16)s; }
            if (s >= 0) quad(s, sx, sy, me & 0x400, me & 0x800); } }
}
/* Rasterize only affine OBJ indices with the original signed 8.8 sampler.
 * Existing atlas upload, palette conversion and priority batching stay shared. */
static void drawAffineOBJ(int n,const u16*oam,const u8*ov,u32 dc,u16 a0,u16 a1,u16 a2,int w,int h){
 const int bw=w*((a0&0x200)?2:1),bh=h*((a0&0x200)?2:1);
 const int x=(s32)((u32)a1<<23)>>23,y=a0&255;
 if(x>=256||x+bw<=0)return;
 const u16*m=oam+((a1>>9)&31)*16;
 const int pa=(s16)m[3],pb=(s16)m[7],pc=(s16)m[11],pd=(s16)m[15];
 const int bpp8=(a0&0x2000)!=0;const unsigned mask=gcEngineB?0x1FFFF:0x3FFFF;
 for(int ty=0;ty<bh;ty+=8)for(int tx=0;tx<bw;tx+=8){
  int sx=x+tx,sy=y+ty;
  if(sx>=256||sx+8<=0||(sy>=192&&sy+8<=256))continue;
  u8 raw[64]={0};
  for(int py=0;py<8;py++){
   int rx=(tx-bw/2)*pa+(ty+py-bh/2)*pb+(w<<7);
   int ry=(tx-bw/2)*pc+(ty+py-bh/2)*pd+(h<<7);
   for(int px=0;px<8;px++,rx+=pa,ry+=pc){
    unsigned c=P3DS_AffineIndex(ov,mask,dc,a2&1023,w,h,bpp8,rx,ry);
    if(bpp8)raw[py*8+px]=c;else raw[py*4+px/2]|=c<<((px&1)*4);
   }
  }
  unsigned key=0x04000000u|((unsigned)n<<8)|(ty/8*16+tx/8);
  int slot=tile(1,bpp8,a2>>12,raw,key);if(slot<0)continue;
  if(sy<192)quad(slot,sx,sy,0,0);
  if(sy+8>256)quad(slot,sx,sy-256,0,0);
 }
}
static void drawOBJ(const u16 *oam, const u8 *ov, u32 dc, unsigned prio) {
    gcTitleScale=0;
    for (int n = 127; n >= 0; n--) { const u16 a0 = oam[n * 4], a1 = oam[n * 4 + 1], a2 = oam[n * 4 + 2];
        if (((a2 >> 10) & 3) != prio || (a0 & 0x300) == 0x200) continue;
        const unsigned sp = (a0 >> 14) | ((a1 & 0xC000) >> 12); const int w = SW[sp], hh = SH[sp];
        if(a0&0x100){drawAffineOBJ(n,oam,ov,dc,a0,a1,a2,w,hh);continue;}
        int y = a0 & 255; if (y >= 192) y -= 256; if (y + hh <= 0) continue;
        const int x = (s32)((u32)a1 << 23) >> 23; if (x <= -w || x >= 256) continue;
        const int bpp8 = (a0 & 0x2000) != 0, hf = (a1 & 0x1000) != 0, vf = (a1 & 0x2000) != 0;
        const unsigned tn = a2 & 0x3FF, bank = a2 >> 12, ts = bpp8 ? 64 : 32;
        for (int r = 0; r < hh / 8; r++) for (int c = 0; c < w / 8; c++) {
            unsigned addr;
            if (dc & 0x10) addr = ((tn << ((dc >> 20) & 3)) << 5) + (r * (w / 8) + c) * ts;
            else addr = (tn << 5) + r * 1024 + c * ts;
            addr &= gcEngineB ? 0x1FFFF : 0x3FFFF;
            const int s = tile(1, bpp8, bank, ov + addr, addr);
            if (s < 0) continue;
            const int dcx = hf ? (w / 8 - 1 - c) : c, dry = vf ? (hh / 8 - 1 - r) : r;
            quad(s, (float)(x + dcx * 8), (float)(y + dry * 8), hf, vf); } }
}
/* run 17: outside the overworld the 3D layer is often unchanged between frames (battle backdrop, menus: ~3 ms of conversion per eye in the 2026-10-26 log).
   Each eye (frame buffer pointer) gets its own texture; a content hash decides whether the converted texture can be reused. Overworld: never hashed. */
static const u32 *t3ptr[2]; static u32 t3hash[2]; static int t3ok[2]; static unsigned t3hit, t3miss;
static void conv3dRaw(const u32 *f, C3D_Tex *tx);
extern C3D_Tex *G3G_DirectTex(const u32 *f); extern void G3G_DirectSynced(void);
static void conv3d(const u32 *f) {
    /* gpu_direct3d: g3gpu left this eye's 3D layer in its render target (tiled RGBA8, row 255-y = DS row y, same as conv3dRaw writes): sample it as is */
    { C3D_Tex *dt = G3G_DirectTex(f); if (dt) { t3cur = dt; return; } }
    int slot = (!t3ptr[0] || t3ptr[0] == f) ? 0 : (!t3ptr[1] || t3ptr[1] == f) ? 1 : -1;
    if (slot < 0) { t3ok[0] = 0; slot = 0; t3ptr[0] = f; }
    t3ptr[slot] = f; t3cur = slot ? &tex3dB : &tex3d;
    if (!P3DS_InField) {
        u32 h0 = 2166136261u, h1 = 0x9E3779B9u, h2 = 0x85EBCA6Bu, h3 = 0xC2B2AE35u;
        for (unsigned i = 0; i < 256 * 192; i += 4) { h0 = (h0 ^ f[i]) * 16777619u; h1 = (h1 ^ f[i + 1]) * 16777619u; h2 = (h2 ^ f[i + 2]) * 16777619u; h3 = (h3 ^ f[i + 3]) * 16777619u; }
        const u32 h = h0 ^ (h1 * 3u) ^ (h2 * 5u) ^ (h3 * 7u);
        if (t3ok[slot] && t3hash[slot] == h) t3hit++;
        else { conv3dRaw(f, t3cur); t3hash[slot] = h; t3ok[slot] = 1; t3miss++; }
        { static unsigned n; if (++n % 600 == 0) PSPNativeMemLog("[GPUC] 3D layer texture outside the overworld: reused %u, converted %u of the last 600 (~3 ms saved per reuse)", t3hit, t3miss), t3hit = t3miss = 0; }
        return;
    }
    t3ok[slot] = 0; conv3dRaw(f, t3cur);
}
typedef struct { const u32 *f; u32 *d; } C3Job;
extern void P3DS_ParFor(void (*fn)(void *, unsigned), void *ctx, unsigned n);
static void conv3dRows(void *ctx, unsigned k) {   /* 24 DS rows = 3 tile rows per chunk (fps_tweaks 7 splits them over two cores) */
    const C3Job *j = (const C3Job *)ctx;
    for (unsigned y = k * 24; y < k * 24 + 24; y++) { const unsigned my = 255 - y; const u8 *m = mort + (my & 7) * 8; u32 *drow = j->d + (my >> 3) * 32 * 64;
        const u32 *s = j->f + y * 256;
        for (unsigned tx = 0; tx < 32; tx++, s += 8) { u32 *t = drow + tx * 64; __builtin_prefetch(s + 8);
            for (unsigned x = 0; x < 8; x++) { const u32 c = s[x], v = c & 0x3F3F3Fu, e = (v << 2) | ((v >> 4) & 0x030303u);
                t[m[x]] = (c >> 24) ? (__builtin_bswap32(e) | 0xFFu) : 0u; } } }
}
static void conv3dRaw(const u32 *f, C3D_Tex *tx) {
    /* NativeFrame 6-bit 0xAA_BB_GG_RR (alpha 5-bit) -> PICA RGBA8 (R<<24|G<<16|B<<8|A), 8-bit expand like melonDS,
       alpha 0 -> transparent. Branch-free per pixel, 8 pixels per tile row with constant Morton offsets. */
    gcIdle();  /* overlap: a queued draw may still sample this texture */
    C3Job job = { f, (u32 *)tx->data };
    P3DS_ParFor(conv3dRows, &job, 8);
    P3DS_DcFlush(tx->data, 256 * 256 * 4);
}

/* GPU state for one layer's colour effect. 0: plain replace. 1: out = src*EVA/16 + dst*EVB/16 (blend constant: A = EVA, RGB = EVB).
 * 2/3: TEV interpolate texture with white/black by EVY/16 (colour only; texture alpha is kept for the alpha test). */
static void setFx(int fx) {
    C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, 0, 0); C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
    if (fx >= 2) {
        const u32 ca = (blendY * 255 + 8) >> 4, cc = fx == 2 ? 0xFF : 0;
        C3D_TexEnvColor(env, (ca << 24) | (cc << 16) | (cc << 8) | cc);
        C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_TEXTURE0, GPU_CONSTANT);
        C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA);
        C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
    } else { C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, 0, 0); C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE); }
    if (fx == 1) {
        const u32 a = (blendA * 255 + 8) >> 4, b = (blendB * 255 + 8) >> 4;
        C3D_BlendingColor((a << 24) | (b << 16) | (b << 8) | b);
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_CONSTANT_ALPHA, GPU_CONSTANT_COLOR, GPU_ONE, GPU_ZERO);
    } else C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
}

typedef struct { const u32 *r; u32 *o; const u8 *t; unsigned stride; } GoJob;
static void gcOutRows(void *ctx, unsigned k) {   /* 24 rows per chunk */
    const GoJob *j = (const GoJob *)ctx; const u32 *r = j->r + k * 24 * 256; u32 *o = j->o + k * 24 * 256; const u8 *t = j->t;
    if(j->stride!=256){
     for(unsigned y=k*24;y<k*24+24;y++){const u32*rr=j->r+y*j->stride;u32*oo=j->o+y*256;
      for(unsigned x=0;x<256;x++){u32 a=rr[x];oo[x]=t?((u32)t[a>>24]|((u32)t[(a>>16)&255]<<8)|((u32)t[(a>>8)&255]<<16)|0xFF000000u):(__builtin_bswap32(a)|0xFF000000u);}}
     return;
    }
    if (t) for (unsigned i = 0; i < 24 * 256; i++) { const u32 a = r[i];  /* PICA RGBA8 R<<24|G<<16|B<<8 -> 0xFFBBGGRR */
        o[i] = (u32)t[a >> 24] | ((u32)t[(a >> 16) & 255] << 8) | ((u32)t[(a >> 8) & 255] << 16) | 0xFF000000u; }
    else for (unsigned i = 0; i < 24 * 256; i += 8, r += 8, o += 8) { __builtin_prefetch(r + 24); __builtin_prefetch(o + 24, 1);
        for (unsigned x = 0; x < 8; x++) o[x] = __builtin_bswap32(r[x]) | 0xFF000000u; }
}
static void gcOutStride(const u32 *rbuf, u16 mbr, u32 *out,unsigned stride) {
    P3DS_DcInval(rbuf, stride * 192 * 4);
    const unsigned mbm = mbr >> 14, mbf = (mbr & 0x1F) > 16 ? 16 : (mbr & 0x1F);
    u8 t[256]; GoJob job = { rbuf, out, NULL, stride };
    if ((mbm == 1 || mbm == 2) && mbf) {
        /* fps_tweaks 3: DS master brightness (melonDS SoftRenderer, after the line is composed): on 6-bit channels,
           up c += ((63 - c) * f) >> 4, down c -= (c * f + 15) >> 4; every composed pixel is an exact 6-bit expansion
           (c8 = c6 << 2 | c6 >> 4), so c8 >> 2 recovers it. Table per channel value, built once per frame. */
        for (unsigned v = 0; v < 256; v++) { unsigned c = v >> 2; c = mbm == 1 ? c + (((63 - c) * mbf) >> 4) : c - ((c * mbf + 15) >> 4); t[v] = (u8)((c << 2) | (c >> 4)); }
        job.t = t; P3DS_GcMbFrames++; }
    P3DS_ParFor(gcOutRows, &job, 8);
}
static void gcOut(const u32*r,u16 m,u32*out){gcOutStride(r,m,out,256);}
static void titleStripOut(u16 mbr){
 P3DS_DcInval(titleReadback,512*192*4);unsigned mode=mbr>>14,f=mbr&31;if(f>16)f=16;
 for(unsigned y=0;y<192;y++)for(unsigned side=0;side<2;side++)for(unsigned x=0;x<32;x++){
  u32 c=titleReadback[y*512+(side?288:0)+x];
  if((mode==1||mode==2)&&f){u32 q=0xFF;for(int sh=8;sh<=24;sh+=8){unsigned v=((c>>sh)&255)>>2;v=mode==1?v+(((63-v)*f)>>4):v-((v*f+15)>>4);q|=((v<<2)|(v>>4))<<sh;}c=q;}
  P3DS_GcTitleStrips[y*64+side*32+x]=c;
 }P3DS_GcTitleWideValid=1;
}
/* the whole composed picture into tgt: every 2D batch of the frame just walked and the 3D layer quad sampling t3 */
/* RGB8 is a 24-bit memory fill: the first three little-endian bytes of the
   fill word are repeated. RGBA8 words in this file are R<<24|G<<16|B<<8|A,
   so that fill stores A,B,G. An opaque backdrop (A = 255) is then sampled as
   blue wherever nothing is drawn over the clear. RGB8 pixels are B,G,R. */
static u32 gcClearWord(const C3D_RenderTarget *tgt, u32 rgba) {
    if (tgt->frameBuf.colorFmt != GPU_RB_RGB8) return rgba;
    static int once;
    if (!once) {
        once = 1;
        PSPNativeMemLog("[FPS60] RGB8 in-between clear keeps the backdrop (word %08lx -> 24-bit fill %08lx)",
                        (unsigned long)rgba, (unsigned long)(rgba >> 8));
    }
    return rgba >> 8;
}
static void gcDrawAll(C3D_RenderTarget *tgt, u32 bd, C3D_Tex *t3, unsigned q3, int b3, GV *vbuf, float width) {
    C3D_RenderTargetClear(tgt, C3D_CLEAR_ALL, gcClearWord(tgt, bd), 0);
    C3D_FrameDrawOn(tgt);
    C3D_BindProgram(&prog);
    C3D_Mtx proj; Mtx_Ortho(&proj, 0.0f, width, 0.0f, 256.0f, 1.0f, -1.0f, true);  /* DS y 0 -> clip -1 (= g3gpu convention, FLIP_VERT readback gives rows top-first) */
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_AttrInfo *ai = C3D_GetAttrInfo(); AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 2); AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2);
    C3D_BufInfo *bi = C3D_GetBufInfo(); BufInfo_Init(bi); BufInfo_Add(bi, vbuf, sizeof(GV), 2, 0x10);
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR); C3D_CullFace(GPU_CULL_NONE);
    C3D_AlphaTest(true, GPU_GREATER, 0);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    { C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env); C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
      for (int k = 1; k < 6; k++) C3D_TexEnvInit(C3D_GetTexEnv(k)); }
    int curFx = 0;
    for (unsigned b = 0; b < nb; b++) { if (!bat[b].is3d && !bat[b].count) continue;
        if (bat[b].fx != curFx) { curFx = bat[b].fx; setFx(curFx); }
        if (bat[b].is3d) { C3D_TexBind(0, t3);
            if (b3) { C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ONE_MINUS_SRC_ALPHA); C3D_DrawArrays(GPU_TRIANGLES, q3 * 6, 6); curFx = -1; }
            else C3D_DrawArrays(GPU_TRIANGLES, q3 * 6, 6); }
        else { C3D_TexBind(0, &atlas); C3D_DrawArrays(GPU_TRIANGLES, bat[b].first * 6, bat[b].count * 6); } }
}
/* Each eye's midpoint has a separate target, so it can be drawn with
   the real image in the same command list using the same 2D vertices. */
static C3D_Tex rtTexM[2];
static C3D_RenderTarget *rtM[2];
static int rtMOk[2] = { -1, -1 };
static void gcMidDrop(void) {
    for (int e = 0; e < 2; e++) {
        if (rtM[e]) C3D_RenderTargetDelete(rtM[e]);
        rtM[e] = NULL;
        if (rtTexM[e].data) C3D_TexDelete(&rtTexM[e]);
        memset(&rtTexM[e], 0, sizeof rtTexM[e]);
        rtMOk[e] = -1;
    }
}
int P3DS_GcMidReserve(void) {
    for (int e = 0; e < 2; e++) {
        if (!C3D_TexInitVRAM(&rtTexM[e], 256, 256, GPU_RGB8)) { gcMidDrop(); return 0; }
        rtM[e] = C3D_RenderTargetCreateFromTex(&rtTexM[e], GPU_TEXFACE_2D, 0, -1);
        if (!rtM[e]) { gcMidDrop(); return 0; }
        C3D_TexSetFilter(&rtTexM[e], GPU_NEAREST, GPU_NEAREST);
    }
    return 1;
}
void P3DS_GcMidRelease(void) { gcMidDrop(); }
int P3DS_Fps60MidPlace(void) { return rtMOk[0] == 1 && rtMOk[1] == 1; }
extern int P3DS_Fps60VramPrepare(void);
static C3D_RenderTarget *gcMidTarget(int eye) {
    if (rtMOk[eye] < 0) {
        if (P3DS_Fps60VramPrepare()) {
            rtMOk[0] = rtMOk[1] = 1;
            PSPNativeMemLog("[FPS60] in-between picture targets from the VRAM reserve (free %lu)", (unsigned long)vramSpaceFree());
            return rtM[eye];
        }
        // The screen scaler samples these pictures as opaque RGB. Keeping
        // RGB8 saves 64 KiB per eye without reducing color precision.
        const GPU_TEXCOLOR formats[] = { GPU_RGB8, GPU_RGBA8 };
        for (unsigned i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i) {
            if (!C3D_TexInitVRAM(&rtTexM[eye], 256, 256, formats[i])) continue;
            rtM[eye] = C3D_RenderTargetCreateFromTex(&rtTexM[eye], GPU_TEXFACE_2D, 0, -1);
            if (rtM[eye]) {
                C3D_TexSetFilter(&rtTexM[eye], GPU_NEAREST, GPU_NEAREST);
                break;
            }
            C3D_TexDelete(&rtTexM[eye]);
        }
        rtMOk[eye] = rtM[eye] != NULL;
        PSPNativeMemLog("[FPS60] in-between picture target for eye %d %s (VRAM free %lu)",
                        eye, rtMOk[eye] ? "ready" : "FAILED: no in-between frames", (unsigned long)vramSpaceFree());
    }
    return rtM[eye];
}
/* returns 1 and fills out[256*192] (0xFFBBGGRR) on success; 0 = not composed (out untouched, caller falls back) */
int P3DS_GcComposeA(const u8 *regs, const u8 *bgv, const u8 *objv, const u16 *bgpal, const u16 *objpal, const u16 *oam, const u32 *f3d, u32 *out,
                    void *mid3d, void **midOut, unsigned *midUs) {
    if (midOut) *midOut = NULL;
    P3DS_Crumb("gc-begin", 0);
    if (!init()) return 0;
    gcTitleWide=gcEngineB&&P3DS_TitleWideRequested;if(gcTitleWide&&!titleInit())return 0;
    const u64 t0 = svcGetSystemTick();
    /* a picture left on the GPU by the last compose is replaced now (its CPU buffer is written by this compose or the CPU fallback) */
    if (!fpEye) dfPend[0] = dfPend[1] = 0; else dfPend[1] = 0;
    if (P3DS_GcDeferOut && fpEye && rtROk < 0) {
        if (C3D_TexInitVRAM(&rtTexR, 256, 256, GPU_RGBA8)) {
            rtR = C3D_RenderTargetCreateFromTex(&rtTexR, GPU_TEXFACE_2D, 0, -1);
            if (!rtR) C3D_TexDelete(&rtTexR);
        }
        rtROk = rtR != NULL;
        PSPNativeMemLog("[GPUC] direct screen: right-eye target %s",
                        rtROk ? "ready (VRAM)" : "failed, the right eye takes the CPU path");
    }
    const int dfEye = P3DS_GcDeferOut && fpEye && rtROk == 1;   /* right eye into its own target */
    if (gpuBusy && !fpOn()) gcIdle();   /* one vertex buffer: a still queued deferred draw must be done before it is rewritten */
    frameNo++; fail = 0; nq = 0; nb = 0; dirtyLo = ~0u; dirtyHi = 0; P3DS_GcDecoded = 0;
    P3DS_GcBeginUs = P3DS_GcEndUs = P3DS_GcSyncUs = 0;
    const int ov = fpOn(); if (ov) vb = (fpSub & 1) ? vbB : vbA;  /* the other half may still be read by the queued draw */
    if (slotsUsed > SLOTS - 3000 || slotsUsed * 2 > HSIZE - 6000) { memset(tab, 0, HSIZE * sizeof(GE)); slotsUsed = 0; P3DS_GcResets++; }

    const u32 dc = rg(regs, 0);
    int has3d = 0;
    /* colour effects (gpu_blend=1). Layers are drawn bottom to top, so blending with "what is below" is GPU blending with
     * the render target. DS semantics (blend only with a second-target pixel directly below) are reproduced exactly when
     * every layer drawn below a first-target layer is a second target and not itself a first target, plus the backdrop;
     * anything else returns 0 and the melonDS path is used. Brightness up/down needs no second target: per-layer TEV. */
    const u32 bl = P3DS_GcBlendOn ? rh(regs, 0x50) : 0, eff = (bl >> 6) & 3, firstM = bl & 0x3F, secondM = (bl >> 8) & 0x3F;
    const u32 eva = rh(regs, 0x52) & 0x1F, evb = (rh(regs, 0x52) >> 8) & 0x1F, evy = rh(regs, 0x54) & 0x1F;
    blendA = eva > 16 ? 16 : eva; blendB = evb > 16 ? 16 : evb; blendY = evy > 16 ? 16 : evy;
    const int exactB=gcEngineB&&P3DS_GcBFadesCfg();gcTileFx=0;
    palPrep(bgpal,0,exactB?eff:0,blendY);palPrep(objpal,1,exactB?eff:0,blendY);
    u32 belowM = 0; int usedFx = 0;
    for (int p = 3; p >= 0; p--) {
        for (int i = 3; i >= 0; i--) { if (!(dc & (0x100u << i)) || (rh(regs, 8 + i * 2) & 3) != (unsigned)p) continue;
            const int fx = (eff && (firstM & (1u << i))) ? (int)eff : 0;
            if (i == 0 && (dc & 8)) { if (fx == 1) { P3DS_GcBlendRej++; rejNote(rh(regs, 0x50), rh(regs, 0x52), dc, 5); fail = 1; break; }
                batchBegin(1, fx); bat[nb - 1].count = 6; has3d = 1; belowM |= 1; if (fx) usedFx = 1; continue; }
            gcTileFx=exactB&&fx>=2; batchBegin(0, gcTileFx?0:fx); const unsigned q0 = nq; drawBG(regs, bgv, dc, i); batchEnd();
            if (nq > q0) { if (fx == 1 && ((belowM & ~secondM) || (belowM & firstM) || !(bl & 0x2000))) { P3DS_GcBlendRej++; rejNote(rh(regs, 0x50), rh(regs, 0x52), dc, (belowM & ~secondM) ? 2 : (belowM & firstM) ? 3 : 4); fail = 1; break; }
                belowM |= 1u << i; if (fx) usedFx = fx; } }
        if (fail) break;
        if (dc & 0x1000) { const int fx = (eff && (firstM & 0x10)) ? (int)eff : 0; gcTileFx=exactB&&fx>=2; batchBegin(0, gcTileFx?0:fx); const unsigned q0 = nq; drawOBJ(oam, objv, dc, p); batchEnd();
            if (nq > q0) { if (fx == 1 && ((belowM & ~secondM) || (belowM & firstM) || !(bl & 0x2000))) { P3DS_GcBlendRej++; rejNote(rh(regs, 0x50), rh(regs, 0x52), dc, (belowM & ~secondM) ? 2 : (belowM & firstM) ? 3 : 4); fail = 1; }
                belowM |= 0x10; if (fx) usedFx = fx; } }
        if (fail) break; }
    if (fail) { P3DS_GcCpuUs = (svcGetSystemTick() - t0) / 268; return 0; }
    if (usedFx == 1) P3DS_GcBlendFrames++; else if (usedFx) P3DS_GcFxFrames++;
    /* the 3D layer quad lives after the tile quads */
    const u64 tw = svcGetSystemTick(); P3DS_GcWalkUs = (unsigned)((tw - t0) / 268);
    const int b3 = has3d && P3DS_FpsTweaks() >= 4 && (bl & 0x3F) == 0 && eff == 1 && (bl & 0x3F00) == 0x3F00 && (rg(regs, 0x350) & 0x7FFF) == 0;
    if (b3 && !G3G_DirectTex(f3d)) { P3DS_GcCpuUs = (svcGetSystemTick() - t0) / 268; return 0; }  /* needs the premultiplied GPU target, not the CPU readback */
    if (b3) P3DS_GcB3Frames++;
    unsigned q3 = nq; if (has3d) { GV *v = vb + nq * 6; nq++;
        const float vt = 0.0f, vbm = 192.0f / 256;
        v[0] = (GV){ 0, 0, 0, vt }; v[1] = (GV){ 0, 192, 0, vbm }; v[2] = (GV){ 256, 0, 1, vt };
        v[3] = (GV){ 256, 0, 1, vt }; v[4] = (GV){ 0, 192, 0, vbm }; v[5] = (GV){ 256, 192, 1, vbm };
        conv3d(f3d); }
    if (dirtyHi > dirtyLo) P3DS_DcFlush((u8 *)atlas.data + dirtyLo, dirtyHi - dirtyLo);
    P3DS_DcFlush(vb, nq * 6 * sizeof(GV));
    P3DS_GcQuads = nq;
    const u64 t1 = svcGetSystemTick(); P3DS_GcConvUs = (unsigned)((t1 - tw) / 268);
    u32 bd = col8(bgpal[0]);
    if (exactB && eff >= 2 && (firstM & 0x20)) bd=exactBrightness(bd,eff,blendY);
    else if (eff >= 2 && (firstM & 0x20)) {  /* backdrop as first target of brightness up/down (bias 8 / 7, as ColorBrightness*) */
        u32 c[3] = { bd >> 24, (bd >> 16) & 255, (bd >> 8) & 255 };
        for (int k = 0; k < 3; k++) c[k] = eff == 2 ? c[k] + (((255 - c[k]) * blendY + 8) >> 4) : c[k] - ((c[k] * blendY + 7) >> 4);
        bd = (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | 0xFF; }
    if (fpEye && !dfEye && dfPend[0]) { dfPend[0] = 0; dfFetch(0, dfOut[0]); }   /* the left eye was left in rt, which this right eye reuses */
    const u16 mbr = rh(regs, 0x6C);
    const int direct = P3DS_GcDeferOut && (!fpEye || dfEye) &&
                       !(((mbr >> 14) == 1 || (mbr >> 14) == 2) && (mbr & 0x1F));
    C3D_RenderTarget *const midTarget = direct && has3d && !gcTitleWide &&
                                       !gcEngineB && mid3d && midOut ? gcMidTarget(dfEye) : NULL;
    P3DS_Crumb("gc-draw", 0);
    { const u64 tb = svcGetSystemTick(); gcFrameBegin(); P3DS_GcBeginUs = (unsigned)((svcGetSystemTick() - tb) / 268); }
    gpuBusy = 0;
    G3G_DirectSynced();   /* FrameBegin waited for every queued draw, including g3gpu's direct 3D frame */
    C3D_RenderTarget *const tgt = gcTitleWide ? titleTarget : (dfEye ? rtR : rt);
    gcDrawAll(tgt, bd, t3cur, q3, b3, vb, gcTitleWide ? 512.0f : 256.0f);
    if (midTarget) {
        const u64 started = svcGetSystemTick();
        gcDrawAll(midTarget, bd, mid3d, q3, b3, vb, 256.0f);
        *midOut = &rtTexM[dfEye];
        if (midUs) *midUs += (unsigned)((svcGetSystemTick() - started) / CPU_TICKS_PER_USEC);
    }
    u32 *const rbuf = gcTitleWide ? titleReadback : (ov && fpEye ? rbR : rb);
    if (direct) {
        const u64 te = svcGetSystemTick(); P3DS_FrameEnd(); P3DS_GcEndUs = (unsigned)((svcGetSystemTick() - te) / 268);
        if (ov) fpSub++; fpDefer = 0; fpEye = 0;
        dfPend[dfEye] = 1; dfOut[dfEye] = out; dfMbr[dfEye] = mbr; gpuBusy = 1; P3DS_GcDeferN++; P3DS_GcOutUs = 0; P3DS_GcGpuUs = 0;
        P3DS_GcCpuUs = (unsigned)((t1 - t0) / 268); P3DS_Crumb("gc-deferred", 0); return 1; }
    C3D_SyncDisplayTransfer((u32 *)(gcTitleWide ? titleTex.data : (dfEye ? rtTexR.data : rtTex.data)), GX_BUFFER_DIM(gcTitleWide?512:256, 256), rbuf, GX_BUFFER_DIM(gcTitleWide?512:256, 256), RB_FLAGS);
    { const u64 te = svcGetSystemTick(); P3DS_FrameEnd(); P3DS_GcEndUs = (unsigned)((svcGetSystemTick() - te) / 268); }
    if (ov) { gpuBusy = 1; fpSub++;
        if (fpPend.out) { gcOut(fpPend.rbuf, fpPend.mbr, fpPend.out); fpPend.out = 0; }  /* the left eye: done before this draw began (FrameBegin waited) */
        if (fpDefer) { fpPend = (GJ){ rbuf, out, mbr }; fpDefer = 0; fpOvN++;  /* converted during the right eye's draw, or by P3DS_GcFinish */
            P3DS_GcOutUs = 0; P3DS_GcCpuUs = (unsigned)((t1 - t0) / 268); P3DS_GcGpuUs = 0;
            { static unsigned lg; if (++lg % 600 == 0) { PSPNativeMemLog("[GPUC] overlap: last 600 left eyes: %u not waited for, %u converted late by finish; %u composes waited", fpOvN, fpFinN, fpWaitN); fpOvN = fpFinN = fpWaitN = 0; } }
            return 1; }
        fpWaitN++; }
    P3DS_Crumb("gc-wait", 0);
    { const u64 ts = svcGetSystemTick(); C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH); P3DS_GcSyncUs = (unsigned)((svcGetSystemTick() - ts) / 268); }
    gpuBusy = 0; fpEye = 0;
    P3DS_Crumb("gc-done", 0);
    const u64 t2 = svcGetSystemTick();
    if(gcTitleWide){gcOutStride(rbuf+32,mbr,out,512);titleStripOut(mbr);}else gcOut(rbuf, mbr, out);
    const u64 t3 = svcGetSystemTick(); P3DS_GcOutUs = (unsigned)((t3 - t2) / 268);
    P3DS_GcCpuUs = (unsigned)((t1 - t0 + t3 - t2) / 268); P3DS_GcGpuUs = (unsigned)((t2 - t1) / 268);
    return 1;
}

/* Main-thread only. The output is fully resolved before engine A reuses the
 * shared target/atlas. Engine-specific tile keys prevent A/B atlas churn. */
int P3DS_GcComposeB(const u8*in,const u8*bg,const u8*obj,const u16*bp,const u16*op,const u16*oam,u32*out,unsigned*why){
 u32 regs[0x400/4]={0};memcpy(regs,in,0x70);
 regs[0]&=~((7u<<24)|(7u<<27)|8u); /* B ignores A's base offsets and 3D bit. */
 regs[0x64/4]=0; /* B has no display capture. */
 /* Brightness/blend fades can differ by a few channel levels on the
  * existing 8-bit TEV path. Keep B on the original exact CPU fade for now. */
 const unsigned bl=regs[0x50/4]&0xFFFF,mb=(regs[0x6C/4]&0xFFFF);
 const unsigned eff=(bl>>6)&3;
 if(!P3DS_GcBFadesCfg()){
  if(eff||(((mb>>14)==1||(mb>>14)==2)&&(mb&31))){if(why)*why=0x8000u;return 0;}
 }else{
  /* Engine B has no 3D layer; plain OBJs cannot trigger implicit alpha.
   * An empty first-target mask makes the effect inert. Active alpha stays CPU. */
  if(!(bl&63))regs[0x50/4]&=~0xC0u;
  else if(eff==1){
   unsigned eva=(regs[0x50/4]>>16)&31,evb=(regs[0x50/4]>>24)&31;
   if(eva>16)eva=16;if(evb>16)evb=16;
   if(eva==16&&evb==0)regs[0x50/4]&=~0xC0u;
   else if(eva==0&&(evb==0||evb==16)){/* Exact black or destination, subject to normal layer/target validation. */}
   else{

   static unsigned seen[12],ns;unsigned sig=bl|(regs[0x50/4]&0xFFFF0000u),i=0;
   for(;i<ns&&seen[i]!=sig;i++);if(i==ns&&ns<12){seen[ns++]=sig;PSPNativeMemLog("[GCBFADE] alpha fallback BLDCNT=%04x BLDALPHA=%04x MBRIGHT=%04x",bl,(unsigned)(regs[0x50/4]>>16),mb);}
   if(why)*why=0x8000u;return 0;
   }
  }
 }
 gcEngineB=1;
 unsigned reject=P3DS_GcEligible((const u8*)regs,oam);if(why)*why=reject;
 if(reject){gcEngineB=0;return 0;}
 P3DS_GcEye(0,0);P3DS_GcDeferOut=0;
 int ok=P3DS_GcComposeA((const u8*)regs,bg,obj,bp,op,oam,NULL,out,NULL,NULL,NULL);
 P3DS_GcFinish();gcEngineB=0;
 if(ok&&P3DS_GcBFadesCfg()){static unsigned n,e[4],m; n++;e[eff]++;m+=((mb>>14)==1||(mb>>14)==2)&&(mb&31);
  if(n%120==0)PSPNativeMemLog("[GCBFADE] accepted=%u plain=%u alpha_endpoints=%u brighten=%u darken=%u master=%u",n,e[0],e[1],e[2],e[3],m);}
 if(!ok&&why)*why=0x4000u;return ok;
}

/* ---- Platinum compatibility (2026-11-21 SoulSilver platform sync) ----
 * This file is now the SoulSilver compositor (engine B on the GPU, exact fades, OBJ affine, deferred output / overlap,
 * the fps60 in-between target, FOE DEEPER row split). Platinum's run 111-118 compositor features map onto it as follows:
 *  - frame_pacing (run 118): replaced by the SoulSilver overlap path (the queued left eye converts during the right
 *    eye's draw); the old frame_pacing key is ignored and P3DS_GcFrame / P3DS_SetFramePacing only log.
 *  - gpu_mbright (run 111), gpu_blend_3d (run 110): replaced by fps_tweaks 3 / 4 (master brightness and 3D
 *    translucency on the GPU), which the tuned defaults turn on.
 *  - gc_atlas16 (run 118, default off): dropped (16-bit atlas never became the default).
 *  - wide_gpu (run 111, GPU wide battle layout): not yet carried onto this compositor. P3DS_GcComposeWide returns 0,
 *    so wide frames take widecomp's CPU path (the pre-run-111 path, same picture). Logged once as [GPUC] wide_gpu. */
int P3DS_GcNoLag;
void P3DS_GcFrame(void) { }
void P3DS_SetFramePacing(int v) { PSPNativeMemLog("[GPUC] frame_pacing=%d ignored: the SoulSilver overlap path does this now", v); }
int P3DS_GcBlendPredictOk(const u8 *regs) {
    if (!P3DS_GcBlendOn) return 1;
    const u32 bl = rh(regs, 0x50), dc = rg(regs, 0);
    if (((bl >> 6) & 3) != 1 || (bl & 0x2000)) return 1;
    u32 on = (dc >> 8) & 0x1F; if (dc & 8) on &= ~1u;
    return (bl & 0x1F & on) == 0;
}
int P3DS_GcComposeWide(const void *plan, const u8 *regs, const u8 *bgv, const u8 *objv, const u16 *bgpal, const u16 *objpal, const u16 *oam, const u32 *f3d, u32 *center, u32 *strip) {
    (void)plan; (void)regs; (void)bgv; (void)objv; (void)bgpal; (void)objpal; (void)oam; (void)f3d; (void)center; (void)strip;
    static int said; if (!said) { said = 1; PSPNativeMemLog("[GPUC] wide_gpu: GPU wide compose not on the new compositor yet, wide frames use the CPU widecomp path"); }
    return 0;
}
int P3DS_GcWideRelease(void) { return 0; }

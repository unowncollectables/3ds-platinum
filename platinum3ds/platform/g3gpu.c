/* platinum3ds: PICA200 (citro3d) rasterizer for g3soft. g3soft still does vertex transform/clipping and DS
 * texture decoding; triangles are queued here per eye, drawn on the GPU into a 256x256 VRAM target, then read
 * back and converted into the compositor's 3D layer format (6-bit RGB, 5-bit alpha). */
#include <3ds.h>
#include <citro3d.h>
#include "dcache3ds.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "g3_shbin.h"
#include "g3k_shbin.h"
#include "g3s_shbin.h" /* g3_stereo_gpu only — never bound unless that toggle is on */
extern int P3DS_GpuInit(void);
extern void PSPNativeMemLog(const char *, ...);
extern int P3DS_FpsTweaks(void);
extern void P3DS_ParFor(void (*fn)(void *, unsigned), void *ctx, unsigned n);
extern void P3DS_FrameEnd(void);  /* gpuscale.c: C3D_FrameEnd, lean flush with fps_tweaks >= 1 */
extern void G3T_VbLock(void); extern void G3T_VbUnlock(void); /* g3_raw_split: serialise vb appends */
extern int G3T_SplitOn(void);
typedef struct { float x, y, z, w, s, t, r, g, b, a; } V;
typedef struct { C3D_Tex *tex; int wrapS, wrapT, blend, dwrite, dfunc, late, fog; unsigned first, count; } B;
/* 2026-10-25: 3*8192 -> 3*16384 (+1 MiB linear per eye) for the removed GPU SPLIT mode; kept at 3*16384 until a device test shows 3*8192 is enough again. */
#define MAXV (3 * 16384)
#define MAXB 2048
static unsigned vn[2];
static V *vb[2]; static B bt[2][MAXB]; static unsigned bn[2];
/* stereoShareOn: game thread writes at Begin / SetStereoShare; worker reads in TriClip2 (g3_thread core 2).
   Must be atomic — plain store was invisible to the worker so share stayed dual forever (explore after 9bbfcd1). */
static volatile int stereoShareOn, stereoPending; /* g3_stereo_share: left-only verts until G3G_StereoExpand */
static float *stereoDsh; static float *stereoDshLin; /* linear copy for the GPU: the PICA reads vertex buffers by physical address, a malloc heap pointer has none */
static unsigned lastExpandUs; /* G3G_StereoExpand wall time (us); 0 until expand runs this frame */
/* fps60 (2026-11-15, Settings > Display > 60 fps field, default OFF): in-between 3D frames, like pokeplatinum-3ds's 60 fps field.
   The game still runs 30 ticks a second. For every raw draw call g3soft.cpp pairs this tick's clip matrix with the same draw's matrix of
   the tick before and hands G3G_MidK the matrix K that carries this tick's clip coordinates halfway back (K = average * inverse(now)).
   Nothing is transformed twice on the CPU: G3G_End draws the SAME vertex buffer a second time through shaders/g3k.v.pica, which puts
   K in front of the position, one uniform change per run of draw calls that share K (the whole static world shares one). That picture
   goes to midTex[] (and the wide strips to rbM[]), then the real frame is drawn as before, all in one GPU queue. render3ds.cpp composes
   the same 2D over midTex (gpucomp.c P3DS_GcMidCompose) and presents it one V-blank before the real picture. Only direct 1x frames
   (gpu_direct3d + direct screen) with a depth that is linear in clip space (not g3_wmode 1); anything else shows no in-between
   picture. Log: [FPS60]. */
#define KR_MAX 1024
int G3G_MidRec; extern int P3DS_Fps60Skip;
typedef struct { unsigned v0[2]; int id; float k[16]; } KR;   /* a run of vertices sharing K; k = K in vertex-buffer space, column-major */
static KR kr[KR_MAX]; static unsigned krN; static float kWant[16]; static int kWantId = 1, kCurWant = 1;
static DVLB_s *dvlbK; static shaderProgram_s progK; static int uK = -1;
enum { MID_STRIP_WIDTH = 64, MID_TEXTURE_HEIGHT = 256 };
static u32 *midStripTiles;
static C3D_Tex midTex[2]; static u32 *rbM[2]; static int midHave[2], midFail, midOver, midValid, midEyes, midWide;
static unsigned midTot, midMat, midMov; unsigned G3G_MidWhy[6];   /* why no in-between 3D: 0 not direct, 1 too many matrix runs, 2 CPU fog, 3 nothing moved, 4 too few draws paired, 5 no memory */
static int fpsHoldFlag;
int G3G_FpsHold(void) { return fpsHoldFlag; }
/* Hold only when the in-between would be the same picture as the real frame.
   tot/mat/mov come from the 3D matrix pairing. sceneMoved is 2D (sprites, scroll, tiles).
   vertsMoved is a sample of positions and texture coordinates (water, particles, a walking NPC). */
int P3DS_FpsHoldDecide(int toggle, unsigned tot, unsigned mat, unsigned mov, int sceneMoved, int vertsMoved) {
    if (!toggle || !tot || mov || mat != tot || sceneMoved || vertsMoved) return 0;
    return 1;
}
extern int P3DS_Fps60Active(void); extern int P3DS_Fps60Cfg(void);
static int midAlloc(int n) {
    if (midFail) return 0;
    for (int e = 0; e < n; e++) { if (midHave[e]) continue;
        if (!dvlbK) { dvlbK = DVLB_ParseFile((u32 *)g3k_shbin, g3k_shbin_size);
            if (dvlbK) { shaderProgramInit(&progK); shaderProgramSetVsh(&progK, &dvlbK->DVLE[0]); uK = shaderInstanceGetUniformLocation(progK.vertexShader, "kmtx"); } }
        static int tileAllocTried;
        if (!tileAllocTried) {
            tileAllocTried = 1;
            midStripTiles = linearAlloc(MID_STRIP_WIDTH * MID_TEXTURE_HEIGHT * sizeof(u32));
        }
        const unsigned readWidth = midStripTiles ? MID_STRIP_WIDTH : 320;
        rbM[e] = linearAlloc(readWidth * MID_TEXTURE_HEIGHT * sizeof(u32));
        if (uK < 0 || !rbM[e] || !C3D_TexInit(&midTex[e], 256, 256, GPU_RGBA8)) { midFail = 1; G3G_MidWhy[5]++;
            PSPNativeMemLog("[FPS60] no linear memory or shader for the in-between 3D frame (eye %d, linear free %lu): 60 fps field stays off", e, (unsigned long)linearSpaceFree()); return 0; }
        C3D_TexSetFilter(&midTex[e], GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&midTex[e], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); midHave[e] = 1;
        PSPNativeMemLog("[FPS60] in-between 3D buffers ready for eye %d (linear free %lu)", e, (unsigned long)linearSpaceFree()); }
    return 1;
}
extern int G3G_FlipY, G3G_ZInv;
static const float kIdent[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
static int wideFrame, hiFrame;
/* K (clip space, column-major, NULL = identity) -> A * K * inverse(A), A = what G3G_TriClip does to a clip coordinate on its way into vb[] */
static void krStart(const float *K) {
    float k[16]; int id = !K;
    if (id) memcpy(k, kIdent, sizeof k);
    else { const float ws = wideFrame ? 0.8f : 1.0f, sy = hiFrame ? (G3G_FlipY ? 1.0f : -1.0f) : (G3G_FlipY ? 0.75f : -0.75f), ty = hiFrame ? 0.0f : (G3G_FlipY ? 0.25f : -0.25f), sz = G3G_ZInv ? 0.5f : -0.5f;
        float t[16];   /* t = K * inverse(A): columns of K combined */
        for (int r = 0; r < 4; r++) { t[r] = K[r] / ws; t[4 + r] = K[4 + r] / sy; t[8 + r] = K[8 + r] / sz; t[12 + r] = K[12 + r] - K[4 + r] * ty / sy + K[8 + r] * 0.5f / sz; }
        for (int c = 0; c < 4; c++) { const float x = t[4 * c], y = t[4 * c + 1], z = t[4 * c + 2], w = t[4 * c + 3];   /* A * t: rows */
            k[4 * c] = ws * x; k[4 * c + 1] = sy * y + ty * w; k[4 * c + 2] = sz * z - 0.5f * w; k[4 * c + 3] = w; } }
    if (krN && kr[krN - 1].v0[0] == vn[0] && kr[krN - 1].v0[1] == vn[1]) krN--;   /* the run before got no vertex */
    if (krN) { const KR *p = &kr[krN - 1]; int same = p->id == id;
        if (same && !id) for (int i = 0; i < 16; i++) { const float d = p->k[i] - k[i]; if (d > 2e-4f || d < -2e-4f) { same = 0; break; } }
        if (same) return; }
    if (krN >= KR_MAX) { midOver = 1; return; }
    KR *q = &kr[krN++]; q->v0[0] = vn[0]; q->v0[1] = vn[1]; q->id = id; memcpy(q->k, k, sizeof k);
}
/* g3soft.cpp, before the triangles of one raw draw call: K = in-between clip matrix * inverse(this tick's), NULL = stays where it is */
void G3G_MidK(const float *K) {
    if (!G3G_MidRec) return;
    /* Hard-clipped screen triangles can temporarily restore identity. Only
       skip the change while the requested run is still active. */
    if (kCurWant && (K ? !kWantId && !memcmp(kWant, K, sizeof kWant) : kWantId) && !G3T_SplitOn()) {
        static unsigned skipped;
        if (++skipped % 8192 == 0)
            PSPNativeMemLog("[FPS60K] identical active midpoint matrices skipped: %u", skipped);
        return;
    }
    kWantId = !K;
    if (K) memcpy(kWant, K, sizeof kWant);
    krStart(K);
    kCurWant = 1;
}
void G3G_MidInfo(unsigned tot, unsigned mat, unsigned mov) { midTot = tot; midMat = mat; midMov = mov; }
int G3G_MidReady(void) { return midValid; }
C3D_Tex *G3G_MidTex(int e) { return midValid && e >= 0 && e < midEyes ? &midTex[e] : NULL; }
static int ok = -1; static DVLB_s *dvlb; static shaderProgram_s prog;
static C3D_Tex rt[2]; static C3D_RenderTarget *tgt[2]; static u32 *rb[2];
static C3D_Tex *pendFree[512]; static unsigned pendN; static C3D_Tex *oldFree[512]; static unsigned oldN;
/* gpu3d_async=1: the GPU draws frame N while the CPU composites; frame N's readback happens at End(N+1)
   (3D layer shows one frame late). Frees are held one extra frame. */
int G3G_Async = 0; static int pendRB, pendEyes;
/* 2026-10-26 gpu_direct3d (Debug tab, default OFF): the 3D layer stays on the GPU. The old path per 3D frame is draw -> wait ->
   readback DMA -> CPU 6-bit convert (NativeFrame) -> CPU re-swizzle into the compositor's texture -> upload. With direct on, G3G_End
   queues the draw and returns without waiting; gpucomp.c samples the 256x256 eye target rt[e] itself as the 3D-layer texture (same
   tiled RGBA8 layout, same row order as its own conv3dRaw output) and NativeFrame is left stale. Any consumer that needs the pixels on
   the CPU (CPU composite, stereo fallback, dumps) calls G3G_DirectResolve(), which does the readback + convert then.
   Colour is 8 bit here instead of the DS's 6 bit expanded to 8. Only plain 256x256 1x frames, rb_sync, gpu_compose=2, no CPU fog.
   dirValid: rt[e] holds the frame that dirKey[e] (the NativeFrame / right-eye pointer) stands for. dirPend: NativeFrame is stale.
   dirOut: GPU work may still be in flight (cleared when gpucomp's own C3D_FrameBegin has waited, or by a sync). */
/* 2026-10-26 wide direct (same toggle; config-only gpu_direct3d_wide=0 turns just this part off): with Wide view on, the eye
   targets are 320x256 plain framebuffers (not textures, 320 is not a power of two), so the compositor cannot sample them.
   G3G_End now also queues, after the draws and in the same GPU queue: (1) a GX texture copy of the centre columns 32..287
   (tile columns 4..35 of every 8-row tile row: 8192 bytes taken, 2048 skipped) into dirTex[e], a 256x256 RGBA8 texture
   in linear RAM with exactly the layout of the 256 target, which gpucomp samples as before; (2) the usual readback DMA into
   rb[e]. The CPU then converts only the two 32-px side strips (G3G_DirectStrips, called before present; the GPU work is
   already done by then because gpucomp's FrameBegin waited) instead of the whole 256x192 centre. dirTex is allocated on
   the first wide direct frame and freed with the wide targets (freeTargets). dirWide: dirTex holds this frame's centre;
   dirStrip: G3G_Strip is not converted yet; dirRb: rb[] readback already queued for this frame. */
static int dirCfg = -1, dirValid, dirPend, dirEyes, dirOut, ruRt;  /* ruRt: rt[] holds the frame saved in ruKey/ruV (direct reuse) */ static u32 *dirKey[2];
static int dirWideCfg = -1, dirWide, dirStrip, dirRb, dirTexOk, dirTexFail; static C3D_Tex dirTex[2];
static unsigned dirDirect, dirReuse, dirResolved, dirUsed, dirPlain, dirWhy[7], dirWideN, dirStripN, dirResWhy[8]; static unsigned long long dirUs, dirResUs, dirPlainUs, dirStripUs;
extern int P3DS_GpuComposeCfg(void);
extern int P3DS_Gc3DBlendRejects(const u8 *regs);
extern int P3DS_InField;
int G3G_DirectCfg(void) { if (dirCfg < 0) { extern int P3DS_CfgInt(const char *, int); dirCfg = P3DS_CfgInt("gpu_direct3d", 0) != 0; PSPNativeMemLog("[GPU3D] gpu_direct3d=%d (1 = 3D layer stays on the GPU: no readback/convert/upload when the GPU compositor takes the frame)", dirCfg); } return dirCfg; }
void G3G_SetDirect(int v) { dirCfg = v != 0; PSPNativeMemLog("[GPU3D] gpu_direct3d=%d from settings", dirCfg); }
static void dirSync(void) { if (dirOut) { dirOut = 0; C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH); } }
void G3G_DirectSynced(void) { dirOut = 0; }
int G3G_DirectPending(void) { return dirPend; }
void G3G_DirectInvalidate(void) { dirValid = dirPend = 0; }
C3D_Tex *G3G_DirectTex(const u32 *f) {
    if (!dirValid || !f) return NULL;
    for (int e = 0; e < dirEyes; e++) if (dirKey[e] == f) { dirUsed++; return dirWide ? &dirTex[e] : &rt[e]; }
    return NULL;
}
static u32 clearRGBA; static float clearD; static int eyes;
/* ss3ds g3_wdepth (Settings > 3D > W-buffer depth, default ON): g3soft gives every raw-mode vertex a depth linear in w
   (DS W-buffer; the title/Lugia projection has z == w). The PICA interpolates z/w affinely in screen space, which is wrong
   for a value linear in w: big body triangles got bad depth and Lugia's spikes/wings poked through or floated. With the
   PICA's own W-buffer mode (C3D_DepthMap(false)) the per-vertex depth -d is interpolated perspective-correctly. 0 = old. */
/* g3_wmode in config.txt (no menu row any more): 0 off, 1 PICA W = default, 2 GPU Z. G3G_WDepth keeps the internal codes:
   0 off, 2 PICA W, 3 GPU Z (the removed 1 = CPU 3D for W frames and 4 = GPU split did not fix the title Lugia; the real
   fix was the clip-matrix patch 0095). Retired keys g3_wcpu and g3_wpersp are ignored. */
int G3G_WDepth = -1, G3G_WMode = -1;
static const int kWModeToCode[3] = {0, 2, 3};
int G3G_WDepthOn(void) { if (G3G_WDepth < 0) { extern int P3DS_CfgInt(const char *, int); G3G_WMode = P3DS_CfgInt("g3_wmode", 1); if (G3G_WMode < 0 || G3G_WMode > 2) G3G_WMode = 1; G3G_WDepth = kWModeToCode[G3G_WMode]; PSPNativeMemLog("[GPU3D] g3_wmode=%d (0 off, 1 PICA W, 2 GPU Z) -> depth code %d", G3G_WMode, G3G_WDepth); } return G3G_WDepth; }
int G3G_TexFlip = 1, G3G_FlipY = 0, G3G_ZInv = 0;  /* run 18: original mapping is correct (run 17 breakage was the texture V flip) */
#define RB_FLAGS (GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
/* run 37: rb_sync=1 (default). The old readback ran C3D_SyncDisplayTransfer OUTSIDE a frame, which waits with
   gspWaitForPPF(). That GSP event is sticky: the GPU-scale present (gpuscale=1) queues 3-4 display transfers per frame
   whose PPF nobody consumes, so the wait returned at once and the CPU converted rb[] WHILE the DMA was still writing it
   (and with stale cache lines: rb[] was never invalidated). Rows past the DMA's progress came from the previous frame:
   a fixed split row, both photos, only visible while the 3D image moves, only with gpuscale=1 (mode 0 has no stray PPF).
   Now the transfer is queued INSIDE the frame after the draws, completion is the citro3d queue itself (gxCmdQueueWait
   via C3D_FrameBegin), and the CPU cache is invalidated before reading. rb_sync=0 = old path. */
static int rbSync = -1;
static int rbSyncOn(void) {
    if (rbSync < 0) { rbSync = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "rb_sync=%d", &x) == 1) rbSync = x != 0; fclose(f); } PSPNativeMemLog("[GPU3D] rb_sync=%d (3D readback: queued in-frame + cache invalidate)", rbSync); }
    return rbSync;
}
/* run 44 wideview v2 (experimental, default OFF): the 3D layer is rendered ONCE into a 320x256 framebuffer (viewport 320); every vertex gets x' = 0.8x (clip space) / (sx+32)/160-1 (screen space): same vertical scale, H-FOV x1.25
   (Hor+). Columns 32..287 are pixel-identical to the normal 256 frame and go to the compositor as before; columns
   0..31 and 288..319 become G3G_Strip[eye] (RGBA8 0xRRGGBBAA, 64x192: left 32 | right 32) for gpuscale. The game's
   matrices/state are untouched (only the port's final viewport mapping changes). The 256x256 targets are swapped for
   320x256 ones while wide is on (VRAM is tight: gpuscale holds 3 MiB), and swapped back when it is off. */
int G3G_Wide = 0, G3G_StripValid = 0;
u32 G3G_Strip[2][64 * 192];
static int curW = 256, wideFrame, wideFail, hiFrame;
static unsigned rtStride(void) { return (unsigned)curW; }
static int curHi;
#define RB_FLAGS_2X (GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_XY))
/* 2026-10-28 g3_sharp=2 ("FAST 3D", stereo only): the LEFT eye is drawn at 2x and box-downscaled as before, the RIGHT eye is drawn at 1x
   (viewport curW x 192) in one corner of the same 2x target, so the second eye costs ~1/4 of the GPU fill. Its 192 rows are copied out
   unscaled (input 2*curW x 192 = the first 24 tile rows of the target, contiguous) into rb[1], which then has stride 2*curW. Where the
   viewport lands (first or second 192 rows of the target) is found at run time on the first frame that draws something: both halves
   are copied, the half that differs from the clear colour wins (r1xHalf), and rb[1] / rbT are swapped if it was the second. */
static int r1xFrame, r1xHalf = -1, r1xFail; static u32 *rbT;
static unsigned rbStride(int e) { return e == 1 && r1xFrame ? 2u * curW : (unsigned)curW; }
static unsigned rbRows(int e) { return e == 1 && r1xFrame ? 192u : 256u; }
static void rbQueue(int e) {
    if (e == 1 && r1xFrame) {
        const unsigned bytes = 2u * curW * 192 * 4; u8 *cb = (u8 *)tgt[1]->frameBuf.colorBuf;
        C3D_SyncDisplayTransfer((u32 *)(cb + (r1xHalf > 0 ? bytes : 0)), GX_BUFFER_DIM(2 * curW, 192), rb[1], GX_BUFFER_DIM(2 * curW, 192), RB_FLAGS);
        if (r1xHalf < 0 && rbT) C3D_SyncDisplayTransfer((u32 *)(cb + bytes), GX_BUFFER_DIM(2 * curW, 192), rbT, GX_BUFFER_DIM(2 * curW, 192), RB_FLAGS);
    }
    else if (curHi == 1) C3D_SyncDisplayTransfer((u32 *)tgt[e]->frameBuf.colorBuf, GX_BUFFER_DIM(2 * curW, 384), rb[e], GX_BUFFER_DIM(2 * curW, 384), RB_FLAGS_2X);   /* SCALE_XY: the output dims field is the PRE-scale size (citro3d passes the same dim for both; run 52 stripes/split picture came from giving the half size) */
    else C3D_SyncDisplayTransfer((u32 *)tgt[e]->frameBuf.colorBuf, GX_BUFFER_DIM(curW, 256), rb[e], GX_BUFFER_DIM(curW, 256), RB_FLAGS); } /* inFrame: queued only */
extern unsigned char PSPNative_GfxRegisters[];
int P3DS_MapOutdoor = 1; int P3DS_CurMap = -1; /* run 76: current map header (field), for Distortion World effects */ /* run 73: set by the follower frame (MapHeader_IsOutdoors) */
static void stripConv(const u32 *s, unsigned rst, unsigned right, u32 *d);
/* 2026-11-17 slim strips (always on since run 920's A/B: real copies -0.35 ms GPU, strip convert -0.3 ms CPU): a real wide frame copies only its two 32-px side strips (the 192
   visible rows, same GX copies as the in-between strips) into rbS[e], instead of reading the whole 320x256 eye back into rb[e].
   slimFrame: this frame's strips are in rbS[]. Anything that needs the full eye (G3G_DirectResolve) still queues the full readback. */
static u32 *rbS[2], *rbSTiles; static int slimFrame, slimFail;
static void stripConvert(int e) {
    if (slimFrame && rbS[e]) { P3DS_DcInval(rbS[e], 64 * 192 * sizeof(u32)); stripConv(rbS[e], 64, 32, G3G_Strip[e]); return; }
    stripConv(rb[e], rbStride(e), 288, G3G_Strip[e]); }
static void stripConv(const u32 *s, const unsigned rst, unsigned right, u32 *d) {
    u16 mb = *(u16 *)(PSPNative_GfxRegisters + 0x6C); unsigned mm = (mb >> 14) & 3, f = mb & 31; if (f > 16) f = 16;
    { extern int P3DS_WideFadeFix(void); extern int P3DS_WideFxCfg(void); if (P3DS_WideFadeFix() || P3DS_WideFxCfg()) f = 0; }  /* run 54: wide_fx fits the strips to the centre */  /* run 52: fade applied when drawing (gpuscale) */
    /* The old edge search required a second sample 32 periods (1024 pixels)
       away in a 320-pixel row. It could never replace a pixel. */
    const int fade = f && (mm == 1 || mm == 2);
    /* 2026-11-17 run 920: ~0.35 ms per eye here, almost all of it waiting on FCRAM cache-line fills of the freshly
       invalidated readback (the loop itself is a few cycles a pixel). PLD the same strip 4 rows ahead so the line
       fills overlap the work. Hints only: same pixels, same picture. */
#define STRIP_PLD(p) __builtin_prefetch((const void *)(p), 0, 0)
    for (unsigned y = 0; y < 4 && y < 192; y++) { const u32 *r = s + y * rst; for (int k = 0; k < 32; k += 8) { STRIP_PLD(r + k); STRIP_PLD(r + right + k); } }
    for (unsigned y = 0; y < 192; y++) {
        const u32 *row = s + y * rst;
        if (y + 4 < 192) { const u32 *r = row + 4 * rst; for (int k = 0; k < 32; k += 8) { STRIP_PLD(r + k); STRIP_PLD(r + right + k); } }
        for (unsigned side = 0; side < 2; side++) {
            const unsigned start = side ? right : 0;
            u32 *out = d + y * 64 + side * 32;
            if (!fade) {
                for (int x = 0; x < 32; x++) {
                    const u32 pixel = row[start + x];
                    out[x] = (pixel & 255) >= 8 ? pixel | 255 : 255;
                }
                continue;
            }
            for (int x = 0; x < 32; x++) {
                const u32 src = row[start + x];
                const u32 pixel = (src & 255) >= 8 ? src : 0;
                unsigned r = pixel >> 24, g = (pixel >> 16) & 255, b = (pixel >> 8) & 255;
                if (mm == 1) {
                    r += (255 - r) * f / 16;
                    g += (255 - g) * f / 16;
                    b += (255 - b) * f / 16;
                } else {
                    r -= r * f / 16;
                    g -= g * f / 16;
                    b -= b * f / 16;
                }
                out[x] = (r << 24) | (g << 16) | (b << 8) | 255;
            }
        }
    }
#undef STRIP_PLD
}
/* run 55 fog3d=1 (default): DS 3D fog (DISP3DCNT bit 7) on the GPU path. Each opaque polygon writes its POLYGON_ATTR fog
   bit into the stencil buffer (clear plane: CLEAR_COLOR bit 15); after the draw the D24S8 buffer is copied out with
   the colour readback (same queue) and fog is applied to the readback exactly like the DS / melonDS: 24-bit depth,
   FOG_OFFSET<<9, (dz>>2)<<FOG_SHIFT -> 32-entry density table (0x360) with linear interpolation, colour and alpha
   (or alpha only, DISP3DCNT bit 6) blended toward FOG_COLOR. Covers the wide-view strips too (whole 320 target).
   Skipped in W-buffer frames (depth units differ). 0 = no fog (old). */
int G3G_CurFog, G3G_FogNoW = 1; unsigned G3G_ConvUs; static u32 *drb[2]; static int fogFrame, fogCfg = -1; static unsigned fogFrames;
static int fogCfgOn(void) { if (fogCfg < 0) { fogCfg = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "fog3d=%d", &x) == 1) fogCfg = x < 0 ? 0 : x > 2 ? 2 : x; fclose(f); } PSPNativeMemLog("[GPU3D] fog3d=%d", fogCfg); } return fogCfg; }
static void applyFog(int e) {
    const unsigned char *R = PSPNative_GfxRegisters; const u16 dc = *(const u16 *)(R + 0x60); const u32 fc = *(const u32 *)(R + 0x358);
    const unsigned shift = (dc >> 8) & 15, alphaOnly = (dc >> 6) & 1; const u32 off = (u32)(*(const u16 *)(R + 0x35C) & 0x7FFF) << 9;
    unsigned tab[34]; for (int i = 0; i < 32; i++) { unsigned d = R[0x360 + i] & 127; tab[i + 1] = d == 127 ? 128 : d; } tab[0] = tab[1]; tab[33] = tab[32];
    const int fr = (int)((fc & 31) * 255 / 31), fg = (int)(((fc >> 5) & 31) * 255 / 31), fb = (int)(((fc >> 10) & 31) * 255 / 31), fa = (int)(((fc >> 16) & 31) * 255 / 31);
    const unsigned st = rtStride(); u32 *c = rb[e]; const u32 *d = drb[e];
    P3DS_DcInval(drb[e], st * 256 * 4);
    for (unsigned i = 0; i < st * 192; i++) { const u32 dv = d[i]; if (!(dv >> 24)) continue;
        u32 z = dv & 0xFFFFFF; unsigned id, fr17;
        if (z < off) { id = 0; fr17 = 0; } else { z -= off; z = (z >> 2) << shift; id = z >> 17; if (id >= 32) { id = 32; fr17 = 0; } else fr17 = z & 0x1FFFF; }
        const int f = (int)((tab[id] * (0x20000 - fr17) + tab[id + 1] * fr17) >> 17); if (!f) continue;
        const u32 p = c[i]; int r = p >> 24, g = (p >> 16) & 255, b = (p >> 8) & 255, a = p & 255;
        if (!alphaOnly) { r = (fr * f + r * (128 - f)) >> 7; g = (fg * f + g * (128 - f)) >> 7; b = (fb * f + b * (128 - f)) >> 7; }
        a = (fa * f + a * (128 - f)) >> 7;
        c[i] = ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (u32)a; }
}
/* run 56: fog3d=1 = PICA fog unit (free on the GPU): DS fog density (same melonDS math as applyFog) sampled into the
   128-entry fog LUT indexed by the stored depth (= DS 24-bit z); per batch on/off from the polygon fog bit; clear plane
   fogged on the CPU (one colour). fog3d=2 = run-55 exact CPU pass (stencil + depth readback, ~6-10 ms). */
static unsigned fogDens(u32 z) { const unsigned char *R = PSPNative_GfxRegisters; const u16 dc = *(const u16 *)(R + 0x60);
    const unsigned shift = (dc >> 8) & 15; const u32 off = (u32)(*(const u16 *)(R + 0x35C) & 0x7FFF) << 9;
    unsigned tab[34]; for (int i = 0; i < 32; i++) { unsigned d = R[0x360 + i] & 127; tab[i + 1] = d == 127 ? 128 : d; } tab[0] = tab[1]; tab[33] = tab[32];
    unsigned id, fr17; if (z < off) { id = 0; fr17 = 0; } else { z -= off; z = (z >> 2) << shift; id = z >> 17; if (id >= 32) { id = 32; fr17 = 0; } else fr17 = z & 0x1FFFF; }
    return (tab[id] * (0x20000 - fr17) + tab[id + 1] * fr17) >> 17; }
static C3D_FogLut fogLut; static u32 fogKey[10]; static int fogLutOk;
static void fogLutSetup(void) { const unsigned char *R = PSPNative_GfxRegisters; u32 k[10];
    k[0] = *(const u16 *)(R + 0x60) & 0x0F40; k[1] = *(const u16 *)(R + 0x35C); memcpy(&k[2], R + 0x360, 32);
    if (fogLutOk && !memcmp(k, fogKey, sizeof k)) { C3D_FogLutBind(&fogLut); return; }
    float d[256]; for (int i = 0; i <= 128; i++) { u32 z = i >= 128 ? 0xFFFFFF : (u32)i << 17; float v = 1.0f - fogDens(z) / 128.0f; if (i < 128) d[i] = v; if (i) d[128 + i - 1] = v - d[i - 1]; }
    { static unsigned nl; if (nl++ < 4) { char b[400]; int o = 0; for (int i = 96; i < 128; i += 2) o += snprintf(b + o, sizeof b - o, " %d", (int)(128 - d[i] * 128)); o += snprintf(b + o, sizeof b - o, " | tab"); for (int i = 0; i < 32; i += 4) o += snprintf(b + o, sizeof b - o, " %d", R[0x360 + i] & 127);
        PSPNativeMemLog("[GPU3D] fog3d LUT (density/128 at depth 0.75..1 step 1/64):%s", b); } }
    FogLut_FromArray(&fogLut, d); memcpy(fogKey, k, sizeof k); fogLutOk = 1; C3D_FogLutBind(&fogLut); }
/* run 57 fast_conv=1: the RGBA8 readback word is R<<24|G<<16|B<<8|A; one byte-reverse gives R|G<<8|B<<16|A<<24, then
   >>2 per byte (6-bit RGB) and alpha >>3 (5-bit): bit-identical to the old per-channel shifts, ~half the ALU work. */
static int fastCfg = -1;
static int fastConv(void) { if (fastCfg < 0) { fastCfg = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "fast_conv=%d", &x) == 1) fastCfg = x != 0; fclose(f); } PSPNativeMemLog("[GPU3D] fast_conv=%d", fastCfg); } return fastCfg; }
void G3G_SetFastConv(int v) { fastCfg = v != 0; } int G3G_FastConv(void) { return fastConv(); }
/* fps_tweaks 7: the 192 rows are independent, so rbConvert hands 24-row chunks to P3DS_ParFor (par.c; serial below level 7, same output) */
typedef struct { const u32 *s; u32 *out; unsigned st, ox; int fast; } RbJob;
extern void P3DS_ParFor(void (*fn)(void *, unsigned), void *ctx, unsigned n);
static void rbConvRows(void *ctx, unsigned k) {
    const RbJob *j = (const RbJob *)ctx;
    for (unsigned y = k * 24; y < k * 24 + 24; y++) { const u32 *row = j->s + y * j->st + j->ox; u32 *o = j->out + y * 256;
        if (j->fast) for (unsigned x = 0; x < 256; x += 8) { __builtin_prefetch(row + x + 24); __builtin_prefetch(o + x + 24, 1);
            for (unsigned i = 0; i < 8; i++) { const u32 t = (__builtin_bswap32(row[x + i]) >> 2) & 0x3F3F3F3Fu; o[x + i] = (t & 0x003F3F3Fu) | ((t >> 1) & 0x1F000000u); } }
        else for (unsigned x = 0; x < 256; x++) { u32 c = row[x]; unsigned a = (c & 255) >> 3;
            o[x] = ((c >> 26) & 63) | (((c >> 18) & 63) << 8) | (((c >> 10) & 63) << 16) | (a << 24); } }
}
static void rbConvert(int e, u32 *out) {
    if (fogFrame == 2 && drb[e]) applyFog(e);
    if (!out) return; const unsigned cw = rtStride(), st = rbStride(e), ox = cw == 320 ? 32 : 0;
    P3DS_DcInval(rb[e], st * rbRows(e) * 4);
    RbJob job = { rb[e], out, st, ox, fastConv() };
    P3DS_ParFor(rbConvRows, &job, 8);
    if (cw == 320) stripConvert(e);
}
/* run 46 res3d=2 (config, default 0): 3D layer rendered at 2x (512x384, or 640x384 with wideview) into plain
   framebuffers; the readback display transfer does a 2x2 box downscale in hardware (GX_TRANSFER_SCALE_XY), so the
   compositor still gets 256x192 (320x192) and the CPU cost is zero. Result: supersampled (4x SSAA) 3D: smooth edges,
   no texture shimmer, cleaner thin lines. VRAM: 1.5 MiB per eye (1.9 MiB wide) with 24-bit depth; if that does not
   fit (stereo needs both eyes), 16-bit depth is tried, then 1x (logged, res3dFail). */
static int res3dCfg = -1, res3dFail;
/* run 121 g3_sharp (from SoulSilver FAST 3D): 0=off (default), 1=SHARP (res3d=2), 2=FAST 3D (right eye 1x when stereo), 3=FAST AUTO (drop left to 1x when frames >34.8 ms). Risky; default OFF. */
static int sharpCfg = -1;
static unsigned gvAcc, gvN, gvDown; static int gvSh = -1;
int G3G_Sharp(void) { if (sharpCfg < 0) { extern int P3DS_CfgInt(const char *, int); sharpCfg = P3DS_CfgInt("g3_sharp", 0); if (sharpCfg < 0 || sharpCfg > 3) sharpCfg = 0; PSPNativeMemLog("[3DRES] g3_sharp=%d (0 off, 1 SHARP, 2 FAST 3D, 3 FAST AUTO)", sharpCfg); } return sharpCfg; }
void G3G_SetSharp(int v) { sharpCfg = v < 0 ? 0 : v > 3 ? 3 : v; if (sharpCfg) { res3dCfg = 2; res3dFail = 0; } else { res3dCfg = 0; } PSPNativeMemLog("[3DRES] g3_sharp=%d from settings", sharpCfg); }
static int sharedTgt;
/* run 49: res3d depth. Run 48 ended up with a 16-bit depth buffer (log: "depth 16"): the far (top) part of the field
   z-fought into stripes and long diagonal seams showed through. The 24-bit depth did not fit (after the 1x targets are
   freed, VRAM has one 1.31 MiB hole + one 0.74 MiB hole; 640x384 needs 2 x 0.94 MiB). Now: color in VRAM, 24-bit depth
   in linear FCRAM (GPU-addressable; cleared by a full-screen quad because the memory-fill engine is VRAM-only).
   Order: VRAM depth24 -> linear depth24 (res3d_lindepth=1, default) -> VRAM depth16 (correct 16-bit clear) -> 1x. */
static void *linDepth; static int depthLin, depth16, linDepthCfg = -1; static V *qv;
static void freeTargets(void) { dirSync(); dirValid = dirPend = ruRt = dirWide = dirStrip = dirRb = slimFrame = 0; if (dirTexOk) { for (int e = 0; e < 2; e++) C3D_TexDelete(&dirTex[e]); memset(dirTex, 0, sizeof dirTex); dirTexOk = 0; } if (tgt[1] == tgt[0]) tgt[1] = NULL; sharedTgt = 0; if (linDepth) linearFree(linDepth); linDepth = NULL; depthLin = depth16 = 0; if (rbT) linearFree(rbT); rbT = NULL; r1xHalf = -1; for (int e = 0; e < 2; e++) { if (tgt[e]) C3D_RenderTargetDelete(tgt[e]); tgt[e] = NULL; if (rt[e].data) C3D_TexDelete(&rt[e]); memset(&rt[e], 0, sizeof rt[e]); if (rb[e]) linearFree(rb[e]); rb[e] = NULL; } }
/* (re)create the eye targets: w = 256 | 320 compositor width, hi = 2x. GPU idle: called at frame begin, sync path. */
static int setTargets(int w, int hi) {
    if (w == curW && hi == curHi) return 1;
    C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH);   /* nothing may still reference the old targets */
    freeTargets();
    unsigned long vf = (unsigned long)vramSpaceFree(); int okW = 0; const char *depth = "24";
    if (!hi && w == 256) {   /* the original 256x256 texture targets */
        okW = 1; for (int e = 0; e < 2 && okW; e++) { rb[e] = linearAlloc(256 * 256 * 4); if (!rb[e] || !C3D_TexInitVRAM(&rt[e], 256, 256, GPU_RGBA8) || !(tgt[e] = C3D_RenderTargetCreateFromTex(&rt[e], GPU_TEXFACE_2D, 0, GPU_RB_DEPTH24_STENCIL8))) okW = 0; else { C3D_TexSetFilter(&rt[e], GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&rt[e], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); } }
    } else {
        const int tw = hi ? 2 * w : w, th = hi ? 384 : 256;
        if (linDepthCfg < 0) { linDepthCfg = 0; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) if (sscanf(l, "res3d_lindepth=%d", &x) == 1) linDepthCfg = x != 0; fclose(f); } PSPNativeMemLog("[3DRES] res3d_lindepth=%d", linDepthCfg); }
        if (!qv) qv = linearAlloc(6 * sizeof(V));
        /* 2026-10-26: pass 3 = colour AND 24-bit depth in linear RAM (run 7 hardware log: 512x384 FAILED with 1.8 MB VRAM
           free in pieces, so not even the colour buffer found one 768 KB hole). Tried before the 16-bit depth pass. */
        /* run 49 hardware log (2026-10-28): with colour+depth in linear RAM (pass 3, PR #169) the 3D GPU draw+readback took
           50 ms instead of 0.5 ms (14 fps) and the picture was garbage (top strips, black bands), on the title Lugia too. The
           PICA does not render usably into FCRAM (citro3d refuses it for a reason). Pass 3 is off; linear depth (pass 1) is
           config-only now (res3d_lindepth=1, default 0) for the same reason. 2x targets are VRAM-only: they need VRAM freed. */
        static const int passOrder[3] = { 0, 1, 2 };
        for (int pi = 0; pi < (hi ? 3 : 1) && !okW; pi++) { const int pass = passOrder[pi];
            if ((pass == 1 || pass == 3) && (!linDepthCfg || !qv)) continue;
            const int lin = pass == 1 || pass == 3, linCol = pass == 3;
            const GPU_DEPTHBUF df = pass == 2 ? GPU_RB_DEPTH16 : GPU_RB_DEPTH24_STENCIL8; depth = pass == 2 ? "16" : linCol ? "24, colour+depth in linear RAM" : lin ? "24 in linear RAM" : "24";
            if (pass) freeTargets();
            /* run 48: res3d shares ONE 2x target between the eyes (draw L, HW-downscale copy L, clear, draw R, copy R:
               all in GPU queue order). Run 47 allocated two and never fitted: 2 x 1.2-2.0 MiB vs 2.07 MiB free. */
            okW = 1; for (int e = 0; e < 2 && okW; e++) { rb[e] = linearAlloc(hi && e == 1 ? (size_t)2 * w * 192 * 4 : (size_t)w * 256 * 4); if (!rb[e]) { okW = 0; break; }
                if (hi && e == 1) { tgt[1] = tgt[0]; sharedTgt = 1; continue; }
                if (lin) {
                    if (linCol) { /* colour buffer = a linear 2^n texture's storage; frame buffer size overridden to tw x th.
                           2026-10-28 (Azahar run of run 48): citro3d's C3D_RenderTargetCreateFromTex returns NULL for a texture that is
                           not in VRAM, so this pass never worked (title "512x384 targets FAILED" on hardware run 7 too). The target is
                           now made from a tiny VRAM texture and then pointed at the linear colour buffer (the PICA can render to FCRAM). */
                        static C3D_Tex dumTex; static int dumOk = -1;
                        if (dumOk < 0) dumOk = C3D_TexInitVRAM(&dumTex, 8, 8, GPU_RGBA8) ? 1 : 0;
                        if (dumOk && C3D_TexInit(&rt[e], tw <= 512 ? 512 : 1024, 512, GPU_RGBA8) && (tgt[e] = C3D_RenderTargetCreateFromTex(&dumTex, GPU_TEXFACE_2D, 0, -1))) {
                            C3D_FrameBufColor(&tgt[e]->frameBuf, rt[e].data, GPU_RB_RGBA8); C3D_FrameBufAttrib(&tgt[e]->frameBuf, tw, th, false); }
                        else PSPNativeMemLog("[3DRES] linear colour target: dummy VRAM tex %d, linear tex %s", dumOk, rt[e].data ? "ok" : "FAILED");
                    } else
                    tgt[e] = C3D_RenderTargetCreate(tw, th, GPU_RB_RGBA8, -1);
                    if (tgt[e]) { linDepth = linearMemAlign(tw * th * 4, 0x80);
                        if (!linDepth) okW = 0; else { memset(linDepth, 0xFF, tw * th * 4); P3DS_DcFlush(linDepth, tw * th * 4);
                            C3D_FrameBufDepth(&tgt[e]->frameBuf, linDepth, GPU_RB_DEPTH24_STENCIL8); depthLin = 1; } }
                    else okW = 0;
                } else tgt[e] = C3D_RenderTargetCreate(tw, th, GPU_RB_RGBA8, df);
                if (!tgt[e]) okW = 0; }
            if (okW && pass == 2) depth16 = 1;
        }
    }
    if (!okW) {
        PSPNativeMemLog("[3DRES] %dx%s targets FAILED (VRAM free before %lu, linear free %lu)", hi ? 2 * w : w, hi ? "384" : "256", vf, (unsigned long)linearSpaceFree());
        freeTargets(); curW = 0; curHi = -1;
        { extern int P3DS_Fps60VramRelease(void); if (P3DS_Fps60VramRelease()) return setTargets(w, hi); }
        if (hi) { extern void P3DS_Toast(const char *); res3dFail = 1; P3DS_Toast("SHARP 3D: NO VRAM"); return setTargets(w, 0) ? 0 : 0; }
        if (w != 256) { wideFail = 1; return setTargets(256, 0) ? 0 : 0; }
        PSPNativeMemLog("[3DRES] restoring 256 targets FAILED: 3D off"); ok = 0; return 0;
    }
    curW = w; curHi = hi;
    PSPNativeMemLog("[3DRES] eye targets now %dx%d (%s, depth %s%s) VRAM free before %lu, after %lu", hi ? 2 * w : w, hi ? 384 : 256, hi ? "res3d=2, HW 2x2 downscale" : "1x", depth, sharedTgt ? ", one target shared by both eyes" : "", vf, (unsigned long)vramSpaceFree());
    return 1;
}
static void readback(int e, u32 *out) {
    C3D_SyncDisplayTransfer(rt[e].data, GX_BUFFER_DIM(256, 256), rb[e], GX_BUFFER_DIM(256, 256), RB_FLAGS);
    if (!out) return; const u32 *s = rb[e];
    for (unsigned i = 0; i < 256 * 192; i++) { u32 c = s[i]; unsigned a = (c & 255) >> 3;
        out[i] = ((c >> 26) & 63) | (((c >> 18) & 63) << 8) | (((c >> 10) & 63) << 16) | (a << 24); }
}
int G3G_Init(void) {
    if (ok >= 0) return ok;
    ok = 0; if (!P3DS_GpuInit()) return 0;
    dvlb = DVLB_ParseFile((u32 *)g3_shbin, g3_shbin_size); shaderProgramInit(&prog); shaderProgramSetVsh(&prog, &dvlb->DVLE[0]);
    for (int e = 0; e < 2; e++) {
        vb[e] = linearAlloc(MAXV * sizeof(V)); rb[e] = linearAlloc(256 * 256 * 4);
        if (!vb[e] || !rb[e] || !C3D_TexInitVRAM(&rt[e], 256, 256, GPU_RGBA8)) { PSPNativeMemLog("[GPU3D] init failed (alloc)"); return 0; }
        tgt[e] = C3D_RenderTargetCreateFromTex(&rt[e], GPU_TEXFACE_2D, 0, GPU_RB_DEPTH24_STENCIL8);
        if (!tgt[e]) { PSPNativeMemLog("[GPU3D] init failed (target)"); return 0; }
        C3D_TexSetFilter(&rt[e], GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&rt[e], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    }
    ok = 1; PSPNativeMemLog("[GPU3D] PICA200 3D path ready"); return 1;
}
/* RGBA8 (u32 R | G<<8 | B<<16 | A<<24, DS row 0 first) -> tiled PICA texture. */
unsigned long long P3DS_SpkTexTicks; unsigned P3DS_SpkTexN; /* run 43 [SPIKE] diag */
static void *TexCreate0(const u32 *px, unsigned w, unsigned h);
void *G3G_TexCreate(const u32 *px, unsigned w, unsigned h) { u64 t0 = svcGetSystemTick(); void *r = TexCreate0(px, w, h); P3DS_SpkTexTicks += svcGetSystemTick() - t0; P3DS_SpkTexN++; return r; }
static void texFill(C3D_Tex *t, const u32 *px, unsigned w, unsigned h);
int G3G_RbSyncOn(void) { return rbSyncOn(); }
/* run 54: refresh an existing GPU texture in place (tex_postvblank): no queued draw has executed yet (sync path). */
void G3G_TexUpdate(void *tp, const u32 *px, unsigned w, unsigned h) { C3D_Tex *t = tp; if (!t || !px || t->width != w || t->height != h) return; dirSync(); texFill(t, px, w, h); }
static void *TexCreate0(const u32 *px, unsigned w, unsigned h) {
    if (w < 8 || h < 8 || w > 1024 || h > 1024) return NULL;
    C3D_Tex *t = malloc(sizeof *t); if (!t) return NULL;
    if (!C3D_TexInit(t, w, h, GPU_RGBA8)) { static unsigned nf; if (nf++ < 20) PSPNativeMemLog("[GPU3D] tex alloc FAIL %ux%u linear free %lu (fail #%u)", w, h, (unsigned long)linearSpaceFree(), nf); free(t); return NULL; }
    texFill(t, px, w, h); C3D_TexSetFilter(t, GPU_NEAREST, GPU_NEAREST);
    return t;
}
static unsigned texGen;  /* bumped on every texture upload/refresh: part of the g3_reuse key */
static void texFill(C3D_Tex *t, const u32 *px, unsigned w, unsigned h) {
    u32 *d = t->data; texGen++;
    for (unsigned y = 0; y < h; y++) {
        unsigned my = G3G_TexFlip ? h - 1 - y : y;  /* sprites render upright with this (run 16) */
        for (unsigned x = 0; x < w; x++) {
            unsigned m = (x & 1) | ((my & 1) << 1) | ((x & 2) << 1) | ((my & 2) << 2) | ((x & 4) << 2) | ((my & 4) << 3);
            u32 c = px[y * w + x];
            d[((my >> 3) * (w >> 3) + (x >> 3)) * 64 + m] = __builtin_bswap32(c);
        }
    }
    C3D_TexFlush(t);
}
void G3G_TexFree(void *t) { if (!t) return; if (pendN < 512) pendFree[pendN++] = t; else { C3D_TexDelete(t); free(t); } }
void G3G_Begin(u32 nativeClear, unsigned clearDepth16, int stereo) {
    lastExpandUs = 0;
    dirSync();   /* gpu_direct3d: the previous frame's draw may still be running; the vertex buffers and textures are about to change */
    unsigned r = nativeClear & 63, g = (nativeClear >> 8) & 63, b = (nativeClear >> 16) & 63, a = (nativeClear >> 24) & 31;
    clearRGBA = ((r << 2 | r >> 4) << 24) | ((g << 2 | g >> 4) << 16) | ((b << 2 | b >> 4) << 8) | (a * 255 / 31);
    clearD = clearDepth16 / 65535.0f; eyes = stereo ? 2 : 1; vn[0] = vn[1] = bn[0] = bn[1] = 0; __atomic_store_n(&stereoPending, 0, __ATOMIC_RELEASE);
    G3G_MidRec = 0; krN = 0; kWantId = kCurWant = 1; midOver = 0; midValid = 0; midTot = midMat = midMov = 0; fpsHoldFlag = 0;
    { static int stLast = -1; if (stereo != stLast) { if (stLast >= 0 && wideFail) { wideFail = 0; PSPNativeMemLog("[3DRES] wide view retried after the 3D slider changed"); } stLast = stereo; }
      /* run 141: while a panel app (party/bag/summary in the JOHTO panel) is opening or open, keep the current eye targets: it draws no 3D the player
         sees (the held field is on top), and re-making the targets (VRAM free + alloc + the 2x scale-target dance) cost a ~100 ms frame at every open. */
      extern volatile int P3DS_PanelPartyWant, P3DS_PanelPartyOn; extern int P3DS_InField;
      static int ppHold, ppBack;   /* held from the tab tap until the overworld is back (wide view on again, or 90 field frames), so the close does not re-make them either */
      if (P3DS_PanelPartyWant || P3DS_PanelPartyOn) { ppHold = 1; ppBack = 0; }
      else if (ppHold && P3DS_InField && (G3G_Wide || ++ppBack >= 90)) ppHold = 0;
      const int ppKeep = ppHold && ok == 1 && curW > 0 && curHi >= 0;
      const int want = ppKeep ? curW : (G3G_Wide && !wideFail && !G3G_Async && rbSyncOn() ? 320 : 256);
      if (res3dCfg < 0) { res3dCfg = 0; PSPNativeMemLog("[3DRES] res3d disabled (run 52: broken on hardware; config res3d ignored)"); }
      /* 2026-10-26 g3_smooth (default 1, Settings > 3D > Smooth title 3D): the title Lugia (DS W-buffer frames; on the BOTTOM screen via the POWCNT
         swap, so never stereo: P3DS_StereoShiftNow is 0 there) uses the res3d 2x target + hardware 2x2 box downscale, i.e. 4x supersampled edges. At 1x Nathan found the title
         model blocky next to footage of the real game. Turns on after 30 W-buffer frames in a row (no target churn
         on short W bursts), off at the first other frame. Other scenes stay at 1x (res3d stays disabled there). */
      static unsigned smRun; static int smLast = -1; extern u8 s_SIM_useWBuffer; extern int P3DS_CfgInt(const char *, int);
      smRun = s_SIM_useWBuffer && !stereo ? smRun + 1 : 0;
      /* 2026-10-28: only with SHARP 3D on. Its 2x target never fitted before (all passes FAILED, title at 1x = the known-good look);
         with SHARP 3D off the title stays on that 1x path whatever the VRAM room work does. */
      const int sm = smRun >= 30 && P3DS_CfgInt("g3_smooth_dbg", 0) != 0 && P3DS_CfgInt("g3_sharp", 0) != 0; /* 2026-11-18: renamed from g3_smooth (row removed): the 2x title ran 19-22 fps, an old g3_smooth=1 line kept it on */
      if (sm != smLast) { smLast = sm; PSPNativeMemLog("[LUGIA] smooth title 3D (2x supersampled) %s | 3D on the %s screen (POWCNT %04x)", sm ? "ON" : "off", (*(const u16 *)(PSPNative_GfxRegisters + 0x304) & 0x8000) ? "TOP" : "BOTTOM", *(const u16 *)(PSPNative_GfxRegisters + 0x304)); }
      /* 2026-10-28 g3_sharp (missing-key default 3 = FAST AUTO, Settings > 3D > Sharp 3D, remove once tested): the same 2x target + box downscale for
         every scene (overworld, battles), mono or stereo (one target shared by both eyes, run 48). The run 52 "garbage" was the
         16-bit depth fallback (Platinum run 49 notes); depth is now 24-bit in VRAM or linear RAM and only 16 as a last resort.
         Not on fog frames (DISP3DCNT bit 7: the GPU fog needs the 1x target, so those stay 1x and keep their fog), and only
         after 30 eligible frames in a row so a fog flicker does not churn targets. GPU 3D direct and frame reuse are 1x-only. */
      static unsigned shRun, shN, shOn, shFog; static int shLast = -1;
      const int shCfg = P3DS_CfgInt("g3_sharp", 0) != 0, shFogNow = (*(const u16 *)(PSPNative_GfxRegisters + 0x60) & 0x80) != 0;
      shRun = shCfg && !shFogNow ? shRun + 1 : 0;
      const int sh = shRun >= 30;
      if (shCfg) { shN++; shOn += curHi == 1; shFog += shFogNow; if (shN >= 600) { PSPNativeMemLog("[3DRES] sharp 3D: %u of %u frames at 2x, %u fog frames kept 1x, depth %s, %s", shOn, shN, shFog, depth16 ? "16 (stripes possible!)" : depthLin ? "24 linear" : "24 VRAM", res3dFail ? "FAILED (no memory, 1x)" : "ok"); shN = shOn = shFog = 0; } }
      if (sh != shLast) { shLast = sh; PSPNativeMemLog("[3DRES] sharp 3D (g3_sharp=%d) %s%s", shCfg, sh ? "ON" : "off", shCfg && shFogNow ? " (fog frame)" : ""); }
      /* 2026-10-28 FAST 3D (g3_sharp=2), slider at 0: the 2x box downscale averages the game's blocky textures (run 54: "a tad mushy"), so
         mono draws at plain 1x = the crisp SHARP-OFF picture; stereo keeps the 2x left eye. Needs 30 mono frames in a row before the
         targets change (the VRAM swap is not free), and the first stereo frame switches back at once. */
      static unsigned monoRun; static int monoLast = -1;
      monoRun = stereo ? 0 : monoRun + 1;
      /* run 62 FAST AUTO (g3_sharp=3): FAST 3D, but while the stereo overworld cannot hold 30 fps (run 62 log: 24-27 fps, G3G_End ~11 ms with
         the 2x left eye) the left eye drops to 1x like mono. The frame period is averaged over 60 stereo frames at 2x; above 34.8 ms the
         governor drops to 1x and stays there until the map or battle state changes, the slider goes to 0, or the setting changes. */
      static int gvDown, gvKey = -1, gvN, gvCfg; static u64 gvT, gvAcc;
      /* 2026-11-04 (Nathan, run 148 log: "still slow entering a battle"): every battle and every return to the field retried 2x and
         spent its first ~2 s at 2x (53 ms frames: no GPU 3D direct at 2x, and the battle intro's 3D translucency then fails the GPU
         compose -> CPU composite 14 ms). A scene that could not hold 2x stays 1x for the rest of the session, and once one battle
         could not, battles start at 1x; after two scenes that could not, every scene does. Battles are always 1x with the slider up. */
      static int gvBad[64], gvBadN, gvBattleBad;
      { extern volatile int P3DS_MmMap; extern int P3DS_InBattle;
        const int cfgV = P3DS_CfgInt("g3_sharp", 0), key = P3DS_MmMap * 2 + (P3DS_InBattle != 0);
        const u64 now = svcGetSystemTick(); const unsigned dt = gvT ? (unsigned)((now - gvT) / 268) : 0; gvT = now;
        if (cfgV != gvCfg || !stereo || key != gvKey) {
            int bad = (cfgV == gvCfg && gvBadN >= 2) || P3DS_InBattle; /* battles: 2x never held with the slider up (run 148 log 53 ms, Azahar 87 ms), so they start and stay 1x */ for (int i = 0; i < gvBadN && !bad; i++) bad = gvBad[i] == key && cfgV == gvCfg;
            if (stereo && cfgV == 3 && key != gvKey) PSPNativeMemLog("[3DRES] FAST AUTO: scene changed, %s", bad ? "this scene could not hold 2x before: staying 1x" : "trying 2x");
            if (cfgV != gvCfg) { gvBadN = 0; gvBattleBad = 0; bad = 0; }
            gvDown = bad; gvN = 0; gvAcc = 0; gvCfg = cfgV; gvKey = key; }
        if (cfgV == 3 && stereo && !gvDown && curHi == 1 && dt < 200000) { gvAcc += dt; if (++gvN >= 15) {   /* 15 frames (~0.5 s) decide; 60 cost ~2 s of slow frames per try */ const unsigned avg = (unsigned)(gvAcc / gvN);
            if (avg > 34800) { gvDown = 1; if (gvBadN < 64) gvBad[gvBadN++] = key; if (P3DS_InBattle) gvBattleBad = 1; PSPNativeMemLog("[3DRES] FAST AUTO: stereo frames average %u us at 2x (over 34800), left eye back to 1x until the scene changes", avg); }
            gvN = 0; gvAcc = 0; } } }
      const int fastMono = shCfg && P3DS_CfgInt("g3_sharp", 0) >= 2 && P3DS_CfgInt("g3_sharp", 0) != 4 && (monoRun >= 30 || gvDown);
      if (fastMono != monoLast) { monoLast = fastMono; PSPNativeMemLog("[3DRES] FAST 3D: mono (slider 0) draws at %s", fastMono ? "1x (crisp textures)" : "2x / stereo"); }
      /* fps60: the in-between frame needs the direct 1x path, so Sharp 3D's 2x target rests in the field while 60 fps field is set */
      const int f60 = P3DS_Fps60Cfg() != 0 && P3DS_InField;
      const int wantHi = ppKeep ? curHi : ((res3dCfg == 2 || sm || (sh && !fastMono && !f60)) && !res3dFail && !G3G_Async && rbSyncOn() ? 1 : 0);
      if ((want != curW || wantHi != curHi) && ok == 1) {
        /* 2026-10-28: the 2x target only works in VRAM (run 49), and VRAM is full of the scale targets: free them first,
           make the 2x target, then get the scale targets back at 16 bits (gpuscale.c P3DS_UpAlloc). Back at 1x: RGBA8 again. */
        extern void P3DS_UpFree(void); extern int P3DS_UpAlloc(int);
        /* run 50 (Azahar): going 512x384 -> 640x384 with the 16-bit scale targets still in place FAILED (VRAM in pieces), the 1x
           fallback then took the holes and the RGBA8 scale targets no longer fitted: no frame was presented. So whenever a 2x
           target is involved (old or new), the scale targets are freed first and allocated last, and P3DS_UpAlloc falls back to
           the other format rather than leaving none. */
        const int hiDance = wantHi || curHi == 1;
        const int dance = hiDance || (f60 && want == 320 && curW != want);
        if (dance) P3DS_UpFree();
        setTargets(want, wantHi);
        if (dance && !P3DS_UpAlloc(curHi == 1)) {
          PSPNativeMemLog("[3DRES] scale targets did not fit next to the %s target, back to 1x", curHi == 1 ? "2x" : "1x");
          if (curHi == 1) { extern void P3DS_Toast(const char *); res3dFail = 1; P3DS_Toast("SHARP 3D: NO VRAM"); }
          P3DS_UpFree(); setTargets(want, 0); P3DS_UpAlloc(0); }
        if (f60 && dance) {
          extern void P3DS_GcRetryRightTarget(void);
          P3DS_GcRetryRightTarget();
        }
        /* run 50 hardware log: a 320x256 (wide) target that failed once during the 2x dance set wideFail for the rest of the
           session ("my wide screen is not loading anymore"). After every 2x change the VRAM layout is new: try wide again. */
        if (hiDance && wideFail) { wideFail = 0; PSPNativeMemLog("[3DRES] wide view retried after the VRAM change"); } }
      if (sh && curHi == 1 && depth16 && !sm) { res3dFail = 1; PSPNativeMemLog("[3DRES] sharp 3D: only 16-bit depth fitted (the run 48 stripes), back to 1x"); { extern void P3DS_Toast(const char *); P3DS_Toast("SHARP 3D: NO MEMORY"); } setTargets(want, 0); { extern int P3DS_UpAlloc(int); P3DS_UpAlloc(0); } }
      wideFrame = curW == 320; hiFrame = curHi == 1; }
    if (P3DS_Fps60Active() && !P3DS_Fps60Skip && ok == 1 && curHi == 0 && !G3G_Async && G3G_WDepthOn() != 2 && midAlloc(eyes)) { G3G_MidRec = 1; krStart(NULL); }
}
static int wrapMode(int rep, int flip) { return !rep ? GPU_CLAMP_TO_EDGE : flip ? GPU_MIRRORED_REPEAT : GPU_REPEAT; }
static B *batchFor(int eye, void *tex, int repS, int repT, int flipS, int flipT, int blend, int dwrite, int dtest, int late) {
    if (ok != 1 || eye < 0 || eye > 1 || vn[eye] + 3 > MAXV) return NULL;
    int ws = wrapMode(repS, flipS), wt = wrapMode(repT, flipT), df = dtest ? GPU_EQUAL : GPU_LEQUAL;
    B *last = bn[eye] ? &bt[eye][bn[eye] - 1] : NULL;
    if (!last || last->tex != tex || last->wrapS != ws || last->wrapT != wt || last->blend != blend || last->dwrite != dwrite || last->dfunc != df || last->late != late || last->fog != G3G_CurFog) {
        if (bn[eye] >= MAXB) return NULL;
        last = &bt[eye][bn[eye]++]; last->tex = tex; last->wrapS = ws; last->wrapT = wt; last->blend = blend; last->dwrite = dwrite; last->dfunc = df; last->late = late; last->fog = G3G_CurFog; last->first = vn[eye]; last->count = 0;
    }
    return last;
}
/* Raw-mode triangle in DS clip space (x,y,z,w per vertex), normalised uv: GPU does the divide, so texturing is
   perspective-correct and clipping is done in hardware. shiftPx: per-eye stereo shift in DS pixels per vertex. */
/* 2026-11-04 (solid-30 fps thread): i/255.0f as a table. The per-vertex colour divides (4 VDIVs, ~15 cycles each on the VFP11)
   were a visible part of GpuRaw; the table holds exactly the same floats, so the vertices are bit-identical. */
static float k255[256];
static inline const float *lut255(void) { if (k255[255] == 0.0f) for (int i = 0; i < 256; i++) k255[i] = i / 255.0f; return k255; }
void G3G_StereoExpand(void); /* sync right from left before any non-share write */
void G3G_Tri(int eye, const float *sx, const float *sy, const float *sz, const float *su, const float *sv, const u32 *col,
             void *tex, unsigned tw, unsigned th, int repS, int repT, int flipS, int flipT, int blend, int dwrite, int dtest);
void G3G_TriClip(int eye, const float *c, const float *su, const float *sv, const u32 *col, void *tex,
                 int repS, int repT, int flipS, int flipT, int blend, int dwrite, int dtest, const float *shiftPx) {
    G3G_StereoExpand(); /* pending share must land before any single-eye write */
    if (G3G_MidRec && !kCurWant) { krStart(kWantId ? NULL : kWant); kCurWant = 1; }
    G3T_VbLock();
    B *last = batchFor(eye, tex, repS, repT, flipS, flipT, blend, dwrite, dtest, blend);
    if (!last) { G3T_VbUnlock(); return; }
    const float *L = lut255();
    for (int k = 0; k < 3; k++) {
        V *o = &vb[eye][vn[eye]++]; float x = c[4 * k], y = c[4 * k + 1], z = c[4 * k + 2], w = c[4 * k + 3];
        o->x = x + (shiftPx ? shiftPx[k] / 128.0f * w : 0); if (wideFrame) o->x *= 0.8f;
        o->y = hiFrame ? (G3G_FlipY ? y : -y) : G3G_FlipY ? (0.25f * w + 0.75f * y) : (-0.25f * w - 0.75f * y);  /* res3d: full-height 384 target */
        o->z = G3G_ZInv ? 0.5f * z - 0.5f * w : -0.5f * z - 0.5f * w; o->w = w;
        if (G3G_WDepth == 2 && w > 1e-6f) o->z /= w;   /* g3_wdepth: W-buffer mode wants the undivided depth (-d) */
        o->s = su[k]; o->t = G3G_TexFlip ? sv[k] : 1.0f - sv[k];
        u32 cc = col[k]; o->r = L[cc & 255]; o->g = L[(cc >> 8) & 255]; o->b = L[(cc >> 16) & 255]; o->a = L[cc >> 24];
    }
    last->count += 3;
    G3T_VbUnlock();
}
/* 2026-11-04 (solid-30 fps thread): both eyes of a stereo raw triangle in one pass. Same values as two G3G_TriClip calls (only x
   differs between the eyes), but y, z, uv and colour are worked out once instead of twice.
   g3_stereo_share: write left only + (shR-shL); G3G_StereoExpand fills right (after drain, or before any non-share write
   such as hard-clip). Bit-identical to dual TriClip2. Cuts ~half the vb store traffic on the raw miss path. */
/* stereoDsh[i] = shR-shL for left vert i (DS px). CPU-only: use malloc, NOT linearAlloc (run 9bbfcd1:
   linearAlloc of MAXV floats ~192 KiB often failed silently → stereoShareOn cleared → no raw cut). */
static unsigned stereoShareTris, stereoDualTris, stereoShareExpands, stereoShareExpVerts;
void G3G_SetStereoShare(int on) {
    __atomic_store_n(&stereoShareOn, on != 0, __ATOMIC_RELEASE);
    if (!on) __atomic_store_n(&stereoPending, 0, __ATOMIC_RELEASE);
}
/* g3_stereo_gpu: isolated path — default V / g3.v.pica / AttrInfo 4-2-4 never change (run 887).
   When ON + share left-only: FlushDeferred may skip CPU expand; End lazily inits g3s and draws both eyes
   from vb[0] with stereok * dsh * w. Prefer gpuRight: skip mid/K when left-only pending; hard-clip /
   translucent screen tris write left+dsh (no expand) so eye 1 stays empty. Fallback reasons counted
   in 60f path log (hw e8900f9: path flipped ~half the frames → right eye jump). */
static int stereoGpuWant; /* FlushDeferred: cfg+share this frame — skip game-thread expand, leave pending for End */
static int stereoGpuSaid, stereoGpuFailSaid;
static DVLB_s *dvlbS; static shaderProgram_s progS; static int uStereoK = -1, uKS = -1, progSOk;
static unsigned stereoGpuRightN, stereoGpuExpandN; /* frames that took each path (summed into 60f stage log) */
/* Fallback reason counts (cpuExpand frames only): mid skipped so unused; eye1 = dual/overflow; nopend;
   noshad; nogeom; eyes. */
enum { STER_WHY_EYE1 = 0, STER_WHY_NOPEND, STER_WHY_NOSHAD, STER_WHY_NOGEOM, STER_WHY_EYES, STER_WHY_N };
static unsigned stereoGpuWhy[STER_WHY_N];
static int stereoGpuEnsure(void) {
    if (progSOk) return 1;
    dvlbS = DVLB_ParseFile((u32 *)g3s_shbin, g3s_shbin_size);
    if (!dvlbS) {
        if (!stereoGpuFailSaid) { stereoGpuFailSaid = 1; PSPNativeMemLog("[G3STER] g3s shader parse FAILED: stereo_gpu stays CPU expand"); }
        return 0;
    }
    shaderProgramInit(&progS); shaderProgramSetVsh(&progS, &dvlbS->DVLE[0]);
    uStereoK = shaderInstanceGetUniformLocation(progS.vertexShader, "stereok");
    uKS = shaderInstanceGetUniformLocation(progS.vertexShader, "kmtx");
    if (uStereoK < 0 || uKS < 0) {
        if (!stereoGpuFailSaid) { stereoGpuFailSaid = 1; PSPNativeMemLog("[G3STER] g3s stereok uniform missing: stereo_gpu stays CPU expand"); }
        return 0;
    }
    if (!stereoDshLin) stereoDshLin = (float *)linearAlloc(MAXV * sizeof(float));
    if (!stereoDshLin) {
        if (!stereoGpuFailSaid) { stereoGpuFailSaid = 1; PSPNativeMemLog("[G3STER] linear dsh buffer alloc FAILED: stereo_gpu stays CPU expand"); }
        return 0;
    }
    progSOk = 1; PSPNativeMemLog("[G3STER] g3s shader ready (stereo_gpu path; default g3 untouched)");
    return 1;
}
void G3G_SetStereoGpu(int on) {
    /* Want only — do not parse/bind g3s here (may run before the first C3D frame). End ensures lazily. */
    const int was = stereoGpuWant;
    stereoGpuWant = on != 0;
    if (stereoGpuWant && !was && !stereoGpuSaid) {
        stereoGpuSaid = 1;
        PSPNativeMemLog("[G3STER] g3_stereo_gpu ON: End will use shader right eye when left-only; else CPU expand");
    }
}
int G3G_StereoGpuOn(void) { return stereoGpuWant; }
int G3G_StereoPending(void) { return __atomic_load_n(&stereoPending, __ATOMIC_ACQUIRE); }
/* True when stereo_gpu can keep writing left+dsh (eye1 still empty, pending share). Hard-clip / translucent
   screen draws use this to avoid Expand+dual which would force cpuExpand for the rest of the frame. */
int G3G_StereoGpuLeftOnly(void) {
    return stereoGpuWant && stereoDsh && vn[1] == 0 && bn[1] == 0 &&
           __atomic_load_n(&stereoPending, __ATOMIC_ACQUIRE);
}
unsigned G3G_LastExpandUs(void) { return lastExpandUs; }
void G3G_StereoPathStats(unsigned *gpuRight, unsigned *cpuExpand) {
    if (gpuRight) *gpuRight = stereoGpuRightN;
    if (cpuExpand) *cpuExpand = stereoGpuExpandN;
    stereoGpuRightN = stereoGpuExpandN = 0;
}
/* whyOut[5]: eye1, nopend, noshad, nogeom, eyes — only meaningful for cpuExpand frames. */
void G3G_StereoPathWhy(unsigned *whyOut, unsigned n) {
    if (whyOut && n) {
        unsigned i; for (i = 0; i < n && i < STER_WHY_N; i++) whyOut[i] = stereoGpuWhy[i];
        for (; i < n; i++) whyOut[i] = 0;
    }
    memset(stereoGpuWhy, 0, sizeof stereoGpuWhy);
}
void G3G_StereoShareStats(unsigned *leftTris, unsigned *dualTris, unsigned *expands, unsigned *expVerts) {
    if (leftTris) *leftTris = stereoShareTris;
    if (dualTris) *dualTris = stereoDualTris;
    if (expands) *expands = stereoShareExpands;
    if (expVerts) *expVerts = stereoShareExpVerts;
    stereoShareTris = stereoDualTris = stereoShareExpands = stereoShareExpVerts = 0;
}
void G3G_StereoExpand(void) {
    if (!__atomic_load_n(&stereoShareOn, __ATOMIC_ACQUIRE) || !stereoDsh || !__atomic_load_n(&stereoPending, __ATOMIC_ACQUIRE)) return;
    G3T_VbLock();
    if (!__atomic_load_n(&stereoPending, __ATOMIC_ACQUIRE) || vn[0] > MAXV || bn[0] > MAXB) { G3T_VbUnlock(); return; }
    const u64 t0 = svcGetSystemTick();
    const float scale = wideFrame ? 0.8f : 1.0f;
    const unsigned v0 = vn[1], b0 = bn[1];
    if (vn[0] > vn[1]) {
        memcpy(vb[1] + v0, vb[0] + v0, (vn[0] - v0) * sizeof(V));
        for (unsigned i = v0; i < vn[0]; i++) vb[1][i].x += scale * (stereoDsh[i] / 128.0f) * vb[0][i].w;
        stereoShareExpVerts += vn[0] - v0;
    }
    /* Incremental batch sync: update in-place count growth, then copy only new batches (not the whole table). */
    if (b0 > 0 && b0 <= bn[0]) bt[1][b0 - 1].count = bt[0][b0 - 1].count;
    if (bn[0] > b0) memcpy(bt[1] + b0, bt[0] + b0, (bn[0] - b0) * sizeof(B));
    vn[1] = vn[0]; bn[1] = bn[0];
    __atomic_store_n(&stereoPending, 0, __ATOMIC_RELEASE); stereoShareExpands++;
    /* Path frame counts live in G3G_End (with fallback reasons); do not bump expandN here. */
    lastExpandUs = (unsigned)((svcGetSystemTick() - t0) / 268);
    G3T_VbUnlock();
}
void G3G_TriClip2(const float *c, const float *su, const float *sv, const u32 *col, void *tex,
                  int repS, int repT, int flipS, int flipT, int blend, int dwrite, int dtest, const float *shL, const float *shR) {
    if (G3G_MidRec && !kCurWant) { krStart(kWantId ? NULL : kWant); kCurWant = 1; }
    G3T_VbLock();
    if (__atomic_load_n(&stereoShareOn, __ATOMIC_ACQUIRE)) {
        if (!stereoDsh) {
            stereoDsh = (float *)malloc(MAXV * sizeof(float));
            if (!stereoDsh) {
                __atomic_store_n(&stereoShareOn, 0, __ATOMIC_RELEASE);
                static unsigned once; if (!once++) PSPNativeMemLog("[G3STER] stereoDsh malloc FAILED (%u floats): share OFF this session", (unsigned)MAXV);
            } else {
                memset(stereoDsh, 0, MAXV * sizeof(float));
                PSPNativeMemLog("[G3STER] stereoDsh ready (%u KiB heap, not linear)", (unsigned)(MAXV * sizeof(float) >> 10));
            }
        }
        if (stereoDsh && vn[0] + 3 <= MAXV) {
            B *l0 = batchFor(0, tex, repS, repT, flipS, flipT, blend, dwrite, dtest, blend);
            if (!l0) { G3T_VbUnlock(); return; }
            const float *L = lut255();
            for (int k = 0; k < 3; k++) {
                V *o = &vb[0][vn[0]]; float x = c[4 * k], y = c[4 * k + 1], z = c[4 * k + 2], w = c[4 * k + 3];
                o->x = x + shL[k] / 128.0f * w; if (wideFrame) o->x *= 0.8f;
                o->y = hiFrame ? (G3G_FlipY ? y : -y) : G3G_FlipY ? (0.25f * w + 0.75f * y) : (-0.25f * w - 0.75f * y);
                o->z = G3G_ZInv ? 0.5f * z - 0.5f * w : -0.5f * z - 0.5f * w; o->w = w;
                if (G3G_WDepth == 2 && w > 1e-6f) o->z /= w;
                o->s = su[k]; o->t = G3G_TexFlip ? sv[k] : 1.0f - sv[k];
                u32 cc = col[k]; o->r = L[cc & 255]; o->g = L[(cc >> 8) & 255]; o->b = L[(cc >> 16) & 255]; o->a = L[cc >> 24];
                stereoDsh[vn[0]] = shR[k] - shL[k];
                vn[0]++;
            }
            l0->count += 3; stereoShareTris++; __atomic_store_n(&stereoPending, 1, __ATOMIC_RELEASE);
            G3T_VbUnlock(); return;
        }
    }
    stereoDualTris++;
    B *l0 = batchFor(0, tex, repS, repT, flipS, flipT, blend, dwrite, dtest, blend), *l1 = batchFor(1, tex, repS, repT, flipS, flipT, blend, dwrite, dtest, blend);
    if (!l0 || !l1) {
        G3T_VbUnlock();
        if (l0) G3G_TriClip(0, c, su, sv, col, tex, repS, repT, flipS, flipT, blend, dwrite, dtest, shL);
        if (l1) G3G_TriClip(1, c, su, sv, col, tex, repS, repT, flipS, flipT, blend, dwrite, dtest, shR);
        return;
    }
    const float *L = lut255();
    for (int k = 0; k < 3; k++) {
        V *o = &vb[0][vn[0]++], *q = &vb[1][vn[1]++]; float x = c[4 * k], y = c[4 * k + 1], z = c[4 * k + 2], w = c[4 * k + 3];
        o->x = x + shL[k] / 128.0f * w; q->x = x + shR[k] / 128.0f * w; if (wideFrame) { o->x *= 0.8f; q->x *= 0.8f; }
        o->y = hiFrame ? (G3G_FlipY ? y : -y) : G3G_FlipY ? (0.25f * w + 0.75f * y) : (-0.25f * w - 0.75f * y);
        o->z = G3G_ZInv ? 0.5f * z - 0.5f * w : -0.5f * z - 0.5f * w; o->w = w;
        if (G3G_WDepth == 2 && w > 1e-6f) o->z /= w;
        o->s = su[k]; o->t = G3G_TexFlip ? sv[k] : 1.0f - sv[k];
        u32 cc = col[k]; o->r = L[cc & 255]; o->g = L[(cc >> 8) & 255]; o->b = L[(cc >> 16) & 255]; o->a = L[cc >> 24];
        q->y = o->y; q->z = o->z; q->w = w; q->s = o->s; q->t = o->t; q->r = o->r; q->g = o->g; q->b = o->b; q->a = o->a;
    }
    l0->count += 3; l1->count += 3;
    G3T_VbUnlock();
}
/* Screen-space stereo under stereo_gpu left-only: write left eye + dsh (right_sx - left_sx in DS px).
   Same parallax as dual G3G_Tri + expand (w=1 → NDC delta = dsh/128, wide uses 0.8). Keeps vn[1]==0 so End
   can stay on gpuRight instead of flip-flopping after hard-clip / translucent. */
void G3G_TriScreenShare(const float *sxL, const float *sy, const float *sz, const float *su, const float *sv, const u32 *col,
                        const float *dshPx, void *tex, unsigned tw, unsigned th, int repS, int repT, int flipS, int flipT,
                        int blend, int dwrite, int dtest) {
    if (!stereoDsh || vn[0] + 3 > MAXV) {
        /* Overflow: fall back to expand + dual write via G3G_Tri. */
        float sxR[3];
        for (int k = 0; k < 3; k++) sxR[k] = sxL[k] + (dshPx ? dshPx[k] : 0.0f);
        G3G_Tri(0, sxL, sy, sz, su, sv, col, tex, tw, th, repS, repT, flipS, flipT, blend, dwrite, dtest);
        G3G_Tri(1, sxR, sy, sz, su, sv, col, tex, tw, th, repS, repT, flipS, flipT, blend, dwrite, dtest);
        return;
    }
    if (G3G_MidRec && !kWantId && kCurWant) { krStart(NULL); kCurWant = 0; }
    G3T_VbLock();
    B *last = batchFor(0, tex, repS, repT, flipS, flipT, blend, dwrite, dtest, 0);
    if (!last) { G3T_VbUnlock(); return; }
    float itw = tw ? 1.0f / tw : 0, ith = th ? 1.0f / th : 0; const float *L = lut255();
    for (int k = 0; k < 3; k++) {
        V *o = &vb[0][vn[0]]; float z = sz[k] / 65535.0f; if (z < 0) z = 0; if (z > 1) z = 1;
        o->x = wideFrame ? (sxL[k] + 32.0f) / 160.0f - 1.0f : sxL[k] / 128.0f - 1.0f;
        o->y = (G3G_FlipY ? 192.0f - sy[k] : sy[k]) / (hiFrame ? 96.0f : 128.0f) - 1.0f;
        o->z = G3G_ZInv ? z - 1.0f : -z; o->w = 1.0f;
        o->s = su[k] * itw; o->t = G3G_TexFlip ? sv[k] * ith : 1.0f - sv[k] * ith;
        u32 c = col[k]; o->r = L[c & 255]; o->g = L[(c >> 8) & 255]; o->b = L[(c >> 16) & 255]; o->a = L[c >> 24];
        stereoDsh[vn[0]] = dshPx ? dshPx[k] : 0.0f;
        vn[0]++;
    }
    last->count += 3; __atomic_store_n(&stereoPending, 1, __ATOMIC_RELEASE);
    G3T_VbUnlock();
}
/* v: 3 x {sx, sy, z(0..65535), u(texels), v(texels), color RGBA8 as u32 R|G<<8|B<<16|A<<24} */
void G3G_Tri(int eye, const float *sx, const float *sy, const float *sz, const float *su, const float *sv, const u32 *col,
             void *tex, unsigned tw, unsigned th, int repS, int repT, int flipS, int flipT, int blend, int dwrite, int dtest) {
    G3G_StereoExpand(); /* expand share before hard-clip / any screen-space write (either eye) */
    if (G3G_MidRec && !kWantId && kCurWant) { krStart(NULL); kCurWant = 0; }   /* screen-space triangle (CPU-clipped): same place in both pictures */
    G3T_VbLock();
    B *last = batchFor(eye, tex, repS, repT, flipS, flipT, blend, dwrite, dtest, 0);
    if (!last) { G3T_VbUnlock(); return; }
    float itw = tw ? 1.0f / tw : 0, ith = th ? 1.0f / th : 0; const float *L = lut255();
    for (int k = 0; k < 3; k++) {
        V *o = &vb[eye][vn[eye]++]; float z = sz[k] / 65535.0f; if (z < 0) z = 0; if (z > 1) z = 1;
        o->x = wideFrame ? (sx[k] + 32.0f) / 160.0f - 1.0f : sx[k] / 128.0f - 1.0f; o->y = (G3G_FlipY ? 192.0f - sy[k] : sy[k]) / (hiFrame ? 96.0f : 128.0f) - 1.0f; o->z = G3G_ZInv ? z - 1.0f : -z; o->w = 1.0f;
        o->s = su[k] * itw; o->t = G3G_TexFlip ? sv[k] * ith : 1.0f - sv[k] * ith;  /* run 17: billboards were upside down with 1-v */
        u32 c = col[k]; o->r = L[c & 255]; o->g = L[(c >> 8) & 255]; o->b = L[(c >> 16) & 255]; o->a = L[c >> 24];
    }
    last->count += 3;
    G3T_VbUnlock();
}
static unsigned lastUs;
unsigned G3G_LastUs(void) { return lastUs; }
int G3G_Hi(void) { return curHi == 1; }
/* run 47: L+R+A toggles res3d 0 <-> 2 in-game (takes effect next frame; a failed 2x allocation is retried on each toggle) */
int G3G_Res3dToggle(void) { res3dCfg = 0; return 0; }  /* run 52: sharp 3D disabled; L+R+A is a swallowed no-op */
/* Draw queued triangles, read back into out[eye] (compositor format). */
/* run 49: clear color + depth of the current target with a full-screen quad (depth in linear RAM: no memory fill) */
static void clearQuad(void) {
    const float d = clearD < 0.9999990f ? clearD : 0.9999990f, z = -d;
    const float r = (clearRGBA >> 24) / 255.0f, g = ((clearRGBA >> 16) & 255) / 255.0f, b = ((clearRGBA >> 8) & 255) / 255.0f, a = (clearRGBA & 255) / 255.0f;
    static const float xy[6][2] = { {-1, -1}, {1, -1}, {1, 1}, {-1, -1}, {1, 1}, {-1, 1} };
    for (int i = 0; i < 6; i++) { V *o = &qv[i]; o->x = xy[i][0]; o->y = xy[i][1]; o->z = z; o->w = 1.0f; o->s = o->t = 0; o->r = r; o->g = g; o->b = b; o->a = a; }
    P3DS_DcFlush(qv, 6 * sizeof(V));
    C3D_BufInfo *bi = C3D_GetBufInfo(); BufInfo_Init(bi); BufInfo_Add(bi, qv, sizeof(V), 3, 0x210);
    C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env); C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    C3D_AlphaTest(false, GPU_ALWAYS, 0); C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    C3D_DepthTest(true, GPU_ALWAYS, GPU_WRITE_ALL);
    C3D_DrawArrays(GPU_TRIANGLES, 0, 6);
    C3D_AlphaTest(true, GPU_GREATER, 3);
}
/* g3_reuse (menu: 3D FRAME REUSE, default ON): menus, title and dialogs draw only a few triangles but still pay the
   GPU draw + readback + convert (18-28 ms). The GPU is deterministic, so identical inputs give an identical image:
   a frame whose complete input (vertex data, batches, texture pointers + a texture upload counter, wrap modes, clear
   colour/depth, fog registers, eye count and layout) is byte-equal to the previous eligible frame reuses that
   frame's converted output (memcpy). A frame with no triangles at all is filled with the converted clear colour.
   Only small frames (<= RU_MAXV vertices, <= RU_MAXB batches) are compared; wide/2x/async/CPU-fog frames never.
   g3_reuse_verify=1 (config.txt): also renders the frame and logs any pixel that differs from the reused output. */
#define RU_MAXV 1200
#define RU_MAXB 64
typedef struct { u32 clr, fc, tg; float depth; int eyes, haveL, haveR, fog, dir; u16 dc, f350, f35c, pad; unsigned vn[2], bn[2]; unsigned char tab[32]; } RK;
static RK ruKey; static V *ruV[2]; static B *ruB[2]; static u32 *ruOut[2]; static int ruValid, ruVerify = -1;
static unsigned ruHit, ruEmpty, ruDraw, ruBig, ruBad; static unsigned long long ruSaveUs, ruHitUs, ruDrawUs; static unsigned ruFrames;
extern int P3DS_G3ReuseCfg(void); extern int P3DS_CfgInt(const char *, int);
static u32 convWord(u32 c) { const u32 t = (__builtin_bswap32(c) >> 2) & 0x3F3F3F3Fu; return (t & 0x003F3F3Fu) | ((t >> 1) & 0x1F000000u); }
static void ruFreeDeferred(void) {
    dirSync(); pendRB = 0; for (unsigned i = 0; i < oldN; i++) { C3D_TexDelete(oldFree[i]); free(oldFree[i]); } oldN = 0;
    for (unsigned i = 0; i < pendN; i++) { C3D_TexDelete(pendFree[i]); free(pendFree[i]); } pendN = 0;
}
static void ruLog(void) {
    if (++ruFrames % 300 != 0) return;
    PSPNativeMemLog("[G3REUSE] 300f: reused %u, empty %u, drawn small %u (avg %u us), drawn big %u | reuse avg %u us, mismatches %u (verify %d) | saved ~%u ms",
        ruHit, ruEmpty, ruDraw, ruDraw ? (unsigned)(ruDrawUs / ruDraw) : 0, ruBig, ruHit ? (unsigned)(ruHitUs / ruHit) : 0, ruBad, ruVerify,
        (unsigned)(ruSaveUs / 1000));
    ruHit = ruEmpty = ruDraw = ruBig = 0; ruDrawUs = ruHitUs = ruSaveUs = 0;
}
extern void P3DS_Crumb(const char *tag, void *caller);
/* ss3ds fps60_probe=1 (log only, default 0): would a 60 fps field be able to pair this frame's triangles with the
   previous frame's? Pokeplatinum-3ds makes its in-between scene by pairing polygons across two game frames in draw
   order; this counts how many triangles at the same index still have the same texture coords (same kind of polygon) and
   how far their screen position moved (DS pixels: x/w*128, y/w*96 of the clip-space vertex). One line per ~5 s. */
static void fps60Probe(void) {
    static int cfg = -1, pfail; int on; static V *prev; static unsigned prevN, frames, tot, kind, near16, near4, still, shortRun; static double sumMove;
    extern int P3DS_CfgInt(const char *, int); extern int P3DS_InField;
    extern int P3DS_FpsTweaks(void);
    if (cfg < 0) cfg = 0 /* fps60_probe: debug option removed */ != 0;
    on = cfg && !pfail;   /* Research-only: explicitly opt in with fps60_probe=1. */
    if (on && !prev) { prev = malloc(MAXV * sizeof(V)); if (!prev) { pfail = 1; on = 0; } }
    if (!on || !P3DS_InField || eyes < 1) { prevN = 0; return; }
    const unsigned n = vn[0] / 3;
    if (prevN && n) { const unsigned m = n < prevN ? n : prevN;
        for (unsigned i = 0; i < m; i++) { const V *a = &vb[0][i * 3], *b = &prev[i * 3]; int same = 1; float mv = 0;
            for (int k = 0; k < 3; k++) { if (fabsf(a[k].s - b[k].s) > 0.02f || fabsf(a[k].t - b[k].t) > 0.02f) same = 0;
                if (a[k].w > 1e-3f && b[k].w > 1e-3f) { float dx = (a[k].x / a[k].w - b[k].x / b[k].w) * 128.0f, dy = (a[k].y / a[k].w - b[k].y / b[k].w) * 96.0f; mv += sqrtf(dx * dx + dy * dy); } else mv += 999.0f; }
            tot++; if (!same) continue; kind++; mv /= 3.0f; sumMove += mv < 999.0f ? mv : 0; if (mv < 16.0f) near16++; if (mv < 4.0f) near4++; if (mv < 0.25f) still++; }
        if (n != prevN) shortRun++;
        tot += (n > prevN ? n : prevN) - m; }
    if (n) { memcpy(prev, vb[0], vn[0] * sizeof(V)); prevN = n; } else prevN = 0;
    if (++frames >= 150 && tot) {
        PSPNativeMemLog("[FPS] probe %u frames: %u tris/frame, same kind at same index %u%%, of those moved <16px %u%% <4px %u%% still %u%%, mean move %.2f px, triangle count changed in %u frames",
            frames, tot / frames, kind * 100 / tot, kind ? near16 * 100 / kind : 0, kind ? near4 * 100 / kind : 0, kind ? still * 100 / kind : 0, kind ? sumMove / kind : 0.0, shortRun);
        frames = tot = kind = near16 = near4 = still = shortRun = 0; sumMove = 0; }
}
/* gpu_direct3d: the CPU needs the 3D layer pixels after all (CPU composite, stereo fallback, dump): queue the readback of the
   still-valid eye targets, wait, and convert into the pointers the frame was begun with. */
/* 2026-11-17 one-log GPU diagnostic (Debug tab, all default OFF):
   g3_gputime: every 30th direct 3D frame, G3G_End submits and waits after each pass (in-between L, R, copies; real L, R, copies)
   and logs the GPU time of each. Only that one frame waits; every other frame runs as before.
   [GPUT] lines are grouped by full readback vs slim strips (slim falls back to full only without linear memory), with the
   left compose's wait for the GPU and the CPU strip convert time. */
extern int P3DS_CfgInt(const char *, int);
static int gtCfg = -1;
int G3G_GpuTimeCfg(void) { if (gtCfg < 0) { gtCfg = P3DS_CfgInt("g3_gputime_dbg", 0) != 0; PSPNativeMemLog("[GPUT] g3_gputime=%d (1 = every 30th 3D frame waits after each GPU pass and logs it)", gtCfg); } return gtCfg; }
void G3G_SetGpuTime(int v) { gtCfg = v != 0; PSPNativeMemLog("[GPUT] g3_gputime=%d from settings", gtCfg); }
static int slimAlloc(void) {
    if (rbS[0] && rbS[1] && rbSTiles) return 1;
    if (slimFail) return 0;
    for (int e = 0; e < 2; e++) if (!rbS[e]) rbS[e] = linearAlloc(64 * 192 * sizeof(u32));
    if (!rbSTiles) rbSTiles = linearAlloc(64 * 192 * sizeof(u32));
    if (!rbS[0] || !rbS[1] || !rbSTiles) { slimFail = 1; PSPNativeMemLog("[GPUT] no linear memory for slim strips (free %lu): full readback kept", (unsigned long)linearSpaceFree()); return 0; }
    PSPNativeMemLog("[GPUT] slim strip buffers ready (3 x 48 KB, linear free %lu)", (unsigned long)linearSpaceFree());
    return 1;
}
/* stats by state: 1 = slim strips used */
static unsigned abFrames, gtCnt; static int abLast = -1, gtSkipCw;
static unsigned long long gtSum[2][6], cwSum[2], scSum[2]; static unsigned gtN[2], cwN[2], scN[2], frN[2];
static void abStripUs(unsigned us) { if (abLast >= 0) { scSum[abLast] += us; scN[abLast]++; } }
void G3G_AbComposeWait(unsigned us) {
    if (abLast < 0) return;
    if (gtSkipCw) { gtSkipCw = 0; return; }   /* the sampled frame already waited, so its compose wait reads ~0 */
    cwSum[abLast] += us; cwN[abLast]++;
}
static void gtMark(unsigned *cur, int slot, int *first) {
    const u64 a = svcGetSystemTick();
    if (*first) { *first = 0; P3DS_FrameEnd(); } else C3D_FrameEnd(GX_CMDLIST_FLUSH);   /* first submit keeps the usual flush rule */
    C3D_FrameBegin(0);   /* waits for the queue: this pass's GPU time */
    cur[slot] = (unsigned)((svcGetSystemTick() - a) / 268);
}
/* 2026-11-17 g3_scissor (Debug tab, default OFF, A/B TEST): the eye targets are 256 rows, but only 192 are ever shown or read
   (GPU rows 64..255: compositor v 0..0.75, strips, readbacks). Triangles that cross the bottom edge of the DS screen still fill the
   64 hidden rows. A scissor rectangle stops the GPU filling them. Which window rows are the hidden ones is not assumed: the first
   time, a full-target quad is drawn with a 64-row scissor and read back once; if neither end matches, the scissor stays off.
   [GPUT] lines are grouped by scissor OFF / ON (A/B steps every 600 3D frames). */
static int scisCfg = -1, scisMode = -1;   /* scisMode: -1 not calibrated, 0 hidden = window rows 0..63, 1 = rows 192..255, 2 failed */
#define SCIS_PENDING "sdmc:/3ds/platinum3ds/g3sci_pending"   /* exists while a calibration is in flight: a hang there never repeats */
int G3G_ScissorCfg(void) { if (scisCfg < 0) { scisCfg = 1;   /* always on since run 913's A/B: GPU 7.8-8.0 -> 4.1-6.1 ms/tick, in-between 44-61% -> 70-74%; g3_scissor is ignored */
      { FILE *f = fopen(SCIS_PENDING, "r"); if (f) { fclose(f); remove(SCIS_PENDING); scisMode = 2; PSPNativeMemLog("[G3SCI] the last session stopped during the calibration: scissor stays OFF this session"); } } PSPNativeMemLog("[G3SCI] hidden rows skip always on (the GPU skips the 64 rows that are never shown; stays off if the check fails)"); } return scisCfg; }
void G3G_SetScissor(int v) { scisCfg = v < 0 || v > 2 ? 0 : v; PSPNativeMemLog("[G3SCI] g3_scissor=%d from settings", scisCfg); }
static void clearQuad(void);
/* run 912: the first version waited for the GPU in the middle of G3G_End (FrameEnd + FrameBegin) and the boot hung on
   the first 3D frame. Now the test quad and its copy are only queued in front of the frame's own work, and the result
   is read after the next frame's FrameBegin (which has waited for this one anyway). scisMode 3 = result pending. */
static u32 *scisBuf; static int scisW;
static void scisCalibrate(int gpuR) {
    if (!qv) qv = linearAlloc(6 * sizeof(V));
    if (!scisBuf) { scisBuf = linearAlloc((size_t)curW * 256 * 4); scisW = curW; }
    if (!qv || !tgt[0] || !scisBuf) { scisMode = 2; PSPNativeMemLog("[G3SCI] calibration skipped (no buffers): scissor stays OFF"); return; }
    P3DS_Crumb("g3sci-queue", 0);
    { FILE *f = fopen(SCIS_PENDING, "w"); if (f) fclose(f); }
    const u32 keepClr = clearRGBA; const float keepD = clearD;
    C3D_RenderTargetClear(tgt[0], C3D_CLEAR_ALL, 0x000000FF, 0xFFFFFF);
    C3D_FrameDrawOn(tgt[0]);
    C3D_BindProgram(&prog); C3D_AttrInfo *ai = C3D_GetAttrInfo(); AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 4); AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2); AttrInfo_AddLoader(ai, 2, GPU_FLOAT, 4);
    C3D_SetScissor(GPU_SCISSOR_NORMAL, 0, 0, (u32)curW, 64);
    clearRGBA = 0xFFFFFFFF; clearD = 0.5f; clearQuad(); clearRGBA = keepClr; clearD = keepD;
    C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
    C3D_SyncDisplayTransfer((u32 *)tgt[0]->frameBuf.colorBuf, GX_BUFFER_DIM(curW, 256), scisBuf, GX_BUFFER_DIM(curW, 256), RB_FLAGS);   /* in frame: queued only */
    C3D_BindProgram(gpuR ? &progS : &prog); ai = C3D_GetAttrInfo(); AttrInfo_Init(ai);   /* same binding G3G_End made before the call */
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 4); AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2); AttrInfo_AddLoader(ai, 2, GPU_FLOAT, 4);
    if (gpuR) AttrInfo_AddLoader(ai, 3, GPU_FLOAT, 1);
    scisMode = 3;
}
/* called right after G3G_End's FrameBegin: the queued test copy is finished */
static void scisResult(void) {
    if (scisMode != 3) return;
    P3DS_DcInval(scisBuf, (u32)scisW * 256 * 4);
    /* scisBuf row r = memory row 255-r (flipped readback). Hidden memory rows 0..63 = rows 192..255. */
    const u32 lo = scisBuf[250 * scisW + scisW / 2], hi = scisBuf[5 * scisW + scisW / 2];
    const int loW = (lo >> 8) == 0xFFFFFF, hiW = (hi >> 8) == 0xFFFFFF;
    scisMode = loW && !hiW ? 0 : hiW && !loW ? 1 : 2;
    if (scisMode < 2 && scisW != curW) scisMode = 2;   /* target width changed in between: not trusted */
    linearFree(scisBuf); scisBuf = NULL; remove(SCIS_PENDING);
    PSPNativeMemLog("[G3SCI] calibration: 64-row scissor lands %s (mem row 5 %08lx, mem row 250 %08lx)%s", scisMode == 0 ? "on the hidden rows" : scisMode == 1 ? "on the shown end (flipped)" : "nowhere clear",
        (unsigned long)lo, (unsigned long)hi, scisMode == 2 ? ": scissor stays OFF" : "");
}
static void scisApply(int on) {
    if (!on) { C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0); return; }
    if (scisMode == 0) C3D_SetScissor(GPU_SCISSOR_NORMAL, 0, 64, (u32)curW, 256);
    else C3D_SetScissor(GPU_SCISSOR_NORMAL, 0, 0, (u32)curW, 192);
}
static void gtLog(void) {
    if (abFrames % 600) return;
    if (gtCfg != 1) return;
    static const char *nm[2] = { "scissor OFF", "scissor ON" };
    for (int s = 0; s < 2; s++) {
        if (!frN[s]) continue;
        const unsigned n = gtN[s] ? gtN[s] : 1;
        PSPNativeMemLog("[GPUT] %s: %u frames | GPU us (%u samples): mid L %u R %u copy %u | real L %u R %u copy %u | total %u | compose wait %u us (%u) | strip convert %u us (%u)",
            nm[s], frN[s], gtN[s], (unsigned)(gtSum[s][0] / n), (unsigned)(gtSum[s][1] / n), (unsigned)(gtSum[s][2] / n),
            (unsigned)(gtSum[s][3] / n), (unsigned)(gtSum[s][4] / n), (unsigned)(gtSum[s][5] / n),
            (unsigned)((gtSum[s][0] + gtSum[s][1] + gtSum[s][2] + gtSum[s][3] + gtSum[s][4] + gtSum[s][5]) / n),
            cwN[s] ? (unsigned)(cwSum[s] / cwN[s]) : 0, cwN[s], scN[s] ? (unsigned)(scSum[s] / scN[s]) : 0, scN[s]);
    }
    memset(gtSum, 0, sizeof gtSum); memset(cwSum, 0, sizeof cwSum); memset(scSum, 0, sizeof scSum);
    memset(gtN, 0, sizeof gtN); memset(cwN, 0, sizeof cwN); memset(scN, 0, sizeof scN); memset(frN, 0, sizeof frN);
}
/* 2026-11-17 state cache: G3G_End sets texture env, texture/wrap, blend and depth test only when they differ from the batch before
   (per eye and pass). citro3d re-emits every state that was set at the next draw. Run 910 A/B: End 1.52-1.62 ms -> 1.25-1.36 ms. */
/* why (logged in the stats line): 0 other, 1 flat stereo shift, 2 CPU right eye, 3 CPU composite, 4 GPU compose rejected,
   5 dump/diagnostic, 6 wide haze fit, 7 g3_shot */
void G3G_DirectResolveWhy(int why) {
    if (!dirPend || ok != 1) return;
    u64 t0 = svcGetSystemTick(); dirPend = 0; dirResolved++; dirResWhy[why & 7]++;
    if (!dirRb) {   /* wide direct frames already queued the readback in G3G_End */
        C3D_FrameBegin(0);
        for (int e = 0; e < dirEyes; e++) if (dirKey[e]) rbQueue(e);
        P3DS_FrameEnd(); dirOut = 1; }
    dirSync();
    for (int e = 0; e < dirEyes; e++) if (dirKey[e]) rbConvert(e, dirKey[e]);   /* 320 wide: also converts the strips */
    if (dirStrip) { for (int e = 0; e < dirEyes; e++) if (!dirKey[e]) { if (!slimFrame) P3DS_DcInval(rb[e], 320 * 256 * 4); stripConvert(e); } dirStrip = 0; }
    dirResUs += (unsigned long long)(svcGetSystemTick() - t0) / 268;
    { static unsigned n; if (n < 8 && !(dirResWhy[why & 7] > 1)) { n++; PSPNativeMemLog("[GPU3D] direct: 3D layer pulled back to the CPU (reason %d: %s)%s", why,
        (const char *[]){ "other", "flat stereo shift", "CPU right eye", "CPU composite", "GPU compose rejected", "dump/diagnostic", "wide haze fit", "g3_shot" }[why & 7], dirWide ? " | wide frame" : ""); } }
}
void G3G_DirectResolve(void) { G3G_DirectResolveWhy(0); }
/* wide direct: convert the side strips from the readback G3G_End queued (before gpuscale draws them) */
void G3G_DirectStrips(void) {
    if (!dirStrip || ok != 1) return;
    u64 t0 = svcGetSystemTick(); dirStrip = 0;
    dirSync();   /* normally a no-op: gpucomp's FrameBegin already waited for the draw, copy and readback */
    for (int e = 0; e < dirEyes; e++) { if (!slimFrame) P3DS_DcInval(rb[e], 320 * 256 * 4); stripConvert(e); }
    const unsigned us = (unsigned)((svcGetSystemTick() - t0) / 268);
    dirStripN++; dirStripUs += us; abStripUs(us);
}
/* A PICA tile row holds eight scanlines. Copy the first and last four
   tiles from each row, then use the normal display-transfer detile/Y flip. */
/* sync=1 is the original path: every copy submits the 3D command list first (C3D_FrameSplit inside C3D_Sync*).
   sync=0 queues the same GX copy onto the list that is already running. fps60_pipe uses that after one split. */
static void issueTexCopy(u32 *in, u32 indim, u32 *out, u32 outdim, u32 size, u32 flags, int sync) {
    if (sync) C3D_SyncTextureCopy(in, indim, out, outdim, size, flags);
    else GX_TextureCopy(in, indim, out, outdim, size, flags);
}
static void issueDisp(u32 *in, u32 indim, u32 *out, u32 outdim, u32 flags, int sync) {
    if (sync) C3D_SyncDisplayTransfer(in, indim, out, outdim, flags);
    else GX_DisplayTransfer(in, indim, out, outdim, flags);
}
static int stripsCfg = -1;
static int fps60Strips(void) {
    if (stripsCfg < 0) { extern int P3DS_CfgInt(const char *, int); stripsCfg = P3DS_CfgInt("fps60_strips", 0) != 0;
        PSPNativeMemLog("[FPS60] fps60_strips=%d (1 = in-between wide strips copy the visible 192 rows)", stripsCfg); }
    return stripsCfg;
}
void G3G_SetFps60Strips(int v) { stripsCfg = v != 0; PSPNativeMemLog("[FPS60] fps60_strips=%d from settings", stripsCfg); }
/* DS row y is GPU row 255-y, so the visible 192 rows are GPU rows 64..255 (tile rows 8..31).
   A flipped transfer of that span puts DS row 0 at the start, which is what the strip convert reads.
   OFF copies all 32 tile rows from the start of the buffer, as before. */
static void stripIssueTo(const void *color, u32 *tiles, u32 *out, int slim, int sync) {
    const unsigned row0 = slim ? 8u : 0u;
    const unsigned rows = slim ? 24u : (MID_TEXTURE_HEIGHT / 8);
    const unsigned height = rows * 8;
    const unsigned width = tiles ? MID_STRIP_WIDTH : 320;
    const u32 *source = color;
    if (tiles) {
        enum { SIDE_BYTES = 32 * 8 * sizeof(u32), WIDE_ROW = 320 * 8 * sizeof(u32) };
        const u8 *base = (const u8 *)color + row0 * WIDE_ROW;
        for (unsigned side = 0; side < 2; side++) {
            const unsigned from = side ? WIDE_ROW - SIDE_BYTES : 0;
            issueTexCopy((u32 *)(base + from),
                         GX_BUFFER_DIM(SIDE_BYTES / 16, (WIDE_ROW - SIDE_BYTES) / 16),
                         (u32 *)((u8 *)tiles + side * SIDE_BYTES),
                         GX_BUFFER_DIM(SIDE_BYTES / 16, SIDE_BYTES / 16),
                         SIDE_BYTES * rows, GX_TRANSFER_RAW_COPY(1), sync);
        }
        source = tiles;
    } else if (row0) {
        const unsigned rowBytes = (320 / 8) * (8 * 8 * 4);
        source = (const u32 *)((const u8 *)color + row0 * rowBytes);
    }
    issueDisp((u32 *)source, GX_BUFFER_DIM(width, height),
              out, GX_BUFFER_DIM(width, height), RB_FLAGS, sync);
}
static void midStripIssue(const void *color, int eye, int sync) { stripIssueTo(color, midStripTiles, rbM[eye], fps60Strips(), sync); }
static void midStripQueue(const void *color, int eye) { midStripIssue(color, eye, 1); }
/* Centre 256 columns of a 320-wide tiled target, into a 256x256 tiled texture. Same offsets as the old Sync call.
   (2026-11-17 run 920 A/B: skipping the 64 hidden rows saved nothing measurable, so the full copy stays.) */
static void centreCopy(const void *color, void *dest, int sync) {
    issueTexCopy((u32 *)((const u8 *)color + 4 * 256), GX_BUFFER_DIM(8192 >> 4, 2048 >> 4),
                 (u32 *)dest, GX_BUFFER_DIM(8192 >> 4, 0), 8192 * 32, GX_TRANSFER_RAW_COPY(1), sync);
}
/* How many C3D_Sync* calls the legacy wide path makes per eye: each one is its own 3D-list break.
   In-between: centre + (two side packs + readback, or one full readback). Real picture: centre + readback. */
static unsigned legacyWideKicks(int nEyes, int mid) {
    const unsigned midK = mid ? (midStripTiles ? 4 : 2) : 0;
    return nEyes * (midK + 2);
}
/* fps60: the in-between frame's side strips (64x192 like G3G_Strip) from the readback G3G_End queued. Call after a FrameBegin has waited. */
int G3G_MidStrip(int e, u32 *dst) {
    if (!midValid || !midWide || e < 0 || e >= midEyes || !dst) return 0;
    const unsigned width = midStripTiles ? MID_STRIP_WIDTH : 320;
    dirSync();
    P3DS_DcInval(rbM[e], width * 192 * sizeof(u32));
    stripConv(rbM[e], width, midStripTiles ? 32 : 288, dst);
    return 1;
}
static void dirLog(void) {
    static unsigned n; if (++n % 600 != 0) return;
    PSPNativeMemLog("[GPU3D] direct 600 3D frames: %u direct (%u of them reused without a draw, %u wide, avg End %u us), %u normal (avg End %u us), compositor sampled the target %u times, %u resolved to the CPU (avg %u us) | not direct because: off-path %u, fog-cpu %u, async/hi %u, compose!=2 %u, verify %u, wide_fx/wide-off/no-mem %u, wide-cpu-blend %u",
        dirDirect, dirReuse, dirWideN, dirDirect ? (unsigned)(dirUs / dirDirect) : 0, dirPlain, dirPlain ? (unsigned)(dirPlainUs / dirPlain) : 0, dirUsed,
        dirResolved, dirResolved ? (unsigned)(dirResUs / dirResolved) : 0, dirWhy[0], dirWhy[1], dirWhy[2], dirWhy[3], dirWhy[4], dirWhy[5], dirWhy[6]);
    PSPNativeMemLog("[GPU3D] direct resolves by reason: other %u, flat-stereo %u, cpu-right-eye %u, cpu-composite %u, compose-rejected %u, dump/diag %u, wide-haze %u, g3_shot %u | wide strips converted %u times (avg %u us)",
        dirResWhy[0], dirResWhy[1], dirResWhy[2], dirResWhy[3], dirResWhy[4], dirResWhy[5], dirResWhy[6], dirResWhy[7], dirStripN, dirStripN ? (unsigned)(dirStripUs / dirStripN) : 0);
    dirDirect = dirReuse = dirPlain = dirUsed = dirResolved = dirWideN = dirStripN = 0; dirUs = dirResUs = dirPlainUs = dirStripUs = 0; memset(dirWhy, 0, sizeof dirWhy); memset(dirResWhy, 0, sizeof dirResWhy);
}
/* wide direct: config gate + the two 256x256 centre textures (linear RAM: VRAM is kept for the eye targets) */
static int dirWideOk(void) {
    if (dirWideCfg < 0) { dirWideCfg = P3DS_CfgInt("gpu_direct3d_wide", 1) != 0; PSPNativeMemLog("[GPU3D] gpu_direct3d_wide=%d (1 = direct mode also with Wide view: centre copied on the GPU, strips from the readback)", dirWideCfg); }
    if (!dirWideCfg || dirTexFail) return 0;
    { extern int P3DS_WideFxCfg(void); if (P3DS_WideFxCfg()) return 0; }   /* Wide FX (slow, default OFF) reads the centre on the CPU every frame */
    if (dirTexOk) return 1;
    for (int e = 0; e < 2; e++) if (!C3D_TexInit(&dirTex[e], 256, 256, GPU_RGBA8)) {
        for (int k = 0; k < e; k++) C3D_TexDelete(&dirTex[k]); memset(dirTex, 0, sizeof dirTex); dirTexFail = 1;
        PSPNativeMemLog("[GPU3D] direct: wide centre textures FAILED (linear free %lu): wide frames stay on the normal path", (unsigned long)linearSpaceFree()); return 0; }
    for (int e = 0; e < 2; e++) { C3D_TexSetFilter(&dirTex[e], GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&dirTex[e], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); }
    dirTexOk = 1; PSPNativeMemLog("[GPU3D] direct: wide centre textures ready (2 x 256 KiB linear, linear free %lu)", (unsigned long)linearSpaceFree());
    return 1;
}
/* 2026-10-27 fog_hole (default 1, 3D tab "Cave light fix"): on the DS a pixel's fog flag is the AND of the fog bits of its
   opaque polygon and of every translucent polygon drawn over it (melonDS: translucent pixel with polygon fog off clears
   the flag). HGSS Flash weather (11/12, Dark Cave) fogs the whole cave black (FOG_COLOR 0, table all 127) and draws a
   translucent no-fog light sphere at the player (ov01_02203F98) to clear the flag there: the small lit circle. The PICA
   fog unit (fog3d=1) fogs each fragment as it is drawn, so the sphere could not un-fog the floor and the cave was fully
   black on hardware. Fog frames that hold a translucent no-fog batch now take the exact stencil path (fog3d=2: costs
   ~6-10 ms CPU in those frames only), where translucent no-fog batches write stencil 0 and translucent fog batches keep it. */
static int fogHoleV = -1; static unsigned fogHoleN;
int P3DS_FogHoleCfg(void) { if (fogHoleV < 0) { extern int P3DS_CfgInt(const char *, int); fogHoleV = P3DS_CfgInt("fog_hole", 1) != 0; PSPNativeMemLog("[GPU3D] fog_hole=%d (1 = translucent no-fog polygons clear the fog like the DS: Dark Cave / Flash light circle)", fogHoleV); } return fogHoleV; }
void P3DS_SetFogHole(int v) { fogHoleV = v != 0; PSPNativeMemLog("[GPU3D] fog_hole=%d from settings", fogHoleV); }
static int fogHoleFrame(void) { for (int e = 0; e < eyes; e++) for (unsigned i = 0; i < bn[e]; i++) if (bt[e][i].count && bt[e][i].blend && !bt[e][i].fog) return 1; return 0; }
static void r1xCalibrate(void) {
    const unsigned n = 2u * curW * 192; const u32 clr = clearRGBA; unsigned d0 = 0, d1 = 0;
    P3DS_DcInval(rb[1], n * 4); P3DS_DcInval(rbT, n * 4);
    for (unsigned i = 0; i < n; i += 7) { d0 += rb[1][i] != clr; d1 += rbT[i] != clr; }
    if (d0 < 64 && d1 < 64) return;   /* nothing drawn yet: try again on a later frame */
    r1xHalf = d1 > d0;
    if (r1xHalf) { u32 *t = rb[1]; rb[1] = rbT; rbT = t; }
    linearFree(rbT); rbT = NULL;
    PSPNativeMemLog("[3DRES] FAST 3D: right eye 1x lands in half %d of the %dx384 target (non-clear samples %u / %u)", r1xHalf, 2 * curW, d0, d1);
}
/* 2026-10-29 CRISP 3D (g3_sharp=4, Settings > 3D > Sharp 3D > CRISP, test; see docs/CRISP_3D_PLAN.md S1): the mono 2x target is also copied out
   UNSCALED (512x384, crispBuf) next to the usual box-downscaled 1x readback. gpuscale.c then draws it over the picture at the final screen size
   wherever the composited pixel is still the plain 3D pixel (nothing 2D on top). Needs Wide view off and the 3D slider at 0 for now. */
static u32 *crispBuf; static int crispPend, crispValid, crispFail; static const u32 *crispNF;
const u32 *G3G_CrispHi(const u32 **nf) {
    extern int P3DS_CfgInt(const char *, int);
    if (!crispValid || curHi != 1 || wideFrame || P3DS_CfgInt("g3_sharp", 0) != 4) return NULL;
    *nf = crispNF; return crispBuf; }
/* fps60_pipe (Settings > Display, default OFF): the wide in-between copies each used to end the 3D
   command list. Both eyes' in-between scenes are now drawn, then every in-between copy, then both
   real scenes, then every real copy. Two list breaks instead of one per copy. Pictures and copy
   rectangles are the same. OFF keeps the original per-copy breaks. */
static int pipeCfg = -1;
static int fps60Pipe(void) {
    if (pipeCfg < 0) { extern int P3DS_CfgInt(const char *, int); pipeCfg = P3DS_CfgInt("fps60_pipe", 0) != 0;
        PSPNativeMemLog("[FPS60] fps60_pipe=%d (1 = batch wide copies after each 3D pass)", pipeCfg); }
    return pipeCfg;
}
void G3G_SetFps60Pipe(int v) { pipeCfg = v != 0; PSPNativeMemLog("[FPS60] fps60_pipe=%d from settings", pipeCfg); }
static int holdCfg = -1;
static int fps60Hold(void) {
    if (holdCfg < 0) { extern int P3DS_CfgInt(const char *, int); holdCfg = P3DS_CfgInt("fps60_hold", 0) != 0;
        PSPNativeMemLog("[FPS60] fps60_hold=%d (1 = repeat the real frame when nothing moved)", holdCfg); }
    return holdCfg;
}
void G3G_SetFps60Hold(int v) { holdCfg = v != 0; PSPNativeMemLog("[FPS60] fps60_hold=%d from settings", holdCfg); }
extern int P3DS_FpsSceneMoved(void);
static int fpsVertsMoved(void) {
    static u32 prev; static int have;
    u32 h = 2166136261u ^ vn[0];
    if (vn[0] && vb[0]) {
        unsigned step = vn[0] > 32 ? vn[0] / 32 : 1;
        for (unsigned i = 0; i < vn[0]; i += step) {
            u32 b; memcpy(&b, &vb[0][i].x, 4); h = (h ^ b) * 16777619u;
            memcpy(&b, &vb[0][i].y, 4); h = (h ^ b) * 16777619u;
            memcpy(&b, &vb[0][i].s, 4); h = (h ^ b) * 16777619u;
            memcpy(&b, &vb[0][i].t, 4); h = (h ^ b) * 16777619u;
        }
    }
    int moved = !have || h != prev; have = 1; prev = h; return moved;
}
unsigned G3G_PipeKicks;   /* fps60_pipe kick counter; file scope so gpuscale.c can read it */
void G3G_End(u32 *outL, u32 *outR) {
    { extern void P3DS_Fps60Poll(void); P3DS_Fps60Poll(); }
    if (ok != 1) return;
    fps60Probe();
    u64 t0 = svcGetSystemTick();
    for (int e = 0; e < eyes; e++) P3DS_DcFlush(vb[e], vn[e] * sizeof(V));
    const int rs = rbSyncOn();
    { extern int P3DS_CfgInt(const char *, int);
      r1xFrame = curHi == 1 && sharedTgt && eyes == 2 && rs && !G3G_Async && !r1xFail && P3DS_CfgInt("g3_sharp", 0) >= 2 && P3DS_CfgInt("g3_sharp", 0) != 4;
      if (r1xFrame && r1xHalf < 0 && !rbT) { rbT = linearAlloc(2u * curW * 192 * 4);
          if (!rbT) { r1xFail = 1; r1xFrame = 0; PSPNativeMemLog("[3DRES] FAST 3D: no linear memory for the calibration buffer, both eyes stay 2x"); } } }
    fogFrame = 0;
    if (fogCfgOn() && (rs || fogCfg == 1) && (!G3G_Async || fogCfg == 1) && !depthLin && (fogCfg == 1 || (!curHi && !depth16)) && G3G_FogNoW && (*(const u16 *)(PSPNative_GfxRegisters + 0x60) & 0x80)) {
        int fm = fogCfg;
        /* 2026-11-17 run 919: rain ("town shower") turns on a grey fog and draws translucent no-fog polygons, so those frames took
           the exact CPU stencil fog (~16 ms per frame: both eyes read back and converted) and 60 fps fell to 30 for the whole
           shower. The light circle this path exists for is the Dark Cave darkness fog, which is black (FOG_COLOR 0); other fog
           stays on the free GPU fog unit (fog3d=1), as it was before the cave fix. */
        if (fm == 1 && (*(const u32 *)(PSPNative_GfxRegisters + 0x358) & 0x7FFF) != 0 && P3DS_FogHoleCfg() && fogHoleFrame()) {
            static unsigned nk; if (nk++ % 1800 == 0) PSPNativeMemLog("[GPU3D] fog_hole: coloured fog (rain/haze) with a translucent no-fog batch stays on the GPU fog unit (%u frames)", nk); }
        else if (fm == 1 && rs && !G3G_Async && !curHi && !depth16 && P3DS_FogHoleCfg() && fogHoleFrame()) { fm = 2;
            if (fogHoleN++ % 600 == 0) PSPNativeMemLog("[GPU3D] fog_hole: fog frame with a translucent no-fog batch (Dark Cave / Flash light) -> exact stencil fog (%u frames)", fogHoleN); }
        if (fm == 2 && !drb[0]) { for (int e = 0; e < 2; e++) drb[e] = linearAlloc(320 * 256 * 4);
            if (!drb[0] || !drb[1]) { for (int e = 0; e < 2; e++) { if (drb[e]) linearFree(drb[e]); drb[e] = NULL; }
                if (fogCfg == 2) { PSPNativeMemLog("[GPU3D] fog3d: no linear memory, fog off"); fogCfg = 0; fm = 0; }
                else { PSPNativeMemLog("[GPU3D] fog_hole: no linear memory (640 KiB), cave light fix off"); fogHoleV = 0; fm = 1; } } }
        /* 2026-11-05 (Nathan: Flash in a dark cave turned the whole screen black): Flash weather fogs the 3D black (FOG_COLOR 0,
           table all 127) while a 2D darkness layer with a lit hole covers the DS picture. After Flash (weather 12, FLASH_2) the game
           removes that layer and stops drawing the light sphere, but the fog registers stay the same, and here the fog then painted the
           whole cave black (on the DS the cave is lit). In that state the darkness fog is skipped. Before Flash nothing changes. */
        if (fm) { extern int SS3DS_FieldWeather(void); const unsigned char *R = PSPNative_GfxRegisters; int black = (*(const u32 *)(R + 0x358) & 0x1F7FFF) == 0;
            for (int i = 0; i < 32 && black; i++) if ((R[0x360 + i] & 127) != 127) black = 0;
            if (black && SS3DS_FieldWeather() == 12) { static unsigned nl; if (nl++ % 600 == 0) PSPNativeMemLog("[GPU3D] Flash used (weather FLASH_2): dark-cave fog skipped (%u frames)", nl); fm = 0; } }
        fogFrame = fm; if (fogFrame == 1) { fogLutSetup(); const u32 fc = *(const u32 *)(PSPNative_GfxRegisters + 0x358);
            C3D_FogColor(((fc & 31) * 255 / 31) | (((fc >> 5) & 31) * 255 / 31) << 8 | (((fc >> 10) & 31) * 255 / 31) << 16); }
        if (fogFrame && ++fogFrames % 600 == 1) PSPNativeMemLog("[GPU3D] fog3d: fog frame (DISP3DCNT %04x, FOG_COLOR %08lx, offset %04x)", *(const u16 *)(PSPNative_GfxRegisters + 0x60), (unsigned long)*(const u32 *)(PSPNative_GfxRegisters + 0x358), *(const u16 *)(PSPNative_GfxRegisters + 0x35C)); }
    int ruCand = 0, ruCheck = 0;
    if (ruVerify < 0) { ruVerify = 0 /* g3_reuse_verify: debug option removed */ != 0; PSPNativeMemLog("[G3REUSE] g3_reuse_verify=%d", ruVerify); }
    int direct = 0, wideCpuBlend = 0;
    if (G3G_DirectCfg()) {
        if (!rs || !outL) dirWhy[0]++;
        else if (fogFrame == 2) dirWhy[1]++;
        else if (G3G_Async || curHi != 0 || depthLin || (curW != 256 && !(curW == 320 && wideFrame))) dirWhy[2]++;
        else if (P3DS_GpuComposeCfg() != 2) dirWhy[3]++;
        else if (ruVerify) dirWhy[4]++;
        else if (wideFrame && !sharedTgt && P3DS_InField && fps60Pipe() && P3DS_Gc3DBlendRejects(PSPNative_GfxRegisters)) {
            /* The CPU compositor needs the linear readback, never the centre texture. */
            wideCpuBlend = 1;
            dirWhy[6]++;
            static int said;
            if (!said) {
                said = 1;
                PSPNativeMemLog("[GPU3D] wide CPU blend: skip centre texture copies; queue the eye readbacks together");
            }
        }
        else if (wideFrame && !dirWideOk()) dirWhy[5]++;
        else direct = 1;
    }
    if (!direct) { if (dirValid) dirSync(); dirValid = dirPend = 0; }
    dirWide = dirStrip = dirRb = slimFrame = 0;
    abFrames++; abLast = -1;
    int mid = 0;
    if (G3G_MidRec) {
        const int holdOn = fps60Hold();
        const int still = P3DS_FpsHoldDecide(holdOn, midTot, midMat, midMov, holdOn ? P3DS_FpsSceneMoved() : 0, holdOn ? fpsVertsMoved() : 0);
        if (still && direct) { fpsHoldFlag = 1; static unsigned n; if (++n == 1 || n % 600 == 0) PSPNativeMemLog("[FPS60] hold real frame x%u (camera, sprites, tiles and 3D all still)", n); }
        else if (!direct) G3G_MidWhy[0]++; else if (midOver) G3G_MidWhy[1]++; else if (fogFrame == 2) G3G_MidWhy[2]++;
        else if (!midTot || !midMov) G3G_MidWhy[3]++; else if (midMat * 4 < midTot * 3) G3G_MidWhy[4]++; else mid = 1;
    }
    /* g3_stereo_gpu: prefer shader right eye whenever left-only pending. Skip mid/K (incompatible with
       g3s) so walking does not flip-flop mid↔gpuRight. Hard fallbacks keep CPU expand; reasons in 60f log. */
    int gpuRight = 0;
    int sterWhy = -1; /* -1 = took gpuRight or no want; else STER_WHY_* */
    const int pend0 = __atomic_load_n(&stereoPending, __ATOMIC_ACQUIRE);
    if (stereoGpuWant) {
        if (eyes != 2) sterWhy = STER_WHY_EYES;
        else if (!stereoDsh || !vn[0] || !bn[0]) sterWhy = STER_WHY_NOGEOM;
        else if (vn[1] != 0 || bn[1] != 0) sterWhy = STER_WHY_EYE1; /* dual TriClip2 overflow / legacy expand */
        else if (!pend0) sterWhy = STER_WHY_NOPEND;
        else {
            if (stereoGpuEnsure()) {
                gpuRight = 1;
                memcpy(stereoDshLin, stereoDsh, vn[0] * sizeof(float));
                P3DS_DcFlush(stereoDshLin, vn[0] * sizeof(float));
                __atomic_store_n(&stereoPending, 0, __ATOMIC_RELEASE);
                stereoGpuRightN++;
                sterWhy = -1;
            } else sterWhy = STER_WHY_NOSHAD;
        }
        if (!gpuRight) {
            if (__atomic_load_n(&stereoPending, __ATOMIC_ACQUIRE)) { G3G_StereoExpand(); P3DS_DcFlush(vb[1], vn[1] * sizeof(V)); } /* expand writes vb[1] after the main flush above */
            /* Count real fallbacks only (skip empty/no-geom frames that never had share work). */
            if (sterWhy == STER_WHY_NOSHAD || sterWhy == STER_WHY_EYE1 ||
                (sterWhy == STER_WHY_NOPEND && (vn[0] || vn[1])) ||
                sterWhy == STER_WHY_EYES) {
                stereoGpuExpandN++;
                if (sterWhy >= 0 && sterWhy < STER_WHY_N) stereoGpuWhy[sterWhy]++;
            }
        }
    } else if (pend0) {
        G3G_StereoExpand(); P3DS_DcFlush(vb[1], vn[1] * sizeof(V));
    }
    if (rs && !G3G_Async && curW == 256 && curHi == 0 && fogFrame != 2 && P3DS_G3ReuseCfg() && (outL || outR)) {
        const unsigned tot = vn[0] + vn[1];
        if (tot == 0 && fogFrame == 0 && !ruVerify && !direct) {
            const u32 w = convWord(clearRGBA);
            for (int e = 0; e < eyes; e++) { u32 *o = e ? outR : outL; if (o) for (unsigned i = 0; i < 256 * 192; i++) o[i] = w; }
            G3G_StripValid = 0; ruFreeDeferred(); ruEmpty++; ruSaveUs += ruDraw ? (unsigned)(ruDrawUs / ruDraw) : 18000; lastUs = (unsigned)((svcGetSystemTick() - t0) / 268); ruLog(); return;
        }
        if (tot <= RU_MAXV && bn[0] <= RU_MAXB && bn[1] <= RU_MAXB) {
            RK k; memset(&k, 0, sizeof k);
            k.clr = clearRGBA; k.depth = clearD; k.eyes = eyes; k.dir = direct; k.haveL = outL != NULL; k.haveR = outR != NULL; k.fog = fogFrame; k.tg = texGen;
            k.dc = *(const u16 *)(PSPNative_GfxRegisters + 0x60); k.f350 = *(const u16 *)(PSPNative_GfxRegisters + 0x350);
            k.f35c = *(const u16 *)(PSPNative_GfxRegisters + 0x35C); k.fc = *(const u32 *)(PSPNative_GfxRegisters + 0x358);
            memcpy(k.tab, PSPNative_GfxRegisters + 0x360, 32); k.vn[0] = vn[0]; k.vn[1] = vn[1]; k.bn[0] = bn[0]; k.bn[1] = bn[1];
            ruCand = 1;
            if (ruValid && !memcmp(&k, &ruKey, sizeof k)) {
                int same = 1; for (int e = 0; e < eyes && same; e++) same = !memcmp(vb[e], ruV[e], vn[e] * sizeof(V)) && !memcmp(bt[e], ruB[e], bn[e] * sizeof(B));
                if (same) {
                    if (!ruVerify && (!direct || ruRt)) {
                        u64 tc = svcGetSystemTick();
                        if (direct) { /* rt[e] still holds exactly this frame (nothing else draws into it) */
                            dirValid = 1; dirPend = 1; dirEyes = eyes; dirKey[0] = outL; dirKey[1] = outR; dirReuse++; dirDirect++; }
                        else
                        for (int e = 0; e < eyes; e++) { u32 *o = e ? outR : outL; if (o) memcpy(o, ruOut[e], 256 * 192 * 4); }
                        G3G_StripValid = 0; ruFreeDeferred(); ruHit++; ruSaveUs += ruDraw ? (unsigned)(ruDrawUs / ruDraw) : 18000;
                        ruHitUs += (unsigned)((svcGetSystemTick() - tc) / 268);
                        { static unsigned n; if (n++ < 3) PSPNativeMemLog("[G3REUSE] frame reused (tris %u, batches %u, tex gen %u)", tot / 3, bn[0], texGen); }
                        lastUs = (unsigned)((svcGetSystemTick() - t0) / 268); ruLog(); return;
                    }
                    ruCheck = ruVerify;
                }
            }
            ruKey = k;
        }
        else ruBig++;
    }
    P3DS_Crumb("g3e-begin", 0); /* run 65: locate the boot g3 stall */
    ruRt = 0;
    C3D_FrameBegin(0);
    scisResult();
    P3DS_Crumb("g3e-draw", 0);
    if (!G3G_Async && oldN) { for (unsigned i = 0; i < oldN; i++) { C3D_TexDelete(oldFree[i]); free(oldFree[i]); } oldN = 0; }   /* gpu_direct3d: textures held back from the previous frame; the FrameBegin above waited for it */
    if (G3G_Async) { /* previous frame's GPU work is finished (FrameBegin waited): read it back, then free its textures */
        if (pendRB) for (int e = 0; e < pendEyes; e++) { if (rs) rbConvert(e, e ? outR : outL); else readback(e, e ? outR : outL); }
        for (unsigned i = 0; i < oldN; i++) { C3D_TexDelete(oldFree[i]); free(oldFree[i]); }
        oldN = 0;
    }
    C3D_BindProgram(gpuRight ? &progS : &prog);
    C3D_AttrInfo *ai = C3D_GetAttrInfo(); AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 4); AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2); AttrInfo_AddLoader(ai, 2, GPU_FLOAT, 4);
    if (gpuRight) AttrInfo_AddLoader(ai, 3, GPU_FLOAT, 1); /* dsh from stereoDsh — only when stereo_gpu ON */
    C3D_CullFace(GPU_CULL_NONE); C3D_DepthMap(G3G_WDepthOn() != 2, -1.0f, 0.0f);
    C3D_AlphaTest(true, GPU_GREATER, 3);   /* DS: texel alpha 0 / final 5-bit alpha 0 are not drawn */
    for (int k = 1; k < 6; k++) C3D_TexEnvInit(C3D_GetTexEnv(k));
    u32 fogClr = clearRGBA;
    if (fogFrame == 1 && (*(const u16 *)(PSPNative_GfxRegisters + 0x350) & 0x8000)) { const u32 fc = *(const u32 *)(PSPNative_GfxRegisters + 0x358);
        const int f = (int)fogDens((u32)(clearD * 0xFFFFFF)); const int fc3[3] = { (int)((fc & 31) * 255 / 31), (int)(((fc >> 5) & 31) * 255 / 31), (int)(((fc >> 10) & 31) * 255 / 31) };
        fogClr = clearRGBA & 255; for (int c = 0; c < 3; c++) { int v = (clearRGBA >> (24 - 8 * c)) & 255; v = (fc3[c] * f + v * (128 - f)) >> 7; fogClr |= (u32)v << (24 - 8 * c); } }
    int decalOn = 0;
    /* Separate wide targets can queue the copies together after either pass. */
    const int pipe = wideFrame && direct && curHi == 0 && !depthLin && !fogFrame && !sharedTgt && !r1xFrame && !G3G_Async && fps60Pipe();
    static unsigned pipeN, pipeKickSum, legacyKickSum;
    /* real wide strips: copy only the side strips (see stripConvert) */
    slimFrame = direct && wideFrame && curHi == 0 && !r1xFrame && !sharedTgt && slimAlloc();   /* falls back to the full readback without memory */
    const int gtS = G3G_GpuTimeCfg() && direct && ++gtCnt % 30 == 0;
    const int scOn = 1;   /* state cache: always on since run 910's A/B (End -0.15..0.37 ms, same picture) */
    const int sciC = G3G_ScissorCfg();
    static unsigned sciWait;
    if (sciC && scisMode < 0 && direct && curHi == 0 && !r1xFrame && !sharedTgt && ++sciWait > 300) scisCalibrate(gpuRight);   /* not during boot */   /* run 911: !gpuRight kept it from ever running (stereo GPU right eye is on every frame) */
    const int scisOn = (sciC == 1 || (sciC == 2 && ((abFrames / 600) & 1))) && (scisMode == 0 || scisMode == 1) && direct && curHi == 0 && !r1xFrame && !sharedTgt;
    int gtFirst = 1; unsigned gtCur[6] = { 0 };
    const float stereoScale = wideFrame ? 0.8f : 1.0f;
    for (int ph = mid ? 0 : 1; ph < 2; ph++) {   /* fps60: ph 0 = the in-between frame (same vertices through the K shader), copied out before ph 1 draws the real one over it */
    for (int e = 0; e < eyes; e++) {
        if (gpuRight) {
            C3D_BindProgram(&progS);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, uStereoK, e ? (stereoScale / 128.0f) : 0.0f, 0, 0, 0);
            for (int i = 0; i < 4; i++) C3D_FVUnifSet(GPU_VERTEX_SHADER, uKS + i, kIdent[i], kIdent[4 + i], kIdent[8 + i], kIdent[12 + i]); /* real frame: identity (also resets the last in-between K) */
        } else if (mid && e == 0) C3D_BindProgram(ph ? &prog : &progK);
        if (!ph && !gpuRight) for (int i = 0; i < 4; i++) C3D_FVUnifSet(GPU_VERTEX_SHADER, uK + i, kIdent[i], kIdent[4 + i], kIdent[8 + i], kIdent[12 + i]);
        if (!depthLin) C3D_RenderTargetClear(tgt[e], C3D_CLEAR_ALL, fogFrame == 1 ? fogClr : clearRGBA, depth16 ? (u32)(clearD * 0xFFFF) : ((u32)(clearD * 0xFFFFFF) | (fogFrame == 2 && (*(const u16 *)(PSPNative_GfxRegisters + 0x350) & 0x8000) ? 0x01000000u : 0)));
        C3D_FrameDrawOn(tgt[e]);   /* viewport = whole target (256/320 x 256, or 2x x 384) */
        const int small = r1xFrame && e == 1;   /* FAST 3D right eye: 1x viewport in the corner of the 2x target */
        if (small) C3D_SetViewport(0, 0, curW, 192);
        if (scisOn) scisApply(1);
        const int hx = curHi == 1 && !small;
        if (depthLin) clearQuad();
        C3D_BufInfo *bi = C3D_GetBufInfo(); BufInfo_Init(bi);
        if (gpuRight) {
            BufInfo_Add(bi, vb[0], sizeof(V), 3, 0x210);
            BufInfo_Add(bi, stereoDshLin, sizeof(float), 1, 0x3);
        } else BufInfo_Add(bi, vb[e], sizeof(V), 3, 0x210);
        const unsigned drawBn = gpuRight ? bn[0] : bn[e];
        B *drawBt = gpuRight ? bt[0] : bt[e];
        int kSet = -1;
        /* g3_statecache: last texture / wrap / texenv / blend / depth set in this eye's pass; -1 = unknown (always set) */
        C3D_Tex *scTex = NULL; int scWrap = -1, scEnv = -1, scBlend = -1, scDepth = -1;
        for (int pass = 0; pass < 2; pass++) for (unsigned i = 0; i < drawBn; i++) {
            B *b = &drawBt[i]; if (!b->count || b->late != pass) continue;
            const int envK = b->tex ? 1 : 0;
            if (!scOn || envK != scEnv) { scEnv = envK;
                C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env);
                if (b->tex) { C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_TEXTURE0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE); }
                else { C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE); } }
            if (b->tex) { const int wk = (b->wrapS << 16) ^ b->wrapT;
                if (!scOn || b->tex != scTex || wk != scWrap) { scTex = b->tex; scWrap = wk; C3D_TexSetWrap(b->tex, b->wrapS, b->wrapT); C3D_TexBind(0, b->tex); } }
            const int blK = b->blend ? 1 : 0;
            if (!scOn || blK != scBlend) { scBlend = blK;
                if (b->blend) C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_MAX, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ONE);
                else C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO); }
            if (fogFrame == 1) C3D_FogGasMode(b->fog ? GPU_FOG : GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
            if (fogFrame == 2) { C3D_StencilTest(true, GPU_ALWAYS, b->fog ? 1 : 0, 0xFF, b->blend && b->fog ? 0x00 : 0xFF); C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE); }
            /* SHARP 3D (run 51 stripes): at 2x a depth-EQUAL overlay (grass/decal over the ground) misses the ground's interpolated depth on
               alternate rows. Draw those as LEQUAL pulled a few hundred depth steps nearer; everything else keeps the plain depth map. */
            { const int dec = hx && b->dfunc == GPU_EQUAL;
              if (dec != decalOn) { decalOn = dec; C3D_DepthMap(G3G_WDepthOn() != 2, -1.0f, dec ? -3e-5f : 0.0f); } }
            { const int df = (hx && b->dfunc == GPU_EQUAL) ? GPU_LEQUAL : b->dfunc, dk = (df << 1) | (b->dwrite ? 1 : 0);
              if (!scOn || dk != scDepth) { scDepth = dk; C3D_DepthTest(true, df, b->dwrite ? GPU_WRITE_ALL : GPU_WRITE_COLOR); } }
            if (ph) { C3D_DrawArrays(GPU_TRIANGLES, b->first, b->count); continue; }
            /* in-between picture: the batch in pieces, one per run of vertices that share K */
            unsigned a = b->first; const unsigned end = b->first + b->count;
            const int ge = gpuRight ? 0 : e; /* gpuRight draws vb[0] for both eyes, so the runs recorded on eye 0 apply */
            unsigned lo = 0, hi = krN - 1; while (lo < hi) { const unsigned m = (lo + hi + 1) / 2; if (kr[m].v0[ge] <= a) lo = m; else hi = m - 1; }
            for (unsigned r = lo; a < end; r++) {
                while (r + 1 < krN && kr[r + 1].v0[ge] <= a) r++;
                const unsigned stop = (r + 1 < krN && kr[r + 1].v0[ge] < end) ? kr[r + 1].v0[ge] : end;
                if ((int)r != kSet) { kSet = (int)r; const float *k = kr[r].k; const int uk = gpuRight ? uKS : uK; for (int i = 0; i < 4; i++) C3D_FVUnifSet(GPU_VERTEX_SHADER, uk + i, k[i], k[4 + i], k[8 + i], k[12 + i]); }
                C3D_DrawArrays(GPU_TRIANGLES, a, stop - a); a = stop; }
        }
        if (fogFrame == 1) C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
        if (!ph) {   /* in-between frame: out of the target before the real frame clears it (same copies as the direct path below) */
            if (!pipe) {
            if (wideFrame) { centreCopy(tgt[e]->frameBuf.colorBuf, midTex[e].data, 1);
                midStripQueue(tgt[e]->frameBuf.colorBuf, e); }
            else C3D_SyncTextureCopy((u32 *)rt[e].data, GX_BUFFER_DIM(8192 >> 4, 0), (u32 *)midTex[e].data, GX_BUFFER_DIM(8192 >> 4, 0), 256 * 256 * 4, GX_TRANSFER_RAW_COPY(1));
            }
            if (gtS) gtMark(gtCur, e ? 1 : 0, &gtFirst);
            continue; }
        if (fogFrame == 2) { C3D_StencilTest(false, GPU_ALWAYS, 0, 0xFF, 0xFF);
            C3D_SyncDisplayTransfer((u32 *)tgt[e]->frameBuf.depthBuf, GX_BUFFER_DIM(curW, 256), drb[e], GX_BUFFER_DIM(curW, 256), RB_FLAGS); }
        if (rs && sharedTgt) rbQueue(e);   /* shared 2x target: copy this eye out before the next eye clears it */
        if (gtS) gtMark(gtCur, e ? 4 : 3, &gtFirst);
    }
    if (pipe && ph == 0) {   /* both in-between scenes are drawn; one list break, then every copy */
        C3D_FrameSplit(0); pipeKickSum++; G3G_PipeKicks++;
        for (int e = 0; e < eyes; e++) { centreCopy(tgt[e]->frameBuf.colorBuf, midTex[e].data, 0); midStripIssue(tgt[e]->frameBuf.colorBuf, e, 0); }
    }
    if (gtS && ph == 0) gtMark(gtCur, 2, &gtFirst);
    }
    if (wideCpuBlend) {
        C3D_FrameSplit(0);
        /* After the Y flip the CPU reads only the first 192 rows. Start on
           the same visible tile row as the in-between strips transfer. */
        const unsigned rows = fps60Strips() ? 192 : 256;
        const u32 dims = GX_BUFFER_DIM(curW, rows);
        for (int e = 0; e < eyes; e++) {
            u32 *source = (u32 *)tgt[e]->frameBuf.colorBuf + (256 - rows) * curW;
            issueDisp(source, dims, rb[e], dims, RB_FLAGS, 0);
        }
        if (rows == 192) {
            static int said;
            if (!said) {
                said = 1;
                PSPNativeMemLog("[GPU3D] wide CPU blend readback: 320x192 visible rows");
            }
        }
    } else if (rs && !sharedTgt && !direct) {
        for (int e = 0; e < eyes; e++) rbQueue(e);
    }
    if (direct && wideFrame && pipe) {   /* both real scenes are drawn; one list break, then every wide copy */
        C3D_FrameSplit(0); pipeKickSum++; G3G_PipeKicks++;
        for (int e = 0; e < eyes; e++) {
            centreCopy(tgt[e]->frameBuf.colorBuf, dirTex[e].data, 0);
            if (slimFrame) stripIssueTo(tgt[e]->frameBuf.colorBuf, rbSTiles, rbS[e], 1, 0);   /* only the side strips */
            else issueDisp((u32 *)tgt[e]->frameBuf.colorBuf, GX_BUFFER_DIM(curW, 256), rb[e], GX_BUFFER_DIM(curW, 256), RB_FLAGS, 0);
        }
        const unsigned legacyKicks = legacyWideKicks(eyes, mid);
        legacyKickSum += legacyKicks;
        if (!mid) {
            static int said;
            if (!said) { said = 1; PSPNativeMemLog("[FPS60] wide real-only copy batch: 1 list break (legacy %u, %d eyes)", legacyKicks, eyes); }
        }
        if (++pipeN >= 600) { PSPNativeMemLog("[FPS60] pipe kicks %u per frame over %u frames (legacy %u) tris %u batches %u",
            pipeKickSum / pipeN, pipeN, legacyKickSum / pipeN, vn[0] / 3, bn[0]); pipeN = pipeKickSum = legacyKickSum = 0; }
    } else if (direct && wideFrame) for (int e = 0; e < eyes; e++) {   /* wide direct: centre 256 columns -> dirTex[e] (texture copy, tiled to tiled), then the strip readback */
        centreCopy(tgt[e]->frameBuf.colorBuf, dirTex[e].data, 1);
        if (slimFrame) stripIssueTo(tgt[e]->frameBuf.colorBuf, rbSTiles, rbS[e], 1, 1);
        else rbQueue(e); }
    if (scisOn) scisApply(0);   /* never leave a scissor for the compositor */
    crispPend = 0;
    { extern int P3DS_CfgInt(const char *, int);
      if (P3DS_CfgInt("g3_sharp", 0) == 4 && curHi == 1 && eyes == 1 && !wideFrame && rs && !direct && !G3G_Async && !crispFail) {
          if (!crispBuf) { crispBuf = linearAlloc(512 * 384 * 4); if (!crispBuf) { crispFail = 1; PSPNativeMemLog("[CRISP] no linear memory for the 512x384 copy, CRISP 3D stays off"); } }
          if (crispBuf) { C3D_SyncDisplayTransfer((u32 *)tgt[0]->frameBuf.colorBuf, GX_BUFFER_DIM(2 * curW, 384), crispBuf, GX_BUFFER_DIM(2 * curW, 384), RB_FLAGS); crispPend = 1; } }
      else crispValid = 0; }
    P3DS_Crumb("g3e-end", 0);
    const u64 gtA = svcGetSystemTick();
    if (gtS && !gtFirst) C3D_FrameEnd(GX_CMDLIST_FLUSH); else P3DS_FrameEnd();
    if (gtS) {   /* sampled frame: wait for the last pass too, then file the six numbers */
        C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH);
        gtCur[5] = (unsigned)((svcGetSystemTick() - gtA) / 268);
        const int st = scisOn;
        for (int i = 0; i < 6; i++) gtSum[st][i] += gtCur[i];
        gtN[st]++; gtSkipCw = 1;
    }   /* fps_tweaks >= 1: no whole-linear-heap flush (vb[] and textures are flushed by their writers) */
    P3DS_Crumb("g3e-wait", 0);
    if (rs && !G3G_Async && !direct) { C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH); }
    P3DS_Crumb("g3e-done", 0); /* exact wait: queue (draw + readback DMA) done */
    if (G3G_Async) { pendRB = 1; pendEyes = eyes; memcpy(oldFree, pendFree, pendN * sizeof *pendFree); oldN = pendN; pendN = 0; }
    else if (direct) {   /* no wait, no readback: the compositor samples rt[e]. Textures this frame used stay alive until the next frame's FrameBegin has waited */
        pendRB = 0; for (unsigned i = 0; i < pendN && oldN < 512; i++) oldFree[oldN++] = pendFree[i]; pendN = 0;
        dirValid = dirPend = 1; dirEyes = eyes; dirKey[0] = outL; dirKey[1] = outR; dirOut = 1; dirDirect++;
        dirWide = dirStrip = wideFrame; dirRb = wideFrame && !slimFrame; if (wideFrame) dirWideN++;   /* slim strips: no full readback queued, resolve queues its own */
        abLast = scisOn; frN[abLast]++;
        if (mid) { midValid = 1; midEyes = eyes; midWide = wideFrame; }
        G3G_StripValid = wideFrame ? eyes : 0;   /* wide: G3G_DirectStrips() fills G3G_Strip before present */
        { static int said; if (wideFrame && !said) { said = 1; PSPNativeMemLog("[GPU3D] direct: first wide frame (320x256 targets, %d eye%s): centre copied on the GPU into the compositor texture, only the side strips converted on the CPU", eyes, eyes > 1 ? "s" : ""); } }
    }
    else {
        pendRB = 0; for (unsigned i = 0; i < oldN; i++) { C3D_TexDelete(oldFree[i]); free(oldFree[i]); } oldN = 0;
        if (r1xFrame && r1xHalf < 0 && rbT) r1xCalibrate();
        if (crispPend) { crispPend = 0; crispValid = 1; crispNF = outL; }
        { u64 tc = svcGetSystemTick(); for (int e = 0; e < eyes; e++) { if (rs) rbConvert(e, e ? outR : outL); else readback(e, e ? outR : outL); } G3G_ConvUs = (unsigned)((svcGetSystemTick() - tc) / 268); }
        G3G_StripValid = wideFrame ? eyes : 0;
        for (unsigned i = 0; i < pendN; i++) { C3D_TexDelete(pendFree[i]); free(pendFree[i]); }
        pendN = 0;
    }
    if (ruCand) {
        if (!ruV[0]) for (int e = 0; e < 2; e++) { ruV[e] = malloc(RU_MAXV * sizeof(V)); ruB[e] = malloc(RU_MAXB * sizeof(B)); ruOut[e] = malloc(256 * 192 * 4); }
        if (ruV[0] && ruV[1] && ruB[0] && ruB[1] && ruOut[0] && ruOut[1]) {
            if (ruCheck) { for (int e = 0; e < eyes; e++) { const u32 *o = e ? outR : outL; if (o && memcmp(o, ruOut[e], 256 * 192 * 4)) { ruBad++;
                static unsigned nb; if (nb++ < 10) PSPNativeMemLog("[G3REUSE] VERIFY MISMATCH eye %d (tris %u): reused frame differs from a fresh render", e, vn[e] / 3); } } }
            for (int e = 0; e < eyes; e++) { memcpy(ruV[e], vb[e], vn[e] * sizeof(V)); memcpy(ruB[e], bt[e], bn[e] * sizeof(B));
                const u32 *o = e ? outR : outL; if (o && !direct) memcpy(ruOut[e], o, 256 * 192 * 4); }
            ruRt = direct; ruValid = 1; ruDraw++; ruDrawUs += (unsigned)((svcGetSystemTick() - t0) / 268);
        } else ruValid = 0;
    }
    lastUs = (unsigned)((svcGetSystemTick() - t0) / 268);
    if (curHi == 1 && eyes == 2 && !direct) {   /* [PERF] stereo 2x cost, per mode: End = both eye draws + wait + readback convert */
        static unsigned n[2], us[2], cv[2]; const int m = r1xFrame ? 1 : 0; n[m]++; us[m] += lastUs; cv[m] += G3G_ConvUs;
        if (n[m] >= 300) { PSPNativeMemLog("[PERF] sharp 3D stereo, %s: %u frames, G3G_End avg %u us (draw+wait+convert), of which CPU convert avg %u us", m ? "FAST 3D (right eye 1x)" : "both eyes 2x", n[m], us[m] / n[m], cv[m] / n[m]); n[m] = us[m] = cv[m] = 0; } }
    if (G3G_DirectCfg()) { if (direct) dirUs += lastUs; else { dirPlain++; dirPlainUs += lastUs; } dirLog(); }
    gtLog();
    { static unsigned f; if (++f % 120 == 0)
 { unsigned late = 0, tx = 0; for (unsigned i = 0; i < bn[0]; i++) { if (bt[0][i].late) late += bt[0][i].count / 3; if (bt[0][i].tex) tx += bt[0][i].count / 3; }
        PSPNativeMemLog("[GPU3D] frame: eyes %d | tris %u (textured %u, translucent %u) | batches %u | gpu %u us", eyes, vn[0] / 3, tx, late, bn[0], lastUs); } }
}
/* run 112: VRAM was freed (gpucomp wide target): the wide 320 targets may fit again */
void G3G_WideRetry(void) { if (wideFail) PSPNativeMemLog("[3DRES] retrying the 320 targets"); wideFail = 0; }

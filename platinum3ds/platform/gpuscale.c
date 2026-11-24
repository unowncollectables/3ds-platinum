#include <stdio.h>
extern void PSPNativeMemLog(const char *, ...);
/* platinum3ds GPU screen scaling (citro3d): each DS screen is nearest-upscaled 2x on the CPU into a 512x384
 * (v2: no CPU prescale) 256x192 RGBA8 into a 256x256 texture via GX display transfer, drawn as one
 * NEAREST-filtered quad scaled by the GPU (crisp).
 * Modes: 1 = bottom 320x240, top 320x240 pillarboxed; 2 = bottom 320x240, top stretched 400x240. */
#include <3ds.h>
#include <citro3d.h>
#include "dcache3ds.h"
#include <string.h>
#include "blit_shbin.h"
#define TEXW 256
#define TEXH 256
static int ready;
static DVLB_s *dvlb; static shaderProgram_s prog; static int uProj;
static C3D_RenderTarget *tgtTop, *tgtBot, *tgtTopR; static int stereoOn;
static C3D_Tex texS[2][3]; static u32 *linS[2][3]; /* 0 top(left eye) 1 bottom 2 top right eye; [set] */
/* run 37: scalesrc_dbl=1 (default): ping-pong two source-texture/staging sets per screen, so a frame never writes the
   set the previous (possibly still queued) present samples. citro3d already serialises this (FrameBegin waits for the
   whole previous queue; the in-frame transfer is queued before the draw), so this is belt-and-braces; 0 = single set. */
static int srcSet, srcDbl = -1;
#define tex texS[srcSet]
#define lin linS[srcSet]
/* sharp bilinear: pass 1 nearest 2x into up[i] (512x512 render-target texture, 512x384 used), pass 2 linear down */
static C3D_Tex up[3]; static C3D_RenderTarget *upTgt[3];
static int upFmt16 = -1, botValid;   /* upFmt16: up[] format, 0 RGBA8, 1 RGB565 (sharp 3D), -1 not allocated; botValid: see run 40 below */
#define DISP_FLAGS (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

#define TEX_FLAGS (GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
/* 2026-10-27 fps thread: fps_tweaks (Debug tab "Speed tweaks", default 0 = OFF), cumulative: 2 = engine B skipped while the
   Bottom menu covers the bottom screen and both blink states of the Bottom menu cached (render3ds.cpp), 3 = + master-brightness
   fades composed on the GPU (gpucomp.c). Level 1 (lean GPU flush) HUNG the game at frame 4 on hardware (compose FrameBegin never
   returned, 2026-10-27): it now lives only behind the config.txt key fps_leanflush=1 (default 0, not in the menu) and level 1 is
   treated as 0. [PERF4] times every stage with the tweaks on or off.
   Boot guard: a non-zero level writes fps_tweaks.pending before it is used and deletes it after 600 presented frames; a boot that
   finds the file (the last boot with the tweaks never got that far: hang, crash, power-off) forces fps_tweaks=0 in config.txt and
   says so in the log, so a bad level can never lock the game out. */
#define FPS_PENDING "sdmc:/3ds/platinum3ds/fps_tweaks.pending"
static int fpsTwV = -1, fpsLean = -1, fpsOkFrames; static unsigned fpsGuardMsg;
extern int P3DS_CfgInt(const char *, int); extern void P3DS_CfgWrite(const char *, int);
static void fpsArm(void) { FILE *f = fopen(FPS_PENDING, "w"); if (f) { fputs("fps_tweaks was switched on; removed after 600 good frames\n", f); fclose(f); } fpsOkFrames = 0; }
int P3DS_FpsTweaks(void) {
    if (fpsTwV < 0) {
        fpsTwV = P3DS_CfgInt("fps_tweaks", 0); if (fpsTwV < 2 || fpsTwV > 7) fpsTwV = 0;
        FILE *g = fopen(FPS_PENDING, "r");
        if (g) { fclose(g); remove(FPS_PENDING);
            if (fpsTwV) { fpsTwV = 0; fpsGuardMsg = 1; PSPNativeMemLog("[PERF4] SAFE BOOT: the last boot with Speed tweaks on never reached 600 frames; tweaks are OFF for THIS boot only, config.txt keeps its fps_tweaks level"); } }
        else if (fpsTwV) fpsArm();
        fpsLean = P3DS_CfgInt("fps_leanflush", 0) != 0;
        PSPNativeMemLog("[PERF4] fps_tweaks=%d (0 off, 2 hidden bottom skip / blink cache, 3 + GPU fades, 4 + GPU 3D translucency, 5 + CPU trims, 6 + stereo compose overlap, 7 + pixel passes on two cores), fps_leanflush=%d", fpsTwV, fpsLean);
    }
    return fpsTwV;
}
void P3DS_SetFpsTweaks(int v) { P3DS_FpsTweaks(); fpsTwV = (v < 2 || v > 7) ? 0 : v; if (fpsTwV) fpsArm(); else remove(FPS_PENDING); PSPNativeMemLog("[PERF4] fps_tweaks=%d from settings", fpsTwV); }
/* menu row: 0 OFF, 1 BOTTOM (level 2), 2 +FADES (level 3) */
int P3DS_FpsTweakRow(void) { int l = P3DS_FpsTweaks(); return l >= 2 ? l - 1 : 0; }
unsigned long long P3DS_FeTicks; unsigned P3DS_FeN;
/* C3D_FrameEnd(0) also flushes the whole linear heap from the cache on every call; fps_leanflush=1 ends frames with GX_CMDLIST_FLUSH only (hung on hardware, see above) */
void P3DS_FrameEnd(void) { P3DS_FpsTweaks(); const u64 t0 = svcGetSystemTick(); C3D_FrameEnd(fpsLean > 0 ? GX_CMDLIST_FLUSH : 0); P3DS_FeTicks += svcGetSystemTick() - t0; P3DS_FeN++; }
/* called once per presented frame */
/* clean exit: a short healthy session must not count as a failed boot (it used to reset fps_tweaks=7 to 0 after any quick test or unrelated crash) */
void P3DS_FpsGuardClean(void) { remove(FPS_PENDING); }
void P3DS_FpsGuardFrame(void) { if (fpsTwV > 0 && fpsOkFrames >= 0 && ++fpsOkFrames >= 600) { fpsOkFrames = -1; remove(FPS_PENDING); } }
/* [PERF4] present stages, last frame (us): FrameBegin wait (previous GPU queue), tearfix wait, FrameEnd */
unsigned P3DS_GpFenceIdleUs;
unsigned P3DS_GpBeginUs, P3DS_GpTearUs, P3DS_GpEndUs, P3DS_GpUpUs, P3DS_GpDrawUs, P3DS_GpStripUs;
/* present_pace (Debug tab, default OFF): avoid double-buffer vsync half-rate snap when a frame just misses 60.
   ON: C3D_FrameBegin(NONBLOCK) first; if the previous GPU queue is still busy, fall back to FrameBegin(0)
   (queue wait only — never SYNCDRAW). tearfree still waits for GSP present before FrameEnd (no tearing).
   OFF = old path (optional vsync=1 → SYNCDRAW). Log: [PACE]. */
static int presentPace = -1;
static unsigned paceNbOk, paceNbBlock, paceLogN;
int P3DS_PresentPaceCfg(void) {
    if (presentPace < 0) {
        presentPace = P3DS_CfgInt("present_pace", 0) != 0;
        PSPNativeMemLog("[PACE] present_pace=%d (1 = NONBLOCK FrameBegin, tearfree kept; cuts 30fps snap when near 60)", presentPace);
    }
    return presentPace;
}
void P3DS_SetPresentPace(int v) {
    presentPace = v != 0;
    PSPNativeMemLog("[PACE] present_pace=%d from settings", presentPace);
}
int P3DS_GpuInit(void) {
    if (ready) return 1;
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) return 0;
    tgtTop = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, -1);
    tgtBot = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, -1);
    if (!tgtTop || !tgtBot) return 0;
    C3D_RenderTargetSetOutput(tgtTop, GFX_TOP, GFX_LEFT, DISP_FLAGS);
    C3D_RenderTargetSetOutput(tgtBot, GFX_BOTTOM, GFX_LEFT, DISP_FLAGS);
    tgtTopR = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, -1); if (!tgtTopR) return 0;
    C3D_RenderTargetSetOutput(tgtTopR, GFX_TOP, GFX_RIGHT, DISP_FLAGS);
    dvlb = DVLB_ParseFile((u32 *)blit_shbin, blit_shbin_size);
    shaderProgramInit(&prog); shaderProgramSetVsh(&prog, &dvlb->DVLE[0]);
    uProj = shaderInstanceGetUniformLocation(prog.vertexShader, "projection");
    if (srcDbl < 0) { srcDbl = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "scalesrc_dbl=%d", &x) == 1) srcDbl = x != 0; fclose(f); } }
    for (int k = 0; k < (srcDbl ? 2 : 1); k++) for (int i = 0; i < 3; i++) {
        if (!C3D_TexInit(&texS[k][i], TEXW, TEXH, GPU_RGBA8)) { if (k) { srcDbl = 0; break; } return 0; }
        C3D_TexSetFilter(&texS[k][i], GPU_NEAREST, GPU_NEAREST);
        C3D_TexSetWrap(&texS[k][i], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
        linS[k][i] = linearAlloc(TEXW * TEXH * 4); if (!linS[k][i]) { if (k) { srcDbl = 0; break; } return 0; } memset(linS[k][i], 0, TEXW * TEXH * 4);
    }
    PSPNativeMemLog("[3DS] scalesrc_dbl=%d (scale source texture sets: %d)", srcDbl, srcDbl ? 2 : 1);
    for (int i = 0; i < 3; i++) {
        if (!C3D_TexInitVRAM(&up[i], 512, 512, GPU_RGBA8)) return 0;
        C3D_TexSetFilter(&up[i], GPU_LINEAR, GPU_LINEAR); C3D_TexSetWrap(&up[i], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
        upTgt[i] = C3D_RenderTargetCreateFromTex(&up[i], GPU_TEXFACE_2D, 0, -1); if (!upTgt[i]) return 0;
    }
    upFmt16 = 0; ready = 1; return 1;
}
/* 2026-10-28 sharp 3D VRAM: the three 512x512 RGBA8 prescale targets take 3 MiB of the 6 MiB VRAM, and the 2x 3D target
   (512x384 or 640x384 colour + 24-bit depth, 1.5-2 MiB) only fits in VRAM (linear-RAM targets drew garbage at 50 ms, run 49).
   While a 2x 3D target is wanted, g3gpu frees these (P3DS_UpFree), makes its target, then gets them back as RGB565 (half the
   size; R/B lose their lowest bit, the DS 2D is 5-bit anyway). Fallback if even that fails: g3gpu drops to 1x, RGBA8 again. */
void P3DS_UpFree(void) {
    if (!ready || upFmt16 < 0) return;
    C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH);   /* GPU idle: nothing may still sample or draw into them */
    for (int i = 0; i < 3; i++) { if (upTgt[i]) C3D_RenderTargetDelete(upTgt[i]); upTgt[i] = NULL; if (up[i].data) C3D_TexDelete(&up[i]); memset(&up[i], 0, sizeof up[i]); }
    upFmt16 = -1; botValid = 0;
}
int P3DS_Fps60VramRelease(void);
/* Run 870: with fps60_vram holding the mid textures, free VRAM is ~839 KiB — not enough for one 512x512 RGBA8
   (~1 MiB), so every map-change dance (UpFree then UpAlloc(0)) logged FAILED and fell back to RGB565. Sticky
   RGB565 after the first RGBA8 failure stops that thrash; same picture we already had after the fallback.
   Cleared when VRAM is clearly large enough again (after UpFree, free > 3 MiB) so a later quiet scene can retry. */
static int upRgba8Fail;
int P3DS_UpAlloc(int fmt16) {
    if (!ready) return 1;   /* GPU scaling not set up (mode 0): nothing to do */
    if (!fmt16 && upRgba8Fail) {
        if (upFmt16 < 0 && vramSpaceFree() > 3u * 1024u * 1024u) {
            upRgba8Fail = 0;
            PSPNativeMemLog("[3DRES] scale targets: retrying RGBA8 (VRAM free %lu)", (unsigned long)vramSpaceFree());
        } else {
            static unsigned stickN; if (!stickN++) PSPNativeMemLog("[3DRES] scale targets: sticking to RGB565 after RGBA8 fail (stops map-change retry)");
            fmt16 = 1;
        }
    }
    if (upFmt16 == fmt16) return 1;
    if (upFmt16 >= 0) P3DS_UpFree();
    const GPU_TEXCOLOR f = fmt16 ? GPU_RGB565 : GPU_RGBA8; int ok = 1;
    for (int i = 0; i < 3 && ok; i++) {
        if (!C3D_TexInitVRAM(&up[i], 512, 512, f)) { ok = 0; break; }
        C3D_TexSetFilter(&up[i], GPU_LINEAR, GPU_LINEAR); C3D_TexSetWrap(&up[i], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
        upTgt[i] = C3D_RenderTargetCreateFromTex(&up[i], GPU_TEXFACE_2D, 0, -1); if (!upTgt[i]) ok = 0; }
    upFmt16 = fmt16; botValid = 0;
    PSPNativeMemLog("[3DRES] scale targets now %s: %s (VRAM free after %lu)", fmt16 ? "RGB565" : "RGBA8", ok ? "ok" : "FAILED", (unsigned long)vramSpaceFree());
    if (!ok) { P3DS_UpFree();
        if (!fmt16) upRgba8Fail = 1;
        if (P3DS_Fps60VramRelease()) return P3DS_UpAlloc(fmt16);
        /* never leave the screen without scale targets: the other format (16-bit is half the size) */
        static int depth; if (depth) return 0;
        depth = 1; const int alt = P3DS_UpAlloc(!fmt16); depth = 0;
        return alt ? 2 : 0; }
    if (!fmt16) upRgba8Fail = 0;
    return 1;
}
/* DS pixel (compositor output: R bits 0-7, G 8-15, B 16-23) -> RGBA8 (0xRRGGBBAA); 256x192 into a 256x256 texture. */
/* run 57 gpu_swizzle=1: the compositor buffers live in linear memory (render3ds allocBufs), so the transfer reads them
   directly; the raw word R|G<<8|B<<16 then samples as texel (r,g,b,a) = (0,B,G,R) and the prescale pass rebuilds RGB
   with 3 texture-combiner stages (a*(1,0,0) + b*(0,1,0) + g*(0,0,1)). No CPU pass over the frame. 0 = old byte swap. */
extern u32 __ctru_linear_heap, __ctru_linear_heap_size;
static int swzCfg = -1, swz[3]; unsigned P3DS_UploadUs; static u64 upTicks;
int P3DS_GpuSwizzle(void) { if (swzCfg < 0) { swzCfg = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "gpu_swizzle=%d", &x) == 1) swzCfg = x != 0; fclose(f); } PSPNativeMemLog("[3DS] gpu_swizzle=%d", swzCfg); } return swzCfg; }
void P3DS_SetGpuSwizzle(int v) { swzCfg = v != 0; }
static int isLinear(const void *p) { u32 a = (u32)p; return a >= __ctru_linear_heap && a + 256 * 192 * 4 <= __ctru_linear_heap + __ctru_linear_heap_size; }
static void upload(int i, const u32 *src) {
    u64 t0 = svcGetSystemTick();
    if (P3DS_GpuSwizzle() && isLinear(src)) { swz[i] = 1;
        P3DS_DcFlush(src, TEXW * 192 * 4);
        C3D_SyncDisplayTransfer((u32 *)src, GX_BUFFER_DIM(TEXW, TEXH), tex[i].data, GX_BUFFER_DIM(TEXW, TEXH), TEX_FLAGS);
        upTicks += svcGetSystemTick() - t0; return; }
    swz[i] = 0;
    u32 *d = lin[i];
    for (int k = 0; k < 256 * 192; k++) { u32 c = src[k]; d[k] = __builtin_bswap32(c) | 0xFF;  /* == R<<24|G<<16|B<<8|FF (box-verified), one REV */ }
    P3DS_DcFlush(d, TEXW * 192 * 4);
    C3D_SyncDisplayTransfer(d, GX_BUFFER_DIM(TEXW, TEXH), tex[i].data, GX_BUFFER_DIM(TEXW, TEXH), TEX_FLAGS);
    upTicks += svcGetSystemTick() - t0;
}
/* 2026-10-29 CRISP 3D (g3gpu.c G3G_CrispHi): the unscaled 2x 3D picture, alpha = "this DS pixel is still the plain 3D pixel", drawn over the
   normal picture at the final size. */
extern const u32 *G3G_CrispHi(const u32 **nf);
static C3D_Tex hiTex; static int hiTexOk = -1; static unsigned crN, crOn, crMask; static u64 crTicks;
static int crispPrepare(const u32 *top) {
    const u32 *nf = NULL; u32 *buf = (u32 *)G3G_CrispHi(&nf);
    if (!buf || !nf || !top) return 0;
    if (hiTexOk < 0) { hiTexOk = C3D_TexInit(&hiTex, 512, 512, GPU_RGBA8) ? 1 : 0;
        if (hiTexOk) { C3D_TexSetFilter(&hiTex, GPU_LINEAR, GPU_LINEAR); C3D_TexSetWrap(&hiTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); }
        PSPNativeMemLog("[CRISP] 512x512 texture %s", hiTexOk ? "ok" : "FAILED (CRISP 3D off)"); }
    if (hiTexOk != 1) return 0;
    const u64 t0 = svcGetSystemTick();
    P3DS_DcInval(buf, 512 * 384 * 4);
    unsigned cnt = 0;
    for (int y = 0; y < 192; y++) for (int x = 0; x < 256; x++) {
        const u32 t = top[y * 256 + x], n = nf[y * 256 + x]; int m = (n >> 24) >= 31;
        for (int c = 0; c < 24 && m; c += 8) { const int nc = (n >> c) & 63, d = (int)((t >> c) & 255) - ((nc << 2) | (nc >> 4)); if (d < -6 || d > 6) m = 0; }
        cnt += m; u32 *p = buf + (2 * y) * 512 + 2 * x; const u32 a = m ? 0xFFu : 0;
        p[0] = (p[0] & ~0xFFu) | a; p[1] = (p[1] & ~0xFFu) | a; p[512] = (p[512] & ~0xFFu) | a; p[513] = (p[513] & ~0xFFu) | a; }
    P3DS_DcFlush(buf, 512 * 384 * 4);
    C3D_SyncDisplayTransfer(buf, GX_BUFFER_DIM(512, 384), hiTex.data, GX_BUFFER_DIM(512, 384), TEX_FLAGS);
    crOn++; crMask += cnt; crTicks += svcGetSystemTick() - t0; return 1;
}
static void crispDraw(C3D_RenderTarget *t, float sw, float x0, float y0, float x1, float y1) {
    C3D_FrameDrawOn(t);
    C3D_Mtx proj; Mtx_OrthoTilt(&proj, 0.0f, sw, 240.0f, 0.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, &hiTex);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ZERO);
    const float t1 = 384.0f / 512.0f;
    C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
    C3D_ImmSendAttrib(x0, y0, 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0);
    C3D_ImmSendAttrib(x0, y1, 0, 0); C3D_ImmSendAttrib(0, t1, 0, 0);
    C3D_ImmSendAttrib(x1, y0, 0, 0); C3D_ImmSendAttrib(1, 0, 0, 0);
    C3D_ImmSendAttrib(x1, y1, 0, 0); C3D_ImmSendAttrib(1, t1, 0, 0);
    C3D_ImmDrawEnd();
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
}
static void tevPlain(void) { C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    for (int k = 1; k < 6; k++) C3D_TexEnvInit(C3D_GetTexEnv(k)); }
static void tevSwizzle(void) { static const GPU_TEVOP_RGB op[3] = { GPU_TEVOP_RGB_SRC_ALPHA, GPU_TEVOP_RGB_SRC_B, GPU_TEVOP_RGB_SRC_G };
    static const u32 cc[3] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };   /* ABGR: R, G, B */
    for (int k = 0; k < 3; k++) { C3D_TexEnv *env = C3D_GetTexEnv(k); C3D_TexEnvInit(env);
        if (!k) { C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_CONSTANT, 0); C3D_TexEnvFunc(env, C3D_RGB, GPU_MODULATE); }
        else { C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_CONSTANT, GPU_PREVIOUS); C3D_TexEnvFunc(env, C3D_RGB, GPU_MULTIPLY_ADD); }
        C3D_TexEnvOpRgb(env, op[k], GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
        C3D_TexEnvSrc(env, C3D_Alpha, GPU_CONSTANT, 0, 0); C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE); C3D_TexEnvColor(env, cc[k]); }
    for (int k = 3; k < 6; k++) C3D_TexEnvInit(C3D_GetTexEnv(k)); }
/* Pass 1: DS texture -> up[i] at exactly 2x with NEAREST. Positions = texcoords * 512 (GL-style texture target,
   no tilt), so the image keeps the same s/t layout as the DS texture (top row t = 0, bottom t = 0.75). */
/* 2026-11-04 direct screen (gpucomp.c P3DS_GcPendTex): the top picture can come straight from the compositor's GPU target. Its
   memory layout is the uploaded texture's (both are RGBA8 written through a FLIP_VERT transfer), so the same quad samples it;
   alpha is forced to 255 like the upload does. The fps counter is then drawn here (P3DS_GpTextMask: 0 none, 1 black, 2 white). */
extern C3D_Tex *P3DS_GcPendTex(const u32 *buf); extern void P3DS_GcPresented(void);
unsigned char P3DS_GpTextMask[8][64]; int P3DS_GpTextOn;
static C3D_Tex *dirTop, *dirTopR;
static C3D_Tex *dirOf(int i) { return i == 0 ? dirTop : i == 2 ? dirTopR : NULL; }
static void tevOpaque(void) { C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, 0, 0); C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_CONSTANT, 0, 0); C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE); C3D_TexEnvColor(env, 0xFFFFFFFF);
    for (int k = 1; k < 6; k++) C3D_TexEnvInit(C3D_GetTexEnv(k)); }
static void textMaskDraw(void) {   /* into upTgt[0] (2x DS pixels, top row y = 0) */
    for (int pass = 1; pass <= 2; pass++) {
        C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env); C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, 0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
        C3D_TexEnvColor(env, pass == 2 ? 0xFFFFFFFF : 0xFF000000);
        C3D_ImmDrawBegin(GPU_TRIANGLES);
        for (int y = 0; y < 8; y++) for (int x = 0; x < 64; x++) { if (P3DS_GpTextMask[y][x] != pass) continue; int e = x; while (e + 1 < 64 && P3DS_GpTextMask[y][e + 1] == pass) e++;
            const float x0 = 2.0f * x, x1 = 2.0f * (e + 1), y0 = 2.0f * y, y1 = y0 + 2.0f;
            const float v[6][2] = { { x0, y0 }, { x1, y0 }, { x0, y1 }, { x1, y0 }, { x1, y1 }, { x0, y1 } };
            for (int k = 0; k < 6; k++) { C3D_ImmSendAttrib(v[k][0], v[k][1], 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0); } x = e; }
        C3D_ImmDrawEnd(); }
    tevPlain(); }
static void prescale(int i) {
    if (dirOf(i)) { tevOpaque(); } else
    if (swz[i]) tevSwizzle();
    C3D_RenderTargetClear(upTgt[i], C3D_CLEAR_ALL, 0x000000FF, 0);
    C3D_FrameDrawOn(upTgt[i]);
    C3D_Mtx proj; Mtx_Ortho(&proj, 0.0f, 512.0f, 0.0f, 512.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, dirOf(i) ? dirOf(i) : &tex[i]);
    const float t1 = 192.0f / TEXH;
    C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
    C3D_ImmSendAttrib(0, 0, 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0);
    C3D_ImmSendAttrib(512, 0, 0, 0); C3D_ImmSendAttrib(1, 0, 0, 0);
    C3D_ImmSendAttrib(0, 512 * t1, 0, 0); C3D_ImmSendAttrib(0, t1, 0, 0);
    C3D_ImmSendAttrib(512, 512 * t1, 0, 0); C3D_ImmSendAttrib(1, t1, 0, 0);
    C3D_ImmDrawEnd();
    if (dirOf(i)) { tevPlain(); if (P3DS_GpTextOn) textMaskDraw(); }
    else if (swz[i]) tevPlain();
}
/* Same 24-bit fill as gpucomp.c. 0x000000FF is opaque black on RGBA8 and pure
   blue on the RGB8 second screen target unless the alpha byte is shifted out. */
static u32 presentClear(const C3D_RenderTarget *t, u32 rgba) {
    return t->frameBuf.colorFmt == GPU_RB_RGB8 ? rgba >> 8 : rgba;
}
static void quad(C3D_RenderTarget *t, float sw, int i, float x0, float y0, float x1, float y1) {
    C3D_RenderTargetClear(t, C3D_CLEAR_ALL, presentClear(t, 0x000000FF), 0);
    C3D_FrameDrawOn(t);
    C3D_Mtx proj; Mtx_OrthoTilt(&proj, 0.0f, sw, 240.0f, 0.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, &up[i]);
    /* Hardware run: the DS image's top row lands at t = 0 and its last row at t = 192/256 (the earlier
       t = 1 .. 0.25 mapping showed the picture upside down and shifted). */
    const float t0 = 0.0f, t1 = 384.0f / 512.0f;
    C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
    C3D_ImmSendAttrib(x0, y0, 0, 0); C3D_ImmSendAttrib(0, t0, 0, 0);
    C3D_ImmSendAttrib(x0, y1, 0, 0); C3D_ImmSendAttrib(0, t1, 0, 0);
    C3D_ImmSendAttrib(x1, y0, 0, 0); C3D_ImmSendAttrib(1, t0, 0, 0);
    C3D_ImmSendAttrib(x1, y1, 0, 0); C3D_ImmSendAttrib(1, t1, 0, 0);
    C3D_ImmDrawEnd();
}
/* run 100 lower_menu: GabeSud0's bottom-screen icon menu (lowermenu.c, see THIRD_PARTY.md). The 320x240 menu picture
   fills the screen; the DS's lower screen is drawn into its panel (3,4 .. 243,236: the canvas 256x247 with DS row 0 at
   canvas row 27, its top row carried up and its last row carried down). P3DS_LowerMenuDraw is set per frame by render3ds. */
#include "nitro_port_lowermenu.h"
int P3DS_LowerMenuDraw; extern int P3DS_LogLevel(void);
static C3D_Tex menuTex; static u32 *menuLin; static int menuOk = -1; static uint32_t menuSerial = 0xFFFFFFFFu;
static void imm(float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1) {
    C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
    C3D_ImmSendAttrib(x0, y0, 0, 0); C3D_ImmSendAttrib(s0, t0, 0, 0);
    C3D_ImmSendAttrib(x0, y1, 0, 0); C3D_ImmSendAttrib(s0, t1, 0, 0);
    C3D_ImmSendAttrib(x1, y0, 0, 0); C3D_ImmSendAttrib(s1, t0, 0, 0);
    C3D_ImmSendAttrib(x1, y1, 0, 0); C3D_ImmSendAttrib(s1, t1, 0, 0);
    C3D_ImmDrawEnd();
}
static int menuDraw(void) {
    if (menuOk < 0) { menuOk = 0;
        if (C3D_TexInit(&menuTex, 512, 256, GPU_RGBA8) && (menuLin = linearAlloc(512 * 256 * 4)) != NULL) {
            C3D_TexSetFilter(&menuTex, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&menuTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
            memset(menuLin, 0, 512 * 256 * 4); menuOk = 1; }
        PSPNativeMemLog("[3DS] lower_menu texture %s", menuOk ? "ok" : "FAILED"); }
    if (!menuOk) return 0;
    uint32_t ser; const uint32_t *pic = NitroPort_LowerMenuLockFrame(&ser);
    if (!pic) { NitroPort_LowerMenuUnlockFrame(); return 0; }
    if (ser != menuSerial) { menuSerial = ser;
        for (int y = 0; y < NITRO_LOWERMENU_H; y++) memcpy(menuLin + y * 512, pic + y * NITRO_LOWERMENU_W, NITRO_LOWERMENU_W * 4);
        GSPGPU_FlushDataCache(menuLin, 512 * 256 * 4);
        C3D_SyncDisplayTransfer(menuLin, GX_BUFFER_DIM(512, 256), menuTex.data, GX_BUFFER_DIM(512, 256), TEX_FLAGS); }
    NitroPort_LowerMenuUnlockFrame();
    C3D_RenderTargetClear(tgtBot, C3D_CLEAR_ALL, 0x000000FF, 0);
    C3D_FrameDrawOn(tgtBot);
    C3D_Mtx proj; Mtx_OrthoTilt(&proj, 0.0f, 320.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, &menuTex);
    imm(0, 0, 320, 240, 0, 0, 320.0f / 512.0f, 240.0f / 256.0f);
    C3D_TexBind(0, &up[1]);
    const float k = (float)NITRO_LOWERMENU_PANEL_H / (float)NITRO_LOWERMENU_CANVAS_H;
    const float x0 = NITRO_LOWERMENU_PANEL_X0, x1 = x0 + NITRO_LOWERMENU_PANEL_W, y0 = NITRO_LOWERMENU_PANEL_Y0;
    const float yA = y0 + NITRO_LOWERMENU_DS_ROW0 * k, yB = y0 + (NITRO_LOWERMENU_DS_ROW0 + 192) * k, yE = y0 + NITRO_LOWERMENU_PANEL_H;
    const float row = (384.0f / 512.0f) / 192.0f, tB = 384.0f / 512.0f;
    imm(x0, y0, x1, yA, 0, 0.5f * row, 1, 0.5f * row);
    imm(x0, yA, x1, yB, 0, 0, 1, tB);
    imm(x0, yB, x1, yE, 0, tB - 0.5f * row, 1, tB - 0.5f * row);
    return 1;
}
/* run 43 wideview: side strips of the wide 3D pass (g3gpu G3G_Strip, 64x192 RGBA8: left 32 | right 32 DS px), drawn
   at x 0..40 and 360..400 (1.25x, LINEAR) after the centred picture. P3DS_WideStrip[eye] = NULL: not drawn. */
const u32 *P3DS_WideStrip[2];
static C3D_Tex stripTex[2]; static u32 *stripLin[2]; static int stripOk = -1;
static C3D_Tex stripTexM[2]; static u32 *stripLinM[2]; static int stripMid;

/* The midpoint conversion writes into the display-transfer source directly.
   Call after the compositor has acquired the GPU for this picture. */
u32 *P3DS_GpuMidStripBuffer(int eye) {
    if (!stripLinM[eye]) {
        u32 *buf = linearAlloc(64 * 256 * sizeof(u32));
        if (!buf) return NULL;
        if (!C3D_TexInit(&stripTexM[eye], 64, 256, GPU_RGBA8)) {
            linearFree(buf);
            return NULL;
        }
        C3D_TexSetFilter(&stripTexM[eye], GPU_LINEAR, GPU_LINEAR);
        C3D_TexSetWrap(&stripTexM[eye], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
        memset(buf, 0, 64 * 256 * sizeof(u32));
        stripLinM[eye] = buf;
    }
    return stripLinM[eye];
}
/* run 52 wide_fadefix=1 (default): the strips get the CURRENT top-screen fade at draw time: MASTER_BRIGHT (0x6C),
   BLDCNT brighten/darken on BG0 (3D) with BLDY, forced blank / BG0 off / display off = black. A fade seen in the last
   3 presents is held (strongest), so the strips never show the new area before the centre has faded in. */
static int fadeCfg = -1;
int P3DS_WideFadeFix(void) {
    if (fadeCfg < 0) { fadeCfg = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "wide_fadefix=%d", &x) == 1) fadeCfg = x != 0; fclose(f); } PSPNativeMemLog("[WIDE] wide_fadefix=%d", fadeCfg); }
    return fadeCfg; }
extern unsigned char PSPNative_GfxRegisters[];
static void fadeNow(int *dir, unsigned *amt) {   /* dir 1 = white, 2 = black; amt 0..16 */
    const unsigned char *R = PSPNative_GfxRegisters; u32 dc = *(const u32 *)R; u16 mb = *(const u16 *)(R + 0x6C), bc = *(const u16 *)(R + 0x50), by = *(const u16 *)(R + 0x54);
    *dir = 0; *amt = 0;
    if ((dc & 0x80) || !(dc & 0x100) || !((dc >> 16) & 3)) { *dir = 2; *amt = 16; return; }
    unsigned mm = (mb >> 14) & 3, f = mb & 31; if (f > 16) f = 16;
    if ((mm == 1 || mm == 2) && f) { *dir = (int)mm; *amt = f; }
    unsigned eff = (bc >> 6) & 3, y = by & 31; if (y > 16) y = 16;
    if ((bc & 1) && (eff == 2 || eff == 3) && y) { int d2 = eff == 2 ? 1 : 2;
        if (!*dir) { *dir = d2; *amt = y; } else if (d2 == *dir) *amt = *amt + y * (16 - *amt) / 16; }
}
/* run 61 wide_haze=1 (default; only while wide-view strips are drawn): a 2D haze/fog layer alpha-blended over the 3D
   (BLDCNT effect 1, BG0 = 2nd target) only exists in the 256-px centre, so the strips looked clear. render3ds fits it
   per frame as C = k*3D + b (k = EVB/16 from BLDALPHA, b = median of the centre's own residual over sparse samples of
   both edge bands, ~0.1 ms) and the strips get it in the texture combiner (tex*k + b): no CPU pass over pixels. */
int P3DS_HazeOn; unsigned P3DS_HazeK, P3DS_HazeB; /* ABGR constants for the combiner */
static int hazeCfg = -1;
int P3DS_WideHazeCfg(void) { if (hazeCfg < 0) { hazeCfg = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "wide_haze=%d", &x) == 1) hazeCfg = x != 0; fclose(f); } PSPNativeMemLog("[WIDE] wide_haze=%d", hazeCfg); } return hazeCfg; }
void P3DS_SetWideHazeCfg(int v) { hazeCfg = v != 0; }
static void strips(C3D_RenderTarget *t, int eye) {
    const u32 *src = P3DS_WideStrip[eye]; if (!src) return;
    static int hDir; static unsigned hAmt, hLeft; int fDir = 0; unsigned fAmt = 0;
    extern int P3DS_WideFxOn; /* run 54: wide_fx already matched the strips to the centre picture */
    if (P3DS_WideFadeFix() && !P3DS_WideFxOn) { static u64 lastT; u64 now = svcGetSystemTick();
        if (!stripMid && (eye == 0 || now - lastT > 268111 * 8)) { int d; unsigned a; fadeNow(&d, &a); lastT = now;
            if (d && a) { hDir = d; hAmt = a; hLeft = 3; } else if (hLeft) { hLeft--; if (!hLeft) hAmt = 0; } }
        if (hAmt) { fDir = hDir; fAmt = hAmt; } }
    if (stripOk < 0) { stripOk = 1; for (int e = 0; e < 2; e++) { if (!C3D_TexInit(&stripTex[e], 64, 256, GPU_RGBA8) || !(stripLin[e] = linearAlloc(64 * 256 * 4))) { stripOk = 0; break; }
        C3D_TexSetFilter(&stripTex[e], GPU_LINEAR, GPU_LINEAR); C3D_TexSetWrap(&stripTex[e], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); memset(stripLin[e], 0, 64 * 256 * 4); }
        PSPNativeMemLog("[WIDE] strip textures %s", stripOk ? "ok" : "FAILED"); }
    if (!stripOk) return;
    /* run 925 crash (memcpy to NULL on the first frame with the 3D slider up): the fps60 "hold" in-between picture passes the real
       strips and never asked for the in-between strip buffer, so for the right eye it did not exist yet. Make it here; none = no strips. */
    if (stripMid && !stripLinM[eye] && !P3DS_GpuMidStripBuffer(eye)) return;
    u32 *const sLin = stripMid ? stripLinM[eye] : stripLin[eye];
    C3D_Tex *const sTx = stripMid ? &stripTexM[eye] : &stripTex[eye];
    if (!fAmt) {
        if (sLin != src) memcpy(sLin, src, 64 * 192 * sizeof(u32));
    }
    else if (fAmt >= 16) { const u32 c = fDir == 1 ? 0xFFFFFFFFu : 0x000000FFu; for (unsigned i = 0; i < 64 * 192; i++) sLin[i] = c; }
    else for (unsigned i = 0; i < 64 * 192; i++) { u32 c = src[i]; unsigned r = c >> 24, g = (c >> 16) & 255, b = (c >> 8) & 255;
        if (fDir == 1) { r += (255 - r) * fAmt / 16; g += (255 - g) * fAmt / 16; b += (255 - b) * fAmt / 16; } else { r -= r * fAmt / 16; g -= g * fAmt / 16; b -= b * fAmt / 16; }
        sLin[i] = (r << 24) | (g << 16) | (b << 8) | 0xFF; }
    { extern u32 P3DS_WeatherObj[]; extern int P3DS_WeatherOn; if (P3DS_WeatherOn && !fAmt) for (unsigned i = 0; i < 64 * 192; i++) if (P3DS_WeatherObj[i]) sLin[i] = P3DS_WeatherObj[i]; } /* run 70 wide_weather */
    P3DS_DcFlush(sLin, 64 * 192 * 4);
    C3D_SyncDisplayTransfer(sLin, GX_BUFFER_DIM(64, 256), sTx->data, GX_BUFFER_DIM(64, 256), TEX_FLAGS);
    C3D_FrameDrawOn(t);
    C3D_Mtx proj; Mtx_OrthoTilt(&proj, 0.0f, 400.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, sTx);
    const int haze = P3DS_HazeOn && !fAmt;
    if (haze) { C3D_TexEnv *e0 = C3D_GetTexEnv(0), *e1 = C3D_GetTexEnv(1); C3D_TexEnvInit(e0); C3D_TexEnvInit(e1);
        C3D_TexEnvSrc(e0, C3D_RGB, GPU_TEXTURE0, GPU_CONSTANT, 0); C3D_TexEnvFunc(e0, C3D_RGB, GPU_MODULATE); C3D_TexEnvColor(e0, P3DS_HazeK);
        C3D_TexEnvSrc(e0, C3D_Alpha, GPU_TEXTURE0, 0, 0); C3D_TexEnvFunc(e0, C3D_Alpha, GPU_REPLACE);
        C3D_TexEnvSrc(e1, C3D_RGB, GPU_PREVIOUS, GPU_CONSTANT, 0); C3D_TexEnvFunc(e1, C3D_RGB, GPU_ADD); C3D_TexEnvColor(e1, P3DS_HazeB);
        C3D_TexEnvSrc(e1, C3D_Alpha, GPU_PREVIOUS, 0, 0); C3D_TexEnvFunc(e1, C3D_Alpha, GPU_REPLACE); }
    const float t1 = 192.0f / 256.0f, hx = 0.5f / 64.0f; /* half-texel inset so LINEAR does not bleed across the 32|32 seam */
    for (int side = 0; side < 2; side++) { float x0 = side ? 360 : 0, x1 = side ? 400 : 40, s0 = side ? 0.5f + hx : 0.0f, s1 = side ? 1.0f : 0.5f - hx;
        C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
        C3D_ImmSendAttrib(x0, 0, 0, 0); C3D_ImmSendAttrib(s0, 0, 0, 0);
        C3D_ImmSendAttrib(x0, 240, 0, 0); C3D_ImmSendAttrib(s0, t1, 0, 0);
        C3D_ImmSendAttrib(x1, 0, 0, 0); C3D_ImmSendAttrib(s1, 0, 0, 0);
        C3D_ImmSendAttrib(x1, 240, 0, 0); C3D_ImmSendAttrib(s1, t1, 0, 0);
        C3D_ImmDrawEnd(); }
    if (haze) tevPlain();
}
/* run 38: called when leaving the GPU-scaled path (mode 0): wait until citro3d's queue (last transfer + its async
   gfxScreenSwapBuffers) is done, and turn 3D off so the never-written right-eye buffer isn't shown. */
/* run 40: present_skipbot=1 (default): when engine B reused its last image (bottom_skip) skip the bottom screen's
   byte-swap upload + 2x prescale and draw from the persistent up[1] (VRAM) texture. Forced after init, mode-0 visits
   and console/bottom toggles. 0 = always upload. */
int P3DS_GpuBotSame; static int skipBotCfg = -1; static unsigned botSkips;
void P3DS_GpuInvalidateBot(void) { botValid = 0; } /* run 64: settings app drew its own bottom picture */
static void dfrFlush(void);
void P3DS_GpuQuiesce(void) { botValid = 0; if (ready) dfrFlush();
    if (ready) { C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH); gspWaitForVBlank(); }
    stereoOn = 0; gfxSet3D(false);
}
/* topR != NULL: stereo (slider > 0): left eye = top, right eye = topR. NULL: 2D mode, no right-eye cost. */
extern void P3DS_Crumb(const char *tag, void *caller);
/* run 11: native-resolution icon overlay for the bottom screen (JOHTO layout). The platform fills P3DS_OvAtlas (512x256 since run 18; was 256x128,
   words R<<24|G<<16|B<<8|A, row 0 = top), sets P3DS_OvDirty when it changed, and lists P3DS_OvN rectangles in 320x240
   bottom-screen pixels with their atlas source (same size: drawn 1:1, NEAREST, alpha-blended after the scaled DS
   picture), so the icons are not resampled by the 256x192 -> 320x240 stretch. N = 0: nothing drawn. */
typedef struct { short dx, dy, w, h, sx, sy; } P3DS_OvRect;
u32 *P3DS_OvAtlas; volatile int P3DS_OvDirty, P3DS_OvN; P3DS_OvRect P3DS_OvRects[96];
static C3D_Tex ovTex; static int ovOk = -1;
u32 *P3DS_OvAtlasGet(void) { if (!P3DS_OvAtlas) { P3DS_OvAtlas = linearAlloc(512 * 256 * 4); if (P3DS_OvAtlas) memset(P3DS_OvAtlas, 0, 512 * 256 * 4); } return P3DS_OvAtlas; }
/* 2026-11-04 (Nathan: transitions must be seamless): while the Bottom menu covers the touch screen, the game's own fade of that
   screen (MASTER_BRIGHT of the engine on the bottom) never reached it, so the menu stayed bright while the top faded out and the
   next screen faded in. P3DS_BotFade (-16..16: down = black, up = white, set by render3ds.cpp only while our picture is there)
   is drawn as one blended quad over the finished bottom screen. */
int P3DS_BotFade;
static void botFadeDraw(C3D_RenderTarget *t) {
    const int f = P3DS_BotFade; if (!f) return; const unsigned a = (unsigned)((f < 0 ? -f : f) * 255 / 16);
    C3D_FrameDrawOn(t);
    C3D_Mtx proj; Mtx_OrthoTilt(&proj, 0.0f, 320.0f, 240.0f, 0.0f, 1.0f, -1.0f, true); C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env); C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, 0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    C3D_TexEnvColor(env, (a << 24) | (f > 0 ? 0xFFFFFF : 0));
    for (int k = 1; k < 6; k++) C3D_TexEnvInit(C3D_GetTexEnv(k));
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ZERO, GPU_ONE);
    C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
    C3D_ImmSendAttrib(0, 0, 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0); C3D_ImmSendAttrib(0, 240, 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0);
    C3D_ImmSendAttrib(320, 0, 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0); C3D_ImmSendAttrib(320, 240, 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0);
    C3D_ImmDrawEnd();
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO); tevPlain(); }
static void ovDraw(C3D_RenderTarget *t) {
    const int n = P3DS_OvN; if (n <= 0 || !P3DS_OvAtlas) return;
    if (ovOk < 0) { ovOk = C3D_TexInit(&ovTex, 512, 256, GPU_RGBA8) ? 1 : 0; if (ovOk) { C3D_TexSetFilter(&ovTex, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&ovTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); P3DS_OvDirty = 1; }
        PSPNativeMemLog("[ICONS] native overlay texture %s", ovOk ? "ready" : "FAILED (icons stay in the DS picture)"); }
    if (!ovOk) return;
    if (P3DS_OvDirty) { P3DS_OvDirty = 0; P3DS_DcFlush(P3DS_OvAtlas, 512 * 256 * 4);
        C3D_SyncDisplayTransfer(P3DS_OvAtlas, GX_BUFFER_DIM(512, 256), ovTex.data, GX_BUFFER_DIM(512, 256), TEX_FLAGS); }
    C3D_FrameDrawOn(t);
    C3D_Mtx proj; Mtx_OrthoTilt(&proj, 0.0f, 320.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, &ovTex);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ZERO);
    for (int i = 0; i < n && i < 96; i++) { const P3DS_OvRect *r = &P3DS_OvRects[i];
        const float x0 = r->dx, y0 = r->dy, x1 = r->dx + r->w, y1 = r->dy + r->h, s0 = r->sx / 512.0f, s1 = (r->sx + r->w) / 512.0f, t0 = r->sy / 256.0f, t1 = (r->sy + r->h) / 256.0f;
        C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
        C3D_ImmSendAttrib(x0, y0, 0, 0); C3D_ImmSendAttrib(s0, t0, 0, 0);
        C3D_ImmSendAttrib(x0, y1, 0, 0); C3D_ImmSendAttrib(s0, t1, 0, 0);
        C3D_ImmSendAttrib(x1, y0, 0, 0); C3D_ImmSendAttrib(s1, t0, 0, 0);
        C3D_ImmSendAttrib(x1, y1, 0, 0); C3D_ImmSendAttrib(s1, t1, 0, 0);
        C3D_ImmDrawEnd(); }
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
}
/* 2026-11-05 (Nathan: the achievement toast looked blocky): one port picture for the TOP screen at the final 400x240 resolution.
   The platform draws it into P3DS_TopOvBuf (512x64 words R<<24|G<<16|B<<8|A, stride 512), sets P3DS_TopOvX/Y/W/H in 400x240 px and
   P3DS_TopOvOn for that present (consumed); drawn 1:1 NEAREST, alpha-blended, on both eyes at screen depth, after the strips. */
u32 *P3DS_TopOvBuf; volatile int P3DS_TopOvOn, P3DS_TopOvDirty; int P3DS_TopOvX, P3DS_TopOvY, P3DS_TopOvW, P3DS_TopOvH;
static C3D_Tex tovTex; static int tovOk = -1;
u32 *P3DS_TopOvBufGet(void) { if (!P3DS_TopOvBuf) { P3DS_TopOvBuf = linearAlloc(512 * 64 * 4); if (P3DS_TopOvBuf) memset(P3DS_TopOvBuf, 0, 512 * 64 * 4); } return P3DS_TopOvBuf; }
static void tovEnsure(void) {
    if (tovOk < 0) { tovOk = C3D_TexInit(&tovTex, 512, 64, GPU_RGBA8) ? 1 : 0; if (tovOk) { C3D_TexSetFilter(&tovTex, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&tovTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); P3DS_TopOvDirty = 1; }
        PSPNativeMemLog("[TOPOV] native top overlay texture %s", tovOk ? "ready" : "FAILED (toast stays in the DS picture)"); }
}
static void tov2Ensure(void); u32 *P3DS_TopOv2BufGet(void);
void P3DS_TopOvInit(void) { P3DS_TopOvBufGet(); P3DS_TopOv2BufGet(); tovEnsure(); tov2Ensure(); }
static void topOvDraw(C3D_RenderTarget *t) {
    if (!P3DS_TopOvBuf || P3DS_TopOvW < 1 || P3DS_TopOvH < 1 || P3DS_TopOvW > 512 || P3DS_TopOvH > 64) return;
    P3DS_TopOvInit();
    if (!tovOk) return;
    if (P3DS_TopOvDirty) { P3DS_TopOvDirty = 0; P3DS_DcFlush(P3DS_TopOvBuf, 512 * 64 * 4);
        C3D_SyncDisplayTransfer(P3DS_TopOvBuf, GX_BUFFER_DIM(512, 64), tovTex.data, GX_BUFFER_DIM(512, 64), TEX_FLAGS); }
    C3D_FrameDrawOn(t);
    C3D_Mtx proj; Mtx_OrthoTilt(&proj, 0.0f, 400.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, &tovTex);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ZERO, GPU_ONE);
    const float x0 = P3DS_TopOvX, y0 = P3DS_TopOvY, x1 = P3DS_TopOvX + P3DS_TopOvW, y1 = P3DS_TopOvY + P3DS_TopOvH, s1 = P3DS_TopOvW / 512.0f, t1 = P3DS_TopOvH / 64.0f;
    C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
    C3D_ImmSendAttrib(x0, y0, 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0);
    C3D_ImmSendAttrib(x0, y1, 0, 0); C3D_ImmSendAttrib(0, t1, 0, 0);
    C3D_ImmSendAttrib(x1, y0, 0, 0); C3D_ImmSendAttrib(s1, 0, 0, 0);
    C3D_ImmSendAttrib(x1, y1, 0, 0); C3D_ImmSendAttrib(s1, t1, 0, 0);
    C3D_ImmDrawEnd();
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
}
/* 2026-11-06: a second, small native top overlay (64x32) for port-drawn list cursors; same rules as P3DS_TopOv above. */
u32 *P3DS_TopOv2Buf; volatile int P3DS_TopOv2On, P3DS_TopOv2Dirty; int P3DS_TopOv2X, P3DS_TopOv2Y, P3DS_TopOv2W, P3DS_TopOv2H;
static C3D_Tex tov2Tex; static int tov2Ok = -1;
u32 *P3DS_TopOv2BufGet(void) { if (!P3DS_TopOv2Buf) { P3DS_TopOv2Buf = linearAlloc(64 * 32 * 4); if (P3DS_TopOv2Buf) memset(P3DS_TopOv2Buf, 0, 64 * 32 * 4); } return P3DS_TopOv2Buf; }
static void tov2Ensure(void) {
    if (tov2Ok < 0) { tov2Ok = C3D_TexInit(&tov2Tex, 64, 32, GPU_RGBA8) ? 1 : 0; if (tov2Ok) { C3D_TexSetFilter(&tov2Tex, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&tov2Tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); P3DS_TopOv2Dirty = 1; }
        PSPNativeMemLog("[TOPOV] small top overlay texture %s", tov2Ok ? "ready" : "FAILED"); }
}
static void topOv2Draw(C3D_RenderTarget *t) {
    if (!P3DS_TopOv2Buf || P3DS_TopOv2W < 1 || P3DS_TopOv2H < 1 || P3DS_TopOv2W > 64 || P3DS_TopOv2H > 32) return;
    P3DS_TopOvInit();
    if (!tov2Ok) return;
    if (P3DS_TopOv2Dirty) { P3DS_TopOv2Dirty = 0; P3DS_DcFlush(P3DS_TopOv2Buf, 64 * 32 * 4);
        C3D_SyncDisplayTransfer(P3DS_TopOv2Buf, GX_BUFFER_DIM(64, 32), tov2Tex.data, GX_BUFFER_DIM(64, 32), TEX_FLAGS); }
    C3D_FrameDrawOn(t);
    C3D_Mtx proj; Mtx_OrthoTilt(&proj, 0.0f, 400.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, &tov2Tex);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ZERO, GPU_ONE);
    const float x0 = P3DS_TopOv2X, y0 = P3DS_TopOv2Y, x1 = P3DS_TopOv2X + P3DS_TopOv2W, y1 = P3DS_TopOv2Y + P3DS_TopOv2H, s1 = P3DS_TopOv2W / 64.0f, t1 = P3DS_TopOv2H / 32.0f;
    C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
    C3D_ImmSendAttrib(x0, y0, 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0);
    C3D_ImmSendAttrib(x0, y1, 0, 0); C3D_ImmSendAttrib(0, t1, 0, 0);
    C3D_ImmSendAttrib(x1, y0, 0, 0); C3D_ImmSendAttrib(s1, 0, 0, 0);
    C3D_ImmSendAttrib(x1, y1, 0, 0); C3D_ImmSendAttrib(s1, t1, 0, 0);
    C3D_ImmDrawEnd();
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
}
/* run 138: the panel app picture (party / bag / summary in the JOHTO panel) at the final bottom-screen resolution. The platform resamples the DS 256x192
   picture ONCE to P3DS_PnW x P3DS_PnH words (R<<24|G<<16|B<<8|A) in P3DS_PnBuf (stride 256, up to 256x256); drawn 1:1 NEAREST at P3DS_PnX,P3DS_PnY (320x240 px)
   over the stretched DS bottom, so it is not resampled a second time by the 1.25x stretch. P3DS_PnOn is consumed (cleared) by each present. */
u32 *P3DS_PnBuf; volatile int P3DS_PnOn, P3DS_PnDirty; int P3DS_PnX, P3DS_PnY, P3DS_PnW, P3DS_PnH;
static C3D_Tex pnTex; static int pnOk = -1;
u32 *P3DS_PnBufGet(void) { if (!P3DS_PnBuf) { P3DS_PnBuf = linearAlloc(256 * 256 * 4); if (P3DS_PnBuf) memset(P3DS_PnBuf, 0, 256 * 256 * 4); } return P3DS_PnBuf; }
static void pnDraw(C3D_RenderTarget *t) {
    const int on = P3DS_PnOn; P3DS_PnOn = 0;
    if (!on || !P3DS_PnBuf || P3DS_PnW < 1 || P3DS_PnH < 1 || P3DS_PnW > 256 || P3DS_PnH > 256) return;
    if (pnOk < 0) { pnOk = C3D_TexInit(&pnTex, 256, 256, GPU_RGBA8) ? 1 : 0; if (pnOk) { C3D_TexSetFilter(&pnTex, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&pnTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); P3DS_PnDirty = 1; }
        PSPNativeMemLog("[PANEL] final-res panel texture %s", pnOk ? "ready" : "FAILED (panel picture stays in the DS bottom)"); }
    if (!pnOk) return;
    if (P3DS_PnDirty) { P3DS_PnDirty = 0; P3DS_DcFlush(P3DS_PnBuf, 256 * 256 * 4);
        C3D_SyncDisplayTransfer(P3DS_PnBuf, GX_BUFFER_DIM(256, 256), pnTex.data, GX_BUFFER_DIM(256, 256), TEX_FLAGS); }
    C3D_FrameDrawOn(t);
    C3D_Mtx proj; Mtx_OrthoTilt(&proj, 0.0f, 320.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, &pnTex);
    const float x0 = P3DS_PnX, y0 = P3DS_PnY, x1 = P3DS_PnX + P3DS_PnW, y1 = P3DS_PnY + P3DS_PnH, s1 = P3DS_PnW / 256.0f, t1 = P3DS_PnH / 256.0f;
    C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
    C3D_ImmSendAttrib(x0, y0, 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0);
    C3D_ImmSendAttrib(x0, y1, 0, 0); C3D_ImmSendAttrib(0, t1, 0, 0);
    C3D_ImmSendAttrib(x1, y0, 0, 0); C3D_ImmSendAttrib(s1, 0, 0, 0);
    C3D_ImmSendAttrib(x1, y1, 0, 0); C3D_ImmSendAttrib(s1, t1, 0, 0);
    C3D_ImmDrawEnd();
}
/* 2026-11-18 sharp bottom text (ss3ds_sharptext.h): the Bottom menu panel's and the settings app's text at the final 320x240 resolution.
   The platform draws it into P3DS_TxBuf (512x256 words R<<24|G<<16|B<<8|A, 320x240 used, transparent where there is no text), sets
   P3DS_TxDirty when it changed and P3DS_TxOn for each present that shows it (consumed). Drawn 1:1 NEAREST, alpha-blended, after the
   stretched DS bottom and before the panel picture and the native column, so both still cover it. P3DS_TxFailed: texture could not
   be made; the platform then keeps the text in the DS picture. */
u32 *P3DS_TxBuf; volatile int P3DS_TxOn, P3DS_TxDirty, P3DS_TxFailed;
static C3D_Tex txTex; static int txOk = -1;
u32 *P3DS_TxBufGet(void) { if (!P3DS_TxBuf && !P3DS_TxFailed) { P3DS_TxBuf = linearAlloc(512 * 256 * 4); if (P3DS_TxBuf) memset(P3DS_TxBuf, 0, 512 * 256 * 4); else { P3DS_TxFailed = 1; PSPNativeMemLog("[TEXT] native text buffer: no linear memory (text stays in the DS picture)"); } } return P3DS_TxBuf; }
static void txDraw(C3D_RenderTarget *t) {
    const int on = P3DS_TxOn; P3DS_TxOn = 0;
    if (!on || !P3DS_TxBuf) return;
    if (txOk < 0) { txOk = C3D_TexInit(&txTex, 512, 256, GPU_RGBA8) ? 1 : 0; if (txOk) { C3D_TexSetFilter(&txTex, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&txTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); P3DS_TxDirty = 1; } else P3DS_TxFailed = 1;
        PSPNativeMemLog("[TEXT] native text texture %s", txOk ? "ready" : "FAILED (text stays in the DS picture)"); }
    if (!txOk) return;
    if (P3DS_TxDirty) { P3DS_TxDirty = 0; P3DS_DcFlush(P3DS_TxBuf, 512 * 256 * 4);
        C3D_SyncDisplayTransfer(P3DS_TxBuf, GX_BUFFER_DIM(512, 256), txTex.data, GX_BUFFER_DIM(512, 256), TEX_FLAGS); }
    C3D_FrameDrawOn(t);
    C3D_Mtx proj; Mtx_OrthoTilt(&proj, 0.0f, 320.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, &txTex);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ZERO);
    const float s1 = 320.0f / 512.0f, t1 = 240.0f / 256.0f;
    C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
    C3D_ImmSendAttrib(0, 0, 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0);
    C3D_ImmSendAttrib(0, 240, 0, 0); C3D_ImmSendAttrib(0, t1, 0, 0);
    C3D_ImmSendAttrib(320, 0, 0, 0); C3D_ImmSendAttrib(s1, 0, 0, 0);
    C3D_ImmSendAttrib(320, 240, 0, 0); C3D_ImmSendAttrib(s1, t1, 0, 0);
    C3D_ImmDrawEnd();
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
}
/* 2026-11-18/19 sharp game text (ss3ds_gametext.c, patch 0289): the game's text at the final resolution, one layer per screen.
   P3DS_GtRender fills the layer's buffer (512x256 words R<<24|G<<16|B<<8|A; 400x240 used on the top, 320x240 on the bottom) and
   says whether it changed; drawn 1:1 NEAREST, alpha-blended. Top: both eyes at screen depth, right after the picture and its
   strips, under the toasts and the list cursor. Bottom: right after the stretched DS bottom, under the panel text, the panel
   picture and the native column. The buffers and textures are made on the render thread (P3DS_GtLayerReady, from P3DS_GtSync);
   if one cannot be made the game's text stays in the DS picture on that screen. */
typedef struct { u32 *buf; C3D_Tex ct; int ok; } GtLayer;
static GtLayer gtL[2] = { { NULL, {0}, -1 }, { NULL, {0}, -1 } };
int P3DS_GtRender(int scr, u32 *buf, float fx, float fy, float fw, float fh, int *dirty);
int P3DS_GtLayerReady(int scr) {
    GtLayer *L = &gtL[scr & 1];
    if (L->ok >= 0) return L->ok;
    if (!(scr ? tgtBot : tgtTop)) return 0;
    L->buf = linearAlloc(512 * 256 * 4);
    if (L->buf) { memset(L->buf, 0, 512 * 256 * 4); L->ok = C3D_TexInit(&L->ct, 512, 256, GPU_RGBA8) ? 1 : 0; } else L->ok = 0;
    if (L->ok) { C3D_TexSetFilter(&L->ct, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&L->ct, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); }
    else if (L->buf) { linearFree(L->buf); L->buf = NULL; }
    PSPNativeMemLog("[GTEXT] native %s text layer %s", scr ? "bottom" : "top", L->ok ? "ready" : "FAILED (game text stays in the DS picture)");
    return L->ok;
}
static void gtDraw(int scr, C3D_RenderTarget *t, float fx, float fy, float fw, float fh) {
    GtLayer *L = &gtL[scr];
    if (L->ok <= 0 || !L->buf || !t) return;
    int dirty = 0; if (!P3DS_GtRender(scr, L->buf, fx, fy, fw, fh, &dirty)) return;
    if (dirty) { P3DS_DcFlush(L->buf, 512 * 256 * 4);
        C3D_SyncDisplayTransfer(L->buf, GX_BUFFER_DIM(512, 256), L->ct.data, GX_BUFFER_DIM(512, 256), TEX_FLAGS); }
    const float W = scr ? 320.0f : 400.0f;
    C3D_FrameDrawOn(t);
    C3D_Mtx proj; Mtx_OrthoTilt(&proj, 0.0f, W, 240.0f, 0.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, &L->ct);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ZERO, GPU_ONE);
    const float s1 = W / 512.0f, t1 = 240.0f / 256.0f;
    C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
    C3D_ImmSendAttrib(0, 0, 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0);
    C3D_ImmSendAttrib(0, 240, 0, 0); C3D_ImmSendAttrib(0, t1, 0, 0);
    C3D_ImmSendAttrib(W, 0, 0, 0); C3D_ImmSendAttrib(s1, 0, 0, 0);
    C3D_ImmSendAttrib(W, 240, 0, 0); C3D_ImmSendAttrib(s1, t1, 0, 0);
    C3D_ImmDrawEnd();
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
}
static void topTxDraw(C3D_RenderTarget *t, float fx, float fy, float fw, float fh) { gtDraw(0, t, fx, fy, fw, fh); }
/* ---- fps60 (2026-11-15; see g3gpu.c): Settings > Display > 60 fps field. 0 OFF (default), 1 ON, 2 A/B TEST = OFF and ON by turns, 1200
   field ticks (~40 s) each, so one log compares both. The game still runs 30 ticks a second; while it is on, every tick whose 3D moved
   shows an in-between picture one V-blank before the real one. Only in the overworld, only when the tick's picture stays on the GPU
   (GPU 3D direct + direct screen); any other tick is presented exactly as before. If ticks keep taking longer than two V-blanks with it
   on, it rests for a while by itself (longer each time) and says so in the log. ---- */
extern int P3DS_InField, P3DS_InBattle; extern unsigned G3G_MidWhy[6];
static int f60Cfg = -1, f60Did; static unsigned f60Phase, f60Pause, f60Ov = 6000, f60MidUs;
static unsigned f60FlipLate, f60FlipN, f60FlipUs, f60LateTotal, f60DownTotal;
static int gZoomSet; static float gFill[4];
int P3DS_Fps60Cfg(void) {
    if (f60Cfg < 0) { f60Cfg = P3DS_CfgInt("fps60", 0); if (f60Cfg < 0 || f60Cfg > 2) f60Cfg = 0;
        PSPNativeMemLog("[FPS60] fps60=%d (0 off, 1 = in-between 3D frames in the overworld, 2 = A/B test: off and on by turns every 1200 ticks)", f60Cfg); }
    return f60Cfg; }
void P3DS_SetFps60(int v) { f60Cfg = v < 0 ? 0 : v > 2 ? 2 : v; f60Phase = 0; f60Pause = 0; f60Ov = 6000; PSPNativeMemLog("[FPS60] fps60=%d from settings", f60Cfg); }
static int f60PhaseOn(void) { const int c = P3DS_Fps60Cfg(); return c == 1 || (c == 2 && ((f60Phase / 1200) & 1)); }
int P3DS_Fps60Active(void) { return P3DS_Fps60Cfg() && P3DS_InField && !P3DS_InBattle && !f60Pause && f60PhaseOn(); }
/* once per presented overworld tick while the setting is not OFF. shown: an in-between picture went out. why (when not): 1 no in-between 3D
   this tick (g3gpu.c says which), 2 the picture went through the CPU (toast, save box, fade...), 3 a held or menu picture.
   Step-down (second version): a tick that runs long no longer costs a whole extra V-blank, the pictures just come a little late. So it
   only steps down to 30 where the game itself would run slow (60-tick average over 34.5 ms, or half of the ticks doing over 33.3 ms of work; run 906: 20 of 60 with a 33.4 ms average ran at 30 for hundreds of ticks for nothing), and
   it comes back by measurement, not by a timer: at 30 the spare time per tick (V-blank idle) is known, and once that is back above
   what the in-between picture costs (f60Ov, learned: +1.5 ms when a comeback fails within 6 s, -0.5 ms after 20 s that hold) it is on again. */
/* Measure the work between presents separately from time spent in the
   game's VBlank waits. A short/long cadence can still average 30 ticks/s. */
int P3DS_Fps60Skip; static unsigned f60NSkip;
/* 2026-11-17 fps60_noskip (Debug > 60 fps after slow tick, default SKIP = old behaviour): run 913 lost 90-114 of 600 in-between
   pictures to "skipped" (the tick after one over 33.3 ms busy). DRAW keeps the in-between there too (the step-down still guards
   real overload). A/B TEST switches SKIP/DRAW every 600 ticks, one [FPS60] line each, marked "slow-tick SKIP/DRAW". */
static int f60NoSkipCfg = -1, f60NoSkipWin;
int P3DS_Fps60NoSkipCfg(void) { if (f60NoSkipCfg < 0) { f60NoSkipCfg = 0;   /* fps60_noskip ignored since run 916 */ if (f60NoSkipCfg < 0 || f60NoSkipCfg > 2) f60NoSkipCfg = 0;
    PSPNativeMemLog("[FPS60] fps60_noskip=%d (0 skip the in-between after a slow tick, 1 draw it anyway, 2 A/B every 600 ticks)", f60NoSkipCfg); } return f60NoSkipCfg; }
void P3DS_SetFps60NoSkip(int v) { f60NoSkipCfg = v < 0 || v > 2 ? 0 : v; PSPNativeMemLog("[FPS60] fps60_noskip=%d from settings", f60NoSkipCfg); }
static int f60NoSkipOn(void) { return 0; }   /* run 916 A/B: DRAW sent the field to 30 fps for 390-570 of 600 ticks (in-between 4-34%) vs SKIP 46-56%: SKIP kept, setting removed */
void P3DS_Fps60Tick(unsigned frameUs, unsigned idleUs, unsigned fenceIdleUs, int shown, int why, unsigned composeUs, unsigned stripUs) {
    static unsigned n, nShown, nSlow, nWhy[4], nPaused, wN, wSlow, wUs, wFenceIdle, rN, rIdle, sinceOn, downs, ups;
    static u64 sumUs, sumMid, sumComp, sumStrip, idleWindow;
    const int act = P3DS_Fps60Active();
    // A completed display swap wait is pacing time. Keep it in elapsed time
    // and keep the recovery budget tied to the game's own VBlank idle.
    const unsigned afterGameIdle = frameUs > idleUs ? frameUs - idleUs : 0;
    const unsigned busyUs = afterGameIdle > fenceIdleUs ? afterGameIdle - fenceIdleUs : 0;
    const int overBudget = busyUs > 33333;
    if (P3DS_Fps60Skip) f60NSkip++;
    P3DS_Fps60Skip = act && !P3DS_Fps60Skip && (overBudget || frameUs > 50000) && !(f60NoSkipOn() && frameUs <= 50000);
    if (shown && !f60Did) { shown = 0; why = 3; }
    idleWindow += idleUs;
    n++; sumUs += frameUs; if (frameUs > 36000) nSlow++;
    if (shown) { nShown++; sumMid += f60MidUs; sumComp += composeUs; sumStrip += stripUs; }
    else if (f60Pause) nPaused++; else if (act) nWhy[why & 3]++;
    if (act) { wN++; wFenceIdle += fenceIdleUs; wUs += frameUs > 100000 ? 100000 : frameUs; if (overBudget) wSlow++; sinceOn++;
        if (sinceOn == 600 && f60Ov > 4000) f60Ov -= 500;
        if (wN >= 60) { if (wSlow > 30 || wUs / 60 > 34500) { f60Pause = 1; rN = rIdle = 0; downs++;
                if (sinceOn < 180 && f60Ov < 14000) f60Ov += 1500;
                f60DownTotal++;
                PSPNativeMemLog("[FPS60] too heavy here (%u of 60 ticks busy over 33.3 ms, avg tick %u us, display wait %u us/tick): 30 fps until the spare time per tick is back over %u us", wSlow, wUs / 60, wFenceIdle / 60, f60Ov + 1500); }
            wN = wSlow = wUs = wFenceIdle = 0; } }
    else if (f60Pause) { wN = wSlow = wUs = wFenceIdle = 0; rN++; rIdle += idleUs > 33000 ? 33000 : idleUs;
        if (rN % 30 == 0) { const unsigned spare = rIdle / 30; rIdle = 0;
            if (rN >= 90 && spare > f60Ov + 1500) { f60Pause = 0; sinceOn = 0; ups++; PSPNativeMemLog("[FPS60] back on (spare time %u us per tick)", spare); } } }
    f60Phase++;
    if (n >= 600 || (P3DS_Fps60Cfg() == 2 && f60Phase % 1200 == 0)) {
        const unsigned k = nShown ? nShown : 1;
        PSPNativeMemLog("[FPS60] %s%s%s | %u ticks: in-between %u (%u%%) | avg %u.%02u ms, %u>36 | idle %u us | why not: no3D %u, unpaired %u, nomem %u, nothingmoved %u, runs %u, fog %u, notdirect %u, cpu %u, menu %u, at30 %u | skipped %u | stepdown %u back %u cost %u | per frame: comp %u strips %u draw %u us | handed over: %u in time (avg %u us), %u late",
            (P3DS_Fps60Cfg() == 1 || (((f60Phase - 1) / 1200) & 1)) ? "ON" : "OFF", P3DS_Fps60Cfg() == 2 ? " (A/B)" : "", P3DS_Fps60NoSkipCfg() ? (f60NoSkipOn() ? " slow-tick DRAW" : " slow-tick SKIP") : "", n, nShown, nShown * 100 / n, (unsigned)(sumUs / n / 1000), (unsigned)(sumUs / n % 1000 / 10), nSlow,
            (unsigned)(idleWindow / n),
            nWhy[1], G3G_MidWhy[4], G3G_MidWhy[5], G3G_MidWhy[3], G3G_MidWhy[1], G3G_MidWhy[2], G3G_MidWhy[0], nWhy[2], nWhy[3], nPaused,
            f60NSkip, downs, ups, f60Ov, (unsigned)(sumComp / k), (unsigned)(sumStrip / k), (unsigned)(sumMid / k),
            f60FlipN - f60FlipLate, f60FlipN ? f60FlipUs / f60FlipN : 0, f60FlipLate);
        f60NoSkipWin++; f60NSkip = 0; idleWindow = 0; n = nShown = nSlow = nPaused = downs = ups = 0; f60FlipN = f60FlipLate = f60FlipUs = 0; memset(nWhy, 0, sizeof nWhy); memset(G3G_MidWhy, 0, 6 * sizeof(unsigned)); sumUs = sumMid = sumComp = sumStrip = 0; }
}
/* One line per area. 300 presented field ticks is about 10 s at 30 Hz.
   OFF returns before any of this and writes nothing. */
extern int P3DS_CurMap;
extern void P3DS_ZoneInfo(int *, int *, int *, unsigned *);
extern const char *P3DS_AreaName(int);
extern unsigned G3G_PipeKicks, P3DS_GcBeginUs;
extern int P3DS_Fps60ScreenPlace(void);
extern int P3DS_Fps60MidPlace(void);
static int areaCfg = -1;
static int fps60AreaOn(void) {
    if (areaCfg < 0) { areaCfg = P3DS_CfgInt("fps60_area_dbg", 0) != 0;
        PSPNativeMemLog("[FPS60] fps60_area=%d", areaCfg); }
    return areaCfg;
}
void P3DS_SetFps60Area(int v) { areaCfg = v != 0; PSPNativeMemLog("[FPS60] fps60_area=%d from settings", areaCfg); }
enum { AREA_TICKS = 300 };
void P3DS_Fps60Area(unsigned frameUs, int shown) {
    static int zone0, map0, open;
    static unsigned ticks, shownN, worst, beginN, pipe0, down0, late0;
    static u64 sumUs, sumBegin;
    if (!fps60AreaOn()) { open = 0; return; }
    int zc, zp, zl; unsigned zms; P3DS_ZoneInfo(&zc, &zp, &zl, &zms); (void)zp; (void)zl; (void)zms;
    const int map = P3DS_CurMap;
    const int due = open && (zc != zone0 || map != map0 || ticks >= AREA_TICKS);
    if (due) {
        const unsigned n = ticks ? ticks : 1;
        const int sp = P3DS_Fps60ScreenPlace();
        PSPNativeMemLog("[FPS60] area zone %d %s map %d ticks %u in-between %u%% late %u stepdowns %u avg %u us worst %u us begin %u us pipe %u screen %s mid %s",
            zone0, P3DS_AreaName(zone0), map0, ticks, shownN * 100 / n,
            f60LateTotal - late0, f60DownTotal - down0, (unsigned)(sumUs / n), worst,
            beginN ? (unsigned)(sumBegin / beginN) : 0, G3G_PipeKicks - pipe0,
            sp == 1 ? "VRAM" : sp == 2 ? "linear" : "none",
            P3DS_Fps60MidPlace() ? "VRAM" : "none");
        open = 0;
    }
    if (!open) { open = 1; zone0 = zc; map0 = map; ticks = shownN = worst = beginN = 0; sumUs = sumBegin = 0;
        pipe0 = G3G_PipeKicks; down0 = f60DownTotal; late0 = f60LateTotal; }
    ticks++; sumUs += frameUs; if (frameUs > worst) worst = frameUs; if (shown) shownN++;
    if (P3DS_GcBeginUs) { sumBegin += P3DS_GcBeginUs; beginN++; }
}
/* The in-between picture and the real one are drawn in ONE GPU frame by P3DS_GpuPresent: the in-between goes into the screen targets
   and out to the LCD at that frame's end; the real picture goes into a second pair of targets (tgtTopB / tgtTopRB, not linked to a
   screen) and is only handed to the LCD ("flipped") one V-blank later, by P3DS_Fps60Poll from the game thread while the next tick
   is already running. So the game never sits waiting between the two pictures (first version, run 835 log: the 2.3 ms wait for the
   GPU plus the wait for the V-blank between them made a 30 ms tick take 50). P3DS_GpuPresentMid only notes what to draw. */
/* RGB8 (288000 bytes each, the LCD is 24 bit anyway) instead of RGBA8 (384000): Nathan's hardware log with the 3D slider up had 375808 bytes of VRAM free
   for the right eye's second target, 8 KB short, so stereo never got an in-between picture. */
#define DISP_FLAGS_B (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
static C3D_RenderTarget *tgtTopB, *tgtTopRB; static int f60BFail;
/* fps60_vram: place both 288000-byte screen targets and both 196608-byte
   in-between textures while the holes are still big enough for that set.
   Any miss frees the whole set, so a VRAM-required target is never the one
   that loses. A later required alloc can release the set and retry once. */
static C3D_RenderTarget *vramScr[2];
static int vramCfg = -1, vramTried, vramHeld, f60VramN, f60LinN;
extern int P3DS_GcMidReserve(void);
extern void P3DS_GcMidRelease(void);
static int fps60VramOn(void) {
    if (vramCfg < 0) { vramCfg = 0; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x;
        if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "fps60_vram=%d", &x) == 1) vramCfg = x != 0; fclose(f); }
        PSPNativeMemLog("[FPS60] fps60_vram=%d", vramCfg); }
    return vramCfg;
}
void P3DS_SetFps60Vram(int v) {
    vramCfg = v != 0;
    PSPNativeMemLog("[FPS60] fps60_vram=%d from settings", vramCfg);
    if (!vramCfg && vramHeld) P3DS_Fps60VramRelease();
    else if (vramCfg) vramTried = 0;
}
int P3DS_Fps60VramRelease(void) {
    if (!vramHeld) return 0;
    C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH);
    if (tgtTopB == vramScr[0]) tgtTopB = NULL;
    if (tgtTopRB == vramScr[1]) tgtTopRB = NULL;
    for (int i = 0; i < 2; i++) { if (vramScr[i]) C3D_RenderTargetDelete(vramScr[i]); vramScr[i] = NULL; }
    P3DS_GcMidRelease();
    vramHeld = 0; f60BFail = 0;
    PSPNativeMemLog("[FPS60] vram reserve released so a required target can fit (free %lu)", (unsigned long)vramSpaceFree());
    return 1;
}
int P3DS_Fps60VramPrepare(void) {
    if (vramTried) return vramHeld;
    vramTried = 1;
    if (!fps60VramOn()) return 0;
    vramScr[0] = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, -1);
    vramScr[1] = vramScr[0] ? C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, -1) : NULL;
    if (!vramScr[0] || !vramScr[1] || !P3DS_GcMidReserve()) {
        for (int i = 0; i < 2; i++) { if (vramScr[i]) C3D_RenderTargetDelete(vramScr[i]); vramScr[i] = NULL; }
        P3DS_GcMidRelease();
        PSPNativeMemLog("[FPS60] vram reserve fallback (free %lu): second screen stays on the old path", (unsigned long)vramSpaceFree());
        return 0;
    }
    vramHeld = 1;
    PSPNativeMemLog("[FPS60] vram reserve ok: both 288000 screen targets and both 196608 in-between textures in VRAM (free %lu)", (unsigned long)vramSpaceFree());
    return 1;
}
/* 1 = both second-screen colour buffers are in VRAM, 2 = at least one is linear, 0 = not made yet */
int P3DS_Fps60ScreenPlace(void) {
    if (vramHeld) return 1;
    if (f60LinN) return 2;
    if (f60VramN) return 1;
    return 0;
}
/* Second screen target. VRAM is two 3 MB banks and one target must sit inside one bank: log of 2026-11-15 had 471808 bytes free in total
   but not 288000 in a single bank (stereo, right eye). Then the colour buffer goes to linear RAM (the PICA renders to FCRAM; same trick as
   the res3d linear target in g3gpu.c: target made from a tiny VRAM texture, colour pointer replaced). */
static C3D_RenderTarget *f60MakeB(void) {
    C3D_RenderTarget *t = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, -1);
    if (t) { f60VramN++; return t; }
    static C3D_Tex dumTex; static int dumOk = -1;
    if (dumOk < 0) dumOk = C3D_TexInitVRAM(&dumTex, 8, 8, GPU_RGBA8) ? 1 : 0;
    void *col = dumOk ? linearMemAlign(240 * 400 * 3, 0x80) : NULL;
    if (col && (t = C3D_RenderTargetCreateFromTex(&dumTex, GPU_TEXFACE_2D, 0, -1))) {
        C3D_FrameBufColor(&t->frameBuf, col, GPU_RB_RGB8); C3D_FrameBufAttrib(&t->frameBuf, 240, 400, false);
        f60LinN++;
        PSPNativeMemLog("[FPS60] second screen target in linear RAM (VRAM free %lu, no single bank block of 288000)", (unsigned long)vramSpaceFree());
        return t; }
    if (col) linearFree(col);
    return NULL;
}
static struct { int on, mode, st; C3D_Tex *l, *r; const u32 *sl, *sr; } f60Arg;
static int f60Flip, f60FlipSt; static u64 f60FlipT0, f60PollT;
int P3DS_GpuPresentMid(C3D_Tex *l, C3D_Tex *r, const u32 *sl, const u32 *sr, int mode, int st) {
    f60Arg.on = 0;
    if (!ready || upFmt16 < 0 || !l || (st && !r) || (mode == 2 && !gZoomSet) || f60BFail) return 0;
    if (!tgtTopB) tgtTopB = (vramHeld && vramScr[0]) ? vramScr[0] : f60MakeB();
    if (st && !tgtTopRB) tgtTopRB = (vramHeld && vramScr[1]) ? vramScr[1] : f60MakeB();
    if (!tgtTopB || (st && !tgtTopRB)) { f60BFail = 1; PSPNativeMemLog("[FPS60] no VRAM for the second screen target (free %lu): no in-between frames", (unsigned long)vramSpaceFree()); return 0; }
    f60Arg.on = 1; f60Arg.mode = mode; f60Arg.st = st; f60Arg.l = l; f60Arg.r = st ? r : NULL; f60Arg.sl = sl; f60Arg.sr = sr;
    return 1;
}
/* inside a begun frame: the real picture waiting in tgtTopB (and tgtTopRB) goes to the LCD */
static void f60FlipNow(void) {
    C3D_RenderTargetSetOutput(tgtTopB, GFX_TOP, GFX_LEFT, DISP_FLAGS_B); C3D_FrameDrawOn(tgtTopB);
    if (f60Flip == 2) { C3D_RenderTargetSetOutput(tgtTopRB, GFX_TOP, GFX_RIGHT, DISP_FLAGS_B); C3D_FrameDrawOn(tgtTopRB); }
    P3DS_FrameEnd();
    C3D_RenderTargetSetOutput(tgtTop, GFX_TOP, GFX_LEFT, DISP_FLAGS); C3D_RenderTargetSetOutput(tgtTopR, GFX_TOP, GFX_RIGHT, DISP_FLAGS);
    f60FlipUs += (unsigned)((svcGetSystemTick() - f60FlipT0) / 268); f60FlipN++; f60Flip = 0;
}
static int f60Pending(void) { return gspIsPresentPending(GFX_TOP) || gspIsPresentPending(GFX_BOTTOM); }
/* game thread, any time no GPU frame is open (per 3D draw call, at the V-blank wait, before the 3D frame ends): flip as soon as the GPU
   has finished the two pictures AND the LCD has taken the in-between one (a flip before that would land in the buffer being shown) */
int P3DS_Fps60Pending(void) { return f60Flip; }
extern int P3DS_G3ThrBusy(void); /* g3soft: worker has queued jobs; do not open a C3D frame over it */
/* 2026-11-17 screen hand-off (always on since run 921). Run 919: tearfix waited 4-7 ms on
   54-579 of 600 presents, with the game thread blocked AND the GPU idle (the frame's draw commands are only handed to the GPU at
   FrameEnd, after that wait). The only step that must wait for the LCD is the copy into the screen buffer (display transfer). With
   ON, when the last screen swap is still pending, the frame ends WITHOUT that copy: the GPU draws at once into tgtTop/tgtTopR/tgtBot
   (off-screen VRAM targets, never scanned out) and the copy + swap is started later by P3DS_Fps60Poll (game thread, at every VBlank
   wait and 3D draw call) as soon as GSP has taken the previous swap. Same pictures, same order: an fps60 real-picture flip waits
   behind it. Anything that presents next (P3DS_Fps60Flush) first finishes it the old way (wait, then copy), so the worst case is
   the old behaviour. Field only (where the poll is proven by fps60). */
static int dfrCfg = -1, dfrMask, dfrSt, dfrUse; static u64 dfrT0; static unsigned dfrPhase;
static unsigned dfrN, dfrLate, dfrPolled, dfrNow, dfrMid, dfrSlow; static u64 dfrUsSum, dfrSaved, dfrGapSum, dfrLastEnd;
/* run 921 A/B (Nathan, stereo + Wide, same area): OFF 73% in-between, 103 skipped, 0.6 ms spare per tick; ON 77%, 84 skipped,
   3.6 ms spare, no hang. Always on since then, setting removed (config key present_defer ignored). */
int P3DS_PresentDeferCfg(void) { if (dfrCfg < 0) { dfrCfg = 1; PSPNativeMemLog("[DEFER] screen hand-off always on (GPU starts at once, the screen copy waits for the LCD instead of the game)"); } return dfrCfg; }
static int dfrOn(void) { const int c = P3DS_PresentDeferCfg(); return c == 1 || (c == 2 && ((dfrPhase / 1200) & 1)); }
/* inside a begun frame: copy the waiting targets to the screens and swap */
static void dfrCopyNow(void) {
    if (dfrMask & 1) C3D_FrameDrawOn(tgtTop);
    if (dfrMask & 2) C3D_FrameDrawOn(tgtTopR);
    if (dfrMask & 4) C3D_FrameDrawOn(tgtBot);
    P3DS_FrameEnd();
    dfrUsSum += (svcGetSystemTick() - dfrT0) / 268; dfrN++; dfrMask = 0; dfrSt = 0;
}
static int dfrPoll(void) { /* 1 = still waiting (nothing else may present) */
    if (!dfrMask) return 0;
    if (dfrSt == 0) { if (!C3D_FrameBegin(C3D_FRAME_NONBLOCK)) return 1; dfrSt = 1;
        if (f60Pending()) { C3D_FrameEnd(GX_CMDLIST_FLUSH); return 1; }
        dfrPolled++; dfrCopyNow(); return 1; }  /* the f60 flip waits for this swap to be taken */
    if (f60Pending()) return 1;
    if (!C3D_FrameBegin(C3D_FRAME_NONBLOCK)) return 1;
    dfrPolled++; dfrCopyNow(); return 1;
}
static void dfrFlush(void) {
    if (!dfrMask) return;
    C3D_FrameBegin(0);
    const u64 t0 = svcGetSystemTick(); while (f60Pending() && svcGetSystemTick() - t0 < (u64)(40 * 268111.856)) svcSleepThread(250000);
    { const unsigned w = (unsigned)((svcGetSystemTick() - t0) / 268); P3DS_GpTearUs += w; P3DS_GpFenceIdleUs += w; }
    dfrLate++; dfrCopyNow();
}
void P3DS_PresentDeferSettle(void) { dfrFlush(); }
void P3DS_Fps60Poll(void) {
    if (!f60Flip && !dfrMask) return;
    if (P3DS_G3ThrBusy()) return; /* g3_thread: FrameBegin while the worker fills vb[] reorders GPU work vs inline */
    const u64 now = svcGetSystemTick(); if (now - f60PollT < 268 * 300) return; f60PollT = now;
    if (dfrMask) { dfrPoll(); return; }
    if (!f60Flip) return;
    if (f60FlipSt == 0) { if (!C3D_FrameBegin(C3D_FRAME_NONBLOCK)) return; f60FlipSt = 1;
        if (f60Pending()) { C3D_FrameEnd(GX_CMDLIST_FLUSH); return; }
        f60FlipNow(); return; }
    if (f60Pending()) return;
    C3D_FrameBegin(0); f60FlipNow();
}
/* before anything else is presented: the flip that no poll got to (a short tick), waited for */
void P3DS_Fps60Flush(void) {
    dfrFlush();
    if (!f60Flip) return;
    C3D_FrameBegin(0);
    const u64 t0 = svcGetSystemTick(); while (f60Pending() && svcGetSystemTick() - t0 < (u64)(40 * 268111.856)) svcSleepThread(250000);
    f60FlipLate++; f60LateTotal++; f60FlipNow();
}
void P3DS_GpuPresent(const u32 *top, const u32 *bot, int mode, int drawBottom, const u32 *topR) {
    P3DS_GpFenceIdleUs = 0;
    P3DS_Crumb("gp-begin", 0); /* run 65: locate the boot stall */
    if (upFmt16 < 0 && !P3DS_UpAlloc(0)) { static unsigned n; if (!n++) PSPNativeMemLog("[3DRES] scale targets missing, frame not presented"); return; }
    P3DS_GpTearUs = 0; P3DS_GpStripUs = 0;
    P3DS_Fps60Flush();
    int st = topR != NULL; if (st != stereoOn) { stereoOn = st; gfxSet3D(st); }
    static int vs = -1; if (vs < 0) { vs = 0; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "vsync=%d", &x) == 1) vs = x != 0; fclose(f); } PSPNativeMemLog("[3DS] vsync (tear-free present) %d", vs); }
    { const u64 tb = svcGetSystemTick();
      const int pace = P3DS_PresentPaceCfg();
      if (pace) {
          /* NONBLOCK: if previous GPU work is done, start immediately (no SYNCDRAW half-rate snap).
             If still busy, wait for the queue only — never an extra vblank align. */
          if (C3D_FrameBegin(C3D_FRAME_NONBLOCK)) paceNbOk++;
          else { paceNbBlock++; C3D_FrameBegin(0); }
          if (++paceLogN >= 600) {
              PSPNativeMemLog("[PACE] 600f: nonblock ok %u, blocked %u (want ok>>blocked when near 60)", paceNbOk, paceNbBlock);
              paceNbOk = paceNbBlock = paceLogN = 0;
          }
      } else {
          C3D_FrameBegin(vs ? C3D_FRAME_SYNCDRAW : 0);
      }
      P3DS_GpBeginUs = (unsigned)((svcGetSystemTick() - tb) / 268); }
    dirTop = P3DS_GcPendTex(top); dirTopR = topR ? P3DS_GcPendTex(topR) : NULL; P3DS_GcPresented();
    if (srcDbl) srcSet ^= 1;  /* game already paces on vblank; SYNCDRAW added a 2nd vblank wait (30 fps cap) */
    /* run 25: upload AFTER FrameBegin. Before, the sync transfer raced the previous frame's queued
       render/transfer (PPF event from that job ended our wait early) and the GPU sampled a half-updated texture: tearing on both screens. */
    if (skipBotCfg < 0) { skipBotCfg = 1; /* run 47: default 1 again (safe: only skips when engine B was really reused) */ FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "present_skipbot=%d", &x) == 1) skipBotCfg = x != 0; fclose(f); } PSPNativeMemLog("[3DS] present_skipbot=%d", skipBotCfg); }
    const int botFresh = !(skipBotCfg && botValid && P3DS_GpuBotSame);
    if (!drawBottom) botValid = 0; else if (!botFresh) botSkips++;
    P3DS_Crumb("gp-upload", 0);
    upTicks = 0; if (!dirTop) upload(0, top); const int crisp = !topR && !dirTop && crispPrepare(top); crN++; if (drawBottom && botFresh) upload(1, bot); if (st && !dirTopR) upload(2, topR); P3DS_UploadUs = (unsigned)(upTicks / 268); P3DS_GpUpUs = P3DS_UploadUs;
    C3D_BindProgram(&prog);
    C3D_AttrInfo *ai = C3D_GetAttrInfo(); AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 2); AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2);
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR); C3D_CullFace(GPU_CULL_NONE); C3D_AlphaTest(false, GPU_ALWAYS, 0);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    for (int k = 1; k < 6; k++) C3D_TexEnvInit(C3D_GetTexEnv(k));
    /* fps60: the in-between picture first, into the screen targets; the real one then goes into the second pair */
    const int f60 = f60Arg.on && dirTop && (!st || dirTopR) && st == f60Arg.st && mode == f60Arg.mode && !crisp; f60Arg.on = 0; f60MidUs = 0; f60Did = f60;
    if (f60) { const u64 tm = svcGetSystemTick();
        C3D_Tex *const kt = dirTop, *const ktR = dirTopR; dirTop = f60Arg.l; dirTopR = f60Arg.r;
        prescale(0); if (st) prescale(2);
        const u32 *k0 = P3DS_WideStrip[0], *k1 = P3DS_WideStrip[1];
        if (k0 && f60Arg.sl) P3DS_WideStrip[0] = f60Arg.sl; if (k1 && f60Arg.sr) P3DS_WideStrip[1] = f60Arg.sr;
        const float gx = gFill[0], gy = gFill[1], gw = gFill[2], gh = gFill[3];
        stripMid = 1;
        if (mode == 2) quad(tgtTop, 400.0f, 0, gx, gy, gx + gw, gy + gh); else { quad(tgtTop, 400.0f, 0, 40, 0, 360, 240); strips(tgtTop, 0); }
        if (st) { if (mode == 2) quad(tgtTopR, 400.0f, 2, gx, gy, gx + gw, gy + gh); else { quad(tgtTopR, 400.0f, 2, 40, 0, 360, 240); strips(tgtTopR, 1); } }
        stripMid = 0;
        P3DS_WideStrip[0] = k0; P3DS_WideStrip[1] = k1;
        if (mode == 2) { topTxDraw(tgtTop, gx, gy, gw, gh); if (st) topTxDraw(tgtTopR, gx, gy, gw, gh); } else { topTxDraw(tgtTop, 40, 0, 320, 240); if (st) topTxDraw(tgtTopR, 40, 0, 320, 240); }
        if (P3DS_TopOvOn) { topOvDraw(tgtTop); if (st) topOvDraw(tgtTopR); }
        if (P3DS_TopOv2On) { topOv2Draw(tgtTop); if (st) topOv2Draw(tgtTopR); }
        dirTop = kt; dirTopR = ktR; tevPlain();
        f60MidUs = (unsigned)((svcGetSystemTick() - tm) / 268); }
    C3D_RenderTarget *const tT = f60 ? tgtTopB : tgtTop, *const tTR = f60 ? tgtTopRB : tgtTopR;
    P3DS_Crumb("gp-prescale", 0); const u64 tDr0 = svcGetSystemTick();
    prescale(0); if (drawBottom && botFresh) { prescale(1); botValid = 1; } if (st) prescale(2);
    { static unsigned n; if (++n % 600 == 0) { if(P3DS_LogLevel()>=2)PSPNativeMemLog("[3DS] present_skipbot: skipped bottom upload %u of 600", botSkips); botSkips = 0; } }
    /* run 33: mode 2 (fill) zoom from config gpuscale_zoom (1.25..1.5625, default 1.40625 = 360x270: 20px bars, 15 rows
       cropped top/bottom). Aspect always 4:3; edges snapped to whole pixels. */
    static float zoom = -1; if (zoom < 0) { gZoomSet = 1; zoom = 1.40625f; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; float z; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "gpuscale_zoom=%f", &z) == 1) zoom = z; fclose(f); } if (zoom < 1.25f) zoom = 1.25f; if (zoom > 1.5625f) zoom = 1.5625f; PSPNativeMemLog("[3DS] gpuscale_zoom=%.5f -> fill mode %dx%d", zoom, (int)(256 * zoom + 0.5f), (int)(192 * zoom + 0.5f)); }
    float fw = (float)(int)(256 * zoom + 0.5f), fh = (float)(int)(192 * zoom + 0.5f), fx = (float)(int)((400 - fw) / 2), fy = (float)(int)((240 - fh) / 2);
    gFill[0] = fx; gFill[1] = fy; gFill[2] = fw; gFill[3] = fh;
    u64 stT = 0;
    if (mode == 2) { quad(tT, 400.0f, 0, fx, fy, fx + fw, fy + fh); if (crisp) crispDraw(tT, 400.0f, fx, fy, fx + fw, fy + fh); } else { quad(tT, 400.0f, 0, 40, 0, 360, 240); if (crisp) crispDraw(tT, 400.0f, 40, 0, 360, 240); const u64 a = svcGetSystemTick(); strips(tT, 0); stT += svcGetSystemTick() - a; }
    if (st) { if (mode == 2) quad(tTR, 400.0f, 2, fx, fy, fx + fw, fy + fh); else { quad(tTR, 400.0f, 2, 40, 0, 360, 240); const u64 a = svcGetSystemTick(); strips(tTR, 1); stT += svcGetSystemTick() - a; } }
    if (mode == 2) { topTxDraw(tT, fx, fy, fw, fh); if (st) topTxDraw(tTR, fx, fy, fw, fh); } else { topTxDraw(tT, 40, 0, 320, 240); if (st) topTxDraw(tTR, 40, 0, 320, 240); }
    if (P3DS_TopOvOn) { P3DS_TopOvOn = 0; topOvDraw(tT); if (st) topOvDraw(tTR); }
    if (P3DS_TopOv2On) { P3DS_TopOv2On = 0; topOv2Draw(tT); if (st) topOv2Draw(tTR); }
    /* Platinum run 100 lower_menu: the native Start menu owns the bottom screen while it is drawn */
    if (drawBottom && P3DS_LowerMenuDraw && menuDraw()) { P3DS_PnOn = 0; P3DS_TxOn = 0; }
    else if (drawBottom) { quad(tgtBot, 320.0f, 1, 0, 0, 320, 240); gtDraw(1, tgtBot, 0, 0, 320, 240); txDraw(tgtBot); pnDraw(tgtBot); ovDraw(tgtBot); botFadeDraw(tgtBot); } else { P3DS_PnOn = 0; P3DS_TxOn = 0; }
    if (crN >= 600) { if (crOn) PSPNativeMemLog("[CRISP] %u of %u presents drew the 3D at final size, avg plain-3D cover %u%%, avg prep %u us", crOn, crN, (unsigned)(crMask / crOn * 100 / (256 * 192)), (unsigned)(crTicks / crOn / 268)); crN = crOn = 0; crMask = 0; crTicks = 0; }
    P3DS_GpStripUs = (unsigned)(stT / 268); P3DS_GpDrawUs = (unsigned)((svcGetSystemTick() - tDr0 - stT) / 268);
    { /* tearfix: citro3d requests the screen swap at the first vblank after the transfer and GSP latches it one vblank
         later. A new transfer inside that window lands in the buffer still being scanned out: the first rows show the
         new frame, the rest the old (run 27 report, both screens). Make sure ~2 vblanks passed since the last present. */
      static int tf = -1; if (tf < 0) { tf = 2; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "tearfix=%d", &x) == 1) tf = x < 0 ? 0 : x > 2 ? 2 : x; fclose(f); } PSPNativeMemLog("[3DS] tearfix=%d", tf); }
      static u64 lastEnd; static unsigned waits, n; static u64 waitTicks;
      dfrUse = 0;
      if (tf == 2 && P3DS_InField && !P3DS_InBattle && dfrOn() && f60Pending()) {
        /* present_defer: hand the draw to the GPU now, copy to the screens later (see dfrPoll) */
        dfrUse = 1; dfrMask = 0;
        if (tgtTop->used) { tgtTop->used = false; dfrMask |= 1; }
        if (tgtTopR && tgtTopR->used) { tgtTopR->used = false; dfrMask |= 2; }
        if (tgtBot->used) { tgtBot->used = false; dfrMask |= 4; }
      } else if (tf == 2) { /* run 32: tearfix=2 (default). C3D_FrameBegin above waited for the previous queue, so citro3d has
           requested the previous swap (gfxScreenSwapBuffers -> gspPresentBuffer); GSP applies it only at the next
           vblank. Until then the "back" buffer that this frame's display transfer writes is STILL ON SCREEN: the
           transfer overwrites it while the LCD scans it -> the first-scanned rows lag (both screens, also 2D video).
           Wait exactly until GSP has taken the previous present, no fixed vblank guesses. */
        P3DS_Crumb("gp-tearfix", 0);
        const u64 t0 = svcGetSystemTick(), timeout = 40 * CPU_TICKS_PER_MSEC;
        int w = 0;
        while ((gspIsPresentPending(GFX_TOP) || gspIsPresentPending(GFX_BOTTOM)) &&
               svcGetSystemTick() - t0 < timeout) { w = 1; svcSleepThread(250000); }
        const u64 elapsed = svcGetSystemTick() - t0;
        P3DS_GpTearUs = (unsigned)(elapsed / CPU_TICKS_PER_USEC);
        if (w) {
            waits++; waitTicks += elapsed;
            if (elapsed < timeout) P3DS_GpFenceIdleUs = P3DS_GpTearUs;
        }
      } else if (tf && lastEnd) { u64 el = svcGetSystemTick() - lastEnd; if (el < (u64)(2 * 16.713 * 268111.856)) { if (el < (u64)(16.713 * 268111.856)) gspWaitForVBlank(); gspWaitForVBlank(); waits++; } }
      if (P3DS_PresentDeferCfg()) { const int ab = P3DS_PresentDeferCfg() == 2; dfrPhase++;
        if (dfrUse) dfrNow++;
        if (f60) dfrMid++;
        { const u64 now = svcGetSystemTick(); if (dfrLastEnd) { const u64 g = now - dfrLastEnd; dfrGapSum += g; if (g > (u64)(34 * 268111.856)) dfrSlow++; } dfrLastEnd = now; }
        if (++dfrSaved >= 600 || (ab && dfrPhase % 1200 == 0)) {
          PSPNativeMemLog("[DEFER] %s%s | %u presents, avg %u us apart, %u over 34 ms | in-between pictures %u (%u%%) | tearfix waited on %u (avg %u us) | GPU started at once %u: copied by poll %u, by next present %u, avg %u us later",
            (dfrCfg == 1 || (((dfrPhase - 1) / 1200) & 1)) ? "ON" : "OFF", ab ? " (A/B)" : "", (unsigned)dfrSaved, (unsigned)(dfrGapSum / dfrSaved / 268), dfrSlow,
            dfrMid, (unsigned)(dfrMid * 100 / dfrSaved), waits, waits ? (unsigned)(waitTicks / waits / 268) : 0,
            dfrNow, dfrPolled, dfrLate, dfrN ? (unsigned)(dfrUsSum / dfrN) : 0);
          dfrSaved = 0; dfrNow = dfrPolled = dfrLate = dfrN = dfrMid = dfrSlow = 0; dfrUsSum = dfrGapSum = 0; waits = 0; waitTicks = 0; } }
      if (tf && ++n % 600 == 0) { PSPNativeMemLog("[3DS] tearfix=%d: waited on %u of 600 presents, avg wait %u us", tf, waits, waits ? (unsigned)(waitTicks / waits / 268) : 0); if (!P3DS_PresentDeferCfg()) { waits = 0; waitTicks = 0; } }
      if (f60 && tf != 2) { const u64 t0 = svcGetSystemTick(); while (f60Pending() && svcGetSystemTick() - t0 < (u64)(40 * 268111.856)) svcSleepThread(250000); }
      P3DS_FpsGuardFrame(); P3DS_Crumb("gp-end", 0); { const u64 te = svcGetSystemTick(); P3DS_FrameEnd(); lastEnd = svcGetSystemTick(); P3DS_GpEndUs = (unsigned)((lastEnd - te) / 268); } P3DS_Crumb("gp-done", 0);
      if (dfrUse) { dfrT0 = lastEnd; dfrSt = 0; f60PollT = 0; }
      if (f60) { f60Flip = st ? 2 : 1; f60FlipSt = 0; f60FlipT0 = lastEnd; f60PollT = 0; } }
}

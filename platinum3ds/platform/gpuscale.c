#include <stdio.h>
extern void PSPNativeMemLog(const char *, ...);
/* platinum3ds GPU screen scaling (citro3d): each DS screen is nearest-upscaled 2x on the CPU into a 512x384
 * (v2: no CPU prescale) 256x192 RGBA8 into a 256x256 texture via GX display transfer, drawn as one
 * NEAREST-filtered quad scaled by the GPU (crisp).
 * Modes: 1 = bottom 320x240, top 320x240 pillarboxed; 2 = bottom 320x240, top stretched 400x240. */
#include <3ds.h>
#include <citro3d.h>
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
#define DISP_FLAGS (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
#define TEX_FLAGS (GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
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
    ready = 1; return 1;
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
        GSPGPU_FlushDataCache(src, TEXW * 192 * 4);
        C3D_SyncDisplayTransfer((u32 *)src, GX_BUFFER_DIM(TEXW, TEXH), tex[i].data, GX_BUFFER_DIM(TEXW, TEXH), TEX_FLAGS);
        upTicks += svcGetSystemTick() - t0; return; }
    swz[i] = 0;
    u32 *d = lin[i];
    for (int k = 0; k < 256 * 192; k++) { u32 c = src[k]; d[k] = __builtin_bswap32(c) | 0xFF;  /* == R<<24|G<<16|B<<8|FF (box-verified), one REV */ }
    GSPGPU_FlushDataCache(d, TEXW * 192 * 4);
    C3D_SyncDisplayTransfer(d, GX_BUFFER_DIM(TEXW, TEXH), tex[i].data, GX_BUFFER_DIM(TEXW, TEXH), TEX_FLAGS);
    upTicks += svcGetSystemTick() - t0;
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
static void prescale(int i) {
    if (swz[i]) tevSwizzle();
    C3D_RenderTargetClear(upTgt[i], C3D_CLEAR_ALL, 0x000000FF, 0);
    C3D_FrameDrawOn(upTgt[i]);
    C3D_Mtx proj; Mtx_Ortho(&proj, 0.0f, 512.0f, 0.0f, 512.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, &tex[i]);
    const float t1 = 192.0f / TEXH;
    C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
    C3D_ImmSendAttrib(0, 0, 0, 0); C3D_ImmSendAttrib(0, 0, 0, 0);
    C3D_ImmSendAttrib(512, 0, 0, 0); C3D_ImmSendAttrib(1, 0, 0, 0);
    C3D_ImmSendAttrib(0, 512 * t1, 0, 0); C3D_ImmSendAttrib(0, t1, 0, 0);
    C3D_ImmSendAttrib(512, 512 * t1, 0, 0); C3D_ImmSendAttrib(1, t1, 0, 0);
    C3D_ImmDrawEnd();
    if (swz[i]) tevPlain();
}
static void quad(C3D_RenderTarget *t, float sw, int i, float x0, float y0, float x1, float y1) {
    C3D_RenderTargetClear(t, C3D_CLEAR_ALL, 0x000000FF, 0);
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
/* run 43 wideview: side strips of the wide 3D pass (g3gpu G3G_Strip, 64x192 RGBA8: left 32 | right 32 DS px), drawn
   at x 0..40 and 360..400 (1.25x, LINEAR) after the centred picture. P3DS_WideStrip[eye] = NULL: not drawn. */
const u32 *P3DS_WideStrip[2];
static C3D_Tex stripTex[2]; static u32 *stripLin[2]; static int stripOk = -1;
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
        if (eye == 0 || now - lastT > 268111 * 8) { int d; unsigned a; fadeNow(&d, &a); lastT = now;
            if (d && a) { hDir = d; hAmt = a; hLeft = 3; } else if (hLeft) { hLeft--; if (!hLeft) hAmt = 0; } }
        if (hAmt) { fDir = hDir; fAmt = hAmt; } }
    if (stripOk < 0) { stripOk = 1; for (int e = 0; e < 2; e++) { if (!C3D_TexInit(&stripTex[e], 64, 256, GPU_RGBA8) || !(stripLin[e] = linearAlloc(64 * 256 * 4))) { stripOk = 0; break; }
        C3D_TexSetFilter(&stripTex[e], GPU_LINEAR, GPU_LINEAR); C3D_TexSetWrap(&stripTex[e], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); memset(stripLin[e], 0, 64 * 256 * 4); }
        PSPNativeMemLog("[WIDE] strip textures %s", stripOk ? "ok" : "FAILED"); }
    if (!stripOk) return;
    if (!fAmt) memcpy(stripLin[eye], src, 64 * 192 * 4);
    else if (fAmt >= 16) { const u32 c = fDir == 1 ? 0xFFFFFFFFu : 0x000000FFu; for (unsigned i = 0; i < 64 * 192; i++) stripLin[eye][i] = c; }
    else for (unsigned i = 0; i < 64 * 192; i++) { u32 c = src[i]; unsigned r = c >> 24, g = (c >> 16) & 255, b = (c >> 8) & 255;
        if (fDir == 1) { r += (255 - r) * fAmt / 16; g += (255 - g) * fAmt / 16; b += (255 - b) * fAmt / 16; } else { r -= r * fAmt / 16; g -= g * fAmt / 16; b -= b * fAmt / 16; }
        stripLin[eye][i] = (r << 24) | (g << 16) | (b << 8) | 0xFF; }
    { extern u32 P3DS_WeatherObj[]; extern int P3DS_WeatherOn; if (P3DS_WeatherOn && !fAmt) for (unsigned i = 0; i < 64 * 192; i++) if (P3DS_WeatherObj[i]) stripLin[eye][i] = P3DS_WeatherObj[i]; } /* run 70 wide_weather */
    GSPGPU_FlushDataCache(stripLin[eye], 64 * 192 * 4);
    C3D_SyncDisplayTransfer(stripLin[eye], GX_BUFFER_DIM(64, 256), stripTex[eye].data, GX_BUFFER_DIM(64, 256), TEX_FLAGS);
    C3D_FrameDrawOn(t);
    C3D_Mtx proj; Mtx_OrthoTilt(&proj, 0.0f, 400.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_TexBind(0, &stripTex[eye]);
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
int P3DS_GpuBotSame; static int botValid, skipBotCfg = -1; static unsigned botSkips;
void P3DS_GpuInvalidateBot(void) { botValid = 0; } /* run 64: settings app drew its own bottom picture */
void P3DS_GpuQuiesce(void) { botValid = 0;
    if (ready) { C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH); gspWaitForVBlank(); }
    stereoOn = 0; gfxSet3D(false);
}
/* topR != NULL: stereo (slider > 0): left eye = top, right eye = topR. NULL: 2D mode, no right-eye cost. */
extern void P3DS_Crumb(const char *tag, void *caller);
void P3DS_GpuPresent(const u32 *top, const u32 *bot, int mode, int drawBottom, const u32 *topR) {
    P3DS_Crumb("gp-begin", 0); /* run 65: locate the boot stall */
    int st = topR != NULL; if (st != stereoOn) { stereoOn = st; gfxSet3D(st); }
    static int vs = -1; if (vs < 0) { vs = 0; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "vsync=%d", &x) == 1) vs = x != 0; fclose(f); } PSPNativeMemLog("[3DS] vsync (tear-free present) %d", vs); }
    C3D_FrameBegin(vs ? C3D_FRAME_SYNCDRAW : 0);
    if (srcDbl) srcSet ^= 1;  /* game already paces on vblank; SYNCDRAW added a 2nd vblank wait (30 fps cap) */
    /* run 25: upload AFTER FrameBegin. Before, the sync transfer raced the previous frame's queued
       render/transfer (PPF event from that job ended our wait early) and the GPU sampled a half-updated texture: tearing on both screens. */
    if (skipBotCfg < 0) { skipBotCfg = 1; /* run 47: default 1 again (safe: only skips when engine B was really reused) */ FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "present_skipbot=%d", &x) == 1) skipBotCfg = x != 0; fclose(f); } PSPNativeMemLog("[3DS] present_skipbot=%d", skipBotCfg); }
    const int botFresh = !(skipBotCfg && botValid && P3DS_GpuBotSame);
    if (!drawBottom) botValid = 0; else if (!botFresh) botSkips++;
    P3DS_Crumb("gp-upload", 0);
    upTicks = 0; upload(0, top); if (drawBottom && botFresh) upload(1, bot); if (st) upload(2, topR); P3DS_UploadUs = (unsigned)(upTicks / 268);
    C3D_BindProgram(&prog);
    C3D_AttrInfo *ai = C3D_GetAttrInfo(); AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 2); AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2);
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR); C3D_CullFace(GPU_CULL_NONE); C3D_AlphaTest(false, GPU_ALWAYS, 0);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    for (int k = 1; k < 6; k++) C3D_TexEnvInit(C3D_GetTexEnv(k));
    P3DS_Crumb("gp-prescale", 0);
    prescale(0); if (drawBottom && botFresh) { prescale(1); botValid = 1; } if (st) prescale(2);
    { static unsigned n; if (++n % 600 == 0) { PSPNativeMemLog("[3DS] present_skipbot: skipped bottom upload %u of 600", botSkips); botSkips = 0; } }
    /* run 33: mode 2 (fill) zoom from config gpuscale_zoom (1.25..1.5625, default 1.40625 = 360x270: 20px bars, 15 rows
       cropped top/bottom). Aspect always 4:3; edges snapped to whole pixels. */
    static float zoom = -1; if (zoom < 0) { zoom = 1.40625f; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; float z; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "gpuscale_zoom=%f", &z) == 1) zoom = z; fclose(f); } if (zoom < 1.25f) zoom = 1.25f; if (zoom > 1.5625f) zoom = 1.5625f; PSPNativeMemLog("[3DS] gpuscale_zoom=%.5f -> fill mode %dx%d", zoom, (int)(256 * zoom + 0.5f), (int)(192 * zoom + 0.5f)); }
    float fw = (float)(int)(256 * zoom + 0.5f), fh = (float)(int)(192 * zoom + 0.5f), fx = (float)(int)((400 - fw) / 2), fy = (float)(int)((240 - fh) / 2);
    if (mode == 2) quad(tgtTop, 400.0f, 0, fx, fy, fx + fw, fy + fh); else { quad(tgtTop, 400.0f, 0, 40, 0, 360, 240); strips(tgtTop, 0); }
    if (st) { if (mode == 2) quad(tgtTopR, 400.0f, 2, fx, fy, fx + fw, fy + fh); else { quad(tgtTopR, 400.0f, 2, 40, 0, 360, 240); strips(tgtTopR, 1); } }
    if (drawBottom) quad(tgtBot, 320.0f, 1, 0, 0, 320, 240);
    { /* tearfix: citro3d requests the screen swap at the first vblank after the transfer and GSP latches it one vblank
         later. A new transfer inside that window lands in the buffer still being scanned out: the first rows show the
         new frame, the rest the old (run 27 report, both screens). Make sure ~2 vblanks passed since the last present. */
      static int tf = -1; if (tf < 0) { tf = 2; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "tearfix=%d", &x) == 1) tf = x < 0 ? 0 : x > 2 ? 2 : x; fclose(f); } PSPNativeMemLog("[3DS] tearfix=%d", tf); }
      static u64 lastEnd; static unsigned waits, n; static u64 waitTicks;
      if (tf == 2) { /* run 32: tearfix=2 (default). C3D_FrameBegin above waited for the previous queue, so citro3d has
           requested the previous swap (gfxScreenSwapBuffers -> gspPresentBuffer); GSP applies it only at the next
           vblank. Until then the "back" buffer that this frame's display transfer writes is STILL ON SCREEN: the
           transfer overwrites it while the LCD scans it -> the first-scanned rows lag (both screens, also 2D video).
           Wait exactly until GSP has taken the previous present, no fixed vblank guesses. */
        P3DS_Crumb("gp-tearfix", 0);
        u64 t0 = svcGetSystemTick(); int w = 0;
        while ((gspIsPresentPending(GFX_TOP) || gspIsPresentPending(GFX_BOTTOM)) && svcGetSystemTick() - t0 < (u64)(40 * 268111.856)) { w = 1; svcSleepThread(250000); }
        if (w) { waits++; waitTicks += svcGetSystemTick() - t0; }
      } else if (tf && lastEnd) { u64 el = svcGetSystemTick() - lastEnd; if (el < (u64)(2 * 16.713 * 268111.856)) { if (el < (u64)(16.713 * 268111.856)) gspWaitForVBlank(); gspWaitForVBlank(); waits++; } }
      if (tf && ++n % 600 == 0) { PSPNativeMemLog("[3DS] tearfix=%d: waited on %u of 600 presents, avg wait %u us", tf, waits, waits ? (unsigned)(waitTicks / waits / 268) : 0); waits = 0; waitTicks = 0; }
      P3DS_Crumb("gp-end", 0); C3D_FrameEnd(0); lastEnd = svcGetSystemTick(); P3DS_Crumb("gp-done", 0); }
}

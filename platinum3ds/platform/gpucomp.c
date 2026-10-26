/* run 110: ported from the SoulSilver port (unowncollectables/Pokemon-Soulsilver-New3ds, runs 17 + 60: gpu_blend
   alpha/brightness on the GPU, per-eye 3D texture reuse outside the overworld). */
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "blit_shbin.h"
extern void PSPNativeMemLog(const char *, ...);
extern int P3DS_CfgInt(const char *, int);
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
static int relax3d(void) { static int v = -1; if (v < 0) { v = P3DS_CfgInt("gpu_blend_3d", 1) != 0; PSPNativeMemLog("[GPUC] gpu_blend_3d=%d", v); } return v; }
unsigned P3DS_GcBlendOn, P3DS_GcFxFrames, P3DS_GcBlendFrames, P3DS_GcBlendRej;  /* gpu_blend stats, logged by render3ds.cpp */
static int blendSet = -1;  /* ss3ds: set from the settings app (System > GPU blend) before or after init */
int P3DS_GpuBlendCfg(void) { if (blendSet < 0) blendSet = P3DS_CfgInt("gpu_blend", 1) != 0; return blendSet; }
void P3DS_SetGpuBlendCfg(int v) { blendSet = v != 0; P3DS_GcBlendOn = (unsigned)blendSet; PSPNativeMemLog("[GPUC] gpu_blend=%u (set from settings)", P3DS_GcBlendOn); }
unsigned P3DS_GcWalkUs, P3DS_GcConvUs, P3DS_GcOutUs, P3DS_GcQuads, P3DS_GcDecoded, P3DS_GcResets, P3DS_GcCpuUs, P3DS_GcGpuUs;

#define RB_FLAGS (GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

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
    C3D_TexSetFilter(&tex3dB, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&tex3dB, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); memset(tex3dB.data, 0, 256 * 256 * 4); GSPGPU_FlushDataCache(tex3dB.data, 256 * 256 * 4);
    C3D_TexSetFilter(&tex3d, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&tex3d, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    memset(atlas.data, 0, AW * AH * 4); memset(tex3d.data, 0, 256 * 256 * 4);
    GSPGPU_FlushDataCache(atlas.data, AW * AH * 4); GSPGPU_FlushDataCache(tex3d.data, 256 * 256 * 4);
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
      if (P3DS_GcBlendOn) { if ((bl & 0x3F00) && (dc & 0x108) == 0x108 && ((bl & 1) || !relax3d())) return 128; }  /* 3D layer alpha blend stays on melonDS; run 110 gpu_blend_3d=1: only when the 3D layer itself is a first target */
      else if ((bl & 0x3F00) || ((bl & 0xC0) && (bl & 0x3F))) return 128; }
    { const u16 mb = rh(regs, 0x6C); const unsigned m = mb >> 14; if ((m == 1 || m == 2) && (mb & 0x1F)) return 256; }
    if (rg(regs, 0x64) & 0x80000000u) return 512;
    if (dc & 0x1000) for (int i = 0; i < 128; i++) { const u16 a0 = oam[i * 4]; if ((a0 & 0x300) == 0x200) continue;
        if (a0 & 0x100) return 1024; if ((a0 >> 10) & 3) return 2048;
        if ((a0 & 0x1000) && (mos >> 8)) return 4096;
        if ((a0 & 0x2000) && (dc & 0x80000000u)) return 8192; }
    return 0;
}

static inline u32 col8(u16 c) { /* DS 15-bit -> melonDS final 8-bit, as PICA RGBA8 R<<24|G<<16|B<<8|A */
    const u32 r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    return (((r << 3) | (r >> 3)) << 24) | (((g << 3) | (g >> 3)) << 16) | (((b << 3) | (b >> 3)) << 8) | 0xFF; }
static u32 palHash[2][17];  /* [bg/obj][bank 0-15, 16 = 256-colour] */
static u32 lut[2][256];
static void palPrep(const u16 *pal, int k) {
    for (int b = 0; b < 16; b++) { u32 h = 2166136261u; for (int i = 0; i < 16; i++) h = (h ^ pal[b * 16 + i]) * 16777619u; palHash[k][b] = h; }
    { u32 h = 2166136261u; for (int i = 0; i < 256; i++) h = (h ^ pal[i]) * 16777619u; palHash[k][16] = h; }
    for (int i = 0; i < 256; i++) lut[k][i] = col8(pal[i]);
}
/* key: bit 24 obj, bit 23 8bpp, bits 19-22 bank, bits 0-18 address (32-byte aligned) */
static int fail;
static int tile(int obj, int bpp8, unsigned bank, const u8 *src, unsigned addr) {
    const u32 key = (1u << 31) | ((u32)obj << 24) | ((u32)bpp8 << 23) | ((bpp8 ? 0 : bank) << 19) | addr;
    unsigned h = ((key * 2654435761u) >> 18) & (HSIZE - 1);
    GE *e;
    for (;;) { e = &tab[h]; if (e->key == key || !e->key) break; h = (h + 1) & (HSIZE - 1); }
    const unsigned n = bpp8 ? 64 : 32; const u32 pv = palHash[obj][bpp8 ? 16 : bank];
    if (e->key == key) {
        if (e->stamp == frameNo) return e->empty ? -1 : e->slot;
        e->stamp = frameNo;
        if (e->palv == pv && !memcmp(e->raw, src, n)) return e->empty ? -1 : e->slot;
    } else {
        if (slotsUsed >= SLOTS || slotsUsed * 2 >= HSIZE) { fail = 1; return -1; }
        e->key = key; e->slot = slotsUsed++; e->stamp = frameNo;
    }
    memcpy(e->raw, src, n); e->palv = pv; P3DS_GcDecoded++;
    unsigned any = 0; for (unsigned i = 0; i < n; i++) any |= src[i];
    e->empty = !any; if (!any) return -1;
    const unsigned s = e->slot, tx = s % (AW / 8), ty = s / (AW / 8);
    u32 *d = (u32 *)atlas.data + (ty * (AW / 8) + tx) * 64;
    const u32 *L = lut[obj];
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
    GV *v = vb + nq * 6; nq++;
    v[0] = (GV){ x, y, u0, vt }; v[1] = (GV){ x, y + 8, u0, vb_ }; v[2] = (GV){ x + 8, y, u1, vt };
    v[3] = (GV){ x + 8, y, u1, vt }; v[4] = (GV){ x, y + 8, u0, vb_ }; v[5] = (GV){ x + 8, y + 8, u1, vb_ };
}
typedef struct { unsigned first, count; int is3d; int fx; } GB;  /* fx: 0 none, 1 alpha blend, 2 brighten, 3 darken (BLDCNT effect of this layer) */
static GB bat[32]; static unsigned nb;
static void batchBegin(int is3d, int fx) { if (nb && bat[nb - 1].is3d == is3d && !is3d && bat[nb - 1].fx == fx) return; bat[nb].first = nq; bat[nb].is3d = is3d; bat[nb].fx = fx; bat[nb].count = 0; nb++; }
static void batchEnd(void) { bat[nb - 1].count = nq - bat[nb - 1].first; }

/* per-frame memo per BG: map entry -> tile result (most map entries repeat, e.g. the blank tile) */
static struct { u32 stamp; u16 me; s16 slot; } memo[4][1024];
static void drawBG(const u8 *regs, const u8 *vram, u32 dc, int i) {
    const u16 cnt = rh(regs, 8 + i * 2);
    const unsigned charb = ((cnt >> 2) & 15) * 0x4000 + ((dc >> 24) & 7) * 0x10000;
    const unsigned mapb = ((cnt >> 8) & 31) * 0x800 + ((dc >> 27) & 7) * 0x10000;
    const unsigned sz = cnt >> 14, w = (sz & 1) ? 64 : 32, h = (sz & 2) ? 64 : 32;
    const int bpp8 = (cnt & 0x80) != 0;
    const unsigned hofs = rh(regs, 0x10 + i * 4) & 0x1FF, vofs = rh(regs, 0x12 + i * 4) & 0x1FF;
    for (unsigned r = 0; r < 25; r++) { const unsigned wy = (vofs >> 3) + r, ty = wy & (h - 1); const float sy = r * 8.0f - (vofs & 7);
        if (sy >= 192) break;
        for (unsigned k = 0; k < 33; k++) { const unsigned wx = (hofs >> 3) + k, tx = wx & (w - 1); const float sx = k * 8.0f - (hofs & 7);
            const unsigned ma = (mapb + ((tx >> 5) + (ty >> 5) * (w >> 5)) * 0x800 + ((ty & 31) * 32 + (tx & 31)) * 2) & 0x7FFFF;
            const u16 me = *(const u16 *)(vram + ma);
            const unsigned mi = (me ^ (me >> 10)) & 1023; const u16 mk = me & 0xF3FF;
            int s;
            if (memo[i][mi].stamp == frameNo && memo[i][mi].me == mk) s = memo[i][mi].slot;
            else { const unsigned ta = (charb + (me & 0x3FF) * (bpp8 ? 64 : 32)) & 0x7FFFF;
                s = tile(0, bpp8, me >> 12, vram + ta, ta); memo[i][mi].stamp = frameNo; memo[i][mi].me = mk; memo[i][mi].slot = (s16)s; }
            if (s >= 0) quad(s, sx, sy, me & 0x400, me & 0x800); } }
}
static void drawOBJ(const u16 *oam, const u8 *ov, u32 dc, unsigned prio) {
    for (int n = 127; n >= 0; n--) { const u16 a0 = oam[n * 4], a1 = oam[n * 4 + 1], a2 = oam[n * 4 + 2];
        if (((a2 >> 10) & 3) != prio || (a0 & 0x300) == 0x200) continue;
        const unsigned sp = (a0 >> 14) | ((a1 & 0xC000) >> 12); const int w = SW[sp], hh = SH[sp];
        int y = a0 & 255; if (y >= 192) y -= 256; if (y + hh <= 0) continue;
        const int x = (s32)((u32)a1 << 23) >> 23; if (x <= -w || x >= 256) continue;
        const int bpp8 = (a0 & 0x2000) != 0, hf = (a1 & 0x1000) != 0, vf = (a1 & 0x2000) != 0;
        const unsigned tn = a2 & 0x3FF, bank = a2 >> 12, ts = bpp8 ? 64 : 32;
        for (int r = 0; r < hh / 8; r++) for (int c = 0; c < w / 8; c++) {
            unsigned addr;
            if (dc & 0x10) addr = ((tn << ((dc >> 20) & 3)) << 5) + (r * (w / 8) + c) * ts;
            else addr = (tn << 5) + r * 1024 + c * ts;
            addr &= 0x3FFFF;
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
static void conv3dRaw(const u32 *f, C3D_Tex *tx) {
    /* NativeFrame 6-bit 0xAA_BB_GG_RR (alpha 5-bit) -> PICA RGBA8 (R<<24|G<<16|B<<8|A), 8-bit expand like melonDS,
       alpha 0 -> transparent. Branch-free per pixel, 8 pixels per tile row with constant Morton offsets. */
    u32 *d = (u32 *)tx->data;
    for (unsigned y = 0; y < 192; y++) { const unsigned my = 255 - y; const u8 *m = mort + (my & 7) * 8; u32 *drow = d + (my >> 3) * 32 * 64;
        const u32 *s = f + y * 256;
        for (unsigned tx = 0; tx < 32; tx++, s += 8) { u32 *t = drow + tx * 64;
            for (unsigned x = 0; x < 8; x++) { const u32 c = s[x], v = c & 0x3F3F3Fu, e = (v << 2) | ((v >> 4) & 0x030303u);
                t[m[x]] = (c >> 24) ? (__builtin_bswap32(e) | 0xFFu) : 0u; } } }
    GSPGPU_FlushDataCache(tx->data, 256 * 256 * 4);
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

/* returns 1 and fills out[256*192] (0xFFBBGGRR) on success; 0 = not composed (out untouched, caller falls back) */
int P3DS_GcComposeA(const u8 *regs, const u8 *bgv, const u8 *objv, const u16 *bgpal, const u16 *objpal, const u16 *oam, const u32 *f3d, u32 *out) {
    P3DS_Crumb("gc-begin", 0);
    if (!init()) return 0;
    const u64 t0 = svcGetSystemTick();
    frameNo++; fail = 0; nq = 0; nb = 0; dirtyLo = ~0u; dirtyHi = 0; P3DS_GcDecoded = 0;
    if (slotsUsed > SLOTS - 3000 || slotsUsed * 2 > HSIZE - 6000) { memset(tab, 0, HSIZE * sizeof(GE)); slotsUsed = 0; P3DS_GcResets++; }
    palPrep(bgpal, 0); palPrep(objpal, 1);
    const u32 dc = rg(regs, 0);
    int has3d = 0, blend3d = 0;
    /* colour effects (gpu_blend=1). Layers are drawn bottom to top, so blending with "what is below" is GPU blending with
     * the render target. DS semantics (blend only with a second-target pixel directly below) are reproduced exactly when
     * every layer drawn below a first-target layer is a second target and not itself a first target, plus the backdrop;
     * anything else returns 0 and the melonDS path is used. Brightness up/down needs no second target: per-layer TEV. */
    const u32 bl = P3DS_GcBlendOn ? rh(regs, 0x50) : 0, eff = (bl >> 6) & 3, firstM = bl & 0x3F, secondM = (bl >> 8) & 0x3F;
    const u32 eva = rh(regs, 0x52) & 0x1F, evb = (rh(regs, 0x52) >> 8) & 0x1F, evy = rh(regs, 0x54) & 0x1F;
    blendA = eva > 16 ? 16 : eva; blendB = evb > 16 ? 16 : evb; blendY = evy > 16 ? 16 : evy;
    u32 belowM = 0; int usedFx = 0;
    for (int p = 3; p >= 0; p--) {
        for (int i = 3; i >= 0; i--) { if (!(dc & (0x100u << i)) || (rh(regs, 8 + i * 2) & 3) != (unsigned)p) continue;
            const int fx = (eff && (firstM & (1u << i))) ? (int)eff : 0;
            if (i == 0 && (dc & 8)) { if (fx == 1) { P3DS_GcBlendRej++; fail = 1; break; }
                batchBegin(1, fx); bat[nb - 1].count = 6; has3d = 1; blend3d = relax3d() && (bl & 0x3F00) && !fx && !(belowM & ~secondM) && (belowM || (bl & 0x2000)); belowM |= 1; if (fx) usedFx = 1; continue; }
            batchBegin(0, fx); const unsigned q0 = nq; drawBG(regs, bgv, dc, i); batchEnd();
            if (nq > q0) { if (fx == 1 && ((belowM & ~secondM) || (belowM & firstM) || !(bl & 0x2000))) { P3DS_GcBlendRej++; fail = 1; break; }
                belowM |= 1u << i; if (fx) usedFx = fx; } }
        if (fail) break;
        if (dc & 0x1000) { const int fx = (eff && (firstM & 0x10)) ? (int)eff : 0; batchBegin(0, fx); const unsigned q0 = nq; drawOBJ(oam, objv, dc, p); batchEnd();
            if (nq > q0) { if (fx == 1 && ((belowM & ~secondM) || (belowM & firstM) || !(bl & 0x2000))) { P3DS_GcBlendRej++; fail = 1; }
                belowM |= 0x10; if (fx) usedFx = fx; } }
        if (fail) break; }
    if (fail) { P3DS_GcCpuUs = (svcGetSystemTick() - t0) / 268; return 0; }
    if (usedFx == 1) P3DS_GcBlendFrames++; else if (usedFx) P3DS_GcFxFrames++;
    /* the 3D layer quad lives after the tile quads */
    const u64 tw = svcGetSystemTick(); P3DS_GcWalkUs = (unsigned)((tw - t0) / 268);
    unsigned q3 = nq; if (has3d) { GV *v = vb + nq * 6; nq++;
        const float vt = 0.0f, vbm = 192.0f / 256;
        v[0] = (GV){ 0, 0, 0, vt }; v[1] = (GV){ 0, 192, 0, vbm }; v[2] = (GV){ 256, 0, 1, vt };
        v[3] = (GV){ 256, 0, 1, vt }; v[4] = (GV){ 0, 192, 0, vbm }; v[5] = (GV){ 256, 192, 1, vbm };
        conv3d(f3d); }
    if (dirtyHi > dirtyLo) GSPGPU_FlushDataCache((u8 *)atlas.data + dirtyLo, dirtyHi - dirtyLo);
    GSPGPU_FlushDataCache(vb, nq * 6 * sizeof(GV));
    P3DS_GcQuads = nq;
    const u64 t1 = svcGetSystemTick(); P3DS_GcConvUs = (unsigned)((t1 - tw) / 268);
    u32 bd = col8(bgpal[0]);
    if (eff >= 2 && (firstM & 0x20)) {  /* backdrop as first target of brightness up/down (bias 8 / 7, as ColorBrightness*) */
        u32 c[3] = { bd >> 24, (bd >> 16) & 255, (bd >> 8) & 255 };
        for (int k = 0; k < 3; k++) c[k] = eff == 2 ? c[k] + (((255 - c[k]) * blendY + 8) >> 4) : c[k] - ((c[k] * blendY + 7) >> 4);
        bd = (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | 0xFF; }
    P3DS_Crumb("gc-draw", 0);
    C3D_FrameBegin(0);
    G3G_DirectSynced();   /* FrameBegin waited for every queued draw, including g3gpu's direct 3D frame */
    C3D_RenderTargetClear(rt, C3D_CLEAR_ALL, bd, 0);
    C3D_FrameDrawOn(rt);
    C3D_BindProgram(&prog);
    C3D_Mtx proj; Mtx_Ortho(&proj, 0.0f, 256.0f, 0.0f, 256.0f, 1.0f, -1.0f, true);  /* DS y 0 -> clip -1 (= g3gpu convention, FLIP_VERT readback gives rows top-first) */
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &proj);
    C3D_AttrInfo *ai = C3D_GetAttrInfo(); AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 2); AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2);
    C3D_BufInfo *bi = C3D_GetBufInfo(); BufInfo_Init(bi); BufInfo_Add(bi, vb, sizeof(GV), 2, 0x10);
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR); C3D_CullFace(GPU_CULL_NONE);
    C3D_AlphaTest(true, GPU_GREATER, 0);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    { C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env); C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
      for (int k = 1; k < 6; k++) C3D_TexEnvInit(C3D_GetTexEnv(k)); }
    int curFx = 0;
    for (unsigned b = 0; b < nb; b++) { if (!bat[b].is3d && !bat[b].count) continue;
        if (bat[b].fx != curFx) { curFx = bat[b].fx; setFx(curFx); }
        if (bat[b].is3d) { C3D_TexBind(0, t3cur);
            /* run 110 gpu_blend_3d: translucent 3D pixels (move effects) blend with the second-target layers below like the DS.
               The 3D target was cleared to alpha 0, so its colour is already alpha-weighted: premultiplied blend. */
            if (blend3d) C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ZERO);
            C3D_DrawArrays(GPU_TRIANGLES, q3 * 6, 6); if (blend3d) setFx(curFx); }
        else { C3D_TexBind(0, &atlas); C3D_DrawArrays(GPU_TRIANGLES, bat[b].first * 6, bat[b].count * 6); } }
    C3D_SyncDisplayTransfer((u32 *)rtTex.data, GX_BUFFER_DIM(256, 256), rb, GX_BUFFER_DIM(256, 256), RB_FLAGS);
    C3D_FrameEnd(0);
    P3DS_Crumb("gc-wait", 0);
    C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH);
    P3DS_Crumb("gc-done", 0);
    GSPGPU_InvalidateDataCache(rb, 256 * 192 * 4);
    const u64 t2 = svcGetSystemTick();
    { const u32 *r = rb; u32 *o = out; for (unsigned i = 0; i < 256 * 192; i += 4, r += 4, o += 4) {
        const u32 a = r[0], b = r[1], c = r[2], e = r[3];
        o[0] = __builtin_bswap32(a) | 0xFF000000u; o[1] = __builtin_bswap32(b) | 0xFF000000u; o[2] = __builtin_bswap32(c) | 0xFF000000u; o[3] = __builtin_bswap32(e) | 0xFF000000u; } }
    const u64 t3 = svcGetSystemTick(); P3DS_GcOutUs = (unsigned)((t3 - t2) / 268);
    P3DS_GcCpuUs = (unsigned)((t1 - t0 + t3 - t2) / 268); P3DS_GcGpuUs = (unsigned)((t2 - t1) / 268);
    return 1;
}

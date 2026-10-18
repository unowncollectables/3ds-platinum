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
#include <stdio.h>
#include "blit_shbin.h"
extern void PSPNativeMemLog(const char *, ...);
extern void P3DS_Crumb(const char *tag, void *caller);  /* watchdog crumbs (only on gpu_compose>=2 frames) */

#define AW 512
#define AH 1024
#define SLOTS ((AW / 8) * (AH / 8))
#define HSIZE 16384
#define MAXQ 12288
typedef struct { float x, y, u, v; } GV;
typedef struct { u32 key; u32 stamp; u32 palv; u16 slot; u8 empty, n; u8 raw[64]; } GE;
static GE *tab; static unsigned slotsUsed, frameNo;
static C3D_Tex atlas, tex3d, rtTex; static C3D_RenderTarget *rt;
static GV *vb; static u32 *rb;
static DVLB_s *dvlb; static shaderProgram_s prog; static int uProj; static int ready = -1;
static u8 mort[64];
static unsigned dirtyLo, dirtyHi;
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
    if (!C3D_TexInit(&atlas, AW, AH, GPU_RGBA8) || !C3D_TexInit(&tex3d, 256, 256, GPU_RGBA8)) { PSPNativeMemLog("[GPUC] init: tex alloc failed"); return 0; }
    C3D_TexSetFilter(&atlas, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&atlas, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    C3D_TexSetFilter(&tex3d, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&tex3d, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    memset(atlas.data, 0, AW * AH * 4); memset(tex3d.data, 0, 256 * 256 * 4);
    GSPGPU_FlushDataCache(atlas.data, AW * AH * 4); GSPGPU_FlushDataCache(tex3d.data, 256 * 256 * 4);
    if (!C3D_TexInitVRAM(&rtTex, 256, 256, GPU_RGBA8)) { PSPNativeMemLog("[GPUC] init: VRAM target failed"); return 0; }
    rt = C3D_RenderTargetCreateFromTex(&rtTex, GPU_TEXFACE_2D, 0, -1); if (!rt) return 0;
    dvlb = DVLB_ParseFile((u32 *)blit_shbin, blit_shbin_size);
    shaderProgramInit(&prog); shaderProgramSetVsh(&prog, &dvlb->DVLE[0]);
    uProj = shaderInstanceGetUniformLocation(prog.vertexShader, "projection");
    PSPNativeMemLog("[GPUC] GPU compositor ready: atlas %ux%u (%u tiles), %u quads max", AW, AH, SLOTS, MAXQ);
    ready = 1; return 1;
}

static inline u32 rg(const u8 *r, unsigned o) { return *(const u32 *)(r + o); }
static inline u16 rh(const u8 *r, unsigned o) { return *(const u16 *)(r + o); }
static const s32 SW[16] = { 8,16,8,8, 16,32,8,8, 32,32,16,8, 64,64,32,8 };
static const s32 SH[16] = { 8,8,16,8, 16,8,32,8, 32,16,32,8, 64,32,64,8 };

/* 0 = this frame can be composed on the GPU; else a reason bit */
int P3DS_GcEligible(const u8 *regs, const u16 *oam) {
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
    { const u16 bl = rh(regs, 0x50); if ((bl & 0x3F00) || ((bl & 0xC0) && (bl & 0x3F))) return 128; }
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
typedef struct { unsigned first, count; int is3d; } GB;
static GB bat[32]; static unsigned nb;
static void batchBegin(int is3d) { if (nb && bat[nb - 1].is3d == is3d && !is3d) return; bat[nb].first = nq; bat[nb].is3d = is3d; bat[nb].count = 0; nb++; }
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
static void conv3d(const u32 *f) {
    /* NativeFrame 6-bit 0xAA_BB_GG_RR (alpha 5-bit) -> PICA RGBA8 (R<<24|G<<16|B<<8|A), 8-bit expand like melonDS,
       alpha 0 -> transparent. Branch-free per pixel, 8 pixels per tile row with constant Morton offsets. */
    u32 *d = (u32 *)tex3d.data;
    for (unsigned y = 0; y < 192; y++) { const unsigned my = 255 - y; const u8 *m = mort + (my & 7) * 8; u32 *drow = d + (my >> 3) * 32 * 64;
        const u32 *s = f + y * 256;
        for (unsigned tx = 0; tx < 32; tx++, s += 8) { u32 *t = drow + tx * 64;
            for (unsigned x = 0; x < 8; x++) { const u32 c = s[x], v = c & 0x3F3F3Fu, e = (v << 2) | ((v >> 4) & 0x030303u);
                t[m[x]] = (c >> 24) ? (__builtin_bswap32(e) | 0xFFu) : 0u; } } }
    GSPGPU_FlushDataCache(tex3d.data, 256 * 256 * 4);
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
    int has3d = 0;
    for (int p = 3; p >= 0; p--) {
        for (int i = 3; i >= 0; i--) { if (!(dc & (0x100u << i)) || (rh(regs, 8 + i * 2) & 3) != (unsigned)p) continue;
            if (i == 0 && (dc & 8)) { batchBegin(1); bat[nb - 1].count = 6; has3d = 1; continue; }
            batchBegin(0); drawBG(regs, bgv, dc, i); batchEnd(); }
        if (dc & 0x1000) { batchBegin(0); drawOBJ(oam, objv, dc, p); batchEnd(); }
        if (fail) break; }
    if (fail) { P3DS_GcCpuUs = (svcGetSystemTick() - t0) / 268; return 0; }
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
    const u32 bd = col8(bgpal[0]);
    P3DS_Crumb("gc-draw", 0);
    C3D_FrameBegin(0);
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
    for (unsigned b = 0; b < nb; b++) { if (bat[b].is3d) { C3D_TexBind(0, &tex3d); C3D_DrawArrays(GPU_TRIANGLES, q3 * 6, 6); }
        else if (bat[b].count) { C3D_TexBind(0, &atlas); C3D_DrawArrays(GPU_TRIANGLES, bat[b].first * 6, bat[b].count * 6); } }
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

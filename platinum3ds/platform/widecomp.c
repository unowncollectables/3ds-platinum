/* platinum3ds run 84: wide_layouts display engine (config wide_layouts=1, default 0; also needs wideview on and the
 * 1.25x scale, engine A on top, outside the 3D overworld). Draws a screen across 320 DS columns (-32..287) the way the
 * game's wide layout says (GabeSud0/pokeplatinum-3ds port_wide_layout.h parts, recorded by wideapi.c):
 *  - "M" layers: every BG whose layout is the same shift D as the sprites' (WIDE_OBJS dx, 0 without one) on every
 *    line. melonDS draws them, with the sprites, in the normal pass (the other BGs switched off for it); that picture
 *    is placed D columns right.
 *  - "CPU" layers: every other text BG. Drawn here, per line and column, as the parts say (WIDE_MOVE / WIDE_FILL, the
 *    DS's own place where a line has no part), in front of the M picture when their priority is ahead of every M
 *    layer, else only where the M picture shows the backdrop.
 *  - columns the M picture doesn't reach: all text BGs drawn here (fills beside the DS screen), back to front.
 * Output: the centre 256 columns go back into the engine A picture, the 32 each side into the wide strips (as
 * wide_menus does). Master brightness is applied here to the centre (the M pass runs without it), gpuscale applies it
 * to the strips. Not done (yet): alpha blending of CPU layers, sprite regions/stretch (one sprite shift for the
 * whole screen), WIDE_SHIFT3D, engine B. Pure C (no 3DS headers), so tools/widecomp_test.c runs it on a PC. */
#include <stdint.h>
#include <string.h>
#include "widecomp.h"

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;

static void readLayers(WcPlan *p, const u8 *R) {
    const u32 dc = *(const u32 *)R; const unsigned mode = dc & 7;
    for (int i = 0; i < 4; i++) { WcLayer *l = &p->L[i]; memset(l, 0, sizeof *l);
        const u16 c = *(const u16 *)(R + 8 + i * 2);
        l->on = (dc & (0x100u << i)) != 0; l->prio = c & 3;
        l->text = !(i == 0 && (dc & 8)) && mode <= 5 && (i < 2 || mode == 0 || (i == 2 && mode == 1));
        l->bpp8 = (c & 0x80) != 0; l->w = (c >> 14) & 1 ? 64 : 32; l->h = (c >> 14) & 2 ? 64 : 32;
        l->charb = ((c >> 2) & 15) * 0x4000 + ((dc >> 24) & 7) * 0x10000; l->mapb = ((c >> 8) & 31) * 0x800 + ((dc >> 27) & 7) * 0x10000;
        l->hofs = *(const u16 *)(R + 0x10 + i * 4) & 0x1FF; l->vofs = *(const u16 *)(R + 0x12 + i * 4) & 0x1FF;
        l->extSlot = (l->bpp8 && (dc & (1u << 30))) ? ((i < 2 && (c & 0x2000)) ? i + 2 : i) : -1;
        /* scroll as a small signed number of the picture's columns: parts use unwrapped columns (-32..287) */
        const int W = l->w * 8; int h = (int)(l->hofs & (W - 1)); if (h > W - 64) h -= W; l->h0 = h; }
}

/* the picture column of layer li shown at DS screen column s (-32..287) on screen line y, -1 = nothing */
static int srcCol(const WcPlan *p, int li, int y, int s) {
    const WcLayer *l = &p->L[li]; const int W = l->w * 8, H = l->h * 8, py = (int)((y + l->vofs) & (H - 1)), pic = s + l->h0;
    int any = 0, taken = 0, res = -1;
    for (int k = 0; k < p->n; k++) { const short *t = p->parts + k * 8; if ((t[0] != 1 && t[0] != 2) || !(t[1] & (1 << li))) continue;
        const int scr = (t[1] & 0x10) != 0, ln = scr ? y : py; if (ln < t[2] || ln >= t[3]) continue;
        any = 1; if (t[0] == 1) { const int c = scr ? s : pic; if (c >= t[4] && c < t[5]) taken = 1; } }
    if (!any) return (s >= 0 && s < 256) ? (pic & (W - 1)) : -1;
    if (s >= 0 && s < 256 && !taken) res = pic & (W - 1);
    for (int k = 0; k < p->n; k++) { const short *t = p->parts + k * 8; if ((t[0] != 1 && t[0] != 2) || !(t[1] & (1 << li))) continue;
        const int scr = (t[1] & 0x10) != 0, ln = scr ? y : py; if (ln < t[2] || ln >= t[3]) continue;
        if (t[0] == 1) { const int c = (scr ? s : pic) - t[6]; if (c >= t[4] && c < t[5]) res = (scr ? c + l->h0 : c) & (W - 1); }
        else { const int c = scr ? s : pic; if (c >= t[4] && c < t[5]) res = t[7] & (W - 1); } }
    return res;
}

/* the whole line of srcCol() at once (r[s+32], s = -32..287), by ranges: a few parts x a few hundred columns, not
   every column x every part (the bag has 18 parts and a plan every frame while its list scrolls) */
static void buildRow(const WcPlan *p, int li, int y, short *r) {
    const WcLayer *l = &p->L[li]; const int W = l->w * 8, H = l->h * 8, py = (int)((y + l->vofs) & (H - 1)), M = W - 1;
    uint8_t act[WC_MAX_PARTS]; int na = 0;
    for (int k = 0; k < p->n; k++) { const short *t = p->parts + k * 8; if ((t[0] != 1 && t[0] != 2) || !(t[1] & (1 << li))) continue;
        const int ln = (t[1] & 0x10) ? y : py; if (ln >= t[2] && ln < t[3]) act[na++] = (uint8_t)k; }
    for (int s = -32; s < 288; s++) r[s + 32] = -1;
    if (!na) { for (int s = 0; s < 256; s++) r[s + 32] = (short)((s + l->h0) & M); return; }
    for (int s = 0; s < 256; s++) r[s + 32] = (short)((s + l->h0) & M);
    for (int a = 0; a < na; a++) { const short *t = p->parts + act[a] * 8; if (t[0] != 1) continue;
        const int scr = (t[1] & 0x10) != 0, a0 = scr ? t[4] : t[4] - l->h0, a1 = scr ? t[5] : t[5] - l->h0; /* columns vacated, as s */
        for (int s = a0 < 0 ? 0 : a0; s < a1 && s < 256; s++) r[s + 32] = -1; }
    for (int a = 0; a < na; a++) { const short *t = p->parts + act[a] * 8; const int scr = (t[1] & 0x10) != 0;
        if (t[0] == 1) { for (int c = t[4]; c < t[5]; c++) { const int s = scr ? c + t[6] : c - l->h0 + t[6]; if (s >= -32 && s < 288) r[s + 32] = (short)((scr ? c + l->h0 : c) & M); } }
        else { for (int c = t[4]; c < t[5]; c++) { const int s = scr ? c : c - l->h0; if (s >= -32 && s < 288) r[s + 32] = (short)(t[7] & M); } } }
}

/* RGB555 -> the melonDS output format (0xAABBGGRR, same rounding as GPU2D_Soft's final conversion) */
static u32 toM(u16 c) { u32 r = (c & 31) << 3, g = ((c >> 5) & 31) << 3, b = ((c >> 10) & 31) << 3;
    r |= r >> 6; g |= g >> 6; b |= b >> 6; return r | (g << 8) | (b << 16) | 0xFF000000u; }

int WC_Plan(WcPlan *p, const u8 *R, const short *parts, int n) {
    u32 h = 2166136261u; for (int k = 0; k < 0x20; k++) h = (h ^ R[k]) * 16777619u;
    for (int k = 0; k < n * 8; k++) h = (h ^ (u16)parts[k]) * 16777619u;
    h ^= (u32)n;
    if (p->valid && h == p->hash) return 1;
    p->hash = h; p->changed = 1; p->n = n < WC_MAX_PARTS ? n : WC_MAX_PARTS; memcpy(p->parts, parts, p->n * 8 * sizeof(short));
    readLayers(p, R);
    int D = 0, objs = 0; for (int k = 0; k < p->n; k++) if (p->parts[k * 8] == 3) { if (!objs++) D = p->parts[k * 8 + 6]; }
    p->D = D; p->objParts = objs; p->mMask = p->cpuMask = 0;
    p->nRows = 0;
    for (int i = 0; i < 4; i++) { const WcLayer *l = &p->L[i]; memset(p->rowOf[i], 255, 192); if (!l->on || !l->text) continue;
        const int H = l->h * 8; uint8_t act[WC_MAX_PARTS], prev[WC_MAX_PARTS]; int na = 0, np = -1;
        for (int y = 0; y < 192; y++) { const int py = (int)((y + l->vofs) & (H - 1)); na = 0;
            for (int k = 0; k < p->n; k++) { const short *t = p->parts + k * 8; if ((t[0] != 1 && t[0] != 2) || !(t[1] & (1 << i))) continue;
                const int ln = (t[1] & 0x10) ? y : py; if (ln >= t[2] && ln < t[3]) act[na++] = (uint8_t)k; }
            if (y && na == np && !memcmp(act, prev, na) && p->rowOf[i][y - 1] != 255) { p->rowOf[i][y] = p->rowOf[i][y - 1]; continue; }
            memcpy(prev, act, na); np = na;
            if (p->nRows >= WC_ROWS) continue;
            buildRow(p, i, y, p->rows[p->nRows]);
            p->rowOf[i][y] = (uint8_t)p->nRows++; } }
    for (int i = 0; i < 4; i++) { const WcLayer *l = &p->L[i]; if (!l->on) continue;
        int m = 1; if (l->text) { const int W = l->w * 8, s0 = D > -32 ? D : -32, s1 = D + 256 < 288 ? D + 256 : 288;
            for (int y = 0; y < 192 && m; y++) { const uint8_t ri = p->rowOf[i][y];
                for (int s = s0; s < s1 && m; s++) if ((ri != 255 ? p->rows[ri][s + 32] : srcCol(p, i, y, s)) != ((s - D + l->h0) & (W - 1))) m = 0; } }
        if (m) p->mMask |= 1 << i; else p->cpuMask |= 1 << i; }
    p->frontMask = 0;
    for (int i = 0; i < 4; i++) if (p->cpuMask & (1 << i)) { int f = 1;
        for (int k = 0; k < 4; k++) if (p->mMask & (1 << k)) { if (!(p->L[i].prio < p->L[k].prio || (p->L[i].prio == p->L[k].prio && i < k))) f = 0; }
        if (f) p->frontMask |= 1 << i; }
    p->valid = 1; return 1;
}

/* one layer's pixels for DS columns s0..s1-1 of line y into row[] (index s+32) where opaque; onlyOver: only where row
   still holds that colour (the backdrop) */
static void drawLayer(const WcPlan *p, int li, int y, int s0, int s1, const u8 *vram, const u32 *lut, const u8 *ext, u32 *row, u8 *lay, int onlyIf, u32 bdM) {
    const WcLayer *l = &p->L[li]; const int py = (int)((y + l->vofs) & (l->h * 8 - 1)); const uint8_t ri = p->rowOf[li][y];
    short tmp[320]; const short *map; if (ri != 255) map = p->rows[ri]; else { buildRow(p, li, y, tmp); map = tmp; }
    int lastT = -1; u16 me = 0; const u8 *trow = 0;
    const unsigned ty = (unsigned)(py >> 3) & (l->h - 1); unsigned fy0 = py & 7;
    for (int s = s0; s < s1; s++) { const int X = s + 32; if (onlyIf && row[X] != bdM) continue;
        const int pc = map ? map[X] : srcCol(p, li, y, s); if (pc < 0) continue;
        const int t = pc >> 3;
        if (t != lastT) { lastT = t; const unsigned tx = (unsigned)t & (l->w - 1);
            const unsigned ma = (l->mapb + ((tx >> 5) + (ty >> 5) * (l->w >> 5)) * 0x800 + ((ty & 31) * 32 + (tx & 31)) * 2) & 0x7FFFF;
            me = *(const u16 *)(vram + ma); const unsigned fy = (me & 0x800) ? 7 - fy0 : fy0;
            trow = vram + ((l->charb + (me & 0x3FF) * (l->bpp8 ? 64 : 32) + fy * (l->bpp8 ? 8 : 4)) & 0x7FFFF); }
        unsigned fx = pc & 7; if (me & 0x400) fx = 7 - fx;
        unsigned ci; if (l->bpp8) ci = trow[fx]; else { const u8 b = trow[fx >> 1]; ci = (fx & 1) ? b >> 4 : b & 15; }
        if (!ci) continue;
        row[X] = l->extSlot >= 0 ? toM(*(const u16 *)(ext + l->extSlot * 0x2000 + ((me >> 12) * 256 + ci) * 2)) : lut[l->bpp8 ? ci : ((me >> 12) * 16 + ci)];
        lay[X] = (u8)li; }
}

void WC_Compose(const WcPlan *p, const u8 *R, const u8 *vram, const u16 *pal, const u8 *ext, u32 *center, u32 *strip) {
    int order[4], no = 0; for (int pr = 3; pr >= 0; pr--) for (int i = 3; i >= 0; i--) if (p->L[i].on && p->L[i].text && p->L[i].prio == pr) order[no++] = i;
    u32 lut[256]; for (int k = 0; k < 256; k++) lut[k] = toM(pal[k]);
    const u32 bdM = lut[0];
    const u16 bc = *(const u16 *)(R + 0x50); unsigned bY = *(const u16 *)(R + 0x54) & 31; if (bY > 16) bY = 16;
    const int eff = (((bc >> 6) & 3) >= 2 && bY) ? (int)((bc >> 6) & 3) : 0;
    const u16 mb = *(const u16 *)(R + 0x6C); unsigned mf = mb & 31; if (mf > 16) mf = 16; const int mm = mf ? (mb >> 14) & 3 : 0;
    const int D = p->D, c0 = D > -32 ? D : -32, c1 = D + 256 < 288 ? D + 256 : 288; /* DS columns the melonDS picture covers */
    for (int y = 0; y < 192; y++) {
        u32 row[320]; u8 lay[320];
        for (int s = -32; s < 288; s++) { if (s >= c0 && s < c1) { row[s + 32] = center[y * 256 + s - D]; lay[s + 32] = 4; } else { row[s + 32] = bdM; lay[s + 32] = 5; } }
        for (int oi = 0; oi < no; oi++) { const int li = order[oi];
            if (c0 > -32) drawLayer(p, li, y, -32, c0, vram, lut, ext, row, lay, 0, bdM);
            if (c1 < 288) drawLayer(p, li, y, c1, 288, vram, lut, ext, row, lay, 0, bdM);
            if (p->cpuMask & (1 << li)) drawLayer(p, li, y, c0, c1, vram, lut, ext, row, lay, !(p->frontMask & (1 << li)), bdM); }
        if (!eff && !mm) { memcpy(center + y * 256, row + 32, 256 * 4);
            for (int X = 0; X < 64; X++) { const u32 c = row[X < 32 ? X : X + 256]; strip[y * 64 + X] = ((c & 255) << 24) | (((c >> 8) & 255) << 16) | (((c >> 16) & 255) << 8) | 0xFF; }
            continue; }
        for (int X = 0; X < 320; X++) { u32 c = row[X]; unsigned r = c & 255, g = (c >> 8) & 255, b = (c >> 16) & 255; const int side = X < 32 || X >= 288;
            /* BLDY brighten/darken on pixels drawn here (the melonDS picture has it already); in the strips a fade that
               targets BG0 comes from gpuscale (wide_fadefix), as for wide_menus */
            if (eff && lay[X] != 4 && (bc & (1u << lay[X])) && !(side && (bc & 1))) {
                if (eff == 2) { r += (255 - r) * bY / 16; g += (255 - g) * bY / 16; b += (255 - b) * bY / 16; } else { r -= r * bY / 16; g -= g * bY / 16; b -= b * bY / 16; } }
            if (side) strip[y * 64 + (X < 32 ? X : X - 256)] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
            else { if (mm == 1) { r += (255 - r) * mf / 16; g += (255 - g) * mf / 16; b += (255 - b) * mf / 16; } else if (mm == 2) { r -= r * mf / 16; g -= g * mf / 16; b -= b * mf / 16; }
                center[y * 256 + X - 32] = r | (g << 8) | (b << 16) | 0xFF000000u; } }
    }
}

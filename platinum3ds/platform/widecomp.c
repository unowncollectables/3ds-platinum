/* platinum3ds run 86: wide_layouts display engine (config wide_layouts=1, default 0; also needs the 1.25x scale, engine A
 * on top). Draws a whole 2D screen across 320 DS columns (-32..287) the way the game's wide layout says (GabeSud0/
 * pokeplatinum-3ds port_wide_layout.h parts, recorded by wideapi.c): its own small compositor for engine A, text BGs +
 * sprites (+ the 3D layer as BG0) in DS priority order, each BG column / sprite / 3D picture moved by the layout.
 * (Run 84/85 let melonDS draw some layers and drew the others on top; that cannot get the priorities between the two
 * groups right, as the party menu and the PC boxes showed.) Anything it does not do (windows, alpha blending, mosaic,
 * affine or bitmap BGs, bitmap sprites, forced blank) makes WC_Compose return 0 and the melonDS picture is used as is.
 * Output: the 256 centre columns into the engine A picture, 32 each side into the wide strips (as wide_menus does).
 * Master brightness: applied here to the centre, by gpuscale to the strips (BLDY fades on BGs / sprites are done here
 * on both). Pure C (no 3DS headers), so tools/widecomp_test.c runs it on a PC. */
#include <stdint.h>
#include <string.h>
#include "widecomp.h"

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int16_t s16;

static void readLayers(WcPlan *p, const u8 *R) {
    const u32 dc = *(const u32 *)R;
    for (int i = 0; i < 4; i++) { WcLayer *l = &p->L[i]; memset(l, 0, sizeof *l);
        const u16 c = *(const u16 *)(R + 8 + i * 2);
        l->on = (dc & (0x100u << i)) != 0; l->prio = c & 3;
        l->text = !(i == 0 && (dc & 8)) && (dc & 7) <= 5 && (i < 2 || (dc & 7) == 0 || (i == 2 && (dc & 7) == 1));
        l->bpp8 = (c & 0x80) != 0; l->w = (c >> 14) & 1 ? 64 : 32; l->h = (c >> 14) & 2 ? 64 : 32;
        l->charb = ((c >> 2) & 15) * 0x4000 + ((dc >> 24) & 7) * 0x10000; l->mapb = ((c >> 8) & 31) * 0x800 + ((dc >> 27) & 7) * 0x10000;
        l->hofs = *(const u16 *)(R + 0x10 + i * 4) & 0x1FF; l->vofs = *(const u16 *)(R + 0x12 + i * 4) & 0x1FF;
        l->extSlot = (l->bpp8 && (dc & (1u << 30))) ? ((i < 2 && (c & 0x2000)) ? i + 2 : i) : -1;
        l->mosaic = (c & 0x40) != 0;
        /* scroll as a small signed number of the picture's columns: parts use unwrapped columns (-32..287) */
        const int W = l->w * 8; int h = (int)(l->hofs & (W - 1)); if (h > W - 64) h -= W; l->h0 = h; }
}

/* the picture column of layer li shown at DS screen column s (-32..287) on screen line y, -1 = nothing
   (reference version of buildRow(); the host test checks they agree) */
__attribute__((unused)) static int srcCol(const WcPlan *p, int li, int y, int s) {
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
    for (int s = 0; s < 256; s++) r[s + 32] = (short)((s + l->h0) & M);
    if (!na) return;
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
/* the 3D layer's 6-bit-per-channel pixel in the same format */
static u32 from3D(u32 c) { u32 v = (c << 2) & 0x00FCFCFCu; return v | ((v & 0x00C0C0C0u) >> 6) | 0xFF000000u; }

int WC_Plan(WcPlan *p, const u8 *R, const short *parts, int n) {
    u32 h = 2166136261u; for (int k = 0; k < 0x20; k++) h = (h ^ R[k]) * 16777619u;
    for (int k = 0; k < n * 8; k++) h = (h ^ (u16)parts[k]) * 16777619u;
    h ^= (u32)n;
    if (p->valid && h == p->hash) return 1;
    p->hash = h; p->changed = 1; p->n = n < WC_MAX_PARTS ? n : WC_MAX_PARTS; memcpy(p->parts, parts, p->n * 8 * sizeof(short));
    readLayers(p, R);
    p->objParts = 0; p->dx3d = 0;
    for (int k = 0; k < p->n; k++) { if (p->parts[k * 8] == 3) p->objParts++; else if (p->parts[k * 8] == 4) p->dx3d = p->parts[k * 8 + 6]; }
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
    p->valid = 1; return 1;
}

/* a sprite's pixel (tx, ty of its picture) as a palette colour, 0 = transparent; normal (non-bitmap) sprites, as melonDS */
static int sprTexel(const u16 *a, u32 dc, const u8 *ov, int width, int tx, int ty, u32 *idx) {
    u32 tile = a[2] & 0x3FF, addr; const int bpp8 = (a[0] & 0x2000) != 0;
    if (dc & 0x10) { tile <<= (dc >> 20) & 3; addr = tile + (u32)(((ty >> 3) * (width >> 3)) << (bpp8 ? 1 : 0)); } else addr = tile + (u32)((ty >> 3) * 0x20);
    addr <<= 5;
    if (bpp8) { addr += (u32)((ty & 7) << 3) + (u32)((tx & ~7) << 3) + (u32)(tx & 7); *idx = ov[addr & 0x3FFFF]; }
    else { addr += (u32)((ty & 7) << 2) + (u32)((tx & ~7) << 2) + (u32)((tx & 7) >> 1); const u8 b = ov[addr & 0x3FFFF]; *idx = (tx & 1) ? b >> 4 : b & 15; }
    return *idx != 0;
}

/* returns 0 = something this compositor does not do (the melonDS picture stays); else the picture is written */
int WC_Compose(const WcPlan *p, const u8 *R, const WcSrc *S, u32 *center, u32 *strip) {
    const u32 dc = *(const u32 *)R;
    if ((dc & 0x80) || ((dc >> 16) & 3) != 1 || (dc & 0xE000)) return 0;                /* forced blank, not graphics mode, windows */
    const u16 bc = *(const u16 *)(R + 0x50); const unsigned eff = (bc >> 6) & 3;
    if (eff == 1 && (bc & 0x3F)) return 0;                                              /* alpha blending */
    for (int i = 0; i < 4; i++) { const WcLayer *l = &p->L[i]; if (!l->on) continue; if (!l->text && !(i == 0 && (dc & 8))) return 0; if (l->mosaic) return 0; }
    if ((dc & 8) && !S->frame3d) return 0;
    unsigned bY = *(const u16 *)(R + 0x54) & 31; if (bY > 16) bY = 16;
    const u16 mb = *(const u16 *)(R + 0x6C); unsigned mf = mb & 31; if (mf > 16) mf = 16; const int mm = mf ? (mb >> 14) & 3 : 0;
    u32 lutB[256], lutO[256]; for (int k = 0; k < 256; k++) { lutB[k] = toM(S->bgpal[k]); lutO[k] = toM(S->objpal[k]); }
    const u32 bd = lutB[0];
    /* sprites: where each one's middle is in a WIDE_OBJS part decides its shift; sorted by priority, then OAM order */
    struct Sp { s16 x; s16 dx; u8 id; } order[128]; int no = 0; u8 spp[128];
    for (int pr = 3; pr >= 0; pr--) for (int i = 127; i >= 0; i--) { const u16 *a = S->oam + i * 4; if (((a[2] >> 10) & 3) != pr) continue;
        const int mode = (a[0] >> 10) & 3; if ((a[0] & 0x300) == 0x200 || mode >= 2) continue;
        order[no].id = (u8)i; spp[no] = (u8)pr; no++; }
    s16 sdx[128]; memset(sdx, 0, sizeof sdx);
    if (p->objParts) for (int i = 0; i < 128; i++) { const u16 *a = S->oam + i * 4; if ((a[0] & 0x300) == 0x200) continue;
        static const u8 W[3][4] = {{8, 16, 32, 64}, {16, 32, 32, 64}, {8, 8, 16, 32}}, H[3][4] = {{8, 16, 32, 64}, {8, 8, 16, 32}, {16, 32, 32, 64}};
        const unsigned shape = a[0] >> 14, size = a[1] >> 14; if (shape > 2) continue;
        const int dbl = (a[0] & 0x300) == 0x300, bw = W[shape][size] * (dbl ? 2 : 1), bh = H[shape][size] * (dbl ? 2 : 1);
        int x = a[1] & 0x1FF, y = a[0] & 255; if (x >= 256) x -= 512; if (y + bh > 256) y -= 256;
        const int cx = x + bw / 2, cy = y + bh / 2;
        for (int k = 0; k < p->n; k++) { const short *t = p->parts + k * 8; if (t[0] != 3) continue;
            if (cx >= t[4] && cx < t[5] && cy >= t[2] && cy < t[3]) { sdx[i] = t[6]; break; } } }
    int layers[4], nl = 0; for (int i = 0; i < 4; i++) if (p->L[i].on) layers[nl++] = i;
    const int bgBld = eff >= 2 && bY ? (int)(bc & 0x3F) : 0;
    for (int y = 0; y < 192; y++) {
        u32 row[320]; u8 lay[320], sprPr[320]; u32 spr[320]; memset(sprPr, 4, sizeof sprPr); memset(spr, 0, sizeof spr);
        for (int X = 0; X < 320; X++) { row[X] = bd; lay[X] = 5; }
        if (dc & 0x1000) for (int n = 0; n < no; n++) { const int i = order[n].id; const u16 *a = S->oam + i * 4; const int aff = (a[0] & 0x100) != 0, dbl = (a[0] & 0x300) == 0x300;
            static const u8 W[3][4] = {{8, 16, 32, 64}, {16, 32, 32, 64}, {8, 8, 16, 32}}, H[3][4] = {{8, 16, 32, 64}, {8, 8, 16, 32}, {16, 32, 32, 64}};
            const unsigned shape = a[0] >> 14, size = a[1] >> 14; if (shape > 2) continue;
            const int width = W[shape][size], height = H[shape][size], bw = width * (dbl ? 2 : 1), bh = height * (dbl ? 2 : 1);
            int ypos = (y - (a[0] & 255)) & 255; if (ypos >= bh) continue;
            int xs = a[1] & 0x1FF; if (xs >= 256) xs -= 512; xs += sdx[i];
            if (xs + bw <= -32 || xs >= 288) continue;
            const u8 *ov = S->objvram; const int ext = (dc & 0x80000000u) != 0;
            for (int xo = 0; xo < bw; xo++) { const int X = xs + xo + 32; if (X < 0 || X >= 320) continue; int tx, ty; u32 ci;
                if (aff) { const int grp = (a[1] >> 9) & 31; const s16 pa = (s16)S->oam[grp * 16 + 3], pb = (s16)S->oam[grp * 16 + 7], pc = (s16)S->oam[grp * 16 + 11], pd = (s16)S->oam[grp * 16 + 15];
                    const int rx = pa * (xo - bw / 2) + pb * (ypos - bh / 2) + (width / 2 << 8), ry = pc * (xo - bw / 2) + pd * (ypos - bh / 2) + (height / 2 << 8);
                    tx = rx >> 8; ty = ry >> 8; if (rx < 0 || ry < 0 || tx >= width || ty >= height) continue; }
                else { tx = (a[1] & 0x1000) ? width - 1 - xo : xo; ty = (a[1] & 0x2000) ? height - 1 - ypos : ypos; if (xo >= width) continue; }
                if (!sprTexel(a, dc, ov, width, tx, ty, &ci)) continue;
                spr[X] = (a[0] & 0x2000) ? (ext ? toM(*(const u16 *)(S->objext + ((a[2] >> 12) * 256 + ci) * 2)) : lutO[ci]) : lutO[(a[2] >> 12) * 16 + ci];
                sprPr[X] = spp[n]; } }
        for (int pr = 3; pr >= 0; pr--) {
            for (int bi = nl - 1; bi >= 0; bi--) { const int li = layers[bi]; const WcLayer *l = &p->L[li]; if (l->prio != pr) continue;
                if (li == 0 && (dc & 8)) { /* the 3D layer in BG0's place */
                    for (int s = -32; s < 288; s++) { const int c = s - p->dx3d; if (c < 0 || c > 255) continue; const u32 v = S->frame3d[y * 256 + c]; if (!(v >> 24)) continue; row[s + 32] = from3D(v); lay[s + 32] = 0; }
                    continue; }
                const uint8_t ri = p->rowOf[li][y]; short tmp[320]; const short *map; if (ri != 255) map = p->rows[ri]; else { buildRow(p, li, y, tmp); map = tmp; }
                const int py = (int)((y + l->vofs) & (l->h * 8 - 1)); const unsigned ty = (unsigned)(py >> 3) & (l->h - 1); const unsigned fy0 = py & 7;
                int lastT = -1; u16 me = 0; const u8 *trow = 0;
                for (int s = -32; s < 288; s++) { const int pc = map[s + 32]; if (pc < 0) continue; const int t = pc >> 3;
                    if (t != lastT) { lastT = t; const unsigned tx = (unsigned)t & (l->w - 1);
                        const unsigned ma = (l->mapb + ((tx >> 5) + (ty >> 5) * (l->w >> 5)) * 0x800 + ((ty & 31) * 32 + (tx & 31)) * 2) & 0x7FFFF;
                        me = *(const u16 *)(S->bgvram + ma); const unsigned fy = (me & 0x800) ? 7 - fy0 : fy0;
                        trow = S->bgvram + ((l->charb + (me & 0x3FF) * (l->bpp8 ? 64 : 32) + fy * (l->bpp8 ? 8 : 4)) & 0x7FFFF); }
                    unsigned fx = pc & 7; if (me & 0x400) fx = 7 - fx; unsigned ci;
                    if (l->bpp8) ci = trow[fx]; else { const u8 b = trow[fx >> 1]; ci = (fx & 1) ? b >> 4 : b & 15; }
                    if (!ci) continue;
                    row[s + 32] = l->extSlot >= 0 ? toM(*(const u16 *)(S->bgext + l->extSlot * 0x2000 + ((me >> 12) * 256 + ci) * 2)) : lutB[l->bpp8 ? ci : ((me >> 12) * 16 + ci)];
                    lay[s + 32] = (u8)li; } }
            for (int X = 0; X < 320; X++) if (spr[X] && sprPr[X] == pr) { row[X] = spr[X]; lay[X] = 4; } }
        for (int X = 0; X < 320; X++) { u32 c = row[X]; unsigned r = c & 255, g = (c >> 8) & 255, b = (c >> 16) & 255; const int side = X < 32 || X >= 288;
            /* BLDY brighten/darken on the first-target layers (bits 0-3 BGs, 4 sprites, 5 backdrop); in the strips a fade
               that targets BG0 comes from gpuscale (wide_fadefix), as for wide_menus */
            if (bgBld && (bgBld & (1 << lay[X])) && !(side && (bgBld & 1))) {
                if (eff == 2) { r += (255 - r) * bY / 16; g += (255 - g) * bY / 16; b += (255 - b) * bY / 16; } else { r -= r * bY / 16; g -= g * bY / 16; b -= b * bY / 16; } }
            if (side) strip[y * 64 + (X < 32 ? X : X - 256)] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
            else { if (mm == 1) { r += (255 - r) * mf / 16; g += (255 - g) * mf / 16; b += (255 - b) * mf / 16; } else if (mm == 2) { r -= r * mf / 16; g -= g * mf / 16; b -= b * mf / 16; }
                center[y * 256 + X - 32] = r | (g << 8) | (b << 16) | 0xFF000000u; } }
    }
    return 1;
}

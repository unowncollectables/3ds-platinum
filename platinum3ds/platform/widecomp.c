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
    if (!p->n) { for (int s = -32; s < 0; s++) r[s + 32] = (short)(l->h0 & M); for (int s = 256; s < 288; s++) r[s + 32] = (short)((255 + l->h0) & M); return; } /* a screen without parts: the BG's edge columns repeat, the sprites still draw across */
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

static const u8 SPW[3][4] = {{8, 16, 32, 64}, {16, 32, 32, 64}, {8, 8, 16, 32}}, SPH[3][4] = {{8, 16, 32, 64}, {8, 8, 16, 32}, {16, 32, 32, 64}};

/* why the compositor can't draw this screen (0 = it can): 1 forced blank, 2 not graphics mode, 3 windows, 4 alpha blending,
   5 a BG that isn't text, 6 BG mosaic, 7 no 3D frame */
int WC_Check(const WcPlan *p, const u8 *R, int have3d) {
    const u32 dc = *(const u32 *)R;
    if (dc & 0x80) return 1; if (((dc >> 16) & 3) != 1) return 2; if (dc & 0xE000) return 3;
    const u16 bc = *(const u16 *)(R + 0x50); if (((bc >> 6) & 3) == 1 && (bc & 1) && (dc & 8)) return 4; /* alpha on the 3D layer: its own alpha, not done here */
    for (int i = 0; i < 4; i++) { const WcLayer *l = &p->L[i]; if (!l->on) continue; if (!l->text && !(i == 0 && (dc & 8))) return 5; if (l->mosaic) return 6; }
    if ((dc & 8) && !have3d) return 7;
    return 0;
}

/* returns 0 = something this compositor does not do (the melonDS picture stays); else the picture is written */
static int composeNow(const WcPlan *p, const u8 *R, const WcSrc *S, u32 *center, u32 *strip, const u8 *rowMask) {
    const u32 dc = *(const u32 *)R;
    if (WC_Check(p, R, S->frame3d != 0)) return 0;
    const u16 bc = *(const u16 *)(R + 0x50); const unsigned eff = (bc >> 6) & 3;
    unsigned bY = *(const u16 *)(R + 0x54) & 31; if (bY > 16) bY = 16;
    const u16 mb = *(const u16 *)(R + 0x6C); unsigned mf = mb & 31; if (mf > 16) mf = 16; const int mm = mf ? (mb >> 14) & 3 : 0;
    u32 lutB[256], lutO[256]; for (int k = 0; k < 256; k++) { lutB[k] = toM(S->bgpal[k]); lutO[k] = toM(S->objpal[k]); }
    const u32 bd = lutB[0];
    /* sprites: where each one's middle is in a WIDE_OBJS part decides its shift (and whether it is stretched back) */
    u8 ord[128], spp[128]; int no = 0;
    for (int pr = 0; pr <= 3; pr++) for (int i = 0; i < 128; i++) { const u16 *a = S->oam + i * 4; if (((a[2] >> 10) & 3) != pr) continue;
        if ((a[0] & 0x300) == 0x200 || ((a[0] >> 10) & 3) >= 2) continue; ord[no] = (u8)i; spp[no] = (u8)pr; no++; } /* front first */
    s16 sdx[128]; u8 sst[128]; memset(sdx, 0, sizeof sdx); memset(sst, 0, sizeof sst);
    if (p->objParts) for (int i = 0; i < 128; i++) { const u16 *a = S->oam + i * 4; if ((a[0] & 0x300) == 0x200) continue;
        const unsigned shape = a[0] >> 14, size = a[1] >> 14; if (shape > 2) continue;
        const int dbl = (a[0] & 0x300) == 0x300, bw = SPW[shape][size] * (dbl ? 2 : 1), bh = SPH[shape][size] * (dbl ? 2 : 1);
        int x = a[1] & 0x1FF, y = a[0] & 255; if (x >= 256) x -= 512; if (y + bh > 256) y -= 256;
        const int cx = x + bw / 2, cy = y + bh / 2;
        for (int k = 0; k < p->n; k++) { const short *t = p->parts + k * 8; if (t[0] != 3) continue;
            if (cx >= t[4] && cx < t[5] && cy >= t[2] && cy < t[3]) { sdx[i] = t[6]; sst[i] = t[7] == 1; break; } } }
    /* layers front to back: priority, then sprites before BGs, BG0 before BG3 */
    int lorder[4], nl = 0; for (int pr = 0; pr <= 3; pr++) for (int i = 0; i < 4; i++) if (p->L[i].on && p->L[i].prio == pr) lorder[nl++] = i;
    const int bgBld = eff >= 2 && bY ? (int)(bc & 0x3F) : 0;
    const int ab = eff == 1 && (bc & 0x3F); const unsigned fm = bc & 0x3F, sm = (bc >> 8) & 0x3F;
    unsigned eva = *(const u16 *)(R + 0x52) & 31, evb = (*(const u16 *)(R + 0x52) >> 8) & 31; if (eva > 16) eva = 16; if (evb > 16) evb = 16;
    #define PUT(X, C, LY) { if (!done[X]) { row[X] = (C); lay[X] = (u8)(LY); done[X] = 1; if (!(ab && (fm >> (LY) & 1))) { d2[X] = 1; left--; } } \
        else if (!d2[X]) { row2[X] = (C); lay2[X] = (u8)(LY); d2[X] = 1; left--; } }
    for (int y = 0; y < 192; y++) { if (rowMask && !rowMask[y]) continue;
        u32 row[320], row2[320], spr[320]; u8 lay[320], lay2[320], done[320], d2[320], sprPr[320]; memset(done, 0, sizeof done); memset(d2, 0, sizeof d2); memset(sprPr, 4, sizeof sprPr); memset(spr, 0, sizeof spr);
        int left = 320; int sx0 = 320, sx1 = 0; /* run 98: span of sprite pixels on this line */
        if (dc & 0x1000) for (int n = no - 1; n >= 0; n--) { /* back to front here: the front sprite is painted last */
            const int i = ord[n]; const u16 *a = S->oam + i * 4; const int aff = (a[0] & 0x100) != 0, dbl = (a[0] & 0x300) == 0x300;
            const unsigned shape = a[0] >> 14, size = a[1] >> 14; if (shape > 2) continue;
            const int width = SPW[shape][size], height = SPH[shape][size], bw = width * (dbl ? 2 : 1), bh = height * (dbl ? 2 : 1);
            const int ypos = (y - (a[0] & 255)) & 255; if (ypos >= bh) continue;
            int xs0 = a[1] & 0x1FF; if (xs0 >= 256) xs0 -= 512; const int xs = xs0 + sdx[i];
            if ((xs + bw <= -32 || xs >= 288) && !sst[i]) continue;
            const u8 *ov = S->objvram; const int ext = (dc & 0x80000000u) != 0; const int bpp8 = (a[0] & 0x2000) != 0;
            s16 pa = 0, pb = 0, pc = 0, pd = 0; if (aff) { const int grp = (a[1] >> 9) & 31; pa = (s16)S->oam[grp * 16 + 3]; pb = (s16)S->oam[grp * 16 + 7]; pc = (s16)S->oam[grp * 16 + 11]; pd = (s16)S->oam[grp * 16 + 15]; }
            #define SPRCOL(XO, OUT) { int tx_, ty_; u32 ci_; OUT = 0; \
                if (aff) { const int rx = pa * ((XO) - bw / 2) + pb * (ypos - bh / 2) + (width / 2 << 8), ry = pc * ((XO) - bw / 2) + pd * (ypos - bh / 2) + (height / 2 << 8); \
                    tx_ = rx >> 8; ty_ = ry >> 8; if (rx < 0 || ry < 0 || tx_ >= width || ty_ >= height) tx_ = -1; } \
                else { tx_ = (a[1] & 0x1000) ? width - 1 - (XO) : (XO); ty_ = (a[1] & 0x2000) ? height - 1 - ypos : ypos; if ((XO) >= width) tx_ = -1; } \
                if (tx_ >= 0 && sprTexel(a, dc, ov, width, tx_, ty_, &ci_)) OUT = bpp8 ? (ext ? toM(*(const u16 *)(S->objext + ((a[2] >> 12) * 256 + ci_) * 2)) : lutO[ci_]) : lutO[(a[2] >> 12) * 16 + ci_]; }
            for (int xo = 0; xo < bw; xo++) { const int X = xs + xo + 32; if (X < 0 || X >= 320) continue; u32 c; SPRCOL(xo, c); if (c) { spr[X] = c; sprPr[X] = spp[n]; if (X < sx0) sx0 = X; if (X >= sx1) sx1 = X + 1; } }
            if (sst[i] && !aff && sdx[i]) { /* stretched: the edge column drawn back over the gap it left */
                const int dx = sdx[i]; u32 c; if (dx > 0) { SPRCOL(0, c); for (int X = xs0 + 32; X < xs0 + dx + 32; X++) if (c && X >= 0 && X < 320) { spr[X] = c; sprPr[X] = spp[n]; if (X < sx0) sx0 = X; if (X >= sx1) sx1 = X + 1; } }
                else { SPRCOL(bw - 1, c); for (int X = xs + bw + 32; X < xs0 + bw + 32; X++) if (c && X >= 0 && X < 320) { spr[X] = c; sprPr[X] = spp[n]; if (X < sx0) sx0 = X; if (X >= sx1) sx1 = X + 1; } } }
            #undef SPRCOL
        }
        for (int li = 0; li < nl && left; li++) { const int L = lorder[li]; const WcLayer *l = &p->L[L];
            /* sprites of this priority go in front of the BGs of the same priority */
            if (li == 0 || p->L[lorder[li - 1]].prio != l->prio) { for (int pr = (li ? p->L[lorder[li - 1]].prio + 1 : 0); pr <= l->prio; pr++) for (int X = sx0; X < sx1; X++) if (!d2[X] && spr[X] && sprPr[X] == pr) { PUT(X, spr[X], 4); spr[X] = 0; } }
            if (!left) break;
            if (L == 0 && (dc & 8)) { /* the 3D layer in BG0's place */
                for (int s = -32; s < 288; s++) { const int X = s + 32, c = s - p->dx3d; if (d2[X] || c < 0 || c > 255) continue; const u32 v = S->frame3d[y * 256 + c]; if (!(v >> 24)) continue; PUT(X, from3D(v), 0); }
                continue; }
            const uint8_t ri = p->rowOf[L][y]; short tmp[320]; const short *map; if (ri != 255) map = p->rows[ri]; else { buildRow(p, L, y, tmp); map = tmp; }
            const int py = (int)((y + l->vofs) & (l->h * 8 - 1)); const unsigned ty = (unsigned)(py >> 3) & (l->h - 1); const unsigned fy0 = py & 7;
            int lastT = -1; u16 me = 0; const u8 *trow = 0;
            for (int X = 0; X < 320; X++) { if (d2[X]) continue; const int pc = map[X]; if (pc < 0) continue; const int t = pc >> 3;
                if (t != lastT) { lastT = t; const unsigned tx = (unsigned)t & (l->w - 1);
                    const unsigned ma = (l->mapb + ((tx >> 5) + (ty >> 5) * (l->w >> 5)) * 0x800 + ((ty & 31) * 32 + (tx & 31)) * 2) & 0x7FFFF;
                    me = *(const u16 *)(S->bgvram + ma); const unsigned fy = (me & 0x800) ? 7 - fy0 : fy0;
                    trow = S->bgvram + ((l->charb + (me & 0x3FF) * (l->bpp8 ? 64 : 32) + fy * (l->bpp8 ? 8 : 4)) & 0x7FFFF); }
                unsigned fx = pc & 7; if (me & 0x400) fx = 7 - fx; unsigned ci;
                if (l->bpp8) ci = trow[fx]; else { const u8 b = trow[fx >> 1]; ci = (fx & 1) ? b >> 4 : b & 15; }
                if (!ci) continue;
                PUT(X, (l->extSlot >= 0 ? toM(*(const u16 *)(S->bgext + l->extSlot * 0x2000 + ((me >> 12) * 256 + ci) * 2)) : lutB[l->bpp8 ? ci : ((me >> 12) * 16 + ci)]), L); } }
        if (left) { /* sprites behind every BG, then the backdrop */
            for (int X = 0; X < 320; X++) if (!d2[X]) { if (spr[X]) PUT(X, spr[X], 4); if (!d2[X]) PUT(X, bd, 5); } }
        if (!bgBld && !mm && !ab) { memcpy(center + y * 256, row + 32, 256 * 4);
            for (int X = 0; X < 64; X++) { const u32 c = row[X < 32 ? X : X + 256]; strip[y * 64 + X] = ((c & 255) << 24) | (((c >> 8) & 255) << 16) | (((c >> 16) & 255) << 8) | 0xFF; }
            continue; }
        for (int X = 0; X < 320; X++) { u32 c = row[X];
            if (ab && (fm >> lay[X] & 1) && (sm >> lay2[X] & 1)) { const u32 c2 = row2[X]; /* alpha blend: top * EVA + second * EVB */
                unsigned r1 = (c & 255) * eva + (c2 & 255) * evb, g1 = ((c >> 8) & 255) * eva + ((c2 >> 8) & 255) * evb, b1 = ((c >> 16) & 255) * eva + ((c2 >> 16) & 255) * evb;
                r1 >>= 4; g1 >>= 4; b1 >>= 4; if (r1 > 255) r1 = 255; if (g1 > 255) g1 = 255; if (b1 > 255) b1 = 255; c = r1 | (g1 << 8) | (b1 << 16); }
            unsigned r = c & 255, g = (c >> 8) & 255, b = (c >> 16) & 255; const int side = X < 32 || X >= 288;
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

/* the same picture is asked for again and again while a menu waits, so the result is kept: a hash of everything
   composeNow reads (registers, palettes, OAM, the BG map entries and tiles on screen, the tiles of each sprite) decides
   whether it is made again. The 3D layer changes every frame, so hashed whole. */
static u32 cCenterA[2][256 * 192], cStripA[2][64 * 192]; static u32 cHashA[2]; static int cValidA[2]; /* run 97: one cache per eye */
static u32 hashSpan(u32 h, const u8 *b, unsigned n) { const u32 *w = (const u32 *)b; for (unsigned i = 0; i < n / 4; i++) h = (h ^ w[i]) * 16777619u; return h; }
static u32 inputHash(const WcPlan *p, const u8 *R, const WcSrc *S) {
    const u32 dc = *(const u32 *)R; u32 h = 2166136261u ^ p->hash;
    h = hashSpan(h, R, 0x80); h = hashSpan(h, (const u8 *)S->bgpal, 512); h = hashSpan(h, (const u8 *)S->objpal, 512); h = hashSpan(h, (const u8 *)S->oam, 1024);
    for (int i = 0; i < 4; i++) { const WcLayer *l = &p->L[i]; if (!l->on) continue;
        if (l->extSlot >= 0) h = hashSpan(h, S->bgext + l->extSlot * 0x2000, 0x2000);
        const unsigned tb = l->hofs >> 3; const unsigned tyb = l->vofs >> 3;
        for (unsigned r = 0; r < 25; r++) { const unsigned ty = (tyb + r) & (l->h - 1);
            for (unsigned c = 0; c < 42; c++) { const unsigned tx = (tb + c - 4) & (l->w - 1);
                const unsigned ma = (l->mapb + ((tx >> 5) + (ty >> 5) * (l->w >> 5)) * 0x800 + ((ty & 31) * 32 + (tx & 31)) * 2) & 0x7FFFF;
                const u16 me = *(const u16 *)(S->bgvram + ma); h = (h ^ me) * 16777619u;
                h = hashSpan(h, S->bgvram + ((l->charb + (me & 0x3FF) * (l->bpp8 ? 64 : 32)) & 0x7FFFF), l->bpp8 ? 64 : 32); } } }
    if (dc & 0x80000000u) h = hashSpan(h, S->objext, 0x2000);
    if (dc & 0x1000) for (int i = 0; i < 128; i++) { const u16 *a = S->oam + i * 4; if ((a[0] & 0x300) == 0x200) continue;
        const unsigned shape = a[0] >> 14, size = a[1] >> 14; if (shape > 2) continue;
        const int width = SPW[shape][size], height = SPH[shape][size], bpp8 = (a[0] & 0x2000) != 0; const u32 tile = a[2] & 0x3FF;
        for (int t8 = 0; t8 < height / 8; t8++) { u32 addr = (dc & 0x10) ? ((tile << ((dc >> 20) & 3)) + (u32)((t8 * (width >> 3)) << (bpp8 ? 1 : 0))) << 5 : (tile + (u32)(t8 * 0x20)) << 5;
            h = hashSpan(h, S->objvram + (addr & 0x3FFFF), (unsigned)(width >> 3) * (bpp8 ? 64 : 32)); } }
    return h;
}
/* run 98 wide_dirty: per-line input hashes; only lines whose inputs changed are drawn again (the rest come from the last
   picture of that eye). A line's inputs: global state (registers, both palettes, extended palettes in use, the plan), for
   each BG the map entries + the tile row bytes the line actually samples (through the same column map composeNow uses),
   every sprite covering the line (its OAM entry, affine group, and a hash of all its tile data), and the 3D layer's row. */
static int dirtyMode; static unsigned dStatFull, dStatRows, dStatFrames, dStatBad; unsigned dStatRace; int dBadRow = -1, dBadMask;
void WC_SetDirty(int mode) { dirtyMode = mode; }
void WC_DirtyStats(unsigned *frames, unsigned *rows, unsigned *full, unsigned *bad) { *frames = dStatFrames; *rows = dStatRows; *full = dStatFull; *bad = dStatBad; dStatFrames = dStatRows = dStatFull = dStatBad = 0; }
static void rowHashes(const WcPlan *p, const u8 *R, const WcSrc *S, u32 *rh) {
    const u32 dc = *(const u32 *)R; u32 g = 2166136261u ^ p->hash;
    g = hashSpan(g, R, 0x80); g = hashSpan(g, (const u8 *)S->bgpal, 512); g = hashSpan(g, (const u8 *)S->objpal, 512);
    g = hashSpan(g, (const u8 *)&p->dx3d, sizeof p->dx3d); g = hashSpan(g, (const u8 *)&p->n, sizeof p->n);
    for (int i = 0; i < 4; i++) if (p->L[i].on && p->L[i].extSlot >= 0) g = hashSpan(g, S->bgext + p->L[i].extSlot * 0x2000, 0x2000);
    if (dc & 0x80000000u) g = hashSpan(g, S->objext, 0x2000);
    for (int y = 0; y < 192; y++) rh[y] = g;
    /* sprites */
    if (dc & 0x1000) for (int i = 0; i < 128; i++) { const u16 *a = S->oam + i * 4; if ((a[0] & 0x300) == 0x200) continue;
        const unsigned shape = a[0] >> 14, size = a[1] >> 14; if (shape > 2) continue;
        const int width = SPW[shape][size], height = SPH[shape][size], bpp8 = (a[0] & 0x2000) != 0; const u32 tile = a[2] & 0x3FF;
        const int dbl = (a[0] & 0x300) == 0x300, bh = height * (dbl ? 2 : 1);
        u32 sh = 2166136261u ^ (u32)i; sh = hashSpan(sh, (const u8 *)a, 8);
        if (a[0] & 0x100) { const int grp = (a[1] >> 9) & 31; for (int k = 0; k < 4; k++) sh = (sh ^ S->oam[grp * 16 + 3 + k * 4]) * 16777619u; }
        for (int t8 = 0; t8 < height / 8; t8++) { u32 addr = (dc & 0x10) ? ((tile << ((dc >> 20) & 3)) + (u32)((t8 * (width >> 3)) << (bpp8 ? 1 : 0))) << 5 : (tile + (u32)(t8 * 0x20)) << 5;
            sh = hashSpan(sh, S->objvram + (addr & 0x3FFFF), (unsigned)(width >> 3) * (bpp8 ? 64 : 32)); }
        const int y0 = a[0] & 255; for (int k = 0; k < bh; k++) { const int y = (y0 + k) & 255; if (y < 192) rh[y] = (rh[y] ^ sh) * 16777619u; } }
    /* BG layers: what each line samples */
    for (int L = 0; L < 4; L++) { const WcLayer *l = &p->L[L]; if (!l->on) continue; if (L == 0 && (dc & 8)) continue;
        for (int y = 0; y < 192; y++) {
            const uint8_t ri = p->rowOf[L][y]; short tmp[320]; const short *map; if (ri != 255) map = p->rows[ri]; else { buildRow(p, L, y, tmp); map = tmp; }
            const int py = (int)((y + l->vofs) & (l->h * 8 - 1)); const unsigned ty = (unsigned)(py >> 3) & (l->h - 1); const unsigned fy0 = py & 7;
            u32 h = rh[y] ^ (u32)(L * 0x9E3779B1u); int lastT = -0x7FFF;
            for (int X = 0; X < 320; X++) { const int pc = map[X]; if (pc < 0) continue; const int t = pc >> 3; if (t == lastT) continue; lastT = t;
                const unsigned tx = (unsigned)t & (l->w - 1);
                const unsigned ma = (l->mapb + ((tx >> 5) + (ty >> 5) * (l->w >> 5)) * 0x800 + ((ty & 31) * 32 + (tx & 31)) * 2) & 0x7FFFF;
                const u16 me = *(const u16 *)(S->bgvram + ma); const unsigned fy = (me & 0x800) ? 7 - fy0 : fy0; h = (h ^ me) * 16777619u;
                h = hashSpan(h, S->bgvram + ((l->charb + (me & 0x3FF) * (l->bpp8 ? 64 : 32) + fy * (l->bpp8 ? 8 : 4)) & 0x7FFFF), l->bpp8 ? 8 : 4); }
            rh[y] = h; } }
    if ((dc & 8) && S->frame3d) for (int y = 0; y < 192; y++) rh[y] = hashSpan(rh[y], (const u8 *)(S->frame3d + y * 256), 1024);
}
static u32 dRH[2][192]; static int dValid[2]; static u32 vCA[2][256 * 192], vSA[2][64 * 192];
int WC_ComposeEye(const WcPlan *p, const u8 *R, const WcSrc *S, u32 *center, u32 *strip, int eye) {
    const u32 dc = *(const u32 *)R; if (WC_Check(p, R, S->frame3d != 0)) return 0;
    eye = eye ? 1 : 0; u32 *cCenter = cCenterA[eye], *cStrip = cStripA[eye];
    if (dirtyMode) { u32 rh[192]; u8 mask[192]; int n = 0; rowHashes(p, R, S, rh);
        static int band[2]; const int b0 = band[eye] * 8; band[eye] = (band[eye] + 1) % 24; /* rolling 8-line refresh: any missed change lasts < 24 frames */
        for (int y = 0; y < 192; y++) { mask[y] = !dValid[eye] || rh[y] != dRH[eye][y] || (y >= b0 && y < b0 + 8); n += mask[y]; }
        if (n && !composeNow(p, R, S, cCenter, cStrip, n == 192 ? 0 : mask)) return 0;
        memcpy(dRH[eye], rh, sizeof rh); dValid[eye] = 1; cValidA[eye] = 0; dStatFrames++; dStatRows += (unsigned)n; if (n == 192) dStatFull++;
        u32 *vC = vCA[eye], *vS = vSA[eye]; if (dirtyMode == 2 && composeNow(p, R, S, vC, vS, 0) && (memcmp(vC, cCenter, 256 * 192 * 4) || memcmp(vS, cStrip, 64 * 192 * 4))) { u32 rh2[192]; rowHashes(p, R, S, rh2); if (!memcmp(rh2, rh, sizeof rh)) { dStatBad++; for (int y = 0; y < 192; y++) if (memcmp(vC + y * 256, cCenter + y * 256, 1024)) { dBadRow = y; dBadMask = mask[y]; break; } } else dStatRace++; memcpy(cCenter, vC, 256 * 192 * 4); memcpy(cStrip, vS, 64 * 192 * 4); }
        memcpy(center, cCenter, 256 * 192 * 4); memcpy(strip, cStrip, 64 * 192 * 4); return 1; }
    u32 h = inputHash(p, R, S);
    if (dc & 8) h = hashSpan(h, (const u8 *)S->frame3d, 256 * 192 * 4); /* the 3D layer is part of the picture; a still scene keeps it */
    if (cValidA[eye] && h == cHashA[eye]) { memcpy(center, cCenter, 256 * 192 * 4); memcpy(strip, cStrip, 64 * 192 * 4); return 1; }
    if (!composeNow(p, R, S, center, strip, 0)) return 0;
    dValid[eye] = 0;
    memcpy(cCenter, center, 256 * 192 * 4); memcpy(cStrip, strip, 64 * 192 * 4); cHashA[eye] = h; cValidA[eye] = 1; return 1;
}
int WC_Compose(const WcPlan *p, const u8 *R, const WcSrc *S, u32 *center, u32 *strip) { return WC_ComposeEye(p, R, S, center, strip, 0); }

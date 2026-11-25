/* ss3ds sharp game text (2026-11-18/19, Nathan: "can we do this for all text boxes and everything in the entire game", "i dont want
   any sloppy text left around after i toggle that").
   The game prints every string through CopyGlyphToWindow (bg_window.c) into a window's pixel buffer, copies that buffer to BG VRAM,
   and the DS picture (256x192) is stretched 1.25x to the screen, so letters come out uneven and soft. Patch 0289 hooks the window
   code: every pixel a glyph changes is RECORDED (with the pixel that was under it), the copy that goes to VRAM has those pixels put
   back to what was under them (the game's own pixel buffer is never touched, so every game routine that reads it still sees the
   text), and the text is drawn at 1 font pixel = 1 screen pixel in a native layer on the screen that shows that engine (gpuscale.c:
   top layer 400x240 on both eyes, bottom layer 320x240 under the Bottom menu panel).
   All text windows of both display engines in every scene (overworld, battle, menus, bag, party, summary, Pokedex...) on 4 bit
   text BGs. Text copied into sprites (battle move names, HP box names, naming screen letters): see "text the game copies into sprites" below.
   8 bit windows and affine / extended tile BGs are followed while the BG is 1:1 (not rotated or scaled).
   Safety rules (nothing doubled, nothing lost):
   - Only pixels a glyph changed are taken out, and only while the buffer still holds exactly the value the glyph wrote; a rectangle
     drawn over text (Blit / FillRect, the blinking arrow) or a full fill forgets the text pixels it covers.
   - The window's VRAM is checked every frame against what was copied there (hash); when anything else wrote over it (a new screen,
     another window) the window is not drawn natively.
   - When a screen cannot show the native layer this frame (Bottom menu panel, settings app, held pictures, panel party, title, 1:1
     scale, layer not made) or the setting is OFF, P3DS_GtSync puts the full text back into VRAM at once, and takes it out again
     when the layer can show it: toggling applies to text already on screen.
   - The native layer draws a text pixel only where the BG tile map still shows that window's tile, the BG is on, the DS window
     regions show it and no higher priority text BG covers it. Master brightness, BLDCNT fades and alpha are applied.
   - If the tile map does not currently show this window's tiles (GTEXT cannot place the native glyphs), text is left in (or put
     back into) the DS picture so it is never blank. The box app (OVY_14 / Mobile PC and the Center PC) hit this: char tiles were
     taken out, then every bottom-screen glyph was hidden as "map" and nothing was drawn.
   - Wide layouts (widecomp.c): text on a moved panel is placed with the panel (WC_MapColumn).
   Log: [GTEXT]. */
#include <3ds.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "widecomp.h"

extern unsigned char PSPNative_GfxRegisters[];
extern unsigned char s_HW_BG_PLTT[], s_HW_DB_BG_PLTT[];
extern const unsigned char *P3DS_GtVram(int eng);
extern int P3DS_SharpGameTextCfg(void);
extern int P3DS_ScaleMode(void);
extern int P3DS_GtLayerReady(int scr);
extern void PSPNativeMemLog(const char *, ...);

/* set by render3ds.cpp: bit 0 = the top screen shows the live engine picture this present, bit 1 = the bottom does */
volatile unsigned P3DS_GtScreensNow;
/* set by render3ds.cpp when the top screen is drawn by the wide layout engine this frame (else NULL) */
const WcPlan *volatile P3DS_GtWidePlan; volatile int P3DS_GtWideEng;

/* what the game side passes about a window (same struct in the patched bg_window.c) */
typedef struct { const void *buf; int bg, left, top, wT, hT, pal, base, is4bpp, is8bpp; } P3DS_GtDesc;

enum { kWins = 48, kGlyphs = 4096, kScratch = 32 * 32 * 64, kNoInk = 0xFFFF, kLoose = 0xFF };   /* kLoose: a window on no BG (AddTextWindowTopLeftCorner, its pixels are copied into sprites) */
typedef struct {
    const void *buf; u8 used, gave, shown, pend, bg, left, top, wT, hT, pal, miss, vValid, loose, dirty, bpp8; u16 base;
    u16 *und; u8 *val;              /* per pixel (row major, wT*8 x hT*8): und = value before the text (kNoInk: not text), val = text value */
    unsigned nInk, stamp;
    u8 *vram; unsigned vSize, vHash;
} GtWin;
typedef struct { int16_t x, y; u8 win, used, x0, x1, y0, y1; } GtGlyph;   /* x0..x1, y0..y1: changed pixels inside the 16x32 cell at x,y */
static GtWin gWin[kWins];
static GtGlyph gGl[kGlyphs];
static unsigned gGlCur;
static u8 gScratch[kScratch] __attribute__((aligned(4))), gScratchR[kScratch] __attribute__((aligned(4)));
static unsigned gGen = 1, gStamp, gErased, gPlain, gRecorded, gFull, gNoMem, gStale, gSyncOn, gSyncOff, gChanged;
static volatile unsigned gOkBits; static volatile int gReady[2];
static LightLock gLk; static int gLkInit;
static void lk(void) { if (!gLkInit) { LightLock_Init(&gLk); gLkInit = 1; } LightLock_Lock(&gLk); }
static void ulk(void) { LightLock_Unlock(&gLk); }

static unsigned reg16(unsigned o) { return *(volatile u16 *)(PSPNative_GfxRegisters + o); }
static unsigned reg32(unsigned o) { return *(volatile u32 *)(PSPNative_GfxRegisters + o); }
static int engOf(const GtWin *w) { return w->bg >= 4; }
/* which screen shows an engine: main (A) is on the top when POWCNT bit 15 is set */
static int scrOf(int eng) { const int mainTop = (reg16(0x304) & 0x8000) != 0; return (eng == 0) == mainTop ? 0 : 1; }
static int cfgOn(void) { return P3DS_SharpGameTextCfg() != 0; }
static int screenOk(int scr) { return ((gOkBits >> scr) & 1) && gReady[scr] && (scr == 1 || P3DS_ScaleMode() != 0); }
static unsigned hashMem(const u8 *p, unsigned n) { unsigned h = 2166136261u; for (unsigned i = 0; i + 4 <= n; i += 4) h = (h ^ *(const u32 *)(p + i)) * 16777619u; return h; }
static unsigned fnv(unsigned h, unsigned v) { return (h ^ v) * 16777619u; }
static inline unsigned nibAt(const u8 *b, int W, int X, int Y) { return (b[(((Y >> 3) * W + (X >> 3)) << 5) + ((Y & 7) << 2) + ((X & 7) >> 1)] >> ((X & 1) * 4)) & 15; }
static inline void nibSet(u8 *b, int W, int X, int Y, unsigned v) { u8 *d = b + (((Y >> 3) * W + (X >> 3)) << 5) + ((Y & 7) << 2) + ((X & 7) >> 1);
    *d = (u8)((X & 1) ? ((*d & 0x0F) | (v << 4)) : ((*d & 0xF0) | v)); }
/* a window pixel, 4 or 8 bit tiles */
static inline unsigned pixAt(const u8 *b, int W, int X, int Y, int b8) { return b8 ? b[(((Y >> 3) * W + (X >> 3)) << 6) + ((Y & 7) << 3) + (X & 7)] : nibAt(b, W, X, Y); }
static inline void pixSet(u8 *b, int W, int X, int Y, unsigned v, int b8) { if (b8) b[(((Y >> 3) * W + (X >> 3)) << 6) + ((Y & 7) << 3) + (X & 7)] = (u8)v; else nibSet(b, W, X, Y, v); }
static inline unsigned tileBytes(const GtWin *w) { return w->bpp8 ? 64u : 32u; }

/* can the native layer follow this window's BG now? An affine or 8 bit BG (the trainer card flip scales its BG) cannot:
   its text goes back into the DS picture, which transforms it with the BG */
typedef struct GtBg GtBg;
static void readBg(int eng, int b, GtBg *o);
static int bgDrawable(const GtWin *w);
static int gtWinMapShows(const GtWin *w); /* tile map still has this window's tiles */
static void dropGlyphs(int wi) { for (int i = 0; i < kGlyphs; i++) if (gGl[i].used && gGl[i].win == wi) gGl[i].used = 0; gGen++; }
static void clearInk(GtWin *w) { if (w->und) memset(w->und, 0xFF, (size_t)w->wT * 8 * w->hT * 8 * 2); w->nInk = 0; dropGlyphs((int)(w - gWin)); }
static void freeWin(GtWin *w) { dropGlyphs((int)(w - gWin)); free(w->und); free(w->val); memset(w, 0, sizeof *w); }
/* put the full text back into this window's VRAM (when it is still ours) and forget the window */
static void restoreVram(GtWin *w) {
    if (w->shown && w->vValid && w->vram && w->buf && w->vSize <= kScratch && hashMem(w->vram, w->vSize) == w->vHash) memcpy(w->vram, w->buf, w->vSize);
    w->shown = 0;
}
static void snapLoose(GtWin *w);
static GtWin *gtFind(const P3DS_GtDesc *d, int create) {
    GtWin *w = 0, *fr = 0;
    for (int i = 0; i < kWins; i++) { if (gWin[i].used && gWin[i].buf == d->buf) { w = &gWin[i]; break; } if (!gWin[i].used && !fr) fr = &gWin[i]; }
    if (w && (w->bg != d->bg || w->wT != d->wT || w->hT != d->hT || w->bpp8 != (d->is8bpp != 0))) { restoreVram(w); freeWin(w); fr = w; w = 0; }   /* the buffer is another window now */
    if (!w) {
        if (!create) return 0;
        if (!fr) { /* evict the window used longest ago, preferring one whose text is not taken out */
            GtWin *o = 0; for (int i = 0; i < kWins; i++) if (!o || (gWin[i].shown < o->shown) || (gWin[i].shown == o->shown && gWin[i].stamp < o->stamp)) o = &gWin[i];
            restoreVram(o); if (o->loose) snapLoose(o); freeWin(o); fr = o; }
        w = fr; memset(w, 0, sizeof *w); w->used = 1; w->buf = d->buf; w->bg = (u8)d->bg; w->wT = (u8)d->wT; w->hT = (u8)d->hT; w->loose = d->bg == kLoose; w->bpp8 = d->is8bpp != 0; }
    w->left = (u8)d->left; w->top = (u8)d->top; w->pal = (u8)d->pal; w->base = (u16)d->base; w->stamp = ++gStamp;
    return w;
}
static void giveUp(GtWin *w, const char *why) {
    if (w->gave) return;
    w->gave = 1; clearInk(w);
    static unsigned n; if (n++ < 8) PSPNativeMemLog("[GTEXT] window bg%d %dx%d: %s, its text stays in the DS picture", w->bg, w->wT, w->hT, why);
}

/* ---- calls from the patched bg_window.c (game thread) ---- */
/* should the next glyph into this window be recorded? */
int P3DS_GtWant(const P3DS_GtDesc *d) {
    if (!(d->is4bpp || (d->is8bpp && d->bg >= 0 && d->bg <= 7)) || d->bg < 0 || (d->bg > 7 && d->bg != kLoose) || d->wT < 1 || d->hT < 1 || d->wT > 32 || d->hT > 32 || !d->buf) return 0;
    if (!cfgOn()) return 0;
    lk();
    GtWin *w = gtFind(d, 1); int r = 0;
    if (w && !w->gave) {
        if (!w->und) { const size_t n = (size_t)w->wT * 8 * w->hT * 8; w->und = (u16 *)malloc(n * 2); w->val = (u8 *)malloc(n);
            if (!w->und || !w->val) { free(w->und); free(w->val); w->und = 0; w->val = 0; gNoMem++; giveUp(w, "no memory for its text layer"); }
            else memset(w->und, 0xFF, n * 2); }
        r = w->und != 0;
    }
    ulk();
    return r;
}
static u16 gPre[32][16];
void P3DS_GtPre(const P3DS_GtDesc *d, int x, int y) {
    const int wpx = d->wT * 8, hpx = d->hT * 8; const u8 *b = (const u8 *)d->buf;
    for (int py = 0; py < 32; py++) for (int px = 0; px < 16; px++) { const int X = x + px, Y = y + py;
        gPre[py][px] = (X < 0 || Y < 0 || X >= wpx || Y >= hpx) ? 0xFFFF : (u16)pixAt(b, d->wT, X, Y, d->is8bpp); }
}
void P3DS_GtPost(const P3DS_GtDesc *d, int x, int y) {
    lk();
    GtWin *w = gtFind(d, 0);
    if (!w || w->gave || !w->und) { ulk(); return; }
    const int wpx = w->wT * 8; const u8 *b = (const u8 *)d->buf; int x0 = 16, x1 = -1, y0 = 32, y1 = -1;
    for (int py = 0; py < 32; py++) for (int px = 0; px < 16; px++) { if (gPre[py][px] == 0xFFFF) continue; const int X = x + px, Y = y + py;
        const unsigned now = pixAt(b, w->wT, X, Y, w->bpp8); if (now == gPre[py][px]) continue;
        const int i = Y * wpx + X; if (w->und[i] == kNoInk) { w->und[i] = gPre[py][px]; w->nInk++; } w->val[i] = (u8)now;
        if (px < x0) x0 = px; if (px > x1) x1 = px; if (py < y0) y0 = py; if (py > y1) y1 = py; }
    if (x1 >= 0 && w->loose) w->dirty = 1;
    else if (x1 >= 0) {
        int slot = -1; for (unsigned k = 0; k < kGlyphs; k++) { const unsigned i = (gGlCur + k) % kGlyphs; if (!gGl[i].used) { slot = (int)i; gGlCur = i + 1; break; } }
        if (slot < 0) { gFull++; giveUp(w, "glyph pool full"); }
        else { GtGlyph *g = &gGl[slot]; g->used = 1; g->win = (u8)(w - gWin); g->x = (int16_t)x; g->y = (int16_t)y; g->x0 = (u8)x0; g->x1 = (u8)x1; g->y0 = (u8)y0; g->y1 = (u8)y1; gRecorded++; gGen++; }
    }
    ulk();
}
void P3DS_GtFill(const P3DS_GtDesc *d, int fillValue) {
    (void)fillValue; lk();
    GtWin *w = gtFind(d, 0); if (w) { clearInk(w); w->gave = 0; w->dirty = 1; }
    ulk();
}
/* something other than text (the blinking down arrow, a cleared rectangle, a picture) is drawn into a rectangle of the window:
   the text pixels there are overwritten, so they are forgotten (never put back, never drawn natively); the rest stays sharp */
void P3DS_GtRect(const P3DS_GtDesc *d, int x, int y, int wd, int ht) {
    lk();
    GtWin *w = gtFind(d, 0);
    if (w && w->und && w->nInk) {
        const int wpx = w->wT * 8, hpx = w->hT * 8, wi = (int)(w - gWin);
        for (int Y = y < 0 ? 0 : y; Y < y + ht && Y < hpx; Y++) for (int X = x < 0 ? 0 : x; X < x + wd && X < wpx; X++) w->und[Y * wpx + X] = kNoInk;
        for (int i = 0; i < kGlyphs; i++) { GtGlyph *g = &gGl[i]; if (!g->used || g->win != wi) continue;
            if (g->x + g->x0 >= x && g->x + g->x1 < x + wd && g->y + g->y0 >= y && g->y + g->y1 < y + ht) g->used = 0; }
        gGen++; w->dirty = 1;
    }
    ulk();
}
/* ScrollWindow4bpp: 0 = up (row Y takes row Y + n), 1 = down (row Y takes row Y - n); new rows get the fill (not text); 2/3 do nothing */
void P3DS_GtScroll(const P3DS_GtDesc *d, int dir, int amount, int fillValue) {
    (void)fillValue; if (dir > 1 || amount <= 0) return;
    lk();
    GtWin *w = gtFind(d, 0);
    if (w && w->und) {
        const int wpx = w->wT * 8, hpx = w->hT * 8, wi = (int)(w - gWin);
        if (dir == 0) { for (int Y = 0; Y < hpx; Y++) { const int s = Y + amount;
                if (s < hpx) { memcpy(w->und + Y * wpx, w->und + s * wpx, wpx * 2); memcpy(w->val + Y * wpx, w->val + s * wpx, wpx); } else memset(w->und + Y * wpx, 0xFF, wpx * 2); } }
        else { for (int Y = hpx - 1; Y >= 0; Y--) { const int s = Y - amount;
                if (s >= 0) { memcpy(w->und + Y * wpx, w->und + s * wpx, wpx * 2); memcpy(w->val + Y * wpx, w->val + s * wpx, wpx); } else memset(w->und + Y * wpx, 0xFF, wpx * 2); } }
        for (int i = 0; i < kGlyphs; i++) { GtGlyph *g = &gGl[i]; if (!g->used || g->win != wi) continue;
            g->y = (int16_t)(g->y + (dir == 0 ? -amount : amount));
            if (g->y + g->y1 < 0 || g->y + g->y0 >= hpx) g->used = 0; }
        gGen++; w->dirty = 1;
    }
    ulk();
}
void P3DS_GtRemove(const void *buf) {
    lk();
    for (int i = 0; i < kWins; i++) if (gWin[i].used && gWin[i].buf == buf) { if (gWin[i].loose) snapLoose(&gWin[i]); freeWin(&gWin[i]); }
    ulk();
}
/* the buffer to copy to VRAM: the window's own, or a copy with the text pixels put back to what was under them */
const void *P3DS_GtUpload(const P3DS_GtDesc *d, unsigned size) {
    if (d->bg < 0 || d->bg > 7) return d->buf;
    lk();
    GtWin *w = gtFind(d, 0); const void *r = d->buf;
    if (w) {
        w->pend = 0;
        if (w->und && !w->gave && w->nInk && size == w->wT * w->hT * tileBytes(w) && size <= kScratch) {
            const int wpx = w->wT * 8, hpx = w->hT * 8, W = w->wT; const u8 *b = (const u8 *)d->buf;
            const int want = cfgOn() && screenOk(scrOf(engOf(w))) && bgDrawable(w) && gtWinMapShows(w);
            if (want) memcpy(gScratch, b, size);
            unsigned n = 0, live = 0;
            for (int Y = 0; Y < hpx; Y++) for (int X = 0; X < wpx; X++) { const int i = Y * wpx + X; if (w->und[i] == kNoInk) continue;
                if (pixAt(b, W, X, Y, w->bpp8) != w->val[i]) { w->und[i] = kNoInk; continue; }   /* written over by something we did not see: not text any more */
                live++; if (want && w->val[i] != w->und[i]) { pixSet(gScratch, W, X, Y, w->und[i], w->bpp8); n++; } }
            w->nInk = live;
            if (want && n) { r = gScratch; w->pend = 1; gErased++;
                static unsigned lg; if (lg < 12) { lg++; PSPNativeMemLog("[GTEXT] window bg%d %dx%d tiles at %d,%d (%s screen): %u text pixels taken out of the VRAM copy",
                    w->bg, w->wT, w->hT, w->left, w->top, scrOf(engOf(w)) ? "bottom" : "top", n); } }
            else if (live) gPlain++;
        }
    }
    ulk();
    return r;
}
/* after the copy: where it went, and its hash (the per-frame check that the VRAM still holds it) */
void P3DS_GtLoaded(const P3DS_GtDesc *d, const void *vram, unsigned size) {
    if (d->bg < 0 || d->bg > 7) return;
    lk();
    GtWin *w = gtFind(d, 0);
    if (w) {
        w->vram = (u8 *)vram; w->vSize = size; w->vHash = hashMem(w->vram, size); w->vValid = size <= kScratch; w->miss = 0;
        if (w->shown != w->pend) gGen++;
        w->shown = w->pend; w->pend = 0;
    }
    ulk();
}

/* ---- text the game copies into sprites (battle move names and PP, HP box names, SAFARI/PARK BALLS...) ----
   Those strings are printed into a window on no BG (AddTextWindowTopLeftCorner) and its 8x8 tiles are copied into OBJ VRAM
   (TextOBJ, battle_hp_bar.c), so no hook sees where they land. Each such window's text tiles are kept as records (the tile as
   printed + the same tile with the text pixels put back to the fill), also after the window is removed. Every present, the
   tiles that visible sprites show are compared with the records: a match is replaced by its text-free version for this
   present's DS picture only (P3DS_GtObjRestore puts the printed tile back before the game runs again, so the game never sees
   a changed VRAM and a tile reused for something else can never pick up old text), and the native layer draws its text pixels
   at the sprite's place. Touching text tiles are drawn as one block: centred where the DS picture has it, 1 font pixel = 1
   screen pixel. Only 4 bit, non affine sprites; DS window regions and BGs in front of the sprite are respected. */
extern unsigned char s_HW_OAM[], s_HW_DB_OAM[], s_HW_OBJ_PLTT[], s_HW_DB_OBJ_PLTT[];
extern unsigned char *P3DS_GtObjVram(int eng);
extern int P3DS_WideBoxExtra(void);
enum { kRec = 1024, kRecH = 1024, kOtMax = 2048, kObjT0 = 8192, kObjT1 = 4096 };
typedef struct { u8 full[32], ers[32]; unsigned hF, seen; u16 next; u8 used, x0, x1; } GtRec;   /* x0..x1: columns with text */
static GtRec gRec[kRec]; static u16 gRecHead[kRecH]; static unsigned gRecN, gObjFrame;
typedef struct { int16_t x, y; u16 rec; u8 eng, scr, pal, prio, semi, hf, vf; } GtOt;   /* a text tile a sprite shows, x,y = its DS place */
static GtOt gOt[kOtMax]; static unsigned gOtN, gOtSig[2];
static u16 gEr[kObjT0 + kObjT1], gErList[kOtMax]; static unsigned gErN;   /* tiles changed for this present: record + 1 */
static unsigned gObjErased, gObjRecs, gObjBack;
static const u8 kDim[3][4][2] = { {{8,8},{16,16},{32,32},{64,64}}, {{16,8},{32,8},{32,16},{64,32}}, {{8,16},{8,32},{16,32},{32,64}} };
static unsigned tileHash(const u8 *p) { unsigned h = 2166136261u; for (int i = 0; i < 32; i += 4) h = (h ^ *(const u32 *)(p + i)) * 16777619u; return h; }
static GtRec *recFind(const u8 *p, unsigned h) {
    for (unsigned i = gRecHead[h & (kRecH - 1)]; i; i = gRec[i - 1].next) { GtRec *r = &gRec[i - 1]; if (r->hF == h && !memcmp(r->full, p, 32)) return r; }
    return 0;
}
static void recAdd(const u8 *full, const u8 *ers) {
    const unsigned h = tileHash(full); GtRec *r = recFind(full, h);
    if (r) { r->seen = gObjFrame; return; }
    int k = -1;
    if (gRecN < kRec) { for (int i = 0; i < kRec; i++) if (!gRec[i].used) { k = i; break; } }
    if (k < 0) { /* full: the record matched longest ago (never one in use this present) */
        for (int i = 0; i < kRec; i++) if (gRec[i].used && gRec[i].seen != gObjFrame && (k < 0 || gRec[i].seen < gRec[k].seen)) k = i;
        if (k < 0) return;
        u16 *pp = &gRecHead[gRec[k].hF & (kRecH - 1)]; while (*pp && *pp != k + 1) pp = &gRec[*pp - 1].next; if (*pp) *pp = gRec[k].next;
        gRecN--; }
    r = &gRec[k]; memcpy(r->full, full, 32); memcpy(r->ers, ers, 32); r->hF = h; r->seen = gObjFrame; r->used = 1; r->x0 = 7; r->x1 = 0;
    for (int py = 0; py < 8; py++) for (int px = 0; px < 8; px++) if (nibAt(full, 1, px, py) != nibAt(ers, 1, px, py)) { if (px < r->x0) r->x0 = (u8)px; if (px > r->x1) r->x1 = (u8)px; }
    r->next = gRecHead[h & (kRecH - 1)]; gRecHead[h & (kRecH - 1)] = (u16)(k + 1); gRecN++; gObjRecs++;
}
/* a window on no BG: keep each of its tiles that has text, as printed and with the text taken out */
static void snapLoose(GtWin *w) {
    w->dirty = 0;
    if (!w->und || !w->nInk || w->gave || !w->buf || w->bpp8) return;
    const int W = w->wT, wpx = W * 8; const u8 *b = (const u8 *)w->buf; u8 full[32] __attribute__((aligned(4))), ers[32] __attribute__((aligned(4)));
    for (int ty = 0; ty < w->hT; ty++) for (int tx = 0; tx < W; tx++) {
        memcpy(full, b + ((ty * W + tx) << 5), 32); memcpy(ers, full, 32); int n = 0;
        for (int py = 0; py < 8; py++) for (int px = 0; px < 8; px++) { const int X = tx * 8 + px, Y = ty * 8 + py, i = Y * wpx + X; const unsigned u = w->und[i];
            if (u == kNoInk || nibAt(b, W, X, Y) != w->val[i] || w->val[i] == u) continue;
            nibSet(ers, 1, px, py, u); n++; }
        if (n) recAdd(full, ers); }
}
/* unk_02013534.c sub_02013A50 (patch 0289): this window's tiles are being copied for sprites (TextOBJ, naming screen letters,
   Pokegear map markings): keep its text tiles now, whatever BG the window is on */
void P3DS_GtToSprite(const void *buf) {
    if (!buf || !cfgOn()) return;
    lk();
    for (int i = 0; i < kWins; i++) if (gWin[i].used && gWin[i].buf == buf) { snapLoose(&gWin[i]); break; }
    ulk();
}
/* start of a present: find the text tiles visible sprites show, take their text out for this present, list them for the layer */
static void objStep(int on) {
    unsigned sig[2] = { 2166136261u, 2166136261u };
    gOtN = 0; gObjFrame++;
    for (int k = 0; k < kWins; k++) if (gWin[k].used && gWin[k].loose && gWin[k].dirty) snapLoose(&gWin[k]);
    if (on && gRecN) for (int e = 0; e < 2; e++) {
        const int scr = scrOf(e); if (!screenOk(scr)) continue;
        u8 *V = P3DS_GtObjVram(e); if (!V) continue;
        const unsigned dc = reg32(e ? 0x1000 : 0);
        if (!(dc & 0x1000) || ((dc >> 16) & 3) != 1) continue;            /* sprites off, or not the graphics picture */
        const unsigned nT = e ? kObjT1 : kObjT0, oneD = (dc & 0x10) != 0, bnd = oneD ? (dc >> 20) & 3 : 0;
        const u16 *oam = (const u16 *)(e ? s_HW_DB_OAM : s_HW_OAM);
        for (int i = 0; i < 128; i++) {
            const unsigned a0 = oam[i * 4], a1 = oam[i * 4 + 1], a2 = oam[i * 4 + 2];
            if (a0 & 0x300) continue;                                      /* affine or hidden */
            const unsigned md = (a0 >> 10) & 3, shp = a0 >> 14, sz = a1 >> 14; if (md >= 2 || (a0 & 0x2000) || shp == 3) continue;   /* OBJ window / bitmap / 8 bit */
            const int w = kDim[shp][sz][0], h = kDim[shp][sz][1], tw = w / 8, th = h / 8;
            int x = (int)(a1 & 0x1FF); if (x >= 256) x -= 512; int y = (int)(a0 & 0xFF); if (y >= 192) y -= 256;
            if (x + w <= -64 || x >= 320 || y + h <= 0) continue;
            const int hf = (a1 >> 12) & 1, vf = (a1 >> 13) & 1; const unsigned tile = a2 & 0x3FF;
            for (int ty = 0; ty < th; ty++) for (int tx = 0; tx < tw; tx++) {
                const unsigned tno = (oneD ? (tile << bnd) + (unsigned)(ty * tw + tx) : tile + (unsigned)(ty * 32 + tx)) & (nT - 1), gi = (e ? kObjT0 : 0) + tno;
                u8 *p = V + tno * 32; unsigned ri = gEr[gi];
                if (!ri) { GtRec *r = recFind(p, tileHash(p)); if (!r || gErN >= kOtMax) continue;
                    memcpy(p, r->ers, 32); ri = (unsigned)(r - gRec) + 1; gEr[gi] = (u16)ri; gErList[gErN++] = (u16)gi; r->seen = gObjFrame; gObjErased++; }
                if (gOtN >= kOtMax) continue;
                GtOt *o = &gOt[gOtN++]; o->x = (int16_t)(x + (hf ? tw - 1 - tx : tx) * 8); o->y = (int16_t)(y + (vf ? th - 1 - ty : ty) * 8); o->rec = (u16)(ri - 1);
                o->eng = (u8)e; o->scr = (u8)scr; o->pal = (u8)(a2 >> 12); o->prio = (u8)((a2 >> 10) & 3); o->semi = md == 1; o->hf = (u8)hf; o->vf = (u8)vf;
                sig[scr] = fnv(fnv(fnv(sig[scr], (unsigned)(o->x * 4096 + o->y)), ri), (unsigned)(o->pal | o->prio << 4 | o->semi << 6 | hf << 7 | vf << 8 | e << 9)); }
        }
    }
    gOtSig[0] = sig[0]; gOtSig[1] = sig[1];
}
/* end of a present (render3ds.cpp): put the printed tiles back before the game runs again */
void P3DS_GtObjRestore(void) {
    lk();
    for (unsigned k = 0; k < gErN; k++) { const unsigned gi = gErList[k], e = gi >= kObjT0, tno = gi - (e ? kObjT0 : 0); u8 *V = P3DS_GtObjVram((int)e);
        if (gEr[gi] && V) { const GtRec *r = &gRec[gEr[gi] - 1]; u8 *p = V + tno * 32; if (!memcmp(p, r->ers, 32)) { memcpy(p, r->full, 32); gObjBack++; } }
        gEr[gi] = 0; }
    gErN = 0;
    ulk();
}

/* ---- render side (render3ds.cpp / gpuscale.c) ---- */
/* start of each frame's compose: which screens could show the layer last present. Text whose screen cannot show it (or the
   setting is OFF) goes back into VRAM now; text that can be shown again is taken out again. */
void P3DS_GtSync(unsigned okBits) {
    const int on = cfgOn();
    if (on) { gReady[0] = P3DS_GtLayerReady(0); gReady[1] = P3DS_GtLayerReady(1); }
    gOkBits = okBits;
    lk();
    for (int k = 0; k < kWins; k++) { GtWin *w = &gWin[k];
        if (!w->used || !w->vValid || !w->vram || !w->buf || w->vSize != w->wT * w->hT * tileBytes(w)) continue;
        const int drawable = on && !w->gave && w->und && w->nInk && screenOk(scrOf(engOf(w))) && bgDrawable(w);
        const int want = drawable && gtWinMapShows(w);
        if (want == w->shown) continue;
        if (hashMem(w->vram, w->vSize) != w->vHash) { w->vValid = 0; w->shown = 0; gStale++; gGen++; continue; }   /* not ours any more */
        const int wpx = w->wT * 8, hpx = w->hT * 8, W = w->wT; const u8 *b = (const u8 *)w->buf; unsigned n = 0;
        memcpy(gScratchR, b, w->vSize);
        if (want) for (int Y = 0; Y < hpx; Y++) for (int X = 0; X < wpx; X++) { const int i = Y * wpx + X; if (w->und[i] == kNoInk) continue;
            if (pixAt(b, W, X, Y, w->bpp8) != w->val[i]) { w->und[i] = kNoInk; continue; }
            if (w->val[i] != w->und[i]) { pixSet(gScratchR, W, X, Y, w->und[i], w->bpp8); n++; } }
        memcpy(w->vram, gScratchR, w->vSize);
        w->vHash = hashMem(w->vram, w->vSize); w->shown = want && n; gGen++;
        if (want) gSyncOn++; else gSyncOff++;
        static unsigned lg; if (lg < 40) { lg++; PSPNativeMemLog("[GTEXT] window bg%d %dx%d: text %s (%s)", w->bg, w->wT, w->hT, w->shown ? "taken out again, native layer draws it" : "put back into the DS picture",
            !on ? "setting OFF" : !screenOk(scrOf(engOf(w))) ? "its screen cannot show the layer now" : !drawable ? "its BG cannot show native text" : !want ? "its tiles are not in the BG map (text stays in the DS picture)" : "screen can show the layer"); }
    }
    objStep(on);
    ulk();
}

static u32 gfx555(u16 c, int mode, int ev, unsigned a) {
    int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    if (mode == 1) { r += (31 - r) * ev / 16; g += (31 - g) * ev / 16; b += (31 - b) * ev / 16; }
    else if (mode == 2) { r -= r * ev / 16; g -= g * ev / 16; b -= b * ev / 16; }
    r = (r << 3) | (r >> 2); g = (g << 3) | (g >> 2); b = (b << 3) | (b >> 2);
    return ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (a & 0xFF);
}
/* one BG of an engine, as the per pixel checks read it */
/* text = the native layer can follow this BG: a text BG, or an affine BG (8 bit map) / extended tile BG (16 bit map) whose matrix is
   1:1 (then its reference point acts as the scroll). aff: 0 text, 1 affine, 2 extended tiles */
struct GtBg { int on, text, prio, bpp8, aff; unsigned cnt, hofs, vofs, W, H, cbase, sbase; };
static void readBg(int eng, int b, GtBg *o) {
    const unsigned R = eng ? 0x1000 : 0, dc = reg32(R), mode = dc & 7, cnt = reg16(R + 8 + 2 * b), size = (cnt >> 14) & 3;
    memset(o, 0, sizeof *o);
    o->on = (dc >> (8 + b)) & 1; o->cnt = cnt; o->prio = (int)(cnt & 3); o->bpp8 = (cnt & 0x80) != 0;
    const int isText = b < 2 ? !(b == 0 && eng == 0 && (dc & 8)) : b == 2 ? (mode <= 1 || mode == 3) : mode == 0;
    o->text = isText;
    o->hofs = reg16(R + 0x10 + 4 * b) & 0x1FF; o->vofs = reg16(R + 0x12 + 4 * b) & 0x1FF;
    o->W = (size & 1) ? 512 : 256; o->H = (size & 2) ? 512 : 256;
    if (!isText && b >= 2 && mode <= 5) {
        const int affine = b == 2 ? (mode == 2 || mode == 4) : (mode == 1 || mode == 2), ext = b == 2 ? mode == 5 : mode >= 3;
        if (affine || (ext && !(cnt & 0x80))) {
            const unsigned A = R + 0x20 + (unsigned)(b - 2) * 0x10;
            const int pa = (int16_t)reg16(A), pb = (int16_t)reg16(A + 2), pc = (int16_t)reg16(A + 4), pd = (int16_t)reg16(A + 6);
            const int rx = (int)(reg32(A + 8) << 4) >> 4, ry = (int)(reg32(A + 12) << 4) >> 4;
            o->aff = affine ? 1 : 2; o->bpp8 = 1; o->W = o->H = 128u << size;
            o->text = pa == 0x100 && pb == 0 && pc == 0 && pd == 0x100 && !(rx & 0xFF) && !(ry & 0xFF);
            o->hofs = (unsigned)(rx >> 8); o->vofs = (unsigned)(ry >> 8); } }
    o->cbase = (eng ? 0 : ((dc >> 24) & 7) * 0x10000) + ((cnt >> 2) & 15) * 0x4000;
    o->sbase = (eng ? 0 : ((dc >> 27) & 7) * 0x10000) + ((cnt >> 8) & 31) * 0x800;
}
static int bgDrawable(const GtWin *w) { GtBg g; readBg(engOf(w), w->bg & 3, &g);
    return g.text && g.bpp8 == (int)w->bpp8 && !(g.bpp8 && g.aff != 1 && (reg32(engOf(w) ? 0x1000 : 0) & 0x40000000u)); }   /* 8 bit with extended palettes: not followed */
static inline unsigned mapEntry(const u8 *V, unsigned vmask, const GtBg *g, unsigned px, unsigned py) {
    const unsigned tx = px >> 3, ty = py >> 3, sz = (g->cnt >> 14) & 3;
    if (g->aff == 1) return V[(g->sbase + ty * (g->W >> 3) + tx) & vmask];
    if (g->aff == 2) { const unsigned a = (g->sbase + (ty * (g->W >> 3) + tx) * 2) & vmask; return V[a] | (V[a + 1] << 8); }
    const unsigned blk = sz == 1 ? (tx >> 5) : sz == 2 ? (ty >> 5) : sz == 3 ? ((tx >> 5) + 2 * (ty >> 5)) : 0u;
    const unsigned a = (g->sbase + blk * 0x800 + ((ty & 31) * 32 + (tx & 31)) * 2) & vmask;
    return V[a] | (V[a + 1] << 8);
}
/* True when the live BG tile map still has enough of this window's sequential tiles (unflipped)
   for the native layer to place them. Sharp text may only take glyphs out of VRAM when this holds;
   otherwise the native layer would hide them as "map" and the screen goes blank (box app: PR on
   cursor/mobile-pc-text). Start taking glyphs out only on a majority (same rule). Once they are
   out, stay until fewer than a quarter of the tiles still match, so a tab or screen change that
   briefly sits near the 50% line cannot flip sharpness on and off every frame. */
static int gtWinMapShows(const GtWin *w) {
    const int e = engOf(w), b = w->bg & 3;
    const u8 *V = P3DS_GtVram(e); const unsigned vmask = e ? 0x1FFFFu : 0x7FFFFu; if (!V || w->wT < 1 || w->hT < 1) return 0;
    GtBg g; readBg(e, b, &g); if (!g.on || !g.text) return 0;
    const unsigned total = (unsigned)w->wT * w->hT;
    const unsigned divisor = w->shown ? 4 : 2;
    const unsigned needed = (total + divisor - 1) / divisor;
    unsigned matches = 0, misses = 0;
    for (int r = 0; r < w->hT; r++) {
        const unsigned y = (unsigned)(w->top + r) * 8 & (g.H - 1);
        for (int c = 0; c < w->wT; c++) {
            const unsigned x = (unsigned)(w->left + c) * 8 & (g.W - 1);
            const unsigned ent = mapEntry(V, vmask, &g, x, y);
            const unsigned tile = (w->base + r * w->wT + c) & 0x3FFu;
            if ((ent & 0xFFFu) == tile) {
                if (++matches == needed) return 1;
            } else if (++misses > total - needed) {
                return 0;
            }
        }
    }
    return 0;
}
/* is this BG's pixel at screen sx,sy opaque (colour index != 0)? */
static int bgOpaque(const u8 *V, unsigned vmask, const GtBg *g, int sx, int sy) {
    const unsigned px = (unsigned)(sx + (int)g->hofs) & (g->W - 1), py = (unsigned)(sy + (int)g->vofs) & (g->H - 1);
    const unsigned e = mapEntry(V, vmask, g, px, py), t = e & 0x3FF;
    unsigned fx = px & 7, fy = py & 7; if (e & 0x400) fx = 7 - fx; if (e & 0x800) fy = 7 - fy;
    if (g->bpp8) return V[(g->cbase + t * 64 + fy * 8 + fx) & vmask] != 0;
    const u8 v = V[(g->cbase + t * 32 + fy * 4 + (fx >> 1)) & vmask]; return ((fx & 1) ? v >> 4 : v & 15) != 0;
}
/* DS window regions: may BG b show at screen sx,sy? */
static int winShows(unsigned R, unsigned dc, int b, int sx, int sy) {
    if (!(dc & 0xE000)) return 1;
    for (int k = 0; k < 2; k++) { if (!(dc & (0x2000u << k))) continue;
        const unsigned h = reg16(R + 0x40 + 2 * k), v = reg16(R + 0x44 + 2 * k); const int x1 = (int)(h >> 8), x2 = (int)(h & 255), y1 = (int)(v >> 8), y2 = (int)(v & 255);
        const int inX = x1 <= x2 ? (sx >= x1 && sx < x2) : (sx >= x1 || sx < x2), inY = y1 <= y2 ? (sy >= y1 && sy < y2) : (sy >= y1 || sy < y2);
        if (inX && inY) return (reg16(R + 0x48) >> (8 * k + b)) & 1; }
    return (reg16(R + 0x4A) >> b) & 1;
}
static int cmpGlyph(const void *a, const void *b) { const GtGlyph *p = &gGl[*(const u16 *)a], *q = &gGl[*(const u16 *)b];
    if (p->y != q->y) return p->y - q->y; return p->x - q->x; }

static unsigned gLastSig[2]; static int gHave[2]; static unsigned gRenders;
/* scr 0 = top (buf 400x240 used), 1 = bottom (320x240); fx..fh: where the DS picture's 256x192 lands. Returns 1 when the layer
   should be drawn (dirty = the buffer changed). */
int P3DS_GtRender(int scr, u32 *buf, float fxf, float fyf, float fwf, float fhf, int *dirty) {
    *dirty = 0;
    if (!((P3DS_GtScreensNow >> scr) & 1) || !cfgOn() || !buf) { gHave[scr] = 0; return 0; }
    const int SW = scr ? 320 : 400;
    const int fx = (int)(fxf + 0.5f), fy = (int)(fyf + 0.5f), fw = (int)(fwf + 0.5f), fh = (int)(fhf + 0.5f);
    const WcPlan *plan = scr == 0 ? P3DS_GtWidePlan : 0; const int planEng = P3DS_GtWideEng;
    static u16 order[kGlyphs]; static u8 vis[32 * 32];   /* per window tile: its map palette, 0xFF = not shown */ static u8 claim[256 * 256 / 8];
    lk();
    /* windows on this screen whose VRAM still holds our copy */
    int list[kWins], nl = 0;
    unsigned sig = fnv(2166136261u, gGen); sig = fnv(sig, (unsigned)(fx * 4096 + fy * 64 + fw)); sig = fnv(sig, (unsigned)fh);
    sig = fnv(sig, (unsigned)(uintptr_t)plan); if (plan) sig = fnv(sig, plan->hash);
    for (int k = 0; k < kWins; k++) { GtWin *w = &gWin[k];
        if (!w->used || !w->shown || !w->vValid || w->gave || !w->und || scrOf(engOf(w)) != scr) continue;
        if (hashMem(w->vram, w->vSize) != w->vHash) { if (++w->miss >= 3) { w->vValid = 0; w->shown = 0; gStale++; gGen++; } continue; }
        w->miss = 0; list[nl++] = k; }
    unsigned nOt = 0; for (unsigned k = 0; k < gOtN; k++) nOt += gOt[k].scr == scr;
    if (!nl && !nOt) { ulk(); gHave[scr] = 0; return 0; }
    if (nOt) { sig = fnv(sig, gOtSig[scr]); for (int e = 0; e < 2; e++) { const u8 *P = e ? s_HW_DB_OBJ_PLTT : s_HW_OBJ_PLTT; for (int i = 0; i < 512; i += 4) sig = fnv(sig, *(const u32 *)(P + i)); } }
    for (int e = 0; e < 2; e++) { const unsigned R = e ? 0x1000 : 0;
        for (unsigned o = 0; o < 0x20; o += 4) sig = fnv(sig, reg32(R + o));
        for (unsigned o = 0x40; o < 0x58; o += 4) sig = fnv(sig, reg32(R + o));
        sig = fnv(sig, reg16(R + 0x6C));
        const u8 *P = e ? s_HW_DB_BG_PLTT : s_HW_BG_PLTT; for (int i = 0; i < 512; i += 4) sig = fnv(sig, *(const u32 *)(P + i)); }
    sig = fnv(sig, reg16(0x304));
    for (int li = 0; li < nl; li++) { const GtWin *w = &gWin[list[li]]; const int e = engOf(w), b = w->bg & 3;
        const u8 *V = P3DS_GtVram(e); const unsigned vmask = e ? 0x1FFFF : 0x7FFFF; if (!V) continue; GtBg g; readBg(e, b, &g);
        sig = fnv(sig, (unsigned)list[li]);
        for (int r = 0; r < w->hT; r++) for (int c = 0; c < w->wT; c++) sig = fnv(sig, mapEntry(V, vmask, &g, (unsigned)(w->left + c) * 8 & (g.W - 1), (unsigned)(w->top + r) * 8 & (g.H - 1)));
        /* what the other BGs show over the window's place (their map entries, sampled per tile) */
        for (int ob = 0; ob < 4; ob++) { if (ob == b) continue; GtBg o; readBg(e, ob, &o); if (!o.on || !o.text) continue;
            for (int r = 0; r < w->hT; r++) for (int c = 0; c < w->wT; c++) { const int sx = (int)(((w->left + c) * 8 - (int)g.hofs) & (int)(g.W - 1)), sy = (int)(((w->top + r) * 8 - (int)g.vofs) & (int)(g.H - 1));
                sig = fnv(sig, mapEntry(V, vmask, &o, (unsigned)(sx + (int)o.hofs) & (o.W - 1), (unsigned)(sy + (int)o.vofs) & (o.H - 1))); } } }
    if (gHave[scr] && sig == gLastSig[scr]) { ulk(); return 1; }
    gLastSig[scr] = sig; gHave[scr] = 1;
    for (int y = 0; y < 240; y++) memset(buf + y * 512, 0, (size_t)SW * 4);
    unsigned drawnPx = 0, nGl = 0, hMap = 0, hWin = 0, hCov = 0, hOff = 0;
    for (int li = 0; li < nl; li++) { const int wi = list[li]; GtWin *w = &gWin[wi]; const int e = engOf(w), b = w->bg & 3; const unsigned R = e ? 0x1000 : 0;
        const u8 *V = P3DS_GtVram(e); const unsigned vmask = e ? 0x1FFFF : 0x7FFFF; if (!V) continue;
        const unsigned dc = reg32(R); GtBg g; readBg(e, b, &g);
        if (!g.on || !g.text || g.bpp8 != (int)w->bpp8 || (g.bpp8 && g.aff != 1 && (dc & 0x40000000u))) continue;
        if (((dc >> 16) & 3) != 1) continue;                     /* display mode: not the graphics picture */
        /* the BGs drawn over this one */
        GtBg cov[3]; int nc = 0;
        for (int ob = 0; ob < 4; ob++) { if (ob == b) continue; GtBg o; readBg(e, ob, &o); if (!o.on || !o.text) continue;
            if (o.prio < g.prio || (o.prio == g.prio && ob < b)) cov[nc++] = o; }
        /* colours: brightness fade (BLDCNT 2/3 or MASTER_BRIGHT), alpha (BLDCNT 1) */
        const unsigned bld = reg16(R + 0x50), bldy = reg16(R + 0x54) & 31, mb = reg16(R + 0x6C), eva = reg16(R + 0x52) & 31; int bmode = 0, ev = 0; unsigned alpha = 255;
        if (((bld >> 6) & 3) >= 2 && (bld & (1u << b))) { bmode = (int)((bld >> 6) & 3) - 1; ev = bldy > 16 ? 16 : (int)bldy; }
        else if (((bld >> 6) & 3) == 1 && (bld & (1u << b)) && (bld & 0x3F00)) alpha = eva >= 16 ? 255u : eva * 255u / 16u;
        if (!bmode && (mb >> 14) >= 1 && (mb >> 14) <= 2) { bmode = (int)(mb >> 14); ev = (int)(mb & 31); if (ev > 16) ev = 16; }
        /* tiles of the window the tile map still shows, with the palette the map gives them (menus highlight a row by its map palette) */
        const u8 *P = e ? s_HW_DB_BG_PLTT : s_HW_BG_PLTT; static u32 col[16][16]; u16 palMade = 0;
        for (int r = 0; r < w->hT; r++) for (int c = 0; c < w->wT; c++) {
            const unsigned ent = mapEntry(V, vmask, &g, (unsigned)(w->left + c) * 8 & (g.W - 1), (unsigned)(w->top + r) * 8 & (g.H - 1));
            const int mine = (ent & 0x3FFu) == (((unsigned)w->base + (unsigned)(r * w->wT + c)) & 0x3FFu) && !(ent & 0xC00u);   /* flipped tiles are not ours */
            const unsigned pl = ent >> 12; vis[r * w->wT + c] = mine ? (u8)pl : 0xFF;
            if (mine && !(palMade & (1u << pl))) { palMade |= (u16)(1u << pl); for (int n = 1; n < 16; n++) col[pl][n] = gfx555(*(const u16 *)(P + (pl * 16 + n) * 2), bmode, ev, alpha); } }
        /* glyphs by line, then left to right; a run = same line, small gaps. The run's start is placed by the picture's scale, its
           letters follow at the font's own pixel advance (1 font pixel = 1 screen pixel) */
        int ng = 0; for (int i = 0; i < kGlyphs; i++) if (gGl[i].used && gGl[i].win == wi) order[ng++] = (u16)i;
        if (!ng) continue;
        qsort(order, (size_t)ng, sizeof order[0], cmpGlyph);
        extern const char *P3DS_WideScreenName; const int namingScr = P3DS_WideScreenName && !strcmp(P3DS_WideScreenName, "naming");
        const int wpx = w->wT * 8, hpx = w->hT * 8; memset(claim, 0, (size_t)(wpx * hpx + 7) / 8);
        const GtGlyph *prev = 0; int ox = 0, oy = 0, rx = 0, ry = 0, runOk = 0;
        for (int oi = 0; oi < ng; oi++) { const GtGlyph *g2 = &gGl[order[oi]]; nGl++;
            /* naming screen (2026-11-20, "tapping M enters L"): the keyboard prints every letter as its own string, centred in a 16 px key,
               and the name field puts each letter in a 12 px cell. As one run the letters were laid out at the font's own advance (16 / 12 px
               instead of the picture's 20 / 15), so a row came out narrower than its keys and the outer letters sat a key away from where the
               game takes the tap. Here every letter that is not touching the one before it is placed by the picture's scale on its own. */
            const int nameGap = namingScr && prev && g2->y == prev->y && (g2->x + g2->x0) - (prev->x + prev->x1) - 1 >= 3;
            if (nameGap) { static unsigned nl; if (nl < 3) { nl++; PSPNativeMemLog("[GTEXT] naming screen: letters placed one by one (keyboard keys / name field)"); } }
            if (!prev || g2->y != prev->y || g2->x - prev->x > 24 || nameGap) {
                /* the run's first text pixel: where the picture shows it */
                const int bx = w->left * 8 + g2->x, by = w->top * 8 + g2->y;
                int sx = (int)(((unsigned)(bx - (int)g.hofs)) & (g.W - 1)), sy = (int)(((unsigned)(by - (int)g.vofs)) & (g.H - 1));
                if (sx >= 256 + 64) sx -= (int)g.W; if (sy >= 192 + 64) sy -= (int)g.H;
                runOk = 1; rx = g2->x; ry = g2->y;
                if (plan && e == planEng) { const int X = WC_MapColumn(plan, b, sy + g2->y0 < 0 ? 0 : sy + g2->y0 > 191 ? 191 : sy + g2->y0, (bx + g2->x0) & (int)(g.W - 1));
                    if (X < 0) runOk = 0; else { ox = X * 5 / 4 - g2->x0; oy = (sy * 240 + (sy >= 0 ? 96 : -96)) / 192 + 2; } }
                else { ox = fx + (sx * fw + (sx >= 0 ? 128 : -128)) / 256; oy = fy + (sy * fh + (sy >= 0 ? 96 : -96)) / 192 + 2; } }
            prev = g2;
            if (!runOk) continue;
            for (int py = g2->y0; py <= g2->y1; py++) { const int Y = g2->y + py; if (Y < 0 || Y >= hpx) continue;
                for (int px = g2->x0; px <= g2->x1; px++) { const int X = g2->x + px; if (X < 0 || X >= wpx) continue;
                    const int i = Y * wpx + X; const unsigned u = w->und[i]; if (u == kNoInk) continue;
                    const unsigned n = w->val[i]; if (!n || n == u) continue;           /* transparent, or no visible change */
                    if (claim[i >> 3] & (1u << (i & 7))) continue;
                    const unsigned tp = vis[(Y >> 3) * w->wT + (X >> 3)]; if (tp == 0xFF) { hMap++; continue; }
                    int sx = (int)(((unsigned)(w->left * 8 + X - (int)g.hofs)) & (g.W - 1)), sy = (int)(((unsigned)(w->top * 8 + Y - (int)g.vofs)) & (g.H - 1));
                    if (sx >= 256 + 64) sx -= (int)g.W; if (sy >= 192 + 64) sy -= (int)g.H;
                    if (sx >= 0 && sx < 256 && sy >= 0 && sy < 192) {
                        if (!winShows(R, dc, b, sx, sy)) { hWin++; continue; }
                        int covered = 0; for (int k = 0; k < nc && !covered; k++) covered = bgOpaque(V, vmask, &cov[k], sx, sy);
                        if (covered) { hCov++; continue; } }
                    else if (!(plan && e == planEng)) { hOff++; continue; }       /* off the DS picture */
                    const int nx = ox + (X - rx), ny = oy + (Y - ry);
                    if (nx < 0 || nx >= SW || ny < 0 || ny >= 240) continue;
                    claim[i >> 3] |= (u8)(1u << (i & 7));
                    buf[ny * 512 + nx] = w->bpp8 ? gfx555(*(const u16 *)(P + n * 2), bmode, ev, alpha) : col[tp][n]; drawnPx++; } } } }
    /* text in sprites: blocks of touching text tiles, each centred where the DS picture shows it, letters 1:1 */
    unsigned objPx = 0;
    if (nOt) {
        static u16 idx[kOtMax], par[kOtMax]; static int16_t gx0[kOtMax], gx1[kOtMax], gy0[kOtMax], gy1[kOtMax];
        int m = 0; for (unsigned k = 0; k < gOtN; k++) if (gOt[k].scr == scr) idx[m++] = (u16)k;
        #define GT_ROOT(v) ({ int r_ = (v); while (par[r_] != r_) { par[r_] = par[par[r_]]; r_ = par[r_]; } r_; })
        for (int a = 0; a < m; a++) { par[a] = (u16)a; gx0[a] = gy0[a] = 32767; gx1[a] = gy1[a] = -32768; }
        for (int a = 0; a < m; a++) for (int b = 0; b < a; b++) { const GtOt *p = &gOt[idx[a]], *q = &gOt[idx[b]];
            if (p->eng != q->eng || abs(p->x - q->x) > 8 || abs(p->y - q->y) > 8) continue;
            const int ra = GT_ROOT(a), rb = GT_ROOT(b); if (ra != rb) par[ra] = (u16)rb; }
        for (int a = 0; a < m; a++) { const GtOt *o = &gOt[idx[a]]; const GtRec *rc = &gRec[o->rec]; const int r = GT_ROOT(a);
            const int i0 = o->x + (o->hf ? 7 - rc->x1 : rc->x0), i1 = o->x + (o->hf ? 7 - rc->x0 : rc->x1);
            if (i0 < gx0[r]) gx0[r] = (int16_t)i0; if (i1 > gx1[r]) gx1[r] = (int16_t)i1; if (o->y < gy0[r]) gy0[r] = o->y; if (o->y + 7 > gy1[r]) gy1[r] = (int16_t)(o->y + 7); }
        GtBg bgs[2][4]; for (int e = 0; e < 2; e++) for (int b = 0; b < 4; b++) readBg(e, b, &bgs[e][b]);
        const int wideStrips = scr == 0 && P3DS_WideBoxExtra() > 0;
        for (int a = 0; a < m; a++) { const GtOt *o = &gOt[idx[a]]; const GtRec *rc = &gRec[o->rec]; const int r = GT_ROOT(a), e = o->eng;
            const unsigned R = e ? 0x1000 : 0, dc = reg32(R); const u8 *V = P3DS_GtVram(e); const unsigned vmask = e ? 0x1FFFF : 0x7FFFF;
            const float ncx = fxf + (float)(gx0[r] + gx1[r] + 1) * 0.5f * fwf / 256.f, ncy = fyf + (float)(gy0[r] + gy1[r] + 1) * 0.5f * fhf / 192.f;
            const int bx = (int)floorf(ncx - (float)(gx1[r] - gx0[r] + 1) * 0.5f + 0.5f), by = (int)floorf(ncy - (float)(gy1[r] - gy0[r] + 1) * 0.5f + 0.5f);
            const unsigned bld = reg16(R + 0x50), bldy = reg16(R + 0x54) & 31, mb = reg16(R + 0x6C), eva = reg16(R + 0x52) & 31; int bmode = 0, ev = 0; unsigned alpha = 255;
            if (!o->semi && ((bld >> 6) & 3) >= 2 && (bld & 0x10)) { bmode = (int)((bld >> 6) & 3) - 1; ev = bldy > 16 ? 16 : (int)bldy; }
            else if (o->semi || (((bld >> 6) & 3) == 1 && (bld & 0x10) && (bld & 0x3F00))) alpha = eva >= 16 ? 255u : eva * 255u / 16u;
            if (!bmode && (mb >> 14) >= 1 && (mb >> 14) <= 2) { bmode = (int)(mb >> 14); ev = (int)(mb & 31); if (ev > 16) ev = 16; }
            const u16 *P = (const u16 *)(e ? s_HW_DB_OBJ_PLTT : s_HW_OBJ_PLTT);
            for (int py = 0; py < 8; py++) for (int px = 0; px < 8; px++) {
                const unsigned n = nibAt(rc->full, 1, px, py); if (!n || n == nibAt(rc->ers, 1, px, py)) continue;
                const int X = o->x + (o->hf ? 7 - px : px), Y = o->y + (o->vf ? 7 - py : py);
                if (X >= 0 && X < 256 && Y >= 0 && Y < 192) {
                    if (!winShows(R, dc, 4, X, Y)) { hWin++; continue; }
                    int covered = 0; if (V) for (int b = 0; b < 4 && !covered; b++) { const GtBg *g = &bgs[e][b]; if (g->on && g->text && g->prio < o->prio) covered = bgOpaque(V, vmask, g, X, Y); }
                    if (covered) { hCov++; continue; } }
                else if (!wideStrips) { hOff++; continue; }
                const int nx = bx + X - gx0[r], ny = by + Y - gy0[r];
                if (nx < 0 || nx >= SW || ny < 0 || ny >= 240) continue;
                buf[ny * 512 + nx] = gfx555(P[o->pal * 16 + n], bmode, ev, alpha); objPx++; } }
        #undef GT_ROOT
    }
    ulk();
    *dirty = 1;
    { static unsigned n, lastKey[2]; const unsigned key = (unsigned)nl * 131071u + nGl * 8191u + drawnPx + nOt * 524287u + objPx * 31u; if ((key != lastKey[scr] && n < 300) || (n % 600) == 0) PSPNativeMemLog("[GTEXT] native layer (%s): %d windows, %u glyphs, %u pixels, sprite text %u tiles %u pixels (hidden: map %u, DS window %u, covered %u, off picture %u) | recorded %u, erased uploads %u, plain %u, pool full %u, no memory %u, stale %u, back on %u / off %u, sprite tile records %u (made %u), taken out %u, put back %u",
        scr ? "bottom" : "top", nl, nGl, drawnPx, nOt, objPx, hMap, hWin, hCov, hOff, gRecorded, gErased, gPlain, gFull, gNoMem, gStale, gSyncOn, gSyncOff, gRecN, gObjRecs, gObjErased, gObjBack); lastKey[scr] = key; n++; }
    (void)gChanged; gRenders++;
    return 1;
}

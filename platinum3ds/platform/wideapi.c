/* platinum3ds run 83: the game-side API for wide-screen layouts, as GabeSud0/pokeplatinum-3ds's patches
 * (patches/pokeplatinum/0012-0030, GPL-3.0) call it; the declarations are from their
 * 3ds_port/nitro/include/nitro_port_wide.h. This file only RECORDS what the game says (windows, parts, splits) so
 * the renderer can read it; it draws nothing. wide_layouts=0 (default): NitroPort_WideScreenExtra() is 0, so every
 * patched code path in the game behaves exactly like the DS's (the tables are still stored, nothing reads them). */
#include <stdint.h>
#include <string.h>
extern int P3DS_CfgInt(const char *, int);
extern void PSPNativeMemLog(const char *, ...);

#define MAX_WINDOWS 24
#define MAX_PARTS 128
#define MAX_SPLITS 16
typedef struct { const void *w; int bg, left, top, width, height, anchor, anchorTiles, frame; } Win;
typedef struct { uint8_t kind, bgs; int16_t top, bottom, x0, x1, dx, col; } Part;
static Win wins[MAX_WINDOWS]; static int nWins; static uint32_t winSerial;
static Part parts[2][MAX_PARTS]; static int nParts[2];
static short splits[2][MAX_SPLITS * 7]; static int nSplits[2];

static int on = -1;
void P3DS_SetWideLayouts(int v) { on = v != 0; PSPNativeMemLog("[WIDE] wide_layouts=%d (settings)", on); }
int P3DS_WideLayoutsOn(void) { if (on < 0) { on = P3DS_CfgInt("wide_layouts", 0) != 0; PSPNativeMemLog("[WIDE] run83 wide-layout API linked, wide_layouts=%d (%s %s)", on, __DATE__, __TIME__); } return on; }
extern int P3DS_GpuBattleUp;  /* render3ds.cpp run 110: slider up + stereo_gpu_battle: GPU-composed 4:3 frames, so no wide HUD offsets */
int NitroPort_WideScreenExtra(void) { return P3DS_WideLayoutsOn() && !P3DS_GpuBattleUp ? 32 : 0; }
int NitroPort_WidePictures(void) { return 0; }
int NitroPort_SunShadows(void) { return 0; }

static Win *find(const void *w) { for (int i = 0; i < nWins; i++) if (wins[i].w == w) return &wins[i]; return 0; }
void NitroPort_WindowAdded(const void *w, int bg, int left, int top, int width, int height) {
    Win *p = find(w); if (!p) { if (nWins >= MAX_WINDOWS) return; p = &wins[nWins++]; memset(p, 0, sizeof *p); }
    p->w = w; p->bg = bg; p->left = left; p->top = top; p->width = width; p->height = height; winSerial++;
}
void NitroPort_WindowRemoved(const void *w) {
    for (int i = 0; i < nWins; i++) if (wins[i].w == w) { wins[i] = wins[--nWins]; winSerial++; return; }
}
void NitroPort_WindowAnchor(const void *w, int side, int tiles) { Win *p = find(w); if (p) { p->anchor = side; p->anchorTiles = tiles; winSerial++; } }
void NitroPort_WindowFramed(const void *w, int frame) { Win *p = find(w); if (p) { p->frame = frame; winSerial++; } }
uint32_t NitroPort_WideWindowsSerial(void) { return winSerial; }

void NitroPort_WideScreenParts(int engine, const short *rows, int count) {
    { static int nl; if (nl < 40) { nl++; PSPNativeMemLog("[WIDE] parts set: engine %d, %d rows", engine, rows ? count : 0); } } /* run 95 diag */
    if (engine < 0 || engine > 1) return;
    if (!rows || count <= 0) { nParts[engine] = 0; return; }
    if (count > MAX_PARTS) count = MAX_PARTS;
    for (int i = 0; i < count; i++) { const short *r = rows + i * 8; Part *p = &parts[engine][i];
        p->kind = (uint8_t)r[0]; p->bgs = (uint8_t)r[1]; p->top = r[2]; p->bottom = r[3]; p->x0 = r[4]; p->x1 = r[5]; p->dx = r[6]; p->col = r[7]; }
    nParts[engine] = count;
}
void NitroPort_WideSplitScreen(int engine, const short *rows, int count) {
    if (engine < 0 || engine > 1) return;
    if (!rows || count <= 0) { nSplits[engine] = 0; return; }
    if (count > MAX_SPLITS) count = MAX_SPLITS;
    memcpy(splits[engine], rows, count * 7 * sizeof(short)); nSplits[engine] = count;
}
/* for the renderer: how many parts an engine has now, and a copy of them (rows of 8 shorts as the game wrote them) */
int P3DS_WideGetParts(int engine, short *out8) {
    if (engine < 0 || engine > 1) return 0;
    for (int i = 0; i < nParts[engine]; i++) { const Part *p = &parts[engine][i]; short *o = out8 + i * 8;
        o[0] = p->kind; o[1] = p->bgs; o[2] = p->top; o[3] = p->bottom; o[4] = p->x0; o[5] = p->x1; o[6] = p->dx; o[7] = p->col; }
    return nParts[engine];
}

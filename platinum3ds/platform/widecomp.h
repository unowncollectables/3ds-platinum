/* platinum3ds run 86: wide_layouts display engine (widecomp.c) */
#ifndef P3DS_WIDECOMP_H
#define P3DS_WIDECOMP_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WC_MAX_PARTS 128
#define WC_ROWS 384 /* distinct column maps (lines with the same parts share one) */
typedef struct { int on, text, prio, bpp8, w, h, extSlot, h0, mosaic; unsigned charb, mapb, hofs, vofs; } WcLayer;
typedef struct {
    int valid, changed, n, objParts, dx3d, nRows; uint32_t hash;
    short parts[WC_MAX_PARTS * 8]; WcLayer L[4];
    uint8_t rowOf[4][192]; short rows[WC_ROWS][320]; /* per layer and line: its map of DS columns -32..287 to picture columns (-1 nothing); 255 = out of rows (built per line) */
} WcPlan;
/* what the compositor reads (engine A): flat BG / OBJ VRAM, the two palettes (256 entries each), extended palettes,
   the OAM (128 x 4 u16), and the 3D layer (256x192, 6 bits per channel, alpha in the top byte) or NULL */
typedef struct { const uint8_t *bgvram, *objvram, *bgext, *objext; const uint16_t *bgpal, *objpal, *oam; const uint32_t *frame3d; } WcSrc;
/* engine A registers (DS layout), the parts as wideapi.c stores them. Returns 1 when the plan is usable; sets
   changed=1 when it was (re)made this call. */
int WC_Plan(WcPlan *p, const uint8_t *regs, const short *parts, int n);
/* 0 when WC_Compose can draw this screen, else why (1 forced blank, 2 not graphics mode, 3 windows, 4 alpha blending,
   5 a non-text BG, 6 BG mosaic, 7 3D layer without a frame) */
int WC_Check(const WcPlan *p, const uint8_t *regs, int have3d);
/* draws engine A across -32..287: centre 256x192 (melonDS 0xAABBGGRR) rewritten, strip 64x192 (32 left, 32 right) as
   0xRRGGBBAA like P3DS_WideFillBuf. Returns 0 (nothing written) when the screen uses something it does not do. */
int WC_Compose(const WcPlan *p, const uint8_t *regs, const WcSrc *src, uint32_t *center, uint32_t *strip);
/* run 97: same, with its own cache per eye (eye 1 = right eye, src->frame3d = the right-eye 3D layer) */
/* run 98: 0 off, 1 redraw only changed lines, 2 = 1 plus a full redraw to compare (counts mismatches, shows the full one) */
void WC_SetDirty(int mode);
void WC_SetDirtyRegFilter(int on); /* run 100 */
void WC_Set3dAlpha(int on); /* run 103 */
void WC_SetWindows(int on); /* run 109 */
void WC_DirtyStats(unsigned *frames, unsigned *rows, unsigned *full, unsigned *bad);
int WC_ComposeEye(const WcPlan *p, const uint8_t *regs, const WcSrc *src, uint32_t *center, uint32_t *strip, int eye);
/* run 105 wide_split (left eye, wide_dirty=1 only): the dirty lines are shared by two threads. Prepare returns the number of
   dirty lines (-1 = not usable: use WC_Compose), Run(part 0 / 1) draws that part's lines, Commit after both Runs. */
typedef struct { uint8_t mask[2][192]; int n[2], eye; uint32_t rh[192]; volatile int next; int dn[2]; } WcSplit;
int WC_SplitPrepare(const WcPlan *p, const uint8_t *regs, const WcSrc *src, WcSplit *sp, int minRows, int eye); /* eye 0 = left, 1 = right (run 106: the right eye in stereo) */
void WC_SplitRun(const WcPlan *p, const uint8_t *regs, const WcSrc *src, const WcSplit *sp, int part);
void WC_SplitRunDyn(const WcPlan *p, const uint8_t *regs, const WcSrc *src, WcSplit *sp, int part); /* run 108: claim 16-line groups */
void WC_SplitCommit(const WcSplit *sp, uint32_t *center, uint32_t *strip);
#ifdef __cplusplus
}
#endif
#endif

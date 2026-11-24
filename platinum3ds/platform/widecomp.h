/* platinum3ds run 86: wide_layouts display engine (widecomp.c) */
#ifndef P3DS_WIDECOMP_H
#define P3DS_WIDECOMP_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WC_MAX_PARTS 128
#define WC_ROWS 255 /* distinct column maps (lines with the same parts share one); 255 is the uint8_t "no row" sentinel, so the 256th map falls back to a per-line build */
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
/* ss3ds wide_layouts: the whole wide picture, cached by its inputs. 0 = not drawable (WC_Check), 1 = drawn, 2 = reused */
int WC_Compose(const WcPlan *p, const uint8_t *regs, const WcSrc *src, uint32_t *center, uint32_t *strip);
/* ss3ds: the strip-sprite entry below is kept from the Platinum port (full compose, caches and dirty modes dropped: BSS) */
/* ss3ds wide_boxes: sprites reaching into the side strips only (see widecomp.c) */
int WC_ComposeObjStrips(const WcPlan *p, const uint8_t *regs, const WcSrc *src, uint32_t *strip);
/* hash of what that compose reads, limited to the sprites that reach into the strips (run 62) */
unsigned WC_StripHash(const uint8_t *regs, const WcSrc *src);
/* ss3ds sharp game text: wide picture column (0..319) showing map column bgcol of layer L on line y, or -1 */
int WC_MapColumn(const WcPlan *p, int L, int y, int bgcol);
#ifdef __cplusplus
}
#endif
#endif

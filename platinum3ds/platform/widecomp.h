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
int WC_ComposeEye(const WcPlan *p, const uint8_t *regs, const WcSrc *src, uint32_t *center, uint32_t *strip, int eye);
#ifdef __cplusplus
}
#endif
#endif

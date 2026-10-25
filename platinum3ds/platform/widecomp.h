/* platinum3ds run 84: wide_layouts display engine (widecomp.c) */
#ifndef P3DS_WIDECOMP_H
#define P3DS_WIDECOMP_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WC_MAX_PARTS 128
#define WC_ROWS 48 /* distinct column maps (lines with the same parts share one) */
typedef struct { int on, text, prio, bpp8, w, h, extSlot, h0; unsigned charb, mapb, hofs, vofs; } WcLayer;
typedef struct {
    int valid, changed, n, D, objParts, mMask, cpuMask, frontMask; uint32_t hash;
    short parts[WC_MAX_PARTS * 8]; WcLayer L[4];
    uint8_t rowOf[4][192]; short rows[WC_ROWS][320]; int nRows; /* per layer and line: its map of DS columns -32..287 to picture columns (-1 nothing); 255 = none (computed per pixel) */
} WcPlan;
/* engine A registers (DS layout), the parts as wideapi.c stores them. Returns 1 when the plan is usable; sets
   changed=1 when it was (re)made this call. cpuMask = BGs to switch off for the melonDS pass. */
int WC_Plan(WcPlan *p, const uint8_t *regs, const short *parts, int n);
/* center: the engine A picture (256x192, melonDS 0xAABBGGRR) drawn with cpuMask's BGs off and no master brightness;
   rewritten in place. strip: 64x192 (32 left, 32 right), 0xRRGGBBAA, as P3DS_WideFillBuf. */
void WC_Compose(const WcPlan *p, const uint8_t *regs, const uint8_t *vram, const uint16_t *pal, const uint8_t *ext, uint32_t *center, uint32_t *strip);
#ifdef __cplusplus
}
#endif
#endif

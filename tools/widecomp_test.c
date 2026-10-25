/* PC test for platinum3ds/platform/widecomp.c (wide_layouts display engine) with the party menu's layout.
   gcc -O2 -I platinum3ds/platform tools/widecomp_test.c platinum3ds/platform/widecomp.c -o /tmp/wct && /tmp/wct */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "widecomp.h"
static uint8_t R[0x70], vram[0x80000], ext[0x8000]; static uint16_t pal[256]; static uint32_t center[256 * 192], strip[64 * 192];
static int fails;
static void check(int ok, const char *what, unsigned got, unsigned want) { if (!ok) { fails++; printf("FAIL %s: got %08x want %08x\n", what, got, want); } }
static uint32_t toM(uint16_t c) { uint32_t r = (c & 31) << 3, g = ((c >> 5) & 31) << 3, b = ((c >> 10) & 31) << 3; r |= r >> 6; g |= g >> 6; b |= b >> 6; return r | g << 8 | b << 16 | 0xFF000000u; }
static uint32_t mToStrip(uint32_t c) { return (c & 255) << 24 | ((c >> 8) & 255) << 16 | ((c >> 16) & 255) << 8 | 0xFF; }
#define WIDE_BG(n) (1 << (n))
#define WIDE_MOVE(bgs, top, bottom, x0, x1, dx)  { 1, (bgs), (top), (bottom), (x0), (x1), (dx), 0 }
#define WIDE_FILL(bgs, top, bottom, x0, x1, col) { 2, (bgs), (top), (bottom), (x0), (x1), 0, (col) }
#define WIDE_OBJS(top, bottom, x0, x1, dx)        { 3, 0, (top), (bottom), (x0), (x1), (dx), 0 }
static const short party[][8] = {
    WIDE_FILL(WIDE_BG(3), 0, 192, -32, 32, 0),
    WIDE_MOVE(WIDE_BG(3), 0, 192, 0, 256, 32),
    WIDE_MOVE(WIDE_BG(1) | WIDE_BG(2), 0, 192, 0, 256, 32),
    WIDE_OBJS(0, 192, -128, 384, 32),
};
int main(void) {
    *(uint32_t *)R = 0x1F00; /* mode 0, BG0-3 + OBJ */
    *(uint16_t *)(R + 8) = 0x1F00 | 0;          /* BG0: prio 0, char 0, map 31 */
    *(uint16_t *)(R + 10) = 0x1D00 | 1;         /* BG1: prio 1, map 29 */
    *(uint16_t *)(R + 12) = 0x1C00 | 2;         /* BG2: prio 2, map 28 */
    *(uint16_t *)(R + 14) = 0x1E00 | 4 | 3;     /* BG3: prio 3, char 1 (0x4000), map 30 */
    pal[0] = 0x0000; pal[1] = 0x001F; pal[2] = 0x03E0; pal[3] = 0x7C00;
    memset(vram + 0x4000 + 32, 0x11, 32); memset(vram + 0x4000 + 64, 0x22, 32); /* BG3 tiles 1 (red), 2 (green) */
    memset(vram + 32, 0x33, 32);                                                 /* BG0 tile 1 (blue) */
    for (int ty = 0; ty < 32; ty++) for (int tx = 0; tx < 32; tx++) *(uint16_t *)(vram + 30 * 0x800 + (ty * 32 + tx) * 2) = tx ? 2 : 1;
    *(uint16_t *)(vram + 31 * 0x800 + (5 * 32 + 5) * 2) = 1; /* BG0: one tile at DS x 40..47, y 40..47 */
    for (int i = 0; i < 256 * 192; i++) center[i] = 0xFF000000u | (unsigned)i; /* the melonDS pass: a unique value per pixel */
    static WcPlan p;
    WC_Plan(&p, R, &party[0][0], 4);
    check(p.D == 32, "D", p.D, 32); check(p.mMask == 0xE, "mMask", p.mMask, 0xE); check(p.cpuMask == 1, "cpuMask", p.cpuMask, 1); check(p.frontMask == 1, "frontMask", p.frontMask, 1);
    WC_Compose(&p, R, vram, pal, ext, center, strip);
    for (int y = 0; y < 192; y += 7) {
        for (int x = 0; x < 32; x++) check(strip[y * 64 + x] == mToStrip(toM(0x001F)), "left strip = BG3 column 0", strip[y * 64 + x], mToStrip(toM(0x001F)));
        for (int x = 0; x < 32; x++) check(strip[y * 64 + 32 + x] == mToStrip(0xFF000000u | (unsigned)(y * 256 + 224 + x)), "right strip = melonDS 224..255", strip[y * 64 + 32 + x], 0);
        for (int s = 0; s < 32; s++) { uint32_t want = (y >= 40 && y < 48 && s >= 40 && s < 48) ? toM(0x7C00) : toM(0x001F); check(center[y * 256 + s] == want, "centre 0..31 = fill", center[y * 256 + s], want); }
        for (int s = 32; s < 256; s++) { uint32_t want = (y >= 40 && y < 48 && s >= 40 && s < 48) ? toM(0x7C00) : (0xFF000000u | (unsigned)(y * 256 + s - 32)); check(center[y * 256 + s] == want, "centre 32..255 = melonDS moved 32 (+ BG0)", center[y * 256 + s], want); }
    }
    /* a layout with no sprite part: D = 0, BGs without parts go to the melonDS pass, moved ones are drawn here */
    static const short moveOnly[][8] = { WIDE_MOVE(WIDE_BG(1), 0, 192, 0, 256, 16) };
    static WcPlan p2; WC_Plan(&p2, R, &moveOnly[0][0], 1);
    check(p2.D == 0, "D2", p2.D, 0); check(p2.mMask == 0xD, "mMask2", p2.mMask, 0xD); check(p2.cpuMask == 2, "cpuMask2", p2.cpuMask, 2);
    /* run 85 party layout: columns spread 16 each way, background in place, sprites per column */
    static const short party2[][8] = { WIDE_OBJS(0, 0, 0, 0, 0), WIDE_FILL(WIDE_BG(3), 0, 192, -32, 0, 0), WIDE_FILL(WIDE_BG(3), 0, 192, 256, 288, 255),
        WIDE_MOVE(WIDE_BG(1) | WIDE_BG(2), 0, 192, 0, 123, -16), WIDE_MOVE(WIDE_BG(1) | WIDE_BG(2), 0, 192, 123, 256, 16),
        WIDE_OBJS(0, 192, -128, 123, -16), WIDE_OBJS(0, 192, 123, 384, 16) };
    static WcPlan p3; WC_Plan(&p3, R, &party2[0][0], 7);
    check(p3.D == 0, "D3", p3.D, 0); check(p3.mMask == 0x9, "mMask3", p3.mMask, 9); check(p3.cpuMask == 6, "cpuMask3", p3.cpuMask, 6);
    uint16_t oam[512]; memset(oam, 0, sizeof oam); for (int i = 0; i < 128; i++) oam[i * 4] = 0x200; /* all disabled */
    oam[0] = 40; oam[1] = 10;  /* 8x8 sprite at x 10, y 40: left column */
    oam[4] = 40; oam[5] = 200; /* x 200: right column */
    WC_AdjustOam(&p3, oam);
    check((oam[1] & 0x1FF) == 0x1FA, "oam left sprite", oam[1] & 0x1FF, 0x1FA); check((oam[5] & 0x1FF) == 216, "oam right sprite", oam[5] & 0x1FF, 216);
    /* BG2 (cpu, behind BG1? no: here prio 2) over BG3 (cpu in another layout) is not hidden by it: PC-box style, two cpu layers, no melonDS BG in front */
    printf(fails ? "%d checks FAILED\n" : "all widecomp checks passed\n", fails);
    return fails != 0;
}

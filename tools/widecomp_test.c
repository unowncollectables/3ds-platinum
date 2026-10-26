/* PC test for platinum3ds/platform/widecomp.c (wide_layouts display engine) with the party menu's layout.
   gcc -O2 -I platinum3ds/platform tools/widecomp_test.c platinum3ds/platform/widecomp.c -o /tmp/wct && /tmp/wct */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "widecomp.h"
void PSPNativeMemLog(const char *f, ...) { (void)f; } /* widecomp.c verify-mode log */
static uint8_t R[0x70], bgv[0x80000], objv[0x40000], bgext[0x8000], objext[0x2000]; static uint16_t bgpal[256], objpal[256], oam[512]; static uint32_t center[256 * 192], strip[64 * 192];
static int fails;
static void check(int ok, const char *what, unsigned got, unsigned want) { if (!ok) { fails++; printf("FAIL %s: got %08x want %08x\n", what, got, want); } }
static uint32_t toM(uint16_t c) { uint32_t r = (c & 31) << 3, g = ((c >> 5) & 31) << 3, b = ((c >> 10) & 31) << 3; r |= r >> 6; g |= g >> 6; b |= b >> 6; return r | g << 8 | b << 16 | 0xFF000000u; }
static uint32_t toS(uint32_t c) { return (c & 255) << 24 | ((c >> 8) & 255) << 16 | ((c >> 16) & 255) << 8 | 0xFF; }
#define WIDE_BG(n) (1 << (n))
#define WIDE_MOVE(bgs, top, bottom, x0, x1, dx)  { 1, (bgs), (top), (bottom), (x0), (x1), (dx), 0 }
#define WIDE_FILL(bgs, top, bottom, x0, x1, col) { 2, (bgs), (top), (bottom), (x0), (x1), 0, (col) }
#define WIDE_OBJS(top, bottom, x0, x1, dx)        { 3, 0, (top), (bottom), (x0), (x1), (dx), 0 }
static const short party[][8] = {
    WIDE_OBJS(0, 0, 0, 0, 0),
    WIDE_FILL(WIDE_BG(3), 0, 192, -32, 0, 0),
    WIDE_FILL(WIDE_BG(3), 0, 192, 256, 288, 255),
    WIDE_MOVE(WIDE_BG(1) | WIDE_BG(2), 0, 192, 0, 123, -16),
    WIDE_MOVE(WIDE_BG(1) | WIDE_BG(2), 0, 192, 123, 256, 16),
    WIDE_OBJS(0, 192, -128, 123, -16),
    WIDE_OBJS(0, 192, 123, 384, 16),
};
static uint32_t at(int y, int s) { return s < 0 ? strip[y * 64 + s + 32] : s >= 256 ? strip[y * 64 + 32 + s - 256] : center[y * 256 + s]; }
static uint32_t pix(int y, int s) { return s < 0 || s >= 256 ? at(y, s) : toS(center[y * 256 + s]); } /* compare in one format */
int main(void) {
    *(uint32_t *)R = 0x11F00; /* graphics mode, mode 0, BG0-3 + OBJ */
    *(uint16_t *)(R + 8) = 0x1F00 | 0; *(uint16_t *)(R + 10) = 0x1D00 | 1; *(uint16_t *)(R + 12) = 0x1C00 | 2; *(uint16_t *)(R + 14) = 0x1E00 | 4 | 3;
    bgpal[0] = 0x0000; bgpal[1] = 0x001F; bgpal[2] = 0x03E0; bgpal[3] = 0x7C00; bgpal[4] = 0x7FFF; objpal[1] = 0x03FF;
    memset(bgv + 0x4000 + 32, 0x11, 32); memset(bgv + 0x4000 + 64, 0x22, 32); /* BG3 tiles 1 (red), 2 (green) */
    memset(bgv + 32, 0x33, 32);                                                /* BG1/BG2 tile 1 (blue), as tile data below (char base 0) */
    memset(bgv + 64, 0x44, 32);                                                /* tile 2 (white) */
    for (int ty = 0; ty < 32; ty++) for (int tx = 0; tx < 32; tx++) *(uint16_t *)(bgv + 30 * 0x800 + (ty * 32 + tx) * 2) = tx == 0 ? 1 : tx == 31 ? 2 : 1; /* BG3 map */
    *(uint16_t *)(bgv + 29 * 0x800 + (5 * 32 + 5) * 2) = 1;  /* BG1: blue tile at DS x 40..47, y 40..47 (left column) */
    *(uint16_t *)(bgv + 29 * 0x800 + (5 * 32 + 20) * 2) = 2; /* BG1: white tile at DS x 160..167 (right column) */
    memset(oam, 0, sizeof oam); for (int i = 0; i < 128; i++) oam[i * 4] = 0x200;
    memset(objv, 0x11, 32);                                  /* sprite tile 0: all colour 1 */
    oam[0] = 40; oam[1] = 10; oam[2] = 0 | (0 << 10);        /* 8x8 at x 10, y 40, prio 0: left column */
    oam[4] = 100; oam[5] = 200; oam[6] = 0 | (2 << 10);      /* 8x8 at x 200, y 100, prio 2 (behind BG1 prio 1) */
    *(uint16_t *)(bgv + 29 * 0x800 + (12 * 32 + 25) * 2) = 2; /* BG1 white tile at DS x 200..207, y 96..103 */
    static WcPlan p; WC_Plan(&p, R, &party[0][0], 7);
    WcSrc S = { bgv, objv, bgext, objext, bgpal, objpal, oam, 0 };
    int ok = WC_Compose(&p, R, &S, center, strip); check(ok == 1, "compose ok", ok, 1);
    const uint32_t red = toS(toM(0x001F)), green = toS(toM(0x03E0)), blue = toS(toM(0x7C00)), white = toS(toM(0x7FFF)), cyan = toS(toM(0x03FF));
    for (int y = 0; y < 192; y += 13) {
        check(at(y, -32) == red && at(y, -1) == red, "left fill = BG3 column 0", at(y, -1), red);
        check(at(y, 256) == green && at(y, 287) == green, "right fill = BG3 column 255", at(y, 256), green);
    }
    check(pix(44, 24) == blue && pix(44, 31) == blue, "BG1 left tile moved -16 (x 40 -> 24)", pix(44, 24), blue);
    check(pix(44, 40) != blue, "BG1 left tile gone from its old place", pix(44, 40), 0);
    check(pix(44, 176) == white && pix(44, 183) == white, "BG1 right tile moved +16 (x 160 -> 176)", pix(44, 176), white);
    check(pix(43, -6) == cyan && pix(44, -6) == cyan && pix(44, 1) == cyan && pix(48, -6) != cyan && pix(44, 2) != cyan, "sprite (left column) moved -16", at(44, -6), cyan);
    check(pix(100, 216) == white && pix(103, 223) == white, "BG1 over a lower-priority sprite", pix(100, 216), white);
    /* windows: not done -> 0, picture untouched */
    WC_SetWindows(0); *(uint32_t *)R = 0x11F00 | 0x2000; ok = WC_Compose(&p, R, &S, center, strip); check(ok == 0, "window (wide_windows=0) -> unsupported", ok, 0);
    /* run 109 wide_windows=1: WIN0 = DS x 0..127, all lines, inside OBJ only, outside everything */
    WC_SetWindows(1); *(uint16_t *)(R + 0x40) = (0 << 8) | 128; *(uint16_t *)(R + 0x44) = (0 << 8) | 192; *(uint16_t *)(R + 0x48) = 0x10; *(uint16_t *)(R + 0x4A) = 0x3F;
    ok = WC_Compose(&p, R, &S, center, strip); check(ok == 1, "window -> drawn", ok, 1);
    check(pix(44, 24) != blue, "WIN0: BG1 hidden inside", pix(44, 24), 0);
    check(pix(44, 176) == white, "WIN0: BG1 shown outside", pix(44, 176), white);
    check(pix(44, 1) == cyan, "WIN0: sprite shown inside (OBJ on)", pix(44, 1), cyan);
    check(at(13, -1) == red, "WIN0: strip uses WINOUT", at(13, -1), red);
    *(uint16_t *)(R + 0x40) = 0; *(uint16_t *)(R + 0x44) = 0; *(uint16_t *)(R + 0x48) = 0; *(uint16_t *)(R + 0x4A) = 0;
    *(uint32_t *)R = 0x11F08; *(uint16_t *)(R + 0x50) = 0x0041 | 0x0100; /* alpha blend with BG0 as the 3D layer: not done here */ ok = WC_Compose(&p, R, &S, center, strip); check(ok == 0, "alpha on 3D -> unsupported", ok, 0);
    *(uint32_t *)R = 0x11F00; *(uint16_t *)(R + 0x50) = 0x0042 | 0x0100; *(uint16_t *)(R + 0x52) = 0x0808; ok = WC_Compose(&p, R, &S, center, strip); check(ok == 1, "alpha BG1 over BG0 -> drawn", ok, 1);
    printf(fails ? "%d checks FAILED\n" : "all widecomp checks passed\n", fails);
    return fails != 0;
}

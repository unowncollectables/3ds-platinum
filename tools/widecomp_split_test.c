/* PC test for wide_split (run 105): WC_SplitPrepare/Run/Commit must give the same picture as a full redraw, over frames
   where sprites move and the 3D layer changes (the dirty-line cache path). Parts run one after the other here; they touch
   disjoint lines, so threads give the same result.
   gcc -O2 -I platinum3ds/platform tools/widecomp_split_test.c platinum3ds/platform/widecomp.c -o /tmp/wcs && /tmp/wcs */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "widecomp.h"
void PSPNativeMemLog(const char *f, ...) { (void)f; } /* widecomp.c verify-mode log */
static uint8_t R[0x70], bgv[0x80000], objv[0x40000], bgext[0x8000], objext[0x2000];
static uint16_t bgpal[256], objpal[256], oam[512];
static uint32_t f3d[256 * 192], cA[256 * 192], sA[64 * 192], cB[256 * 192], sB[64 * 192];
int main(void) {
    srand(7);
    *(uint32_t *)R = 0x11F08; /* graphics, BG0 = 3D, BG1-3, OBJ */
    *(uint16_t *)(R + 8) = 0x1F00; *(uint16_t *)(R + 10) = 0x1D00 | 1; *(uint16_t *)(R + 12) = 0x1C00 | 2; *(uint16_t *)(R + 14) = 0x1E00 | 3;
    for (int i = 0; i < 256; i++) { bgpal[i] = (uint16_t)(rand() & 0x7FFF); objpal[i] = (uint16_t)(rand() & 0x7FFF); }
    for (int i = 0; i < 0x8000; i++) bgv[i] = (uint8_t)rand();
    for (int i = 0; i < 0x8000; i++) objv[i] = (uint8_t)rand();
    for (int i = 0; i < 128; i++) oam[i * 4] = 0x200;
    WcPlan p; memset(&p, 0, sizeof p); WcSrc S = { bgv, objv, bgext, objext, bgpal, objpal, oam, f3d };
    WC_SetDirty(1); int bad = 0;
    for (int f = 0; f < 200; f++) {
        for (int i = 0; i < 6; i++) { oam[i * 4] = (uint16_t)((20 + i * 25 + f) & 255); oam[i * 4 + 1] = (uint16_t)((10 + i * 40 + f * (i + 1)) & 0x1FF); oam[i * 4 + 2] = (uint16_t)(i * 8); }
        for (int y = 40 + (f % 50); y < 90 + (f % 50); y++) for (int x = 30; x < 200; x++) f3d[y * 256 + x] = 0x1F000000u | (uint32_t)rand();
        if (f % 37 == 0) bgpal[1] = (uint16_t)rand();
        *(uint16_t *)(R + 0x10) = (uint16_t)(f & 7);
        if (!WC_Plan(&p, R, NULL, 0)) { puts("plan failed"); return 1; }
        WcSplit sp; int n = WC_SplitPrepare(&p, R, &S, &sp, 24);
        if (n < 0) { puts("prepare declined"); return 1; }
        WC_SplitRun(&p, R, &S, &sp, 1); WC_SplitRun(&p, R, &S, &sp, 0); WC_SplitCommit(&sp, cA, sA);
        if (sp.n[0] + sp.n[1] != n) { printf("FAIL frame %d: parts %d+%d != %d\n", f, sp.n[0], sp.n[1], n); bad++; }
        WC_SetDirty(0); int ok = WC_ComposeEye(&p, R, &S, cB, sB, 1); WC_SetDirty(1);
        if (!ok || memcmp(cA, cB, sizeof cA) || memcmp(sA, sB, sizeof sA)) { printf("FAIL frame %d: picture differs\n", f); bad++; }
    }
    printf(bad ? "%d FAILURES\n" : "all ok\n", bad); return bad != 0;
}

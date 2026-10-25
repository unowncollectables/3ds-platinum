/* checks buildRow() == srcCol() for random layouts: gcc -O2 -I platinum3ds/platform tools/widecomp_rows_test.c -o /tmp/wrt && /tmp/wrt */
#include "../platinum3ds/platform/widecomp.c"
#include <stdio.h>
#include <stdlib.h>
int main(void) { srand(1); long bad = 0, n = 0; static WcPlan p;
    for (int it = 0; it < 3000; it++) { memset(&p, 0, sizeof p); p.n = 1 + rand() % 20;
        for (int k = 0; k < p.n; k++) { short *t = p.parts + k * 8; t[0] = 1 + rand() % 2; t[1] = (short)((1 << (rand() % 4)) | (rand() % 4 == 0 ? 0x10 : 0));
            t[2] = (short)(rand() % 192); t[3] = (short)(t[2] + rand() % 192); t[4] = (short)(rand() % 300 - 40); t[5] = (short)(t[4] + rand() % 300); t[6] = (short)(rand() % 80 - 40); t[7] = (short)(rand() % 256); }
        for (int i = 0; i < 4; i++) { WcLayer *l = &p.L[i]; l->w = rand() % 2 ? 64 : 32; l->h = 32; l->h0 = rand() % 100 - 40; l->vofs = rand() % 256; }
        int li = rand() % 4, y = rand() % 192; short r[320]; buildRow(&p, li, y, r);
        for (int s = -32; s < 288; s++) { n++; if (r[s + 32] != srcCol(&p, li, y, s)) { if (bad++ < 5) printf("mismatch it %d li %d y %d s %d: row %d src %d\n", it, li, y, s, r[s + 32], srcCol(&p, li, y, s)); } } }
    printf(bad ? "%ld of %ld differ\n" : "buildRow == srcCol (%ld mismatches of %ld)\n", bad, n); return bad != 0; }

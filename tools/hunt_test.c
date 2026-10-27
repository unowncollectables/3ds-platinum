/* x86 test for platinum3ds/platform/hunt.c (run 88 shiny_suite).
 * gcc -O2 -I platinum3ds/platform -DHUNT_DIR='"/tmp/hunt_test/"' tools/hunt_test.c -lm -o ht && ./ht */
#define P3DS_HUNT_HOST
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
static uint64_t hostMs;
uint64_t HuntHostTicks(void) { return hostMs * 1000; } /* CPU_TICKS_PER_MSEC 1000 below */
static int cfgShiny = 1;
static int cfgRate, cfgVisShiny;
int P3DS_CfgInt(const char *k, int d) { return !strcmp(k, "shiny_suite") ? cfgShiny : !strcmp(k, "shiny_rate") ? cfgRate : !strcmp(k, "wild_vis_shiny") ? cfgVisShiny : d; }
void PSPNativeMemLog(const char *f, ...) { va_list a; va_start(a, f); vprintf(f, a); va_end(a); putchar('\n'); }
#include "../platinum3ds/platform/hunt.c"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
static void reload(void) { memset(rec, 0, sizeof rec); target = 0; loaded = 0; }

int main(void) {
    mkdir(HUNT_DIR, 0777); remove(HUNT_DIR "hunt.txt"); remove(HUNT_DIR "hunt.tmp");
    /* an old run 76 file: species=count only */
    FILE *f = fopen(HUNT_DIR "hunt.txt", "w"); fprintf(f, "25=40\n399=7\n"); fclose(f);
    CHECK(P3DS_HuntGet(25)->enc == 40 && P3DS_HuntGet(25)->streak == 40 && P3DS_HuntTarget() == 0);

    P3DS_HuntEncounter(25, 0); P3DS_HuntSoftReset(); P3DS_HuntSoftReset();
    CHECK(P3DS_HuntTarget() == 25 && P3DS_HuntGet(25)->enc == 41 && P3DS_HuntGet(25)->resets == 2 && P3DS_HuntGet(25)->start > 20000000);
    P3DS_HuntEncounter(25, 1);
    CHECK(P3DS_HuntGet(25)->shinies == 1 && P3DS_HuntGet(25)->streak == 0 && P3DS_HuntGet(25)->lastShinyAt == 42);
    P3DS_HuntEncounter(487, 0); P3DS_HuntSoftReset();
    CHECK(P3DS_HuntTarget() == 487 && P3DS_HuntGet(487)->resets == 1 && P3DS_HuntGet(25)->resets == 2);

    /* time: 30 s of 16 ms frames, a 5 s gap (sleep) is not counted, a pause is not counted */
    for (int i = 0; i < 1875; i++) { hostMs += 16; P3DS_HuntFrame(0); }
    hostMs += 5000; P3DS_HuntFrame(0);
    for (int i = 0; i < 100; i++) { hostMs += 16; P3DS_HuntFrame(1); }
    CHECK(P3DS_HuntGet(487)->secs == 29); /* the first frame only starts the clock */
    /* periodic write once 2 minutes have passed since the last write (the encounter at t=0) */
    for (int i = 0; i < 7600; i++) { hostMs += 16; P3DS_HuntFrame(0); }
    reload(); CHECK(P3DS_HuntGet(487)->secs >= 110 && P3DS_HuntGet(487)->secs <= 121);
    P3DS_HuntFlush();

    /* round trip through the file */
    reload();
    CHECK(P3DS_HuntTarget() == 487 && P3DS_HuntGet(25)->enc == 42 && P3DS_HuntGet(25)->shinies == 1 && P3DS_HuntGet(25)->lastShinyAt == 42
          && P3DS_HuntGet(399)->enc == 7 && P3DS_HuntGet(487)->resets == 1);
    /* an old build reading the new file still gets the count */
    { FILE *g = fopen(HUNT_DIR "hunt.txt", "r"); char l[128]; int a, b, got = 0; while (fgets(l, sizeof l, g)) if (sscanf(l, "%d=%d", &a, &b) == 2 && a == 25) got = b; fclose(g); CHECK(got == 42); }
    /* power cut between remove and rename: hunt.tmp is used */
    rename(HUNT_DIR "hunt.txt", HUNT_DIR "hunt.tmp"); reload(); CHECK(P3DS_HuntGet(399)->enc == 7);

    /* cycling through species with records */
    CHECK(P3DS_HuntNext(25, 1) == 399 && P3DS_HuntNext(399, 1) == 487 && P3DS_HuntNext(487, 1) == 25 && P3DS_HuntNext(25, -1) == 487 && P3DS_HuntNext(0, 1) == 25);
    P3DS_HuntClear(399); CHECK(P3DS_HuntGet(399)->enc == 0 && P3DS_HuntNext(25, 1) == 487);

    /* odds */
    CHECK(fabs(P3DS_HuntChanceBy(8192, 8192) - 0.6321) < 0.001 && P3DS_HuntChanceBy(0, 8192) == 0.0 && fabs(P3DS_HuntChanceBy(1, 8192) - 1.0 / 8192) < 1e-9);

    /* off: soft resets are not counted */
    cfgShiny = 0; P3DS_HuntSetOn(0); P3DS_HuntSoftReset(); CHECK(P3DS_HuntGet(487)->resets == 1);

    static const char *const nm[494] = {[25] = "Pikachu", [487] = "Giratina"};
    char path[128]; CHECK(P3DS_HuntExportCsv(nm, 494, path, sizeof path) == 2);
    { FILE *g = fopen(path, "r"); char l[256]; int lines = 0, gir = 0; while (fgets(l, sizeof l, g)) { lines++; if (strstr(l, "487,Giratina,1,1,1,0,")) gir = 1; } fclose(g); CHECK(lines == 3 && gir); }

    /* run 119: odds helpers, shiny_rate extra roll (plus the game's own 1/8192 = the chosen odds), wild_vis force */
    CHECK(P3DS_HuntEncFor(0.5, 8192) == 5678 && P3DS_HuntEncFor(0.9, 8192) == 18862 && P3DS_MasudaOdds() == 1639);
    CHECK(P3DS_RadarPatchOdds(0) == 0 && P3DS_RadarPatchOdds(1) == 8000 && P3DS_RadarPatchOdds(40) == 200 && P3DS_RadarPatchOdds(9999) == 200);
    CHECK(P3DS_HuntRateOdds() == 8192); { int hit = 0; for (int i = 0; i < 100000; i++) hit += P3DS_ShinyRoll(); CHECK(hit == 0); } /* original: never an extra roll */
    for (int ri = 1; ri < 4; ri++) { P3DS_HuntSetRate(ri); const int odds = P3DS_HuntRateOdds(); long long n = 20000000, sh = 0; unsigned g = 12345;
        for (long long i = 0; i < n; i++) { g = g * 1103515245u + 12345u; sh += P3DS_ShinyRoll() || (g >> 8) % 8192 == 0; }
        const double got = (double)n / (double)sh; printf("shiny_rate %d: 1 in %.0f (want %d)\n", ri, got, odds); CHECK(got > odds * 0.97 && got < odds * 1.03); }
    P3DS_HuntSetRate(3); P3DS_ShinyForce = 2; CHECK(P3DS_ShinyRoll() == 0 && P3DS_ShinyForce == 0); P3DS_ShinyForce = 1; CHECK(P3DS_ShinyRoll() == 1 && P3DS_ShinyForce == 0);
    CHECK(P3DS_WildShinyRoll(1) == 0); cfgVisShiny = 1; CHECK(P3DS_WildShinyRoll(1) == 1);
    { int hit = 0; for (int i = 0; i < 1024000; i++) hit += P3DS_WildShinyRoll(0); CHECK(hit > 900 && hit < 1100); }
    P3DS_HuntSetRate(0);
    printf(fails ? "hunt_test: %d FAILED\n" : "hunt_test: all passed\n", fails);
    return fails != 0;
}

/* platinum3ds step 1: boot to debug text. Platform layer + ROM FS + the game's real heap.c/narc.c. */
#include <3ds.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <nitro.h>
#include "platform.h"
#include "heap.h"
#include "narc.h"
#include "sha1.h"

#define DIR_PATH "sdmc:/3ds/platinum3ds"
#define ROM_PATH DIR_PATH "/platinum.nds"
#define LOG_PATH DIR_PATH "/log.txt"
static const char kSha1[] = "0862ec35b24de5c7e2dcb88c9eea0873110d755c";

void P_Log(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    printf("%s\n", buf);
    FILE *f = fopen(LOG_PATH, "a"); if (f) { fprintf(f, "%s\n", buf); fclose(f); } /* open/append/close: survives crashes */
}
static void wait_exit(void) {
    printf("\nPress START to exit.\n");
    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_START) break; gspWaitForVBlank(); }
    gfxExit(); exit(0);
}
void P_Fatal(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    P_Log("[FATAL] %s", buf); wait_exit();
}
void p3ds_assert_fail(const char *file, int line) { P_Fatal("GF_ASSERT failed %s:%d", file, line); }
BOOL CommManager_IsInitialized(void) { return FALSE; }
void ErrorMessageReset_PrintErrorAndReset(void) { P_Fatal("ErrorMessageReset (alloc fail)"); }

/* DS main RAM stand-in: 4 MiB arena, Lo grows up, Hi grows down (OS_ARENA_MAIN). */
#define ARENA_SIZE (4u << 20)
static u8 *arena, *arenaLo, *arenaHi;
void *OS_AllocFromMainArenaLo(u32 size, u32 align) {
    u8 *p = (u8 *)(((uintptr_t)arenaLo + align - 1) & ~(uintptr_t)(align - 1));
    if (p + size > arenaHi) P_Fatal("main arena exhausted (lo %lu)", (unsigned long)size);
    arenaLo = p + size; return p;
}
void *OS_AllocFromMainExArenaHi(u32 size, u32 align) {
    u8 *p = (u8 *)(((uintptr_t)arenaHi - size) & ~(uintptr_t)(align - 1));
    if (p < arenaLo) P_Fatal("main arena exhausted (hi %lu)", (unsigned long)size);
    arenaHi = p; return p;
}

/* Same templates as pokeplatinum src/system.c sHeapInitParams. */
static const HeapParam sHeapInitParams[] = {
    { HEAP_SIZE_SYSTEM, OS_ARENA_MAIN }, { HEAP_SIZE_SAVE, OS_ARENA_MAIN },
    { HEAP_SIZE_DEBUG, OS_ARENA_MAIN }, { HEAP_SIZE_APPLICATION, OS_ARENA_MAIN },
};

static void check_sha1(void) {
    FILE *r = Rom_Handle(); static u8 buf[256 * 1024]; SHA1_CTX c; SHA1Init(&c);
    fseek(r, 0, SEEK_END); long len = ftell(r); fseek(r, 0, SEEK_SET);
    long done = 0; size_t n; int lastPct = -1;
    while ((n = fread(buf, 1, sizeof buf, r)) > 0) {
        SHA1Update(&c, buf, n); done += n; int pct = (int)(done * 100 / len);
        if (pct / 10 != lastPct / 10) { printf("\r  SHA1 %3d%%", pct); lastPct = pct; }
    }
    clearerr(r); u8 d[20]; SHA1Final(d, &c); char hex[41];
    for (int i = 0; i < 20; i++) sprintf(hex + i * 2, "%02x", d[i]);
    printf("\n"); P_Log("SHA1 %s %s", hex, strcmp(hex, kSha1) ? "MISMATCH (not US Rev1 clean)" : "OK (US Rev 1)");
}

int main(void) {
    gfxInitDefault(); consoleInit(GFX_TOP, NULL);
    mkdir("sdmc:/3ds", 0777); mkdir(DIR_PATH, 0777);
    FILE *lf = fopen(LOG_PATH, "w"); if (lf) fclose(lf);
    bool n3ds = false; APT_CheckNew3DS(&n3ds);
    if (n3ds) osSetSpeedupEnable(true);
    P_Log("platinum3ds step1 (%s %s) New3DS=%d", __DATE__, __TIME__, n3ds);

    u8 hdr[512];
    if (!Rom_Open(ROM_PATH, hdr)) P_Fatal("cannot open " ROM_PATH);
    char title[13] = {0}, code[5] = {0}; memcpy(title, hdr, 12); memcpy(code, hdr + 0x0C, 4);
    P_Log("ROM header: '%s' code %s rev %u", title, code, hdr[0x1E]);
    if (strcmp(code, "CPUE") || hdr[0x1E] != 1) P_Fatal("expected Pokemon Platinum US Rev 1 (CPUE rev 1)");
    printf("Hold Y now to verify full SHA1 (~slow)...\n");
    for (int i = 0; i < 60; i++) gspWaitForVBlank();
    hidScanInput(); if (hidKeysHeld() & KEY_Y) check_sha1(); else P_Log("SHA1 check skipped (header OK)");

    arena = memalign(32, ARENA_SIZE); arenaLo = arena; arenaHi = arena + ARENA_SIZE;
    Heap_InitSystem(sHeapInitParams, NELEMS(sHeapInitParams), HEAP_ID_MAX, 0);
    P_Log("Heap_InitSystem OK: system free %lu, app free %lu",
          (unsigned long)HeapExp_FndGetTotalFreeSize(HEAP_ID_SYSTEM), (unsigned long)HeapExp_FndGetTotalFreeSize(HEAP_ID_APPLICATION));
    if (!Heap_Create(HEAP_ID_APPLICATION, HEAP_ID_FIELD1, HEAP_SIZE_FIELD1)) P_Fatal("Heap_Create FIELD1");
    P_Log("Heap_Create FIELD1 (0x%x) OK", HEAP_SIZE_FIELD1);

    NARC *narc = NARC_ctor(NARC_INDEX_POKETOOL__PERSONAL__PL_PERSONAL, HEAP_ID_FIELD1);
    if (!narc) P_Fatal("NARC_ctor");
    P_Log("NARC pl_personal: %u members", NARC_GetFileCount(narc));
    u8 *mon = NARC_AllocAndReadWholeMember(narc, 1, HEAP_ID_FIELD1);
    P_Log("Species #1 base stats HP/Atk/Def/Spe/SpA/SpD = %u/%u/%u/%u/%u/%u", mon[0], mon[1], mon[2], mon[3], mon[4], mon[5]);
    Heap_Free(mon); NARC_dtor(narc);
    u32 sz = NARC_GetMemberSizeByIndexPair(NARC_INDEX_GRAPHIC__PL_FONT, 0);
    void *font = NARC_AllocAndReadWholeMemberByIndexPair(NARC_INDEX_GRAPHIC__PL_FONT, 0, HEAP_ID_APPLICATION);
    P_Log("pl_font member0: %lu bytes, first word %08lx", (unsigned long)sz, (unsigned long)*(u32 *)font);
    Heap_Free(font); Heap_Destroy(HEAP_ID_FIELD1);
    P_Log("app heap free after cleanup: %lu", (unsigned long)HeapExp_FndGetTotalFreeSize(HEAP_ID_APPLICATION));
    P_Log("STEP 1 REACHED: heap + ROM FS + NARC OK");
    wait_exit(); return 0;
}

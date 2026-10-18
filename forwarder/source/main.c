/* Platinum 3DS forwarder: hands sdmc:/3ds/platinum3ds/platinum3ds_full.3dsx to Luma's hb:ldr, then jumps to
 * the hbldr 3DSX title (Luma default 0x000400000D921E00, the Homebrew Launcher CIA), which Luma's loader
 * replaces with our .3dsx. argv[0] is the real sdmc path, so the game finds its files. */
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#define TARGET "sdmc:/3ds/platinum3ds/platinum3ds_full.3dsx"
#define HBLDR_TID 0x000400000D921E00ULL
static Handle hbldr;
static Result hb(u32 hdr, const void *buf, u32 len, int id) {
    u32 *cmd = getThreadCommandBuffer(); cmd[0] = hdr;
    if (buf) { cmd[1] = IPC_Desc_StaticBuffer(len, id); cmd[2] = (u32)buf; }
    Result r = svcSendSyncRequest(hbldr); return R_FAILED(r) ? r : (Result)cmd[1];
}
static void fail(const char *m) { printf("\x1b[31;1m%s\x1b[0m\n\nPress START to exit.\n", m); while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_START) break; gspWaitForVBlank(); } }
static FILE *lg;
#define LOG(...) do { if (lg) { fprintf(lg, __VA_ARGS__); fputc('\n', lg); fflush(lg); } printf(__VA_ARGS__); printf("\n"); } while (0)
int main(void) {
    gfxInitDefault(); consoleInit(GFX_TOP, NULL); printf("Platinum 3DS (dev) forwarder v3\n\n"); lg = fopen("sdmc:/3ds/platinum3ds/forwarder_log.txt", "w"); hidScanInput(); int plain = (hidKeysHeld() & KEY_Y) != 0; LOG("v3 start, Y held (plain HBL test) = %d", plain);
    FILE *f = fopen(TARGET, "rb"); if (!f) { fail("Missing " TARGET); gfxExit(); return 0; } fclose(f);
    if (R_FAILED(svcConnectToPort(&hbldr, "hb:ldr"))) { fail("hb:ldr not available (needs Luma3DS with hbldr enabled)"); gfxExit(); return 0; }
    /* SetTarget (cmd 2): path without "sdmc:" prefix, as hbmenu does */
    static char path[1024]; memset(path, 0, sizeof path); strcpy(path, TARGET + 5); /* hbldr reads up to PATH_MAX */
    Result r = 0; if (!plain) r = hb(IPC_MakeHeader(2, 0, 2), path, strlen(path) + 1, 0);
    /* SetArgv (cmd 3): 3dsx argv block = u32 argc then NUL-separated strings */
    static u8 argbuf[0x400]; memset(argbuf, 0, sizeof argbuf); *(u32 *)argbuf = 1; strcpy((char *)argbuf + 4, TARGET);
    LOG("SetTarget(%s) -> %08lx", path, r); if (!plain && R_SUCCEEDED(r)) r = hb(IPC_MakeHeader(3, 0, 2), argbuf, sizeof argbuf, 1);
    LOG("SetArgv -> %08lx", r); svcCloseHandle(hbldr);
    if (R_FAILED(r)) { char m[64]; snprintf(m, sizeof m, "hb:ldr SetTarget/SetArgv failed %08lx", r); fail(m); gfxExit(); return 0; }
    amInit(); AM_TitleEntry e; u64 tid = HBLDR_TID; Result ri = AM_GetTitleInfo(MEDIATYPE_SD, 1, &tid, &e); amExit(); LOG("HBL title on SD: %08lx", ri);
    if (R_FAILED(ri)) { fail("Homebrew Launcher CIA (0x000400000D921E00) not installed.\nInstall it once with FBI, or launch\nplatinum3ds_full.3dsx from HBL."); gfxExit(); return 0; }
    LOG("chainloading %016llx", (unsigned long long)HBLDR_TID); if (lg) fclose(lg);
    aptSetChainloader(HBLDR_TID, MEDIATYPE_SD);
    gfxExit(); return 0;
}

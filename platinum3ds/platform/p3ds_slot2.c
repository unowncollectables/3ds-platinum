/* Platinum: virtual GBA Slot 2 for "Migrate from <GBA game>" / Pal Park (settings "GBA slot", key slot2, default OFF).
 *
 * Ported from the SoulSilver port (soulsilver3ds/platform/ss3ds_ctrdg.c + ss3ds_slot2.c, hooks 0294-0297, 08b73fd).
 * Platinum's own GBA code (main_menu/ov97_02235D18.c, gba_migrator.c, distribution_cartridge.c) already talks the
 * NitroSDK CTRDG API, so no game hook is needed: these link wraps (full/Makefile) answer that API from
 *   sdmc:/3ds/platinum3ds/slot2/<name>.gba        your own GBA ROM dump (read only)
 *   sdmc:/3ds/platinum3ds/slot2/<name>.sav        its save (never written)
 *   sdmc:/3ds/platinum3ds/slot2-work/<name>.sav   a 128 KiB WORK COPY made from it the first time; the game reads and
 *                                                 writes only this copy (a migrate removes the six Pokemon from it)
 * Which pak is in the slot: settings "GBA pak" (slot2_pak, the n-th .gba in the folder by name). The game looks at the
 * slot on the main menu, so a change applies at the next start / soft reset, like taking a real pak out.
 * With a pak in, Platinum also does what it does with a real one in the DS Lite slot: the "Migrate from" main menu row
 * (after the National Pokedex) and the GBA-slot wild Pokemon on some routes. Never touches platinum.sav. */
#include <3ds.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

extern void PSPNativeMemLog(const char *, ...);
extern int P3DS_CfgInt(const char *, int);
extern void P3DS_CfgWrite(const char *, int);
extern void P3DS_Toast(const char *);

#define SLOT2_DIR  "sdmc:/3ds/platinum3ds/slot2"
#define SLOT2_WORK "sdmc:/3ds/platinum3ds/slot2-work"
#define SLOT2_MAX 16
#define GBA_ROM_BASE   0x08000000u
#define GBA_ROM_END    0x0A000000u
#define GBA_FLASH_BASE 0x0A000000u
#define FLASH_SIZE     0x20000u
#define FLASH_SECTOR   0x1000u
#define FLASH_SHIFT    12
#define FLASH_COUNT    32

/* NitroSDK ctrdg_task.h layout (the game's callbacks read .result) */
typedef struct CTRDGTaskInfo_tag {
    u32 (*task)(struct CTRDGTaskInfo_tag *);
    u32 (*callback)(struct CTRDGTaskInfo_tag *);
    u32 result;
    u8 *data;
    u8 *adr;
    u32 offset;
    u32 size;
    u8 *dst;
    u16 sec_num;
    u8 busy;
    u8 param[1];
} CTRDGTaskInfo;
typedef u32 (*CTRDG_TASK_FUNC)(CTRDGTaskInfo *);
typedef int BOOL;

/* ---------------- settings ---------------- */
static int sOn = -1, sPak = -1, sTried;
int P3DS_Slot2Cfg(void) {
    if (sOn < 0) { sOn = P3DS_CfgInt("slot2", 0) != 0; PSPNativeMemLog("[SLOT2] slot2=%d (virtual GBA slot, default OFF)", sOn); }
    return sOn;
}
static int pakCfg(void) {
    if (sPak < 0) { sPak = P3DS_CfgInt("slot2_pak", 0); if (sPak < 0 || sPak >= SLOT2_MAX) sPak = 0; }
    return sPak;
}

/* ---------------- virtual cartridge (ss3ds_ctrdg.c) ---------------- */
static int sPresent, sEnabled = 1;
static char sRomPath[256], sWorkPath[256];
static u8 *sFlash; /* FLASH_SIZE, allocated on attach */
static FILE *sRom;
static long sRomSize;
static u32 sGameCode;
static u16 sMakerCode;
static u8 sHeader[0xC0];
static unsigned sWrites;

/* only the changed bytes go to the work copy (a migrate writes ~28 sectors) */
static void persistRange(u32 off, u32 size) {
    if (!sPresent || !sWorkPath[0] || !size || off + size > FLASH_SIZE) return;
    FILE *f = fopen(sWorkPath, "r+b");
    if (!f) { PSPNativeMemLog("[SLOT2] work copy open FAILED %s", sWorkPath); return; }
    if (fseek(f, (long)off, SEEK_SET) != 0 || fwrite(sFlash + off, 1, size, f) != size)
        PSPNativeMemLog("[SLOT2] work copy write FAILED off=%05x size=%u", (unsigned)off, (unsigned)size);
    fflush(f); fclose(f);
}
static void eject(void) {
    if (sPresent && sWrites) PSPNativeMemLog("[SLOT2] eject: %u flash sector(s) were written to the work copy", sWrites);
    if (sRom) { fclose(sRom); sRom = 0; }
    sPresent = 0; sRomPath[0] = sWorkPath[0] = 0; sRomSize = 0; sGameCode = 0; sMakerCode = 0; sWrites = 0;
    memset(sHeader, 0, sizeof sHeader);
}
static int readRom(u32 off, void *dst, u32 size) {
    if (!sRom || !dst || !size) return 0;
    if ((long)off >= sRomSize) { memset(dst, 0xFF, size); return 1; }
    if (fseek(sRom, (long)off, SEEK_SET) != 0) return 0;
    size_t n = fread(dst, 1, size, sRom);
    if (n < size) memset((u8 *)dst + n, 0xFF, size - n);
    return 1;
}
/* the GBA header check the game does (complement byte); the logo is checked by the game itself */
static int headerSumOk(const u8 *h) { u8 sum = 0; for (unsigned i = 0xA0; i <= 0xBC; i++) sum = (u8)(sum + h[i]); return h[0xBD] == (u8)-(sum + 0x19); }
static int attach(const char *rom, const char *work) {
    eject();
    if (!sFlash && !(sFlash = (u8 *)malloc(FLASH_SIZE))) { PSPNativeMemLog("[SLOT2] no memory for the flash image"); return 0; }
    if (!(sRom = fopen(rom, "rb")) || fread(sHeader, 1, 0xC0, sRom) != 0xC0) { PSPNativeMemLog("[SLOT2] cannot read ROM %s", rom); if (sRom) fclose(sRom); sRom = 0; return 0; }
    fseek(sRom, 0, SEEK_END); sRomSize = ftell(sRom);
    memcpy(&sGameCode, sHeader + 0xAC, 4); memcpy(&sMakerCode, sHeader + 0xB0, 2);
    FILE *f = fopen(work, "rb"); long n = 0;
    memset(sFlash, 0xFF, FLASH_SIZE);
    if (f) { n = (long)fread(sFlash, 1, FLASH_SIZE, f); fclose(f); }
    if (n < 0x8000) { PSPNativeMemLog("[SLOT2] work copy missing or too small (%ld) %s", n, work); fclose(sRom); sRom = 0; return 0; }
    snprintf(sRomPath, sizeof sRomPath, "%s", rom); snprintf(sWorkPath, sizeof sWorkPath, "%s", work);
    sPresent = 1; sEnabled = 1;
    PSPNativeMemLog("[SLOT2] pak in: %s (%.4s, %ld bytes, header %s), flash = work copy %s", rom, (const char *)(sHeader + 0xAC), sRomSize,
                    headerSumOk(sHeader) ? "ok" : "BAD: not a clean dump, the game will not offer Migrate", work);
    return 1;
}
static int copyCart(u32 addr, void *dst, u32 size) {
    if (!sPresent || !dst || !size) return 0;
    if (addr >= GBA_ROM_BASE && addr < GBA_ROM_END) return readRom(addr - GBA_ROM_BASE, dst, size);
    if (addr >= GBA_FLASH_BASE && addr < GBA_FLASH_BASE + FLASH_SIZE) { u32 off = addr - GBA_FLASH_BASE; if (off + size > FLASH_SIZE) return 0; memcpy(dst, sFlash + off, size); return 1; }
    return 0;
}

/* ---------------- pak choice ---------------- */
static int endsWith(const char *n, const char *e) { size_t L = strlen(n), E = strlen(e); if (L < E) return 0; for (size_t i = 0; i < E; i++) { char c = n[L - E + i]; if (c >= 'A' && c <= 'Z') c += 32; if (c != e[i]) return 0; } return 1; }
static int cmpName(const void *a, const void *b) { return strcmp((const char *)a, (const char *)b); }
static int ensureWork(const char *src, const char *work) {
    struct stat st;
    if (stat(work, &st) == 0 && st.st_size >= 0x8000) return 1;
    FILE *in = fopen(src, "rb"); if (!in) { PSPNativeMemLog("[SLOT2] no save next to the ROM: %s", src); return 0; }
    FILE *out = fopen(work, "wb"); if (!out) { fclose(in); PSPNativeMemLog("[SLOT2] cannot make work copy %s", work); return 0; }
    u8 buf[0x1000]; size_t total = 0, n;
    while (total < FLASH_SIZE && (n = fread(buf, 1, sizeof buf, in)) > 0) { if (total + n > FLASH_SIZE) n = FLASH_SIZE - total; if (fwrite(buf, 1, n, out) != n) break; total += n; }
    fclose(in);
    memset(buf, 0xFF, sizeof buf);
    while (total < FLASH_SIZE) { n = FLASH_SIZE - total; if (n > sizeof buf) n = sizeof buf; if (fwrite(buf, 1, n, out) != n) break; total += n; }
    fclose(out);
    if (total < FLASH_SIZE) { remove(work); return 0; }
    PSPNativeMemLog("[SLOT2] made work copy %s from %s (the source is never written)", work, src);
    return 1;
}
/* insert the configured pak (once per setting change) */
static void ensureAttached(void) {
    if (!P3DS_Slot2Cfg()) { if (sPresent) eject(); return; }
    if (sTried) return;
    sTried = 1;
    mkdir("sdmc:/3ds", 0755); mkdir("sdmc:/3ds/platinum3ds", 0755); mkdir(SLOT2_DIR, 0755); mkdir(SLOT2_WORK, 0755);
    static char names[SLOT2_MAX][96]; int n = 0;
    DIR *d = opendir(SLOT2_DIR);
    if (d) { struct dirent *e; while ((e = readdir(d)) && n < SLOT2_MAX) if (e->d_name[0] != '.' && endsWith(e->d_name, ".gba") && strlen(e->d_name) < 90) snprintf(names[n++], 96, "%s", e->d_name); closedir(d); }
    if (!n) { PSPNativeMemLog("[SLOT2] slot is empty: no .gba in " SLOT2_DIR); return; }
    qsort(names, n, sizeof names[0], cmpName);
    const int k = pakCfg() < n ? pakCfg() : 0;
    char rom[256], src[256], work[256], sav[96];
    snprintf(sav, sizeof sav, "%s", names[k]); memcpy(sav + strlen(sav) - 4, ".sav", 4);
    snprintf(rom, sizeof rom, SLOT2_DIR "/%s", names[k]); snprintf(src, sizeof src, SLOT2_DIR "/%s", sav); snprintf(work, sizeof work, SLOT2_WORK "/%s", sav);
    PSPNativeMemLog("[SLOT2] %d pak(s) in the folder, using #%d %s", n, k + 1, names[k]);
    if (ensureWork(src, work)) attach(rom, work);
}
void P3DS_Slot2SetCfg(int on) { sOn = on != 0; P3DS_CfgWrite("slot2", sOn); sTried = 0; if (!sOn) eject(); else ensureAttached();
    if (sOn) { char t[48]; snprintf(t, sizeof t, sPresent ? "GBA SLOT: %.4s (next start)" : "GBA SLOT: no pak found", (const char *)(sHeader + 0xAC)); P3DS_Toast(t); }
    PSPNativeMemLog("[SLOT2] slot2=%d from settings", sOn); }
void P3DS_Slot2SetPak(int k) { sPak = k < 0 || k >= SLOT2_MAX ? 0 : k; P3DS_CfgWrite("slot2_pak", sPak); sTried = 0; if (P3DS_Slot2Cfg()) { ensureAttached();
    char t[48]; snprintf(t, sizeof t, sPresent ? "GBA PAK %d: %.4s (next start)" : "GBA PAK %d: nothing there", sPak + 1, (const char *)(sHeader + 0xAC)); P3DS_Toast(t); } }

/* ---------------- CTRDG link wraps ---------------- */
/* (CTRDG_Init stays pspoke's ctrdg_absent.c wrap: it keeps the SDK's own cartridge code from waiting on the ARM7) */
BOOL __wrap_CTRDG_IsExisting(void) { ensureAttached(); return sPresent; }
BOOL __wrap_CTRDG_IsAgbCartridge(void) { ensureAttached(); return sPresent; }
BOOL __wrap_CTRDG_IsPulledOut(void) { ensureAttached(); return !sPresent; }
BOOL __wrap_CTRDG_IsAgbCartridgePulledOut(void) { return 0; }
u32 __wrap_CTRDG_GetAgbGameCode(void) { ensureAttached(); return sPresent ? sGameCode : 0; }
u16 __wrap_CTRDG_GetAgbMakerCode(void) { ensureAttached(); return sPresent ? sMakerCode : 0; }
void __wrap_CTRDG_Enable(BOOL enable) { sEnabled = enable != 0; }
BOOL __wrap_CTRDG_IsEnabled(void) { return sEnabled; }
void __wrap_CTRDG_CheckEnabled(void) { }
void __wrap_CTRDG_CheckPulledOut(void) { }
void __wrap_CTRDG_TerminateForPulledOut(void) { PSPNativeMemLog("[SLOT2] TerminateForPulledOut ignored (virtual pak)"); }
BOOL __wrap_CTRDG_Read32(const u32 *address, u32 *rdata) { u32 t; if (!rdata || !copyCart((u32)address, &t, 4)) return 0; *rdata = t; return 1; }
/* the game only READS the pak through CpuCopy; a source outside the pak range is plain RAM */
static BOOL cpuCopy(const void *src, void *dest, u32 size) {
    const u32 s = (u32)src;
    if (!src || !dest) return 0;
    if (sPresent && s >= GBA_ROM_BASE && s < GBA_FLASH_BASE + FLASH_SIZE) return copyCart(s, dest, size);
    if (s >= GBA_ROM_BASE && s < GBA_FLASH_BASE + FLASH_SIZE) { memset(dest, 0xFF, size); return 0; } /* no pak: open bus */
    memcpy(dest, src, size); return 1;
}
BOOL __wrap_CTRDG_CpuCopy8(const void *src, void *dest, u32 size) { return cpuCopy(src, dest, size); }
BOOL __wrap_CTRDG_CpuCopy16(const void *src, void *dest, u32 size) { return cpuCopy(src, dest, size); }
BOOL __wrap_CTRDG_CpuCopy32(const void *src, void *dest, u32 size) { return cpuCopy(src, dest, size); }
u16 __wrap_CTRDG_IdentifyAgbBackup(int type) { (void)type; if (!sPresent) return 1; PSPNativeMemLog("[SLOT2] IdentifyAgbBackup -> Flash 1M (128 KiB)"); return 0; }
void __wrap_CTRDG_ReadAgbFlash(u16 sec, u32 offset, u8 *dst, u32 size) {
    if (!sPresent || !dst || sec >= FLASH_COUNT) return;
    const u32 off = ((u32)sec << FLASH_SHIFT) + offset; if (off + size > FLASH_SIZE) return;
    memcpy(dst, sFlash + off, size);
}
void __wrap_CTRDG_ReadAgbFlashAsync(u16 sec, u32 offset, u8 *dst, u32 size, CTRDG_TASK_FUNC cb) {
    static CTRDGTaskInfo info; __wrap_CTRDG_ReadAgbFlash(sec, offset, dst, size);
    if (cb) { memset(&info, 0, sizeof info); info.dst = dst; info.offset = offset; info.size = size; info.sec_num = sec; cb(&info); }
}
static u32 writeSector(u16 sec, const u8 *src, u32 verify) {
    if (!sPresent || !src || sec >= FLASH_COUNT) return 0x8000;
    const u32 off = (u32)sec << FLASH_SHIFT;
    memcpy(sFlash + off, src, FLASH_SECTOR);
    if (verify && verify <= FLASH_SECTOR && memcmp(sFlash + off, src, verify)) return 0x8000;
    persistRange(off, FLASH_SECTOR); sWrites++;
    return 0;
}
u32 __wrap_CTRDG_WriteAndVerifyAgbFlash(u16 sec, u8 *src, u32 verify) { return writeSector(sec, src, verify); }
void __wrap_CTRDG_WriteAndVerifyAgbFlashAsync(u16 sec, u8 *src, u32 verify, CTRDG_TASK_FUNC cb) {
    static CTRDGTaskInfo info; const u32 r = writeSector(sec, src, verify);
    if (cb) { memset(&info, 0, sizeof info); info.result = r; info.data = src; info.size = verify; info.sec_num = sec; cb(&info); }
}
u16 __wrap_CTRDG_WriteAgbFlashSector(u16 sec, u8 *src) { return (u16)writeSector(sec, src, FLASH_SECTOR); }
void __wrap_CTRDG_WriteAgbFlashSectorAsync(u16 sec, u8 *src, CTRDG_TASK_FUNC cb) { __wrap_CTRDG_WriteAndVerifyAgbFlashAsync(sec, src, FLASH_SECTOR, cb); }
u16 __wrap_CTRDG_EraseAgbFlashSector(u16 sec) {
    if (!sPresent || sec >= FLASH_COUNT) return 0x8000;
    const u32 off = (u32)sec << FLASH_SHIFT; memset(sFlash + off, 0xFF, FLASH_SECTOR); persistRange(off, FLASH_SECTOR); return 0;
}
void __wrap_CTRDG_EraseAgbFlashSectorAsync(u16 sec, CTRDG_TASK_FUNC cb) {
    static CTRDGTaskInfo info; const u16 r = __wrap_CTRDG_EraseAgbFlashSector(sec);
    if (cb) { memset(&info, 0, sizeof info); info.result = r; info.sec_num = sec; cb(&info); }
}

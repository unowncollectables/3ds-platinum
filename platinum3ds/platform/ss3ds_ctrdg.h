/* Virtual CTRDG (GBA Slot 2) for the official migrate path.
   Serves ROM bytes from the user's .gba and 128KB flash from a work COPY of
   the matching .sav. Never writes the source .sav or /workspace/private-rom. */
#ifndef SS3DS_CTRDG_H
#define SS3DS_CTRDG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Attach a pak: rom_path is read-only; work_sav_path is the 128KB work copy
   (created/padded by the caller). Returns 1 on success. */
int P3DS_CtrdgAttach(const char *rom_path, const char *work_sav_path);
void P3DS_CtrdgEject(void);
int P3DS_CtrdgPresent(void); /* a pak is inserted and flash+header are ready */
/* Official GBA header check on a 0xC0-byte header: 0 ok, 4 = Nintendo logo or
   header checksum wrong (not a full, clean ROM dump). */
int P3DS_CtrdgCheckHeader(const unsigned char *hdr);

#ifdef __cplusplus
}
#endif
#endif

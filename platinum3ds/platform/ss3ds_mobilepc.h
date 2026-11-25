/* ss3ds mobile PC (Settings > SYSTEM, default OFF): a BOXES chip on the party
   screen (footer, left of CANCEL; SwSh/BDSP "To Boxes" style) opens the game's
   own box app (PCBox_LaunchApp mode 2, Move Pokémon) from a safe idle overworld.
   Hook 0293 does the launch and the first-time unlock message. Never writes the
   .sav; unlock "already shown" is sdmc:/3ds/soulsilver3ds/mobile_pc.txt keyed by
   trainer ID. Field message + box launch follow pret/pokeheartgold (GPL-3.0)
   Task_PrintRegisteredKeyItemUseMessage and ScrCmd_158. */
#ifndef SS3DS_MOBILEPC_H
#define SS3DS_MOBILEPC_H

#ifdef __cplusplus
extern "C" {
#endif

int P3DS_MobilePcCfg(void);
int P3DS_MobilePcDevCfg(void);
int P3DS_MobilePcOn(void);      /* cfg on, and (game clear or dev bypass): boxes can be opened */
int P3DS_MobilePcChipOn(void);  /* cfg on, and (unlocked or chip intro anim): the BOXES chip may be drawn */
void P3DS_MobilePcSetCfg(int on);
void P3DS_MobilePcSetDev(int on);
int P3DS_MobilePcUnlockShown(unsigned tid);
void P3DS_MobilePcUnlockMarkShown(unsigned tid);
void P3DS_MobilePcRequestOpen(void);
void P3DS_MobilePcRequestPreview(void);
int P3DS_MobilePcAnimFrame(void); /* -1 = no animation; 0..kMobilePcAnimLast while the chip pop/glow plays */
void P3DS_MobilePcAnimTick(void);
void P3DS_MobilePcNotePartyVisible(void); /* first view of the party after unlock: start the chip pop-in */
int P3DS_MobilePcZoomFrame(void); /* -1 none; 0..kMobilePcZoomLast during the open zoom */
int P3DS_MobilePcChipFooterShown(void); /* official party-select footer chip (not battle) */
int P3DS_MobilePcChipHitDS(int x, int y); /* 256-wide bottom-screen point, before party dest remap */
void P3DS_MobilePcChipRectDS(int *x0, int *y0, int *x1, int *y1);
void P3DS_MobilePcChipRectNative(int *x0, int *y0, int *x1, int *y1);
void P3DS_MobilePcOnChipTap(void); /* start the zoom, then close-if-needed and request the box app */

extern volatile int P3DS_MobilePcReq;         /* 1 = open the box app (game thread) */
extern volatile int P3DS_MobilePcPreviewReq;  /* 1 = play unlock message+anim, do not write sidecar */
extern volatile int P3DS_MobilePcUnlockPlay;  /* 1 while the game-thread unlock task is running */
extern volatile int P3DS_GameClear;           /* hook 0293: CheckGameClearFlag, read-only */
extern volatile int P3DS_InPartyMenu;         /* 1 while the party-menu overlay is the active app */
extern volatile int P3DS_MobilePcCloseThenLaunch; /* 1: inject B until the party closes, then request open */

enum { kMobilePcAnimLast = 40 };
enum { kMobilePcZoomLast = 10 };
/* Official HGSS party-select footer, in the 256x192 party picture: just left of CANCEL,
   over the empty end of the "Choose a Pokémon." window (text ends earlier). */
enum { kMobilePcChipSX0 = 138, kMobilePcChipSY0 = 166, kMobilePcChipSX1 = 198, kMobilePcChipSY1 = 187 };

#ifdef __cplusplus
}
#endif

#endif

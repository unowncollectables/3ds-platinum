/* Slot 2 (GBA SLOT) on the continue menu. Settings toggle default OFF.
   When on, a virtual CTRDG serves a work COPY of the matching GBA .sav
   (never the source). Official migrate / Pal Park are unchanged. */
#ifndef SS3DS_SLOT2_H
#define SS3DS_SLOT2_H

#ifdef __cplusplus
extern "C" {
#endif

int P3DS_Slot2Cfg(void);
void P3DS_Slot2SetCfg(int on);
void P3DS_Slot2ContinueTick(void);
void P3DS_Slot2ContinueEnd(void);
int P3DS_Slot2ListOpen(void);
int P3DS_Slot2BlockMenuInput(void);
int P3DS_Slot2ButtonHit(int x, int y);
void P3DS_Slot2OpenList(void);
void P3DS_Slot2Present(unsigned *top, unsigned *bot);

/* Load last-pick / keep the virtual cart ready before DetectInsertedGBACart. */
void P3DS_Slot2PrepareCart(void);
int P3DS_Slot2VirtualCartPresent(void);
/* After a mid-menu insert/eject: caller should Unlink + re-Detect + rebuild. */
int P3DS_Slot2ConsumeRedraw(void);
void P3DS_Slot2UnlinkAgbSpec(void);
/* hook 0296: DetectInsertedGBACart result (offsets 0 = identified). */
void P3DS_Slot2Detected(int offsets, int natdex, int langOk);

#ifdef __cplusplus
}
#endif
#endif

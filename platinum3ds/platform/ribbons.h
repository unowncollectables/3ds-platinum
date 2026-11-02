/* ribbon_check: read-only ribbon checklist on the Pokemon summary screen (ribbons.c). */
#ifndef P3DS_RIBBONS_H
#define P3DS_RIBBONS_H
#ifdef __cplusplus
extern "C" {
#endif
#define RIBBON_MAX 96
#define RIBBON_KEY_X (1u << 10)
#define RIBBON_KEY_L (1u << 9)
#define RIBBON_KEY_R (1u << 8)
int P3DS_RibbonOn(void);
void P3DS_RibbonSetOn(int v);
void P3DS_RibbonCapture(unsigned mon, unsigned (*get)(unsigned, int, void *));
void P3DS_RibbonFrame(unsigned held_keys);
void P3DS_RibbonDraw(unsigned *img);
#ifdef __cplusplus
}
#endif
#endif

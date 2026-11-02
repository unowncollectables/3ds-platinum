/* celebrations: birthday and holiday particle effects (celebrate.c). */
#ifndef P3DS_CELEBRATE_H
#define P3DS_CELEBRATE_H
#ifdef __cplusplus
extern "C" {
#endif
int P3DS_CelebOn(void);
void P3DS_CelebSetOn(int v);
void P3DS_CelebDraw(unsigned *img, int eye);
#ifdef __cplusplus
}
#endif
#endif

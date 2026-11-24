#ifndef SS3DS_GTS_PREVIEW_H
#define SS3DS_GTS_PREVIEW_H

#ifdef __cplusplus
extern "C" {
#endif

enum { GTS_PREVIEW_NONE, GTS_PREVIEW_OFFERED, GTS_PREVIEW_PICKUP };

typedef struct {
    unsigned species;
    unsigned level;
    unsigned wanted;
    unsigned state;
} GtsPreview;

void P3DS_GtsPreviewTrainer(unsigned trainerId);
GtsPreview P3DS_GtsPreviewRead(int server);

#ifdef __cplusplus
}
#endif
#endif

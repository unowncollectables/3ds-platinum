/* platinum3ds: PSP-only app pieces with no 3DS equivalent yet. */
#include <nitro.h>
int PSPNativeOskIsActive(void) { return 0; }
int PSPNativeOskFinished(void) { return 1; }
int PSPNativeOskAccepted(void) { return 0; }
const unsigned short *PSPNativeOskText(void) { static const unsigned short z[1]; return z; }
void PSPNativeOskRelease(void) {}
void PSPNativeOskBegin(const unsigned short *d, const unsigned short *i, int l) {}
void PSPNativeOskFrameHook(void) { extern void P3DS_FrameHook(void); P3DS_FrameHook(); }
/* Not reached on 3DS (memprobe keeps the DS naming screen); never return NULL as a template. */
extern const char gNamingScreenAppTemplate[];
const void *PSPNativeNamingOskTemplate(int heapID) { return gNamingScreenAppTemplate; }

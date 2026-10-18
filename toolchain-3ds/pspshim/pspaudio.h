/* platinum3ds step 2a: PSP audio output stubbed (silent, paced). */
#pragma once
#include <pspkernel.h>
#define PSP_AUDIO_VOLUME_MAX 0x8000
#define PSP_AUDIO_NEXT_CHANNEL (-1)
#define PSP_AUDIO_FORMAT_STEREO 0
#define PSP_AUDIO_SAMPLE_ALIGN(s) (((s) + 63) & ~63)
int sceAudioChReserve(int ch, int samples, int fmt);
int sceAudioChRelease(int ch);
int sceAudioOutputPannedBlocking(int ch, int l, int r, void *buf);

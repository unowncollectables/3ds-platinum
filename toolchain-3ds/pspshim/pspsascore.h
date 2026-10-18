/* platinum3ds step 2a: sceSasCore stubbed (voices accepted, mix is silence). */
#pragma once
#include <pspkernel.h>
#define PSP_SAS_VOICES_MAX 32
#define PSP_SAS_GRAIN_SAMPLES 256
#define PSP_SAS_VOLUME_MAX 4096
#define PSP_SAS_LOOP_MODE_OFF 0
#define PSP_SAS_LOOP_MODE_ON 1
#define PSP_SAS_PITCH_MIN 0x1
#define PSP_SAS_PITCH_BASE 0x1000
#define PSP_SAS_PITCH_MAX 0x4000
#define PSP_SAS_NOISE_FREQ_MAX 0x3F
#define PSP_SAS_ENVELOPE_HEIGHT_MAX 0x40000000
#define PSP_SAS_ENVELOPE_FREQ_MAX 0x7FFFFFFF
#define PSP_SAS_ADSR_CURVE_MODE_LINEAR_INCREASE 0
#define PSP_SAS_ADSR_CURVE_MODE_LINEAR_DECREASE 1
#define PSP_SAS_ADSR_CURVE_MODE_LINEAR_BENT 2
#define PSP_SAS_ADSR_CURVE_MODE_EXPONENT_REV 3
#define PSP_SAS_ADSR_CURVE_MODE_EXPONENT 4
#define PSP_SAS_ADSR_CURVE_MODE_DIRECT 5
#define PSP_SAS_ADSR_ATTACK 1
#define PSP_SAS_ADSR_DECAY 2
#define PSP_SAS_ADSR_SUSTAIN 4
#define PSP_SAS_ADSR_RELEASE 8
#define PSP_SAS_ADSR_EVERYTHING 15
#define PSP_SAS_OUTPUTMODE_STEREO 0
#define PSP_SAS_OUTPUTMODE_MULTICHANNEL 1
#define PSP_SAS_EFFECT_TYPE_OFF (-1)
#define PSP_SAS_GAIN_SHIFT 0
int __sceSasInit(void *core, int grain, int voices, int mode, int rate);
int __sceSasCore(void *core, void *out);
int __sceSasSetVoicePCM(void *core, int v, void *buf, int size, int loop);
int __sceSasSetPitch(void *core, int v, int pitch);
int __sceSasSetVolume(void *core, int v, int l, int r, int el, int er);
int __sceSasSetKeyOn(void *core, int v);
int __sceSasSetKeyOff(void *core, int v);
int __sceSasSetADSR(void *core, int v, int flag, unsigned a, unsigned d, unsigned s, unsigned r);
int __sceSasSetADSRmode(void *core, int v, int flag, int a, int d, int s, int r);
int __sceSasSetSL(void *core, int v, int level);
int __sceSasGetAllEnvelopeHeights(void *core, int *heights);
#define sceSasInit __sceSasInit
#define sceSasCore __sceSasCore
#define sceSasSetVoicePCM __sceSasSetVoicePCM
#define sceSasSetPitch __sceSasSetPitch
#define sceSasSetVolume __sceSasSetVolume
#define sceSasSetKeyOn __sceSasSetKeyOn
#define sceSasSetKeyOff __sceSasSetKeyOff
#define sceSasSetADSR __sceSasSetADSR
#define sceSasSetADSRmode __sceSasSetADSRmode
#define sceSasSetSL __sceSasSetSL
#define sceSasGetAllEnvelopeHeights __sceSasGetAllEnvelopeHeights
typedef struct { unsigned char data[512]; } SceSasCore;

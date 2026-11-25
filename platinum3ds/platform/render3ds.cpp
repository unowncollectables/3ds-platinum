#include "crumbs.h"
#include <time.h>
#include <algorithm>
#include <math.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "../native-render-opt/native_render.h"
#include "../native-render-opt/native_gpu.h"
#include "widecomp.h"
#include <nitro.h>
#include <simulator/g3_handler.h>
#include <simulator/g3_draw.h>
#include <cstdio>
#include <malloc.h>
extern "C" void PSPNativeMemLog(const char*,...); extern "C" void P3DS_FpsGuardFrame(void); extern "C" int P3DS_FpsTweaks(void); extern "C" void P3DS_SetFpsTweaks(int); extern "C" int P3DS_FpsTweakRow(void); extern "C" void P3DS_SetAudioGain(int); extern "C" void P3DS_SetAudioLimiter(int); extern "C" void P3DS_SetAudioStallFade(int); extern "C" void G3G_SetSharp(int); extern "C" void P3DS_SetStereoDepth(int); extern "C" int P3DS_SaveExport(void);
extern "C" int P3DS_CfgInt(const char*,int);
extern "C" unsigned PSPNativeVramPageGen[];
extern "C" volatile unsigned long long P3DS_WaitTicks;extern "C" volatile unsigned P3DS_WaitCalls;
extern "C" int P3DS_LogLevel(void); extern "C" volatile unsigned P3DS_LogLines,P3DS_LogBytes; /* run 113: logring.c counters for the [BATT] line */ extern "C" void P3DS_ProfFrame(unsigned); extern "C" int P3DS_ProfOn; extern "C" void P3DS_HeapFastStats(void);
/* run 101: read-only views of the game's own save data (never written here) */
extern "C" { void*SaveData_Ptr(void);void*SaveData_GetTrainerInfo(void*);unsigned char TrainerInfo_Gender(const void*);void*SaveData_GetPlayTime(void*);unsigned short PlayTime_GetHours(const void*);unsigned char PlayTime_GetMinutes(const void*); }
static bool p3dsPlayTime(unsigned*h,unsigned*m){void*sd=SaveData_Ptr();if(!sd)return false;void*pt=SaveData_GetPlayTime(sd);if(!pt)return false;*h=PlayTime_GetHours(pt);*m=PlayTime_GetMinutes(pt);return true;}
extern "C" { void*SaveData_GetBag(void*); }
static bool p3dsHasTownMap(){void*sd=SaveData_Ptr();if(!sd)return false;const unsigned short*b=(const unsigned short*)SaveData_GetBag(sd);if(!b)return false;const unsigned short*k=b+165*2;for(int i=0;i<50;i++)if(k[i*2]==442&&k[i*2+1])return true;return false;}
static int p3dsFemale(){void*sd=SaveData_Ptr();if(!sd)return -1;void*ti=SaveData_GetTrainerInfo(sd);return ti?TrainerInfo_Gender(ti)==1:-1;}
extern "C" { int P3DS_StripOn; extern int P3DS_LowerMenuDraw; int lmYPress; int lmTouchInj; volatile int P3DS_BagPocketReq=-1; }
#include "nitro_port_lowermenu.h"
static unsigned P3DS_Perf3Worst;static u64 P3DS_St[16];static unsigned P3DS_SerialSt;static char P3DS_PathC='-';static int lastFld=-1;static unsigned long long P3DS_SpkWait0; /* run 95 [SPIKE] stage stamps */extern "C" int P3DS_CurMap;extern "C" void P3DS_ZoneInfo(int*,int*,int*,unsigned*),P3DS_RomSpikeFiles(char*,unsigned),P3DS_RomSpikeReset(void);extern "C" const char*P3DS_AreaName(int);
extern "C" { extern volatile int P3DS_MmMap,P3DS_MmX,P3DS_MmZ,P3DS_MmCx,P3DS_MmCy,P3DS_MmType,P3DS_MmMain,P3DS_MmSec;extern volatile unsigned P3DS_MmTick;extern char P3DS_MmName[24]; }
extern "C" { extern volatile unsigned P3DS_TileKeys;extern volatile int P3DS_TileOn,P3DS_TileCur;volatile int P3DS_BmOn,P3DS_BmBlock,P3DS_CatchMask; /* CatchMask: the battle Y screen (or the moment after it closes) owns the buttons and touch screen, the game gets none */ /* run 142: BmBlock = the game's own Start menu is alive in the field and not ours to use: no touch, no A/B/D-pad reach it */ extern volatile int P3DS_StartReq,P3DS_InjectFrames;extern volatile unsigned P3DS_InjectKeys,P3DS_StartMenuTick,P3DS_StartAvail;extern volatile int P3DS_StartClose,P3DS_StartPrompt,P3DS_StartIdle; extern volatile int P3DS_PanelPartyWant,P3DS_PanelPartyOn; extern volatile int P3DS_PanelDest[4], P3DS_PanelSrcHeight, P3DS_PanelGearTooltip; }

/* run 76 hunt_counter / shiny_suite (ported from Platinum run 88): wild encounters per species in sdmc:/3ds/platinum3ds/hunt.txt
   (never the save; hunt.c). Counted once per wild battle when the command menu first opens (P3DS_CatchInfo snapshot);
   [18] = the game's own Pokemon_IsShiny on the foe. shiny_suite: chime + toast + top-screen flash (drawShinyFlash) on a shiny. */
#include "hunt.h"
#include "ribbons.h"
#include "tracker.h"
#include "achievements.h"
#include "ss3ds_mobilepc.h"
#include "ss3ds_slot2.h"
#include "celebrate.h"
static int P3DS_HuntCount(int sp){return P3DS_HuntGet(sp)->enc;}
extern "C" volatile int P3DS_CatchInfo[20];extern "C" int P3DS_InField,P3DS_InBattle;/* InBattle: defined ss3ds_glue.c; needed by huntTick (above the wide_battle extern below) */
extern "C" volatile int P3DS_MenuOpen;extern "C" void P3DS_Toast(const char*);
extern "C" volatile int P3DS_ResetSeen; /* run 53 (main3ds.c) */
#define PSPNativeMemLogC PSPNativeMemLog
#include <cstring>
#include <array>
static constexpr auto alphaTo5=[](){std::array<u32,256>a{};for(unsigned i=0;i<256;i++)a[i]=((i*31+127)/255)<<24;return a;}();
/* run 57: compositor output buffers in linear (GPU-readable) memory so the present can DMA them straight into the
   scale textures (gpu_swizzle=1, gpuscale.c) instead of a CPU byte-swap pass per screen. */
extern "C" void* linearMemAlign(size_t size, size_t alignment);
static u32 (*raw)[256*256];static u32 (*&display)[256*256]=raw;static u32*rawR;
static void allocBufs(){if(raw)return;void*p=linearMemAlign(3*256*256*4,0x80);const bool lin=p!=nullptr;if(!p)p=memalign(64,3*256*256*4);memset(p,0,3*256*256*4);
 raw=(u32(*)[256*256])p;rawR=(u32*)p+2*256*256;PSPNativeMemLog("[2D] compositor buffers in %s memory",lin?"linear":"heap");}
static GPU2D::Unit engineA(0),engineB(1);static GPU2D::SoftRenderer renderer,rendererB;
/* Engine B (sub screen) composites on a core-2 worker in parallel with engine A on the main thread (New 3DS). */
extern "C" {
}
extern "C" {
#include <3ds/types.h>
#include <3ds/svc.h>
#include <3ds/synchronization.h>
#include <3ds/thread.h>
#include <3ds/services/apt.h>
#include <3ds/allocator/linear.h>
}
extern __thread u32*P3DS_3DSrc;
static GPU2D::Unit engineA2(0);static GPU2D::SoftRenderer rendererR;
/* issue #484 FOE DEEPER (stereo_2d=4, see P3DS_BattleSideExtra): the CPU right eye switches the back BG to bdLow from line bdSplit down */
static const int kBattleSplitY=128; /* foe platform ends ~y120, yours starts ~y140 (HGSS singles and doubles) */
extern "C" int P3DS_BattleRowExtra(int y,int sd);
static int bdMask,bdSplit,bdSd;static u16 bdG[4],bdMine[4],bdD[4];
/* run 50 split_top=1 (config, default 1 since run 57): MONO only. Engine A (top screen) lines 96..191 are composited on the core-2
   worker (own Unit copy + own renderer) while the main thread does lines 0..95. The copy is fast-forwarded through
   lines 0..95 without drawing (windows, affine BG reference points, BG/OBJ mosaic counters: same steps melonDS's
   skipped-line path uses). Stereo already keeps both cores busy (left eye main, right eye worker), so no split there.
   Expected: -3..-5 ms per mono frame when the top screen is the critical path. */
static GPU2D::Unit engineAS(0);static GPU2D::SoftRenderer rendererS;static volatile bool bSplit,line0Done;static int splitCfg=-1;static unsigned splitN;
static void dryLine(GPU2D::Unit&u,unsigned line){
 if(line==0){u.OBJMosaicY=0;u.OBJMosaicYCount=0;}
 u.CheckWindows(line);
 if(!(u.DispCnt&(1<<7))){const u32 m=u.DispCnt&7;
  if((u.DispCnt&0x0800)&&m>=1&&m<=5){u.BGXRefInternal[1]+=u.BGRotB[1];u.BGYRefInternal[1]+=u.BGRotD[1];}
  if((u.DispCnt&0x0400)&&(m==2||m==4||m==5||m==6)){u.BGXRefInternal[0]+=u.BGRotB[0];u.BGYRefInternal[0]+=u.BGRotD[0];}
  if(u.BGMosaicY>=u.BGMosaicYMax){u.BGMosaicY=0;u.BGMosaicYMax=u.BGMosaicSize[1];}else u.BGMosaicY++;}
 u.UpdateMosaicCounters(line);}static u32*volatile bRightSrc;
static LightEvent bStart,bDone;static Thread bThread;static volatile bool bQuit;static int bMode=-1;
/* run 38: stereo_c1=1 (default). In stereo the core-2 worker had engine B AND the right eye in series (~2x9 ms) while
   the main thread did engine A (~9 ms) then waited: composite 18.6 ms vs 9.6 mono (log_run37). Now, in stereo only,
   engine B runs on a core-1 (syscore) thread (APT_SetAppCpuTimeLimit(core1_limit, default 80)), so the three ~9 ms
   jobs run on three cores. Falls back to the old split if the thread can't be created. stereo_c1=0 = old behaviour. */
static volatile bool bGpuFrame,bGpuVerify;static u32*bGpuCheck;static int gpuBV=-1;
static int gpuBCfg(){if(gpuBV<0){gpuBV=P3DS_CfgInt("gpu_compose_b",1);if(gpuBV<0||gpuBV>2)gpuBV=0;PSPNativeMemLog("[GCB] gpu_compose_b=%d (0 CPU, 1 GPU, 2 pixel check)",gpuBV);}return gpuBV;}
static volatile bool bSkipB;static LightEvent c1Start,c1Done;static Thread c1Thread;static int c1Mode=-1;
/* run 40: bottom_skip=1 (default). Engine B (sub screen, never 3D) is recomposed only when one of its inputs changed:
   regs 0x1000-0x106F, sub palettes, sub OAM, sub BG/OBJ VRAM, sub ext palettes (pspoke's BottomSame/SaveBottom
   snapshot compare). The compare runs on whichever thread composes engine B (core-2 worker), off the main thread.
   Invalidated when POWCNT (enable / screen swap) changes. Saves ~9 ms of core-2 time in stereo, where the worker also
   composes the right eye. bottom_skip=0 = always recompose. */
// Engine B jobs run serially; the main thread reads the draw totals after the existing completion waits.
static u64 bScanTicks;
static unsigned bScanFrames;
static void drawEngineB();
static void c1Worker(void*){P3DS_ThreadGuard("c1worker");for(;;){LightEvent_Wait(&c1Start);if(bQuit)break;drawEngineB();LightEvent_Signal(&c1Done);}}
static bool c1Ready(){if(c1Mode<0){c1Mode=0;int on=0,lim=80;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f)){if(sscanf(l,"stereo_c1=%d",&x)==1)on=x!=0;if(sscanf(l,"core1_limit=%d",&x)==1)lim=x;}fclose(f);}
  if(lim<10)lim=10;if(lim>80)lim=80;
  if(on){Result r=APT_SetAppCpuTimeLimit(lim);LightEvent_Init(&c1Start,RESET_ONESHOT);LightEvent_Init(&c1Done,RESET_ONESHOT);
   if(R_SUCCEEDED(r))c1Thread=threadCreate(c1Worker,nullptr,0x10000,0x2F,1,false);c1Mode=c1Thread?1:0;
   PSPNativeMemLog("[2D] stereo_c1: engine B on core 1 in stereo: %s (limit %d%%, rc %08lx)",c1Mode?"yes":"no",lim,(unsigned long)r);}
  else PSPNativeMemLog("[2D] stereo_c1=0: engine B stays on core 2 in stereo");}
 return c1Mode==1;}
/* run 41: steal_b=1 (default). In stereo the worker composes the right eye FIRST, then engine B only if the main
   thread (finished with engine A) has not already claimed it; the main thread claims engine B right after engine A.
   Whoever is free first takes it (atomic claim), so the bottom compare/compose no longer sits in front of the right
   eye on core 2. steal_b=0 = old order (B then right eye on the worker). */
static volatile int bClaim;static int stealCfg=-1;static unsigned stealMain,stealWorker;
static bool claimB(){return __atomic_exchange_n(&bClaim,1,__ATOMIC_ACQ_REL)==0;}
/* run 48 early_b=1 (default): engine B (bottom screen, never uses 3D) is started on the core-2 worker BEFORE the main
   thread blocks on the GPU 3D draw + readback (G3SIM_FlushDeferred), instead of after it. No game code runs between
   the two points, so it composes exactly the same state; it just overlaps the 4-8 ms GPU wait. The worker's later
   right-eye job then no longer has engine B queued in front of it. 0 = old order. */
static volatile bool bEarly;static LightEvent bEarlyDone;static int earlyCfg=-1;static unsigned earlyN;
struct P3DSMapEntR{unsigned short species;unsigned char lo,hi,pct,sec;};extern "C" volatile int P3DS_MapId,P3DS_MapEntN,P3DS_MapTod;extern "C" P3DSMapEntR P3DS_MapEnt[48];extern "C" unsigned*P3DS_MapIcons;extern "C" int P3DS_MapIconMem[48],P3DS_MapIconPal[48],P3DS_MapIconPalFile;extern "C" volatile int P3DS_MapGen;extern "C" int P3DS_IconLoad(int,const int*,const int*,int,uint32_t*);
extern "C" void P3DS_Crumb(const char*,void*);
/* run 65 boot-freeze fix: the early-B and the frame job both signal the ONESHOT bStart; when both signals land before
   the worker wakes they collapse into one, the worker runs only the early part and the main thread waits for bDone
   forever ("game stalled ... main last: b-wait"). A pending-work counter makes the hand-off lossless. */
static volatile int bPend;
static void bPost(){__atomic_add_fetch(&bPend,1,__ATOMIC_SEQ_CST);LightEvent_Signal(&bStart);}
/* frame-0 stall fix (friend's console, log-4): the worker never consumed a job on a skip-intro soft-reset boot and main waited bDone forever.
   The job body is shared so main can run it inline when the worker has taken nothing. */
/* H-blank scroll wave (unk_02014A08: water/surf battle intro and other wavy effects). On the DS the game H-blank-DMAs one
   value per screen line into an engine A BG register (BG0HOFS/VOFS at 0x04000010). __wrap_MI_HBlankDmaCopy32/16
   (main3ds.c) records the line table instead of copying to the DS address (runs 939/940 crashed there); each CPU
   composite pass writes that line's value into the unit before drawing the line, exactly as the DMA would. Snapshot
   per frame so the split/right-eye workers read a stable copy. While it runs the GPU compositor (whole-frame
   registers) is skipped and the CPU path draws the frame. */
extern "C" volatile const void*P3DS_HbSrc;extern "C" volatile unsigned P3DS_HbReg,P3DS_HbUnit,P3DS_HbSeq;
static u32 hbVal[192];static unsigned hbReg,hbUnit;static bool hbOn;
static void hbSnap(){static unsigned lastSeq,idle,logN;const unsigned seq=P3DS_HbSeq;if(seq!=lastSeq){lastSeq=seq;idle=0;}else if(idle<255)idle++;
 const u8*src=(const u8*)P3DS_HbSrc;const unsigned reg=P3DS_HbReg,unit=P3DS_HbUnit;
 /* re-armed every game VBlank while the wave runs; MI_StopDma clears the source when the effect ends */
 hbOn=src&&seq&&idle<4&&(unit==2||unit==4)&&reg>=8&&reg+unit<=0x58;if(!hbOn)return;hbReg=reg;hbUnit=unit;
 for(unsigned y=0;y<192;y++){const unsigned k=y?y-1:0;hbVal[y]=unit==4?*(const u32*)(src+4*k):*(const u16*)(src+2*k);} /* the DMA for line y-1's H-blank feeds line y */
 if(logN<3&&idle==0){logN++;PSPNativeMemLog("[HBLANK] wave on engine A reg %03x (%u bytes a line), lines 0/96/191 = %08x %08x %08x",reg,unit,(unsigned)hbVal[0],(unsigned)hbVal[96],(unsigned)hbVal[191]);}}
static inline void hbLine(GPU2D::Unit&u,unsigned line){if(!hbOn)return;const u32 v=hbVal[line];
 if(hbUnit==2)u.Write16(hbReg,(u16)v);
 else if(hbReg==0x28||hbReg==0x2c||hbReg==0x38||hbReg==0x3c)u.Write32(hbReg,v);
 else{u.Write16(hbReg,(u16)v);u.Write16(hbReg+2,(u16)(v>>16));}}
static volatile bool bWideR,bWideROk;static void wideRJob(); /* run 97: right-eye wide compose on core 2 */
static volatile bool bWideM;static void wideMJob(); /* run 105 wide_split: half of the mono wide compose on core 2 */
static void engineBFrameJob(){
  const bool steal=stealCfg==1&&bRightSrc;
  if(!bSkipB&&!steal)drawEngineB();
  if(bWideR){wideRJob();bWideR=false;}
  if(bWideM){wideMJob();bWideM=false;}
  if(bRightSrc){/* stereo: right-eye top screen = engine A again (snapshot copy) with the right-eye 3D layer */
   P3DS_3DSrc=bRightSrc;rendererR.SetFramebuffer(rawR,rawR);
   for(unsigned line=0;line<192;line++){hbLine(engineA2,line);if(bdMask)for(int i=1;i<4;i++)if(bdMask&(1<<i)){const u16 cur=engineA2.BGXPos[i];if(cur!=bdMine[i])bdG[i]=cur;/* the game wrote a new scroll (HBlank effects): keep it, add our depth */ const u16 m=(u16)((bdG[i]-bdD[i]-P3DS_BattleRowExtra((int)line,bdSd))&0x1FF);engineA2.BGXPos[i]=m;bdMine[i]=m;}
    engineA2.CheckWindows(line);rendererR.DrawSprites(line,&engineA2);rendererR.DrawScanline(line,&engineA2);}
   P3DS_3DSrc=nullptr;}
  if(!bSkipB&&steal&&claimB()){stealWorker++;drawEngineB();}
  if(bSplit){for(unsigned line=0;line<96;line++)dryLine(engineAS,line);
   while(!line0Done)svcSleepThread(20000);
   rendererS.SetFramebuffer(raw[0],raw[1]);
   for(unsigned line=96;line<192;line++){hbLine(engineAS,line);engineAS.CheckWindows(line);rendererS.DrawSprites(line,&engineAS);rendererS.DrawScanline(line,&engineAS);}
   bSplit=false;}}
static void engineBWorker(void*){P3DS_ThreadGuard("engineB");P3DS_Crumb("w-start",0);for(;;){
  /* take one pending job; CAS so main can reclaim (exchange to 0) jobs the worker never took */
  bool got=false;while(!bQuit){int v=__atomic_load_n(&bPend,__ATOMIC_SEQ_CST);if(v>0){if(__atomic_compare_exchange_n(&bPend,&v,v-1,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST)){got=true;break;}}else LightEvent_Wait(&bStart);}
  if(!got||bQuit)break;P3DS_Crumb(bEarly?"w-early":"w-job",0);
  if(bEarly){drawEngineB();bEarly=false;P3DS_Crumb("w-early-sig",0);LightEvent_Signal(&bEarlyDone);continue;}
  engineBFrameJob();
  P3DS_Crumb("w-done-sig",0);LightEvent_Signal(&bDone);}}
/* Wait for the frame job. If nothing is signalled for 150 ms AND the worker has taken none of this frame's `posted` jobs,
   reclaim them and run them inline (stale-signal safe: the worker never ran them, so it signals nothing), then keep engine B
   inline for the rest of this boot. Returns true when reclaimed (early job already done too). */
static bool bWaitDone(bool excl,int posted,bool&earlyB){
  u64 t=svcGetSystemTick();
  for(;;){
   if(excl){if(LightEvent_TryWait(&bDone))return false;}
   else if(!LightEvent_WaitTimeout(&bDone,10000000LL))return false;
   if(svcGetSystemTick()-t<150000ULL*268ULL)continue;
   int n=__atomic_exchange_n(&bPend,0,__ATOMIC_SEQ_CST);
   if(n==posted){PSPNativeMemLog("[2D] engine B worker took no job in 150 ms: reclaimed %d, engine B inline from now on",n);P3DS_Crumb("b-reclaim",0);
    if(earlyB){drawEngineB();bEarly=false;earlyB=false;}
    engineBFrameJob();bMode=0;return true;}
   if(n>0){__atomic_add_fetch(&bPend,n,__ATOMIC_SEQ_CST);LightEvent_Signal(&bStart);}
   t=svcGetSystemTick();}}
static bool engineBAsync(){if(bMode<0){{FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");int x;char l[64];if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"async2d=%d",&x)==1&&!x)bMode=0;fclose(f);}if(!bMode){PSPNativeMemLog("[2D] async2d=0: engine B inline");return false;}}LightEvent_Init(&bStart,RESET_ONESHOT);LightEvent_Init(&bDone,RESET_ONESHOT);
  LightEvent_Init(&bEarlyDone,RESET_ONESHOT);bThread=threadCreate(engineBWorker,nullptr,0x10000,0x2F,2,false);bMode=bThread?1:0;PSPNativeMemLog("[2D] engine B worker on core 2: %s",bMode?"yes":"no (inline)");}
 return bMode==1;}

static bool initialized=false,openFrame=false;static unsigned frames=0,lastDraw=0,previousPower=~0u,grace=0;
static unsigned inputKeys=0,oldKeys=0;static int mode=0,oldMode=0,down=0,oldDown=0,x=128,y=96,oldX=128,oldY=96;
static unsigned bindUs=0,drawUs=0,convertUs=0;
static unsigned displayOffset=0,lastReadbackUs=0,last2DUs=0;
extern "C" void PSPNativeG3Release();
extern "C" unsigned char PSPNative_GfxRegisters[];
static unsigned reg16(unsigned off){return *(volatile u16*)(PSPNative_GfxRegisters+off);}
/* 2026-11-04 (Nathan: wide view must look right all the time): DS windows (WIN0/WIN1, engine A) hide layers in parts of the screen
   (battle intro wipes, the black band under a battle intro, transition effects). The side strips ignored them, so they kept
   showing the scene where the picture's edge was black. Layer mask (bits 0-3 BG, 4 OBJ, 5 effects) the DS uses at the screen edge
   pixel (x 0 or 255) of row y; 0x3F when no window is on. The OBJ window needs sprite pixels and is not followed. */
static unsigned winEdge(int y,int side){const unsigned dc=*(volatile u32*)PSPNative_GfxRegisters;if(!(dc&0x6000))return 0x3F;const int x=side?255:0;
 auto in=[](int v,int a,int b){return a<=b?(v>=a&&v<b):(v>=a||v<b);};
 const unsigned h0=reg16(0x40),h1=reg16(0x42),v0=reg16(0x44),v1=reg16(0x46),wi=reg16(0x48),wo=reg16(0x4A);
 if((dc&0x2000)&&in(y,(int)(v0>>8),(int)(v0&255))&&in(x,(int)(h0>>8),(int)(h0&255)))return wi&63;
 if((dc&0x4000)&&in(y,(int)(v1>>8),(int)(v1&255))&&in(x,(int)(h1>>8),(int)(h1&255)))return (wi>>8)&63;
 return wo&63;}
static unsigned winStripN,winStripRows;
static unsigned reg32(unsigned off){return *(volatile u32*)(PSPNative_GfxRegisters+off);}
#include "../native-render-opt/bottom_cache.h"
extern "C" { volatile unsigned P3DS_SceneGen; int P3DS_LandFast; }
/* run 121: dropped saveJohtoChrome (stale 256x192 snapshot with short DEX/PKMN alts; panel uses live ppChrome + bottom_hd) */
/* ss3ds run 17: BLINV = bottomCache.valid=false + a generation bump, so the JOHTO/tiles overlay image (blCache) is redrawn only when something it shows changed */
static unsigned blGen,blLine;static bool blSameFrame;
#define BLINV (bottomCache.valid=false,blGen++,blLine=__LINE__)
/* fps_tweaks >= 2 (2026-10-27): the Bottom menu covered the whole bottom screen last frame (it paints all 256x192 of it),
   so engine B's picture would be thrown away: drawEngineB skips the compose and forces a full one on the next shown frame */
static volatile bool bHideB;static unsigned pfBHid;
/* 2026-11-04 (Nathan: screen swaps must be clean): engine B's buffer still holds the Bottom menu picture when its compose was
   skipped (bHideB). When the game swaps the screens (POWCNT bit 15, e.g. opening the Pokedex from a tile) on such a frame, that
   stale menu picture was shown on the TOP screen for a frame, at full brightness in the middle of the fade. */
static volatile bool bStale;static unsigned bStaleShown;
static bool gBmGapOnly;
static int bSkipCfg=-1;static volatile unsigned bPower;static unsigned bLastPower=~0u;static unsigned bSkips,bDraws,lS,lD;volatile int P3DS_BottomReused;
#include "title_layers.h"
extern "C" int P3DS_InTitle;
extern "C" int P3DS_InPokewalker;
static int titleCleanCfg(){return 0;} /* SoulSilver title layout (Lugia title layers); Platinum keeps its own title */
extern "C" {extern int P3DS_TitleWideRequested,P3DS_GcTitleWideValid;extern u32 P3DS_GcTitleStrips[];}
static u32 titleCanvas[320*192],titleStrips[64*192];static volatile bool titleWideValid;
static int titleCompose(bool wide){return SS3DS_Title2D(PSPNative_GfxRegisters+0x1000,GPU::VRAMFlat_BBG,(const uint16_t*)(GPU::Palette+1024),(uint32_t*)titleCanvas,wide);}
static void titleCopyWide(){for(unsigned y=0;y<192;y++){
 memcpy(display[1]+y*256,titleCanvas+y*320+32,256*4);
 for(unsigned x=0;x<32;x++){titleStrips[y*64+x]=__builtin_bswap32(titleCanvas[y*320+x]);titleStrips[y*64+32+x]=__builtin_bswap32(titleCanvas[y*320+288+x]);}}
 titleWideValid=true;bottomCache.valid=false;
}
static void drawEngineB(){
 if(bGpuFrame){bottomCache.valid=false;bStale=false;P3DS_BottomReused=0;return;}

  if(bSkipCfg<0){bSkipCfg=1;/* run 47: default 1 again (+ bottom_refresh) */FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"bottom_skip=%d",&x)==1)bSkipCfg=x!=0;fclose(f);}PSPNativeMemLog("[2D] bottom_skip=%d",bSkipCfg);}
  unsigned pw=bPower;if(pw!=bLastPower){bottomCache.valid=false;bLastPower=pw;}
  /* run 47: bottom_skip back to 1 (run 46 data: skips off cost +8.1 ms composite in stereo field, 27.4 -> 21.9 fps)
     plus bottom_refresh=N (default 8): after any scene change (ApplicationManager_New: battle start, menus, Poketch
     apps...), a VRAMCNT bank remap or an engine-A DISPCNT change, engine B is fully recomposed for N frames without
     the compare, so a transition can never leave a stale bottom image (the run 45 battle-start glitch). */
  {static int refCfg=-1;static u32 lastSig[4];static int force;
   if(refCfg<0){refCfg=8;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"bottom_refresh=%d",&x)==1)refCfg=x<0?0:x;fclose(f);}PSPNativeMemLog("[2D] bottom_refresh=%d",refCfg);}
   extern volatile unsigned P3DS_SceneGen;u32 sig[4]={P3DS_SceneGen,reg32(0x240),reg32(0x244)&0xFFFF,reg32(0)};
   if(memcmp(sig,lastSig,sizeof sig)){memcpy(lastSig,sig,sizeof sig);force=refCfg;}
   if(force>0){force--;bottomCache.valid=false;}}
  // SoulSilver stamps all VRAM pages at VBlank. During the field Save prompt,
  // BottomSame can compare the actual Engine B inputs instead of treating
  // that stamp as a change. Keep the conservative invalidation elsewhere.
  const bool savePrompt = P3DS_InField && P3DS_StartPrompt && (pw & 0x8000) &&
                          !P3DS_MenuOpen && !P3DS_PanelPartyWant && !P3DS_PanelPartyOn;
  static bool wasSavePrompt;
  if(savePrompt != wasSavePrompt) {
   bottomCache.valid = false;
   wasSavePrompt = savePrompt;
  }
  {static unsigned lastG;unsigned g=0,gp=0;
   for(unsigned pg=0x40;pg<0x80;pg++)if(PSPNativeVramPageGen[pg]>g){g=PSPNativeVramPageGen[pg];gp=pg;}
   for(unsigned pg=0x98;pg<0xA4;pg++)if(PSPNativeVramPageGen[pg]>g){g=PSPNativeVramPageGen[pg];gp=pg;}
   if(g!=lastG){lastG=g;if(!savePrompt&&bottomCache.valid&&P3DS_CfgInt("bottom_vramgen",1)){bottomCache.valid=false;static int n;if(n<20){n++;PSPNativeMemLog("[2D] bottom changed by VRAM page %u",gp);}}}}
  if(bHideB){bottomCache.valid=false;P3DS_BottomReused=0;pfBHid++;bStale=true;return;}
  const int titleMode=P3DS_InTitle?titleCleanCfg():0;
  if(titleMode<2&&bSkipCfg&&!bGpuVerify&&BottomSame(pw)){bSkips++;P3DS_BottomReused=1;return;}
  P3DS_BottomReused=0;bDraws++;bStale=false;
  if(titleMode==2){u64 t=svcGetSystemTick();int ok=titleCompose(true);if(ok){titleCopyWide();static u64 ticks;static unsigned n;ticks+=svcGetSystemTick()-t;if(++n%300==0)PSPNativeMemLog("[TITLE2D] wide frames=%u mean=%u us",n,(unsigned)(ticks/268/n));return;}}
  const u64 scanStart = svcGetSystemTick();
  rendererB.SetFramebuffer(raw[0],raw[1]);
  for(unsigned line=0;line<192;line++){engineB.CheckWindows(line);rendererB.DrawSprites(line,&engineB);rendererB.DrawScanline(line,&engineB);}
  bScanTicks += svcGetSystemTick() - scanStart;
  ++bScanFrames;
  if(bSkipCfg)SaveBottom(pw);}
/* run 34: comp_excl=1 (opt-in, default 0). DS worker threads / the alarm thread run on the game core; with a higher
   priority (or while the main thread blocked on engine B) they could write VRAM/OAM/palettes/BG regs MID-COMPOSITE,
   so rows composited before the write showed the old state ("top rows late", intro video + logo orbs). During the
   composite the main thread is raised above every other core-0 thread and spins (no block) on engine B. */
static int compExcl(){static int v=-1;if(v<0){v=0;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"comp_excl=%d",&x)==1)v=x!=0;fclose(f);}PSPNativeMemLog("[2D] comp_excl=%d",v);}return v;}
static unsigned midWrites,midChecks;
static u32 stateHash(){u32 h=2166136261u;const u32*p=(const u32*)s_HW_OAM;for(int i=0;i<256;i++)h=(h^p[i])*16777619u;p=(const u32*)s_HW_DB_OAM;for(int i=0;i<256;i++)h=(h^p[i])*16777619u;
 p=(const u32*)s_HW_BG_PLTT;for(int i=0;i<128;i++)h=(h^p[i])*16777619u;p=(const u32*)s_HW_DB_BG_PLTT;for(int i=0;i<128;i++)h=(h^p[i])*16777619u;
 p=(const u32*)PSPNative_GfxRegisters;for(int i=0;i<0x58/4;i++)h=(h^p[i]^p[i+0x400])*16777619u;return h;}
/* fps60_hold: 1 when sprites, 2D scroll, or background tiles changed since the previous call.
   The first call counts as movement so a just-enabled hold does not repeat a stale frame. */
extern "C" int P3DS_FpsSceneMoved(void) {
 u32 h=2166136261u;const u32*oam=(const u32*)s_HW_OAM;for(int i=0;i<256;i++)h=(h^oam[i])*16777619u;
 const u32*r=(const u32*)PSPNative_GfxRegisters;for(int i=0x10/4;i<0x40/4;i++)h=(h^r[i])*16777619u;
 const u8*bg=GPU::VRAMFlat_ABG;if(bg)for(unsigned i=0;i<256*1024;i+=128)h=(h^bg[i])*16777619u;
 static u32 prev;static int have;int moved=!have||h!=prev;have=1;prev=h;return moved;
}
/* run 59 gpu_compose stage 0: ANALYZER ONLY (gpu_compose=1). Before writing a PICA 2D compositor, measure which DS 2D
   features real frames use, per engine, so the GPU path can start with the common case and fall back to melonDS
   for the rest. Register/OAM reads only (no pixel work); logs one [GPUC] line per engine every 300 frames.
   gpu_compose=0 (default): not even called. Nothing is drawn differently in either setting. */
static int gcCfgV=-1;
extern "C" {extern int P3DS_DexNoWide,P3DS_DexOpen;extern unsigned long long WC_Prof[4];extern unsigned WC_ProfLines,WC_ProfSpr,WC_ProfComposes;int P3DS_GcBlendPredictOk(const u8*);int P3DS_GcWideRelease(void);void G3G_WideRetry(void);void G3G_DirectResolve(void);int G3G_DirectPending(void);}
extern "C" int P3DS_GpuComposeCfg(void){if(gcCfgV<0){gcCfgV=P3DS_CfgInt("gpu_compose",2);/* run 61: default 2 (hardware-confirmed by the user, run-60 parity 0 px) */if(gcCfgV<0||gcCfgV>3)gcCfgV=2;PSPNativeMemLog("[GPUC] gpu_compose=%d (1 = analyzer, 2 = GPU compositor on simple frames, 3 = verify)",gcCfgV);}return gcCfgV;}
extern "C" void P3DS_SetGpuComposeCfg(int v){gcCfgV=v;}
enum{GC_DM,GC_AFFBG,GC_WIN,GC_ALPHA,GC_BRIGHT,GC_MOSAIC,GC_BG8,GC_EXTPAL,GC_OBJAFF,GC_OBJSEMI,GC_OBJWIN,GC_OBJBMP,GC_CAP,GC_MBRIGHT,GC_N};
static const char*const gcName[GC_N]={"dispmode","affineBG","window","alpha","brighten/darken","mosaic","bg8bpp","extpal","objAffine","objSemi","objWin","objBmp","capture","masterBright"};
static void gcAnalyze(unsigned e,bool field){
 static unsigned frames[2],simple[2],cnt[2][GC_N],fieldF[2],fieldS[2];static u64 tk[2];
 const unsigned b=e?0x1000:0;const u32 dc=reg32(b);const unsigned mode=dc&7,dm=(dc>>16)&3;u32 f=0;const u64 t0=svcGetSystemTick();
 if(dm!=1)f|=1u<<GC_DM;
 for(int i=0;i<4;i++){if(!(dc&(0x100u<<i)))continue;const unsigned c=reg16(b+8+i*2);
  if(i==0&&!e&&(dc&8))continue; /* BG0 = 3D */
  const bool text=i<2||mode==0||(i==2&&mode==1);if(!text)f|=1u<<GC_AFFBG;
  if((c&0x40)&&(reg16(b+0x4C)&0xFF))f|=1u<<GC_MOSAIC;if(text&&(c&0x80)){f|=1u<<GC_BG8;if(dc&(1u<<30))f|=1u<<GC_EXTPAL;}}
 if(dc&0xE000)f|=1u<<GC_WIN;
 {const unsigned bl=reg16(b+0x50),eff=(bl>>6)&3;if(eff==1)f|=1u<<GC_ALPHA;else if(eff)f|=1u<<GC_BRIGHT;}
 if(reg16(b+0x6c)&0x1F&&(reg16(b+0x6c)>>14))f|=1u<<GC_MBRIGHT;
 if(!e&&(reg32(0x64)&0x80000000u))f|=1u<<GC_CAP;
 if(dc&0x1000){const u16*o=(const u16*)(e?s_HW_DB_OAM:s_HW_OAM);for(int i=0;i<128;i++){const u16 a0=o[i*4];if((a0&0x300)==0x200)continue;
   if(a0&0x100)f|=1u<<GC_OBJAFF;const unsigned m=(a0>>10)&3;if(m==1)f|=1u<<GC_OBJSEMI;else if(m==2)f|=1u<<GC_OBJWIN;else if(m==3)f|=1u<<GC_OBJBMP;if((a0&0x1000)&&(reg16(b+0x4C)>>8))f|=1u<<GC_MOSAIC;}}
 frames[e]++;if(field)fieldF[e]++;
 /* "simple" = what a first GPU path can do: text BGs (4/8bpp, ext pal), plain/affine-free OBJs, 3D BG0, brightness
    fades (BLDY + master brightness) - no windows, no alpha blend, no mosaic, no affine/bitmap, normal display. */
 const u32 hard=f&~((1u<<GC_BRIGHT)|(1u<<GC_MBRIGHT)|(1u<<GC_BG8)|(1u<<GC_EXTPAL));
 if(!hard){simple[e]++;if(field)fieldS[e]++;}
 for(int k=0;k<GC_N;k++)if(f&(1u<<k))cnt[e][k]++;
 tk[e]+=svcGetSystemTick()-t0;
 if(frames[e]>=300){char buf[400];int n=snprintf(buf,sizeof buf,"[GPUC] eng%c %u frames: simple %u%% (field %u/%u simple) | uses:",e?'B':'A',frames[e],simple[e]*100/frames[e],fieldS[e],fieldF[e]);
  for(int k=0;k<GC_N;k++)if(cnt[e][k])n+=snprintf(buf+n,sizeof buf-n," %s %u%%",gcName[k],cnt[e][k]*100/frames[e]);
  snprintf(buf+n,sizeof buf-n," | cost %.1f us/frame",(double)tk[e]/frames[e]/268.123);PSPNativeMemLog("%s",buf);
  frames[e]=simple[e]=fieldF[e]=fieldS[e]=0;tk[e]=0;memset(cnt[e],0,sizeof cnt[e]);}
}
/* run 60 gpu_compose=2/3: GPU compositor (platform/gpucomp.c) for engine A on simple frames; 3 = verify (melonDS output is
   shown, the GPU result is composed too and compared pixel by pixel, [GPUC] verify log, first mismatch dumped). */
#include "widecomp.h"
extern "C" {int P3DS_GcComposeWide(const WcPlan*,const u8*,const u8*,const u8*,const u16*,const u16*,const u16*,const u32*,u32*,u32*);void P3DS_GcFrame(void);extern int P3DS_GcNoLag;void P3DS_SetFramePacing(int);int P3DS_GcBlendPredictOk(const u8*);int P3DS_GcWideRelease(void);}
extern "C" {void G3G_DirectResolve(void);void G3G_DirectResolveWhy(int);void G3G_DirectStrips(void);int G3G_DirectPending(void);int G3G_DirectCfg(void);void G3G_SetDirect(int);int P3DS_GcEligible(const u8*,const u16*);int P3DS_GcComposeA(const u8*,const u8*,const u8*,const u16*,const u16*,const u16*,const u32*,u32*,void* = nullptr,void** = nullptr,unsigned* = nullptr);
 extern unsigned P3DS_GcBlendOn,P3DS_GcFxFrames,P3DS_GcBlendFrames,P3DS_GcBlendRej;extern unsigned P3DS_GcWalkUs,P3DS_GcConvUs,P3DS_GcOutUs,P3DS_GcQuads,P3DS_GcDecoded,P3DS_GcResets,P3DS_GcCpuUs,P3DS_GcGpuUs;}
extern "C" void P3DS_GcRejLog(void);
extern "C" {extern unsigned P3DS_GcB3Frames,P3DS_GcBeginUs,P3DS_GcEndUs,P3DS_GcSyncUs,P3DS_GcMbFrames,P3DS_GpBeginUs,P3DS_GpTearUs,P3DS_GpFenceIdleUs,P3DS_GpEndUs,P3DS_GpUpUs,P3DS_GpDrawUs,P3DS_GpStripUs,P3DS_FeN;extern unsigned long long P3DS_FeTicks;extern volatile unsigned long long P3DS_VblIdleTicks;int P3DS_FpsTweaks(void);int P3DS_FpsTweakRow(void);void P3DS_GcEye(int,int);void P3DS_GcFinish(void);void P3DS_SetFpsTweaks(int);}
/* [PERF4] (2026-10-27 fps thread) per-second sums of the frame's stages, in ticks (pfT) or us (pfU) */
enum{PF_IDLE,PF_GCL,PF_GCR,PF_CPUA,PF_BWAIT,PF_C1WAIT,PF_EARLYBWAIT,PF_OVL,PF_BMENU,PF_PRESENT,PF_WPREP,PF_DRAWS,PF_DSTRIP,PF_N};static u64 pfT[PF_N];
enum{PU_LB,PU_LE,PU_LS,PU_RB,PU_RE,PU_RS,PU_PB,PU_PT,PU_PE,PU_UP,PU_DR,PU_ST,PU_N};static u64 pfU[PU_N];static unsigned pfR,pfBlRep;
static unsigned gcFrames,gcUsed,gcInel,gcFail,gcVer,gcVerOk,gcVerBad;static u64 gcW,gcC,gcO,gcBadPx,gcCpu,gcGpu,gcDec,gcQ;static unsigned gcReason[16];
static u64 gcBadBig;static u32*gcScratch;static unsigned gcMenuF,gcMenuOK,gcMenuW64[4],gcMenuOn[4];extern "C" int P3DS_InField;
static bool gcCompose(u32*out,void* mid3d=nullptr,void** midOut=nullptr,unsigned* midUs=nullptr){const int r=P3DS_GcComposeA((const u8*)PSPNative_GfxRegisters,GPU::VRAMFlat_ABG,GPU::VRAMFlat_AOBJ,(const u16*)GPU::Palette,(const u16*)(GPU::Palette+512),(const u16*)GPU::OAM,GPU3D::NativeFrame,out,mid3d,midOut,midUs);
 if(r){gcW+=P3DS_GcWalkUs;gcC+=P3DS_GcConvUs;gcO+=P3DS_GcOutUs;gcCpu+=P3DS_GcCpuUs;gcGpu+=P3DS_GcGpuUs;gcDec+=P3DS_GcDecoded;gcQ+=P3DS_GcQuads;}else gcFail++;return r!=0;}
extern "C" {extern unsigned P3DS_GcDeferN,P3DS_GcResolveN;}
static void gcLog(){if(gcFrames<300)return;const unsigned u=(gcUsed+gcVer)?(gcUsed+gcVer):1;
 PSPNativeMemLog("[GPUC] direct screen: last %u frames: %u eye pictures went to the screen straight from the GPU, %u were fetched back for the CPU (overlay, hold, capture)",gcFrames,P3DS_GcDeferN>P3DS_GcResolveN?P3DS_GcDeferN-P3DS_GcResolveN:0,P3DS_GcResolveN);P3DS_GcDeferN=P3DS_GcResolveN=0;
 PSPNativeMemLog("[GPUC] %u frames: GPU-composed %u, ineligible %u (reasons dm %u fb %u win %u 3dofs %u affbg %u bgmos %u extpal %u blend %u mbright %u cap %u objaff %u objmode %u objmos %u objext %u), failed %u | per composed frame: cpu %u us (walk %u, 3D tex %u, out %u), gpu+wait %u us, decoded %u tiles, %u quads, cache resets %u",
  gcFrames,gcUsed,gcInel,gcReason[0],gcReason[1],gcReason[2],gcReason[3],gcReason[4],gcReason[5],gcReason[6],gcReason[7],gcReason[8],gcReason[9],gcReason[10],gcReason[11],gcReason[12],gcReason[13],gcFail,(unsigned)(gcCpu/u),(unsigned)(gcW/u),(unsigned)(gcC/u),(unsigned)(gcO/u),(unsigned)(gcGpu/u),(unsigned)(gcDec/u),(unsigned)(gcQ/u),P3DS_GcResets);
 if(gcMenuF)PSPNativeMemLog("[GPUC] non-field frames %u, GPU-eligible %u | text BG on/64-wide: BG0 %u/%u BG1 %u/%u BG2 %u/%u BG3 %u/%u",gcMenuF,gcMenuOK,gcMenuOn[0],gcMenuW64[0],gcMenuOn[1],gcMenuW64[1],gcMenuOn[2],gcMenuW64[2],gcMenuOn[3],gcMenuW64[3]);
 P3DS_GcRejLog();
 if(P3DS_GcBlendOn)PSPNativeMemLog("[GPUC] gpu_blend: GPU-composed frames with alpha blend %u, with brightness fade %u, rejected (blend not exactly representable) %u; verify px off by >4: %llu",P3DS_GcBlendFrames,P3DS_GcFxFrames,P3DS_GcBlendRej,(unsigned long long)gcBadBig);
 if(gcVer)PSPNativeMemLog("[GPUC] verify: %u frames compared, %u identical, %u differ, %llu px differ in total",gcVer,gcVerOk,gcVerBad,(unsigned long long)gcBadPx);
 gcMenuF=gcMenuOK=0;memset(gcMenuW64,0,sizeof gcMenuW64);memset(gcMenuOn,0,sizeof gcMenuOn);gcFrames=gcUsed=gcInel=gcFail=gcVer=gcVerOk=gcVerBad=0;gcBadBig=0;P3DS_GcBlendFrames=P3DS_GcFxFrames=P3DS_GcBlendRej=0;gcW=gcC=gcO=gcBadPx=gcCpu=gcGpu=gcDec=gcQ=0;memset(gcReason,0,sizeof gcReason);}
/* stage 2 groundwork (log only): outside the field, how many top-screen frames are GPU-eligible and how wide their
   text BG maps are (64-tile maps hold real content beyond the 256-px window -> candidates for native 400-px menus) */
static int gcEligible(){const int r=P3DS_GcEligible((const u8*)PSPNative_GfxRegisters,(const u16*)GPU::OAM);if(r){gcInel++;for(int k=0;k<16;k++)if(r&(1<<k)){gcReason[k]++;break;}}
 if(!P3DS_InField){gcMenuF++;if(!r){gcMenuOK++;const u32 dc=reg32(0);for(int i=0;i<4;i++)if(dc&(0x100u<<i)&&!(i==0&&(dc&8))){gcMenuOn[i]++;if(reg16(8+i*2)&0x4000)gcMenuW64[i]++;}}}
 return r==0;}
static void bindRegisters(GPU2D::Unit&u,unsigned base){
 u.Write32(0,reg32(base));
 for(unsigned off=8;off<=0x54;off+=2){
  if(off==0x28||off==0x2c||off==0x38||off==0x3c){u.Write32(off,reg32(base+off));off+=2;}
  else u.Write16(off,reg16(base+off));
 }
 u.Write16(0x6c,reg16(base+0x6c));
}
static void syncExtPalette(u8 *dst, const u8 *src, size_t size, bool &wasMapped) {
 if (src) memcpy(dst, src, size);
 else if (wasMapped) memset(dst, 0, size);
 wasMapped = src != nullptr;
}
static void mapExtPalettes() {
 // These flat palettes start zero-filled. Only an unmapped span that
 // previously had a source needs clearing.
 static bool bgALowMapped, bgAHighMapped, objAMapped, bgBMapped, objBMapped;
 constexpr size_t bgHalfSize = sizeof(GPU::VRAMFlat_ABGExtPal) / 2;
 const u8 *bgA[2] = {};
 const u8 *objA = nullptr;
 if ((*(volatile u8 *)(PSPNative_GfxRegisters + 0x244) & 0x87) == 0x84) {
  bgA[0] = s_HW_LCDC_VRAM + 0x80000;
  bgA[1] = bgA[0] + bgHalfSize;
 }
 // Bank F overlays E, and G overlays both when targeting the same span.
 for (unsigned i = 0; i < 2; ++i) {
  const unsigned c = *(volatile u8 *)(PSPNative_GfxRegisters + 0x245 + i);
  const u8 *src = s_HW_LCDC_VRAM + 0x90000 + i * 0x4000;
  if ((c & 0x87) == 0x84) bgA[(c >> 3) & 1] = src;
  if ((c & 0x87) == 0x85) objA = src;
 }
 const unsigned h = *(volatile u8 *)(PSPNative_GfxRegisters + 0x248);
 const unsigned i = *(volatile u8 *)(PSPNative_GfxRegisters + 0x249);
 const u8 *bgB = (h & 0x87) == 0x82 ? s_HW_LCDC_VRAM + 0x98000 : nullptr;
 const u8 *objB = (i & 0x87) == 0x83 ? s_HW_LCDC_VRAM + 0xA0000 : nullptr;
 syncExtPalette(GPU::VRAMFlat_ABGExtPal, bgA[0], bgHalfSize, bgALowMapped);
 syncExtPalette(GPU::VRAMFlat_ABGExtPal + bgHalfSize, bgA[1], bgHalfSize, bgAHighMapped);
 syncExtPalette(GPU::VRAMFlat_AOBJExtPal, objA, sizeof(GPU::VRAMFlat_AOBJExtPal), objAMapped);
 syncExtPalette(GPU::VRAMFlat_BBGExtPal, bgB, sizeof(GPU::VRAMFlat_BBGExtPal), bgBMapped);
 syncExtPalette(GPU::VRAMFlat_BOBJExtPal, objB, sizeof(GPU::VRAMFlat_BOBJExtPal), objBMapped);
}
static void mapMemory(){
 memcpy(GPU::Palette,s_HW_BG_PLTT,512);memcpy(GPU::Palette+512,s_HW_OBJ_PLTT,512);
 memcpy(GPU::Palette+1024,s_HW_DB_BG_PLTT,512);memcpy(GPU::Palette+1536,s_HW_DB_OBJ_PLTT,512);
 memcpy(GPU::OAM,s_HW_OAM,1024);memcpy(GPU::OAM+1024,s_HW_DB_OAM,1024);
 GPU::VRAMMap_LCDC=0;for(unsigned i=0;i<4;i++){GPU::VRAM[i]=s_HW_LCDC_VRAM+i*0x20000;if((*(volatile u8*)(PSPNative_GfxRegisters+0x240+i)&0x87)==0x80)GPU::VRAMMap_LCDC|=1u<<i;}
 mapExtPalettes();
}
#include "../native-render-opt/top_cache.h"
/* ---- platinum3ds present path: melonDS software 2D -> 3DS framebuffers (no GPU yet). ---- */
extern "C" int P3DS_ConsoleOnBottom(void);
extern "C" void P3DS_GameFramePresented(void);
extern "C" void PSPNativeRenderSetInput(unsigned keys,int touchMode,int touchDown,int tx,int ty){inputKeys=keys;mode=touchMode;down=touchDown;x=tx;y=ty;}
extern "C" unsigned PSPNativeRenderLastDrawMask(){return lastDraw;}
extern "C" unsigned PSPNativeRenderFrameCount(){return frames;}
#include <3ds.h>
static volatile u64 shinyAt;
/* 2026-11-04 direct screen (gc_direct, Display tab, default ON; SPEEDUPS.md stage 2): a GPU-composed mono field frame stays on the
   GPU (gpucomp.c P3DS_GcDeferOut) and the screen scaler samples it, so the readback, the CPU colour pass and the re-upload are
   skipped. Every CPU reader or writer of that picture calls topTouch() first, which fetches the pixels exactly as before; the
   field hold for the party panel is fetched only when the panel needs it (holdSync). */
extern "C" {extern int P3DS_GcDeferOut;u32*P3DS_GcResolveOut(void);int P3DS_GcResolveTo(u32*);void P3DS_GcDrop(void);void*P3DS_GcPendTex(const u32*);extern unsigned P3DS_GcDeferN,P3DS_GcResolveN;extern unsigned char P3DS_GpTextMask[8][64];extern int P3DS_GpTextOn;}
static u32*gHoldBuf;static int gHoldLazy,gDfDec;static unsigned gTouchN;
static void topTouch(){u32*o=P3DS_GcResolveOut();if(o){gTouchN++;if(gHoldLazy&&gHoldBuf){memcpy(gHoldBuf,o,256*192*4);gHoldLazy=0;}}}
static void holdSync(){if(gHoldLazy){if(gHoldBuf)P3DS_GcResolveTo(gHoldBuf);gHoldLazy=0;}}
extern "C" void P3DS_TopTouch(void){topTouch();}
static int gcDirectV=-1;static int gcDirectCfg(){if(gcDirectV<0){gcDirectV=P3DS_CfgInt("gc_direct",1)!=0;PSPNativeMemLog("[GPUC] gc_direct=%d (GPU-composed field frames go to the screen without a CPU round trip)",gcDirectV);}return gcDirectV;}
static void drawShinyFlash(u32*img,u64 now){/* 0.9 s white pulses + twinkling sparkles where the wild foe stands */
 if(!shinyAt)return;const unsigned ms=(unsigned)((now-shinyAt)/CPU_TICKS_PER_MSEC);if(ms>900)return;topTouch();
 const unsigned ph=ms%300;const unsigned a=ph<150?ph*6/150:(300-ph)*6/150;/* 0..6 of 16 */
 if(a)for(int i=0;i<256*192;i++){u32 d=img[i];img[i]=(d&0xFF000000u)|((((d&255)*(16-a))+255*a)>>4)|(((((d>>8)&255)*(16-a)+255*a)>>4)<<8)|(((((d>>16)&255)*(16-a)+255*a)>>4)<<16);}
 static const signed char sx[7]={-22,14,26,-8,4,-28,20},sy[7]={-18,-26,6,10,-34,0,22};
 for(int k=0;k<7;k++){const int on=((int)ms/90+k)%3;if(!on)continue;const int cx=188+sx[k],cy=62+sy[k],r=on==1?2:4;
  for(int d=-r;d<=r;d++){const int pts[2][2]={{cx+d,cy},{cx,cy+d}};for(auto&p:pts)if(p[0]>=0&&p[0]<256&&p[1]>=0&&p[1]<192)img[p[1]*256+p[0]]=(img[p[1]*256+p[0]]&0xFF000000u)|0x00A0F8FFu;}}}
static void huntTick(){static bool armed=true;
 /* Wild battles only: gift/starter path already counted via P3DS_HuntGift (0292). Without InBattle, leaving the
    field for ChooseStarter re-fired HuntEncounter on whatever CatchInfo the last HuntGift wrote (Totodile 158 = 2x). */
 P3DS_HuntFrame(P3DS_MenuOpen);
 if(P3DS_InField){if(!armed){armed=true;P3DS_CatchInfo[0]=0;}return;}
 if(!armed||!P3DS_CatchInfo[0]||!P3DS_InBattle)return;armed=false;if(!P3DS_HuntCounting())return;int sp=P3DS_CatchInfo[1];if(sp<=0||sp>=494)return;
 const bool shiny=P3DS_CatchInfo[18]!=0;P3DS_HuntEncounter(sp,shiny);
 if(shiny)P3DS_AchUnlock(ACH_SHINY);/* achievements: first shiny encounter */
 if(shiny&&P3DS_HuntOn()){shinyAt=svcGetSystemTick();P3DS_HuntChime();if(!P3DS_CfgInt("catch_overlay",0))P3DS_Toast("SHINY POKEMON!");}}
extern "C" void G3SIM_DrawCleanUp();
extern "C" void G3SIM_FlushDeferred();
extern "C" void P3DS_RomCachePump(void);static inline void P3DS_RomCachePumpC(){P3DS_RomCachePump();}
extern "C" void P3DS_G3BeginFrame(void);
extern "C" void G3SIM_FlushArray();
static void blit(u8 *fb, unsigned fbw, const u32 *src) {
 /* 3DS framebuffers are 240-tall columns, rotated; centre the 256x192 DS screen. */
 unsigned ox = (fbw - 256) / 2, oy = (240 - 192) / 2;
 for (unsigned x = 0; x < 256; x++) {
  u8 *col = fb + ((ox + x) * 240 + (239 - oy)) * 3;
  for (unsigned y = 0; y < 192; y++) { u32 c = src[y * 256 + x]; u8 *p = col - y * 3; p[0] = (u8)(c >> 16); p[1] = (u8)(c >> 8); p[2] = (u8)c; }
 }
}
/* Scaled bilinear blit: output W x H at (ox,oy), 8-bit weights from per-axis tables (cheap). */
static void blitScaled(u8 *fb, unsigned fbw, const u32 *src, unsigned W, unsigned H) {
 unsigned ox = (fbw - W) / 2, oy = (240 - H) / 2;
 static unsigned short yi[240]; static u8 yw[240]; static unsigned lastH;
 if (lastH != H) { lastH = H; for (unsigned y = 0; y < H; y++) { int f = (int)(((y * 2 + 1) * 192 * 128) / H) - 128; if (f < 0) f = 0; unsigned i = f >> 8; if (i >= 191) { i = 190; f = 191 << 8; } yi[y] = i; yw[y] = f - (i << 8) > 255 ? 255 : f - (i << 8); } }
 for (unsigned x = 0; x < W; x++) {
  int fx = (int)(((x * 2 + 1) * 256 * 128) / W) - 128; if (fx < 0) fx = 0; unsigned xi = fx >> 8; unsigned wx = fx & 255; if (xi >= 255) { xi = 254; wx = 255; }
  u8 *col = fb + ((ox + x) * 240 + (239 - oy)) * 3;
  for (unsigned y = 0; y < H; y++) {
   const u32 *r0 = src + yi[y] * 256 + xi, *r1 = r0 + 256; unsigned wy = yw[y];
   u32 a = r0[0], b = r0[1], c = r1[0], d = r1[1]; u8 *p = col - y * 3;
   for (int k = 0; k < 3; k++) { unsigned sh = (2 - k) * 8;
    unsigned t = (((a >> sh) & 255) * (256 - wx) + ((b >> sh) & 255) * wx) >> 8, u = (((c >> sh) & 255) * (256 - wx) + ((d >> sh) & 255) * wx) >> 8;
    p[k] = (u8)((t * (256 - wy) + u * wy) >> 8); }
  }
 }
}
extern "C" int P3DS_ScaleMode(void);
extern "C" int P3DS_GpuInit(void);
extern "C" void P3DS_GpuPresent(const u32*,const u32*,int,int,const u32*);
/* Stereo (slider): the 3D layer (NativeFrame) is shifted per eye before compositing engine A, so the 3D scene
   sits behind the screen while 2D BG/OBJ stay at screen depth. Left eye: 3D content moved left by d. */
/* Debug: hold L+R and press X to dump the 3D layer and both composited screens as BMPs to sdmc:/3ds/platinum3ds/. */
/* run 61 debug: stereo_fake=N (1..100) pretends the 3D slider is at N% (emulator tests of the stereo paths; Azahar
   reports 0). Default 0 = the real slider. */
static float p3dsSlider(){static int v=-1;if(v<0){v=P3DS_CfgInt("stereo_fake",0);if(v<0||v>100)v=0;if(v)PSPNativeMemLog("[STEREO] stereo_fake=%d",v);}static int tg=-1;if(tg<0)tg=P3DS_CfgInt("stereo_fake_toggle",0);if(v&&tg>0&&((unsigned)(svcGetSystemTick()/CPU_TICKS_PER_MSEC/1000)/tg)%2)return 0.0f; /* run 109 test only: fake slider flips every tg s */
 return v?v/100.0f:osGet3DSliderState();}
static void dumpBmp(const char*path,const u32*px,bool six){FILE*f=fopen(path,"wb");if(!f)return;
 unsigned char h[54]={'B','M'};unsigned sz=54+256*192*3;memcpy(h+2,&sz,4);h[10]=54;h[14]=40;int w=256,hh=192;memcpy(h+18,&w,4);memcpy(h+22,&hh,4);h[26]=1;h[28]=24;fwrite(h,1,54,f);
 for(int y=191;y>=0;y--)for(int x=0;x<256;x++){u32 c=px[y*256+x];unsigned r=c&(six?63:255),g=(c>>8)&(six?63:255),b=(c>>16)&(six?63:255);if(six){r<<=2;g<<=2;b<<=2;}unsigned char o[3]={(unsigned char)b,(unsigned char)g,(unsigned char)r};fwrite(o,1,3,f);}
 fclose(f);}
/* ---- FPS overlay (L+R+START toggles; on by default) + 1 s summary line ---- */
static const unsigned short kFont[16]={0x7B6F,0x2C97,0x73E7,0x73CF,0x5BC9,0x79CF,0x79EF,0x7249,0x7BEF,0x7BCF,0,0x01C0,0x0400,0x0000,0,0};/* 0-9, '-'(11) '.'(12) ' '(13): 3x5 bitmaps, bit14 = top-left */
static void drawText(u32*img,int x,int y,const char*t){
 for(;*t;t++,x+=4){int g=*t>='0'&&*t<='9'?*t-'0':*t=='.'?12:*t=='-'?11:13;
  for(int yy=-1;yy<6;yy++)for(int xx=-1;xx<4;xx++){int px=x+xx,py=y+yy;if(px<0||px>255||py<0||py>191)continue;
   bool on=yy>=0&&yy<5&&xx>=0&&xx<3&&((kFont[g]>>(14-(yy*3+xx)))&1);img[py*256+px]=on?0x00FFFFFFu:0x00000000u;}}}
/* ---- run 44 save indicator (save_anim=1 default, 0 off): SD writes are instant, so the port shows its own
   "SAVING" box with a spinner in the top screen's bottom-right corner for at least save_minms (default 1500) after
   the first backup write (and 300 ms after the last), then "SAVED!" for 0.8 s. Pure overlay: the game's save
   timing is not changed. Original 3x5 glyphs drawn at 2x. */
static volatile u64 saveFirst,saveLast;static int saveAnim=-1,saveMinMs=1500;
static volatile int saveSeq,saveYN=-1;static int saveSeqCfg=2;
/* run 48 save_seq=2 (default): while the save sequence is open and the game's yes/no menu is up, the port draws its
   own YES/NO box over the game's (DS x 192..255, y 96..143), showing the game's own cursor (Menu_GetCursorPos read
   each frame in ScriptContext_WaitForYesNoResult). Input and the menu logic are the game's. 1 = run 47 behaviour. */
static volatile u64 seqAct;extern "C" int P3DS_InField;
extern "C" void P3DS_SaveYesNo(int cur){saveYN=cur;seqAct=svcGetSystemTick();}
static void saveCfg(){
 if(saveAnim<0){saveAnim=2;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f)){if(sscanf(l,"save_anim=%d",&x)==1)saveAnim=x<0?0:x>2?2:x;if(sscanf(l,"save_minms=%d",&x)==1&&x>=0&&x<=10000)saveMinMs=x;if(sscanf(l,"save_seq=%d",&x)==1)saveSeqCfg=x<0?0:x>2?2:x;}fclose(f);}PSPNativeMemLog("[SAVE] save_anim=%d save_minms=%d save_seq=%d",saveAnim,saveMinMs,saveSeqCfg);}}
/* run 47 save_seq=1 (default, with save_anim=2): the port box takes over the WHOLE field save sequence: from the game's
   OpenSaveInfo (script "Would you like to save?") until CloseSaveInfo, the message window area shows only our box:
   "SAVE?" while the yes/no menu is up (the menu itself stays visible), "SAVING" + spinner during the write (min
   save_minms), then "SAVED!" until the game closes the save info. Answering NO closes it at once; a save error drops
   the box so the game's error text is visible. Hooks only mark state; the game's save code is untouched. */
static volatile int seqRaw;static volatile u64 seqRawT; /* run 51: raw save-box state for zone_prefetch (independent of save_seq/save_anim) */
extern "C" int P3DS_SaveActive(void){u64 now=svcGetSystemTick();const u64 ms=CPU_TICKS_PER_MSEC;
 return (seqRaw&&now-seqRawT<(u64)(60000*ms))||(saveLast&&now-saveLast<(u64)(10000*ms));}
extern "C" void P3DS_SaveSeq(int on){seqRaw=on==1;seqRawT=svcGetSystemTick();saveCfg();if(on==1)saveFirst=0;saveYN=-1;seqAct=svcGetSystemTick();saveSeq=saveSeqCfg&&saveAnim==2?on:0;PSPNativeMemLog("[SAVE] seq %s",on==1?"open":on==0?"close":"error");}
extern "C" void P3DS_SaveWriteNotify(unsigned len){(void)len;u64 now=svcGetSystemTick();saveCfg();
 if(!saveFirst||now-saveLast>(u64)(3000*CPU_TICKS_PER_MSEC))saveFirst=now;saveLast=now;}
static const struct{char c;unsigned short b;}kLet[]={{'S',0x79CF},{'A',0x2BED},{'V',0x5B6A},{'I',0x7497},{'N',0x6B6D},{'G',0x396B},{'E',0x79A7},{'D',0x6B6E},{'!',0x2482},{'?',0x7282},{'H',0x5BED},{'R',0x6BAD},{'P',0x6BA4},{'O',0x7B6F},{'F',0x79A4},{'W',0x5B7D},{'L',0x4927},{'C',0x3923},{'T',0x7492},{'X',0x5AAD},{'M',0x5FED},{'U',0x5B6F},{'K',0x5BAD},{'Y',0x5A92},{'/',0x12A4},{':',0x0410},{'-',0x01C0},{'.',0x0002},{'B',0x6BAE},{'>',0x4454},{'Z',0x72A7},{'<',0x1511},{'(',0x2922},{')',0x224A},{'+',0x05D0},{'J',0x126A},{'Q',0x2B73},{',',0x0014},{'\'',0x2400}};
static void drawBig(u32*img,int x,int y,const char*t,u32 col){for(;*t;t++,x+=8){unsigned short b=0;for(auto&k:kLet)if(k.c==*t)b=k.b;if(*t>='0'&&*t<='9')b=kFont[*t-'0'];
  for(int yy=0;yy<10;yy++)for(int xx=0;xx<6;xx++){if(!((b>>(14-((yy/2)*3+xx/2)))&1))continue;int px=x+xx,py=y+yy;if(px>=0&&px<256&&py>=0&&py<192)img[py*256+px]=col;}}}
static void drawTiny(u32*img,int x,int y,const char*t,u32 col){for(;*t;t++,x+=4){unsigned short b=0;for(auto&k:kLet)if(k.c==*t)b=k.b;if(*t>='0'&&*t<='9')b=kFont[*t-'0'];
  for(int r=0;r<5;r++)for(int c=0;c<3;c++){if(!((b>>(14-(r*3+c)))&1))continue;int px=x+c,py=y+r;if(px>=0&&px<256&&py>=0&&py<192)img[py*256+px]=col;}}}
/* run 64 (item 4): port UI in the game's own font + message-box frame (both read from the user's ROM at boot by
   p3ds_gamefont.c); ui_gamefont=0 or a missing ROM file -> the old port font. */
extern "C" int P3DS_GameFontReady(void);extern "C" int P3DS_GameFontWidth(int,const char*);
extern "C" int P3DS_GameFontDraw(u32*,int,int,int,int,int,const char*,u32,u32,int,int);
extern "C" int P3DS_GameBoxReady(void);extern "C" void P3DS_GameBoxDraw(u32*,int,int,int,int,int,int,int);
static int uiGF=-1;static bool useGF(){if(uiGF<0){uiGF=P3DS_CfgInt("ui_gamefont",1)!=0;PSPNativeMemLog("[UI] ui_gamefont=%d (font %d box %d)",uiGF,P3DS_GameFontReady(),P3DS_GameBoxReady());}return uiGF&&P3DS_GameFontReady()&&P3DS_GameBoxReady();}
static const u32 kMsgFg=0x00505858u,kMsgSh=0x00B8B8A8u;
/* word-wrap t to pixel width maxw (game font); up to nl lines of 63 chars */
static int gfWrap(const char*t,int maxw,char out[][64],int nl){int n=0;while(*t&&n<nl){while(*t==' ')t++;int best=0,i=0;char buf[64];
  for(;t[i]&&i<63;i++){memcpy(buf,t,i+1);buf[i+1]=0;if(P3DS_GameFontWidth(1,buf)>maxw)break;if(t[i+1]==' '||!t[i+1])best=i+1;}
  if(!best)best=i?i:1;memcpy(out[n],t,best);out[n][best]=0;n++;t+=best;}return n;}
static void toUpper(char*d,const char*s,int n){int i=0;for(;s[i]&&i<n-1;i++)d[i]=(s[i]>='a'&&s[i]<='z')?s[i]-32:s[i];d[i]=0;}
/* run 48: hotkey toast. Any port hotkey that changes a setting shows a short box at the top of the top screen
   (e.g. "SHARP 3D ON", "WIDE VIEW OFF") for 1.5 s. */
static void prettyLab(char*d,const char*s,int n);
static char toastText[24];static volatile u64 toastAt;
extern "C" void P3DS_Toast(const char*t){snprintf(toastText,sizeof toastText,"%s",t);toastAt=svcGetSystemTick();PSPNativeMemLog("[TOAST] %s",t);}
/* run 76 dw_debris=1 (default 0): Distortion World void, 2D layer drawn over the field picture: 18 slow-drifting
   debris flecks at 3 parallax depths + 6 near-camera dust motes. Deterministic seed, capped counts, alpha-blended
   (dark violet / grey), per-eye shift by depth so it sits behind/in front in stereo. Visual only. */
extern "C" int P3DS_CurMap,P3DS_InField,P3DS_StereoShiftNow(void); extern "C" volatile int P3DS_MmMap,P3DS_SumOpen; extern "C" volatile int P3DS_MoveHint[24]; extern "C" int P3DS_InBattle;
static void drawTodTint(u32*img){/* run 76 tod_tint: soft evening/night tint on the DS picture outdoors (off by default) */
 extern int P3DS_MapOutdoor;if(!P3DS_InField||!P3DS_MapOutdoor)return;if(!P3DS_CfgInt("tod_tint",0))return;
 static const int ch[5]={0,7,12,18,22};int ct=P3DS_CfgInt("cheat_time",0);int h;if(ct>0&&ct<5)h=ch[ct];else{time_t t=time(nullptr);struct tm*lt=localtime(&t);h=lt?lt->tm_hour:12;}
 u32 col;int a;if(h>=17&&h<20){col=0x003060E0u;a=2;}else if(h>=20||h<4){col=0x00801810u;a=3;}else if(h<7){col=0x00A07090u;a=1;}else return;topTouch();
 const unsigned cr=(col&255)*a,cg=((col>>8)&255)*a,cb=((col>>16)&255)*a,ia=16-a;
 for(int i=0;i<256*192;i++){u32 d=img[i];img[i]=(d&0xFF000000u)|(((d&255)*ia+cr)>>4)|(((((d>>8)&255)*ia+cg)>>4)<<8)|(((((d>>16)&255)*ia+cb)>>4)<<16);}}
static void drawDwDebris(u32*img,int eye){
 if(!P3DS_InField||P3DS_CurMap<573||P3DS_CurMap>583)return;if(!P3DS_CfgInt("dw_debris",0))return;topTouch();
 static float px[24],py[24];static bool init;if(!init){init=true;u32 r=0x5EED1234u;for(int i=0;i<24;i++){r=r*1664525u+1013904223u;px[i]=(float)(r>>8&255);r=r*1664525u+1013904223u;py[i]=(float)((r>>8)%192);}}
 static unsigned lastF=~0u;if(eye==0&&lastF!=frames){lastF=frames;for(int i=0;i<24;i++){const int L=i<18?i%3:3;const float sp=0.05f+0.06f*L;px[i]+=sp*((i&1)?1.0f:-0.6f);py[i]-=sp*0.7f;
   if(px[i]<-4)px[i]+=264;if(px[i]>260)px[i]-=264;if(py[i]<-4)py[i]+=200;}}
 const int sd=P3DS_StereoShiftNow();
 for(int i=0;i<24;i++){const int L=i<18?i%3:3;const int sz=L==3?3:1+L/2;const int par=eye?-(sd*(L-1))/2:(sd*(L-1))/2;
  const int x0=(int)px[i]+par,y0=(int)py[i];const u32 col=L==3?0x00C0B0B8u:(i%5==0?0x00603850u:0x00403038u);const int a=L==3?5:8;/* alpha /16 */
  for(int y=y0;y<y0+sz;y++)for(int x=x0;x<x0+sz+(L==0);x++){if(x<0||x>255||y<0||y>191)continue;u32&d=img[y*256+x];
   unsigned r=((d&255)*(16-a)+(col&255)*a)/16,g=(((d>>8)&255)*(16-a)+((col>>8)&255)*a)/16,b=(((d>>16)&255)*(16-a)+((col>>16)&255)*a)/16;d=(d&0xFF000000u)|r|(g<<8)|(b<<16);}}}
/* achievements (achievements.c): 360-style toast on the top screen's bottom edge; the queue is advanced by eye 0 */
extern "C" int P3DS_GameFontWidth34(int,const char*);extern "C" int P3DS_GameFontDraw34(u32*,int,int,int,int,int,const char*,u32,int);
/* 2026-11-05 (Nathan: "make these look even more Xbox 360 like, they are still a bit blocky"): the toast is drawn at the final
   400x240 resolution into the native top overlay (gpuscale.c topOvDraw), not into the DS picture that gets stretched 1.25x:
   anti-aliased pill (4x4 coverage) with a glossy dark gradient and soft rim, round gold coin, game-font text 1:1. Both eyes show
   it at screen depth. Falls back to the DS-picture toast below when the overlay buffer or the game font is missing. */
extern "C" u32*P3DS_TopOvBufGet(void);extern "C" volatile int P3DS_TopOvOn,P3DS_TopOvDirty;extern "C" int P3DS_TopOvX,P3DS_TopOvY,P3DS_TopOvW,P3DS_TopOvH;
extern "C" int P3DS_GameFontWidth(int,const char*);extern "C" int P3DS_GameFontDraw(u32*,int,int,int,int,int,const char*,u32,u32,int,int);
static bool drawAchToastHD(const char*l1in,const char*l2,int sl){
 if(!P3DS_GameFontReady())return false;u32*ob=P3DS_TopOvBufGet();if(!ob)return false;
 static char key[100];static int lastSl=-1;char k2[100];snprintf(k2,sizeof k2,"%s|%s",l1in,l2);
 char l1[48],sc[12]="";snprintf(l1,sizeof l1,"%s",l1in);{char*tb=strchr(l1,'\t');if(tb){*tb=0;snprintf(sc,sizeof sc,"%s",tb+1);}}
 const int fw1=P3DS_GameFontWidth34(1,l1),fw2=P3DS_GameFontWidth(1,l2)+(sc[0]?P3DS_GameFontWidth(1,sc)+8:0);
 const int H=36,R=H/2,coin=13,tx=R+2*coin-4;int W=tx+(fw1>fw2?fw1:fw2)+R;if(W<200)W=200;if(W>380)W=380;
 if(strcmp(k2,key)||P3DS_TopOvW!=W){snprintf(key,sizeof key,"%s",k2);
  static u32 cv[64*512];memset(cv,0,sizeof cv);   /* canvas: r|g<<8|b<<16|a<<24 */
  auto blend=[&](int x,int y,int r,int g,int b,int a){if(x<0||x>=W||y<0||y>=H||a<=0)return;u32&d=cv[y*512+x];const int da=d>>24;
   const int oa=a+da*(255-a)/255;if(!oa)return;const int rr=(r*a+(d&255)*da*(255-a)/255)/oa,gg=(g*a+((d>>8)&255)*da*(255-a)/255)/oa,bb=(b*a+((d>>16)&255)*da*(255-a)/255)/oa;
   d=(u32)rr|((u32)gg<<8)|((u32)bb<<16)|((u32)oa<<24);};
  /* pill coverage (4x4) and rim */
  auto inPill=[&](float x,float y,float r){const float cx=x<R?R:x>W-R?W-R:x,dx=x-cx,dy=y-R;return dx*dx+dy*dy<=r*r;};
  for(int y=0;y<H;y++)for(int x=0;x<W;x++){int c=0,ci=0;for(int sy=0;sy<4;sy++)for(int sx=0;sx<4;sx++){const float fx=x+(sx+0.5f)/4,fy=y+(sy+0.5f)/4;if(inPill(fx,fy,R))c++;if(inPill(fx,fy,R-1.6f))ci++;}
   if(!c)continue;const float t=(float)y/(H-1);int r=(int)(58-40*t),g=(int)(60-40*t),b=(int)(64-40*t);if(y<H/2){r+=14;g+=14;b+=14;}   /* gloss on the upper half */
   blend(x,y,r,g,b,ci*235/16);if(c>ci)blend(x,y,150,155,160,(c-ci)*230/16);}
  /* gold coin with a highlight, anti-aliased */
  const float ccx=R+3.0f,ccy=R;for(int y=0;y<H;y++)for(int x=0;x<(int)(ccx+coin+2);x++){int c=0;float hl=0;for(int sy=0;sy<4;sy++)for(int sx=0;sx<4;sx++){const float fx=x+(sx+0.5f)/4-ccx,fy=y+(sy+0.5f)/4-ccy;if(fx*fx+fy*fy<=coin*coin)c++;}
   if(!c)continue;const float fx=x+0.5f-ccx,fy=y+0.5f-ccy,d=sqrtf(fx*fx+fy*fy)/coin;hl=fy<0?(-fy/coin)*0.5f:0;const bool ring=d>0.78f;
   int r=ring?196:(int)(236+19*hl),g=ring?150:(int)(182+50*hl),b=ring?40:(int)(52+60*hl);blend(x,y,r>255?255:r,g>255?255:g,b>255?255:b,c*255/16);}
  P3DS_GameFontDraw34(cv,512,W,H,(int)ccx-3,(int)ccy-6,"G",0xFF0A3A5Au,1);
  /* text: small grey title, then the score in gold and the name in white, 1:1 game font */
  P3DS_GameFontDraw34(cv,512,W,H,tx,4,l1,0xFFC8C8C8u,1);
  int x2=tx;if(sc[0]){x2+=P3DS_GameFontDraw(cv,512,W,H,x2,16,sc,0xFF40D6FFu,0,1,1)+8;}
  P3DS_GameFontDraw(cv,512,W,H,x2,16,l2,0xFFFFFFFFu,0,1,1);
  for(int y=0;y<H;y++)for(int x=0;x<W;x++){const u32 c=cv[y*512+x];ob[y*512+x]=((c&255)<<24)|(((c>>8)&255)<<16)|(((c>>16)&255)<<8)|(c>>24);}
  P3DS_TopOvW=W;P3DS_TopOvH=H;P3DS_TopOvDirty=1;}
 (void)lastSl;P3DS_TopOvX=(400-W)/2;P3DS_TopOvY=240-((H+6)*sl/16);P3DS_TopOvOn=1;return true;}
/* 2026-11-06 (Nathan: no cursor on the Mystery Gift menu): the game prints its list cursor as a text character into the list window,
   and on some lists (Mystery Gift) that never shows up. SS3DS_ListMenuProcessInput (ss3ds_glue.c, patch 0227) records the list the game
   is running; when that list's cursor spot in the finished top picture is still blank (nothing of the game's own cursor there), a
   smooth full-resolution arrow is drawn on it in the native top overlay. Lists whose own cursor shows are left alone. */
extern "C" volatile struct{unsigned frame;unsigned char bgId,left,top,width,height,cursorX,upY,kind;unsigned short pos,above,total,shown;unsigned char font,padding;}P3DS_ListMenuNow;
extern "C" u32*P3DS_TopOv2BufGet(void);extern "C" volatile int P3DS_TopOv2On,P3DS_TopOv2Dirty;extern "C" int P3DS_TopOv2X,P3DS_TopOv2Y,P3DS_TopOv2W,P3DS_TopOv2H;
extern "C" unsigned PSPNativeRenderFrameCount(void);extern "C" void P3DS_TopOvInit(void);
__attribute__((noinline)) static void drawListCursor(const u32*img){
 static bool pre;if(!pre){pre=true;P3DS_TopOvInit();}   /* buffers and textures at the first frame, never while a screen is loading (they crashed the game thread) */
 const unsigned fr=PSPNativeRenderFrameCount();if(!P3DS_ListMenuNow.frame||fr>P3DS_ListMenuNow.frame+3||fr+3<P3DS_ListMenuNow.frame)return;
 if(P3DS_ListMenuNow.kind!=0||P3DS_ListMenuNow.bgId>3||P3DS_ListMenuNow.total==0||P3DS_ListMenuNow.height<2)return;
 const int rowH=16+P3DS_ListMenuNow.padding,bx=P3DS_ListMenuNow.left*8+P3DS_ListMenuNow.cursorX,by=P3DS_ListMenuNow.top*8+P3DS_ListMenuNow.upY+P3DS_ListMenuNow.above*rowH;
 if(bx<0||by<0||bx+7>256||by+16>192)return;
 /* the game's own cursor is there when this 7x16 spot holds ink: more than a few pixels that differ from the corner (window fill) */
 const u32 fill=img[by*256+bx];int ink=0;for(int y=by;y<by+16;y++)for(int x=bx;x<bx+7;x++)if((img[y*256+x]&0xFFFFFF)!=(fill&0xFFFFFF))ink++;
 if(ink>4)return;
 u32*ob=P3DS_TopOv2BufGet();if(!ob)return;
 static bool made;if(!made){made=true;memset(ob,0,64*32*4);const int W=10,H=18;   /* right-pointing triangle, 4x4 coverage, dark with a soft edge */
  for(int y=0;y<H;y++)for(int x=0;x<W;x++){int c=0;for(int sy=0;sy<4;sy++)for(int sx=0;sx<4;sx++){const float fx=x+(sx+0.5f)/4,fy=y+(sy+0.5f)/4;const float ty=fy-H*0.5f;const float lim=(fx-1.0f)*(H*0.5f-2.0f)/(W-3.0f);   /* half height shrinks toward the tip */
    if(fx>=1.0f&&fx<=W-2.0f&&ty<=((W-2.0f-fx)/(W-3.0f))*(H*0.5f-2.0f)+0*lim&&ty>=-((W-2.0f-fx)/(W-3.0f))*(H*0.5f-2.0f))c++;}
   if(c)ob[y*64+x]=(0x30u<<24)|(0x30u<<16)|(0x30u<<8)|(unsigned)(c*255/16);}   /* R<<24|G<<16|B<<8|A */
  P3DS_TopOv2Dirty=1;}
 P3DS_TopOv2W=10;P3DS_TopOv2H=18;P3DS_TopOv2X=40+(int)(bx*1.25f);P3DS_TopOv2Y=(int)(by*1.25f)+1;P3DS_TopOv2On=1;}
static void drawAchToast(u32*img,int eye){char l1[48],l2[48];int sl=0;if(!P3DS_AchToastNow(l1,sizeof l1,l2,sizeof l2,&sl,eye))return;
 if(eye==0&&drawAchToastHD(l1,l2,sl))return;if(eye==1&&P3DS_TopOvOn)return;topTouch();
 /* Xbox 360 style, small: dark pill, green rim, round gold badge, tiny green title + score, name in the game font (3/4 size, lower case works) */
 const bool gf=P3DS_GameFontReady();char sc[12]="";{char*tb=strchr(l1,'\t');if(tb){*tb=0;snprintf(sc,sizeof sc,"%s",tb+1);}}
 for(char*p=l1;*p;p++)if(*p>='a'&&*p<='z')*p-=32;
 if(!gf)for(char*p=l2;*p;p++)if(*p>='a'&&*p<='z')*p-=32;
 const int nw=gf?P3DS_GameFontWidth34(1,l2):(int)strlen(l2)*4,tw=(int)strlen(l1)*4+(sc[0]?(int)strlen(sc)*4+8:0);
 int bw=26+(nw>tw?nw:tw)+8;if(bw<120)bw=120;if(bw>230)bw=230;
 const int bh=24,by=192-(bh*sl/16)-4,bx=(256-bw)/2;
 auto C=[](int r,int g,int b){return (u32)(r|(g<<8)|(b<<16));};
 auto put=[&](int x,int y,u32 c){if(x>=0&&x<256&&y>=0&&y<192)img[y*256+x]=c;};
 const int rr=bh/2;
 for(int y=by;y<by+bh;y++)for(int x=bx;x<bx+bw;x++){
  int dx=x<bx+rr?bx+rr-x:x>bx+bw-1-rr?x-(bx+bw-1-rr):0,dy=y-(by+rr);if(dx*dx+dy*dy>rr*rr)continue; /* full pill */
  const bool edge=dx*dx+dy*dy>(rr-1)*(rr-1)||y==by||y==by+bh-1;
  put(x,y,edge?C(70,180,80):(C(18,22,28)+(unsigned)(((y-by)*8/bh)*0x00010101u)));}
 const int cxm=bx+rr+1,cym=by+rr,R=9;
 for(int y=-R;y<=R;y++)for(int x=-R;x<=R;x++){const int d=x*x+y*y;if(d>R*R)continue;put(cxm+x,cym+y,d>(R-1)*(R-1)?C(200,160,48):(y<0?C(250,220,100):C(232,178,50)));}
 drawTiny(img,cxm-1,cym-2,"G",C(70,45,10));
 const int tx=bx+rr+R+5;
 drawTiny(img,tx,by+4,l1,C(112,224,112));
 if(sc[0])drawTiny(img,bx+bw-rr-(int)strlen(sc)*4,by+4,sc,C(255,214,64));
 if(gf)P3DS_GameFontDraw34(img,256,256,192,tx,by+9,l2,0x00FFFFFFu,1);else drawTiny(img,tx,by+12,l2,0x00FFFFFFu);}
static void drawToast(u32*img,u64 now){if(!toastAt||now-toastAt>(u64)(1500*CPU_TICKS_PER_MSEC))return;topTouch();
 if(useGF()){char tt2[32];prettyLab(tt2,toastText,sizeof tt2);const char*toastText=tt2;int tw=P3DS_GameFontWidth(1,toastText);int iw=(tw+7)&~7,bx=128-(iw+40)/2+16;P3DS_GameBoxDraw(img,256,256,192,bx,8,iw,16);P3DS_GameFontDraw(img,256,256,192,bx+(iw-tw)/2,8,toastText,kMsgFg,kMsgSh,1,1);return;}
 const int tw=(int)strlen(toastText)*8,bw=tw+12,bh=20,bx=128-bw/2,by=6;
 for(int y=by;y<by+bh;y++)for(int x=bx;x<bx+bw;x++){if(x<0||x>255)continue;bool edge=y==by||y==by+bh-1||x==bx||x==bx+bw-1||y==by+1||y==by+bh-2||x==bx+1||x==bx+bw-2;img[y*256+x]=edge?0x00F0F0F0u:0x00602814u;}
 drawBig(img,bx+6,by+5,toastText,0x00FFFFFFu);}
/* run 61 follower_talk: port overlay text box (HGSS-style "<NAME> IS HAPPY..."), bottom of the top screen, drawn
   after the frame is composed (like the toast) -> no game state, no script, no save. Word-wrapped, 2 lines max. */
static char folText[100];static volatile u64 folAt;static volatile int folMs;
static int folSlotV=-1,folTalkV=-1;
extern "C" int P3DS_FollowerSlotCfg(void){if(folSlotV<0){folSlotV=P3DS_CfgInt("follower_slot",0);if(folSlotV<0||folSlotV>6)folSlotV=0;PSPNativeMemLog("[FOL] follower_slot=%d (0 = lead)",folSlotV);}return folSlotV;}
extern "C" int P3DS_FollowerTalkCfg(void){if(folTalkV<0){folTalkV=P3DS_CfgInt("follower_talk",0)!=0;PSPNativeMemLog("[FOL] follower_talk=%d",folTalkV);}return folTalkV;}
extern "C" void P3DS_FolSay(const char*t,int ms){snprintf(folText,sizeof folText,"%s",t);folMs=ms;folAt=t[0]?svcGetSystemTick():0;}
/* summary_ivs=1 (default 0 in SoulSilver): on the Pokemon summary screen, Y toggles a read-only IV / EV box over the ability
   text (order HP/Atk/Def/SpA/SpD/Spe). Values copied by the summary screen; nothing is written to the Pokemon. */
extern "C" volatile int P3DS_SumIV[6],P3DS_SumEV[6],P3DS_SumAlive;extern "C" volatile int P3DS_SumOpen;volatile int P3DS_SumOpen;static int sumShow;
static int sumIvCfg(void);static void drawSumIV(u32*img){int cfg=sumIvCfg();
 static int alive;if(img){if(P3DS_SumAlive>0){alive=P3DS_SumAlive--;}else alive=0;P3DS_SumOpen=alive>0;}
 {static int la;if((alive>0)!=la){la=alive>0;PSPNativeMemLog("[UI] summary skills page %d",la);}}
 if(!cfg||!alive){sumShow=0;return;}
 {static u32 prevK;u32 k=hidKeysHeld();if((k&KEY_Y)&&!(prevK&KEY_Y)&&!(k&(KEY_L|KEY_R))){sumShow^=1;PSPNativeMemLog("[UI] summary IV/EV box %d",sumShow);}prevK=k;}
 if(!sumShow||!P3DS_GameBoxReady())return;topTouch();
 char a[64],b[64];snprintf(a,sizeof a,"IV %d/%d/%d/%d/%d/%d",P3DS_SumIV[0],P3DS_SumIV[1],P3DS_SumIV[2],P3DS_SumIV[3],P3DS_SumIV[4],P3DS_SumIV[5]);
 snprintf(b,sizeof b,"EV %d/%d/%d/%d/%d/%d",P3DS_SumEV[0],P3DS_SumEV[1],P3DS_SumEV[2],P3DS_SumEV[3],P3DS_SumEV[4],P3DS_SumEV[5]);
 P3DS_GameBoxDraw(img,256,256,192,16,152,216,32);P3DS_GameFontDraw(img,256,256,192,18,152,a,kMsgFg,kMsgSh,1,1);P3DS_GameFontDraw(img,256,256,192,18,168,b,kMsgFg,kMsgSh,1,1);}
extern "C" volatile int P3DS_PromptOpen,P3DS_PromptSel;extern "C" char P3DS_PromptText[64];
static void drawPrompt(u32*img){if(P3DS_PromptOpen!=1||!P3DS_GameBoxReady())return;topTouch(); /* run 64 repel_prompt box */
 P3DS_GameBoxDraw(img,256,256,192,16,152,216,32);P3DS_GameFontDraw(img,256,256,192,18,152,P3DS_PromptText,kMsgFg,kMsgSh,1,1);
 const int ix=192,iy=104,iw=40,ih=32;P3DS_GameBoxDraw(img,256,256,192,ix,iy,iw,ih);
 P3DS_GameFontDraw(img,256,256,192,ix+10,iy,"Yes",kMsgFg,kMsgSh,1,1);P3DS_GameFontDraw(img,256,256,192,ix+10,iy+16,"No",kMsgFg,kMsgSh,1,1);
 {int cy=iy+8+(P3DS_PromptSel?16:0);for(int r=0;r<7;r++){int h=r<4?r:6-r;for(int c=0;c<=h;c++)img[(cy+r-3+4)*256+ix+3+c]=kMsgFg;}}}
/* run 71 follower_emotes: HGSS emote bubble (fuki_* art from the user's SoulSilver ROM, SD file emotes.bin made by
   followers/extract_emotes.py) over the follower's head while it talks. Position = the follower's projected head top
   (p3ds_follower.h, every frame). Short pop-in, then a 2-frame bob. Missing file -> no bubble. */
static u32 emTex[14][32*32];static int emN=-1;static volatile int emIdx=-1,emX,emY;static volatile u64 emAt;static volatile int emMs;
static void emLoad(){emN=0;FILE*f=fopen("sdmc:/3ds/platinum3ds/emotes.bin","rb");if(!f){PSPNativeMemLog("[FOL] emotes.bin missing: no bubbles");return;}
 char h[8];u32 n=0;if(fread(h,1,8,f)==8&&!memcmp(h,"P3EM",4)){memcpy(&n,h+4,4);if(n>14)n=14;
  for(u32 i=0;i<n;i++){u8 b[12+512+32];if(fread(b,1,sizeof b,f)!=sizeof b)break;const u16*pl=(const u16*)(b+524);
   for(int k=0;k<1024;k++){u8 v=b[12+k/2];unsigned ci=(k&1)?v>>4:v&15;if(!ci){emTex[i][k]=0;continue;}u16 c=pl[ci];u32 r=c&31,g=(c>>5)&31,bb=(c>>10)&31;
    emTex[i][k]=(((r<<3)|(r>>2))<<24)|(((g<<3)|(g>>2))<<16)|(((bb<<3)|(bb>>2))<<8)|0xFF;}emN=(int)i+1;}}
 fclose(f);PSPNativeMemLog("[FOL] emotes.bin: %d bubbles",emN);}
extern "C" void P3DS_FolEmote(int i,int ms){if(emN<0)emLoad();emIdx=i;emMs=ms;emAt=svcGetSystemTick();PSPNativeMemLog("[FOL] emote %d",i);}
extern "C" void P3DS_FolEmoteAt(int x,int y){emX=x;emY=y;}

/* run 123 type_hint (default OFF): badges on FIGHT menu when P3DS_MoveHint is filled by a battle hook (not yet wired on Platinum). */
extern "C" { void P3DS_RibbonDraw(unsigned*); void P3DS_RibbonFrame(unsigned); int P3DS_RibbonOn(void); void P3DS_CelebDraw(unsigned*,int); int P3DS_CelebOn(void);
 void P3DS_AchSetToasts(int); int P3DS_AchToastsOn(void); void P3DS_TrkSetOn(int); int P3DS_TrkOn(void); void P3DS_RibbonSetOn(int); void P3DS_CelebSetOn(int); }
static int hintMult(int a,int d){static const signed char T[][3]={{0,5,1},{0,8,1},{0,7,0},{10,10,1},{10,11,1},{10,12,4},{10,15,4},{10,6,4},{10,5,1},{10,16,1},{10,8,4},
 {11,10,4},{11,11,1},{11,12,1},{11,4,4},{11,5,4},{11,16,1},{13,11,4},{13,13,1},{13,12,1},{13,4,0},{13,2,4},{13,16,1},
 {12,10,1},{12,11,4},{12,12,1},{12,3,1},{12,4,4},{12,2,1},{12,6,1},{12,5,4},{12,16,1},{12,8,1},{15,10,1},{15,11,1},{15,12,4},{15,15,1},{15,4,4},{15,2,4},{15,16,4},{15,8,1},
 {1,0,4},{1,15,4},{1,3,1},{1,2,1},{1,14,1},{1,6,1},{1,5,4},{1,7,0},{1,17,4},{1,8,4},{3,12,4},{3,3,1},{3,4,1},{3,5,1},{3,7,1},{3,8,0},
 {4,10,4},{4,13,4},{4,3,4},{4,2,0},{4,6,1},{4,5,4},{4,8,4},{2,13,1},{2,12,4},{2,1,4},{2,6,4},{2,5,1},{2,8,1},
 {14,1,4},{14,3,4},{14,14,1},{14,17,0},{14,8,1},{6,10,1},{6,12,4},{6,1,1},{6,3,1},{6,2,1},{6,14,4},{6,7,1},{6,17,4},{6,8,1},
 {5,10,4},{5,15,4},{5,1,1},{5,4,1},{5,2,4},{5,6,4},{5,8,1},{7,0,0},{7,14,4},{7,7,4},{7,17,1},{7,8,1},{16,16,4},{16,8,1},
 {17,1,1},{17,14,4},{17,7,4},{17,17,1},{17,8,1},{8,10,1},{8,11,1},{8,13,1},{8,15,4},{8,5,4},{8,8,1}};
 for(int i=0;i<(int)(sizeof T/sizeof T[0]);i++)if(T[i][0]==a&&T[i][1]==d)return T[i][2];return 2;}
static void drawTypeHint(u32*img){
 if(!P3DS_InBattle||!P3DS_MoveHint[0]||!P3DS_CfgInt("type_hint",0))return;
 static const int kHintX[2]={4,132},kHintY[2]={79,144},kHintW=120;
 for(int k=0;k<4;k++){const int mv=P3DS_MoveHint[2+k],ty=P3DS_MoveHint[6+k];if(mv<=0||mv>=468||ty>=18||P3DS_MoveHint[10+k]<=0)continue;
  int best=-1;bool two=P3DS_MoveHint[16]<18;
  for(int f=0;f<(two?2:1);f++){const int t1=P3DS_MoveHint[14+2*f],t2=P3DS_MoveHint[15+2*f];if(t1>=18)continue;
   int m=hintMult(ty,t1)*(t2<18&&t2!=t1?hintMult(ty,t2):2);if(f==0||m>best)best=m;}
  if(best<0||best==4)continue;
  const char*txt=best==0?"NO EFFECT":best==1?"RESIST X1/4":best==2?"RESIST X1/2":best==8?"SUPER X2":"SUPER X4";
  u32 col=best==0?0x00707070u:best<4?0x003030C8u:0x0030A030u;
  const int x0=kHintX[k&1],y0=kHintY[k>>1];const int w=(int)strlen(txt)*4-1,pw=w+6,px=x0+kHintW-4-pw,py=y0,ph=9;const u32 dk=(col>>1)&0x007F7F7Fu;
  for(int yy=0;yy<ph;yy++)for(int xx=0;xx<pw;xx++){if((xx==0||xx==pw-1)&&(yy==0||yy>=ph-2))continue;
   const int X=px+xx,Y=py+yy;if(X>=0&&X<256&&Y>=0&&Y<192)img[Y*256+X]=yy>=ph-1?dk:col;}
  drawTiny(img,px+3,py+2,txt,0x00FFFFFFu);}}

static void drawFolEmote(u32*img,u64 now){if(emIdx<0||emIdx>=emN||!emAt)return;const u64 el=(now-emAt)/CPU_TICKS_PER_MSEC;if(el>(u64)emMs||!P3DS_InField){emAt=0;return;}topTouch();
 const u32*t=emTex[emIdx];int sc=el<120?(int)(el*16/120)+1:16;if(sc>16)sc=16;const int bob=el>=120&&((el/250)&1)?1:0;
 const int w=32*sc/16,x0=emX-w/2,y0=emY-w+10-bob;
 for(int y=0;y<w;y++){const int yy=y0+y;if(yy<0||yy>=192)continue;const u32*row=t+(y*16/sc)*32;
  for(int x=0;x<w;x++){const int xx=x0+x;if(xx<0||xx>=256)continue;const u32 c=row[x*16/sc];if(c)img[yy*256+xx]=c;}}}
static void drawFolTalk(u32*img,u64 now){if(!folAt||!folText[0]||now-folAt>(u64)folMs*CPU_TICKS_PER_MSEC)return;
 if(!P3DS_InField){folAt=0;return;}topTouch();
 if(useGF()){char ln[2][64];int n=gfWrap(folText,208,ln,2);/* the game's field message window: tiles (2,19) 27x4 -> interior 16,152 216x32 */
  P3DS_GameBoxDraw(img,256,256,192,16,152,216,32);for(int i=0;i<n;i++)P3DS_GameFontDraw(img,256,256,192,18,152+i*16,ln[i],kMsgFg,kMsgSh,1,1);return;}
 char up[100];toUpper(up,folText,sizeof up);const char*folTextU=up;
 char l1[32],l2[32];l1[0]=l2[0]=0;{const int W=27;const char*t=folTextU;int n=(int)strlen(t),cut=n;if(n>W){cut=W;while(cut>0&&t[cut]!=' ')cut--;if(!cut)cut=W;}
  snprintf(l1,sizeof l1,"%.*s",cut,t);const char*r=t+cut;while(*r==' ')r++;snprintf(l2,sizeof l2,"%.27s",r);}
 const int bx=8,by=146,bw=240,bh=42;
 for(int y=by;y<by+bh;y++)for(int x=bx;x<bx+bw;x++){bool e=y<by+2||y>=by+bh-2||x<bx+2||x>=bx+bw-2;bool e2=!e&&(y<by+3||y>=by+bh-3||x<bx+3||x>=bx+bw-3);img[y*256+x]=e?0x00483828u:e2?0x00B0A8A0u:0x00F8F8F8u;}
 drawBig(img,bx+10,by+8,l1,0x00403830u);if(l2[0])drawBig(img,bx+10,by+24,l2,0x00403830u);}
/* run 64 credits_splash=1 (default): boot credits drawn by the port over the first game frames (the game is still
   loading / black then, so no wait is added): credits page (patch by, Claude, pspoke, pret, Platinum port) in the game's own
   font (read from the user's ROM; port font fallback), splash_ms (default 3000) incl. fades, A skips. ROM untouched. */
static int splashCfg=-1,splashMs=3000;static u64 splashT0,splashEnd;static bool splashDone;
static u32 scaleCol(u32 c,int a){return ((((c&255)*a)>>8)&255)|(((((c>>8)&255)*a)>>8)<<8)|(((((c>>16)&255)*a)>>8)<<16);}
extern "C" volatile int P3DS_SkipIntro; /* ss3ds 0071 (main3ds.c) */
static bool splashActive(u64 now){if(splashDone)return false;
 if(splashCfg<0){splashCfg=P3DS_CfgInt("credits_splash",1)!=0&&!P3DS_SkipIntro;/* no credits splash on a skip-intro reset */splashMs=P3DS_CfgInt("splash_ms",3000);if(splashMs<300)splashMs=300;if(splashMs>5000)splashMs=5000;PSPNativeMemLog("[SPLASH] credits_splash=%d splash_ms=%d font=%d",splashCfg,splashMs,P3DS_GameFontReady());}
 if(!splashCfg){splashDone=true;return false;}
 const u64 ms=CPU_TICKS_PER_MSEC;if(!splashT0){splashT0=now;splashEnd=now+(u64)splashMs*ms;}
 if(!(splashEnd-now<(u64)250*ms)&&(hidKeysHeld()&KEY_A)){splashEnd=now+(u64)250*ms;PSPNativeMemLog("[SPLASH] skipped (A)");}
 if(now>=splashEnd){splashDone=true;PSPNativeMemLog("[SPLASH] done after %u ms",(unsigned)((now-splashT0)/ms));return false;}return true;}
static void drawSplash(u32*img,u64 now){topTouch();const u64 ms=CPU_TICKS_PER_MSEC;int in=(int)((now-splashT0)/ms),out=(int)((splashEnd-now)/ms);
 int a=256;if(in<300)a=in*256/300;if(out<250&&out*256/250<a)a=out*256/250;if(a<0)a=0;
 for(int i=0;i<256*192;i++)img[i]=0;
 /* run 100 (v1.0): full credits. Port by Unown Collectables; built on pspoke (native PSP/PC runtime of pret's
    pokeplatinum decomp); wide layouts and the bottom icon menu from GabeSud0's pokeplatinum-3ds. */
 static const char*const L[6]={"Platinum 3DS v1.0","Port by Unown Collectables","with 3DS Homebrew Dev","Built on pspoke + pret pokeplatinum","Wide layouts & icon menu: GabeSud0","Not affiliated with Nintendo/Game Freak"};
 static const int Y[6]={40,70,86,112,128,156};static const u32 C[6]={0x00F8F8F8u,0x00F8F8F8u,0x00B8B0A8u,0x00B8B0A8u,0x00B8B0A8u,0x00787068u};
 for(int k=0;k<6;k++){if(P3DS_GameFontReady()){int w=P3DS_GameFontWidth(1,L[k]);P3DS_GameFontDraw(img,256,256,192,128-w/2,Y[k],L[k],scaleCol(C[k],a),scaleCol(0x00404040u,a),1,1);}
  else drawBig(img,8,Y[k],L[k],scaleCol(C[k],a));}
 for(int x=48;x<208;x++)img[60*256+x]=scaleCol(0x00585048u,a);}
static void drawSaveBox(u32*img,u64 now){
 /* run 50 fail-safe: the takeover box only while it is clearly the field save sequence. Leaving the overworld, or 20 s
    with no yes/no menu frame and no save write (a script path that never closes the save info), drops it so the
    game's own windows are visible again. */
 if(saveSeq==1){u64 last=seqAct>saveLast?seqAct:saveLast;
  if(!P3DS_InField||now-last>(u64)(20000*CPU_TICKS_PER_MSEC)){saveSeq=0;saveYN=-1;PSPNativeMemLog("[SAVE] seq fail-safe: box dropped (field %d)",P3DS_InField);}}
 const bool seq=saveSeq==1;
 if(saveAnim<=0||(!saveFirst&&!seq))return;
 const u64 ms=CPU_TICKS_PER_MSEC;u64 endSaving=saveFirst+(u64)saveMinMs*ms;if(saveLast+300*ms>endSaving)endSaving=saveLast+300*ms;
 if(saveFirst&&!seq&&now>endSaving+800*ms){saveFirst=0;return;}topTouch();
 const bool asking=seq&&!saveFirst;const bool saving=!asking&&now<endSaving;
 if(saveAnim==2&&(seq||P3DS_InField)){/* run 50: outside the overworld only the small corner overlay. run 46 save_anim=2 (default): cover the game's own message window area (bottom of the top screen,
    DS y 144..187) with the port box, so the game's short "Saving..." box is replaced by our animation; the game's
    flow/state is untouched (its "saved the game" message is revealed when our box ends). */
  if(useGF()){/* run 64: the game's message window + font (same place as the game's field message box) */
   P3DS_GameBoxDraw(img,256,256,192,16,152,216,32);
   const char*t=asking?"Would you like to save the game?":saving?"Saving...":"Your progress has been saved!";
   int tw=P3DS_GameFontDraw(img,256,256,192,18,152,t,kMsgFg,kMsgSh,1,1);
   if(saving){static const signed char dx[8]={0,3,4,3,0,-3,-4,-3},dy[8]={-4,-3,0,3,4,3,0,-3};unsigned ph=(unsigned)((now/ms)/90)%8;int cx=18+tw+10,cy=152+7;
    for(int i=0;i<8;i++){u32 c=i==(int)ph?0x003030FFu:((i+1)%8==(int)ph?0x002020A0u:0x00A0A0A0u);for(int yy=0;yy<2;yy++)for(int xx=0;xx<2;xx++)img[(cy+dy[i]+yy)*256+cx+dx[i]+xx]=c;}}
   if(seq&&saveSeqCfg==2&&saveYN>=0){const int ix=200,iy=104,iw=32,ih=32;P3DS_GameBoxDraw(img,256,256,192,ix,iy,iw,ih);
    P3DS_GameFontDraw(img,256,256,192,ix+10,iy,"Yes",kMsgFg,kMsgSh,1,1);P3DS_GameFontDraw(img,256,256,192,ix+10,iy+16,"No",kMsgFg,kMsgSh,1,1);
    int cy=iy+(saveYN==0?4:20);for(int k=0;k<4;k++)for(int yy=k;yy<8-k;yy++)img[(cy+yy)*256+ix+k]=kMsgFg;}
   return;}
  const int bx=4,by=144,bw=248,bh=44;
  for(int y=by;y<by+bh;y++)for(int x=bx;x<bx+bw;x++){bool edge=y==by||y==by+bh-1||x==bx||x==bx+bw-1||y==by+1||y==by+bh-2||x==bx+1||x==bx+bw-2;img[y*256+x]=edge?0x00F0F0F0u:0x00602814u;}
  const char*t=asking?"SAVE?":saving?"SAVING":"SAVED!";const int tw=(int)strlen(t)*8,tx=128-(tw+(saving?18:0))/2,ty=by+17;
  drawBig(img,tx,ty,t,asking||saving?0x00FFFFFFu:0x0060FF60u);
  if(seq&&saveSeqCfg==2&&saveYN>=0){const int yx=192,yy=96,yw=64,yh=48;
   for(int y=yy;y<yy+yh;y++)for(int x=yx;x<yx+yw;x++){bool edge=y==yy||y==yy+yh-1||x==yx||x==yx+yw-1||y==yy+1||y==yy+yh-2||x==yx+1||x==yx+yw-2;img[y*256+x]=edge?0x00F0F0F0u:0x00602814u;}
   drawBig(img,yx+20,yy+10,"YES",saveYN==0?0x00FFFFFFu:0x00909090u);drawBig(img,yx+20,yy+27,"NO",saveYN==1?0x00FFFFFFu:0x00909090u);
   drawBig(img,yx+8,yy+(saveYN==0?10:27),">",0x0040C0FFu);}
  if(saving){static const signed char dx[8]={0,3,4,3,0,-3,-4,-3},dy[8]={-4,-3,0,3,4,3,0,-3};unsigned ph=(unsigned)((now/ms)/90)%8;int cx=tx+tw+10,cy=ty+4;
   for(int i=0;i<8;i++){u32 c=i==(int)ph?0x003030FFu:((i+1)%8==(int)ph?0x002020A0u:0x00808080u);
    for(int yy=0;yy<2;yy++)for(int xx=0;xx<2;xx++)img[(cy+dy[i]+yy)*256+cx+dx[i]+xx]=c;}}
  return;}
 if(useGF()){const char*t=saving?"Saving...":"Saved!";int tw=P3DS_GameFontWidth(1,t),iw=((tw+(saving?14:0))+7)&~7;int ix=256-24-iw-2;/* run 64: game frame + font */
  P3DS_GameBoxDraw(img,256,256,192,ix,168,iw,16);P3DS_GameFontDraw(img,256,256,192,ix,168,t,kMsgFg,kMsgSh,1,1);
  if(saving){static const signed char dx[8]={0,3,4,3,0,-3,-4,-3},dy[8]={-4,-3,0,3,4,3,0,-3};unsigned ph=(unsigned)((now/ms)/90)%8;int cx=ix+tw+8,cy=168+7;
   for(int i=0;i<8;i++){u32 c=i==(int)ph?0x003030FFu:((i+1)%8==(int)ph?0x002020A0u:0x00A0A0A0u);for(int yy=0;yy<2;yy++)for(int xx=0;xx<2;xx++)img[(cy+dy[i]+yy)*256+cx+dx[i]+xx]=c;}}
  return;}
 const int bw=saving?70:56,bh=16,bx=252-bw,by=172;
 for(int y=by;y<by+bh;y++)for(int x=bx;x<bx+bw;x++){bool edge=y==by||y==by+bh-1||x==bx||x==bx+bw-1;img[y*256+x]=edge?0x00F0F0F0u:0x00602814u;}
 if(saving){drawBig(img,bx+4,by+3,"SAVING",0x00FFFFFFu);
  static const signed char dx[8]={0,3,4,3,0,-3,-4,-3},dy[8]={-4,-3,0,3,4,3,0,-3};unsigned ph=(unsigned)((now/ms)/90)%8;int cx=bx+bw-9,cy=by+8;
  for(int i=0;i<8;i++){u32 c=i==(int)ph?0x003030FFu:((i+1)%8==(int)ph?0x002020A0u:0x00808080u);/* red head (R low byte) + trail */
   for(int yy=0;yy<2;yy++)for(int xx=0;xx<2;xx++)img[(cy+dy[i]+yy)*256+cx+dx[i]+xx]=c;}}
 else drawBig(img,bx+4,by+3,"SAVED!",0x0060FF60u);}
static bool fpsOn=false;static unsigned fpsShown=0,msShown=0;static int wideCfg=-1;/* run 55: file scope (settings menu) */
extern "C" { extern unsigned P3DS_UploadUs, G3G_ConvUs; }
static unsigned secUp,secConv;static u64 tPrevEnd,secStart;static unsigned secFrames,secG3,secComp,secGpu,secGame,secTris,secG3G;
extern "C" { int G3G_Hi(void); unsigned G3G_LastUs(void); }
extern "C" unsigned P3DS_G3LastUs(void);extern "C" unsigned P3DS_G3LastTris(void);
static u32 stereoL[256*192] __attribute__((aligned(64)));
static void shift3D(int dx){ /* dx>0 moves content right; vacated pixels transparent */
 u32*f=GPU3D::NativeFrame;
 for(unsigned y=0;y<192;y++){u32*r=f+y*256;
  if(dx>0){memmove(r+dx,r,(256-dx)*4);memset(r,0,dx*4);}else{memmove(r,r-dx,(256+dx)*4);memset(r+256+dx,0,(-dx)*4);}}
}
/* run 54 wide_fx=1 (default; only while wide view draws strips): the side strips are made to match the CENTRE'S OWN
   final picture of the same frame instead of guessing from registers.
   (a) brightness/fade/blend: a robust per-channel fit centre = k*3D + b over the 32 px next to each edge (all rows,
       outliers like text boxes / menus / sprites trimmed in 2 passes) is applied to the strips: master brightness,
       BLDY fades, uniform alpha overlays (fog/flash tint), white/black screens all follow exactly, no timing guess.
   (b) wipes / door / warp / battle bars: per row and side, when the centre's edge band is black but the 3D is not,
       that strip row is black too.
   (c) full-screen 2D take-over (capture swirls, 2D screens over the field): fit inliers < 35% -> strips black.
   (d) weather/overlays (rain, snow, sand, fog texture, flash circle edge): the centre's own 2D-over-3D difference is
       tiled into the strips with the DS 256 px wrap (left strip <- columns 224..255, right strip <- 0..31): the BG
       layers really repeat every 256 px, sprite weather is spread evenly. Rows where that band is mostly covered by
       an opaque panel (text box / menu) are skipped. Off (0): run 52 register fade (wide_fadefix). */
extern "C" { int P3DS_WideFxOn; u32 P3DS_WideFxBuf[2][64*192]; extern int G3G_Wide,G3G_StripValid; extern u32 G3G_Strip[2][64*192]; extern int P3DS_InField; }
extern "C" int P3DS_WideFxCfg(void);static int wideFxCfg(){return P3DS_WideFxCfg();}
static int wideFxV=-1;extern "C" int P3DS_WideFxCfg(void){int&v=wideFxV;if(v<0){v=0;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"wide_fx=%d",&x)==1)v=x!=0;fclose(f);}PSPNativeMemLog("[WIDE] wide_fx=%d",v);}return v;}
static inline void px8T(u32 t,int*o){int r=t&63,g=(t>>8)&63,b=(t>>16)&63;o[0]=(r<<2)|(r>>4);o[1]=(g<<2)|(g>>4);o[2]=(b<<2)|(b>>4);}
static inline void px8C(u32 c,int*o){o[0]=c&255;o[1]=(c>>8)&255;o[2]=(c>>16)&255;}
/* robust per-channel fit C = k*T + b over the edge band(s) in sideMask (1 = left 0..31, 2 = right 224..255) */
static void wideFit(const u32*C,const u32*T,int sideMask,float*k,float*b,int*inlO,int*totO){
 k[0]=k[1]=k[2]=1;b[0]=b[1]=b[2]=0;int inl=0,tot=0;
 static const int thr[5]={0,128,80,48,28};/* run 55: shrinking outlier threshold (one 28 cut after a biased first fit dropped everything) */
 for(int pass=0;pass<5;pass++){double sT[3]={0},sC[3]={0},sTT[3]={0},sTC[3]={0};int n=0;inl=0;tot=0;
  for(int y=0;y<192;y+=2)for(int side=0;side<2;side++){if(!(sideMask&(1<<side)))continue;const int x0=side?224:0;
   for(int x=x0+(y&2?1:0);x<x0+32;x+=2){u32 t=T[y*256+x];if((t>>24)==0)continue;int tv[3],cv[3];px8T(t,tv);px8C(C[y*256+x],cv);tot++;
    if(pass){int r=0;for(int c=0;c<3;c++){int d=cv[c]-(int)(k[c]*tv[c]+b[c]);if(d<0)d=-d;if(d>r)r=d;}if(r>=thr[pass])continue;}
    inl++;n++;for(int c=0;c<3;c++){sT[c]+=tv[c];sC[c]+=cv[c];sTT[c]+=tv[c]*tv[c];sTC[c]+=tv[c]*cv[c];}}}
  if(n<32)break;
  for(int c=0;c<3;c++){double mT=sT[c]/n,mC=sC[c]/n,vT=sTT[c]/n-mT*mT,cv=sTC[c]/n-mT*mC;
   if(vT<16){k[c]=1;b[c]=(float)(mC-mT);}else{k[c]=(float)(cv/vT);if(k[c]<0)k[c]=0;if(k[c]>1.15f)k[c]=1.15f;b[c]=(float)(mC-k[c]*mT);}}}
 *inlO=inl;*totO=tot;}
static void wideFx(const u32*C,const u32*T,int eyes){
 static int take;static unsigned stN,stTake,stBlackRows,stResRows;
 float k[3],b[3];int inl=0,tot=0;
 /* run 55: fit each side alone; a panel covering one edge band (start menu) must not decide the strips. If the two
    sides agree, fit both together; otherwise use the side that is closer to "unchanged" (higher k, better inliers). */
 {float kL[3],bL[3],kR[3],bR[3];int iL,tL,iR,tR;wideFit(C,T,1,kL,bL,&iL,&tL);wideFit(C,T,2,kR,bR,&iR,&tR);
  float dk=0,db=0;for(int c=0;c<3;c++){dk+=fabsf(kL[c]-kR[c]);db+=fabsf(bL[c]-bR[c]);}
  if(tL<32||tR<32||(dk<0.75f&&db<120.0f)){wideFit(C,T,3,k,b,&inl,&tot);}
  else{float sL=(kL[0]+kL[1]+kL[2])+(tL?3.0f*iL/tL:0),sR=(kR[0]+kR[1]+kR[2])+(tR?3.0f*iR/tR:0);
   if(sL>=sR){memcpy(k,kL,sizeof k);memcpy(b,bL,sizeof b);inl=iL;tot=tL;}else{memcpy(k,kR,sizeof k);memcpy(b,bR,sizeof b);inl=iR;tot=tR;}}}
 if(tot>=64){const int q=inl*100/tot;if(!take&&q<35)take=1;else if(take&&q>50)take=0;}
 stN++;if(take)stTake++;
 for(int e=0;e<eyes;e++){const u32*S=G3G_Strip[e];u32*O=P3DS_WideFxBuf[e];
  if(take){for(int i=0;i<64*192;i++)O[i]=0x000000FFu;continue;}
  for(int y=0;y<192;y++)for(int side=0;side<2;side++){
   /* (b) black row test on the ADJACENT band */
   const int ax=side?224:0;int nb=0,nv=0;
   for(int x=ax;x<ax+32;x++){u32 t=T[y*256+x];if((t>>24)==0)continue;int tv[3],cv[3];px8T(t,tv);px8C(C[y*256+x],cv);
    int pm=0;for(int c=0;c<3;c++){int pv=(int)(k[c]*tv[c]+b[c]);if(pv>pm)pm=pv;}if(pm<40)continue;nv++;if(cv[0]<12&&cv[1]<12&&cv[2]<12)nb++;}
   u32*o=O+y*64+side*32;const u32*sp=S+y*64+side*32;
   if(nv>=8&&nb*4>=nv*3){for(int i=0;i<32;i++)o[i]=0x000000FFu;if(!e)stBlackRows++;continue;}
   /* (d) residual band with the 256 px wrap */
   const int wx=side?0:224;int res[32][3];int cov=0;bool any=false;
   for(int i=0;i<32;i++){u32 t=T[y*256+wx+i];int cv[3];px8C(C[y*256+wx+i],cv);
    if((t>>24)==0){res[i][0]=res[i][1]=res[i][2]=0;continue;}int tv[3];px8T(t,tv);int m=0;
    for(int c=0;c<3;c++){int d=cv[c]-(int)(k[c]*tv[c]+b[c]);if(d>-7&&d<7)d=0;res[i][c]=d;if(d<0)d=-d;if(d>m)m=d;}if(m>=28)cov++;if(m)any=true;}
   const bool useRes=any&&cov*2<32;if(useRes&&!e)stResRows++;
   for(int i=0;i<32;i++){u32 c=sp[i];int v[3]={(int)(c>>24),(int)((c>>16)&255),(int)((c>>8)&255)};
    for(int ch=0;ch<3;ch++){int x=(int)(k[ch]*v[ch]+b[ch]+0.5f);if(useRes)x+=res[i][ch];v[ch]=x<0?0:x>255?255:x;}
    o[i]=((u32)v[0]<<24)|((u32)v[1]<<16)|((u32)v[2]<<8)|0xFF;}}}
 if(stN>=600){PSPNativeMemLog("[WIDE] wide_fx 600 frames: takeover %u, black strip rows %u, overlay rows %u; last k %.2f %.2f %.2f b %.0f %.0f %.0f",stTake,stBlackRows,stResRows,k[0],k[1],k[2],b[0],b[1],b[2]);stN=stTake=stBlackRows=stResRows=0;}
}
/* run 61 wide_haze (see gpuscale.c): fit C = k*T + b for an alpha-blended 2D haze layer over the 3D (BLDCNT effect 1,
   BG0 2nd target, a BG1-3 1st target). k = EVB/16 exactly; b = per-channel median of C - k*T over 192 sparse samples
   of the two 32-px edge bands (rows step 4, x step 8; sprites/text boxes are outliers the median ignores). */
extern "C" { extern int P3DS_HazeOn; extern unsigned P3DS_HazeK,P3DS_HazeB; }
static void wideHaze(const u32*C,const u32*T){
 const unsigned bc=reg16(0x50),ba=reg16(0x52);static unsigned n,on,usAcc;static unsigned lastBc,lastBa;
 if(++n%600==0){if(P3DS_LogLevel()>=2)PSPNativeMemLog("[WIDE] wide_haze: %u of 600 strip frames hazed, avg %u us | BLDCNT %04x BLDALPHA %04x",on,on?usAcc/on:0,lastBc,lastBa);on=0;usAcc=0;}
 lastBc=bc;lastBa=ba;
 if(((bc>>6)&3)!=1||!(bc&0x100)||!(bc&0x0E))return;
 topTouch();
 G3G_DirectResolveWhy(6);/* gpu_direct3d: the haze fit samples the centre 3D pixels (T) on the CPU */
 const u64 t0=svcGetSystemTick();
 unsigned evb=(ba>>8)&31;if(evb>16)evb=16;const int k16=(int)evb;
 static short res[3][192];int m=0;
 for(int y=2;y<192&&m<192;y+=4)for(int side=0;side<2;side++)for(int x=(side?224:0)+((y>>2)&1)*4;x<(side?256:32)&&m<192;x+=8){u32 t=T[y*256+x];if(!(t>>24))continue;int tv[3],cv[3];px8T(t,tv);px8C(C[y*256+x],cv);
  for(int c=0;c<3;c++)res[c][m]=(short)(cv[c]-((k16*tv[c])>>4));m++;}
 if(m<24)return;
 int b[3];for(int c=0;c<3;c++){std::nth_element(res[c],res[c]+m/2,res[c]+m);b[c]=res[c][m/2];if(b[c]<0)b[c]=0;if(b[c]>255)b[c]=255;}
 if(k16>=16&&b[0]<3&&b[1]<3&&b[2]<3)return;
 const unsigned kk=k16>=16?255:(unsigned)(k16*16);
 P3DS_HazeK=0xFF000000u|(kk<<16)|(kk<<8)|kk;P3DS_HazeB=0xFF000000u|((unsigned)b[2]<<16)|((unsigned)b[1]<<8)|(unsigned)b[0];P3DS_HazeOn=1;on++;
 usAcc+=(unsigned)((svcGetSystemTick()-t0)/CPU_TICKS_PER_USEC);}
/* run 70 wide_weather=1 (default; only while wide-view field strips are drawn): weather particles (snow, rain, ...) are
   2D OBJs the DS clips at the screen edge, so the strips had none. Small OBJs (<=32x32, normal/semi-transparent, not
   affine) are drawn into P3DS_WeatherObj (64x192, alpha 0 = nothing): the part really past the edge as is, plus the
   particles of the opposite 32-px edge band moved by one screen width (the game wraps its particles over 256 px, so
   this is exactly where the next particle of the field would be). No mirror/blur; gpuscale strips() overlays it. */
extern "C" { u32 P3DS_WeatherObj[64*192]; int P3DS_WeatherOn; }
static int wideWeatherCfg(){static int v=-1;if(v<0){v=P3DS_CfgInt("wide_weather",1)!=0;PSPNativeMemLog("[WIDE] wide_weather=%d",v);}return v;}
static void wideWeather(){
 const u8*R=(const u8*)PSPNative_GfxRegisters;const u32 dc=*(const u32*)R;P3DS_WeatherOn=0;if(!(dc&0x1000))return;
 const u16*o=(const u16*)GPU::OAM;const u8*vr=GPU::VRAMFlat_AOBJ;const u16*pal=(const u16*)(GPU::Palette+512);
 static const u8 SW[3][4]={{8,16,32,64},{16,32,32,64},{8,8,16,32}},SH[3][4]={{8,16,32,64},{8,8,16,32},{16,32,32,64}};
 bool any=false;
 for(int i=127;i>=0;i--){const u16 a0=o[i*4],a1=o[i*4+1],a2=o[i*4+2];if(a0&0x100)continue;if(a0&0x200)continue;const unsigned om=(a0>>10)&3;if(om>=2)continue;
  const unsigned sh=a0>>14;if(sh==3)continue;const int w=SW[sh][a1>>14],h=SH[sh][a1>>14];if(w>32||h>32)continue;
  int x=a1&0x1FF;if(x>=256)x-=512;int y=a0&0xFF;if(y>=192)y-=256;const bool b8=(a0&0x2000)!=0,hf=(a1&0x1000)!=0,vf=(a1&0x2000)!=0;
  const unsigned tn=a2&0x3FF,pb=a2>>12;const bool oneD=(dc&0x10)!=0;const unsigned bnd=oneD?(32u<<((dc>>20)&3)):32;
  /* run 73: weather particles come in groups (same palette, shape/size and nearby tiles, >= 4 OBJs); a lone sprite (menu cursor, emote)
     is not weather and is never copied */
  {int same=0;for(int j=0;j<128&&same<4;j++){const u16 b0=o[j*4],b2=o[j*4+2];const u16 b1=o[j*4+1];int td=(int)(b2&0x3FF)-(int)(a2&0x3FF);if(!(b0&0x200)&&(b2>>12)==(a2>>12)&&(b0>>14)==(a0>>14)&&(b1>>14)==(a1>>14)&&td<=16&&td>=-16)same++;}if(same<4)continue;}
  for(int pass=0;pass<3;pass++){int sx=x+(pass==1?-256:pass==2?256:0);  /* strip coords: DS x -32..-1 -> 0..31, 256..287 -> 32..63 */
   if(sx+w<=-32||sx>=288||(sx>=0&&sx+w<=256))continue;
   for(int py=0;py<h;py++){const int yy=y+py;if(yy<0||yy>=192)continue;const int ty=vf?h-1-py:py;
    for(int px=0;px<w;px++){const int xx=sx+px;int col;if(xx<0&&xx>=-32)col=xx+32;else if(xx>=256&&xx<288)col=xx-224;else continue;
     const int tx=hf?w-1-px:px;unsigned addr;
     if(oneD)addr=tn*bnd+((ty>>3)*(w>>3)+(tx>>3))*(b8?64:32);else addr=(tn+(ty>>3)*32+(tx>>3)*(b8?2:1))*32;
     unsigned ci;if(b8)ci=vr[(addr+(ty&7)*8+(tx&7))&0x3FFFF];else{const u8 b=vr[(addr+(ty&7)*4+((tx&7)>>1))&0x3FFFF];ci=(tx&1)?b>>4:b&15;}
     if(!ci)continue;const u16 cc=pal[b8?ci:pb*16+ci];const u32 r=cc&31,g=(cc>>5)&31,b=(cc>>10)&31;
     P3DS_WeatherObj[yy*64+col]=(((r<<3)|(r>>2))<<24)|(((g<<3)|(g>>2))<<16)|(((b<<3)|(b>>2))<<8)|0xFF;any=true;}}}}
 P3DS_WeatherOn=any;}
static void wideWeatherRun(){memset(P3DS_WeatherObj,0,sizeof P3DS_WeatherObj);wideWeather();}
/* run 55 wide_fill=1 (default; only with wideview on, 1.25x mode): every screen WITHOUT real wide 3D strips (battles,
   title, bag/party/Pokedex/summary menus, field screens whose 3D is off) fills the side areas with the centre
   picture's own edge, mirrored and softened (box blur 9x9, 80% brightness) instead of black bars. Built from the same
   frame's final picture, so fades/flashes follow; nothing is stretched, the centre stays native size. 0 = black bars. */
extern "C" { u32 P3DS_WideFillBuf[64*192]; int P3DS_CfgInt(const char*,int); extern int P3DS_InIntroMovie; }
static int wideFillCfg(){static int v=-1;if(v<0){v=P3DS_CfgInt("wide_fill",1)!=0; /* 2026-11-05 default 1 (Nathan: the whole game wide) */PSPNativeMemLog("[WIDE] wide_fill=%d",v);}return v&&!P3DS_InIntroMovie;} /* 2026-11-18: never in the intro movie (Nathan: the blurred edges looked wrong); its sides stay dark */
extern "C" int P3DS_WideEdgeClamp; /* ss3ds_widescreens.c: this screen continues its own edge column sideways (no mirror, no blur) */
static void wideFill(const u32*C){static u16 acc[3][64*192];
 if(P3DS_WideEdgeClamp){for(int y=0;y<192;y++)for(int side=0;side<2;side++){const u32 q=C[y*256+(side?255:0)];const u32 c=((q&255)<<24)|(((q>>8)&255)<<16)|(((q>>16)&255)<<8)|0xFFu;for(int i=0;i<32;i++)P3DS_WideFillBuf[y*64+side*32+i]=c;}return;}
 /* mirror: left strip col i (DS x -32+i) <- x 31-i, right strip col i (DS x 256+i) <- x 255-i; horizontal 9-tap box */
 for(int y=0;y<192;y++){const u32*row=C+y*256;
  for(int side=0;side<2;side++){int tmp[3][41];
   for(int i=-4;i<37;i++){int ii=i<0?0:i>31?31:i;int sx=side?255-ii:31-ii;u32 c=row[sx];tmp[0][i+4]=c&255;tmp[1][i+4]=(c>>8)&255;tmp[2][i+4]=(c>>16)&255;}
   for(int ch=0;ch<3;ch++){int sum=0;for(int k=0;k<9;k++)sum+=tmp[ch][k];for(int i=0;i<32;i++){acc[ch][y*64+side*32+i]=(u16)sum;sum+=tmp[ch][i+9]-tmp[ch][i];}}}}
 /* vertical 9-tap box, x 0.8 */
 for(int x=0;x<64;x++){int sum[3]={0,0,0};
  for(int k=-4;k<=4;k++){int yy=k<0?0:k;for(int ch=0;ch<3;ch++)sum[ch]+=acc[ch][yy*64+x];}
  for(int y=0;y<192;y++){int v[3];for(int ch=0;ch<3;ch++){v[ch]=sum[ch]*8/(81*10);if(v[ch]>255)v[ch]=255;}
   P3DS_WideFillBuf[y*64+x]=((u32)v[0]<<24)|((u32)v[1]<<16)|((u32)v[2]<<8)|0xFF;
   int ya=y+5>191?191:y+5,yr=y-4<0?0:y-4;for(int ch=0;ch<3;ch++)sum[ch]+=acc[ch][ya*64+x]-acc[ch][yr*64+x];}}
}
/* run 21: wide Pokedex / PC strips without mirroring or blur (only things that continue cleanly are extended).
   The background is rebuilt, not copied: the frame's most common colour, plus grid lines when one colour forms
   evenly spaced full lines (e.g. the Pokedex grid paper, every 40 px both ways). Per row and side: if the picture's
   8 edge pixels match that background, the strip shows the background (panels such as the text box just end at the
   edge); otherwise, if one colour fills >= 75% of the row (the red header band), the strip continues that colour;
   otherwise the background. Recomputed only when the picture changes. */
static u32 wtBuf[64*192];static void wideTile(const u32*C){
 {static u32 lastH=1;u32 h=2166136261u;for(int k=0;k<256*192;k+=7)h=(h^C[k])*16777619u;if(h==lastH){memcpy(P3DS_WideFillBuf,wtBuf,sizeof wtBuf);return;}lastH=h;}
 enum{NT=9};u32 tc[NT];int nt=0;
 {static u32 col[96];static unsigned n[96];int k=0;for(int i=0;i<256*192;i+=3){const u32 c=C[i]&0xFFFFFF;int j=0;while(j<k&&col[j]!=c)j++;if(j<k)n[j]++;else if(k<96){col[k]=c;n[k]=1;k++;}}
  for(;nt<NT;nt++){int b=-1;for(int j=0;j<k;j++)if(n[j]&&(b<0||n[j]>n[b]))b=j;if(b<0)break;tc[nt]=col[b];n[b]=0;}}
 if(!nt)return;const u32 dom=tc[0];
 static u16 cp[NT][256],rp[NT][192];memset(cp,0,sizeof cp);memset(rp,0,sizeof rp);
 for(int y=0;y<192;y++)for(int x=0;x<256;x++){const u32 c=C[y*256+x]&0xFFFFFF;for(int k=1;k<nt;k++)if(c==tc[k]){cp[k][x]++;rp[k][y]++;break;}}
 /* evenly spaced isolated peaks of one colour = grid lines; returns the spacing and the line phases */
 u32 lineC=dom;int P=1,Q=1;u64 vph=0,hph=0;int bestN=0;
 auto peaks=[&](const u16*pr,int len,int*pk)->int{int n=0;for(int j=2;j<len-2&&n<32;j++){int m=pr[j-2]>pr[j+2]?pr[j-2]:pr[j+2];if(pr[j]>=12&&pr[j]>2*m)pk[n++]=j;}return n;};
 for(int k=1;k<nt;k++){int px[32],py[32];const int nx=peaks(cp[k],256,px),ny=peaks(rp[k],192,py);if(nx<3)continue;
  int d=999;for(int i=1;i<nx;i++)if(px[i]-px[i-1]>=4&&px[i]-px[i-1]<d)d=px[i]-px[i-1];if(d>64)continue;bool ok=true;for(int i=1;i<nx;i++)ok&=(px[i]-px[i-1])%d==0;if(!ok||nx<=bestN)continue;
  bestN=nx;lineC=tc[k];P=d;vph=0;for(int i=0;i<nx;i++)vph|=1ull<<(px[i]%d);
  Q=1;hph=0;if(ny>=2){int e=999;for(int i=1;i<ny;i++)if(py[i]-py[i-1]>=4&&py[i]-py[i-1]<e)e=py[i]-py[i-1];bool o2=e<=64;for(int i=1;i<ny&&o2;i++)o2=(py[i]-py[i-1])%e==0;if(o2){Q=e;for(int i=0;i<ny;i++)hph|=1ull<<(py[i]%e);}}}
 {static int lp=-1,lq=-1;if(P!=lp||Q!=lq){lp=P;lq=Q;PSPNativeMemLog("[WIDE] tiled strips: grid %d x %d (line colour %06x, background %06x)",P,Q,(unsigned)lineC,(unsigned)dom);}}
 auto bg=[&](int x,int y)->u32{const int fx=((x%P)+P)%P,fy=((y%Q)+Q)%Q;return (P>1&&((vph>>fx)&1))||(Q>1&&((hph>>fy)&1))?lineC:dom;};
 for(int y=0;y<192;y++){const u32*r=C+y*256;
  u32 rm=0;unsigned rn=0;{u32 col[24];unsigned n[24];int k=0;for(int x=0;x<256;x++){const u32 c=r[x]&0xFFFFFF;int j=0;while(j<k&&col[j]!=c)j++;if(j<k)n[j]++;else if(k<24){col[k]=c;n[k]=1;k++;}}for(int j=0;j<k;j++)if(n[j]>rn){rn=n[j];rm=col[j];}}
  for(int side=0;side<2;side++){int ok=0;for(int i=0;i<8;i++){const int x=side?248+i:i;ok+=(r[x]&0xFFFFFF)==bg(x,y);}
   bool end=true;for(int i=0;i<4;i++){const u32 c=r[side?252+i:i]&0xFFFFFF;end&=c!=dom&&c!=lineC;} /* the band itself reaches the edge (no background beyond a panel) */
   const bool row=ok<6&&rn*4>=256*3&&rm!=dom&&end; /* a panel on the background colour: background */
   for(int i=0;i<32;i++){const int x=side?256+i:-32+i;const u32 c=row?rm:bg(x,y);wtBuf[y*64+side*32+i]=((c&255)<<24)|(((c>>8)&255)<<16)|(((c>>16)&255)<<8)|0xFF;}}}
 memcpy(P3DS_WideFillBuf,wtBuf,sizeof wtBuf);}
/* ss3ds: default 0. SoulSilver's intro movie, summary screens and title put panels and 3D at the screen edges, so
   the continued background strips showed cut-off panels and mismatched pieces; outside the overworld the DS picture
   is shown as is. run 65 wide_menus=1 (only with wideview on, 1.25x mode, engine A on top, frames WITHOUT wide 3D strips:
   menus, battles, title): the side areas show what the DS 2D engine itself would draw at DS x -32..-1 / 256..287 -
   a small CPU BG renderer reads the same VRAM / palettes / scroll registers. Per BG layer: 64-tile-wide maps (real
   content past the edge) are drawn as is; 32-wide maps only when the strip would show something that really
   continues: the map is horizontally periodic (tiled pattern backgrounds) or the wrapped-in columns are empty.
   Other layers are left out of the strips (a panel touching the edge just ends there). Nothing visible -> backdrop.
   No mirroring, no blur, no stretching; fades come from gpuscale strips() (wide_fadefix). OBJs are not drawn in the
   strips. 0 = old behaviour (black bars, or wide_fill=1). */
#include "widecomp.h"
extern "C" unsigned dStatRace,dStat3dRace;extern "C" int dBadRow,dBadMask;
static WcPlan wcPlanG;static WcSrc wideRSrc;static u32 wideRStrip[64*192];static const u8*wideRRegs;
static void wideRJob(){
 bWideROk=WC_ComposeEye(&wcPlanG,wideRRegs,&wideRSrc,rawR,wideRStrip,1)!=0;}
/* run 105 wide_split=1: the mono wide compose (battles at slider 0) is shared by the main thread and the core-2 worker, after
   engine B. The main thread hashes the lines, deals them to two parts (WC_SplitPrepare), signals wmGo, draws part 0 while the
   worker waits for / draws part 1, then waits for wmDone and commits. 0 = one thread (run 104). */
static LightEvent wmGo,wmDone;static WcSplit wmSp;static WcSrc wmSrc;static const u8*wmRegs;
static void wideMJob(){LightEvent_Wait(&wmGo);WC_SplitRun(&wcPlanG,wmRegs,&wmSrc,&wmSp,1);LightEvent_Signal(&wmDone);}
extern "C" { int P3DS_WideLayoutsOn(void); void P3DS_SetWideLayouts(int); int P3DS_WideGetParts(int,short*); }
static int wideMenusV=-1;extern "C" int P3DS_WideMenusCfg(void){if(wideMenusV<0){wideMenusV=P3DS_CfgInt("wide_menus",1)!=0;PSPNativeMemLog("[WIDE] wide_menus=%d",wideMenusV);}return wideMenusV;}
/* ss3ds wide_battle (Settings > Display > Wide battles, default 0): while a battle runs (P3DS_InBattle, set by
   ss3ds_glue.c from the battle overlay manager) the side areas continue the battle background with the same cheap
   wideNative() strips wide_menus uses (cached by a hash of the registers/edge tiles; no wide FX). The centre keeps
   the native DS picture, so the HP boxes and Pokemon stay where the game puts them. */
extern "C" int P3DS_InBattle;
static int wideBattleV=-1;static int wideBattleCfg(){if(wideBattleV<0){wideBattleV=P3DS_CfgInt("wide_battle",0)!=0;PSPNativeMemLog("[WIDE] wide_battle=%d",wideBattleV);}return wideBattleV;}
/* ss3ds wide_menu_screens (Settings > Display > Wide party/bag, default 0): like wide_battle but for the party menu, bag
   and Pokemon summary only (P3DS_InWideMenu, set by ss3ds_glue.c from the overlay manager templates). Same wideNative()
   strips, no Wide FX; the title, intro movie and other 2D screens keep the DS picture with bars (wide_menus=1 widens
   those too, but cut off panels there). */
extern "C" int P3DS_InWideMenu;
extern "C" int P3DS_InWideKind;
static int wideDexV=-1,widePcV=-1,bottomHdV=-1;
static int wideDexCfg(){if(wideDexV<0){wideDexV=P3DS_CfgInt("wide_pokedex",0)!=0;PSPNativeMemLog("[WIDE] wide_pokedex=%d",wideDexV);}return wideDexV;}
static int widePcCfg(){if(widePcV<0){widePcV=P3DS_CfgInt("wide_pc",0)!=0;PSPNativeMemLog("[WIDE] wide_pc=%d",widePcV);}return widePcV;}
static int bottomHdCfg(){bottomHdV=1;return 1;} /* run 151 (Nathan): the sharp column is the only one; the bottom_hd toggle and the old stretched column look are gone */
/* 2026-11-18 sharp_text (Settings > Display "Sharp text", default ON; row 90, the old bottom_hd slot): panel and settings text at 320x240 (ss3ds_sharptext.h) */
static int sharpTextV=-1;static bool sharpTextCfg(){if(sharpTextV<0){sharpTextV=P3DS_CfgInt("sharp_text",1)!=0;PSPNativeMemLog("[TEXT] sharp_text=%d",sharpTextV);}return sharpTextV!=0;}
static void sharpTextSet(int v);
static int sharpGameV=-1;static bool sharpGameCfg(){if(sharpGameV<0){sharpGameV=P3DS_CfgInt("sharp_game_text",0)!=0;/* Platinum: OFF until tested on hardware (SoulSilver: ON) */PSPNativeMemLog("[GTEXT] sharp_game_text=%d (game text windows on both screens at the 3DS resolution)",sharpGameV);}return sharpGameV!=0;}
extern "C" int P3DS_SharpGameTextCfg(void){return sharpGameCfg()?1:0;}
extern "C" const unsigned char*P3DS_GtVram(int eng){return eng?GPU::VRAMFlat_BBG:GPU::VRAMFlat_ABG;}
extern "C" unsigned char*P3DS_GtObjVram(int eng){return (unsigned char*)(eng?GPU::VRAMFlat_BOBJ:GPU::VRAMFlat_AOBJ);}
/* sharp game text (ss3ds_gametext.c): which screens show the live engine picture (bit 0 top, bit 1 bottom), the wide layout of the top */
extern "C" {extern volatile unsigned P3DS_GtScreensNow;extern const WcPlan*volatile P3DS_GtWidePlan;extern volatile int P3DS_GtWideEng;void P3DS_GtSync(unsigned);void P3DS_GtObjRestore(void);}
static unsigned gtOkPrev;
static int wideScreensV=-1;static int wideScreensCfg(){if(wideScreensV<0){wideScreensV=P3DS_CfgInt("wide_menu_screens",0)!=0;PSPNativeMemLog("[WIDE] wide_menu_screens=%d (party, bag, summary)",wideScreensV);}return wideScreensV;}
/* ss3ds wide_boxes (Settings > Wide > Wide HP boxes, default 0): the Platinum port's battle layout. The game hook (pspoke patch
   0100, battle_hp_bar.c) moves the player's HP box 32 px right and the enemy's 32 px left via P3DS_WideBoxExtra(); the wide
   native pass draws the side strips as for wide_battle, and the sprites that reach into a strip (widecomp.c
   WC_ComposeObjStrips, only the lines they cover) are drawn on top of them. Needs wide view + 1.25x scale. */
static int wideBoxV=-1;static int wideBoxCfg(){if(wideBoxV<0){wideBoxV=P3DS_CfgInt("wide_boxes",0)!=0;PSPNativeMemLog("[WIDE] wide_boxes=%d (Platinum-style HP boxes at the screen edges)",wideBoxV);}return wideBoxV;}
extern "C" int P3DS_WideBoxExtra(void){static int lg=-1;const int e=(wideBoxCfg()&&wideCfg>0&&P3DS_ScaleMode()==1)?32:0;if(e!=lg){lg=e;PSPNativeMemLog("[WIDE] HP box offset for new battles: %d px (wide_boxes=%d wide=%d scale=%d)",e,wideBoxV,wideCfg,P3DS_ScaleMode());}return e;}
static int wnOverlayDirty; /* the HP-box overlay drew into P3DS_WideFillBuf last frame: wideNative must redraw (a reused strip kept the old sprite pixels stuck on the side) */
static void wideBoxOverlay(){wnOverlayDirty=0;static WcPlan plan;static u32 st[64*192];static unsigned nF,nL,nPx;
 const u8*R=(const u8*)PSPNative_GfxRegisters;
 WC_Plan(&plan,R,nullptr,0);const int why=WC_Check(&plan,R,GPU3D::NativeFrame!=nullptr);
 {static int lastWhy=-2;if(why!=lastWhy){lastWhy=why;PSPNativeMemLog("[WIDE] HP box overlay: screen %s (WC_Check %d, DISPCNT %08X)",why?"not drawable, strips as wide_battle":"drawable",why,*(const u32*)R);}}
 if(why)return;
 const WcSrc ws={GPU::VRAMFlat_ABG,GPU::VRAMFlat_AOBJ,GPU::VRAMFlat_ABGExtPal,GPU::VRAMFlat_AOBJExtPal,(const u16*)GPU::Palette,(const u16*)(GPU::Palette+512),(const u16*)GPU::OAM,GPU3D::NativeFrame};
 /* fps_tweaks >= 5 (run 33 log: this compose cost 10-14 ms a frame in stereo battles, wide prep 12.5 ms vs 1 ms with the boxes off): the HP boxes change only when
    HP, text or a window changes, so when every input of the compose (2D registers, OAM, both palettes, BG and OBJ VRAM) hashes to last frame's value the previous result is reused. */
 static unsigned hLast,hHit,hMiss;static int nLast=-1;static u64 hUs;int n;bool reuse=false;unsigned h=0;
 if(P3DS_FpsTweaks()>=5){const u64 th=svcGetSystemTick();h=2166136261u;
  auto hs=[&](const void*q,unsigned bytes){const u32*w=(const u32*)q;for(unsigned i=0;i<bytes/4;i++)h=(h^w[i])*16777619u;};
  /* run 62: only the sprites that reach into the strips count (WC_StripHash); the old all-OAM/palette/VRAM hash missed on every attack-animation frame */
  (void)hs;h=WC_StripHash(R,&ws);hUs+=(svcGetSystemTick()-th)/CPU_TICKS_PER_USEC;
  reuse=nLast>=0&&h==hLast&&((hHit+hMiss)&7)!=7;}  /* a full recompose at least every 8th frame bounds anything the hash does not cover (BG VRAM) */
 if(reuse){n=nLast;hHit++;}else{memset(st,0,sizeof st);n=WC_ComposeObjStrips(&plan,R,&ws,st);if(P3DS_FpsTweaks()>=5){nLast=n;hLast=h;hMiss++;}}
 if(P3DS_FpsTweaks()>=5&&(hHit+hMiss)%600==0&&(hHit+hMiss)){PSPNativeMemLog("[WIDE] HP box cache (level 5): reused %u, recomposed %u, hash %u us per frame",hHit,hMiss,(unsigned)(hUs/(hHit+hMiss)));}
 if(!n)return;
 unsigned px=0;for(int i=0;i<64*192;i++)if((st[i]&255)==255){P3DS_WideFillBuf[i]=st[i];px++;}
 wnOverlayDirty=px>0;nF++;nL+=n;nPx+=px;if(nF==1||nF%600==0){PSPNativeMemLog("[WIDE] HP box overlay: %u frames, last %d lines, %u strip px (avg lines %u, px %u)",nF,n,px,nL/nF,nPx/nF);}}
static unsigned wnUs,wnN,wnLay[5];
/* 2026-11-04 (Nathan: wide view must look right all the time): the black blocks of the battle-intro wipe (and other black 2D covers)
   are a BG layer in front of the 3D, so the 3D side strips kept showing the scene next to a black picture edge. True when the topmost
   opaque text-BG pixel in front of BG0 at the picture's edge pixel (x 0 / 255) of row y is black. */
/* topmost opaque text-BG pixel at the picture's edge (x 0 / 255) of row y among layers with priority < maxPrio (BG0 skipped when it
   is the 3D layer); returns its layer and RGB555 colour, -1 when none */
static int edgeBgTop(int y,int side,int maxPrio,u16*colOut){
 const u32 dc=*(volatile u32*)PSPNative_GfxRegisters;const unsigned mode=dc&7;if(mode>5)return -1;const unsigned wm=winEdge(y,side);const int x=side?255:0;
 for(int pr=0;pr<maxPrio;pr++)for(int i=0;i<4;i++){if(!(dc&(0x100u<<i))||!((wm>>i)&1)||(i==0&&(dc&8)))continue;const bool text=i<2||mode==0||(i==2&&mode==1);if(!text)continue;
  const unsigned c=reg16(8+i*2);if((int)(c&3)!=pr)continue;
  const bool bpp8=(c&0x80)!=0;const int w=(c>>14)&1?64:32,h=(c>>14)&2?64:32;
  const unsigned charb=((c>>2)&15)*0x4000+((dc>>24)&7)*0x10000,mapb=((c>>8)&31)*0x800+((dc>>27)&7)*0x10000;
  const unsigned px=(unsigned)(x+(int)(reg16(0x10+i*4)&0x1FF))&(unsigned)(w*8-1),py=(unsigned)(y+(int)(reg16(0x12+i*4)&0x1FF))&(unsigned)(h*8-1);
  const unsigned tx=px>>3,ty=py>>3,ma=(mapb+((tx>>5)+(ty>>5)*(unsigned)(w>>5))*0x800+((ty&31)*32+(tx&31))*2)&0x7FFFF;
  const u16 me=*(const u16*)(GPU::VRAMFlat_ABG+ma);unsigned fx=px&7,fy=py&7;if(me&0x400)fx=7-fx;if(me&0x800)fy=7-fy;
  const u8*t=GPU::VRAMFlat_ABG+((charb+(me&0x3FF)*(bpp8?64:32)+fy*(bpp8?8:4))&0x7FFFF);unsigned ci;if(bpp8)ci=t[fx];else{const u8 b=t[fx>>1];ci=(fx&1)?b>>4:b&15;}
  if(!ci)continue;
  const int ext=(bpp8&&(dc&(1u<<30)))?((i<2&&(c&0x2000))?i+2:i):-1;
  *colOut=ext>=0?*(const u16*)(GPU::VRAMFlat_ABGExtPal+ext*0x2000+((me>>12)*256+ci)*2):((const u16*)GPU::Palette)[bpp8?ci:((me>>12)*16+ci)];
  return i;}
 return -1;}
/* 2026-11-04 (Nathan: wide view must look right all the time): the black blocks of the battle-intro wipe (and other black 2D covers)
   are a BG layer in front of the 3D, so the 3D side strips kept showing the scene next to a black picture edge. True when the topmost
   opaque text-BG pixel in front of BG0 at the picture's edge pixel (x 0 / 255) of row y is black. */
static bool edgeBgBlack(int y,int side){
 const u32 dc=*(volatile u32*)PSPNative_GfxRegisters;if(!(dc&8))return false;const int p0=(int)(reg16(8)&3);if(!p0)return false;
 u16 col;return edgeBgTop(y,side,p0,&col)>=0&&(col&0x7FFF)==0;}
static u8 wnUnres[64*192];static u32 wnClean[64*192];static int wnCleanOk;static unsigned wnRestores;static void wideNative(){const u64 t0=svcGetSystemTick();
 const u8*R=(const u8*)PSPNative_GfxRegisters;const u32 dc=*(const u32*)R;const unsigned mode=dc&7;
 const u8*vram=GPU::VRAMFlat_ABG;const u16*pal=(const u16*)GPU::Palette;const u8*ext=GPU::VRAMFlat_ABGExtPal;
 struct L{int on,prio,bpp8,w,h,extSlot,clamp;unsigned charb,mapb,hofs,vofs;u8 per[64][2];u8 src[64][2];};static L Ly[4];int order[4],no=0;
 auto ent=[&](const L&l,unsigned tx,unsigned ty)->u16{tx&=l.w-1;ty&=l.h-1;const unsigned ma=(l.mapb+((tx>>5)+(ty>>5)*(l.w>>5))*0x800+((ty&31)*32+(tx&31))*2)&0x7FFFF;return *(const u16*)(vram+ma);};
 for(int i=0;i<4;i++){L&l=Ly[i];l.on=0;l.clamp=0;if(!(dc&(0x100u<<i)))continue;if(i==0&&(dc&8))continue;
  const bool text=i<2||mode==0||(i==2&&mode==1);if(!text||mode>5)continue;
  const u16 c=*(const u16*)(R+8+i*2);l.prio=c&3;l.bpp8=(c&0x80)!=0;l.w=(c>>14)&1?64:32;l.h=(c>>14)&2?64:32;
  l.charb=((c>>2)&15)*0x4000+((dc>>24)&7)*0x10000;l.mapb=((c>>8)&31)*0x800+((dc>>27)&7)*0x10000;
  l.hofs=*(const u16*)(R+0x10+i*4)&0x1FF;l.vofs=*(const u16*)(R+0x12+i*4)&0x1FF;
  /* run 73 shake hold: a layer whose horizontal scroll reverses direction (attack screen shake, ~2-8 px back and
     forth) keeps its last steady scroll in the strips, so they stay still instead of jittering; real scrolls
     (same direction twice) and changes > 24 px follow at once, and a value held 6 frames becomes the steady one. */
  {static unsigned st[4],last[4],hold[4];static int pd[4];int d=(int)((l.hofs-last[i])&0x1FF);if(d>=256)d-=512;
   if(!d){if(++hold[i]>=6)st[i]=l.hofs;}else{hold[i]=0;int dd=(int)((l.hofs-st[i])&0x1FF);if(dd>=256)dd-=512;
    if((d>0)==(pd[i]>0)||dd>24||dd<-24)st[i]=l.hofs;pd[i]=d;}
   last[i]=l.hofs;l.hofs=st[i];}
  l.extSlot=(l.bpp8&&(dc&(1u<<30)))?((i<2&&(c&0x2000))?i+2:i):-1;
  if(l.w==32){/* per visible tile row and side: the period (1, 2 or 4 tiles) of the map entries at that screen edge,
     0 = not periodic there (layer left out of that row's strip). The strip repeats those edge tiles. */
   const unsigned E0=l.hofs>>3,E1=(l.hofs+255)>>3;int any=0;
   for(unsigned r=0;r<25;r++){const unsigned ty=(l.vofs>>3)+r;
    for(int side=0;side<2;side++){int got=0;for(int pp=1;pp<=4&&!got;pp<<=1){bool ok=true;
      for(int k=0;k<pp&&ok;k++){if(side==0){if(ent(l,E0+k,ty)!=ent(l,E0+k+pp,ty))ok=false;}else{if(ent(l,E1-k,ty)!=ent(l,E1-k-pp,ty))ok=false;}}
      if(ok)got=pp;}
     l.per[r][side]=(u8)got;l.src[r][side]=(u8)r;any|=got;}}
   /* run 18 (wide_pokedex / wide_pc / wide_menu_screens on HGSS screens): their backgrounds are pictures, not repeating
      tiles, so no edge is periodic and the strips stayed black. Such a layer is kept in "clamp" mode: each row repeats
      the layer's outermost screen pixel (x 0 / 255), continuing the background's colours outwards. Menu screens only. */
   if(!any){if(!(P3DS_InWideKind>0&&!P3DS_InBattle))continue;l.clamp=3;for(int r=0;r<64;r++){l.per[r][0]=l.per[r][1]=1;l.src[r][0]=l.src[r][1]=(u8)r;}}}
  l.on=1;}
 for(int pr=0;pr<4;pr++)for(int i=0;i<4;i++)if(Ly[i].on&&Ly[i].prio==pr)order[no++]=i;
 /* run 73: only the back-most layer (the screen's background) continues into the strips; 32-wide front layers
    (panels, number rows, list boxes) touching the edge were repeated as rows of '0's / cut-off panels. 64-wide
    layers have real content there and stay. */
 {int k=0;for(int oi=0;oi<no;oi++){const L&l=Ly[order[oi]];if(oi==no-1||(l.w==64&&!P3DS_InBattle))order[k++]=order[oi];}no=k;} /* battle: a 64-wide front layer (trainer picture sliding in/out) showed its wrapped copies in the strips */
 {static int lk=-1;if(lk!=P3DS_InWideKind){lk=P3DS_InWideKind;PSPNativeMemLog("[WIDE] strips for screen kind %d: %d layer(s)%s, mode %u dispcnt %08x",lk,no,no&&Ly[order[no-1]].clamp?" (back layer clamped)":"",mode,(unsigned)dc);}}
 wnLay[no]++;
 /* the back-most layer (the screen's background) also fills rows where a panel touches the edge: those rows borrow
    the nearest row whose edge is periodic, so the background continues behind the panels instead of a black gap */
 if(no&&Ly[order[no-1]].w==32){L&b=Ly[order[no-1]];for(int side=0;side<2;side++)for(int r=0;r<25;r++)if(!b.per[r][side]){
   for(int d=1;d<25;d++){int a=r-d,c2=r+d;if(a>=0&&b.per[a][side]&&b.src[a][side]==a){b.per[r][side]=b.per[a][side];b.src[r][side]=(u8)a;break;}
    if(c2<25&&b.per[c2][side]&&b.src[c2][side]==c2){b.per[r][side]=b.per[c2][side];b.src[r][side]=(u8)c2;break;}}}}
 /* run 18: a side of the back layer with no periodic row at all (HGSS menu screens: a panel runs off that edge) is clamped
    like a non-periodic layer above, menu screens only */
 if(no&&Ly[order[no-1]].w==32&&P3DS_InWideKind>0&&!P3DS_InBattle){L&b=Ly[order[no-1]];for(int side=0;side<2;side++){bool any=false;for(int r=0;r<25;r++)any|=b.per[r][side]!=0;
  if(!any){b.clamp|=1<<side;for(int r=0;r<25;r++){b.per[r][side]=1;b.src[r][side]=(u8)r;}}}}
 /* skip the redraw when nothing the strips use changed: registers, palette, the edge map entries and their tiles */
 {u32 h=2166136261u;auto H=[&](u32 v){h=(h^v)*16777619u;};H(dc);for(unsigned k=0x40;k<0x4C;k+=2)H(reg16(k));if(P3DS_InBattle)for(int y=0;y<192;y++)for(int sd=0;sd<2;sd++){u16 ec=0;const int tl=edgeBgTop(y,sd,4,&ec);H((u32)(tl+1)|((u32)ec<<8));} /* the black front-layer rows above */for(int k=0;k<0x20;k+=4)H(*(const u32*)(R+8+k));H(*(const u32*)(R+0x50));H(*(const u32*)(R+0x54));
  {const u32*pp32=(const u32*)pal;for(int k=0;k<128;k++)H(pp32[k]);}
  for(int oi=0;oi<no;oi++){const L&l=Ly[order[oi]];if(l.extSlot>=0){const u32*e=(const u32*)(ext+l.extSlot*0x2000);for(int k=0;k<2048;k++)H(e[k]);}
   const unsigned E0=l.hofs>>3,E1=(l.hofs+255)>>3;
   for(unsigned r=0;r<25;r++)for(int side=0;side<2;side++){int pp=l.w==32?l.per[r][side]:5;if(!pp)continue;H(pp|(l.w==32?l.src[r][side]<<8:0));
    const unsigned ty=(l.vofs>>3)+(l.w==32?l.src[r][side]:r);
    for(int k=0;k<pp;k++){unsigned mc=l.w==32?(side?E1-k:E0+k):(side?E1+1+k:E0-1-k);u16 me=ent(l,mc,ty);H(me);
     const u32*t=(const u32*)(vram+((l.charb+(me&0x3FF)*(l.bpp8?64:32))&0x7FFFF));for(int q=0;q<(l.bpp8?16:8);q++)H(t[q]);}}}
  /* 2026-11-04 (solid-30 fps thread): same inputs but the HP-box overlay drew over last frame's strips: put back the copy kept
     from the draw that had this hash (what a redraw would give) instead of redrawing (~3 ms a frame in wide battles) */
  static u32 lastH;static unsigned skips;if(h==lastH&&wnOverlayDirty&&wnCleanOk){memcpy(P3DS_WideFillBuf,wnClean,sizeof wnClean);wnRestores++;wnOverlayDirty=0;}
  if(h==lastH&&!wnOverlayDirty){skips++;wnUs+=(unsigned)((svcGetSystemTick()-t0)/CPU_TICKS_PER_USEC);if(++wnN==600){PSPNativeMemLog("[WIDE] wide_menus avg %u us/frame (%u of 600 reused, %u of them restored under the HP boxes)",wnUs/600,skips,wnRestores);wnUs=wnN=skips=wnRestores=0;}return;}lastH=h;wnCleanOk=0;}
 static u32 lutBg[256];for(int k=0;k<256;k++){u16 cc=pal[k];u32 r=cc&31,g=(cc>>5)&31,b=(cc>>10)&31;lutBg[k]=(((r<<3)|(r>>2))<<24)|(((g<<3)|(g>>2))<<16)|(((b<<3)|(b>>2))<<8)|0xFF;}
 const u32 bd=lutBg[0];
 /* BLDCNT brighten / darken (effect 2/3, BLDY) on the 1st-target layers / backdrop, like the centre. A fade that targets
    BG0 is applied to the whole strip by gpuscale strips() instead (wide_fadefix), so it is not done twice. */
 const u16 bc=*(const u16*)(R+0x50);unsigned bY=*(const u16*)(R+0x54)&31;if(bY>16)bY=16;const int bEff=(!(bc&1)&&((bc>>6)&3)>=2&&bY)?(int)((bc>>6)&3):0;
 for(int y=0;y<192;y++)for(int side=0;side<2;side++){u32*o=P3DS_WideFillBuf+y*64+side*32;u32 done=0;const int x0=side?256:-32;u8 lay[32];
  const unsigned wmE=winEdge(y,side);
  /* 2026-11-04: a black front layer at the picture's edge (battle intro band, black frames) continues into the strip row, like
     the field strips (edgeBgBlack); only the back-most layer is drawn into the strips otherwise */
  if(P3DS_InBattle&&no){u16 ec;const int tl=edgeBgTop(y,side,4,&ec);if(tl>=0&&tl!=order[no-1]&&(ec&0x7FFF)==0){for(int xi=0;xi<32;xi++){o[xi]=0x000000FFu;wnUnres[y*64+side*32+xi]=0;}continue;}}
  for(int oi=0;oi<no&&done!=0xFFFFFFFFu;oi++){const L&l=Ly[order[oi]];if(!((wmE>>order[oi])&1))continue;const unsigned py=(unsigned)(y+l.vofs)&(l.h*8-1);const unsigned wm=l.w*8-1;
   int pp=0;const int E0=(int)(l.hofs>>3),E1=(int)((l.hofs+255)>>3);
   unsigned pyU=py;if(l.w==32){const unsigned r=((unsigned)(y+(l.vofs&7)))>>3;pp=r<25?l.per[r][side]:0;if(!pp)continue;pyU=((((l.vofs>>3)+l.src[r][side])*8)+(py&7))&(l.h*8-1);}
   u16 me=0;const u8*trow=0;int hf=0;int lastT=0x7FFFFFFF;
   for(int xi=0;xi<32;xi++){if(done&(1u<<xi))continue;const int lx=((l.clamp>>side)&1)?(side?255:0)+(int)l.hofs:x0+xi+(int)l.hofs;const int t=lx>>3;
    if(t!=lastT){lastT=t;unsigned mc;
     if(!pp||((l.clamp>>side)&1))mc=(unsigned)t;else if(side==0){int d=(t-E0)%pp;if(d<0)d+=pp;mc=(unsigned)(E0+d);}else{const int b0=E1-pp+1;int d=(t-b0)%pp;if(d<0)d+=pp;mc=(unsigned)(b0+d);}
     me=ent(l,mc,pyU>>3);unsigned fy=pyU&7;if(me&0x800)fy=7-fy;hf=(me&0x400)!=0;
     trow=vram+((l.charb+(me&0x3FF)*(l.bpp8?64:32)+fy*(l.bpp8?8:4))&0x7FFFF);}
    unsigned fx=(unsigned)lx&7;if(hf)fx=7-fx;unsigned ci;if(l.bpp8)ci=trow[fx];else{u8 b=trow[fx>>1];ci=(fx&1)?b>>4:b&15;}
    (void)wm;if(!ci)continue;
    if(l.extSlot>=0){u16 cc=*(const u16*)(ext+l.extSlot*0x2000+((me>>12)*256+ci)*2);u32 r=cc&31,g=(cc>>5)&31,b=(cc>>10)&31;o[xi]=(((r<<3)|(r>>2))<<24)|(((g<<3)|(g>>2))<<16)|(((b<<3)|(b>>2))<<8)|0xFF;}
    else o[xi]=lutBg[l.bpp8?ci:((me>>12)*16+ci)];
    done|=1u<<xi;lay[xi]=(u8)order[oi];}}
  for(int xi=0;xi<32;xi++){wnUnres[y*64+side*32+xi]=!(done&(1u<<xi));if(!(done&(1u<<xi))){o[xi]=bd;lay[xi]=5;}}
  if(bEff){for(int xi=0;xi<32;xi++)if(bc&(1u<<lay[xi])){u32 c=o[xi];unsigned r=c>>24,g=(c>>16)&255,b=(c>>8)&255;
    if(bEff==2){r+=(255-r)*bY/16;g+=(255-g)*bY/16;b+=(255-b)*bY/16;}else{r-=r*bY/16;g-=g*bY/16;b-=b*bY/16;}o[xi]=(r<<24)|(g<<16)|(b<<8)|0xFF;}}}
 memcpy(wnClean,P3DS_WideFillBuf,sizeof wnClean);wnCleanOk=1;
 wnUs+=(unsigned)((svcGetSystemTick()-t0)/CPU_TICKS_PER_USEC);if(++wnN==600){PSPNativeMemLog("[WIDE] wide_menus avg %u us/frame | layers in strips 0:%u 1:%u 2:%u 3:%u 4:%u",wnUs/600,wnLay[0],wnLay[1],wnLay[2],wnLay[3],wnLay[4]);wnUs=wnN=0;memset(wnLay,0,sizeof wnLay);}
}
/* run 57 stereo_strength (menu "3D STRENGTH"): 0 = 1x (original, 6 DS px per eye at full slider), 1 = 1.5x, 2 = 2x.
   Scales the per-vertex parallax of the whole 3D world (overworld billboards/NPCs are 3D, so they follow) and the
   stereo_2d battle BG offsets. Zero cost (one multiply per frame). */
static int stereoStr=-1;
/* run 61 stereo_2d: 1 = run-55 battle BG depth (prio 3 = 2x shift, prio 2 = 1.5x: up to 12 DS px between the eyes at
   full slider, far more than the battle Pokemon's own 3D disparity -> the Pokemon looked doubled against its
   background/platform); 2 (default) = gentle (prio 3 = 0.5x, prio 2 = 0.25x); 0 = flat 2D. */
extern "C" int P3DS_BattleDepthLock(void);
static int stereo2dV=-1;static int stereo2dCfg(){if(stereo2dV<0){stereo2dV=P3DS_CfgInt("stereo_2d",3);if(stereo2dV<0||stereo2dV>4)stereo2dV=3;PSPNativeMemLog("[STEREO] stereo_2d=%d (0 flat, 1 strong/run55, 2 gentle, 3 gentle+lock, 4 foe deeper)",stereo2dV);}return stereo2dV;}
/* run 61 gpu_compose_stereo=1 (default; only with gpu_compose=2): with the 3D slider up, overworld frames that are
   GPU-eligible are composed on the GPU for BOTH eyes (left with the left 3D layer, right with the right one; 2D layers
   identical, as on the CPU path in the field). Before, stereo always took the CPU path (~17 ms -> 25 fps). */
extern "C" int P3DS_BattleDepthLock(void){return P3DS_InBattle&&stereo2dCfg()>=3;}
/* 2026-11-18 issue #484 stereo_2d=4 FOE DEEPER (menu BATTLE 3D DEPTH, default stays 3): like GENTLE+LOCK, plus the foe's side
   sits further into the screen and yours nearer. Extra right-eye shift in DS px (left eye stays 0, as the lock): 3D quads get
   +e on the foe's side (screen x right of centre) and -e on yours, sliding smoothly in between (lunges, intro slide-ins); the
   back BG (priority 3) rows above kBattleSplitY (sky + foe's platform) get +e and the rows below (your platform) -e, so each Pokemon stays
   on its platform. e = (sd+1)/3: 2 px at full slider (foe 5 px, yours 1 px; GENTLE+LOCK has both at 3). */
extern "C" int P3DS_BattleSideExtra(int sd){return (P3DS_InBattle&&stereo2dCfg()==4&&sd>0)?(sd+1)/3:0;}
/* #589: FOE DEEPER as a ground plane, anchored on the HGSS battle layout (pret/pokeheartgold): after the intro the game bakes both
   platforms into BG3 (BattleSystem_SetBackground), so backdrop + platforms are ONE layer and only a per-row depth can separate them.
   Pokepic centres (ov07_022377F4 minus the 256 px slide-in): foe (176,50) -> feet ~y90-100, yours (80,112) -> feet ~y150-160;
   platform OBJ centres (ov12_0226E100): foe (176,88), yours (80,136).
   Total right-eye depth at the default strength (sd 6, full slider; scales with sd), + = into the screen:
     backdrop y<=48: 8 | foe's feet y~96: 5 | your feet y~160: 1 | bottom: 0.  HUD/text BGs and OBJ (HP boxes) stay at 0.
   So the backdrop sits 3 px behind the foe, the foe is 4 px deeper than yours (the #484 ask), and each Pokemon takes the depth of
   the row its feet are on (g3soft parSideOf): it stands ON its platform while the ground behind its body is deeper (#589 1-3).
   Returned as the extra over the GENTLE+LOCK base (sd+1)/2, per 8 px row block so CPU per-line and GPU per-tile-row BGs match. */
extern "C" int P3DS_BattleRowExtra(int y,int sd){if(!(P3DS_InBattle&&stereo2dCfg()==4&&sd>0))return 0;
 static const float ky[5]={0,48,96,160,192},kp[5]={8,8,5,1,0};
 float yc=(float)((y&~7)+4);if(yc<0)yc=0;if(yc>192)yc=192;int k=0;while(k<3&&yc>ky[k+1])k++;
 const float p=kp[k]+(kp[k+1]-kp[k])*(yc-ky[k])/(ky[k+1]-ky[k]);
 return (int)lrintf(p*(float)sd*(1.0f/6.0f))-(sd+1)/2;}
extern "C" {extern int P3DS_GcRowSd;extern int P3DS_GcRowOn[4];}
static int gcStNfV=-1;static int gcStNfCfg(){if(gcStNfV<0){gcStNfV=P3DS_CfgInt("gpu_compose_stereo_nf",1)!=0;PSPNativeMemLog("[GPUC] gpu_compose_stereo_nf=%d (battles/menus with the 3D slider on: GPU-compose both eyes; 0 = CPU, ~14-22 ms)",gcStNfV);}return gcStNfV;}
static int gcStereoV=-1;static int gcStereoCfg(){if(gcStereoV<0){gcStereoV=P3DS_CfgInt("gpu_compose_stereo",1)!=0;PSPNativeMemLog("[GPUC] gpu_compose_stereo=%d",gcStereoV);}return gcStereoV;}
/* run 76 dw_depth (Video tab, default 2 = moderate): in the Distortion World the 3D separation ramps up over ~1.5 s
   on entry (and back on exit): 0 off, 1 = x1.25, 2 = x1.5, 3 = x2. Only the stereo strength changes. */
extern "C" int P3DS_CurMap,P3DS_InField;static inline bool p3dsInDW(){return P3DS_InField&&P3DS_CurMap>=573&&P3DS_CurMap<=583;}
/* run 77 dw_roll=1 (default 0): on a Distortion World map change (new gravity plane) a ~400 ms damped tilt (row shear of
   the DS picture, both eyes) plus a small depth pulse. Off = no work at all. */
static u64 dwRollT;static int dwRollMap=-1,dwRollDir=1;
static float dwRollEnv(){if(!P3DS_CfgInt("dw_roll",0))return 0.0f;const int m=(P3DS_InField&&P3DS_CurMap>=573&&P3DS_CurMap<=583)?P3DS_CurMap:-1;
 if(m!=dwRollMap){if(m>=0&&dwRollMap>=0){dwRollT=svcGetSystemTick();dwRollDir=m>dwRollMap?1:-1;PSPNativeMemLog("[DW] camera roll %d -> %d",dwRollMap,m);}dwRollMap=m;}
 if(!dwRollT)return 0.0f;const float ms=(float)((svcGetSystemTick()-dwRollT)/CPU_TICKS_PER_MSEC);if(ms>400.0f){dwRollT=0;return 0.0f;}
 return expf(-ms/140.0f)*cosf(ms*(6.2831853f/400.0f));}
static void drawDwRoll(u32*img){const float e=dwRollEnv();if(e>-0.01f&&e<0.01f)return;topTouch();const float k=0.07f*e*dwRollDir;u32 row[256];
 for(int y=0;y<192;y++){const int sh=(int)(k*(y-96));if(!sh)continue;u32*r=img+y*256;memcpy(row,r,sizeof row);for(int x=0;x<256;x++){int sx=x-sh;sx=sx<0?0:sx>255?255:sx;r[x]=row[sx];}}}
static float P3DS_DwDepth(){static float f=1.0f;static u32 lastF=~0u;if(lastF==frames)return f;lastF=frames;
 static const float mul[4]={1.0f,1.25f,1.5f,2.0f};int c=P3DS_CfgInt("dw_depth",2);if(c<0||c>3)c=2;const float t=p3dsInDW()?mul[c]:1.0f;
 const float st=(mul[3]-1.0f)/45.0f;if(f<t)f=f+st>t?t:f+st;else if(f>t)f=f-st<t?t:f-st;{const float e=dwRollEnv();if(e>0.0f)return f*(1.0f+0.3f*e);}return f;}
static int stereoShiftF(int mode){ if(!mode)return 0; if(stereoStr<0){stereoStr=P3DS_CfgInt("stereo_strength",0);if(stereoStr<0||stereoStr>2)stereoStr=0;PSPNativeMemLog("[STEREO] stereo_strength=%d",stereoStr);}
 float s=p3dsSlider()*P3DS_DwDepth(); return (int)(s*6.0f*(stereoStr==2?2.0f:stereoStr==1?1.5f:1.0f)+0.5f); }
extern "C" u32 *P3DS_G3RightEye(void);
extern "C" unsigned long long P3DS_TickNow(void){return svcGetSystemTick();}
extern "C" { int P3DS_InField; int P3DS_WideCull; extern int G3G_Wide,G3G_StripValid; extern u32 G3G_Strip[2][64*192]; extern const u32 *P3DS_WideStrip[2]; }
extern "C" unsigned long long P3DS_SpkRomTicks,P3DS_SpkTexTicks,P3DS_SpkGetTexTicks;extern "C" unsigned P3DS_SpkRomBlocks,P3DS_SpkTexN;
/* per-vertex parallax amount for g3soft at frame start (DS px per eye); 0 = mono */
extern "C" int P3DS_StereoShiftNow(void){return ((reg16(0x304)&0x8000)&&(reg16(0x304)&2))?stereoShiftF(P3DS_ScaleMode()):0;}
static int stereoShift(int mode){ if(!mode)return 0; float s=p3dsSlider(); return (int)(s*4.0f+0.5f); }
/* run 38: mode-0 leftovers. After GPU-scaled modes (1/2): (a) gfxSet3D stayed on (gpuscale enabled it for stereo) and
   mode 0 never writes the RIGHT-eye framebuffer, so one eye kept showing the old scaled frame (= "eyes at two sizes"
   and "old larger frame behind the 1:1 image"); (b) citro3d's last queued transfer + swap could still land after the
   switch. Now on entering mode 0: wait for the citro3d queue, force 3D off, clear every top (L+R) and bottom buffer
   (both double-buffer halves); and in mode 0 the border around the 256x192 image is cleared every frame
   (mode0_border=1, default; cost ~0.1 ms). */
extern "C" void P3DS_GpuQuiesce(void);
static int mode0Border=-1;
static void clearBorder(u8 *fb, unsigned fbw){unsigned ox=(fbw-256)/2,oy=(240-192)/2;
 memset(fb,0,ox*240*3);memset(fb+(ox+256)*240*3,0,(fbw-ox-256)*240*3);
 for(unsigned x=ox;x<ox+256;x++){u8*c=fb+x*240*3;memset(c,0,oy*3);memset(c+(oy+192)*3,0,(240-oy-192)*3);}}
static void clearAllFbs(){for(int k=0;k<2;k++){
  u8*t=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,NULL,NULL);memset(t,0,400*240*3);
  u8*r=gfxGetFramebuffer(GFX_TOP,GFX_RIGHT,NULL,NULL);if(r&&r!=t)memset(r,0,400*240*3);
  u8*b=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,NULL,NULL);memset(b,0,320*240*3);
  gfxFlushBuffers();gfxSwapBuffers();gspWaitForVBlank();}}
static void present(gfxScreen_t scr, unsigned fbw, const u32 *src, int mode) {
 static int lastMode[2] = {-1, -1}, clearLeft[2]; int i = scr == GFX_TOP ? 0 : 1;
 int key = P3DS_ScaleMode() * 4 + mode; /* clear both fbs when the GPU-scaled mode is left too */
 if (lastMode[i] != key) { if (i == 0) { P3DS_GpuQuiesce(); clearAllFbs(); } lastMode[i] = key; clearLeft[i] = 2; }
 if (mode0Border < 0) { mode0Border = 1; FILE *f = fopen("sdmc:/3ds/platinum3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "mode0_border=%d", &x) == 1) mode0Border = x != 0; fclose(f); } PSPNativeMemLog("[3DS] mode0_border=%d", mode0Border); }
 u8 *fb = gfxGetFramebuffer(scr, GFX_LEFT, NULL, NULL);
 if (clearLeft[i]) { clearLeft[i]--; memset(fb, 0, fbw * 240 * 3); }
 else if (mode == 0 && mode0Border) clearBorder(fb, fbw);
 if (mode == 0) blit(fb, fbw, src);
 else if (mode == 2 && scr == GFX_TOP) blitScaled(fb, fbw, src, 400, 240);
 else blitScaled(fb, fbw, src, 320, 240);
}
extern "C" int PSPNativeRenderInit(){
 if(initialized)return 0;frames=0;BLINV;topCache.valid=false;previousPower=~0u;grace=0;engineA.Reset();engineB.Reset();
 initialized=true;return 0;
}
/* ---- run 55 settings menu: L+R+Y (menu_combo=1, default; 0 = off). Pauses the game (the game thread waits here
   inside Present), draws a native-size box over the last top-screen picture, D-pad/circle pad up/down = item,
   left/right/A = change, B / START / L+R+Y = close. Changes apply on close and are written to config.txt. ---- */
extern "C" { void P3DS_SetScaleMode(int); void P3DS_SetCstickMode(int); int P3DS_CstickMode(void); void P3DS_CfgWrite(const char*,int);
 int P3DS_CfgInt(const char*,int); void P3DS_SetLogLevel(int); int P3DS_LogLevel(void); extern int P3DS_CtlZr,P3DS_CtlZl,P3DS_CtlCp; volatile int P3DS_MenuOpen; }
static int menuCfg(){static int v=-1;if(v<0){v=P3DS_CfgInt("menu_combo",1)!=0;PSPNativeMemLog("[MENU] menu_combo=%d",v);}return v;}
static void menuBox(u32*img,int x0,int y0,int x1,int y1,u32 fill,u32 edge){for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++){if(x<0||x>255||y<0||y>191)continue;bool e=y<y0+2||y>=y1-2||x<x0+2||x>=x1-2;img[y*256+x]=e?edge:fill;}}
/* run 57: table-driven, scrolling (10 rows visible). Values are indexes into lab[]. */
extern "C" { int P3DS_GpuSwizzle(void); void P3DS_SetGpuSwizzle(int); int G3G_FastConv(void); void G3G_SetFastConv(int); int P3DS_CamZoom(void); void P3DS_SetCamZoom(int); int P3DS_FollowerCfg(void); void P3DS_SetFollowerCfg(int); }
/* run 61 menu_audio=1 (default): the music/sound driver keeps running while the menu is open (the game is still
   paused: only PSPNativeSoundAdvance, the same pump the game frame calls, with the real elapsed time). 0 = frozen. */
static int menuAudioV=-1;static int menuAudioCfg(){if(menuAudioV<0){menuAudioV=P3DS_CfgInt("menu_audio",1)!=0;PSPNativeMemLog("[MENU] menu_audio=%d",menuAudioV);}return menuAudioV;}
extern "C" void PSPNativeSoundAdvance(unsigned elapsedMicroseconds);
extern "C" { int P3DS_WideHazeCfg(void); void P3DS_SetWideHazeCfg(int); }
/* run 64 fast_text=0 (default off) / 2..4: characters per frame for the game's text printer (text.c; stops at
   every wait/prompt so nothing is skipped). Gameplay tab of the settings app. */
/* run 64 reusable_tms=1 (default on when the key is absent): 1 = teaching a TM does not use it up (party_menu callbacks.c TeachMove). */
static int reuseTmV=-1;extern "C" int P3DS_ReusableTMsCfg(void){if(reuseTmV<0){reuseTmV=P3DS_CfgInt("reusable_tms",0)!=0;PSPNativeMemLog("[ITEM] reusable_tms=%d",reuseTmV);}return reuseTmV;}
/* run 64 Cheats tab: cheat_no_encounters=0 (default off): walking in grass / caves / surfing never starts a random
   wild battle (overlay006 wild_encounters.c returns "no encounter" before rolling). Nothing is written to the save. */
extern "C" void P3DS_Use(const char*);static int noEncV=-1;extern "C" int P3DS_CheatNoEncCfg(void){if(noEncV<0){noEncV=P3DS_CfgInt("cheat_no_encounters",0)!=0;PSPNativeMemLog("[CHEAT] cheat_no_encounters=%d",noEncV);}return noEncV;}
static int repPV=-1;static int repelPrompt(void){if(repPV<0){repPV=P3DS_CfgInt("repel_prompt",0)!=0;PSPNativeMemLog("[REPEL] repel_prompt=%d",repPV);}return repPV;}
/* called by the game (patch 0040) each time the "use another Repel?" question would be asked: one [REPEL] line per event */
extern "C" int P3DS_RepelPromptCfg(void){const int v=repelPrompt();PSPNativeMemLog("[REPEL] repel check: prompt %s",v?"asked":"skipped (repel_prompt=0)");return v;}
/* ss3ds gameplay toggles read by the game source (pspoke patch 0040, patches/soulsilver/gameplay.patch). hm_free is a generic
   row (hm_free, index 50) so it follows the Settings app live; cheat_no_encounters skips the land / surf / fishing / Rock Smash
   checks, logging the first skipped check and every 25th after it. */
static int genGet(int g);enum{kHmFreeItem=50};
extern "C" int P3DS_UnionGuardCfg(void){return 0;} /* guard retired 2026-10-29: the Union Room works (run 59); union_guard in config.txt is ignored, no menu row */
extern "C" int P3DS_MenuGreyUnsupportedCfg(void){static int logged;int v=P3DS_CfgInt("menu_grey_unsupported",1)!=0;if(!logged){logged=1;PSPNativeMemLog("[MENU] menu_grey_unsupported=%d",v);}return v;}
extern "C" int P3DS_HmFreeCfg(void){static int last=-1;int v=genGet(kHmFreeItem-33)!=0;if(v!=last){last=v;PSPNativeMemLog("[HM] hm_free=%d",v);}return v;}
extern "C" int P3DS_NoEncSuppress(void){static unsigned n;if(!P3DS_CheatNoEncCfg())return 0;P3DS_Use("no_wild_battle_skips");if(n++%25==0)PSPNativeMemLog("[CHEAT] no wild encounters: skipped %u random encounter checks",n);return 1;}
static int sumIvV=-1;static int sumIvCfg(void){if(sumIvV<0){sumIvV=P3DS_CfgInt("summary_ivs",0)!=0;PSPNativeMemLog("[SUMMARY] summary_ivs=%d",sumIvV);}return sumIvV;}extern "C" int P3DS_SummaryIvsCfg(void){return sumIvCfg();}
/* ss3ds fast text (pspoke patch 0020, text.c RenderFontFast): characters printed per frame while a text box is
   printing. Menu index 0..3 = OFF / 2X / 4X / INSTANT <-> fast_text 0 / 2 / 4 / 64. Never skips a wait or prompt. */
static const int kFastTextN[4]={0,2,4,64};
static int fastTextIdx(int n){return n<2?0:n<=2?1:n<=4?2:3;}
static int fastTextV=-1;extern "C" int P3DS_FastTextCfg(void){if(fastTextV<0){fastTextV=P3DS_CfgInt("fast_text",0);fastTextV=kFastTextN[fastTextIdx(fastTextV)];PSPNativeMemLog("[TEXT] fast_text=%d",fastTextV);}return fastTextV;}
extern "C" int P3DS_TextCharsPerFrame(void){int n=P3DS_FastTextCfg();return n<1?1:n;}
struct MItem{const char*name;const char*key;int n;const char*lab[7];};
static const MItem mItems[]={
 {"FPS COUNTER","fps_counter",2,{"OFF","ON"}},{"WIDE VIEW","wideview",2,{"OFF","ON"}},{"SCALE","gpuscale",3,{"1:1","1.25X","FILL"}},
 {"C-STICK","cstick_mode",2,{"OFF","ON"}},{"ZL RUN TOGGLE","zl_run",2,{"OFF","ON"}},{"ZR = ITEM (Y)","zr_item",2,{"OFF","ON"}},
 {"CIRCLE PAD DPAD","cpad_dpad",2,{"OFF","ON"}},{"WIDE FX (SLOW)","wide_fx",2,{"OFF","ON"}},{"LOG DETAIL","log_level",2,{"SHORT","FULL"}},
 {"CAMERA ZOOM","cam_zoom",5,{"-2","-1","0","+1","+2"}},
 {"3D STRENGTH","stereo_strength",3,{"1X","1.5X","2X"}},
 {"FAST UPLOAD","gpu_swizzle",2,{"OFF","ON"}},{"FAST 3D CONVERT","fast_conv",2,{"OFF","ON"}},{"SPLIT TOP (MONO)","split_top",2,{"OFF","ON"}},
 {"FOLLOWING POKEMON","follower",2,{"OFF","ON"}},
 {"GPU COMPOSE","gpu_compose",4,{"OFF","ANALYZE","GPU","VERIFY"}},
 {"FOLLOWER SLOT","follower_slot",7,{"LEAD","1","2","3","4","5","6"}},{"FOLLOWER TALK","follower_talk",2,{"OFF","ON"}},
 {"MENU MUSIC","menu_audio",2,{"OFF","ON"}},{"GPU COMPOSE 3D","gpu_compose_stereo",2,{"OFF","ON"}},
 {"BATTLE 3D DEPTH","stereo_2d",5,{"FLAT","STRONG","GENTLE","GENTLE+LOCK","FOE DEEPER"}},{"WIDE HAZE","wide_haze",2,{"OFF","ON"}},
 {"FAST TEXT","fast_text",4,{"OFF","2X","4X","INSTANT"}},
 {"REUSABLE TMS","reusable_tms",2,{"OFF","ON"}},
 {"NO WILD ENCOUNTERS","cheat_no_encounters",2,{"OFF","ON"}},{"REPEL PROMPT","repel_prompt",2,{"OFF","ON"}},{"IV/EV ON SUMMARY","summary_ivs",2,{"OFF","ON"}},
 /* run 67 cheats 27.. (P3DS_Cheat(i-27)), all default OFF */
 {"INFINITE MONEY","cheat_money",2,{"OFF","ON"}},{"ALWAYS CATCH","cheat_catch",2,{"OFF","ON"}},{"SHINY WILD","cheat_shiny",2,{"OFF","ON"}},
 {"FAST EGGS","cheat_eggs",2,{"OFF","ON"}},{"EXP BOOST","cheat_exp",3,{"OFF","2X","3X"}},{"RARE CANDY 99","cheat_candy",2,{"OFF","ON"}},
 /* run 74: 33.. generic config rows (value = label index; log_max_kb mapped). Most apply after a restart. */
 {"FOLLOWER_AREA_TALK","follower_area_talk",2,{"OFF","ON"}},{"FOLLOWER_CRY","follower_cry",2,{"OFF","ON"}},{"FOLLOWER_ITEMS","follower_items",2,{"OFF","ON"}},{"FOLLOWER_BALLPOP","follower_ballpop",2,{"OFF","ON"}},{"FOLLOWER_EMOTES","follower_emotes",2,{"OFF","ON"}},{"FOLLOWER_FRIEND_TALK","follower_friend_talk",2,{"OFF","ON"}},{"FOLLOWER_IDLE","follower_idle",2,{"OFF","ON"}},{"FOLLOWER_GAP","follower_gap",2,{"OFF","ON"}},{"FOLLOWER_BEHIND","follower_behind",2,{"OFF","ON"}},{"WIDE_WEATHER","wide_weather",2,{"OFF","ON"}},{"WIDE_EDGEFILL","wide_edgefill",2,{"OFF","ON"}},{"WIDE_MENUS","wide_menus",2,{"OFF","ON"}},{"WIDE_FILL","wide_fill",2,{"OFF","ON"}},{"WILD_VIS","wild_vis",2,{"OFF","ON"}},{"WILD_VIS_MAX","wild_vis_max",5,{"0","1","2","3","4"}},{"WILD_VIS_REPLACE","wild_vis_replace",2,{"OFF","ON"}},{"WILD_VIS_TEST","wild_vis_test",2,{"OFF","ON"}},{"HM_FREE","hm_free",2,{"OFF","ON"}},{"SAVE_BACKUP","save_backup",2,{"OFF","ON"}},{"LOG_MAX_KB","log_max_kb",4,{"OFF","512 KB","1 MB","2 MB"}},{"CHEAT_PP","cheat_pp",2,{"OFF","ON"}},{"CHEAT_SPEED","cheat_speed",2,{"OFF","ON"}},{"CHEAT_WALK_WALLS","cheat_walk_walls",2,{"OFF","ON"}},{"CHEAT_TIME","cheat_time",5,{"OFF","MORNING","DAY","EVENING","NIGHT"}},{"CATCH_OVERLAY","catch_overlay",2,{"OFF","ON"}},{"BATTLE_ANIM_SPEED","battle_anim_speed",3,{"OFF","SKIP MOVES","SKIP + FAST HP"}},{"DW_DEPTH","dw_depth",4,{"OFF","SUBTLE","MODERATE","STRONG"}},{"DW_DEBRIS","dw_debris",2,{"OFF","ON"}},{"DW_UNEASE","dw_unease",2,{"OFF","ON"}},{"DW_CRY","dw_cry",2,{"OFF","ON"}},{"HUNT_COUNTER","hunt_counter",2,{"OFF","ON"}},{"TOD_TINT","tod_tint",2,{"OFF","ON"}},{"ITEM_PING","item_ping",2,{"OFF","ON"}},{"TOUCH_MENU","touch_menu",2,{"OFF","ON"}},{"DW_ROLL","dw_roll",2,{"OFF","ON"}},{"BOTTOM_STRIP","bottom_strip",2,{"OFF","ON"}},{"TEXT_HOLD_B","text_hold_b",2,{"OFF","ON"}},{"TEXT_AUTO","text_auto",2,{"OFF","ON"}},{"WIDE_LAYOUTS","wide_layouts",2,{"OFF","ON"}},{"SHINY_SUITE","shiny_suite",2,{"OFF","ON"}},{"WIDE_EARLY","wide_early",2,{"OFF","ON"}},{"WIDE_STEREO","wide_stereo",2,{"OFF","ON"}},{"WIDE_DIRTY","wide_dirty",3,{"OFF","ON","VERIFY"}},{"WIDE_DIRTY_REGS","wide_dirty_regs",2,{"OFF","ON"}},{"LOWER_MENU","lower_menu",2,{"OFF","ON"}},{"FIELD_PROF","field_prof",3,{"OFF","ON","AUTO"}},{"HEAP_FAST","heap_fast",2,{"OFF","ON"}},{"DC_FLUSH_SKIP","dc_flush_skip",2,{"OFF","ON"}},{"WIDE_3D_ALPHA","wide_3d_alpha",2,{"OFF","ON"}},{"HUNT_ASYNC_SAVE","hunt_async_save",2,{"OFF","ON"}},{"WIDE_SPLIT","wide_split",2,{"OFF","ON"}},{"WIRELESS","wireless",3,{"OFF","LOCAL","ONLINE"}},{"BATTLE_LOG","battle_log",2,{"OFF","ON"}},{"WIDE_SPLIT_R","wide_split_r",3,{"OFF","HALF","SHARED"}},{"G3_CONV_SPLIT","g3_conv_split",2,{"OFF","ON"}},{"STEREO_FLAT_FALLBACK","stereo_flat_fallback",2,{"OFF","ON"}},{"WIDE_WINDOWS","wide_windows",2,{"OFF","ON"}},{"STEREO_GPU_BATTLE","stereo_gpu_battle",2,{"OFF","ON"}},{"GPU_BLEND_3D","gpu_blend_3d",2,{"OFF","ON"}},{"GPU_3D_DIRECT","gpu_direct3d",2,{"OFF","ON"}},{"WIDE_GPU","wide_gpu",3,{"OFF","SLIDER UP","ALWAYS"}},{"GPU_MBRIGHT","gpu_mbright",2,{"OFF","ON"}},{"WIDE_GPU_PRECHECK","wide_gpu_precheck",2,{"OFF","ON"}},{"DEX_WIDE","dex_wide",2,{"OFF","ON"}},{"WIDE_STEREO_FLAT2D","wide_stereo_flat2d",2,{"OFF","ON"}},{"FRAME_PACING","frame_pacing",3,{"OFF","FIXED","ADAPTIVE"}},{"GC_ATLAS16","gc_atlas16",2,{"OFF","ON"}},{"EXP_SHARE_ALL","exp_share_all",2,{"OFF","ON"}},{"AUTO_RUN","auto_run",2,{"OFF","ON"}},{"CATCH_SWAP","catch_swap",2,{"OFF","ON"}},
 /* run 119 shiny suite extras 103..106, all default OFF */
 {"SHINY_RATE","shiny_rate",4,{"1/8192","1/4096","1/2048","1/1024"}},{"RNG_INFO","rng_info",2,{"OFF","ON"}},{"WILD_VIS_SHINY","wild_vis_shiny",2,{"OFF","ON"}},{"BATTERY_WARN","battery_warn",2,{"OFF","ON"}},
 /* run 120 skip intro / select reset 107..108, default OFF */
 {"RESET_SKIP_INTRO","reset_skip_intro",2,{"OFF","ON"}},{"SELECT_RESET","select_reset",2,{"OFF","ON"}},
 /* run 121 SoulSilver ports 109..113: speed tweaks, FAST 3D, audio style/stall fade (risky ones default OFF) */
 {"SPEED_TWEAKS","fps_tweaks",7,{"OFF","L2","L3","L4","L5","L6","L7"}},{"G3_SHARP","g3_sharp",4,{"OFF","SHARP","FAST 3D","FAST AUTO"}},
 {"AUDIO_STYLE","audio_limiter",4,{"OFF","CLEAN","LIGHT","DS RAW"}},{"AUDIO_STALLFADE","audio_stallfade",2,{"OFF","ON"}},{"AUDIO_GAIN","audio_gain",7,{"4","5","6","7","8","9","10"}},
 /* run 122 Batch 2: stereo depth lock, export save action */
 {"STEREO_DEPTH","stereo_depth",2,{"FLAT","DEPTH"}},{"EXPORT_SAVE","save_export",2,{"A: EXPORT","EXPORT ON CLOSE"}},
 /* run 123 Batch 3: type hint, ribbons, celebrate, achievements toasts, tracker, pm shortcuts (risky/default OFF) */
 {"TYPE_HINT","type_hint",2,{"OFF","ON"}},{"RIBBON_CHECK","ribbon_check",2,{"OFF","ON"}},{"CELEBRATIONS","celebrations",2,{"OFF","ON"}},
 {"ACH_TOASTS","ach_toasts",2,{"OFF","ON"}},{"TRACKER","tracker",2,{"OFF","ON"}},{"PM_SHORTCUTS","pm_shortcuts",2,{"OFF","ON"}},
 /* SoulSilver phase 2 122..123: battle Y damage check page, ZR hunt stats overlay (default OFF) */
 {"DMG_CHECK","dmg_check",2,{"OFF","ON"}},{"HUNT_STATS","hunt_stats",2,{"OFF","ON"}},
 /* 124: SoulSilver 0289 sharp game text (default OFF on Platinum until tested) */
 {"SHARP_GAME_TEXT","sharp_game_text",2,{"OFF","ON"}},
 /* 125..126: SoulSilver 0293 Mobile PC (quick menu Boxes tile after the Hall of Fame; dev skips that gate at runtime only) */
 {"MOBILE_PC","mobile_pc",2,{"OFF","ON"}},{"MOBILE_PC_DEV","mobile_pc_dev",2,{"OFF","ON"}},
 /* 127: SoulSilver 0298-0300 Black 2 animated sprites (needs b2sprites.bin from the builder) */
 {"B2_SPRITES","b2_sprites",2,{"OFF","ON"}},
 /* 128..129: SoulSilver 0294-0297 virtual GBA slot (p3ds_slot2.c): Migrate from / Pal Park, GBA-slot wild Pokemon */
 {"GBA_SLOT","slot2",2,{"OFF","ON"}},{"GBA_PAK","slot2_pak",6,{"1","2","3","4","5","6"}},
 /* 130..131: SoulSilver 0281-0283 visible wilds life (p3ds_follower.h), default OFF */
 {"WILD_VIS_ALIVE","wild_vis_alive",2,{"OFF","ON"}},{"WILD_VIS_CRIES","wild_vis_cries",2,{"OFF","ON"}},
 /* 132: SoulSilver 0269 GTS server picker */
 {"GTS_PICKER","gts_picker",2,{"OFF","ON"}},
 /* 133..134: SoulSilver ambient weather + seasonal trees (p3ds_follower.h, area_data.c) */
 {"AMBIENT_WEATHER","ambient_weather",2,{"OFF","ON"}},{"SEASONAL_TREES","seasonal_trees",2,{"OFF","ON"}},
 /* 135: SoulSilver event_any_mon, Platinum events */
 {"EVENT_ANY_MON","event_any_mon",2,{"OFF","ON"}},
};
#define MENU_N ((int)(sizeof mItems/sizeof mItems[0]))
extern "C" {int P3DS_B2Cfg(void);void P3DS_B2SetCfg(int);void P3DS_Slot2SetPak(int);}
extern "C" {void P3DS_PresentDeferSettle(void);int G3G_GpuTimeCfg(void);void G3G_SetGpuTime(int);void G3G_AbComposeWait(unsigned);}
extern "C" {int P3DS_GpuBlendCfg(void);void P3DS_SetGpuBlendCfg(int);}
enum{kGtsPickerItem=MENU_N-28,kWideLayItem=MENU_N-27,kG3ThrItem=MENU_N-26,kFps60Item=MENU_N-25,kFps60PipeItem=MENU_N-24,kFps60HoldItem=MENU_N-23,kFps60StripsItem=MENU_N-22,kFps60VramItem=MENU_N-21,kFps60AreaItem=MENU_N-20,kG3XfItem=MENU_N-19,kG3ProfItem=MENU_N-18,kG3WakeItem=MENU_N-17,kG3FastXfItem=MENU_N-16,kG3TexFenceItem=MENU_N-15,kG3TexSafeItem=MENU_N-14,kG3StereoShareItem=MENU_N-13,kPresentPaceItem=MENU_N-12,kG3FrontOvItem=MENU_N-11,kG3StereoGpuItem=MENU_N-10,kG3FrontBgItem=MENU_N-9,kGpuTimeItem=MENU_N-8,kFastTabsItem=MENU_N-7,kSharpGameItem=MENU_N-6,kMobilePcItem=MENU_N-5,kMobilePcDevItem=MENU_N-4,kMobilePcPreviewItem=MENU_N-3,kSlot2Item=MENU_N-2,kB2Item=MENU_N-1};
extern "C" {int P3DS_G3ThreadCfg(void);void P3DS_SetG3Thread(int);int P3DS_G3XfCacheCfg(void);void P3DS_SetG3XfCache(int);int P3DS_G3ProfCfg(void);void P3DS_SetG3Prof(int);int P3DS_G3WakeCfg(void);void P3DS_SetG3Wake(int);int P3DS_G3ThrCheckCfg(void);void P3DS_SetG3ThrCheck(int);int P3DS_G3FastXformCfg(void);void P3DS_SetG3FastXform(int);int P3DS_G3TexFenceCfg(void);void P3DS_SetG3TexFence(int);int P3DS_G3TexSafeCfg(void);void P3DS_SetG3TexSafe(int);int P3DS_G3StereoShareCfg(void);void P3DS_SetG3StereoShare(int);int P3DS_PresentPaceCfg(void);void P3DS_SetPresentPace(int);int P3DS_G3FrontOvCfg(void);void P3DS_SetG3FrontOv(int);int P3DS_G3StereoGpuCfg(void);void P3DS_SetG3StereoGpu(int);int P3DS_G3FrontBgCfg(void);void P3DS_SetG3FrontBg(int);}
/* fps60 (g3gpu.c, gpucomp.c, gpuscale.c): in-between pictures for the overworld */
extern "C"{int P3DS_Fps60Cfg(void);void P3DS_SetFps60(int);int P3DS_Fps60Active(void);void P3DS_Fps60Tick(unsigned,unsigned,unsigned,int,int,unsigned,unsigned);int G3G_MidReady(void);void*G3G_MidTex(int);int G3G_MidStrip(int,u32*);void G3G_SetFps60Pipe(int);void G3G_SetFps60Hold(int);void G3G_SetFps60Strips(int);void P3DS_SetFps60Vram(int);void P3DS_SetFps60Area(int);void P3DS_Fps60Area(unsigned,int);int G3G_FpsHold(void);
 u32*P3DS_GpuMidStripBuffer(int);int P3DS_GpuPresentMid(void*,void*,const u32*,const u32*,int,int);}
static int gWlDone; /* this frame's top picture and strips came from the wide layout compose (fades already in them) */
extern "C" {extern const char*P3DS_WideScreenName;extern int P3DS_WideShow[2];int P3DS_WideLayoutsOn(void);void P3DS_SetWideLayouts(int);int P3DS_WideGetParts(int,short*);}
enum{kGpuBlendItem=72};enum{kSharpItem=117};enum{kAchToastItem=118};enum{kMenuGreyItem=119};enum{kPanelPartyItem=120};enum{kPanelLiveItem=121};enum{kBreedItem=125};enum{kEventAnyItem=129};enum{kContestItem=133};enum{kSteadyItem=130};enum{kEvenItem=131};enum{kBatchCullItem=126};enum{kGcDirectItem=127};enum{kMenuBtnItem=73,kWideBattleItem=74,kWDepthItem=75,kBottomMenuItem=76,kWCpuItem=77,kSelectResetItem=78,kSkipIntroItem=79,kWideScreensItem=80,kSmoothItem=81,kBottomMapItem=82,kBottomLayoutItem=83,kPgKantoItem=84,kVolumeItem=85,kLimiterItem=86,kStallFadeItem=87,kGcNfItem=88,kDirectItem=89,kBottomHdItem=90,kWideDexItem=91,kWidePcItem=92,kWirelessItem=93,kFpsTweakItem=94,kWideBoxItem=95,kDcSkipItem=96,kWildVisItem=97,kWildLastItem=101,kFogHoleItem=102,kHuntItem=103,kRateItem=104,kBattItem=105,kRibbonItem=106,kCelebItem=107,kUnionGuardItem=108,kSaveExportItem=109,kTrackerItem=110,kHintItem=111,kPgExtrasItem=112,kPmItem=113,kAutoRunItem=114,kSprDepthItem=115,kBumpItem=116};enum{kHuntStatsItem=123};enum{kWildAliveItem=128};enum{kPgFindItem=124};enum{kFlyTapItem=132};enum{kMapZoomItem=140};enum{kRoamSprItem=141};/* (SoulSilver row-number static_assert dropped: Platinum keeps its own settings rows) */static int pmV=-1;extern "C" int P3DS_PmShortcutsCfg(void){if(pmV<0){pmV=P3DS_CfgInt("pm_shortcuts",0)!=0;PSPNativeMemLog("[PMENU] pm_shortcuts=%d",pmV);}return pmV;}extern "C" {int P3DS_FogHoleCfg(void);void P3DS_SetFogHole(int);}
extern "C" {extern volatile int P3DS_AudioGainV,P3DS_AudioLimiterV,P3DS_AudioStallFadeV;void P3DS_SetAudioGain(int);void P3DS_SetAudioLimiter(int);void P3DS_SetAudioStallFade(int);}
static const int kVolGain[7]={4,5,6,7,8,9,10};
static int volIdx(){int g=P3DS_AudioGainV;if(g<0)g=P3DS_CfgInt("audio_gain",7);int b=0;for(int k=1;k<7;k++)if(abs(kVolGain[k]-g)<abs(kVolGain[b]-g))b=k;return b;}
/* ss3ds 0060 bottom_menu (default OFF): Start-menu tiles drawn over the overworld touch screen. A tap sets P3DS_StartReq and
   presses X for the game; the hook in the game's start_menu.c then picks that entry exactly like a touch on the real menu. */
static int pxOnV=-1;
static bool pxOn(){
 if(pxOnV<0){
  const int mode=P3DS_CfgInt("pg_extras",1);
  pxOnV=mode==2?2:mode!=0;
  PSPNativeMemLog("[PHONE] pg_extras=%d (0 off, 1 original Phone/Radio, 2 previews)",pxOnV);
 }
 return pxOnV!=0;
}
static bool pxNativeGear(int pane){return pxOn()&&pxOnV==1&&(pane==1||pane==2);}
static int fastTabsV=-1;static bool fastTabsCfg(){if(fastTabsV<0){fastTabsV=P3DS_CfgInt("fast_tabs",1)!=0;PSPNativeMemLog("[PANEL] fast_tabs=%d (app tab to app tab picks the new app from the open Start menu)",fastTabsV);}return fastTabsV!=0;}
static int evenV=-1;static bool evenCfg(){if(evenV<0){evenV=P3DS_CfgInt("audio_even",1)!=0;PSPNativeMemLog("[AUDIO] audio_even=%d (slow loudness leveller + full-scale guard)",evenV);}return evenV!=0;}
extern "C" int P3DS_EvenLoudnessCfg(void){return evenCfg()?1:0;}
static int steadyV=-1;static bool steadyCfg(){if(steadyV<0){steadyV=P3DS_CfgInt("steady_music",1)!=0;PSPNativeMemLog("[AUDIO] steady_music=%d (field music is not faded by soundplates, hook 0226)",steadyV);}return steadyV!=0;}
extern "C" int P3DS_SteadyMusicCfg(void){return steadyCfg()?1:0;}
static int evtV=-1;static bool evtCfg(){if(evtV<0){evtV=P3DS_CfgInt("event_any_mon",0)!=0;PSPNativeMemLog("[EVENT] event_any_mon=%d (event scenes trigger on species alone, hook 0225)",evtV);}return evtV!=0;}
extern "C" int P3DS_EventAnyCfg(void){static int last=-1;int v=evtCfg()?1:0;if(v!=last){last=v;PSPNativeMemLog("[EVENT] event_any_mon=%d",v);}return v;}
static int brOnV=-1;static bool brCfg(){if(brOnV<0){brOnV=P3DS_CfgInt("breed_tab",0)!=0;PSPNativeMemLog("[BREED] breed_tab=%d (EGGS tab on the Pokegear panel, hook 0210)",brOnV);}return brOnV!=0;}
static int pfyV=-1;static bool pgFlyCfg(){if(pfyV<0){pfyV=P3DS_CfgInt("pg_flytap",0)!=0;PSPNativeMemLog("[FLY] pg_flytap=%d (FLY chip on the JOHTO map card, hook 0228)",pfyV);}return pfyV!=0;}
static int pgZoomIdxV=-1;static int pgMapZoom(){if(pgZoomIdxV<0){int v=P3DS_CfgInt("map_zoom",0);pgZoomIdxV=v>=1?1:0;PSPNativeMemLog("[PGMAP] map_zoom=%d (%dx)",pgZoomIdxV,pgZoomIdxV?2:1);}return pgZoomIdxV?2:1;}
static int pgMapZoomIdx(){pgMapZoom();return pgZoomIdxV;}
static void sharpTextSet(int v){v=v!=0;if(sharpTextCfg()==(v!=0))return;sharpTextV=v;P3DS_CfgWrite("sharp_text",v);BLINV;PSPNativeMemLog("[TEXT] sharp_text -> %d (%s)",v,v?"panel and settings text at 320x240":"old stretched text");}
static void pgSetZoom(int idx){if(idx<0)idx=0;if(idx>1)idx=1;if(pgZoomIdxV<0)pgMapZoom();if(idx==pgZoomIdxV)return;const int from=pgZoomIdxV?2:1;pgZoomIdxV=idx;P3DS_CfgWrite("map_zoom",idx);BLINV;PSPNativeMemLog("[PGMAP] zoom %dx -> %dx",from,idx?2:1);}
static int pgRoamV=-1;static bool pgRoamSpr(){if(pgRoamV<0){pgRoamV=P3DS_CfgInt("roamer_sprites",0)!=0;PSPNativeMemLog("[TRACK] roamer_sprites=%d (0 keeps the letter)",pgRoamV);}return pgRoamV!=0;}
static void pgSetRoamSpr(int v){v=v!=0;if(pgRoamV<0)pgRoamSpr();if(v==pgRoamV)return;pgRoamV=v;P3DS_CfgWrite("roamer_sprites",v);BLINV;PSPNativeMemLog("[TRACK] roamer_sprites=%d from settings",v);}
extern "C" int P3DS_FlyCfg(void){return pgFlyCfg()?1:0;}
static int pfOnV=-1;static bool pgFindCfg(){if(pfOnV<0){pfOnV=P3DS_CfgInt("pg_find",0)!=0;PSPNativeMemLog("[FIND] pg_find=%d (tap a wild-list name on the JOHTO map to pin where it lives, hook 0209 times)",pfOnV);}return pfOnV!=0;}
static int plOnV=-1;static bool panelLiveCfg(){if(plOnV<0){plOnV=P3DS_CfgInt("panel_live",0)!=0;PSPNativeMemLog("[LIVE] panel_live=%d (PARTY/BAG tabs act with the world running, hook 0207)",plOnV);}return plOnV!=0;}
static int ppOnV=-1;static bool panelPartyCfg(){if(ppOnV<0){ppOnV=P3DS_CfgInt("panel_party",0)!=0;PSPNativeMemLog("[PANEL] panel_party=%d (PARTY tab opens Pokemon menu in the JOHTO panel)",ppOnV);}return ppOnV!=0;}
static int pkV=-1;static int pkCfg(){if(pkV<0){pkV=P3DS_CfgInt("pg_kanto",0)!=0;PSPNativeMemLog("[PGMAP] pg_kanto=%d (0 gated, 1 always)",pkV);}return pkV;}
/* ss3ds 0096: the field's touch-screen mode, published every field frame by the hook in field_system.c (FieldSystem_Main).
   0 = the game's normal touch menu (what the Bottom menu replaces). Anything else is the game using the touch screen itself:
   3 = hidden by a script, where HGSS shows the YES/NO buttons (nurse, NPC questions) and the starter Poke Balls; 1 = Start
   menu; 2 = shop; 4 = dowsing/item. The Bottom menu and the menu button stay off while it is not 0, so those screens are
   visible and touchable (Nathan 2026-10-27: Yes/No boxes and starter balls invisible under the JOHTO map). Updated once per frame. */
extern "C" { volatile int P3DS_SubMode=-1; volatile unsigned P3DS_SubTick; }
static bool subBusyV;static const unsigned kBusyTicks=12;
static void subBusyUpdate(){static unsigned lastTick,stale=999,fieldFrames;static int lastMode=-2;static bool warned;
 const unsigned t=P3DS_SubTick;static unsigned busyTicks,peak;
 const int m=P3DS_SubMode;
 if(t!=lastTick){const unsigned d=t-lastTick>8?8:t-lastTick;lastTick=t;stale=0;
  /* run 50 (Nathan: old Start menu visible ~4 frames after walking out a door): the log shows the field touch-screen mode jumping 0 -> 3 -> 0 across the zone change (a
     script/warp holds it for a few frames), which hid the tiles and showed the game's own touch menu. A real YES/NO or starter-ball screen stays up, so the tiles only give way
     once the mode has held for kBusyTicks field frames. */
  if(m>0&&P3DS_InField){busyTicks+=d;if(busyTicks>peak)peak=busyTicks;}
  else{if(peak)PSPNativeMemLog("[BMENU] touch-screen mode was non-zero for %u field frames (%s)",peak,peak<kBusyTicks?"tiles stayed up, no old menu flash":"tiles were hidden");busyTicks=0;peak=0;}}
 else if(stale<999)stale++;
 const bool fresh=stale<30;
 subBusyV=P3DS_InField&&fresh&&m>0&&busyTicks>=kBusyTicks;
 if(fresh&&m!=lastMode){PSPNativeMemLog("[BMENU] game touch screen mode %d -> %d (%s)",lastMode,m,m>0?"game's own touch screen: Bottom menu hidden":"normal touch menu: Bottom menu allowed");lastMode=m;}
 if(P3DS_InField&&!warned&&!t&&++fieldFrames>600){warned=true;PSPNativeMemLog("[BMENU] no touch-screen mode from the game after 600 field frames: hook 0096 is not in this build (the Bottom menu can cover YES/NO buttons)");}}
static int bumpV=-1;static int bumpCfg(){if(bumpV<0){bumpV=P3DS_CfgInt("bottom_bump",1)!=0;PSPNativeMemLog("[BMENU] bottom_bump=%d",bumpV);}return bumpV;}
static int bmV=-1;static int bmCfg(){if(bmV<0){bmV=P3DS_CfgInt("bottom_menu",1)!=0;PSPNativeMemLog("[BMENU] bottom_menu=%d",bmV);}return bmV;}
static int menuBtnV=-1;static int menuBtnCfg(){if(menuBtnV<0){menuBtnV=P3DS_CfgInt("menu_button",1);if(menuBtnV<0||menuBtnV>2)menuBtnV=1;PSPNativeMemLog("[BTN] menu_button=%d",menuBtnV);}return menuBtnV;}
static const char*const kCheatKey[6]={"cheat_money","cheat_catch","cheat_shiny","cheat_eggs","cheat_exp","cheat_candy"};static int cheatV[6]={-1,-1,-1,-1,-1,-1};
extern "C" int P3DS_Cheat(int k){if(k<0||k>5)return 0;if(cheatV[k]<0){int v=P3DS_CfgInt(kCheatKey[k],0);cheatV[k]=v<0||v>(k==4?2:1)?0:v;if(cheatV[k])PSPNativeMemLog("[CHEAT] %s=%d",kCheatKey[k],cheatV[k]);}return cheatV[k];}
static const int kGenDef[]={1,1,0,1,1,1,1,1,1,1,1,1,0,0,2,0,0,0,1,1024,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,1,1,1,1,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,3,1,6,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1};static int genV[sizeof kGenDef/sizeof kGenDef[0]];static bool genInit;
static int genGet(int g){if(!genInit){genInit=true;for(int k=0;k<(int)(sizeof kGenDef/sizeof kGenDef[0]);k++){int v=P3DS_CfgInt(mItems[33+k].key,kGenDef[k]);if(!strcmp(mItems[33+k].key,"log_max_kb"))v=v<=0?0:v<=512?1:v<=1024?2:3;if(!strcmp(mItems[33+k].key,"fps_tweaks"))v=v<=0?0:v>=2&&v<=7?v-1:0;
   if(!strcmp(mItems[33+k].key,"audio_gain")){static const int kG[7]={4,5,6,7,8,9,10};int b=6;for(int t=0;t<7;t++)if(abs(kG[t]-v)<abs(kG[b]-v))b=t;v=b;}
   if(v<0)v=0;if(v>=mItems[33+k].n)v=mItems[33+k].n-1;genV[k]=v;}}return genV[g];}
static int menuGet(int i){if(i>=33)return genGet(i-33);if(i>=27)return P3DS_Cheat(i-27);switch(i){case 0:return fpsOn?1:0;case 1:return wideCfg>0?1:0;case 2:return P3DS_ScaleMode();case 3:return P3DS_CstickMode();
 case 4:return P3DS_CtlZl>0?1:0;case 5:return P3DS_CtlZr>0?1:0;case 6:return P3DS_CtlCp>0?1:0;case 7:return P3DS_WideFxCfg()?1:0;case 8:return P3DS_LogLevel()>=2?1:0;
 case 9:return P3DS_CamZoom()+2;case 10:return stereoStr<0?0:stereoStr;case 11:return P3DS_GpuSwizzle()?1:0;case 12:return G3G_FastConv()?1:0;case 13:return splitCfg>0?1:0;case 14:return P3DS_FollowerCfg()?1:0;case 15:return P3DS_GpuComposeCfg();
 case 16:return P3DS_FollowerSlotCfg();case 17:return P3DS_FollowerTalkCfg();case 18:return menuAudioCfg();case 19:return gcStereoCfg();case 20:return stereo2dCfg();case 21:return P3DS_WideHazeCfg();case 22:{int v=P3DS_FastTextCfg();return v<=1?0:v-1;}case 23:return P3DS_ReusableTMsCfg();case 24:return P3DS_CheatNoEncCfg();case 25:return P3DS_RepelPromptCfg();case 26:return sumIvCfg();}return 0;}
static void menuSet(int i,int v){if(i>=33){genGet(0);genV[i-33]=v;static const int kLog[4]={0,512,1024,2048};P3DS_CfgWrite(mItems[i].key,!strcmp(mItems[i].key,"log_max_kb")?kLog[v&3]:v);if(!strcmp(mItems[i].key,"wide_layouts"))P3DS_SetWideLayouts(v);if(!strcmp(mItems[i].key,"shiny_suite"))P3DS_HuntSetOn(v);if(!strcmp(mItems[i].key,"shiny_rate"))P3DS_HuntSetRate(v);if(!strcmp(mItems[i].key,"frame_pacing"))P3DS_SetFramePacing(v);
  if(!strcmp(mItems[i].key,"fps_tweaks")){P3DS_SetFpsTweaks(v==0?0:v+1);P3DS_CfgWrite("fps_tweaks",v==0?0:v+1);return;}
  if(!strcmp(mItems[i].key,"slot2")){P3DS_Slot2SetCfg(v);return;}
  if(!strcmp(mItems[i].key,"slot2_pak")){P3DS_Slot2SetPak(v);return;}
  if(!strcmp(mItems[i].key,"b2_sprites")){P3DS_B2SetCfg(v);return;}
  if(!strcmp(mItems[i].key,"mobile_pc")){P3DS_MobilePcSetCfg(v);return;}
  if(!strcmp(mItems[i].key,"mobile_pc_dev")){P3DS_MobilePcSetDev(v);return;}
  if(!strcmp(mItems[i].key,"sharp_game_text")){sharpGameV=v!=0;PSPNativeMemLog("[GTEXT] sharp_game_text -> %d",v);}
  if(!strcmp(mItems[i].key,"g3_sharp")){G3G_SetSharp(v);P3DS_CfgWrite("g3_sharp",v);return;}
  if(!strcmp(mItems[i].key,"audio_limiter")){P3DS_SetAudioLimiter(v);P3DS_CfgWrite("audio_limiter",v);return;}
  if(!strcmp(mItems[i].key,"audio_stallfade")){P3DS_SetAudioStallFade(v);P3DS_CfgWrite("audio_stallfade",v);return;}
  if(!strcmp(mItems[i].key,"audio_gain")){static const int kG[7]={4,5,6,7,8,9,10};P3DS_SetAudioGain(kG[v]);P3DS_CfgWrite("audio_gain",kG[v]);return;}
  if(!strcmp(mItems[i].key,"stereo_depth")){extern void P3DS_SetStereoDepth(int);P3DS_SetStereoDepth(v);P3DS_CfgWrite("stereo_depth",v);return;}
  if(!strcmp(mItems[i].key,"save_export")){if(v){extern int P3DS_SaveExport(void);P3DS_SaveExport();}genV[i-33]=0;return;}
  if(!strcmp(mItems[i].key,"type_hint")){P3DS_CfgWrite("type_hint",v);return;}
  if(!strcmp(mItems[i].key,"ribbon_check")){P3DS_RibbonSetOn(v);return;}
  if(!strcmp(mItems[i].key,"celebrations")){P3DS_CelebSetOn(v);return;}
  if(!strcmp(mItems[i].key,"ach_toasts")){P3DS_AchSetToasts(v);P3DS_CfgWrite("ach_toasts",v);return;}
  if(!strcmp(mItems[i].key,"tracker")){P3DS_TrkSetOn(v);return;}
  if(!strcmp(mItems[i].key,"pm_shortcuts")){P3DS_CfgWrite("pm_shortcuts",v);return;}
  return;}if(i>=27&&i<33)cheatV[i-27]=v;switch(i){case 0:fpsOn=v!=0;break;case 1:wideCfg=v;break;case 2:P3DS_SetScaleMode(v);break;case 3:P3DS_SetCstickMode(v);break;
  case 4:P3DS_CtlZl=v;if(!v){extern volatile int P3DS_RunToggle;P3DS_RunToggle=0;}break;case 5:P3DS_CtlZr=v;break;case 6:P3DS_CtlCp=v;break;case 7:wideFxV=v;break;case 8:P3DS_SetLogLevel(v?2:1);break;
  case 9:P3DS_SetCamZoom(v-2);break;case 10:stereoStr=v;break;case 11:P3DS_SetGpuSwizzle(v);break;case 12:G3G_SetFastConv(v);break;case 13:splitCfg=v;break;case 14:P3DS_SetFollowerCfg(v);break;case 15:P3DS_SetGpuComposeCfg(v);break;
  case 16:folSlotV=v;break;case 17:folTalkV=v;break;case 18:menuAudioV=v;break;case 19:gcStereoV=v;break;case 20:stereo2dV=v;break;case 21:P3DS_SetWideHazeCfg(v);break;case 22:fastTextV=kFastTextN[v&3];PSPNativeMemLog("[TEXT] fast_text=%d (from settings)",fastTextV);break;case 23:reuseTmV=v;break;case 24:noEncV=v;break;case 25:repPV=v;break;case 26:sumIvV=v;break;}
 if(i!=2)P3DS_CfgWrite(mItems[i].key,i==3?(v?2:0):i==8?(v?2:1):i==9?v-2:i==22?kFastTextN[v&3]:v);}
static void prettyLab(char*d,const char*s,int n);
static void runMenu(const u32*top,const u32*bot,int mode,bool gpu,bool drawBot){
 static u32 base[256*192],img[256*192];memcpy(base,top,sizeof base);
 enum{N=MENU_N};
 int val[N];for(int i=0;i<N;i++)val[i]=menuGet(i);
 int old[N];memcpy(old,val,sizeof old);int sel=0,top0=0;const int VIS=10;
 // dim the picture once
 for(int i=0;i<256*192;i++){u32 c=base[i];base[i]=((c>>1)&0x007F7F7Fu)|(c&0xFF000000u);}
 P3DS_MenuOpen=1;P3DS_LowerMenuDraw=0;PSPNativeMemLog("[MENU] open");P3DS_Toast("");toastAt=0;
 u32 held=hidKeysHeld();int rep=0;
 while(aptMainLoop()){
  hidScanInput();u32 kh=hidKeysHeld(),kd=hidKeysDown()&~held;held&=kh; /* ignore the opening combo until released */
  {circlePosition c;hidCircleRead(&c);static int cpPrev;int dir=c.dy>60?1:c.dy<-60?2:c.dx>60?3:c.dx<-60?4:0;if(dir&&dir!=cpPrev){kd|=dir==1?KEY_DUP:dir==2?KEY_DDOWN:dir==3?KEY_DRIGHT:KEY_DLEFT;}cpPrev=dir;}
  if(kh&(KEY_DUP|KEY_DDOWN)){if(++rep>18&&rep%5==0)kd|=kh&(KEY_DUP|KEY_DDOWN);}else rep=0;
  if(kd&(KEY_B|KEY_START))break;if((kd&KEY_Y)&&(kh&KEY_L)&&(kh&KEY_R))break;
  if(kd&KEY_DUP)sel=(sel+N-1)%N;if(kd&KEY_DDOWN)sel=(sel+1)%N;
  const int cn=mItems[sel].n;
  if(kd&(KEY_DRIGHT|KEY_A))val[sel]=(val[sel]+1)%cn;if(kd&KEY_DLEFT)val[sel]=(val[sel]+cn-1)%cn;
  if(sel<top0)top0=sel;if(sel>=top0+VIS)top0=sel-VIS+1;
  memcpy(img,base,sizeof img);
  const int bx=8,by=10,bw=240,bh=172;menuBox(img,bx,by,bx+bw,by+bh,0x00502010u,0x00F0F0F0u);
  if(useGF())P3DS_GameFontDraw(img,256,256,192,bx+8,by+3,"Settings (game paused)",0x0060E0FFu,0x00604020u,1,1);else
  drawBig(img,bx+8,by+6,"SETTINGS (PAUSED)",0x0060E0FFu);
  {char pg[16];snprintf(pg,sizeof pg,"%d/%d",sel+1,N);drawBig(img,bx+bw-8-(int)strlen(pg)*8,by+6,pg,0x00C0C0C0u);}
  for(int r=0;r<VIS&&top0+r<N;r++){const int i=top0+r;int y=by+22+r*13;if(i==sel)menuBox(img,bx+4,y-2,bx+bw-4,y+11,0x00A04820u,0x00A04820u);
   if(useGF()){char nm[32],lb[24],b[32];prettyLab(nm,mItems[i].name,sizeof nm);prettyLab(lb,mItems[i].lab[val[i]],sizeof lb);snprintf(b,sizeof b,"%s",lb);/* run 64: game font */
    P3DS_GameFontDraw(img,256,256,192,bx+8,y-3,nm,0x00FFFFFFu,0x00604020u,1,1);P3DS_GameFontDraw(img,256,256,192,bx+bw-8-P3DS_GameFontWidth(1,b),y-3,b,i==sel?0x0000FFFFu:0x00C0C0C0u,0x00604020u,1,1);continue;}
   drawBig(img,bx+8,y,mItems[i].name,0x00FFFFFFu);
   char b[16];snprintf(b,sizeof b,"<%s>",mItems[i].lab[val[i]]);drawBig(img,bx+bw-8-(int)strlen(b)*8,y,b,i==sel?0x0000FFFFu:0x00C0C0C0u);}
  if(useGF())P3DS_GameFontDraw(img,256,256,192,bx+8,by+bh-17,"A / Left / Right: change    B: close",0x00C0C0C0u,0x00604020u,1,1);else
  drawBig(img,bx+8,by+bh-14,"A/<> CHANGE  B CLOSE",0x00C0C0C0u);
  if(mode&&gpu)P3DS_GpuPresent(img,bot,mode,drawBot,nullptr);
  else{present(GFX_TOP,400,img,0);if(drawBot)present(GFX_BOTTOM,320,bot,0);gfxFlushBuffers();gfxSwapBuffers();}
  gspWaitForVBlank();
  if(menuAudioCfg()){static u64 mt;u64 nw=svcGetSystemTick();unsigned us=mt&&nw>mt?(unsigned)((nw-mt)/CPU_TICKS_PER_USEC):16667;mt=nw;if(us>66666)us=16667;PSPNativeSoundAdvance(us);}
 }
 /* wait until buttons are released so the closing press doesn't reach the game */
 for(int i=0;i<120&&aptMainLoop();i++){hidScanInput();if(!(hidKeysHeld()&(KEY_A|KEY_B|KEY_START|KEY_Y|KEY_L|KEY_R|KEY_DUP|KEY_DDOWN|KEY_DLEFT|KEY_DRIGHT)))break;gspWaitForVBlank();}
 for(int i=0;i<N;i++)if(val[i]!=old[i])menuSet(i,val[i]);
 P3DS_MenuOpen=0;PSPNativeMemLog("[MENU] closed");
 tPrevEnd=svcGetSystemTick();secStart=0;secFrames=secG3=secComp=secGpu=secGame=secTris=0;secG3G=0;
}
/* ---- run 64 (Big build 5a) settings_app=1 (default): L+R+Y opens a Poketch-style touch settings app on the bottom
   screen instead of the run 55 list (settings_app=0 = old list). Same model as runMenu: the game is paused inside
   Present (no game state touched, Poketch untouched; it is back as soon as the app closes), the same items/getters/
   setters, changes applied on close and written to config.txt. Tabs Display / Performance / Gameplay / Cheats
   (tap a tab again = next page), 2 x 5 cells: tap the cell / its arrows to change, D-pad + A also work, L/R = tab,
   B / START / L+R+Y close. Drawn in the game's font + message box frame (port font fallback). The top screen shows
   the dimmed game picture with the title and the build/version string. */
#ifndef P3DS_VERSION
#define P3DS_VERSION "v1.0 (run103)"
#endif
static const char*kAppVer="Platinum 3DS " P3DS_VERSION " (" __DATE__ ")";
#include "p3ds_areanames.h"
/* run 86: map header id -> name for the log ([ZONE], [SPIKE], [PERF3]) */
extern "C" const char*P3DS_AreaName(int id){return id>=0&&id<(int)(sizeof kP3dsAreaName/sizeof kP3dsAreaName[0])?kP3dsAreaName[id]:"-";}
#include "p3ds_abilnames.h"
extern "C" volatile int P3DS_CatchInfo[20];static int appStartTab=-1;
/* settings redesign (after run 88): 6 tabs (+ hidden Debug), each a one-per-line list (8 rows a page). Item ids >= L_HUNT are
   rows that open one of the full pages below the tab bar (Hunt stats = the old Ball tab, Wild Pokemon here = the old
   Map tab, Quick menu = the old Menu tab); B on such a page goes back to the row that opened it. */
enum{L_HUNT=1000,L_MAP,L_MENU,L_TRACK};
struct ATab{const char*name;int n;int it[40];const char*nm[40];};
/* run 100 (v1.0): one/two-line help for the selected setting (top screen). */
static const char*settingHelp(const char*k){static const char*const H[][2]={
 {"fps_counter","Frame rate in the top corner."},{"wideview","Fills the 400px top screen\nwith extra 3D view."},{"wide_layouts","Menus and battles laid out\nfor the wide screen."},
 {"gpuscale","Picture size on the top screen."},{"stereo_strength","3D depth (slider still applies)."},{"stereo_2d","Depth for battle scenes."},
 {"cam_zoom","Overworld camera distance."},{"tod_tint","Evening/night tint outdoors."},{"wide_weather","Rain/snow also in the side strips."},
 {"fast_text","Text prints at full speed."},{"text_hold_b","Hold B to rush text."},{"text_auto","Text advances by itself."},
 {"battle_anim_speed","Speeds up battle animations."},{"repel_prompt","Asks to use another Repel."},{"reusable_tms","TMs are not used up."},
 {"hm_free","Field moves without teaching HMs."},{"item_ping","Ping near hidden items."},{"save_backup","Keeps 8 rotating validated copies of\nplatinum.sav on the SD (after each save)."},{"stereo_depth","3D slider: per-vertex depth (DEPTH)\nor flat 3D layer (FLAT)."},{"save_export","A: copy a verified platinum.sav\nto export/ on the SD."},
 {"cstick_mode","What the C-stick does."},{"zl_run","ZL toggles running."},{"zr_item","ZR uses your registered item."},{"cpad_dpad","Circle pad acts as D-pad."},
 {"touch_menu","Tap the bottom screen corner\nfor the quick menu."},{"bottom_strip","Button strip on the bottom\nscreen in the field."},
 {"lower_menu","Icon menu around the bottom\nscreen in the field (GabeSud0)."},{"follower","A Pok\xe9mon follows you."},
 {"wild_vis_alive","Visible wild ones pace, look\naround, come over, go home."},{"wild_vis_cries","Visible wild ones nearby call\nnow and then."},{"slot2","A GBA game from slot2/ on the\nSD in the GBA slot (next start)."},{"slot2_pak","Which .gba in slot2/ (by name)\nis in the slot (next start)."},{"b2_sprites","Animated Black 2 Pok\xe9mon\n(b2sprites.bin from the builder)."},{"mobile_pc","Quick menu Boxes tile: the PC\nboxes anywhere (after the League)."},{"mobile_pc_dev","Mobile PC before the League\n(testing; nothing is saved)."},{"sharp_game_text","All game text drawn at the 3DS\nresolution (needs a scale mode)."},{"hunt_counter","Counts encounters for hunts."},{"catch_overlay","Y in battle: catch info, or the\ntrainer's team in trainer battles."},{"dmg_check","Adds a damage and speed page\nto the battle Y screen."},{"hunt_stats","ZR in the field: hunt stats\n(needs the encounter counter)."},{"wild_vis","Wild Pok\xe9mon show in grass."},
 {"shiny_suite","Shiny hunting helpers."},{"wide_early","Faster wide layout in battle."},{"wide_stereo","3D depth for wide layouts."},
 {"wide_dirty","Redraws only changed lines."},{"summary_ivs","IVs/EVs on the summary screen."},{"menu_audio","Music in the settings app."},
 {"dw_depth","Distortion World 3D depth."},{"dw_roll","Distortion World camera roll."},{"follower_slot","Which party slot follows."},{"cheat_time","Fixed time of day."},{"heap_fast","Skips a slow table scan on every\nfree (same result). Faster loads."},{"dc_flush_skip","Skips the game's DS cache flushes\n(not needed here). Restart."},{"wide_windows","Wide view draws screens that use DS\nwindows (battle intro) itself."},{"stereo_gpu_battle","3D slider up in battles: 4:3 view on\nthe GPU (no wide panels). Default OFF."},{"gpu_blend_3d","GPU compose allows 2D blends\nnext to the 3D layer."},{"gpu_direct3d","3D stays on the GPU (no readback)\nwhen GPU compose draws the frame."},{"wide_gpu","Wide battles/menus drawn on the\nGPU (both eyes). SLIDER UP = 3D on."},{"gpu_mbright","GPU compose does screen fades\n(master brightness)."},{"exp_share_all","Party Pok\xe9mon that didn't fight\nget Exp. too (like Exp. Share\nfor the whole party)."},{"auto_run","Run without holding B.\nHold B to walk."},{"catch_swap","Party full: a caught Pok\xe9mon can\nswap with one in the party\n(that one goes to the PC)."},{"shiny_rate","Wild shiny odds. 1/8192 is the\noriginal game. Uses the game's\nown shiny code; save unchanged."},{"rng_info","Hunt stats: the game's RNG seed\nand advances (read only). Seeds\ndiffer from a real DS."},{"wild_vis_shiny","Visible wild Pok\xe9mon can be\nshiny and sparkle in the grass."},{"battery_warn","Low battery: a warning, and\nthe hunt counters are saved."},{"frame_pacing","GPU compose doesn't wait for the GPU.\nFIXED/ADAPTIVE: up to 1 frame of lag."},{"gc_atlas16","GPU compose 2D tiles in 16-bit colour\n(half the memory). Restart."},{"stereo_flat_fallback","Slider up: frames the wide view can't\ndraw (battle intro) show no depth\ninstead of rendering twice."},{"g3_conv_split","With the slider up, the right eye's\n3D readback converts on core 2."},{"hunt_async_save","Writes the shiny-hunt file in the\nbackground (no battle-start hitch)."},{"wireless","OFF / LOCAL (3DS-to-3DS UDS) / ONLINE\n(Poke Classic Network DNS; WIP GTS).\nBack up your save first."},{"wide_3d_alpha","Wide layouts draw 3D fade-ins\n(send-out) instead of the slow path."},{"field_prof","Logs the slowest game functions\nof frames over 70 ms. AUTO: only for\n8 s after battle start/end."},{"wide_dirty_regs","Ignore timing-only registers\nfor the line check."},{"wide_split","Battles at slider 0: the wide\ncompose uses two cores. Restart."},{"battle_log","Logs every battle/menu frame over\n40 ms with where the time went."},{"wide_split_r","Slider up: right-eye wide compose on\nboth cores. SHARED = whichever is free.\nRestart."}};
 for(auto&h:H)if(!strcmp(h[0],k))return h[1];return nullptr;}
static const ATab kTabs[]={
 {"Video",16,{0,1,71,2,124,127,10,20,9,64,42,59,60,67,61,62},{"FPS counter","Wide view","Wide menu layouts","Scale","Sharp game text","Black 2 sprites","3D strength","Battle 3D depth","Camera zoom","Time-of-day tint",
  "Weather in strips","Distortion 3D depth","Distortion debris","Distortion camera roll","Uneasy Pok\xe9mon","Distant cry"}},
 {"Game",18,{22,69,70,58,25,26,23,50,65,51,18,84,100,101,102,125,128,129},{"Fast text","Hold B: fast text","Auto-advance text","Battle anim speed","Repel prompt","IV/EV on summary","Reusable TMs","HMs without learning",
  "Hidden item ping","Save backups (3)","Menu music","Local wireless (test)","Exp. Share: whole party","Auto-run","Swap catch into party","Mobile PC (Boxes)","GBA slot","GBA pak"}},
 {"Keys",8,{3,4,5,6,66,68,77,L_MENU},{"C-stick","ZL run toggle","ZR = item (Y)","Circle pad D-pad","Touch quick menu","Bottom button strip","Bottom icon menu","Quick menu"}},
 {"Pal",12,{14,16,17,34,35,37,36,33,38,39,40,41},{"Following Pok\xe9mon","Follower slot","Follower talk","Follower cry","Finds items","Emote bubbles","Ball pop-out","Area talk",
  "Friendship talk","Idle look-around","Follower gap","Behind player"}},
 {"Hunt",17,{L_HUNT,L_MAP,72,107,108,63,57,122,123,46,47,48,130,131,103,105,106},{"Hunt stats","Wild Pok\xe9mon here","Shiny hunt suite","Skip intro on reset","Hold SELECT = reset (battle)","Encounter counter","Catch info (Y)","Damage check (Y)","Hunt stats on ZR","Visible wild Pok\xe9mon","Visible wild max","Only visible battle","Wild Pok\xe9mon behave","Wild Pok\xe9mon call","Shiny odds","Visible shiny sparkle","Low battery warning"}},
 {"Cheats",11,{24,27,28,29,30,31,32,53,54,55,56},{"No wild encounters","Infinite money","Always catch","Shiny wild","Fast eggs","EXP boost","Rare Candy 99","Infinite PP",
  "Fast running (B)","Walls: hold R","Time of day"}},
 /* run 67: the old Speed tab, hidden: only shown after SELECT is pressed in the settings app */
 {"Debug",36,{11,12,15,19,8,52,13,7,21,43,44,78,79,80,82,73,81,74,75,76,83,85,87,88,89,91,92,93,94,95,96,97,98,99,104,126},{"Fast upload","Fast 3D convert","GPU compose","GPU compose 3D","Log detail","Log size limit","Split top (mono)","Wide view FX","Wide haze","Edge fill","Side fill in menus","Load-hitch profiler","Fast heap free","Skip game cache flushes","Hunt file bg save","Wide layout early","Wide 3D fade-in","Wide layout 3D depth","Wide redraw changed lines","Ignore timing registers","Wide compose on 2 cores","Battle frame log","3D convert on 2 cores","Flat 3D when slow","Wide view windows","GPU blend by 3D","GPU 3D direct","Wide on GPU","GPU fades","Wide GPU blend check","Wide Pok\xe9""dex","Copy eye when 3D empty","Frame pacing","16-bit 2D tiles","RNG info (hunt stats)","Mobile PC before League"}},
 /* pages (not in the tab bar) */
 {"Hunt stats",-3,{},{}},
 {"Wild Pok\xe9mon here",-2,{},{}},
 {"Quick menu",-1,{},{}},
};
enum{T_BAR=6,T_DEBUG=6,T_PAGES=7};
static void prettyLab(char*d,const char*s,int n){/* run 64: "WIDE VIEW FX" -> "Wide view FX" (acronyms kept) */
 static const char*const acr[]={"FPS","GPU","ZL","ZR","FX","3D","1X","2X","25X","5X","ROM","IV","EV","TM","HM","LED","UI","OK","KB","MB","Y","X","A","B","L","R"};
 int o=0,first=1;for(int i=0;s[i]&&o<n-1;){if(!((s[i]>='A'&&s[i]<='Z')||(s[i]>='a'&&s[i]<='z')||(s[i]>='0'&&s[i]<='9'))){d[o++]=s[i++];continue;}
  int j=i;while((s[j]>='A'&&s[j]<='Z')||(s[j]>='a'&&s[j]<='z')||(s[j]>='0'&&s[j]<='9'))j++;char w[16];int L=j-i<15?j-i:15;memcpy(w,s+i,L);w[L]=0;
  bool k=false;for(auto a2:acr)if(!strcmp(w,a2))k=true;bool prevAlpha=i>0&&s[i-1]=='-';
  for(int q=0;q<L&&o<n-1;q++){char c=w[q];if(!k&&c>='A'&&c<='Z'&&(q>0||(!first&&!prevAlpha&&false)))c+=32;if(!k&&q==0&&!first&&c>='A'&&c<='Z')c+=32;d[o++]=c;}
  first=0;i=j;}d[o]=0;
 if(!strcmp(s,"DPAD"))snprintf(d,n,"D-pad");else if(!strcmp(s,"POKETCH"))snprintf(d,n,"Poketch");else if(!strcmp(s,"CIRCLE PAD DPAD"))snprintf(d,n,"Circle pad D-pad");else if(!strcmp(s,"FOLLOWING POKEMON"))snprintf(d,n,"Following Pok\xe9mon");}
static void plate(u32*img,int x0,int y0,int x1,int y1,u32 fill,u32 edge){for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++){if(x<0||x>255||y<0||y>191)continue;
 bool corner=(x==x0||x==x1-1)&&(y==y0||y==y1-1);if(corner)continue;bool e=y==y0||y==y1-1||x==x0||x==x1-1;img[y*256+x]=e?edge:fill;}}
static int gtxt(u32*img,int x,int y,const char*t,u32 fg,u32 sh){if(P3DS_GameFontReady())return P3DS_GameFontDraw(img,256,256,192,x,y,t,fg,sh,1,1);char u[64];toUpper(u,t,sizeof u);drawBig(img,x,y+4,u,fg);return (int)strlen(u)*8;}
static int gw(const char*t){if(P3DS_GameFontReady())return P3DS_GameFontWidth(1,t);return (int)strlen(t)*8;}
extern "C" void P3DS_GpuInvalidateBot(void);
static void tri(u32*img,int x,int y,int dir,u32 c){for(int k=0;k<4;k++)for(int yy=k;yy<8-k;yy++){int X=dir>0?x+k:x+3-k;if(X>=0&&X<256&&y+yy>=0&&y+yy<192)img[(y+yy)*256+X]=c;}}
static u32 kMenuFg(){return kMsgFg;}
static int appCfg(){static int v=-1;if(v<0){v=P3DS_CfgInt("settings_app",1)!=0;PSPNativeMemLog("[APP] settings_app=%d",v);}return v;}

#include "ss3ds_settings_ui.h"
#include "p3ds_movenames.h"
extern "C" { extern volatile int P3DS_OvDirty,P3DS_OvN; }
static int suiDraw(u32*b,int x,int y,const char*t,u32 fg,u32 sh){return gtxt(b,x,y,t,fg,sh);}
extern "C" int P3DS_GameFontWidth34(int,const char*);extern "C" int P3DS_GameFontDraw34(u32*,int,int,int,int,int,const char*,u32,int);
static int suiDrawS(u32*b,int x,int y,const char*t,u32 fg,u32 sh){if(P3DS_GameFontReady())return P3DS_GameFontDraw34(b,256,256,192,x,y,t,fg,1);return gtxt(b,x,y,t,fg,sh);}
static int gwS(const char*t){return P3DS_GameFontReady()?P3DS_GameFontWidth34(1,t):gw(t);}
/* 2026-11-18 sharp text (ss3ds_sharptext.h): with sharp_text ON, the GPU stretch on and the ROM font loaded, kSuiText measures and draws at
   the native size; strings drawn while btxBegin records leave the DS picture and go to the 320x240 text layer (gpuscale.c txDraw).
   Widths and drawing switch together, so layouts and touch hit tests always match what is shown. */
#include "ss3ds_sharptext.h"
extern "C" { int P3DS_GameFontGlyph(int,int,unsigned char(*)[16]); u32*P3DS_TxBufGet(void); extern volatile int P3DS_TxOn,P3DS_TxDirty,P3DS_TxFailed; }
static SBtxRec*gBtx;static bool txLastOn;static const SBtxList*txShownL;static uint32_t txShownGen;static unsigned txLogN;
static bool btxMode(){
 if(!sharpTextCfg()||P3DS_ScaleMode()==0||P3DS_TxFailed||!P3DS_GameFontReady())return false;
 if(!gBtx){gBtx=(SBtxRec*)malloc(sizeof(SBtxRec));if(!gBtx){P3DS_TxFailed=1;PSPNativeMemLog("[TEXT] no memory for the text recorder (%u bytes): text stays in the DS picture",(unsigned)sizeof(SBtxRec));return false;}gBtx->on=false;gBtx->dst=nullptr;gBtxGlyph=P3DS_GameFontGlyph;}
 return P3DS_TxBufGet()!=nullptr;}
static int sharpDraw(u32*b,int x,int y,const char*t,u32 fg,u32 sh){return btxMode()?btxText(*gBtx,b,x,y,t,fg,sh,false):suiDraw(b,x,y,t,fg,sh);}
static int sharpW(const char*t){return btxMode()?btxDsW(t,false):gw(t);}
static int sharpDrawS(u32*b,int x,int y,const char*t,u32 fg,u32 sh){return btxMode()?btxText(*gBtx,b,x,y,t,fg,0,true):suiDrawS(b,x,y,t,fg,sh);}
static int sharpWS(const char*t){return btxMode()?btxDsW(t,true):gwS(t);}
static const SUiText kSuiText={sharpDraw,sharpW,sharpDrawS,sharpWS};
/* the list on screen: re-rendered into the layer only when another list or a changed one is shown; P3DS_TxOn for this present */
static void btxShow(const SBtxList*l){
 if(!l||!l->n){P3DS_TxOn=0;return;}u32*a=P3DS_TxBufGet();if(!a){P3DS_TxOn=0;return;}
 if(l!=txShownL||l->gen!=txShownGen){btxRender(*l,a);P3DS_TxDirty=1;txShownL=l;txShownGen=l->gen;}
 P3DS_TxOn=1;}
static void btxLog(const char*where,const SBtxList*l){if(!gBtx||txLogN>=12||(txLogN>=2&&!gBtx->covered&&!gBtx->full))return;txLogN++;
 PSPNativeMemLog("[TEXT] sharp %s: %d strings at 320x240, %d left in the DS picture (something drawn over them), %d over the recorder size",where,l?l->n:0,gBtx->covered,gBtx->full);}
static int touchToDS(int px,int py,int*x,int*y){int dx,dy;if(P3DS_ScaleMode()==0){dx=px-32;dy=py-24;}else{dx=px*256/320;dy=py*192/240;}if(dx<0||dx>255||dy<0||dy>191)return 0;*x=dx;*y=dy;return 1;}
/* run 88 shiny_suite hunt panel (Ball tab when not showing catch info): one species' counters from hunt.txt.
   D-pad left/right = other species with a record, X twice = reset that counter, A = export all to a CSV in hunts/. */
static int huntSel,huntConfirm;static char huntMsg[64];
static bool huntPanelShown(){const volatile int*ci=P3DS_CatchInfo;return P3DS_HuntOn()&&!(ci[0]&&!P3DS_InField&&P3DS_CfgInt("catch_overlay",0));}
static int huntShown(){const int b=P3DS_CatchInfo[0]&&!P3DS_InField?P3DS_CatchInfo[1]:0;return huntSel>0?huntSel:b>0&&b<494?b:P3DS_HuntTarget();}
static void huntKeys(u32 kd){const int sp=huntShown();
 if(kd&(KEY_DLEFT|KEY_DRIGHT)){const int n=P3DS_HuntNext(sp,(kd&KEY_DRIGHT)?1:-1);if(n)huntSel=n;huntConfirm=0;huntMsg[0]=0;}
 if((kd&KEY_X)&&sp>0){if(huntConfirm==sp){P3DS_HuntClear(sp);huntConfirm=0;snprintf(huntMsg,sizeof huntMsg,"%s counter reset.",kP3dsSpec[sp]);}
  else{huntConfirm=sp;snprintf(huntMsg,sizeof huntMsg,"Press X again to reset %s.",kP3dsSpec[sp]);}}
 if(kd&KEY_A){char path[96];const int n=P3DS_HuntExportCsv(kP3dsSpec,(int)(sizeof kP3dsSpec/sizeof kP3dsSpec[0]),path,sizeof path);huntConfirm=0;
  if(n<0)snprintf(huntMsg,sizeof huntMsg,"Could not write the CSV file.");else{const char*f=strrchr(path,'/');snprintf(huntMsg,sizeof huntMsg,"Saved hunts/%s",f?f+1:path);}}}
/* run 119: the RNG line (rng_info, Debug tab): the first LCRNG seed (hunt.c wraps LCRNG_SetSeed) and how far the game's
   LCRNG has advanced from it, counted by stepping a copy (read only; the game's generator is never called or changed). */
extern "C" unsigned LCRNG_GetSeed(void);
static bool rngLine(char*l,int n){static unsigned st,adv;static int seeds=-1;static bool lost;int sd;const unsigned s0=P3DS_RngInitial(&sd);if(!sd)return false;
 if(sd!=seeds&&seeds<0){st=s0;adv=0;lost=false;}seeds=sd;const unsigned cur=LCRNG_GetSeed();
 if(!lost){int k=0;while(st!=cur&&k<2000000){st=st*0x41C64E6Du+0x6073u;adv++;k++;}if(st!=cur)lost=true;}
 if(lost)snprintf(l,n,"RNG seed %08X  now %08X  (reseeded)",s0,cur);else snprintf(l,n,"RNG seed %08X  now %08X  +%u",s0,cur,adv);return true;}
static void huntPanel(u32*bimg,u32 valC,u32 valSh){const int sp=huntShown();char l[96];const u32 gy=0x00787878u,gs=0x00D0D0D0u;
 const volatile int*hg=P3DS_HuntGame;const int odds=P3DS_HuntRateOdds();
 if(sp<=0){gtxt(bimg,128-gw("No hunt yet.")/2,64,"No hunt yet.",kMsgFg,kMsgSh);gtxt(bimg,128-gw("Every wild battle is counted.")/2,84,"Every wild battle is counted.",gy,gs);
  if(hg[0]){snprintf(l,sizeof l,"Shiny in party: %d of %d",hg[1],hg[2]);gtxt(bimg,8,136,l,gy,gs);}
  if(P3DS_CfgInt("rng_info",0)&&rngLine(l,sizeof l))gtxt(bimg,8,152,l,gy,gs);return;}
 const P3DS_HuntRec&r=*P3DS_HuntGet(sp);const bool cur=sp==P3DS_HuntTarget();
 snprintf(l,sizeof l,"%s%s",kP3dsSpec[sp],cur?"  (current hunt)":"");gtxt(bimg,8,24,l,valC,valSh);
 snprintf(l,sizeof l,"Encounters %d   Since last shiny %d",r.enc,r.streak);gtxt(bimg,8,40,l,kMsgFg,kMsgSh);
 snprintf(l,sizeof l,"Soft resets %d   Shinies found %d",r.resets,r.shinies);gtxt(bimg,8,56,l,kMsgFg,kMsgSh);
 if(r.start)snprintf(l,sizeof l,"Time %dh %02dm   Started %04d-%02d-%02d",r.secs/3600,r.secs/60%60,r.start/10000,r.start/100%100,r.start%100);else snprintf(l,sizeof l,"Time %dh %02dm",r.secs/3600,r.secs/60%60);
 gtxt(bimg,8,72,l,kMsgFg,kMsgSh);
 /* odds for the situation, from the game's own rules: wild 1/8192 (or shiny_rate), Poke Radar patches by chain, Masuda eggs */
 if(P3DS_Cheat(2))gtxt(bimg,8,88,"Shiny wild cheat is on.",0x002020C0u,gs);
 else if(hg[0]&&hg[3]>0){snprintf(l,sizeof l,"Radar chain %d: each patch 1 in %d",hg[3],P3DS_RadarPatchOdds(hg[3]));gtxt(bimg,8,88,l,valC,valSh);}
 else{snprintf(l,sizeof l,"Odds 1 in %d per wild encounter%s",odds,odds<P3DS_HUNT_ODDS?" (setting)":"");gtxt(bimg,8,88,l,valC,valSh);}
 snprintf(l,sizeof l,"Chance of a shiny by now: %.1f%%",100.0*P3DS_HuntChanceBy(r.streak,odds));gtxt(bimg,8,104,l,kMsgFg,kMsgSh);
 if(r.shinies)snprintf(l,sizeof l,"Last shiny came after %d encounters",r.lastShinyAt);else snprintf(l,sizeof l,"50%% find one by %d, 90%% by %d",P3DS_HuntEncFor(0.5,odds),P3DS_HuntEncFor(0.9,odds));
 gtxt(bimg,8,120,l,gy,gs);
 if(hg[0]){const char*dc=hg[4]==2?"   Eggs: Masuda":hg[4]==1?"   Eggs: 1 in 8192":"";char m[24]="";if(hg[4]==2)snprintf(m,sizeof m," 1 in %d",P3DS_MasudaOdds());
  snprintf(l,sizeof l,"Shiny in party %d of %d%s%s",hg[1],hg[2],dc,m);gtxt(bimg,8,136,l,gy,gs);}
 if(P3DS_CfgInt("rng_info",0)&&rngLine(l,sizeof l))gtxt(bimg,8,152,l,gy,gs);
 gtxt(bimg,8,168,huntMsg[0]?huntMsg:"Left/Right species   X reset   A save CSV",huntMsg[0]?valC:gy,huntMsg[0]?valSh:gs);}
static int tabBarOf(int t){if(t<T_PAGES)return t;if(kTabs[t].n==-1)return 2;return 4;} /* pages light up the tab whose row opens them */
static int tabFind(int n){for(int t=0;t<(int)(sizeof kTabs/sizeof kTabs[0]);t++)if(kTabs[t].n==n)return t;return 0;}
static void runAppPages(const u32*top,int mode,bool gpu){ /* the pre-2026-11-25 settings app: now only its pages (hunt stats, wild Pokemon here, quick menu) */
 const size_t PX=256*192;u32*base=(u32*)malloc(PX*4*3);if(!base){PSPNativeMemLog("[APP] no memory");return;}u32*img=base+PX,*bimg=img+PX;memcpy(base,top,PX*4);
 enum{N=MENU_N};int val[N];for(int i=0;i<N;i++)val[i]=menuGet(i);int old[N];memcpy(old,val,sizeof old);
 for(int i=0;i<256*192;i++){u32 c=base[i];base[i]=((c>>1)&0x007F7F7Fu)|(c&0xFF000000u);}
 P3DS_MenuOpen=1;P3DS_LowerMenuDraw=0;huntSel=huntConfirm=0;huntMsg[0]=0;PSPNativeMemLog("[APP] settings open");P3DS_Toast("");toastAt=0;
 /* run 67: SELECT inside the app shows/hides the Debug tab */
 static bool dbgTabs=false;int NT=T_BAR+(dbgTabs?1:0);static int tab=0;static int mapPage=0;int sel=0,back=-1,backSel=0;
 if(appStartTab>=0){tab=appStartTab;appStartTab=-1;}else if(tab>=T_PAGES)tab=tabBarOf(tab); /* a page opened directly: B closes */
 if(tab==T_DEBUG&&!dbgTabs)tab=0;
 u32 held=hidKeysHeld();bool wasTouch=(held&KEY_TOUCH)!=0;u64 t0=svcGetSystemTick();int rep=0;
 const u32 bg=0x00B0D8B8u,cellF=0x00F8F8F8u,cellE=0x00586058u,selF=0x00E0F4E0u,selE=0x0040B040u,valC=0x001828E0u,valSh=0x00A0A8F0u,tabSel=0x0060C070u,gy=0x00787878u,gs=0x00D0D0D0u;
 const int ROWS=8,RH=19,ROW0=23,VX0=148,VXC=198; /* list: 8 rows of 19 px; value column 148..252 */
 while(aptMainLoop()){
  hidScanInput();u32 kh=hidKeysHeld(),kd=hidKeysDown()&~held;held&=kh;
  if(kh&(KEY_UP|KEY_DOWN)){if(++rep>18&&rep%5==0)kd|=kh&(KEY_UP|KEY_DOWN);}else rep=0;
  if(kd&KEY_B){if(back>=0){tab=back;sel=backSel;back=-1;continue;}break;}
  if(kd&KEY_START)break;if((kd&KEY_Y)&&(kh&KEY_L)&&(kh&KEY_R))break;if((kd&KEY_Y)&&kTabs[tab].n==-3)break;
  if(kd&KEY_SELECT){dbgTabs=!dbgTabs;NT=T_BAR+(dbgTabs?1:0);if(tab==T_DEBUG&&!dbgTabs){tab=0;sel=0;}back=-1;PSPNativeMemLog("[APP] debug tab %s",dbgTabs?"shown":"hidden");}
  if(kd&(KEY_L|KEY_R)){const int b=tabBarOf(tab);tab=(kd&KEY_L)?(b+NT-1)%NT:(b+1)%NT;sel=0;back=-1;continue;}
  const ATab&T=kTabs[tab];const bool list=T.n>0,menuTab=T.n==-1;const int pages=list?(T.n+ROWS-1)/ROWS:1;int startReq=-1,open=-1;
  static const char*const kMenuNm[8]={"Pok\xe9mon","Bag","Pok\xe9" "dex","Trainer card","Save","Options","Boxes","Close"};static const int kMenuOpt[8]={1,2,0,3,4,5,100,101};
  const int MN=P3DS_MobilePcCfg()?8:6,MRH=MN>6?41:54; /* SoulSilver 0293 Mobile PC: Boxes (100) + Close (101) tiles while mobile_pc is on */
  if(T.n==-3&&huntPanelShown())huntKeys(kd);
  if(T.n==-2&&(kd&(KEY_A|KEY_RIGHT)))mapPage++;
  if(list){if(sel>=T.n)sel=T.n-1;if(kd&KEY_UP)sel=(sel+T.n-1)%T.n;if(kd&KEY_DOWN)sel=(sel+1)%T.n;
   const int it=T.it[sel];if(it>=L_HUNT){if(kd&(KEY_A|KEY_RIGHT))open=it;}
   else{const int nn=mItems[it].n;if(kd&(KEY_A|KEY_RIGHT))val[it]=(val[it]+1)%nn;if(kd&(KEY_X|KEY_LEFT))val[it]=(val[it]+nn-1)%nn;}}
  /* touch: act on press (not while the opening combo is still held) */
  bool tdown=(kh&KEY_TOUCH)!=0;if(tdown&&!wasTouch){touchPosition tp;hidTouchRead(&tp);int x,y;if(touchToDS(tp.px,tp.py,&x,&y)){
    if(y<21){int t=x*NT/256;if(t>=0&&t<NT){tab=t;sel=0;back=-1;}}
    else if(T.n==-2)mapPage++;
    else if(menuTab&&y>=22){int r=(y-22)/MRH,c=x>=128?1:0,i=r*2+c;if(i<MN)startReq=kMenuOpt[i];}
    else if(list&&y>=ROW0&&y<ROW0+ROWS*RH){const int i=sel/ROWS*ROWS+(y-ROW0)/RH;if(i<T.n){sel=i;const int it=T.it[i];
      if(it>=L_HUNT)open=it;else{const int nn=mItems[it].n;if(x>=VX0&&x<VXC)val[it]=(val[it]+nn-1)%nn;else val[it]=(val[it]+1)%nn;}}}
    else if(list&&pages>1&&y>=ROW0+ROWS*RH)sel=(sel/ROWS+1)%pages*ROWS;}}
  wasTouch=tdown;
  if(open>=0){back=tab;backSel=sel;tab=tabFind(open==L_HUNT?-3:open==L_MAP?-2:-1);sel=0;PSPNativeMemLog("[APP] page %s",kTabs[tab].name);continue;}
  if(menuTab&&(kd&KEY_A))startReq=kMenuOpt[sel<MN?sel:0];
  if(menuTab&&(kd&(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT))){if(sel>=MN)sel=0;if(kd&KEY_UP)sel=(sel+MN-2)%MN;if(kd&KEY_DOWN)sel=(sel+2)%MN;if(kd&(KEY_LEFT|KEY_RIGHT))sel^=1;}
  if(startReq==101)break;
  if(startReq==100){if(P3DS_InField)P3DS_MobilePcRequestOpen();else P3DS_Toast("Boxes: only in the field");PSPNativeMemLog("[APP] Boxes tile (field %d)",P3DS_InField);break;}
  if(startReq>=0){extern volatile int P3DS_StartReq;P3DS_StartReq=P3DS_InField?startReq:-1;PSPNativeMemLog("[APP] start menu request %d (field %d)",startReq,P3DS_InField);break;}
  /* top: dimmed game, title box (tab + version) and a one-line key hint */
  memcpy(img,base,PX*4);
  if(useGF()){char t2[48];snprintf(t2,sizeof t2,"Settings: %s",T.name);if(list&&pages>1)snprintf(t2+strlen(t2),sizeof t2-strlen(t2),"  (%d/%d)",sel/ROWS+1,pages);
   P3DS_GameBoxDraw(img,256,256,192,16,16,216,32);gtxt(img,18,16,t2,kMsgFg,kMsgSh);gtxt(img,18,32,kAppVer,gy,gs);
   {unsigned ph,pm;if(p3dsPlayTime(&ph,&pm)){char pl[32];snprintf(pl,sizeof pl,"Play time %u:%02u",ph,pm);P3DS_GameBoxDraw(img,256,256,192,16,56,216,16);gtxt(img,18,56,pl,kMsgFg,kMsgSh);}}
   const char*h1=list?"Up/Down: pick   Left/Right: change":menuTab?"Pick a menu with the D-pad or a tap.":T.n==-2?"Tap or A: next page":"Game paused";
   const char*h2=back>=0?"L/R: tab   B: back":"L/R: tab   B: close";
   if(list&&T.it[sel]<L_HUNT){const char*hp=settingHelp(mItems[T.it[sel]].key);if(hp){P3DS_GameBoxDraw(img,256,256,192,16,104,216,32);
    const char*nl=strchr(hp,'\n');if(nl){char a1[64];int n=(int)(nl-hp);if(n>63)n=63;memcpy(a1,hp,n);a1[n]=0;gtxt(img,18,104,a1,kMsgFg,kMsgSh);gtxt(img,18,120,nl+1,kMsgFg,kMsgSh);}else gtxt(img,18,104,hp,kMsgFg,kMsgSh);}}
   P3DS_GameBoxDraw(img,256,256,192,16,152,216,32);gtxt(img,18,152,h1,kMsgFg,kMsgSh);gtxt(img,18,168,h2,kMsgFg,kMsgSh);}
  else{menuBox(img,8,10,248,52,0x00502010u,0x00F0F0F0u);drawBig(img,16,18,"SETTINGS (PAUSED)",0x0060E0FFu);drawBig(img,16,34,P3DS_VERSION,0x00C0C0C0u);}
  /* bottom: tab bar + the tab's list or page */
  for(int i=0;i<256*192;i++)bimg[i]=bg;
  {const int on=tabBarOf(tab);for(int t=0;t<NT;t++){int x0=t*256/NT+1,x1=(t+1)*256/NT-1;const bool s2=t==on;plate(bimg,x0,1,x1,20,s2?tabSel:cellF,cellE);
   gtxt(bimg,(x0+x1)/2-gw(kTabs[t].name)/2,2,kTabs[t].name,s2?0x00F8F8F8u:kMsgFg,s2?0x00387038u:kMsgSh);}}
  if(menuTab)for(int i=0;i<MN;i++){int r=i/2,c=i%2;int x0=c?130:4,y0=24+r*MRH,x1=x0+122,y1=y0+MRH-4;bool s2=i==sel;
   plate(bimg,x0,y0,x1,y1,cellF,s2?selE:cellE);if(s2)plate(bimg,x0+1,y0+1,x1-1,y1-1,cellF,selE);gtxt(bimg,(x0+x1)/2-gw(kMenuNm[i])/2,y0+(MRH-4)/2-9,kMenuNm[i],kMenuFg(),kMsgSh);}
  if(T.n==-3){/* run 75 Catch tab: read-only snapshot of the wild foe (taken when the command menu opens) */
   plate(bimg,2,22,254,190,cellF,cellE);const volatile int*ci=P3DS_CatchInfo;
   if(huntPanelShown())huntPanel(bimg,valC,valSh);
   else if(!P3DS_CfgInt("catch_overlay",0)){gtxt(bimg,128-gw("Turn on Catch info in Hunt.")/2,80,"Turn on Catch info in Hunt.",0x00787878u,0x00D0D0D0u);}
   else if(!ci[0]||P3DS_InField){gtxt(bimg,128-gw("Only in wild battles.")/2,80,"Only in wild battles.",0x00787878u,0x00D0D0D0u);}
   else{static const char*const nat[25]={"Hardy","Lonely","Brave","Adamant","Naughty","Bold","Docile","Relaxed","Impish","Lax","Timid","Hasty","Serious","Jolly","Naive","Modest","Mild","Quiet","Bashful","Rash","Calm","Gentle","Sassy","Careful","Quirky"};
    char l[96];const int sp=ci[1],ab=ci[6];
    snprintf(l,sizeof l,"%s  Lv%d  %s%s",sp>0&&sp<494?kP3dsSpec[sp]:"?",ci[2],ci[16]?"(caught before)":"(new)",P3DS_HuntOn()&&ci[18]?"  SHINY":"");gtxt(bimg,8,24,l,valC,valSh);
    snprintf(l,sizeof l,"Nature: %s   Ability: %s",nat[ci[7]%25],ab>=0&&ab<124?kP3dsAbil[ab]:"?");gtxt(bimg,8,42,l,kMsgFg,kMsgSh);
    int tot=0;for(int k=0;k<6;k++)tot+=ci[9+k];
    snprintf(l,sizeof l,"IVs %d/%d/%d/%d/%d/%d",ci[9],ci[10],ci[11],ci[12],ci[13],ci[14]);gtxt(bimg,8,60,l,kMsgFg,kMsgSh);
    snprintf(l,sizeof l,"(HP/Atk/Def/SpA/SpD/Spe)  %s",tot>=151?"Outstanding":tot>=121?"Relatively superior":tot>=91?"Above average":"Decent");gtxt(bimg,8,76,l,0x00787878u,0x00D0D0D0u);
    const int st=ci[5];const char*stn=(st&7)?"Asleep":(st&0x20)?"Frozen":(st&0x40)?"Paralyzed":(st&0x10)?"Burned":(st&0x88)?"Poisoned":"OK";
    snprintf(l,sizeof l,"HP %d%%  Status: %s  Pok\xe9rus: %s",ci[4]?ci[3]*100/ci[4]:0,stn,ci[8]?((ci[8]&15)?"Yes":"Cured"):"No");gtxt(bimg,8,94,l,kMsgFg,kMsgSh);
    const double sm=(st&0x27)?2.0:(st&0xD8)?1.5:1.0;
    auto odds=[&](int ball)->int{double a=(double)(ci[15]*ball/10)*(3.0*ci[4]-2.0*ci[3])/(3.0*ci[4]);a=(double)(int)a*sm;if(a>=255)return 100;if(a<1)a=1;
     double b=1048560.0/sqrt(sqrt(16711680.0/a));double p=b/65536.0;p=p*p*p*p;if(p>1)p=1;return (int)(p*100+0.5);};
    snprintf(l,sizeof l,"Catch rate %d.  Odds per throw:",ci[15]);gtxt(bimg,8,116,l,valC,valSh);
    const int tm=ci[17]+10>40?40:ci[17]+10;
    snprintf(l,sizeof l,"Pok\xe9 %d%%   Great %d%%   Ultra %d%%",odds(10),odds(15),odds(20));gtxt(bimg,8,134,l,kMsgFg,kMsgSh);
    snprintf(l,sizeof l,"%s %d%%   Timer %d%%   Nest %d%%",ci[17]<1?"Quick":"Quick (turn 1)",odds(ci[17]<1?40:10),odds(tm),odds(ci[2]<30?40-ci[2]:10));gtxt(bimg,8,152,l,kMsgFg,kMsgSh);
    if(P3DS_HuntCounting()){snprintf(l,sizeof l,"Met %d times (hunt.txt)",P3DS_HuntCount(sp));gtxt(bimg,256-8-gw(l),116,l,0x00787878u,0x00D0D0D0u);}
    snprintf(l,sizeof l,"Dusk %d%% (night/cave)  Net %d%% (Water/Bug)",odds(35),odds(30));gtxt(bimg,8,170,l,kMsgFg,kMsgSh);}}
  else if(T.n==-2){/* run 65 Map tab: place + grid of wild Pokemon (party icon, chance %, levels) per section; tap = next page */
   plate(bimg,2,22,254,190,cellF,cellE);
   const char*pn=P3DS_MapId>=0&&P3DS_MapId<(int)(sizeof kP3dsAreaName/sizeof kP3dsAreaName[0])?kP3dsAreaName[P3DS_MapId]:"";
   static const char*const hd[5]={"Grass / cave","Surfing","Old Rod","Good Rod","Super Rod"};
   const int n=P3DS_MapEntN;
   {static int iconGen=-1;if(n&&iconGen!=P3DS_MapGen){iconGen=P3DS_MapGen;if(!P3DS_MapIcons)P3DS_MapIcons=(unsigned*)malloc(48*1024*4);
     if(P3DS_MapIcons){u64 t0=svcGetSystemTick();int got=P3DS_IconLoad(n,P3DS_MapIconMem,P3DS_MapIconPal,P3DS_MapIconPalFile,(uint32_t*)P3DS_MapIcons);
      PSPNativeMemLog("[MAP] %d of %d icons loaded from the ROM in %u us (settings app, game paused)",got,n,(unsigned)((svcGetSystemTick()-t0)/CPU_TICKS_PER_USEC));}}}
   /* layout pass: assign pages (header 14 px, rows of 4 cells 62x44) */
   static int pg[48],py[48],px[48],hdY[5],hdPg[5];int page=0,y=40,col=0,lastSec=-1;for(int k=0;k<5;k++)hdPg[k]=-1;
   for(int k=0;k<n;k++){const int sc=P3DS_MapEnt[k].sec;if(sc!=lastSec){if(col){y+=44;col=0;}if(y+14+44>190){page++;y=40;}hdY[sc]=y;hdPg[sc]=page;y+=14;lastSec=sc;}
    if(col==4){col=0;y+=44;if(y+44>190){page++;y=40;}}pg[k]=page;px[k]=4+col*62;py[k]=y;col++;}
   const int pages=page+1;if(mapPage>=pages)mapPage=0;
   char t2[64];if(pages>1)snprintf(t2,sizeof t2,"%s  %d/%d",pn,mapPage+1,pages);else snprintf(t2,sizeof t2,"%s",pn);gtxt(bimg,128-gw(t2)/2,22,t2,kMsgFg,kMsgSh);
   if(!n)gtxt(bimg,128-gw("No wild Pok\xe9" "mon here.")/2,80,"No wild Pok\xe9" "mon here.",0x00787878u,0x00D0D0D0u);
   for(int sc=0;sc<5;sc++)if(hdPg[sc]==mapPage){char h2[32];snprintf(h2,sizeof h2,"%s%s",hd[sc],sc==0?(P3DS_MapTod?" (night)":" (day)"):"");gtxt(bimg,8,hdY[sc]-3,h2,valC,valSh);}
   for(int k=0;k<n;k++){if(pg[k]!=mapPage)continue;const P3DSMapEntR&e=P3DS_MapEnt[k];const int cx=px[k],cy=py[k];
    if(P3DS_MapIcons){const unsigned*ic=P3DS_MapIcons+k*1024;for(int yy=0;yy<32;yy++){int dy=cy+yy-6;if(dy<0||dy>=192)continue;for(int xx=0;xx<32;xx++){unsigned c=ic[yy*32+xx];if(c>>24)continue;bimg[dy*256+cx+15+xx]=c;}}}
    char a[24];if(e.lo==e.hi)snprintf(a,sizeof a,"%d%% Lv%d",e.pct,e.lo);else snprintf(a,sizeof a,"%d%% %d-%d",e.pct,e.lo,e.hi);
    gtxt(bimg,cx+31-gw(a)/2,cy+25,a,kMsgFg,kMsgSh);}}
  if(list){const int pg=sel/ROWS;
   for(int r=0;r<ROWS;r++){const int i=pg*ROWS+r;if(i>=T.n)break;const int y0=ROW0+r*RH,it=T.it[i];const bool s2=i==sel;
    plate(bimg,3,y0,253,y0+RH-1,s2?selF:cellF,s2?selE:cellE);gtxt(bimg,10,y0+1,T.nm[i],kMsgFg,kMsgSh);
    if(it>=L_HUNT){gtxt(bimg,240-gw("Open"),y0+1,"Open",valC,valSh);tri(bimg,244,y0+5,1,cellE);continue;}
    char lab[24];prettyLab(lab,mItems[it].lab[val[it]],sizeof lab);gtxt(bimg,VXC+2-gw(lab)/2,y0+1,lab,valC,valSh);
    tri(bimg,VX0+2,y0+5,-1,s2?selE:cellE);tri(bimg,244,y0+5,1,s2?selE:cellE);
    if(val[it]!=old[it])for(int k=0;k<3;k++)for(int q=0;q<3;q++)bimg[(y0+8+k)*256+5+q]=selE;} /* dot = changed, saved on close */
   if(pages>1){char f[32];snprintf(f,sizeof f,"Page %d/%d  (tap for more)",pg+1,pages);gtxt(bimg,128-gw(f)/2,ROW0+ROWS*RH,f,gy,gs);}}
  {extern int P3DS_GpuBotSame;P3DS_GpuBotSame=0;}
  if(mode&&gpu)P3DS_GpuPresent(img,bimg,mode,true,nullptr);
  else{P3DS_PresentDeferSettle();present(GFX_TOP,400,img,0);present(GFX_BOTTOM,320,bimg,0);gfxFlushBuffers();gfxSwapBuffers();}
  gspWaitForVBlank();
  if(menuAudioCfg()){static u64 mt;u64 nw=svcGetSystemTick();unsigned us=mt&&nw>mt?(unsigned)((nw-mt)/CPU_TICKS_PER_USEC):16667;mt=nw;if(us>66666)us=16667;PSPNativeSoundAdvance(us);}
 }
 for(int i=0;i<120&&aptMainLoop();i++){hidScanInput();if(!(hidKeysHeld()&(KEY_A|KEY_B|KEY_X|KEY_START|KEY_Y|KEY_L|KEY_R|KEY_TOUCH|KEY_DUP|KEY_DDOWN|KEY_DLEFT|KEY_DRIGHT)))break;gspWaitForVBlank();}
 free(base);P3DS_GpuInvalidateBot();int ch=0;for(int i=0;i<N;i++)if(val[i]!=old[i]){menuSet(i,val[i]);ch++;PSPNativeMemLog("[APP] saved %s=%s",mItems[i].key,mItems[i].lab[val[i]]);}
 P3DS_MenuOpen=0;PSPNativeMemLog("[APP] settings closed after %u ms, %d change(s)",(unsigned)((svcGetSystemTick()-t0)/CPU_TICKS_PER_MSEC),ch);
 tPrevEnd=svcGetSystemTick();secStart=0;secFrames=secG3=secComp=secGpu=secGame=secTris=0;secG3G=0;
}

/* ---- 2026-11-25 settings app in SoulSilver's design (Nathan: "you can completely rewrite the setting menu"). L+R+Y or the corner
   touch opens it; the game is paused. Tabs LOOK / BATTLE / PLAY / PALS / SYSTEM, SELECT shows DEBUG. Up/Down = row, Left/X and
   Right/A = value, tap a value's left/right half, L/R = tab, B / START / L+R+Y close (changes are applied and saved then).
   A row with id >= L_HUNT opens a page of the old app (runAppPages): hunt stats, wild Pokemon here, quick menu. ---- */
/* SoulSilver 0269 Online Settings (gts_picker, default ON like SoulSilver): the main menu's Nintendo WFC Settings row opens this
   screen instead: pick the GTS server (Poke Classic or StarMe, key gts_server), or read how Wi-Fi is set up. Last-seen GTS
   previews are shown when the network layer has one. Platinum: no party icons (SoulSilver reads them from HGSS archives). */
#include "ss3ds_gts_preview.h"
extern "C" int P3DS_GtsPickerEnabled(void);
extern "C" int P3DS_GtsStarMeSelected(void);
static bool onlineSettingsRequested;
extern "C" void P3DS_RequestOnlineSettings(void){__atomic_store_n(&onlineSettingsRequested,true,__ATOMIC_RELEASE);}
static void runOnlineSettings(const u32*top,int mode,bool gpu){
 u32*bottom=(u32*)malloc(256*192*sizeof(u32));if(!bottom)return;
 P3DS_MenuOpen=1;P3DS_LowerMenuDraw=0;
 const char*const names[]={"Poke Classic","StarMe","Wi-Fi Setup"};
 const GtsPreview preview[]={P3DS_GtsPreviewRead(0),P3DS_GtsPreviewRead(1),{}};
 const int rowHeight=20,previewHeight=26;int rowY[]={26,0,0};
 for(int sv=0;sv<2;++sv)rowY[sv+1]=rowY[sv]+rowHeight+3+(preview[sv].state?previewHeight+3:0);
 bool wifiSetup=false;int choice=P3DS_GtsStarMeSelected();u32 held=hidKeysHeld();
 PSPNativeMemLog("[GTS] online settings open (server %s)",choice?"StarMe":"Poke Classic");
 while(aptMainLoop()){
  hidScanInput();held&=hidKeysHeld();const u32 pressed=hidKeysDown()&~held;bool select=(pressed&KEY_A)!=0;
  if(!wifiSetup&&(pressed&KEY_TOUCH)){touchPosition t;hidTouchRead(&t);int x,y;if(touchToDS(t.px,t.py,&x,&y)&&x>=4&&x<252)for(int r=0;r<3;++r)if(y>=rowY[r]&&y<rowY[r]+rowHeight){choice=r;select=true;break;}}
  if(pressed&KEY_B){if(!wifiSetup)break;wifiSetup=false;}
  else if(!wifiSetup){if(pressed&KEY_DUP)choice=(choice+2)%3;if(pressed&KEY_DDOWN)choice=(choice+1)%3;
   if(select){if(choice==2)wifiSetup=true;else if(choice!=P3DS_GtsStarMeSelected()){P3DS_CfgWrite("gts_server",choice);PSPNativeMemLog("[GTS] selected %s",choice?"StarMe":"Poke Classic");}}}
  suiBackground(bottom);
  gtxt(bottom,12,6,wifiSetup?"Wi-Fi Setup":"GTS Server",kMsgFg,kMsgSh);
  if(wifiSetup){gtxt(bottom,12,44,"Set up your connection in",kMsgFg,kMsgSh);gtxt(bottom,12,62,"Nintendo 3DS System Settings.",kMsgFg,kMsgSh);gtxt(bottom,12,96,"Online connects on its own.",kMsgFg,kMsgSh);}
  else{const int active=P3DS_GtsStarMeSelected();
   for(int r=0;r<3;++r){const int y=rowY[r];const bool on=choice==r;
    suiPanel(bottom,4,y,252,y+rowHeight,on?kSuiSelCell:kSuiCell,on?kSuiSelLo:kSuiCellLo,on?kSuiSelEdge:kSuiEdge,on);
    gtxt(bottom,10,y+2,names[r],kSuiName,kSuiNameSh);if(r==active)gtxt(bottom,240-gw("Active"),y+2,"Active",kSuiVal,kSuiValSh);
    const GtsPreview&pv=preview[r];if(!pv.state)continue;const int t=y+rowHeight+2;
    suiPanel(bottom,14,t,252,t+previewHeight,kSuiCell,kSuiCellLo,kSuiEdge,false);char line[64];
    if(pv.species&&pv.species<494){if(pv.wanted&&pv.wanted<494)snprintf(line,sizeof line,"%s Lv.%u, seeking %s",kP3dsSpec[pv.species],(unsigned)pv.level,kP3dsSpec[pv.wanted]);else snprintf(line,sizeof line,"%s Lv.%u deposited",kP3dsSpec[pv.species],(unsigned)pv.level);}
    else snprintf(line,sizeof line,"%s",pv.state==GTS_PREVIEW_PICKUP?"Trade ready for pickup":"Pokemon deposited");
    suiDrawS(bottom,20,t+6,line,kSuiName,kSuiNameSh);}
   gtxt(bottom,12,174,"A: Select   B: Back",kSuiName,kSuiNameSh);}
  {extern int P3DS_GpuBotSame;P3DS_GpuBotSame=0;}
  if(mode&&gpu)P3DS_GpuPresent(top,bottom,mode,true,nullptr);else{present(GFX_TOP,400,top,0);present(GFX_BOTTOM,320,bottom,0);gfxFlushBuffers();gfxSwapBuffers();}
  gspWaitForVBlank();}
 for(int f=0;f<120&&aptMainLoop()&&(hidKeysHeld()&(KEY_A|KEY_B|KEY_TOUCH));++f){gspWaitForVBlank();hidScanInput();}
 free(bottom);P3DS_GpuInvalidateBot();P3DS_MenuOpen=0;}

/* SoulSilver 0120 tracker page (Platinum): the roaming Pokemon and the Day-Care, from the field publisher in p3ds_follower.h */
#include "tracker.h"
extern "C" char P3DS_TrkPlace[4][24];
static void runTrackerPage(const u32*top,int mode,bool gpu){
 u32*bottom=(u32*)malloc(256*192*sizeof(u32));if(!bottom)return;
 P3DS_MenuOpen=1;P3DS_LowerMenuDraw=0;u32 held=hidKeysHeld();
 while(aptMainLoop()){hidScanInput();held&=hidKeysHeld();const u32 pressed=hidKeysDown()&~held;if(pressed&(KEY_A|KEY_B|KEY_START|KEY_TOUCH))break;
  suiBackground(bottom);gtxt(bottom,12,6,"Roamers & Day Care",kMsgFg,kMsgSh);char l[64];int y=28;
  if(!P3DS_TrkOn()){gtxt(bottom,12,44,"Turn on Tracker (BATTLE tab).",kMsgFg,kMsgSh);}
  else if(!P3DS_TrkReady){gtxt(bottom,12,44,"Walk around a little first.",kMsgFg,kMsgSh);}
  else{int any=0;
   for(int i=0;i<4;i++){const P3DS_TrkRoamer&rm=P3DS_TrkRoam[i];if(!rm.active)continue;any=1;
    suiPanel(bottom,4,y,252,y+30,kSuiCell,kSuiCellLo,kSuiEdge,false);
    snprintf(l,sizeof l,"%s  Lv.%d  HP %d",rm.species>0&&rm.species<494?kP3dsSpec[rm.species]:"?",rm.level,rm.hp);gtxt(bottom,10,y+1,l,kSuiName,kSuiNameSh);
    suiDrawS(bottom,10,y+17,P3DS_TrkPlace[i][0]?P3DS_TrkPlace[i]:"somewhere in Sinnoh",kSuiVal,kSuiValSh);y+=33;}
   if(!any){gtxt(bottom,12,y+2,"No Pokemon is roaming.",kSuiName,kSuiNameSh);y+=22;}
   const P3DS_TrkDaycare&dc=P3DS_TrkDc;suiPanel(bottom,4,y,252,y+44,kSuiCell,kSuiCellLo,kSuiEdge,false);
   if(dc.n==0)snprintf(l,sizeof l,"Day Care: empty");else if(dc.n==1)snprintf(l,sizeof l,"Day Care: %s",kP3dsSpec[dc.sp[0]<494?dc.sp[0]:0]);
   else snprintf(l,sizeof l,"Day Care: %s + %s",kP3dsSpec[dc.sp[0]<494?dc.sp[0]:0],kP3dsSpec[dc.sp[1]<494?dc.sp[1]:0]);
   gtxt(bottom,10,y+1,l,kSuiName,kSuiNameSh);
   if(dc.n==2){if(dc.egg)snprintf(l,sizeof l,"An Egg is waiting!");else snprintf(l,sizeof l,"Egg chance %d%%%s, next check in %d steps",dc.compat,dc.masuda?" (Masuda)":"",dc.toCheck);suiDrawS(bottom,10,y+19,l,kSuiVal,kSuiValSh);}}
  gtxt(bottom,12,174,"A / B: Back",kSuiName,kSuiNameSh);
  {extern int P3DS_GpuBotSame;P3DS_GpuBotSame=0;}
  if(mode&&gpu)P3DS_GpuPresent(top,bottom,mode,true,nullptr);else{present(GFX_TOP,400,top,0);present(GFX_BOTTOM,320,bottom,0);gfxFlushBuffers();gfxSwapBuffers();}
  gspWaitForVBlank();}
 for(int f=0;f<120&&aptMainLoop()&&(hidKeysHeld()&(KEY_A|KEY_B|KEY_TOUCH));++f){gspWaitForVBlank();hidScanInput();}
 free(bottom);P3DS_GpuInvalidateBot();P3DS_MenuOpen=0;}

struct PAppRow{int it;const char*nm,*tip;};
enum{kPMaxRows=40};
struct PAppTab{int n;PAppRow r[kPMaxRows];};
enum{kPPlayerTabs=5,kPTotalTabs=6};
static const char*const kPTabNm[kPTotalTabs]={"LOOK","BATTLE","PLAY","PALS","SYSTEM","DEBUG"};
static const PAppTab kPTabs[kPTotalTabs]={
 {18,{{9,"Camera zoom","Overworld camera distance. 0 is the original; minus is closer, plus is farther."},
     {133,"Ambient weather","Weekly seasons bring showers to clear towns and routes, and snow in winter. Only the picture: battles see the real weather."},
     {134,"Seasonal trees","Tree leaves change with the weekly season: blossom, summer green, autumn orange, winter frost. Applies when an area loads."},
     {10,"3D strength","How deep the 3D slider looks: 1X is normal, 2X is stronger."},
     {20,"Battle 3D depth","How far battle backgrounds sit behind the screen with the 3D slider up. FOE DEEPER: the foe sits further back."},
     {124,"Sharp game text","Every text box drawn at the 3DS resolution instead of the stretched DS picture. Needs 1.25X or FILL."},
     {127,"Black 2 sprites","Animated Black 2 / White 2 Pokemon in battle and on big pictures. Needs b2sprites.bin from the builder."},
     {2,"Scale","How the DS picture fills the screen: 1:1, 1.25X, or FILL."},
     {1,"Wide view","More of the overworld at the sides of the top screen."},
     {71,"Wide menu layouts","Battle, menus and boxes spread over the whole wide screen."},
     {42,"Weather in strips","Rain, snow and sand also fall in the wide side strips."},
     {64,"Time-of-day tint","The overworld gets warmer in the evening and darker at night."},
     {59,"Distortion 3D depth","3D depth in the Distortion World."},
     {60,"Distortion debris","Drifting debris and dust over the Distortion World."},
     {67,"Distortion camera roll","A slow camera roll in the Distortion World."},
     {61,"Uneasy Pokemon","Your follower looks uneasy in the Distortion World."},
     {62,"Distant cry","Giratina's cry now and then in the Distortion World."},
     {0,"FPS counter","Shows the frame rate on screen."}}},
 {23,{{58,"Faster battles","1: skips plain move animations. 2: HP bars also drain 3 times faster."},
     {57,"Battle Y screen","Y at the command menu: catch info for wild Pokemon, the foe's moves and team in trainer battles."},
     {122,"Damage check","Adds a rough damage and speed page to the battle Y screen (singles)."},
     {116,"Move type hint","SUPER / RESIST / NO EFFECT tags on the FIGHT menu's move buttons."},
     {108,"SELECT = reset","SELECT soft-resets the game in a wild battle (never on a shiny). Handy for hunting."},
     {107,"Skip intro on reset","After a soft reset, skips the logos and title straight to the menu."},
     {72,"Shiny hunt suite","Hunt tools: chain, Masuda and shiny counters on the hunt page, shiny chime."},
     {103,"Shiny odds","Wild shiny chance: 1/8192 is the real game, lower numbers are easier."},
     {63,"Encounter counter","Counts encounters per species for your hunts."},
     {123,"Hunt stats (ZR)","ZR in the overworld opens the hunt stats screen. Needs the encounter counter."},
     {L_HUNT,"Hunt stats page","Opens the hunt page: target, counts, odds, chains."},
     {46,"Visible wild Pokemon","Wild Pokemon walk in the grass; walk into one to battle it."},
     {47,"Visible at once","How many visible wild Pokemon can be out at the same time."},
     {130,"Wild Pokemon behave","Visible ones have a pace, look around, come over when you stand still and go home at dusk."},
     {131,"Wild Pokemon call","Visible ones nearby cry now and then, quieter further away."},
     {105,"Visible shiny sparkle","A visible shiny twinkles, and the battle is shiny too."},
     {48,"Only visible battle","No random encounters while visible Pokemon are on: only walked-into ones."},
     {L_MAP,"Wild Pokemon here","Opens the list of this area's wild Pokemon with levels and chances."},
     {102,"Swap catch into party","A caught Pokemon can go straight into your party."},
     {100,"Exp. Share: party","The Exp. Share gives experience to the whole party."},
     {106,"Low battery warning","Warns when the battery is low and saves the hunt file."},
     {120,"Tracker","Follows the roaming Pokemon and the Day-Care pair (egg chance, Masuda)."},
     {L_TRACK,"Roamers & Day Care","Opens the tracker page: where each roaming Pokemon is now, and the Day-Care."}}},
 {29,{{23,"Reusable TMs","TMs are not used up when you teach a move."},
     {135,"Event Pokemon w/o event","Darkrai, Shaymin, Arceus and the Rotom room work without the Mystery Gift items. The story conditions still apply."},
     {50,"Field moves w/o HM","Field moves like Cut and Surf work without a Pokemon that knows them."},
     {25,"Repel prompt","When a Repel runs out, asks if you want to use another."},
     {26,"IV/EV on summary","Shows IVs and EVs on the summary screen."},
     {117,"Ribbon checklist (X)","Press X on a summary screen to see which ribbons it has and is missing."},
     {118,"Celebrations","Little celebrations for milestones (badges, shinies, the League)."},
     {119,"Achievement toasts","Pop-up on the top screen when you unlock an achievement."},
     {121,"Pokemon menu X/Y","In the Pokemon menu, X opens the Move Reminder and Y the Move Tutor."},
     {22,"Fast text","Speeds up text boxes: 2X, 4X, or instant. Never skips a prompt."},
     {69,"Hold B skips text","Holding B prints text instantly and moves on. Never picks a choice."},
     {70,"Auto-advance text","Text boxes move on by themselves after a moment."},
     {65,"Hidden item ping","A ping when you walk near a hidden item."},
     {101,"Auto run","Run by default; B walks instead."},
     {125,"Mobile PC","After the Hall of Fame, a Boxes tile in the Quick menu opens your PC boxes anywhere."},
     {128,"GBA slot","A GBA game from sdmc:/3ds/platinum3ds/slot2/ in the GBA slot: Pal Park migrate. Next start."},
     {129,"GBA pak","Which .gba in the slot2 folder (by name) is in the slot. Next start."},
     {L_MENU,"Quick menu","Opens the Pokemon / Bag / Pokedex / Boxes quick menu."},
     {24,"No wild battles","Cheat: random wild encounters never happen."},
     {27,"Infinite money","Cheat: money never runs out."},
     {28,"Always catch","Cheat: every Poke Ball catches."},
     {29,"Shiny wild","Cheat: every wild Pokemon is shiny."},
     {30,"Fast eggs","Cheat: eggs hatch quickly."},
     {31,"EXP boost","Cheat: more experience from battles."},
     {32,"Rare Candy 99","Cheat: 99 Rare Candies in the bag."},
     {53,"Infinite PP","Cheat: your moves use no PP."},
     {54,"Fast running (B)","Cheat: holding B runs at bike speed."},
     {55,"Walls: hold R","Cheat: hold R to walk through walls."},
     {56,"Time of day","Cheat: fixed time of day (the save clock is untouched)."}}},
 {12,{{14,"Following Pokemon","A Pokemon from your party follows you."},
     {16,"Follower slot","Which party slot follows (LEAD = the first one)."},
     {17,"Follower talk","Press A to talk to your follower."},
     {34,"Follower cry","Your follower cries when you talk to it."},
     {35,"Finds items","Your follower sometimes finds items."},
     {37,"Emote bubbles","Little mood bubbles over your follower."},
     {36,"Ball pop-out","Your follower pops out of its ball."},
     {33,"Area talk","Your follower comments on places."},
     {38,"Friendship talk","Lines that depend on friendship."},
     {39,"Idle look-around","Your follower looks around when you stand still."},
     {40,"Follower gap","Your follower keeps a step's distance."},
     {41,"Behind player","Your follower always walks behind you."}}},
 {16,{{3,"C-stick","What the C-stick does."},
     {4,"ZL run toggle","ZL switches running on and off."},
     {5,"ZR = item (Y)","ZR uses your registered item (then ZR hunt stats is off)."},
     {6,"Circle pad D-pad","The circle pad acts as the D-pad."},
     {66,"Touch quick menu","Tap the bottom screen corner for the quick menu."},
     {68,"Bottom button strip","Button strip on the bottom screen in the field."},
     {77,"Bottom icon menu","Icon menu around the bottom screen in the field (GabeSud0)."},
     {132,"GTS server picker","The main menu's Wi-Fi settings row opens Online Settings: pick the GTS server (Poke Classic or StarMe)."},
     {51,"Save backups","Keeps 8 rotating copies of your save: at launch, after each save, at exit."},
     {115,"Export save","Press A, then close this menu: copies your save to the SD export folder."},
     {18,"Menu music","The game's music keeps playing in this menu."},
     {111,"Audio style","CLEAN, LIGHT or the raw DS mix."},
     {113,"Audio gain","Overall loudness."},
     {112,"Audio stall fade","Fades the sound instead of a buzz when the game stalls."},
     {8,"Log detail","How much the log file records."},
     {52,"Log size limit","How big the log file may grow."}}},
 {38,{{11,"Fast upload","Debug."},{12,"Fast 3D convert","Debug."},{15,"GPU compose","Debug."},{19,"GPU compose 3D","Debug."},{13,"Split top (mono)","Debug."},
     {7,"Wide view FX","Debug."},{21,"Wide haze","Debug."},{43,"Edge fill","Debug."},{44,"Side fill in menus","Debug."},{78,"Load-hitch profiler","Debug."},
     {79,"Fast heap free","Debug."},{80,"Skip game cache flushes","Debug."},{82,"Hunt file bg save","Debug."},{73,"Wide layout early","Debug."},{81,"Wide 3D fade-in","Debug."},
     {74,"Wide layout 3D depth","Debug."},{75,"Wide redraw changed lines","Debug."},{76,"Ignore timing registers","Debug."},{83,"Wide compose on 2 cores","Debug."},{85,"Battle frame log","Debug."},
     {87,"3D convert on 2 cores","Debug."},{88,"Flat 3D when slow","Debug."},{89,"Wide view windows","Debug."},{91,"GPU blend by 3D","Debug."},{92,"GPU 3D direct","Debug."},
     {93,"Wide on GPU","Debug: the wide battle and menu layout drawn on the GPU."},{94,"GPU fades","Debug."},{95,"Wide GPU blend check","Debug."},{96,"Wide Pokedex","Debug."},{97,"Copy eye when 3D empty","Debug."},
     {104,"RNG info (hunt stats)","Debug."},{109,"Speed tweaks","Debug: 1-7, higher = more speed shortcuts."},{110,"Sharp 3D","Debug: SHARP, FAST 3D or FAST AUTO."},{114,"Stereo depth","Debug."},{126,"Mobile PC before League","Debug: Mobile PC before the Hall of Fame (runtime only)."},
     {49,"Visible wild test","Debug: one test sprite in front of you."},{45,"Side fill","Debug."},{86,"Right eye split","Debug."}}},
};
static void runApp(const u32*top,int mode,bool gpu){
 const size_t PX=256*192;u32*base=(u32*)malloc(PX*4*3);if(!base){PSPNativeMemLog("[APP] no memory");return;}u32*img=base+PX,*bimg=img+PX;memcpy(base,top,PX*4);
 enum{N=MENU_N};int val[N];for(int i=0;i<N;i++)val[i]=menuGet(i);int old[N];memcpy(old,val,sizeof old);
 for(int i=0;i<256*192;i++){u32 c=base[i];base[i]=((c>>1)&0x007F7F7Fu)|(c&0xFF000000u);}
 P3DS_MenuOpen=1;P3DS_LowerMenuDraw=0;P3DS_Toast("");toastAt=0;
 static int tab=0;static bool dbgOn=false;int NT=dbgOn?kPTotalTabs:kPPlayerTabs;if(tab>=NT)tab=0;int sel=0,ltop=0,openPage=-1;
 PSPNativeMemLog("[APP] settings open (tab %s, font %d, frame %d)",kPTabNm[tab],P3DS_GameFontReady(),P3DS_GameBoxReady());
 u32 held=hidKeysHeld();bool wasTouch=(held&KEY_TOUCH)!=0;u64 t0=svcGetSystemTick();
 auto step=[&](int i,int dv){const int it=kPTabs[tab].r[i].it;if(it>=L_HUNT){if(dv>0)openPage=it;return;}const int nn=mItems[it].n;val[it]=(val[it]+nn+dv)%nn;PSPNativeMemLog("[APP] %s -> %s",kPTabs[tab].r[i].nm,mItems[it].lab[val[it]]);};
 while(aptMainLoop()){
  hidScanInput();u32 kh=hidKeysHeld(),kd=hidKeysDown()&~held;held&=kh;
  if(kd&(KEY_B|KEY_START))break;if((kd&KEY_Y)&&(kh&KEY_L)&&(kh&KEY_R))break;
  if(kd&KEY_SELECT){dbgOn=!dbgOn;NT=dbgOn?kPTotalTabs:kPPlayerTabs;if(tab>=NT)tab=0;sel=0;ltop=0;PSPNativeMemLog("[APP] debug tab %s",dbgOn?"shown":"hidden");}
  if(kd&KEY_L){tab=(tab+NT-1)%NT;sel=0;ltop=0;}if(kd&KEY_R){tab=(tab+1)%NT;sel=0;ltop=0;}
  const PAppTab&T=kPTabs[tab];const int cnt=T.n;
  if(kd&KEY_DUP)sel=(sel+cnt-1)%cnt;if(kd&KEY_DDOWN)sel=(sel+1)%cnt;if(sel>=cnt)sel=cnt-1;
  {const int dv=(kd&(KEY_A|KEY_DRIGHT))?1:(kd&(KEY_X|KEY_DLEFT))?-1:0;if(dv)step(sel,dv);}
  bool tdown=(kh&KEY_TOUCH)!=0;if(tdown&&!wasTouch){touchPosition tp;hidTouchRead(&tp);int x,y;if(touchToDS(tp.px,tp.py,&x,&y)){
    if(y<kSaTabH){int t=x*NT/256;if(t>=0&&t<NT&&t!=tab){tab=t;sel=0;ltop=0;}}
    else{const int r=(y-kSaListY)/kSaRowPitch,i=ltop+r;if(y>=kSaListY&&y<kSaFootY&&r<kSaRows&&i<cnt&&x<246){const bool was=sel==i;sel=i;
       const int dv=x>=kSaValX?(x<(kSaValX+240)/2?-1:1):was?1:0;if(dv)step(i,dv);}}}}
  wasTouch=tdown;
  if(openPage>=0)break;
  memcpy(img,base,PX*4);
  if(useGF()){static const SUiText kTopText={gtxt,gw,suiDrawS,gwS};suiAppTop(img,kTopText,kAppVer,__DATE__,kPTabNm[tab],T.r[sel].nm,T.r[sel].tip);}
  else{menuBox(img,8,10,248,52,0x00703020u,0x00F0E0D0u);drawBig(img,16,18,"SETTINGS (PAUSED)",0x00F8F0E0u);}
  static SBtxList*appTx;const bool txApp=mode&&gpu&&btxMode();if(txApp&&!appTx)appTx=(SBtxList*)calloc(1,sizeof(SBtxList));if(txApp&&appTx)btxBegin(*gBtx,bimg);
  SUiCell cells[kPMaxRows];char labs[kPMaxRows][24];
  for(int i=0;i<cnt;i++){const int it=T.r[i].it;if(it>=L_HUNT){snprintf(labs[i],sizeof labs[i],"Open");cells[i]={T.r[i].nm,labs[i],i==sel,false};continue;}
   prettyLab(labs[i],mItems[it].lab[val[it]],sizeof labs[i]);cells[i]={T.r[i].nm,labs[i],i==sel,val[it]!=old[it]};}
  if(sel<ltop)ltop=sel;if(sel>=ltop+kSaRows)ltop=sel-kSaRows+1;if(ltop>cnt-kSaRows)ltop=cnt>kSaRows?cnt-kSaRows:0;if(ltop<0)ltop=0;
  suiAppBottom(bimg,kSuiText,kPTabNm,NT,tab,cells,cnt,ltop);
  if(txApp&&appTx){btxEnd(*gBtx,*appTx);if(appTx->gen!=txShownGen||txShownL!=appTx)btxLog("settings",appTx);btxShow(appTx);}else{if(gBtx)gBtx->on=false;P3DS_TxOn=0;}
  {extern int P3DS_GpuBotSame;P3DS_GpuBotSame=0;}
  if(mode&&gpu)P3DS_GpuPresent(img,bimg,mode,true,nullptr);
  else{P3DS_PresentDeferSettle();present(GFX_TOP,400,img,0);present(GFX_BOTTOM,320,bimg,0);gfxFlushBuffers();gfxSwapBuffers();}
  gspWaitForVBlank();
  if(menuAudioCfg()){static u64 mt;u64 nw=svcGetSystemTick();unsigned us=mt&&nw>mt?(unsigned)((nw-mt)/CPU_TICKS_PER_USEC):16667;mt=nw;if(us>66666)us=16667;PSPNativeSoundAdvance(us);}
 }
 for(int i=0;i<120&&aptMainLoop();i++){hidScanInput();if(!(hidKeysHeld()&(KEY_A|KEY_B|KEY_X|KEY_START|KEY_Y|KEY_L|KEY_R|KEY_TOUCH|KEY_DUP|KEY_DDOWN|KEY_DLEFT|KEY_DRIGHT)))break;gspWaitForVBlank();}
 free(base);P3DS_TxOn=0;P3DS_GpuInvalidateBot();int ch=0;for(int i=0;i<N;i++)if(val[i]!=old[i]){menuSet(i,val[i]);ch++;PSPNativeMemLog("[APP] saved %s=%s",mItems[i].key,mItems[i].lab[val[i]]);}
 P3DS_MenuOpen=0;PSPNativeMemLog("[APP] settings closed after %u ms, %d change(s)%s",(unsigned)((svcGetSystemTick()-t0)/CPU_TICKS_PER_MSEC),ch,openPage>=0?", opening a page":"");
 if(openPage==L_TRACK){runTrackerPage(top,mode,gpu);return;}
 if(openPage>=0){appStartTab=tabFind(openPage==L_HUNT?-3:openPage==L_MAP?-2:-1);runAppPages(top,mode,gpu);}
}

/* ss3ds 0030 catch info: Y at a wild battle's command menu (catch_overlay=1) shows what the game knows about the foe.
   The snapshot is P3DS_CatchInfo (battle_controller_player.c hook, read-only); [0] = valid, [19] = battle counter. */
static void catchTick(){static int seen;volatile int*ci=P3DS_CatchInfo;
 if(!ci[0]||ci[19]==seen)return;seen=ci[19];
 PSPNativeMemLog("[CATCH] wild #%d Lv%d hp %d/%d nature %d ability %d IVs %d/%d/%d/%d/%d/%d catch rate %d %s%s",ci[1],ci[2],ci[3],ci[4],ci[7],ci[6],ci[9],ci[10],ci[11],ci[12],ci[13],ci[14],ci[15],ci[16]?"(caught before)":"(new)",ci[18]?" SHINY":"");
 if(ci[18]&&P3DS_CfgInt("catch_overlay",0)){P3DS_Toast("SHINY POKEMON!");PSPNativeMemLog("[CATCH] shiny alert");}}
/* 2026-11-19 battle Y screen, rework (Nathan: "total reworking, ugly, old text", "must not interrupt the battle at all", "Ultra Sun and Moon pretty").
   It is an OVERLAY now, not a modal loop: the game, the top screen, the music and the battle all keep running. Y (catch_overlay or the hunt
   counter on, command menu up) swaps the bottom picture for this screen; while it is up the game gets no buttons or touches (P3DS_CatchMask,
   psp_ctru.c / main3ds.c) and for a few frames after, until every key is released, so the B that closes it cannot also cancel a game menu.
   Pages (tabs on the screen): wild = INFO, CATCH, [HUNT], [DAMAGE]; trainer = FOE, PARTY, [DAMAGE]. Tap a tab, swipe, Left/Right or L/R to
   change page; B, A, Y, Start, ZR or the X chip close it. Drawing lives in ss3ds_catchui.h (sharp native text through the same recorder
   as the settings app); this file only gathers the snapshots and runs the input. Logs: [CATCH] open / page / close with the battle frames passed. */
#include "ss3ds_catchui.h"
extern "C" volatile int P3DS_FoeInfo[40];
extern "C" volatile int P3DS_DmgInfo[32];
static const char*cStatus(int st){return (st&7)?"SLP":(st&0x20)?"FRZ":(st&0x40)?"PAR":(st&0x10)?"BRN":(st&0x88)?"PSN":"";}
static void cFill(u32*b,int x0,int y0,int x1,int y1,u32 c){for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++)suiPx(b,x,y,c);}
static void cBar(u32*b,int x,int y,int w,int hp,int mx){cFill(b,x-1,y-1,x+w+1,y+7,SUI_RGB(8,12,24));cFill(b,x,y,x+w,y+6,SUI_RGB(52,60,84));
 if(mx<=0||hp<=0)return;int f=hp*w/mx;if(f<1)f=1;const int pc=hp*100/mx;cFill(b,x,y,x+f,y+6,pc>50?SUI_RGB(64,216,96):pc>20?SUI_RGB(240,200,48):SUI_RGB(232,64,56));}
/* battery_warn=0 (default off, Battle tab "Low battery warning"; Platinum run 119): every 10 s the 3DS battery level (PTMU, 0-5);
   at 1 or less and not charging: a toast every minute and the hunt counters are written to the SD at once. */
static void batteryTick(){static u64 last;static int inited,warned;const u64 now=svcGetSystemTick();
 if(last&&now-last<10000ull*CPU_TICKS_PER_MSEC)return;last=now;if(!P3DS_CfgInt("battery_warn",0)){warned=0;return;}
 if(!inited){inited=R_SUCCEEDED(ptmuInit())?1:-1;PSPNativeMemLog("[BATTERY] ptmu %s",inited>0?"ok":"failed");}if(inited<0)return;
 u8 lv=5,ch=0;if(R_FAILED(PTMU_GetBatteryLevel(&lv))||R_FAILED(PTMU_GetBatteryChargeState(&ch)))return;
 if(lv>1||ch){warned=0;return;}
 if(warned++%6==0){P3DS_Toast("BATTERY LOW: SAVE NOW");P3DS_HuntFlush();PSPNativeMemLog("[BATTERY] low battery (level %u), hunt.txt written",(unsigned)lv);}}
static int hintMultCb(int a,int d){return hintMult(a,d);}
struct CyState{bool open,release,trainer,tPrev,zrPrev,field;/* field: the overworld HUNT STATS screen (hsOpen below), not the battle Y screen */int pg,n,kinds[4];u32 key;u64 t0;unsigned frames;u32*img;SBtxList*tx;int tx0,ty0,tx1,ty1;};
static CyState cy;
struct CyData{cc::Wild w;cc::Foe fo;cc::Dmg dm;cc::Hunt hu;char wn[24],wab[24],fn[24],fab[24],dn[24],hn[24];bool huntPg,dmgPg;};
static void cyGather(CyData&D,bool*trainer,int*kinds,int*n){
 static const char*const nat[25]={"Hardy","Lonely","Brave","Adamant","Naughty","Bold","Docile","Relaxed","Impish","Lax","Timid","Hasty","Serious","Jolly","Naive","Modest","Mild","Quiet","Bashful","Rash","Calm","Gentle","Sassy","Careful","Quirky"};
 for(int i=0;i<20;i++)D.w.c[i]=P3DS_CatchInfo[i];for(int i=0;i<40;i++)D.fo.f[i]=P3DS_FoeInfo[i];for(int i=0;i<32;i++)D.dm.d[i]=P3DS_DmgInfo[i];
 const int*c=D.w.c;const int*f=D.fo.f;const int*d=D.dm.d;const bool tr=!c[0]&&f[0];*trainer=tr;
 D.huntPg=!tr&&P3DS_HuntOn()&&c[1]>0;D.dmgPg=P3DS_CfgInt("dmg_check",0)&&d[0];
 snprintf(D.wn,sizeof D.wn,"%s",c[1]>0&&c[1]<494?kP3dsSpec[c[1]]:"?");D.w.name=D.wn;D.w.ability=c[6]>=0&&c[6]<124?kP3dsAbil[c[6]]:"?";D.w.nature=nat[c[7]%25];
 D.w.metOn=P3DS_CfgInt("hunt_counter",0)!=0;D.w.met=D.w.metOn?P3DS_HuntCount(c[1]):0;
 snprintf(D.fn,sizeof D.fn,"%s",f[4]>0&&f[4]<494?kP3dsSpec[f[4]]:"?");D.fo.name=D.fn;D.fo.ability=f[9]>=0&&f[9]<124?kP3dsAbil[f[9]]:"?";
 D.fo.moves=kP3dsMove;D.fo.nMoves=468;D.fo.spec=kP3dsSpec;D.fo.nSpec=494;D.fo.moveType=nullptr;
 snprintf(D.dn,sizeof D.dn,"%s",d[17]>0&&d[17]<494?kP3dsSpec[d[17]]:"?");D.dm.foeName=D.dn;D.dm.moves=kP3dsMove;D.dm.nMoves=468;D.dm.mult=hintMultCb;
 if(D.huntPg){const P3DS_HuntRec&r=*P3DS_HuntGet(c[1]);const int odds=P3DS_HuntRateOdds();snprintf(D.hn,sizeof D.hn,"%s",D.wn);
  D.hu={D.hn,r.enc,r.streak,r.resets,r.shinies,r.secs,r.start,r.lastShinyAt,odds,P3DS_HuntEncFor(0.5,odds),P3DS_HuntEncFor(0.9,odds),P3DS_HuntChanceBy(r.streak,odds),P3DS_Cheat(2)!=0};}
 int k=0;if(tr){kinds[k++]=cc::P_FOE;kinds[k++]=cc::P_PARTY;}else{kinds[k++]=cc::P_INFO;kinds[k++]=cc::P_CATCH;if(D.huntPg)kinds[k++]=cc::P_HUNT;}
 if(D.dmgPg)kinds[k++]=cc::P_DMG;*n=k;}
static u32 cyHash(const CyData&D,int pg){u32 h=2166136261u^(u32)pg;auto mx=[&](const int*a,int n){for(int i=0;i<n;i++){h^=(u32)a[i];h*=16777619u;}};mx(D.w.c,20);mx(D.fo.f,40);mx(D.dm.d,32);
 h^=D.huntPg?(u32)(D.hu.enc*31+D.hu.streak):0;h*=16777619u;return h;}
static void cyClose(const char*why){if(!cy.open)return;cy.open=false;cy.release=true;P3DS_CatchMask=1;free(cy.img);cy.img=nullptr;P3DS_GpuInvalidateBot();P3DS_TxOn=0;txShownL=nullptr;
 if(cy.field)BLINV; /* the field panel picture is recomposed on the next present */
 PSPNativeMemLog("%s screen closed (%s) after %u ms, %u %s frames ran underneath, keys held back until released",cy.field?"[HSTAT] stats":"[CATCH]",why,(unsigned)((svcGetSystemTick()-cy.t0)/CPU_TICKS_PER_MSEC),cy.frames,cy.field?"field":"battle");}
static void cyOpen(){if(cy.open)return;static CyData D;bool tr;cyGather(D,&tr,cy.kinds,&cy.n);cy.img=(u32*)malloc(256*192*4);if(!cy.img){PSPNativeMemLog("[CATCH] no memory for the screen (%u bytes)",256u*192u*4u);return;}
 cy.open=true;cy.field=false;cy.release=false;cy.trainer=tr;cy.pg=0;cy.key=0;cy.frames=0;cy.t0=svcGetSystemTick();cy.tPrev=hidKeysHeld()&KEY_TOUCH;cy.zrPrev=true;P3DS_CatchMask=1;
 const int*c=D.w.c;const int*f=D.fo.f;
 PSPNativeMemLog("[CATCH] screen opened as an overlay, the battle keeps running (%s #%d, %d pages)",tr?"trainer foe":"wild",tr?f[4]:c[1],cy.n);
 if(tr)PSPNativeMemLog("[CATCH] foe Lv%d hp %d/%d status %d ability %d moves %d/%d/%d/%d pp %d/%d/%d/%d party %d",f[5],f[6],f[7],f[8],f[9],f[11],f[12],f[13],f[14],f[15],f[16],f[17],f[18],f[2]);
 if(D.dmgPg){const int*d=D.dm.d;PSPNativeMemLog("[DMG] me Lv%d atk %d spa %d spe %d st %d types %d/%d | foe #%d Lv%d def %d spd %d spe %d hp %d/%d st %d types %d/%d | moves %d/%d/%d/%d types %d/%d/%d/%d power %d/%d/%d/%d",d[2],d[3],d[4],d[5],d[6],d[7],d[8],d[17],d[9],d[10],d[11],d[12],d[13],d[30],d[16],d[14],d[15],d[18],d[19],d[20],d[21],d[22],d[23],d[24],d[25],d[26],d[27],d[28],d[29]);}}
/* every present while it is up: returns the picture to show on the bottom screen (nullptr = the game's own). txOk = the sharp text layer can be used */
static const u32*hsFrame(bool txOk);static void hsInput(u32 kd,u32 kh);
static const u32*cyFrame(bool txOk){
 if(!cy.open)return nullptr;
 if(cy.field)return hsFrame(txOk);
 if(P3DS_InField||!(P3DS_CatchInfo[0]||P3DS_FoeInfo[0])){cyClose("battle moved on");return nullptr;}
 static CyData D;bool tr;int kinds[4],n;cyGather(D,&tr,kinds,&n);if(tr!=cy.trainer||n!=cy.n){cy.trainer=tr;cy.n=n;if(cy.pg>=n)cy.pg=0;}memcpy(cy.kinds,kinds,sizeof kinds);
 if(cy.pg>=cy.n)cy.pg=0;cy.frames++;
 const u32 key=cyHash(D,cy.pg)^(txOk?0x9E3779B9u:0);
 if(key!=cy.key||!cy.tx){cy.key=key;if(!cy.tx)cy.tx=(SBtxList*)calloc(1,sizeof(SBtxList));
  const bool rec=txOk&&cy.tx;if(rec)btxBegin(*gBtx,cy.img);const SUiText&T=rec?kSuiText:SUiText{suiDraw,gw,suiDrawS,gwS};const int k=cy.kinds[cy.pg];
  switch(k){case cc::P_INFO:cc::wildInfo(cy.img,T,D.w,cy.n,cy.kinds,cy.pg);break;case cc::P_CATCH:cc::wildCatch(cy.img,T,D.w,cy.n,cy.kinds,cy.pg);break;
   case cc::P_HUNT:cc::huntPage(cy.img,T,D.hu,cy.n,cy.kinds,cy.pg);break;case cc::P_DMG:cc::dmgPage(cy.img,T,D.dm,cy.n,cy.kinds,cy.pg);break;
   case cc::P_FOE:cc::foePage(cy.img,T,D.fo,cy.n,cy.kinds,cy.pg);break;default:cc::partyPage(cy.img,T,D.fo,cy.n,cy.kinds,cy.pg);}
  if(rec){btxEnd(*gBtx,*cy.tx);btxLog("battle Y screen",cy.tx);}else{if(gBtx)gBtx->on=false;}}
 if(txOk&&cy.tx)btxShow(cy.tx);else P3DS_TxOn=0;
 P3DS_OvN=0;return cy.img;}
/* input, once per present while it is up (hid state, the game is masked from these keys by P3DS_CatchMask) */
static void cyInput(u32 kd,u32 kh){
 if(!cy.open)return;
 if(cy.field){hsInput(kd,kh);return;}
 const bool zrEdge=(kh&KEY_ZR)&&!cy.zrPrev;cy.zrPrev=(kh&KEY_ZR)!=0;
 if((kd&(KEY_B|KEY_A|KEY_Y|KEY_START))||zrEdge){cyClose("button");return;}
 int np=cy.pg;if(cy.n>1){if(kd&(KEY_RIGHT|KEY_R|KEY_DRIGHT))np=(cy.pg+1)%cy.n;if(kd&(KEY_LEFT|KEY_L|KEY_DLEFT))np=(cy.pg+cy.n-1)%cy.n;}
 const bool tNow=(kh&KEY_TOUCH)!=0;
 if(tNow){touchPosition tp;hidTouchRead(&tp);int x,y;if(touchToDS(tp.px,tp.py,&x,&y)){if(!cy.tPrev){cy.tx0=x;cy.ty0=y;}cy.tx1=x;cy.ty1=y;}}
 else if(cy.tPrev){const int dx=cy.tx1-cy.tx0,dy=cy.ty1-cy.ty0;
  if(cy.n>1&&(dx>=36||dx<=-36)&&(dy<40&&dy>-40))np=dx<0?(cy.pg+1)%cy.n:(cy.pg+cy.n-1)%cy.n;
  else if(dx<10&&dx>-10&&dy<10&&dy>-10){const int h=cc::hit(cy.n,cy.tx0,cy.ty0);if(h==-2){cyClose("X tapped");cy.tPrev=false;return;}if(h>=0&&h<cy.n)np=h;}}
 cy.tPrev=tNow;
 if(np!=cy.pg){cy.pg=np;PSPNativeMemLog("[CATCH] page %d/%d: %s",np+1,cy.n,cc::kPageName[cy.kinds[np]]);}}
/* after it closed: keep the game masked until every key and the stylus are up, so the closing press never reaches the game as a new press */
static void cyTick(u32 kh){if(cy.release&&!cy.open){const u32 m=KEY_A|KEY_B|KEY_X|KEY_Y|KEY_L|KEY_R|KEY_ZL|KEY_ZR|KEY_START|KEY_DUP|KEY_DDOWN|KEY_DLEFT|KEY_DRIGHT|KEY_TOUCH;if(!(kh&m)){cy.release=false;P3DS_CatchMask=0;}}}

/* 2026-11-19 FIGHT-menu type hint, prettier (Nathan: old, primitive, blocky): a glossy pill with an arrow and sharp native text in the gap under each
   move button. Same data and chart as before (hintMult); only the drawing changed. Falls back to the old 3x5 badge when the sharp text layer is off. */
static SBtxList*hintTx;static unsigned hintLogN;
static bool hintSharpBegin(u32*img){if(!btxMode())return false;if(!hintTx)hintTx=(SBtxList*)calloc(1,sizeof(SBtxList));if(!hintTx)return false;btxBegin(*gBtx,img);return true;}
static bool hintSharpDraw(u32*img,int x0,int y0,int m){
 const char*t=m==0?"No effect":m==1?"Resist x1/4":m==2?"Resist x1/2":m==8?"Super x2":"Super x4";
 const u32 c=m==0?SUI_RGB(110,116,132):m<4?SUI_RGB(208,86,62):m==8?SUI_RGB(48,168,84):SUI_RGB(30,192,70);
 const int tw=btxDsW(t,false),pw=tw+22,px=x0+120-4-pw,py=y0-2,ph=10;
 cc::pillBase(img,px,py,px+pw,py+ph,c);
 if(m==0)cc::cross(img,px+5,py+2,SUI_RGB(255,255,255));else if(m<4)cc::triDn(img,px+5,py+3,SUI_RGB(255,255,255));else cc::triUp(img,px+5,py+3,SUI_RGB(255,255,255));
 btxText(*gBtx,img,px+16,py-1,t,SUI_RGB(255,255,255),cc::mixc(c,SUI_RGB(0,0,0),150),false);
 if(hintLogN<3){hintLogN++;PSPNativeMemLog("[HINT] sharp pill \"%s\" at DS %d,%d (%d wide)",t,px,py,pw);}
 return true;}
static void hintSharpEnd(bool began){if(!began||!hintTx)return;btxEnd(*gBtx,*hintTx);btxShow(hintTx);}
/* hunt_stats (Battle tab "HUNT STATS (ZR)"): ZR in the overworld opens the HUNT STATS screen. Reworked 2026-11-19 (Nathan: "needs the same
   overhaul as the y button in battle ... actually make it useful not kill the game"). It used to be a modal loop with its own presents: the
   whole game (field, music timing, the panel) stood still until it closed. Now it is the same OVERLAY as the battle Y screen (cy above,
   cy.field = true): the bottom picture is swapped in Present, the field keeps running, and the game only loses the buttons and the touch
   screen (P3DS_CatchMask) while it is up and until every key is released after it closes. Drawing: ss3ds_huntui.h. Data: the hunt.txt
   counters (hunt.c), read only. Pages HUNT / TOTALS / TOP / RECENT / SHINY; Up/Down on HUNT = other species. Closes itself if the field
   ends (battle, warp fade does not count: P3DS_InField stays), a menu opens or the console takes the bottom screen.
   Logs: [HSTAT] opened / page / species / closed (with the field frames that ran underneath). */
#include "ss3ds_huntui.h"
static const long kHsNone=-1000000000L;
static long hsKeyEnc(const P3DS_HuntRec&r){return r.enc>0?(long)r.enc:kHsNone;}
static long hsKeyStart(const P3DS_HuntRec&r){return r.start>0?(long)r.start:kHsNone;}
static long hsKeyBest(const P3DS_HuntRec&r){return r.shinies>0?-(long)r.lastShinyAt:kHsNone;}
/* the n species with the largest key (ties: lower species id first); returns how many were found */
static int hsTop(int n,int*out,long(*key)(const P3DS_HuntRec&)){
 static bool taken[P3DS_HUNT_SPECIES];memset(taken,0,sizeof taken);int cnt=0;
 for(int s=0;s<n;s++){long best=kHsNone;int bk=0;
  for(int k=1;k<P3DS_HUNT_SPECIES;k++){if(taken[k])continue;const long v=key(*P3DS_HuntGet(k));if(v==kHsNone)continue;if(!bk||v>best){best=v;bk=k;}}
  if(!bk)break;taken[bk]=true;out[cnt++]=bk;}
 return cnt;
}
static void hsName(char*o,int n,int sp){snprintf(o,n,"%s",sp>0&&sp<P3DS_HUNT_SPECIES?kP3dsSpec[sp]:"?");for(char*q=o;*q;q++)if(*q>='a'&&*q<='z')*q-=32;}
static bool hsHas(const P3DS_HuntRec&r){return r.enc||r.resets||r.shinies;}
static int hsSp; /* species shown on the HUNT page (0 = none recorded) */
static bool hsAll; /* Y on the HUNT page: Up/Down walks every species (to pin one not met yet), not only the recorded ones */
static unsigned hsArmed; /* RESET was tapped once: a second tap before this frame count clears the species (cy.frames) */
/* cheap fingerprint of everything the pages show; a change redraws the picture */
static u32 hsKey(int pg){u32 h=2166136261u^(u32)pg^((u32)hsSp<<8);auto mx=[&](int v){h^=(u32)v;h*=16777619u;};
 for(int k=1;k<P3DS_HUNT_SPECIES;k++){const P3DS_HuntRec&r=*P3DS_HuntGet(k);if(!hsHas(r))continue;mx(k);mx(r.enc);mx(r.resets);mx(r.shinies);mx(r.streak);}
 const P3DS_HuntRec&v=*P3DS_HuntGet(hsSp);mx(v.secs/60);mx(P3DS_HuntTarget());mx(P3DS_HuntPinned());mx(hsAll);mx(hsArmed>cy.frames);mx(P3DS_HuntRateOdds());mx(P3DS_Cheat(2)!=0);return h;}
static void hsGather(hs::Data&D){
 memset(&D,0,sizeof D);const int odds=P3DS_HuntRateOdds(),tg=P3DS_HuntTarget();
 hs::Tot&t=D.t;int luckSp=0,longSp=0,n=0,idx=0;
 for(int k=1;k<P3DS_HUNT_SPECIES;k++){const P3DS_HuntRec&r=*P3DS_HuntGet(k);if(!hsHas(r))continue;n++;if(k==hsSp)idx=n;
  t.met++;t.enc+=r.enc;t.resets+=r.resets;t.shinies+=r.shinies;t.secs+=r.secs;
  if(r.shinies&&(!luckSp||r.lastShinyAt<t.luckAt)){luckSp=k;t.luckAt=r.lastShinyAt;}
  if(r.streak>t.longN){t.longN=r.streak;longSp=k;}
  if(r.start&&(!t.start||r.start<t.start))t.start=r.start;}
 if(luckSp)hsName(t.luck,sizeof t.luck,luckSp);if(longSp)hsName(t.lng,sizeof t.lng,longSp);
 if(tg>0&&hsHas(*P3DS_HuntGet(tg))){hsName(t.cur,sizeof t.cur,tg);t.curStreak=P3DS_HuntGet(tg)->streak;}
 hs::View&v=D.v;
 const int pin=P3DS_HuntPinned();t.pinned=pin>0&&pin==tg;if(t.pinned&&!t.cur[0]){hsName(t.cur,sizeof t.cur,tg);t.curStreak=0;}
 v.all=hsAll;
 if(hsSp>0&&(hsAll||hsSp==pin||hsHas(*P3DS_HuntGet(hsSp)))){const P3DS_HuntRec&r=*P3DS_HuntGet(hsSp);hsName(v.name,sizeof v.name,hsSp);v.sp=hsSp;
  v.pinned=hsSp==pin;v.met=r.enc>0;v.armed=hsArmed>cy.frames;
  v.enc=r.enc;v.streak=r.streak;v.resets=r.resets;v.shinies=r.shinies;v.secs=r.secs;v.start=r.start;v.lastShinyAt=r.lastShinyAt;
  v.odds=odds;v.enc50=P3DS_HuntEncFor(0.5,odds);v.enc90=P3DS_HuntEncFor(0.9,odds);v.chance=P3DS_HuntChanceBy(r.streak,odds);v.cheat=P3DS_Cheat(2)!=0;
  v.target=hsSp==tg;v.idx=idx;v.nIdx=n;}
 int ids[6];
 D.top.n=hsTop(6,ids,hsKeyEnc);for(int i=0;i<D.top.n;i++){const P3DS_HuntRec&r=*P3DS_HuntGet(ids[i]);hsName(D.top.name[i],24,ids[i]);D.top.a[i]=r.enc;D.top.b[i]=r.shinies;}
 D.recent.n=hsTop(6,ids,hsKeyStart);for(int i=0;i<D.recent.n;i++){const P3DS_HuntRec&r=*P3DS_HuntGet(ids[i]);hsName(D.recent.name[i],24,ids[i]);D.recent.a[i]=r.start;D.recent.b[i]=r.enc;D.recent.c[i]=r.shinies;}
 D.shiny.n=hsTop(6,ids,hsKeyBest);for(int i=0;i<D.shiny.n;i++){const P3DS_HuntRec&r=*P3DS_HuntGet(ids[i]);hsName(D.shiny.name[i],24,ids[i]);D.shiny.a[i]=r.lastShinyAt;D.shiny.b[i]=r.shinies;}
}
static void hsOpen(){if(cy.open)return;cy.img=(u32*)malloc(256*192*4);if(!cy.img){PSPNativeMemLog("[HSTAT] no memory for the screen (%u bytes)",256u*192u*4u);return;}
 const int tg=P3DS_HuntTarget(),pin=P3DS_HuntPinned();hsSp=pin>0?pin:tg>0&&hsHas(*P3DS_HuntGet(tg))?tg:P3DS_HuntNext(0,1);hsAll=false;hsArmed=0;
 cy.open=true;cy.field=true;cy.release=false;cy.pg=hs::H_HUNT;cy.n=hs::H_N;cy.key=0;cy.frames=0;cy.t0=svcGetSystemTick();cy.tPrev=hidKeysHeld()&KEY_TOUCH;cy.zrPrev=true;P3DS_CatchMask=1;
 PSPNativeMemLog("[HSTAT] stats screen opened as an overlay, the field keeps running (species #%d, target #%d, pinned #%d, odds 1/%d)",hsSp,tg,pin,P3DS_HuntRateOdds());}
static const u32*hsFrame(bool txOk){
 if(!P3DS_InField){cyClose("left the field");return nullptr;}
 if(P3DS_MenuOpen||P3DS_ConsoleOnBottom()||P3DS_PanelPartyOn||P3DS_PanelPartyWant){cyClose("a menu took the screen");return nullptr;}
 cy.frames++;
 /* the fingerprint walks every species: every 15th frame is plenty (a new encounter cannot happen while the field is masked anyway) */
 if(cy.tx&&cy.key&&(cy.frames%15)!=1&&((cy.key&1u)==(txOk?1u:0u))){if(txOk)btxShow(cy.tx);else P3DS_TxOn=0;P3DS_OvN=0;return cy.img;}
 const u32 key=(hsKey(cy.pg)&~1u)|(txOk?1u:0u);
 if(key!=cy.key||!cy.tx){cy.key=key;if(!cy.tx)cy.tx=(SBtxList*)calloc(1,sizeof(SBtxList));
  static hs::Data D;const u64 tg=svcGetSystemTick();hsGather(D);
  const bool rec=txOk&&cy.tx;if(rec)btxBegin(*gBtx,cy.img);const SUiText&T=rec?kSuiText:SUiText{suiDraw,gw,suiDrawS,gwS};
  hs::page(cy.img,T,D,cy.pg);
  if(rec){btxEnd(*gBtx,*cy.tx);btxLog("hunt stats screen",cy.tx);}else{if(gBtx)gBtx->on=false;}
  static unsigned nLog;if(nLog++<12)PSPNativeMemLog("[HSTAT] drew %s page in %u us (%d species, %d encounters, %d shinies)",hs::kTab[cy.pg],(unsigned)((svcGetSystemTick()-tg)/CPU_TICKS_PER_USEC),D.t.met,D.t.enc,D.t.shinies);}
 if(txOk&&cy.tx)btxShow(cy.tx);else P3DS_TxOn=0;
 P3DS_OvN=0;return cy.img;}
/* HUNT page actions: Up/Down species, X pin/unpin, Y every species on/off, RESET (touch, twice) clears the shown species' counters */
static void hsStep(int dir,const char*how){int nx;
 if(hsAll){nx=hsSp+dir;if(nx<1)nx=P3DS_HUNT_SPECIES-1;if(nx>=P3DS_HUNT_SPECIES)nx=1;}else nx=P3DS_HuntNext(hsSp,dir);
 if(nx&&nx!=hsSp){hsSp=nx;hsArmed=0;cy.key=0;PSPNativeMemLog("[HSTAT] species #%d (%s%s)",hsSp,how,hsAll?", all species":"");}}
static void hsPinToggle(){if(hsSp<=0)return;const bool un=P3DS_HuntPinned()==hsSp;P3DS_HuntPin(un?0:hsSp);hsArmed=0;cy.key=0;
 PSPNativeMemLog("[HSTAT] %s species #%d",un?"unpinned":"pinned the hunt to",hsSp);P3DS_Toast(un?"HUNT UNPINNED":"HUNT SET");}
static void hsAllToggle(){hsAll=!hsAll;hsArmed=0;if(!hsAll&&hsSp>0&&!hsHas(*P3DS_HuntGet(hsSp))&&hsSp!=P3DS_HuntPinned()){const int t=P3DS_HuntTarget();hsSp=t>0&&hsHas(*P3DS_HuntGet(t))?t:P3DS_HuntNext(0,1);}
 if(hsAll&&hsSp<=0)hsSp=1;cy.key=0;PSPNativeMemLog("[HSTAT] all species %s",hsAll?"on":"off");}
static void hsResetTap(){const P3DS_HuntRec&r=*P3DS_HuntGet(hsSp);if(hsSp<=0||!(r.enc||r.resets||r.shinies))return;
 if(hsArmed>cy.frames){P3DS_HuntClear(hsSp);hsArmed=0;cy.key=0;P3DS_Toast("HUNT COUNTER RESET");PSPNativeMemLog("[HSTAT] species #%d counters reset",hsSp);
  if(!hsAll&&hsSp!=P3DS_HuntPinned()){const int nx=P3DS_HuntNext(hsSp,1);hsSp=nx;}}
 else{hsArmed=cy.frames+180;cy.key=0;PSPNativeMemLog("[HSTAT] reset armed for species #%d (tap again within 3 s)",hsSp);}}
static void hsInput(u32 kd,u32 kh){
 const bool zrEdge=(kh&KEY_ZR)&&!cy.zrPrev;cy.zrPrev=(kh&KEY_ZR)!=0;
 if((kd&(KEY_B|KEY_A|KEY_START))||zrEdge){cyClose("button");return;}
 if(hsArmed&&hsArmed<=cy.frames){hsArmed=0;cy.key=0;}
 int np=cy.pg;if(kd&(KEY_RIGHT|KEY_R|KEY_DRIGHT))np=(cy.pg+1)%cy.n;if(kd&(KEY_LEFT|KEY_L|KEY_DLEFT))np=(cy.pg+cy.n-1)%cy.n;
 if(kd&(KEY_X|KEY_Y)){if(cy.pg!=hs::H_HUNT){np=hs::H_HUNT;}else if(kd&KEY_X)hsPinToggle();else hsAllToggle();}
 if(cy.pg==hs::H_HUNT&&(kd&(KEY_UP|KEY_DUP|KEY_DOWN|KEY_DDOWN)))hsStep((kd&(KEY_UP|KEY_DUP))?-1:1,"D-pad");
 const bool tNow=(kh&KEY_TOUCH)!=0;
 if(tNow){touchPosition tp;hidTouchRead(&tp);int x,y;if(touchToDS(tp.px,tp.py,&x,&y)){if(!cy.tPrev){cy.tx0=x;cy.ty0=y;}cy.tx1=x;cy.ty1=y;}}
 else if(cy.tPrev){const int dx=cy.tx1-cy.tx0,dy=cy.ty1-cy.ty0;
  if((dx>=36||dx<=-36)&&(dy<40&&dy>-40))np=dx<0?(cy.pg+1)%cy.n:(cy.pg+cy.n-1)%cy.n;
  else if(cy.pg==hs::H_HUNT&&(dy>=30||dy<=-30)&&dx<30&&dx>-30)hsStep(dy>0?-1:1,"swipe");
  else if(dx<10&&dx>-10&&dy<10&&dy>-10){const int h=cc::hit(cy.n,cy.tx0,cy.ty0);if(h==-2){cyClose("X tapped");cy.tPrev=false;return;}if(h>=0&&h<cy.n)np=h;
   else if(cy.pg==hs::H_HUNT){const int bt=hs::btnHit(cy.tx0,cy.ty0);if(bt==0)hsPinToggle();else if(bt==1)hsAllToggle();else if(bt==2)hsResetTap();}}}
 cy.tPrev=tNow;
 if(np!=cy.pg){cy.pg=np;cy.key=0;PSPNativeMemLog("[HSTAT] page %d/%d: %s",np+1,cy.n,hs::kTab[np]);}}
extern "C" int PSPNativeRenderBegin(){if(!initialized||openFrame)return -1;openFrame=true;G3SIM_DrawCleanUp();P3DS_G3BeginFrame();return 0;}
/* Precompute pixel coverage for the exact box filter. Geometry rarely changes,
 * so division is confined to setup rather than the scanline loops. */
struct PanelFilterSpan {
    int first, last, firstWeight, lastWeight;
};

static void panelFilterSpans(PanelFilterSpan *spans, int source, int dest) {
    for (int i = 0; i < dest; i++) {
        const int begin = i * source, end = begin + source;
        const int first = begin / dest, last = (end - 1) / dest;
        spans[i] = {first, last, first == last ? source : (first + 1) * dest - begin,
                    end - last * dest};
    }
}

static void ppFinalResample(const u32 *snap, int sourceHeight, u32 *out, int fw, int fh) {
    static PanelFilterSpan cols[256], rows[256];
    static int cachedWidth, cachedHeight, cachedSourceHeight;
    static u16 tmp[192 * 256 * 3];
    if (fw != cachedWidth) {
        panelFilterSpans(cols, 256, fw);
        cachedWidth = fw;
    }
    if (fh != cachedHeight || sourceHeight != cachedSourceHeight) {
        panelFilterSpans(rows, sourceHeight, fh);
        cachedSourceHeight = sourceHeight;
        cachedHeight = fh;
    }
    for (int y = 0; y < sourceHeight; y++) {
        const u32 *src = snap + y * 256;
        u16 *dst = tmp + y * fw * 3;
        for (int x = 0; x < fw; x++) {
            const auto &span = cols[x];
            unsigned r = 0, g = 0, b = 0;
            for (int i = span.first; i <= span.last; i++) {
                const int weight = i == span.first ? span.firstWeight :
                                   i == span.last ? span.lastWeight : fw;
                const u32 c = src[i];
                r += (c & 255) * weight;
                g += ((c >> 8) & 255) * weight;
                b += ((c >> 16) & 255) * weight;
            }
            dst[x * 3] = r;
            dst[x * 3 + 1] = g;
            dst[x * 3 + 2] = b;
        }
    }
    for (int y = 0; y < fh; y++) {
        const auto &span = rows[y];
        for (int x = 0; x < fw; x++) {
            unsigned r = 0, g = 0, b = 0;
            for (int i = span.first; i <= span.last; i++) {
                const int weight = i == span.first ? span.firstWeight :
                                   i == span.last ? span.lastWeight : fh;
                const u16 *c = tmp + (i * fw + x) * 3;
                r += c[0] * weight;
                g += c[1] * weight;
                b += c[2] * weight;
            }
            const unsigned area = 256 * sourceHeight;
            r = (r + area / 2) / area;
            g = (g + area / 2) / area;
            b = (b + area / 2) / area;
            out[y * 256 + x] = (r << 24) | (g << 16) | (b << 8) | 0xFFu;
        }
    }
}
static unsigned ppTapF,ppTapQ;static int ppLatPend; /* run 141: tab-tap frame / latency log pending; ppTapQ (2026-11-18): frame of a tab tap that waited for a swap or a busy menu, so the latency line counts from the finger */

extern "C" int P3DS_GcComposeB(const u8*,const u8*,const u8*,const u16*,const u16*,const u16*,u32*,unsigned*);
static void composeGpuB(int mode){
 bGpuFrame=bGpuVerify=false;if(!mode||!engineB.Enabled||bHideB||P3DS_MenuOpen)return;
 if(mode==2&&!bGpuCheck)bGpuCheck=(u32*)memalign(64,256*256*4);
 u32*out=mode==2?bGpuCheck:display[1];if(!out)return;
 /* Preserve the held field before the shared compositor target is reused. */
 holdSync();P3DS_GcFinish();unsigned why=0;
 const u64 t=svcGetSystemTick();int ok=P3DS_GcComposeB(PSPNative_GfxRegisters+0x1000,GPU::VRAMFlat_BBG,GPU::VRAMFlat_BOBJ,(const u16*)(GPU::Palette+1024),(const u16*)(GPU::Palette+1536),(const u16*)(GPU::OAM+1024),out,&why);
 static unsigned frames,good,reasons[16];static u64 ticks;ticks+=svcGetSystemTick()-t;frames++;good+=ok!=0;
 if(why)for(unsigned k=0;k<16;k++)reasons[k]+=(why>>k)&1;
 if(ok){bGpuFrame=mode==1;bGpuVerify=mode==2;}
 if(frames%300==0){PSPNativeMemLog("[GCB] frames=%u composed=%u avg=%u us/frame rejects dm=%u forceblank=%u window=%u affineBG=%u extpal=%u blend=%u affineOBJ=%u objmode=%u fade=%u",frames,good,(unsigned)(ticks/CPU_TICKS_PER_USEC/frames),reasons[0],reasons[1],reasons[2],reasons[4],reasons[6],reasons[7],reasons[10],reasons[11],reasons[15]);}
}
static void verifyGpuB(){if(!bGpuVerify||!bGpuCheck)return;
 static unsigned count,badFrames,badPixels,maxDiff;unsigned bad=0,mx=0;
 for(unsigned i=0;i<256*192;i++){u32 a=display[1][i],b=bGpuCheck[i];if((a^b)&0xFFFFFFu){bad++;for(unsigned shift=0;shift<24;shift+=8){int d=(int)((a>>shift)&255)-(int)((b>>shift)&255);if(d<0)d=-d;if((unsigned)d>mx)mx=d;}}}
 if(titleWideValid&&P3DS_GcTitleWideValid)for(unsigned i=0;i<64*192;i++){u32 a=titleStrips[i],b=P3DS_GcTitleStrips[i];if((a^b)&0xFFFFFF00u){bad++;for(unsigned sh=8;sh<=24;sh+=8){int d=(int)((a>>sh)&255)-(int)((b>>sh)&255);if(d<0)d=-d;if((unsigned)d>mx)mx=d;}}}
 count++;badFrames+=bad!=0;badPixels+=bad;if(mx>maxDiff)maxDiff=mx;
 if(count%120==0)PSPNativeMemLog("[GCBVERIFY] checked=%u differing_frames=%u differing_pixels=%u max_channel=%u",count,badFrames,badPixels,maxDiff);
}
extern "C" volatile int P3DS_PanelBagOpening;
static int sFieldHoldOk;
static int PresentFrame(bool);
/* sharp game text: the text tiles that sprites show were taken out for this present's picture only (P3DS_GtSync); the printed
   tiles go back before the game runs again, on every way out of the present */
static int Present(bool w){const int r=PresentFrame(w);P3DS_GtObjRestore();return r;}
static int PresentFrame(bool){
 if(!openFrame)return -1;
 const u64 presentStart=svcGetSystemTick();
 allocBufs();
 const u64 textStart=svcGetSystemTick();
 P3DS_GtSync(gtOkPrev);P3DS_GtWidePlan=nullptr; /* sharp game text: text whose screen could not show it last present goes back into VRAM before this compose */
 const u64 prefetchStart=svcGetSystemTick();
 P3DS_RomCachePumpC(); /* run 51 zone_prefetch: adopt staged blocks on the game thread */
 const u64 prefetchEnd=svcGetSystemTick();
 if(earlyCfg<0){earlyCfg=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"early_b=%d",&x)==1)earlyCfg=x!=0;fclose(f);}PSPNativeMemLog("[2D] early_b=%d",earlyCfg);}
 P3DS_TitleWideRequested=P3DS_InTitle&&titleCleanCfg()==2;P3DS_GcTitleWideValid=0;
 // Use the DS scanline compositor for the Pokewalker screens to avoid GPU tile seams.
 const bool pokewalker = P3DS_InPokewalker != 0;
 const int gpuBModeFrame=(!P3DS_InField&&!pokewalker)?gpuBCfg():0;bGpuFrame=bGpuVerify=false;titleWideValid=false;
 bool earlyB=false;
 if(!gpuBModeFrame&&earlyCfg&&engineBAsync()&&stealCfg!=1&&c1Mode!=1){unsigned pw=reg16(0x304);
  mapMemory();engineB.Enabled=(pw&512)!=0;bindRegisters(engineB,0x1000);FrameStart(engineB);bPower=pw;bRightSrc=nullptr;
  bEarly=true;earlyB=true;earlyN++;bPost();}
 const u64 flushStart=svcGetSystemTick();
 G3SIM_FlushDeferred();openFrame=false;
 const u64 flushEnd=svcGetSystemTick();
 unsigned power=reg16(0x304),a=reg32(0);
 u64 t0=svcGetSystemTick();unsigned g3us=P3DS_G3LastUs();
 {u32 kd=hidKeysUp(),kh=hidKeysHeld();if((kd&KEY_START)&&(kh&KEY_L)&&(kh&KEY_R)&&!P3DS_ResetSeen){fpsOn=!fpsOn;/* run 53: on release, not after the soft-reset combo */P3DS_Toast(fpsOn?"FPS ON":"FPS OFF");}}
 if((reg32(0x64)&0x80000000u)||((a>>16)&3)==3){static int once;if(!once++)printf("[NATIVE-RENDER] display capture/FIFO unsupported (ignored)\n");}
 /* run 42 [STEREO] diag (stereo only): display capture is NOT implemented. Log (max 1 line / 15 frames) whenever
    capture is enabled, the display mode is not "normal", or engine A shows large OBJs (>=32x32, e.g. a Pokemon drawn as
    2D sprites): a 2D copy of something the 3D engine also draws gets zero disparity -> doubled image in 3D. */
 if(p3dsSlider()>0.0f){static unsigned lastF;unsigned cap=reg32(0x64),dm=(a>>16)&3;unsigned big=0;
  if(a&(1u<<12)){const u16*o=(const u16*)s_HW_OAM;for(int i=0;i<128;i++){u16 a0=o[i*4],a1=o[i*4+1];if((a0&0x300)==0x200)continue;unsigned shape=a0>>14,size=a1>>14;
    static const u8 W[3][4]={{8,16,32,64},{16,32,32,64},{8,8,16,32}},H[3][4]={{8,16,32,64},{8,8,16,32},{16,32,32,64}};if(shape<3&&W[shape][size]>=32&&H[shape][size]>=32)big++;}}
  if(((cap&0x80000000u)||dm!=1||big)&&frames-lastF>=15){lastF=frames;PSPNativeMemLog("[STEREO] f=%u dispcntA %08x mode %u capcnt %08x bigOBJ %u bg0cnt %04x 3Dpolys %u",frames,a,dm,cap,big,reg16(8),P3DS_G3LastTris());}}
 const bool excl=compExcl();s32 oldPrio=0x30;if(excl){svcGetThreadPriority(&oldPrio,CUR_THREAD_HANDLE);svcSetThreadPriority(CUR_THREAD_HANDLE,0x18);}
 if(P3DS_GpuComposeCfg()==1){gcAnalyze(0,P3DS_InField);gcAnalyze(1,P3DS_InField);}
 u32 hash0=P3DS_FpsTweaks()>=5?0:stateHash();  /* level 5: the mid-composite change check is a diagnostic only */
 if(!earlyB){mapMemory();engineB.Enabled=(power&512)!=0;bindRegisters(engineB,0x1000);} /* early_b: already mapped; the worker may be reading it */
 engineA.Enabled=(power&2)!=0;bindRegisters(engineA,0);hbSnap(); P3DS_St[1]=svcGetSystemTick();
 /* run 84-86 wide_layouts (widecomp.c): a screen with a wide layout (party menu, bag, boxes, ...) is drawn across the wide view by
    widecomp's own compositor from the same VRAM / registers / OAM, after the normal melonDS pass (which stays as the fallback
    picture when the screen uses something widecomp does not do). No GPU compose on those frames. */
 WcPlan&wcPlan=wcPlanG;bool wlNow=false;
 /* run 98 wide_dirty=1 (default): widecomp redraws only the lines whose inputs changed (see widecomp.c); 2 = verify against a full
    redraw and count mismatches; 0 = full redraw every changed frame (run 97). */
 {static int wd=-1;if(wd<0){wd=P3DS_CfgInt("wide_dirty",1);PSPNativeMemLog("[WIDE] wide_dirty=%d",wd);}WC_SetDirty(wd);{static int rf=-1;if(rf<0){rf=P3DS_CfgInt("wide_dirty_regs",1);PSPNativeMemLog("[WIDE] wide_dirty_regs=%d",rf);}WC_SetDirtyRegFilter(rf);}{static int a3=-1;if(a3<0){a3=P3DS_CfgInt("wide_3d_alpha",1);PSPNativeMemLog("[WIDE] wide_3d_alpha=%d",a3);}WC_Set3dAlpha(a3);}{static int ww=-1;if(ww<0){ww=P3DS_CfgInt("wide_windows",1)!=0;PSPNativeMemLog("[WIDE] wide_windows=%d",ww);}WC_SetWindows(ww);}
  static unsigned dn;if(++dn%600==0&&wd){unsigned fr,rw,fu,bad;WC_DirtyStats(&fr,&rw,&fu,&bad);if(fr)PSPNativeMemLog("[WIDE] wide_dirty: %u composes, %u lines/compose avg, %u full, %u verify mismatches (%u races; last bad line %d dirty %d)",fr,rw/fr,fu,bad,dStatRace,dBadRow,dBadMask);dStatRace=0;}}
 if(P3DS_WideLayoutsOn()&&!P3DS_DexNoWide&&P3DS_ScaleMode()==1&&(power&0x8000)&&engineA.Enabled&&!(G3G_Wide&&G3G_StripValid>=1&&P3DS_InField)){
  static short wlParts[WC_MAX_PARTS*8];const int n=P3DS_WideGetParts(0,wlParts);
  if(n>=0&&WC_Plan(&wcPlan,PSPNative_GfxRegisters,wlParts,n)){wlNow=true;
   if(wcPlan.changed){wcPlan.changed=0;static unsigned nPl;if(++nPl<=20||nPl%600==0)PSPNativeMemLog("[WIDE] layout #%u: %d parts, %d sprite parts, 3D shift %d",nPl,wcPlan.n,wcPlan.objParts,wcPlan.dx3d);}}} P3DS_St[2]=svcGetSystemTick();
 /* run 76 display capture through the renderer's own DoCapture (melonDS): source A = 2D+3D composite or 3D, source B =
    VRAM, blend A/B. Motion blur at Spear Pillar (Dialga/Palkia) shows VRAM C fed by capture; ignored it was black. */
 {const u32 cc=reg32(0x64);const int cm=P3DS_CfgInt("display_capture",1);engineA.CaptureCnt=cm==1?cc:0;engineA.CaptureLatch=cm==1&&(cc&0x80000000u)&&engineA.Enabled;}
 /* per-vertex stereo (g3soft rendered both eyes) if it was enabled at frame start; else flat-layer fallback */
 {static unsigned diagN;if(++diagN%120==0&&P3DS_LogLevel()>=2&&!G3G_DirectPending()){/* run 113: the 49k-pixel scan only with log detail FULL */unsigned n=0;for(unsigned i=0;i<256*192;i++)if(GPU3D::NativeFrame[i]>>24)n++;
   printf("[3DLAYER] dispcntA %08x powcnt %04x 3D px alpha>0 %u bg0cnt %04x blend %04x/%04x\n",reg32(0),power,n,reg16(8),reg16(0x50),reg16(0x52));}}
 const bool async = engineBAsync();
 // Engine A's 2D result is discarded while the Bag displays B below the held field.
 const bool hidePanelTop = false; /* SoulSilver: Pokegear panel app held over the field; Platinum has no such panel */
 u32*rightEye=P3DS_G3RightEye();
 int sd=rightEye?0:stereoShift(P3DS_ScaleMode());bool stereo=(rightEye||sd)&&(power&0x8000)&&engineA.Enabled;
 if(stereo&&!rightEye){G3G_DirectResolveWhy(1);shift3D(-sd);}  /* gpu_direct3d: shift3D edits NativeFrame on the CPU */
 FrameStart(engineA);if(!earlyB)FrameStart(engineB);renderer.SetFramebuffer(raw[0],raw[1]);
 composeGpuB(gpuBModeFrame);
 static int par3d=-1;if(par3d<0){par3d=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");int x;char l[64];if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"stereo2d_par=%d",&x)==1)par3d=x!=0;fclose(f);}PSPNativeMemLog("[2D] stereo right eye on core 2: %d",par3d);}
 const int gcMode0=P3DS_GpuComposeCfg();
 /* run 111 wide_gpu (Debug "Wide on GPU", default 1 = with the 3D slider up; 2 = also slider down; 0 = off): a wide layout frame
    (battles, menus) is composed on the GPU from widecomp's plan for both eyes (gpucomp.c P3DS_GcComposeWide): BGs and
    sprites at their wide positions, strips, 3D layer from the gpu_direct3d texture. The CPU wide compose and engine A
    are skipped. Frames the GPU path can't take (windows, affine/semi sprites, ...) use the CPU wide path as before. */
 /* run 114 wide_gpu_precheck (Debug "Wide GPU blend check", default ON): a wide frame whose alpha blend the GPU compositor
    would reject (a first-target BG/sprite over a backdrop that is not a second target, e.g. the opening cutscene) no longer
    picks wide_gpu. Before, it skipped wide_early, the GPU then declined it, and the right eye fell to the serial CPU path
    (path S, ~54 ms frames for the first ~6 s after launch with the slider up). Now it takes the CPU wide path as in Run 109. */
 static int wgPre=-1;if(wgPre<0){wgPre=P3DS_CfgInt("wide_gpu_precheck",1)!=0;PSPNativeMemLog("[WIDE] wide_gpu_precheck=%d",wgPre);}
 static int wgCfg=-1;if(wgCfg<0){wgCfg=P3DS_CfgInt("wide_gpu",1);if(wgCfg<0||wgCfg>2)wgCfg=1;PSPNativeMemLog("[WIDE] wide_gpu=%d (re-ported onto the SoulSilver compositor 2026-11-25)",wgCfg);}
 const bool wgNow=wgCfg&&wlNow&&(stereo?rightEye!=nullptr:wgCfg==2)&&gcMode0==2&&engineA.Enabled&&!engineA.CaptureLatch&&WC_Check(&wcPlan,PSPNative_GfxRegisters,1)==0&&gcEligible()&&(!wgPre||P3DS_GcBlendPredictOk((const u8*)PSPNative_GfxRegisters));
 /* run 113: the run 110 stereo_gpu_battle 4:3 mode (default off since run 111, superseded by wide_gpu) is removed; the key is ignored. */
 if((wlNow&&!wgNow)||engineA.CaptureLatch)G3G_DirectResolveWhy(2);  /* gpu_direct3d: widecomp / display capture read NativeFrame */
 const bool gcSt=!hbOn&&!wlNow&&gcMode0==2&&stereo&&rightEye&&(P3DS_InField||gcStNfCfg())&&engineA.Enabled&&gcStereoCfg()&&gcEligible();
 if(stereo&&!gcSt&&!wgNow)G3G_DirectResolveWhy(2);  /* gpu_direct3d: the CPU right-eye paths read the 3D layer */
 bool rightPar=async&&par3d&&stereo&&rightEye&&!gcSt&&!wgNow;
 /* run 96 wide_early in stereo: with a wide layout both eyes end up as widecomp's single picture (the right eye is a copy),
    so the right-eye engine A pass on core 2 was thrown away too. wide_early now also covers stereo (per-vertex 3D eyes). */
 const bool wlEarlyPre=wlNow&&!wgNow&&async&&!engineA.CaptureLatch&&!(reg32(0x64)&0x80000000u)&&(!stereo||rightEye)&&P3DS_CfgInt("wide_early",1)!=0
  /* run 99 wide_early_check=1: only when widecomp will accept this frame. Before, a declined frame (windows: the Poke Ball
     send-out, ...) in stereo fell to the serial right-eye redraw on the main thread (48-57 ms on hardware). */
  &&(!P3DS_CfgInt("wide_early_check",1)||WC_Check(&wcPlan,PSPNative_GfxRegisters,GPU3D::NativeFrame!=nullptr)==0);
 if(wlEarlyPre&&stereo)rightPar=false;
 /* run 108 stereo_flat_fallback (default 0): a stereo frame the wide compositor declines (battle intro OBJ window, ...)
    shows the left eye to both eyes for that frame instead of rendering engine A a second time (45 ms frames on hardware). */
 static int sffCfg=-1;if(sffCfg<0){sffCfg=P3DS_CfgInt("stereo_flat_fallback",0)!=0;PSPNativeMemLog("[WIDE] stereo_flat_fallback=%d",sffCfg);}
 const bool flatEye=sffCfg&&stereo&&!wlEarlyPre&&!gcSt&&!wgNow;if(flatEye)rightPar=false;
 if(rightPar){memcpy((void*)&engineA2,(const void*)&engineA,sizeof engineA);engineA2.CaptureCnt=0;engineA2.CaptureLatch=false;}
 /* run 55 stereo_2d=1 (default): outside the overworld (battles, menus, title) the right eye's 2D background layers
    get depth by BG priority: priority 3 (back) = the far-3D disparity (2 x shift), priority 2 = 1.5 x, priorities 0-1
    (text boxes, menus, HUD) and sprites stay at the screen plane. Text-mode BGs only (affine/bitmap untouched).
    Overworld unchanged (its BG overlays sit in front of the 3D world). */
 bdMask=0;bdSplit=kBattleSplitY;
 if(rightPar&&!P3DS_InField){const int cfg=stereo2dCfg();
  if(cfg){const int sd=stereoShiftF(P3DS_ScaleMode());const unsigned bgm=engineA2.DispCnt&7;const int bdE=P3DS_BattleSideExtra(sd);
   for(int i=1;i<4;i++){if(!(engineA2.DispCnt&(0x100<<i)))continue;const bool text=i<2||bgm==0||(i==2&&bgm==1);if(!text)continue;
    const int pr=engineA2.BGCnt[i]&3,d=cfg==1?(pr==3?2*sd:pr==2?(3*sd)/2:0):cfg==4?(pr>=2?(sd+1)/2:0):(pr==3?(sd+1)/2:pr==2?(sd+2)/4:0);
    if(d&&bdE&&pr>=2){bdG[i]=engineA2.BGXPos[i];bdMine[i]=0xFFFF;bdD[i]=(u16)d;bdSd=sd;bdMask|=1<<i;}else if(d)engineA2.BGXPos[i]=(u16)((engineA2.BGXPos[i]-d)&0x1FF);}}}
 bRightSrc=rightPar?rightEye:nullptr;
 bPower=power;if(!async)P3DS_BottomReused=0;
 const bool onC1=rightPar&&!earlyB&&c1Ready();bSkipB=onC1||earlyB;
 if(onC1)LightEvent_Signal(&c1Start);
 if(stealCfg<0){stealCfg=0;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"steal_b=%d",&x)==1)stealCfg=x!=0;fclose(f);}PSPNativeMemLog("[2D] steal_b=%d",stealCfg);}
 bClaim=0;const bool stealNow=async&&stealCfg==1&&rightPar&&!onC1&&!earlyB;
 if(splitCfg<0){splitCfg=1;/* run 57: default 1 (Azahar: identical images overworld/battle/menus, mono composite -37%) */FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"split_top=%d",&x)==1)splitCfg=x!=0;fclose(f);}PSPNativeMemLog("[2D] split_top=%d",splitCfg);}
 P3DS_GcNoLag=wideCfg>0;P3DS_GcFrame();  /* run 118 frame_pacing: frame boundary (no-op when off); no lag while the wide view is on */
 const int gcMode=P3DS_GpuComposeCfg();if(gcMode>=2)gcFrames++;
 const bool gcOK=!hbOn&&!wlNow&&gcMode==2&&(!stereo||gcSt)&&!rightPar&&engineA.Enabled&&(gcSt||gcEligible());
 if(!gcOK&&!wgNow){G3G_DirectResolveWhy(3);P3DS_GcDrop();}  /* gpu_direct3d: CPU composite reads NativeFrame; a picture left pending by the last GPU frame is stale */
 /* run 95 wide_early=1 (default): a wide-layout screen (party, bag, boxes, ...) used to be drawn twice: the full melonDS
    engine A pass, then widecomp over all of it (in the emulator 16 + ~55 ms, the menus' stutter). widecomp reads only
    registers / VRAM / OAM / palettes / the 3D layer, never engine A's picture, so it now runs right after engine B is handed
    to the worker (overlapping it) and the engine A pass is skipped when it succeeded. Same pixels: widecomp overwrites
    every centre pixel either way. If widecomp declines (windows, unsupported blend...) engine A is drawn as before.
    Not with stereo, display capture or a synchronous engine B. 0 = old order. */
 const bool wlEarlyWant=wlEarlyPre&&!rightPar&&!onC1&&!stealNow;
 bool wlEarly=false;
 const bool splitNow=!wlEarlyWant&&!gcOK&&!wgNow&&splitCfg&&async&&!stereo&&!rightPar&&!onC1&&!stealNow&&engineA.Enabled&&((a>>16)&3)==1&&!(reg32(0x64)&0x80000000u);
 if(splitNow){memcpy((void*)&engineAS,(const void*)&engineA,sizeof engineA);line0Done=false;bSplit=true;splitN++;}
 /* run 97 wide_stereo_par=1: with wide_early in stereo, the right eye's wide compose runs on core 2 (after engine B) while
    the main thread composes the left eye: both eyes keep their own 3D layer at about the cost of one. */
 bWideROk=false;
 /* run 117 wide_stereo_flat2d (Debug "Copy eye when 3D empty", default ON): with the slider up, a wide frame whose 3D layer has
    no visible pixel in either eye (the Pokedex: 2 hidden triangles) gives the same picture in both eyes, but each eye was
    composed in full (one per core, ~33 ms each on hardware, 15 fps). Now the left eye is composed once, shared by both
    cores (wide_split), and copied to the right eye. 0 = Run 116. */
 static int wsf=-1;if(wsf<0){wsf=P3DS_CfgInt("wide_stereo_flat2d",1)!=0;PSPNativeMemLog("[WIDE] wide_stereo_flat2d=%d",wsf);}
 bool flat3d=false;
 if(wsf&&wlEarlyWant&&stereo&&rightEye&&async){flat3d=true;if(PSPNative_GfxRegisters[0]&8){const u32*f0=GPU3D::NativeFrame,*f1=rightEye;for(int i=0;i<256*192;i++)if((f0[i]|f1[i])>>24){flat3d=false;break;}}
  static unsigned fN,fT;fT++;if(flat3d)fN++;if(fT==600){PSPNativeMemLog("[WIDE] wide_stereo_flat2d: %u of 600 slider-up wide frames copied the left eye",fN);fN=fT=0;}}
 {static int wsp=-1;if(wsp<0){wsp=P3DS_CfgInt("wide_stereo_par",1);PSPNativeMemLog("[WIDE] wide_stereo_par=%d",wsp);}
  static int wsc2=-1;if(wsc2<0)wsc2=P3DS_CfgInt("wide_stereo",1);
  if(wsp&&wsc2&&wlEarlyWant&&stereo&&rightEye&&!flat3d&&(PSPNative_GfxRegisters[0]&8)){wideRRegs=PSPNative_GfxRegisters;
   wideRSrc={GPU::VRAMFlat_ABG,GPU::VRAMFlat_AOBJ,GPU::VRAMFlat_ABGExtPal,GPU::VRAMFlat_AOBJExtPal,(const u16*)GPU::Palette,(const u16*)(GPU::Palette+512),(const u16*)GPU::OAM,rightEye};
   bWideR=true;}}  /* run 113: wide_split_r (run 106/108, default off since run 110: slower on hardware) removed; the key is ignored */
 static int wsplitCfg=-1;if(wsplitCfg<0){wsplitCfg=P3DS_CfgInt("wide_split",1);if(wsplitCfg){LightEvent_Init(&wmGo,RESET_ONESHOT);LightEvent_Init(&wmDone,RESET_ONESHOT);}PSPNativeMemLog("[WIDE] wide_split=%d",wsplitCfg);}
 static int wdCfg=-1;if(wdCfg<0)wdCfg=P3DS_CfgInt("wide_dirty",1);
 const bool wmUse=wsplitCfg&&wdCfg==1&&wlEarlyWant&&(!stereo||flat3d)&&async;
 if(wmUse){wmRegs=PSPNative_GfxRegisters;wmSrc={GPU::VRAMFlat_ABG,GPU::VRAMFlat_AOBJ,GPU::VRAMFlat_ABGExtPal,GPU::VRAMFlat_AOBJExtPal,(const u16*)GPU::Palette,(const u16*)(GPU::Palette+512),(const u16*)GPU::OAM,GPU3D::NativeFrame};wmSp.n[0]=wmSp.n[1]=0;bWideM=true;}
 if(async)bPost();
 if(wlEarlyWant){WcSrc ws={GPU::VRAMFlat_ABG,GPU::VRAMFlat_AOBJ,GPU::VRAMFlat_ABGExtPal,GPU::VRAMFlat_AOBJExtPal,(const u16*)GPU::Palette,(const u16*)(GPU::Palette+512),(const u16*)GPU::OAM,GPU3D::NativeFrame};
  if(wmUse){const int n=WC_SplitPrepare(&wcPlan,PSPNative_GfxRegisters,&ws,&wmSp,24,0);
   if(n<0){wmSp.n[0]=wmSp.n[1]=0;LightEvent_Signal(&wmGo);LightEvent_Wait(&wmDone);wlEarly=WC_Compose(&wcPlan,PSPNative_GfxRegisters,&ws,display[0],P3DS_WideFillBuf)!=0;}
   else{LightEvent_Signal(&wmGo);WC_SplitRun(&wcPlan,PSPNative_GfxRegisters,&ws,&wmSp,0);LightEvent_Wait(&wmDone);WC_SplitCommit(&wmSp,display[0],P3DS_WideFillBuf);wlEarly=true;
    static unsigned sN,sA,sB;sN++;sA+=wmSp.n[0];sB+=wmSp.n[1];if(sN==600){PSPNativeMemLog("[WIDE] wide_split: %u composes, lines main %u / worker %u avg",sN,sA/sN,sB/sN);sN=sA=sB=0;}}}
  else wlEarly=WC_Compose(&wcPlan,PSPNative_GfxRegisters,&ws,display[0],P3DS_WideFillBuf)!=0;
  if(wlEarly)line0Done=true;
  {static unsigned ne,nf;if(wlEarly)ne++;else nf++;if((ne+nf)%600==1)PSPNativeMemLog("[WIDE] wide_early: %u composed early (engine A skipped), %u fell back",ne,nf);}}
 const unsigned aEnd=splitNow?96:192;
 bool gcDone=false,gcStR=false;static unsigned gcStN;void*f60L=nullptr,*f60R=nullptr;unsigned f60CompUs=0,f60StripUs=0;int f60Shown=0,f60Why=1;{static unsigned gsl;if(gcSt&&++gsl%600==0){PSPNativeMemLog("[GPUC] stereo: %u of last 600 stereo-eligible frames GPU-composed for both eyes",gcStN);gcStN=0;}}
 if(gcOK){if(!async&&!bGpuFrame)for(unsigned line=0;line<192;line++){GPU::VCount=line;engineB.CheckWindows(line);renderer.DrawSprites(line,&engineB);renderer.DrawScanline(line,&engineB);}
  {
   gDfDec=gcDirectCfg()&&(!stereo||gcSt)&&(P3DS_InField||P3DS_InBattle)&&(power&0x8000)&&P3DS_ScaleMode()!=0&&!P3DS_MenuOpen&&P3DS_CfgInt("g3_sharp",0)!=4;P3DS_GcDeferOut=gDfDec;
   const u64 tq=svcGetSystemTick();P3DS_GcEye(0,gcSt);gcDone=gcCompose(display[0],G3G_MidReady()?G3G_MidTex(0):nullptr,&f60L,&f60CompUs);P3DS_GcDeferOut=0;pfT[PF_GCL]+=svcGetSystemTick()-tq;pfU[PU_LB]+=P3DS_GcBeginUs;G3G_AbComposeWait(P3DS_GcBeginUs);pfU[PU_LE]+=P3DS_GcEndUs;pfU[PU_LS]+=P3DS_GcSyncUs;
  }if(gcDone)gcUsed++;line0Done=true;
  if(gcDone&&gcSt){const u8*regsR=(const u8*)PSPNative_GfxRegisters;
   /* run 17: outside the overworld the right eye's BG layers get depth by priority (stereo_2d, same rule as the CPU path above): shift their H scroll in a register copy */
   static u8 regsCopy[0x100] __attribute__((aligned(4)));if(!P3DS_InField&&stereo2dCfg()){memcpy(regsCopy,regsR,sizeof regsCopy);regsR=regsCopy;
    const int cfg=stereo2dCfg(),sd2=stereoShiftF(P3DS_ScaleMode());const unsigned dc2=reg32(0),bgm=dc2&7;
    for(int i=1;i<4;i++){if(!(dc2&(0x100u<<i)))continue;const bool text=i<2||bgm==0||(i==2&&bgm==1);if(!text)continue;
     const int pr=reg16(8+i*2)&3,d=cfg==1?(pr==3?2*sd2:pr==2?(3*sd2)/2:0):cfg==4?(pr>=2?(sd2+1)/2:0):(pr==3?(sd2+1)/2:pr==2?(sd2+2)/4:0);
     const int bdE=(d&&pr>=2)?P3DS_BattleSideExtra(sd2):0;
     if(d)*(u16*)(regsCopy+0x10+i*4)=(u16)((reg16(0x10+i*4)-d)&0x1FF);
     if(bdE){P3DS_GcRowSd=sd2;P3DS_GcRowOn[i]=1;{static unsigned bdLg;if(bdLg++%300==0)PSPNativeMemLog("[BATTLE] foe deeper ground plane, BG layer %d prio %d: total right-eye px backdrop y20 %d / foe feet y96 %d / your feet y160 %d / bottom y188 %d",i,pr,d+P3DS_BattleRowExtra(20,sd2),d+P3DS_BattleRowExtra(96,sd2),d+P3DS_BattleRowExtra(160,sd2),d+P3DS_BattleRowExtra(188,sd2));}}}}
   {static unsigned nfN;if(!P3DS_InField&&++nfN%300==1)PSPNativeMemLog("[GPUC] stereo outside the overworld: both eyes GPU-composed (%u frames so far, battle %d)",nfN,P3DS_InBattle);}
   {const u64 tq=svcGetSystemTick();P3DS_GcEye(1,0);P3DS_GcDeferOut=gDfDec;gcStR=P3DS_GcComposeA(regsR,GPU::VRAMFlat_ABG,GPU::VRAMFlat_AOBJ,(const u16*)GPU::Palette,(const u16*)(GPU::Palette+512),(const u16*)GPU::OAM,rightEye,rawR,f60L?G3G_MidTex(1):nullptr,&f60R,&f60CompUs)!=0;P3DS_GcDeferOut=0;
    P3DS_GcRowSd=-1;memset(P3DS_GcRowOn,0,sizeof P3DS_GcRowOn);
    pfT[PF_GCR]+=svcGetSystemTick()-tq;pfU[PU_RB]+=P3DS_GcBeginUs;pfU[PU_RE]+=P3DS_GcEndUs;pfU[PU_RS]+=P3DS_GcSyncUs;pfR++;
   }if(gcStR)gcStN++;}}
 P3DS_GcFinish();  /* fps_tweaks 6 overlap: a left eye still pending is converted before anything reads display[0] */
 bool wgDone=false;
 if(wgNow){if(!async)for(unsigned line=0;line<192;line++){GPU::VCount=line;engineB.CheckWindows(line);renderer.DrawSprites(line,&engineB);renderer.DrawScanline(line,&engineB);}
  const u8*R0=(const u8*)PSPNative_GfxRegisters;const u8*bgv=GPU::VRAMFlat_ABG,*obv=GPU::VRAMFlat_AOBJ;const u16*bp=(const u16*)GPU::Palette,*op=(const u16*)(GPU::Palette+512),*oa=(const u16*)GPU::OAM;
  P3DS_GcEye(0,stereo);wgDone=P3DS_GcComposeWide(&wcPlan,R0,bgv,obv,bp,op,oa,GPU3D::NativeFrame,display[0],P3DS_WideFillBuf)!=0;
  if(wgDone&&stereo){P3DS_GcEye(1,0);wgDone=P3DS_GcComposeWide(&wcPlan,R0,bgv,obv,bp,op,oa,rightEye,rawR,nullptr)!=0;}
  P3DS_GcFinish();  /* run 118 frame_pacing: a left eye still pending is converted before any CPU fallback draws over it */
  static unsigned wgN,wgF;if(wgDone)wgN++;else wgF++;if((wgN+wgF)%600==0)PSPNativeMemLog("[WIDE] wide_gpu: %u of the last 600 wide frames on the GPU (%u fell back to the CPU)",wgN,wgF),wgN=wgF=0;
  if(wgDone){gcDone=true;line0Done=true;
   /* test hook wide_gpu_verify=N: every Nth GPU wide frame is also drawn by the CPU widecomp (left eye) and compared */
   static int wv=-1;if(wv<0){wv=P3DS_CfgInt("wide_gpu_verify",0);}static unsigned wvn;
   if(wv>0&&++wvn%wv==0){static u32 vc[256*192],vs[64*192];G3G_DirectResolve();WcSrc ws={GPU::VRAMFlat_ABG,GPU::VRAMFlat_AOBJ,GPU::VRAMFlat_ABGExtPal,GPU::VRAMFlat_AOBJExtPal,bp,op,oa,GPU3D::NativeFrame};
    if(WC_Compose(&wcPlan,R0,&ws,vc,vs)){unsigned bc=0,bs=0,md=0,fx=-1,fy=-1;for(int i=0;i<256*192;i++){const u32 a=vc[i],b=display[0][i];if((a&0xFFFFFF)!=(b&0xFFFFFF)){bc++;for(int k=0;k<24;k+=8){const int d=abs((int)((a>>k)&255)-(int)((b>>k)&255));if((unsigned)d>md)md=d;}if(fx==(unsigned)-1){fx=i%256;fy=i/256;}}}
     for(int i=0;i<64*192;i++)if((vs[i]|0xFF)!=(P3DS_WideFillBuf[i]|0xFF))bs++;
     PSPNativeMemLog("[WIDE] wide_gpu verify #%u: centre %u px differ (max channel diff %u, first x %d y %d), strips %u px differ | DISPCNT %08X BLDCNT %04X",wvn,bc,md,(int)fx,(int)fy,bs,(unsigned)reg32(0),(unsigned)reg16(0x50));
     static int dumped;if(bc>200&&dumped<2){dumped++;dumpBmp(dumped==1?"sdmc:/3ds/platinum3ds/wgv_gpu.bmp":"sdmc:/3ds/platinum3ds/wgv_gpu2.bmp",display[0],false);dumpBmp(dumped==1?"sdmc:/3ds/platinum3ds/wgv_cpu.bmp":"sdmc:/3ds/platinum3ds/wgv_cpu2.bmp",vc,false);}}}}
  else if(!async){/* engine B already drawn above; the CPU fallback below must not draw it again */}}
 if(!gcDone||(gcSt&&!gcStR))G3G_DirectResolveWhy(4);  /* gpu_direct3d: GPU compose rejected the frame, fall back to the CPU pixels */
 {const u64 ta=svcGetSystemTick();
 if(!gcDone&&!wlEarly)for(unsigned line=0;line<192;line++){if(line>=aEnd&&async)break;GPU::VCount=line;hbLine(engineA,line);engineA.CheckWindows(line);renderer.DrawSprites(line,&engineA);renderer.DrawScanline(line,&engineA);if(!line)line0Done=true;
  if(!async&&!gcOK&&!wgNow&&!bGpuFrame){engineB.CheckWindows(line);renderer.DrawSprites(line,&engineB);renderer.DrawScanline(line,&engineB);}}
 pfT[PF_CPUA]+=svcGetSystemTick()-ta;} P3DS_St[3]=svcGetSystemTick();
 if(stealNow&&claimB()){stealMain++;drawEngineB();}
 P3DS_Crumb(async?(excl?"b-spin":"b-wait"):"b-none",0);
 const u64 tBw=svcGetSystemTick();
 if(async)bWaitDone(excl,earlyB?2:1,earlyB);
 const u64 bFrameDone = svcGetSystemTick();
 pfT[PF_BWAIT] += bFrameDone - tBw;
 if(onC1) {
  LightEvent_Wait(&c1Done);
  pfT[PF_C1WAIT] += svcGetSystemTick() - bFrameDone;
 }
 P3DS_St[4]=svcGetSystemTick();
 huntTick();batteryTick();
 {/* run 76 display capture (DISPCAPCNT 0x4000064): the Spear Pillar / Giratina transition captures the screen into a VRAM
     bank and shows that bank; unimplemented it was black. Minimal version: source A (engine A output, 8-bit -> RGB555); display_capture=0 restores the old ignore,
     no blending, written into the LCDC bank; the enable bit is cleared like on hardware. */
  volatile u32*capR=(volatile u32*)(PSPNative_GfxRegisters+0x64);const u32 cap=*capR;
  const int capCfg=P3DS_CfgInt("display_capture",1);
  if(engineA.CaptureLatch){engineA.CaptureLatch=false;*capR=cap&0x7FFFFFFFu;static unsigned cn;if(cn++%120==0)PSPNativeMemLog("[CAP] renderer capture %08x (%u)",(unsigned)cap,cn);}
  if(capCfg==2&&(cap&0x80000000u)&&display[0]){topTouch();static const int hs[4]={128,64,128,192};const int bank=(cap>>16)&3,off=((cap>>18)&3)*0x8000,sz=(cap>>20)&3,w=sz?256:128,h=hs[sz];
   u8*vb=s_HW_LCDC_VRAM+bank*0x20000;const u32*src=display[0];
   for(int y=0;y<h;y++)for(int x=0;x<w;x++){u32 c=src[y*256+x];u16 v=(u16)(((c>>3)&31)|(((c>>11)&31)<<5)|(((c>>19)&31)<<10)|0x8000);*(u16*)(vb+((off+(y*w+x)*2)&0x1FFFF))=v;}
   *capR=cap&~0x80000000u;static unsigned nc;if(!(nc++%120))PSPNativeMemLog("[CAP] display capture bank %d off %x size %d (%u)",bank,off,sz,nc);}}
 P3DS_St[10]=svcGetSystemTick();
 P3DS_Crumb(earlyB?"b-early":"b-noearly",0);
 if(earlyB) {
  const u64 waitStart = svcGetSystemTick();
  LightEvent_Wait(&bEarlyDone);
  pfT[PF_EARLYBWAIT] += svcGetSystemTick() - waitStart;
 }
 P3DS_Crumb("b-ok",0);
 verifyGpuB();
 if(P3DS_FpsTweaks()<5){midChecks++;if(stateHash()!=hash0)midWrites++;if(midChecks>=600){PSPNativeMemLog("[2D] game state (OAM/palette/2D regs) changed DURING composite in %u of %u frames (comp_excl=%d)",midWrites,midChecks,(int)excl);midChecks=midWrites=0;}}
 if(excl)svcSetThreadPriority(CUR_THREAD_HANDLE,oldPrio);
 if(!wlNow&&gcMode==3&&!stereo&&engineA.Enabled&&gcEligible()){if(!gcScratch)gcScratch=(u32*)linearMemAlign(256*256*4,0x80);
  if(gcScratch)topTouch();
  if(gcScratch&&gcCompose(gcScratch)){gcVer++;unsigned bad=0;for(unsigned i=0;i<256*192;i++)if((gcScratch[i]^display[0][i])&0xFFFFFF){bad++;const u32 a=gcScratch[i],b=display[0][i];for(int sh=0;sh<24;sh+=8){int d=(int)((a>>sh)&255)-(int)((b>>sh)&255);if(d>4||d<-4){gcBadBig++;break;}}}
   if(bad){gcVerBad++;gcBadPx+=bad;static int dumps;if(dumps<3){char fn[96];snprintf(fn,sizeof fn,"sdmc:/3ds/platinum3ds/gpuc_mm%d_cpu.bmp",dumps);dumpBmp(fn,display[0],false);snprintf(fn,sizeof fn,"sdmc:/3ds/platinum3ds/gpuc_mm%d_gpu.bmp",dumps);dumpBmp(fn,gcScratch,false);
     unsigned fi=0;while(fi<256*192&&!((gcScratch[fi]^display[0][fi])&0xFFFFFF))fi++;PSPNativeMemLog("[GPUC] verify mismatch #%d frame %u: %u px, first at (%u,%u) cpu %08x gpu %08x",dumps,frames,bad,fi%256,fi/256,display[0][fi],gcScratch[fi]);dumps++;}}
   else gcVerOk++;}}
 if(gcMode>=2)gcLog(); P3DS_St[11]=svcGetSystemTick();
 lastDraw=3;
 {u32 kh=hidKeysHeld();static bool dumped;static int dumpAt=-2;if(dumpAt==-2){dumpAt=-1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"dump_frame=%d",&x)==1)dumpAt=x;fclose(f);}}
  bool autoDump=dumpAt>=0&&(int)frames==dumpAt;
  {/* [FXREG] Dark Cave / Flash darkness is DS 3D fog (weather 11: black FOG_COLOR, full density table) plus a light-sphere model at the player; log fog + window/blend/brightness regs on change while fog, a window or master brightness is active (max 120 lines) */
   static unsigned pv[16];static int nl;const unsigned dc=reg16(0),wn=dc&0xE000,mb=reg16(0x6c),d3=reg16(0x60);const bool fg=(d3&0x80)!=0;
   if((fg||wn||((mb>>14)&&(mb&0x1F)))&&nl<120){const unsigned v[16]={dc,reg16(0x40),reg16(0x44),reg16(0x48),reg16(0x4a),reg16(0x50),reg16(0x54),mb,d3,reg16(0x358),reg16(0x35c),reg16(0x360),reg16(0x362),reg16(0x364),reg16(0x37c),(unsigned)P3DS_InField};
    if(memcmp(v,pv,sizeof v)){memcpy(pv,v,sizeof v);nl++;PSPNativeMemLog("[FXREG] f=%u A DISPCNT %04x WIN0H %04x V %04x WININ %04x WINOUT %04x BLDCNT %04x BLDY %04x MBRIGHT %04x | DISP3DCNT %04x FOGCOL %04x OFFS %04x TAB %04x %04x %04x ..%04x field=%u",frames,v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8],v[9],v[10],v[11],v[12],v[13],v[14],v[15]);}}}
  if(autoDump){char b[400];int n=0;for(int e=0;e<2;e++){unsigned base=e?0x1000:0;n+=snprintf(b+n,sizeof b-n,"[DUMP] eng%c DISPCNT %04x%04x BG0-3CNT %04x %04x %04x %04x BLDCNT %04x BLDALPHA %04x BLDY %04x WININ %04x WINOUT %04x | ",e?'B':'A',reg16(base+2),reg16(base),reg16(base+8),reg16(base+10),reg16(base+12),reg16(base+14),reg16(base+0x50),reg16(base+0x52),reg16(base+0x54),reg16(base+0x48),reg16(base+0x4a));}
   n+=snprintf(b+n,sizeof b-n,"DISP3DCNT %04x POWCNT %04x",reg16(0x60),reg16(0x304));PSPNativeMemLog("%s",b);}
  if(autoDump||(kh&(KEY_L|KEY_R|KEY_X|KEY_Y))==(KEY_L|KEY_R|KEY_X|KEY_Y)){topTouch();/* run 43: BMP dump moved to L+R+X+Y (L+R+X = wideview) */G3G_DirectResolveWhy(5);if(!dumped){dumped=true;dumpBmp("sdmc:/3ds/platinum3ds/dump_3dlayer.bmp",GPU3D::NativeFrame,true);dumpBmp("sdmc:/3ds/platinum3ds/dump_engineA.bmp",display[0],false);dumpBmp("sdmc:/3ds/platinum3ds/dump_engineB.bmp",display[1],false);printf("[3DS] dumped BMPs\n");}}else dumped=false;}
 {/* run 34 debug: dump_seq=START,COUNT dumps composed engine A/B frames START..START+COUNT-1 as raw RGBA to sdmc:/3ds/platinum3ds/seq/ */
  static int sq0=-2,sqn=0;if(sq0==-2){sq0=-1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int a,b;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"dump_seq=%d,%d",&a,&b)==2){sq0=a;sqn=b;}fclose(f);}if(sq0>=0)mkdir("sdmc:/3ds/platinum3ds/seq",0777);}
  if(sq0>=0&&(int)frames>=sq0&&(int)frames<sq0+sqn){char fn[80];snprintf(fn,sizeof fn,"sdmc:/3ds/platinum3ds/seq/f%05u.raw",frames);FILE*f=fopen(fn,"wb");if(f){fwrite(display[0],4,256*192,f);fwrite(display[1],4,256*192,f);fclose(f);}}}
 P3DS_St[12]=svcGetSystemTick(); const u32*eyeL=nullptr,*eyeR=nullptr;
 P3DS_PathC=wgDone?(stereo?'X':'x'):!stereo?(wlEarly?'w':'m'):gcStR?'G':rightPar?'P':(flatEye&&!wlEarly)?'F':wlEarly?'W':'S';
 if(stereo&&gcStR){eyeL=display[0];eyeR=rawR;} /* run 61: both eyes from the GPU compositor */
 else if(stereo&&rightPar){eyeL=display[0];eyeR=rawR;}
 else if(stereo&&flatEye&&!wlEarly){eyeL=eyeR=display[0];static unsigned fn;if(fn++%300==0)PSPNativeMemLog("[WIDE] flat fallback frame (%u)",fn);}
 else if(stereo&&(wlEarly||wgDone)){eyeL=display[0];eyeR=rawR;} /* run 96: widecomp drew eyeL; the wide block copies it to eyeR */ /* no copies: present reads both eyes in place */
 else if(stereo){memcpy(stereoL,display[0],sizeof stereoL);if(rightEye)memcpy(GPU3D::NativeFrame,rightEye,256*192*4);else shift3D(2*sd);FrameStart(engineA);
  for(unsigned line=0;line<192;line++){GPU::VCount=line;hbLine(engineA,line);engineA.CheckWindows(line);renderer.DrawSprites(line,&engineA);renderer.DrawScanline(line,&engineA);}
  if(!rightEye)shift3D(-sd);eyeL=stereoL;eyeR=display[0];P3DS_SerialSt++;}
 P3DS_St[13]=svcGetSystemTick();
 {/* run 61 debug: dump_eyes=F dumps both eyes at frame F and every 300 frames after (6 max) -> eyeN_L/R.bmp */
  static int de=-2,dn;if(de==-2)de=P3DS_CfgInt("dump_eyes",-1);
  if(de>=0&&stereo&&eyeL&&eyeR&&dn<6&&(int)frames>=de+dn*300){char fn[64];snprintf(fn,sizeof fn,"sdmc:/3ds/platinum3ds/eye%d_L.bmp",dn);dumpBmp(fn,eyeL,false);snprintf(fn,sizeof fn,"sdmc:/3ds/platinum3ds/eye%d_R.bmp",dn);dumpBmp(fn,eyeR,false);PSPNativeMemLog("[STEREO] dumped eyes %d at frame %u field %d gcStereo %d",dn,frames,P3DS_InField,(int)gcStR);dn++;}}
 /* run 55 stereo blob diag (stereo_blobdiag=1; default 0 since run 113: its first hit froze for SD BMP dumps): every 60 stereo frames, 16x16 cells where one eye is black
    (rgb < 8) on > 60% of the cell and the other eye is not -> one log line; the first hit dumps both eyes and both 3D
    layers to sdmc:/3ds/platinum3ds/stereo_blob_{L,R,3dL,3dR}.bmp for the next report. */
 if(stereo&&eyeL&&eyeR&&!wlEarly&&!wgDone){/* run 97: not with wide_early (eyeR is only filled by the wide block below; the first battle hit dumped 4 BMPs = 2.7 s freeze) */static int cfg=-1;static unsigned n,hits;static bool dumpedB;if(cfg<0){cfg=P3DS_CfgInt("stereo_blobdiag",0)!=0;}
  if(cfg&&++n%60==0&&hits<40){int bestN=0,bx=0,by=0,beye=0;
   for(int cy=0;cy<12;cy++)for(int cx=1;cx<15;cx++){int l=0,r=0;for(int y=cy*16;y<cy*16+16;y++)for(int x=cx*16;x<cx*16+16;x++){u32 a=eyeL[y*256+x]&0xF8F8F8u,b=eyeR[y*256+x]&0xF8F8F8u;if(!a&&b)l++;else if(a&&!b)r++;}
    if(l>bestN){bestN=l;bx=cx;by=cy;beye=0;}if(r>bestN){bestN=r;bx=cx;by=cy;beye=1;}}
   if(bestN>154){hits++;G3G_DirectResolveWhy(5);const u32*t3R=P3DS_G3RightEye();PSPNativeMemLog("[STEREO] blob: %s eye black-only cell x %d..%d y %d..%d (%d/256 px) | slider %.2f field %d dispcnt %08x",beye?"RIGHT":"LEFT",bx*16,bx*16+15,by*16,by*16+15,bestN,(double)p3dsSlider(),P3DS_InField,(unsigned)reg32(0));
    if(!dumpedB){dumpedB=true;dumpBmp("sdmc:/3ds/platinum3ds/stereo_blob_L.bmp",eyeL,false);dumpBmp("sdmc:/3ds/platinum3ds/stereo_blob_R.bmp",eyeR,false);
     if(rightPar)dumpBmp("sdmc:/3ds/platinum3ds/stereo_blob_3dL.bmp",GPU3D::NativeFrame,true);if(t3R)dumpBmp("sdmc:/3ds/platinum3ds/stereo_blob_3dR.bmp",t3R,true);}}}}
 /* POWCNT1 bit15: engine A on top screen. */
 const u32 *top=(power&0x8000)?display[0]:display[1],*bot=(power&0x8000)?display[1]:display[0];
 if(!(power&0x8000)&&bStale){static u32 kBlack[256*192]; /* bss, stays zero */top=kBlack;if(stereo&&eyeL==display[1])eyeL=kBlack;if(stereo&&eyeR==display[1])eyeR=kBlack;
  if(bStaleShown++%60==0)PSPNativeMemLog("[2D] screen swap: engine B still held the Bottom menu picture, top shown black this frame (%u so far)",bStaleShown);}
 /* run 126 panel_party: freeze clean overworld; snap LCD-top ONLY (SUB_MAIN -> party on top).
    Never bot/sPanelBot (run 125 bot-fallback ate chrome leftover -> recursive JOHTO in the panel). */
 static u32 sFieldHold[256*192],sPartySnap[256*192]; static int sRetHold,sRetN,sSwapAct; /* run 144: tab swap under way: freeze/hide the app picture so the Cancel press is never seen */ /* run 143: after the app closes keep the held field + strips until the live field's fade-in from black is done */ static int sPartySnapOk,sPartySrcLogged,sPartyNbLogged; static unsigned sPartySnapGen;
 static u32 sStripHold[2][64*192]; static int sStripHoldOk; /* run 136 */
 auto ppDistinct=[](const u32 *p)->int{if(!p)return 0;u32 seen[48];int ns=0;for(int i=0;i<256*192;i+=11){const u32 r=p[i]&0xFFFFFFu;int f=0;for(int k=0;k<ns;k++)if(seen[k]==r){f=1;break;}if(!f&&ns<(int)(sizeof seen/sizeof *seen))seen[ns++]=r;}return ns;};
 {static int rhPrevOn;if(rhPrevOn&&!P3DS_PanelPartyOn&&P3DS_InField&&sFieldHoldOk){sRetHold=1;sRetN=0;PSPNativeMemLog("[PANEL] return hold: keep the held field until the fade-in is done");}rhPrevOn=P3DS_PanelPartyOn;} /* the glue clears On the frame the field returns */
 if(sRetHold){topTouch();holdSync();sRetN++;unsigned long lv=0,hv=0;if(display[0])for(int i=0;i<256*192;i+=7){const u32 q=display[0][i],h=sFieldHold[i];lv+=(q&255)+((q>>8)&255)+((q>>16)&255);hv+=(h&255)+((h>>8)&255)+((h>>16)&255);}
  const bool done=display[0]&&lv*100>=hv*96; /* the palette fade-in (SS-FADE steps=6) is over once the live field is as bright as the held one */
  if(P3DS_PanelPartyWant||P3DS_PanelPartyOn||!P3DS_InField||sRetN>120||(sRetN>=2&&done)){PSPNativeMemLog("[PANEL] return hold released after %d frames (live %lu held %lu)",sRetN,lv,hv);sRetHold=0;}}
 if(!(P3DS_PanelPartyWant||P3DS_PanelPartyOn||sRetHold)){
  if(P3DS_InField&&(power&0x8000)&&display[0]){gHoldBuf=sFieldHold;if(P3DS_GcPendTex(display[0]))gHoldLazy=1;else{gHoldLazy=0;memcpy(sFieldHold,display[0],sizeof sFieldHold);}sFieldHoldOk=1;}
 }else if(P3DS_InField&&(power&0x8000)&&display[0]&&!sFieldHoldOk){
  topTouch();memcpy(sFieldHold,display[0],sizeof sFieldHold);sFieldHoldOk=1;
 }
 {/* run 127: new open => forget the previous open's snapshot (run 126 log: 2nd open showed the 1st open's stale snap=1 frame; Exit
     clears On before the InField branch, so sPartySnapOk was never reset) */
  static int ppPrevOn;if(P3DS_PanelPartyOn&&!ppPrevOn){sPartySnapOk=0;sPartySrcLogged=0;sPartyNbLogged=0;}ppPrevOn=P3DS_PanelPartyOn;}
 if(P3DS_PanelPartyOn&&!P3DS_InField){
  /* run 128: the party list is engine A (GX_DISP_SELECT_SUB_MAIN: main engine on the bottom LCD, POWCNT bit15 clear), i.e. display[0]; engine B (top) only has a backdrop
     (plus, until redrawn, the chrome this code painted into display[1] in the field). Snap display[0] here, before the chrome is painted onto it. */
  const bool useEngineB = (power & 0x8000) != 0;
  const u32 *src = useEngineB ? (bStale ? nullptr : display[1]) : display[0];
  int nSrc = 0;
  u32 rgbChanges = 0;
  if (src && !sSwapAct && !P3DS_PanelBagOpening) {
    for (int i = 0; i < 256 * 192; i++) {
      const u32 rgb = src[i] & 0xFFFFFFu;
      nSrc += rgb != 0;
      rgbChanges |= rgb ^ (sPartySnap[i] & 0xFFFFFFu);
    }
  }
  /* run 137: flashing fix. A frame where the engine buffer is mid-redraw/cleared (or the source engine just swapped) has few lit pixels; showing it flashed the panel. Take a frame only if it is not far emptier than the held one, or it persists 4 frames (a real screen change). */
  static int ppHeldNz,ppLowRun;const bool ppTake=nSrc>64&&(!sPartySnapOk||nSrc*10>=ppHeldNz*6||++ppLowRun>=4);if(ppTake){ppLowRun=0;ppHeldNz=nSrc;}else if(nSrc>=ppHeldNz*6/10)ppLowRun=0;
  if (ppTake && (!sPartySnapOk || rgbChanges)) {
    for (int i = 0; i < 256 * 192; i++) sPartySnap[i] = src[i] | 0xFF000000u;
    sPartySnapOk = 1;
    sPartySnapGen++;
  } /* run 134: engine B pixels have alpha 0 */
  if(!sPartySrcLogged&&nSrc>64){sPartySrcLogged=1;
   PSPNativeMemLog("[PANEL] party source engine %c rgbNz=%d distinct~%d pow=%04x /* once per open */",useEngineB?'B':'A',nSrc,ppDistinct(src),(unsigned)power);}
 }
 if(P3DS_PanelPartyOn&&P3DS_InField){P3DS_PanelPartyOn=0;P3DS_PanelPartyWant=0;sPartySnapOk=0;sPartySrcLogged=0;sPartyNbLogged=0;PSPNativeMemLog("[PANEL] cleared On (back in the field)");if(sFieldHoldOk){sRetHold=1;sRetN=0;}}
 u64 tComp=svcGetSystemTick();
 /* run 54 wide_fx: before the save box / toast / fps overlay are drawn onto the centre */
 const bool titleCleanActive=titleCleanCfg()&&P3DS_InTitle;
 static bool fillReady;fillReady=false;
 static bool wnReady;wnReady=false;
 bool wlDone=false;
 if(wlNow){const u64 tw=svcGetSystemTick();u32*c0=(u32*)(stereo?eyeL:top);
  WcSrc ws={GPU::VRAMFlat_ABG,GPU::VRAMFlat_AOBJ,GPU::VRAMFlat_ABGExtPal,GPU::VRAMFlat_AOBJExtPal,(const u16*)GPU::Palette,(const u16*)(GPU::Palette+512),(const u16*)GPU::OAM,GPU3D::NativeFrame};
  if(wlEarly||wgDone)wlDone=true; /* run 95 wide_early: already composed while engine B rendered */
  else wlDone=WC_Compose(&wcPlan,PSPNative_GfxRegisters,&ws,c0,P3DS_WideFillBuf)!=0;
  /* run 97 wide_stereo=1 (default): the right eye is composed again with the right-eye 3D layer, so battles (3D BG0) keep
     their slider depth (run 96 copied the left picture: no depth). 2D layers are the same in both eyes, as in wide_early's
     source. 0 = copy (faster, flat). */
  if(wlDone&&stereo&&eyeR&&eyeR!=c0){static int wsc=-1;if(wsc<0){wsc=P3DS_CfgInt("wide_stereo",1);PSPNativeMemLog("[WIDE] wide_stereo=%d",wsc);}
   static u32 stripR[64*192];bool okR=false;
   if(bWideROk||wgDone)okR=true;else if(wsc&&rightEye&&!flat3d&&(PSPNative_GfxRegisters[0]&8)){WcSrc wr=ws;wr.frame3d=rightEye;okR=WC_ComposeEye(&wcPlan,PSPNative_GfxRegisters,&wr,(u32*)eyeR,stripR,1)!=0;}
   if(!okR)memcpy((u32*)eyeR,c0,256*192*4);}
  {static int lastOk=-1;const int why=WC_Check(&wcPlan,PSPNative_GfxRegisters,GPU3D::NativeFrame!=nullptr);const int key=wlDone?0:why+1;if(key!=lastOk){lastOk=key;PSPNativeMemLog("[WIDE] wide layout drawing %s (reason %d, DISPCNT %08X BLDCNT %04X)",wlDone?"on":"off",why,*(const u32*)PSPNative_GfxRegisters,*(const u16*)(PSPNative_GfxRegisters+0x50));}}
  if(wlDone){wnReady=true;static u64 acc;static unsigned n;acc+=svcGetSystemTick()-tw;if(++n==600){PSPNativeMemLog("[WIDE] wide layout avg %u us/frame",(unsigned)(acc/600/CPU_TICKS_PER_USEC));acc=0;n=0;}}}
 P3DS_St[6]=svcGetSystemTick();if(wlDone){}
 else if(wideCfg>0&&P3DS_ScaleMode()==1&&P3DS_WideMenusCfg()&&!(G3G_Wide&&G3G_StripValid>=1&&P3DS_InField)&&(power&0x8000)&&engineA.Enabled){wideNative();wnReady=true;}
 else if(wideCfg>0&&P3DS_ScaleMode()==1&&wideFillCfg()&&!(G3G_Wide&&G3G_StripValid>=1&&P3DS_InField)&&(power&0x8000)){wideFill(display[0]);fillReady=true;}
 P3DS_WeatherOn=0;if(wideWeatherCfg()&&G3G_Wide&&G3G_StripValid>=1&&P3DS_InField&&(power&0x8000))wideWeatherRun();
 {extern int P3DS_HazeOn;P3DS_HazeOn=0;if(P3DS_WideHazeCfg()&&!wideFxCfg()&&G3G_Wide&&G3G_StripValid>=1&&P3DS_InField&&(power&0x8000))wideHaze(display[0],GPU3D::NativeFrame);}
 P3DS_WideFxOn=0;if(wideFxCfg()&&G3G_Wide&&G3G_StripValid>=1&&P3DS_InField&&(power&0x8000)){u64 tw=svcGetSystemTick();wideFx(display[0],GPU3D::NativeFrame,G3G_StripValid>=2?2:1);P3DS_WideFxOn=G3G_StripValid>=2?2:1;
  {static u64 acc;static unsigned n;acc+=svcGetSystemTick()-tw;if(++n==600){PSPNativeMemLog("[WIDE] wide_fx avg %u us/frame",(unsigned)(acc/600/268));acc=0;n=0;}}}
 const u64 tOv0=svcGetSystemTick();pfT[PF_WPREP]+=tOv0-tComp;
 {u64 nowT=svcGetSystemTick();u32*tt=(u32*)(stereo?eyeL:top);drawSaveBox(tt,nowT);if(stereo)drawSaveBox((u32*)eyeR,nowT);drawToast(tt,nowT);if(stereo)drawToast((u32*)eyeR,nowT);drawAchToast(tt,0);if(stereo)drawAchToast((u32*)eyeR,1);drawListCursor(tt);drawShinyFlash(tt,nowT);if(stereo)drawShinyFlash((u32*)eyeR,nowT);drawTodTint(tt);if(stereo)drawTodTint((u32*)eyeR);drawDwRoll(tt);if(stereo)drawDwRoll((u32*)eyeR);drawFolTalk(tt,nowT);drawFolEmote(tt,nowT);drawDwDebris(tt,0);if(stereo)drawDwDebris((u32*)eyeR,1);if(stereo)drawFolEmote((u32*)eyeR,nowT);drawSumIV(tt);drawPrompt(tt);if(stereo)drawPrompt((u32*)eyeR);if(stereo&&sumShow)drawSumIV((u32*)eyeR);if(stereo)drawFolTalk((u32*)eyeR,nowT);
  drawTypeHint((u32*)bot);if(P3DS_RibbonOn()){P3DS_RibbonFrame(hidKeysHeld());P3DS_RibbonDraw((unsigned*)bot);}
  if(P3DS_CelebOn()){P3DS_CelebDraw((unsigned*)(stereo?eyeL:top),0);if(stereo)P3DS_CelebDraw((unsigned*)eyeR,1);}
 } P3DS_St[7]=svcGetSystemTick();
 {static int fi;if(!fi){fi=1;fpsOn=P3DS_CfgInt("fps_counter",0)!=0;/* run 64: default 0 (clean launch) */}}/* run 55: fps_counter (menu-saved) */
 P3DS_GpTextOn=0;
 if(fpsOn){char b[24];snprintf(b,sizeof b,"%u %u.%u",fpsShown,msShown/10,msShown%10);u32*tt=(u32*)(stereo?eyeL:top);
  if(P3DS_GcPendTex(tt)&&(!stereo||P3DS_GcPendTex(eyeR))){/* direct screen: the same pixels as drawText, drawn by the GPU over the picture (gpuscale.c textMaskDraw) */
   memset(P3DS_GpTextMask,0,sizeof P3DS_GpTextMask);int x=2;for(const char*t=b;*t;t++,x+=4){int g=*t>='0'&&*t<='9'?*t-'0':*t=='.'?12:*t=='-'?11:13;
    for(int yy=-1;yy<6;yy++)for(int xx=-1;xx<4;xx++){int px=x+xx,py=2+yy;if(px<0||px>63||py<0||py>7)continue;
     bool on=yy>=0&&yy<5&&xx>=0&&xx<3&&((kFont[g]>>(14-(yy*3+xx)))&1);P3DS_GpTextMask[py][px]=on?2:1;}}P3DS_GpTextOn=1;}
  else{topTouch();drawText(tt,2,2,b);if(stereo)drawText((u32*)eyeR,2,2,b);}}
 {u64 nowS=svcGetSystemTick();if(splashActive(nowS)){drawSplash((u32*)(stereo?eyeL:top),nowS);if(stereo)drawSplash((u32*)eyeR,nowS);}}
 {const u64 tBm0=svcGetSystemTick();pfT[PF_OVL]+=tBm0-tComp;pfT[PF_DRAWS]+=tBm0-tOv0;}
 /* run 78 bottom_strip=1 (default 0): overworld-only strip of 4 buttons in the bottom screen's lower border (DS y 177..190,
    x 8..207: below the Poketch screen, left of its buttons). Taps there go to the strip only (main3ds touch read
    drops them while P3DS_StripOn). Off = nothing drawn, every touch unchanged. */
 static int stripReq=0;
 /* run 100 lower_menu (default 0): GabeSud0's bottom-screen icon menu (lowermenu.c, THIRD_PARTY.md) around the field's
    lower screen. Shown only in the field; a button press opens that start-menu entry through the same request path as the
    settings Menu tab (P3DS_StartReq); the item squares press Y (registered item). The touch inside the panel goes to the
    game unchanged (main3ds). Off: nothing changes. */
 {const bool lmWant=P3DS_CfgInt("lower_menu",0)&&P3DS_InField&&!P3DS_ConsoleOnBottom()&&!cy.open; /* hidden while the ZR hunt stats overlay is up */
  if(lmWant&&!NitroPort_LowerMenuOn()){const int fe=p3dsFemale();if(fe>=0)NitroPort_LowerMenuSetFemale(fe);}if(lmWant!=(NitroPort_LowerMenuOn()!=0))NitroPort_LowerMenuShow(lmWant);P3DS_LowerMenuDraw=lmWant&&NitroPort_LowerMenuOn();
  if(P3DS_LowerMenuDraw){const int b=NitroPort_LowerMenuTakePress();
   if(b>=0){static const int opt[NITRO_LOWERMENU_COUNT]={-1,1,2,3,0,-1,4,5,-2,-2};extern volatile int P3DS_StartReq;const int o=opt[b];
    if(o>=0)P3DS_StartReq=o;else if(o==-2)lmYPress=3;
    else if(b==NITRO_LOWERMENU_POKETCH){lmTouchInj=4;} /* run 102: the Pokétch's upper side button (next app) */
    else if(b==NITRO_LOWERMENU_MAP){if(p3dsHasTownMap()){P3DS_BagPocketReq=7;P3DS_StartReq=2;P3DS_Toast("Town Map is in Key Items");}else P3DS_Toast("No Town Map yet");}PSPNativeMemLog("[LMENU] button %d -> %s %d",b,o>=0?"start menu option":o==-2?"Y (item)":"none",o);}}}
 {P3DS_StripOn=P3DS_CfgInt("bottom_strip",0)&&P3DS_InField&&appCfg()&&!P3DS_ConsoleOnBottom()&&!P3DS_LowerMenuDraw&&!cy.open;
  if(P3DS_StripOn&&bot){static const char*const nm[4]={"Menu","Wide","Ball","Set"};u32*bb=(u32*)bot;
   for(int i=0;i<4;i++){const int x0=8+i*50;plate(bb,x0,177,x0+48,191,0x00F8F8F8u,0x00585858u);gtxt(bb,x0+24-gw(nm[i])/2,176,nm[i],kMsgFg,kMsgSh);}
   if(hidKeysDown()&KEY_TOUCH){touchPosition tp;hidTouchRead(&tp);int x,y;if(touchToDS(tp.px,tp.py,&x,&y)&&y>=176&&x>=8&&x<208){const int b=(x-8)/50;if(b>=0&&b<4){stripReq=b+1;PSPNativeMemLog("[STRIP] %s",nm[b]);}}}}}
 int mode=P3DS_ScaleMode();
 static int gpuOk=-1;if(mode&&gpuOk<0){gpuOk=P3DS_GpuInit();printf("[3DS] GPU scaling init %s\n",gpuOk?"ok":"FAILED (falling back to 1:1)");}
 {/* run 43 wideview (experimental, config wideview=1, default 0; L+R+X toggles): Hor+ side strips of the 3D world in
     the pillarbox (mode 1, overworld only, 3D on BG0). The flag drives the NEXT frame's 3D pass; the strips drawn now
     come from this frame's pass. Off = no extra work at all. */
  if(wideCfg<0){wideCfg=1;/* ss3ds: default on (wide 400 px top screen) */FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"wideview=%d",&x)==1)wideCfg=x!=0;fclose(f);}PSPNativeMemLog("[WIDE] wideview=%d",wideCfg);}
  /* 2026-11-19: the L+R+X wide view toggle is gone; wide view is how the port looks (config.txt wideview=0 is still read as an escape hatch) */
  const u32 dc=reg32(0);const bool act=wideCfg&&P3DS_InField&&mode==1&&gpuOk==1&&(dc&8)&&(dc&0x100)&&((dc>>16)&3)==1&&engineA.Enabled;
  {static int lastAct=-1;if((int)act!=lastAct){lastAct=act;PSPNativeMemLog("[WIDE] active=%d (cfg %d field %d mode %d 3Dbg0 %d)",(int)act,wideCfg,P3DS_InField,mode,(int)((dc&0x108)==0x108));}}
  {const u64 tds=svcGetSystemTick();G3G_DirectStrips();pfT[PF_DSTRIP]+=svcGetSystemTick()-tds;}/* gpu_direct3d wide: convert this frame's side strips from the readback G3G_End queued */
  P3DS_WideStrip[0]=act&&G3G_Wide&&G3G_StripValid>=1?G3G_Strip[0]:nullptr;P3DS_WideStrip[1]=act&&G3G_Wide&&G3G_StripValid>=2?G3G_Strip[1]:nullptr;
  if(act&&P3DS_WideStrip[0]){/* window-aware 3D strips (see winEdge): rows where the DS hides BG0 (the 3D) at the edge get the backdrop; rows where a black BG covers the edge (edgeBgBlack) go black */
   const u16 bp=*(const u16*)GPU::Palette;const u32 r=bp&31,g=(bp>>5)&31,b=(bp>>10)&31;const u32 bdc=(((r<<3)|(r>>2))<<24)|(((g<<3)|(g>>2))<<16)|(((b<<3)|(b>>2))<<8)|0xFF;
   static u32 wcp[2][64*192];bool cp=false;unsigned rows=0;for(int y=0;y<192;y++)for(int side=0;side<2;side++){const bool hid=!(winEdge(y,side)&1);if(!hid&&!edgeBgBlack(y,side))continue;rows++;
    if(!cp){cp=true;for(int e=0;e<2;e++)if(P3DS_WideStrip[e]){memcpy(wcp[e],P3DS_WideStrip[e],sizeof wcp[e]);P3DS_WideStrip[e]=wcp[e];}} /* a copy: the 3D strips may be reused unchanged next frame */
    for(int e=0;e<2;e++)if(P3DS_WideStrip[e]){u32*o=wcp[e]+y*64+side*32;for(int x=0;x<32;x++)o[x]=hid?bdc:0x000000FFu;}}
   if(rows){winStripN++;winStripRows+=rows;if(winStripN==1||winStripN%300==0)PSPNativeMemLog("[WIDE] window/black-BG hidden strip rows: %u frames, avg %u rows (DISPCNT %08x WININ %04x WINOUT %04x)",winStripN,winStripRows/winStripN,(unsigned)dc,reg16(0x48),reg16(0x4A));}}
  if(!P3DS_WideStrip[0]&&fillReady&&gpuOk==1){P3DS_WideStrip[0]=P3DS_WideFillBuf;P3DS_WideStrip[1]=P3DS_WideFillBuf;P3DS_WideFxOn=1;}
  if(!P3DS_WideStrip[0]&&wnReady&&gpuOk==1){P3DS_WideStrip[0]=P3DS_WideFillBuf;P3DS_WideStrip[1]=stereo?P3DS_WideFillBuf:nullptr;} /* run 65 wide_menus (fade applied in strips()) */
  /* 2026-11-05: only the real wide_fx pass fills P3DS_WideFxBuf; the edge fill also sets WideFxOn (= fades already in the picture),
     and this swap used to replace its strips with the empty wide_fx buffer: edge fill always showed black bars */
  if(wideFxCfg()&&P3DS_WideFxOn>=1&&P3DS_WideStrip[0])P3DS_WideStrip[0]=P3DS_WideFxBuf[0];if(wideFxCfg()&&P3DS_WideFxOn>=2&&P3DS_WideStrip[1])P3DS_WideStrip[1]=P3DS_WideFxBuf[1];
  if(act){static bool retried;if(P3DS_GcWideRelease()||!retried){retried=true;G3G_WideRetry();}} /* run 112: the field wide view gets the VRAM back from wide_gpu */
  G3G_Wide=act;
  /* run 45 wide_cull=1 (default; only while wideview is active): the game's box test (map props / buildings) uses a
     1.25x wider view so the side strips are populated. 0 = DS culling (edge pop-in). */
  static int wideCull=-1;if(wideCull<0){wideCull=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"wide_cull=%d",&x)==1)wideCull=x!=0;fclose(f);}PSPNativeMemLog("[WIDE] wide_cull=%d",wideCull);}
  P3DS_WideCull=act&&wideCull;
  /* run 47 land_fastload=1 (default: only while wideview is active; 2 = always, 0 = off): the game's lazy map-block
     loader reads a new block's model in 56 KiB chunks, one per frame, then binds/initialises it on later frames; with
     the wider view the far corners can show a block (grass, ground) popping in during those frames. Fast mode does the
     whole read + bind + init in one task step (ROM is cached; ~100-200 KiB). */
  static int landCfg=-1;if(landCfg<0){landCfg=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"land_fastload=%d",&x)==1)landCfg=x;fclose(f);}PSPNativeMemLog("[WIDE] land_fastload=%d",landCfg);}
  P3DS_LandFast=landCfg==2||(landCfg==1&&act);} P3DS_St[8]=svcGetSystemTick();
 {/* SoulSilver battle Y screen / ZR hunt stats overlay: the bottom picture is ours while it is up; the game underneath keeps running */
  const bool txOk=mode&&gpuOk==1&&btxMode();const u32*cyImg=cyFrame(txOk);if(cyImg){bot=cyImg;{extern int P3DS_BotFade;P3DS_BotFade=0;}{extern volatile int P3DS_PnOn;P3DS_PnOn=0;}}}
 unsigned presentFenceIdleUs=0;
 {/* sharp game text: the screens whose picture this present is the live engine picture (not a held, black, panel or menu picture) */
  const u32*liveTop=(power&0x8000)?display[0]:display[1],*liveBot=(power&0x8000)?display[1]:display[0];
  const bool topOk=mode&&gpuOk==1&&top==liveTop&&(!stereo||eyeL!=nullptr);
  const bool botOk=mode&&gpuOk==1&&!P3DS_ConsoleOnBottom()&&bot==liveBot&&!P3DS_MenuOpen&&!P3DS_StripOn&&!P3DS_LowerMenuDraw;
  const unsigned gtNow=(topOk?1u:0u)|(botOk?2u:0u);
  /* P3DS_GtSync already ran this frame with gtOkPrev (VRAM in or out to match). Present the layer
     with those same bits: using this frame's decision a frame early flashed missing text when a
     tab or menu hid the layer, or doubled it when the layer came back. This decision is synced next frame. */
  if(gtNow!=gtOkPrev){static unsigned n;if(n++<60)PSPNativeMemLog("[GTEXT] screens: top %s, bottom %s",topOk?"live":top!=liveTop?"held/other picture":"not scaled",
   botOk?"live":P3DS_ConsoleOnBottom()?"console":bot!=liveBot?"panel picture":P3DS_MenuOpen?"menu open":(P3DS_StripOn||P3DS_LowerMenuDraw)?"bottom menu":"not scaled");}
  P3DS_GtScreensNow=gtOkPrev;if(!(gtOkPrev&1u))P3DS_GtWidePlan=nullptr;
  gtOkPrev=gtNow;}
 if(mode&&gpuOk==1){extern int P3DS_GpuBotSame;P3DS_GpuBotSame=(power&0x8000)&&P3DS_BottomReused;
  /* fps60: the in-between picture goes out first, top screen only; the present below then waits for the screen to take it, so the two
     land on two V-blanks in a row. Only while this tick's own picture is still on the GPU and is what gets shown. */
  if(f60L){const u32*tp0=stereo?eyeL:top;
   if(tp0!=display[0]||(stereo&&eyeR!=rawR)||P3DS_MenuOpen)f60Why=3;
   else if(!P3DS_GcPendTex(tp0)||(stereo&&(!f60R||!P3DS_GcPendTex(eyeR))))f60Why=2;
   else{u32*midStrips[2]={};const u64 ts=svcGetSystemTick();bool ok=true;
    for(int eye=0;eye<(stereo?2:1);eye++){
     if(!P3DS_WideStrip[eye])continue;
     if(P3DS_WideStrip[eye]!=G3G_Strip[eye]){ok=false;break;}
     midStrips[eye]=P3DS_GpuMidStripBuffer(eye);
     if(!midStrips[eye]||!G3G_MidStrip(eye,midStrips[eye])){ok=false;break;}
    }
    f60StripUs=(unsigned)((svcGetSystemTick()-ts)/CPU_TICKS_PER_USEC);
    if(!ok)f60Why=3;else f60Shown=P3DS_GpuPresentMid(f60L,stereo?f60R:nullptr,midStrips[0],midStrips[1],mode,stereo?1:0);}}
  else if(G3G_FpsHold()){const u32*tp0=stereo?eyeL:top;void*rl=P3DS_GcPendTex(display[0]);void*rr=stereo?(void*)P3DS_GcPendTex(rawR):nullptr;
   if(tp0!=display[0]||(stereo&&eyeR!=rawR)||P3DS_MenuOpen||!rl||(stereo&&!rr))f60Why=2;
   else f60Shown=P3DS_GpuPresentMid(rl,rr,P3DS_WideStrip[0],stereo?P3DS_WideStrip[1]:nullptr,mode,stereo?1:0);}
  {const u64 tp=svcGetSystemTick();P3DS_GpuPresent(stereo?eyeL:top,bot,mode,!P3DS_ConsoleOnBottom(),stereo?eyeR:nullptr);P3DS_GtScreensNow=0;pfT[PF_PRESENT]+=svcGetSystemTick()-tp;pfU[PU_PB]+=P3DS_GpBeginUs;pfU[PU_PT]+=P3DS_GpTearUs;presentFenceIdleUs=P3DS_GpFenceIdleUs;pfU[PU_PE]+=P3DS_GpEndUs;pfU[PU_UP]+=P3DS_GpUpUs;pfU[PU_DR]+=P3DS_GpDrawUs;pfU[PU_ST]+=P3DS_GpStripUs;}}
 else{P3DS_PresentDeferSettle();topTouch();present(GFX_TOP,400,top,0);if(!P3DS_ConsoleOnBottom())present(GFX_BOTTOM,320,bot,0);gfxFlushBuffers();gfxSwapBuffers();}
 P3DS_GameFramePresented();
 u64 tEnd=svcGetSystemTick();last2DUs=(unsigned)((tEnd-t0)/CPU_TICKS_PER_USEC);frames++;P3DS_MmMap=P3DS_CurMap;{extern volatile int P3DS_CatchInfo[20];if(P3DS_CatchInfo[0]&&!P3DS_InField)P3DS_InBattle=1;else if(P3DS_InField)P3DS_InBattle=0;}
 {static unsigned bl;if(++bl%600==0){if(P3DS_LogLevel()>=2)PSPNativeMemLog("[2D] bottom_skip: reused %u, recomposed %u of last 600 | steal_b: engine B on main %u, worker %u | early_b %u | split_top %u",bSkips,bDraws,stealMain,stealWorker,earlyN,splitN);earlyN=0;splitN=0;bSkips=bDraws=0;stealMain=stealWorker=0;lS=lD=0;}}
 {unsigned total=tPrevEnd?(unsigned)((tEnd-tPrevEnd)/CPU_TICKS_PER_USEC):0,comp=(unsigned)((tComp-t0)/CPU_TICKS_PER_USEC),gpu=(unsigned)((tEnd-tComp)/CPU_TICKS_PER_USEC);
#ifdef P3DS_PROF
  {const int fp=P3DS_CfgInt("field_prof",0);/* run 113: AUTO (2) was the default; profiling every load now only on request */static unsigned armed;if(fp==2){if(P3DS_InField!=lastFld)armed=240;lastFld=P3DS_InField;P3DS_ProfOn=armed?(armed--,1):0;}else P3DS_ProfOn=fp;P3DS_HeapFastStats();}
  {static u64 lp;const u64 n=svcGetSystemTick();P3DS_ProfFrame(lp?(unsigned)((n-lp)/CPU_TICKS_PER_MSEC):0);lp=n;}
#endif
  if (total >= 1000000) {
   PSPNativeMemLog("[STALL] f=%u frame %u ms | before present %u | text sync %u | ROM cache pump %u | 3D flush %u | composite %u | present %u",
       frames, total/1000, (unsigned)((presentStart-tPrevEnd)/CPU_TICKS_PER_MSEC),
       (unsigned)((prefetchStart-textStart)/CPU_TICKS_PER_MSEC), (unsigned)((prefetchEnd-prefetchStart)/CPU_TICKS_PER_MSEC),
       (unsigned)((flushEnd-flushStart)/CPU_TICKS_PER_MSEC), comp/1000, gpu/1000);
  }
  {/* run 43 [SPIKE] diag: any frame > spike_ms (default 70, 0 = off) logs where the time went (max 60 lines / session).
     g3soft time includes the game's own CPU work between frame start and the 3D flush (loads, decompression). */
   static int spikeMs=-1;static unsigned spikeN;bool spkPr=false;if(spikeMs<0){spikeMs=70;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"spike_ms=%d",&x)==1)spikeMs=x;fclose(f);}PSPNativeMemLog("[SPIKE] spike_ms=%d",spikeMs);}
   static unsigned lastSpikeF,runLen;/* run 94: a run of back-to-back slow frames (a menu, a 3D-slider present) logs its first frame and then every 30th, so the 60-line cap is not used up by one slow screen */
   /* run 106 battle_log=1 (default): outside the field (battles, menus) every frame over 40 ms logs the same stage lines (40 per episode, see below), so a one-frame-a-second hitch in a battle shows where it went without editing spike_ms (0 = old threshold) */
   static int blCfg=-1;static unsigned battleN;if(blCfg<0){blCfg=P3DS_CfgInt("battle_log",1)!=0;PSPNativeMemLog("[SPIKE] battle_log=%d",blCfg);}
   /* one episode = one stay outside the field (a battle, a menu): 40 stage lines each, then a [BATT] summary line when it ends */
   static int epField=1;static unsigned epL0,epB0,epF0,epN,epSum,ep34,ep40,ep50,epMax;static double epSl;
   if(blCfg&&total){if(P3DS_InField!=epField){if(!epField&&epN>=30)PSPNativeMemLog("[BATT] f=%u..%u slider %.2f: %u frames, avg %u.%u ms, over 34 ms %u, over 40 ms %u, over 50 ms %u, max %u ms | log %u lines %u KB (incl. hidden detail)",epF0,frames,epSl,epN,epSum/epN/1000,epSum/epN/100%10,ep34,ep40,ep50,epMax/1000,P3DS_LogLines-epL0,(P3DS_LogBytes-epB0)/1024);
     epField=P3DS_InField;epF0=frames;epN=epSum=ep34=ep40=ep50=epMax=0;epL0=P3DS_LogLines;epB0=P3DS_LogBytes;battleN=0;epSl=p3dsSlider();}
    if(!epField){epN++;epSum+=total;if(total>34000)ep34++;if(total>40000)ep40++;if(total>50000)ep50++;if(total>epMax)epMax=total;}}
   const bool bsp=blCfg&&!P3DS_InField&&spikeMs>40&&total>40000u&&total<=(unsigned)spikeMs*1000;
   const bool spk=(spikeMs>0&&total>(unsigned)spikeMs*1000)||bsp;if(spk){runLen=(frames==lastSpikeF+1)?runLen+1:0;lastSpikeF=frames;}
   if(spk&&(runLen==0||runLen%30==0)&&(bsp?battleN<40:spikeN<60)){spkPr=true;if(bsp)battleN++;else spikeN++;struct mallinfo mi=mallinfo();
    PSPNativeMemLog("[SPIKE] f=%u frame %u ms = g3soft+game %u + composite %u + present %u | romfs %u ms (%u x16K misses) | GetTexture %u ms | GPU tex create %u ms (%u) | tris %u | malloc %u KiB | field %d slider %.2f bottom %s split %d/%d path %c%s",
     frames,total/1000,g3us/1000,comp/1000,gpu/1000,(unsigned)(P3DS_SpkRomTicks/CPU_TICKS_PER_MSEC),P3DS_SpkRomBlocks,(unsigned)(P3DS_SpkGetTexTicks/CPU_TICKS_PER_MSEC),(unsigned)(P3DS_SpkTexTicks/CPU_TICKS_PER_MSEC),P3DS_SpkTexN,P3DS_G3LastTris(),(unsigned)(mi.uordblks/1024),P3DS_InField,(double)p3dsSlider(),P3DS_BottomReused?"reused":"drawn",wmSp.n[0],wmSp.n[1],P3DS_PathC,bsp?" [battle_log]":"");
    /* run 86: second line: the map (and the one before it, whether zone_prefetch had learned it, ms since the change) and the ROM files that missed */
    {auto D=[&](int a,int b)->unsigned{return P3DS_St[b]>P3DS_St[a]?(unsigned)((P3DS_St[b]-P3DS_St[a])/CPU_TICKS_PER_USEC/1000):0;};P3DS_St[0]=t0;P3DS_St[5]=tComp;P3DS_St[9]=tEnd;
     PSPNativeMemLog("[SPIKE] f=%u stages ms: bind %u | widePlan %u | engineA %u | B-wait %u | capture..tComp %u | wideCompose %u | overlays %u | wide/app %u | present %u | game-wait %u | cap %u bwait+gc %u misc %u stereo %u%s blob %u",frames,D(0,1),D(1,2),D(2,3),D(3,4),D(4,5),D(5,6),D(6,7),D(7,8),D(8,9),(unsigned)((P3DS_WaitTicks-P3DS_SpkWait0)/CPU_TICKS_PER_USEC/1000),D(4,10),D(10,11),D(11,12),D(12,13),P3DS_SerialSt?"(serial)":"",D(13,5));}
    int zc,zp,zl;unsigned zms;P3DS_ZoneInfo(&zc,&zp,&zl,&zms);char mf[200];P3DS_RomSpikeFiles(mf,sizeof mf);
    PSPNativeMemLog("[SPIKE] f=%u map %s <- %s (%s, +%u ms)%s",frames,P3DS_AreaName(zc),P3DS_AreaName(zp),zl<0?"no prefetch data":zl?"learned":"first visit",zms,mf);}
   else P3DS_RomSpikeReset();
   {/* per-frame deltas of the [PERF4] stage accumulators, printed on a spike line so one slow frame shows which stage ate it */
    static u64 pv[PF_N];u64 dl[PF_N];for(int i=0;i<PF_N;i++){dl[i]=pfT[i]>=pv[i]?pfT[i]-pv[i]:pfT[i];pv[i]=pfT[i];}
    if(spkPr)PSPNativeMemLog("[SPIKE] f=%u stages ms: composite L %u R %u CPU-A %u | strips prep %u overlays %u bottom menu %u GPU present %u | VBlank idle %u",frames,(unsigned)(dl[PF_GCL]/CPU_TICKS_PER_MSEC),(unsigned)(dl[PF_GCR]/CPU_TICKS_PER_MSEC),(unsigned)(dl[PF_CPUA]/CPU_TICKS_PER_MSEC),(unsigned)(dl[PF_WPREP]/CPU_TICKS_PER_MSEC),(unsigned)(dl[PF_OVL]/CPU_TICKS_PER_MSEC),(unsigned)(dl[PF_BMENU]/CPU_TICKS_PER_MSEC),(unsigned)(dl[PF_PRESENT]/CPU_TICKS_PER_MSEC),(unsigned)(dl[PF_IDLE]/CPU_TICKS_PER_MSEC));}
   P3DS_SpkRomTicks=P3DS_SpkTexTicks=P3DS_SpkGetTexTicks=0;P3DS_SpkRomBlocks=P3DS_SpkTexN=0;}
  static unsigned f60SecN;f60SecN+=f60Shown;   /* in-between pictures count as shown frames in the fps figure */
  static u64 lastIdle;
  const u64 idleNow=P3DS_VblIdleTicks,idleTicks=idleNow-lastIdle;
  lastIdle=idleNow;pfT[PF_IDLE]+=idleTicks;
  if(P3DS_Fps60Cfg()&&P3DS_InField&&!P3DS_InBattle&&total)
   P3DS_Fps60Tick(total,(unsigned)(idleTicks/CPU_TICKS_PER_USEC),presentFenceIdleUs,f60Shown,f60Why,f60CompUs,f60StripUs);
  if(P3DS_InField&&!P3DS_InBattle&&total)P3DS_Fps60Area(total,f60Shown);
  secUp+=P3DS_UploadUs;secConv+=G3G_ConvUs;
  P3DS_SpkWait0=P3DS_WaitTicks;P3DS_SerialSt=0;{static unsigned w3;if(total>w3)w3=total;P3DS_Perf3Worst=w3;if(secFrames==0)w3=total;}tPrevEnd=tEnd;secFrames++;secG3+=g3us;secComp+=comp;secGpu+=gpu;secGame+=total>comp+gpu+g3us?total-comp-gpu-g3us:0;secTris+=P3DS_G3LastTris();secG3G+=G3G_LastUs();
  if(!secStart)secStart=tEnd;unsigned el=(unsigned)((tEnd-secStart)/CPU_TICKS_PER_USEC);
  if(el>=1000000){unsigned n=secFrames;fpsShown=(unsigned)((u64)(n+f60SecN)*1000000/el);f60SecN=0;msShown=(unsigned)((u64)el/n/100);
   /* perf pass 2026-11-14: mallinfo() walks every heap chunk (it showed as ~3% of guest samples in the cloud profile, all of it in one frame per second);
      the [PERF] "malloc used" figure is refreshed every 20 s instead. */
   static struct mallinfo mi;static unsigned miAge=99;if(++miAge>=20){miAge=0;mi=mallinfo();}
   {/* run 116 diagnostic: widecomp stage split, logged every second while the Pokedex is open (or with Log detail FULL) */
    if((P3DS_DexOpen||P3DS_LogLevel()>=2)&&WC_ProfComposes){const unsigned c=WC_ProfComposes;
     PSPNativeMemLog("[WCPROF] dex %d: %u composes, %u lines/compose, sprites/compose %u | us/compose: sprites %u windows %u BGs %u backdrop+blend+out %u",P3DS_DexOpen,c,WC_ProfLines/c,WC_ProfSpr/c,(unsigned)(WC_Prof[0]/c/268),(unsigned)(WC_Prof[1]/c/268),(unsigned)(WC_Prof[2]/c/268),(unsigned)(WC_Prof[3]/c/268));}
    WC_Prof[0]=WC_Prof[1]=WC_Prof[2]=WC_Prof[3]=0;WC_ProfLines=WC_ProfSpr=WC_ProfComposes=0;}
   PSPNativeMemLogC("[PERF] fps %u | ms/frame %u.%u = game %u.%u + g3soft %u.%u + composite %u.%u + present %u.%u | tris/frame %u | malloc used %u KiB | mode %d slider %.2f | res3d %d, 3D GPU draw+readback %u us",
    fpsShown,msShown/10,msShown%10,secGame/n/1000,(secGame/n/100)%10,secG3/n/1000,(secG3/n/100)%10,secComp/n/1000,(secComp/n/100)%10,secGpu/n/1000,(secGpu/n/100)%10,secTris/n,(unsigned)(mi.uordblks/1024),P3DS_ScaleMode(),(double)p3dsSlider(),G3G_Hi()?2:0,secG3G/n);
   PSPNativeMemLogC("[PERF2] upload %u us | 3D convert %u us | composite %u us",secUp/n,secConv/n,secComp/n);secUp=secConv=0;
   {static unsigned lS,lD;int zc,zp,zl;unsigned zms;P3DS_ZoneInfo(&zc,&zp,&zl,&zms);PSPNativeMemLogC("[PERF3] worst frame %u ms | bottom reused %u drawn %u | map %d field %d scene %s stereo %d | zone %s <- %s",P3DS_Perf3Worst/1000,bSkips-lS,bDraws-lD,P3DS_CurMap,P3DS_InField,P3DS_InBattle?"battle":P3DS_InField?"field":"menu/other",(int)stereo,P3DS_AreaName(zc),P3DS_AreaName(zp));lS=bSkips;lD=bDraws;}
   {/* [PERF4] (2026-10-27): where the [PERF] bins go, per frame in us. g3soft = idle in the game's VBlank waits + real work
     (game logic, 3D transform/clip, 3D submit: see [G3PROF]); composite = GPU compose left/right eye (FrameBegin wait for
     earlier GPU work, FrameEnd submit, wait for compose+readback) + CPU engine A + wait for the engine B / right-eye worker;
     present = wide strips/overlays + Bottom menu + GPU present (FrameBegin wait, tearfix wait, FrameEnd). FrameEnd = all
     citro3d frame ends (with fps_tweaks 0 each one flushes the whole linear heap from the cache). */
    auto U=[&](u64 t){return (unsigned)(t/CPU_TICKS_PER_USEC/n);};const unsigned r=pfR?pfR:1;
    PSPNativeMemLogC("[PERF4] tweaks %d | g3soft %u = VBlank idle %u + work %u | composite %u = L %u (wait %u, end %u, sync %u) + R %u x%u (wait %u, end %u, sync %u) + CPU A %u + worker wait %u",
     P3DS_FpsTweaks(),secG3/n,U(pfT[PF_IDLE]),secG3/n>U(pfT[PF_IDLE])?secG3/n-U(pfT[PF_IDLE]):0,secComp/n,U(pfT[PF_GCL]),(unsigned)(pfU[PU_LB]/n),(unsigned)(pfU[PU_LE]/n),(unsigned)(pfU[PU_LS]/n),
     (unsigned)(pfT[PF_GCR]/CPU_TICKS_PER_USEC/r),pfR,(unsigned)(pfU[PU_RB]/r),(unsigned)(pfU[PU_RE]/r),(unsigned)(pfU[PU_RS]/r),U(pfT[PF_CPUA]),U(pfT[PF_BWAIT]+pfT[PF_C1WAIT]+pfT[PF_EARLYBWAIT]));
    PSPNativeMemLogC("[BPERF] us/frame waiting: core 2 frame job %u | core 1 Engine B %u | early Engine B %u | CPU Engine B scanlines %u draws, %u us/draw",
     U(pfT[PF_BWAIT]),U(pfT[PF_C1WAIT]),U(pfT[PF_EARLYBWAIT]),bScanFrames,
     bScanFrames?(unsigned)(bScanTicks/CPU_TICKS_PER_USEC/bScanFrames):0);
    bScanTicks=0;bScanFrames=0;
    PSPNativeMemLogC("[PERF4] present %u = strips+overlays %u + bottom menu %u (%u repaints) + GPU present %u (wait %u, tearfix %u, end %u) | FrameEnd x%u avg %u us | engine B hidden-skips %u | GPU fades %u | GPU 3D blend %u",
     secGpu/n,U(pfT[PF_OVL]),U(pfT[PF_BMENU]),pfBlRep,U(pfT[PF_PRESENT]),(unsigned)(pfU[PU_PB]/n),(unsigned)(pfU[PU_PT]/n),(unsigned)(pfU[PU_PE]/n),
     P3DS_FeN,P3DS_FeN?(unsigned)(P3DS_FeTicks/CPU_TICKS_PER_USEC/P3DS_FeN):0,pfBHid,P3DS_GcMbFrames,P3DS_GcB3Frames);
    PSPNativeMemLogC("[PERF5] tweaks %d | overlay stage %u = wide prep %u + overlay draws %u | wide DirectStrips (after the Bottom menu, inside present) %u | GPU present %u = upload %u + prescale/quads/overlay icons %u + wide strips %u",
     P3DS_FpsTweaks(),U(pfT[PF_OVL]),U(pfT[PF_WPREP]),U(pfT[PF_DRAWS]),U(pfT[PF_DSTRIP]),U(pfT[PF_PRESENT]),(unsigned)(pfU[PU_UP]/n),(unsigned)(pfU[PU_DR]/n),(unsigned)(pfU[PU_ST]/n));
    memset(pfT,0,sizeof pfT);memset(pfU,0,sizeof pfU);pfR=pfBlRep=pfBHid=0;P3DS_FeN=0;P3DS_FeTicks=0;P3DS_GcMbFrames=0;P3DS_GcB3Frames=0;}
   secStart=tEnd;secFrames=secG3=secComp=secGpu=secGame=secTris=0;secG3G=0;}}
 /* run 66 settings_touch=1 (default): touch and hold the top-left corner of the touch screen (DS 0..15 x 0..15, or the
    border left of / above the picture in 1:1 mode) for 0.3 s in the field = settings app (same as L+R+Y). The hold must
    start in the corner and stay there; anything else is a normal touch for the game. ss3ds: Platinum's other corners
    (Pokemon / Bag through its Start-menu hook) and the catch-info Y shortcut need Platinum game hooks and are gone. */
 static int stCfg=-1;if(stCfg<0){stCfg=P3DS_CfgInt("settings_touch",1)!=0;PSPNativeMemLog("[APP] settings_touch=%d",stCfg);}
 bool cornerOpen=false;
 if(stCfg&&P3DS_InField&&menuCfg()&&!P3DS_ConsoleOnBottom()&&!cy.open&&!cy.release){static u64 cT;static int cZone;static bool cPrev;u32 kh=hidKeysHeld();const bool tNow=(kh&KEY_TOUCH)!=0,tDown=tNow&&!cPrev;cPrev=tNow;
  if(tNow){touchPosition tp;hidTouchRead(&tp);int x,y,z=0;
   if(touchToDS(tp.px,tp.py,&x,&y)){if(x<16&&y<16)z=1;}else if(tp.px<160&&tp.py<120)z=1; /* 1:1 mode: the black border around the DS screen */
   if(tDown){cZone=z;cT=svcGetSystemTick();}else if(z!=cZone)cZone=0;
   if(cZone&&svcGetSystemTick()-cT>CPU_TICKS_PER_MSEC*300){cZone=0;cornerOpen=true;PSPNativeMemLog("[APP] opened by corner touch");}}
  else cZone=0;}
 if(__atomic_exchange_n(&onlineSettingsRequested,false,__ATOMIC_ACQ_REL)){if(P3DS_GtsPickerEnabled()&&!P3DS_ConsoleOnBottom())runOnlineSettings(stereo?eyeL:top,mode,mode&&gpuOk==1);return 0;} /* SoulSilver 0269 */
 /* SoulSilver battle Y companion + ZR hunt stats: both are overlays on the bottom picture (cyFrame in the present above), the game keeps
    running underneath. Y in battle (catch_overlay or the hunt counter on): wild catch info, trainer foe info, damage check (dmg_check).
    ZR in the field (hunt_stats, needs the hunt counter): hunt stats. ZR stays the item button when zr_item is on. */
 {u32 kd=hidKeysDown(),kh=hidKeysHeld();catchTick();
  static bool zrHeldPrev=true;const bool zrDown=(kh&KEY_ZR)&&!zrHeldPrev;zrHeldPrev=(kh&KEY_ZR)!=0; /* ZR edge from our own held history: hidKeysDown() can miss ZR (see psp_ctru.c) */
  cyTick(kh);
  if(cy.open)cyInput(kd,kh);
  else if((kd&KEY_Y)&&!cy.release&&!(kh&(KEY_L|KEY_R|KEY_X))&&(P3DS_CatchInfo[0]||P3DS_FoeInfo[0])&&!P3DS_InField&&!P3DS_SumOpen&&!P3DS_ConsoleOnBottom()&&(P3DS_CfgInt("catch_overlay",0)||P3DS_HuntOn()))cyOpen();
  else if(zrDown&&!cy.release&&!(kh&(KEY_L|KEY_R|KEY_X|KEY_Y))&&P3DS_InField&&!P3DS_MenuOpen&&!P3DS_BmBlock&&!P3DS_PanelPartyOn&&!P3DS_PanelPartyWant&&!P3DS_ConsoleOnBottom()&&P3DS_CtlZr<=0&&P3DS_CfgInt("hunt_stats",0)&&P3DS_HuntCounting())hsOpen();
  else if(zrDown&&P3DS_InField&&!cy.open){static unsigned zn;if(zn++<6)PSPNativeMemLog("[HSTAT] ZR not opened: release %d LRXY %d menu %d startmenu %d panelapp %d/%d console %d zr_item %d hunt_stats %d counting %d",(int)cy.release,(kh&(KEY_L|KEY_R|KEY_X|KEY_Y))!=0,(int)P3DS_MenuOpen,(int)P3DS_BmBlock,(int)P3DS_PanelPartyOn,(int)P3DS_PanelPartyWant,(int)P3DS_ConsoleOnBottom(),P3DS_CtlZr>0,P3DS_CfgInt("hunt_stats",0),P3DS_HuntCounting());}
  else if(cornerOpen||((kd&KEY_Y)&&(kh&KEY_L)&&(kh&KEY_R)&&!(kh&KEY_X)&&menuCfg())){if(appCfg()&&!P3DS_ConsoleOnBottom())runApp(stereo?eyeL:top,mode,mode&&gpuOk==1);else runMenu(stereo?eyeL:top,bot,mode,mode&&gpuOk==1,!P3DS_ConsoleOnBottom());}}
 return 0;
}
/* Soft-reset / clean exit: stop engine B + core-1 workers and join them before any GPU/heap free
   (run 934: crumbs showed w-job still running after main stopped → jump through freed state). */
extern "C" int P3DS_RenderShutdown(){
 initialized=false;
 if(!bQuit){
  bQuit=true;
  if(bMode==1){__atomic_add_fetch(&bPend,1,__ATOMIC_SEQ_CST);LightEvent_Signal(&bStart);LightEvent_Signal(&bDone);LightEvent_Signal(&bEarlyDone);}
  if(c1Mode==1)LightEvent_Signal(&c1Start);
 }
 bool stopped=true;
 if(bThread){if(threadJoin(bThread,500000000ULL)==0){threadFree(bThread);bThread=nullptr;bMode=0;}else{stopped=false;PSPNativeMemLog("[3DS] exit: engine B worker did not stop in 500 ms");}}
 if(c1Thread){if(threadJoin(c1Thread,500000000ULL)==0){threadFree(c1Thread);c1Thread=nullptr;c1Mode=0;}else{stopped=false;PSPNativeMemLog("[3DS] exit: c1 worker did not stop in 500 ms");}}
 if(stopped)PSPNativeMemLog("[3DS] exit: render workers stopped");
 return stopped;
}
extern "C" void PSPNativeRenderShutdown(){P3DS_RenderShutdown();}
extern "C" unsigned PSPNativeRenderTestPixel(unsigned e,unsigned x,unsigned y){allocBufs();return raw[e&1][(y%192)*256+(x%256)];}
extern "C" void PSPNativeRenderGetTimings(unsigned*r,unsigned*s){if(r)*r=lastReadbackUs;if(s)*s=last2DUs;}
extern "C" unsigned RenderStage(unsigned stage){return stage==0?bindUs:stage==1?drawUs:convertUs;}
extern "C" int PSPNativeRenderPresent(){return Present(true);}
extern "C" int PSPNativeRenderPresentNoWait(){return Present(false);}

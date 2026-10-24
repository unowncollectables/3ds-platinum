#include <time.h>
#include <algorithm>
#include <math.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "../native-render-opt/native_render.h"
#include "../native-render-opt/native_gpu.h"
#include <nitro.h>
#include <simulator/g3_handler.h>
#include <simulator/g3_draw.h>
#include <cstdio>
#include <malloc.h>
extern "C" void PSPNativeMemLog(const char*,...);
extern "C" int P3DS_CfgInt(const char*,int);
extern "C" unsigned PSPNativeVramPageGen[];
extern "C" { int P3DS_StripOn; }
static unsigned P3DS_Perf3Worst;extern "C" int P3DS_CurMap;

/* run 76 hunt_counter: wild encounters per species in sdmc:/3ds/platinum3ds/hunt.txt (never the save) */
static int huntN[494];static bool huntLoaded;
static void huntLoad(){if(huntLoaded)return;huntLoaded=true;FILE*f=fopen("sdmc:/3ds/platinum3ds/hunt.txt","r");if(!f)return;char l[64];int a,b;while(fgets(l,sizeof l,f))if(sscanf(l,"%d=%d",&a,&b)==2&&a>0&&a<494)huntN[a]=b;fclose(f);}
static int P3DS_HuntCount(int sp){huntLoad();return sp>0&&sp<494?huntN[sp]:0;}
extern "C" volatile int P3DS_CatchInfo[20];extern "C" int P3DS_InField;
static void huntTick(){static bool armed=true;
 if(P3DS_InField){if(!armed){armed=true;P3DS_CatchInfo[0]=0;}return;}
 if(!armed||!P3DS_CatchInfo[0])return;armed=false;if(!P3DS_CfgInt("hunt_counter",0))return;int sp=P3DS_CatchInfo[1];if(sp<=0||sp>=494)return;
 huntLoad();huntN[sp]++;FILE*f=fopen("sdmc:/3ds/platinum3ds/hunt.txt","w");if(f){for(int k=1;k<494;k++)if(huntN[k])fprintf(f,"%d=%d\n",k,huntN[k]);fclose(f);}
 PSPNativeMemLog("[HUNT] species %d met %d times",sp,huntN[sp]);}
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
static volatile bool bSkipB;static LightEvent c1Start,c1Done;static Thread c1Thread;static int c1Mode=-1;
/* run 40: bottom_skip=1 (default). Engine B (sub screen, never 3D) is recomposed only when one of its inputs changed:
   regs 0x1000-0x106F, sub palettes, sub OAM, sub BG/OBJ VRAM, sub ext palettes (pspoke's BottomSame/SaveBottom
   snapshot compare). The compare runs on whichever thread composes engine B (core-2 worker), off the main thread.
   Invalidated when POWCNT (enable / screen swap) changes. Saves ~9 ms of core-2 time in stereo, where the worker also
   composes the right eye. bottom_skip=0 = always recompose. */
static void drawEngineB();
static void c1Worker(void*){for(;;){LightEvent_Wait(&c1Start);if(bQuit)break;drawEngineB();LightEvent_Signal(&c1Done);}}
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
static void engineBWorker(void*){for(;;){while(!__atomic_load_n(&bPend,__ATOMIC_SEQ_CST)&&!bQuit)LightEvent_Wait(&bStart);if(bQuit)break;__atomic_sub_fetch(&bPend,1,__ATOMIC_SEQ_CST);P3DS_Crumb(bEarly?"w-early":"w-job",0);
  if(bEarly){drawEngineB();bEarly=false;P3DS_Crumb("w-early-sig",0);LightEvent_Signal(&bEarlyDone);continue;}
  const bool steal=stealCfg==1&&bRightSrc;
  if(!bSkipB&&!steal)drawEngineB();
  if(bRightSrc){/* stereo: right-eye top screen = engine A again (snapshot copy) with the right-eye 3D layer */
   P3DS_3DSrc=bRightSrc;rendererR.SetFramebuffer(rawR,rawR);
   for(unsigned line=0;line<192;line++){engineA2.CheckWindows(line);rendererR.DrawSprites(line,&engineA2);rendererR.DrawScanline(line,&engineA2);}
   P3DS_3DSrc=nullptr;}
  if(!bSkipB&&steal&&claimB()){stealWorker++;drawEngineB();}
  if(bSplit){for(unsigned line=0;line<96;line++)dryLine(engineAS,line);
   while(!line0Done)svcSleepThread(20000); /* main's line 0 made the shared flat VRAM views coherent */
   rendererS.SetFramebuffer(raw[0],raw[1]);
   for(unsigned line=96;line<192;line++){engineAS.CheckWindows(line);rendererS.DrawSprites(line,&engineAS);rendererS.DrawScanline(line,&engineAS);}
   bSplit=false;}
  P3DS_Crumb("w-done-sig",0);LightEvent_Signal(&bDone);}}
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
static unsigned reg32(unsigned off){return *(volatile u32*)(PSPNative_GfxRegisters+off);}
#include "../native-render-opt/bottom_cache.h"
extern "C" { volatile unsigned P3DS_SceneGen; int P3DS_LandFast; }
static int bSkipCfg=-1;static volatile unsigned bPower;static unsigned bLastPower=~0u;static unsigned bSkips,bDraws;volatile int P3DS_BottomReused;
static void drawEngineB(){
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
  /* run 76: Underground fix - bottom VRAM written through tracked copies (banks C/D/H/I) invalidates the reuse */
  {static unsigned lastG;unsigned g=0,gp=0;
   for(unsigned pg=0x40;pg<0x80;pg++)if(PSPNativeVramPageGen[pg]>g){g=PSPNativeVramPageGen[pg];gp=pg;}
   for(unsigned pg=0x98;pg<0xA4;pg++)if(PSPNativeVramPageGen[pg]>g){g=PSPNativeVramPageGen[pg];gp=pg;}
   if(g!=lastG){lastG=g;if(bottomCache.valid&&P3DS_CfgInt("bottom_vramgen",1)){bottomCache.valid=false;static int n;if(n<20){n++;PSPNativeMemLog("[2D] bottom changed by VRAM page %u",gp);}}}}
  if(bSkipCfg&&BottomSame(pw)){bSkips++;P3DS_BottomReused=1;return;}
  P3DS_BottomReused=0;bDraws++;
  rendererB.SetFramebuffer(raw[0],raw[1]);
  for(unsigned line=0;line<192;line++){engineB.CheckWindows(line);rendererB.DrawSprites(line,&engineB);rendererB.DrawScanline(line,&engineB);}
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
/* run 59 gpu_compose stage 0: ANALYZER ONLY (gpu_compose=1). Before writing a PICA 2D compositor, measure which DS 2D
   features real frames use, per engine, so the GPU path can start with the common case and fall back to melonDS
   for the rest. Register/OAM reads only (no pixel work); logs one [GPUC] line per engine every 300 frames.
   gpu_compose=0 (default): not even called. Nothing is drawn differently in either setting. */
static int gcCfgV=-1;
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
extern "C" {int P3DS_GcEligible(const u8*,const u16*);int P3DS_GcComposeA(const u8*,const u8*,const u8*,const u16*,const u16*,const u16*,const u32*,u32*);
 extern unsigned P3DS_GcWalkUs,P3DS_GcConvUs,P3DS_GcOutUs,P3DS_GcQuads,P3DS_GcDecoded,P3DS_GcResets,P3DS_GcCpuUs,P3DS_GcGpuUs;}
static unsigned gcFrames,gcUsed,gcInel,gcFail,gcVer,gcVerOk,gcVerBad;static u64 gcW,gcC,gcO,gcBadPx,gcCpu,gcGpu,gcDec,gcQ;static unsigned gcReason[16];
static u32*gcScratch;static unsigned gcMenuF,gcMenuOK,gcMenuW64[4],gcMenuOn[4];extern "C" int P3DS_InField;
static bool gcCompose(u32*out){const int r=P3DS_GcComposeA((const u8*)PSPNative_GfxRegisters,GPU::VRAMFlat_ABG,GPU::VRAMFlat_AOBJ,(const u16*)GPU::Palette,(const u16*)(GPU::Palette+512),(const u16*)GPU::OAM,GPU3D::NativeFrame,out);
 if(r){gcW+=P3DS_GcWalkUs;gcC+=P3DS_GcConvUs;gcO+=P3DS_GcOutUs;gcCpu+=P3DS_GcCpuUs;gcGpu+=P3DS_GcGpuUs;gcDec+=P3DS_GcDecoded;gcQ+=P3DS_GcQuads;}else gcFail++;return r!=0;}
static void gcLog(){if(gcFrames<300)return;const unsigned u=(gcUsed+gcVer)?(gcUsed+gcVer):1;
 PSPNativeMemLog("[GPUC] %u frames: GPU-composed %u, ineligible %u (reasons dm %u fb %u win %u 3dofs %u affbg %u bgmos %u extpal %u blend %u mbright %u cap %u objaff %u objmode %u objmos %u objext %u), failed %u | per composed frame: cpu %u us (walk %u, 3D tex %u, out %u), gpu+wait %u us, decoded %u tiles, %u quads, cache resets %u",
  gcFrames,gcUsed,gcInel,gcReason[0],gcReason[1],gcReason[2],gcReason[3],gcReason[4],gcReason[5],gcReason[6],gcReason[7],gcReason[8],gcReason[9],gcReason[10],gcReason[11],gcReason[12],gcReason[13],gcFail,(unsigned)(gcCpu/u),(unsigned)(gcW/u),(unsigned)(gcC/u),(unsigned)(gcO/u),(unsigned)(gcGpu/u),(unsigned)(gcDec/u),(unsigned)(gcQ/u),P3DS_GcResets);
 if(gcMenuF)PSPNativeMemLog("[GPUC] non-field frames %u, GPU-eligible %u | text BG on/64-wide: BG0 %u/%u BG1 %u/%u BG2 %u/%u BG3 %u/%u",gcMenuF,gcMenuOK,gcMenuOn[0],gcMenuW64[0],gcMenuOn[1],gcMenuW64[1],gcMenuOn[2],gcMenuW64[2],gcMenuOn[3],gcMenuW64[3]);
 if(gcVer)PSPNativeMemLog("[GPUC] verify: %u frames compared, %u identical, %u differ, %llu px differ in total",gcVer,gcVerOk,gcVerBad,(unsigned long long)gcBadPx);
 gcMenuF=gcMenuOK=0;memset(gcMenuW64,0,sizeof gcMenuW64);memset(gcMenuOn,0,sizeof gcMenuOn);gcFrames=gcUsed=gcInel=gcFail=gcVer=gcVerOk=gcVerBad=0;gcW=gcC=gcO=gcBadPx=gcCpu=gcGpu=gcDec=gcQ=0;memset(gcReason,0,sizeof gcReason);}
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
static void mapMemory(){
 memcpy(GPU::Palette,s_HW_BG_PLTT,512);memcpy(GPU::Palette+512,s_HW_OBJ_PLTT,512);
 memcpy(GPU::Palette+1024,s_HW_DB_BG_PLTT,512);memcpy(GPU::Palette+1536,s_HW_DB_OBJ_PLTT,512);
 memcpy(GPU::OAM,s_HW_OAM,1024);memcpy(GPU::OAM+1024,s_HW_DB_OAM,1024);
 GPU::VRAMMap_LCDC=0;for(unsigned i=0;i<4;i++){GPU::VRAM[i]=s_HW_LCDC_VRAM+i*0x20000;if((*(volatile u8*)(PSPNative_GfxRegisters+0x240+i)&0x87)==0x80)GPU::VRAMMap_LCDC|=1u<<i;}
 memset(GPU::VRAMFlat_ABGExtPal,0,32768);memset(GPU::VRAMFlat_BBGExtPal,0,32768);memset(GPU::VRAMFlat_AOBJExtPal,0,8192);memset(GPU::VRAMFlat_BOBJExtPal,0,8192);
 unsigned e=*(volatile u8*)(PSPNative_GfxRegisters+0x244);
 if((e&0x87)==0x84)memcpy(GPU::VRAMFlat_ABGExtPal,s_HW_LCDC_VRAM+0x80000,32768);
 for(unsigned i=0;i<2;i++){unsigned c=*(volatile u8*)(PSPNative_GfxRegisters+0x245+i);const u8*src=s_HW_LCDC_VRAM+0x90000+i*0x4000;
  if((c&0x87)==0x84)memcpy(GPU::VRAMFlat_ABGExtPal+((c&8)?16384:0),src,16384);
  if((c&0x87)==0x85)memcpy(GPU::VRAMFlat_AOBJExtPal,src,8192);
 }
 if((*(volatile u8*)(PSPNative_GfxRegisters+0x248)&0x87)==0x82)memcpy(GPU::VRAMFlat_BBGExtPal,s_HW_LCDC_VRAM+0x98000,32768);
 if((*(volatile u8*)(PSPNative_GfxRegisters+0x249)&0x87)==0x83)memcpy(GPU::VRAMFlat_BOBJExtPal,s_HW_LCDC_VRAM+0xa0000,8192);
}
#include "../native-render-opt/top_cache.h"
/* ---- platinum3ds present path: melonDS software 2D -> 3DS framebuffers (no GPU yet). ---- */
extern "C" int P3DS_ConsoleOnBottom(void);
extern "C" void P3DS_GameFramePresented(void);
extern "C" void PSPNativeRenderSetInput(unsigned keys,int touchMode,int touchDown,int tx,int ty){inputKeys=keys;mode=touchMode;down=touchDown;x=tx;y=ty;}
extern "C" unsigned PSPNativeRenderLastDrawMask(){return lastDraw;}
extern "C" unsigned PSPNativeRenderFrameCount(){return frames;}
#include <3ds.h>
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
static float p3dsSlider(){static int v=-1;if(v<0){v=P3DS_CfgInt("stereo_fake",0);if(v<0||v>100)v=0;if(v)PSPNativeMemLog("[STEREO] stereo_fake=%d",v);}return v?v/100.0f:osGet3DSliderState();}
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
extern "C" int P3DS_CurMap,P3DS_InField,P3DS_StereoShiftNow(void);
static void drawTodTint(u32*img){/* run 76 tod_tint: soft evening/night tint on the DS picture outdoors (off by default) */
 extern int P3DS_MapOutdoor;if(!P3DS_InField||!P3DS_MapOutdoor)return;if(!P3DS_CfgInt("tod_tint",0))return;
 static const int ch[5]={0,7,12,18,22};int ct=P3DS_CfgInt("cheat_time",0);int h;if(ct>0&&ct<5)h=ch[ct];else{time_t t=time(nullptr);struct tm*lt=localtime(&t);h=lt?lt->tm_hour:12;}
 u32 col;int a;if(h>=17&&h<20){col=0x003060E0u;a=2;}else if(h>=20||h<4){col=0x00801810u;a=3;}else if(h<7){col=0x00A07090u;a=1;}else return;
 const unsigned cr=(col&255)*a,cg=((col>>8)&255)*a,cb=((col>>16)&255)*a,ia=16-a;
 for(int i=0;i<256*192;i++){u32 d=img[i];img[i]=(d&0xFF000000u)|(((d&255)*ia+cr)>>4)|(((((d>>8)&255)*ia+cg)>>4)<<8)|(((((d>>16)&255)*ia+cb)>>4)<<16);}}
static void drawDwDebris(u32*img,int eye){
 if(!P3DS_InField||P3DS_CurMap<573||P3DS_CurMap>583)return;if(!P3DS_CfgInt("dw_debris",0))return;
 static float px[24],py[24];static bool init;if(!init){init=true;u32 r=0x5EED1234u;for(int i=0;i<24;i++){r=r*1664525u+1013904223u;px[i]=(float)(r>>8&255);r=r*1664525u+1013904223u;py[i]=(float)((r>>8)%192);}}
 static unsigned lastF=~0u;if(eye==0&&lastF!=frames){lastF=frames;for(int i=0;i<24;i++){const int L=i<18?i%3:3;const float sp=0.05f+0.06f*L;px[i]+=sp*((i&1)?1.0f:-0.6f);py[i]-=sp*0.7f;
   if(px[i]<-4)px[i]+=264;if(px[i]>260)px[i]-=264;if(py[i]<-4)py[i]+=200;}}
 const int sd=P3DS_StereoShiftNow();
 for(int i=0;i<24;i++){const int L=i<18?i%3:3;const int sz=L==3?3:1+L/2;const int par=eye?-(sd*(L-1))/2:(sd*(L-1))/2;
  const int x0=(int)px[i]+par,y0=(int)py[i];const u32 col=L==3?0x00C0B0B8u:(i%5==0?0x00603850u:0x00403038u);const int a=L==3?5:8;/* alpha /16 */
  for(int y=y0;y<y0+sz;y++)for(int x=x0;x<x0+sz+(L==0);x++){if(x<0||x>255||y<0||y>191)continue;u32&d=img[y*256+x];
   unsigned r=((d&255)*(16-a)+(col&255)*a)/16,g=(((d>>8)&255)*(16-a)+((col>>8)&255)*a)/16,b=(((d>>16)&255)*(16-a)+((col>>16)&255)*a)/16;d=(d&0xFF000000u)|r|(g<<8)|(b<<16);}}}
static void drawToast(u32*img,u64 now){if(!toastAt||now-toastAt>(u64)(1500*CPU_TICKS_PER_MSEC))return;
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
/* run 64 summary_ivs=1 (default): on the summary's Skills page, Y toggles a read-only IV / EV box over the ability
   text (order HP/Atk/Def/SpA/SpD/Spe). Values copied by the summary screen; nothing is written to the Pokemon. */
extern "C" volatile int P3DS_SumIV[6],P3DS_SumEV[6],P3DS_SumAlive;static int sumShow;
static int sumIvCfg(void);static void drawSumIV(u32*img){int cfg=sumIvCfg();
 static int alive;if(img){if(P3DS_SumAlive>0){alive=P3DS_SumAlive--;}else alive=0;}
 {static int la;if((alive>0)!=la){la=alive>0;PSPNativeMemLog("[UI] summary skills page %d",la);}}
 if(!cfg||!alive){return;}
 {static u32 prevK;u32 k=hidKeysHeld();if((k&KEY_Y)&&!(prevK&KEY_Y)&&!(k&(KEY_L|KEY_R))){sumShow^=1;PSPNativeMemLog("[UI] summary IV/EV box %d",sumShow);}prevK=k;}
 if(!sumShow||!P3DS_GameBoxReady())return;
 char a[64],b[64];snprintf(a,sizeof a,"IV %d/%d/%d/%d/%d/%d",P3DS_SumIV[0],P3DS_SumIV[1],P3DS_SumIV[2],P3DS_SumIV[3],P3DS_SumIV[4],P3DS_SumIV[5]);
 snprintf(b,sizeof b,"EV %d/%d/%d/%d/%d/%d",P3DS_SumEV[0],P3DS_SumEV[1],P3DS_SumEV[2],P3DS_SumEV[3],P3DS_SumEV[4],P3DS_SumEV[5]);
 P3DS_GameBoxDraw(img,256,256,192,16,152,216,32);P3DS_GameFontDraw(img,256,256,192,18,152,a,kMsgFg,kMsgSh,1,1);P3DS_GameFontDraw(img,256,256,192,18,168,b,kMsgFg,kMsgSh,1,1);}
extern "C" volatile int P3DS_PromptOpen,P3DS_PromptSel;extern "C" char P3DS_PromptText[64];
static void drawPrompt(u32*img){if(P3DS_PromptOpen!=1||!P3DS_GameBoxReady())return; /* run 64 repel_prompt box */
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
static void drawFolEmote(u32*img,u64 now){if(emIdx<0||emIdx>=emN||!emAt)return;const u64 el=(now-emAt)/CPU_TICKS_PER_MSEC;if(el>(u64)emMs||!P3DS_InField){emAt=0;return;}
 const u32*t=emTex[emIdx];int sc=el<120?(int)(el*16/120)+1:16;if(sc>16)sc=16;const int bob=el>=120&&((el/250)&1)?1:0;
 const int w=32*sc/16,x0=emX-w/2,y0=emY-w+10-bob;
 for(int y=0;y<w;y++){const int yy=y0+y;if(yy<0||yy>=192)continue;const u32*row=t+(y*16/sc)*32;
  for(int x=0;x<w;x++){const int xx=x0+x;if(xx<0||xx>=256)continue;const u32 c=row[x*16/sc];if(c)img[yy*256+xx]=c;}}}
static void drawFolTalk(u32*img,u64 now){if(!folAt||!folText[0]||now-folAt>(u64)folMs*CPU_TICKS_PER_MSEC)return;
 if(!P3DS_InField){folAt=0;return;}
 if(useGF()){char ln[2][64];int n=gfWrap(folText,208,ln,2);/* the game's field message window: tiles (2,19) 27x4 -> interior 16,152 216x32 */
  P3DS_GameBoxDraw(img,256,256,192,16,152,216,32);for(int i=0;i<n;i++)P3DS_GameFontDraw(img,256,256,192,18,152+i*16,ln[i],kMsgFg,kMsgSh,1,1);return;}
 char up[100];toUpper(up,folText,sizeof up);const char*folTextU=up;
 char l1[32],l2[32];l1[0]=l2[0]=0;{const int W=27;const char*t=folTextU;int n=(int)strlen(t),cut=n;if(n>W){cut=W;while(cut>0&&t[cut]!=' ')cut--;if(!cut)cut=W;}
  snprintf(l1,sizeof l1,"%.*s",cut,t);const char*r=t+cut;while(*r==' ')r++;snprintf(l2,sizeof l2,"%.27s",r);}
 const int bx=8,by=146,bw=240,bh=42;
 for(int y=by;y<by+bh;y++)for(int x=bx;x<bx+bw;x++){bool e=y<by+2||y>=by+bh-2||x<bx+2||x>=bx+bw-2;bool e2=!e&&(y<by+3||y>=by+bh-3||x<bx+3||x>=bx+bw-3);img[y*256+x]=e?0x00483828u:e2?0x00B0A8A0u:0x00F8F8F8u;}
 drawBig(img,bx+10,by+8,l1,0x00403830u);if(l2[0])drawBig(img,bx+10,by+24,l2,0x00403830u);}
/* run 64 credits_splash=1 (default): boot credits drawn by the port over the first game frames (the game is still
   loading / black then, so no wait is added): "Patch by" / "Unown Collectables & 3DS Homebrew Dev" in the game's own
   font (read from the user's ROM; port font fallback), splash_ms (default 2000) incl. fades, A skips. ROM untouched. */
static int splashCfg=-1,splashMs=2000;static u64 splashT0,splashEnd;static bool splashDone;
static u32 scaleCol(u32 c,int a){return ((((c&255)*a)>>8)&255)|(((((c>>8)&255)*a)>>8)<<8)|(((((c>>16)&255)*a)>>8)<<16);}
static bool splashActive(u64 now){if(splashDone)return false;
 if(splashCfg<0){splashCfg=P3DS_CfgInt("credits_splash",1)!=0;splashMs=P3DS_CfgInt("splash_ms",2000);if(splashMs<300)splashMs=300;if(splashMs>5000)splashMs=5000;PSPNativeMemLog("[SPLASH] credits_splash=%d splash_ms=%d font=%d",splashCfg,splashMs,P3DS_GameFontReady());}
 if(!splashCfg){splashDone=true;return false;}
 const u64 ms=CPU_TICKS_PER_MSEC;if(!splashT0){splashT0=now;splashEnd=now+(u64)splashMs*ms;}
 if(!(splashEnd-now<(u64)250*ms)&&(hidKeysHeld()&KEY_A)){splashEnd=now+(u64)250*ms;PSPNativeMemLog("[SPLASH] skipped (A)");}
 if(now>=splashEnd){splashDone=true;PSPNativeMemLog("[SPLASH] done after %u ms",(unsigned)((now-splashT0)/ms));return false;}return true;}
static void drawSplash(u32*img,u64 now){const u64 ms=CPU_TICKS_PER_MSEC;int in=(int)((now-splashT0)/ms),out=(int)((splashEnd-now)/ms);
 int a=256;if(in<300)a=in*256/300;if(out<250&&out*256/250<a)a=out*256/250;if(a<0)a=0;
 for(int i=0;i<256*192;i++)img[i]=0;
 const char*l1="Patch by",*l2="Unown Collectables & 3DS Homebrew Dev";
 if(P3DS_GameFontReady()){int w1=P3DS_GameFontWidth(1,l1),w2=P3DS_GameFontWidth(1,l2);
  P3DS_GameFontDraw(img,256,256,192,128-w1/2,70,l1,scaleCol(0x00B8B0A8u,a),0,1,1);
  for(int x=128-w2/2;x<128+w2/2;x++)img[90*256+x]=scaleCol(0x00585048u,a);
  P3DS_GameFontDraw(img,256,256,192,128-w2/2,96,l2,scaleCol(0x00F8F8F8u,a),scaleCol(0x00505050u,a),1,1);}
 else{drawBig(img,128-4*8/2*2,74,"PATCH BY",scaleCol(0x00B8B0A8u,a));drawBig(img,128-18*4,96,"UNOWN COLLECTABLES",scaleCol(0x00F8F8F8u,a));drawBig(img,128-16*4,112,"3DS HOMEBREW DEV",scaleCol(0x00F8F8F8u,a));}}
static void drawSaveBox(u32*img,u64 now){
 /* run 50 fail-safe: the takeover box only while it is clearly the field save sequence. Leaving the overworld, or 20 s
    with no yes/no menu frame and no save write (a script path that never closes the save info), drops it so the
    game's own windows are visible again. */
 if(saveSeq==1){u64 last=seqAct>saveLast?seqAct:saveLast;
  if(!P3DS_InField||now-last>(u64)(20000*CPU_TICKS_PER_MSEC)){saveSeq=0;saveYN=-1;PSPNativeMemLog("[SAVE] seq fail-safe: box dropped (field %d)",P3DS_InField);}}
 const bool seq=saveSeq==1;
 if(saveAnim<=0||(!saveFirst&&!seq))return;
 const u64 ms=CPU_TICKS_PER_MSEC;u64 endSaving=saveFirst+(u64)saveMinMs*ms;if(saveLast+300*ms>endSaving)endSaving=saveLast+300*ms;
 if(saveFirst&&!seq&&now>endSaving+800*ms){saveFirst=0;return;}
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
 if(++n%600==0){PSPNativeMemLog("[WIDE] wide_haze: %u of 600 strip frames hazed, avg %u us | BLDCNT %04x BLDALPHA %04x",on,on?usAcc/on:0,lastBc,lastBa);on=0;usAcc=0;}
 lastBc=bc;lastBa=ba;
 if(((bc>>6)&3)!=1||!(bc&0x100)||!(bc&0x0E))return;
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
extern "C" { u32 P3DS_WideFillBuf[64*192]; int P3DS_CfgInt(const char*,int); }
static int wideFillCfg(){static int v=-1;if(v<0){v=P3DS_CfgInt("wide_fill",0)!=0;PSPNativeMemLog("[WIDE] wide_fill=%d",v);}return v;}
static void wideFill(const u32*C){static u16 acc[3][64*192];
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
/* run 65 wide_menus=1 (default; only with wideview on, 1.25x mode, engine A on top, frames WITHOUT wide 3D strips:
   menus, battles, title): the side areas show what the DS 2D engine itself would draw at DS x -32..-1 / 256..287 -
   a small CPU BG renderer reads the same VRAM / palettes / scroll registers. Per BG layer: 64-tile-wide maps (real
   content past the edge) are drawn as is; 32-wide maps only when the strip would show something that really
   continues: the map is horizontally periodic (tiled pattern backgrounds) or the wrapped-in columns are empty.
   Other layers are left out of the strips (a panel touching the edge just ends there). Nothing visible -> backdrop.
   No mirroring, no blur, no stretching; fades come from gpuscale strips() (wide_fadefix). OBJs are not drawn in the
   strips. 0 = old behaviour (black bars, or wide_fill=1). */
static int wideMenusV=-1;extern "C" int P3DS_WideMenusCfg(void){if(wideMenusV<0){wideMenusV=P3DS_CfgInt("wide_menus",1)!=0;PSPNativeMemLog("[WIDE] wide_menus=%d",wideMenusV);}return wideMenusV;}
static unsigned wnUs,wnN,wnLay[5];
static void wideNative(){const u64 t0=svcGetSystemTick();
 const u8*R=(const u8*)PSPNative_GfxRegisters;const u32 dc=*(const u32*)R;const unsigned mode=dc&7;
 const u8*vram=GPU::VRAMFlat_ABG;const u16*pal=(const u16*)GPU::Palette;const u8*ext=GPU::VRAMFlat_ABGExtPal;
 struct L{int on,prio,bpp8,w,h,extSlot;unsigned charb,mapb,hofs,vofs;u8 per[64][2];u8 src[64][2];};static L Ly[4];int order[4],no=0;
 auto ent=[&](const L&l,unsigned tx,unsigned ty)->u16{tx&=l.w-1;ty&=l.h-1;const unsigned ma=(l.mapb+((tx>>5)+(ty>>5)*(l.w>>5))*0x800+((ty&31)*32+(tx&31))*2)&0x7FFFF;return *(const u16*)(vram+ma);};
 for(int i=0;i<4;i++){L&l=Ly[i];l.on=0;if(!(dc&(0x100u<<i)))continue;if(i==0&&(dc&8))continue;
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
   if(!any)continue;}
  l.on=1;}
 for(int pr=0;pr<4;pr++)for(int i=0;i<4;i++)if(Ly[i].on&&Ly[i].prio==pr)order[no++]=i;
 /* run 73: only the back-most layer (the screen's background) continues into the strips; 32-wide front layers
    (panels, number rows, list boxes) touching the edge were repeated as rows of '0's / cut-off panels. 64-wide
    layers have real content there and stay. */
 {int k=0;for(int oi=0;oi<no;oi++){const L&l=Ly[order[oi]];if(oi==no-1||l.w==64)order[k++]=order[oi];}no=k;}
 wnLay[no]++;
 /* the back-most layer (the screen's background) also fills rows where a panel touches the edge: those rows borrow
    the nearest row whose edge is periodic, so the background continues behind the panels instead of a black gap */
 if(no&&Ly[order[no-1]].w==32){L&b=Ly[order[no-1]];for(int side=0;side<2;side++)for(int r=0;r<25;r++)if(!b.per[r][side]){
   for(int d=1;d<25;d++){int a=r-d,c2=r+d;if(a>=0&&b.per[a][side]&&b.src[a][side]==a){b.per[r][side]=b.per[a][side];b.src[r][side]=(u8)a;break;}
    if(c2<25&&b.per[c2][side]&&b.src[c2][side]==c2){b.per[r][side]=b.per[c2][side];b.src[r][side]=(u8)c2;break;}}}}
 /* skip the redraw when nothing the strips use changed: registers, palette, the edge map entries and their tiles */
 {u32 h=2166136261u;auto H=[&](u32 v){h=(h^v)*16777619u;};H(dc);for(int k=0;k<0x20;k+=4)H(*(const u32*)(R+8+k));H(*(const u32*)(R+0x50));H(*(const u32*)(R+0x54));
  {const u32*pp32=(const u32*)pal;for(int k=0;k<128;k++)H(pp32[k]);}
  for(int oi=0;oi<no;oi++){const L&l=Ly[order[oi]];if(l.extSlot>=0){const u32*e=(const u32*)(ext+l.extSlot*0x2000);for(int k=0;k<2048;k++)H(e[k]);}
   const unsigned E0=l.hofs>>3,E1=(l.hofs+255)>>3;
   for(unsigned r=0;r<25;r++)for(int side=0;side<2;side++){int pp=l.w==32?l.per[r][side]:5;if(!pp)continue;H(pp|(l.w==32?l.src[r][side]<<8:0));
    const unsigned ty=(l.vofs>>3)+(l.w==32?l.src[r][side]:r);
    for(int k=0;k<pp;k++){unsigned mc=l.w==32?(side?E1-k:E0+k):(side?E1+1+k:E0-1-k);u16 me=ent(l,mc,ty);H(me);
     const u32*t=(const u32*)(vram+((l.charb+(me&0x3FF)*(l.bpp8?64:32))&0x7FFFF));for(int q=0;q<(l.bpp8?16:8);q++)H(t[q]);}}}
  static u32 lastH;static unsigned skips;if(h==lastH){skips++;wnUs+=(unsigned)((svcGetSystemTick()-t0)/CPU_TICKS_PER_USEC);if(++wnN==600){PSPNativeMemLog("[WIDE] wide_menus avg %u us/frame (%u of 600 reused)",wnUs/600,skips);wnUs=wnN=skips=0;}return;}lastH=h;}
 static u32 lutBg[256];for(int k=0;k<256;k++){u16 cc=pal[k];u32 r=cc&31,g=(cc>>5)&31,b=(cc>>10)&31;lutBg[k]=(((r<<3)|(r>>2))<<24)|(((g<<3)|(g>>2))<<16)|(((b<<3)|(b>>2))<<8)|0xFF;}
 const u32 bd=lutBg[0];
 /* BLDCNT brighten / darken (effect 2/3, BLDY) on the 1st-target layers / backdrop, like the centre. A fade that targets
    BG0 is applied to the whole strip by gpuscale strips() instead (wide_fadefix), so it is not done twice. */
 const u16 bc=*(const u16*)(R+0x50);unsigned bY=*(const u16*)(R+0x54)&31;if(bY>16)bY=16;const int bEff=(!(bc&1)&&((bc>>6)&3)>=2&&bY)?(int)((bc>>6)&3):0;
 for(int y=0;y<192;y++)for(int side=0;side<2;side++){u32*o=P3DS_WideFillBuf+y*64+side*32;u32 done=0;const int x0=side?256:-32;u8 lay[32];
  for(int oi=0;oi<no&&done!=0xFFFFFFFFu;oi++){const L&l=Ly[order[oi]];const unsigned py=(unsigned)(y+l.vofs)&(l.h*8-1);const unsigned wm=l.w*8-1;
   int pp=0;const int E0=(int)(l.hofs>>3),E1=(int)((l.hofs+255)>>3);
   unsigned pyU=py;if(l.w==32){const unsigned r=((unsigned)(y+(l.vofs&7)))>>3;pp=r<25?l.per[r][side]:0;if(!pp)continue;pyU=((((l.vofs>>3)+l.src[r][side])*8)+(py&7))&(l.h*8-1);}
   u16 me=0;const u8*trow=0;int hf=0;int lastT=0x7FFFFFFF;
   for(int xi=0;xi<32;xi++){if(done&(1u<<xi))continue;const int lx=x0+xi+(int)l.hofs;const int t=lx>>3;
    if(t!=lastT){lastT=t;unsigned mc;
     if(!pp)mc=(unsigned)t;else if(side==0){int d=(t-E0)%pp;if(d<0)d+=pp;mc=(unsigned)(E0+d);}else{const int b0=E1-pp+1;int d=(t-b0)%pp;if(d<0)d+=pp;mc=(unsigned)(b0+d);}
     me=ent(l,mc,pyU>>3);unsigned fy=pyU&7;if(me&0x800)fy=7-fy;hf=(me&0x400)!=0;
     trow=vram+((l.charb+(me&0x3FF)*(l.bpp8?64:32)+fy*(l.bpp8?8:4))&0x7FFFF);}
    unsigned fx=(unsigned)lx&7;if(hf)fx=7-fx;unsigned ci;if(l.bpp8)ci=trow[fx];else{u8 b=trow[fx>>1];ci=(fx&1)?b>>4:b&15;}
    (void)wm;if(!ci)continue;
    if(l.extSlot>=0){u16 cc=*(const u16*)(ext+l.extSlot*0x2000+((me>>12)*256+ci)*2);u32 r=cc&31,g=(cc>>5)&31,b=(cc>>10)&31;o[xi]=(((r<<3)|(r>>2))<<24)|(((g<<3)|(g>>2))<<16)|(((b<<3)|(b>>2))<<8)|0xFF;}
    else o[xi]=lutBg[l.bpp8?ci:((me>>12)*16+ci)];
    done|=1u<<xi;lay[xi]=(u8)order[oi];}}
  for(int xi=0;xi<32;xi++)if(!(done&(1u<<xi))){o[xi]=bd;lay[xi]=5;}
  if(bEff){for(int xi=0;xi<32;xi++)if(bc&(1u<<lay[xi])){u32 c=o[xi];unsigned r=c>>24,g=(c>>16)&255,b=(c>>8)&255;
    if(bEff==2){r+=(255-r)*bY/16;g+=(255-g)*bY/16;b+=(255-b)*bY/16;}else{r-=r*bY/16;g-=g*bY/16;b-=b*bY/16;}o[xi]=(r<<24)|(g<<16)|(b<<8)|0xFF;}}}
 wnUs+=(unsigned)((svcGetSystemTick()-t0)/CPU_TICKS_PER_USEC);if(++wnN==600){PSPNativeMemLog("[WIDE] wide_menus avg %u us/frame | layers in strips 0:%u 1:%u 2:%u 3:%u 4:%u",wnUs/600,wnLay[0],wnLay[1],wnLay[2],wnLay[3],wnLay[4]);wnUs=wnN=0;memset(wnLay,0,sizeof wnLay);}
}
/* run 57 stereo_strength (menu "3D STRENGTH"): 0 = 1x (original, 6 DS px per eye at full slider), 1 = 1.5x, 2 = 2x.
   Scales the per-vertex parallax of the whole 3D world (overworld billboards/NPCs are 3D, so they follow) and the
   stereo_2d battle BG offsets. Zero cost (one multiply per frame). */
static int stereoStr=-1;
/* run 61 stereo_2d: 1 = run-55 battle BG depth (prio 3 = 2x shift, prio 2 = 1.5x: up to 12 DS px between the eyes at
   full slider, far more than the battle Pokemon's own 3D disparity -> the Pokemon looked doubled against its
   background/platform); 2 (default) = gentle (prio 3 = 0.5x, prio 2 = 0.25x); 0 = flat 2D. */
static int stereo2dV=-1;static int stereo2dCfg(){if(stereo2dV<0){stereo2dV=P3DS_CfgInt("stereo_2d",2);if(stereo2dV<0||stereo2dV>2)stereo2dV=2;PSPNativeMemLog("[STEREO] stereo_2d=%d (0 flat, 1 strong/run55, 2 gentle)",stereo2dV);}return stereo2dV;}
/* run 61 gpu_compose_stereo=1 (default; only with gpu_compose=2): with the 3D slider up, overworld frames that are
   GPU-eligible are composed on the GPU for BOTH eyes (left with the left 3D layer, right with the right one; 2D layers
   identical, as on the CPU path in the field). Before, stereo always took the CPU path (~17 ms -> 25 fps). */
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
static void drawDwRoll(u32*img){const float e=dwRollEnv();if(e>-0.01f&&e<0.01f)return;const float k=0.07f*e*dwRollDir;u32 row[256];
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
 if(initialized)return 0;frames=0;bottomCache.valid=false;topCache.valid=false;previousPower=~0u;grace=0;engineA.Reset();engineB.Reset();
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
/* run 64 reusable_tms=0 (default off): 1 = teaching a TM does not use it up (party_menu callbacks.c TeachMove). */
static int reuseTmV=-1;extern "C" int P3DS_ReusableTMsCfg(void){if(reuseTmV<0){reuseTmV=P3DS_CfgInt("reusable_tms",0)!=0;PSPNativeMemLog("[ITEM] reusable_tms=%d",reuseTmV);}return reuseTmV;}
/* run 64 Cheats tab: cheat_no_encounters=0 (default off): walking in grass / caves / surfing never starts a random
   wild battle (overlay006 wild_encounters.c returns "no encounter" before rolling). Nothing is written to the save. */
static int noEncV=-1;extern "C" int P3DS_CheatNoEncCfg(void){if(noEncV<0){noEncV=P3DS_CfgInt("cheat_no_encounters",0)!=0;PSPNativeMemLog("[CHEAT] cheat_no_encounters=%d",noEncV);}return noEncV;}
static int repPV=-1;extern "C" int P3DS_RepelPromptCfg(void){if(repPV<0)repPV=P3DS_CfgInt("repel_prompt",1)!=0;return repPV;}
static int sumIvV=-1;static int sumIvCfg(void){if(sumIvV<0)sumIvV=P3DS_CfgInt("summary_ivs",1)!=0;return sumIvV;}
static int fastTextV=-1;extern "C" int P3DS_FastTextCfg(void){if(fastTextV<0){fastTextV=P3DS_CfgInt("fast_text",0);if(fastTextV<2)fastTextV=0;if(fastTextV>4)fastTextV=4;PSPNativeMemLog("[TEXT] fast_text=%d",fastTextV);}return fastTextV;}
struct MItem{const char*name;const char*key;int n;const char*lab[7];};
static const MItem mItems[]={
 {"FPS COUNTER","fps_counter",2,{"OFF","ON"}},{"WIDE VIEW","wideview",2,{"OFF","ON"}},{"SCALE","gpuscale",3,{"1:1","1.25X","FILL"}},
 {"C-STICK","cstick_mode",3,{"OFF","POKETCH","DPAD"}},{"ZL RUN TOGGLE","zl_run",2,{"OFF","ON"}},{"ZR = ITEM (Y)","zr_item",2,{"OFF","ON"}},
 {"CIRCLE PAD DPAD","cpad_dpad",2,{"OFF","ON"}},{"WIDE VIEW FX","wide_fx",2,{"OFF","ON"}},{"LOG DETAIL","log_level",2,{"SHORT","FULL"}},
 {"CAMERA ZOOM","cam_zoom",5,{"-2","-1","0","+1","+2"}},
 {"3D STRENGTH","stereo_strength",3,{"1X","1.5X","2X"}},
 {"FAST UPLOAD","gpu_swizzle",2,{"OFF","ON"}},{"FAST 3D CONVERT","fast_conv",2,{"OFF","ON"}},{"SPLIT TOP (MONO)","split_top",2,{"OFF","ON"}},
 {"FOLLOWING POKEMON","follower",2,{"OFF","ON"}},
 {"GPU COMPOSE","gpu_compose",4,{"OFF","ANALYZE","GPU","VERIFY"}},
 {"FOLLOWER SLOT","follower_slot",7,{"LEAD","1","2","3","4","5","6"}},{"FOLLOWER TALK","follower_talk",2,{"OFF","ON"}},
 {"MENU MUSIC","menu_audio",2,{"OFF","ON"}},{"GPU COMPOSE 3D","gpu_compose_stereo",2,{"OFF","ON"}},
 {"BATTLE 3D DEPTH","stereo_2d",3,{"FLAT","STRONG","GENTLE"}},{"WIDE HAZE","wide_haze",2,{"OFF","ON"}},
 {"FAST TEXT","fast_text",4,{"OFF","2X","3X","4X"}},
 {"REUSABLE TMS","reusable_tms",2,{"OFF","ON"}},
 {"NO WILD ENCOUNTERS","cheat_no_encounters",2,{"OFF","ON"}},{"REPEL PROMPT","repel_prompt",2,{"OFF","ON"}},{"IV/EV ON SUMMARY","summary_ivs",2,{"OFF","ON"}},
 /* run 67 cheats 27.. (P3DS_Cheat(i-27)), all default OFF */
 {"INFINITE MONEY","cheat_money",2,{"OFF","ON"}},{"ALWAYS CATCH","cheat_catch",2,{"OFF","ON"}},{"SHINY WILD","cheat_shiny",2,{"OFF","ON"}},
 {"FAST EGGS","cheat_eggs",2,{"OFF","ON"}},{"EXP BOOST","cheat_exp",3,{"OFF","2X","3X"}},{"RARE CANDY 99","cheat_candy",2,{"OFF","ON"}},
 /* run 74: 33.. generic config rows (value = label index; log_max_kb mapped). Most apply after a restart. */
 {"FOLLOWER_AREA_TALK","follower_area_talk",2,{"OFF","ON"}},{"FOLLOWER_CRY","follower_cry",2,{"OFF","ON"}},{"FOLLOWER_ITEMS","follower_items",2,{"OFF","ON"}},{"FOLLOWER_BALLPOP","follower_ballpop",2,{"OFF","ON"}},{"FOLLOWER_EMOTES","follower_emotes",2,{"OFF","ON"}},{"FOLLOWER_FRIEND_TALK","follower_friend_talk",2,{"OFF","ON"}},{"FOLLOWER_IDLE","follower_idle",2,{"OFF","ON"}},{"FOLLOWER_GAP","follower_gap",2,{"OFF","ON"}},{"FOLLOWER_BEHIND","follower_behind",2,{"OFF","ON"}},{"WIDE_WEATHER","wide_weather",2,{"OFF","ON"}},{"WIDE_EDGEFILL","wide_edgefill",2,{"OFF","ON"}},{"WIDE_MENUS","wide_menus",2,{"OFF","ON"}},{"WIDE_FILL","wide_fill",2,{"OFF","ON"}},{"WILD_VIS","wild_vis",2,{"OFF","ON"}},{"WILD_VIS_MAX","wild_vis_max",5,{"0","1","2","3","4"}},{"WILD_VIS_REPLACE","wild_vis_replace",2,{"OFF","ON"}},{"WILD_VIS_TEST","wild_vis_test",2,{"OFF","ON"}},{"HM_FREE","hm_free",2,{"OFF","ON"}},{"SAVE_BACKUP","save_backup",2,{"OFF","ON"}},{"LOG_MAX_KB","log_max_kb",4,{"OFF","512 KB","1 MB","2 MB"}},{"CHEAT_PP","cheat_pp",2,{"OFF","ON"}},{"CHEAT_SPEED","cheat_speed",2,{"OFF","ON"}},{"CHEAT_WALK_WALLS","cheat_walk_walls",2,{"OFF","ON"}},{"CHEAT_TIME","cheat_time",5,{"OFF","MORNING","DAY","EVENING","NIGHT"}},{"CATCH_OVERLAY","catch_overlay",2,{"OFF","ON"}},{"BATTLE_ANIM_SPEED","battle_anim_speed",3,{"OFF","SKIP MOVES","SKIP + FAST HP"}},{"DW_DEPTH","dw_depth",4,{"OFF","SUBTLE","MODERATE","STRONG"}},{"DW_DEBRIS","dw_debris",2,{"OFF","ON"}},{"DW_UNEASE","dw_unease",2,{"OFF","ON"}},{"DW_CRY","dw_cry",2,{"OFF","ON"}},{"HUNT_COUNTER","hunt_counter",2,{"OFF","ON"}},{"TOD_TINT","tod_tint",2,{"OFF","ON"}},{"ITEM_PING","item_ping",2,{"OFF","ON"}},{"TOUCH_MENU","touch_menu",2,{"OFF","ON"}},{"DW_ROLL","dw_roll",2,{"OFF","ON"}},{"BOTTOM_STRIP","bottom_strip",2,{"OFF","ON"}},{"TEXT_HOLD_B","text_hold_b",2,{"OFF","ON"}},{"TEXT_AUTO","text_auto",2,{"OFF","ON"}},
};
#define MENU_N ((int)(sizeof mItems/sizeof mItems[0]))
static const char*const kCheatKey[6]={"cheat_money","cheat_catch","cheat_shiny","cheat_eggs","cheat_exp","cheat_candy"};static int cheatV[6]={-1,-1,-1,-1,-1,-1};
extern "C" int P3DS_Cheat(int k){if(k<0||k>5)return 0;if(cheatV[k]<0){int v=P3DS_CfgInt(kCheatKey[k],0);cheatV[k]=v<0||v>(k==4?2:1)?0:v;if(cheatV[k])PSPNativeMemLog("[CHEAT] %s=%d",kCheatKey[k],cheatV[k]);}return cheatV[k];}
static const int kGenDef[]={1,1,0,1,1,1,1,1,1,1,1,1,0,0,2,0,0,0,1,1024,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0};static int genV[sizeof kGenDef/sizeof kGenDef[0]];static bool genInit;
static int genGet(int g){if(!genInit){genInit=true;for(int k=0;k<(int)(sizeof kGenDef/sizeof kGenDef[0]);k++){int v=P3DS_CfgInt(mItems[33+k].key,kGenDef[k]);if(!strcmp(mItems[33+k].key,"log_max_kb"))v=v<=0?0:v<=512?1:v<=1024?2:3;if(v<0)v=0;if(v>=mItems[33+k].n)v=mItems[33+k].n-1;genV[k]=v;}}return genV[g];}
static int menuGet(int i){if(i>=33)return genGet(i-33);if(i>=27)return P3DS_Cheat(i-27);switch(i){case 0:return fpsOn?1:0;case 1:return wideCfg>0?1:0;case 2:return P3DS_ScaleMode();case 3:return P3DS_CstickMode();
 case 4:return P3DS_CtlZl>0?1:0;case 5:return P3DS_CtlZr>0?1:0;case 6:return P3DS_CtlCp>0?1:0;case 7:return P3DS_WideFxCfg()?1:0;case 8:return P3DS_LogLevel()>=2?1:0;
 case 9:return P3DS_CamZoom()+2;case 10:return stereoStr<0?0:stereoStr;case 11:return P3DS_GpuSwizzle()?1:0;case 12:return G3G_FastConv()?1:0;case 13:return splitCfg>0?1:0;case 14:return P3DS_FollowerCfg()?1:0;case 15:return P3DS_GpuComposeCfg();
 case 16:return P3DS_FollowerSlotCfg();case 17:return P3DS_FollowerTalkCfg();case 18:return menuAudioCfg();case 19:return gcStereoCfg();case 20:return stereo2dCfg();case 21:return P3DS_WideHazeCfg();case 22:{int v=P3DS_FastTextCfg();return v<=1?0:v-1;}case 23:return P3DS_ReusableTMsCfg();case 24:return P3DS_CheatNoEncCfg();case 25:return P3DS_RepelPromptCfg();case 26:return sumIvCfg();}return 0;}
static void menuSet(int i,int v){if(i>=33){genGet(0);genV[i-33]=v;static const int kLog[4]={0,512,1024,2048};P3DS_CfgWrite(mItems[i].key,!strcmp(mItems[i].key,"log_max_kb")?kLog[v&3]:v);return;}if(i>=27&&i<33)cheatV[i-27]=v;switch(i){case 0:fpsOn=v!=0;break;case 1:wideCfg=v;break;case 2:P3DS_SetScaleMode(v);break;case 3:P3DS_SetCstickMode(v);break;
  case 4:P3DS_CtlZl=v;if(!v){extern volatile int P3DS_RunToggle;P3DS_RunToggle=0;}break;case 5:P3DS_CtlZr=v;break;case 6:P3DS_CtlCp=v;break;case 7:wideFxV=v;break;case 8:P3DS_SetLogLevel(v?2:1);break;
  case 9:P3DS_SetCamZoom(v-2);break;case 10:stereoStr=v;break;case 11:P3DS_SetGpuSwizzle(v);break;case 12:G3G_SetFastConv(v);break;case 13:splitCfg=v;break;case 14:P3DS_SetFollowerCfg(v);break;case 15:P3DS_SetGpuComposeCfg(v);break;
  case 16:folSlotV=v;break;case 17:folTalkV=v;break;case 18:menuAudioV=v;break;case 19:gcStereoV=v;break;case 20:stereo2dV=v;break;case 21:P3DS_SetWideHazeCfg(v);break;case 22:fastTextV=v?v+1:0;break;case 23:reuseTmV=v;break;case 24:noEncV=v;break;case 25:repPV=v;break;case 26:sumIvV=v;break;}
 if(i!=2)P3DS_CfgWrite(mItems[i].key,i==8?(v?2:1):i==9?v-2:i==22?(v?v+1:0):v);}
static void prettyLab(char*d,const char*s,int n);
static void runMenu(const u32*top,const u32*bot,int mode,bool gpu,bool drawBot){
 static u32 base[256*192],img[256*192];memcpy(base,top,sizeof base);
 enum{N=MENU_N};
 int val[N];for(int i=0;i<N;i++)val[i]=menuGet(i);
 int old[N];memcpy(old,val,sizeof old);int sel=0,top0=0;const int VIS=10;
 // dim the picture once
 for(int i=0;i<256*192;i++){u32 c=base[i];base[i]=((c>>1)&0x007F7F7Fu)|(c&0xFF000000u);}
 P3DS_MenuOpen=1;PSPNativeMemLog("[MENU] open");P3DS_Toast("");toastAt=0;
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
#define P3DS_VERSION "run80"
#endif
static const char*kAppVer="Platinum 3DS " P3DS_VERSION " (" __DATE__ ")";
#include "p3ds_areanames.h"
#include "p3ds_abilnames.h"
extern "C" volatile int P3DS_CatchInfo[20];static int appStartTab=-1;
struct ATab{const char*name;int n;int it[16];const char*nm[16];};
static const ATab kTabs[]={
 {"Video",9,{0,1,2,7,9,10,20,21,13},{"FPS counter","Wide view","Scale","Wide view FX","Camera zoom","3D strength","Battle 3D depth","Wide haze","Split top (mono)"}},
 {"Game",12,{22,23,3,4,5,6,14,16,17,18,25,26},{"Fast text","Reusable TMs","C-stick","ZL run toggle","ZR = item (Y)","Circle pad D-pad","Following Pok\xe9mon","Follower slot","Follower talk","Menu music","Repel prompt","IV/EV on summary"}},
 {"Cheat",11,{24,27,28,29,30,31,32,53,54,55,56},{"No wild encounters","Infinite money","Always catch","Shiny wild","Fast eggs","EXP boost","Rare Candy 99","Infinite PP","Fast running (B)","Walls: hold R","Time of day"}},
 {"Pal",9,{33,34,35,36,37,38,39,40,41},{"Area talk","Follower cry","Follower finds items","Ball pop-out","Emote bubbles","Friendship talk","Idle look-around","Follower gap","Behind player"}},
 {"Wide",9,{42,43,44,45,59,60,61,62,67},{"Weather in strips","Edge fill","Wide menus","Mirror fill (old)","Distortion 3D depth","Distortion debris","Uneasy Pok\xe9mon","Distant cry","Distortion camera roll"}},
 {"Extra",16,{46,47,48,49,50,51,52,57,58,63,64,65,66,68,69,70},{"Visible wild Pok\xe9mon","Visible wild max","Only visible battle","Wild sprite test","HMs without learning","Save backups (3)","Log size limit","Catch info (Y)","Battle anim speed","Encounter counter","Time-of-day tint","Hidden item ping","Touch quick menu","Bottom button strip","Hold B: fast text","Auto-advance text"}},
 {"Ball",-3,{},{}},
 {"Menu",-1,{},{}},
 {"Map",-2,{},{}},
 /* run 67: the old Speed tab, hidden: only shown when SELECT is held while the settings app opens */
 {"Debug",5,{11,12,15,19,8},{"Fast upload","Fast 3D convert","GPU compose","GPU compose 3D","Log detail"}},
};
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
static void tri(u32*img,int x,int y,int dir,u32 c){for(int k=0;k<4;k++)for(int yy=k;yy<8-k;yy++){int X=dir>0?x+k:x+3-k;if(X>=0&&X<256&&y+yy>=0&&y+yy<192)img[(y+yy)*256+X]=c;}}
static int gtxt(u32*img,int x,int y,const char*t,u32 fg,u32 sh){if(P3DS_GameFontReady())return P3DS_GameFontDraw(img,256,256,192,x,y,t,fg,sh,1,1);char u[64];toUpper(u,t,sizeof u);drawBig(img,x,y+4,u,fg);return (int)strlen(u)*8;}
static int gw(const char*t){if(P3DS_GameFontReady())return P3DS_GameFontWidth(1,t);return (int)strlen(t)*8;}
extern "C" void P3DS_GpuInvalidateBot(void);
static u32 kMenuFg(){return kMsgFg;}
static int appCfg(){static int v=-1;if(v<0){v=P3DS_CfgInt("settings_app",1)!=0;PSPNativeMemLog("[APP] settings_app=%d",v);}return v;}
static int touchToDS(int px,int py,int*x,int*y){int dx,dy;if(P3DS_ScaleMode()==0){dx=px-32;dy=py-24;}else{dx=px*256/320;dy=py*192/240;}if(dx<0||dx>255||dy<0||dy>191)return 0;*x=dx;*y=dy;return 1;}
static void runApp(const u32*top,int mode,bool gpu){
 const size_t PX=256*192;u32*base=(u32*)malloc(PX*4*3);if(!base){PSPNativeMemLog("[APP] no memory");return;}u32*img=base+PX,*bimg=img+PX;memcpy(base,top,PX*4);
 enum{N=MENU_N};int val[N];for(int i=0;i<N;i++)val[i]=menuGet(i);int old[N];memcpy(old,val,sizeof old);
 for(int i=0;i<256*192;i++){u32 c=base[i];base[i]=((c>>1)&0x007F7F7Fu)|(c&0xFF000000u);}
 P3DS_MenuOpen=1;PSPNativeMemLog("[APP] settings open");P3DS_Toast("");toastAt=0;
 static bool dbgTabs=false;int NT=(int)(sizeof kTabs/sizeof kTabs[0])-(dbgTabs?0:1);static int tab=0,page=0;if(appStartTab>=0){tab=appStartTab;page=0;appStartTab=-1;}if(tab>=NT)tab=0; /* run 67: SELECT inside the app shows/hides the Debug tab */int sel=0;static int mapPage=0;
 u32 held=hidKeysHeld();bool wasTouch=(held&KEY_TOUCH)!=0;u64 t0=svcGetSystemTick();
 const u32 bg=0x00B0D8B8u,cellF=0x00F8F8F8u,cellE=0x00586058u,selE=0x0040B040u,valC=0x001828E0u,valSh=0x00A0A8F0u,tabSel=0x0060C070u;
 while(aptMainLoop()){
  hidScanInput();u32 kh=hidKeysHeld(),kd=hidKeysDown()&~held;held&=kh;
  if(kd&(KEY_B|KEY_START))break;if((kd&KEY_Y)&&(kh&KEY_L)&&(kh&KEY_R))break;if((kd&KEY_Y)&&kTabs[tab].n==-3)break;if(kd&KEY_SELECT){dbgTabs=!dbgTabs;NT=(int)(sizeof kTabs/sizeof kTabs[0])-(dbgTabs?0:1);if(tab>=NT){tab=0;page=0;}sel=0;PSPNativeMemLog("[APP] debug tab %s",dbgTabs?"shown":"hidden");}
  const ATab&T=kTabs[tab];int pages=T.n>10?(T.n+9)/10:1;if(page>=pages)page=0;int first=page*10,cnt=T.n-first;if(cnt>10)cnt=10;if(cnt<0)cnt=0;
  static const char*const kMenuNm[6]={"Pok\xe9mon","Bag","Pok\xe9" "dex","Trainer card","Save","Options"};static const int kMenuOpt[6]={1,2,0,3,4,5};
  const bool menuTab=T.n==-1;int startReq=-1;
  if(kd&KEY_L){tab=(tab+NT-1)%NT;page=0;sel=0;continue;}if(kd&KEY_R){tab=(tab+1)%NT;page=0;sel=0;continue;}
  if(cnt){if(kd&KEY_DUP)sel=(sel+cnt-2)%cnt;if(kd&KEY_DDOWN)sel=(sel+2)%cnt;if(kd&(KEY_DLEFT|KEY_DRIGHT)&&cnt>1)sel^=1;if(sel>=cnt)sel=cnt-1;
   int it=T.it[first+sel],nn=mItems[it].n;if(kd&KEY_A)val[it]=(val[it]+1)%nn;if(kd&KEY_X)val[it]=(val[it]+nn-1)%nn;}
  /* touch: act on press (not while the opening combo is still held) */
  bool tdown=(kh&KEY_TOUCH)!=0;if(tdown&&!wasTouch){touchPosition tp;hidTouchRead(&tp);int x,y;if(touchToDS(tp.px,tp.py,&x,&y)){
    if(y<20){int t=x*NT/256;if(t>=0&&t<NT){if(t==tab){int pg=kTabs[t].n>10?(kTabs[t].n+9)/10:1;page=(page+1)%pg;}else{tab=t;page=0;}sel=0;}}
    else if(y>=22&&T.n==-2)mapPage++;
    else if(y>=22&&menuTab){int r=(y-22)/54,c=x>=128?1:0,i=r*2+c;if(r<3&&i<6)startReq=kMenuOpt[i];}
    else if(y>=22&&cnt){int r=(y-22)/34,c=x>=128?1:0,i=r*2+c;if(r<5&&i<cnt){sel=i;int it=T.it[first+i],nn=mItems[it].n;int cx=c?130:4;
      if(x<cx+22)val[it]=(val[it]+nn-1)%nn;else val[it]=(val[it]+1)%nn;}}}}
  wasTouch=tdown;
  if(menuTab&&(kd&KEY_A)){static const int o[6]={1,2,0,3,4,5};startReq=o[sel<6?sel:0];}
  if(menuTab&&(kd&(KEY_DUP|KEY_DDOWN|KEY_DLEFT|KEY_DRIGHT))){if(kd&KEY_DUP)sel=(sel+4)%6;if(kd&KEY_DDOWN)sel=(sel+2)%6;if(kd&(KEY_DLEFT|KEY_DRIGHT))sel^=1;}
  if(startReq>=0){extern volatile int P3DS_StartReq;P3DS_StartReq=P3DS_InField?startReq:-1;PSPNativeMemLog("[APP] start menu request %d (field %d)",startReq,P3DS_InField);break;}
  /* top: dimmed game + title box with version */
  memcpy(img,base,PX*4);
  if(useGF()){P3DS_GameBoxDraw(img,256,256,192,16,16,216,32);gtxt(img,18,16,"Settings  (game paused)",kMsgFg,kMsgSh);gtxt(img,18,32,kAppVer,0x00787878u,0x00D0D0D0u);
   P3DS_GameBoxDraw(img,256,256,192,16,152,216,32);gtxt(img,18,152,"Tap a setting to change it.",kMsgFg,kMsgSh);gtxt(img,18,168,"L/R: tab   A: change   B: close",kMsgFg,kMsgSh);}
  else{menuBox(img,8,10,248,52,0x00502010u,0x00F0F0F0u);drawBig(img,16,18,"SETTINGS (PAUSED)",0x0060E0FFu);drawBig(img,16,34,"RUN64",0x00C0C0C0u);}
  /* bottom: the app */
  for(int i=0;i<256*192;i++)bimg[i]=bg;
  for(int t=0;t<NT;t++){int x0=t*256/NT+1,x1=(t+1)*256/NT-1;bool on=t==tab;plate(bimg,x0,1,x1,20,on?tabSel:cellF,cellE);
   char nm[24];if(on&&kTabs[t].n>10)snprintf(nm,sizeof nm,"%d/%d",page+1,(kTabs[t].n+9)/10);else snprintf(nm,sizeof nm,"%s",kTabs[t].name);
   gtxt(bimg,(x0+x1)/2-gw(nm)/2,2,nm,on?0x00F8F8F8u:kMsgFg,on?0x00387038u:kMsgSh);}
  if(menuTab)for(int i=0;i<6;i++){int r=i/2,c=i%2;int x0=c?130:4,y0=24+r*54,x1=x0+122,y1=y0+50;bool s2=i==sel;
   plate(bimg,x0,y0,x1,y1,cellF,s2?selE:cellE);if(s2)plate(bimg,x0+1,y0+1,x1-1,y1-1,cellF,selE);gtxt(bimg,(x0+x1)/2-gw(kMenuNm[i])/2,y0+16,kMenuNm[i],kMenuFg(),kMsgSh);}
  else if(T.n==-3){/* run 75 Catch tab: read-only snapshot of the wild foe (taken when the command menu opens) */
   plate(bimg,2,22,254,190,cellF,cellE);const volatile int*ci=P3DS_CatchInfo;
   if(!P3DS_CfgInt("catch_overlay",0)){gtxt(bimg,128-gw("Turn on Catch info in Extra.")/2,80,"Turn on Catch info in Extra.",0x00787878u,0x00D0D0D0u);}
   else if(!ci[0]||P3DS_InField){gtxt(bimg,128-gw("Only in wild battles.")/2,80,"Only in wild battles.",0x00787878u,0x00D0D0D0u);}
   else{static const char*const nat[25]={"Hardy","Lonely","Brave","Adamant","Naughty","Bold","Docile","Relaxed","Impish","Lax","Timid","Hasty","Serious","Jolly","Naive","Modest","Mild","Quiet","Bashful","Rash","Calm","Gentle","Sassy","Careful","Quirky"};
    char l[96];const int sp=ci[1],ab=ci[6];
    snprintf(l,sizeof l,"%s  Lv%d  %s",sp>0&&sp<494?kP3dsSpec[sp]:"?",ci[2],ci[16]?"(caught before)":"(new)");gtxt(bimg,8,24,l,valC,valSh);
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
    if(P3DS_CfgInt("hunt_counter",0)){snprintf(l,sizeof l,"Met %d times (hunt.txt)",P3DS_HuntCount(sp));gtxt(bimg,256-8-gw(l),116,l,0x00787878u,0x00D0D0D0u);}
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
  else if(!cnt){plate(bimg,8,40,248,120,cellF,cellE);gtxt(bimg,128-gw("No cheats yet.")/2,56,"No cheats yet.",kMsgFg,kMsgSh);
   gtxt(bimg,128-gw("Cheats will be added here, all off.")/2,80,"Cheats will be added here, all off.",0x00787878u,0x00D0D0D0u);}
  for(int i=0;i<cnt;i++){int it=T.it[first+i],r=i/2,c=i%2;int x0=c?130:4,y0=22+r*34,x1=x0+122,y1=y0+32;bool s2=i==sel;
   plate(bimg,x0,y0,x1,y1,cellF,s2?selE:cellE);if(s2)plate(bimg,x0+1,y0+1,x1-1,y1-1,cellF,selE);
   const char*nm=T.nm[first+i];gtxt(bimg,(x0+x1)/2-gw(nm)/2,y0-1,nm,kMsgFg,kMsgSh);
   char lab[24];prettyLab(lab,mItems[it].lab[val[it]],sizeof lab);gtxt(bimg,(x0+x1)/2-gw(lab)/2,y0+14,lab,valC,valSh);
   tri(bimg,x0+5,y0+19,-1,cellE);tri(bimg,x1-9,y0+19,1,cellE);
   if(val[it]!=old[it])for(int k=0;k<3;k++)for(int q=0;q<3;q++)bimg[(y0+4+k)*256+x1-6+q]=selE;}
  {extern int P3DS_GpuBotSame;P3DS_GpuBotSame=0;}
  if(mode&&gpu)P3DS_GpuPresent(img,bimg,mode,true,nullptr);
  else{present(GFX_TOP,400,img,0);present(GFX_BOTTOM,320,bimg,0);gfxFlushBuffers();gfxSwapBuffers();}
  gspWaitForVBlank();
  if(menuAudioCfg()){static u64 mt;u64 nw=svcGetSystemTick();unsigned us=mt&&nw>mt?(unsigned)((nw-mt)/CPU_TICKS_PER_USEC):16667;mt=nw;if(us>66666)us=16667;PSPNativeSoundAdvance(us);}
 }
 for(int i=0;i<120&&aptMainLoop();i++){hidScanInput();if(!(hidKeysHeld()&(KEY_A|KEY_B|KEY_X|KEY_START|KEY_Y|KEY_L|KEY_R|KEY_TOUCH|KEY_DUP|KEY_DDOWN|KEY_DLEFT|KEY_DRIGHT)))break;gspWaitForVBlank();}
 free(base);P3DS_GpuInvalidateBot();int ch=0;for(int i=0;i<N;i++)if(val[i]!=old[i]){menuSet(i,val[i]);ch++;}
 P3DS_MenuOpen=0;PSPNativeMemLog("[APP] settings closed after %u ms, %d change(s)",(unsigned)((svcGetSystemTick()-t0)/CPU_TICKS_PER_MSEC),ch);
 tPrevEnd=svcGetSystemTick();secStart=0;secFrames=secG3=secComp=secGpu=secGame=secTris=0;secG3G=0;
}
extern "C" int PSPNativeRenderBegin(){if(!initialized||openFrame)return -1;openFrame=true;G3SIM_DrawCleanUp();P3DS_G3BeginFrame();return 0;}
static int Present(bool){
 if(!openFrame)return -1;
 allocBufs();
 P3DS_RomCachePumpC(); /* run 51 zone_prefetch: adopt staged blocks on the game thread */
 if(earlyCfg<0){earlyCfg=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"early_b=%d",&x)==1)earlyCfg=x!=0;fclose(f);}PSPNativeMemLog("[2D] early_b=%d",earlyCfg);}
 bool earlyB=false;
 if(earlyCfg&&engineBAsync()&&stealCfg!=1&&c1Mode!=1){unsigned pw=reg16(0x304);
  mapMemory();engineB.Enabled=(pw&512)!=0;bindRegisters(engineB,0x1000);FrameStart(engineB);bPower=pw;bRightSrc=nullptr;
  bEarly=true;earlyB=true;earlyN++;bPost();}
 G3SIM_FlushDeferred();openFrame=false;
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
 u32 hash0=stateHash();
 if(!earlyB){mapMemory();engineB.Enabled=(power&512)!=0;bindRegisters(engineB,0x1000);} /* early_b: already mapped; the worker may be reading it */
 engineA.Enabled=(power&2)!=0;bindRegisters(engineA,0);
 /* run 76 display capture through the renderer's own DoCapture (melonDS): source A = 2D+3D composite or 3D, source B =
    VRAM, blend A/B. Motion blur at Spear Pillar (Dialga/Palkia) shows VRAM C fed by capture; ignored it was black. */
 {const u32 cc=reg32(0x64);const int cm=P3DS_CfgInt("display_capture",1);engineA.CaptureCnt=cm==1?cc:0;engineA.CaptureLatch=cm==1&&(cc&0x80000000u)&&engineA.Enabled;}
 /* per-vertex stereo (g3soft rendered both eyes) if it was enabled at frame start; else flat-layer fallback */
 {static unsigned diagN;if(++diagN%120==0){unsigned n=0;for(unsigned i=0;i<256*192;i++)if(GPU3D::NativeFrame[i]>>24)n++;
   printf("[3DLAYER] dispcntA %08x powcnt %04x 3D px alpha>0 %u bg0cnt %04x blend %04x/%04x\n",reg32(0),power,n,reg16(8),reg16(0x50),reg16(0x52));}}
 u32*rightEye=P3DS_G3RightEye();
 int sd=rightEye?0:stereoShift(P3DS_ScaleMode());bool stereo=(rightEye||sd)&&(power&0x8000)&&engineA.Enabled;
 if(stereo&&!rightEye)shift3D(-sd);
 FrameStart(engineA);if(!earlyB)FrameStart(engineB);renderer.SetFramebuffer(raw[0],raw[1]);
 bool async=engineBAsync();
 static int par3d=-1;if(par3d<0){par3d=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");int x;char l[64];if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"stereo2d_par=%d",&x)==1)par3d=x!=0;fclose(f);}PSPNativeMemLog("[2D] stereo right eye on core 2: %d",par3d);}
 const int gcMode0=P3DS_GpuComposeCfg();
 const bool gcSt=gcMode0==2&&stereo&&rightEye&&P3DS_InField&&engineA.Enabled&&gcStereoCfg()&&gcEligible();
 bool rightPar=async&&par3d&&stereo&&rightEye&&!gcSt;
 if(rightPar){memcpy((void*)&engineA2,(const void*)&engineA,sizeof engineA);engineA2.CaptureCnt=0;engineA2.CaptureLatch=false;}
 /* run 55 stereo_2d=1 (default): outside the overworld (battles, menus, title) the right eye's 2D background layers
    get depth by BG priority: priority 3 (back) = the far-3D disparity (2 x shift), priority 2 = 1.5 x, priorities 0-1
    (text boxes, menus, HUD) and sprites stay at the screen plane. Text-mode BGs only (affine/bitmap untouched).
    Overworld unchanged (its BG overlays sit in front of the 3D world). */
 if(rightPar&&!P3DS_InField){const int cfg=stereo2dCfg();
  if(cfg){const int sd=stereoShiftF(P3DS_ScaleMode());const unsigned bgm=engineA2.DispCnt&7;
   for(int i=1;i<4;i++){if(!(engineA2.DispCnt&(0x100<<i)))continue;const bool text=i<2||bgm==0||(i==2&&bgm==1);if(!text)continue;
    const int pr=engineA2.BGCnt[i]&3,d=cfg==1?(pr==3?2*sd:pr==2?(3*sd)/2:0):(pr==3?(sd+1)/2:pr==2?(sd+2)/4:0);if(d)engineA2.BGXPos[i]=(u16)((engineA2.BGXPos[i]-d)&0x1FF);}}}
 bRightSrc=rightPar?rightEye:nullptr;
 bPower=power;if(!async)P3DS_BottomReused=0;
 const bool onC1=rightPar&&!earlyB&&c1Ready();bSkipB=onC1||earlyB;
 if(onC1)LightEvent_Signal(&c1Start);
 if(stealCfg<0){stealCfg=0;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"steal_b=%d",&x)==1)stealCfg=x!=0;fclose(f);}PSPNativeMemLog("[2D] steal_b=%d",stealCfg);}
 bClaim=0;const bool stealNow=async&&stealCfg==1&&rightPar&&!onC1&&!earlyB;
 if(splitCfg<0){splitCfg=1;/* run 57: default 1 (Azahar: identical images overworld/battle/menus, mono composite -37%) */FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"split_top=%d",&x)==1)splitCfg=x!=0;fclose(f);}PSPNativeMemLog("[2D] split_top=%d",splitCfg);}
 const int gcMode=P3DS_GpuComposeCfg();if(gcMode>=2)gcFrames++;
 const bool gcOK=gcMode==2&&(!stereo||gcSt)&&!rightPar&&engineA.Enabled&&(gcSt||gcEligible());
 const bool splitNow=!gcOK&&splitCfg&&async&&!stereo&&!rightPar&&!onC1&&!stealNow&&engineA.Enabled&&((a>>16)&3)==1&&!(reg32(0x64)&0x80000000u);
 if(splitNow){memcpy((void*)&engineAS,(const void*)&engineA,sizeof engineA);line0Done=false;bSplit=true;splitN++;}
 if(async)bPost();
 const unsigned aEnd=splitNow?96:192;
 bool gcDone=false,gcStR=false;static unsigned gcStN;{static unsigned gsl;if(gcSt&&++gsl%600==0){PSPNativeMemLog("[GPUC] stereo: %u of last 600 stereo-eligible frames GPU-composed for both eyes",gcStN);gcStN=0;}}
 if(gcOK){if(!async)for(unsigned line=0;line<192;line++){GPU::VCount=line;engineB.CheckWindows(line);renderer.DrawSprites(line,&engineB);renderer.DrawScanline(line,&engineB);}
  gcDone=gcCompose(display[0]);if(gcDone)gcUsed++;line0Done=true;
  if(gcDone&&gcSt){gcStR=P3DS_GcComposeA((const u8*)PSPNative_GfxRegisters,GPU::VRAMFlat_ABG,GPU::VRAMFlat_AOBJ,(const u16*)GPU::Palette,(const u16*)(GPU::Palette+512),(const u16*)GPU::OAM,rightEye,rawR)!=0;if(gcStR)gcStN++;}}
 if(!gcDone)for(unsigned line=0;line<192;line++){if(line>=aEnd&&async)break;GPU::VCount=line;engineA.CheckWindows(line);renderer.DrawSprites(line,&engineA);renderer.DrawScanline(line,&engineA);if(!line)line0Done=true;
  if(!async&&!gcOK){engineB.CheckWindows(line);renderer.DrawSprites(line,&engineB);renderer.DrawScanline(line,&engineB);}}
 if(stealNow&&claimB()){stealMain++;drawEngineB();}
 P3DS_Crumb(async?(excl?"b-spin":"b-wait"):"b-none",0);
 if(async){if(excl){while(!LightEvent_TryWait(&bDone)){}}else LightEvent_Wait(&bDone);}
 if(onC1)LightEvent_Wait(&c1Done);
 huntTick();
 {/* run 76 display capture (DISPCAPCNT 0x4000064): the Spear Pillar / Giratina transition captures the screen into a VRAM
     bank and shows that bank; unimplemented it was black. Minimal version: source A (engine A output, 8-bit -> RGB555); display_capture=0 restores the old ignore,
     no blending, written into the LCDC bank; the enable bit is cleared like on hardware. */
  volatile u32*capR=(volatile u32*)(PSPNative_GfxRegisters+0x64);const u32 cap=*capR;
  const int capCfg=P3DS_CfgInt("display_capture",1);
  if(engineA.CaptureLatch){engineA.CaptureLatch=false;*capR=cap&0x7FFFFFFFu;static unsigned cn;if(cn++%120==0)PSPNativeMemLog("[CAP] renderer capture %08x (%u)",(unsigned)cap,cn);}
  if(capCfg==2&&(cap&0x80000000u)&&display[0]){static const int hs[4]={128,64,128,192};const int bank=(cap>>16)&3,off=((cap>>18)&3)*0x8000,sz=(cap>>20)&3,w=sz?256:128,h=hs[sz];
   u8*vb=s_HW_LCDC_VRAM+bank*0x20000;const u32*src=display[0];
   for(int y=0;y<h;y++)for(int x=0;x<w;x++){u32 c=src[y*256+x];u16 v=(u16)(((c>>3)&31)|(((c>>11)&31)<<5)|(((c>>19)&31)<<10)|0x8000);*(u16*)(vb+((off+(y*w+x)*2)&0x1FFFF))=v;}
   *capR=cap&~0x80000000u;static unsigned nc;if(!(nc++%120))PSPNativeMemLog("[CAP] display capture bank %d off %x size %d (%u)",bank,off,sz,nc);}}
 P3DS_Crumb(earlyB?"b-early":"b-noearly",0);
 if(earlyB)LightEvent_Wait(&bEarlyDone);
 P3DS_Crumb("b-ok",0);
 {midChecks++;if(stateHash()!=hash0)midWrites++;if(midChecks>=600){PSPNativeMemLog("[2D] game state (OAM/palette/2D regs) changed DURING composite in %u of %u frames (comp_excl=%d)",midWrites,midChecks,(int)excl);midChecks=midWrites=0;}}
 if(excl)svcSetThreadPriority(CUR_THREAD_HANDLE,oldPrio);
 if(gcMode==3&&!stereo&&engineA.Enabled&&gcEligible()){if(!gcScratch)gcScratch=(u32*)linearMemAlign(256*256*4,0x80);
  if(gcScratch&&gcCompose(gcScratch)){gcVer++;unsigned bad=0;for(unsigned i=0;i<256*192;i++)if((gcScratch[i]^display[0][i])&0xFFFFFF)bad++;
   if(bad){gcVerBad++;gcBadPx+=bad;static int dumps;if(dumps<3){char fn[96];snprintf(fn,sizeof fn,"sdmc:/3ds/platinum3ds/gpuc_mm%d_cpu.bmp",dumps);dumpBmp(fn,display[0],false);snprintf(fn,sizeof fn,"sdmc:/3ds/platinum3ds/gpuc_mm%d_gpu.bmp",dumps);dumpBmp(fn,gcScratch,false);
     unsigned fi=0;while(fi<256*192&&!((gcScratch[fi]^display[0][fi])&0xFFFFFF))fi++;PSPNativeMemLog("[GPUC] verify mismatch #%d frame %u: %u px, first at (%u,%u) cpu %08x gpu %08x",dumps,frames,bad,fi%256,fi/256,display[0][fi],gcScratch[fi]);dumps++;}}
   else gcVerOk++;}}
 if(gcMode>=2)gcLog();
 lastDraw=3;
 {u32 kh=hidKeysHeld();static bool dumped;static int dumpAt=-2;if(dumpAt==-2){dumpAt=-1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"dump_frame=%d",&x)==1)dumpAt=x;fclose(f);}}
  bool autoDump=dumpAt>=0&&(int)frames==dumpAt;
  if(autoDump){char b[400];int n=0;for(int e=0;e<2;e++){unsigned base=e?0x1000:0;n+=snprintf(b+n,sizeof b-n,"[DUMP] eng%c DISPCNT %04x%04x BG0-3CNT %04x %04x %04x %04x BLDCNT %04x BLDALPHA %04x BLDY %04x WININ %04x WINOUT %04x | ",e?'B':'A',reg16(base+2),reg16(base),reg16(base+8),reg16(base+10),reg16(base+12),reg16(base+14),reg16(base+0x50),reg16(base+0x52),reg16(base+0x54),reg16(base+0x48),reg16(base+0x4a));}
   n+=snprintf(b+n,sizeof b-n,"DISP3DCNT %04x POWCNT %04x",reg16(0x60),reg16(0x304));PSPNativeMemLog("%s",b);}
  if(autoDump||(kh&(KEY_L|KEY_R|KEY_X|KEY_Y))==(KEY_L|KEY_R|KEY_X|KEY_Y)){/* run 43: BMP dump moved to L+R+X+Y (L+R+X = wideview) */if(!dumped){dumped=true;dumpBmp("sdmc:/3ds/platinum3ds/dump_3dlayer.bmp",GPU3D::NativeFrame,true);dumpBmp("sdmc:/3ds/platinum3ds/dump_engineA.bmp",display[0],false);dumpBmp("sdmc:/3ds/platinum3ds/dump_engineB.bmp",display[1],false);printf("[3DS] dumped BMPs\n");}}else dumped=false;}
 {/* run 34 debug: dump_seq=START,COUNT dumps composed engine A/B frames START..START+COUNT-1 as raw RGBA to sdmc:/3ds/platinum3ds/seq/ */
  static int sq0=-2,sqn=0;if(sq0==-2){sq0=-1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int a,b;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"dump_seq=%d,%d",&a,&b)==2){sq0=a;sqn=b;}fclose(f);}if(sq0>=0)mkdir("sdmc:/3ds/platinum3ds/seq",0777);}
  if(sq0>=0&&(int)frames>=sq0&&(int)frames<sq0+sqn){char fn[80];snprintf(fn,sizeof fn,"sdmc:/3ds/platinum3ds/seq/f%05u.raw",frames);FILE*f=fopen(fn,"wb");if(f){fwrite(display[0],4,256*192,f);fwrite(display[1],4,256*192,f);fclose(f);}}}
 const u32*eyeL=nullptr,*eyeR=nullptr;
 if(stereo&&gcStR){eyeL=display[0];eyeR=rawR;} /* run 61: both eyes from the GPU compositor */
 else if(stereo&&rightPar){eyeL=display[0];eyeR=rawR;} /* no copies: present reads both eyes in place */
 else if(stereo){memcpy(stereoL,display[0],sizeof stereoL);if(rightEye)memcpy(GPU3D::NativeFrame,rightEye,256*192*4);else shift3D(2*sd);FrameStart(engineA);
  for(unsigned line=0;line<192;line++){GPU::VCount=line;engineA.CheckWindows(line);renderer.DrawSprites(line,&engineA);renderer.DrawScanline(line,&engineA);}
  if(!rightEye)shift3D(-sd);eyeL=stereoL;eyeR=display[0];}
 {/* run 61 debug: dump_eyes=F dumps both eyes at frame F and every 300 frames after (6 max) -> eyeN_L/R.bmp */
  static int de=-2,dn;if(de==-2)de=P3DS_CfgInt("dump_eyes",-1);
  if(de>=0&&stereo&&eyeL&&eyeR&&dn<6&&(int)frames>=de+dn*300){char fn[64];snprintf(fn,sizeof fn,"sdmc:/3ds/platinum3ds/eye%d_L.bmp",dn);dumpBmp(fn,eyeL,false);snprintf(fn,sizeof fn,"sdmc:/3ds/platinum3ds/eye%d_R.bmp",dn);dumpBmp(fn,eyeR,false);PSPNativeMemLog("[STEREO] dumped eyes %d at frame %u field %d gcStereo %d",dn,frames,P3DS_InField,(int)gcStR);dn++;}}
 /* run 55 stereo blob diag (stereo_blobdiag=1, default): every 60 stereo frames, 16x16 cells where one eye is black
    (rgb < 8) on > 60% of the cell and the other eye is not -> one log line; the first hit dumps both eyes and both 3D
    layers to sdmc:/3ds/platinum3ds/stereo_blob_{L,R,3dL,3dR}.bmp for the next report. */
 if(stereo&&eyeL&&eyeR){static int cfg=-1;static unsigned n,hits;static bool dumpedB;if(cfg<0){cfg=P3DS_CfgInt("stereo_blobdiag",1)!=0;}
  if(cfg&&++n%60==0&&hits<40){int bestN=0,bx=0,by=0,beye=0;
   for(int cy=0;cy<12;cy++)for(int cx=1;cx<15;cx++){int l=0,r=0;for(int y=cy*16;y<cy*16+16;y++)for(int x=cx*16;x<cx*16+16;x++){u32 a=eyeL[y*256+x]&0xF8F8F8u,b=eyeR[y*256+x]&0xF8F8F8u;if(!a&&b)l++;else if(a&&!b)r++;}
    if(l>bestN){bestN=l;bx=cx;by=cy;beye=0;}if(r>bestN){bestN=r;bx=cx;by=cy;beye=1;}}
   if(bestN>154){hits++;const u32*t3R=P3DS_G3RightEye();PSPNativeMemLog("[STEREO] blob: %s eye black-only cell x %d..%d y %d..%d (%d/256 px) | slider %.2f field %d dispcnt %08x",beye?"RIGHT":"LEFT",bx*16,bx*16+15,by*16,by*16+15,bestN,(double)p3dsSlider(),P3DS_InField,(unsigned)reg32(0));
    if(!dumpedB){dumpedB=true;dumpBmp("sdmc:/3ds/platinum3ds/stereo_blob_L.bmp",eyeL,false);dumpBmp("sdmc:/3ds/platinum3ds/stereo_blob_R.bmp",eyeR,false);
     if(rightPar)dumpBmp("sdmc:/3ds/platinum3ds/stereo_blob_3dL.bmp",GPU3D::NativeFrame,true);if(t3R)dumpBmp("sdmc:/3ds/platinum3ds/stereo_blob_3dR.bmp",t3R,true);}}}}
 /* POWCNT1 bit15: engine A on top screen. */
 const u32 *top=(power&0x8000)?display[0]:display[1],*bot=(power&0x8000)?display[1]:display[0];
 u64 tComp=svcGetSystemTick();
 /* run 54 wide_fx: before the save box / toast / fps overlay are drawn onto the centre */
 static bool fillReady;fillReady=false;
 static bool wnReady;wnReady=false;
 if(wideCfg>0&&P3DS_ScaleMode()==1&&P3DS_WideMenusCfg()&&!(G3G_Wide&&G3G_StripValid>=1&&P3DS_InField)&&(power&0x8000)&&engineA.Enabled){wideNative();wnReady=true;}
 else if(wideCfg>0&&P3DS_ScaleMode()==1&&wideFillCfg()&&!(G3G_Wide&&G3G_StripValid>=1&&P3DS_InField)&&(power&0x8000)){wideFill(display[0]);fillReady=true;}
 P3DS_WeatherOn=0;if(wideWeatherCfg()&&G3G_Wide&&G3G_StripValid>=1&&P3DS_InField&&(power&0x8000))wideWeatherRun();
 {extern int P3DS_HazeOn;P3DS_HazeOn=0;if(P3DS_WideHazeCfg()&&!wideFxCfg()&&G3G_Wide&&G3G_StripValid>=1&&P3DS_InField&&(power&0x8000))wideHaze(display[0],GPU3D::NativeFrame);}
 P3DS_WideFxOn=0;if(wideFxCfg()&&G3G_Wide&&G3G_StripValid>=1&&P3DS_InField&&(power&0x8000)){u64 tw=svcGetSystemTick();wideFx(display[0],GPU3D::NativeFrame,G3G_StripValid>=2?2:1);P3DS_WideFxOn=G3G_StripValid>=2?2:1;
  {static u64 acc;static unsigned n;acc+=svcGetSystemTick()-tw;if(++n==600){PSPNativeMemLog("[WIDE] wide_fx avg %u us/frame",(unsigned)(acc/600/268));acc=0;n=0;}}}
 {u64 nowT=svcGetSystemTick();u32*tt=(u32*)(stereo?eyeL:top);drawSaveBox(tt,nowT);if(stereo)drawSaveBox((u32*)eyeR,nowT);drawToast(tt,nowT);if(stereo)drawToast((u32*)eyeR,nowT);drawTodTint(tt);if(stereo)drawTodTint((u32*)eyeR);drawDwRoll(tt);if(stereo)drawDwRoll((u32*)eyeR);drawFolTalk(tt,nowT);drawFolEmote(tt,nowT);drawDwDebris(tt,0);if(stereo)drawDwDebris((u32*)eyeR,1);if(stereo)drawFolEmote((u32*)eyeR,nowT);drawSumIV(tt);drawPrompt(tt);if(stereo)drawPrompt((u32*)eyeR);if(stereo&&sumShow)drawSumIV((u32*)eyeR);if(stereo)drawFolTalk((u32*)eyeR,nowT);}
 {static int fi;if(!fi){fi=1;fpsOn=P3DS_CfgInt("fps_counter",0)!=0;/* run 64: default 0 (clean launch) */}}/* run 55: fps_counter (menu-saved) */
 if(fpsOn){char b[24];snprintf(b,sizeof b,"%u %u.%u",fpsShown,msShown/10,msShown%10);u32*tt=(u32*)(stereo?eyeL:top);drawText(tt,2,2,b);if(stereo)drawText((u32*)eyeR,2,2,b);}
 {u64 nowS=svcGetSystemTick();if(splashActive(nowS)){drawSplash((u32*)(stereo?eyeL:top),nowS);if(stereo)drawSplash((u32*)eyeR,nowS);}}
 /* run 78 bottom_strip=1 (default 0): overworld-only strip of 4 buttons in the bottom screen's lower border (DS y 177..190,
    x 8..207: below the Poketch screen, left of its buttons). Taps there go to the strip only (main3ds touch read
    drops them while P3DS_StripOn). Off = nothing drawn, every touch unchanged. */
 static int stripReq=0;
 {P3DS_StripOn=P3DS_CfgInt("bottom_strip",0)&&P3DS_InField&&appCfg()&&!P3DS_ConsoleOnBottom();
  if(P3DS_StripOn&&bot){static const char*const nm[4]={"Menu","Wide","Ball","Set"};u32*bb=(u32*)bot;
   for(int i=0;i<4;i++){const int x0=8+i*50;plate(bb,x0,177,x0+48,191,0x00F8F8F8u,0x00585858u);gtxt(bb,x0+24-gw(nm[i])/2,176,nm[i],kMsgFg,kMsgSh);}
   if(hidKeysDown()&KEY_TOUCH){touchPosition tp;hidTouchRead(&tp);int x,y;if(touchToDS(tp.px,tp.py,&x,&y)&&y>=176&&x>=8&&x<208){const int b=(x-8)/50;if(b>=0&&b<4){stripReq=b+1;PSPNativeMemLog("[STRIP] %s",nm[b]);}}}}}
 int mode=P3DS_ScaleMode();
 static int gpuOk=-1;if(mode&&gpuOk<0){gpuOk=P3DS_GpuInit();printf("[3DS] GPU scaling init %s\n",gpuOk?"ok":"FAILED (falling back to 1:1)");}
 {/* run 43 wideview (experimental, config wideview=1, default 0; L+R+X toggles): Hor+ side strips of the 3D world in
     the pillarbox (mode 1, overworld only, 3D on BG0). The flag drives the NEXT frame's 3D pass; the strips drawn now
     come from this frame's pass. Off = no extra work at all. */
  if(wideCfg<0){wideCfg=0;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"wideview=%d",&x)==1)wideCfg=x!=0;fclose(f);}PSPNativeMemLog("[WIDE] wideview=%d",wideCfg);}
  {u32 kd=hidKeysDown(),kh=hidKeysHeld();if(((kd&KEY_X)&&(kh&KEY_L)&&(kh&KEY_R)&&!(kh&KEY_Y))||stripReq==2){if(stripReq==2)stripReq=0;wideCfg=!wideCfg;PSPNativeMemLog("[WIDE] L+R+X: wideview=%d",wideCfg);P3DS_Toast(wideCfg?"WIDE VIEW ON":"WIDE VIEW OFF");}}
  const u32 dc=reg32(0);const bool act=wideCfg&&P3DS_InField&&mode==1&&gpuOk==1&&(dc&8)&&(dc&0x100)&&((dc>>16)&3)==1&&engineA.Enabled;
  {static int lastAct=-1;if((int)act!=lastAct){lastAct=act;PSPNativeMemLog("[WIDE] active=%d (cfg %d field %d mode %d 3Dbg0 %d)",(int)act,wideCfg,P3DS_InField,mode,(int)((dc&0x108)==0x108));}}
  P3DS_WideStrip[0]=act&&G3G_Wide&&G3G_StripValid>=1?G3G_Strip[0]:nullptr;P3DS_WideStrip[1]=act&&G3G_Wide&&G3G_StripValid>=2?G3G_Strip[1]:nullptr;
  if(!P3DS_WideStrip[0]&&fillReady&&gpuOk==1){P3DS_WideStrip[0]=P3DS_WideFillBuf;P3DS_WideStrip[1]=P3DS_WideFillBuf;P3DS_WideFxOn=1;}
  if(!P3DS_WideStrip[0]&&wnReady&&gpuOk==1){P3DS_WideStrip[0]=P3DS_WideFillBuf;P3DS_WideStrip[1]=stereo?P3DS_WideFillBuf:nullptr;} /* run 65 wide_menus (fade applied in strips()) */
  if(P3DS_WideFxOn>=1&&P3DS_WideStrip[0])P3DS_WideStrip[0]=P3DS_WideFxBuf[0];if(P3DS_WideFxOn>=2&&P3DS_WideStrip[1])P3DS_WideStrip[1]=P3DS_WideFxBuf[1];
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
  P3DS_LandFast=landCfg==2||(landCfg==1&&act);}
 if(mode&&gpuOk==1){extern int P3DS_GpuBotSame;P3DS_GpuBotSame=(power&0x8000)&&P3DS_BottomReused;/* run 41: !fpsOn removed (FPS text is drawn on the top screen; it blocked every skip) */P3DS_GpuPresent(stereo?eyeL:top,bot,mode,!P3DS_ConsoleOnBottom(),stereo?eyeR:nullptr);}
 else{present(GFX_TOP,400,top,0);if(!P3DS_ConsoleOnBottom())present(GFX_BOTTOM,320,bot,0);gfxFlushBuffers();gfxSwapBuffers();}
 P3DS_GameFramePresented();
 u64 tEnd=svcGetSystemTick();last2DUs=(unsigned)((tEnd-t0)/CPU_TICKS_PER_USEC);frames++;
 {static unsigned bl;if(++bl%600==0){PSPNativeMemLog("[2D] bottom_skip: reused %u, recomposed %u of last 600 | steal_b: engine B on main %u, worker %u | early_b %u | split_top %u",bSkips,bDraws,stealMain,stealWorker,earlyN,splitN);earlyN=0;splitN=0;bSkips=bDraws=0;stealMain=stealWorker=0;}}
 {unsigned total=tPrevEnd?(unsigned)((tEnd-tPrevEnd)/CPU_TICKS_PER_USEC):0,comp=(unsigned)((tComp-t0)/CPU_TICKS_PER_USEC),gpu=(unsigned)((tEnd-tComp)/CPU_TICKS_PER_USEC);
  {/* run 43 [SPIKE] diag: any frame > spike_ms (default 70, 0 = off) logs where the time went (max 60 lines / session).
     g3soft time includes the game's own CPU work between frame start and the 3D flush (loads, decompression). */
   static int spikeMs=-1;static unsigned spikeN;if(spikeMs<0){spikeMs=70;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"spike_ms=%d",&x)==1)spikeMs=x;fclose(f);}PSPNativeMemLog("[SPIKE] spike_ms=%d",spikeMs);}
   if(spikeMs>0&&total>(unsigned)spikeMs*1000&&spikeN<60){spikeN++;struct mallinfo mi=mallinfo();
    PSPNativeMemLog("[SPIKE] f=%u frame %u ms = g3soft+game %u + composite %u + present %u | romfs %u ms (%u x16K misses) | GetTexture %u ms | GPU tex create %u ms (%u) | tris %u | malloc %u KiB",
     frames,total/1000,g3us/1000,comp/1000,gpu/1000,(unsigned)(P3DS_SpkRomTicks/CPU_TICKS_PER_MSEC),P3DS_SpkRomBlocks,(unsigned)(P3DS_SpkGetTexTicks/CPU_TICKS_PER_MSEC),(unsigned)(P3DS_SpkTexTicks/CPU_TICKS_PER_MSEC),P3DS_SpkTexN,P3DS_G3LastTris(),(unsigned)(mi.uordblks/1024));}
   P3DS_SpkRomTicks=P3DS_SpkTexTicks=P3DS_SpkGetTexTicks=0;P3DS_SpkRomBlocks=P3DS_SpkTexN=0;}
  secUp+=P3DS_UploadUs;secConv+=G3G_ConvUs;
  {static unsigned w3;if(total>w3)w3=total;P3DS_Perf3Worst=w3;if(secFrames==0)w3=total;}tPrevEnd=tEnd;secFrames++;secG3+=g3us;secComp+=comp;secGpu+=gpu;secGame+=total>comp+gpu+g3us?total-comp-gpu-g3us:0;secTris+=P3DS_G3LastTris();secG3G+=G3G_LastUs();
  if(!secStart)secStart=tEnd;unsigned el=(unsigned)((tEnd-secStart)/CPU_TICKS_PER_USEC);
  if(el>=1000000){unsigned n=secFrames;fpsShown=(unsigned)((u64)n*1000000/el);msShown=(unsigned)((u64)el/n/100);
   struct mallinfo mi=mallinfo();
   PSPNativeMemLogC("[PERF] fps %u | ms/frame %u.%u = game %u.%u + g3soft %u.%u + composite %u.%u + present %u.%u | tris/frame %u | malloc used %u KiB | mode %d slider %.2f | res3d %d, 3D GPU draw+readback %u us",
    fpsShown,msShown/10,msShown%10,secGame/n/1000,(secGame/n/100)%10,secG3/n/1000,(secG3/n/100)%10,secComp/n/1000,(secComp/n/100)%10,secGpu/n/1000,(secGpu/n/100)%10,secTris/n,(unsigned)(mi.uordblks/1024),P3DS_ScaleMode(),(double)p3dsSlider(),G3G_Hi()?2:0,secG3G/n);
   PSPNativeMemLogC("[PERF2] upload %u us | 3D convert %u us | composite %u us",secUp/n,secConv/n,secComp/n);secUp=secConv=0;
   {static unsigned lS,lD;PSPNativeMemLogC("[PERF3] worst frame %u ms | bottom reused %u drawn %u | map %d field %d",P3DS_Perf3Worst/1000,bSkips-lS,bDraws-lD,P3DS_CurMap,P3DS_InField);lS=bSkips;lD=bDraws;}
   secStart=tEnd;secFrames=secG3=secComp=secGpu=secGame=secTris=0;secG3G=0;}}
 /* run 66 settings_touch=1 (default): overworld touch shortcuts in the Poketch's dead corners (no Poketch screen or
    button there: DS screen area 16..207 x 16..175, buttons x 224..255 y 32..160). Touch and hold 0.3 s:
      top-left  = settings app (same as L+R+Y)      top-right    = Pokemon (party, the game's real screen)
      bottom-right = Bag (the game's real screen, via the real Start menu - save-safe, same as the Menu tab)
    The hold must start in the corner and stay there; anything else is a normal touch for the game. */
 static int stCfg=-1;if(stCfg<0){stCfg=P3DS_CfgInt("settings_touch",1)!=0;PSPNativeMemLog("[APP] settings_touch=%d",stCfg);}
 bool cornerOpen=false;
 if(stripReq==1||stripReq==3||stripReq==4){const int want=stripReq==1?-1:stripReq==3?-3:0;if(want)for(int t=0;t<(int)(sizeof kTabs/sizeof kTabs[0]);t++)if(kTabs[t].n==want)appStartTab=t;cornerOpen=true;stripReq=0;}
 if(stCfg&&P3DS_InField&&appCfg()&&menuCfg()&&!P3DS_ConsoleOnBottom()){static u64 cT;static int cZone;static bool cPrev;u32 kh=hidKeysHeld();const bool tNow=(kh&KEY_TOUCH)!=0,tDown=tNow&&!cPrev;cPrev=tNow;
  if(tNow){touchPosition tp;hidTouchRead(&tp);int x,y,z=0;
   if(touchToDS(tp.px,tp.py,&x,&y)){if(x<16&&y<16)z=1;else if(x>=224&&y<32)z=2;else if(x>=224&&y>=161)z=3;else if(x<16&&y>=161&&P3DS_CfgInt("touch_menu",0))z=4;}
   else{if(tp.px<160&&tp.py<120)z=1;else if(tp.px>=160&&tp.py<120)z=2;else if(tp.px>=160)z=3;else if(P3DS_CfgInt("touch_menu",0))z=4;} /* 1:1 mode: the black border around the DS screen */
   if(tDown){cZone=z;cT=svcGetSystemTick();}else if(z!=cZone)cZone=0;
   if(cZone&&svcGetSystemTick()-cT>CPU_TICKS_PER_MSEC*300){const int zz=cZone;cZone=0;
    if(zz==1){cornerOpen=true;PSPNativeMemLog("[APP] opened by corner touch");}
    else if(zz==4){cornerOpen=true;for(int t=0;t<(int)(sizeof kTabs/sizeof kTabs[0]);t++)if(kTabs[t].n==-1)appStartTab=t;PSPNativeMemLog("[APP] touch_menu: quick menu opened");} /* run 77 touch_menu */
    else{extern volatile int P3DS_StartReq;P3DS_StartReq=zz==2?1:2;PSPNativeMemLog("[APP] corner touch: open %s",zz==2?"Pokemon":"Bag");}}}
  else cZone=0;}
 {u32 kd=hidKeysDown(),kh=hidKeysHeld();bool catchY=(kd&KEY_Y)&&!(kh&(KEY_L|KEY_R|KEY_X))&&P3DS_CatchInfo[0]&&!P3DS_InField&&appCfg()&&P3DS_CfgInt("catch_overlay",0);if(catchY){for(int t=0;t<(int)(sizeof kTabs/sizeof kTabs[0]);t++)if(kTabs[t].n==-3)appStartTab=t;PSPNativeMemLog("[CATCH] Y: catch info opened");}
  if(catchY||cornerOpen||((kd&KEY_Y)&&(kh&KEY_L)&&(kh&KEY_R)&&!(kh&KEY_X)&&menuCfg())){if(appCfg()&&!P3DS_ConsoleOnBottom())runApp(stereo?eyeL:top,mode,mode&&gpuOk==1);else runMenu(stereo?eyeL:top,bot,mode,mode&&gpuOk==1,!P3DS_ConsoleOnBottom());}}
 return 0;
}
extern "C" void PSPNativeRenderShutdown(){initialized=false;}
extern "C" unsigned PSPNativeRenderTestPixel(unsigned e,unsigned x,unsigned y){allocBufs();return raw[e&1][(y%192)*256+(x%256)];}
extern "C" void PSPNativeRenderGetTimings(unsigned*r,unsigned*s){if(r)*r=lastReadbackUs;if(s)*s=last2DUs;}
extern "C" unsigned RenderStage(unsigned stage){return stage==0?bindUs:stage==1?drawUs:convertUs;}
extern "C" int PSPNativeRenderPresent(){return Present(true);}
extern "C" int PSPNativeRenderPresentNoWait(){return Present(false);}

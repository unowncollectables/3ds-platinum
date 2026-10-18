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
static void engineBWorker(void*){for(;;){LightEvent_Wait(&bStart);if(bQuit)break;
  if(bEarly){drawEngineB();bEarly=false;LightEvent_Signal(&bEarlyDone);continue;}
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
  LightEvent_Signal(&bDone);}}
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
extern "C" int P3DS_GpuComposeCfg(void){if(gcCfgV<0){gcCfgV=P3DS_CfgInt("gpu_compose",0);if(gcCfgV<0||gcCfgV>3)gcCfgV=0;PSPNativeMemLog("[GPUC] gpu_compose=%d (1 = analyzer, 2 = GPU compositor on simple frames, 3 = verify)",gcCfgV);}return gcCfgV;}
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
static const struct{char c;unsigned short b;}kLet[]={{'S',0x79CF},{'A',0x2BED},{'V',0x5B6A},{'I',0x7497},{'N',0x6B6D},{'G',0x396B},{'E',0x79A7},{'D',0x6B6E},{'!',0x2482},{'?',0x7282},{'H',0x5BED},{'R',0x6BAD},{'P',0x6BA4},{'O',0x7B6F},{'F',0x79A4},{'W',0x5B7D},{'L',0x4927},{'C',0x3923},{'T',0x7492},{'X',0x5AAD},{'M',0x5FED},{'U',0x5B6F},{'K',0x5BAD},{'Y',0x5A92},{'/',0x12A4},{':',0x0410},{'-',0x01C0},{'.',0x0002},{'B',0x6BAE},{'>',0x4454},{'Z',0x72A7},{'<',0x1511},{'(',0x2922},{')',0x224A},{'+',0x05D0}};
static void drawBig(u32*img,int x,int y,const char*t,u32 col){for(;*t;t++,x+=8){unsigned short b=0;for(auto&k:kLet)if(k.c==*t)b=k.b;if(*t>='0'&&*t<='9')b=kFont[*t-'0'];
  for(int yy=0;yy<10;yy++)for(int xx=0;xx<6;xx++){if(!((b>>(14-((yy/2)*3+xx/2)))&1))continue;int px=x+xx,py=y+yy;if(px>=0&&px<256&&py>=0&&py<192)img[py*256+px]=col;}}}
/* run 48: hotkey toast. Any port hotkey that changes a setting shows a short box at the top of the top screen
   (e.g. "SHARP 3D ON", "WIDE VIEW OFF") for 1.5 s. */
static char toastText[24];static volatile u64 toastAt;
extern "C" void P3DS_Toast(const char*t){snprintf(toastText,sizeof toastText,"%s",t);toastAt=svcGetSystemTick();PSPNativeMemLog("[TOAST] %s",t);}
static void drawToast(u32*img,u64 now){if(!toastAt||now-toastAt>(u64)(1500*CPU_TICKS_PER_MSEC))return;
 const int tw=(int)strlen(toastText)*8,bw=tw+12,bh=20,bx=128-bw/2,by=6;
 for(int y=by;y<by+bh;y++)for(int x=bx;x<bx+bw;x++){if(x<0||x>255)continue;bool edge=y==by||y==by+bh-1||x==bx||x==bx+bw-1||y==by+1||y==by+bh-2||x==bx+1||x==bx+bw-2;img[y*256+x]=edge?0x00F0F0F0u:0x00602814u;}
 drawBig(img,bx+6,by+5,toastText,0x00FFFFFFu);}
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
 const int bw=saving?70:56,bh=16,bx=252-bw,by=172;
 for(int y=by;y<by+bh;y++)for(int x=bx;x<bx+bw;x++){bool edge=y==by||y==by+bh-1||x==bx||x==bx+bw-1;img[y*256+x]=edge?0x00F0F0F0u:0x00602814u;}
 if(saving){drawBig(img,bx+4,by+3,"SAVING",0x00FFFFFFu);
  static const signed char dx[8]={0,3,4,3,0,-3,-4,-3},dy[8]={-4,-3,0,3,4,3,0,-3};unsigned ph=(unsigned)((now/ms)/90)%8;int cx=bx+bw-9,cy=by+8;
  for(int i=0;i<8;i++){u32 c=i==(int)ph?0x003030FFu:((i+1)%8==(int)ph?0x002020A0u:0x00808080u);/* red head (R low byte) + trail */
   for(int yy=0;yy<2;yy++)for(int xx=0;xx<2;xx++)img[(cy+dy[i]+yy)*256+cx+dx[i]+xx]=c;}}
 else drawBig(img,bx+4,by+3,"SAVED!",0x0060FF60u);}
static bool fpsOn=true;static unsigned fpsShown=0,msShown=0;static int wideCfg=-1;/* run 55: file scope (settings menu) */
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
/* run 57 stereo_strength (menu "3D STRENGTH"): 0 = 1x (original, 6 DS px per eye at full slider), 1 = 1.5x, 2 = 2x.
   Scales the per-vertex parallax of the whole 3D world (overworld billboards/NPCs are 3D, so they follow) and the
   stereo_2d battle BG offsets. Zero cost (one multiply per frame). */
static int stereoStr=-1;
static int stereoShiftF(int mode){ if(!mode)return 0; if(stereoStr<0){stereoStr=P3DS_CfgInt("stereo_strength",0);if(stereoStr<0||stereoStr>2)stereoStr=0;PSPNativeMemLog("[STEREO] stereo_strength=%d",stereoStr);}
 float s=osGet3DSliderState(); return (int)(s*6.0f*(stereoStr==2?2.0f:stereoStr==1?1.5f:1.0f)+0.5f); }
extern "C" u32 *P3DS_G3RightEye(void);
extern "C" unsigned long long P3DS_TickNow(void){return svcGetSystemTick();}
extern "C" { int P3DS_InField; int P3DS_WideCull; extern int G3G_Wide,G3G_StripValid; extern u32 G3G_Strip[2][64*192]; extern const u32 *P3DS_WideStrip[2]; }
extern "C" unsigned long long P3DS_SpkRomTicks,P3DS_SpkTexTicks,P3DS_SpkGetTexTicks;extern "C" unsigned P3DS_SpkRomBlocks,P3DS_SpkTexN;
/* per-vertex parallax amount for g3soft at frame start (DS px per eye); 0 = mono */
extern "C" int P3DS_StereoShiftNow(void){return ((reg16(0x304)&0x8000)&&(reg16(0x304)&2))?stereoShiftF(P3DS_ScaleMode()):0;}
static int stereoShift(int mode){ if(!mode)return 0; float s=osGet3DSliderState(); return (int)(s*4.0f+0.5f); }
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
struct MItem{const char*name;const char*key;int n;const char*lab[5];};
static const MItem mItems[]={
 {"FPS COUNTER","fps_counter",2,{"OFF","ON"}},{"WIDE VIEW","wideview",2,{"OFF","ON"}},{"SCALE","gpuscale",3,{"1:1","1.25X","FILL"}},
 {"C-STICK","cstick_mode",3,{"OFF","POKETCH","DPAD"}},{"ZL RUN TOGGLE","zl_run",2,{"OFF","ON"}},{"ZR = ITEM (Y)","zr_item",2,{"OFF","ON"}},
 {"CIRCLE PAD DPAD","cpad_dpad",2,{"OFF","ON"}},{"WIDE VIEW FX","wide_fx",2,{"OFF","ON"}},{"LOG DETAIL","log_level",2,{"SHORT","FULL"}},
 {"CAMERA ZOOM","cam_zoom",5,{"-2","-1","0","+1","+2"}},
 {"3D STRENGTH","stereo_strength",3,{"1X","1.5X","2X"}},
 {"FAST UPLOAD","gpu_swizzle",2,{"OFF","ON"}},{"FAST 3D CONVERT","fast_conv",2,{"OFF","ON"}},{"SPLIT TOP (MONO)","split_top",2,{"OFF","ON"}},
 {"FOLLOWING POKEMON","follower",2,{"OFF","ON"}},
 {"GPU COMPOSE","gpu_compose",4,{"OFF","ANALYZE","GPU","VERIFY"}},
};
#define MENU_N ((int)(sizeof mItems/sizeof mItems[0]))
static int menuGet(int i){switch(i){case 0:return fpsOn?1:0;case 1:return wideCfg>0?1:0;case 2:return P3DS_ScaleMode();case 3:return P3DS_CstickMode();
 case 4:return P3DS_CtlZl>0?1:0;case 5:return P3DS_CtlZr>0?1:0;case 6:return P3DS_CtlCp>0?1:0;case 7:return P3DS_WideFxCfg()?1:0;case 8:return P3DS_LogLevel()>=2?1:0;
 case 9:return P3DS_CamZoom()+2;case 10:return stereoStr<0?0:stereoStr;case 11:return P3DS_GpuSwizzle()?1:0;case 12:return G3G_FastConv()?1:0;case 13:return splitCfg>0?1:0;case 14:return P3DS_FollowerCfg()?1:0;case 15:return P3DS_GpuComposeCfg();}return 0;}
static void menuSet(int i,int v){switch(i){case 0:fpsOn=v!=0;break;case 1:wideCfg=v;break;case 2:P3DS_SetScaleMode(v);break;case 3:P3DS_SetCstickMode(v);break;
  case 4:P3DS_CtlZl=v;if(!v){extern volatile int P3DS_RunToggle;P3DS_RunToggle=0;}break;case 5:P3DS_CtlZr=v;break;case 6:P3DS_CtlCp=v;break;case 7:wideFxV=v;break;case 8:P3DS_SetLogLevel(v?2:1);break;
  case 9:P3DS_SetCamZoom(v-2);break;case 10:stereoStr=v;break;case 11:P3DS_SetGpuSwizzle(v);break;case 12:G3G_SetFastConv(v);break;case 13:splitCfg=v;break;case 14:P3DS_SetFollowerCfg(v);break;case 15:P3DS_SetGpuComposeCfg(v);break;}
 if(i!=2)P3DS_CfgWrite(mItems[i].key,i==8?(v?2:1):i==9?v-2:v);}
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
  drawBig(img,bx+8,by+6,"SETTINGS (PAUSED)",0x0060E0FFu);
  {char pg[16];snprintf(pg,sizeof pg,"%d/%d",sel+1,N);drawBig(img,bx+bw-8-(int)strlen(pg)*8,by+6,pg,0x00C0C0C0u);}
  for(int r=0;r<VIS&&top0+r<N;r++){const int i=top0+r;int y=by+22+r*13;if(i==sel)menuBox(img,bx+4,y-2,bx+bw-4,y+11,0x00A04820u,0x00A04820u);
   drawBig(img,bx+8,y,mItems[i].name,0x00FFFFFFu);
   char b[16];snprintf(b,sizeof b,"<%s>",mItems[i].lab[val[i]]);drawBig(img,bx+bw-8-(int)strlen(b)*8,y,b,i==sel?0x0000FFFFu:0x00C0C0C0u);}
  drawBig(img,bx+8,by+bh-14,"A/<> CHANGE  B CLOSE",0x00C0C0C0u);
  if(mode&&gpu)P3DS_GpuPresent(img,bot,mode,drawBot,nullptr);
  else{present(GFX_TOP,400,img,0);if(drawBot)present(GFX_BOTTOM,320,bot,0);gfxFlushBuffers();gfxSwapBuffers();}
  gspWaitForVBlank();
 }
 /* wait until buttons are released so the closing press doesn't reach the game */
 for(int i=0;i<120&&aptMainLoop();i++){hidScanInput();if(!(hidKeysHeld()&(KEY_A|KEY_B|KEY_START|KEY_Y|KEY_L|KEY_R|KEY_DUP|KEY_DDOWN|KEY_DLEFT|KEY_DRIGHT)))break;gspWaitForVBlank();}
 for(int i=0;i<N;i++)if(val[i]!=old[i])menuSet(i,val[i]);
 P3DS_MenuOpen=0;PSPNativeMemLog("[MENU] closed");
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
  bEarly=true;earlyB=true;earlyN++;LightEvent_Signal(&bStart);}
 G3SIM_FlushDeferred();openFrame=false;
 unsigned power=reg16(0x304),a=reg32(0);
 u64 t0=svcGetSystemTick();unsigned g3us=P3DS_G3LastUs();
 {u32 kd=hidKeysUp(),kh=hidKeysHeld();if((kd&KEY_START)&&(kh&KEY_L)&&(kh&KEY_R)&&!P3DS_ResetSeen){fpsOn=!fpsOn;/* run 53: on release, not after the soft-reset combo */P3DS_Toast(fpsOn?"FPS ON":"FPS OFF");}}
 if((reg32(0x64)&0x80000000u)||((a>>16)&3)==3){static int once;if(!once++)printf("[NATIVE-RENDER] display capture/FIFO unsupported (ignored)\n");}
 /* run 42 [STEREO] diag (stereo only): display capture is NOT implemented. Log (max 1 line / 15 frames) whenever
    capture is enabled, the display mode is not "normal", or engine A shows large OBJs (>=32x32, e.g. a Pokemon drawn as
    2D sprites): a 2D copy of something the 3D engine also draws gets zero disparity -> doubled image in 3D. */
 if(osGet3DSliderState()>0.0f){static unsigned lastF;unsigned cap=reg32(0x64),dm=(a>>16)&3;unsigned big=0;
  if(a&(1u<<12)){const u16*o=(const u16*)s_HW_OAM;for(int i=0;i<128;i++){u16 a0=o[i*4],a1=o[i*4+1];if((a0&0x300)==0x200)continue;unsigned shape=a0>>14,size=a1>>14;
    static const u8 W[3][4]={{8,16,32,64},{16,32,32,64},{8,8,16,32}},H[3][4]={{8,16,32,64},{8,8,16,32},{16,32,32,64}};if(shape<3&&W[shape][size]>=32&&H[shape][size]>=32)big++;}}
  if(((cap&0x80000000u)||dm!=1||big)&&frames-lastF>=15){lastF=frames;PSPNativeMemLog("[STEREO] f=%u dispcntA %08x mode %u capcnt %08x bigOBJ %u bg0cnt %04x 3Dpolys %u",frames,a,dm,cap,big,reg16(8),P3DS_G3LastTris());}}
 const bool excl=compExcl();s32 oldPrio=0x30;if(excl){svcGetThreadPriority(&oldPrio,CUR_THREAD_HANDLE);svcSetThreadPriority(CUR_THREAD_HANDLE,0x18);}
 if(P3DS_GpuComposeCfg()==1){gcAnalyze(0,P3DS_InField);gcAnalyze(1,P3DS_InField);}
 u32 hash0=stateHash();
 if(!earlyB){mapMemory();engineB.Enabled=(power&512)!=0;bindRegisters(engineB,0x1000);} /* early_b: already mapped; the worker may be reading it */
 engineA.Enabled=(power&2)!=0;bindRegisters(engineA,0);
 /* per-vertex stereo (g3soft rendered both eyes) if it was enabled at frame start; else flat-layer fallback */
 {static unsigned diagN;if(++diagN%120==0){unsigned n=0;for(unsigned i=0;i<256*192;i++)if(GPU3D::NativeFrame[i]>>24)n++;
   printf("[3DLAYER] dispcntA %08x powcnt %04x 3D px alpha>0 %u bg0cnt %04x blend %04x/%04x\n",reg32(0),power,n,reg16(8),reg16(0x50),reg16(0x52));}}
 u32*rightEye=P3DS_G3RightEye();
 int sd=rightEye?0:stereoShift(P3DS_ScaleMode());bool stereo=(rightEye||sd)&&(power&0x8000)&&engineA.Enabled;
 if(stereo&&!rightEye)shift3D(-sd);
 FrameStart(engineA);if(!earlyB)FrameStart(engineB);renderer.SetFramebuffer(raw[0],raw[1]);
 bool async=engineBAsync();
 static int par3d=-1;if(par3d<0){par3d=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");int x;char l[64];if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"stereo2d_par=%d",&x)==1)par3d=x!=0;fclose(f);}PSPNativeMemLog("[2D] stereo right eye on core 2: %d",par3d);}
 bool rightPar=async&&par3d&&stereo&&rightEye;
 if(rightPar)memcpy((void*)&engineA2,(const void*)&engineA,sizeof engineA);
 /* run 55 stereo_2d=1 (default): outside the overworld (battles, menus, title) the right eye's 2D background layers
    get depth by BG priority: priority 3 (back) = the far-3D disparity (2 x shift), priority 2 = 1.5 x, priorities 0-1
    (text boxes, menus, HUD) and sprites stay at the screen plane. Text-mode BGs only (affine/bitmap untouched).
    Overworld unchanged (its BG overlays sit in front of the 3D world). */
 if(rightPar&&!P3DS_InField){static int cfg=-1;if(cfg<0){cfg=P3DS_CfgInt("stereo_2d",1)!=0;PSPNativeMemLog("[STEREO] stereo_2d=%d",cfg);}
  if(cfg){const int sd=stereoShiftF(P3DS_ScaleMode());const unsigned bgm=engineA2.DispCnt&7;
   for(int i=1;i<4;i++){if(!(engineA2.DispCnt&(0x100<<i)))continue;const bool text=i<2||bgm==0||(i==2&&bgm==1);if(!text)continue;
    const int pr=engineA2.BGCnt[i]&3,d=pr==3?2*sd:pr==2?(3*sd)/2:0;if(d)engineA2.BGXPos[i]=(u16)((engineA2.BGXPos[i]-d)&0x1FF);}}}
 bRightSrc=rightPar?rightEye:nullptr;
 bPower=power;if(!async)P3DS_BottomReused=0;
 const bool onC1=rightPar&&!earlyB&&c1Ready();bSkipB=onC1||earlyB;
 if(onC1)LightEvent_Signal(&c1Start);
 if(stealCfg<0){stealCfg=0;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"steal_b=%d",&x)==1)stealCfg=x!=0;fclose(f);}PSPNativeMemLog("[2D] steal_b=%d",stealCfg);}
 bClaim=0;const bool stealNow=async&&stealCfg==1&&rightPar&&!onC1&&!earlyB;
 if(splitCfg<0){splitCfg=1;/* run 57: default 1 (Azahar: identical images overworld/battle/menus, mono composite -37%) */FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"split_top=%d",&x)==1)splitCfg=x!=0;fclose(f);}PSPNativeMemLog("[2D] split_top=%d",splitCfg);}
 const int gcMode=P3DS_GpuComposeCfg();if(gcMode>=2)gcFrames++;
 const bool gcOK=gcMode==2&&!stereo&&!rightPar&&engineA.Enabled&&gcEligible();
 const bool splitNow=!gcOK&&splitCfg&&async&&!stereo&&!rightPar&&!onC1&&!stealNow&&engineA.Enabled&&((a>>16)&3)==1&&!(reg32(0x64)&0x80000000u);
 if(splitNow){memcpy((void*)&engineAS,(const void*)&engineA,sizeof engineA);line0Done=false;bSplit=true;splitN++;}
 if(async)LightEvent_Signal(&bStart);
 const unsigned aEnd=splitNow?96:192;
 bool gcDone=false;
 if(gcOK){if(!async)for(unsigned line=0;line<192;line++){GPU::VCount=line;engineB.CheckWindows(line);renderer.DrawSprites(line,&engineB);renderer.DrawScanline(line,&engineB);}
  gcDone=gcCompose(display[0]);if(gcDone)gcUsed++;line0Done=true;}
 if(!gcDone)for(unsigned line=0;line<192;line++){if(line>=aEnd&&async)break;GPU::VCount=line;engineA.CheckWindows(line);renderer.DrawSprites(line,&engineA);renderer.DrawScanline(line,&engineA);if(!line)line0Done=true;
  if(!async&&!gcOK){engineB.CheckWindows(line);renderer.DrawSprites(line,&engineB);renderer.DrawScanline(line,&engineB);}}
 if(stealNow&&claimB()){stealMain++;drawEngineB();}
 if(async){if(excl){while(!LightEvent_TryWait(&bDone)){}}else LightEvent_Wait(&bDone);}
 if(onC1)LightEvent_Wait(&c1Done);
 if(earlyB)LightEvent_Wait(&bEarlyDone);
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
 if(stereo&&rightPar){eyeL=display[0];eyeR=rawR;} /* no copies: present reads both eyes in place */
 else if(stereo){memcpy(stereoL,display[0],sizeof stereoL);if(rightEye)memcpy(GPU3D::NativeFrame,rightEye,256*192*4);else shift3D(2*sd);FrameStart(engineA);
  for(unsigned line=0;line<192;line++){GPU::VCount=line;engineA.CheckWindows(line);renderer.DrawSprites(line,&engineA);renderer.DrawScanline(line,&engineA);}
  if(!rightEye)shift3D(-sd);eyeL=stereoL;eyeR=display[0];}
 /* run 55 stereo blob diag (stereo_blobdiag=1, default): every 60 stereo frames, 16x16 cells where one eye is black
    (rgb < 8) on > 60% of the cell and the other eye is not -> one log line; the first hit dumps both eyes and both 3D
    layers to sdmc:/3ds/platinum3ds/stereo_blob_{L,R,3dL,3dR}.bmp for the next report. */
 if(stereo&&eyeL&&eyeR){static int cfg=-1;static unsigned n,hits;static bool dumpedB;if(cfg<0){cfg=P3DS_CfgInt("stereo_blobdiag",1)!=0;}
  if(cfg&&++n%60==0&&hits<40){int bestN=0,bx=0,by=0,beye=0;
   for(int cy=0;cy<12;cy++)for(int cx=1;cx<15;cx++){int l=0,r=0;for(int y=cy*16;y<cy*16+16;y++)for(int x=cx*16;x<cx*16+16;x++){u32 a=eyeL[y*256+x]&0xF8F8F8u,b=eyeR[y*256+x]&0xF8F8F8u;if(!a&&b)l++;else if(a&&!b)r++;}
    if(l>bestN){bestN=l;bx=cx;by=cy;beye=0;}if(r>bestN){bestN=r;bx=cx;by=cy;beye=1;}}
   if(bestN>154){hits++;const u32*t3R=P3DS_G3RightEye();PSPNativeMemLog("[STEREO] blob: %s eye black-only cell x %d..%d y %d..%d (%d/256 px) | slider %.2f field %d dispcnt %08x",beye?"RIGHT":"LEFT",bx*16,bx*16+15,by*16,by*16+15,bestN,(double)osGet3DSliderState(),P3DS_InField,(unsigned)reg32(0));
    if(!dumpedB){dumpedB=true;dumpBmp("sdmc:/3ds/platinum3ds/stereo_blob_L.bmp",eyeL,false);dumpBmp("sdmc:/3ds/platinum3ds/stereo_blob_R.bmp",eyeR,false);
     if(rightPar)dumpBmp("sdmc:/3ds/platinum3ds/stereo_blob_3dL.bmp",GPU3D::NativeFrame,true);if(t3R)dumpBmp("sdmc:/3ds/platinum3ds/stereo_blob_3dR.bmp",t3R,true);}}}}
 /* POWCNT1 bit15: engine A on top screen. */
 const u32 *top=(power&0x8000)?display[0]:display[1],*bot=(power&0x8000)?display[1]:display[0];
 u64 tComp=svcGetSystemTick();
 /* run 54 wide_fx: before the save box / toast / fps overlay are drawn onto the centre */
 static bool fillReady;fillReady=false;
 if(wideCfg>0&&P3DS_ScaleMode()==1&&wideFillCfg()&&!(G3G_Wide&&G3G_StripValid>=1&&P3DS_InField)&&(power&0x8000)){wideFill(display[0]);fillReady=true;}
 P3DS_WideFxOn=0;if(wideFxCfg()&&G3G_Wide&&G3G_StripValid>=1&&P3DS_InField&&(power&0x8000)){u64 tw=svcGetSystemTick();wideFx(display[0],GPU3D::NativeFrame,G3G_StripValid>=2?2:1);P3DS_WideFxOn=G3G_StripValid>=2?2:1;
  {static u64 acc;static unsigned n;acc+=svcGetSystemTick()-tw;if(++n==600){PSPNativeMemLog("[WIDE] wide_fx avg %u us/frame",(unsigned)(acc/600/268));acc=0;n=0;}}}
 {u64 nowT=svcGetSystemTick();u32*tt=(u32*)(stereo?eyeL:top);drawSaveBox(tt,nowT);if(stereo)drawSaveBox((u32*)eyeR,nowT);drawToast(tt,nowT);if(stereo)drawToast((u32*)eyeR,nowT);}
 {static int fi;if(!fi){fi=1;fpsOn=P3DS_CfgInt("fps_counter",1)!=0;}}/* run 55: fps_counter (menu-saved) */
 if(fpsOn){char b[24];snprintf(b,sizeof b,"%u %u.%u",fpsShown,msShown/10,msShown%10);u32*tt=(u32*)(stereo?eyeL:top);drawText(tt,2,2,b);if(stereo)drawText((u32*)eyeR,2,2,b);}
 int mode=P3DS_ScaleMode();
 static int gpuOk=-1;if(mode&&gpuOk<0){gpuOk=P3DS_GpuInit();printf("[3DS] GPU scaling init %s\n",gpuOk?"ok":"FAILED (falling back to 1:1)");}
 {/* run 43 wideview (experimental, config wideview=1, default 0; L+R+X toggles): Hor+ side strips of the 3D world in
     the pillarbox (mode 1, overworld only, 3D on BG0). The flag drives the NEXT frame's 3D pass; the strips drawn now
     come from this frame's pass. Off = no extra work at all. */
  if(wideCfg<0){wideCfg=0;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"wideview=%d",&x)==1)wideCfg=x!=0;fclose(f);}PSPNativeMemLog("[WIDE] wideview=%d",wideCfg);}
  {u32 kd=hidKeysDown(),kh=hidKeysHeld();if((kd&KEY_X)&&(kh&KEY_L)&&(kh&KEY_R)&&!(kh&KEY_Y)){wideCfg=!wideCfg;PSPNativeMemLog("[WIDE] L+R+X: wideview=%d",wideCfg);P3DS_Toast(wideCfg?"WIDE VIEW ON":"WIDE VIEW OFF");}}
  const u32 dc=reg32(0);const bool act=wideCfg&&P3DS_InField&&mode==1&&gpuOk==1&&(dc&8)&&(dc&0x100)&&((dc>>16)&3)==1&&engineA.Enabled;
  {static int lastAct=-1;if((int)act!=lastAct){lastAct=act;PSPNativeMemLog("[WIDE] active=%d (cfg %d field %d mode %d 3Dbg0 %d)",(int)act,wideCfg,P3DS_InField,mode,(int)((dc&0x108)==0x108));}}
  P3DS_WideStrip[0]=act&&G3G_Wide&&G3G_StripValid>=1?G3G_Strip[0]:nullptr;P3DS_WideStrip[1]=act&&G3G_Wide&&G3G_StripValid>=2?G3G_Strip[1]:nullptr;
  if(!P3DS_WideStrip[0]&&fillReady&&gpuOk==1){P3DS_WideStrip[0]=P3DS_WideFillBuf;P3DS_WideStrip[1]=P3DS_WideFillBuf;P3DS_WideFxOn=1;}
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
  tPrevEnd=tEnd;secFrames++;secG3+=g3us;secComp+=comp;secGpu+=gpu;secGame+=total>comp+gpu+g3us?total-comp-gpu-g3us:0;secTris+=P3DS_G3LastTris();secG3G+=G3G_LastUs();
  if(!secStart)secStart=tEnd;unsigned el=(unsigned)((tEnd-secStart)/CPU_TICKS_PER_USEC);
  if(el>=1000000){unsigned n=secFrames;fpsShown=(unsigned)((u64)n*1000000/el);msShown=(unsigned)((u64)el/n/100);
   struct mallinfo mi=mallinfo();
   PSPNativeMemLogC("[PERF] fps %u | ms/frame %u.%u = game %u.%u + g3soft %u.%u + composite %u.%u + present %u.%u | tris/frame %u | malloc used %u KiB | mode %d slider %.2f | res3d %d, 3D GPU draw+readback %u us",
    fpsShown,msShown/10,msShown%10,secGame/n/1000,(secGame/n/100)%10,secG3/n/1000,(secG3/n/100)%10,secComp/n/1000,(secComp/n/100)%10,secGpu/n/1000,(secGpu/n/100)%10,secTris/n,(unsigned)(mi.uordblks/1024),P3DS_ScaleMode(),(double)osGet3DSliderState(),G3G_Hi()?2:0,secG3G/n);
   PSPNativeMemLogC("[PERF2] upload %u us | 3D convert %u us | composite %u us",secUp/n,secConv/n,secComp/n);secUp=secConv=0;
   secStart=tEnd;secFrames=secG3=secComp=secGpu=secGame=secTris=0;secG3G=0;}}
 {u32 kd=hidKeysDown(),kh=hidKeysHeld();if((kd&KEY_Y)&&(kh&KEY_L)&&(kh&KEY_R)&&!(kh&KEY_X)&&menuCfg())runMenu(stereo?eyeL:top,bot,mode,mode&&gpuOk==1,!P3DS_ConsoleOnBottom());}
 return 0;
}
extern "C" void PSPNativeRenderShutdown(){initialized=false;}
extern "C" unsigned PSPNativeRenderTestPixel(unsigned e,unsigned x,unsigned y){allocBufs();return raw[e&1][(y%192)*256+(x%256)];}
extern "C" void PSPNativeRenderGetTimings(unsigned*r,unsigned*s){if(r)*r=lastReadbackUs;if(s)*s=last2DUs;}
extern "C" unsigned RenderStage(unsigned stage){return stage==0?bindUs:stage==1?drawUs:convertUs;}
extern "C" int PSPNativeRenderPresent(){return Present(true);}
extern "C" int PSPNativeRenderPresentNoWait(){return Present(false);}

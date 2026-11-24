/* platinum3ds software G3 backend: rasterizes pspoke's screen-space GuVertex triangles into GPU3D::NativeFrame
 * (the 3D layer the melonDS 2D compositor blends as BG0), with a 16-bit depth buffer, DS texture formats
 * decoded from the emulated texture/palette VRAM, DS opaque-then-translucent ordering, colour/alpha modulate.
 * Same entry points as pspoke native-stack-render/g3_backend.cpp (and the old g3null.cpp). */
#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <cmath>
extern "C" unsigned long long svcGetSystemTick(void);
#include "crumbs.h"
#include <nitro.h>
#include <simulator/g3_draw.h>
#include <simulator/g3_handler.h>
#include "../native-stack-render/g3_vtx.h"
#include "../native-render-opt/native_gpu.h"
extern "C" u8 s_HW_LCDC_VRAM[0xA4000];
extern "C" unsigned char PSPNative_GfxRegisters[];
extern "C" unsigned PSPNativeVramGen;
extern "C" void PSPNativeMemLog(const char*,...);
extern "C" u8 s_SIM_transDepthWrite;
/* run 73: 6144 -> 24576. The Spear Pillar cutscene (overlay 100, distortion world) submits more than 6144 vertices in one frame -> "[NATIVE-RENDER] vertex overflow" abort. Run 73 fix. */
static GuVertex vertices[24576] __attribute__((aligned(16)));
GuVertex *g3ChunkPtr=vertices;unsigned g3ChunkLeft=24576;
unsigned g3RecCount=0;static bool recActive=false,recBroken=false;
static unsigned drawnTris;
/* diagnostics (logged with [G3SOFT]): why pixels are dropped, vertex ranges */
static unsigned stDecodes,stPix,stDepth,stTexA,stA0,stWritten,stTexTris,stTrans;static float stZmin=1e9f,stZmax=-1e9f,stXmin=1e9f,stXmax=-1e9f;static unsigned stAmin=255,stAmax=0,stFmt[8];static u32 stColOr,stColAnd=~0u;
extern "C" void abort(void);
extern "C" void G3SIM_ChunkAlloc(){printf("[NATIVE-RENDER] vertex overflow\n");abort();for(;;){}}
static void ResetVertices(){if(recActive&&g3RecCount)recBroken=true;g3ChunkPtr=vertices;g3ChunkLeft=24576;}
static bool cullAll=false;static u64 prTex,prRaw,prEnd,prN,prCull,prPair,prXform,prClip,prQueue,prFlush,prIdle,prCmd,prWake;static int cullFace=0;/*1=GL_FRONT 2=GL_BACK*/
extern "C" int P3DS_CfgInt(const char*,int);
extern "C" volatile unsigned long long P3DS_VblIdleTicks;
/* g3_prof (Debug tab, default OFF): every 60 3D frames, one [G3PROF] line for the WHOLE PERF g3soft window
   (Begin→FlushDeferred): idle (VBlank waits inside the window), front (game+G3 command decode/handler outside Draw),
   raw (GpuRaw), flush (FlushDeferred before End), end (G3G_End). Per-triangle timers are not used (they cost fps).
   g3_xfcache (Display tab, default OFF): replay draws whose clip matrix matches the last time they were
   transformed skip the matrix multiply and plane tests and feed the same kept triangles to the GPU path.
   g3_wake (Debug tab, default OFF): with g3_thread ON, wake the worker on every queued job (not every 6). Same picture.
   g3_thr_check (Debug tab, default OFF): with g3_thread ON, CRC the job POD at submit and again on the worker; log [G3THR] snap mismatch.
   g3_fastxform (Debug tab, default OFF): hoist the clip matrix into locals for the raw transform loop (same IEEE ops, less reload).
   g3_thr_texfence (Debug tab, default OFF): 0 = #488 depth-1, 1 = pin in-flight gts (GetTexture while busy),
   2 = DEEP: same pins AND skip TexCreate's gtWaitIdle (TexCreate0 only fills tex memory; worker only writes vb[]).
   Run 879 avg depth stayed ~1–2 mainly because every new GPU tex forced idle. Levels 0 and 1 unchanged.
   g3_thr_texsafe (Debug tab, default OFF): with g3_thread ON, do not TexUpdate in place after VBlank for textures this
   frame already queued (replace gt, keep old object for G3G_End). Also hold fence pins until after End. Fixes brief
   texture flashes while walking (run 879). Run 883: with texfence+texsafe, LRU/texFree must NOT gtWaitPinned
   (pins stay until End → deadlock); defer GPU free until after pin release. Wait loops drop the big intrlock,
   force-Signal, and time out → g3_thread OFF. OFF = old in-place post-vblank path.
   g3_stereo_share (Debug tab, default OFF): TriClip2 writes left only + (shR-shL); expand right after drain (or
   before any eye-1 write such as hard-clip). Bit-identical to dual TriClip2; cuts ~half the vb store traffic.
   stereoDsh is malloc (not linearAlloc): run 9bbfcd1 linearAlloc of ~192 KiB often failed → share silently OFF.
   With g3_thread, share is also published per GJob (stereoShare) + atomic SetStereoShare so the worker sees ON.
   Log [G3STER] 60f leftOnly/dual/expands — want leftOnly>>dual when ON. G3PROF "raw" is submit time when threaded;
   worker store cut shows as lower [G3THR] worker busy / wait, not always lower raw.
   g3_front_ov (Debug tab, default OFF): with g3_thread ON, wake every job and run post-VBlank GatherRaw/Decode
   into SIDE pixel buffers while waiting for the worker drain (never mutates t->px/gen/raw/useF until after drain).
   Log [FRONT]. Run 889: fence+pin freezes GetTexture for ALL thread paths (not only front_ov) so thr_check
   texCrc stays 0; CRC hashes live t->gen again (c2087a3 like-for-like). Safety ignores boot 0-tris frames.
   g3_stereo_gpu (Debug tab, default OFF): with share, FlushDeferred may skip CPU expand; End lazily inits g3s and
   draws the right eye from vb[0] (stereok * dsh * w). Prefers gpuRight (skips mid; hard-clip/translucent write
   left+dsh). CPU-expand fallback only when eye1 already filled / shader fail; 60f log includes reason counts.
   Default V/g3.v.pica untouched.
   g3_raw_split: live toggle drains then Start2/park worker 2; per-worker busy us in [G3THR] 300f line.
   g3_front_bg (Debug, default OFF): side-buffer prep during the worker drain.
   Host: tools/g3_thr_race_test.c, tools/g3_stereo_share_test.c, tools/g3_fastxform_test.c. */
static int g3ProfCfg=-1,g3XfCfg=-1,g3WakeCfg=-1,g3ThrCheckCfg=-1,g3FastXformCfg=-1,g3TexFenceCfg=-1,g3TexSafeCfg=-1,g3StereoShareCfg=-1,g3FrontOvCfg=-1,g3StereoGpuCfg=-1;
static int g3FrontBgCfg=-1;
/* Hot path for g3packet: 0 until g3_prof is read/on, so GeBuffer pays nothing when profiling is OFF. */
extern "C" volatile int P3DS_G3ProfLive;
volatile int P3DS_G3ProfLive;
extern "C" int P3DS_G3ProfCfg(void){if(g3ProfCfg<0){g3ProfCfg=P3DS_CfgInt("g3_prof",0)!=0;P3DS_G3ProfLive=g3ProfCfg;PSPNativeMemLog("[G3PROF] g3_prof=%d (1 = avg every 60 3D frames over the whole Begin→Flush window)",g3ProfCfg);}return g3ProfCfg;}
extern "C" void P3DS_SetG3Prof(int v){g3ProfCfg=v!=0;P3DS_G3ProfLive=g3ProfCfg;PSPNativeMemLog("[G3PROF] g3_prof=%d from settings",g3ProfCfg);}
extern "C" int P3DS_G3XfCacheCfg(void){if(g3XfCfg<0){g3XfCfg=P3DS_CfgInt("g3_xfcache",0)!=0;PSPNativeMemLog("[G3XF] g3_xfcache=%d (1 = reuse transformed list-cache draws when the matrix is unchanged)",g3XfCfg);}return g3XfCfg;}
extern "C" void P3DS_SetG3XfCache(int v); /* defined with xfClear below */
extern "C" int P3DS_G3WakeCfg(void){if(g3WakeCfg<0){g3WakeCfg=P3DS_CfgInt("g3_wake",0)!=0;PSPNativeMemLog("[G3THR] g3_wake=%d (1 = wake worker every job when g3_thread is ON)",g3WakeCfg);}return g3WakeCfg;}
extern "C" void P3DS_SetG3Wake(int v){g3WakeCfg=v!=0;PSPNativeMemLog("[G3THR] g3_wake=%d from settings",g3WakeCfg);}
extern "C" int P3DS_G3ThrCheckCfg(void){if(g3ThrCheckCfg<0){g3ThrCheckCfg=0;   /* 2026-11-17: always off (run 916: 0 mismatches in 17 checks; cloud profile: its CRCs were ~15% of the busy samples) */PSPNativeMemLog("[G3THR] g3_thr_check=%d (1 = CRC job POD at submit vs worker; logs mismatches)",g3ThrCheckCfg);}return g3ThrCheckCfg;}
extern "C" void P3DS_SetG3ThrCheck(int v){g3ThrCheckCfg=v!=0;PSPNativeMemLog("[G3THR] g3_thr_check=%d from settings",g3ThrCheckCfg);}
extern "C" int P3DS_G3FastXformCfg(void){if(g3FastXformCfg<0){g3FastXformCfg=P3DS_CfgInt("g3_fastxform",0)!=0;PSPNativeMemLog("[G3XF] g3_fastxform=%d (1 = hoist clip matrix in the raw transform loop; same picture)",g3FastXformCfg);}return g3FastXformCfg;}
extern "C" void P3DS_SetG3FastXform(int v){g3FastXformCfg=v!=0;PSPNativeMemLog("[G3XF] g3_fastxform=%d from settings",g3FastXformCfg);}
extern "C" int P3DS_G3TexFenceCfg(void){if(g3TexFenceCfg<0){int v=P3DS_CfgInt("g3_thr_texfence",0);g3TexFenceCfg=v<0?0:v>2?2:v;PSPNativeMemLog("[G3THR] g3_thr_texfence=%d (0 depth-1, 1 pin, 2 DEEP = pin + TexCreate without idle wait)",g3TexFenceCfg);}return g3TexFenceCfg;}
extern "C" void P3DS_SetG3TexFence(int v){g3TexFenceCfg=v<0?0:v>2?2:v;PSPNativeMemLog("[G3THR] g3_thr_texfence=%d from settings",g3TexFenceCfg);}
extern "C" int P3DS_G3TexSafeCfg(void){if(g3TexSafeCfg<0){g3TexSafeCfg=P3DS_CfgInt("g3_thr_texsafe",0)!=0;PSPNativeMemLog("[G3THR] g3_thr_texsafe=%d (1 = no in-place TexUpdate of queued gts; hold pins until End)",g3TexSafeCfg);}return g3TexSafeCfg;}
extern "C" void P3DS_SetG3TexSafe(int v){g3TexSafeCfg=v!=0;PSPNativeMemLog("[G3THR] g3_thr_texsafe=%d from settings",g3TexSafeCfg);}
extern "C" int P3DS_G3StereoShareCfg(void){if(g3StereoShareCfg<0){g3StereoShareCfg=P3DS_CfgInt("g3_stereo_share",0)!=0;PSPNativeMemLog("[G3STER] g3_stereo_share=%d (1 = left-only TriClip2 + expand right after drain; heap stereoDsh; bit-identical)",g3StereoShareCfg);}return g3StereoShareCfg;}
extern "C" void P3DS_SetG3StereoShare(int v){g3StereoShareCfg=v!=0;PSPNativeMemLog("[G3STER] g3_stereo_share=%d from settings",g3StereoShareCfg);}
extern "C" int P3DS_G3FrontOvCfg(void){if(g3FrontOvCfg<0){g3FrontOvCfg=P3DS_CfgInt("g3_front_ov",0)!=0;PSPNativeMemLog("[FRONT] g3_front_ov=%d (1 = wake every job + side-buffer postvblank decode during drain; no t->px race)",g3FrontOvCfg);}return g3FrontOvCfg;}
extern "C" void P3DS_SetG3FrontOv(int v){g3FrontOvCfg=v!=0;PSPNativeMemLog("[FRONT] g3_front_ov=%d from settings",g3FrontOvCfg);}
extern "C" int P3DS_G3StereoGpuCfg(void){if(g3StereoGpuCfg<0){g3StereoGpuCfg=P3DS_CfgInt("g3_stereo_gpu",0)!=0;PSPNativeMemLog("[G3STER] g3_stereo_gpu=%d (1 = with share, shader right eye; End inits g3s lazily; default V untouched)",g3StereoGpuCfg);}return g3StereoGpuCfg;}
extern "C" void P3DS_SetG3StereoGpu(int v){g3StereoGpuCfg=v!=0;PSPNativeMemLog("[G3STER] g3_stereo_gpu=%d from settings",g3StereoGpuCfg);}
/* Live apply defined with gtApplyRawSplit below (drain + Start2/Stop2). */
extern "C" void P3DS_SetG3RawSplit(int v);
extern "C" int P3DS_G3FrontBgCfg(void){if(g3FrontBgCfg<0){g3FrontBgCfg=P3DS_CfgInt("g3_front_bg",0)!=0;PSPNativeMemLog("[FRONT] g3_front_bg=%d (1 = side-buffer postvblank prep during drain without wake-every; OFF = front_ov-only)",g3FrontBgCfg);}return g3FrontBgCfg;}
extern "C" void P3DS_SetG3FrontBg(int v){g3FrontBgCfg=v!=0;PSPNativeMemLog("[FRONT] g3_front_bg=%d from settings",g3FrontBgCfg);}
static u64 idleAtBegin,prWall,prExpand;static unsigned prFrames,prTrisSum,prXfHitSum,prXfMissSum;
static unsigned frameHadGeom; /* Draw() saw geometry this 3D frame — front_ov safety net */
static unsigned postReplace,postInPlace; /* tex_postvblank: replace vs in-place counts */
static unsigned texCreateWait,texCreateSkip; /* fence DEEP: TexCreate idle waits skipped */
static int cull3dCfg(){static int v=-1;if(v<0){v=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"cull3d=%d",&x)==1)v=x;fclose(f);}PSPNativeMemLog("[G3] back-face culling cull3d=%d (0 off, 1 normal, 2 inverted winding)",v);}return v;}
/* ---------------- textures ---------------- */
static bool Gather(unsigned address,unsigned size,bool palette,u8*dst){
 while(size){unsigned segment=palette?0x4000:0x20000;unsigned slot=address/segment,within=address&(segment-1),n=segment-within;if(n>size)n=size;
  const u8*src[4];unsigned count=0;
  if(!palette){for(unsigned b=0;b<4;b++){unsigned c=PSPNative_GfxRegisters[0x240+b];if((c&0x87)==0x83&&((c>>3)&3)==slot)src[count++]=s_HW_LCDC_VRAM+b*0x20000+within;}}
  else{unsigned e=PSPNative_GfxRegisters[0x244];if((e&0x87)==0x83&&slot<4)src[count++]=s_HW_LCDC_VRAM+0x80000+slot*0x4000+within;
   for(unsigned b=0;b<2;b++){unsigned c=PSPNative_GfxRegisters[0x245+b],ofs=(c>>3)&3;if((c&0x87)==0x83&&((ofs&1)+((ofs&2)<<1))==slot)src[count++]=s_HW_LCDC_VRAM+0x90000+b*0x4000+within;}}
  if(!count)memset(dst,0,n);else if(count==1)memcpy(dst,src[0],n);else for(unsigned i=0;i<n;i++){u8 v=0;for(unsigned b=0;b<count;b++)v|=src[b][i];dst[i]=v;}
  dst+=n;address+=n;size-=n;}
 return true;
}
static inline u32 c555(u16 c,unsigned a){unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;return (r<<3|r>>2)|((g<<3|g>>2)<<8)|((b<<3|b>>2)<<16)|(a<<24);}
struct Tex{unsigned off,pal,fmt,w,h,c0,gen;u32*px;unsigned lru;u8*raw;unsigned rawN;void*gt;u64 cnt;unsigned useF;unsigned gpuGen;unsigned freeze;}; /* freeze: hold px/gen while gt pinned (fence) */
static bool gtInWorker=false;static int gtCfg=0;static void gtDrainIfBusy(int why);extern "C" {static void ListFreeNow(void*p);}static void*gtPendFree[512];static unsigned gtPendN;
static void gtPinRel(void*gt); /* defined with the fence pin table */
/* Pins released after G3G_End (not when the worker finishes GpuRawJ): batches still sample gt until End. */
enum{GT_PIN_DONE_N=1024};static void*gtPinDone[GT_PIN_DONE_N];static volatile unsigned gtPinDoneH,gtPinDoneT;
static void gtPinDonePush(void*gt){
 if(!gt)return;
 unsigned t=__atomic_load_n(&gtPinDoneT,__ATOMIC_RELAXED);
 gtPinDone[t&(GT_PIN_DONE_N-1)]=gt;
 __atomic_store_n(&gtPinDoneT,t+1,__ATOMIC_RELEASE);}
static void gtPinReleaseDone(void){
 unsigned h=__atomic_load_n(&gtPinDoneH,__ATOMIC_RELAXED);
 unsigned t=__atomic_load_n(&gtPinDoneT,__ATOMIC_ACQUIRE);
 while(h!=t){gtPinRel(gtPinDone[h&(GT_PIN_DONE_N-1)]);h++;}
 __atomic_store_n(&gtPinDoneH,h,__ATOMIC_RELAXED);}
extern "C" {int G3G_Init(void);void*G3G_TexCreate(const u32*,unsigned,unsigned);void G3G_TexFree(void*);void G3G_Begin(u32,unsigned,int);
 void G3G_Tri(int,const float*,const float*,const float*,const float*,const float*,const u32*,void*,unsigned,unsigned,int,int,int,int,int,int,int);
 void G3G_TriScreenShare(const float*,const float*,const float*,const float*,const float*,const u32*,const float*,void*,unsigned,unsigned,int,int,int,int,int,int,int);
 int G3G_StereoGpuLeftOnly(void);
 void G3G_End(u32*,u32*);void G3G_DirectResolve(void);void G3G_DirectResolveWhy(int);void G3G_DirectInvalidate(void);void G3G_TexUpdate(void*,const u32*,unsigned,unsigned);int G3G_RbSyncOn(void);void G3G_TriClip(int,const float*,const float*,const float*,const u32*,void*,int,int,int,int,int,int,int,const float*);void G3G_TriClip2(const float*,const float*,const float*,const u32*,void*,int,int,int,int,int,int,int,const float*,const float*);void G3G_SetStereoShare(int);void G3G_StereoExpand(void);void G3G_StereoShareStats(unsigned*,unsigned*,unsigned*,unsigned*);void G3G_StereoPathStats(unsigned*,unsigned*);void G3G_StereoPathWhy(unsigned*,unsigned);void G3G_SetStereoGpu(int);int G3G_StereoGpuOn(void);int G3G_StereoPending(void);unsigned G3G_LastUs(void);unsigned G3G_LastExpandUs(void);extern int G3G_TexFlip,G3G_FlipY,G3G_ZInv,G3G_Async,G3G_Wide,G3G_CurFog,G3G_FogNoW;}
/* g3_thr_texfence: refcount GPU tex pointers held by in-flight ring jobs. GetTexture must not free a pinned gt
   (that was the deep-queue white-water / ghost-billboard bug: LRU/re-decode freed a gt the worker still used). */
enum{GT_PIN_N=128};
static struct{void*gt;unsigned n;}gtPins[GT_PIN_N]; /* gt/n touched from game thread + worker → atomics */
static unsigned gtPinWaits,gtPinHits,gtInFlightMax,gtFenceDepthSum,gtFenceDepthN;
static unsigned gtPinGet(void*gt){
 if(!gt)return 0;
 for(unsigned i=0;i<GT_PIN_N;i++)if(__atomic_load_n(&gtPins[i].gt,__ATOMIC_ACQUIRE)==gt)return __atomic_load_n(&gtPins[i].n,__ATOMIC_ACQUIRE);
 return 0;}
static void gtPinAdd(void*gt){
 if(!gt)return;
 for(unsigned i=0;i<GT_PIN_N;i++)if(__atomic_load_n(&gtPins[i].gt,__ATOMIC_ACQUIRE)==gt){__atomic_fetch_add(&gtPins[i].n,1,__ATOMIC_ACQ_REL);gtPinHits++;return;}
 for(unsigned i=0;i<GT_PIN_N;i++){void*exp=nullptr;if(__atomic_compare_exchange_n(&gtPins[i].gt,&exp,gt,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)){__atomic_store_n(&gtPins[i].n,1,__ATOMIC_RELEASE);gtPinHits++;return;}}
 /* table full: drain so the worker releases pins, then retry once */
 gtPinWaits++;gtDrainIfBusy(3);
 for(unsigned i=0;i<GT_PIN_N;i++){void*exp=nullptr;if(__atomic_compare_exchange_n(&gtPins[i].gt,&exp,gt,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)){__atomic_store_n(&gtPins[i].n,1,__ATOMIC_RELEASE);gtPinHits++;return;}}}
static void gtPinRel(void*gt){
 if(!gt)return;
 for(unsigned i=0;i<GT_PIN_N;i++)if(__atomic_load_n(&gtPins[i].gt,__ATOMIC_ACQUIRE)==gt){
  if(__atomic_fetch_sub(&gtPins[i].n,1,__ATOMIC_ACQ_REL)==1)__atomic_store_n(&gtPins[i].gt,(void*)nullptr,__ATOMIC_RELEASE);
  return;}}
static void gtWaitPinned(void*gt); /* defined with the worker */
/* Run 883: with texsafe, pins stay until after End. texFree must not gtWaitPinned those — that deadlocks
   (drain empties the queue but pins remain; End never runs). Defer the free until pins are released. */
static void*gtTexDeferFree[512];static unsigned gtTexDeferN,gtTexDeferHits,gtWaitTimeouts,gtFlashRisk;
static void gtFlushTexDeferFree(void){
 for(unsigned i=0;i<gtTexDeferN;i++)G3G_TexFree(gtTexDeferFree[i]);gtTexDeferN=0;}
static void texFree(void*p){
 if(!p)return;
 if(gtCfg&&P3DS_G3TexFenceCfg()&&gtPinGet(p)){
  if(P3DS_G3TexSafeCfg()){
   /* Still pinned for End: queue the free; never wait here. */
   if(gtTexDeferN<512){gtTexDeferFree[gtTexDeferN++]=p;gtTexDeferHits++;return;}
   static unsigned ov;if(ov++<8)PSPNativeMemLog("[G3THR] tex defer-free full; forcing pin wait with timeout");
   gtWaitPinned(p);
  }else gtWaitPinned(p);
 }else if(!(gtCfg&&P3DS_G3TexFenceCfg()))gtDrainIfBusy(3);
 G3G_TexFree(p);}
static int gpuOn=-1,gpuReq=-1;
/* Source bytes (texels [+4x4 index data] + palette) of a texture; used to skip re-decoding when VRAM gen moved
   but this texture's bytes did not (outdoors the gen changes every frame). */
static u8 rawScratch[512*512*2+0x10000];
/* tex_pagecheck: skip the raw re-gather when no source 4 KB page (vram_dirty.c shims) was written since the entry
   was validated and VRAMCNT is unchanged. 0 off (default), 1 on, 2 verify (does both, logs mismatches). */
extern "C" unsigned PSPNativeVramPageGen[];
static unsigned PageGen(unsigned address,unsigned size,bool palette){unsigned g=0;
 while(size){unsigned segment=palette?0x4000:0x20000;unsigned slot=address/segment,within=address&(segment-1),n=segment-within;if(n>size)n=size;
  unsigned base[4];unsigned count=0;
  if(!palette){for(unsigned b=0;b<4;b++){unsigned c=PSPNative_GfxRegisters[0x240+b];if((c&0x87)==0x83&&((c>>3)&3)==slot)base[count++]=b*0x20000+within;}}
  else{unsigned e=PSPNative_GfxRegisters[0x244];if((e&0x87)==0x83&&slot<4)base[count++]=0x80000+slot*0x4000+within;
   for(unsigned b=0;b<2;b++){unsigned c=PSPNative_GfxRegisters[0x245+b],ofs=(c>>3)&3;if((c&0x87)==0x83&&((ofs&1)+((ofs&2)<<1))==slot)base[count++]=0x90000+b*0x4000+within;}}
  for(unsigned b=0;b<count;b++)for(unsigned pg=base[b]>>12;pg<=(base[b]+n-1)>>12;pg++)if(PSPNativeVramPageGen[pg]>g)g=PSPNativeVramPageGen[pg];
  address+=n;size-=n;}
 return g;}
static unsigned TexPageGen(unsigned fmt,unsigned w,unsigned h,unsigned off,unsigned pal){
 unsigned n=w*h,tb=0,palN=0;switch(fmt){case 1:tb=n;palN=32;break;case 2:tb=n/4;palN=4;break;case 3:tb=n/2;palN=16;break;case 4:tb=n;palN=256;break;case 5:tb=n/4;break;case 6:tb=n;palN=8;break;case 7:tb=n*2;break;}
 unsigned g=PageGen(off,tb,false);
 if(fmt==5){unsigned ia=0x20000+((off&0x1FFFF)>>1)+((off>=0x40000)?0x10000:0);unsigned g2=PageGen(ia,(n/16)*2,false);if(g2>g)g=g2;}
 if(palN){unsigned g2=PageGen(pal,palN*2,true);if(g2>g)g=g2;}
 return g;}
static u64 VramCnt(){u64 v=0;for(int i=0;i<7;i++)v|=(u64)PSPNative_GfxRegisters[0x240+i]<<(8*i);return v;}
static int texPageCfg(){static int v=-1;if(v<0){v=0;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"tex_pagecheck=%d",&x)==1)v=x;fclose(f);}PSPNativeMemLog("[G3] tex_pagecheck=%d",v);}return v;}
static unsigned texPageSkips,texPageMismatch;
static unsigned GatherRaw(unsigned fmt,unsigned w,unsigned h,unsigned off,unsigned pal){
 unsigned n=w*h,tb=0,palN=0;switch(fmt){case 1:tb=n;palN=32;break;case 2:tb=n/4;palN=4;break;case 3:tb=n/2;palN=16;break;case 4:tb=n;palN=256;break;case 5:tb=n/4;break;case 6:tb=n;palN=8;break;case 7:tb=n*2;break;}
 if(tb>sizeof rawScratch-0x10000)return 0;Gather(off,tb,false,rawScratch);unsigned k=tb;
 if(fmt==5){unsigned ia=0x20000+((off&0x1FFFF)>>1)+((off>=0x40000)?0x10000:0);Gather(ia,(n/16)*2,false,rawScratch+k);k+=(n/16)*2;}
 if(palN){Gather(pal,palN*2,true,rawScratch+k);k+=palN*2;}
 return k;}
static Tex texCache[96];static unsigned texN,texTick;
static u8 scratch[512*512*2+0x10000];static u16 palbuf[256];
static Tex*GetTextureImpl();
extern "C" { unsigned long long P3DS_SpkGetTexTicks; }
/* run 54 tex_postvblank=1 (default): the DS rasterises a frame's geometry AFTER that frame's VBlank VRAM transfers
   (sprite frames, new billboards), but the port resolves textures while the game submits geometry (before VBlank).
   A just-created billboard (bike mount) was therefore drawn with its freshly allocated, not yet transferred VRAM
   slot -> one frame of stale texels/palette (the "little yellow box"); sprite animation frames lagged one frame.
   Every texture used this frame is re-checked at G3SIM_FlushDeferred (after the VBlank callback, before the GPU
   draws): if any of its VRAM pages were written since it was decoded and the bytes really differ, it is re-decoded.
   Without g3_thread (or with g3_thr_texsafe OFF): GPU texture is updated IN PLACE (same object queued draws
   reference). With g3_thread + g3_thr_texsafe: upload a NEW gt and pendFree the old so this End still samples
   submit-time texels (in-place was the run-879 walking flash). GPU sync path only. */
static unsigned g3FrameNo;static Tex*usedTex[96];static unsigned usedN,postFixes;
static int postVblCfg(){static int v=-1;if(v<0){v=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"tex_postvblank=%d",&x)==1)v=x!=0;fclose(f);}PSPNativeMemLog("[G3] tex_postvblank=%d",v);}return v;}
extern "C" int P3DS_CfgInt(const char*,int);
static Tex folTex[8*8]; /* run 73: slot 0 follower, slots 1..6 visible wild Pokemon (ss3ds 0110: 6 at once; 8 frames each), 7 = ambient flyer (0217) */
static Tex*GetTexture(){u64 t=svcGetSystemTick();Tex*r=GetTextureImpl();if(r&&(r<folTex||r>=folTex+8*8)&&r->useF!=g3FrameNo+1){r->useF=g3FrameNo+1;if(usedN<96)usedTex[usedN++]=r;}u64 d=svcGetSystemTick()-t;prTex+=d;P3DS_SpkGetTexTicks+=d;return r;}
/* Decode into an explicit pixel buffer. front_ov prep uses a side buffer so t->px is never rewritten while the
   worker may still CRC/sample the submit-time image (a61df82: 71 texCrc mismatches with in-place DecodeTex). */
static bool DecodeTexTo(Tex*e,u32*o){if(!e||!o)return false;const unsigned fmt=e->fmt,w=e->w,h=e->h;
   stDecodes++;
   unsigned n=w*h;unsigned palN=0;
   switch(fmt){case 1:case 6:palN=fmt==1?32:8;break;case 2:palN=4;break;case 3:palN=16;break;case 4:palN=256;break;case 5:palN=0;break;default:break;}
   unsigned palAddr=e->pal;/* libntr already converts PLTT_BASE to bytes (<<3 for 4-colour, <<4 otherwise) */
   if(palN)Gather(palAddr,palN*2,true,(u8*)palbuf);
   bool c0=e->c0;
   switch(fmt){
   case 2:{Gather(e->off,n/4,false,scratch);for(unsigned i=0;i<n;i++){unsigned ix=(scratch[i>>2]>>((i&3)*2))&3;o[i]=(c0&&!ix)?0:c555(palbuf[ix],255);}break;}
   case 3:{Gather(e->off,n/2,false,scratch);for(unsigned i=0;i<n;i++){unsigned ix=(scratch[i>>1]>>((i&1)*4))&15;o[i]=(c0&&!ix)?0:c555(palbuf[ix],255);}break;}
   case 4:{Gather(e->off,n,false,scratch);for(unsigned i=0;i<n;i++){unsigned ix=scratch[i];o[i]=(c0&&!ix)?0:c555(palbuf[ix],255);}break;}
   case 1:{Gather(e->off,n,false,scratch);for(unsigned i=0;i<n;i++){unsigned v=scratch[i],a=v>>5;a=(a<<2)+(a>>1);o[i]=c555(palbuf[v&31],a*255/31);}break;}
   case 6:{Gather(e->off,n,false,scratch);for(unsigned i=0;i<n;i++){unsigned v=scratch[i],a=v>>3;o[i]=c555(palbuf[v&7],a*255/31);}break;}
   case 7:{Gather(e->off,n*2,false,scratch);const u16*s=(const u16*)scratch;for(unsigned i=0;i<n;i++)o[i]=c555(s[i],(s[i]&0x8000)?255:0);break;}
   case 5:{ /* 4x4 compressed: texels in slot 0/2, index data in slot 1 at (off&0x1FFFF)/2 (+0x10000 for slot 2) */
     Gather(e->off,n/4,false,scratch);unsigned off=e->off;unsigned idxAddr=0x20000+((off&0x1FFFF)>>1)+((off>=0x40000)?0x10000:0);
     u16*idx=(u16*)(scratch+n/4+16);Gather(idxAddr,(n/16)*2,false,(u8*)idx);
     static u16 big[0x8000];unsigned bw=w/4;
     for(unsigned by=0;by<h/4;by++)for(unsigned bx=0;bx<bw;bx++){unsigned bi=by*bw+bx;u32 bits=((u32*)scratch)[bi];u16 d=idx[bi];unsigned pa=e->pal+(d&0x3FFF)*4,mode=d>>14;
      u16 pc[4];Gather(pa,8,true,(u8*)pc);u32 col[4];col[0]=c555(pc[0],255);col[1]=c555(pc[1],255);
      auto mix=[&](u32 a,u32 b,unsigned wa,unsigned wb,unsigned dv){u32 r=0;for(int k=0;k<24;k+=8)r|=((((a>>k)&255)*wa+((b>>k)&255)*wb)/dv)<<k;return r|0xFF000000u;};
      switch(mode){case 0:col[2]=c555(pc[2],255);col[3]=0;break;case 1:col[2]=mix(col[0],col[1],1,1,2);col[3]=0;break;case 2:col[2]=c555(pc[2],255);col[3]=c555(pc[3],255);break;default:col[2]=mix(col[0],col[1],5,3,8);col[3]=mix(col[0],col[1],3,5,8);}
      for(unsigned k=0;k<16;k++)o[(by*4+k/4)*w+bx*4+(k&3)]=col[(bits>>(k*2))&3];}
     (void)big;break;}
   default:for(unsigned i=0;i<n;i++)o[i]=0xFFFFFFFFu;}
   return true;}
static bool DecodeTex(Tex*e){unsigned n=e->w*e->h;if(!e->px)e->px=(u32*)malloc(n*4);if(!e->px)return false;return DecodeTexTo(e,e->px);}
/* run 58: following Pokemon (render-only, never a map object). The field hook (overlay005 fieldmap.c) draws one
   quad whose TEXIMAGE_PARAM address is the magic 0x7FFF8 (a 4bpp texture there would run past the end of texture
   VRAM, so no real draw can use it) and whose PLTT_BASE low bits pick one of 8 frame slots. GetTextureImpl hands
   back a port-owned texture decoded from sdmc:/3ds/platinum3ds/followers.bin (HGSS sprites extracted from the
   user's own SoulSilver ROM). Missing file -> P3DS_FollowerOpen()==0 -> the hook draws nothing. */
static int folState=-1;static u32 folNSpr,folNSpec;static u8*folSpec,*folDir;static int folCur=-1,folCurShiny=-1;
#define FOL_PATH "sdmc:/3ds/platinum3ds/followers.bin"
/* ss3ds 0110: missing -> built once from the ROM on the card (p3ds_gamefont.c P3DS_FollowersBuild, ~2.7 MB, a few seconds) */
extern "C" int P3DS_FollowersBuild(const char*out);
extern "C" int P3DS_FollowerOpen(void){
 if(folState>=0)return folState;folState=0;FILE*f=fopen(FOL_PATH,"rb");if(!f&&P3DS_FollowersBuild(FOL_PATH))f=fopen(FOL_PATH,"rb");if(!f){PSPNativeMemLog("[FOL] %s missing and could not be built from the ROM -> sprites off",FOL_PATH);return 0;}
 u32 h[4];if(fread(h,4,4,f)==4&&h[0]==0x4C463350u&&h[1]==1&&h[2]&&h[2]<2048&&h[3]&&h[3]<1024){folNSpr=h[2];folNSpec=h[3];
  folSpec=(u8*)malloc(folNSpec*4);folDir=(u8*)malloc(folNSpr*8);
  if(folSpec&&folDir&&fread(folSpec,4,folNSpec,f)==folNSpec&&fread(folDir,8,folNSpr,f)==folNSpr)folState=1;}
 fclose(f);PSPNativeMemLog("[FOL] followers.bin %s: %u sprites %u species",folState?"ok":"BAD",(unsigned)folNSpr,(unsigned)folNSpec);return folState;}
extern "C" int P3DS_FollowerSprite(int species,int form,int female){
 if(folState!=1||species<=0||(u32)species>=folNSpec)return -1;const u8*e=folSpec+species*4;int idx=e[0]|e[1]<<8;
 if(e[3]){if(female)idx++;}else if(form>0&&form<=e[2])idx+=form;
 if(idx<0)idx=0;if((u32)idx>=folNSpr)idx=folNSpr-1;return idx;}
/* loads sprite spr (8 frames) into the frame slots; returns the frame width (32/64) or 0 */
static int folCurS[8]={-1,-1,-1,-1,-1,-1,-1,-1},folCurShS[8];
static int folLoadSlot(int sl,int spr,int shiny){
 if(folState!=1||spr<0||(u32)spr>=folNSpr)return 0;const u8*d=folDir+spr*8;unsigned w=d[4],h=d[5],nf=d[6];u32 off=d[0]|d[1]<<8|d[2]<<16|(u32)d[3]<<24;
 if(spr==folCurS[sl]&&shiny==folCurShS[sl])return w;
 if(nf!=8||(w!=32&&w!=64)||h!=w)return 0;unsigned fs=w*h/2,n=8*fs+64;u8*b=(u8*)malloc(n);if(!b)return 0;
 FILE*f=fopen(FOL_PATH,"rb");bool ok=f&&!fseek(f,off,SEEK_SET)&&fread(b,1,n,f)==n;if(f)fclose(f);
 if(!ok){free(b);PSPNativeMemLog("[FOL] read fail spr %d",spr);return 0;}
 const u8*pp=b+8*fs+(shiny?32:0);u16 pal[16];for(int i=0;i<16;i++)pal[i]=pp[i*2]|pp[i*2+1]<<8;
 // Replacing a slot mutates the CPU pixels the worker may still read.
 gtDrainIfBusy(3);
 for(int k=0;k<8;k++){Tex&t=folTex[sl*8+k];if(!t.px||t.w!=w){free(t.px);t.px=(u32*)malloc(w*h*4);}if(!t.px){free(b);return 0;}
  const u8*s=b+k*fs;for(unsigned i=0;i<w*h;i++){unsigned ix=(s[i>>1]>>((i&1)*4))&15;t.px[i]=ix?c555(pal[ix],255):0;}
  texFree(t.gt);t.gt=nullptr;t.off=0x7FFF8-sl*8;t.pal=0x1FF00+k*16;t.fmt=3;t.w=w;t.h=h;t.c0=1;t.gen=0;t.raw=nullptr;t.rawN=0;}
 free(b);folCurS[sl]=spr;folCurShS[sl]=shiny;if(!sl){folCur=spr;folCurShiny=shiny;}PSPNativeMemLog(sl?"[WILD] slot %d load sprite %d shiny %d %ux%u":"[FOL] slot %d sprite %d shiny %d %ux%u",sl,spr,shiny,w,h);return w;}
extern "C" int P3DS_FollowerLoad(int spr,int shiny){return folLoadSlot(0,spr,shiny);}
extern "C" int P3DS_WildLoad(int sl,int spr,int shiny){return sl>=1&&sl<=7?folLoadSlot(sl,spr,shiny):0;}
/* ss3ds 0110: visible wild Pokemon state, owned by the game hook (layout in fieldmap.c); here so it outlives the field overlay */
extern "C" void *P3DS_WildMemV2(void){static u32 m[144];return m;}
/* winFlag of the last battle, set by the game hook in encounter.c (sub_02050724); a visible one the player ran from stays */
extern "C" {volatile int P3DS_WildBattleResult=-1;}
extern "C" void P3DS_WildFree(int sl){
 if(sl<1||sl>7)return;
 // GPU pins defer the GPU free through End. Drain the jobs before freeing CPU pixels.
 gtDrainIfBusy(3);
 for(int k=0;k<8;k++){Tex&t=folTex[sl*8+k];texFree(t.gt);t.gt=nullptr;free(t.px);t.px=nullptr;}
 folCurS[sl]=-1;
}
static inline Tex*folLookup(const G3SIM_TexImageParam_t&p){if(p.textureOffset>0x7FFF8||p.textureOffset<0x7FFC0||p.textureFormat!=3||(s_texPlttBase&~0x70u)!=0x1FF00)return nullptr;Tex*t=&folTex[((0x7FFF8-p.textureOffset)/8)*8+((s_texPlttBase>>4)&7)];return t->px?t:nullptr;}
static Tex*GetTextureImpl(){
 const auto&p=s_texImageParam;unsigned fmt=p.textureFormat;if(!fmt)return nullptr;
 /* 2026-11-07: only a live follower/wild/flyer slot (format 3, its palette base, pixels loaded) takes this branch. #357 widened it to 0x7FFC0 and
    returned folLookup's nullptr for any other texture there: a real texture at the very top of texture VRAM (the battle grass / shiny sparkle
    8x8 at 0x7FFC0) then drew untextured, a big flat box in the wild battle intro and no shiny effect. Everything else uses the normal cache. */
 if(p.textureOffset>=0x7FFC0&&p.textureOffset<=0x7FFF8){Tex*ft=folLookup(p);if(ft)return ft;}
 unsigned w=p.textureSSize,h=p.textureTSize;if(w<8||h<8||w>1024||h>1024)return nullptr;
 for(unsigned i=0;i<texN;i++){Tex&t=texCache[i];if(t.off==p.textureOffset&&t.pal==s_texPlttBase&&t.fmt==fmt&&t.w==w&&t.h==h&&t.c0==p.color0){
   if(t.gen==PSPNativeVramGen){t.lru=++texTick;return &t;}
   /* Fence+pin: keep submit-time px/gen while an in-flight job still CRCs / samples this gt.
      Run 889: was front_ov-only; rare redecode under pin → texCrc with front_ov=0 (and after safety killed front_ov). */
   if(t.freeze||(gtCfg&&P3DS_G3TexFenceCfg()&&t.gt&&gtPinGet(t.gt))){
    t.freeze=1;t.lru=++texTick;return &t;}
   {int pc=texPageCfg();if(pc&&t.px&&t.cnt==VramCnt()&&TexPageGen(fmt,w,h,p.textureOffset,s_texPlttBase)<=t.gen){
     if(pc==1){t.gen=PSPNativeVramGen;t.lru=++texTick;texPageSkips++;return &t;}
     unsigned rn=GatherRaw(fmt,w,h,p.textureOffset,s_texPlttBase);if(!(rn&&t.raw&&t.rawN==rn&&!memcmp(t.raw,rawScratch,rn))){if(texPageMismatch++<20)PSPNativeMemLog("[G3] tex_pagecheck MISMATCH off %x fmt %u %ux%u",t.off,fmt,w,h);}else texPageSkips++;}}
   {unsigned rn=GatherRaw(fmt,w,h,p.textureOffset,s_texPlttBase);
    if(rn&&t.raw&&t.rawN==rn&&t.px&&!memcmp(t.raw,rawScratch,rn)){t.gen=PSPNativeVramGen;t.cnt=VramCnt();t.lru=++texTick;return &t;}
    free(t.raw);t.raw=rn?(u8*)malloc(rn):nullptr;t.rawN=t.raw?rn:0;if(t.raw)memcpy(t.raw,rawScratch,rn);}
   free(t.px);texFree(t.gt);t.gt=nullptr;t.px=nullptr;t.gen=PSPNativeVramGen;t.cnt=VramCnt();t.lru=++texTick;goto decode_into_i;}
  continue;
  decode_into_i:{Tex*e=&t;if(!DecodeTex(e))return nullptr;return e;}
 }
 /* new entry — with tex fence, never evict a slot whose gt is still pinned by an in-flight job.
    Run 883: texsafe keeps pins until End, so "drain then force LRU" + gtWaitPinned deadlocked. Prefer unpinned;
    if every slot is pinned, still evict LRU but texFree defers the GPU free (no wait). */
 {unsigned slot;if(texN<96)slot=texN++;else{
   slot=~0u;
   for(unsigned i=0;i<texN;i++)if(!gtPinGet(texCache[i].gt)&&(slot==~0u||texCache[i].lru<texCache[slot].lru))slot=i;
   if(slot==~0u){
    gtDrainIfBusy(3);
    for(unsigned i=0;i<texN;i++)if(!gtPinGet(texCache[i].gt)&&(slot==~0u||texCache[i].lru<texCache[slot].lru))slot=i;
    if(slot==~0u){slot=0;for(unsigned i=1;i<texN;i++)if(texCache[i].lru<texCache[slot].lru)slot=i;}}
   free(texCache[slot].px);free(texCache[slot].raw);texFree(texCache[slot].gt);}
  Tex&t=texCache[slot];t={p.textureOffset,s_texPlttBase,fmt,w,h,p.color0,PSPNativeVramGen-1u,nullptr,0,nullptr,0,nullptr};
  return GetTexture();}
}
/* ---------------- rasterizer ---------------- */
static u16 depth[256*192];
/* 2026-11-05: fog = the polygon fog bit when the batch was deferred. The replay ran with whatever G3G_CurFog the last polygon
   left, so a deferred translucent batch could get the wrong fog bit in fog frames (non-raw path). */
/* Deferred translucents must carry the same snapshotted stereo/tex as the job: FlushDeferred runs without gtCur. */
struct Batch{const GuVertex*v;unsigned n;Tex*tex;void*gt;unsigned tw,th;bool hasTex;G3SIM_TexImageParam_t tp;bool dw;unsigned dtest;int fog;int sd;int parL;bool wide;};
static GuVertex transVerts[24576];static unsigned transVN;static Batch trans[1024];static unsigned transN;
static inline unsigned wrap(int c,unsigned size,bool rep,bool flip){
 if(!rep)return c<0?0:(unsigned)c>=size?size-1:c;
 unsigned m=(unsigned)c&(size*2-1);if(m>=size)return flip?size*2-1-m:m-size;return m;}
/* Stereo: per-vertex parallax. Each eye rasterizes into its own colour/depth buffer with x shifted by
   eye * sd * zf, zf = depth (0 near .. 1 far): near geometry ~at the screen plane, far geometry behind it. */
static u32 frameR[256*192] __attribute__((aligned(64)));static u16 depthR[256*192];static int stereoSd;
/* run 40: stereo_depth=1 (default) per-vertex parallax from depth (0.3 + 0.7 z^8); 0 = one uniform parallax for the
   whole 3D layer (nothing inside the 3D scene can separate between the eyes; test for "doubled" sprites). */
/* run 41: stereo_clamp=1 (default): a triangle with a degenerate vertex (w ~ 0, or depth outside 0..1 before the
   clamp, e.g. a vertex at/behind the near or far plane during an effect) gets one uniform parallax for all three
   vertices instead of per-vertex values, so it cannot shear/split between the eyes. 0 = old per-vertex always. */
static int stereoClamp=-1;static unsigned stereoClamped;
static inline bool zDegenerate(float w,float zf){if(stereoClamp<0){stereoClamp=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"stereo_clamp=%d",&x)==1)stereoClamp=x!=0;fclose(f);}PSPNativeMemLog("[G3] stereo_clamp=%d",stereoClamp);}
 return stereoClamp&&(w<1e-3f||zf<-0.001f||zf>1.001f);}
/* Battle 3D parallax per eye as a fraction of the stereo shift: total disparity 2*par*sd = the right-eye-only shift stereo_2d gives the back BG (platform, grass),
   (sd+1)/2, i.e. par ~ 0.25, so a Pokemon sits at the depth of the ground it stands on. 0.65 (the old generic fallback) put it ~5 DS px in front of its own platform. */
static const float kBattlePar=0.25f;
/* run 80 battle depth lock, exact match to the 2D BG: render3ds shifts only the RIGHT eye's back BG by the integer d=(sd+1)/2 (left eye unshifted).
   The 3D quads used +-0.25*sd (fractional for odd sd, and both eyes moved), so the Pokemon landed up to half a pixel off its platform
   and snapped between pixel rows/columns as it moved. Lock now uses the same integer: left eye 0, right eye +d. */
/* 2026-11-18 issue #484: a locked parL is 64+e, e = extra right-eye px for this draw (stereo_2d=4 FOE DEEPER: + on the foe's
   side, - on yours; 0 otherwise). Any parL != 0 still means "locked" everywhere else. */
static const int kParLock=64;
static inline float lockSh(int eye,int sd,int parL){return eye?(float)((sd+1)/2+(parL>=32?parL-kParLock:0)):0.0f;}
static int parSideE;static unsigned parFoeN,parMineN;
static int parLock,parTrack;static unsigned parN;static float parZMin=1,parZMax=0;
/* One raw/screen draw as a value. Worker path must only read this (plus immutable config), never live stereo/matrix/tex globals. */
struct GJob{const GuVertex*v;unsigned n;float M[16];bool cullAll;int cullFace;int fog;unsigned depthTest;G3SIM_TexImageParam_t tp;bool tdw;bool wbuf;bool replay;
 const void*xfKey;int sd;int parL;bool mid,wide;
 Tex*tex;void*gt;bool hasTex;unsigned tw,th;unsigned texGen,texGpuGen;u32 texCrc;bool chunked;const u8*cmask;bool screen,deferOk;
 /* post-#485/#486: midpoint K, cull/clip/sprite flags, submit CRC + clip xform hash for g3_thr_check */
 bool hasK;float K[16];int sprDepth;int cull3d;bool cpuClip,guardClip,guardBat,edgeFix,fieldSpr;u32 snapCrc,xformCrc;
 /* g3_stereo_share: publish per job so the worker sees share ON (stereoShareOn alone raced across cores). */
 bool stereoShare;};
static const GJob* gtCur; /* set while GpuRawJ / DrawScreenJ run */
static unsigned gtCheckN,gtCheckBad,gtTexBad; /* g3_thr_check: jobs verified / POD+xform / tex mismatches */
static int stereoDepth=-1;
extern "C" int P3DS_StereoDepth(void){if(stereoDepth<0){stereoDepth=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"stereo_depth=%d",&x)==1)stereoDepth=x!=0;fclose(f);}PSPNativeMemLog("[G3] stereo_depth=%d",stereoDepth);}return stereoDepth;}
extern "C" void P3DS_SetStereoDepth(int v){stereoDepth=v!=0;PSPNativeMemLog("[G3] stereo_depth=%d from settings",stereoDepth);}
static inline float parOf(float zf){if(stereoDepth<0)P3DS_StereoDepth();
 /* stereo_2d=3 (default): in battles every 3D polygon gets the same parallax. Battle Pokemon are 3D quads whose z changes during the
    send-out and attack animations, so the depth-based parallax moved each eye's copy by a different amount for about a second
    ("sprite in a different spot in each eye"). Prefer the job's parL when gtCur is set (g3_thread worker / hard-clip). */
 const int lock=gtCur?gtCur->parL:parLock;
 if(lock){parN++;return kBattlePar;}
 if(!stereoDepth)return 0.65f;if(zf<0)zf=0;if(zf>1)zf=1;float z2=zf*zf,z4=z2*z2,z8=z4*z4;float p=0.3f+0.7f*z8;if(!gtCur&&parTrack){parN++;if(zf<parZMin)parZMin=zf;if(zf>parZMax)parZMax=zf;}return p;}
static void*G3SIM_GpuTex(Tex*t){void*g=G3G_TexCreate(t->px,t->w,t->h);if(g)t->gpuGen++;return g;}
/* Identity + sampled px content + gpu upload gen. Hashes live t->gen (c2087a3 like-for-like); freeze-on-pin
   keeps gen/px stable for in-flight jobs so thr_check does not false-positive. */
static u32 gtTexCrcOf(const Tex*t,void*gt,unsigned tw,unsigned th,unsigned gpuGen){
 u32 h=2166136261u;auto feed=[&](const void*p,unsigned n){const u8*b=(const u8*)p;for(unsigned i=0;i<n;i++)h=(h^b[i])*16777619u;};
 if(!t){feed(&gt,sizeof gt);feed(&tw,sizeof tw);feed(&th,sizeof th);feed(&gpuGen,sizeof gpuGen);return h;}
 feed(&t->off,sizeof t->off);feed(&t->pal,sizeof t->pal);feed(&t->fmt,sizeof t->fmt);feed(&t->w,sizeof t->w);feed(&t->h,sizeof t->h);
 feed(&t->c0,sizeof t->c0);feed(&t->gen,sizeof t->gen);feed(&gt,sizeof gt);feed(&gpuGen,sizeof gpuGen);
 if(t->px&&t->w&&t->h){const unsigned n=t->w*t->h;const unsigned step=n>64u?(n/64u):1u;
  for(unsigned i=0;i<n;i+=step)feed(&t->px[i],sizeof(u32));
  feed(&t->px[n-1],sizeof(u32));}
 return h;}
extern "C" u32 *P3DS_G3RightEye(void){return stereoSd?frameR:nullptr;}
static void TriEye(const GuVertex&a,const GuVertex&b,const GuVertex&c,Tex*tex,const G3SIM_TexImageParam_t&tp,bool blend,bool dwrite,unsigned dtest,u32*fb,u16*depth);
/* 2026-10-25 [LUGIA] W-buffer frames on the CPU renderer (fallback when GPU 3D is off; g3_wpersp key and the Debug CPU-W toggle were removed):
   TriEye interpolates depth, UV and colour AFFINELY in screen space. For a W-buffer frame that is the same error as the
   GPU path had (depth = w is not affine on screen: Lugia's spikes poke through / float, holes in the body), so run 2's
   "CPU 3D" never drew a true W-buffer and could not tell geometry bugs from depth bugs. Here each vertex's w is recovered
   from its W-buffer z (z/w = 2*w*wInv-1, see wDepthZ; G3SIM_AddVtx uses the same scale), 1/w and attr/w are interpolated
   and divided per pixel: depth = w like the DS W-buffer, UV/colour perspective-correct like the DS. */
static bool wPerspFrame;
static float wInvNow();
static void TriWPersp(const GuVertex&a,const GuVertex&b,const GuVertex&c,Tex*tex,const G3SIM_TexImageParam_t&tp,bool blend,bool dwrite,unsigned dtest,u32*fb,u16*depth,int x0,int x1,int y0,int y1,float ia){
 const GuVertex*vv[3]={&a,&b,&c};const float wi=wInvNow(),wmax=1.0f/wi;float va[3][7];
 for(int k=0;k<3;k++){const GuVertex&p=*vv[k];float d=((p.z-32768.0f)*(1.0f/32767.0f)+1.0f)*0.5f;float w=d*wmax;if(w<1e-4f)w=1e-4f;const float q=1.0f/w;float*o=va[k];
  o[0]=q;o[1]=float(p.color&255)*q;o[2]=float((p.color>>8)&255)*q;o[3]=float((p.color>>16)&255)*q;o[4]=float(p.color>>24)*q;o[5]=p.u*q;o[6]=p.v*q;}
 float dX[7],dY[7];for(int k=0;k<7;k++){const float fa=va[0][k],fb2=va[1][k],fc=va[2][k];dX[k]=((fb2-fa)*(c.y-a.y)-(fc-fa)*(b.y-a.y))*ia;dY[k]=((fc-fa)*(b.x-a.x)-(fb2-fa)*(c.x-a.x))*ia;}
 float Ex[3]={-(c.y-b.y)*ia,-(a.y-c.y)*ia,0},Ey[3]={(c.x-b.x)*ia,(a.x-c.x)*ia,0},E0[3];
 E0[0]=(b.x*c.y-b.y*c.x)*ia;E0[1]=(c.x*a.y-c.y*a.x)*ia;E0[2]=1.0f-E0[0]-E0[1];Ex[2]=-Ex[0]-Ex[1];Ey[2]=-Ey[0]-Ey[1];
 const float eps=1e-5f,i255=1.0f/255.0f,zK=wi*65534.0f;
 for(int y=y0;y<y1;y++){float py=y+0.5f;int xs=x0,xe=x1;bool skip=false;
  for(int k=0;k<3&&!skip;k++){float e=E0[k]+Ey[k]*py+Ex[k]*(x0+0.5f),d=Ex[k];
   if(d>1e-12f){float t=(-eps-e)/d;int kk=(int)ceilf(t);if(x0+kk>xs)xs=x0+kk;}
   else if(d<-1e-12f){float t=(e+eps)/-d;int kk=(int)floorf(t);if(x0+kk+1<xe)xe=x0+kk+1;}
   else if(e<-eps)skip=true;}
  if(skip||xs>=xe)continue;
  float fx=xs+0.5f-a.x,fy=py-a.y;float at[7];for(int k=0;k<7;k++)at[k]=va[0][k]+dX[k]*fx+dY[k]*fy;
  u32*frow=fb+y*256;u16*drow=depth+y*256;
  for(int x=xs;x<xe;x++,at[0]+=dX[0],at[1]+=dX[1],at[2]+=dX[2],at[3]+=dX[3],at[4]+=dX[4],at[5]+=dX[5],at[6]+=dX[6]){
   if(at[0]<=0)continue;const float iw=1.0f/at[0];
   float z=iw*zK+1.0f;unsigned zi=z<0?0:z>65534?65534:(unsigned)z;u16&dz=drow[x];
   if(dtest?zi!=dz:zi>dz){stDepth++;continue;}
   float r=at[1]*iw,g=at[2]*iw,bl=at[3]*iw,al=at[4]*iw;
   if(tex){u32 t=tex->px[wrap((int)floorf(at[6]*iw),tex->h,tp.repeatT,tp.flipT)*tex->w+wrap((int)floorf(at[5]*iw),tex->w,tp.repeatS,tp.flipS)];
    unsigned ta=t>>24;if(!ta){stTexA++;continue;}
    r*=(t&255)*i255;g*=((t>>8)&255)*i255;bl*=((t>>16)&255)*i255;al*=ta*i255;}
   int A=(int)(al*(31.0f/255.0f)+0.5f);if(A<=0){stA0++;continue;}if(A>31)A=31;stWritten++;
   int ri=(int)r,gi=(int)g,bi=(int)bl;ri=ri<0?0:ri>255?255:ri;gi=gi<0?0:gi>255?255:gi;bi=bi<0?0:bi>255?255:bi;
   u32 R=ri>>2,G=gi>>2,B=bi>>2;u32&dd=frow[x];
   if(blend&&A<31){unsigned da=dd>>24;if(da){u32 dr=dd&63,dg=(dd>>8)&63,db=(dd>>16)&63;R=(R*(A+1)+dr*(31-A))>>5;G=(G*(A+1)+dg*(31-A))>>5;B=(B*(A+1)+db*(31-A))>>5;A=(unsigned)A>da?A:da;}}
   dd=R|(G<<8)|(B<<16)|((u32)A<<24);if(dwrite)dz=(u16)zi;
  }}
}
static void Tri(const GuVertex&a,const GuVertex&b,const GuVertex&c,Tex*tex,const G3SIM_TexImageParam_t&tp,bool blend,bool dwrite,unsigned dtest){
 const int sd=gtCur?gtCur->sd:stereoSd;const int parL=gtCur?gtCur->parL:parLock;
 if(!sd){TriEye(a,b,c,tex,tp,blend,dwrite,dtest,GPU3D::NativeFrame,depth);return;}
 /* stereo_gpu left-only: one screen write + dsh (right_sx-left_sx) so End stays on gpuRight — dual G3G_Tri
    would Expand and fill eye 1 (hw e8900f9 path flip after hard-clip / translucent). */
 if(gpuOn==1&&G3G_StereoGpuLeftOnly()){
  const bool wide=gtCur?gtCur->wide:(G3G_Wide!=0);const float xl=wide?-32.0f:0.0f,xr=wide?288.0f:256.0f;
  const GuVertex*o[3]={&a,&b,&c};float XL[3],XR[3],Y[3],Z[3],U[3],W[3],dsh[3];u32 C[3];
  for(int k=0;k<3;k++){float par=parOf(o[k]->z*(1.0f/65535.0f));float shL=parL?lockSh(0,sd,parL):-sd*par,shR=parL?lockSh(1,sd,parL):sd*par;
   XL[k]=o[k]->x+shL;XR[k]=o[k]->x+shR;dsh[k]=XR[k]-XL[k];Y[k]=o[k]->y;Z[k]=o[k]->z;U[k]=o[k]->u;W[k]=o[k]->v;C[k]=o[k]->color;}
  if(fmaxf(XL[0],fmaxf(XL[1],XL[2]))<xl&&fmaxf(XR[0],fmaxf(XR[1],XR[2]))<xl)return;
  if(fminf(XL[0],fminf(XL[1],XL[2]))>xr&&fminf(XR[0],fminf(XR[1],XR[2]))>xr)return;
  if(fmaxf(Y[0],fmaxf(Y[1],Y[2]))<0||fminf(Y[0],fminf(Y[1],Y[2]))>192)return;
  drawnTris++;
  void*gt=nullptr;unsigned tw=0,th=0;
  if(gtCur&&gtCur->hasTex){gt=gtCur->gt;tw=gtCur->tw;th=gtCur->th;}
  else if(tex){if(!tex->gt&&!gtInWorker)tex->gt=G3G_TexCreate(tex->px,tex->w,tex->h);gt=tex->gt;tw=tex->w;th=tex->h;}
  G3G_TriScreenShare(XL,Y,Z,U,W,C,dsh,gt,tw,th,tp.repeatS,tp.repeatT,tp.flipS,tp.flipT,blend,dwrite,dtest);return;}
 GuVertex v[3]={a,b,c};const GuVertex*o[3]={&a,&b,&c};
 for(int eye=-1;eye<=1;eye+=2){for(int k=0;k<3;k++){float par=parOf(o[k]->z*(1.0f/65535.0f));/* perspective NDC z bunches near 1: z^8 spreads it */v[k].x=o[k]->x+(parL?lockSh(eye>0,sd,parL):eye*sd*par);}
  TriEye(v[0],v[1],v[2],tex,tp,blend,dwrite,dtest,eye<0?GPU3D::NativeFrame:frameR,eye<0?depth:depthR);}
}
static void TriEye(const GuVertex&a,const GuVertex&b,const GuVertex&c,Tex*tex,const G3SIM_TexImageParam_t&tp,bool blend,bool dwrite,unsigned dtest,u32*fb,u16*depth){
 if(gpuOn==1){const bool wide=gtCur?gtCur->wide:(G3G_Wide!=0);const float xl=wide?-32.0f:0.0f,xr=wide?288.0f:256.0f;if(fmaxf(a.x,fmaxf(b.x,c.x))<xl||fminf(a.x,fminf(b.x,c.x))>xr||fmaxf(a.y,fmaxf(b.y,c.y))<0||fminf(a.y,fminf(b.y,c.y))>192)return;drawnTris++;
  void*gt=nullptr;unsigned tw=0,th=0;
  if(gtCur&&gtCur->hasTex){gt=gtCur->gt;tw=gtCur->tw;th=gtCur->th;}
  else if(tex){if(!tex->gt&&!gtInWorker)tex->gt=G3G_TexCreate(tex->px,tex->w,tex->h);gt=tex->gt;tw=tex->w;th=tex->h;}
  float X[3]={a.x,b.x,c.x},Y[3]={a.y,b.y,c.y},Z[3]={a.z,b.z,c.z},U[3]={a.u,b.u,c.u},W[3]={a.v,b.v,c.v};u32 C[3]={a.color,b.color,c.color};
  G3G_Tri(fb==frameR?1:0,X,Y,Z,U,W,C,gt,tw,th,tp.repeatS,tp.repeatT,tp.flipS,tp.flipT,blend,dwrite,dtest);return;}
 float area=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);if(fabsf(area)<1e-6f)return;
 int x0=(int)floorf(fminf(a.x,fminf(b.x,c.x))),x1=(int)ceilf(fmaxf(a.x,fmaxf(b.x,c.x)));
 int y0=(int)floorf(fminf(a.y,fminf(b.y,c.y))),y1=(int)ceilf(fmaxf(a.y,fmaxf(b.y,c.y)));
 if(x0<0)x0=0;if(y0<0)y0=0;if(x1>256)x1=256;if(y1>192)y1=192;if(x0>=x1||y0>=y1)return;
 float ia=1.0f/area;drawnTris++;if(tex)stTexTris++;if(blend)stTrans++;stFmt[tp.textureFormat&7]++;stColOr|=a.color;stColAnd&=a.color;
 {const GuVertex*vv[3]={&a,&b,&c};for(int k=0;k<3;k++){float z=vv[k]->z,x=vv[k]->x;unsigned al=vv[k]->color>>24;if(z<stZmin)stZmin=z;if(z>stZmax)stZmax=z;if(x<stXmin)stXmin=x;if(x>stXmax)stXmax=x;if(al<stAmin)stAmin=al;if(al>stAmax)stAmax=al;}}
 if(wPerspFrame){TriWPersp(a,b,c,tex,tp,blend,dwrite,dtest,fb,depth,x0,x1,y0,y1,ia);stPix+=(unsigned)((x1-x0)*(y1-y0));return;}
 float ca[4]={float(a.color&255),float((a.color>>8)&255),float((a.color>>16)&255),float(a.color>>24)};
 float cb[4]={float(b.color&255),float((b.color>>8)&255),float((b.color>>16)&255),float(b.color>>24)};
 float cc[4]={float(c.color&255),float((c.color>>8)&255),float((c.color>>16)&255),float(c.color>>24)};
 /* plane-equation setup: every attribute is linear in screen space (affine, like before) */
 auto pl=[&](float fa,float fb,float fc,float&dx,float&dy){dx=((fb-fa)*(c.y-a.y)-(fc-fa)*(b.y-a.y))*ia;dy=((fc-fa)*(b.x-a.x)-(fb-fa)*(c.x-a.x))*ia;};
 float att0[7]={a.z,ca[0],ca[1],ca[2],ca[3],a.u,a.v},dX[7],dY[7];
 pl(a.z,b.z,c.z,dX[0],dY[0]);for(int k=0;k<4;k++)pl(ca[k],cb[k],cc[k],dX[k+1],dY[k+1]);pl(a.u,b.u,c.u,dX[5],dY[5]);pl(a.v,b.v,c.v,dX[6],dY[6]);
 /* barycentric edge functions, linear in x: w_i(x,y) = E_i + Ex_i*x + Ey_i*y */
 float Ex[3]={-(c.y-b.y)*ia,-(a.y-c.y)*ia,0},Ey[3]={(c.x-b.x)*ia,(a.x-c.x)*ia,0},E0[3];
 E0[0]=(b.x*c.y-b.y*c.x)*ia;E0[1]=(c.x*a.y-c.y*a.x)*ia;E0[2]=1.0f-E0[0]-E0[1];Ex[2]=-Ex[0]-Ex[1];Ey[2]=-Ey[0]-Ey[1];
 const float eps=1e-5f;const float i255=1.0f/255.0f;
 for(int y=y0;y<y1;y++){float py=y+0.5f;int xs=x0,xe=x1;bool skip=false;
  for(int k=0;k<3&&!skip;k++){float e=E0[k]+Ey[k]*py+Ex[k]*(x0+0.5f),d=Ex[k];
   if(d>1e-12f){float t=(-eps-e)/d;int kk=(int)ceilf(t);if(x0+kk>xs)xs=x0+kk;}
   else if(d<-1e-12f){float t=(e+eps)/-d;int kk=(int)floorf(t);if(x0+kk+1<xe)xe=x0+kk+1;}
   else if(e<-eps)skip=true;}
  if(skip||xs>=xe)continue;
  float fx=xs+0.5f-a.x,fy=py-a.y;float at[7];for(int k=0;k<7;k++)at[k]=att0[k]+dX[k]*fx+dY[k]*fy;
  u32*frow=fb+y*256;u16*drow=depth+y*256;
  for(int x=xs;x<xe;x++,at[0]+=dX[0],at[1]+=dX[1],at[2]+=dX[2],at[3]+=dX[3],at[4]+=dX[4],at[5]+=dX[5],at[6]+=dX[6]){
   float z=at[0];unsigned zi=z<0?0:z>65534?65534:(unsigned)z;u16&dz=drow[x];
   if(dtest?zi!=dz:zi>dz){stDepth++;continue;}
   float r=at[1],g=at[2],bl=at[3],al=at[4];
   if(tex){u32 t=tex->px[wrap((int)floorf(at[6]),tex->h,tp.repeatT,tp.flipT)*tex->w+wrap((int)floorf(at[5]),tex->w,tp.repeatS,tp.flipS)];
    unsigned ta=t>>24;if(!ta){stTexA++;continue;}
    r*=(t&255)*i255;g*=((t>>8)&255)*i255;bl*=((t>>16)&255)*i255;al*=ta*i255;}
   int A=(int)(al*(31.0f/255.0f)+0.5f);if(A<=0){stA0++;continue;}if(A>31)A=31;stWritten++;
   int ri=(int)r,gi=(int)g,bi=(int)bl;ri=ri<0?0:ri>255?255:ri;gi=gi<0?0:gi>255?255:gi;bi=bi<0?0:bi>255?255:bi;
   u32 R=ri>>2,G=gi>>2,B=bi>>2;u32&d=frow[x];
   if(blend&&A<31){unsigned da=d>>24;if(da){u32 dr=d&63,dg=(d>>8)&63,db=(d>>16)&63;R=(R*(A+1)+dr*(31-A))>>5;G=(G*(A+1)+dg*(31-A))>>5;B=(B*(A+1)+db*(31-A))>>5;A=(unsigned)A>da?A:da;}}
   d=R|(G<<8)|(B<<16)|((u32)A<<24);if(dwrite)dz=(u16)zi;
  }}
 stPix+=(unsigned)((x1-x0)*(y1-y0));
}
/* Raw mode (pspoke fast path, used by field maps): vertices are model-space with normalised UVs and a separate
   DS clip matrix (fx32 4x4, row vectors: clip = [x y z 1] * M). Transform, clip against w > eps (near) and
   emit screen-space triangles in the same convention as G3SIM_AddVtx. */
void G3SIM_FlushArray();
/* ---- g3_thread: GJob is defined above (near Tri). GpuRawJ / RawToScreen / DrawScreenJ take all mutable state from the job. ---- */
static bool rawMode=false;static float rawM[16];
struct CV{float x,y,z,w,u,v;float c[4];};
static GuVertex rawOut[6144*3]; /* game-thread CPU raw path only; hard-clip inside GpuRawJ uses a stack buffer */
static unsigned packC(const float*c){unsigned r=0;for(int k=0;k<4;k++){float f=c[k];if(f<0)f=0;if(f>255)f=255;r|=((unsigned)(f+0.5f))<<(8*k);}return r;}
static GuVertex toScreen(const CV&a){float iw=1.0f/a.w;GuVertex o;o.u=a.u;o.v=a.v;o.color=packC(a.c);
 o.x=128.0f+a.x*iw*128.0f;o.y=96.0f-a.y*iw*96.0f;o.z=a.z*iw*32767.0f+32768.0f;return o;}
extern "C" u8 s_SIM_useWBuffer;
static float wInv=1.0f/4096;
static float wInvNow(){return wInv;}
/* g3_wmode (config.txt only, no menu row): depth code 2 = PICA W (default), 3 = GPU Z: W-ordered depth done in software for the GPU path. The DS
   W-buffer orders by w; run 1 gave each vertex clip z = (2*w/wmax-1)*w, which is linear in w, and the GPU interpolates
   z/w affinely in screen space - wrong across big triangles (title Lugia: spikes over the back, holes). The PICA
   W-buffer mode (run 1 fix, now value 2) changed nothing on hardware. Here z_ndc = A - B/w (a normal projection depth:
   monotonic in w, so the same order as the W-buffer, and z = A*w - B is linear in clip space, so every GPU interpolates
   it exactly with a plain z-buffer). w near 1 .. far 16384; w past far is pinned to the far plane (z = w). 0 = run 0. */
extern "C" int G3G_WDepthOn(void);
/* near/far follow the previous frame's w range (near = half the smallest w, far = 1.25x the largest) so the 24-bit
   depth (and the PICA's float24 interpolation) keeps its precision where the scene is: hyperbolic depth with near=1 put
   the whole title scene in the last 1/1000 of the range. */
static float wdN=1.0f,wdF=16384.0f,wdMinW=1e30f,wdMaxW=0;
static void wDepthFrame(){if(wdMaxW>0){float n=0.5f*wdMinW;if(n<0.5f)n=0.5f;float f=1.25f*wdMaxW;if(f<4*n)f=4*n;wdN=n;wdF=f;}wdMinW=1e30f;wdMaxW=0;}
static inline float wDepthZ(float w){
 if(G3G_WDepthOn()==3){if(w>0){if(w<wdMinW)wdMinW=w;if(w>wdMaxW)wdMaxW=w;}const float n=wdN,f=wdF,A=(f+n)/(f-n),B=2.0f*f*n/(f-n);if(w>=f)return w;return A*w-B;}
 /* run 2: a vertex at or behind the eye (w <= 0) got z = (2w/wmax-1)w >= -w, i.e. INSIDE the near plane (z >= -w), so
    the CPU clipper kept tris crossing behind the camera: the title water's near strip came out as stretched streaks and
    an empty block (CPU renderer, and GPU tris that take the CPU-clip path). z = w - 1 puts it outside (needs w >= 0.5). */
 if(w<=0)return w-1.0f;float d=w*wInv;if(d>1)d=1;return (2*d-1)*w;}
static int wbufOn(){static int v=-1;if(v<0){v=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f)){if(sscanf(l,"g3_wbuf=%d",&x)==1)v=x;if(sscanf(l,"g3_wmax=%d",&x)==1&&x>0)wInv=1.0f/x;}fclose(f);}PSPNativeMemLog("[GPU3D] g3_wbuf=%d (0 off, 1 when game selects W, 2 force)",v);}
 static u8 last=9;if(last!=s_SIM_useWBuffer){last=s_SIM_useWBuffer;PSPNativeMemLog("[GPU3D] game depth mode now %s",last?"W-buffer":"Z-buffer");}
 return v==2||(v==1&&s_SIM_useWBuffer);}
static unsigned RawToScreenBuf(const GJob&J,const GuVertex*v,unsigned n,GuVertex*dst,unsigned dstCap){
 unsigned out=0;const float tw=(float)J.tp.textureSSize,th=(float)J.tp.textureTSize;const float*M=J.M;
 for(unsigned i=0;i+2<n;i+=3){CV in[3],poly[8],tmp[8];
  for(int k=0;k<3;k++){const GuVertex&p=v[i+k];CV&c=in[k];
   c.x=p.x*M[0]+p.y*M[4]+p.z*M[8]+M[12];c.y=p.x*M[1]+p.y*M[5]+p.z*M[9]+M[13];c.z=p.x*M[2]+p.y*M[6]+p.z*M[10]+M[14];c.w=p.x*M[3]+p.y*M[7]+p.z*M[11]+M[15];if(J.wbuf)c.z=wDepthZ(c.w);
   c.u=p.u*tw;c.v=p.v*th;for(int q=0;q<4;q++)c.c[q]=(float)((p.color>>(8*q))&255);}
  /* clip against near plane z >= -w (DS/GL convention) and w > eps */
  int np=3;for(int k=0;k<3;k++)poly[k]=in[k];
  for(int plane=0;plane<2;plane++){int nt=0;
   for(int k=0;k<np;k++){const CV&A=poly[k],&B=poly[(k+1)%np];
    float da=plane?A.w-1e-4f:A.z+A.w,db=plane?B.w-1e-4f:B.z+B.w;
    if(da>=0)tmp[nt++]=A;
    if((da>=0)!=(db>=0)){float t=da/(da-db);CV r;r.x=A.x+(B.x-A.x)*t;r.y=A.y+(B.y-A.y)*t;r.z=A.z+(B.z-A.z)*t;r.w=A.w+(B.w-A.w)*t;r.u=A.u+(B.u-A.u)*t;r.v=A.v+(B.v-A.v)*t;for(int q=0;q<4;q++)r.c[q]=A.c[q]+(B.c[q]-A.c[q])*t;tmp[nt++]=r;}}
   np=nt;for(int k=0;k<np;k++)poly[k]=tmp[k];if(np<3)break;}
  if(np<3)continue;
  GuVertex s0=toScreen(poly[0]);
  for(int k=1;k+1<np;k++){if(out+3>dstCap)return out;dst[out++]=s0;dst[out++]=toScreen(poly[k]);dst[out++]=toScreen(poly[k+1]);}
 }
 return out;
}
static unsigned RawToScreen(const GJob&J,const GuVertex*v,unsigned n){return RawToScreenBuf(J,v,n,rawOut,(unsigned)(sizeof rawOut/sizeof rawOut[0]));}
static void DrawScreen(const GuVertex*v,unsigned n,bool deferOk);static void DrawScreenJ(const GJob&J,const GuVertex*v,unsigned n,bool deferOk);
static unsigned culledTris,cpuClipped,rejOut,rejMask;int vdump,vdumpN;
/* g3_guardclip (config.txt only, default 0 since 2026-11-05): 1 = also send tris past the 2x guard band (but inside near/far)
   through the CPU clipper. That path draws in screen space without perspective, and on Bellchime Trail its sky quads covered
   the whole map (Nathan saw the sky box low at the tower). The PICA draws those tris correctly (Azahar: Ilex, Lake of Rage,
   Ecruteak, Union Cave, Olivine, Ice Path, Cianwood, National Park identical with 0 and 1; no black spots). */
/* 2026-11-07 (enemy attacks / Intimidate invisible since #353): battle effect quads are huge and run far past the 2x guard band; #353 sent them to the PICA clipper, which
   drops them. Battles keep the old CPU-clip rule; only the field (Bellchime sky) uses the new one. */
static inline bool guardBattle(){extern int P3DS_InBattle,P3DS_InField;return P3DS_InBattle!=0||P3DS_InField==0;} /* name kept: "not the field" = battle, evolution, Hall of Fame, title/intro, contest, Pokedex... only the field uses the PICA clipper for the guard band */
static int guardClip(){static int v=-1;if(v<0){v=P3DS_CfgInt("g3_guardclip",0);PSPNativeMemLog("[GPU3D] g3_guardclip=%d",v);}return v;}
/* gpu3d_cpuclip=1 (default): clip/divide on the CPU (same as the clean CPU renderer), raster on the GPU.
   Run 26: black spots only on the clip-space GPU path. 0 = old clip-space path. */
static int gpuCpuClip(){static int v=-1;if(v<0){v=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"gpu3d_cpuclip=%d",&x)==1)v=x!=0;fclose(f);}PSPNativeMemLog("[GPU3D] gpu3d_cpuclip=%d",v);}return v;}
/* 2026-10-28 sprite_depth (Settings > 3D "Sprite depth", default OFF until tested): overworld sprites (the following Pokemon, NPCs,
   the player) are small upright textured quads. Per-vertex parallax gives their top and bottom different shifts (the quad leans back
   a little), so with the 3D slider up a sprite looked sheared / slightly detached from the ground. With this on, such a quad gets ONE
   parallax for all its vertices, taken from its lowest (feet) vertex: the sprite stands on the ground at the ground's depth. */
static int sprDepthV=-1;extern "C" int P3DS_SpriteDepthCfg(void){if(sprDepthV<0){sprDepthV=P3DS_CfgInt("sprite_depth",0)!=0;PSPNativeMemLog("[SPRDEPTH] sprite_depth=%d",sprDepthV);}return sprDepthV;}
extern "C" void P3DS_SetSpriteDepth(int v){sprDepthV=v!=0;}
static unsigned sprN,sprSkip;static double sprSpread;
/* ---- fps60 (g3gpu.c): pair every raw draw call with the same draw of the tick before, by what it draws (the list-cache array for replayed
   display lists, else the vertex count and the first/last vertex) and, where one thing is drawn several times (trees, people), by the one
   whose clip matrix is nearest where the camera's move says it should be. The in-between picture transforms the same vertices with the
   average of the two matrices. A draw with no partner (it just came into view) moves with the camera: for everything standing still in
   the world the two matrices differ only in their translation row, by the same four numbers (mdG). ---- */
extern "C"{void P3DS_Fps60Poll(void);extern int G3G_MidRec;void G3G_MidK(const float*);void G3G_MidInfo(unsigned,unsigned,unsigned);}
static const float kMidEdge=1.06f;   /* off-screen rejects keep a 6% rim while recording: a triangle just outside this tick may be inside the in-between picture */
struct MDraw{const void*key;unsigned n;u32 sig;float M[16];};
enum{MD_MAX=1536,MD_H=512};
static MDraw mdA[MD_MAX],mdB[MD_MAX];static MDraw*mdCur=mdA,*mdPrev=mdB;static unsigned mdCurN,mdPrevN;static bool mdPrevOk;
static short mdHead[MD_H],mdNext[MD_MAX];static float mdG[4];static unsigned mdTot,mdMat,mdMov;
static inline unsigned mdHash(const void*key,unsigned n,u32 sig){return (unsigned)((((uintptr_t)key>>3)^(n*40503u)^sig)*2654435761u)>>23;}
static void mdFrameBegin(bool rec){
 /* the table just filled becomes "the tick before" only if that tick recorded (else nothing may be paired with stale matrices) */
 MDraw*t=mdCur;mdCur=mdPrev;mdPrev=t;mdPrevN=mdPrevOk?mdCurN:0;mdCurN=0;mdPrevOk=rec;mdTot=mdMat=mdMov=0;
 if(!rec){mdPrevN=0;return;}
 memset(mdHead,0xFF,sizeof mdHead);
 for(unsigned i=0;i<mdPrevN;i++){const unsigned h=mdHash(mdPrev[i].key,mdPrev[i].n,mdPrev[i].sig);mdNext[i]=mdHead[h];mdHead[h]=(short)i;}}
static const float*mdMid(const GJob&J,float*Mm){
 const float*M=J.M;const void*key=J.replay?(J.xfKey?J.xfKey:(const void*)J.v):nullptr;u32 sig=0;
 if(!J.replay){const GuVertex&a=J.v[0],&b=J.v[J.n-1];u32 w[6];memcpy(w,&a.x,12);memcpy(w+3,&b.x,12);sig=2166136261u;for(int i=0;i<6;i++)sig=(sig^w[i])*16777619u;}
 mdTot++;
 const MDraw *best=nullptr,*bestA=nullptr;float bs=1e30f,bsA=1e30f;unsigned cand=0;
 for(int i=mdHead[mdHash(key,J.n,sig)];i>=0;i=mdNext[i]){
  const MDraw&d=mdPrev[i];if(d.key!=key||d.n!=J.n||d.sig!=sig)continue;cand++;
  const float dx=M[12]-d.M[12],dy=M[13]-d.M[13],dw=M[15]-d.M[15];
  const float sc=fabsf(dx-mdG[0])+fabsf(dy-mdG[1])+fabsf(dw-mdG[3]);
  const float scA=fabsf(dx)+fabsf(dy)+fabsf(dw);
  if(sc<bs){bs=sc;best=&d;}if(scA<bsA){bsA=scA;bestA=&d;}
 }
 if(mdCurN<MD_MAX){MDraw&c=mdCur[mdCurN++];c.key=key;c.n=J.n;c.sig=sig;memcpy(c.M,M,sizeof c.M);}
 const float wr=fabsf(M[15])>1.0f?fabsf(M[15]):1.0f;
 /* Limits (run 838 photo and log; #458 tightened every draw and the jumping got worse, so only what is proven needed stays tight):
    - a unique draw (cand==1: a building, the ground, a big model) keeps the loose limit of a third of the screen;
    - a repeated model (several candidates with the same key: trees, fences) may only pair within 0.1 of the half screen, 12.8 px, under
      a tile, so one that has just come into view does not pair with its neighbour and sit halfway between them;
    - every draw also pairs with a partner that has not moved on the screen at all (fixed to the camera: the player, the player on the bike,
      whose camera step alone is more than the limit; glass and reflections drawn over the view), and a small draw (a character sprite,
      all the same quad) pairs within 0.15 (19 px) of either place. Unpaired draws move with the camera (mdG/2), which is wrong for the player, so he must pair. */
 const float lim=J.n<24?0.15f:(cand>1?0.1f:0.6f);
 if(bestA&&bsA<bs){best=bestA;bs=bsA;}
 if(best&&bs<lim*wr){mdMat++;bool same=true,rigid=true;
  for(int i=0;i<16;i++){const float d=M[i]-best->M[i];if(d!=0.0f){same=false;if(i<12&&fabsf(d)>1e-5f*(fabsf(M[i])+1.0f))rigid=false;}Mm[i]=0.5f*(M[i]+best->M[i]);}
  if(!same)mdMov++;
  if(cand==1&&rigid&&J.n>=24){mdG[0]=M[12]-best->M[12];mdG[1]=M[13]-best->M[13];mdG[2]=M[14]-best->M[14];mdG[3]=M[15]-best->M[15];}
  return Mm;}
 memcpy(Mm,M,16*sizeof(float));for(int i=0;i<4;i++)Mm[12+i]-=0.5f*mdG[i];
 return Mm;}
/* K = in-between clip matrix * inverse(this tick's clip matrix): what carries this tick's clip coordinates to the in-between picture.
   Doubles: the model part of M has big translations. false = the draw does not move (or M cannot be inverted): it stays where it is. */
static bool mdSolveK(const float *M,const float *Mm,float*K){
 double m[16],inv[16];for(int i=0;i<16;i++)m[i]=M[i];
 inv[0]=m[5]*m[10]*m[15]-m[5]*m[11]*m[14]-m[9]*m[6]*m[15]+m[9]*m[7]*m[14]+m[13]*m[6]*m[11]-m[13]*m[7]*m[10];
 inv[4]=-m[4]*m[10]*m[15]+m[4]*m[11]*m[14]+m[8]*m[6]*m[15]-m[8]*m[7]*m[14]-m[12]*m[6]*m[11]+m[12]*m[7]*m[10];
 inv[8]=m[4]*m[9]*m[15]-m[4]*m[11]*m[13]-m[8]*m[5]*m[15]+m[8]*m[7]*m[13]+m[12]*m[5]*m[11]-m[12]*m[7]*m[9];
 inv[12]=-m[4]*m[9]*m[14]+m[4]*m[10]*m[13]+m[8]*m[5]*m[14]-m[8]*m[6]*m[13]-m[12]*m[5]*m[10]+m[12]*m[6]*m[9];
 inv[1]=-m[1]*m[10]*m[15]+m[1]*m[11]*m[14]+m[9]*m[2]*m[15]-m[9]*m[3]*m[14]-m[13]*m[2]*m[11]+m[13]*m[3]*m[10];
 inv[5]=m[0]*m[10]*m[15]-m[0]*m[11]*m[14]-m[8]*m[2]*m[15]+m[8]*m[3]*m[14]+m[12]*m[2]*m[11]-m[12]*m[3]*m[10];
 inv[9]=-m[0]*m[9]*m[15]+m[0]*m[11]*m[13]+m[8]*m[1]*m[15]-m[8]*m[3]*m[13]-m[12]*m[1]*m[11]+m[12]*m[3]*m[9];
 inv[13]=m[0]*m[9]*m[14]-m[0]*m[10]*m[13]-m[8]*m[1]*m[14]+m[8]*m[2]*m[13]+m[12]*m[1]*m[10]-m[12]*m[2]*m[9];
 inv[2]=m[1]*m[6]*m[15]-m[1]*m[7]*m[14]-m[5]*m[2]*m[15]+m[5]*m[3]*m[14]+m[13]*m[2]*m[7]-m[13]*m[3]*m[6];
 inv[6]=-m[0]*m[6]*m[15]+m[0]*m[7]*m[14]+m[4]*m[2]*m[15]-m[4]*m[3]*m[14]-m[12]*m[2]*m[7]+m[12]*m[3]*m[6];
 inv[10]=m[0]*m[5]*m[15]-m[0]*m[7]*m[13]-m[4]*m[1]*m[15]+m[4]*m[3]*m[13]+m[12]*m[1]*m[7]-m[12]*m[3]*m[5];
 inv[14]=-m[0]*m[5]*m[14]+m[0]*m[6]*m[13]+m[4]*m[1]*m[14]-m[4]*m[2]*m[13]-m[12]*m[1]*m[6]+m[12]*m[2]*m[5];
 inv[3]=-m[1]*m[6]*m[11]+m[1]*m[7]*m[10]+m[5]*m[2]*m[11]-m[5]*m[3]*m[10]-m[9]*m[2]*m[7]+m[9]*m[3]*m[6];
 inv[7]=m[0]*m[6]*m[11]-m[0]*m[7]*m[10]-m[4]*m[2]*m[11]+m[4]*m[3]*m[10]+m[8]*m[2]*m[7]-m[8]*m[3]*m[6];
 inv[11]=-m[0]*m[5]*m[11]+m[0]*m[7]*m[9]+m[4]*m[1]*m[11]-m[4]*m[3]*m[9]-m[8]*m[1]*m[7]+m[8]*m[3]*m[5];
 inv[15]=m[0]*m[5]*m[10]-m[0]*m[6]*m[9]-m[4]*m[1]*m[10]+m[4]*m[2]*m[9]+m[8]*m[1]*m[6]-m[8]*m[2]*m[5];
 const double det=m[0]*inv[0]+m[1]*inv[4]+m[2]*inv[8]+m[3]*inv[12];if(det==0.0||det!=det)return false;const double id=1.0/det;
 /* trust the inverse only if M * inverse really is the identity (a nearly flat model matrix is not worth a wrong picture) */
 for(int c=0;c<4;c++)for(int r=0;r<4;r++){double a=0;for(int k=0;k<4;k++)a+=m[4*k+r]*inv[4*c+k];a*=id;if(fabs(a-(r==c?1.0:0.0))>1e-6)return false;}
 for(int c=0;c<4;c++)for(int r=0;r<4;r++){double a=0;for(int k=0;k<4;k++)a+=(double)Mm[4*k+r]*inv[4*c+k];a*=id;if(!(fabs(a)<64.0))return false;K[4*c+r]=(float)a;}
 return true;}
/* Cache exact matrix pairs. Pairing still runs for every draw so its
   motion estimate and records advance even when a transform can be reused. */
static bool mdK(const GJob& job,float *out){
 float midpoint[16];
 mdMid(job,midpoint);
 if(!memcmp(midpoint,job.M,sizeof midpoint))return false;

 struct Entry{float source[16],midpoint[16],transform[16];bool usable;};
 static Entry recent[4];
 static unsigned count,next,hits,misses;
 bool usable;
 for(unsigned i=0;i<count;i++){
  const Entry& item=recent[(next+3-i)%4];
  if(memcmp(job.M,item.source,sizeof item.source)||
     memcmp(midpoint,item.midpoint,sizeof item.midpoint))continue;
  hits++;
  if(item.usable)memcpy(out,item.transform,sizeof item.transform);
  return item.usable;
 }
 if(++misses%1024==0)
  PSPNativeMemLog("[FPS60K] matrix solves %u, reused %u",misses,hits);
 Entry& item=recent[next];
 memcpy(item.source,job.M,sizeof item.source);
 memcpy(item.midpoint,midpoint,sizeof item.midpoint);
 usable=mdSolveK(job.M,midpoint,item.transform);
 item.usable=usable;
 if(usable)memcpy(out,item.transform,sizeof item.transform);
 next=(next+1)%4;
 if(count<4)count++;
 return usable;
}
/* 2026-11-04 g3_batchcull (Settings > 3D "Off-screen skip", default ON, remove once tested; log [G3CULL]): in towns most raw
   triangles the game sends are off screen (Goldenrod run 148: ~7,200 trivially rejected per frame against ~770 drawn), and each one
   was transformed (16 multiply-adds per vertex) before the per-triangle reject below. Each batch, and then each run of 16 triangles
   in it, gets an object-space bounding box; a box with all 8 corners outside the same clip plane is skipped. Clip-space x, y, z
   and w are linear in the vertex, so every vertex in the box is outside that plane too and the per-triangle test would have
   rejected every triangle in it: same picture. Boxes of list-cache replay arrays (immutable until G3ListDeferFree) are kept. */
static int bcV=-1;
extern "C" int P3DS_BatchCullCfg(void){if(bcV<0){bcV=P3DS_CfgInt("g3_batchcull",1)!=0;PSPNativeMemLog("[G3CULL] g3_batchcull=%d (skip raw 3D triangles whose bounding box is off screen)",bcV);}return bcV;}
extern "C" void P3DS_SetBatchCull(int v){bcV=v!=0;}
enum{kChunkV=48};   /* 16 triangles */
struct BBox{const void*p;unsigned n;float(*cb)[6];};   /* cb[0] = whole batch, cb[1+k] = chunk k: min xyz, max xyz */
static BBox bbTab[1024];static const GuVertex*replayV;static unsigned bcTested,bcSkipB,bcSkipC,bcSkipT,bcHits;
static float bcTmp[2+24576/kChunkV][6];static u8 chunkOff[1+24576/kChunkV];
static inline BBox*bbSlot(const void*p){return &bbTab[(unsigned)(((uintptr_t)p>>4)*2654435761u)>>22];}
static void BoxesOf(const GuVertex*v,unsigned n,float(*cb)[6]){
 float*W=cb[0];W[0]=W[1]=W[2]=1e30f;W[3]=W[4]=W[5]=-1e30f;
 for(unsigned c=0,i=0;i<n;c++){const unsigned e=i+kChunkV<n?i+kChunkV:n;float x0=v[i].x,x1=x0,y0=v[i].y,y1=y0,z0=v[i].z,z1=z0;
  for(unsigned k=i+1;k<e;k++){const GuVertex&q=v[k];if(q.x<x0)x0=q.x;if(q.x>x1)x1=q.x;if(q.y<y0)y0=q.y;if(q.y>y1)y1=q.y;if(q.z<z0)z0=q.z;if(q.z>z1)z1=q.z;}
  float*B=cb[1+c];B[0]=x0;B[1]=y0;B[2]=z0;B[3]=x1;B[4]=y1;B[5]=z1;
  if(x0<W[0])W[0]=x0;if(y0<W[1])W[1]=y0;if(z0<W[2])W[2]=z0;if(x1>W[3])W[3]=x1;if(y1>W[4])W[4]=y1;if(z1>W[5])W[5]=z1;i=e;}}
static float(*BoxesFor(const GuVertex*v,unsigned n,bool replay))[6]{
 if(!replay){BoxesOf(v,n,bcTmp);return bcTmp;}
 BBox*b=bbSlot(v);if(b->p==v&&b->n==n&&b->cb){bcHits++;return b->cb;}
 free(b->cb);b->p=nullptr;b->cb=(float(*)[6])malloc((2+n/kChunkV)*sizeof(float[6]));if(!b->cb){BoxesOf(v,n,bcTmp);return bcTmp;}
 BoxesOf(v,n,b->cb);b->p=v;b->n=n;return b->cb;}
/* plane mask that all 8 corners of box B share (0 = may be visible) */
static unsigned BoxOutOrig(const float*B,const float*M,float qwK,bool zp){
 unsigned all=zp?0x3F:0x0F;
 for(int c=0;c<8&&all;c++){const float x=B[(c&1)?3:0],y=B[(c&2)?4:1],z=B[(c&4)?5:2];
  const float q0=x*M[0]+y*M[4]+z*M[8]+M[12],q1=x*M[1]+y*M[5]+z*M[9]+M[13],q2=x*M[2]+y*M[6]+z*M[10]+M[14],q3=x*M[3]+y*M[7]+z*M[11]+M[15];
  /* margin: a corner must be clearly outside, so float rounding inside the box can never put a vertex back on the inside */
  const float qw=qwK*q3,e=1e-4f*(fabsf(q0)+fabsf(q1)+fabsf(q2)+fabsf(q3))+1e-4f;unsigned o=0;
  if(q0>qw+e)o|=1;if(q0<-qw-e)o|=2;if(q1>q3+e)o|=4;if(q1<-q3-e)o|=8;if(zp){if(q2>q3+e)o|=16;if(q2<-q3-e)o|=32;}all&=o;}
 return all;}

#include "g3_boxplanes.h"
static FastBoxPlanes fastPlanes;static bool fastBatchOn;static int fastBoundsV=-1;
extern "C" int P3DS_FastBoundsCfg(void){if(fastBoundsV<0){fastBoundsV=P3DS_CfgInt("g3_fast_bounds",1)!=0;PSPNativeMemLog("[G3BOUNDS] g3_fast_bounds=%d (support planes, original calculation near boundaries)",fastBoundsV);}return fastBoundsV;}
extern "C" void P3DS_SetFastBounds(int v){fastBoundsV=v!=0;PSPNativeMemLog("[G3BOUNDS] g3_fast_bounds=%d from settings",fastBoundsV);}
static unsigned fastBox(const float*B,const float*M,float qwK,bool zp){unsigned m=fastPlanes.test(B);return (m&0x80000000u)?BoxOutOrig(B,M,qwK,zp):m;}
static unsigned BoxOut(const float*B,const float*M,float qwK,bool zp){
 if(!fastBatchOn)return BoxOutOrig(B,M,qwK,zp);
#ifdef P3DS_BOUNDS_ORACLE
 static unsigned checks,bad,samples;static u64 oldTicks,newTicks;
 if(checks<500000){unsigned ref,next;bool timed=((checks&63)==0);
  if(timed){u64 a=svcGetSystemTick();ref=BoxOutOrig(B,M,qwK,zp);u64 b=svcGetSystemTick();next=fastBox(B,M,qwK,zp);u64 c=svcGetSystemTick();oldTicks+=b-a;newTicks+=c-b;samples++;}
  else{ref=BoxOutOrig(B,M,qwK,zp);next=fastBox(B,M,qwK,zp);}
  checks++;if(next!=ref){if(bad++<8)PSPNativeMemLog("[BOUNDSCHECK] mismatch old=%02x new=%02x",ref,next);fastBoundsV=0;fastBatchOn=false;next=ref;}
  if(checks%100000==0)PSPNativeMemLog("[BOUNDSCHECK] checks=%u mismatches=%u samples=%u old_ticks=%llu new_ticks=%llu",checks,bad,samples,oldTicks,newTicks);
  return next;
 }
#endif
 return fastBox(B,M,qwK,zp);
}
static float qwKOf(const GJob&J){
 return ((J.wide?1.25f:1.0f)+(J.edgeFix&&J.sd?J.sd*(1.0f/128.0f):0.0f))*(J.mid?kMidEdge:1.0f);}
/* batch-level off-screen reject (g3_batchcull). Returns 1 if the whole batch is outside one clip plane. Fills chunkOff / chunked for the per-chunk skip. */
static int CullBatch(const GJob&J,bool&chunked){
 const GuVertex*v=J.v;const unsigned n=J.n;const float *matrix=J.M;const float qwK=qwKOf(J);chunked=false;
 float mcM[16];if(J.mid){memcpy(mcM,J.M,sizeof mcM);for(int k=0;k<4;k++)mcM[4*k+1]*=1.0f/kMidEdge;matrix=mcM;}   /* fps60: clip y shrunk = the same rim top and bottom as qwK gives left and right */
 if(n>=(J.replay?9u:24u)&&P3DS_BatchCullCfg()){float(*cb)[6]=BoxesFor(v,n,J.replay && (!J.xfKey || J.xfKey==v));const bool zp=!J.wbuf;bcTested++;fastBatchOn=P3DS_FastBoundsCfg()!=0;if(fastBatchOn)fastPlanes.prepare(matrix,qwK,zp);
  const unsigned m=BoxOut(cb[0],matrix,qwK,zp);if(m){bcSkipB++;bcSkipT+=n/3;rejOut+=n/3;rejMask|=m;return 1;}
  if(n>kChunkV){const unsigned nc=(n+kChunkV-1)/kChunkV;for(unsigned c=0;c<nc;c++){const unsigned mc=BoxOut(cb[1+c],matrix,qwK,zp);chunkOff[c]=mc!=0;rejMask|=mc;}chunked=true;}}
 return 0;}
/* g3_xfcache: keep the last accepted clip-space triangles for a list-cache replay draw. Same pointer + identical M + same
   stereo/mid/wide rim -> skip matrix multiply, plane reject and face cull, and queue the stored triangles. Hard (CPU-clip)
   triangles are not stored: a batch with any hard tri is left uncached. OFF never touches the tables. */
enum{XF_SLOTS=256,XF_TRIS=3072};
struct XfTri{float c[12];u32 col[3];float su[3],sv[3];};
struct XfSlot{const void*key;unsigned n,nTri,base,gen,cullFace;float M[16];};
static XfSlot xfTab[XF_SLOTS];static XfTri xfArena[XF_TRIS];static unsigned xfArenaN,xfHits,xfMiss,xfStore,xfSkipHard;
static inline unsigned xfHash(const void*p){return (unsigned)(((uintptr_t)p>>4)*2654435761u)>>24;}
static inline unsigned xfGenOf(int sd,bool mid,bool wide,int cull){return ((unsigned)(sd&255))|((mid?1u:0u)<<8)|((wide?1u:0u)<<9)|((unsigned)(cull&3)<<10);}
static inline unsigned xfGenNow(void){return xfGenOf(stereoSd,G3G_MidRec!=0,G3G_Wide!=0,cullFace);}
static void xfClear(void){xfArenaN=0;for(unsigned i=0;i<XF_SLOTS;i++)xfTab[i].key=nullptr;}
extern "C" void P3DS_SetG3XfCache(int v){g3XfCfg=v!=0;if(!g3XfCfg)xfClear();PSPNativeMemLog("[G3XF] g3_xfcache=%d from settings",g3XfCfg);}
static void DrawScreenJ(const GJob&J,const GuVertex*v,unsigned n,bool deferOk); /* used by hard-clip inside GpuRawJ */
static u32 gtSnapCrc(const GJob&J){
 u32 h=2166136261u;auto feed=[&](const void*p,unsigned n){const u8*b=(const u8*)p;for(unsigned i=0;i<n;i++)h=(h^b[i])*16777619u;};
 feed(J.M,sizeof J.M);feed(&J.n,sizeof J.n);feed(&J.sd,sizeof J.sd);feed(&J.parL,sizeof J.parL);feed(&J.mid,1);feed(&J.wide,1);
 feed(&J.cullFace,sizeof J.cullFace);feed(&J.fog,sizeof J.fog);feed(&J.depthTest,sizeof J.depthTest);feed(&J.tp,sizeof J.tp);
 feed(&J.tdw,1);feed(&J.wbuf,1);feed(&J.gt,sizeof J.gt);feed(&J.tw,sizeof J.tw);feed(&J.th,sizeof J.th);feed(&J.hasTex,1);feed(&J.texGen,sizeof J.texGen);feed(&J.texGpuGen,sizeof J.texGpuGen);feed(&J.texCrc,sizeof J.texCrc);
 feed(&J.hasK,1);if(J.hasK)feed(J.K,sizeof J.K);feed(&J.sprDepth,sizeof J.sprDepth);feed(&J.cull3d,sizeof J.cull3d);
 feed(&J.cpuClip,1);feed(&J.guardClip,1);feed(&J.guardBat,1);feed(&J.edgeFix,1);feed(&J.fieldSpr,1);
 return h;}
/* Deterministic clip-space hash of the job's vertices × M (first 48 verts). Same on submit and worker → bit-identical xform. */
static u32 gtXformCrc(const GJob&J){
 u32 h=2166136261u;const float*M=J.M;const unsigned n=J.n<48u?J.n:48u;
 for(unsigned i=0;i<n;i++){const GuVertex&p=J.v[i];float c[4]={
  p.x*M[0]+p.y*M[4]+p.z*M[8]+M[12],p.x*M[1]+p.y*M[5]+p.z*M[9]+M[13],
  p.x*M[2]+p.y*M[6]+p.z*M[10]+M[14],p.x*M[3]+p.y*M[7]+p.z*M[11]+M[15]};
  const u8*b=(const u8*)c;for(unsigned k=0;k<sizeof c;k++)h=(h^b[k])*16777619u;
  h=(h^(u8)(p.color))*16777619u;h=(h^(u8)(p.color>>8))*16777619u;}
 h=(h^(u8)J.sd)*16777619u;h=(h^(u8)J.parL)*16777619u;h=(h^(u8)J.wide)*16777619u;h=(h^(u8)J.hasTex)*16777619u;
 h=(h^(u8)J.tw)*16777619u;h=(h^(u8)J.th)*16777619u;return h;}
static void GpuRawJ(const GJob&J0){
 if(J0.cullAll)return;
 const GJob*prev=gtCur;gtCur=&J0; /* Tri/parOf/TriEye and hard-clip DrawScreenJ read stereo/tex from the job */
 const GJob&J=J0;
 /* Per-job share flag (release via gtTail): worker must not rely only on Begin's SetStereoShare. */
 G3G_SetStereoShare(J.sd&&J.stereoShare);
 if(P3DS_G3ThrCheckCfg()){
  gtCheckN++;
  if(J.snapCrc&&J.snapCrc!=gtSnapCrc(J)){gtCheckBad++;static unsigned mism;if(mism++<16)PSPNativeMemLog("[G3THR] snap mismatch job n=%u sd=%d (worker POD != submit CRC)",J.n,J.sd);}
  if(J.xformCrc&&J.xformCrc!=gtXformCrc(J)){gtCheckBad++;static unsigned xm;if(xm++<16)PSPNativeMemLog("[G3THR] snap mismatch xform n=%u (worker clip hash != submit)",J.n);}
  if(J.hasTex&&!J.gt){gtCheckBad++;static unsigned nt;if(nt++<8)PSPNativeMemLog("[G3THR] snap mismatch empty gt n=%u tw=%u (hasTex but no GPU tex)",J.n,J.tw);}
  if(J.hasTex&&J.texCrc){
   const unsigned liveGpu=J.tex?J.tex->gpuGen:0;
   const u32 live=gtTexCrcOf(J.tex,J.gt,J.tw,J.th,liveGpu);
   if(live!=J.texCrc||(J.texGpuGen&&liveGpu!=J.texGpuGen)){
    gtTexBad++;gtCheckBad++;
    static unsigned tm;if(tm++<16)PSPNativeMemLog("[G3THR] tex mismatch off=%x gt=%p gpuGen %u->%u crc %08x->%08x (px/gpu changed between submit and draw)",
     J.tex?J.tex->off:0,J.gt,J.texGpuGen,liveGpu,(unsigned)J.texCrc,(unsigned)live);}}
 }
 /* xfcache is safe with depth-1 + ListFreeNow clearing the slot; hits help when the camera is still. */
 const bool doProf=P3DS_G3ProfCfg()!=0;const bool doXf=P3DS_G3XfCacheCfg()!=0;const bool fastXf=P3DS_G3FastXformCfg()!=0;
 /* Coarse timers only (one tick read per stage per batch). Per-triangle svcGetSystemTick cost ~10+ ms/frame with g3_prof=1. */
 u64 tRaw0=0,t0=0,t1=0;if(doProf)tRaw0=t0=svcGetSystemTick();
 const GuVertex*v=J.v;const unsigned n=J.n;const void*xfKey=J.xfKey?J.xfKey:v;
 const int sd=J.sd,parL=J.parL;const bool mid=J.mid,wide=J.wide;
 const float qwK=qwKOf(J);
 // Both submitters cull before transforming. The worker uses only its
 // job's copied mask and never touches the producer's culling scratch.
 const bool chunked=J.chunked;const u8*cmask=J.cmask;
 if(doProf){t1=svcGetSystemTick();prCull+=t1-t0;t0=t1;}
 /* Midpoint K is computed on the game thread at submit (gtFinishMid). Worker must not touch md* tables. */
 if(mid)G3G_MidK(J.hasK?J.K:nullptr);const float yK=mid?kMidEdge:1.0f;
 if(doProf){t1=svcGetSystemTick();prPair+=t1-t0;t0=t1;}
 G3G_CurFog=J.fog;const int cullMode3d=J.cull3d;const G3SIM_TexImageParam_t tp=J.tp;
 bool translucent=(v[0].color>>24)<250||(J.hasTex&&(tp.textureFormat==1||tp.textureFormat==6));
 bool dw=!translucent||J.tdw;unsigned dtest=J.depthTest;
 const float*M=J.M;
 const unsigned xfGen=xfGenOf(sd,mid,wide,J.cullFace);
 /* g3_xfcache hit: same clip matrix as last time -> queue the stored triangles.
    Only list-cache replay has stable vertex backing. */
 if(doXf&&J.xfKey&&n>=3&&!J.wbuf){
  XfSlot&slot=xfTab[xfHash(xfKey)];
  if(slot.key==xfKey&&slot.n==n&&slot.gen==xfGen&&slot.cullFace==(unsigned)J.cullFace&&!memcmp(slot.M,M,sizeof slot.M)&&slot.base+slot.nTri<=XF_TRIS){
   xfHits++;
   float sprZf=-1.0f;
   if(sd&&!parL&&n<=6&&J.hasTex&&J.tw<=64&&J.th<=64&&J.sprDepth&&J.fieldSpr){float lowY=1e9f,zfLo=2.0f,zfMin=2.0f,zfMax=-1.0f;bool ok=true;
     for(unsigned k=0;k<n&&ok;k++){const GuVertex&p=v[k];float w=p.x*M[3]+p.y*M[7]+p.z*M[11]+M[15];if(w<1e-3f){ok=false;break;}
      float y=(p.x*M[1]+p.y*M[5]+p.z*M[9]+M[13])/w,z=(p.x*M[2]+p.y*M[6]+p.z*M[10]+M[14])/w,zf=0.5f*z+0.5f;
      if(y<lowY){lowY=y;zfLo=zf;}if(zf<zfMin)zfMin=zf;if(zf>zfMax)zfMax=zf;}
     if(ok&&zfLo>=0.0f&&zfLo<=1.0f){sprZf=zfLo;sprN++;sprSpread+=parOf(zfMax)-parOf(zfMin);}else sprSkip++;}
   if(doProf){t1=svcGetSystemTick();prXform+=t1-t0;t0=t1;}
   for(unsigned t=0;t<slot.nTri;t++){const XfTri&T=xfArena[slot.base+t];drawnTris++;
    if(!sd){G3G_TriClip(0,T.c,T.su,T.sv,T.col,J.gt,tp.repeatS,tp.repeatT,tp.flipS,tp.flipT,translucent,dw,dtest,nullptr);continue;}
    float zf3[3];bool degen=false;for(int k=0;k<3;k++){float w=T.c[4*k+3];zf3[k]=w>1e-6f?0.5f*T.c[4*k+2]/w+0.5f:1.0f;if(!degen&&zDegenerate(w,zf3[k]))degen=true;}if(degen)stereoClamped++;
    float shL[3],shR[3];for(int k=0;k<3;k++){if(parL){shL[k]=lockSh(0,sd,parL);shR[k]=lockSh(1,sd,parL);}else{const float p=degen?0.65f:parOf(sprZf>=0.0f?sprZf:zf3[k]);shL[k]=-sd*p;shR[k]=sd*p;}}
    G3G_TriClip2(T.c,T.su,T.sv,T.col,J.gt,tp.repeatS,tp.repeatT,tp.flipS,tp.flipT,translucent,dw,dtest,shL,shR);}
   if(doProf){prQueue+=svcGetSystemTick()-t0;prRaw+=svcGetSystemTick()-tRaw0;}
   gtCur=prev;return;}}
/* run 62 stereo_edgefix=1 (default): the per-eye stereo shift (up to stereoSd DS px, right eye +, left eye -) is added
   AFTER this x-plane reject, so tris just outside the left plane that the right-eye shift would bring on screen were
   dropped -> a few-px black column at the far left of the right eye (wide view, slider up; mirror case at the right edge
   of the left eye). Widen the x reject planes by the maximum shift (stereoSd/128 clip units per w; par <= 1). */
 /* sprite_depth: one parallax for the whole quad (feet vertex), field only */
 float sprZf=-1.0f;
 if(sd&&!parL&&n<=6&&J.hasTex&&J.tw<=64&&J.th<=64&&J.sprDepth&&J.fieldSpr){float lowY=1e9f,zfLo=2.0f,zfMin=2.0f,zfMax=-1.0f;bool ok=true;
   for(unsigned k=0;k<n&&ok;k++){const GuVertex&p=v[k];float w=p.x*M[3]+p.y*M[7]+p.z*M[11]+M[15];if(w<1e-3f){ok=false;break;}
    float y=(p.x*M[1]+p.y*M[5]+p.z*M[9]+M[13])/w,z=(p.x*M[2]+p.y*M[6]+p.z*M[10]+M[14])/w,zf=0.5f*z+0.5f;
    if(y<lowY){lowY=y;zfLo=zf;}if(zf<zfMin)zfMin=zf;if(zf>zfMax)zfMax=zf;}
   if(ok&&zfLo>=0.0f&&zfLo<=1.0f){sprZf=zfLo;sprN++;sprSpread+=parOf(zfMax)-parOf(zfMin);
    if(sprN==1||sprN%3000==0){PSPNativeMemLog("[SPRDEPTH] sprite quads fixed %u (skipped %u), avg parallax spread removed %.4f of the shift, feet zf %.4f, shift %d px",sprN,sprSkip,sprSpread/sprN,zfLo,sd);}}
   else sprSkip++;}
 /* optional store into xfcache: replay, no hard tris, room in the arena */
 XfSlot*xfOut=nullptr;unsigned xfBase=0,xfNTri=0;bool xfHard=false;
 if(doXf&&J.xfKey&&n>=3&&!J.wbuf){
  xfMiss++;
  if(xfArenaN+n/3>XF_TRIS){xfClear();xfBase=0;}
  if(xfArenaN+n/3<=XF_TRIS){xfOut=&xfTab[xfHash(xfKey)];xfBase=xfArenaN;}}
 u64 tLoop=doProf?svcGetSystemTick():0;unsigned clipN=0;
 /* Stereo field already transforms once (G3G_TriClip2 writes both eyes from the same clip verts). g3_fastxform only
    hoists M[] into locals so the VFP reloads less — same mul/add order, bit-identical floats. New 3DS has no NEON. */
 const float m0=M[0],m1=M[1],m2=M[2],m3=M[3],m4=M[4],m5=M[5],m6=M[6],m7=M[7],m8=M[8],m9=M[9],m10=M[10],m11=M[11],m12=M[12],m13=M[13],m14=M[14],m15=M[15];
 for(unsigned i=0;i+2<n;i+=3){float c[12];unsigned out=0x3F;u32 col[3];float su[3],sv[3];
  if(chunked&&i%kChunkV==0&&cmask[i/kChunkV]){const unsigned t=((i+kChunkV<n?i+kChunkV:n)-i)/3;rejOut+=t;bcSkipC++;bcSkipT+=t;i+=3*t-3;continue;}
  for(int k=0;k<3;k++){const GuVertex&p=v[i+k];float*q=c+4*k;
   if(fastXf){q[0]=p.x*m0+p.y*m4+p.z*m8+m12;q[1]=p.x*m1+p.y*m5+p.z*m9+m13;q[2]=p.x*m2+p.y*m6+p.z*m10+m14;q[3]=p.x*m3+p.y*m7+p.z*m11+m15;}
   else{q[0]=p.x*M[0]+p.y*M[4]+p.z*M[8]+M[12];q[1]=p.x*M[1]+p.y*M[5]+p.z*M[9]+M[13];q[2]=p.x*M[2]+p.y*M[6]+p.z*M[10]+M[14];q[3]=p.x*M[3]+p.y*M[7]+p.z*M[11]+M[15];}
   unsigned o=0;const float qw=qwK*q[3];/* run 43 wideview: keep tris in the side strips */const float qy=yK*q[3];if(q[0]>qw)o|=1;if(q[0]<-qw)o|=2;if(q[1]>qy)o|=4;if(q[1]<-qy)o|=8;if(q[2]>q[3])o|=16;if(q[2]<-q[3])o|=32;out&=o;
   col[k]=p.color;su[k]=p.u;sv[k]=p.v;}
  if(J.wbuf){/* run 31: DS W-buffer (SwapBuffers bit1): depth = w, linear. With z-buffer depth the title's projection
     gave z==w for every vertex (all depth ~1.0), so Giratina's body lost the depth test. */
   for(int k=0;k<3;k++){float*q=c+4*k;q[2]=wDepthZ(q[3]);}}
  if(out){rejOut++;rejMask|=out;if(vdump&&vdumpN<400){vdumpN++;PSPNativeMemLog("[VDUMP] REJ out=%02x v0 %.2f %.2f %.2f %.2f v1 %.2f %.2f %.2f %.2f v2 %.2f %.2f %.2f %.2f",out,c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7],c[8],c[9],c[10],c[11]);}continue;}/* all three vertices outside one plane */
  if(vdump&&vdumpN<400&&(drawnTris%8)==0){vdumpN++;PSPNativeMemLog("[VDUMP] OK v0 %.2f %.2f %.2f %.2f v1 %.2f %.2f %.2f %.2f v2 %.2f %.2f %.2f %.2f",c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7],c[8],c[9],c[10],c[11]);}
  if(J.cpuClip){/* run 27: only tris touching the near/far planes or outside a 2x guard band go through CPU clipping
     (PICA clipping there gave the black spots); the rest stay on the fast clip-space path. 2026-11-05: guard band only with g3_guardclip=1. */
   bool hard=false;for(int k=0;k<3;k++){const float*q=c+4*k;float w=q[3];if(w<1e-3f||q[2]>w||q[2]<-w||((J.guardClip||J.guardBat)&&(fabsf(q[0])>2*w||fabsf(q[1])>2*w))){hard=true;break;}}
    if(hard){if(xfOut){xfHard=true;xfOut=nullptr;xfSkipHard++;}
    /* Hard-clip draws both eyes via G3G_Tri; G3G_Tri expands any pending left-only share verts first. */
    GuVertex hardBuf[24];unsigned m=RawToScreenBuf(J,v+i,3,hardBuf,(unsigned)(sizeof hardBuf/sizeof hardBuf[0]));if(m)DrawScreenJ(J,hardBuf,m,false);cpuClipped++;clipN++;continue;}}
  if(J.cullFace&&cullMode3d&&c[3]>1e-6f&&c[7]>1e-6f&&c[11]>1e-6f){
   float x0=c[0]/c[3],y0=c[1]/c[3],x1=c[4]/c[7],y1=c[5]/c[7],x2=c[8]/c[11],y2=c[9]/c[11];
   float area=(x1-x0)*(y2-y0)-(x2-x0)*(y1-y0);if(cullMode3d==1)area=-area;/* run 26: the old sign culled ~all field tris (36 left) */
   if((J.cullFace==1&&area>0)||(J.cullFace==2&&area<0)){culledTris++;continue;}}
  drawnTris++;
  if(xfOut){XfTri&T=xfArena[xfBase+xfNTri++];memcpy(T.c,c,sizeof T.c);memcpy(T.col,col,sizeof T.col);memcpy(T.su,su,sizeof T.su);memcpy(T.sv,sv,sizeof T.sv);}
  if(!sd){G3G_TriClip(0,c,su,sv,col,J.gt,tp.repeatS,tp.repeatT,tp.flipS,tp.flipT,translucent,dw,dtest,nullptr);continue;}
  /* 2026-11-04: zf and the parallax once per vertex, both eyes written in one G3G_TriClip2 pass (was: all of it per eye) */
  float zf3[3];bool degen=false;for(int k=0;k<3;k++){float w=c[4*k+3];zf3[k]=w>1e-6f?0.5f*c[4*k+2]/w+0.5f:1.0f;if(!degen&&zDegenerate(w,zf3[k]))degen=true;}if(degen)stereoClamped++;
  float shL[3],shR[3];for(int k=0;k<3;k++){if(parL){shL[k]=lockSh(0,sd,parL);shR[k]=lockSh(1,sd,parL);}else{const float p=degen?0.65f:parOf(sprZf>=0.0f?sprZf:zf3[k]);shL[k]=-sd*p;shR[k]=sd*p;}}
  G3G_TriClip2(c,su,sv,col,J.gt,tp.repeatS,tp.repeatT,tp.flipS,tp.flipT,translucent,dw,dtest,shL,shR);
 }
 if(doProf){u64 tn=svcGetSystemTick();const u64 loop=tn-tLoop;
  /* Coarse: hard-clip share by triangle count; the rest is transform+reject+face-cull+TriClip (reported as xform; queue stays for xfcache hits). */
  if(clipN&&n>=3){const u64 c=(loop*(u64)clipN)/(u64)(n/3);prClip+=c;prXform+=loop-c;}else prXform+=loop;
  prRaw+=tn-tRaw0;}
 if(xfOut&&!xfHard){xfOut->key=xfKey;xfOut->n=n;xfOut->nTri=xfNTri;xfOut->base=xfBase;xfOut->gen=xfGen;xfOut->cullFace=(unsigned)J.cullFace;memcpy(xfOut->M,M,sizeof xfOut->M);xfArenaN=xfBase+xfNTri;xfStore++;}
 gtCur=prev;
}
/* GeBuffer / G3 command decode time (subset of front) when g3_prof is on. */
extern "C" void P3DS_G3ProfAddCmd(unsigned long long ticks){if(P3DS_G3ProfCfg())prCmd+=ticks;}
static GuVertex scrBuf[8192];
/* ================= g3_thread: worker for the raw 3D transform (GpuRawJ) =================
   The game thread builds a GJob per raw draw call (full state snapshot; EVERY vertex list is copied into the per-frame
   arena — including list-cache replay — so the worker never reads game memory the game thread can mutate). A worker on
   core 2 runs GpuRawJ. Acquire/release barriers pair the ring publish with the worker read and the drain with vb[]
   writes (ARM11 is weakly ordered; missing acquire let the worker see a new gtTail with a stale/partial GJob → flashing
   terrain). The worker is the only writer of G3G batches while the queue is busy; the game thread drains before
   DrawScreen / G3SIM_FlushDeferred / G3ListDeferFree / G3G_TexFree. OFF runs GpuRawJ in Draw. Log: [G3THR]. */
static int gtCfgWant=-1,gtThreadOk=0;static unsigned gtAbN;   /* g3_thread 2 = A/B test: OFF and ON alternate every 1200 3D frames (~40 s) in one session */
enum{GT_N=1024,GT_MASK=GT_N-1,GT_ARENA=96*1024};          /* 96K verts (~2.2 MB): fewer mid-frame arena drains that reset depth */
static GJob gtRing[GT_N];static GuVertex*gtArena;static u8*gtCmArena;static unsigned gtArenaPos,gtCmPos;
static volatile unsigned gtHead,gtTail,gtDone,gtKicked,gtSleep,gtExpandReq; /* gtDone: completed claims (g3_raw_split); gtExpandReq: worker expand */
extern "C" {int G3T_Start(void(*)(void*));int G3T_Start2(void(*)(void*));void G3T_Stop2(void);void G3T_Wait(void);void G3T_Signal(void);void G3T_Wait2(void);void G3T_Signal2(void);void G3T_SleepNs(long long);int G3T_SplitOn(void);int G3T_Quiting(void);void G3T_RequestQuit(void);void G3T_Join(void);void*memalign(size_t,size_t);}
extern "C" {int P3DS_BigLockDropForWait(void);void P3DS_BigLockRestoreAfterWait(int);int P3DS_BigLockDepth(void);}
static u64 gtBusyTicks,gtWaitTicks;static unsigned gtJobs,gtDrains[6],gtFrames;static const char*const gtWhy[6]={"screen","frame","listfree","texfree","ringfull","arena"};
extern "C" void P3DS_ThreadGuard(const char*);
/* Conditional wake (publish path): only Signal if the worker marked itself asleep. */
static void gtWake(){__sync_synchronize();if(__sync_lock_test_and_set(&gtSleep,0)==1){G3T_Signal();if(G3T_SplitOn())G3T_Signal2();}}
/* Drain/pin waits: always Signal so a lost TAS cannot leave the worker asleep with work queued (run 883). */
static void gtWakeForce(){__sync_synchronize();__sync_lock_test_and_set(&gtSleep,0);G3T_Signal();if(G3T_SplitOn())G3T_Signal2();}
/* ~1 s at 268 MHz. On timeout: log, abandon queue / force-unpin, turn g3_thread OFF for the rest of the session. */
enum{GT_WAIT_TICKS=268111856ull};
static inline bool gtBusy();
static void gtFallbackInline(const char*why){
 gtWaitTimeouts++;
 PSPNativeMemLog("[G3THR] wait timeout (%s): g3_thread OFF, draining then force-empty if needed",why);
 if(gtCfg){gtCfg=0;PSPNativeMemLog("[G3THR] now OFF (timeout fallback)");}
 /* Stop new work; keep waking so the worker can finish what it has. */
 const u64 t1=svcGetSystemTick();
 while(gtBusy()&&svcGetSystemTick()-t1<GT_WAIT_TICKS/2){gtWakeForce();G3T_SleepNs(100000);}
 if(gtBusy()){
  unsigned t=__atomic_load_n(&gtTail,__ATOMIC_ACQUIRE);
  __atomic_store_n(&gtHead,t,__ATOMIC_RELEASE);
  __atomic_store_n(&gtDone,t,__ATOMIC_RELEASE);
  PSPNativeMemLog("[G3THR] force-empty queue after timeout (worker may skip leftover jobs)");}
 gtSleep=0;G3T_Signal();}
/* Claim next job (CAS on gtHead). Returns 1 and fills *outH, or 0 if empty. */
static int gtClaim(unsigned*outH){
 for(;;){
  unsigned h=__atomic_load_n(&gtHead,__ATOMIC_RELAXED);
  unsigned t=__atomic_load_n(&gtTail,__ATOMIC_ACQUIRE);
  if(h==t)return 0;
  if(__atomic_compare_exchange_n(&gtHead,&h,h+1,0,__ATOMIC_ACQ_REL,__ATOMIC_RELAXED)){*outH=h;return 1;}
 }}
static void gtRunOne(unsigned h){
 const u64 t0=svcGetSystemTick();
 {const GJob J=gtRing[h&GT_MASK];if(J.screen)DrawScreenJ(J,J.v,J.n,J.deferOk);else GpuRawJ(J);
  if(J.gt&&gtCfg&&P3DS_G3TexFenceCfg()){if(P3DS_G3TexSafeCfg())gtPinDonePush(J.gt);else gtPinRel(J.gt);}}
 gtBusyTicks+=svcGetSystemTick()-t0;
 __atomic_fetch_add(&gtDone,1,__ATOMIC_RELEASE);}
static void gtTryExpand(void){
 if(__atomic_load_n(&gtExpandReq,__ATOMIC_ACQUIRE)){
  const u64 t0=svcGetSystemTick();G3G_StereoExpand();
  gtBusyTicks+=svcGetSystemTick()-t0;
  __atomic_store_n(&gtExpandReq,0,__ATOMIC_RELEASE);}}
static void gtMain(void*){P3DS_ThreadGuard("g3thread");gtInWorker=true;
 for(;;){
  if(G3T_Quiting())break;
  unsigned h;
  if(!gtClaim(&h)){
   gtTryExpand();
   gtSleep=1;__sync_synchronize();
   if(G3T_Quiting())break;
   if(gtClaim(&h)){gtSleep=0;gtRunOne(h);continue;}
   if(__atomic_load_n(&gtExpandReq,__ATOMIC_ACQUIRE)){gtSleep=0;continue;}
   G3T_Wait();continue;}
  gtRunOne(h);
 }
 gtInWorker=false;}
/* Optional second raw worker (g3_raw_split / G3T_Start2); same quit path. */
static void gtMain2(void*){P3DS_ThreadGuard("g3thread2");gtInWorker=true;
 for(;;){
  if(G3T_Quiting())break;
  unsigned h;
  if(!gtClaim(&h)){
   gtSleep=1;__sync_synchronize();
   if(G3T_Quiting())break;
   if(gtClaim(&h)){gtSleep=0;gtRunOne(h);continue;}
   G3T_Wait2();continue;}
  gtRunOne(h);
 }
 gtInWorker=false;}
static void gtDrain(int why); /* fwd for shutdown */
/* Soft-reset / exit: drain leftover jobs, then quit+join workers before GPU/heap free. */
extern "C" void P3DS_G3ThrShutdown(void){
 if(gtThreadOk<=0)return;
 gtDrain(1);
 G3T_RequestQuit();
 gtWakeForce();
 G3T_Join();
 gtCfg=0;gtThreadOk=0;
 PSPNativeMemLog("[3DS] exit: g3thr workers stopped");
}
/* Midpoint pairing mutates md* tables: must run on the game thread after cull, before the arena copy. */
static void gtFinishMid(GJob&J){
 J.hasK=false;if(!J.mid||J.n<3||J.wbuf)return;
 float K[16];if(mdK(J,K)){memcpy(J.K,K,sizeof J.K);J.hasK=true;}}
static int edgeFixCfg(){static int v=-1;if(v<0){v=P3DS_CfgInt("stereo_edgefix",1)!=0;PSPNativeMemLog("[G3] stereo_edgefix=%d",v);}return v;}
static bool gtStart(){
 if(gtThreadOk!=0)return gtThreadOk>0;
 gtArena=(GuVertex*)memalign(16,GT_ARENA*sizeof(GuVertex));gtCmArena=(u8*)malloc(64*1024);
 bool started=false;if(gtArena&&gtCmArena)started=G3T_Start(gtMain)!=0;
 if(!started){free(gtArena);free(gtCmArena);gtArena=nullptr;gtCmArena=nullptr;gtThreadOk=-1;PSPNativeMemLog("[G3THR] worker thread could not start: g3_thread stays OFF");return false;}
 gtThreadOk=1;PSPNativeMemLog("[G3THR] worker thread started on core 2 (queue %d jobs, arena %u KB)",GT_N,(unsigned)(GT_ARENA*sizeof(GuVertex)>>10));
 return true;}
/* Busy until every claimed job has finished (gtDone), not merely claimed (gtHead). */
static inline bool gtBusy(){return __atomic_load_n(&gtDone,__ATOMIC_ACQUIRE)!=__atomic_load_n(&gtTail,__ATOMIC_ACQUIRE);}
extern "C" int P3DS_G3ThrBusy(void){return gtCfg&&(gtBusy()||__atomic_load_n(&gtExpandReq,__ATOMIC_ACQUIRE));}
/* Spin/sleep until pred(); drop big lock around the wait. Returns 0 on timeout (fallback already applied if fallBack). */
static int gtWaitLoop(int(*pred)(void*),void*arg,const char*why,int fallBack){
 const int held=P3DS_BigLockDropForWait();
 gtWakeForce();const u64 t0=svcGetSystemTick();unsigned spins=0;
 int ok=1;
 while(!pred(arg)){
  if(svcGetSystemTick()-t0>GT_WAIT_TICKS){ok=0;break;}
  if(++spins>4000){G3T_SleepNs(50000);spins=0;gtWakeForce();}else __asm__ volatile("yield");}
 __sync_synchronize();
 gtWaitTicks+=svcGetSystemTick()-t0;
 if(!ok&&fallBack)gtFallbackInline(why);
 P3DS_BigLockRestoreAfterWait(held);
 return ok;}
static int gtPredIdle(void*){return !gtBusy()&&!__atomic_load_n(&gtExpandReq,__ATOMIC_ACQUIRE);}
static int gtPredPinClear(void*gt){return !gtPinGet(gt);}
static void gtDrain(int why){
 if(gtBusy()||__atomic_load_n(&gtExpandReq,__ATOMIC_ACQUIRE)){
  if(!gtWaitLoop(gtPredIdle,nullptr,gtWhy[why],1)){
   /* queue abandoned; still rewind arenas below */
   __atomic_store_n(&gtExpandReq,0,__ATOMIC_RELEASE);}
  else gtDrains[why]++;}
 gtArenaPos=0;gtCmPos=0;gtKicked=__atomic_load_n(&gtTail,__ATOMIC_RELAXED);
 for(unsigned i=0;i<gtPendN;i++)ListFreeNow(gtPendFree[i]);gtPendN=0;}   /* queue empty: nothing references the arena */
static void gtDrainIfBusy(int why){if(gtBusy())gtDrain(why);}
/* Wait until the worker is idle WITHOUT rewinding the arenas. Depth-1 (g3_thr_texfence OFF): GetTexture/TexCreate
   run only when no job is in flight. With texfence ON, GetTexture may run while jobs are in flight (pins protect
   their gt); only TexCreate still waits here. Arena is reset only by gtDrain. */
static void gtWaitIdle(void){
 if(!gtBusy())return;
 gtWaitLoop(gtPredIdle,nullptr,"idle",1);}
static void gtWaitPinned(void*gt){
 if(!gt||!gtPinGet(gt))return;
 /* texsafe: pins are held until after End — callers must defer (texFree) instead of waiting. */
 if(P3DS_G3TexSafeCfg()){
  static unsigned n;if(n++<8)PSPNativeMemLog("[G3THR] gtWaitPinned skipped under texsafe (would deadlock); defer free");
  return;}
 gtPinWaits++;
 if(!gtWaitLoop(gtPredPinClear,gt,"pin",0)){
  /* Timed out without abandoning the whole thread: force-unpin this gt so LRU can proceed. */
  while(gtPinGet(gt))gtPinRel(gt);
  PSPNativeMemLog("[G3THR] pin wait timeout gt=%p: force-unpinned",gt);}}
extern "C" int P3DS_InTitle;
/* issue #484: one depth per draw from where its vertices sit across the screen (x 80 = yours, 176 = the foe's; smooth in
   between, so a lunge or an intro slide-in glides between depths). raw = v still in object space (J.M = clip matrix). */
static int parSideOf(const GJob&J,const GuVertex*v,unsigned n,bool raw){
 if(!parSideE||!n)return kParLock;const float*M=J.M;const unsigned step=n>48u?n/48u:1u;float sx=0;unsigned k=0;
 for(unsigned i=0;i<n;i+=step){const GuVertex&p=v[i];float x=p.x;
  if(raw){const float w=p.x*M[3]+p.y*M[7]+p.z*M[11]+M[15];if(w<1e-3f)continue;x=128.0f+(p.x*M[0]+p.y*M[4]+p.z*M[8]+M[12])/w*128.0f;}
  sx+=x;k++;}
 if(!k)return kParLock;float s=(sx/k-128.0f)*(1.0f/48.0f);if(s<-1)s=-1;if(s>1)s=1;
 const int e=(int)lrintf(s*parSideE);if(e>0)parFoeN++;else if(e<0)parMineN++;return kParLock+e;}
static void gtSnapshot(GJob&J,const GuVertex*v,unsigned n,bool raw){
 extern int P3DS_InField,P3DS_InBattle;
 J.v=v;J.n=n;J.replay=(v==replayV);J.xfKey=J.replay?v:nullptr;J.sd=stereoSd;J.parL=parLock;J.mid=G3G_MidRec!=0;J.wide=G3G_Wide!=0;
 memcpy(J.M,rawM,sizeof J.M);J.cullAll=cullAll;J.cullFace=cullFace;J.fog=s_curPolygonAttr.fogEnable?1:0;J.depthTest=s_curPolygonAttr.depthTest;
 J.tp=s_texImageParam;J.tdw=s_SIM_transDepthWrite!=0;J.wbuf=wbufOn();J.tex=nullptr;J.gt=nullptr;J.hasTex=false;J.tw=J.th=0;J.texGen=0;J.texGpuGen=0;J.texCrc=0;J.chunked=false;J.cmask=nullptr;J.screen=false;J.deferOk=false;
 // The title's W-buffer triangles must retain their clip-space w in GPU Z
 // mode. CPU clipping sends hard triangles through the screen-space path.
 const bool titleGpuZ=P3DS_InTitle&&J.wbuf&&G3G_WDepthOn()==3;
 J.hasK=false;J.sprDepth=P3DS_SpriteDepthCfg()!=0;J.cull3d=cull3dCfg();
 J.cpuClip=gpuCpuClip()!=0&&!titleGpuZ;J.guardClip=guardClip()!=0;J.guardBat=guardBattle();
 if(titleGpuZ){static bool said;if(!said){said=true;PSPNativeMemLog("[LUGIA] GPU clips title W-buffer triangles in GPU Z mode");}}
 J.edgeFix=edgeFixCfg()!=0;J.fieldSpr=(P3DS_InField!=0&&P3DS_InBattle==0);J.snapCrc=0;J.xformCrc=0;
 if(J.parL)J.parL=parSideOf(J,v,n,raw);
 J.stereoShare=stereoSd&&P3DS_G3StereoShareCfg();}
static void gtResolveTex(GJob&J){
 /* Depth-1 (fence 0): wait for idle before any resolve. Fence 1+: GetTexture/Decode anytime; pin the gt so a
    later LRU/re-decode cannot free it. Fence 1 still waits before TexCreate; fence 2 (DEEP) skips that wait
    (TexCreate0 only allocates/fills tex memory; the worker only writes preallocated vb[] during GpuRawJ). */
 const int fence=gtCfg?P3DS_G3TexFenceCfg():0;
 if(!fence)gtWaitIdle();
 Tex*t=GetTexture();
 if(t&&!t->gt){
  if(fence==1){gtWaitIdle();texCreateWait++;}
  else if(fence>=2)texCreateSkip++;
  t->gt=G3SIM_GpuTex(t);}
 J.tex=t;J.hasTex=t!=nullptr;J.gt=t?t->gt:nullptr;J.tw=t?t->w:0;J.th=t?t->h:0;J.texGen=t?t->gen:0;J.texGpuGen=t?t->gpuGen:0;
 // The texture hash is only consumed by the job diagnostics.
 J.texCrc=(t&&J.gt&&P3DS_G3ThrCheckCfg())?gtTexCrcOf(t,J.gt,J.tw,J.th,J.texGpuGen):0;
 if(fence&&J.gt)gtPinAdd(J.gt);}
static void gtPush(GJob&J,bool chunked);
static void gtSubmit(GJob&J){
 /* the batch-level reject runs here (same code, same counters); a rejected batch never costs a copy, a texture fetch or a queue slot */
 if(J.cullAll)return;
 bool chunked=false;if(CullBatch(J,chunked))return;
 gtResolveTex(J);gtFinishMid(J);J.chunked=chunked;
 if(P3DS_G3ThrCheckCfg()){J.snapCrc=gtSnapCrc(J);J.xformCrc=gtXformCrc(J);}
 gtPush(J,chunked);}
static void gtPush(GJob&J,bool chunked){
 /* Always copy vertices into the arena (replay too). List-cache arrays are "immutable until free", but on a weakly ordered
    MPCore the worker must not race the game thread's next use of that pointer, and a fresh arena write pairs with the
    publish barrier below. xfKey keeps the list identity for g3_xfcache. */
 const unsigned nc = chunked ? (J.n + kChunkV - 1) / kChunkV : 0;
 unsigned done = __atomic_load_n(&gtDone, __ATOMIC_ACQUIRE);
 unsigned tail = __atomic_load_n(&gtTail, __ATOMIC_RELAXED);
 /* Ring full vs completed jobs (gtDone), not claimed (gtHead): a claimed slot is still live until gtRunOne finishes. */
 if (tail - done >= GT_N) { gtDrain(4); done = __atomic_load_n(&gtDone, __ATOMIC_ACQUIRE); tail = __atomic_load_n(&gtTail, __ATOMIC_RELAXED); }
 else if (gtArenaPos + J.n > GT_ARENA || gtCmPos + nc > 64 * 1024) { gtDrain(5); done = __atomic_load_n(&gtDone, __ATOMIC_ACQUIRE); tail = __atomic_load_n(&gtTail, __ATOMIC_RELAXED); }
 if (J.n) {
  GuVertex *dst = gtArena + gtArenaPos;
  memcpy(dst, J.v, J.n * sizeof(GuVertex));
  gtArenaPos += J.n;
  if (J.replay && !J.xfKey) J.xfKey = J.v;
  J.v = dst;
 }
 if (nc) {
  u8 *dst = gtCmArena + gtCmPos;
  memcpy(dst, chunkOff, nc);
  gtCmPos += nc;
  J.cmask = dst;
 }
 /* Keep the submit-side xform CRC so the worker also checks the arena copy. */
 {
  gtRing[tail&GT_MASK]=J;
  __atomic_store_n(&gtTail,tail+1,__ATOMIC_RELEASE); /* release: ring + arena visible before the new tail */
  gtJobs++;
  {const unsigned depth=(tail+1)-__atomic_load_n(&gtHead,__ATOMIC_ACQUIRE);if(depth>gtInFlightMax)gtInFlightMax=depth;gtFenceDepthSum+=depth;gtFenceDepthN++;}
  {const unsigned every=(P3DS_G3WakeCfg()||P3DS_G3TexFenceCfg()>=2||P3DS_G3FrontOvCfg())?1u:6u;if((tail+1)-gtKicked>=every){gtKicked=tail+1;gtWake();if(every==1)prWake++;}}}}
static void gtFrameStats(){
 if(++gtFrames%300)return;
  PSPNativeMemLog("[G3THR] %s | 300 frames: %u jobs, worker busy %u us/frame, game thread waited %u us/frame at drains (frame %u screen %u listfree %u texfree %u ringfull %u arena %u)",gtCfg?"ON":"OFF",gtJobs,(unsigned)(gtBusyTicks/268/300),(unsigned)(gtWaitTicks/268/300),gtDrains[1],gtDrains[0],gtDrains[2],gtDrains[3],gtDrains[4],gtDrains[5]);
 if(P3DS_G3TexFenceCfg()){const unsigned avgD=gtFenceDepthN?gtFenceDepthSum/gtFenceDepthN:0;
  PSPNativeMemLog("[G3THR] texfence: max in-flight %u, avg depth %u, pins %u, pin-waits %u, TexCreate wait %u skip %u, defer-free %u, wait-timeouts %u (level %d; depth>1 means front/cmd overlapped raw)",gtInFlightMax,avgD,gtPinHits,gtPinWaits,texCreateWait,texCreateSkip,gtTexDeferHits,gtWaitTimeouts,P3DS_G3TexFenceCfg());
  gtInFlightMax=gtFenceDepthSum=gtFenceDepthN=gtPinHits=gtPinWaits=0;texCreateWait=texCreateSkip=0;gtTexDeferHits=gtWaitTimeouts=0;}
 /* mismatches = POD/xform/tex-CRC failures (gtCheckBad). texCrc = subset that were texture CRC. flashRisk =
    in-place postvblank while thread ON (not a CRC mismatch; was wrongly lumped into "(N tex)" before). */
 if(P3DS_G3ThrCheckCfg()){PSPNativeMemLog("[G3THR] check: %u jobs, %u mismatches (%u texCrc, %u flashRisk) (want 0 mismatches)",gtCheckN,gtCheckBad,gtTexBad,gtFlashRisk);gtCheckN=gtCheckBad=gtTexBad=gtFlashRisk=0;}
 if(postReplace|postInPlace){PSPNativeMemLog("[G3THR] tex_postvblank 300f: replace %u in-place %u (texsafe wants replace when thread ON)",postReplace,postInPlace);postReplace=postInPlace=0;}
 gtJobs=0;gtBusyTicks=gtWaitTicks=0;memset(gtDrains,0,sizeof gtDrains);}
extern "C" int P3DS_G3ThreadCfg(void){if(gtCfgWant<0){gtCfgWant=P3DS_CfgInt("g3_thread",0);if(gtCfgWant<0||gtCfgWant>2)gtCfgWant=0;PSPNativeMemLog("[G3THR] g3_thread=%d (raw 3D transform on a worker thread on core 2; OFF = on the game thread)",gtCfgWant);}return gtCfgWant;}
extern "C" void P3DS_SetG3Thread(int v){gtCfgWant=v<0?0:v>2?2:v;gtAbN=0;PSPNativeMemLog("[G3THR] g3_thread=%d from settings (applies at the next 3D frame)",gtCfgWant);}
static void Draw(const GuVertex*v,unsigned n,bool deferOk){
 /* 2026-10-26 [LUGIA] scrfix: with W-buffer depth = GPU Z (depth code 3) the raw triangles get hyperbolic depth A-B/w
    (wDepthZ), but the title's skinned polygons (the 1632 'scr' verts, G3SIM non-raw path) still carried the DS W depth
    (z linear in w, g3_handler.cpp). Two depth scales in one Z-buffer: skinned and rigid parts of Lugia sorted wrongly
    against each other (spikes over the wing, holes) - only with GPU Z, which Nathan's config.txt pins (g3_wmode=2);
    Azahar ran PICA W (code 2), where both scales match. Convert those vertices to the same hyperbolic depth. */
 if(!rawMode&&n&&gpuOn==1&&s_SIM_useWBuffer&&wbufOn()&&G3G_WDepthOn()==3&&n<=sizeof scrBuf/sizeof*scrBuf){
  const float wmax=1.0f/wInv,nn=wdN,ff=wdF,A=(ff+nn)/(ff-nn),B=2.0f*ff*nn/(ff-nn);
  for(unsigned i=0;i<n;i++){GuVertex o=v[i];const float d=((o.z-32768.0f)*(1.0f/32767.0f)+1.0f)*0.5f,w=d*wmax;
   float zn=w>=ff?1.0f:w>1e-6f?A-B/w:-1.0f;if(zn<-1)zn=-1;if(zn>1)zn=1;o.z=zn*32767.0f+32768.0f;scrBuf[i]=o;}
  DrawScreen(scrBuf,n,deferOk);return;}
 if(rawMode&&n&&gpuOn==1){frameHadGeom++;P3DS_Fps60Poll();u64 t=svcGetSystemTick();GJob J;gtSnapshot(J,v,n,true);if(gtCfg)gtSubmit(J);else if(!J.cullAll){bool ch=false;if(!CullBatch(J,ch)){gtResolveTex(J);gtFinishMid(J);J.chunked=ch;J.cmask=chunkOff;if(P3DS_G3ThrCheckCfg()){J.snapCrc=gtSnapCrc(J);J.xformCrc=gtXformCrc(J);}GpuRawJ(J);}}prRaw+=svcGetSystemTick()-t;return;}
 if(rawMode&&n){if(gtBusy())gtDrain(0);GJob J;gtSnapshot(J,v,n,true);unsigned m=RawToScreen(J,v,n);DrawScreen(rawOut,m,deferOk);return;}
 DrawScreen(v,n,deferOk);
}
static void DrawScreenJ(const GJob&J0,const GuVertex*v,unsigned n,bool deferOk){
 if(!n||J0.cullAll)return;
 const GJob*prev=gtCur;gtCur=&J0; /* hard-clip / queued screen draws: Tri must use job sd/parL/wide/gt */
 const GJob&J=J0;
 G3G_SetStereoShare(J.sd&&J.stereoShare);
 if(P3DS_G3ThrCheckCfg()){
  gtCheckN++;
  if(J.snapCrc&&J.snapCrc!=gtSnapCrc(J)){gtCheckBad++;static unsigned mism;if(mism++<16)PSPNativeMemLog("[G3THR] snap mismatch screen job n=%u",J.n);}
  if(J.xformCrc&&J.xformCrc!=gtXformCrc(J)){gtCheckBad++;static unsigned xm;if(xm++<16)PSPNativeMemLog("[G3THR] snap mismatch screen xform n=%u",J.n);}
 }
 G3G_CurFog=J.fog;Tex*tex=J.tex;const G3SIM_TexImageParam_t tp=J.tp;
 bool translucent=(v[0].color>>24)<250||((J.hasTex||tex)&&(tp.textureFormat==1||tp.textureFormat==6));
 unsigned dtest=J.depthTest;
 if(translucent&&deferOk){if(transN<1024&&transVN+n<=24576){memcpy(transVerts+transVN,v,n*sizeof(GuVertex));
  trans[transN++]={transVerts+transVN,n,tex,J.gt,J.tw,J.th,J.hasTex,tp,J.tdw,dtest,G3G_CurFog,J.sd,J.parL,J.wide};transVN+=n;gtCur=prev;return;}}
 for(unsigned i=0;i+2<n;i+=3)Tri(v[i],v[i+1],v[i+2],tex,tp,translucent,!translucent||J.tdw,dtest);
 gtCur=prev;
}
static void DrawScreen(const GuVertex*v,unsigned n,bool deferOk){   /* game thread */
 if(gtCfg&&gpuOn==1){ /* resolve: depth-1 idle wait, or texfence pin (see gtResolveTex) */
  GJob J;gtSnapshot(J,v,n,false);if(!n||J.cullAll)return;gtResolveTex(J);
  J.screen=true;J.deferOk=deferOk;if(P3DS_G3ThrCheckCfg()){J.snapCrc=gtSnapCrc(J);J.xformCrc=gtXformCrc(J);}gtPush(J,false);return;}
 if(gtBusy())gtDrain(0);
 GJob J;gtSnapshot(J,v,n,false);if(n&&!J.cullAll){gtResolveTex(J);}DrawScreenJ(J,v,n,deferOk);}
extern "C" {
void G3ListRecordBegin(){recActive=true;recBroken=false;g3RecCount=0;}
int G3ListRecordEnd(unsigned*count,const GuVertex**verts){recActive=false;*count=g3RecCount;*verts=g3ChunkPtr-g3RecCount;return !recBroken;}
void G3ListReplay(const GuVertex*v,unsigned n){if(n>g3ChunkLeft)G3SIM_ChunkAlloc();memcpy(g3ChunkPtr,v,n*sizeof(GuVertex));g3ChunkPtr+=n;g3ChunkLeft-=n;}
void G3SIM_FlushArray();
void G3ListDrawDirect(const GuVertex*v,unsigned n){G3SIM_FlushArray();replayV=v;Draw(v,n,true);replayV=nullptr;}
static void ListFreeNow(void*p){if(p){BBox*b=bbSlot(p);if(b->p==p){b->p=nullptr;free(b->cb);b->cb=nullptr;}
 XfSlot&xs=xfTab[xfHash(p)];if(xs.key==p)xs.key=nullptr;}free(p);}
void G3ListDeferFree(void*p){if(gtBusy()){if(gtPendN<512){gtPendFree[gtPendN++]=p;return;}gtDrain(2);}ListFreeNow(p);}
void G3ListPendingFreeDrain(){}
unsigned G3ListPendingFreeHigh(){return 0;}
u8 s_SIM_useWBuffer=0;
void SIM_u16ToRGB(u16 c,u8*r,u8*g,u8*b){unsigned x=c&31,y=(c>>5)&31,z=(c>>10)&31;*r=(x<<3)|(x>>2);*g=(y<<3)|(y>>2);*b=(z<<3)|(z>>2);}
static void APIENTRY NoDebug(GLenum,GLenum,GLuint,GLenum,GLsizei,const GLchar*){}
static void APIENTRY NoCap(GLenum){}
static void APIENTRY Cull(GLenum c){G3SIM_FlushArray();cullAll=c==GL_FRONT_AND_BACK;cullFace=c==GL_FRONT?1:c==GL_BACK?2:0;}
static void APIENTRY Dis(GLenum c){if(c==GL_CULL_FACE){G3SIM_FlushArray();cullAll=false;cullFace=0;}}
PFNGLDEBUGMESSAGEINSERTPROC glad_glDebugMessageInsert=NoDebug;
PFNGLENABLEPROC glad_glEnable=NoCap;PFNGLDISABLEPROC glad_glDisable=Dis;PFNGLCULLFACEPROC glad_glCullFace=Cull;PFNGLDEPTHFUNCPROC glad_glDepthFunc=NoCap;
static unsigned channel(float f){if(f<0)f=0;if(f>1)f=1;return unsigned(f*255.0f+0.5f);}
void G3SIM_AddVtx(G3SIM_Vertex_t*v){if(!g3ChunkLeft)G3SIM_ChunkAlloc();
 *g3ChunkPtr++={v->s*s_texImageParam.textureSSize,v->t*s_texImageParam.textureTSize,channel(v->r)|(channel(v->g)<<8)|(channel(v->b)<<16)|(channel(v->a)<<24),128.0f+v->x*128.0f,96.0f-v->y*96.0f,v->z*32767.0f+32768.0f};g3ChunkLeft--;}
void G3SIM_DrawArray(){Draw(vertices,(unsigned)(g3ChunkPtr-vertices),true);}
void G3SIM_DrawCleanUp(){ResetVertices();}
void G3SIM_FlushArray(){if(g3ChunkPtr==vertices){ResetVertices();return;}G3SIM_DrawArray();ResetVertices();}
static u64 frameTicks,sumTicks;static unsigned frames,lastUs,lastTris;
/* g3_front_ov: gather/decode into SIDE buffers only — never touch t->px/gen/raw while the worker may CRC them. */
static unsigned frontOvPrepN,frontOvPrepUs,frontOvFrames;
struct FrontPrep{Tex*t;u32*px;u8*raw;unsigned rawN,np;};
static FrontPrep frontPrep[96];static unsigned frontPrepN;
static void frontPrepClear(void){
 for(unsigned i=0;i<frontPrepN;i++){free(frontPrep[i].px);free(frontPrep[i].raw);frontPrep[i]={};}
 frontPrepN=0;}
static FrontPrep*frontPrepFind(Tex*t){for(unsigned i=0;i<frontPrepN;i++)if(frontPrep[i].t==t)return&frontPrep[i];return nullptr;}
static int postVblPrepOne(Tex*t){
 if(!t||!t->px||t->useF!=g3FrameNo+1||t->gen==PSPNativeVramGen)return 0;
 if(TexPageGen(t->fmt,t->w,t->h,t->off,t->pal)<=t->gen)return 0;
 unsigned rn=GatherRaw(t->fmt,t->w,t->h,t->off,t->pal);
 /* Unchanged: do NOT bump t->gen or rewrite t->px/useF here (worker may still CRC). Apply after drain. */
 if(!(rn&&!(t->raw&&t->rawN==rn&&!memcmp(t->raw,rawScratch,rn))))return 0;
 if(frontPrepN>=96||frontPrepFind(t))return 0;
 const unsigned np=t->w*t->h;u32*side=(u32*)malloc(np*4);u8*rside=rn?(u8*)malloc(rn):nullptr;
 if(!side||(rn&&!rside)){free(side);free(rside);return 0;}
 if(rn)memcpy(rside,rawScratch,rn);
 if(!DecodeTexTo(t,side)){free(side);free(rside);return 0;}
 frontPrep[frontPrepN++]={t,side,rside,rn,np}; /* mark via table only — no Tex field stores */
 return 1;}
static void postVblApplyOne(Tex*t,bool texSafe){
 FrontPrep*fp=frontPrepFind(t);
 const bool prepped=fp&&fp->px;
 if(!prepped){
  if(!t->px||t->useF!=g3FrameNo+1||t->gen==PSPNativeVramGen)return;
  if(TexPageGen(t->fmt,t->w,t->h,t->off,t->pal)<=t->gen)return;
  unsigned rn=GatherRaw(t->fmt,t->w,t->h,t->off,t->pal);
  if(!(rn&&!(t->raw&&t->rawN==rn&&!memcmp(t->raw,rawScratch,rn)))){if(rn){t->gen=PSPNativeVramGen;t->cnt=VramCnt();}return;}
  if(t->raw&&t->rawN==rn)memcpy(t->raw,rawScratch,rn);else{free(t->raw);t->raw=(u8*)malloc(rn);t->rawN=t->raw?rn:0;if(t->raw)memcpy(t->raw,rawScratch,rn);}
  if(!DecodeTex(t))return;}
 else{
  /* Publish side decode into t->px/raw only after the worker is idle (drain done). */
  if(!t->px)t->px=(u32*)malloc(fp->np*4);
  if(t->px)memcpy(t->px,fp->px,fp->np*4);
  if(fp->rawN){if(t->raw&&t->rawN==fp->rawN)memcpy(t->raw,fp->raw,fp->rawN);else{free(t->raw);t->raw=(u8*)malloc(fp->rawN);t->rawN=t->raw?fp->rawN:0;if(t->raw)memcpy(t->raw,fp->raw,fp->rawN);}}
  free(fp->px);free(fp->raw);fp->px=nullptr;fp->raw=nullptr;fp->t=nullptr;}
 if(t->gt&&texSafe){
  if(P3DS_G3ThrCheckCfg()){static unsigned fr;if(fr++<16)PSPNativeMemLog("[G3THR] tex flash risk avoided: postvblank replace off=%x gt=%p (queued this frame)",t->off,t->gt);}
  void*old=t->gt;void*neu=G3SIM_GpuTex(t);
  if(neu){t->gt=neu;G3G_TexFree(old);postReplace++;postFixes++;}
  else{G3G_TexUpdate(t->gt,t->px,t->w,t->h);t->gpuGen++;postInPlace++;postFixes++;}}
 else if(t->gt){
  if(gtCfg&&P3DS_G3ThrCheckCfg()){static unsigned fi;if(fi++<16)PSPNativeMemLog("[G3THR] tex flash risk: postvblank in-place off=%x gt=%p (enable g3_thr_texsafe)",t->off,t->gt);gtFlashRisk++;}
  G3G_TexUpdate(t->gt,t->px,t->w,t->h);t->gpuGen++;postInPlace++;postFixes++;}
 else{t->gt=G3SIM_GpuTex(t);postFixes++;}
 t->gen=PSPNativeVramGen;t->cnt=VramCnt();}
void G3SIM_FlushDeferred(){
 CRUMB("g3-flush");const bool doProf=P3DS_G3ProfCfg()!=0;
 const int frontOv=gtCfg&&P3DS_G3FrontOvCfg();
 /* Freeze used textures before FlushArray so GetTexture cannot redecode/bump gen while jobs still CRC. */
 if(frontOv){for(unsigned i=0;i<usedN;i++)if(usedTex[i])usedTex[i]->freeze=1;}
 G3SIM_FlushArray(); /* may Draw → counted in prRaw, not flush */
 u64 tFlush0=doProf?svcGetSystemTick():0;
 const int shareOn=gpuOn==1&&stereoSd&&P3DS_G3StereoShareCfg();
 const int stereoGpu=shareOn&&P3DS_G3StereoGpuCfg();
 G3G_SetStereoGpu(stereoGpu);
 /* Worker expand after raw unless GPU stereo skips expand (shader right eye). */
 if(shareOn&&!stereoGpu&&gtCfg&&G3G_StereoPending()){
  __atomic_store_n(&gtExpandReq,1,__ATOMIC_RELEASE);gtWakeForce();}
 /* g3_front_ov / g3_front_bg: side-buffer decode while worker finishes raw (+ optional expand).
    front_bg = prep only (no wake-every / freeze-all); front_ov also wakes every job. */
 {const int frontPrep=frontOv||(gtCfg&&P3DS_G3FrontBgCfg());
  if(frontPrep&&(gtBusy()||__atomic_load_n(&gtExpandReq,__ATOMIC_ACQUIRE))&&gpuOn==1&&!G3G_Async&&G3G_RbSyncOn()&&postVblCfg()){
   if(frontOv)gtWakeForce();
   const u64 tp0=svcGetSystemTick();unsigned did=0;
   for(unsigned i=0;i<usedN;i++){
    if(!gtBusy()&&!__atomic_load_n(&gtExpandReq,__ATOMIC_ACQUIRE))break;
    if(postVblPrepOne(usedTex[i]))did++;}
   frontOvPrepUs+=(unsigned)((svcGetSystemTick()-tp0)/268);frontOvPrepN+=did;if(did)frontOvFrames++;
  }}
 gtDrain(1);gtFrameStats();
 /* Clear freeze after drain (pins may still be held until End under texsafe; GpuRawJ CRC is done). */
 for(unsigned i=0;i<texN;i++)texCache[i].freeze=0;
 for(unsigned i=0;i<64;i++)folTex[i].freeze=0;
 if(frontOv){static unsigned n;if(++n>=300){PSPNativeMemLog("[FRONT] 300f: prep during drain on %u frames, %u tex side-decoded, %u us (freeze+side; overlaps raw/expand)",frontOvFrames,frontOvPrepN,frontOvPrepUs);frontOvPrepN=frontOvPrepUs=frontOvFrames=n=0;}}
 /* Stereo share: expand on game thread if worker did not (GPU stereo leaves pending for End). */
 {u64 tExp0=svcGetSystemTick();
  if(shareOn&&!stereoGpu)G3G_StereoExpand();
  const unsigned expUs=G3G_LastExpandUs()?G3G_LastExpandUs():(unsigned)((svcGetSystemTick()-tExp0)/268);
  if(doProf)prExpand+=expUs*268ull;
  static unsigned accL,accD,accE,accV,accN,accExpUs;
  unsigned a=0,b=0,c=0,d=0;G3G_StereoShareStats(&a,&b,&c,&d);
  accL+=a;accD+=b;accE+=c;accV+=d;accExpUs+=expUs;
  if(++accN>=60){
   /* Path counts (gpuRight vs cpuExpand) are logged after End — see sterPathAcc below. */
   if(accL|accD|accE|stereoGpu)
    PSPNativeMemLog("[G3STER] 60f: leftOnly %u tris, dual %u, expands %u (%u verts)%s | expand %u us/f — want leftOnly>>dual",
     accL,accD,accE,accV,stereoGpu?" [want gpu]":"",accExpUs/accN);
   accL=accD=accE=accV=accN=accExpUs=0;}}
 G3SIM_TexImageParam_t save=s_texImageParam;
 const int fogSave=G3G_CurFog;
 for(unsigned i=0;i<transN;i++){
  Batch&b=trans[i];G3G_CurFog=b.fog;
  /* Rebuild a stack job so Tri/parOf/TriEye use the snapshotted stereo/tex, not live globals. */
  GJob tmp={};tmp.v=b.v;tmp.n=b.n;tmp.tex=b.tex;tmp.gt=b.gt;tmp.tw=b.tw;tmp.th=b.th;tmp.hasTex=b.hasTex;tmp.tp=b.tp;
  tmp.tdw=b.dw;tmp.depthTest=b.dtest;tmp.fog=b.fog;tmp.sd=b.sd;tmp.parL=b.parL;tmp.wide=b.wide;
  const GJob*prev=gtCur;gtCur=&tmp;
  for(unsigned k=0;k+2<b.n;k+=3)Tri(b.v[k],b.v[k+1],b.v[k+2],b.tex,b.tp,true,b.dw,b.dtest);
  gtCur=prev;}
 transN=transVN=0;s_texImageParam=save;G3G_CurFog=fogSave;
 if(gpuOn==1&&!G3G_Async&&G3G_RbSyncOn()&&postVblCfg()){
  /* Run 879: with g3_thread, batches already hold gt pointers into vb/bt. In-place TexUpdate mutates those objects
     before G3G_End samples them → brief texture flashes (gone with g3_thread=0). texsafe: upload a NEW gt and
     pendFree the old one so this End still sees the submit-time image. OFF = original in-place update.
     g3_front_ov may have side-decoded (frontPrep table); apply copies into t->px then uploads GPU. */
  const bool texSafe=gtCfg&&P3DS_G3TexSafeCfg();
  for(unsigned i=0;i<usedN;i++)postVblApplyOne(usedTex[i],texSafe);
  frontPrepClear();
  {static unsigned f;if(++f%900==0){PSPNativeMemLog("[G3] tex_postvblank: %u textures refreshed after VBlank (30 s)",postFixes);postFixes=0;}}}
 else frontPrepClear();
 /* run 63 tex_trace=1 (debug, default 0): per 3D frame, every small texture used (<=64x64) with a hash of its VRAM
    bytes -> shows whether a sprite's VRAM really changes (game side) or the cache misses it (port side). */
 {static int tt=-1;if(tt<0)tt=0 /* tex_trace: debug option removed */;if(tt>0){struct E{unsigned off,pal,wh;u32 h;};static E prv[96],cur[96];static unsigned pn;unsigned cn=0;
  for(unsigned i=0;i<usedN&&cn<96;i++){Tex*t=usedTex[i];if(t->w>64||t->h>64)continue;u32 hsh=2166136261u;for(unsigned j=0;j<t->rawN;j++)hsh=(hsh^t->raw[j])*16777619u;cur[cn++]={t->off,t->pal,t->w<<8|t->h|t->fmt<<16,hsh};}
  char b[200];int k=0;auto fl=[&](){if(k){PSPNativeMemLog("[TEXTR] f%u g%u%s",g3FrameNo,PSPNativeVramGen,b);k=0;}};
  for(unsigned i=0;i<cn;i++){bool same=false,had=false;for(unsigned j=0;j<pn;j++)if(prv[j].off==cur[i].off&&prv[j].pal==cur[i].pal&&prv[j].wh==cur[i].wh){had=true;same=prv[j].h==cur[i].h;break;}
   if(!same){k+=snprintf(b+k,sizeof b-k," %c%x/%x/%x:%08lx",had?'*':'+',cur[i].off,cur[i].pal,cur[i].wh,(unsigned long)cur[i].h);if(k>140)fl();}}
  for(unsigned j=0;j<pn;j++){bool f=false;for(unsigned i=0;i<cn;i++)if(prv[j].off==cur[i].off&&prv[j].pal==cur[i].pal&&prv[j].wh==cur[i].wh){f=true;break;}
   if(!f){k+=snprintf(b+k,sizeof b-k," -%x/%x/%x",prv[j].off,prv[j].pal,prv[j].wh);if(k>140)fl();}}
  fl();memcpy(prv,cur,cn*sizeof(E));pn=cn;}}
 usedN=0;
 if(doProf)prFlush+=svcGetSystemTick()-tFlush0;
 if(gpuOn==1){u64 t=doProf?svcGetSystemTick():0;G3G_FogNoW=!wbufOn();G3G_MidInfo(mdTot,mdMat,mdMov);G3G_End(GPU3D::NativeFrame,stereoSd?frameR:nullptr);if(doProf){prEnd+=svcGetSystemTick()-t;prN++;}
  /* After End: path counts + fallback reasons (eye1/nopend/noshad/nogeom/eyes). */
  {static unsigned accGR,accCE,accPN,accWhy[5];
   unsigned gr=0,ce=0,why[5]={0};G3G_StereoPathStats(&gr,&ce);G3G_StereoPathWhy(why,5);
   accGR+=gr;accCE+=ce;for(int i=0;i<5;i++)accWhy[i]+=why[i];
   if(++accPN>=60){if(accGR|accCE|P3DS_G3StereoGpuCfg())
     PSPNativeMemLog("[G3STER] 60f path: gpuRight %u frames, cpuExpand %u (want=%d) reasons eye1=%u nopend=%u noshad=%u nogeom=%u eyes=%u — want gpuRight>>cpuExpand",
      accGR,accCE,P3DS_G3StereoGpuCfg(),accWhy[0],accWhy[1],accWhy[2],accWhy[3],accWhy[4]);
    accGR=accCE=accPN=0;memset(accWhy,0,sizeof accWhy);}}}
 /* Safety: front_ov on + geometry submitted but 0 tris after we have already drawn successfully this
    session → blank top risk (run 887). Ignore title/boot frames that legitimately submit then cull to 0
    (run 889 false trip turned front_ov off and exposed texCrc). */
 {static int frontSawDraw;
  if(drawnTris)frontSawDraw=1;
  if(frontOv&&frontSawDraw&&frameHadGeom&&!drawnTris){
   static int said;if(!said){said=1;PSPNativeMemLog("[FRONT] safety: 0 tris after geometry submit — g3_front_ov OFF for this session (was blanking top)");}
   g3FrontOvCfg=0;}}
 frameHadGeom=0;
 /* Release fence pins held for texsafe (after End finished sampling those gts), then GPU-free any
    cache evictions that were deferred because those pins were still held (run 883 hang fix). */
 if(gtCfg&&P3DS_G3TexFenceCfg()&&P3DS_G3TexSafeCfg()){gtPinReleaseDone();gtFlushTexDeferFree();}
 else if(gtTexDeferN)gtFlushTexDeferFree();
 /* debug g3_shot=N: dump the finished 3D layer (256x192 RGBA, 6-bit) of 3D frames N, N+60, ... (8 files) to
    sdmc:/3ds/platinum3ds/g3shot_<frame>.bin, to compare renderers on the exact same game frame. Off (0) by default. */
 {static int shotF=-1;if(shotF<0)shotF=0 /* g3_shot: debug option removed */;if(shotF>0&&g3FrameNo>=(unsigned)shotF&&(g3FrameNo-shotF)%60==0&&g3FrameNo<(unsigned)shotF+480){G3G_DirectResolveWhy(7);char fn[64];snprintf(fn,sizeof fn,"sdmc:/3ds/platinum3ds/g3shot_%u.bin",g3FrameNo);FILE*f=fopen(fn,"wb");if(f){fwrite(GPU3D::NativeFrame,4,256*192,f);fclose(f);PSPNativeMemLog("[G3SHOT] %s (gpu %d)",fn,gpuOn);}}}
 {u64 d=svcGetSystemTick()-frameTicks;lastUs=(unsigned)(d/268);lastTris=drawnTris;sumTicks+=d;}if(++frames==120){sumTicks=0;frames=0;
  if(drawnTris)PSPNativeMemLog("[G3DIAG] gpu3d %d %uus | decodes %u | tex %u trans %u | px tested %u depthrej %u texalpha0 %u alpha0 %u written %u | z %.0f..%.0f x %.1f..%.1f a %u..%u | clear %08lx depth %04x dtest %u | fmt0-7 %u %u %u %u %u %u %u %u | vcol or %08lx and %08lx",gpuOn,G3G_LastUs(),stDecodes,
   stTexTris,stTrans,stPix,stDepth,stTexA,stA0,stWritten,stZmin,stZmax,stXmin,stXmax,stAmin,stAmax,(unsigned long)*(u32*)(PSPNative_GfxRegisters+0x350),*(u16*)(PSPNative_GfxRegisters+0x354),s_curPolygonAttr.depthTest,stFmt[0],stFmt[1],stFmt[2],stFmt[3],stFmt[4],stFmt[5],stFmt[6],stFmt[7],(unsigned long)stColOr,(unsigned long)stColAnd);}
 memset(stFmt,0,sizeof stFmt);stColOr=0;stColAnd=~0u;
 stDecodes=stPix=stDepth=stTexA=stA0=stWritten=stTexTris=stTrans=0;stZmin=stXmin=1e9f;stZmax=stXmax=-1e9f;stAmin=255;stAmax=0;drawnTris=0;wDepthFrame();{static unsigned f;++f;vdump=(f%150==0&&f<=600);if(f%60==0)PSPNativeMemLog("[GPU3D] f=%u trivially rejected %u (planes %02x) of drawn %u",f,rejOut,rejMask,drawnTris);rejOut=rejMask=0;if(f%300==0&&bcV>0){PSPNativeMemLog("[G3CULL] last 300 frames: %u batches tested (%u cached boxes), %u skipped whole, %u 16-tri chunks skipped = %u tris/frame never transformed",bcTested,bcHits,bcSkipB,bcSkipC,bcSkipT/300);bcTested=bcHits=bcSkipB=bcSkipC=bcSkipT=0;}
  /* g3_prof=1: whole PERF g3soft window (Begin→FlushDeferred); one averaged line every 60 3D frames. */
  if(P3DS_G3ProfCfg()){
   const u64 idleNow=P3DS_VblIdleTicks;const u64 idleTicks=idleNow>idleAtBegin?idleNow-idleAtBegin:0;
   prIdle+=idleTicks;prWall+=svcGetSystemTick()-frameTicks;
   prFrames++;prTrisSum+=lastTris;prXfHitSum+=xfHits;prXfMissSum+=xfMiss;
   xfHits=xfMiss=xfStore=xfSkipHard=0;
   if(prFrames>=60){const unsigned n=prFrames;
    auto U=[&](u64 t){return (unsigned)(t/268/n);};
    const unsigned frameUs=U(prWall),idleUs=U(prIdle),rawUs=U(prRaw),flushUs=U(prFlush),endUs=U(prEnd),cmdUs=U(prCmd),expUs=U(prExpand);
    static u64 waitMark;const u64 waitDelta=gtWaitTicks>waitMark?gtWaitTicks-waitMark:0;waitMark=gtWaitTicks;
    const unsigned waitUs=(unsigned)(waitDelta/268/n); /* game-thread stalls on worker (TexCreate idle / drains) */
    /* front residual: wall minus idle/raw/flush/end/wait so TexCreate serialization is not mistaken for GeBuffer work */
    const unsigned accounted=idleUs+rawUs+flushUs+endUs+waitUs;
    const unsigned frontUs=frameUs>accounted?frameUs-accounted:0;
    PSPNativeMemLog("[G3PROF] 60f avg: frame %u us | idle %u | front %u | wait %u | cmd %u | raw %u (cull %u pair %u xform %u clip %u queue %u tex %u) | flush %u | expand %u | end %u | tris %u | xf hit %u miss %u",
     frameUs,idleUs,frontUs,waitUs,cmdUs,rawUs,U(prCull),U(prPair),U(prXform),U(prClip),U(prQueue),U(prTex),flushUs,expUs,endUs,
     prTrisSum/n,prXfHitSum/n,prXfMissSum/n);
    if(stereoSd)
     PSPNativeMemLog("[G3STER] 60f stages us: expand %u | end %u | front %u | raw %u | flush %u | wait %u | idle %u | wall %u | wantGpu %d | tris %u",
      expUs,endUs,frontUs,rawUs,flushUs,waitUs,idleUs,frameUs,P3DS_G3StereoGpuCfg(),prTrisSum/n);
    prCull=prPair=prXform=prClip=prQueue=prTex=prRaw=prEnd=prFlush=prIdle=prCmd=prWall=prExpand=0;prN=0;prFrames=prTrisSum=prXfHitSum=prXfMissSum=0;}
  }
  else if(f%300==0){PSPNativeMemLog("[GPU3D] cpu-clipped tris %u, culled %u (this frame) | tex page-skips %u mismatches %u (300 f)",cpuClipped,culledTris,texPageSkips,texPageMismatch);texPageSkips=0;if(prN)PSPNativeMemLog("[G3PROF] avg/frame: GetTexture %u us | GpuRaw (transform+clip+queue, incl. its GetTexture) %u us | G3G_End (GPU wait+readback) %u us | rest of g3soft = pspoke frontend/handler",(unsigned)(prTex/prN/268),(unsigned)(prRaw/prN/268),(unsigned)(prEnd/prN/268));prTex=prRaw=prEnd=prN=0;if(P3DS_G3XfCacheCfg()&&(xfHits|xfMiss)){PSPNativeMemLog("[G3XF] last 300 frames: hit %u miss %u store %u hardskip %u",xfHits,xfMiss,xfStore,xfSkipHard);xfHits=xfMiss=xfStore=xfSkipHard=0;}}
  cpuClipped=culledTris=0;}}
unsigned G3SIM_DeferredHighWater(){return 0;}
void PSPNativeG3Release(){ResetVertices();}
void PSPNativeG3TrimCache(){}
static unsigned rawGen;static s32 rawMFx[16];static int rawMFxOk;
void PSPNativeRawMatrix(const s32*m,unsigned gen){
 /* Identical skip (always on): same 16 fx32 words → no flush, no float convert. Same picture. */
 if(rawMode&&rawMFxOk&&!memcmp(m,rawMFx,64)){rawGen=gen;return;}
 G3SIM_FlushArray();rawMode=true;rawGen=gen;memcpy(rawMFx,m,64);rawMFxOk=1;for(int i=0;i<16;i++)rawM[i]=m[i]/4096.0f;}
void PSPNativeRawMode(int v){if(rawMode!=(v!=0)){G3SIM_FlushArray();rawMode=v!=0;if(!rawMode)rawMFxOk=0;}}
/* Frame start: clear the 3D layer to the DS CLEAR_COLOR / CLEAR_DEPTH registers. */
extern "C" int P3DS_StereoShiftNow(void);
void P3DS_G3BeginFrame(void){CRUMB("g3-begin");g3FrameNo++;usedN=0;frameTicks=svcGetSystemTick();idleAtBegin=P3DS_VblIdleTicks;(void)P3DS_G3ProfCfg();stereoSd=P3DS_StereoShiftNow();{extern int P3DS_InBattle;extern int P3DS_BattleDepthLock(void);static unsigned bf;static int wasB;
 parLock=(stereoSd&&P3DS_BattleDepthLock())?kParLock:0;{extern int P3DS_BattleSideExtra(int);parSideE=parLock?P3DS_BattleSideExtra(stereoSd):0;}parTrack=stereoSd&&P3DS_InBattle&&!parLock;
 if(P3DS_InBattle&&stereoSd){if(!wasB){wasB=1;bf=0;PSPNativeMemLog("[BATTLE] stereo 3D: shift %d px, battle depth lock %d (stereo_2d=3: all 3D at one depth; else per-vertex z), lock offsets L 0 / R +%d px (same integer as the BG), foe deeper %s (foe R +%d px, yours R +%d px)",stereoSd,parLock,(stereoSd+1)/2,parSideE?"ON":"off",(stereoSd+1)/2+parSideE,(stereoSd+1)/2-parSideE);if(parSideE){extern void P3DS_Use(const char*);P3DS_Use("battle_foe_deeper");}}
  if(++bf%150==1&&bf>1){PSPNativeMemLog("[BATTLE] stereo 3D f%u: %u vertices, z range %.3f..%.3f, clamped tris so far %u, lock %d, foe deeper draws foe %u / yours %u",bf,parN,parZMin,parZMax,stereoClamped,parLock,parFoeN,parMineN);parN=0;parFoeN=parMineN=0;parZMin=1;parZMax=0;}}
 else wasB=0;}
 u32 cc=*(u32*)(PSPNative_GfxRegisters+0x350);u16 cd=*(u16*)(PSPNative_GfxRegisters+0x354);
 unsigned r=cc&31,g=(cc>>5)&31,b=(cc>>10)&31,a=(cc>>16)&31;u32 v=(r<<1|r>>4)|((g<<1|g>>4)<<8)|((b<<1|b>>4)<<16)|(a<<24);
 if(gpuOn<0){if(P3DS_CfgInt("g3_raw_split",0))PSPNativeMemLog("[G3THR] legacy g3_raw_split ignored: using one 3D worker");if(P3DS_CfgInt("g3_xf_map",0))PSPNativeMemLog("[G3XF] legacy g3_xf_map ignored: transform cache is list replay only");int want=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");if(f){char l[64];int x;while(fgets(l,sizeof l,f)){if(sscanf(l,"gpu3d=%d",&x)==1)want=x;if(sscanf(l,"gpu3d_texflip=%d",&x)==1)G3G_TexFlip=x;if(sscanf(l,"gpu3d_flipy=%d",&x)==1)G3G_FlipY=x;if(sscanf(l,"gpu3d_zinv=%d",&x)==1)G3G_ZInv=x;if(sscanf(l,"gpu3d_async=%d",&x)==1)G3G_Async=x!=0;}fclose(f);}
  gpuOn=want&&G3G_Init()?1:0;if(want&&!gpuOn)PSPNativeMemLog("[GPU3D] GPU 3D init FAILED -> automatic CPU 3D fallback");PSPNativeMemLog("[GPU3D] gpu3d=%d (config wants %d) texflip=%d zinv=%d async=%d",gpuOn,want,G3G_TexFlip,G3G_ZInv,G3G_Async);}
 if(gpuReq>=0){gpuOn=gpuReq&&G3G_Init()?1:0;gpuReq=-1;PSPNativeMemLog("[GPU3D] L+R+Y: gpu3d=%d",gpuOn);}
 /* CPU renderer (GPU 3D init failed or turned off with L+R+Y): DS W-buffer frames (title Lugia) use the perspective-correct raster TriWPersp */
 wPerspFrame=gpuOn!=1&&s_SIM_useWBuffer&&wbufOn()&&G3G_WDepthOn()!=3;
 if(gpuOn==1){gtDrain(1);const int cw=P3DS_G3ThreadCfg();const int ab=cw==2?(int)((gtAbN++/1200)&1):cw;const int want=ab&&gtStart();if(want!=gtCfg){gtCfg=want;if(gtCfg)xfClear();PSPNativeMemLog("[G3THR] now %s%s",gtCfg?"ON":"OFF",cw==2?" (A/B test block)":"");}u16 dz=(u16)(((cd&0x7FFF)*2)|1);G3G_SetStereoShare(stereoSd&&P3DS_G3StereoShareCfg());frameHadGeom=0;G3G_Begin(v,dz?dz:0xFFFF,stereoSd);mdFrameBegin(G3G_MidRec!=0);transN=transVN=0;return;}
 if(gtCfg){gtDrain(1);gtCfg=0;}
 G3G_DirectInvalidate();/* gpu_direct3d: this frame is CPU-rendered, NativeFrame is the truth again */
 for(unsigned i=0;i<256*192;i++)GPU3D::NativeFrame[i]=v;
 u16 dz=(u16)(((cd&0x7FFF)*2)|1);for(unsigned i=0;i<256*192;i++)depth[i]=dz?dz:0xFFFF;
 if(stereoSd){memcpy(frameR,GPU3D::NativeFrame,sizeof frameR);memcpy(depthR,depth,sizeof depthR);}
 transN=transVN=0;
}

void P3DS_G3ToggleGpu(void){gpuReq=gpuOn>=1?0:1;}
unsigned P3DS_G3TakeTris(void){unsigned d=drawnTris;drawnTris=0;return d;}
unsigned P3DS_G3LastUs(void){return lastUs;}
unsigned P3DS_G3LastTris(void){return lastTris;}
}
extern "C" int P3DS_CfgInt(const char*,int);
/* run 58: follower=0 (default until emu-verified) / 1, live from the L+R+Y menu */
static int folCfgV=-1;
extern "C" int P3DS_FollowerCfg(void){if(folCfgV<0){folCfgV=P3DS_CfgInt("follower",0)!=0;PSPNativeMemLog("[FOL] follower=%d",folCfgV);}return folCfgV;}
extern "C" void P3DS_SetFollowerCfg(int v){folCfgV=v!=0;}
extern "C" { unsigned char P3DS_FolMem[256] __attribute__((aligned(8))); }

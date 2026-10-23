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
static bool cullAll=false;static u64 prTex,prRaw,prEnd,prN;static int cullFace=0;/*1=GL_FRONT 2=GL_BACK*/
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
struct Tex{unsigned off,pal,fmt,w,h,c0,gen;u32*px;unsigned lru;u8*raw;unsigned rawN;void*gt;u64 cnt;unsigned useF;};
extern "C" {int G3G_Init(void);void*G3G_TexCreate(const u32*,unsigned,unsigned);void G3G_TexFree(void*);void G3G_Begin(u32,unsigned,int);
 void G3G_Tri(int,const float*,const float*,const float*,const float*,const float*,const u32*,void*,unsigned,unsigned,int,int,int,int,int,int,int);
 void G3G_End(u32*,u32*);void G3G_TexUpdate(void*,const u32*,unsigned,unsigned);int G3G_RbSyncOn(void);void G3G_TriClip(int,const float*,const float*,const float*,const u32*,void*,int,int,int,int,int,int,int,const float*);unsigned G3G_LastUs(void);extern int G3G_TexFlip,G3G_FlipY,G3G_ZInv,G3G_Async,G3G_Wide,G3G_CurFog,G3G_FogNoW;}
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
   draws): if any of its VRAM pages were written since it was decoded and the bytes really differ, it is re-decoded
   and the GPU texture is updated IN PLACE (same object the queued draws reference). GPU sync path only. */
static unsigned g3FrameNo;static Tex*usedTex[96];static unsigned usedN,postFixes;
static int postVblCfg(){static int v=-1;if(v<0){v=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"tex_postvblank=%d",&x)==1)v=x!=0;fclose(f);}PSPNativeMemLog("[G3] tex_postvblank=%d",v);}return v;}
extern "C" int P3DS_CfgInt(const char*,int);
static Tex folTex[5*8]; /* run 73: slot 0 follower, slots 1..4 wild_vis (8 frames each) */
static Tex*GetTexture(){u64 t=svcGetSystemTick();Tex*r=GetTextureImpl();if(r&&(r<folTex||r>=folTex+40)&&r->useF!=g3FrameNo+1){r->useF=g3FrameNo+1;if(usedN<96)usedTex[usedN++]=r;}u64 d=svcGetSystemTick()-t;prTex+=d;P3DS_SpkGetTexTicks+=d;return r;}
static bool DecodeTex(Tex*e){const unsigned fmt=e->fmt,w=e->w,h=e->h;
   stDecodes++;
   unsigned n=w*h;if(!e->px)e->px=(u32*)malloc(n*4);if(!e->px)return false;u32*o=e->px;unsigned palN=0;
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
/* run 58: following Pokemon (render-only, never a map object). The field hook (overlay005 fieldmap.c) draws one
   quad whose TEXIMAGE_PARAM address is the magic 0x7FFF8 (a 4bpp texture there would run past the end of texture
   VRAM, so no real draw can use it) and whose PLTT_BASE low bits pick one of 8 frame slots. GetTextureImpl hands
   back a port-owned texture decoded from sdmc:/3ds/platinum3ds/followers.bin (HGSS sprites extracted from the
   user's own SoulSilver ROM). Missing file -> P3DS_FollowerOpen()==0 -> the hook draws nothing. */
static int folState=-1;static u32 folNSpr,folNSpec;static u8*folSpec,*folDir;static int folCur=-1,folCurShiny=-1;
#define FOL_PATH "sdmc:/3ds/platinum3ds/followers.bin"
extern "C" int P3DS_FollowerOpen(void){
 if(folState>=0)return folState;folState=0;FILE*f=fopen(FOL_PATH,"rb");if(!f){PSPNativeMemLog("[FOL] %s missing -> follower off",FOL_PATH);return 0;}
 u32 h[4];if(fread(h,4,4,f)==4&&h[0]==0x4C463350u&&h[1]==1&&h[2]&&h[2]<2048&&h[3]&&h[3]<1024){folNSpr=h[2];folNSpec=h[3];
  folSpec=(u8*)malloc(folNSpec*4);folDir=(u8*)malloc(folNSpr*8);
  if(folSpec&&folDir&&fread(folSpec,4,folNSpec,f)==folNSpec&&fread(folDir,8,folNSpr,f)==folNSpr)folState=1;}
 fclose(f);PSPNativeMemLog("[FOL] followers.bin %s: %u sprites %u species",folState?"ok":"BAD",(unsigned)folNSpr,(unsigned)folNSpec);return folState;}
extern "C" int P3DS_FollowerSprite(int species,int form,int female){
 if(folState!=1||species<=0||(u32)species>=folNSpec)return -1;const u8*e=folSpec+species*4;int idx=e[0]|e[1]<<8;
 if(e[3]){if(female)idx++;}else if(form>0&&form<=e[2])idx+=form;
 if(idx<0)idx=0;if((u32)idx>=folNSpr)idx=folNSpr-1;return idx;}
/* loads sprite spr (8 frames) into the frame slots; returns the frame width (32/64) or 0 */
static int folCurS[5]={-1,-1,-1,-1,-1},folCurShS[5];
static int folLoadSlot(int sl,int spr,int shiny){
 if(folState!=1||spr<0||(u32)spr>=folNSpr)return 0;const u8*d=folDir+spr*8;unsigned w=d[4],h=d[5],nf=d[6];u32 off=d[0]|d[1]<<8|d[2]<<16|(u32)d[3]<<24;
 if(spr==folCurS[sl]&&shiny==folCurShS[sl])return w;
 if(nf!=8||(w!=32&&w!=64)||h!=w)return 0;unsigned fs=w*h/2,n=8*fs+64;u8*b=(u8*)malloc(n);if(!b)return 0;
 FILE*f=fopen(FOL_PATH,"rb");bool ok=f&&!fseek(f,off,SEEK_SET)&&fread(b,1,n,f)==n;if(f)fclose(f);
 if(!ok){free(b);PSPNativeMemLog("[FOL] read fail spr %d",spr);return 0;}
 const u8*pp=b+8*fs+(shiny?32:0);u16 pal[16];for(int i=0;i<16;i++)pal[i]=pp[i*2]|pp[i*2+1]<<8;
 for(int k=0;k<8;k++){Tex&t=folTex[sl*8+k];if(!t.px||t.w!=w){free(t.px);t.px=(u32*)malloc(w*h*4);}if(!t.px){free(b);return 0;}
  const u8*s=b+k*fs;for(unsigned i=0;i<w*h;i++){unsigned ix=(s[i>>1]>>((i&1)*4))&15;t.px[i]=ix?c555(pal[ix],255):0;}
  G3G_TexFree(t.gt);t.gt=nullptr;t.off=0x7FFF8-sl*8;t.pal=0x1FF00+k*16;t.fmt=3;t.w=w;t.h=h;t.c0=1;t.gen=0;t.raw=nullptr;t.rawN=0;}
 free(b);folCurS[sl]=spr;folCurShS[sl]=shiny;if(!sl){folCur=spr;folCurShiny=shiny;}PSPNativeMemLog(sl?"[WILD] slot %d load sprite %d shiny %d %ux%u":"[FOL] slot %d sprite %d shiny %d %ux%u",sl,spr,shiny,w,h);return w;}
extern "C" int P3DS_FollowerLoad(int spr,int shiny){return folLoadSlot(0,spr,shiny);}
extern "C" int P3DS_WildLoad(int sl,int spr,int shiny){return sl>=1&&sl<=4?folLoadSlot(sl,spr,shiny):0;}
extern "C" void P3DS_WildFree(int sl){if(sl<1||sl>4)return;for(int k=0;k<8;k++){Tex&t=folTex[sl*8+k];G3G_TexFree(t.gt);t.gt=nullptr;free(t.px);t.px=nullptr;}folCurS[sl]=-1;}
static inline Tex*folLookup(const G3SIM_TexImageParam_t&p){if(p.textureOffset>0x7FFF8||p.textureOffset<0x7FFD8||p.textureFormat!=3||(s_texPlttBase&~0x70u)!=0x1FF00)return nullptr;Tex*t=&folTex[((0x7FFF8-p.textureOffset)/8)*8+((s_texPlttBase>>4)&7)];return t->px?t:nullptr;}
static Tex*GetTextureImpl(){
 const auto&p=s_texImageParam;unsigned fmt=p.textureFormat;if(!fmt)return nullptr;
 if(p.textureOffset>=0x7FFD8&&p.textureOffset<=0x7FFF8)return folLookup(p);
 unsigned w=p.textureSSize,h=p.textureTSize;if(w<8||h<8||w>1024||h>1024)return nullptr;
 for(unsigned i=0;i<texN;i++){Tex&t=texCache[i];if(t.off==p.textureOffset&&t.pal==s_texPlttBase&&t.fmt==fmt&&t.w==w&&t.h==h&&t.c0==p.color0){
   if(t.gen==PSPNativeVramGen){t.lru=++texTick;return &t;}
   {int pc=texPageCfg();if(pc&&t.px&&t.cnt==VramCnt()&&TexPageGen(fmt,w,h,p.textureOffset,s_texPlttBase)<=t.gen){
     if(pc==1){t.gen=PSPNativeVramGen;t.lru=++texTick;texPageSkips++;return &t;}
     unsigned rn=GatherRaw(fmt,w,h,p.textureOffset,s_texPlttBase);if(!(rn&&t.raw&&t.rawN==rn&&!memcmp(t.raw,rawScratch,rn))){if(texPageMismatch++<20)PSPNativeMemLog("[G3] tex_pagecheck MISMATCH off %x fmt %u %ux%u",t.off,fmt,w,h);}else texPageSkips++;}}
   {unsigned rn=GatherRaw(fmt,w,h,p.textureOffset,s_texPlttBase);
    if(rn&&t.raw&&t.rawN==rn&&t.px&&!memcmp(t.raw,rawScratch,rn)){t.gen=PSPNativeVramGen;t.cnt=VramCnt();t.lru=++texTick;return &t;}
    free(t.raw);t.raw=rn?(u8*)malloc(rn):nullptr;t.rawN=t.raw?rn:0;if(t.raw)memcpy(t.raw,rawScratch,rn);}
   free(t.px);G3G_TexFree(t.gt);t.gt=nullptr;t.px=nullptr;t.gen=PSPNativeVramGen;t.cnt=VramCnt();t.lru=++texTick;goto decode_into_i;}
  continue;
  decode_into_i:{Tex*e=&t;if(!DecodeTex(e))return nullptr;return e;}
 }
 /* new entry */
 {unsigned slot;if(texN<96)slot=texN++;else{slot=0;for(unsigned i=1;i<texN;i++)if(texCache[i].lru<texCache[slot].lru)slot=i;free(texCache[slot].px);free(texCache[slot].raw);G3G_TexFree(texCache[slot].gt);}
  Tex&t=texCache[slot];t={p.textureOffset,s_texPlttBase,fmt,w,h,p.color0,PSPNativeVramGen-1u,nullptr,0,nullptr,0,nullptr};
  return GetTexture();}
}
/* ---------------- rasterizer ---------------- */
static u16 depth[256*192];
struct Batch{const GuVertex*v;unsigned n;Tex*tex;G3SIM_TexImageParam_t tp;bool dw;unsigned dtest;};
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
static int stereoDepth=-1;static inline float parOf(float zf){if(stereoDepth<0){stereoDepth=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"stereo_depth=%d",&x)==1)stereoDepth=x!=0;fclose(f);}PSPNativeMemLog("[G3] stereo_depth=%d",stereoDepth);}
 if(!stereoDepth)return 0.65f;if(zf<0)zf=0;if(zf>1)zf=1;float z2=zf*zf,z4=z2*z2,z8=z4*z4;return 0.3f+0.7f*z8;}
static void*G3SIM_GpuTex(Tex*t){return G3G_TexCreate(t->px,t->w,t->h);}
extern "C" u32 *P3DS_G3RightEye(void){return stereoSd?frameR:nullptr;}
static void TriEye(const GuVertex&a,const GuVertex&b,const GuVertex&c,Tex*tex,const G3SIM_TexImageParam_t&tp,bool blend,bool dwrite,unsigned dtest,u32*fb,u16*depth);
static void Tri(const GuVertex&a,const GuVertex&b,const GuVertex&c,Tex*tex,const G3SIM_TexImageParam_t&tp,bool blend,bool dwrite,unsigned dtest){
 if(!stereoSd){TriEye(a,b,c,tex,tp,blend,dwrite,dtest,GPU3D::NativeFrame,depth);return;}
 GuVertex v[3]={a,b,c};const GuVertex*o[3]={&a,&b,&c};
 for(int eye=-1;eye<=1;eye+=2){for(int k=0;k<3;k++){float par=parOf(o[k]->z*(1.0f/65535.0f));/* perspective NDC z bunches near 1: z^8 spreads it */v[k].x=o[k]->x+eye*stereoSd*par;}
  TriEye(v[0],v[1],v[2],tex,tp,blend,dwrite,dtest,eye<0?GPU3D::NativeFrame:frameR,eye<0?depth:depthR);}
}
static void TriEye(const GuVertex&a,const GuVertex&b,const GuVertex&c,Tex*tex,const G3SIM_TexImageParam_t&tp,bool blend,bool dwrite,unsigned dtest,u32*fb,u16*depth){
 if(gpuOn==1){const float xl=G3G_Wide?-32.0f:0.0f,xr=G3G_Wide?288.0f:256.0f;if(fmaxf(a.x,fmaxf(b.x,c.x))<xl||fminf(a.x,fminf(b.x,c.x))>xr||fmaxf(a.y,fmaxf(b.y,c.y))<0||fminf(a.y,fminf(b.y,c.y))>192)return;drawnTris++;
  if(tex&&!tex->gt)tex->gt=G3G_TexCreate(tex->px,tex->w,tex->h);
  float X[3]={a.x,b.x,c.x},Y[3]={a.y,b.y,c.y},Z[3]={a.z,b.z,c.z},U[3]={a.u,b.u,c.u},W[3]={a.v,b.v,c.v};u32 C[3]={a.color,b.color,c.color};
  G3G_Tri(fb==frameR?1:0,X,Y,Z,U,W,C,tex?tex->gt:nullptr,tex?tex->w:0,tex?tex->h:0,tp.repeatS,tp.repeatT,tp.flipS,tp.flipT,blend,dwrite,dtest);return;}
 float area=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);if(fabsf(area)<1e-6f)return;
 int x0=(int)floorf(fminf(a.x,fminf(b.x,c.x))),x1=(int)ceilf(fmaxf(a.x,fmaxf(b.x,c.x)));
 int y0=(int)floorf(fminf(a.y,fminf(b.y,c.y))),y1=(int)ceilf(fmaxf(a.y,fmaxf(b.y,c.y)));
 if(x0<0)x0=0;if(y0<0)y0=0;if(x1>256)x1=256;if(y1>192)y1=192;if(x0>=x1||y0>=y1)return;
 float ia=1.0f/area;drawnTris++;if(tex)stTexTris++;if(blend)stTrans++;stFmt[tp.textureFormat&7]++;stColOr|=a.color;stColAnd&=a.color;
 {const GuVertex*vv[3]={&a,&b,&c};for(int k=0;k<3;k++){float z=vv[k]->z,x=vv[k]->x;unsigned al=vv[k]->color>>24;if(z<stZmin)stZmin=z;if(z>stZmax)stZmax=z;if(x<stXmin)stXmin=x;if(x>stXmax)stXmax=x;if(al<stAmin)stAmin=al;if(al>stAmax)stAmax=al;}}
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
static bool rawMode=false;static float rawM[16];
struct CV{float x,y,z,w,u,v;float c[4];};
static GuVertex rawOut[6144*3];
static unsigned packC(const float*c){unsigned r=0;for(int k=0;k<4;k++){float f=c[k];if(f<0)f=0;if(f>255)f=255;r|=((unsigned)(f+0.5f))<<(8*k);}return r;}
static GuVertex toScreen(const CV&a){float iw=1.0f/a.w;GuVertex o;o.u=a.u;o.v=a.v;o.color=packC(a.c);
 o.x=128.0f+a.x*iw*128.0f;o.y=96.0f-a.y*iw*96.0f;o.z=a.z*iw*32767.0f+32768.0f;return o;}
extern "C" u8 s_SIM_useWBuffer;
static float wInv=1.0f/4096;
static int wbufOn(){static int v=-1;if(v<0){v=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f)){if(sscanf(l,"g3_wbuf=%d",&x)==1)v=x;if(sscanf(l,"g3_wmax=%d",&x)==1&&x>0)wInv=1.0f/x;}fclose(f);}PSPNativeMemLog("[GPU3D] g3_wbuf=%d (0 off, 1 when game selects W, 2 force)",v);}
 static u8 last=9;if(last!=s_SIM_useWBuffer){last=s_SIM_useWBuffer;PSPNativeMemLog("[GPU3D] game depth mode now %s",last?"W-buffer":"Z-buffer");}
 return v==2||(v==1&&s_SIM_useWBuffer);}
static unsigned RawToScreen(const GuVertex*v,unsigned n){
 unsigned out=0;const float tw=(float)s_texImageParam.textureSSize,th=(float)s_texImageParam.textureTSize;const float*M=rawM;
 for(unsigned i=0;i+2<n;i+=3){CV in[3],poly[8],tmp[8];
  for(int k=0;k<3;k++){const GuVertex&p=v[i+k];CV&c=in[k];
   c.x=p.x*M[0]+p.y*M[4]+p.z*M[8]+M[12];c.y=p.x*M[1]+p.y*M[5]+p.z*M[9]+M[13];c.z=p.x*M[2]+p.y*M[6]+p.z*M[10]+M[14];c.w=p.x*M[3]+p.y*M[7]+p.z*M[11]+M[15];if(wbufOn()){float d=c.w*wInv;if(d>1)d=1;c.z=(2*d-1)*c.w;}
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
  for(int k=1;k+1<np;k++){if(out+3>sizeof rawOut/sizeof rawOut[0])return out;rawOut[out++]=s0;rawOut[out++]=toScreen(poly[k]);rawOut[out++]=toScreen(poly[k+1]);}
 }
 return out;
}
static void DrawScreen(const GuVertex*v,unsigned n,bool deferOk);
static unsigned culledTris,cpuClipped,rejOut,rejMask;int vdump,vdumpN;
/* gpu3d_cpuclip=1 (default): clip/divide on the CPU (same as the clean CPU renderer), raster on the GPU.
   Run 26: black spots only on the clip-space GPU path. 0 = old clip-space path. */
static int gpuCpuClip(){static int v=-1;if(v<0){v=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");char l[64];int x;if(f){while(fgets(l,sizeof l,f))if(sscanf(l,"gpu3d_cpuclip=%d",&x)==1)v=x!=0;fclose(f);}PSPNativeMemLog("[GPU3D] gpu3d_cpuclip=%d",v);}return v;}
static void GpuRaw(const GuVertex*v,unsigned n){
 if(cullAll)return;G3G_CurFog=s_curPolygonAttr.fogEnable?1:0;const int cullMode3d=cull3dCfg();Tex*tex=GetTexture();const G3SIM_TexImageParam_t tp=s_texImageParam;
 bool translucent=(v[0].color>>24)<250||(tex&&(tp.textureFormat==1||tp.textureFormat==6));
 bool dw=!translucent||s_SIM_transDepthWrite;unsigned dtest=s_curPolygonAttr.depthTest;
 if(tex&&!tex->gt)tex->gt=G3SIM_GpuTex(tex);
 const float*M=rawM;
/* run 62 stereo_edgefix=1 (default): the per-eye stereo shift (up to stereoSd DS px, right eye +, left eye -) is added
   AFTER this x-plane reject, so tris just outside the left plane that the right-eye shift would bring on screen were
   dropped -> a few-px black column at the far left of the right eye (wide view, slider up; mirror case at the right edge
   of the left eye). Widen the x reject planes by the maximum shift (stereoSd/128 clip units per w; par <= 1). */
 static int edgeFix=-1;if(edgeFix<0){edgeFix=P3DS_CfgInt("stereo_edgefix",1)!=0;PSPNativeMemLog("[G3] stereo_edgefix=%d",edgeFix);}
 const float qwK=(G3G_Wide?1.25f:1.0f)+(edgeFix&&stereoSd?stereoSd*(1.0f/128.0f):0.0f);
 for(unsigned i=0;i+2<n;i+=3){float c[12];unsigned out=0x3F;u32 col[3];float su[3],sv[3];
  for(int k=0;k<3;k++){const GuVertex&p=v[i+k];float*q=c+4*k;
   q[0]=p.x*M[0]+p.y*M[4]+p.z*M[8]+M[12];q[1]=p.x*M[1]+p.y*M[5]+p.z*M[9]+M[13];q[2]=p.x*M[2]+p.y*M[6]+p.z*M[10]+M[14];q[3]=p.x*M[3]+p.y*M[7]+p.z*M[11]+M[15];
   unsigned o=0;const float qw=qwK*q[3];/* run 43 wideview: keep tris in the side strips */if(q[0]>qw)o|=1;if(q[0]<-qw)o|=2;if(q[1]>q[3])o|=4;if(q[1]<-q[3])o|=8;if(q[2]>q[3])o|=16;if(q[2]<-q[3])o|=32;out&=o;
   col[k]=p.color;su[k]=p.u;sv[k]=p.v;}
  if(wbufOn()){/* run 31: DS W-buffer (SwapBuffers bit1): depth = w, linear. With z-buffer depth the title's projection
     gave z==w for every vertex (all depth ~1.0), so Giratina's body lost the depth test. */
   for(int k=0;k<3;k++){float*q=c+4*k;float d=q[3]*wInv;if(d>1)d=1;q[2]=(2*d-1)*q[3];}}
  if(out){rejOut++;rejMask|=out;if(vdump&&vdumpN<400){vdumpN++;PSPNativeMemLog("[VDUMP] REJ out=%02x v0 %.2f %.2f %.2f %.2f v1 %.2f %.2f %.2f %.2f v2 %.2f %.2f %.2f %.2f",out,c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7],c[8],c[9],c[10],c[11]);}continue;}/* all three vertices outside one plane */
  if(vdump&&vdumpN<400&&(drawnTris%8)==0){vdumpN++;PSPNativeMemLog("[VDUMP] OK v0 %.2f %.2f %.2f %.2f v1 %.2f %.2f %.2f %.2f v2 %.2f %.2f %.2f %.2f",c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7],c[8],c[9],c[10],c[11]);}
  if(gpuCpuClip()){/* run 27: only tris touching the near/far planes or outside a 2x guard band go through CPU clipping
     (PICA clipping there gave the black spots); the rest stay on the fast clip-space path. */
   bool hard=false;for(int k=0;k<3;k++){const float*q=c+4*k;float w=q[3];if(w<1e-3f||q[2]>w||q[2]<-w||fabsf(q[0])>2*w||fabsf(q[1])>2*w){hard=true;break;}}
   if(hard){unsigned m=RawToScreen(v+i,3);if(m)DrawScreen(rawOut,m,false);cpuClipped++;continue;}}
  if(cullFace&&cullMode3d&&c[3]>1e-6f&&c[7]>1e-6f&&c[11]>1e-6f){
   float x0=c[0]/c[3],y0=c[1]/c[3],x1=c[4]/c[7],y1=c[5]/c[7],x2=c[8]/c[11],y2=c[9]/c[11];
   float area=(x1-x0)*(y2-y0)-(x2-x0)*(y1-y0);if(cullMode3d==1)area=-area;/* run 26: the old sign culled ~all field tris (36 left) */
   if((cullFace==1&&area>0)||(cullFace==2&&area<0)){culledTris++;continue;}}
  drawnTris++;
  if(!stereoSd){G3G_TriClip(0,c,su,sv,col,tex?tex->gt:nullptr,tp.repeatS,tp.repeatT,tp.flipS,tp.flipT,translucent,dw,dtest,nullptr);continue;}
  bool degen=false;for(int k=0;k<3;k++){float w=c[4*k+3],zf=w>1e-6f?0.5f*c[4*k+2]/w+0.5f:1.0f;if(zDegenerate(w,zf)){degen=true;break;}}if(degen)stereoClamped++;
  for(int eye=0;eye<2;eye++){float sh[3];for(int k=0;k<3;k++){float w=c[4*k+3],zf=w>1e-6f?0.5f*c[4*k+2]/w+0.5f:1.0f;sh[k]=(eye?1:-1)*stereoSd*(degen?0.65f:parOf(zf));}
   G3G_TriClip(eye,c,su,sv,col,tex?tex->gt:nullptr,tp.repeatS,tp.repeatT,tp.flipS,tp.flipT,translucent,dw,dtest,sh);}
 }
}
static void Draw(const GuVertex*v,unsigned n,bool deferOk){
 if(rawMode&&n&&gpuOn==1){u64 t=svcGetSystemTick();GpuRaw(v,n);prRaw+=svcGetSystemTick()-t;return;}
 if(rawMode&&n){unsigned m=RawToScreen(v,n);DrawScreen(rawOut,m,deferOk);return;}
 DrawScreen(v,n,deferOk);
}
static void DrawScreen(const GuVertex*v,unsigned n,bool deferOk){
 if(!n||cullAll)return;G3G_CurFog=s_curPolygonAttr.fogEnable?1:0;Tex*tex=GetTexture();const G3SIM_TexImageParam_t tp=s_texImageParam;
 bool translucent=(v[0].color>>24)<250||(tex&&(tp.textureFormat==1||tp.textureFormat==6));
 unsigned dtest=s_curPolygonAttr.depthTest;
 if(translucent&&deferOk){if(transN<1024&&transVN+n<=24576){memcpy(transVerts+transVN,v,n*sizeof(GuVertex));trans[transN++]={transVerts+transVN,n,tex,tp,(bool)s_SIM_transDepthWrite,dtest};transVN+=n;return;}}
 for(unsigned i=0;i+2<n;i+=3)Tri(v[i],v[i+1],v[i+2],tex,tp,translucent,!translucent||s_SIM_transDepthWrite,dtest);
}
extern "C" {
void G3ListRecordBegin(){recActive=true;recBroken=false;g3RecCount=0;}
int G3ListRecordEnd(unsigned*count,const GuVertex**verts){recActive=false;*count=g3RecCount;*verts=g3ChunkPtr-g3RecCount;return !recBroken;}
void G3ListReplay(const GuVertex*v,unsigned n){if(n>g3ChunkLeft)G3SIM_ChunkAlloc();memcpy(g3ChunkPtr,v,n*sizeof(GuVertex));g3ChunkPtr+=n;g3ChunkLeft-=n;}
void G3SIM_FlushArray();
void G3ListDrawDirect(const GuVertex*v,unsigned n){G3SIM_FlushArray();Draw(v,n,true);}
void G3ListDeferFree(void*p){free(p);}
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
void G3SIM_FlushArray(){G3SIM_DrawArray();ResetVertices();}
static u64 frameTicks,sumTicks;static unsigned frames,lastUs,lastTris;
void G3SIM_FlushDeferred(){
 CRUMB("g3-flush");G3SIM_FlushArray();G3SIM_TexImageParam_t save=s_texImageParam;
 for(unsigned i=0;i<transN;i++){Batch&b=trans[i];for(unsigned k=0;k+2<b.n;k+=3)Tri(b.v[k],b.v[k+1],b.v[k+2],b.tex,b.tp,true,b.dw,b.dtest);}
 transN=transVN=0;s_texImageParam=save;
 if(gpuOn==1&&!G3G_Async&&G3G_RbSyncOn()&&postVblCfg()){
  for(unsigned i=0;i<usedN;i++){Tex*t=usedTex[i];if(!t->px||t->useF!=g3FrameNo+1||t->gen==PSPNativeVramGen)continue;
   if(TexPageGen(t->fmt,t->w,t->h,t->off,t->pal)>t->gen){unsigned rn=GatherRaw(t->fmt,t->w,t->h,t->off,t->pal);
    if(rn&&!(t->raw&&t->rawN==rn&&!memcmp(t->raw,rawScratch,rn))){
     if(t->raw&&t->rawN==rn)memcpy(t->raw,rawScratch,rn);else{free(t->raw);t->raw=(u8*)malloc(rn);t->rawN=t->raw?rn:0;if(t->raw)memcpy(t->raw,rawScratch,rn);}
     if(DecodeTex(t)){if(t->gt)G3G_TexUpdate(t->gt,t->px,t->w,t->h);postFixes++;}}
    if(rn){t->gen=PSPNativeVramGen;t->cnt=VramCnt();}}}
  {static unsigned f;if(++f%900==0){PSPNativeMemLog("[G3] tex_postvblank: %u textures refreshed after VBlank (30 s)",postFixes);postFixes=0;}}}
 /* run 63 tex_trace=1 (debug, default 0): per 3D frame, every small texture used (<=64x64) with a hash of its VRAM
    bytes -> shows whether a sprite's VRAM really changes (game side) or the cache misses it (port side). */
 {static int tt=-1;if(tt<0)tt=P3DS_CfgInt("tex_trace",0);if(tt>0){struct E{unsigned off,pal,wh;u32 h;};static E prv[96],cur[96];static unsigned pn;unsigned cn=0;
  for(unsigned i=0;i<usedN&&cn<96;i++){Tex*t=usedTex[i];if(t->w>64||t->h>64)continue;u32 hsh=2166136261u;for(unsigned j=0;j<t->rawN;j++)hsh=(hsh^t->raw[j])*16777619u;cur[cn++]={t->off,t->pal,t->w<<8|t->h|t->fmt<<16,hsh};}
  char b[200];int k=0;auto fl=[&](){if(k){PSPNativeMemLog("[TEXTR] f%u g%u%s",g3FrameNo,PSPNativeVramGen,b);k=0;}};
  for(unsigned i=0;i<cn;i++){bool same=false,had=false;for(unsigned j=0;j<pn;j++)if(prv[j].off==cur[i].off&&prv[j].pal==cur[i].pal&&prv[j].wh==cur[i].wh){had=true;same=prv[j].h==cur[i].h;break;}
   if(!same){k+=snprintf(b+k,sizeof b-k," %c%x/%x/%x:%08lx",had?'*':'+',cur[i].off,cur[i].pal,cur[i].wh,(unsigned long)cur[i].h);if(k>140)fl();}}
  for(unsigned j=0;j<pn;j++){bool f=false;for(unsigned i=0;i<cn;i++)if(prv[j].off==cur[i].off&&prv[j].pal==cur[i].pal&&prv[j].wh==cur[i].wh){f=true;break;}
   if(!f){k+=snprintf(b+k,sizeof b-k," -%x/%x/%x",prv[j].off,prv[j].pal,prv[j].wh);if(k>140)fl();}}
  fl();memcpy(prv,cur,cn*sizeof(E));pn=cn;}}
 usedN=0;
 if(gpuOn==1){u64 t=svcGetSystemTick();G3G_FogNoW=!wbufOn();G3G_End(GPU3D::NativeFrame,stereoSd?frameR:nullptr);prEnd+=svcGetSystemTick()-t;prN++;}
 {u64 d=svcGetSystemTick()-frameTicks;lastUs=(unsigned)(d/268);lastTris=drawnTris;sumTicks+=d;}if(++frames==120){sumTicks=0;frames=0;
  if(drawnTris)PSPNativeMemLog("[G3DIAG] gpu3d %d %uus | decodes %u | tex %u trans %u | px tested %u depthrej %u texalpha0 %u alpha0 %u written %u | z %.0f..%.0f x %.1f..%.1f a %u..%u | clear %08lx depth %04x dtest %u | fmt0-7 %u %u %u %u %u %u %u %u | vcol or %08lx and %08lx",gpuOn,G3G_LastUs(),stDecodes,
   stTexTris,stTrans,stPix,stDepth,stTexA,stA0,stWritten,stZmin,stZmax,stXmin,stXmax,stAmin,stAmax,(unsigned long)*(u32*)(PSPNative_GfxRegisters+0x350),*(u16*)(PSPNative_GfxRegisters+0x354),s_curPolygonAttr.depthTest,stFmt[0],stFmt[1],stFmt[2],stFmt[3],stFmt[4],stFmt[5],stFmt[6],stFmt[7],(unsigned long)stColOr,(unsigned long)stColAnd);}
 memset(stFmt,0,sizeof stFmt);stColOr=0;stColAnd=~0u;
 stDecodes=stPix=stDepth=stTexA=stA0=stWritten=stTexTris=stTrans=0;stZmin=stXmin=1e9f;stZmax=stXmax=-1e9f;stAmin=255;stAmax=0;drawnTris=0;{static unsigned f;++f;vdump=(f%150==0&&f<=600);if(f%60==0)PSPNativeMemLog("[GPU3D] f=%u trivially rejected %u (planes %02x) of drawn %u",f,rejOut,rejMask,drawnTris);rejOut=rejMask=0;if(f%300==0){PSPNativeMemLog("[GPU3D] cpu-clipped tris %u, culled %u (this frame) | tex page-skips %u mismatches %u (300 f)",cpuClipped,culledTris,texPageSkips,texPageMismatch);texPageSkips=0;if(prN)PSPNativeMemLog("[G3PROF] avg/frame: GetTexture %u us | GpuRaw (transform+clip+queue, incl. its GetTexture) %u us | G3G_End (GPU wait+readback) %u us | rest of g3soft = pspoke frontend/handler",(unsigned)(prTex/prN/268),(unsigned)(prRaw/prN/268),(unsigned)(prEnd/prN/268));prTex=prRaw=prEnd=prN=0;if(prN)PSPNativeMemLog("[G3PROF] per frame avg: GetTexture %u us, GpuRaw (transform+clip+queue, incl. its GetTexture) %u us, G3G_End (GPU wait+readback) %u us; rest of g3soft = pspoke frontend/handler",(unsigned)(prTex/prN/268),(unsigned)(prRaw/prN/268),(unsigned)(prEnd/prN/268));prTex=prRaw=prEnd=prN=0;}cpuClipped=culledTris=0;}}
unsigned G3SIM_DeferredHighWater(){return 0;}
void PSPNativeG3Release(){ResetVertices();}
void PSPNativeG3TrimCache(){}
static unsigned rawGen;
void PSPNativeRawMatrix(const s32*m,unsigned gen){if(rawMode&&rawGen==gen)return;G3SIM_FlushArray();rawMode=true;rawGen=gen;for(int i=0;i<16;i++)rawM[i]=m[i]/4096.0f;}
void PSPNativeRawMode(int v){if(rawMode!=(v!=0)){G3SIM_FlushArray();rawMode=v!=0;}}
/* Frame start: clear the 3D layer to the DS CLEAR_COLOR / CLEAR_DEPTH registers. */
extern "C" int P3DS_StereoShiftNow(void);
void P3DS_G3BeginFrame(void){CRUMB("g3-begin");g3FrameNo++;usedN=0;frameTicks=svcGetSystemTick();stereoSd=P3DS_StereoShiftNow();
 u32 cc=*(u32*)(PSPNative_GfxRegisters+0x350);u16 cd=*(u16*)(PSPNative_GfxRegisters+0x354);
 unsigned r=cc&31,g=(cc>>5)&31,b=(cc>>10)&31,a=(cc>>16)&31;u32 v=(r<<1|r>>4)|((g<<1|g>>4)<<8)|((b<<1|b>>4)<<16)|(a<<24);
 if(gpuOn<0){int want=1;FILE*f=fopen("sdmc:/3ds/platinum3ds/config.txt","r");if(f){char l[64];int x;while(fgets(l,sizeof l,f)){if(sscanf(l,"gpu3d=%d",&x)==1)want=x;if(sscanf(l,"gpu3d_texflip=%d",&x)==1)G3G_TexFlip=x;if(sscanf(l,"gpu3d_flipy=%d",&x)==1)G3G_FlipY=x;if(sscanf(l,"gpu3d_zinv=%d",&x)==1)G3G_ZInv=x;if(sscanf(l,"gpu3d_async=%d",&x)==1)G3G_Async=x!=0;}fclose(f);}
  gpuOn=want&&G3G_Init()?1:0;if(want&&!gpuOn)PSPNativeMemLog("[GPU3D] GPU 3D init FAILED -> automatic CPU 3D fallback");PSPNativeMemLog("[GPU3D] gpu3d=%d (config wants %d) texflip=%d zinv=%d async=%d",gpuOn,want,G3G_TexFlip,G3G_ZInv,G3G_Async);}
 if(gpuReq>=0){gpuOn=gpuReq&&G3G_Init()?1:0;gpuReq=-1;PSPNativeMemLog("[GPU3D] L+R+Y: gpu3d=%d",gpuOn);}
 if(gpuOn==1){u16 dz=(u16)(((cd&0x7FFF)*2)|1);G3G_Begin(v,dz?dz:0xFFFF,stereoSd);transN=transVN=0;return;}
 for(unsigned i=0;i<256*192;i++)GPU3D::NativeFrame[i]=v;
 u16 dz=(u16)(((cd&0x7FFF)*2)|1);for(unsigned i=0;i<256*192;i++)depth[i]=dz?dz:0xFFFF;
 if(stereoSd){memcpy(frameR,GPU3D::NativeFrame,sizeof frameR);memcpy(depthR,depth,sizeof depthR);}
 transN=transVN=0;
}

void P3DS_G3ToggleGpu(void){gpuReq=gpuOn==1?0:1;}
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

/* platinum3ds G3 backend (step 2a: vertices collected in the stack-render GuVertex format, then dropped).
 * Same entry points as pspoke native-stack-render/g3_backend.cpp, so a citro3d DrawVertices can slot in. */
#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <nitro.h>
#include <simulator/g3_draw.h>
#include <simulator/g3_handler.h>
#include "../native-stack-render/g3_vtx.h"
static GuVertex vertices[24576] __attribute__((aligned(16)));
GuVertex *g3ChunkPtr=vertices;unsigned g3ChunkLeft=24576;
unsigned g3RecCount=0;static bool recActive=false,recBroken=false;
static unsigned drawnVerts;
extern "C" void abort(void);
extern "C" void G3SIM_ChunkAlloc(){printf("[NATIVE-RENDER] vertex overflow\n");abort();for(;;){}}
static void ResetVertices(){if(recActive&&g3RecCount)recBroken=true;g3ChunkPtr=vertices;g3ChunkLeft=24576;}
static void DrawVertices(const GuVertex*v,unsigned n,bool copy){(void)v;(void)copy;drawnVerts+=n;}
extern "C" {
void G3ListRecordBegin(){recActive=true;recBroken=false;g3RecCount=0;}
int G3ListRecordEnd(unsigned*count,const GuVertex**verts){recActive=false;*count=g3RecCount;*verts=g3ChunkPtr-g3RecCount;return !recBroken;}
void G3ListReplay(const GuVertex*v,unsigned n){if(n>g3ChunkLeft)G3SIM_ChunkAlloc();memcpy(g3ChunkPtr,v,n*sizeof(GuVertex));g3ChunkPtr+=n;g3ChunkLeft-=n;}
void G3SIM_FlushArray();
void G3ListDrawDirect(const GuVertex*v,unsigned n){G3SIM_FlushArray();DrawVertices(v,n,false);}
void G3ListDeferFree(void*p){free(p);}
void G3ListPendingFreeDrain(){}
unsigned G3ListPendingFreeHigh(){return 0;}
u8 s_SIM_useWBuffer=0;
void SIM_u16ToRGB(u16 c,u8*r,u8*g,u8*b){unsigned x=c&31,y=(c>>5)&31,z=(c>>10)&31;*r=(x<<3)|(x>>2);*g=(y<<3)|(y>>2);*b=(z<<3)|(z>>2);}
static void APIENTRY NoDebug(GLenum,GLenum,GLuint,GLenum,GLsizei,const GLchar*){}
static void APIENTRY NoCap(GLenum){}
PFNGLDEBUGMESSAGEINSERTPROC glad_glDebugMessageInsert=NoDebug;
PFNGLENABLEPROC glad_glEnable=NoCap;PFNGLDISABLEPROC glad_glDisable=NoCap;PFNGLCULLFACEPROC glad_glCullFace=NoCap;PFNGLDEPTHFUNCPROC glad_glDepthFunc=NoCap;
static unsigned channel(float f){if(f<0)f=0;if(f>1)f=1;return unsigned(f*255.0f+0.5f);}
void G3SIM_AddVtx(G3SIM_Vertex_t*v){if(!g3ChunkLeft)G3SIM_ChunkAlloc();
 *g3ChunkPtr++={v->s*s_texImageParam.textureSSize,v->t*s_texImageParam.textureTSize,channel(v->r)|(channel(v->g)<<8)|(channel(v->b)<<16)|(channel(v->a)<<24),128.0f+v->x*128.0f,96.0f-v->y*96.0f,v->z*32767.0f+32768.0f};g3ChunkLeft--;}
void G3SIM_DrawArray(){DrawVertices(vertices,(unsigned)(g3ChunkPtr-vertices),true);}
void G3SIM_DrawCleanUp(){ResetVertices();}
void G3SIM_FlushArray(){G3SIM_DrawArray();ResetVertices();}
void G3SIM_FlushDeferred(){}
unsigned G3SIM_DeferredHighWater(){return 0;}
void PSPNativeG3Release(){ResetVertices();}
void PSPNativeG3TrimCache(){}
void PSPNativeRawMatrix(const s32*m,unsigned gen){(void)m;(void)gen;}
void PSPNativeRawMode(int v){(void)v;}
unsigned P3DS_G3TakeVerts(void){unsigned d=drawnVerts;drawnVerts=0;return d;}
}

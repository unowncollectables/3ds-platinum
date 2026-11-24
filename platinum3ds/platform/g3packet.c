#include <nitro.h>
#include <simulator/drawmsg.h>
#include <string.h>

extern int P3DS_CfgInt(const char *,int);
extern void PSPNativeMemLog(const char *,...);
extern void PSPNativeG3CommandsBorrowed(const void *,unsigned);
extern void __real_NNS_G3dGeBufferOP_N(u32,const u32 *,u32);
extern BOOL NNS_G3dGeIsBufferExist(void);
extern BOOL NNS_G3dGeIsSendDLBusy(void);
extern volatile int P3DS_G3ProfLive;
extern void P3DS_G3ProfAddCmd(unsigned long long);
extern unsigned long long svcGetSystemTick(void);
#ifdef P3DS_PACKET_ORACLE
extern int P3DS_PacketDecodeCheck(u32,const u32 *,u32,const draw_msg_t *,unsigned);
#endif

static int enabled=-1;
int P3DS_G3PacketCfg(void){
 if(enabled<0){enabled=P3DS_CfgInt("g3_packets",1)!=0;PSPNativeMemLog("[G3PACK] g3_packets=%d (bounded synchronous setup-command shortcut)",enabled);}
 return enabled;
}
void P3DS_SetG3Packets(int v){enabled=v!=0;PSPNativeMemLog("[G3PACK] g3_packets=%d from settings",enabled);}

/* Only non-vertex setup commands. The original frontend remains responsible
 * for primitive assembly and the previous-vertex state used by partial VTX ops.
 * Payload fields have the same compact representation as ConvertMsgParams. */
static unsigned direct(u32 op,const u32 *args,u32 words,draw_msg_t *m){
 unsigned n,bytes;draw_command_type_t type;
#define SET(o,t,w,b) case o:type=t;n=w;bytes=b;break
 switch(op){
 SET(G3OP_NOP,DRAW_CMD_G3_NOP,0,0);
 SET(G3OP_MTX_MODE,DRAW_CMD_G3_MTXMODE,1,1);
 SET(G3OP_MTX_PUSH,DRAW_CMD_G3_PUSHMTX,0,0);
 SET(G3OP_MTX_POP,DRAW_CMD_G3_POPMTX,1,1);
 SET(G3OP_MTX_STORE,DRAW_CMD_G3_STOREMTX,1,1);
 SET(G3OP_MTX_RESTORE,DRAW_CMD_G3_RESTOREMTX,1,1);
 SET(G3OP_MTX_IDENTITY,DRAW_CMD_G3_IDENTITY,0,0);
 SET(G3OP_MTX_LOAD_4x4,DRAW_CMD_G3_LOADMTX44,16,64);
 SET(G3OP_MTX_LOAD_4x3,DRAW_CMD_G3_LOADMTX43,12,48);
 SET(G3OP_MTX_MULT_4x4,DRAW_CMD_G3_MTXMULT44,16,64);
 SET(G3OP_MTX_MULT_4x3,DRAW_CMD_G3_MTXMULT43,12,48);
 SET(G3OP_MTX_MULT_3x3,DRAW_CMD_G3_MTXMULT33,9,36);
 SET(G3OP_MTX_SCALE,DRAW_CMD_G3_SCALE,3,12);
 SET(G3OP_MTX_TRANS,DRAW_CMD_G3_TRANSLATE,3,12);
 SET(G3OP_COLOR,DRAW_CMD_G3_COLOR,1,2);
 SET(G3OP_NORMAL,DRAW_CMD_G3_NORMAL,1,4);
 SET(G3OP_TEXCOORD,DRAW_CMD_G3_TEXCOORD,1,4);
 SET(G3OP_POLYGON_ATTR,DRAW_CMD_G3_POLYGONATTR,1,4);
 SET(G3OP_TEXIMAGE_PARAM,DRAW_CMD_G3_TEXIMAGEPARAM,1,4);
 SET(G3OP_TEXPLTT_BASE,DRAW_CMD_G3_TEXPLTTBASE,1,4);
 SET(G3OP_DIF_AMB,DRAW_CMD_G3_DIFF_AMB,1,4);
 SET(G3OP_SPE_EMI,DRAW_CMD_G3_SPEC_EMI,1,4);
 SET(G3OP_LIGHT_VECTOR,DRAW_CMD_G3_LIGHTVECTOR,1,4);
 SET(G3OP_LIGHT_COLOR,DRAW_CMD_G3_LIGHTCOLOR,1,4);
 SET(G3OP_BOX_TEST,DRAW_CMD_G3_BOXTEST,3,12);
 default:return 0;
 }
#undef SET
 if(words!=n)return 0;
 m->type=type;m->size=0;if(bytes)memcpy(&m->data,args,bytes);
#ifdef P3DS_PACKET_ORACLE
 if(!P3DS_PacketDecodeCheck(op,args,words,m,bytes))return 0;
#endif
 return 1;
}

void __wrap_NNS_G3dGeBufferOP_N(u32 op,const u32 *args,u32 words){
 static unsigned used,packed,fallback;
 const int prof=P3DS_G3ProfLive;
 const unsigned long long t0=prof?svcGetSystemTick():0;
 if(!P3DS_G3PacketCfg()||words>64||(words&&!args)||NNS_G3dGeIsBufferExist()||NNS_G3dGeIsSendDLBusy()){
  fallback++;__real_NNS_G3dGeBufferOP_N(op,args,words);
  if(prof)P3DS_G3ProfAddCmd(svcGetSystemTick()-t0);
  return;
 }
 draw_msg_t msg;
 if(direct(op,args,words,&msg)){used++;SIM_HandleG3Command(&msg);}
 else{
  /* Borrowed parsing consumes the packet synchronously and retains no pointer
   * to this stack storage. Unsupported/grouped opcodes keep the existing parser. */
  u32 packet[65];packet[0]=op;if(words)memcpy(packet+1,args,words*4);
  packed++;PSPNativeG3CommandsBorrowed(packet,(words+1)*4);
 }
 if(prof)P3DS_G3ProfAddCmd(svcGetSystemTick()-t0);
 if((used+packed)%100000==0)PSPNativeMemLog("[G3PACK] direct=%u packed=%u original=%u",used,packed,fallback);
}

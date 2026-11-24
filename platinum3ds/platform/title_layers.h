#pragma once
#include <stdint.h>
#include <string.h>
/* Restricted engine-B text compositor. Palette colours, alpha and master
 * brightness use the original renderer's integer six-bit channel arithmetic.
 * Wide layout scales only BG3; logo/text layers keep native coordinates. */
static uint16_t TitleR16(const uint8_t*r,unsigned n){return r[n]|((unsigned)r[n+1]<<8);}
static uint32_t TitlePal6(unsigned c){return ((c&31)<<1)|((c&0x3E0)<<4)|((c&0x7C00)<<7);}
static uint32_t TitleBlend(uint32_t a,uint32_t b,unsigned eva,unsigned evb){
 unsigned r=(((a&63)*eva)+((b&63)*evb)+8)>>4;
 unsigned g=((a&0x3F00)*eva+(b&0x3F00)*evb+0x800)>>4&0x7F00;
 unsigned bl=((a&0x3F0000)*eva+(b&0x3F0000)*evb+0x80000)>>4&0x7F0000;
 if(r>63)r=63;if(g>0x3F00)g=0x3F00;if(bl>0x3F0000)bl=0x3F0000;return r|g|bl;
}
static uint32_t TitleBright(uint32_t c,unsigned eff,unsigned f,unsigned bias){
 uint32_t rb=c&0x3F003F,g=c&0x3F00;
 if(eff==2){rb+=((((0x3F003F-rb)*f+bias*0x10001)>>4)&0x3F003F);g+=((((0x3F00-g)*f+bias*0x100)>>4)&0x3F00);}
 else{rb-=(((rb*f+bias*0x10001)>>4)&0x3F003F);g-=(((g*f+bias*0x100)>>4)&0x3F00);}return rb|g;
}
static void TitleTextRow(const uint8_t*bg,const uint32_t*pal,unsigned cnt,unsigned ho,unsigned vo,unsigned y,uint32_t*row){
 const unsigned bpp=!!(cnt&128),sz=cnt>>14,w=(sz&1)?64:32,h=(sz&2)?64:32;
 const unsigned wy=(y+vo)&(h*8-1),ty=wy>>3,map=((cnt>>8)&31)*2048,chars=((cnt>>2)&15)*0x4000;
 for(unsigned k=0;k<33;k++){
  unsigned tx=((ho>>3)+k)&(w-1),ma=(map+((tx>>5)+(ty>>5)*(w>>5))*2048+((ty&31)*32+(tx&31))*2)&0x1FFFF;
  unsigned me=bg[ma]|((unsigned)bg[(ma+1)&0x1FFFF]<<8),py=wy&7;if(me&0x800)py=7-py;
  unsigned addr=(chars+(me&1023)*(bpp?64:32)+py*(bpp?8:4))&0x1FFFF;
  for(unsigned p=0;p<8;p++){
   int sx=(int)(k*8+p)-(int)(ho&7);if(sx<0||sx>=256)continue;
   unsigned px=(me&0x400)?7-p:p,c=bg[addr+(bpp?px:px/2)];if(!bpp)c=(c>>((px&1)*4))&15;
   row[sx]=c?(pal[bpp?c:((me>>12)*16+c)]|0x80000000u):0;
  }
 }
}
static int SS3DS_Title2D(const uint8_t*r,const uint8_t*bg,const uint16_t*palette,uint32_t*out,int wide){
 unsigned dc=(unsigned)TitleR16(r,0)|((unsigned)TitleR16(r,2)<<16);
 if(((dc>>16)&3)!=1||(dc&7)||(dc&0xF080)||((dc>>30)&1))return 0;
 unsigned cnt[4],layers[4],n=0;uint32_t pal[256],rows[4][256];
 for(unsigned i=0;i<4;i++){cnt[i]=TitleR16(r,8+i*2);if((dc&(0x100u<<i))&&(cnt[i]&64)&&(TitleR16(r,0x4C)&255))return 0;}
 /* Highest priority first; BG0 wins ties, then BG1, BG2, BG3. */
 for(unsigned p=0;p<4;p++)for(unsigned i=0;i<4;i++)if((dc&(0x100u<<i))&&(cnt[i]&3)==p)layers[n++]=i;
 for(unsigned i=0;i<256;i++)pal[i]=TitlePal6(palette[i]);
 unsigned bl=TitleR16(r,0x50),eff=(bl>>6)&3,eva=TitleR16(r,0x52)&31,evb=(TitleR16(r,0x52)>>8)&31,evy=TitleR16(r,0x54)&31;
 if(eva>16)eva=16;if(evb>16)evb=16;if(evy>16)evy=16;
 unsigned mb=TitleR16(r,0x6C),mm=mb>>14,mf=mb&31;if(mf>16)mf=16;
 uint8_t expand[64];for(unsigned c=0;c<64;c++){unsigned v=c;if(mm==1)v+=((63-v)*mf)>>4;else if(mm==2)v-=(v*mf+15)>>4;expand[c]=(v<<2)|(v>>4);}
 const unsigned width=wide?320:256,off=wide?32:0;
 for(unsigned y=0;y<192;y++){
  for(unsigned k=0;k<n;k++){unsigned i=layers[k];TitleTextRow(bg,pal,cnt[i],TitleR16(r,0x10+i*4)&511,TitleR16(r,0x12+i*4)&511,y,rows[i]);}
  for(unsigned x=0;x<width;x++){
   uint32_t top=pal[0],below=pal[0];unsigned topMask=32,belowMask=32,found=0;
   for(unsigned k=0;k<n;k++){
    unsigned i=layers[k],sx;if(wide&&i==3)sx=(4*x+2)/5;else{if(x<off||x>=off+256)continue;sx=x-off;}
    uint32_t c=rows[i][sx];if(!(c&0x80000000u))continue;
    if(!found){top=c;topMask=1u<<i;found=1;}else{below=c;belowMask=1u<<i;break;}
   }
   uint32_t c=top&0x3F3F3F;
   if(bl&topMask){if(eff==1&&((bl>>8)&belowMask))c=TitleBlend(c,below,eva,evb);else if(eff>=2)c=TitleBright(c,eff,evy,eff==2?8:7);}
   out[y*width+x]=0xFF000000u|expand[c&63]|((unsigned)expand[(c>>8)&63]<<8)|((unsigned)expand[(c>>16)&63]<<16);
  }
 }
 return 1;
}

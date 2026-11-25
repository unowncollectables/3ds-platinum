#pragma once
/* Tile sampler for a normal 4/8-bit DS affine OBJ. Coordinates stay in the
 * original renderer's signed 8.8 fixed-point representation. Index zero is
 * transparent. Derived from GPU2D_Soft.cpp DrawSprite_Rotscale. */
static inline unsigned P3DS_AffineIndex(const unsigned char *vram,unsigned mask,
 unsigned dc,unsigned tile,unsigned width,unsigned height,int bpp8,int rx,int ry){
 if((unsigned)rx>=(width<<8)||(unsigned)ry>=(height<<8))return 0;
 unsigned x=(unsigned)rx>>8,y=(unsigned)ry>>8;
 unsigned base=tile*32,bytes=bpp8?64:32;
 unsigned stride=1024;
 if(dc&16){base<<=(dc>>20)&3;stride=(width>>3)*bytes;}
 unsigned addr=base+(y>>3)*stride+(x>>3)*bytes+(y&7)*(bpp8?8:4)+(bpp8?(x&7):((x&7)>>1));
 unsigned value=vram[addr&mask];
 return bpp8?value:((value>>((x&1)*4))&15);
}

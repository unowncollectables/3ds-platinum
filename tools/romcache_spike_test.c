/* run 86: x86 test for romcache3ds.c [SPIKE] file naming. Build + run (from an empty dir, it writes ./sdmc:/...):
 *   mkdir -p /tmp/rct/inc /tmp/rct/run && cp tools/romcache_spike_test_3ds.h /tmp/rct/inc/3ds.h
 *   gcc -O2 -I/tmp/rct/inc tools/romcache_spike_test.c platinum3ds/platform/romcache3ds.c -o /tmp/rct/t && (cd /tmp/rct/run && ../t)
 * Expect: "missed in: file 0 +0K 1x, field/land.narc +64K 2x, file 4 +320K 1x, field/sub/x.dat +64K 1x (+1 more)" */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/stat.h>
void PSPNativeMemLog(const char*f,...){va_list a;va_start(a,f);vprintf(f,a);va_end(a);puts("");}
unsigned long long P3DS_SpkRomTicks; unsigned P3DS_SpkRomBlocks;
int P3DS_SaveActive(void){return 0;} volatile int P3DS_LogWriting;
const char*P3DS_AreaName(int id){return id==3?"Jubilife City":"-";}
int P3DS_RomCacheRead(FILE*,uint32_t,uint32_t,void*,uint32_t); void P3DS_RomSpikeFiles(char*,unsigned); void P3DS_ZoneEnter(int); void P3DS_ZoneInfo(int*,int*,int*,unsigned*);
static uint8_t rom[0x200000];
static void w32(uint32_t o,uint32_t v){memcpy(rom+o,&v,4);} static void w16(uint32_t o,uint16_t v){memcpy(rom+o,&v,2);}
int main(void){
 /* FNT at 0x200: 3 dirs */
 uint32_t F=0x200; w32(F+0,24);w16(F+4,2);w16(F+6,3); w32(F+8,40);w16(F+12,3);w16(F+14,0xF000); w32(F+16,60);w16(F+20,5);w16(F+22,0xF001);
 uint8_t *p=rom+F+24; *p++=5;memcpy(p,"a.bin",5);p+=5; *p++=0x85;memcpy(p,"field",5);p+=5;*p++=0x01;*p++=0xF0; *p++=0;
 p=rom+F+40; *p++=9;memcpy(p,"land.narc",9);p+=9; *p++=0x83;memcpy(p,"sub",3);p+=3;*p++=0x02;*p++=0xF0; *p++=0;
 p=rom+F+60; *p++=5;memcpy(p,"x.dat",5);p+=5; *p++=0;
 uint32_t fntLen=80, FAT=0x400; w32(0x40,F);w32(0x44,fntLen);w32(0x48,FAT);w32(0x4C,6*8);
 uint32_t st[6]={0x1000,0x2000,0x10000,0x40000,0x100000,0x180000},en[6]={0x2000,0x3000,0x20000,0x100000,0x180000,0x1C0000};
 for(int i=0;i<6;i++){w32(FAT+i*8,st[i]);w32(FAT+i*8+4,en[i]);}
 mkdir("sdmc:",0777);mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/platinum3ds",0777);
 FILE*f=fopen("sdmc:/3ds/platinum3ds/platinum.nds","wb");fwrite(rom,1,sizeof rom,f);fclose(f);
 f=fopen("sdmc:/3ds/platinum3ds/platinum.nds","rb"); static uint8_t b[0x20000];
 P3DS_ZoneEnter(3);
 P3DS_RomCacheRead(f,sizeof rom,0x1100,b,16); P3DS_RomCacheRead(f,sizeof rom,0x50000,b,0x100); P3DS_RomCacheRead(f,sizeof rom,0x150000,b,0x100);
 P3DS_RomCacheRead(f,sizeof rom,0x190000,b,0x100); P3DS_RomCacheRead(f,sizeof rom,0x1F0000,b,0x100);P3DS_RomCacheRead(f,sizeof rom,0x60000,b,0x100);
 char o[200];P3DS_RomSpikeFiles(o,sizeof o);printf("[%s]\n",o);
 int c,pv,l;unsigned ms;P3DS_ZoneInfo(&c,&pv,&l,&ms);printf("zone %d prev %d learned %d\n",c,pv,l);
 P3DS_RomSpikeFiles(o,sizeof o);printf("after reset [%s]\n",o);
 return 0;}

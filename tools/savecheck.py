#!/usr/bin/env python3
"""Validate a Platinum (retail layout) save: per half (0x0/0x40000), normal block 0xCF2C and boxes block 0x121E4,
footer = last 20 bytes, CRC16-CCITT(init 0xFFFF) over block minus 20 bytes, magic 0x20060623."""
import struct,sys
T=[]
for i in range(256):
    c=i<<8
    for _ in range(8): c=((c<<1)^0x1021)&0xffff if c&0x8000 else (c<<1)&0xffff
    T.append(c)
def crc(d):
    c=0xffff
    for b in d: c=((c<<8)&0xffff)^T[((c>>8)^b)&0xff]
    return c
d=open(sys.argv[1],'rb').read()
for base in (0,0x40000):
    for name,off,size in (("normal",0,0xCF2C),("boxes",0xCF2C,0x121E4)):
        blk=d[base+off:base+off+size];f=blk[-20:]
        cnt,sz,mag=struct.unpack('<III',f[4:16]) if False else struct.unpack('<III',blk[-16:-4])
        ok=mag==0x20060623 and sz==size and crc(blk[:-20])==struct.unpack('<H',blk[-2:])[0]
        print(f"half {base:#x} {name}: size_field={sz:#x} magic={mag:#x} counter={cnt} {'OK' if ok else 'BAD'}")

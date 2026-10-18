import struct
def info3d(b,o):
    dummy,count,size=struct.unpack_from('<BBH',b,o)
    p=o+4+8+4*count
    esz,dsz=struct.unpack_from('<HH',b,p); p+=4
    ents=[b[p+i*esz:p+(i+1)*esz] for i in range(count)]; p+=esz*count
    names=[b[p+i*16:p+(i+1)*16].rstrip(b'\0').decode('latin1') for i in range(count)]
    return list(zip(names,ents))
def parse(b):
    assert b[:4]==b'BTX0'
    off=struct.unpack_from('<I',b,16)[0]; t=b[off:]
    assert t[:4]==b'TEX0'
    texInfo=struct.unpack_from('<H',t,0x0E)[0]; texData=struct.unpack_from('<I',t,0x14)[0]
    palSize=struct.unpack_from('<I',t,0x30)[0]<<3; palInfo=struct.unpack_from('<I',t,0x34)[0]; palData=struct.unpack_from('<I',t,0x38)[0]
    texs=[]
    for name,e in info3d(t,texInfo):
        o,par=struct.unpack_from('<HH',e,0); w=8<<((par>>4)&7); h=8<<((par>>7)&7); fmt=(par>>10)&7; c0=(par>>13)&1
        bpp={1:8,2:2,3:4,4:8,5:2,6:8,7:16}[fmt]; n=w*h*bpp//8
        texs.append(dict(name=name,w=w,h=h,fmt=fmt,c0=c0,data=t[texData+(o<<3):texData+(o<<3)+n]))
    pals=[]
    pl=info3d(t,palInfo)
    for i,(name,e) in enumerate(pl):
        o=struct.unpack_from('<H',e,0)[0]<<3
        end=palSize if i==len(pl)-1 else (struct.unpack_from('<H',pl[i+1][1],0)[0]<<3)
        # entries are sorted? take up to 16 colors
        pals.append(dict(name=name,data=t[palData+o:palData+o+min(32,max(8,end-o) if end>o else 32)]))
    return texs,pals

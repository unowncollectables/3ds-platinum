import struct,sys
from PIL import Image
b=open('/workspace/platinum3ds/platinum3ds/full/followers.bin','rb').read()
_,v,N,S=struct.unpack_from('<4sIII',b,0)
def sprite(i):
    o,w,h,f,_=struct.unpack_from('<IBBBB',b,16+S*4+i*8);return o,w,h,f
def img(sp,shiny=0):
    o,w,h,f=sprite(sp);fs=w*h//2;pal=struct.unpack_from('<16H',b,o+8*fs+32*shiny)
    im=Image.new('RGBA',(w*8,h),(40,120,40,255))
    for k in range(8):
        for j in range(w*h):
            ix=(b[o+k*fs+j//2]>>((j&1)*4))&15
            if ix:c=pal[ix];im.putpixel((k*w+j%w,j//w),((c&31)*8,((c>>5)&31)*8,((c>>10)&31)*8,255))
    return im
sp=[int(x) for x in sys.argv[1:]]
ims=[img(s).resize((img(s).width*3,img(s).height*3),Image.NEAREST) for s in sp]
W=max(i.width for i in ims);H=sum(i.height for i in ims);out=Image.new('RGBA',(W,H));y=0
for i in ims:out.paste(i,(0,y));y+=i.height
out.save('/tmp/fol/prev.png')

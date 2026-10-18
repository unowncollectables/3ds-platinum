# Insert pspoke's per-overlay data/sinit/bss output sections into devkitARM's 3dsx.ld before .tdata.
import sys
s=open('/opt/devkitpro/devkitARM/arm-none-eabi/lib/3dsx.ld').read()
ov=open(sys.argv[1]).read().split('SECTIONS {',1)[1].rsplit('}',1)[0]
i=s.index('\t.tdata : ALIGN(4)')
open(sys.argv[2],'w').write(s[:i]+ov+'\n'+s[i:])

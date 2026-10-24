#!/bin/sh
# Placeholder HOME Menu art, drawn here (original, no game assets): icon 48x48, banner 256x128, 1 s of silence.
set -e; o=$1; mkdir -p $o
convert -size 48x48 xc:'#3a3f6b' -fill '#c9c2e8' -draw 'circle 24,24 24,6' -fill '#3a3f6b' -draw 'circle 24,24 24,14' -fill white -pointsize 11 -gravity south -annotate +0+1 'P3DS' $o/icon.png
convert -size 256x128 gradient:'#20223e'-'#5b4f8f' -fill white -pointsize 26 -gravity center -annotate +0-10 'Platinum 3DS' -pointsize 12 -annotate +0+22 'native port' $o/banner.png
python3 -c "import wave;w=wave.open('$o/banner.wav','wb');w.setnchannels(1);w.setsampwidth(2);w.setframerate(22050);w.writeframes(b'\0\0'*22050);w.close()"

#!/bin/sh
# run 55: icon = shiny Origin Giratina front sprite, banner sound = Giratina cry (both extracted from the user's own ROM by assets/extract.py)
# One-time forwarder CIA: sets Luma hb:ldr target to sdmc:/3ds/platinum3ds/platinum3ds_full.3dsx and chainloads it.
set -e; cd "$(dirname "$0")"; export DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM
make; B=../tools/bt/linux-x86_64/bannertool
$B makebanner -i assets/banner256.png -a assets/giratina_cry.wav -o banner.bnr
$B makesmdh -s "Platinum 3DS" -l "Pokemon Platinum 3DS port (forwarder)" -p "local build" -i assets/icon48.png -o icon.icn
../tools/makerom -f cia -o platinum3ds_forwarder.cia -elf forwarder.elf -rsf forwarder.rsf -icon icon.icn -banner banner.bnr -exefslogo -target t

set -e
export PATH=/workspace/platinum3ds/toolchain-3ds/bin:/opt/devkitpro/devkitARM/bin:/opt/devkitpro/tools/bin:$PATH DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM
cd /workspace/platinum3ds/pspoke/.work/tree/test_out/native-audio-app/overlays && python3 rebuild_obj.py overlay005/fieldmap.c | grep -v "^$"
cd /workspace/platinum3ds/platinum3ds/full && rm -f platinum3ds_full.elf && make 2>&1 | grep -E "error|undefined|Error" || true
ls -la platinum3ds_full.3dsx

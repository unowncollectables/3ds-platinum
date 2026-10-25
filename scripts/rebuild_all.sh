#!/bin/bash
# Full pspoke -> 3DS rebuild from scratch (needed after toolchain flag changes, e.g. -fno-short-enums).
set -e
export PATH=/workspace/platinum3ds/toolchain-3ds/bin:$PATH PSPDEV=/workspace/platinum3ds/toolchain-3ds
cd /workspace/platinum3ds/pspoke
[ -d .work ] && { rm -rf .work.bak; mv .work .work.bak; }
bash -c 'source scripts/phases.sh; phase_base; fill_rom PLATINUM_ROM /workspace/platinum3ds/baserom/platinum_us_rev1.nds; phase_generated; phase_services; phase_libraries'
bash -c 'source scripts/phases.sh; O="$T/native-audio-app/overlays"; qol_header
  step ov-generate  bash -c "cd \"$O\" && python3 generate.py"
  step ov-build     bash -c "cd \"$O\" && python3 build.py"
  step qol-patch    bash -c "cd \"$O/source\" && patch -p1 -s < \"$ROOT/patches/platinum/qol-overlay-sources.patch\""
  step qol-rebuild  bash -c "cd \"$O\" && python3 rebuild_obj.py text.c applications/bag/main.c game_options.c item.c overlay006/repel_step_update.c pokemon.c move_table.c applications/pokemon_summary_screen/main.c battle_sub_menus/battle_party.c"
  step ov-internal  bash -c "cd \"$O\" && python3 internal.py && cp -f libplatinum-overlays.a libplatinum-overlays.a.base"'
./scripts/p3ds_game.sh
cd .work/tree/test_out/native-audio-app
APP=$(sed -n 's/^APPOBJS:= *//p' /workspace/platinum3ds/platinum3ds/full/Makefile)
make $APP
psp-objcopy --weaken-symbol=getlocalhost offline.o
make -C ../native-render-opt native_gpu.o GPU2D_Soft.o
make -C ../native-stack-render g3_handler.o frontend.o
# The final link is the builder's relink step, after work_patches are applied: they define
# symbols the platform code needs (P3DS_SpkRomTicks, __wrap_G3X_GetVectorMtx, ...), so linking here fails.
cd /workspace/platinum3ds/platinum3ds/full && rm -rf build/*.o platinum3ds_full.elf
echo REBUILD-ALL-DONE

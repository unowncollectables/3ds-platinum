CRUMBS ?= 0
COMMA := ,
OV5WRAP := $(if $(filter 1,$(CRUMBS)),-Wl$(COMMA)@ov5wrap.txt)
# platinum3ds full-game link: pspoke staged objects/archives (built by the psp-* -> devkitARM wrappers) + 3DS platform.
W      ?= /workspace/platinum3ds/pspoke/.work/tree/test_out
TC     := /workspace/platinum3ds/toolchain-3ds/bin
CC     := $(TC)/psp-gcc
CXX    := $(TC)/psp-g++
DKA    := /opt/devkitpro/devkitARM/bin/arm-none-eabi-
A      := $(W)/native-audio-app
DEFS   := -O2 -DSDK_PORT -DSDK_X86 -DSDK_TS -DSDK_4M -DSDK_FINALROM -DNNS_FINALROM -DSDK_VERSION_MAJOR=4 -DSDL_MAIN_HANDLED -DMELONDS_PSP -DPSP_NATIVE_RGBA_OUTPUT=1 -DPOKEPLATINUM_GENERATED_ENUM -ffunction-sections -fdata-sections
INC    := -I$(W)/native-probe/backing/include -I$(W)/native-graphics/alias-proof/include -I$(W)/native-probe/generated -I$(W)/native-graphics/libntr/include -I$(W)/../melon -I$(W)/native-render-opt -I$(A)/overlays/include
PLAT   := ../platform
POBJS  := build/main3ds.o build/logring.o build/psp_ctru.o build/sdl_ctru.o build/stubs3ds.o build/render3ds.o build/g3soft.o $(if $(filter 1,$(CRUMBS)),build/ov5crumbs.o) build/gpuscale.o build/blit_shbin.o build/g3gpu.o build/g3_shbin.o build/gpucomp.o build/romcache3ds.o build/p3ds_gamefont.o build/p3ds_foltalk.o
APPOBJS:= sas_out.o vram_dirty.o stackprobe.o ctrdg_absent.o search_disabled.o wm_absent.o fx_mtx_native.o cadence.o audio_bank.o audio_loader.o audio_out.o audio_backend.o audio_seq.o audio_exchannel.o audio_channel.o audio_engine.o mic_pm.o os_sync.o offline.o gx_dma.o frame.o owner_lock.o threads.o alarms.o overlay.o config.o platform.o input.o romfs.o backup.o backing.o graphics_registers.o trainer_ai.o mi_memory.o mi_dma.o memprobe.o power.o
RENDER := $(W)/native-render-opt/native_gpu.o $(W)/native-render-opt/GPU2D_Soft.o $(W)/native-stack-render/g3_handler.o $(W)/native-stack-render/frontend.o
LIBS   := -Wl,--start-group $(A)/overlays/libplatinum-overlays.a $(A)/overlays/libinternal-overlays.a $(W)/native-probe/libnetwork-probe.a $(A)/libsdk-filtered.a -Wl,--end-group
ARCH   := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
all: platinum3ds_full.3dsx
build/%.o: $(PLAT)/%.c
	@mkdir -p build; $(CC) $(DEFS) $(INC) -c $< -o $@
build/main3ds.o build/stubs3ds.o: DEFS += -DPLATINUM3DS_APP
build/%.o: $(PLAT)/%.S
	@mkdir -p build; /opt/devkitpro/devkitARM/bin/arm-none-eabi-as -march=armv6k -mfloat-abi=hard -mfpu=vfp $< -o $@
build/%.o: $(PLAT)/%.cpp
	@mkdir -p build; $(CXX) $(DEFS) $(INC) -std=gnu++17 -fno-exceptions -fno-rtti -fpermissive -c $< -o $@
build/sdl_ctru.o build/psp_ctru.o: INC :=
# libctru/citro3d ABI uses short enums; game-facing code uses int enums (pspoke/DS data layout).
build/sdl_ctru.o build/psp_ctru.o build/gpuscale.o build/gpucomp.o: DEFS += -fshort-enums
build/blit_shbin.h build/blit_shbin.s: $(PLAT)/shaders/blit.v.pica
	@mkdir -p build; /opt/devkitpro/tools/bin/picasso -o build/blit.shbin $< && cd build && /opt/devkitpro/tools/bin/bin2s -H blit_shbin.h blit.shbin > blit_shbin.s
build/blit_shbin.o: build/blit_shbin.s
	/opt/devkitpro/devkitARM/bin/arm-none-eabi-as $< -o $@
build/gpuscale.o: $(PLAT)/gpuscale.c build/blit_shbin.h
	$(CC) $(DEFS) -Ibuild -I/opt/devkitpro/libctru/include -c $< -o $@
build/platinum3ds.ld: $(A)/overlays/overlays.ld
	python3 ../full/mkld.py $< $@
platinum3ds_full.elf: $(POBJS) build/platinum3ds.ld build/p3ds.specs
	$(DKA)gcc -specs=build/p3ds.specs -Lbuild -B/opt/devkitpro/devkitARM/arm-none-eabi/lib/ $(ARCH) -Wl,--gc-sections $(shell cat $(A)/overlays/constructors.flags) $(OV5WRAP) -Wl,--wrap=abort,--wrap=NNS_SndArcPlayerStartSeq,--wrap=NNS_SndArcPlayerStartSeqEx,--wrap=NNS_SndArcPlayerStartSeqArc,--wrap=NNS_SndArcPlayerStartSeqArcEx,--wrap=Easy3D_UploadTextureToVRAM,--wrap=NNS_G3dResDefaultSetup,--wrap=TextureResource_UploadToVRam,--wrap=NNS_GfdDoVramTransfer,--wrap=VramTransfer_Request,--wrap=Heap_Alloc,--wrap=Heap_AllocAtEnd,--wrap=Heap_Free,--wrap=Heap_FreeExplicit,--wrap=VramTransfer_Process,--wrap=NNS_G3dTexLoad,--wrap=NNS_G3dPlttLoad,--wrap=CommManager_IsInitialized,--wrap=G3X_GetBoxTestResult,--wrap=OS_WaitVBlankIntr,--wrap=OS_WaitIrq,--wrap=MI_WaitDma,--wrap=printf,--wrap=puts,--wrap=PSPNativeFatal -Wl,--wrap=G3X_GetClipMtx -Wl,--wrap=G3X_GetVectorMtx,--wrap=ApplicationManager_New,--wrap=CTRDG_Init,--wrap=CommManager_InitializeSearchParty,--wrap=CommManager_GetAvailableConnections -Wl,--no-enum-size-warning -Wl,-Map,build/full.map \
	  $(POBJS) $(addprefix $(A)/,$(APPOBJS)) $(RENDER) $(LIBS) -L/opt/devkitpro/libctru/lib -lcitro3d -lctru -lstdc++ -lm -o $@
platinum3ds_full.smdh:
	/opt/devkitpro/tools/bin/smdhtool --create "Platinum 3DS (dev)" "personal port, step 2" "local" /opt/devkitpro/libctru/default_icon.png $@
platinum3ds_full.3dsx: platinum3ds_full.elf platinum3ds_full.smdh
	/opt/devkitpro/tools/bin/3dsxtool $< $@ --smdh=platinum3ds_full.smdh
clean:
	rm -rf build platinum3ds_full.*
build/p3ds.specs:
	@mkdir -p build; sed 's#-T 3dsx.ld%s#-T $(CURDIR)/build/platinum3ds.ld#' /opt/devkitpro/devkitARM/arm-none-eabi/lib/3dsx.specs > $@

cia: platinum3ds_full.elf
	../../tools/makerom -f cia -o platinum3ds_full.cia -elf platinum3ds_full.elf -rsf cia/game.rsf -icon cia/icon.icn -banner cia/banner.bnr -exefslogo -target t
build/g3soft.o build/render3ds.o build/gpuscale.o build/g3gpu.o build/gpucomp.o: DEFS += -O3 -marm

build/g3_shbin.h build/g3_shbin.s: $(PLAT)/shaders/g3.v.pica
	@mkdir -p build; /opt/devkitpro/tools/bin/picasso -o build/g3.shbin $< && cd build && /opt/devkitpro/tools/bin/bin2s -H g3_shbin.h g3.shbin > g3_shbin.s
build/g3_shbin.o: build/g3_shbin.s
	/opt/devkitpro/devkitARM/bin/arm-none-eabi-as $< -o $@
build/gpucomp.o: $(PLAT)/gpucomp.c build/blit_shbin.h
	$(CC) $(DEFS) -fshort-enums -Ibuild -I/opt/devkitpro/libctru/include -c $< -o $@
build/g3gpu.o: $(PLAT)/g3gpu.c build/g3_shbin.h
	$(CC) $(DEFS) -fshort-enums -Ibuild -I/opt/devkitpro/libctru/include -c $< -o $@

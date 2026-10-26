# platinum3ds progress
## Done
- Step 1: platinum3ds/platinum3ds.3dsx (heap.c + narc.c + ROM FS). Hardware-confirmed on New 3DS.
  Build: `cd platinum3ds && DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM make`
- devkitPro installed (/opt/devkitpro, 3ds-dev, 3ds-sdl unused). /etc/mtab -> /proc/self/mounts. rsync installed.
## Step 2a: pspoke pipeline on devkitARM (in progress)
Approach: pretend-PSPDEV at toolchain-3ds/: bin/psp-gcc wraps arm-none-eabi-gcc (armv6k hard-float, drops -G0,
adds -I toolchain-3ds/pspshim); psp-ar/ld/as/... symlink to arm-none-eabi-*; psp-config + pspsdk/lib/build.mak
stand-in (object rules only). pspshim/ = PSP kernel/audio/sas/utility + SDL2 subset headers;
implementations in platinum3ds/platform/psp_ctru.c (SDL impl TODO: sdl_ctru.c).
pspoke edits live on branch `platinum3ds` in pspoke/ (local git, never pushed): .type %object, MIPS `sync`/`$sp` asm.
Env for every phase:
  export PATH=/workspace/platinum3ds/toolchain-3ds/bin:$PATH PSPDEV=/workspace/platinum3ds/toolchain-3ds
  cd pspoke && bash -c 'source scripts/phases.sh; phase_base; fill_rom PLATINUM_ROM /workspace/platinum3ds/baserom/platinum_us_rev1.nds; phase_generated; phase_services; phase_libraries'
  (phases are stamped in .work/tree/.stamp-*; delete stamp to rerun; logs in .work/tree/logs)
Status: generated OK, services OK, libraries: sdk OK, network was failing on errno (fixed via -include errno.h).
## 2a RESULT: FULL GAME LINKS -> platinum3ds/full/platinum3ds_full.3dsx (5.2 MB text, 20 MB bss)
Rebuild from scratch:
  export PATH=/workspace/platinum3ds/toolchain-3ds/bin:$PATH PSPDEV=/workspace/platinum3ds/toolchain-3ds
  cd pspoke && rm -rf .work && bash -c 'source scripts/phases.sh; phase_base; fill_rom PLATINUM_ROM /workspace/platinum3ds/baserom/platinum_us_rev1.nds; phase_generated; phase_services; phase_libraries'
  (re-apply: staged tree is rsynced from pspoke/port on branch platinum3ds, so edits persist)
  ./scripts/p3ds_game.sh   # NOTE: first run needs ov-generate/ov-build/qol-patch/qol-rebuild/ov-internal lines (see platinum.sh); trimmed after first run
  cd .work/tree/test_out/native-audio-app && make <each APPOBJS .o>  ; objcopy --weaken-symbol=getlocalhost offline.o
  make -C ../native-render-opt native_gpu.o GPU2D_Soft.o ; make -C ../native-stack-render g3_handler.o frontend.o
  cd /workspace/platinum3ds/platinum3ds/full && make
3DS platform files: platinum3ds/platform/{main3ds.c,psp_ctru.c,sdl_ctru.c,stubs3ds.c,render3ds.cpp (melonDS soft 2D -> fb),g3null.cpp (3D dropped)}
Needs on SD: sdmc:/3ds/platinum3ds/{platinum.nds, platinum.sav (512 KiB)}; log -> log.txt
## HW run 1 (13:05): aborted after audio init. Cause: missing pspoke link wraps (--wrap=CTRDG_Init etc., from
native-audio-app/Makefile line 160) -> real SDK CTRDG_Init -> PXI callback abort. Fixed in full/Makefile.
Also: log name is now log.txt (memprobe.o rebuilt), printf/puts wrapped to the log, abort() logs its caller address,
bottom screen = status checklist + red error box, exit via svcExitProcess (no hang), aptHook ONEXIT.
## HW run 2 (13:11): all boot steps OK, then abort in MIi_NativeRangeOk (services/mi_dma.c) - PSP-only address
windows. Now accepts 3DS image/heap 0x00100000-0x10000000, linear 0x14000000-0x1C000000 and 0x30000000-0x40000000,
VRAM 0x1F000000-0x1F600000. Error box now shows abort caller + last printf/PSPNativeFatal message (bad addr/size/ra).
Swept port for other PSP address windows: none other live (render-gu2d pspDebugScreen 0x44000000 not linked;
mi_dma DMA0 source check is OS_Panic in debug-only path).
## HW run 3 (13:17): SUCCESS - boots, intro plays, Rowan + text boxes correct on top, buttons work.
Then: bottom screen switches to DS sub screen on first presented game frame (centred +32,+24); hidTouchRead mapped
to DS coords (outside 256x192 ignored); PSP stylus-mode R toggle disabled on 3DS (R forwarded as DS R);
SELECT+START held ~1 s toggles the checklist; errors re-show checklist + red box.
## Exit fix (13:20): HOME->close / START-on-error now: p3ds_quit flag; all PSP-shim blocking points (sleep, delay in
10 ms slices, sema wait, audio output, alarm loop, SDL sem/timers) exit worker threads; shutdown wakes sleepers,
joins every thread (300 ms timeout), frees partition blocks, closes save, aptUnhook, gfxExit, exit() normally.
Alarm + SDL timer threads are now joinable (not detached).
## HW run 4 (13:25): crash entering naming screen: pspoke swaps gNamingScreenAppTemplate for a PSP-OSK app; our
stub returned a NULL template. Fix: memprobe.c defines PSP_NATIVE_KEEP_DS_NAMING on PLATINUM3DS (real DS naming
screen), stub returns the real template as fallback. Only PSP-utility substitution in the port (osk/naming_osk);
saving is pspoke's file-backed backup.c (sdmc platinum.sav), no sceUtility savedata. heap_free=78 KB is a
per-game-heap figure (fixed HEAP_SIZE_* templates, x2 under SDK_PORT), same as PSP; arena is 6 of 8 MiB backing.
## Bleed-through fix (13:27): on switch to game view, console retargeted to an off-screen buffer, both bottom
framebuffers (incl. border) cleared, double buffering on; blit each frame into the current back buffer.
## HW run 5 (13:35): title 2D + DS naming OK; hard freeze entering overworld after overlay 7 load.
Likely cause (shim): PSP thread priorities mapped ABOVE the 3DS game thread (0x19..0x28 vs main 0x30) and
sceKernelRotateThreadReadyQueue = svcSleepThread(0), which never runs lower-priority threads -> a yielding/spinning
worker starves the game thread forever (no exception, HOME dead). Fix: PSP 0x20 -> 0x30, workers at/below game
thread (clamp 0x28..0x3F), yield = 50 us sleep. Added 3 s no-frame watchdog thread (prio 0x20) -> log + red box.
Log is open/append/close per line (already flushed to SD each line). Heap 40 KB is game-heap figure (unchanged).
## Forwarder CIA (13:41): forwarder/platinum3ds_forwarder.cia (forwarder/build.sh; tools/ = makerom 0.19 + bannertool 1.2
binaries, not committed). Uses Luma hb:ldr SetTarget/SetArgv then aptSetChainloader(0x000400000D921E00) - needs the
Homebrew Launcher CIA (that TID, Luma default hbldr_3dsx_tid) installed. UniqueId 0xF7D3D, original "PLAT 3DS" art.
## 3D milestone 1 (13:44): platform/g3soft.cpp replaces g3null.cpp. Software rasterizer of pspoke's screen-space
GuVertex triangles into GPU3D::NativeFrame (6-bit RGB + 5-bit alpha, composited by melonDS 2D as BG0), 16-bit depth
(LESS / EQUAL from polygon attr), DS CLEAR_COLOR/CLEAR_DEPTH, textures decoded from emulated tex/palette VRAM
(PLTT4/16/256, A3I5, A5I3, DIRECT, 4x4 compressed), color0 transparency, repeat/flip/clamp, vertex-colour modulate,
translucent batches deferred to end of frame (DS order). Not yet: per-face culling (only FRONT_AND_BACK), perspective-
correct UVs, toon/shadow/fog/edge, GPU (citro3d) path, stereo. Decision: software path first because the 3D layer
must be composited by the DS 2D engine (priorities/blending); citro3d version would render + read back.
## Next
- boot test (hardware/emulator); fix crashes; then citro3d or software G3 backend in g3null.cpp DrawVertices

## Forwarder v2 (13:47)
v1 crashed Luma loader (panic in hbldrLoadProcess for 0D921E00). v2: forwarder ExHeader back to legacy 64MB/268MHz (likely cause: chainloading from a 124MB-mode app). Fallback: full game as its own CIA (platinum3ds/full/platinum3ds_full.cia, `make cia`, UniqueId 0xF7D3E), absolute sdmc paths, no loader dependency.

## Run 6 follow-up (14:00)
- Field-map stall: suspect = gfx_box_test.c spins `while (G3X_GetBoxTestResult()!=READY)`. Now --wrap: never reports busy (crumb "boxtest-busy"). Also crumbs on OS_WaitVBlankIntr/OS_WaitIrq/MI_WaitDma, PSP/SDL waits, lock contention, g3 begin/flush. Watchdog 10 s, dumps [CRUMB] ring (tag, caller addr -> addr2line on build/../platinum3ds_full.elf, thread, frame) to log.txt. [G3SOFT] avg us/frame every 120 frames.
- Full CIA RSF rebuilt from Steveice10 buildtools template.rsf (Legacy 64MB, 804MHz, L2, stack 0x40000, no extra services).
- QUEUED: scaling modes (1:1 / 1.25x default / top stretch), L+R+SELECT cycle, saved to sdmc config, touch mapping per mode.

## Scaling modes (14:01)
render3ds.cpp blitScaled (bilinear, per-axis tables). Modes: 0=1:1, 1=1.25x both screens 320x240 (default; top pillarboxed 40px), 2=bottom 1.25x + top 400x240 stretch. L+R held + SELECT cycles; saved in sdmc:/3ds/platinum3ds/config.txt (scale=N). Touch: mode0 offset 32/24, else px*256/320, py*192/240. Borders cleared on mode change (both buffers).

## Run 7 follow-up (14:12)
- Run-7 crumbs (waitdma in GX_EndLoadTex/EndLoadTexPltt) = NNS_GfdDoVramTransfer from the VBlank thread (animated field textures, normal) flooding the single ring. Now 2 rings: MAIN thread vs others; watchdog shows main-thread last crumb. Heap_Alloc failures logged via wrapped CommManager_IsInitialized ([HEAP] ... FAILED).
- Scaling removed: 1:1 only, config.txt ignored. Planned: GPU (citro3d) sharp scaling after stall+forwarder.
- Full CIA shelved (user).
- Forwarder v3 = diagnostic: logs each step to sdmc:/3ds/platinum3ds/forwarder_log.txt; hold Y at launch = chainload HBL CIA without setting hb:ldr target (isolates whether chainload itself crashes Luma loader).

## Run 8 follow-up (14:23)
- Stall: main thread loops in GX_EndLoadTex/Pltt. Static callers: Easy3D_UploadTextureToVRAM, NNS_G3dResDefaultSetup, TextureResource_UploadToVRam, NNS_G3dTex/PlttLoad, NNS_G2dLoad*, SPLManager_*, NNS_GfdDoVramTransfer. All wrapped with crumbs (+ [TEX] log every 256 calls w/ pointer+caller) -> next run names the looping caller.
- Error box: SELECT = power off (PTMSYSM_ShutdownAsync, fallback APT_HardwareResetAsync).
- Forwarder v4: RSF = buildtools template (no custom caps). Pipeline test: forwarder/p3ds_hello_test.cia (prints OK, writes hello_cia.txt). Old RSF kept as forwarder_old.rsf.

## Run 9 follow-up (14:33)
- Forwarder v4 WORKS on hardware (template RSF). Done.
- Stall is after AreaDataManager_Load (fieldmap.c FIELD_MAP_INIT_STATE_LOAD): next steps MapPropManager_New, SecretBase_LoadProps, FieldSystem_InitLandManager, ov5_021D1878/1968, weather, FieldBGM, init script, HBlankSystem_New/Start.
- Diagnostic: platform/ov5crumbs.S (generated) = crumb trampolines for all 1443 cross-object functions called by overlay005 (excl. FX/MTX/VEC/MI/libc); full/ov5wrap.txt holds the --wrap list. Crumb rings 64 deep. Remove both (Makefile POBJS + -Wl,@ov5wrap.txt) once found.

## GPU scaling (branch gpu-scale, 14:35)
platform/gpuscale.c + shaders/blit.v.pica (citro3d). CPU 2x nearest prescale (512x384 RGBA8 linear) -> GX tiled texture -> LINEAR-filtered quad = sharp bilinear. Mode 1 (default): bottom 320x240, top 320x240 pillarboxed; mode 2: top 400x240; mode 0: CPU 1:1. L+R+SELECT cycles, saved as gpuscale=N in config.txt (old scale= ignored). Touch: modes 1/2 scale x*256/320, y*192/240. GPU init failure falls back to 1:1. Untested on hardware.

## GPU scaling v2 (14:41)
No CPU prescale: 256x192 -> 256x256 RGBA8 texture, NEAREST, t = 0 (top) .. 0.75 (bottom) per hardware run (v1 was upside-down/offset). Merged into master.

## ROOT CAUSE of field-map stall (14:58)
arm-none-eabi defaults to SHORT enums (Tag_ABI_enum_size small); DS/PSP data layouts use 4-byte enums. SpriteResourceTableEntryNARC.narcID (enum) became 1 byte -> `!= 0xFFFFFFFE` always true -> GCC emitted `b .` in SpriteResourceTable_LoadFromBinary. Same mismatch affects every enum field in ROM-data structs. Fix: -fno-short-enums in toolchain-3ds/bin/psp-gcc/psp-g++ (all game/pspoke code); libctru-facing files (psp_ctru.c, sdl_ctru.c, gpuscale.c) keep -fshort-enums; link with --no-enum-size-warning. Full rebuild: ./scripts_rebuild_all.sh (log rebuild_all.log, ~6 min). Verified: all 1630 objects enum_size int; LoadFromBinary loop now steps 24-byte entries.
Static scan for compiler-proved infinite loops (`b .`): only save-path traps after CARD async (pspoke backup callback is synchronous -> fine), network/error handlers, libc. 
Also: wrapper adds -include errno.h only for C/C++ (not .S).
## Sharp bilinear (in same build): pass 1 nearest 2x into 512x512 VRAM render-target texture, pass 2 linear to screen. Orientation of the render-to-texture pass untested.

## Stereo (branch stereo, 14:58) -> platinum3ds/full/platinum3ds_stereo_test.3dsx
Slider > 0 (GPU modes 1/2, engine A on top): 3D layer (NativeFrame) shifted per eye by d = round(slider*4) DS px (left eye -d, right +d) and engine A re-composited for the right eye -> 3D scene behind the screen, 2D at screen depth. gfxSet3D toggled only when stereo state changes; slider 0 = no extra work. Right eye: tex/up[2] + tgtTopR (GFX_RIGHT). Bottom untouched. Next: per-vertex depth parallax in g3soft.

## 15:21 single build: platinum3ds/full/platinum3ds_full.3dsx (master; stereo merged)
Stereo now per-vertex: g3soft rasterizes each triangle per eye (x += eye*sd*z/65535, sd = slider*6 DS px) into NativeFrame (left) + frameR/depthR (right); render3ds re-composites engine A with the right-eye 3D layer. Flat-layer shift kept as fallback path in render3ds (used if g3soft right eye unavailable). Slider 0 = mono, no extra cost. Note: screen-space z is non-linear, so depth separation is mostly far-vs-very-near.

## 16:01 build (run 11 follow-up)
- Black field: fixes tried: 4-colour palette base (libntr already gives bytes; removed >>1); far-plane z clamped to 65534 so it passes a max clear depth. Diagnostics: [G3DIAG] every 120 frames (pixel reject reasons, z/x/alpha ranges, tex formats, clear regs, vertex colour OR/AND), [3DLAYER] (DISPCNT, POWCNT, 3D px with alpha, BG0CNT, BLDCNT), L+R+X dumps dump_3dlayer.bmp / dump_engineA.bmp / dump_engineB.bmp to sdmc:/3ds/platinum3ds/.
- Mode 2 = aspect fill-and-crop (top 400x300, 30 px cropped top/bottom).
- FPS overlay top-left (L+R+START toggle, default on); [PERF] line each second: fps, ms split game/g3soft/composite/present, tris, malloc used, mode, slider.
- Audio: real SAS mixer (16 PCM voices, pitch/vol/loop) + ndsp output (8 x grain wavebufs). Logs [AUDIO] ndsp ready / ndspInit failed -> needs sdmc:/3ds/dspfirm.cdc. AUDIO-SAS grain/key spam hidden unless verbose=1 in config.txt.
- Stereo depth: parallax = sd*(0.3 + 0.7*z^8), sd = slider*6.

## Run 12 fixes (16:13 build)
- Black field: G3DIAG showed vertex x -4..4 / z -3..5 => field uses pspoke RAW mode (model-space verts + clip matrix, normalised UVs) which g3soft ignored. Implemented PSPNativeRawMatrix/RawMode: transform by DS clip matrix, near/w clip, UV*texsize, then normal raster.
- "Audio thread malloc/free spin" = watchdog's own log writes (fopen/fclose) - not a deadlock. Main stalled right after g3-begin; likely GPU stall in mode 2 (offscreen quad) -> mode 2 removed (any aspect fill >1.25x crops text boxes). Modes: 0=1:1, 1=1.25x pillarbox.
- Silent audio: pspoke SAS volumes peak ~16/4096 => mixer was ~-48 dB. Gain +36 dB; [AUDIO] 10s peak/voices log.
- 30 fps cap: C3D_FrameBegin(SYNCDRAW) added a 2nd vblank wait. Now FrameBegin(0).

## Run 13 fixes (16:21 build)
- Field renders. PERF: g3soft 35 ms of 53 ms (453 tris). Rasterizer rewritten: plane-equation attribute stepping, exact per-row span from edge functions (no bbox pixel tests), no divides in loop; g3soft.o built -O3 -marm.
- ov5 crumb wrappers (1443 fns) now off by default (make CRUMBS=1 to re-enable); watchdog + main wrappers kept.
- Audio: gain >>9 (run13 peak ~200k at >>6) + soft clip above 24576; threads named *audio*/*sas* run on core 2 (N3DS), fallback default core.
- Next if still slow: pipeline 2D compositor (11 ms) onto core 2 / split g3soft bands across cores.

## Run 14 analysis + fixes (16:34 build)
- PERF: bedroom stereo 1070 tris: g3soft 44 ms, composite 20 ms (right-eye recomposite). Outdoors slider 0, 739 tris: g3soft 75 ms -> not raster-bound: outdoors PSPNativeVramGen changes every frame, invalidating EVERY cached texture -> full re-decode each frame.
  Fix: cache keeps raw source bytes (texels+idx+palette); on gen change re-gather (memcpy) and memcmp, only re-decode if changed. [G3DIAG] now logs "decodes N" per 120 frames.
- Audio "slow": pspoke's SAS thread follows game-produced sound snapshots and holds the clock when the game is behind -> tempo tracks game FPS (10-14 fps). Not a mixer bug; fixes with FPS. Gain x1.5.
- NOT done: citro3d PICA200 G3 backend (multi-chunk job), per-eye raster on core 2 (needs tex-cache lifetime safety).

## GPU 3D chunk 1 (16:52 build)  — fallback: tag working-1634 / backups/platinum3ds_full_1634.3dsx
- platform/g3gpu.c + shaders/g3.v.pica: PICA200 rasterizer. g3soft keeps CPU transform/near-clip/stereo shift/tex decode;
  TriEye() forwards triangles to G3G_Tri (per-eye batches by tex/wrap/blend/depth state). G3G_End draws into 256x256 VRAM
  targets (D24S8), reads back with display transfer, converts into NativeFrame (6-bit RGB/5-bit A) for melonDS compositor.
- Depth: z_clip=-d, DepthMap(-1,0) -> LEQUAL (EQUAL for DS depth-equal). Alpha test >3. Translucent: SRC_ALPHA blend, alpha MAX.
- Textures: decoded RGBA -> CPU Morton swizzle into C3D_Tex (linear mem), cached on Tex entry, deletes deferred to frame end.
- config.txt: gpu3d=0 -> pure g3soft. Orientation knobs if wrong on HW: gpu3d_texflip=0 (textures upside down), gpu3d_flipy=1 (3D layer upside down).
- Log: "[GPU3D] gpu3d=..." at start; [G3DIAG] begins "gpu3d 1 <us>us" (GPU draw+readback time).

## GPU 3D chunk 2 (17:00 build, tag gpu3d-chunk2)
- Raw (field) path in GPU mode now sends DS CLIP-space verts (x,y,z,w) to the GPU: perspective-correct texturing + HW clipping
  (was CPU divide -> affine, likely the warped/scrambled building textures). Translucent raw batches drawn in a late pass.
- Trivial reject (all verts outside one clip plane / screen bbox) before queuing; tris counter now counts kept tris only (run16 4455 counted culled ones).
- L+R+Y toggles gpu3d<->g3soft live for A/B comparison. [GPU3D] frame stats line every 120 frames.

## GPU 3D chunk 3 (17:08 build, tag gpu3d-chunk3)
- Run 17: GPU view showed far geometry winning (walls hidden, roofs/backs, "top-down" look, different roof colour) -> depth
  treated as inverted: G3G_ZInv=1 default maps depth = 1-d. Live toggle L+R+A (logged), config gpu3d_zinv=0/1.
- Billboards upside down -> texture t now = v (gpu3d_texflip=0 restores old 1-v). Morton swizzle checked vs sf2d layout (matches).

## Chunk 4 (17:16 build, tag gpu3d-chunk4)
- Run 18: original depth mapping correct -> G3G_ZInv default 0 (L+R+A still toggles). texflip change kept.
- Numbers (Twinleaf, slider 0): GPU 33.3 ms/29 fps = game 0.9 + g3soft 17 (of which GPU draw+readback 3.3) + composite 11.2 + present 4.2; CPU mode 42.7 ms/23 fps (g3soft 25.4).
  Slider 1 GPU: 44-45 ms (composite 21 ms: engine A composited twice).
- Engine B (sub screen) now composites on a core-2 worker in parallel with engine A (separate SoftRenderer; GPU2D_Soft uses its line arg instead of shared GPU::VCount — patched in port/ and .work staged copy). config async2d=0 = inline.
- Remaining g3soft ~13 ms is pspoke's G3 frontend (matrix/vertex processing in g3_handler), not the raster.

## Chunk 5 (17:26 build, tag chunk5)
- Stereo: right-eye top-screen composite now runs on the core-2 worker (after engine B) using a memcpy'd engineA snapshot,
  its own SoftRenderer and a __thread 3D-layer source (P3DS_3DSrc in native_gpu.cpp GetLine). Main does left eye in parallel.
  config stereo2d_par=0 = old sequential path.
- pspoke g3_handler.o / frontend.o rebuilt at -O3 in .work (staged only; O2 copies saved as *.o.O2).
- Audio: sound pumps were already wall-clock paced (frame.c PSPNativeSoundAdvance). Slowness = SAS player starting only
  10 pumps (52 ms) behind; at 25 fps a frame is ~8 pumps, bursty -> queue runs dry -> clock held -> slow/choppy.
  audio_realtime=1 (default): 24 pumps (125 ms) latency; 0 = old 10. ndsp ring 16 x 256 samples (93 ms).
- NOTE: pspoke changes are in pspoke repo (port/) + patched staged copies in .work; a full pipeline rebuild keeps port/ ones.

## Chunk 6 (17:36 build, tag retail-save)
- Retail save incompatibility: GCC EABI aligns u64/s64 to 8, DS mwcc to 4. Port sizes: SystemData 104 (retail 92),
  Daycare 488 (480), PCBoxes 74192 (74184) -> normal block 0xCF40 vs 0xCF2C, boxes 0x121EC vs 0x121E4 -> footer size check fails.
  Fix: u64_ds/s64_ds (aligned(4) typedefs) for PokemonDataBlockC.ribbonsDS2, GameTime timestamps, SystemData.rtcOffset.
  All 38 save-table entry sizes now equal retail (verified vs retail ARM9 save table). Overlays rebuilt (build.py, 1 min).
  Patch: patches/retail-save-u64-align.patch (staged tree only - reapply after a pipeline re-stage!).
- tools_savecheck.py validates retail layout (footer 20 B, CRC16-CCITT init FFFF). twilight save: all 4 blocks OK.
- Saves written by older port builds (0xCF40 layout) are NOT loadable now (would need a converter).

## Run 21 fix (poketch-fix, 17:45 build)
- Crash: Poketch BerrySearcher (app 7) init pointer NULL -> PC=0. Root cause: link dropped 31 of 34 overlay
  static-init constructors (PSPNativeCtor_N were never pulled from libplatinum-overlays.a; constructors.flags
  was not passed). NitroStaticInit for those overlays never ran. Fixed in full/Makefile: all 34 now linked.
  Not weather-related.
- Added a libctru exception handler (main thread): logs [CRASH] pc/lr/sp/far + crumbs to log.txt, shows a message, and exits cleanly.
## Known bug (todo)
- Title screen: Giratina 3D model partly renders on the GPU3D path (title uses regular NNS model path, not raw field).
  Suspects: culling/winding, texture/palette on non-raw path. Check with L+R+Y (CPU toggle) to compare.

## Run 22 (tear-fix, build after poketch-fix)
- Run 22: real save loads/plays in Pastoria, ~28 fps.
- Tearing: GPU present used C3D_FrameBegin(0); the frame's display transfer could land on the buffer still on screen.
  Now C3D_FRAME_SYNCDRAW, config `vsync=1` (default; vsync=0 restores the old behaviour). At ~33 ms frames the cost should be small;
  if fps drops to 20, set vsync=0.
- Giratina partial in BOTH renderers -> shared pspoke geometry front end (g3_handler/frontend), not GPU3D.
  Checked: matrix stack push/pop/store/restore, list cache (2048), no poly cap found (~990 tris drawn). Still open.
  Next: dump per-list stats ([G3-STATS]) on title, compare against the vertex/strip decode for quad-strip and 4-vertex commands.

## Run 23 -> texgen-fix (18:03 build)
- Giratina body missing in both renderers: pspoke g3_handler ignored TEXGEN mode 2 (normal-source texcoords, env mapping).
  Implemented ST = N*TexMtx/2 + ST0 in G3SIM_Normal (patches/g3-texgen-normal.patch, applied to port/ and staged; g3_handler.o rebuilt -O3).
- Scale toggle: CPU 1:1 present never cleared the 1.25x image left by the GPU path (clear was keyed on an always-0 arg). Now keyed on scale mode, both buffers cleared.
  Toggle now logs "[3DS] scale mode -> N". Mode 1 = 360x240 (4:3, full height, no stretch).
- Map "black spots": added [GPU3D] tex alloc FAIL logging; no root cause yet. vsync=1 kept (present ~9 ms vs 4, fps still 28-29).

## Run 24 -> run24-fix (18:13 build)
- FPS: vsync=1 locked to 30 (work ~21 ms + wait = 33.9 ms frames; present 9-12 ms was mostly the wait). Default now vsync=0
  (expect ~35-40 fps in Pastoria). The edge "tearing" persisted with vsync on, so it is not present-related.
- Giratina: shadow polygons (polygon mode 3) were drawn as solid geometry (pspoke ignored polygon mode). Now skipped.
  TEXGEN mode 3 (vertex source) added; list cache now keys texgen + texture matrix for NORMAL/VTX.
  [G3MAT] line every 300 frames: per-frame texgen/format/polygon-mode histogram.
- Audio: sequencer is already wall-clock (frame.c real elapsed us, 1 SeqMain per 5.21 ms, max 10 pumps/frame); pitch math
  (16756991/period vs 44100 SAS, ndsp 44100) checked consistent. Added [AUDIO-CLK] every ~2 s: wall ms per 1997 ms of audio,
  pumps consumed (expect ~383), underfeed holds. Need run 25 log to locate the slowdown.
- Not done: the 19 ms pspoke 3D setup profiling; real back-face culling (g3soft never culls front/back, only cull-all) is a likely big win, next chunk.

## Run 25 -> run25-fix (18:27 build)
- MUSIC: frame.c fed PSPNativeSoundAdvance with (now - last), where `last` is stamped after render+present, so only the
  game-logic slice of each frame (~15 ms of 33) counted -> ~45% pump rate. Now uses the full frame period (prevSnd).
  patches/audio-frame-period.patch. Expect [AUDIO-CLK] ~383 pumps / 2 s, underfeed ~0.
- FPS lock 33.3 ms with vsync=0: the game itself paces on DS vblank (OS_WaitVBlankIntr, 2 vblanks/frame = 30 fps,
  same as real hardware in the overworld). 3D savings give headroom (stereo 19 fps), not >30 fps.
- Back-face culling on GPU path: config cull3d=0/1/2 (off / normal / inverted winding). Default off until verified.
- g3cache=0 disables the display-list cache (Giratina test).
- Tearing (both screens, 2D too): texture uploads ran before C3D_FrameBegin and raced the previous frame's queued GPU
  work (sync transfer wait could end on the previous job's PPF event). Uploads now after FrameBegin.

## Run 26 -> run26-fix (18:35 build)
- cull3d=1 black field: wrong winding sign (36 tris survived). Sign flipped; cull3d=1 should now be the correct one, cull3d=2 the inverse. Default still 0.
- Music: [AUDIO-CLK] 376/383 pumps (-1.8%), 8-9 underfeed holds/2 s, backlog 4-7 (latency margin lost after the first stall:
  a hold pinned playback 1 pump behind the head). Hold now re-opens the full 24-pump margin. patches/audio-underfeed-margin.patch.
- GPU-only black spots: new default gpu3d_cpuclip=1 = CPU clip/divide (same as the clean CPU path) + GPU raster.
  Suspect PICA clipping of tris crossing w~0 / near plane on the clip-space path. gpu3d_cpuclip=0 restores it.
- Tex lifetime checked: frees are deferred until after the readback (GPU done); cache key includes VRAM gen + raw-byte compare. Not the cause.

## Run 27 -> run27-fix (18:42 build)
- Spots gone with CPU clip. Now hybrid: only tris with a vertex at w<1e-3, outside near/far, or beyond a 2x guard band
  are CPU-clipped; the rest go down the fast clip-space path. Log: "[GPU3D] cpu-clipped tris N, culled M".
  Stereo was 15 fps (g3soft 40 ms) with full CPU clip; expect back near the previous ~19 fps or better.
- Giratina: G3X_GetVectorMtx read VECMTX_RESULT, which the simulator never wrote -> zero normal matrix for NNS
  envelope (weighted-skin) joints -> unlit/black skinned body. Wrapped (--wrap=G3X_GetVectorMtx) to return the live
  vector matrix. patches/g3-vecmtx-read.patch.
- Audio: [AUDIO-CLK] 383-384 / 383 pumps, 0-1 underfeed holds. Fixed.
- Perf todo (not done): overlap GPU readback with next frame, split composite to core 3, profile 3D setup. Speedup (804 MHz + L2) is already on (osSetSpeedupEnable in main). 30 fps cap is the game's own vblank pacing; lifting it would speed up gameplay.

## perf-chunk1 (18:50 build, NOT delivered; waiting on run 28)
- gpu3d_async (default 0): GPU draws frame N during composite/present; readback of N at End(N+1). 3D layer is 1 frame late
  vs 2D. Texture frees held an extra frame. Expected: hides G3G_End GPU wait (~2-3 ms mono, ~4-6 ms stereo).
- [G3PROF] line every 300 frames: GetTexture / GpuRaw / G3G_End per-frame us; the remainder of g3soft is pspoke frontend.
  Use it to target texture-gather (VRAM gen recheck) vs frontend.
- Core 3: not available to applications (system core), so no compositing there. Core 2 already runs engine B + right eye.
- Texture conversion: steady state already ~0 decodes; the possible cost is the raw-byte re-gather on every VRAM gen
  change. Measure with [G3PROF] GetTexture before changing it.

## perf-chunk2 (18:5x build, not delivered)
- tex_pagecheck (default 0; 2 = verify mode logs "[G3] tex_pagecheck MISMATCH"): skips the per-texture VRAM re-gather +
  memcmp after a VRAM write elsewhere, using vram_dirty.c 4 KB page generations (11 shim call sites linked) + VRAMCNT snapshot.
  Gain depends on [G3PROF] GetTexture; expected 0.3-2 ms when water/flower texture animation bumps the VRAM gen.
- REV byteswap for RGBA conversion in gpuscale upload (3x 49k px/frame) and GPU texture create: box-verified identical over 1e8 inputs.
- render3ds/gpuscale/g3gpu now -O3 -marm (g3gpu rule had a trailing -O2 that overrode DEFS).
- Checked: GPU2D_Soft/native_gpu (compositor) and g3_handler/frontend already -O3, ARM mode.
- Not done: no box harness can run the 3DS front end, so front-end hot loops need [G3PROF] numbers from hardware first.

## battle-audio + giratina-diag (18:56 build, on top of perf-chunk2)
- Audio: run 26 margin reset jumped playback back 24 pumps on every underfeed, which froze all channel/note updates
  ~125 ms per hold (stalls are frequent in battle: loads, many players) -> broken BGM/SE timing.
  New audio_margin_reset: 0 pin (original), 1 jump (run 26), 2 = default: pin, then while the backlog is under half the
  latency the state clock runs 1/16 slow until the margin is back (no freezes; ~6% tempo for <2 s after a stall).
  PSG/noise are already handled in sas_out (duty table / LFSR).
- Giratina: [G3DIAG-TITLE] every 300 frames: MTX_RESTORE slot histogram, lightmask, polygon IDs, alpha-0 attrs, BoxTest
  visible/hidden counts. New g3_boxtest_force=1: every BoxTest reports visible (if Giratina then draws fully,
  the box test / NNS culling is the bug).

## tearfix (18:56 build, tag tearfix)
- Top rows lagging on both screens: citro3d requests the swap at the vblank after the transfer and GSP latches it one vblank
  later; a frame presented inside that window was transferred into the buffer still being scanned out.
  tearfix=1 (default): if <2 vblanks passed since the last present, wait for vblank (twice if <1). At 30 fps (>=33.4 ms)
  it almost never waits; logs "[3DS] tearfix: waited on N of 600 presents". 60 fps screens may drop to 30 with it on.

## emu-pass1 (Azahar harness, 19:3x build)
- Emulator screenshots in emu/: title v_grid/v_gridb.png (default/boxforce/CPU), y_grid.png (lightfix), overworld o_grid.png.
- Giratina: same partial look on GPU, CPU and boxtest-forced -> shared front end; boxtest counts 0 (not used on title).
  3D layer dump (dump_frame=N config) shows the body really is missing/dark in the 3D layer itself (emu/dump_dc.png).
  Tried: projection Store/Restore index (made no difference, reverted); g3_lightfix (light vectors transformed by the
  vector matrix at LIGHT_VECTOR time, per GBATEK; default 1) - no visible change on title. Still open.
- cull3d: emulator confirms 1 = correct winding (image identical to 0, 457 vs 473 tris in Route 212), 2 = black.
  Default now cull3d=1.
- Scale modes 0/1/2 render correctly (o_s0, o_c0, o_s2).
- New config dump_frame=N: dumps 3D layer + both engines BMPs and 2D/3D registers at frame N.

## Run 31 (Giratina fixed, harness hold support) — tag run31-wbuf
- ROOT CAUSE (Giratina partial): game selects W-buffer (SwapBuffers bit1); port ignored it. Vertex dump ([VDUMP]) showed z==w for every title vertex -> all depth ~1.0 -> body lost the depth test. Now depth = w/g3_wmax (default 4096) in both GPU and CPU paths. Toggle g3_wbuf=1 (0 off, 2 force). Log: "[GPU3D] game depth mode now W-buffer".
- MTX_SCALE in POS_VEC mode no longer scales the vector matrix (GBATEK). Toggle g3_scalefix=1. patches/g3-scale-vecmtx.patch
- Harness: KEYS supports holds/combos "57:g@400", "30:q+w+n@300". D-pad = t/g/f/h (arrow/circle pad doesn't move the player).
- Battle reached in emulator from Route 212 save (wild Buizel). Screens: emu/walk3_grid.png, emu/walk3/t0082..t0097.
- Title: emu/w1_grid.png (GPU), emu/w_cpu_vs_gpu.png (CPU|GPU). Before: emu/z_grid.png.

## Run 32 (tearfix=2) — tag run32-tearfix2
- log_run29: tearfix=1 waited 1-2 vblanks on ~93% of presents (present 7-18 ms, mono locked at 33.9 ms/29 fps) and the top-rows tearing remained.
- New tearfix=2 (default): after C3D_FrameBegin (prev queue done -> citro3d has called gfxScreenSwapBuffers), wait right before C3D_FrameEnd until gspIsPresentPending(top/bottom) is false, i.e. GSP has latched the previous swap, so the display transfer never writes the buffer being scanned out. No fixed vblank waits. tearfix=1 = old heuristic, 0 = off. Log: "[3DS] tearfix=2: waited on N of 600 presents, avg wait X us".
- Caveat: LCD scanout walks framebuffer memory = landscape columns, so a pure scanout race should look like a vertical split; a horizontal "top rows late" band may instead be content-side. If tearing persists with tearfix=2 (+ vsync=1 test), look at the composite/upload path next.

## Run 33 — tag run33-se-zoom
- Battle SE missing: psp_ctru.c sceKernelAllocPartitionMemory had only 64 blocks; SAS sample cache (384 slots/3 MiB) needs one per converted sample -> after 64 distinct samples every new sample failed (voice skipped). Emulator battle already reached 60 blocks. Now 1024 blocks + [AUDIO-SE] line (keyons, skipped, cache, blocks high, alloc fails).
- Fill mode 2 zoom: gpuscale_zoom (1.25..1.5625, default 1.40625 = 360x270, 20px bars). emu: /workspace/emu_runs/r33_mode1_vs_mode2.png
- Tearing: tearfix=2 waited on 0-2 of 600 presents on hardware -> swap race NOT the cause. Composite barrier verified (LightEvent_Wait(bDone) before upload; per-engine line counters, no shared VCount). Cause still unknown; bisect with gpuscale=0 (CPU blit, no citro3d).

## Run 34 — tag run34-combined
- Audio clipping: our sceSasCore shim mixed at 12x per full-volume voice (DS ~0.5x) -> battle peaks ~73000, heavy soft-clip compression. New audio_gain (default 10, old 24). [AUDIO] 10s line now counts clipped grains.
- Battle SE silence: emulator battle (Thunder + level-up) shows every NNS seq/seqarc start OK ([AUDIO-SEQ], 0 fails), all keyons have real samples (pk>0), PSG/noise loop with correct pitch. Cause not found; suspect masking by the over-driven mix. audio_keylog=1 logs every keyon ([AUDIO-KEY]); [AUDIO-SE] has per-type keyon counts.
- Tearing: comp_excl=1 (opt-in) raises the main thread above all core-0 threads during the composite and spins on engine B. Diag "[2D] game state changed DURING composite in N of 600" (0 in emu). dump_seq=START,COUNT dumps composed frames (emu intro: coherent).
- Stereo blob: not investigated yet.

## Run 35 — tag run35-logring
- log_run32: audio_keylog=1 wrote ~2.5k lines synchronously (open/append/close per line) from the audio thread -> 2 s of audio took 7-16 s -> broken sound. ALL PSPNativeMemLog output now goes to a 512 KiB RAM ring (logring.c), flushed every 100 ms by a prio-0x3F thread with one persistent FILE; whole lines copied atomically (no interleaving); "[LOG] N lines dropped" if full; sync flush on crash/hold/exit.
- Backlog skip after a stall now drops the backlog (applies only the last skipped state) instead of replaying every snapshot (keyon storm). Toggle audio_skipfold=1. [AUDIO-CLK] shows "backlog drops N (pumps)".
- Gain fix confirmed on hardware: 10s peak 21301, 0 clipped grains.

## Run 36 — tag run36-quietboot
- Quiet boot (default): bottom screen black with a dim "Loading..." line; checklist only in log.txt. verbose_boot=1 restores the checklist console. Errors/crashes (hold -> uiError) switch to the full checklist + red error box. emu: /workspace/emu_runs/r36c/t001.40.full.png (loading), /workspace/emu_runs/r36_load_err.png (error path).
- Harness: SHOTS accepts fractional seconds (e.g. 1.4); shot files now tNNN.NN.*.png.

## Run 37: tearing root cause (3D readback race), scalesrc_dbl
- Box was reset mid-run (/opt wiped: devkitPro reinstalled via dkp-pacman; Azahar emulator NOT reinstalled -> no emulator boot check this run).
- Tearing root cause (code analysis): g3gpu readback ran C3D_SyncDisplayTransfer outside a frame, which waits with gspWaitForPPF().
  The PPF event is sticky; with gpuscale=1 the present queues 3-4 display transfers per frame whose PPF nobody consumes, so the
  wait returned immediately and the CPU converted rb[] while the DMA was still writing it (also no cache invalidate on rb[]).
  Rows beyond the DMA's progress = previous frame's 3D -> fixed split row (~DS row 68 in the photo), only while moving, only gpuscale=1.
- Fix rb_sync=1 (default): readback transfer queued in-frame after the draws, exact wait via citro3d queue (FrameBegin), then
  GSPGPU_InvalidateDataCache before reading. rb_sync=0 = old path. Also covers gpu3d_async.
- scalesrc_dbl=1 (default): ping-pong scale-source texture/staging sets. Analysis: citro3d already serialises this (FrameBegin waits
  for the whole previous queue; in-frame transfer queued before the draw), so belt-and-braces only.
- Mode 0 (gpuscale=0) should already have been immune (no stray PPF events), consistent with the user report.
- Sound: rain (SEQ_SE_DP_W240, keyon v11 vol 4/4 i.e. starts nearly silent, CC7 ramps) and Psychic (W060 vibrato CC1) analysed, no fix landed yet.

## Run 38: mode-0 leftovers + eye mismatch, stereo composite on 3 cores
- Bugs 1+2 (mode 0 after mode 1/2): gfxSet3D stayed on from the GPU stereo path, and mode 0 never writes the right-eye
  buffer -> one eye showed the stale scaled frame (two sizes / larger frame behind). Also citro3d's last queued transfer +
  async swap could land after the switch. Fix: on entering mode 0 P3DS_GpuQuiesce() (wait citro3d queue, 3D off),
  clear top L+R and bottom, both double-buffer halves; plus per-frame border clear in mode 0 (mode0_border=1 default).
  Mode 0 is mono by design (stereoShift(0)=0), so both eyes are now identical.
- FPS: in stereo the core-2 worker did engine B + right eye serially (~2x9 ms) -> composite 18.6 ms vs 9.6 mono. Mode 0
  looked faster only because it is mono. stereo_c1=1 (default): in stereo, engine B runs on a core-1 thread
  (APT_SetAppCpuTimeLimit(core1_limit=80)); falls back to old split if it can't start. stereo_c1=0 = old.
- Sound: LFO/mod-depth path (snd_exchannel.c SND_UpdateLfo/GetLfoValue, SND_MML_MOD_DEPTH) matches retail; no clear bug -> no change.
- 9 fps frame (g3soft 79 ms): not investigated.
- Untested: Azahar not reinstalled (box reset wiped /opt); no emulator check this run.

## Run 39: rain crackle, stereo_c1 default off
- Run 38 hardware: Psychic fixed, rain plays but crackly. stereo_c1=1 backfired (18.2 vs 23.9 fps, g3soft 18->36 ms,
  underfeed lines 22->45): core-1 thread starves g3soft + audio. Now default stereo_c1=0 (toggle kept).
- Rain = seq 1593, looping ADPCM voice (n=8712 loop=2888, ~20 kHz), heavy CC7/CC10 ramps. ADPCM is pre-decoded linearly
  so the loop-point state is inherently right. Causes fixed in the SAS mixer (psp_ctru.c):
  audio_volramp=1 (default): volume/pan ramped across each 256-sample grain instead of a step per grain (zipper);
    keyon starts at target (attacks unchanged).
  audio_interp=1 (default): linear interpolation instead of nearest-sample resampling.
  Either can be set to 0 to restore the old mixer behaviour.
- Underfeed holds (still ~22 lines/session with stereo) can also crackle; not changed.
- Stereo FPS: not done this run (timebox). Plan: skip recomposing engine B when its regs/palette/OAM/mapped VRAM are
  unchanged (memcmp ~1 ms vs ~9 ms composite), or compose 2D layers once for both eyes. g3soft spikes: not investigated.

## Run 40: 32728 Hz mixing, bottom-screen reuse, stereo_depth toggle
- Audio root cause candidate: SAS mixed at 44100 and ndsp resampled to the DSP's native 32728 Hz with linear interp ->
  16-22 kHz content (high-rate noise/PSG/rain) aliased back as inharmonic tones. Now audio_rate=32728 default (no ndsp
  resample, NDSP_INTERP_NONE); audio_rate=44100 = old. PSP 4x pitch cap removed in sas_out (PSP_SAS_PITCH_MAX 0x20000):
  high-rate voices no longer point-decimated at conversion. Patch: patches/audio-rate-32k.patch (sas_out.o must be
  rebuilt in native-audio-sound and copied to native-audio-app; the full Makefile does not rebuild it).
- Verified vs DS rules (code review): ADPCM decoder (header predictor, index &0x7F clamp 0..88, low nibble first,
  +-0x7FFF clamp), loop units (SND7 stores loopStart<<2 bytes; sample loop = pnt*2-8), linear decode gives the
  loop-point state, noise LFSR (0x6000 xor, init 0x7FFF), timer->pitch u64 math. No WAV dump (time).
- 3D doubled enemy at send-out: rb_sync covers both eyes (both queued + converted the same frame); CPU-clipped and
  clip-space paths use the same depth mapping. Suspect per-vertex parallax (0.3+0.7 z^8) jumping with sprite z.
  stereo_depth=0 = uniform parallax (diagnostic/fallback); default 1.
- FPS: bottom_skip=1 (default): engine B recomposed only when its regs/palettes/OAM/VRAM/ext-pal changed (pspoke
  BottomSame/SaveBottom compare, runs on the core-2 worker). present_skipbot=1 (default): skip bottom upload+prescale
  when engine B was reused. Logs: "[2D] bottom_skip: reused N, recomposed M of last 600", "[3DS] present_skipbot: ...".
- Not done: compose shared 2D layers once for both eyes (too invasive for the timebox).

## Run 41: present_skipbot fix, steal_b, stereo_clamp; rain/Poketch analysis
- present_skipbot was gated on !fpsOn (FPS overlay on) -> never skipped. The overlay is drawn on the top screen, so the
  gate was removed.
- steal_b=1 (default): in stereo the core-2 worker composes the right eye first; engine B (bottom compare/compose) is
  claimed atomically by whichever thread is free first (normally main, right after engine A). Log: "steal_b: engine B
  on main N, worker M" on the bottom_skip line. steal_b=0 = old order.
- stereo_clamp=1 (default): triangles with a degenerate vertex (w<1e-3 or depth outside 0..1) get uniform parallax.
- Sound: Poketch app switch = SEQ_SE_DP_DENSI01 (1647, prog 103 -> SWAR 750 SWAV 0, looped ADPCM tone, loop 1160/3008,
  played ~4 octaves up). Rain = SEQ_SE_DP_T_AME (1593, prog 120 -> SWAR 750 SWAV 17, 8712 samples, loop 2888, bend
  range 12 + bend -8192, CC7 ramp, re-triggered every 120 ticks). Both in WAVE_ARC_SE_FIELD (750). Decoded with an
  independent IMA decoder from the ROM via ndspy: count/loop match the port exactly; seams continuous (rain seam is
  noise, jump 5714; densi seam jump 241). WAVs (box only): test_saves/rain_swav17.wav, densi_swav0.wav.
  No decoder/loop bug found. audio_keylog cap raised 4000 -> 15000 (run40 log hit the cap before the Poketch presses).
  Open leads: the very high playback ratio (densi ~6x) and pitch-bend/first-tick timer; need [AUDIO-KEY] lines.

## Run 42: steal_b default 0, cut-off + doubled-enemy diagnostics
- steal_b hurt (23.7 fps vs 26.5): main-thread engine B lengthens the left-eye critical path. Default 0.
- Sounds "cut off": run41 [AUDIO-KEY]: DENSI01 = 2 notes on SWAV0 (n=3008 loop=1160, loop flag set: cnt c84000xx ->
  repeat=1) at pitch 6.6x / 9.8x, 112 ms apart (12 ticks @ tempo 400000 = 100 ms: gate timing correct). Rain = SWAV17
  re-keyed every ~690 ms alternating v11/v14 (120 ticks @ 272727: correct). Loop flag reaches the SAS voice; the silent
  channel advance (SIM_Audio_AdvanceChannelSilent) keeps looping channels active; backlog drops 0. Cause NOT found.
  Note: no keyon ever shows the rain's -12 semitone bend (tmr fcbe = +7 st); bend may only arrive on later timer updates.
  New: audio_keylog=2 also logs [AUDIO-OFF] (driver cleared enable: when/which voice ended) and [AUDIO-PIT] (timer
  changes after keyon) -> next log shows when the voices end and whether the bend arrives.
- Doubled enemy: display capture (DISPCAPCNT) is NOT implemented (only a one-time printf). Hypothesis: during the
  player's send-out a 2D copy (OBJ or captured bitmap) coexists with the 3D sprite; 2D gets zero disparity, 3D gets
  per-vertex disparity -> 2 images. New [STEREO] diag (slider up, max 1 line/15 frames): capture enable, display mode,
  number of >=32x32 OBJs on engine A, 3D polys. Fix pending on that data.

## Run 43: sound cut-off FIXED (SND_SetChannelPan7), audio_adapt, [SPIKE] diag, wideview (experimental)
- Cut-off root cause (run42 [AUDIO-OFF]): 40 voices ended with a garbage SOUNDxCNT (7fc0ffxx / ffc0ffff) ~100 ms after
  keyon. sim7 snd_channel.c SND_SetChannelPan7 (SDK_PORT path) did `reg = ~(0x7F<<16)` instead of `reg &= ...`: every
  pan change (Poketch DENSI CC10 sweeps, LFO/pan updates) overwrote the whole control word -> format bits = PSG on
  ch0-7 (forced off), repeat/volume lost. Fixed to `&=` (no toggle: plain bug). Normal key-offs (48400301 etc.)
  unaffected. Patch: patches/snd-setpan-mask.patch (object: native-audio-sound/audio_channel.o -> native-audio-app).
- Rain (seq 1593): SSEQ has NO pitch bend (only bend-range/portamento-off events + a track-volume fade-in); timer
  stays fcbe as it should. Rain voices end with normal key-offs. The "-12 bend" was a misreading of the MIDI.
- Doubled enemy: capcnt 0 in all 117 [STEREO] lines (no display capture), no mid-composite OAM/register changes, battle
  = 4-6 3D polys (sprite quads) + up to 23 big OBJs. Capture theory disproved; cause not identified. Test with
  stereo_depth=0 (uniform 3D parallax) to see if the per-vertex depth is responsible.
- audio_adapt=1 (default; 0 = old): sound-state clock speed follows the snapshot backlog (50%..104%, smoothed) instead
  of running full speed into the head and holding. [AUDIO-CLK] now prints the real window (2689 ms at 32728 Hz), the
  right expected pumps (~516) and adapt avg/min. Patch: patches/audio-adapt.patch.
- Spikes: the 79-232 ms "g3soft" frames are at overlay/app loads (gFieldMapTemplate entry after load/battle; g3soft
  time includes the game's own CPU work). New [SPIKE] line per frame > spike_ms (default 70, 0 off, max 60): romfs
  sdmc read ms + 16K misses, GetTexture ms, GPU texture creates ms/count, malloc. Fix next run from that data.
- wideview=1 (default 0, L+R+X toggles; BMP dump moved to L+R+X+Y): Hor+ side strips. Same 3D triangles drawn a 2nd
  time into a 512x256 target, viewport 320, x' = 0.8x (H-FOV x1.25, same vertical scale); the 32 px side strips are
  read back and drawn at x 0..40 / 360..400 in mode 1 (gpuscale=1) around the unchanged centre. Only overworld
  (last app = gFieldMapTemplate), BG0 = 3D, gpu3d on, rb_sync=1/async 0. Tri rejects widened to 1.25w / -32..288.
  Strips follow master brightness; 3D-clear pixels black. Limits: the game culls objects/map chunks outside the DS
  view (edge pop-in; g3_boxtest_force=1 may help for BoxTest-culled models), 2D (BG/OBJ, text boxes, weather
  overlays) stays 256 wide, BG0 scroll/blending/fog/edge marking not applied to strips, battles stay 4:3.

## Run 44: hotkey swallow, wideview v2 (single wide pass), rom_fastread, save indicator
- run43 log: wideview never drew a strip: "[WIDE] init failed (target)" (2nd 512x256 target + depth did not fit in
  VRAM; gpuscale holds 3 MiB). Only the widened triangle rejects were active (FPS 28-30 with it, no visible cost).
  "Bike" + "grass changed": the log shows "[GPU3D] L+R+Y: gpu3d=0" -> L+R+Y switched to the SOFTWARE 3D renderer
  (different-looking grass, 160 ms frames at f 1873-2086) and the Y press reached the game (registered item = bike).
- hotkey_swallow=1 (default; 0 = old): while L+R are held together with A/X/Y/SELECT/START, L R A X Y SELECT START are
  hidden from the game until L and R are both released (latch). Covers L+R+SELECT/Y/X/A/START and L+R+X+Y.
- wideview v2 (default 0, L+R+X): ONE 3D pass. While active the eye targets are swapped (at frame begin, GPU idle)
  from 256x256 textures to plain 320x256 framebuffers (640 KiB each incl. depth; frees the 1 MiB of the 256 ones
  first); vertices get x' = 0.8x (clip) / (sx+32)/160-1 (screen). Columns 32..287 = the normal 3D frame (pixel
  identical) -> compositor; columns 0..31 / 288..319 -> G3G_Strip -> gpuscale side strips (mode 1). Game matrices,
  culling and state untouched. Swap back when inactive; on allocation failure it logs VRAM free and disables itself
  (wideFail) with 256 targets restored. Log: "[WIDE] eye targets now 320x256 (VRAM free before/after)".
- rom_fastread=1 (default; 0 = old): run43 [SPIKE]: freezes are ROM reads (92 x 16 KiB misses = 532-545 ms at
  overworld entry, ~5.8 ms per 16 KiB fread). Reads >= 64 KiB go straight to the caller in one fread; cache misses
  read 64 KiB ahead (4 blocks) in one fread. ROM is read-only, no staleness.
- save_anim=1 (default): port "SAVING" box + spinner (top screen, bottom-right) for >= save_minms (1500) after the first
  backup write / 300 ms after the last, then "SAVED!" 0.8 s. Overlay only (holding the game's save status was not
  done: unsafe without knowing every caller's wait loop).
- Doubled enemy: no new cheap lead; untouched.

## Run 45: wide_cull, 16 MiB ROM cache, L+R+Y retired, log_level
- run44: wideview works (strips filled, "[WIDE] eye targets now 320x256 (VRAM free before 2070528, after 759808)"),
  26.6 fps stereo; buildings pop in at the edges (game culling). ROM spike still 533 ms (208 x 16K) at f=331.
- wide_cull=1 (default, only while wideview is active): the port's G3 box test (native-stack-render g3_handler.cpp
  G3SIM_BoxTest, used by map_prop.c GFXBoxTest_IsModelInView for buildings/props, also berry patches/pokeradar)
  tests against x' = 0.8x, i.e. a 1.25x wider view. Terrain = the loaded 2x2 map blocks (no per-block culling), so
  only props were culled. Map loading is unchanged. Patch: patches/run45-wide-cull-boxtest.patch.
- rom_cache_mb=16 (default; 0 = off -> run44 path), rom_ra_kb=1024: platform/romcache3ds.c, 64 KiB blocks, LRU,
  direct block->slot table, read-ahead doubling on consecutive misses (64 KiB..1 MiB) in one fread; keeps >= 12 MiB
  heap headroom (falls back 8/4/2 MiB). Host-tested against the ROM: 20000 random + sequential reads byte-identical.
  Re-entering areas hits the cache. Background prefetch thread NOT added (core contention risk).
- L+R+Y removed: CPU 3D renderer only as automatic fallback when GPU init fails (logged) or config gpu3d=0.
- log_level=1 (default; 2 = old full detail): per-frame/second detail tags go to a 128 KiB RAM ring that is appended to
  log.txt on every synchronous flush (crash handler, hold, exit). [PERF] every 10th, [AUDIO-CLK] every 4th; lines
  with fail/error/abort/crash/fatal/exception/assert always written. audio_keylog default was already 0.

## Run 46: save box replace, bottom_skip/present_skipbot default 0, lower sound latency
- run45: wide_cull fixed edge pop-in (kept). Save OK. Bottom: small glitch at battle start + "delayed" taps.
- Bottom analysis: touch is one sample per frame (input.c, P3DS_TouchRead each poll; auto-sampling fills the buffer
  with the current point per vblank) -> no queue. bottom_skip compares engine-B regs 0x1000-0x106F (incl. master
  bright), sub palettes, OAM, 128 KiB BG + OBJ VRAM, ext palettes when enabled; no missing input found, and raw[]
  is single-buffered (no older half). Cause unconfirmed -> bottom_skip=0 and present_skipbot=0 by DEFAULT now
  (costs ~1-2 fps stereo; set =1 to test). A real contributor to the "late" feel: total sound delay was ~250 ms
  (24 pumps snapshot latency 125 ms + 16 x 256-sample DSP lead 125 ms), so Poketch tap sounds kept coming after the
  taps stopped. Now audio_latency=16 (83 ms, override 6..48) and audio_lead=8 (62 ms, 4..16): ~145 ms total.
- save_anim=2 (new default): the port box covers the game's message-window area (top screen DS y 144..187) with
  "SAVING" + spinner for >= save_minms, then "SAVED!"; game flow untouched (its own window is covered, not removed;
  removing it would need a hook in the field save menu code). 1 = small corner overlay, 0 = off.
- res3d=2 (config, default 0; added to run 46 after the first run46 commit): 3D layer rendered at 2x (512x384, wideview
  640x384, both eyes) into plain framebuffers; the readback display transfer downsamples 2x2 in hardware
  (GX_TRANSFER_SCALE_XY) into the unchanged 256x192/320x192 compositor input -> 4x supersampled 3D (smooth edges,
  no shimmer) at zero CPU cost; GPU fill 4x. VRAM 1.5 MiB/eye (1.9 wide) with 24-bit depth, falls back to 16-bit
  depth, then to 1x (res3dFail, logged "[3DRES] ... FAILED"). [PERF] now ends with "res3d N, 3D GPU draw+readback X us".
  Not a true high-res composite (the 2D compositor stays 256 wide), so detail gain is AA, not extra texels.
  res3d=1.5 not offered: the transfer engine only box-downscales by exactly 2 (1.5 needs an extra GPU pass).

## Run 47 (tag run47)
- FPS regression of run 46 = bottom_skip=0. log_run45 vs log_run46, stereo field frames (tris>100): composite 11.0 ->
  19.1 ms (+8.1), present 3.3 -> 4.3, ms/frame 36.0 -> 45.2, fps 27.4 -> 21.9. g3soft unchanged (21.5/21.6).
  Fix: bottom_skip=1 + present_skipbot=1 defaults again, plus NEW bottom_refresh=8: engine B is recomposed without the
  compare for 8 frames after any scene change (ApplicationManager_New counter P3DS_SceneGen from memprobe.o), any
  VRAMCNT change (0x240-0x249) or engine-A DISPCNT change (battle start = scene change). 0 = no forced refresh.
- res3d hotkey: L+R+A toggles res3d 0<->2 in-game (was the old zinv debug toggle; zinv stays config gpu3d_zinv).
  Swallowed by hotkey_swallow (A is in the combo mask). Logged "[3DRES] L+R+A: res3d=N" + target change line.
- land_fastload=1 (default; only while wideview active, 2 = always, 0 = off): overlay005 land_data.c lazy map-model
  loader does read + texture bind + render-obj init in ONE task step instead of 56 KiB/frame chunks + 2 extra frames
  (block pop-in in the wideview corners). Rebuilt via overlays/rebuild_obj.py (scrcmd.c, overlay005/land_data.c).
- touch_latch=1 (default): taps shorter than one game frame (~35-45 ms) were lost (only the current touch state was
  sampled per game frame). Now the HID touch ring (samples since the last poll) is scanned and a missed tap is
  reported for one frame. Present path checked: no extra buffering (FrameBegin waits previous, tearfix=2 waited on
  0-2 of 600 presents). Remaining input->display latency is frame-time bound (~2 frames).
- save_seq=1 (default, with save_anim=2): box covers the message window from ScrCmd_OpenSaveInfo ("Would you like to
  save?" -> box shows "SAVE?", yes/no menu stays visible) through the write ("SAVING", min save_minms) and "SAVED!"
  until ScrCmd_CloseSaveInfo. NO closes it; a failed TrySaveGame drops the box so the game's error text shows.
- PERF breakdown (log_run45/46, field): stereo 27.4 fps / 36.0 ms = game+geometry ~13.4 + 3D GPU draw+readback wait
  ~8.1 + 2D composite 11.0 + present 3.3 (all SERIAL on the main thread; core 2 does right eye + engine B in parallel
  with engine A inside "composite"). Mono 29.3 fps / 33.4 ms = ~15.7 + 4.5 + 10.1 + 3.0.
  Build flags OK: -O2 -mtune=mpcore ARM; render/g3 files -O3 -marm.

## Run 48 (tag run48)
- L+R+A did fire (log_run47: "[3DRES] L+R+A: res3d=2" x6) but every time "640x384 targets FAILED (VRAM free 2070528)":
  res3d allocated TWO 2x targets (one per eye): 2 x 1.47-1.97 MiB (wide) / 2 x 1.18-1.57 MiB (normal) > 2.07 MiB free
  (gpuscale holds the rest), so it silently fell back to 1x. Fix: ONE 2x target shared by both eyes; each eye is drawn,
  then HW-downscale-copied to its own readback buffer, before the next eye clears the target (GPU queue order).
  Needs 1.97 MiB (wide, depth24) / 1.57 MiB (normal), with depth16 fallback (1.47 / 1.18 MiB).
- Hotkey toasts: L+R+A "SHARP 3D ON/OFF" ("SHARP 3D: NO VRAM" on fallback), L+R+X "WIDE VIEW ON/OFF",
  L+R+SELECT "SCALE 1:1 / 1.25X / FILL", L+R+START "FPS ON/OFF". 1.5 s box at the top of the top screen; logged [TOAST].
- early_b=1 (default): engine B (bottom) starts on the core-2 worker BEFORE the main thread blocks on the 3D GPU
  draw+readback (no game code runs between the two points, same state). Overlaps the 4.5 ms (mono) / 8 ms (stereo)
  GPU wait. Expected: ~1 ms/frame when the bottom is static (the compare moves under the wait), 5-8 ms on frames where
  the bottom is recomposed in stereo (worker had B + right eye in series). [2D] bottom_skip line now counts early_b.
- save_seq=2 (default): port YES/NO box over the game's yes/no window (DS 192..255 x 96..143) during the save sequence,
  cursor read from the game's Menu (Menu_GetCursorPos in ScrCmd_ShowYesNoMenu / ScriptContext_WaitForYesNoResult,
  read-only). 1 = run 47 behaviour.

## Run 49 (tag run49)
- Sharp 3D (res3d=2) garbage = 16-bit depth. log_run48: "640x384 ... depth 16, one target shared": 24-bit depth did
  not fit (after freeing the 1x wide targets VRAM has a ~1.31 MiB hole + a ~0.74 MiB hole; 640x384 color + depth24 =
  2 x 0.94 MiB), so the depth16 fallback ran: far (top) area z-fought into stripes, long diagonal seams between
  coplanar ground layers. (Also fixed: the depth16 clear used the 24-bit clear value.) Stride/transfer were fine
  (output 320x192 from 640x384, SCALE_XY, rb stride = curW).
  Fix: res3d_lindepth=1 (default): color stays in VRAM, the 24-bit depth buffer goes to linear FCRAM (GPU-addressable),
  cleared with a full-screen quad at the clear depth/color (memory fill is VRAM-only). Order: VRAM depth24 -> linear
  depth24 -> VRAM depth16 (correct 16-bit clear) -> 1x. Log line says which ("depth 24 in linear RAM").
  res3d cost (log_run48, mono, slider 0, wide): 3D GPU draw+readback 7.6 ms vs ~4.5 ms at 1x; fps 29-30 (game cap).
  FCRAM depth adds GPU time (unknown, maybe +1-3 ms); stereo not measured yet.
- Zone-change hitch (log_run48 [SPIKE]): field spikes are ROM reads: 8-172 x16K misses at ~2.4 ms each
  (~6.7 MB/s from SD romfs), e.g. f=320 488 ms (410 ms romfs, 172 misses), f=806 172 ms (118 ms romfs). The 195 ms
  spike at f=2987 is the save write (under the save box). Not fixed this run (needs either faster SD reads via direct
  FSFILE_Read of large chunks or a neighbour-zone prefetch thread).

## Run 50 (FINAL, tag run50)
- split_top=1 (config, default 0, untested on HW): mono only; engine A lines 96..191 on the core-2 worker with an
  engineA copy fast-forwarded through lines 0..95 (windows, affine refs, BG/OBJ mosaic: same steps as melonDS's
  skipped-line path); worker waits for the main thread's line 0 (shared flat-VRAM coherency). Not used in stereo:
  left eye (main) + right eye (worker) already fill both usable cores; core 1 hurt before (stereo_c1). Mono is mostly
  at the game's 30 fps cap, so default off. [2D] bottom_skip line counts split_top frames.
- zone_prefetch NOT implemented: zone hitches are SD throughput (~6.7 MB/s with up to 1 MiB reads; the spike counter
  counts 16 KiB units of data, not requests), so bigger reads don't help; a background prefetch would make the game
  thread wait behind the prefetch's SD reads (FS serialises) and needs game-data parsing for neighbour zones.
- Fail-safes: save takeover box drops itself when leaving the overworld or after 20 s without a yes/no frame or save
  write ("[SAVE] seq fail-safe"); outside the overworld save_anim=2 uses the small corner box (never covers text in
  other apps). Frontier/other saves that don't use OpenSaveInfo never show the takeover.
- README_settings.txt in platinum3ds/full (plain-English toggles + hotkeys).

## Run 51: zone_prefetch (background loading for area changes)
- Toggle zone_prefetch (default 1; 0 = run 50 behaviour). rom_cache_mb default 16 -> 32 (auto falls back 24/16/8/4/2
  keeping 12 MiB heap headroom).
- Design (platform/romcache3ds.c): hook P3DS_ZoneEnter(mapHeaderID) in FieldMapChange_UpdateGameData (+ DistortionWorld
  variant) -- covers warps AND walking zone changes (fieldmap.c calls UpdateGameData(fs,1)). Patch copy in patches/run51/.
  Learning (game thread): edge prevZone->zone (8 neighbours, MRU first) and, for 4 s after entering, every cache-miss
  fill range of that zone (32 ranges, merged). Prefetch thread (core 2, prio 0x3F, own FILE*): while in zone A and
  the game has not touched the ROM for 150 ms, no save box open / no save write in 10 s, no log write: reads ONE 64 KiB
  block of A's neighbours' learned ranges that is not cached into one of 8 staging slots (state flag only, no locks).
  Game thread adopts ready slots (max 4/call) in Present and at each ROM read entry -> cache never touched off-thread.
  Budget per zone visit: cache/4. Table persisted to prefetch.bin (magic PF01 + romSize, tmp+rename, max 1/min,
  only idle & not saving); loaded on the game thread at the first frame after the cache is up; invalid file ignored.
- Log: '[PREFETCH] zone_prefetch=1', '[PREFETCH] learned table loaded: N zones', per zone change
  '[PREFETCH] zone A -> B: learned X blocks, Y already in RAM | totals: fetched/adopted/used | zone changes, ready % |
  worst learn-window SD read ms'. Box build only; untested on hardware. Fallback: zone_prefetch=0 or run 50/48.

## Run 52: wide_fadefix, sharp 3D disabled
- wide_fadefix=1 (default; gpuscale.c strips()): side strips get the current top fade at DRAW time (MASTER_BRIGHT 0x6C,
  BLDCNT 2/3 on BG0 + BLDY, forced blank / BG0 off / display off = black), strongest fade held 3 presents after it
  ends. With it on, stripConvert no longer bakes master brightness in (no double fade). Log: '[WIDE] wide_fadefix=N'.
- res3d forced 0: config res3d ignored, G3G_Res3dToggle is a no-op, L+R+A does nothing (still swallowed, no toast).
  Log: '[3DRES] res3d disabled'. Box build only; untested on hardware.

## Run 53: New 3DS controls, soft reset, audio long-sample fix, diagnostics
- Controls (platform/psp_ctru.c, main3ds.c): cpad_dpad=1 (circle pad -> D-pad, dead zone 40/156, 4-way with 1.25x
  hysteresis, only when the D-pad is idle), zr_item=1 (ZR -> Y), zl_run=1 (ZL toggles P3DS_RunToggle; the game's
  PlayerAvatar_IsRunButtonHeld returns B XOR toggle -> overlays player_move.c), cstick_poketch=1 (C-stick flick up/down
  = 4-poll tap on the Poketch side button at (240,64)/(240,128), only P3DS_InField and stylus up, re-armed at centre).
- soft_reset=1: hotkey_swallow now passes L+R+START+SELECT through when all four are held; the game's own soft reset
  calls OS_ResetSystem -> (platform.c, PLATINUM3DS) P3DS_SoftReset: CIA -> aptSetChainloaderToSelf; 3dsx -> hb:ldr
  SetTarget(argv[0]) + SetArgv + aptSetChainloaderToSelf (the hbmenu/Rosalina way); else plain exit. Then hardExit
  (threads stopped, platinum.sav fclose'd). Was: PSPNativeFatal "native restart unsupported" (never reachable because
  the combo was swallowed). L+R+START (FPS) and L+R+SELECT (scale) now fire on release and not after the reset combo
  (P3DS_ResetSeen); SELECT+START 60-frame checklist hold ignored while L/R held. UNTESTED on hardware: if the relaunch
  fails the app just exits.
- Audio (native-audio-sound/sas_out.c): PCM_MAX_SAMPLES 0x10000 -> 0x400000. Samples longer than 65536 frames were
  halved by point decimation (no filter) for the PSP sceSas limit our 3DS mixer doesn't have -> upper half of their
  spectrum lost + aliasing. Root cause of "missing high-pitched music sounds" NOT confirmed (run 52 log: no pitch clamps,
  channel 13 never needed so no voice stealing, 0 skipped samples). New diag in [AUDIO-SE]: 'notes without wave'
  (snd_bank.c SND_NoteOn found no wave data) and 'halved'.
- Bike: mount path reviewed (MountOrUnmountBicycle -> PlayerAvatar_RequestStateCycle -> Redraw -> billboard swap).
  Port frame order matches the DS (vblank callback / VRAM transfers run before render+present); texture cache is
  content-checked; VRAM dirty redirects intact. No port bug found; added '[BIKE] mount at f=N' log marker
  (overlay005/ov5_021DFB54.c) to line up with [SPIKE]/glitch reports.
- Save check: backup.c writes in place with fwrite+fflush per CARD write request, fclose at clean exit (log_run52 shows
  'exit: clean'). User confirmed progress persists. No change.
- Patched pspoke files copied to patches/run53/ (platform.c, player_move.c, overlay005__ov5_021DFB54.c).

## Run 54: bike yellow box, C-stick D-pad, wide view fades + weather
- Yellow box (platform/g3soft.cpp, g3gpu.c): ROOT CAUSE = timing. The DS rasterises a frame's 3D AFTER that frame's
  VBlank VRAM transfers; the port resolves/decodes textures while the game submits geometry (before VBlank). The bike
  mount swaps the player billboard to a new VRAM slot whose texture/palette is transferred at the next VBlank, so for
  one frame the port decoded the not-yet-written slot (old/garbage texels + palette = a solid coloured box). Also made
  every sprite animation frame one frame late. Fix tex_postvblank=1 (default): every texture used this frame is
  re-checked in G3SIM_FlushDeferred (after the VBlank callback, before G3G_End draws): VRAM page gen newer than the
  decode -> GatherRaw compare -> re-decode into the same px + G3G_TexUpdate (re-swizzle in place, C3D_TexFlush; same
  C3D_Tex the queued batches reference). Only on the GPU sync path (gpuOn==1, !G3G_Async, rb_sync on). Decode code
  factored into DecodeTex(Tex*) (uses entry fields, not s_texImageParam). Log '[G3] tex_postvblank: N textures
  refreshed after VBlank (30 s)'. No placeholder colours exist in the decode path (unmapped VRAM decodes to 0).
- cstick_mode (psp_ctru.c P3DS_CstickMode, main3ds.c): 0 off, 1 Poketch flick (run 53 behaviour), 2 = C-stick -> D-pad
  with the circle-pad logic (dead zone 40, 4-way, 1.25x hysteresis, only when D-pad and circle pad idle). Default 2.
  cstick_poketch ignored. Controls log line shows cstick_mode.
- wide_fx=1 (render3ds.cpp wideFx, before save box/toast/FPS are drawn): strips derived from the centre composite of
  the same frame: robust per-channel affine fit composite = k*3D + b over the 32 px edge bands (3 passes, outliers
  >=28 trimmed) applied to the strips (master bright, BLDY, uniform alpha tints, white/black); per row+side black when
  the adjacent band is >=75% black while 3D is not (wipes, door/warp, battle bars); inlier share <35% (release >50%) =
  2D take-over -> black strips; residual (composite - fit) of the wrapped band (left <- 224..255, right <- 0..31, DS
  BG 256 px wrap) added to the strips for weather/overlays, skipped on rows where >=50% of that band is an opaque panel.
  Stereo: eye 1 uses the same fit/residual. gpuscale strips() skips its register fade and stripConvert skips master
  bright while wide_fx is on. Logs '[WIDE] wide_fx 600 frames: ...' and '[WIDE] wide_fx avg N us/frame'.
  NOTE: DS 3D fog (DISP3DCNT bit 7 / FOG_TABLE) is not emulated by the port at all (centre included) - not added.
- Box build only; all of run 54 is UNTESTED on hardware.

## Run 55: big batch (doubled battle sprite, settings menu, wide fill, 3D fog, stereo 2D depth, forwarder)
- 1 Doubled Pokemon at send-out (overlays battle_anim__battle_anim_system.c, copy in patches/run55/): CAUSE =
  BattleMonOBJData_NewAll. During a send-out (Task_ShowPokemon), the encounter start (Task_ShowEncounter) and a ball
  effect (ov12_02235E94) the game makes 2D OBJ copies of every VISIBLE battle Pokemon, only for layering with the
  ball-seal effect. On the DS they sit pixel-exactly over the 3D sprite quads; on the 3DS the 3D sprites get stereo
  parallax (and keep animating) while the 2D copy stays at screen depth -> the Pokemon already out (e.g. the enemy
  while you send yours) shows twice until the ball opens. Fix battle_monobj_hide=1 (default): NewAll hides the copies
  (ManagedSprite_SetDrawFlag 0); the 3D sprites stay. Only visible side effect: seal particles no longer go behind the
  far Pokemon. (Matches run 42/43 notes: no display capture, 4-6 3D quads + big OBJs in battle.)
- 2 Stereo black blob: no description/log of it exists; code review of both eye paths (per-eye targets/clears, vertex
  budgets, clip vs CPU-clip parallax, stereo_clamp, right-eye engineA2 composite) found no defect. Added
  stereo_blobdiag=1: every 60 stereo frames find 16x16 cells black in one eye only (>60%), log '[STEREO] blob: ...'
  and dump stereo_blob_{L,R,3dL,3dR}.bmp once to the SD. Azahar is no longer installed on the box (/opt/azahar gone),
  so no emulator check.
- 3 Settings menu (render3ds.cpp runMenu, menu_combo=1): L+R+Y (Y was already swallowed with L+R; not with X, so
  L+R+X+Y dump still works). Runs inside Present on the game thread = game paused; draws a native-size box over the
  dimmed last frame (mono while open), D-pad/circle pad navigation. Items: FPS counter, wide view, scale, cstick_mode,
  zl_run, zr_item, cpad_dpad, wide_fx, log detail (1/2). Applied on close; written with P3DS_CfgWrite (psp_ctru.c,
  replaces/appends key lines) / saveScale. Waits for button release before resuming. Watchdog ignores the pause
  (P3DS_MenuOpen). New fps_counter=1 key (menu-saved start state). Live setters: P3DS_SetScaleMode,
  P3DS_SetCstickMode, P3DS_SetLogLevel, P3DS_CtlZr/Zl/Cp globals, render3ds wideCfg/wideFxV file scope.
- 4 wide_fill=1: with wideview on and 1.25x mode, any frame without real wide 3D strips (battles, title, menus, field
  with 3D off) fills the side areas from the same frame's final picture: edge mirrored, 9x9 box blur, 80% brightness.
  Real wide battle 3D/BG extension NOT done (battle BGs are 256-px 2D layers; would need a 320-wide 2D compositor).
  wide_fx robustness: fit now per side first (a menu covering one edge band made the run-54 combined fit lock onto
  the panel: k=0,b=242 -> white strips; host test reproduced it), shrinking outlier thresholds 128/80/48/28.
- 5 fog3d=1 (g3gpu.c applyFog): opaque batches write the polygon fog bit (s_curPolygonAttr.fogEnable via G3G_CurFog)
  into stencil, clear plane fog = CLEAR_COLOR bit 15; D24S8 copied out with the colour readback (in-frame display
  transfer) and fog applied to the RGBA8 readback before conversion (melonDS math: offset<<9, (dz>>2)<<shift,
  34-entry table, alpha-only mode). Whole 320 target -> strips get fog. Only rb_sync path, 1x, D24S8, not W-buffer.
  Untested on hardware; cost only on fog frames (~49-61k px pass + 320 KiB transfer). Log '[GPU3D] fog3d: fog frame'.
- 6 stereo_2d=1: right-eye engineA2 snapshot (rightPar path) outside the overworld: text-mode BG1-3 with priority 3
  shifted by 2 x sd, priority 2 by 1.5 x sd (sd = slider*6), prio 0-1 and OBJ at screen depth. Overworld untouched.
- 7 FPS: NOT done - no hardware/emulator to measure on; blind changes to the GPU wait/readback/compositor would break
  the 'measure, never drop' rule. New per-frame costs this run: wide_fill (only non-wide screens in wide mode, ~0.3 ms
  est.), fog (fog maps only), wideFx per-side fit (~2x fit cost, est. < 1 ms). [WIDE] wide_fx avg line measures it.
- 8 Forwarder (forwarder/build.sh, assets/extract.py): icon/banner = shiny Origin Giratina front (pl_otherpoke.narc
  153, palette 247; Pt NCGR decrypt seed=data[0] forward), banner sound = WAVE_ARC_PV487 (PCM8 13379 Hz -> 32 kHz
  WAV). CIA: backups/platinum3ds_forwarder_run55.cia (reinstall via FBI). Same title ID as before.

## Run 56 hotfix (hardware run-55 log: 14-20 fps overworld, draw+readback 10-20 ms, present up to 20 ms)
- Causes from log_run55.txt (vs run 52 at 28-30 fps, readback 3-4 ms, present ~2 ms):
  * wide_fx: '[WIDE] wide_fx avg 13940 us/frame' (run-55 per-side robust fit, in present). -> default wide_fx=0
    (wide_fadefix look, as run 52). Also it and wide_fill copied menu/YES-NO box edges into the strips.
  * wide_fill (battles/menus/title): looked bad + copied menu bits. -> default wide_fill=0 (black bars = run 54 look).
  * fog3d (CPU exact pass): mid-frame C3D_SyncDisplayTransfer of the depth buffer per eye (GPU stall) + a CPU pass
    over 320x192 per eye; the title and most routes have DISP3DCNT fog on, so it hit nearly every frame (and twice with
    the 3D slider up) = the 10-20 ms readback. -> fog3d=1 now uses the PICA fog unit: 128-entry fog LUT built from the
    DS fog table/offset/shift with the same melonDS density math (rebuilt only when registers change), indexed by the
    stored depth (= DS 24-bit z; Azahar: fog_index = depth*128), per batch on/off from the polygon fog bit, clear plane
    fogged on the CPU (one colour). ~0 ms. Differences vs DS: fog also on translucent polygons with the fog bit, no
    alpha fog (Platinum uses colour fog), works on async/hi-res paths too. Old exact path = fog3d=2.
    Logged once: '[GPU3D] fog3d LUT ...' (Route 212: table 0..124 linear, offset 0x726f, shift 3).
  * kept: tex_postvblank (bike fix; ~6 refreshes/s, g3soft side), stereo_2d (just BG scroll offsets on the right-eye
    snapshot, ~0 ms), stereo_blobdiag (one scan every 60 frames), menu, controls, doubled-sprite fix, forwarder.
- Emulator (Azahar 2126.2 reinstalled at /opt/azahar, imagemagick installed): boots to Route 212 overworld. fog3d=1
  shows the distance haze (emu_shots/run56/fog3d1_gpu_route212.png vs fog3d0_off_route212.png); fog3d=2 shows none
  in Azahar (its depth-buffer display transfer is not emulated) - so the run-55 hardware look should equal fog3d=1.
  Other planned emulator checks (menu, battles, stereo) stopped by priority change.

## Run 57 (tag run57; builds on run56 dbeadab). Every FPS item measured in Azahar 2126.2 (relative, emulated ticks)
- New [PERF2] line each second: upload us (present source uploads) | 3D convert us (rbConvert, all eyes + strips) |
  composite us.
- 1a gpu_swizzle=1 (gpuscale.c, render3ds allocBufs): compositor buffers raw[2] + rawR now in linear memory (one
  768 KiB linearMemAlign, heap fallback); the present DMAs them straight into the scale textures and the prescale pass
  rebuilds RGB with 3 TEV stages (texel = (0,B,G,R) -> a*(1,0,0) + b*(0,1,0) + g*(0,0,1), alpha const 1). Removes the
  CPU byte-swap pass over every uploaded screen (1-3 x 49k px on the main thread). Azahar: upload 1250 -> 66 us/frame
  (mono overworld), images identical (diff = rain only). Non-linear sources (stereoL fallback) use the old path.
- 1b fast_conv=1 (g3gpu.c rbConvert): bswap + mask form of the RGBA8 -> 6/6/6/5 conversion, verified bit-identical
  on host over 1M words. Azahar: 3D convert 2465 -> 1730 us/frame (-30%).
- 1c split_top now default 1 (run 50 code, mono only): Azahar overworld composite 22.5 -> 14.2 ms (-37%), Route 212,
  wild battle intro/send-out, start menu and party menu visually identical to split_top=0 (no seam at line 96).
  emu_shots/run57/battle_sendout_split_top.png, party_split_top.png.
- Not done (honest): GPU 2D compositing (a new BG/OBJ renderer on the PICA; weeks, not a run), direct 3D readback
  into the compositor format (needs a TEV swizzle pass + 256 texture targets; wide targets are not textures), moving
  the right-eye convert to core 2 (no gain: the worker is on the critical path in stereo), overlapping compose with
  the GPU wait (every engine-A line needs this frame's 3D). Core 3 is not available to apps. Stereo FPS therefore
  only gains from 1a/1b. Expected HW: mono -0.5..1 ms (1a) -0.3 ms (1b) and -3..5 ms in 2D-heavy mono scenes (1c);
  stereo -1..1.5 ms (1a with 3 screens, 1b).
- 2 Wide battles: SKIPPED. Battle Pokemon are 3D quads but the background + platforms are a 256-px 2D BG; the
  melonDS compositor is fixed at 256 px, so real side content needs extra engine-A composites (~8 ms each) or a new
  320-px compositor. Natural BG wrap only repeats the opposite edge (fake). Black bars stay.
- 3 Overworld depth: stereo_strength=0/1/2 (1x/1.5x/2x per-vertex parallax; 6/9/12 DS px per eye at full slider),
  zero cost. Overworld people/Pokemon are 3D billboards, so they already get depth from the 3D pass; 2D text boxes
  and HUD stay at the screen plane (correct). Default 0 (original). Menu: 3D STRENGTH.
- 4 Following Pokemon: SKIPPED. Platinum has the native follower movement (MOVEMENT_TYPE_FOLLOW_PLAYER, used for
  Cheryl/Mira/Riley etc.), but (a) map objects are written into the save (an extra object would end up in the user's
  save - against "fix the port, not the save"), (b) scripts address LOCALID_FOLLOWER by that movement type (partner
  scripts would grab it), (c) Platinum has overworld sprites for only a handful of species; a party-icon billboard
  needs a new sprite resource path. Too big/risky for this run.
- 5 cam_zoom=-2..+2 (patches/run57/overlay005__field_camera.c): FieldCamera_Create scales the camera distance by
  86/93/100/108/116 %; the menu re-applies it live to the current field camera (P3DS_SetCamZoom, main3ds.c, relative
  to the step it was created with). Azahar: menu change -> '[CAM] live zoom 2 (distance 2731713 -> 3168787)', saved
  as cam_zoom=2. Zoom out moves geometry deeper into DS fog (Route 212 looks hazier). Default 0.
- Menu now table-driven + scrolling (10 rows): 14 items. Azahar: open/navigate/change/close/save verified
  (emu_shots/run57/menu_cam_zoom.png).

## Run 58 (following Pokemon, HGSS sprites from the user's SoulSilver ROM) — tag run58 (WIP checkpoint run58-wip)
- Extractor: followers/extract.py (+ btx.py NSBTX parser, hgss_luts.py = species->model code tables transcribed from
  the pokeheartgold decomp source; no asset downloaded). Reads roms/soulsilver.nds (IPGE rev 0) NARC a/0/8/1 files
  297..862 -> platinum3ds/full/followers.bin (2,742,008 B, 566 sprites: 535 x 32x32, 31 x 64x64, 8 frames 4bpp +
  normal/shiny palette). followers/preview.py renders a sprite strip. Side frames in HGSS face RIGHT; left = mirrored.
- Runtime: render-only entity, NOT a MapObject (never in the save, invisible to scripts, no collision).
  patches/run58/p3ds_follower.h (included by overlay005__fieldmap.c, called right after BillboardLists_Draw() inside
  the billboard projection bias) does the tile trail + hiding + draws one quad through the normal G3 path
  (-> g3soft/g3gpu: stereo depth + wide view for free). Texture: port-owned (g3soft.cpp folTex[8]), picked by the
  magic TEXIMAGE address 0x7FFF8 + PLTT_BASE 0x1FF00+frame*16; kept out of the tex_postvblank VRAM recheck.
  State in P3DS_FolMem (port memory) so it survives the overlay reload around battles.
- Trail: when the player's tile changes, follower walks from where it is to the tile the player left, interpolated
  by the player's own step progress (ledge = 2 tiles). Facing = its own move direction; 2-frame anim (8 frames
  walking, 16 idle). Hidden (until the next normal step) when: not walking (bike/surf/field move states),
  Distortion World, any MOVEMENT_TYPE_FOLLOW_PLAYER object or the partner system flag, player hidden, map change,
  a tile change during a field task/script (doors, cutscene movement, rock climb), a jump > 2 tiles.
- Config follower=0/1 (WIP default 0) + menu item FOLLOWING POKEMON (15th). follower_debug=1 draws it 2 tiles right
  of the player (alignment calibration; HGSS frames needed +3 view units up).
- Azahar so far (Route 212 save, lead = Girafarig F): follows walking down/left/up/right with correct facing and
  depth order (north = behind the player, south = in front); bike: hidden while riding, back after dismount + step;
  save: follower=1 vs follower=0 save files differ only at the same 11-13 offsets two follower=0 runs differ at
  (play time/RTC/wandering NPCs/checksums) -> nothing follower-related stored.
- Fixes after the checkpoint: a map header change alone no longer hides it (Route 212 north/south are seamless);
  the step already under way when a script starts (trainer's sight) is still followed, a 2nd tile change inside the
  same task hides it (cutscene movement / door exit / rock climb). Hide reasons are logged on change ([FOL] state).
- Azahar final: trainer spots the player -> follower stays visible during the approach; trainer battle (Ranger
  Taylor) won and back on the field -> follower is still there (state survives the overlay reload) and walks behind
  again (emu_shots/run58/after_battle_back.png, after_battle_walk.png). Bike hidden/back, walking 4 directions,
  depth order, save comparison as above. Boot checked with the default (follower=0) and with follower=1.
- Stalls: 3 of the early follower=1 battle runs halted with the watchdog "no frame for 10 s, main last: g3-flush"
  inside battle (overlay 5 and the follower code are not running there). The same halt happened in run 57
  (emu_runs/r57w, no follower code), and 4 later parallel runs (2 on, 2 off) of the same 5-minute battle had none
  -> looks like the existing random emulator g3-flush stall, but not proven. One reason the default stays 0.
- Cost: one quad + one 32x32/64x64 texture set loaded once per lead species (~4-16 KB file read). Zero-cost by
  construction; a clean on/off [PERF] A/B was NOT done (the last pair ran two emulators in parallel = noise).
- Untested: doors/buildings (Route 212 save has none nearby), stairs, surf/fly/rock climb in practice (code paths:
  player state != walking / warp jump), partner sections (Cheryl/Mira: hidden via FOLLOW_PLAYER object + partner flag),
  64x64 species, shiny/forms in game (extractor + preview checked), hardware FPS.
- Default follower=0 (new, doors untested). Ideas not done: HGSS "comes out of its ball" pop on the player's tile
  after warps (the reference repo does this), hiding indoors in small rooms.
- Reference read (ideas only, nothing copied; port code there is MIT, see refs/emerald-3ds/LICENSE-PORT.md):
  /workspace/refs/emerald-3ds (shallow clone of ZallaxDev/emerald-3ds).
  * Its follower (patches/pokeemerald/0056-follower-pokemon.patch) IS a game object event, but with a reserved local
    id that SaveObjectEvents skips and load_save.c wipes; it "sits in its ball" on the player's tile after warps /
    surf / bike and pops out on the next step; door warps walk it in behind the player; hides indoors where HGSS
    keeps it in its ball. Its builder downloads the sprites — we extract from the user's own SoulSilver ROM instead.
    Ours stays render-only (no object at all), which is stricter for the save/scripts.
  * GPU compositor notes for the upcoming full-width renderer: 3ds_port/src/3ds_video.c (11k lines, citro2d/citro3d
    on the PICA). CPU only decodes DIRTY 8x8 tiles (LayerRenderCells: per-cell dirty list + memo of tile/palette
    entries -> slots in a texture atlas `sAtlas`); each BG layer is a render-target texture (LayerCreate/LayersRender),
    sprites are batched atlas quads (SpriteBatchRoom/Flush, DrawObjectAt/DrawObjects by priority), blending/fades via
    C2D image tints (DrawSlotTinted), windows split the screen into rectangles of uniform layer mask drawn with
    GPU scissor (WindowPartition -> ComposeBand -> Layers(mask)); per-line window changes become horizontal bands
    (sLineBand). Priority = draw order per priority level. The GPU queue is kicked early mid-frame (GpuStartEarly,
    wrapped GX_* calls flush the linear heap first) so CPU recording and GPU drawing overlap. Field geometry/
    placement helpers in 3ds_field_view.c (screen-space metatile mapping for a 400-px wide field).
    For us (DS, 2 engines, affine/extended BGs, 3D layer as BG0): same tile-atlas + per-layer-quad idea would
    replace the melonDS per-pixel GPU2D_Soft; line-based effects (HBlank scroll, window per line) need the band
    splitting. Must stay additive behind its own toggle (user constraint), melonDS path unchanged when off.

## Run 59 (follower draw order + GPU 2D compositor) — tag run59-wip
- Stage A (follower fix): a follower on the tile SOUTH of the player (walking up) is nearer the camera and its
  sprite covered the player's legs (looked like it sat on/under the player, run58 after_battle_walk.png). Now within
  1.5 tiles of the player its quad is pushed to just behind the player's sprite depth -> the player always overlaps
  it (follower_behind=1 default, 0 = old). Tile offset unchanged (calibrated in run 58 against the player sprite with
  follower_debug=1: same feet line). Azahar 4 directions: emu_shots/run59/follower_{down,left,up_behind_player,right}.png
  + follower_4dir_crops.png. patches/run59/p3ds_follower.h.
- Stage B0 (GPU 2D compositor groundwork): gpu_compose=1 = ANALYZER ONLY (menu GPU COMPOSE OFF/ANALYZE, default 0):
  per engine, every 300 frames, a [GPUC] line with the % of frames a first PICA compositor could take ("simple" =
  text BGs 4/8bpp + ext palettes, plain OBJs, 3D BG0, BLDY/master-brightness fades; no windows, alpha blend,
  mosaic, affine/bitmap BGs or OBJs, capture) and how often each hard feature is used. Register/OAM reads only,
  ~4-12 us/frame in Azahar, nothing drawn differently. Azahar 5-min run (Route 212 walk, trainer battle, field again,
  emu_runs/r59gc/log.txt):
    * engine A overworld: 100% simple (objAffine 23% in one window = Cut/field-move effect).
    * engine B overworld (Poketch): flagged mosaic 100% -> check MOSAIC register; probably mosaic bit set with size
      0 (= no-op) -> treat size-0 mosaic as simple.
    * battle engine A: 17-80% simple (alpha 20-73%, bg8bpp 80-100%, objSemi 4-28%, objAffine 10-16%);
      battle engine B (touch menu): alpha 100%.
    * fades: masterBright / BLDY common on transitions.
  => Stage 1 GPU target = engine A overworld (text BGs + OBJs + 3D, brightness fades), fall back to melonDS for any
     frame with a hard feature. Battles need alpha blending (stage 3, as planned).
- Stage 1 (GPU compositor drawing) NOT started this run (timebox). Plan, all additive behind gpu_compose=2:
  1. platform/gpucomp.c (new): tile atlas (RGBA8 C3D texture, 8x8 cells) filled from VRAM only for tiles whose VRAM
     page gen (PSPNativeVramPageGen) or palette changed; per text BG one quad per visible map cell (<= 33x25),
     h/v flip via UVs, scroll from BGxHOFS/VOFS; OBJs as atlas quads (1D/2D mapping, all sizes, flips); priority by
     draw order (BG3..BG0 / OBJ per priority level, OBJ above BG of equal priority), alpha test for colour 0.
  2. Compose into the existing 256x192 composite buffer target ONLY for frames gcAnalyze() calls simple, else the
     melonDS path runs as now (per-frame fallback, so no picture can get worse).
  3. Parity check: dump_seq=START,COUNT already dumps composed frames as raw RGBA -> add a host script comparing
     gpu_compose=0 vs 2 dumps pixel by pixel for the same key sequence (Route 212 walk, start menu, bag).
  4. Measure [PERF] composite ms both ways, one emulator at a time; default stays 0 until parity + a win.
  5. Stage 2 (menus at native 400 px) builds on the same quads: reposition BG/OBJ layers into the 400-px target
     instead of stretching; separate toggle.

## Run 60 (GPU 2D compositor stage 1, gpu_compose=2) — tag run60
- New platform/gpucomp.c (C, citro3d, blit shader reused), hooked in render3ds.cpp; default gpu_compose=0 = the
  function is never called (old path unchanged). Menu GPU COMPOSE: OFF / ANALYZE / GPU / VERIFY (config 0-3).
- Eligibility (P3DS_GcEligible, engine A only, per frame; anything else -> melonDS path that frame): display mode 1,
  no forced blank, no windows, text BGs only, BG0 = 3D only with BG0HOFS 0, mosaic bit allowed only with MOSAIC size
  0 (BG low byte / OBJ high byte of 0x4C; the analyzer now counts it the same way), no 8bpp+ext palette, BLDCNT with
  no 2nd targets and no effect, master brightness off, no capture, OBJs plain only (no affine / semi / window /
  bitmap / extpal 8bpp). Also skipped when stereo is on. split_top is off on GPU frames (engine B worker unchanged).
- Tile cache: key (VRAM address, bpp, palette bank, BG/OBJ); validated by CONTENT every frame (32/64 raw bytes +
  palette-bank FNV hash), re-decoded only if changed -> exact. Atlas 512x1024 RGBA8 (8192 tiles), Morton writes,
  texel = melonDS final 8-bit colour ((c5<<3)|(c5>>3)), alpha 0 = colour index 0. Fully transparent tiles make no
  quad. Per-BG per-frame memo of map entries. Dirty atlas range flushed once per frame. Reset at > 5192 slots.
- Draw: 256x256 VRAM target cleared to backdrop; prio 3..0: BG3..BG0 of that prio (BG0 = 3D texture made from
  NativeFrame, 6->8-bit expand like melonDS), then OBJs of that prio OAM 127..0 (melonDS DrawSprites order), alpha
  test > 0; readback like g3gpu rb_sync (in-frame display transfer + exact wait) + byte swap into display[0].
- Orientation (measured): texture memory row r <-> t = 1 - r/H; DS y 0 -> clip -1 (g3gpu convention).
- PARITY (gpu_compose=3: same frame, same registers/VRAM/OAM/3D layer; old output shown, GPU result compared on all
  256x192 pixels): r60v3 title/intro 499, r60v4 Route 212 walk 1,880, r60vb boot + Route 212 + trainer battle + back
  1,774, r60menu2 X menu / Pokedex / party / bag 987 (latest build) = 5,140 frames, 0 differing pixels. Cross-run
  dump_seq/screenshot diffs are 1-2% from timing nondeterminism (water, position), so the in-process check is the
  real parity test.
- [PERF] Azahar 2126.2, same key scripts, one emulator at a time, relative:
    Route 212 walk: gpu_compose=0 composite 13.7-16.0 ms (r60p0) vs =2 5.5 ms (r60p2; 15.9 in the Cut window).
    5-min battle script: mean composite 26.5 -> 19.9 ms, mean fps 25.3 -> 27.1 (r60b0 vs r60b2; 50-88% of battle
    frames GPU-eligible, alpha-blend frames fall back).
    gpucomp per frame: CPU 3.7 ms (walk 0.55-0.65, 3D layer -> texture 2.2, readback swap 0.83) + GPU 0.49 ms.
- Stage 2 groundwork only (log): non-field (menu) frames are 73-95% GPU-eligible and use ONLY 32-tile-wide BG maps
  (0 of all frames 64-wide) -> no hidden map content to show; native 400-px menus need re-layout per screen (place
  the 256-px layers in a 400-px target + game-specific side content), not a renderer switch. Not started.
- Pure-defaults boot (no config.txt, final build sha1 0d10af56): r60def2 boots to Route 212, 30 fps, composite 14.1 ms.
- Flagged CPU passes: 3D layer -> texture and readback byte swap (256x192 each).
- Watchdog "g3-flush" stall (10 s, intermittent, pre-existing: r57w, r58bat4/8/9, r60b0 with gpu_compose=0) also
  hit r60v3/r60menu/r60vb/r60menu2 (verify mode) and r60b2 (mode 2). New gc-* crumbs show r60menu2 stalled BEFORE
  gpucomp was entered (last gc-done ok in the frame before) -> not inside gpucomp; whether the extra GPU frame
  makes it more likely is open.

## Run 61 (hardware feedback on run 60) — tag run61
Input: test_saves/log_run60.txt (user config stereo_c1=0 wideview=1 gpuscale=1 follower=1 gpu_compose=2) + photos.
All new behaviour behind config keys; new menu entries FOLLOWER SLOT, FOLLOWER TALK, MENU MUSIC, GPU COMPOSE 3D,
BATTLE 3D DEPTH, WIDE HAZE (MItem labels widened to 7). Build sha1 b8e1934f34bd0d5273cce63278691d94f4fc08be.
1. FOG / HAZE in the wide strips (wide_haze=1; gpuscale.c strips() + render3ds wideHaze): the hardware log's field
   frames have BLDCNT 2144 / BLDALPHA 0709 = BG2 haze alpha-blended over BG0 (3D) + backdrop, EVA 9 / EVB 7. The
   strips are 3D-only (G3G_Strip; fog3d's GPU fog is already in them - no seam in r61base column means), so the 2D
   haze stopped at the 256-px centre. Now per frame: k = EVB/16, b = per-channel median of (C - k*3D) over 192 sparse
   samples of the two 32-px edge bands; strips drawn through a 2-stage texture combiner (tex*k + b). No per-pixel CPU
   pass. Only for BLDCNT effect 1 + BG0 2nd target + a BG1-3 1st target. "[WIDE] wide_haze" line every 600 strip
   frames (count, avg us, BLDCNT/BLDALPHA). NOT verified on a hazed map: the test save (map 371, Route 212 rain) has
   BLDCNT 0000 (r61a log: 0 of 600 hazed).
2. FOLLOWER FACING (follower_sidefix=1): HGSS NSBTX frame order is up 0-1, down 2-3, LEFT 4-5, RIGHT 6-7 (preview of
   Pikachu / Girafarig); run 58 drew 4-5 unmirrored for right and mirrored for left -> both side views backwards
   (r61base t043/t047). Now frame = dir*2 + anim, no mirroring. Azahar: left r61a t043, right r61a t047, up r61c
   t040.5, down r61c t039.2 (emu_shots/run61/fol_4dirs_zoom_left_right_up_down.png, before: fol_run60_BEFORE_*).
3. FOLLOWER TALK (follower_talk=1, default 0): A while still + facing the follower -> after 2 frames in which the game
   started no field task (hidden item / sign / script win), P3DS_FolSay draws a word-wrapped HGSS-style box (port 3x5
   font x2, added J Q , ') at the bottom of the top screen for 2.6 s; cleared on movement, task start or leaving the
   field. Nickname from MON_DATA_NICKNAME (charmap digits 0x121, A-Z 0x12B, a-z 0x145 -> upper case). 8 original
   lines. No game message system, no script object, nothing saved. r61a: "[FOL] talk: BOOTYTOOF GAVE YOU A CHEERFUL
   LOOK!" (fol_talk.png).
4. FOLLOWER PICK (follower_slot=0..6, config only): 1-6 = that party slot; empty / egg -> lead. Part of the sprite
   key -> reload within 30 frames after a change.
5. 3D IN BATTLE (stereo_2d): run 55 shifted the right eye's battle BG prio 3 by 2x and prio 2 by 1.5x the 3D shift
   (12 / 9 DS px at full slider, 1x strength) while the battle Pokemon are 3D quads (user log [STEREO]: 4 tris in
   battle) with only +-6*par px per eye (par ~0.3 -> ~3.6 px between the eyes): 8+ px disparity between a Pokemon and
   its background, so with the eyes on one the other is doubled. New default stereo_2d=2 GENTLE: prio 3 = (sd+1)/2,
   prio 2 = (sd+2)/4 (3 / 2 px at full slider), close to the Pokemon's own disparity. 1 = old STRONG, 0 = FLAT.
   NOT confirmed (Azahar reports slider 0; with stereo_fake=100 the key scripts didn't reach a battle; the box was
   then reset, see below) -> conservative default as requested.
6. LAG with gpu_compose=0 (measured, user log): mono 29-30 fps both ways (composite 8.7 ms CPU split vs 4.0 ms GPU);
   slider 1.00: composite 15-18 ms -> 25-26 fps (game logic is frame-locked: 25 fps = 17% slow motion) and run 60
   skipped gpu_compose in stereo. The 35 s gap at f=2700-2760 (game pumps 0) is a HOME/sleep pause, not a stall. No
   input-path change since run 57 (follower = one cheap per-frame function; analyzer only at gpu_compose=1; swizzle is
   GPU-side). Fixes: gpu_compose default 2 (user-confirmed on hardware; run-60 parity 5,140 frames 0 px) and
   gpu_compose_stereo=1: in the field both eyes are GPU-composed (left with NativeFrame, right with
   P3DS_G3RightEye into rawR; 2D layers identical like the CPU field path; any failure -> CPU right eye).
   Azahar, stereo_fake=100, same save + keys: CPU stereo (r61d, gpu_compose_stereo=0) composite 26-28 ms, 20 fps ->
   GPU both eyes (r61c, r61e) 9.0 ms, 29-30 fps, 599-600 of 600 stereo frames composed for both eyes. Eye dumps
   (dump_eyes, r61c eye1) show the 3D world shifted between eyes, rain/2D identical.
7. MENU MUSIC (menu_audio=1): the L+R+Y menu loop calls PSPNativeSoundAdvance(real elapsed us) every iteration (the
   game frame's own sound pump; game logic stays paused). r61a: game pumps 546-565 (expect 516) per 2.7 s while the
   menu was open, 0 underfeed holds (run 60 hardware log: 0 pumps / 344 holds while paused).
Defaults boot (no config, no followers.bin, r61def): gpu_compose=2, wide_haze=1, follower=0 logged; field 29 fps,
composite 5.4 ms. Debug: stereo_fake=N (pretend slider N%), dump_eyes=F (both eyes at F, F+300.. -> eyeN_L/R.bmp).
BOX RESET during the run (~16:57): /tmp, /opt/devkitpro and /opt/azahar disappeared and /workspace rolled back to
the run61b-wip state (commits/tag/shots after it lost, re-made here from surviving runs r61base/a/c/d). The final
.3dsx (b8e1934f) is the run61b-wip build = all run-61 code; r61e / r61def outputs were lost (numbers above are from
the logs read before the reset). Rebuilding needs devkitPro reinstalled.

## Run 62 WIP (tag run62-wip; builds on run61 53ee385)
- Tools: devkitPro reinstalled (dkp-pacman 3ds-dev + 3ds-portlibs; devkitARM r68 / gcc 16.1.0; needed
  `ln -sf /proc/self/mounts /etc/mtab`), Azahar 2126.2 AppImage re-extracted to /opt/azahar (+ libopengl0 etc).
  Clean relink of run61 sources = 5 bytes different from backups/platinum3ds_full_run61.3dsx (build stamp); boots.
- 1) Stereo black sliver (right eye, far left, wide view + slider): cause found in g3soft.cpp GpuRaw: the x reject
  planes (+-1.25w in wide view) were tested BEFORE the per-eye stereo shift (right eye +stereoSd*par DS px), so tris
  just left of the plane that the right-eye shift would bring on screen were dropped -> a <=4 DS px (5 screen px)
  empty column at x=0 of the right eye (mirror case at the right edge of the left eye). Fix stereo_edgefix=1
  (default): reject planes widened by stereoSd/128 clip units per w while stereo is on. Cost: one add per frame, a few
  extra tris per eye at the edges. NOT verified on hardware (Azahar reports slider 0).
- 2) Stuck walk animation in fog: REPRODUCED in Azahar, cause NOT found yet. Save warp: run22 save with Location
  (normal block 0x1280: map 363, warp -1, x 564, z 543) + player MapObjectSave (0x2858: xInit/x, zInit/z) patched,
  CRCs fixed (/tmp/r62/r210n.sav, not committed). Route 210 North fog shows (BLDCNT 2144, fog3d frames).
  Starting IN the fog: player turns/animates fine (emu_shots/run62/fog_first_run_turns_ok.png). Walking down into
  map 362 and back up into 363 (fog weather starts): while walking left the player keeps its back (up) frame,
  the follower trails correctly (emu_shots/run62/stuck_facing_up_fog_vs_fog3d0.png). Same with follower=0,
  follower_talk=0, wide_haze=0, fog3d=0, tex_postvblank=0, wideview=0 (emu_shots/run62/stuck_postvblank0_cpu3d_wide0.png;
  gpu3d=0 run showed a black screen). So NOT caused by follower_talk/slot/sidefix/wide_haze. Next: log the player
  object facing dir + the player's billboard texture VRAM address/decode per frame across the 362->363 change.
- tools/emu_smoke.sh: PRESS_BG=1 runs key holds in the background so screenshots land during a hold.
- 3) Bottom-screen settings app / game-font text boxes: not started.

## Big build (multi-run delivery; checklist kept here across resumes)
Legend: [x] done+verified  [~] in progress  [ ] not started
- [x] 1. Fog stuck walk animation: fog_animfix=1 (run 64, verified in Azahar with repro save)
- [x] 2. Clean launch (run 64: quiet boot = black, fps_counter default 0, plain error screen, missing-ROM check)
- [x] 3. Boot credits splash (run 64: credits_splash=1, splash_ms=2000, game font from ROM, A skips)
- [~] 4. Game font + text-box frame (run 64: follower_talk mixed case, toasts, save box + yes/no, splash, settings app done; old L+R+Y list (settings_app=0) and corner save overlay still port font)
- [x] 5a. Settings touch app on the bottom screen (run 64: settings_app=1, L+R+Y, tabs, version string; verified in Azahar)
- [~] 5b. Touch Start-menu app (run 64: "Menu" tab in the settings app opens the real Pokemon/Bag/Pokedex/Trainer card/Save/Options; entry is still L+R+Y only (no free bottom-screen spot without covering the Poketch) -> next run)
- [~] 5c. Touch party + bag on bottom screen (run 64: reachable by touch via the Menu tab -> the game's own party / Bag screens; a native touch-only party/bag layout -> next run with item 6)
- [x] 5d. Live map app (run 64: Map tab = place name + wild Pokemon grass/surf/Super Rod with levels from the game's encounter data and species names from the ROM; items not listed)
- [x] 5e. Cheats page (run 64: Cheats tab, cheat_no_encounters=0; more cheats can be added as rows)
- [ ] 6. (moved to the NEXT run by the user) Full-width 400 px menus, then battles (native layout; gpucomp alpha blending for battles)
- [x] 7. Toggles: repel_prompt=1, summary_ivs=1 (Y on Skills page), reusable_tms=0, fast_text=0 (run 64). Visible wild Pokemon NOT done (needs map-object spawning + encounter hand-off; next run). reusable_tms real teach untested (hardware).
- [x] 8. Polish: lid_sound=1, follower_ballpop=1 (voxel sprites skipped by the user)
- [x] F1. follower_gap=1: follower farther back / offset by sprite size (big species extra), keep follower_behind
- [x] F2. Area follower dialogue: lines by map/area (towns, routes, caves, lakes, legendary spots), weather, time of
      day, species/type; port-side text table, game font + box
- [x] F3. follower_items=0 (default off): follower sometimes finds a common item after talking, added only through the
      game's own Bag_TryAddItem path (as script additem / hidden items), cooldown
- [x] F4. follower_cry=1: species cry (with form) via the game's Pokemon_PlayCry when you talk to the follower
- [x] (run 62) stereo_edgefix (code; hardware check pending)

## Run 63 WIP (tag run63-wip; builds on run62-wip def2a37)
- Save copy: test_saves/r210n_edit.sav (run22 save warped to Route 210 North 564,543). Repro config:
  test_saves/cfg_fogtrace_T1.txt; keys: title A x6, Start x2 (journal), Down 3 s (into map 362), Up 3 s (back into 363),
  Left 3.5 s (PRESS_BG=1 tools/emu_smoke.sh).
- New debug toggles (all default 0, log_level=2): tex_trace (g3soft: per-frame small-texture add/remove/content diff),
  player_trace (p3ds_follower.h: player status/facing/anim, billboard state + gfx sequence), vram_trace (main3ds.c:
  VramTransfer_Request/Process counts), heap_watch=<decimal addr> (main3ds.c: Heap_Alloc/Free covering an address).
  New link wraps: VramTransfer_Request/Process, Heap_Alloc/AllocAtEnd/Free/FreeExplicit (pass-through, ~0 cost).
- Findings (all in Azahar, reproducible):
  * NOT the port texture cache, NOT the display-list cache (g3cache=0 still stuck), NOT follower/haze/fog3d.
  * Game logic is fine: facing/move dir change (PLTR), billboard ACTIVE, anim# and frame advance.
  * The player's billboard gfx sequence table (startFrame/textureIdx/plttIdx, one 132-byte block at 0x9bf150 in
    heap 65 via ResourceHeap_AllocItem) is FREED at the first field load (ResourceHeap_FreeItem, from
    ov5_021ECE40.c ov5_021ED63C = "free anim resources no active map object uses", called from line ~1027) while the
    player billboard still points into it; at the 362->363 seamless map change ResourceHeap_AllocItem reuses the block
    (12 bytes) and overwrites startFrame[1..7] (0 4 8 12.. -> 0 30 256 256 18002 0 776 0) -> every walk frame maps to
    the same texture index -> frozen walk cycle, wrong facing. (A use-after-free in the game data path; the
    exposure is the port, the trigger is any later alloc that lands in that block.)
- Next: find why ov5_021EDAB4 does not count the player for that resource id (log id, player gfx modelType,
  ov5_021EDD2C(0)); fix in the port (toggle fog_animfix=1): keep the resource while the player billboard references it
  (skip the free when the id equals the player's), verify with T1 repro (TEXTR offsets must cycle while walking left).

## Run 64 WIP (tag run64-wip)
- Item 1 DONE: fog_animfix=1 (default). overlay005 ov5_021ECE40.c: ov5_021ED63C/ov5_021ED778 now skip
  ResourceHeap_FreeItem when the player's billboard (sub_02062AF0(player)+4) still points into that item
  (gfxSequence startFrame/textureIdx, anims, animTexture, texture). Patched source: patches/run64/.
  Verified with test_saves/r210n_edit.sav + cfg_fogtrace_T1.txt: log shows "[FIELD] fog_animfix: kept resource 1",
  PLTR3 startFrame stays "0 4 8 12 16..." after 362->363, TEXTR player texture cycles 2a120/2a320/2a520 while walking
  left in the fog. Shots: emu_shots/run64/fogfix_walk_*.png. Cost: a loop over the item list only on map change.
  Side effect: the kept 132-byte block is freed normally once the player no longer references it.
- Item 2 DONE (clean launch): quiet boot no longer prints "Loading..." (plain black until the first game frame);
  fps_counter default 0 (L+R+START / settings still turn it on); errors in quiet mode show a plain screen (title +
  red box, word-wrapped, no checklist; verbose_boot=1 keeps the old checklist); the ROM is checked up front ->
  "ROM not found. Put your Pokemon Platinum (US) ROM at sdmc:/3ds/platinum3ds/platinum.nds" (emu_shots/run64/error_norom.png).
  Before this a missing ROM ended in "abort() ... OS_Terminate". All detail stays in log.txt.
- Item 3 DONE (credits splash): render3ds.cpp drawSplash over the first game frames (the game is black/loading then,
  so no wait is added): "Patch by" / line / "Unown Collectables & 3DS Homebrew Dev" in the game's message font,
  fade in 300 ms / out 250 ms, A skips (log "[SPLASH] skipped (A)"). credits_splash=0 disables, splash_ms=300..5000.
  In Azahar it overlaps the game's copyright fade on the bottom screen; the top copyright appears right after.
  Shot: emu_shots/run64/splash.png.
- New platform/p3ds_gamefont.c (start of item 4): reads /graphic/pl_font.narc from the user's ROM at boot (NDS FNT/FAT
  + NARC parse with stdio, ~33 KB x 2 fonts, member 0 system / 1 message), ASCII->charcode, draws 16x16 2bpp glyphs
  (1 = text, 2 = shadow) into port overlays: P3DS_GameFontLoad/Ready/Width/Draw. Nothing ROM-derived in git.
- Item 4 (mostly done): p3ds_gamefont.c also loads the game's message-box frame (pl_winframe.narc message_box_NN
  tiles 2+N / palette 25+N, ui_frame=0..19, default 0) and draws it like DrawMessageBoxFrame (2 tiles left, 3 right).
  ui_gamefont=1 (default; 0 = old port font). Now in game font + frame: follower_talk (mixed case, nickname case kept,
  same place as the game's field message window, emu_shots/run64/follower_talk_gamefont.png), hotkey toasts, the
  save box ("Would you like to save the game?" / "Saving..." + spinner / "Your progress has been saved!") and its
  Yes/No box (emu_shots/run64/save_ask_gamefont.png). Port-font leftovers: the run 55 list menu (only with
  settings_app=0), the small corner save overlay outside the field, FPS digits.
  Harness note: in Azahar the 2nd A on the game's Yes/No did not register (same with the run 63 build), so the
  "Saving..."/"saved" states were not screenshotted this run.
- Item 5a DONE: settings_app=1 (default): L+R+Y opens a touch settings app on the bottom screen (Poketch is back
  on close; the game is paused like the old menu). Tabs Display / Perform. / Gameplay / Cheats (cheats: placeholder,
  nothing yet), 2 x 5 cells in the game font: label, value in red, arrows; tap left third = previous value, rest =
  next; D-pad + A (next) / X (previous), L/R = tab, B / START / L+R+Y close. Changed cells get a green dot; changes
  apply on close and are saved to config.txt (same getters/setters as the list menu). Top screen: dimmed game, title
  box with the version string "Platinum 3DS run64 (<build date>)" and a help box. Buffers malloc'd only while open
  (a 590 KB static BSS version caused a g3-flush stall at boot in one run). Bottom picture re-uploaded after close
  (P3DS_GpuInvalidateBot). Shots: emu_shots/run64/settings_app_*.png.
- Item 7 (one toggle): fast_text=0 (default off) / 2 / 3 / 4 = characters per frame for the game's text printer
  (text.c SysTask_RunTextPrinter renders extra characters only while the printer is in the plain "print character"
  state; any wait, scroll or button prompt stops the loop, so no text page can be skipped by a held A). Settings app
  Gameplay tab "Fast text" (Off / 2X / 3X / 4X). Azahar: with 4X the save question's Yes/No is already up 0.6 s after
  the A press, without it not yet (emu_shots/run64/fast_text4_vs_off.png; the port save box covers the text itself).
  Patched source: patches/run64/text.c.
- Item 5b (first version): settings app tab "Menu" with 6 big cells (Pokemon, Bag, Pokedex, Trainer card, Save,
  Options). Tapping one closes the app and asks the field (p3ds_follower.h, game thread, only when no field task runs)
  to open the game's OWN Start menu (X injected for 2 pad polls via psp_ctru.c P3DS_InjectKeys) with the cursor
  preset through the game's own fieldSystem->menuCursorPos; start_menu.c reports whether that option exists in this
  menu (P3DS_StartReqFound), and only then A is injected 10 frames later. If the option is hidden (no Pokedex yet...)
  the Start menu just stays open. Busy field for 3 s -> request dropped (logged [STARTAPP]). Verified in Azahar:
  tap Bag -> the real Bag opens (emu_shots/run64/menu_tab*.png). Patched sources: patches/run64/start_menu.c,
  p3ds_follower.h. Not yet: Map (Town Map is a key item, not a Start-menu entry), opening it without L+R+Y first
  (needs an always-available bottom-screen entry point next to the Poketch), accents now drawn (Latin-1 0xE9 -> game charcode 0x188, "Poke-acutemon").
- Run 64 summary: items 1, 2, 3, 5a done and verified in Azahar; 4 mostly done (all new port UI in the game font +
  frame; FPS digits and the "1/22" page counter of the old list stay port font); 5b first version (Menu tab);
  7 one toggle (fast_text). Next run: 5b entry point without L+R+Y (small always-there touch tab beside the Poketch),
  5c touch party/bag, 5d live map app, 5e cheats, then 6 (400 px menus / battles).
  Backup: backups/platinum3ds_full_run64wip.3dsx. github_export untouched; nothing pushed or copied to the PC.
- Item 7 more: reusable_tms=0 (default off) / 1: teaching a TM no longer removes it (party_menu callbacks.c
  TeachMove; HMs were never consumed). Settings app Gameplay tab "Reusable TMs" (toggle + config write verified in
  Azahar; the actual TM teach was not run in the emulator this run). Patched: patches/run64/applications__party_menu__callbacks.c.
  Running indoors: Platinum's player_move.c has no indoor restriction (running shoes work everywhere already), so
  no toggle is needed; noted as N/A.

## Run 64 part 2 (follower requests)
- F1 follower_gap=1 (default): render-only offset away from the player along the follow direction: 32-px sprites
  +4 units (+8 when north of the player, where the sprite rises over the head), 64-px +8 (+12 north), big/tall species
  list (Girafarig, Onix, Steelix, Wailord, Gyarados, Lapras, Snorlax, legendaries ...) +8 more. Trail/logic position
  and follower_behind depth unchanged. Azahar: Girafarig now clear of the player's head (emu_shots/run64/
  follower_gap_on.png vs follower_gap_off_before.png).
- F2 follower_area_talk=1 (default): platform/p3ds_foltalk.c original text table: 17 towns x 2 lines, categories
  (route, sea route, lake, legendary spot, forest, cave via MapHeader_IsCave, Pokemon Center, Mart, Gym, Great Marsh,
  Old Chateau/Lost Tower, League, Galactic, house interiors, snow), weather (rain/snow/fog/sandstorm/ash, outdoors),
  time of day (morning/evening/night), type and species flavour. Area table platform/p3ds_areas.h generated by
  platform/gen/gen_areas.py from the decomp's map header NAMES (no ROM data). Drawn in the game font/box.
  Azahar Route 210: 3 talks -> 3 different route lines (emu_shots/run64/follower_area_talk_*.png).
- F3 follower_items=0 (default off): after a talk, if 120 player steps passed since the last find (port memory), 1 in
  3: one of Potion, Antidote, Paralyze Heal, Awakening, Poke Ball, Great Ball, Super Potion, Escape Rope, Repel, Oran /
  Pecha / Cheri Berry, Honey is added with the game's own Bag_TryAddItem(SaveData_GetBag(save), id, 1, HEAP_ID_FIELD1)
  (the call ScrCmd_AddItem, hidden items and berry picking use), so the bag data is exactly what vanilla writes.
  Full pocket -> nothing found, nothing said. Text "<name> found a Potion! You put it in your Bag."
  follower_items_test=1 = debug, no cooldown/chance. Azahar: "[FOL] item: ... found item 18 via Bag_TryAddItem"
  + message (emu_shots/run64/follower_item_found.png). Not checked: the saved file after a real save (harness Yes/No).
- F4 follower_cry=1 (default): Pokemon_PlayCry(mon) (species + form, the game's own cry path) on each talk. Azahar log:
  "[AUDIO-SEQ] seqEx 2 ... bank=203 -> ok" = the game's cry sequence for Girafarig; BGM keeps running (no audio
  capture in the harness, so not listened to).

## Run 64 part 3 (final; tag run64)
User plan change: item 6 (400-px menus/battles) moved to the next run; voxel sprites skipped; quick smoke tests only.
All new features behind config keys (README_settings.txt), all save-safe:
- follower_ballpop=1: after >=20 hidden frames (warp/door/cutscene/load) the follower's first step plays the game's
  SEQ_SE_DP_BOWA and the quad grows in over 12 frames (render only). Azahar: log "[FOL] ball pop after 95 hidden frames".
  Shot emu_shots/run64/follower_ballpop_grow.png (hard to see in a still).
- lid_sound=1: APTHOOK_ONSLEEP/ONWAKEUP -> game thread (P3DS_FrameHook) plays SEQ_SE_DP_CLOSE2 / SEQ_SE_DP_WIN_OPEN, only in
  the overworld. Azahar cannot close a lid -> untested.
- summary_ivs=1: summary main.c copies IVs/EVs (Pokemon_GetValue) into port globals in SetMonDataFromMon and flags the
  Skills page each frame; Y toggles a game-font box. Shot emu_shots/run64/summary_ivs_evs.png (Girafarig 23/12/7/17/27/18).
- repel_prompt=1: Repel_UpdateSteps flags the wear-off; 8 frames after the game's message task ends, a port Yes/No
  (game input held while asking, Up/Down/A/B); Yes = Bag_TryRemoveItem + repel steps = Item_LoadParam(EFFECT_PARAM) +
  SEQ_SE_DP_CARD2 (same as the Bag). Debug repel_prompt_test=1 sets 2 steps once. Azahar: yes -> "used item 79 -> 100
  steps", no -> declined. Shot emu_shots/run64/repel_prompt.png.
- Settings app tabs renamed Video / Speed / Game (2 pages) / Cheats / Menu / Map. Game page 2: Repel prompt, IV/EV on
  summary. Cheats: No wild encounters (cheat_no_encounters, wild_encounters.c Try*Encounter early return: walking,
  surfing, Sweet Scent, mud). Shot emu_shots/run64/cheats_tab.png; config write verified.
- Map tab: place name from the decomp map header identifiers (platform/gen/gen_areanames.py -> p3ds_areanames.h, no ROM
  data) + wild Pokemon (grass, surf, Super Rod, level ranges) from MapHeaderData_GetWildEncounters and species names
  from the ROM's message bank, snapshot on the game thread when the map changes and the player has stood still 30
  frames (snapshot costs ~12.6 ms once, never mid-walk). Shot emu_shots/run64/map_tab.png (Route 212 South).
- Azahar note: the known intermittent watchdog "g3-flush" stall (pre-existing since run 57) hit 5 of ~16 boots this
  session, all before the overworld (frame 194-628, title/intro); a rerun always booted. Not reproduced as a code
  change effect (the HEAD-equivalent build booted, the same new build booted on rerun). Needs a hardware check.
- Not done (next run): item 6, visible wild Pokemon, touch-native party/bag, an L+R+Y-free entry to the app.

## Run 65 WIP (full-width menus/battles, Map tab grid, boot freeze fix; builds on run64 bc56149)
- **wide_menus=1** (default 1, render3ds.cpp `wideNative`): with wideview + 1.25x, every non-overworld screen (bag, party,
  summary, battles, title...) fills 400 px. CPU renderer for DS x -32..-1 / 256..287: per text BG and tile row the edge
  period (1/2/4 tiles) is detected from the map entries at the screen edges and those DS tiles are repeated (not mirrored,
  not blurred); the back-most layer borrows the nearest periodic row so panels' backgrounds continue; 64-wide maps use the
  real wrap; ext palettes + BLDCNT brighten/darken handled; master-brightness fades applied in gpuscale strips().
  FNV input hash -> unchanged frames reuse. Cost 0.3-1.1 ms/frame avg in Azahar (was 2.3 ms per-pixel first try).
  Same 1.25x as the field, so no size jump between field and menus. OBJs are not continued. Field keeps G3G strips.
- Battles use the same path (no gpucomp alpha blending needed for the fill; gpucomp alpha blending = NOT done, only
  a perf item).
- **encounter_test=1** (debug, default 0, overlay006 wild_encounters): every step = wild grass battle (for testing).
- **Map tab grid**: game-thread snapshot (p3ds_follower.h p3dsMapSnap, 15 us, once per map/time-of-day after 30 still
  frames) builds per section (grass with day/night slots 2-3, surf, old/good/super rod) species / level range / summed
  slot-rate chance, sorted by chance, plus icon member/palette via PokeIconSpriteIndex/PaletteIndex. Icons are read
  from the ROM (pl_poke_icon.narc, P3DS_IconLoad in p3ds_gamefont.c) by the settings app on first view (game paused,
  ~150 ms in Azahar for 13 icons). Grid 4 per row, "45% 23-26" under each icon, tap panel = next page. Nothing
  ROM-derived committed.
- **Boot freeze ("game stalled ... main last: b-wait") root cause found and fixed**: early_b and the frame job both
  signalled the ONESHOT LightEvent bStart; if both landed before the engine-B worker woke they collapsed, the worker ran
  only the early part and the main thread waited for bDone forever. Crumbs proved it (w-early, w-early-sig, no w-job;
  main at b-wait). Fix: pending-work counter (bPost / bPend) -> lossless hand-off. Stall crumbs left in (cheap).
- Commits: 26112d5 (wide_menus), 82322f8 (wide battles, encounter_test, crumbs), b7891d6 (Map grid + freeze fix),
  4003c11 (icons loaded in settings app), + docs/version commit (tag run65-wip). Azahar: 8 boots after the fix, 0 stalls (before: ~6 in 18). Build sha1 25736936b341d462c78f8fbdf13eebae92012617, 5,914,188 bytes.
- Screenshots: emu_shots/run65/wide_menus_party.png, wide_menus_summary.png, wide_battle.png, map_grid.png,
  map_grid_page2.png.
- **Run 65 final (tag run65):** lid_sound now asks for the Poké Ball sounds, recall SEQ_SE_DP_SUIKOMU (decomp
  SEQ_SE_DP_SLOT02_sseq_1 = 1797) on close and send-out SEQ_SE_DP_BOWA2 (SEQ_SE_DP_SLOT01_sseq_1 = 1798, the battle send-out
  sound in battle_display.c) on open. Both are in BANK_SE_BATTLE, which isn't resident in the overworld: Azahar
  [AUDIO-SEQ] shows 1797/1798 FAIL, then the fallback SEQ_SE_DP_BOWA (1534, BANK_BASIC, the Poké Center ball sound) plays.
  Playing the real ones would need the battle SE bank/wave arc loaded into the field sound heap (risky; not done). SE players
  only, so the BGM isn't stopped. Debug lid_test=1 fakes a close/open. The version string is run65 (P3DS_VERSION).

## Run 66 (FPS regression fix, touch shortcuts, GitHub export prep; tag run66)
- **FPS regression cause (runs 64-65):** `P3DS_FollowerFrame` (p3ds_follower.h, repel_prompt block, run 64 c8d4642) called
  `P3DS_CfgInt("repel_prompt_test", 0)` on EVERY overworld frame. P3DS_CfgInt opens and parses config.txt on the SD each
  call. Hardware log_run65: field g3soft (game thread) 31.7 ms vs 22.6 in run61 (G3PROF GetTexture/GpuRaw/G3G_End
  unchanged, so the +9 ms was outside 3D); 3D field 21.9 fps vs 29.0. Azahar (same config): field 18.2 fps / g3soft 41.5 ms
  (run65) -> 28.4 fps / 27.0 ms (run66). Fix: cache it (static), also cache vram_trace (was read every 2 s).
  Guard: P3DS_CfgInt now counts reads + time and logs "[CFG] N config.txt reads in the last 10 s" when any happen, and the
  first 60 reads by key ("[CFG] read <key> (us)"), so a per-frame reader shows up in any log. All other config reads are
  one-time (audited). wide_menus (0.75 ms, non-field only), Map snapshot (19-34 us per map), bPost/bPend and crumbs were not
  the cause. The boot-freeze fix (bPost/bPend) is kept. Note: the user's config has log_level=2 (writes all detail lines to
  SD, e.g. 400 [VDUMP] lines); log_level=1 is the normal setting.
- **settings_touch=1** (default): overworld only, touch and hold 0.3 s in the Poketch's dead corners (no Poketch screen or
  button there: DS screen 16..207 x 16..175, buttons x 224..255 y 32..160): top-left = settings app, top-right = Pokemon,
  bottom-right = Bag. Party/Bag go through the existing P3DS_StartReq path (the game's own Start menu, then its real
  screens; the party bottom screen is the game's own touch ball layout, the Bag bottom screen its own touch pocket wheel),
  so save-safe. In 1:1 scale the black border quadrants are used. Azahar: all three verified (emu_shots/run66).
- **GitHub export** (github_export, committed locally, NOT pushed): Runs 62-66 platform sources (+ p3ds_gamefont.c,
  p3ds_foltalk.c, p3ds_areas(.h)/areanames.h + their gen/ scripts that read only decomp map-header identifiers), README_settings,
  Makefile, emu_smoke.sh, PROGRESS, and work_patches/overlays regenerated against pokeplatinum for all 16 port-modified
  overlay sources (new: bag main, party callbacks, summary main, ov5_021ECE40, repel_step_update, wild_encounters,
  start_menu, text) + p3ds_follower.h; every patch verified to reproduce the work-tree file from the pristine source.
  Audit: no ROM, saves, builds, images, sound or binary data (export .gitignore + manual list check; largest file
  render3ds.cpp 112 KB of code).

## Run 67 (config cache, cheats, hidden Debug tab; tag run67)
- **Remaining [CFG] reads (log_run66: 9-23 config.txt reads per 10 s, ~13 ms each):** overlay code caches settings in
  overlay statics (repel_prompt_test, player_trace, fog_animfix, encounter_test, follower_items...), and overlay data is
  reset on every overlay reload (map change, battle), so each reload re-read config.txt from the SD. Fix: P3DS_CfgInt
  now keeps config.txt in RAM (loaded once, LightLock, invalidated by P3DS_CfgWrite / saveScale). [CFG] now counts only
  real SD loads. Azahar: one load (161 bytes) per boot.
- **Cheats tab** (all default 0, config cheat_*): cheat_money (999999 in the field), cheat_catch (CalcCatchShakes ->
  success), cheat_shiny (CreateWildMon -> the game's own CreateWildMonShinyWithGenderOrNature), cheat_eggs (party egg
  cycles 0 + daycare step counter 250 -> the normal hatch event within ~5 steps), cheat_exp (x2/x3 after the game's own
  multipliers), cheat_candy (Bag topped up to 99 Rare Candy via Bag_TryAddItem). Field cheats run every 30 frames, only
  without a running field task. Not done: walk through walls (time). Instant text = existing fast_text 4X.
- **Speed tab removed** -> hidden "Debug" tab (SELECT inside the settings app toggles it), keys unchanged.
- Fog 3D dip analysis: not done (next).

## Run 68 (Snowpoint City crash fix; tag run68)
- Crash: GF_ASSERT in ov5_021D6EC8 (weather particle setup, overlay005/ov5_021D5EB8.c) on entering Snowpoint (snow).
  The decomp has two variants of the weather particle buffer: SDK_BUILD_ARM = s32[10], otherwise s64[10]. This port
  builds the non-SDK_BUILD_ARM path, so snow/blizzard pass sizeof(s64)*10 (80) / *8 (64), but the size check was the
  unguarded `param1 <= sizeof(s32)*10` (40) -> assertion. Not caused by any port toggle (reproduced with follower=0,
  fog_animfix=0 and plain defaults). Fix: check/clear against sizeof(param0->unk_0C) (the real buffer, 80 bytes here).
- New: [ASSERT] log line (always, log_level 1) with the caller address of any GF_ASSERT failure (resolve with
  arm-none-eabi-addr2line -f -e platinum3ds_full.elf <addr>) + crumb dump; Makefile --wrap=ErrorHandling_AssertFail.
- Azahar: snowpoint_crash.sav walked into Snowpoint and around with the user's config and with defaults: no assert.

## Run 73 (2026-10-23)
- Spear Pillar cutscene abort "[NATIVE-RENDER] vertex overflow" (overlay 100 submits >6144 vertices/frame): g3soft/g3null vertex buffer 6144 -> 24576. Emu: cutscene start + Mars/Jupiter double battle reached without abort; red chains not reached in emu (battle automation stuck).
- wide_weather copies only grouped weather OBJs (>=4 same palette/size/nearby tiles): no cursor / lone sprites.
- wide_edgefill: off indoors (MapHeader_IsOutdoors via follower frame), outdoors only extends pixels that repeat one 32-px period in (no NPC/player copies).
- wide_menus: only the back-most BG layer (and 64-wide layers) continue into strips (no '0' rows / cut panels).
- Strips hold the last steady BG scroll during attack screen shake.
- wild_vis (default 0): visible wild Pokemon (followers.bin sprites in texture slots 1..4), grass-only spawn from the map's table, wander, contact -> normal wild battle. Untested in emu.
- dbg_onehit (test only), setup_box.sh (reinstall devkitPro/Azahar after a box reset).

## Run 74 (2026-10-23)
- Settings app: new tabs Pal (follower toggles), Wide (wide_weather/edgefill/menus/fill), Extra (wild_vis*, hm_free, save_backup, log_max_kb). Generic rows write config.txt; most apply after a restart.
- save_backup=1 (default on): at launch platinum.sav is byte-copied to platinum.sav.bak1 (.bak2/.bak3 rotate). The save itself is never written by the port.
- log_max_kb=1024 (default): log.txt rolls over to log_old.txt.
- hm_free=0: Party_HasMonWithMove also accepts a party mon that can learn the HM (game's CanPokemonFormLearnTM); badges still required. Untested in emu.
- 8c audit: DWC_ (141) and WM_ (35) symbols from the decomp are linked; no 3DS network (soc/ac/uds) is linked. No network code written.

## Run 75
Off by default; each has a settings-app row; nothing writes to platinum.sav (checked that the emulator save matches byte for byte).
- cheat_pp: your moves cost no PP (battle_controller_player.c, the PP-deduct step).
- cheat_speed: B-run uses the bike-speed walk action (player_move.c NormalOverworld).
- cheat_walk_walls: hold R to pass walls (PlayerAvatar_CheckCollision; warps and jumps are kept).
- cheat_time: time-of-day override in TimeOfDayForHour (rtc.c); view only.
- catch_overlay: snapshot of the wild foe at InitCommandSelection -> P3DS_CatchInfo; "Ball" tab in the settings app; Y opens it in wild battles.
  Ability/species names come from the decomp constant lists (p3ds_abilnames.h). Odds use the Gen 4 formula estimate (Poke/Great/Ultra/Quick/Timer/Nest).
- battle_anim_speed=1: Task_SetMoveAnimation skips state 4 -> 6 for animMode 0, except for moves whose animations change sprites.
- Tabs: a paged tab's label shows only "1/2"; the Catch tab is named "Ball".
- Tested: settings smoke (Cheat pages, Ball tab "Only in wild battles."), 30 fps, save unchanged. Not tested: in-battle Y overlay, the anim skip, walk-walls and speed (wild-battle setup failed in the emulator).
- Left: Underground bottom-screen fix, fast HP drain (battle_anim_speed=2), Dusk/Net ball odds.
- SHA1 9977c2205c556fc747e05d3767f35aafb8ab5fbc

## Run 76
- Spear Pillar black screen (Dialga/Palkia appearance): the game's motion blur (ov6) shows VRAM C in "VRAM display" mode
  and fills it by display capture (2D+3D blended with the previous capture). Capture was ignored, so the screen stayed black.
  Now CaptureCnt (0x4000064) is passed to the melonDS soft renderer's own DoCapture (blend A/B, all sizes), the right-eye
  copy never captures, and the enable bit is cleared after the frame. display_capture=0 = old, 2 = run-76a minimal copy.
  Reproduced in the emulator from test_saves/coronet_1023 (Mars/Jupiter battle -> Cyrus -> Palkia line -> black).
- Distortion World: dw_depth (default 2, eased stereo multiplier), dw_debris, dw_unease, dw_cry (off by default).
  Spear Pillar / Distortion World count as outdoor for wide edge fill. Emulator shot of Spear Pillar shows filled strips.
- Underground bottom screen: bottom reuse is dropped whenever a tracked VRAM page in banks C/D/H/I gets a newer
  generation (bottom_vramgen=1, logs "[2D] bottom changed by VRAM page N"). Not reproduced in the emulator (no save there).
- battle_anim_speed=2: also 3x HP bar drain. Ball tab: Dusk and Net odds; hunt_counter (hunt.txt, never the save).
- tod_tint, item_ping (Poketch dowsing blip within 2 tiles), [PERF3] log line (worst frame, bottom reuse, map).
- Skipped: fast-forward key (user said skip), Poketch touch app, fog slowdown (needs a hardware log), camera roll.

## Run 77
- touch_menu (default 0, Extra tab "Touch quick menu"): hold the bottom-left corner 0.3 s in the field -> settings app
  opened on its Menu tab (game font/boxes; Pokemon, Bag, Pokedex, Trainer card, Save, Options through the real Start
  menu, save-safe; Map tab one tap right). Boot + open checked in the emulator.
- dw_roll (default 0, Wide tab "Distortion camera roll"): on a Distortion World map change, damped row-shear tilt
  (exp(-t/140ms)*cos, 400 ms, direction by map order) on both eyes plus up to +30% stereo depth. Untested in the emulator (no DW save).

## Run 78
- bottom_strip (default 0, Extra tab "Bottom button strip"): overworld strip of 4 game-font buttons in the bottom border (DS y 177..190, x 8..207): Menu -> Menu tab, Wide -> runtime wide view toggle (like L+R+X, not saved), Ball -> Ball tab, Set -> settings app. main3ds touch read drops taps in that band while the strip is on. Boot + Menu tap checked in the emulator.

## Run 79
- `make cia` (platinum3ds/full): same ELF as the 3dsx, makerom + bannertool; rsf adapted from GabeSud0/pokeplatinum-3ds (124MB SystemModeExt, 804 MHz, L2 cache, title ID 000400000F7D3E00). Placeholder icon/banner/silent tune generated at build time (ImageMagick), nothing from the ROM. Same sdmc:/3ds/platinum3ds/ save/config/log as the 3dsx. CIA boot not tested in the emulator.
- Pre-push audit runs the borrowed tools/check_ci_artifacts.py plus a game-data magic scan (NARC/SDAT/BMD0/BTX0/RGCN/RLCN).
- User save/log/config of 2026-10-24 copied to test_saves/latest_1024 (box only).
- Skipped: porting their bottom-screen icon menu (port_lowermenu.c), over budget.

## Run 80
- text_hold_b / text_auto (Extra tab, default off; render_text.c): B held = instant print + advances a finished text box; auto = advance after 45 game frames (~1.5 s). Only the text-box wait (TextPrinter_Continue); yes/no and menus untouched. Boot checked; behaviour untested.

## Run 81
- speedcheck=1 (default; platform/speedcheck.c, called from main3ds.c after the save backup): at boot on a New 3DS, logs the result of the ptm:sysm 804 MHz + L2 request and times three small jobs (integer / 64-bit multiply / double divide) on core 0, 2 and 1, then a verdict line ("core 0 runs at 804 MHz" / "~268 MHz"). Until now "speed-up on" was only the call being made (run 29), never measured. Idea and job loops adapted from GabeSud0/pokeplatinum-3ds main_3ds.c (credited in THIRD_PARTY.md).
- Core 1 reads -1: a thread can only be created there after APT_SetAppCpuTimeLimit, which the renderer requests later. Not changed on purpose (the check only observes).
- NOT built or run: no devkitARM on the box that wrote it; host syntax check only with stub headers. Look for the [perf] lines in the next hardware log.

## Run 82 (HOME Menu art from GabeSud0/pokeplatinum-3ds)
- `make cia` now makes the icon and the animated 3D banner (portal, title-screen Giratina idle, logo, chime) from your ROM
  (default `baserom/platinum_us_rev1.nds`, or `ROM=...`): tools/home_menu_art.py + tools/banner3d (from their repo, GPL-3.0,
  see THIRD_PARTY.md). `BANNER=flat` = flat banner from the ROM's DS icon; no ROM = the run-79 placeholder art; a failed
  3D banner falls back to flat. Needs Python 3.12+ and git (pycgfx fetched once into build/pycgfx). Nothing made is committed.
- Tests: `python3 -m unittest discover -s tools -p 'test_*.py'` (13 pass in the cloud, pycgfx part included). Not built
  into a CIA here (no devkitPro/ROM). HOME Menu caches banners: delete and reinstall the title to see the new one.

## Run 83 (sharper 3D experiment)
- res3d_test=2 (default 0): re-enables the run 46-49 "sharp 3D" path (3D at 2x, HW 2x2 downscale, 24-bit depth in linear RAM) for a hardware test. It is smoothing only (4x supersampled edges), not extra texels. Disabled since run 52 for garbage on hardware; run 49 fixed the 16-bit depth cause but was never re-tested. Look at [3DRES] / [PERF] lines.
- Real sharpness (3D drawn at the screen's own 400x240 under a 2D overlay, as GabeSud0's port does) is a bigger project, not started: needs the 2D composite to leave 3D pixels clear and a hi-res 3D target (400x240 needs ~0.7 MB VRAM, far less than 2x).

- wide_layouts (default 0, run 83 WIP): platform/wideapi.c = the game-side API of GabeSud0's wide-layout patches (WideScreenParts, WindowAdded/Removed/Anchor/Framed, WideSplitScreen...). Records only; with wide_layouts=0 NitroPort_WideScreenExtra() is 0, so patched game code behaves like the DS. Renderer part (320-wide 2D compositor) not built yet.

## Run 84 (wide layouts: the display engine)
- platform/widecomp.c (+ widecomp.h): draws a screen that has a wide layout (GabeSud0's parts, recorded by wideapi.c) across
  DS columns -32..287. BGs that move with the sprites (WIDE_OBJS dx, 0 without one) stay in the normal melonDS pass, which is
  placed that far right; the other BGs are switched off for that pass and drawn by WC_Compose per line from per-line column
  maps (WIDE_MOVE / WIDE_FILL / the DS's place), in front when their priority is ahead, else only over the backdrop. Columns
  the melonDS picture does not reach get every text BG. Centre goes back into the engine A picture, sides into the wide strips.
  Master brightness: off for the melonDS pass, applied by WC_Compose to the centre, by gpuscale to the strips.
- Active only with wide_layouts=1 AND wideview=1 AND the 1.25x scale, engine A on top, not the 3D overworld, and a layout set
  (party menu first). GPU compose is skipped on those frames. Logs: "[WIDE] wide layout drawing on/off", "[WIDE] layout #N: ..."
  (shift, melonDS BGs, drawn BGs), "[WIDE] wide layout avg N us/frame".
- Not done: alpha blending of drawn BGs, sprite regions / stretch (one sprite shift per screen), WIDE_SHIFT3D, engine B,
  WindowAnchor/Framed (text boxes moving to the wide edge).
- Test: `gcc -O2 -I platinum3ds/platform tools/widecomp_test.c platinum3ds/platform/widecomp.c -o wct && ./wct` (party layout,
  passes). x86: 0.4 ms per frame for the party menu. render3ds.cpp glue NOT compiled here (no devkitARM in the cloud).
- Settings app: Wide tab row 1 "Wide menu layouts" (wide_layouts, applies at once via P3DS_SetWideLayouts, saved to config.txt).
  No longer needs wideview=1, only the 1.25x scale (the default). Old "Wide menus" row renamed "Side fill in menus".
- Settings app tidy (Nathan: old options go to the hidden Debug tab, SELECT): Debug now also has Log size limit, Split top,
  Wide view FX, Wide haze, Edge fill, Side fill in menus, Mirror fill (old), Wild sprite test. Video = FPS counter, Wide view,
  Scale, Camera zoom, 3D strength, Battle 3D depth. Wide = Wide menu layouts, Weather in strips + Distortion World rows.
  Values and defaults unchanged, only where the rows are.

## Run 85 (wide layouts: bag speed, party centred)
- Hardware log (run 84): party menu layout engaged, no crash. Nathan: party panels pushed fully right with a blank left (GabeSud0's
  reference layout does exactly that); the bag looked wide but dropped frames. Cause of the drops: the bag has 18 layout parts and a
  new plan every frame while its list scrolls, and the plan built each line's column map column-by-column x part-by-part. Now
  widecomp.c buildRow() fills each map by ranges (tools/widecomp_rows_test.c: identical to the old per-column result on 3000 random
  layouts; x86 plan 1.3 ms -> 0.03 ms for an 18-part layout), and map slots cover all lines of all four BGs (WC_ROWS 384).
- Party menu: panels, sprites and background moved 16 px instead of 32, with the background's plain left/right columns drawn
  on beside them, so the extra width is split over both sides (work_patches applications__party_menu__main.c.patch).

## Run 85b (from the photos of run 85)
- Party: the 16 px shift was no centring at all (the wide screen already centres the 256 columns); left blank was 48 px, right 16.
  Now background stays, the two panel columns (and sprites) spread 16 px each way, background's plain edge columns fill both sides.
- PC boxes (photo: info panel streaky, icons misplaced): (1) CPU BG layers behind the melonDS ones were only drawn where nothing had
  been drawn before them, which let BG3's stripes cover BG2's panel; they now check the melonDS picture only. (2) WIDE_OBJS
  regions: WC_AdjustOam moves each sprite by (its part's dx - the picture's shift) in the OAM copy before the melonDS pass;
  sprites that would leave the 256 columns are cut there. Still not done: WIDE_SHIFT3D (the 3D Pokemon preview), stretch sprites.

## Run 86 (area-change hitches, step 1: the log names the map and the files that missed)
- Oct 24 hardware log: 29.3 fps average, 61 [SPIKE] frames; 15 of them ROM read misses (worst 398 ms, 330 ms of it SD reads),
  44 "loading" frames with no ROM cost. Before changing the cache or prefetcher, the log has to say where and what.
- New `[ZONE] 3 Jubilife City -> 286 Route 202` line on every zone change (warp or walking seam), also with zone_prefetch=0.
- Every [SPIKE] frame now gets a second line: `[SPIKE] f=N map <now> <- <before> (learned | first visit | no prefetch data,
  +ms since the change) | missed in: fielddata/land_data/land_data.narc +812K 6x, ...` (up to 4 files, offset of the first miss
  inside the file, misses per file). File names come from the ROM's own FAT/FNT, read once at the first spike with misses
  (own file handle, a few tens of KiB in RAM); nothing is written and nothing ROM-derived is in git. Unnamed files show as
  `file N` (overlays). [PERF3] also says `zone <now> <- <before>`.
- No config key: diagnostics only, inside the existing spike_ms log (spike_ms=0 still turns [SPIKE] off). Game, save and
  timing untouched; the extra work happens only on frames that are already over spike_ms.
- Test (x86, synthetic ROM with nested FNT dirs): `tools/romcache_spike_test.c`.
- Next: read a 15 minute walking log, sort the misses by file and by first visit vs learned, then pick the fix (earlier
  prefetch near warps, whole-file read-ahead for the big archives, or a cache for the 44 loading frames).

## Run 87 (wide layouts: one compositor, after the run 85 photos)
- Run 84/85 let melonDS draw some BGs and drew the others over its picture; that cannot order the two groups (party panels lost
  their text, PC info panel streaky, bag selection box missing). widecomp.c now has its own engine A compositor: text BGs (column
  maps from the layout parts), sprites (normal + affine, 4/8 bpp, each moved by its WIDE_OBJS region), the 3D layer as BG0 (moved by
  WIDE_SHIFT3D), DS priority order, BLDY fades on BG/OBJ targets, master brightness on the centre. melonDS still draws its normal
  picture first and stays the fallback: WC_Compose returns 0 (log "[WIDE] wide layout drawing off") for windows, alpha blending,
  mosaic, affine/bitmap BGs, bitmap sprites, forced blank. OBJ-window / semi-transparent sprites are drawn as normal sprites or
  skipped. Stereo: the same picture goes to both eyes (menus lose their per-layer 3D depth while this is on).
- Party layout: background and message box stay, the two panel columns (x < 123 / >= 123) and their sprites spread 16 px each way.
- tools/widecomp_test.c checks the party layout (fills, moved tiles, moved sprite, priority, fallbacks); widecomp_rows_test.c checks
  the row builder. render3ds.cpp glue not compiled in the cloud.
## Run 88 (shiny hunt suite, `shiny_suite`, plan Part 2.1 items 1 to 5)
- New key shiny_suite=0 (default off), settings app Extra tab, first row "Shiny hunt suite". Applies at once. Nothing is written to
  platinum.sav; the save format is untouched. Counters live in sdmc:/3ds/platinum3ds/hunt.txt next to config.txt.
- platform/hunt.c (new) owns hunt.txt for both hunt_counter (run 76) and shiny_suite. Per species: encounters, encounters since the
  last shiny, soft resets, shinies, time spent, start date, how long the last shiny took; plus `target=` (the last species met,
  which soft resets are tagged to). The run 76 `species=count` field stays first, so an old build still reads the counts and an old
  hunt.txt loads in this one. Written on every encounter, every soft reset (before the relaunch), lid close, exit and every 2 min of
  hunting, via hunt.tmp (a power cut mid-write keeps the old file).
- Shiny flag: battle_controller_player.c now also stores the game's own Pokemon_IsShiny(foe) in P3DS_CatchInfo[18] (read-only,
  same snapshot as the catch info). The work tree's copy of that file has to be refreshed from pspoke-patches/ before building.
- Shiny alert (shiny_suite=1): when the command menu first opens against a shiny wild Pokemon, a two-note chime on DSP channel 1
  (the game's audio only uses channel 0), a "Shiny!" toast and 0.9 s of white pulses with sparkles where the foe stands. Log:
  `[HUNT] SHINY species N after X encounters`. Not yet: the overworld sparkle on wild_vis Pokemon.
- Hunt panel: the Ball tab shows it in the field (L+R+Y, then the Ball tab) and in battle when Catch info is off (Y opens it).
  Odds 1 in 8192 (the Gen 4 rule for wild Pokemon), chance of a shiny by now from the encounters since the last one, the 50% / 90%
  marks (5678 / 18862). D-pad left/right = other species, X twice = reset that species' counter, A = export every hunt to
  sdmc:/3ds/platinum3ds/hunts/hunt_YYYYMMDD_HHMM.csv. Masuda eggs and Poke Radar chains are not detected, and the panel says so.
- Not done from the plan: shiny_rate, RNG info, party shiny count, battery warning, Masuda/Radar odds.
- Test (x86): `gcc -O2 -I platinum3ds/platform -DHUNT_DIR='"/tmp/hunt_test/"' tools/hunt_test.c -lm -o ht && ./ht` (old file
  format, round trip, soft resets, time with sleep gaps, periodic write, power-cut recovery, species cycling, reset, odds, CSV).
  render3ds.cpp and the game file are not compiled in the cloud (no devkitARM / pspoke tree here).

## Settings app redesign (after run 88, from Nathan's "cluttered and ugly" note)
- Tab bar: 6 tabs instead of 9 (Video, Game, Keys, Pal, Hunt, Cheats; SELECT still adds the hidden Debug tab). Each tab is a
  one-per-line list (8 rows a page, name left, value right between arrows, a green dot on rows changed this visit) instead of
  the 2 x 5 grid of boxes. Up/Down (D-pad or circle pad, held = repeat) pick, Left/Right or A/X change, L/R tab, B close.
  Touch: tab bar, a row's left half of the value = previous value, anywhere else on the row = next value, footer = next page.
- The Ball, Map and Menu tabs left the tab bar and became pages opened from rows: Hunt > "Hunt stats" (the old Ball tab:
  hunt panel / catch info), Hunt > "Wild Pokemon here" (the old Map tab, A or tap = next page), Keys > "Quick menu" (the old
  Menu tab). B on a page opened from a row goes back to that row; a page opened directly (Y in battle, bottom strip, touch
  quick menu corner) still closes on B. Reopening the app after closing on a page shows the tab that page belongs to.
- Moves: Wide menu layouts and Time-of-day tint -> Video; the Distortion World rows and Weather in strips -> Video page 2;
  text/battle/item rows from Extra -> Game; Touch quick menu and Bottom strip -> Keys; Shiny hunt suite, Encounter counter,
  Catch info and the visible wild rows -> Hunt. Same items, keys, getters and setters (mItems untouched), so config.txt and
  the save are unchanged.
- Top screen: one title box ("Settings: <tab>  (page/pages)" + version) and a key hint box that says B back or B close.
- Test (x86): runApp + tables compiled against stubs with ASan/UBSan and driven by a scripted key sequence (pick row, change
  Wide menu layouts, L/R across tabs, open Hunt stats with A, B back); not built for the 3DS in the cloud.

## Run 88b (catch info hook as a normal overlay patch)
- The run 75 / run 88 hook in `BattleControllerPlayer_InitCommandSelection` (P3DS_CatchInfo snapshot, now with [18] = shiny) lived
  only as a full-file copy in pspoke-patches/, which no script applies. A build that compiled an older copy of that file never set
  P3DS_CatchInfo: Y did nothing, no encounter counted (Nathan's log, Oct 25). It is now also `work_patches/overlays/battle__battle_controller_player.c.patch`
  (against pokeplatinum 47c28d53, like the other overlay edits; applies to the pristine file and reproduces the pspoke-patches copy
  byte for byte). Apply it to native-audio-app/overlays/source/battle__battle_controller_player.c and make sure that object is rebuilt.
- Check after a build: a wild battle's fight menu + Y opens the page, and log.txt gets `[HUNT] species N met M times`.

## Run 94 (first read of the run 86 log)
- Nathan's log (build fefa04f, ~90 s of play, mostly menus and a battle): overworld steady 29-30 fps (present 1 ms, 575 tris). Frame drops are
  in menus: 22 fps at 3D slider 0 (present 20-24 ms vs 1 ms in the overworld, composite 8-15 ms), 13 fps with the slider up (present 41-47 ms).
  Not area changes. Seems to be the CPU present path menus take (wide menus on); belongs to the wide menu work.
- No [ZONE] or [PREFETCH] zone lines and `map -1` in [PERF3]: P3DS_ZoneEnter (work_patches/overlays/field_map_change.c.patch) is not in this
  build (like the catch-info hook in run 88b, overlay patches are applied by hand), so zone_prefetch also does nothing and prefetch.bin
  is stale (144 zones from an older build). Without it the area-change spikes cannot be named or prefetched.
- [SPIKE]: a run of consecutive slow frames now logs its first frame and every 30th (it used all 60 lines on one slow menu at f=1454-1497).

## Run 95 (2026-10-25, Grok Bot) - menu stutter with wide_layouts, [PERF4]

- [PERF4] (1 in 10 lines): game-thread time blocked in OS_WaitVBlankIntr / OS_WaitIrq. The overworld is capped at 30 fps by the game's
  own 2 V-blank waits; in Azahar ~11-12 ms of the 33 ms frame is that wait, ~15-16 ms real work.
- [SPIKE] has a second line with per-stage ms (bind | widePlan | engineA | B-wait | capture..tComp | wideCompose | overlays | wide/app | present | game-wait).
- Hitch found: with wide_layouts=1 every non-field top screen (title, party, bag, boxes...) was drawn by melonDS engine A (~16 ms)
  and then completely redrawn by widecomp (WC_Compose ~55-65 ms in the emulator), after waiting for engine B (~23 ms).
  widecomp never reads engine A's picture, so wide_early=1 (default, Debug tab) runs it right after engine B is posted to the
  worker and skips engine A when it succeeds. Same pixels (widecomp writes every centre pixel either way); falls back to the old
  order if widecomp declines. Azahar, title screen: engineA+B-wait+compose 105 ms -> 64 ms per frame (15 -> 20 fps); party
  44.5 -> 38.5 ms/frame (22 -> 25 fps); bag 43.0 -> 40.1 ms/frame. 592 of 601 eligible frames composed early.
  WC_Compose itself (~1 ms on x86, ~55 ms on the 3DS CPU) is now the remaining cost; next step would be dirty-row compose.
- A frame-level cache of the composed picture (wide_cache) was tried and dropped: WC_Compose already caches by input hash,
  and animated menus change every frame.
- "layout #1-#4: 0 parts" is expected: wide_layouts runs widecomp on every non-field 2D screen; screens without a WIDE_LAYOUT
  table (title, intro) get a 0-part plan (edge extension only). Bag (3 parts) and party (21 parts) plans appear later in the log
  ([WIDE] parts set: diag line, first 40 calls).
- gpu_compose=2 readback skip: not done. gpucomp uploads the read-back NativeFrame as its 3D texture; skipping the readback needs the
  render target bound as a texture directly. Too risky for this run.
- Not tested this run in the emulator: PC boxes, a battle, shiny_suite in an encounter, text_hold_b/text_auto in dialogue.

## Run 96 (2026-10-25, Grok Bot) - stereo battle speed, CIA banner
- wide_early extended to stereo (see docs/HANDOFF.md): Spear Pillar double battle with stereo_fake=60 in Azahar 7-14 -> 13-22 fps.
- run95 CIA had been built with BANNER=flat (broken HOME banner/logo); run96 uses the run82 3D banner again.
- Emulator: trainer double battle (Mars/Jupiter, Spear Pillar) with wide_layouts: battle UI fine, no crash. wide_early on/off pixel
  diff on party and bag: identical apart from the clock and an icon animation frame.

## Run 97 (2026-10-25, Grok Bot) - battle depth with wide layouts
- wide_stereo / wide_stereo_par (default 1): per-eye wide compose (right eye on core 2), restoring battle slider depth.
- Fixed the 2.7 s first-battle freeze (stereo blob diag dumping BMPs on a not-yet-filled right eye under wide_early).
- Azahar stereo battle fps: run96 flat 13-22, run97 11-19 with depth. See docs/HANDOFF.md.

## Run 98 (2026-10-25, Grok Bot) - widecomp dirty lines
- wide_dirty (default 1): per-line input hashes, only changed lines redrawn, rolling 8-line refresh; verify mode 0 mismatches.
- Azahar stereo battle 10-23 fps (run97 11-19); emulator noise is large; 30 not confirmed. See docs/HANDOFF.md.

## Builder parity: overlay edits that only lived as pspoke-patches/*.c (2026-10-25, Claude)
- Same problem as run 88b for five more files: the run 75-80 edits below existed only as full-file copies in pspoke-patches/,
  which no script applies, so a build made with the builder (patch-builder, apply_worktree.py) never had them. They are now
  overlay patches against pokeplatinum 47c28d53 (each applies to the pristine file and reproduces the copy byte for byte;
  where a wide-layout patch already existed it is merged in):
  - battle__healthbox.c: battle_anim_speed=2 fast HP drain (+ the existing GabeSud0 wide health boxes)
  - battle__battle_display.c: battle_anim_speed=1 skip plain move animations
  - player_move.c: cheat_speed, cheat_walk_walls (+ the existing zl_run hunk)
  - render_text.c: text_hold_b, text_auto
  - rtc.c: cheat_time
- pspoke-patches/battle__healthbox.c removed (fully covered by the patch). The other copies there stay for now as reference.
- README: the builder is now the recommended way to build; the hand build follows the builder's steps; troubleshooting section.
- Not compiled in the cloud (no devkitARM / generated tree). On an existing tree the builder reverts the old healthbox /
  player_move patches and applies the new ones; on a tree that already has the copies' edits it just records them.

## Run 99 (2026-10-25, Grok Bot) - Poke Ball send-out hitch
- wide_early_check=1: declined widecomp frames in stereo keep the parallel right-eye path (send-out hitch 48-57 ms in the hw log).
- Battle-end hitch (87 ms game-side during the field reload) analysed, not fixed. See docs/HANDOFF.md.

## Run 100 (2026-10-25, Grok Bot)
- wide_dirty_regs=1: fewer full widecomp redraws (only registers composeNow reads are hashed). Other run-100 goals deferred; see docs/HANDOFF.md.

## Run 100 = v1.0 (2026-10-25)

Bottom icon menu (lower_menu, GabeSud0 port, default 0), v1.0 version string and credits splash, settings help lines, quieter log_level 1, wide_dirty_regs. Battle-end hitch, async readback, mono split not done (see HANDOFF). Builds: 3dsx adb863b6ecb81a741e7dbfe6335a9a5544a2d0df, CIA 59ba3ce209df4f3c9673c0342deb131221ea565f.

## Run 101 = v1.0 (2026-10-25)

field_prof profiler (default 0), heap_fast (default 1: skips pspoke's 8192-entry table scan per heap free; emulator free time 1232 ms -> 1.5 ms), lower menu gender icon, play time in settings, Debug tab tidy. Builds: 3dsx 194b9cf473791bbaaf3c71de6af793b72c76ec2a, CIA a8e7c54261a3635c4844ac21c91b04c86739a237.

## Run 102 = v1.0 (2026-10-25)

lower menu MAP/POKETCH actions, menu hidden behind settings, play time checked, dc_flush_skip (default 1). Builds: 3dsx a49f1cd39acf018a6282f786aa28d3bf6d98abc7, CIA 17f02de4b388423744f605e75536bc73a440098d.

## Run 103 = v1.0 (2026-10-25)

MAP -> Bag on Key Items; wide_dirty mismatch localised (battler sprite pixel, line 52) but not fixed. Builds: see HANDOFF / report.

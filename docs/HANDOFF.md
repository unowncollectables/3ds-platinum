# Handoff log

Dated entries, newest first. See AGENTS.md for the rules and PROGRESS.md for the full run log.

## 2026-10-27 — Run 118c: gameplay extras (Claude, branch `run118-gameplay`)
- Three game-side extras in `work_patches/overlays/`, each with a row in the settings app's Game tab, all default OFF. With a key at 0 the game's code paths are unchanged. No save format change.
- `exp_share_all` ("Exp. Share: whole party"): every party Pokemon that has HP and is not an Egg gets Exp. after a win; one that didn't fight gets as much as one that did (battle_script.c). Read live from the RAM config cache, so the menu change applies at the next battle.
- `auto_run` ("Auto-run"): running is the default and holding B walks (player_move.c PlayerAvatar_IsRunButtonHeld). Combines with the ZL run toggle (both on = back to normal). Applies at once.
- `catch_swap` ("Swap catch into party"): when you catch a Pokemon with a full party, the battle asks "Your party is full. Add X to your party?" before the nickname. YES opens the battle party screen (SHIFT reads SEND TO PC) to choose who goes to the PC; once the party is written back to the save (UpdateFieldSystemFromDTO in encounter.c) the two swap places. Pokemon holding Mail or with a Ball Capsule can't be picked. Not in the Safari Zone, Pal Park or the catching tutorial. Idea from the reference port pokeplatinum-3ds (0042); the code is ours.
- Rebuild the overlays after applying the patches: `python3 rebuild_obj.py battle/battle_script.c battle/battle_system.c battle_sub_menus/battle_party.c battle_sub_menus/battle_party_text.c encounter.c player_move.c`.
- **Untested on hardware and in the emulator** (compile-only: overlay objects rebuilt, `make` + `make cia`). Hardware check, with a save backup: a win with exp_share_all (Exp. messages for the bench, level-ups), walking with auto_run and B, catch_swap with a full party (pick, cancel/NO, nickname, the swapped Pokemon in the right PC box after a save + reload).

## 2026-10-27 — Run 118b: gc_atlas16, 16-bit 2D tiles (Claude, branch `run118-atlas16`)
- New `gc_atlas16` (Debug "16-bit 2D tiles", default OFF, read once at start, so restart after changing it). The GPU compose 2D tile atlas (gpucomp.c) is RGBA5551 instead of RGBA8: half the atlas memory (1 MiB less linear RAM) and half the texture reads per quad.
- Safe because 2D tiles are only opaque or fully transparent; semi-transparency comes from the blend registers, not from texel alpha, so one alpha bit is enough. The 3D layer textures stay RGBA8.
- Not bit-identical: the PICA widens 5-bit colour as c<<3|c>>2, melonDS as c<<3|c>>3, so a channel can differ by up to 4/255 from the RGBA8 atlas and the CPU path (GPU compose verify mode will report those pixels).
- OFF = Run 118a. **Untested on hardware and in the emulator** (compile-only: `make` + `make cia`). Hardware check: turn it on, restart, compare menus/battles for colour or transparency glitches and fps, look for "[GPUC] gc_atlas16=1" in the log.
- **Run 118 hardware: gc_atlas16=1 hard-froze the New 3DS at boot** (black screen, forced power-off; log.txt ends after `[perf] speed-up`, no [GPUC] lines because the log thread lost its tail). Re-read gpucomp.c for the cause: RGBA5551 size (512x1024x2 = 1 MiB), 8x8 tile = 128 bytes at slot*64 u16, Morton index, dirtyLo/dirtyHi byte offsets and the flush size, the init memset/flush (2 bytes/px), PICA 5551 order (R15-11 G10-6 B5-1 A0), the TEV (texture alpha replace + alpha test > 0 works with 1-bit alpha), settings table (mItems 100 = 33 + kGenDef 67) all check out; no other code writes the atlas; the SoulSilver port has no RGBA5551 texture to compare with. **No definite bug found, cause unknown** (it may not even be atlas16: the log does not reach GPU compose init). Added: the 16-bit atlas is checked after C3D_TexInit (fmt, size, 128-byte alignment) and falls back to RGBA8 with a log line; **boot guard** `sdmc:/3ds/platinum3ds/gc16_boot.flag` (written when gc_atlas16=1 is read at GPU compose init, holds the last stage reached, removed after 300 frames); if it is still there at the next start, gc_atlas16 is forced to 0 in config.txt and `[GPUC] gc_atlas16=1 forced OFF ... last stage: <stage>` is logged, which also tells how far the dead start got (before the texture init / atlas ready / 30 frames). Compile-only.

## 2026-10-27 — Run 118a: frame_pacing (Claude, branch `run118-frame-pacing`)
- New `frame_pacing` (Debug "Frame pacing", OFF / FIXED / ADAPTIVE, default OFF). Until now every GPU compose (gpucomp.c) waited for its own draw + readback (~1.1-1.6 ms per compose in the Run 116 hardware log, twice a frame with the slider up).
- FIXED: a compose queues its draw and shows the same eye's pixels from the frame before (one frame of latency) while the CPU carries on. ADAPTIVE: no latency while frames are on time (the stereo left eye's draw overlaps the right eye's tile walk); after a frame over 35 ms it works as FIXED until 600 frames in a row are on time (doubling, up to 16x, if lag returns within 60 frames).
- Frame boundary hooks in render3ds.cpp (P3DS_GcFrame / P3DS_GcFinish). Two extra linear-memory buffers; if they can't be allocated it logs "[GPUC] frame_pacing buffers: out of linear memory, pacing off". A 600-frame summary is logged. The menu change applies at once.
- OFF = Run 117 behaviour. **Untested on hardware and in the emulator** (compile-only: `make` + `make cia`). Hardware check: turn it on, look for one-frame ghosting on transitions/fades, compare battle/menu fps with OFF, read the [GPUC] frame_pacing lines.
- **Run 118a fix (after the Run 118 hardware test):** FIXED worked on hardware, but with the wide view on the lagged centre was one frame behind the 3D side extension (visible seam), wide field view and wide (gW) frames. The one-frame lag is now never used on wide frames (gW) or while the wide view is on (render3ds sets `P3DS_GcNoLag = wideCfg>0` each frame); those frames get the overlap-only, no-latency behaviour (ADAPTIVE without lag), so one eye always shows one frame. **Boot guard:** with frame_pacing on, the start writes `sdmc:/3ds/platinum3ds/fp_boot.flag` and removes it after 300 frames; if it is still there at the next start, frame_pacing is forced to 0 (config.txt too) and `[GPUC] frame_pacing=N forced OFF ...` is logged. Compile-only; hardware check: wide view + FIXED, no seam at the side extension.

## 2026-10-26 — Run 117: slider-up Pokedex, one compose for both eyes (Grok Bot, branch `run117`)
- Run 116 hardware [WCPROF]: with the slider up, the Pokedex composed both eyes in full on the CPU (192 lines each, ~33 ms per eye: sprites 12, BGs 14.7, out 6.3 ms), 15 fps. Slider 0 was 29-30 fps (dirty lines + wide_split).
- An emulator diagnostic showed the global inputs rarely change (blend regs only), so the dirty detection is right. The emulator draws the Pokedex mostly on the GPU wide path and can't reproduce the slowness.
- New `wide_stereo_flat2d` (Debug "Copy eye when 3D empty", default ON): a slider-up wide frame whose 3D layer has no visible pixel in either eye skips the right-eye compose and uses wide_split for the left eye (both cores). The left eye is then copied to the right. Not measured on hardware yet; [WCPROF] still logs while the Pokedex is open.

## 2026-10-26 — Run 116: Pokedex diagnostic build (Grok Bot, branch `run116`)
- The user wants the Pokedex wide and fast. dex_wide defaults to ON again.
- Diagnostic: widecomp.c composeNow accumulates ticks per stage (sprites / windows / BG layers / backdrop+blend+output), plus lines, sprites and composes (WC_Prof*). render3ds prints a [WCPROF] line every second while overlay 21 is loaded (P3DS_DexOpen) or with Log detail FULL.
- Both cores add to the same counters (wide_split), so treat the numbers as approximate.
- Run 114 log: about 84 dirty lines per compose still took about 30 ms on two cores (~0.7 ms/line). The [WCPROF] split should show which stage.

## 2026-10-26 — Run 115: fast Pokedex (Grok Bot, branch `run115`)
- Hardware Run 114 log: Pokedex frames (overlay 21) were 58-77 ms. The wide layout made widecomp redraw the whole screen every frame (engineA/wide stage 30-42 ms, at slider 0 and up). The GPU can't take those frames: ineligible 145/300 (blend 116, affine sprites 29).
- Fix: new toggle `dex_wide` (Debug "Wide Pokedex", default OFF). While overlay 21 is loaded (link --wrap of FS_LoadOverlay / FS_StartOverlay / FS_UnloadOverlay in wideapi.c sets P3DS_DexNoWide), the wide layout is off. The Pokedex shows the DS layout through the normal engine A path. 1 = Run 114.
- One emulator boot-to-field check only (overlay loads OK). The Pokedex itself is untested in the emulator; check the [WIDE] Pokedex overlay lines in the hardware log.

## 2026-10-26 — Run 114: no more slow launch with the slider up (Grok Bot, branch `run114`)
- Hardware Run 113 log: for the first ~6 s after launch (opening cutscene) frames ran 54-62 ms (fps 15-16), `path S`, `stereo 17(serial)`. Run 111's log is the same, frame for frame, so the Run 113 clean-up didn't cause it. Earlier boot logs had the slider at 0.
- Cause (Run 111 wide_gpu): the opening is a wide-layout frame with an alpha blend (BLDCNT 0842, BG1 over BG3, backdrop not a 2nd target). wide_gpu picked it (wgNow), so wide_early was skipped, then P3DS_GcComposeA rejected the blend. The right eye fell to the serial CPU path.
- Fix: gpucomp.c P3DS_GcBlendPredictOk predicts that rejection. With new toggle `wide_gpu_precheck` (Debug "Wide GPU blend check", default ON) such frames don't pick wide_gpu and take the CPU wide path (path W), as in Run 109. 0 = Run 113 behaviour.
- Emulator, slider up, boot to the field: [BATT] boot episode avg 55.4 ms (Run 113) -> 35.9 ms. The slider-up battle is still wide on the GPU. Quick runs only; the user is testing on hardware.
- Builds: 3dsx 3e5ea67a7c9d1b04c8987f82f8083101085d41ad, CIA e0ebb90a8c935af6c68931ed9185c780c1a22223.

## 2026-10-26 — Run 113: clean-up of loggers, settings and files (Grok Bot, branch `run113`)
- Logging cost measured (emulator, same battle script, slider up): ~420 log lines / 50 KB per 590-frame battle, hidden detail included (~0.7 lines/frame). The log is written by a background thread every 100 ms, so the main-thread cost is only the string formatting (tens of µs per frame, well under 1% of a 33 ms frame). [BATT] avg 55.4 ms before and 55.4 / 55.3 ms after: logging was not the bottleneck.
- Real costs removed from default play:
  - stereo_blobdiag now defaults to 0. Its first hit wrote 4 BMPs to the SD on the main thread (a multi-second freeze) and forced a GPU 3D readback.
  - field_prof now defaults to OFF (was AUTO, which profiled every wrapped overlay005 call for 240 frames after each battle start/end).
  - The [3DLAYER] 49k-pixel scan (every 120 frames) only runs with Log detail FULL.
- Kept: battle_log ([BATT] summary per battle plus up to 40 [SPIKE] stage lines) and the [PERF] lines. [BATT] now also prints the log lines/KB written in that battle (logring.c counters P3DS_LogLines/P3DS_LogBytes).
- Removed settings and code (both default OFF; old config.txt keys are ignored and still parse):
  - wide_split_r (Debug "Right eye on 2 cores"): render3ds.cpp split paths and widecomp.c WC_SplitRunDyn.
  - stereo_gpu_battle (Debug "3D battles on GPU", the 4:3 mode): sgbNow and P3DS_GpuBattleUp (wideapi.c).
  - The Debug tab has 29 rows. mItems/kGenDef indices are unchanged, so config writes are the same.
- Test-only keys kept, but hidden (not in the app): wide_gpu_verify, dump_eyes, lid_test, res3d_test, heap_watch, stereo_fake.
- Repo: removed patches/run67, run68, run71, run73 and run74 (superseded by work_patches/overlays) and forwarder/hello_test.c (the old CIA pipeline test).
- SD files (nothing deleted):
  - Still read: config.txt, platinum.nds, platinum.sav (+ .bak1-3), followers.bin, emotes.bin (follower emotes), prefetch.bin (zone_prefetch), hunt.txt and hunts/.
  - Write-only diagnostics, not read back, safe to delete: log_old.txt, dump_*.bmp, eye*_L/R.bmp, stereo_blob_*.bmp, wgv_*.bmp, gpuc_mm*.bmp, seq/, prefetch.tmp, hunt.tmp, forwarder_log.txt, hello_cia.txt.
- Emulator checks: tools/field_wide_test.sh PASS (slider 0 and up, with the user's run111 config). The slider-up battle stays wide on the GPU path. The settings app Debug tab is OK.
- Builds: 3dsx cbc78828e4adfcc2e38d5d6a233cb28fa66d6095, CIA a04d1f94590f1a1853f71b8a30234fa736fb4625.

## 2026-10-26 — Run 112: overworld wide view back (Grok Bot, branch `run112`)
- Run 111 hardware: slider-on battles much better ([BATT] avg 45.1 / 36.1 / 34.2 / 34.1 ms over 4 battles; Run 109 was 47.6-50.7, Run 108 40.3-41.0). But the overworld showed 4:3 with bars, whatever wideview was set to.
- Cause (Run 111, wide_gpu): its 512x256 wide target (512 KiB VRAM) was allocated on the first slider-up wide frame (the title screen) and never freed. The overworld wide view then needs two 320x256 3D eye targets with depth, but `[3DRES] 320x256 targets FAILED (VRAM free before 1284096)`, and g3gpu set wideFail for good. So the field stayed 256 wide (pillarbox) for the rest of the session at any slider position. It shows in the emulator too, when the slider is up before the field.
- Fix: gpucomp frees the wide target when the field wide view turns active (P3DS_GcWideRelease) and g3gpu retries the 320 targets (G3G_WideRetry). The target is made again the next time a battle/menu wide frame needs it (after the field's 320 targets went back to 256). A failed allocation is retried after 30 frames instead of never.
- New test tools/field_wide_test.sh: boots a save into the overworld with the slider at 0 and up and checks:
  - the side strips (non-black, not the pillarbox colour);
  - that the log shows the 320x256 targets with no failure.
  - Run 111 binary: slider 0 PASS (no slider-up frame allocates the target), slider up FAIL. Run 112: both PASS. A battle afterwards still runs on the GPU wide path (emulator 41.5 ms/frame, path X).
- Builds: 3dsx bcb8dfd43088d9df2cf4a22491d946c5bf55d212, CIA 18d0f9e76b6f2175f91e6b6d40732b5bb492d5e9.

## 2026-10-26 — Run 111: the wide battle layout on the GPU (Grok Bot, branch `run111`)
- The user wants the wide side panels with the slider up. Run 110's `stereo_gpu_battle` (4:3) now defaults to OFF.
- New `wide_gpu` (Debug "Wide on GPU": OFF / SLIDER UP / ALWAYS, default SLIDER UP): a wide-layout frame is composed on the GPU for both eyes by gpucomp.c `P3DS_GcComposeWide`, from widecomp's plan.
  - The 320-column layout goes into a 512x256 VRAM target.
  - BGs are drawn per band of lines sharing a column map. Each map splits into contiguous runs (the layer shifted and clipped, with quads clipped and UV-interpolated) and constant runs (stretched edge columns, one texel column).
  - Sprites use the WIDE_OBJS part shift and edge stretch.
  - The 3D layer sits at the WIDE_3D shift from the gpu_direct3d texture.
  - The backdrop shows in the strips. BG0-targeted fades are skipped in the strips via scissor, as in widecomp.
  - The right eye uses the right-eye 3D texture. The strips come from the left eye, as before.
  - Engine A, the CPU wide compose, wide_early and the 3D readback are skipped on those frames.
- New `gpu_mbright` (Debug "GPU fades", default ON): master brightness is a final GPU quad (centre only in wide). It is no longer a gpucomp decline.
- gpu_blend_3d now uses colour*alpha + below*(1-alpha), the same rule as widecomp wide_3d_alpha. Run 110's premultiplied version differed by up to 230 per channel on translucent move effects.
- Test hook `wide_gpu_verify=N` (with wide_dirty=0): every Nth GPU wide frame is also drawn by the CPU widecomp and compared. Emulator results over 193 frames:
  - Strips were exact in all of them.
  - Centre was exact on 2D-only frames.
  - 3D frames had a max channel diff of 3-7 (8-bit GPU 3D vs 6-bit NativeFrame, 8-bit vs 5-bit alpha).
- Emulator, Coronet battle, slider on (one host thread, noisy):
  - Run 109 wide CPU: 57.9 ms/frame.
  - Run 110 4:3 GPU: 40.8-46.9 ms/frame.
  - Run 111 wide GPU: 41.1-44.9 ms/frame, path X (new battle_log char: wide GPU stereo) 55 of 71 logged frames.
  - Slider flip every 5 s (30 transitions): no hang.
- Still on the CPU wide path: affine (rotated/scaled) sprites (the most common decline), semi-transparent/window sprites, and the battle intro windows. They need per-sprite affine sampling and per-pixel window masks on the PICA.
- **For Claude / SoulSilver:** P3DS_GcComposeWide plus widecomp's WcPlan is the piece that lets the wide layout keep gpu_direct3d. It should port to SoulSilver's wide mode the same way.
- Builds: 3dsx a87fcfab54c2fe16df1b62b6168e9b896fbf457e, CIA 2124e12140184ce50d51c41a4aadf6a3c29d95e0.

## 2026-10-26 — Run 110: slider-on battles on the GPU, the SoulSilver way (Grok Bot, branch `run110`)
- Why Run 109 was worse (hardware slider-on battles avg 47.6-50.7 ms vs 40.3-41.0 in Run 108; slider 0 unchanged): `wide_split_r` SHARED. Composite rose to 33-46 ms. Every 16-line group re-runs composeNow's per-call setup (palette LUT, OAM walk), both cores thrash the same caches and VRAM-flat buffers, and main ends up doing right-eye work on top of the left eye. `wide_windows` is not the cause: battle DISPCNT 00111f18 has no windows, so its code is never reached. wide_split_r is back to OFF by default (HALF / SHARED still in Debug).
- **SoulSilver comparison (useful to Claude too):** the SoulSilver port (`unowncollectables/Pokemon-Soulsilver-New3ds`) has no widecomp in battles. With the slider up it GPU-composes both eyes outside the overworld (`gpu_compose_stereo_nf`), with GPU alpha/brightness blending (`gpu_blend`), 3D-layer texture reuse by hash, a right-eye register copy for stereo_2d BG depth, and Claude's `gpu_direct3d`: the 3D frame stays in the PICA render target and the compositor samples it directly (no wait, no readback DMA, no 6-bit convert, no re-swizzle/upload). Anything that needs CPU pixels calls G3G_DirectResolve(). Platinum battles went through the CPU wide path W (two CPU composes plus readback) instead. Wide mode can't use the direct texture because widecomp is a CPU compositor.
- Changes (all Debug-tab toggles):
  - `gpu_direct3d` (default ON): ported from SoulSilver g3gpu.c. Resolve points in render3ds.cpp: shift3D, CPU right-eye paths, widecomp, display capture, CPU composite, GPU-compose rejection, dumps. g3soft invalidates on CPU-rendered frames.
  - `stereo_gpu_battle` (default ON): when the slider is up outside the field and gpucomp can take the frame, both eyes are GPU-composed (path G) instead of wide path W. These frames are 4:3 DS layout. While it's active, `NitroPort_WideScreenExtra()` returns 0, so health boxes created then sit at DS positions; before this they were clipped at the 4:3 edges.
  - `gpu_blend_3d` (default ON): gpucomp no longer rejects blend frames just because the 3D layer is on (only when 3D is itself a first target). Translucent 3D pixels use a premultiplied blend over the second-target layers below. Without the blend, a Gyro Ball effect showed as a black blob.
  - gpucomp.c replaced by SoulSilver's version (gpu_blend, 3D tex reuse). Right-eye stereo_2d register copy for GPU stereo.
  - `wide_split_r` default 0. `wide_windows` kept ON.
- Emulator, Coronet battle, slider on (Azahar runs on one host thread, so the numbers are noisy): r109 57.9 ms/frame, r110 40.8-46.9; with gpu_direct3d=0 47.8; with gpu_blend_3d=0 66.1 (mostly path W). Path counts r110: G 46, W 21 (intro/windows/mbright frames), S 8, P 2. Screenshots: HUD fully visible, effects correct. The slider toggle test every 5 s ran with no hang.
- Not done: wide layout plus GPU (would need gpucomp to draw the wide side parts). Remaining W frames are battle intro / brightness / objaff ineligible frames.
- Builds: 3dsx 7251d06b0221cdd7da88dfad9f82d48b31ba6e5d, CIA 4f72269a04b0fa597cf5b091f771a7ac8099eaec.

## 2026-10-26 — Run 109: slider-on battles, part 2 (Grok Bot, branch `run109`)
- Run 108 hardware log: slider-on battles avg 40.3-41.0 ms (about 160/320 frames over 40 ms); slider 0 avg 33.2-33.9. Battle frames take the wide-early stereo path (new `path` field in battle_log lines: W = wide early stereo). Composite 17-32 ms (engineA stage 19-31) vs 11 ms in field 3D; g3soft 15-17. The 2-eye compose is the bottleneck: main draws the left eye then waits while core 2 draws engine B + the whole right eye.
- Why g3_conv_split did not help: it worked (PERF2 3D convert 2.7-3.7 ms, was ~5.4 for 2 eyes) but the 3D convert is not the critical path in battles; the compose is.
- First-battle 119 ms (f=427): romfs 106 ms, 44 x16K misses (battle -> field load, [PROF] FS_ReadFile 54-80 ms). Same SD read cost as other load spikes; not fixed.
- Changes:
  - `wide_split_r` default now SHARED (2): main and core 2 claim 16-line groups of the right eye until done. OFF / HALF still available.
  - New Debug toggle `wide_windows` (default ON): widecomp draws WIN0 / WIN1 / OBJ-window frames (per-pixel WININ/WINOUT layer + effect enables, strips use WINOUT) instead of declining (reason 3) to the double engine-A render. Host test tools/widecomp_test.c extended (WIN0 cases).
  - battle_log stage line: `path` (w/m mono wide/plain, W stereo wide, P right eye on core 2, S serial, F flat, G GPU).
  - Test-only config `stereo_fake_toggle=N` (with stereo_fake): fake slider flips every N s.
- Emulator (Azahar runs all cores on one host thread, so 2-core gains don't show; noisy): Coronet battle slider-on avg 63.0 ms/frame vs 60.4 for 8e84ea9. Trainer intro/send-outs rendered correctly with no reason-3 declines. Slider flip every 5 s through the battle (12+ flips while composing on 2 cores): no hang, no watchdog.
- Not done: GPU compositor (needs per-pixel priority + 3D alpha + windows on the PICA), skipping the right eye when unchanged (dirty lines already skip unchanged rows). A wild-battle OBJ-window intro was not reproduced in the emulator.

## 2026-10-26 — Wireless: host test + fix (Claude)
- New `tools/wm_uds_test.c`: two processes run the real `wm_uds.c` against a mock UDS (socketpair with loss/dup/reorder) and a fake game shaped like pret's `wireless_manager.c` (parent params, scan + GGID/entry filter, connect with child SSID, MP, port send/recv). It checks every message arrives once, in order, intact. Scenarios: clean, 2.5% loss, 5% loss + dup + reorder, 15% loss, host vanishing mid-trade. All pass.
- Bug it found (fixed): `netPump` read the clock, then the child's hello stamped a later time, so `t - firstAt` wrapped and the brand-new link was dropped as "no ack" right after connecting. All age checks now use `since()`, which never goes negative. This would very likely have broken the first hardware trade.

## 2026-10-26 — Run 108: slider-on battles (Grok Bot, branch `run108`)
- Stall cause: there is no extra stall in the vblank wait. The game's VBlank waits run on a fixed 60 Hz grid (pspoke cadence.c, 2 waits per game frame). The Run 104 hardware log shows steady slider-on battles at 34.2-34.3 ms/frame = g3soft 21.6 (of which G3G_End GPU draw+readback ~10.6 ms for 2 eyes, incl. ~5.4 ms CPU convert) + composite 10.7 + present 1.7. That is ~1 ms over the 33.3 ms budget, so frames miss the next tick and wait for the one after (the "vblank wait 14 ms" in 40-44 ms frames). Fix = cut work below 33 ms, not change the wait.
- Worst slider-on drops are the battle intro (DISPCNT 00119F18 = OBJ window, wide declines with reason 3): 45-50 ms composite, 14-15 fps on hardware, right eye rendered by engine A a second time.
- New Debug toggles:
  - `g3_conv_split` (default ON, same pixels): the right eye's 3D readback convert runs on a core-2 helper while main converts the left. Emulator: 3D convert 6.5 -> 3.6 ms per frame (main thread).
  - `stereo_flat_fallback` (default OFF, visual change): stereo frames the wide compositor declines show the left eye to both eyes (no depth for those frames) instead of a second engine A render.
  - `wide_split_r` now OFF / HALF (run 106 static split) / SHARED (=2, new: both threads claim 16-line groups until done). Default stays OFF.
- Emulator (Coronet double battle, stereo_fake=100, ~25 s of battle, ms/frame avg): 8e84ea9 60.4, run108 (conv split + flat fallback) 57.9; wide_split_r=2 55.0 in one run. Azahar runs all CPU cores on one host thread, so the core-2 gains cannot show there; numbers are noisy. No hangs in slider-on runs or a slider-0 run with wide_split_r=2.
- Not done: GPU compositor (needs per-pixel priority + 3D alpha), OBJ-window support in widecomp (would keep the battle intro on the fast path), skip-unchanged right eye. Untested on hardware; battle end not reached in the emulator runs.

## 2026-10-26 — Run 107: hang + slowdown from wide_split_r (branch `run107`, Claude)
- Hardware log of the run 106 build: watchdog hang at frame 1710 (main thread's last crumb g3e-done, never b-wait; worker finished its jobs) right after the 3D slider went 1.0 -> 0.0. Cause: `bWrSplit` (wide_split_r) was only set in stereo frames and never cleared, so the first mono wide-early frame after a stereo one waited forever on `wrDone` that the worker never signalled. Fixed: cleared every frame before the stereo block.
- Slowdown: slider-1 battle/menu frames got slower (composite 37-50 ms, engineA stage 45-51, 14-27 fps; run 105 build had 15-31 ms). The right eye used to run entirely on core 2 in parallel with the main thread's left eye; the static line split made main do its left eye plus half of the right and wait for the worker (engine B + its half). `wide_split_r` now defaults to OFF (0 = old behaviour); a version that lets whichever thread is free claim the next lines would be needed to make it a gain.
- Slider-0 battles in the same log are fine (28-30 fps, composite 4-13 ms), so `wide_split` and `battle_log` stay ON.

## 2026-10-26 — Run 106: battle frame log (branch `run106`, Claude)
- Run 105 hardware log (wide_split works, balanced 74/73 lines): slider-0 battles run 29-30 fps, composite 8-17 ms (run 103: 17-27), game thread idle 9-20 ms/frame, but the slowest frame of nearly every second is 40-56 ms (one lost vblank a second). The log could not say where it goes (SPIKE only logs frames over 70 ms).
- New Debug toggle `battle_log` ("Battle frame log", default ON, 0 = old): outside the field every frame over 40 ms logs the usual [SPIKE] stage lines (compose stages, B-wait, game-wait = vblank wait, romfs ms/misses, g3soft+game) plus field/slider/bottom drawn-or-reused/wide_split lines, at most 40 lines per stay outside the field (battle/menu), and a `[BATT]` summary line when each stay ends (frames, avg ms, counts over 34/40/50 ms, max, slider). First hardware log of it: 200 lines were used up by the title/menu at frames 2-4360 (the cap was global), and the slider was 1.00 all session. Only logging; spike_ms behaviour for field frames is unchanged (it only applies when spike_ms > 40).
- Not measured here: no toolchain in the cloud. Save time is not a stage (SaveData_Save shows up in [PROF] only).

- Hardware log of battle_log (slider 1.00 all session): 124 of 127 logged 3D-battle frames took 40-44 ms = game logic 13 + vblank wait 14 + compose 12, where the compose is 4 ms of own work plus 5-8 ms waiting for the core-2 worker (engine B, then the right-eye wide compose). New Debug toggle `wide_split_r` ("Right eye on 2 cores", default ON, needs wide_dirty=1 + wide_stereo_par): the right eye's dirty lines are shared by the main thread (after its left eye) and the worker (after engine B), like wide_split. WC_SplitPrepare/Run/Commit now take an eye. Host test covers both eyes. Not compiled in the cloud; log line `[WIDE] wide_split_r: ... right-eye lines main X / worker Y`.

## 2026-10-26 — Run 105: battle 30 fps work (branch `run105`, Claude)
- Touches widecomp.c and the stereo/compose path in render3ds.cpp (the other bot also edits these): everything is behind `wide_split`; nothing else in the stereo path changed.
- New Debug toggle `wide_split` ("Wide compose on 2 cores", default ON, 0 = run 104 behaviour). Mono only, needs wide_early=1 + wide_dirty=1. widecomp's changed lines are hashed on the main thread, dealt to two parts in 4-line groups (always to the part with fewer lines), the main thread draws part 0 while the core-2 worker, after engine B, draws part 1 (disjoint lines of the left-eye cache), then the main thread commits. API: WC_SplitPrepare/Run/Commit in widecomp.c. Expected: the mono compose (17-27 ms in the run 103 log) drops towards half. Stereo (slider on) is unchanged: left eye on main, right eye already on core 2.
- Fixed the PERF3 "bottom reused 4294967234": bSkips/bDraws are reset every 600 frames but the PERF3 baselines were not.
- Host test tools/widecomp_split_test.c: 200 frames of moving sprites + changing 3D layer, split picture == full redraw (all ok). widecomp_test.c still passes (both tests got a PSPNativeMemLog stub, they did not link before).
- NOT done / why: no compositor on the GPU (needs per-pixel priority + 3D alpha; gpucomp has no alpha), no g3soft pass cut, no vblank-wait removal (the wait is the game thread idle; compose needs that frame's final state, so overlapping it means a frame of latency or a state snapshot).
- NOT built or run: the cloud has no devkitARM and no ROM, so render3ds.cpp is not compiled and there are no emulator numbers; only widecomp.c is host-tested. Needs a build + hardware log: look for `[WIDE] wide_split=1`, `[WIDE] wide_split: ... lines main X / worker Y avg`, then [PERF] composite and fps in a battle at slider 0. Slider-on battles still need their g3soft/right-eye cost cut (not addressed).

## 2026-10-26 — Wireless M0 results + M1 local wireless layer (Claude; compiles and links on Nathan's PC with run 105, untested on hardware)
- Merged after run 105: `wireless` is settings item 84 (run 105's `wide_split` is 83).
- M0 (`tools/uds_probe`, two New 3DS): RTT p50 4-6 ms, p99 13-20 ms. 0.5-2.5% loss, all of it on broadcast frames; unicast lost nothing. Details in docs/WIRELESS.md.
- M1: `platform/wm_uds.c` + `platform/uds_ctru.c` replace pspoke's `wm_absent.o` in `full/Makefile`. New toggle `wireless` (Game tab "Local wireless (test)", default OFF; OFF = wm_absent.c code unchanged). ON = the DS WM API over 3DS UDS with unicast and resends. Log tag `[WIRELESS]`.
- Test plan: build, then on BOTH consoles with COPIED saves turn the toggle on, Pokemon Center 2F > Union Room, then trade one Pokemon. Send both log.txt files. Then check both saves with tools/savecheck.py and in DeSmuME.

## 2026-10-26 — Wireless / Wi-Fi feasibility (docs only, Claude)
- New `docs/WIRELESS.md` (same file in the SoulSilver repo). Local 3DS-to-3DS play (trade, battle, Union Room, Underground, Platinum port <-> SoulSilver port) is realistic by replacing pspoke's `wm_absent.c` with a UDS-backed WM layer. Local play with a real DS is not possible from native 3DS mode. Trades with legit games go online through a revival server (Wiimmfi), with GTS first.
- Recommended first milestone: M1 local trade between two 3DSes, after an M0 UDS latency probe. Nothing is built or tested. No code changed.

## 2026-10-26 — Run 104: battle frame drops (hardware run103 log)
- wide_3d_alpha works on hardware: no reason-4 decline in the run103 log (only reason 2/3 lines; reason 3 = battle intro windows).
- Recurring 70 ms battle-start drop (f=752/1174/1746/2301, tris 6, "capture..tComp" ~70) followed every `[HUNT] species N met` line: the shiny-hunt counter wrote hunt.txt synchronously (fopen/write/remove/rename on SD) on the game thread. The same write ran every 2 min during play. Fix: new Debug toggle `hunt_async_save` (default ON, same file contents): the text is built in memory and written by a low-priority thread.
- Load spikes (f=192..572, 70–425 ms) are romfs misses (20–156 x16K) during battle/area loads. f=2659 (159 ms, tris 621) is game-side, cause unknown.
- Steady battle cost: g3soft+game ~20 ms + composite (PERF2) 17–27 ms → 25–29 fps. Not reduced this run (g3soft speed-up, moving composite to core 2/GPU not done).
- SPIKE stage line now splits capture..tComp into cap / bwait+gc / misc / stereo (serial) / blob.
- field_prof now has AUTO (new default, 2): the profiler is on for 240 frames after every field<->battle switch, so the next hardware log has [PROF] lines for battle start/end. Overhead while armed not measured on hardware.
- Emulator: Coronet battle + send-outs OK, no crash. hunt_async_save path not exercised (trainer battle).

## 2026-10-25 — Run 103 (continued): Tropius send-out dip fix
- Hardware log (run101): send-out/battle-start spikes 80–84 ms, stage "capture..tComp" 51–53 ms, romfs 0. Cause: `[WIDE] drawing off (reason 4, BLDCNT 0841)` — the send-out fades the 3D Pokémon in with BLDCNT alpha on BG0, widecomp declined, the frame fell back to the full-engine stereo path.
- Fix: widecomp now blends the 3D layer by its per-pixel alpha onto the 2nd target (DS rule eva=a, evb=32-a, 31 opaque). New Debug toggle `wide_3d_alpha` (default ON; OFF = old decline).
- Emulator check: Coronet battle start + send-outs stay on the wide path (no reason-4 lines), wide_dirty=2 verify reported no mismatches, screenshots correct. Not measured on hardware yet.
- Hitch comparison: run101 battle-end game-side spikes 150–152 ms (run99: 112 ms, 87 ms game-side) — not improved, game-side (not compose), needs field_prof ON on hardware. Load spikes are romfs misses (45–369 ms), same as before.

## 2026-10-25 - Run 103 = v1.0 (Grok Bot), branch `run100` (PR #11), tag `v1.0` moved here

- lower_menu MAP now opens the Bag on the Key Items pocket (start_menu.c patch: P3DS_BagPocketReq ->
  BagCursor_SetFieldPocket, field RAM only, not the save). Emulator: Bag opened on KEY ITEMS with Town Map listed.
- wide_dirty one-line mismatch, investigated with wide_dirty=2 in the Spear Pillar double battle (emulator): reproducible,
  1 per ~600 composes, always line 52 (2 rows), ONE pixel, the stale pixel is a sprite colour where a full redraw gives
  black: x 49 (in OAM 9, a 64x64 battler at y 17) and x 25 (last column of OAM 8). The line hash did not change, the
  3D row did not change (tested a "3D landed late" re-hash: 0 hits, removed). Not fixed; widecomp.c now logs pixel, layer
  inputs and the OAM entries on the line in verify mode for the next attempt. Rolling refresh still bounds it to < 24 frames.
- Bug sweep (emulator, quick): field, start menu, save prompt, Bag, Pokétch, settings app, double battle with wide
  layouts: no new visual bugs seen. PC and a female save not tested.

## 2026-10-25 - Run 102 = v1.0 (Grok Bot), branch `run100` (PR #11), tag `v1.0` moved here

- lower_menu MAP: with the Town Map in Key Items (save read only) opens the Bag and toasts "Town Map: Bag > Key Items";
  without it toasts "No Town Map yet". (Opening the Town Map app directly needs game-code hooks; GabeSud0's fork has no
  MAP/POKETCH action either.) POKETCH: injects a tap on the Pokétch's upper side button (next app), 4 polls.
  Emulator: POKETCH switched to the next app; MAP opened the Bag with the toast.
- Lower menu is no longer drawn behind the settings app / quick menu (it was framing the settings screen).
- Play time in the settings app: emulator-checked ("Play time 43:06", matches the game's save screen).
- dc_flush_skip (Debug "Skip game cache flushes", default 1): game DC_FlushRange calls (one GSP IPC each; 70 in one
  field-load frame per field_prof) are skipped: no 3DS hardware reads game buffers directly in this port. 0 = old.
  Emulator: field, start menu save prompt, Bag and Pokétch all drew correctly.
- Not done: wide_dirty one-line mismatch (cause still unknown), battle/HUD wide polish, defaults otherwise unchanged,
  no further known bugs fixed.

## 2026-10-25 - Run 101 = v1.0 (Grok Bot), branch `run100` (PR #11), tag `v1.0` moved here

- Load-hitch profiler `field_prof` (Debug "Load-hitch profiler", default 0): `ov5prof.S` (generated by `full/genprof.py`)
  wraps 1440 functions (every cross-object call from overlay005 plus FS/NARC/overlay/heap/LZ entry points). Off: 4
  instructions per call. On: shadow-stack timing on the game thread; any frame >= 70 ms logs top self/inclusive functions ([PROF]).
- Found: every game heap free (NNS_FndFreeToExpHeap) ran pspoke's SDK_PORT WIN_CheckAndFree{AnimBank,CellBank,CharData},
  scanning 8192 table entries (64-bit compares) per free. Fix `heap_fast` (Debug "Fast heap free", default 1, `heapfast.c`):
  registrations (NNS_G2dGetUnpacked*) are mirrored in a short live list; a free with no live owner in its block skips
  the scan; otherwise the real scan runs. Same result (the live list is a superset).
  Emulator (Spear Pillar, menu open/close field reloads), heap_fast 0 -> 1: NNS_FndFreeToExpHeap self time over the run
  1232 ms -> 1.5 ms; field-load frame 594 -> 529 ms (the rest is emulator SD reads: FS_ReadFile 466 ms x1106, cached on
  hardware); frames >= 70 ms 37 -> 26. 4083 of 4110 scans skipped. Hardware gain not measured: the user should compare
  the battle-end [SPIKE] lines (run99 log: 87 ms game-side) and can turn field_prof on for a hardware profile.
- Lower menu: sets the bag icon from the player's gender (TrainerInfo_Gender, read-only). Already hidden outside the field
  (P3DS_InField is 0 in battles and start-menu apps). MAP still has no action (needs the Town Map app launched from the bag).
- Settings app: "Play time H:MM" box on the top screen (SaveData_GetPlayTime, read-only).
- UI: "Wild sprite test" and "Mirror fill (old)" removed from the Debug tab (config keys still work).
- Not done: battle/HUD wide polish (needs visual checks on hardware), defaults otherwise unchanged, no further known bugs fixed.

## 2026-10-25 - Run 100 = v1.0 (Grok Bot), branch `run100`, tag `v1.0`

- Version string "Platinum 3DS v1.0 (run100)" (settings title box, console banner "v1.0").
- Bottom icon menu (GabeSud0 port): `lowermenu.c` + `nitro_port_lowermenu.h` (credit header, THIRD_PARTY.md). Toggle
  `lower_menu` (Keys tab "Bottom icon menu", default 0). gpuscale draws the 320x240 menu picture and the DS lower screen into
  its panel; main3ds routes touches through NitroPort_LowerMenuTouch (panel touches reach the game); presses map to
  P3DS_StartReq (Pokemon 1, Bag 2, Trainer 3, Pokedex 0, Save 4, Options 5), item squares press Y, MAP/POKETCH no action.
  Shown whenever P3DS_InField (no overlay-005 patch, unlike GabeSud0's patch 0031); always the male bag icon (no gender read).
  Emulator (Spear Pillar save): menu drawn correctly, POKEMON and TRAINER taps opened those screens.
- Credits splash: Unown Collectables / 3DS Homebrew Dev, pspoke + pret pokeplatinum, GabeSud0, non-affiliation line; 3 s, A skips.
- Settings app: one/two-line help for the selected row on the top screen (settingHelp()).
- log_level 1: wide_haze, GPUC stereo, bottom_skip, present_skipbot, tearfix periodic lines only at log_level 2.
- Earlier this run: wide_dirty_regs=1 (Debug "Ignore timing registers").
- NOT done, with reasons:
  - Battle-end hitch: the 87 ms is one game-thread call (field map/overlay 5 setup inside the game's own load state); cutting
    it needs splitting that game code across frames or moving it off-thread with the game's heap, which is not safe without a
    proper game-code audit. No change.
  - Async 3D readback: the compositor needs that frame's 3D layer in the same frame; async readback means one frame of added
    latency, which the rules forbid.
  - Mono compositor split across cores: with wide_dirty on (default) most composes redraw only changed lines into a per-eye
    cache; splitting needs that cache shared between cores. Not done.
  - wide_dirty one-line mismatch: cause still unknown (rolling refresh bounds it).
  - Phase 2 playtime display: needs reading the game's play-time from save RAM; not done.

## 2026-10-25 - Run 99 (Grok Bot), branch `run99`

- Hardware log (run 98): battles 23-30 fps in stereo, composite 12-24 ms/frame; wide_dirty averages 58-108 of 192 lines per compose,
  13-35% full redraws, 0 verify mismatches logged (verify mode off on hardware).
- Hitch 1, Poke Ball send-out (f=2172, f=2610): 75-84 ms frames, 48-57 ms in "capture..tComp". widecomp declines those frames
  (windows), but run 96-98 had already turned off the parallel right eye, so the right eye was redrawn serially on the main thread.
  Fix: wide_early_check=1 (default) checks WC_Check first; declined frames keep the normal parallel stereo path (run 95 behaviour).
- Hitch 2, battle end (f=2253 / f=2691): 112 ms frame, 87-88 ms game-side, while overlay 5 + gFieldMapTemplate load (no ROM reads,
  no texture work), then a 78 ms frame. It is the game's own field reload on the game thread; not fixed this run.
- Emulator: build only (the user tests on hardware).

## 2026-10-25 - Run 98 (Grok Bot), branch `run98`

- wide_dirty=1 (Debug tab): widecomp hashes each line's inputs (registers/palettes/plan, BG map entries + sampled tile rows,
  sprites covering the line, the 3D row) and redraws only changed lines, plus 8 rolling lines a frame. Verify mode (=2) in Azahar, Spear
  Pillar stereo battle: 2 one-line mismatches in 1200 composes (cause not found); the rolling refresh bounds any such stale line
  to < 24 frames. Verify was not re-run after adding the refresh. Battle averages 71 of 192 lines per compose; about 23% of composes are still full (camera moves).
- Sprite pass scans only the line's sprite span (identical output, host test hash unchanged).
- Azahar stereo battle fps: run96 flat 13-22, run97 11-19, run98 10-23 (noisy; the emulator CPU is far slower than hardware for this
  code). For reference wide_layouts=0 gives 10-17 in the same battle, so battle cost in the emulator is mostly the 2D path itself.
  30 fps in stereo battles is NOT confirmed; needs a hardware log ([PERF] composite, [WIDE] wide_dirty lines).
- Battles use a 0-part wide plan (plain edge extension). Next ideas: GPU compose for 0-part plans with alpha blending, splitting
  the left-eye compose across cores in mono.

## 2026-10-25 - Run 97 (Grok Bot), branch `run97` (on top of run96)

- Hardware log (run 96): battles 22-29 fps with the slider at 1.00. Per frame: g3soft 16-23 ms (3D GPU draw + readback 7-14 ms),
  composite 13-25 ms. One 2.7 s freeze at the first battle: the stereo blob diag saw an empty right eye (with wide_early the
  right eye is only filled after the diag) and wrote 4 BMPs to SD. Fixed: the diag skips wide_early frames.
- Depth was lost in battles (run 96 copied the left eye). wide_stereo=1 composes the right eye with the right-eye 3D layer;
  wide_stereo_par=1 does it on core 2 alongside the left eye. Both eyes dumped in Azahar differ by about 3-4.5k px (3D layer shifted).
- Azahar, Spear Pillar double battle, stereo_fake=60: run 96 (flat) 13-22 fps, run 97 serial 6-17, run 97 parallel 11-19.
  Azahar's CPU is slower than hardware for widecomp, so hardware numbers are needed. Not a steady 30 yet.
- Next: make widecomp itself cheaper (it is the battle cost), e.g. recompose only rows whose inputs changed, or GPU compose for wide layouts.

## 2026-10-25 - Run 96 (Grok Bot), branch `run96` (on top of run95)

- Stereo battles with wide_layouts: widecomp's picture is used for both eyes (the right eye is a copy), so the right-eye engine A
  pass on core 2 and the left engine A pass were wasted. wide_early now also covers stereo (per-vertex 3D eyes): compose once,
  skip both passes. Azahar, Spear Pillar double battle, stereo_fake=60: 7-14 fps -> 13-22 fps. wide_early=0 = old path.
- Known, unchanged: with wide_layouts on, battles show no 3D depth (both eyes = widecomp's left picture); g3soft still renders the
  right-eye 3D layer that is then unused (next candidate speedup).
- CIA: build with the default `make cia` (animated 3D banner, ROM icon). BANNER=flat broke the HOME Menu banner/logo in run95.
- wide_early pixel check: party and bag shots with wide_early 0/1 differ only in the clock overlay and the party icon animation frame.

## 2026-10-25 - Run 95 (Grok Bot), branch `run95`

- New toggle `wide_early=1` (Debug tab "Wide layout early"): with `wide_layouts=1` the wide compositor runs while engine B renders
  and the redundant melonDS engine A pass is skipped. Identical output; menus/title about 40% less composite time in Azahar.
- New log lines: `[PERF4]` (game-thread V-blank wait), a second `[SPIKE]` stage line, `[WIDE] wide_early` and `[WIDE] parts set` (diag).
- Version string run95. Open: PC boxes / battle / shiny_suite / text_auto not emulator-tested this run; dirty-row widecomp is the
  next speedup; gpu_compose=2 readback skip not attempted.

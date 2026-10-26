# Handoff log

Dated entries, newest first. See AGENTS.md for the rules and PROGRESS.md for the full run log.

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

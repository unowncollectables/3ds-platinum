# Handoff log

Dated entries, newest first. See AGENTS.md for the rules and PROGRESS.md for the full run log.

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

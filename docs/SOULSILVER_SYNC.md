# SoulSilver -> Platinum sync

Platinum 3DS and SoulSilver 3DS share one platform layer (`platinum3ds/platform/`). It started in this repo; SoulSilver
forked it on 2026-10-25 (Platinum run 80, commit `0e374ca`) and then kept going for about 500 commits. This sync brings
SoulSilver's version of that layer back into Platinum with a three-way merge against that fork point, so Platinum's own work
since then (wide layouts, wide_gpu, frame pacing, follower hooks, lower menu, settings app) is kept where it still applies.

## Phase 1: platform layer (done, builds and links)

Carried over from SoulSilver, active as soon as the build runs:

| Area | What | Switch |
|---|---|---|
| 60 fps | in-between 3D picture in the overworld, hold, strips, VRAM, area, pipe | `fps60` and `fps60_*` (default ON) |
| 3D | worker thread (`g3thr.c`), packet shortcut (`g3packet.c`), transform cache, fast bounds, stereo GPU right eye (`g3s` shader), texture fence/safe, front overlap | `g3_*` (default ON, tuned values) |
| 2D | GPU engine B compose, exact fades, OBJ affine on the GPU, deferred output, overlap | `gpu_compose_b`, `gpu_b_fades`, `gpu_obj_affine`, `gc_direct` |
| Present | present pacing, screen hand-off, hidden-rows skip, 3D state cache | `present_pace`, always-on items |
| Text | sharp game text layer (`ss3ds_gametext.c`), sharp panel text | `sharp_game_text`, `sharp_text` |
| H-blank | line-scroll waves (water battle intro etc.) drawn by the renderer instead of crashing on a DS address | always |
| Audio | bit-exact DS mixer timing, even loudness, DSP watchdog, DSP released on exit, bounded fader | `audio_*` |
| Stability | soft-reset teardown order (workers joined first), safer save backup chain (one `.bak1`), log fsync, crash screen, watchdog for slow network | always |
| Save | `platinum.sav` stays the raw 512 KiB image; one validated `.bak1` (older `.bak2-8` removed after the next good backup) | `save_backup` |
| Wireless | automatic: local (UDS) or online (sockets) per session, decided by the game's own WM init; the Wireless setting is retired | automatic |
| GTS | StarMe server picker, last-seen deposit preview, login/log fixes | `gts_picker`, `gts_server` |
| Soft reset | (SoulSilver up to 08b73fd, 2026-11-25) wait for the old run's memory before carving heaps (`heapwait3ds.c`), stop leftover threads, DSP refcount fix, `[STALL]` lines | always |
| Perf | cached config lookups, boot speed check, zone prefetch, `[SPIKE]`/`[PERF4]` diagnostics | always |

Defaults: only platform / speed / display defaults came across (`kCfgDefaults` in `psp_ctru.c`). Anything that changes
Platinum gameplay (cheats, visible wilds, shiny odds, HM-free, ...) keeps Platinum's own default, so an existing
`config.txt` plays as before.

### Adapted, not copied

* `p3ds_ssglue.c`: Platinum's answers to the hooks the shared layer expects (title/Pokewalker/intro state, list cursor, field weather ...). Zero means "that SoulSilver-only screen never happens here".
* `gpucomp.c`: SoulSilver's compositor. Platinum's `wide_gpu` (GPU wide battle layout), `frame_pacing`, `gpu_mbright`, `gpu_blend_3d`, `gc_atlas16` are not on it. `frame_pacing` is replaced by the overlap path, `gpu_mbright`/`gpu_blend_3d` by fps_tweaks 3/4. **`wide_gpu` falls back to the CPU wide layout** (same picture, slower); porting it onto the new compositor is a follow-up.
* `widecomp.c` is Platinum's (wide layouts, dirty lines, split). Added from SoulSilver: `WC_StripHash`, `WC_MapColumn` (sharp game text). `WC_ComposeObjStrips` (wide HP boxes) is a stub: Platinum has its own wide battle layout.
* `render3ds.cpp`: SoulSilver's frame pipeline (H-blank lines, 60 fps in-between present, sharp text, overlay timing, `[PERF4]`) with Platinum's settings app, wide layout engine, lower menu, bottom strip and follower overlays kept.

## Phase 2: game-side features (not in this change)

Platform code for these is in place but inert until a Platinum hook feeds it: Mobile PC (`ss3ds_mobilepc.h`, SoulSilver hook 0293), virtual GBA Slot 2 migrate (`ss3ds_slot2.h`/`ss3ds_ctrdg.h`, hooks 0294-0297; Platinum's Pal Park would need its own C port), Black 2 animated battle sprites (`b2sprites.c`, hooks 0298-0300, not copied yet).

These need hooks in Platinum's decomp (`work_patches/`), not just the platform layer. Tracked in the PR description.

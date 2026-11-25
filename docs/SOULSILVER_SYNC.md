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
| Stability | soft-reset teardown order (workers joined first), log fsync, crash screen, watchdog for slow network | always |
| Save | unchanged on purpose: Platinum keeps its own validated 8-deep `.bak1-8` chain (SoulSilver's chain validates HGSS save footers and deletes `.bak2-8`, so it is not used) | `save_backup` |
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

## Phase 2: game-side features

SoulSilver's game hooks are Python scripts that edit pokeheartgold sources (`pspoke/scripts/ss3ds_hooks.d/NNNN_*.py`).
Platinum has no hook runner: its game changes are unified diffs of the generated overlay sources
(`work_patches/overlays/<dir>__<file>.c.patch`, pokeplatinum names). Each group below is one branch / PR, stacked in order.
All new options default OFF; none of them writes the save.

| Group | SoulSilver | Platinum | Switch |
|---|---|---|---|
| Battle Y companion + hunt stats | 0030 catch counter, 0031 `P3DS_FoeInfo`, 0208/0211 `P3DS_DmgInfo` (`battle_controller_player.c`); render3ds `cyOpen`/`hsOpen` wiring | same snapshots in `battle__battle_controller_player.c.patch` (BattleMon fields instead of party reads: same values); render3ds input + present wiring, lower menu and bottom strip hidden while the overlay is up; Y in battle no longer opens the settings app | `catch_overlay`, `dmg_check`, `hunt_stats` (ZR; not while `zr_item` is on) |

Still to come: visible wilds improvements, Pokedex finder chances, sharp game text hook (0289), Mobile PC (0293),
Slot 2 GBA migrate (0294-0297), Black 2 sprites (0298-0300), `wide_gpu` on the new compositor.

Nothing in phase 2 has been run on a 3DS or an emulator yet: each group is checked by compiling the patched overlay
object and linking `platinum3ds_full.elf` only.

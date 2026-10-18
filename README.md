# Platinum 3DS

A native **New Nintendo 3DS** homebrew port of Pokémon Platinum (US, Rev 1), built on top of
[pspoke](https://github.com/IbrahimIrfan/pspoke) (GPL-3.0), which compiles the community decompilation
([pokeplatinum](https://github.com/cybervisi0n/pokeplatinum)) for a non-DS target. This repository adds the 3DS
platform layer (citro3d/libctru), a GPU 3D path, a GPU 2D compositor, screen scaling / wide view, stereoscopic 3D,
audio fixes, a settings menu and HGSS-style following Pokémon.

> **Bring your own game.** This repository contains **no** ROMs, no game code or data extracted from a ROM, no
> graphics, audio or text from the games, no saves and no prebuilt binaries. You build everything yourself from the
> decompilation projects and **your own legally dumped cartridge**. Not affiliated with or endorsed by Nintendo,
> Game Freak, Creatures or The Pokémon Company. Do not ask for ROMs or builds.

New 3DS / New 2DS only (needs the extra RAM and CPU cores). Private personal project; experimental.

## What you need

| File | Required | SHA1 |
|---|---|---|
| Pokémon Platinum (US, Rev 1) `.nds` | yes | `0862ec35b24de5c7e2dcb88c9eea0873110d755c` |
| Pokémon SoulSilver (US, `IPGE`) `.nds` | optional (following Pokémon sprites) | tested dump `0959fe9e64dc749382460f1ccf90996f5ef4088d` (pspoke documents `f8dc38ea20c17541a43b58c5e6d18c1732c7e582`; the extractor only checks the game code) |

Build host: Linux (or WSL2) with Python 3 (+ `pip install ndspy pillow` for the follower / forwarder extractors),
git, make, and:

- **devkitPro** (`dkp-pacman`): `devkitARM` (tested r68), `libctru` (2.7.0), `citro3d` (1.7.1), `3dstools`,
  `picasso`, `general-tools`. Install per <https://devkitpro.org/wiki/Getting_Started>, then
  `sudo dkp-pacman -S 3ds-dev`. Set `DEVKITPRO=/opt/devkitpro`, `DEVKITARM=$DEVKITPRO/devkitARM`.
- Optional for the HOME-menu forwarder CIA: `makerom` and `bannertool` (not included; get them from their own
  projects).

## Repository layout

| Path | What |
|---|---|
| `platinum3ds/platform/` | the 3DS platform layer: main loop, input, threads, renderer glue (`render3ds.cpp`), GPU 3D (`g3gpu.c`), GPU 2D compositor (`gpucomp.c`), scaling/present (`gpuscale.c`), ROM cache, shaders |
| `platinum3ds/full/` | the full-game link (`Makefile`, linker script generator, `README_settings.txt` = every config option) |
| `platinum3ds/{source,shim,game,hosttest}` + `platinum3ds/Makefile` | early bring-up builds (kept for reference) |
| `toolchain-3ds/` | `psp-gcc`/`psp-g++` wrapper scripts and PSP-SDK header shims that make pspoke's build emit devkitARM objects |
| `pspoke-patches/` | our commits on top of pspoke (`git format-patch`, base commit in `BASE_COMMIT`) |
| `patches/` | per-feature patches applied to pspoke's generated work tree (audio, 3D, ROM fast read, ...) |
| `work_patches/` | the complete current delta of the generated work tree: `overlays/*.patch` against pokeplatinum `47c28d53` (the commit pspoke pins), `native/*.patch` against pspoke `port/`, plus our new header `p3ds_follower.h` |
| `followers/` | `extract.py` builds `followers.bin` from YOUR SoulSilver ROM (`hgss_luts.py` = species tables transcribed from the pokeheartgold decomp source) |
| `forwarder/` | optional HOME-menu forwarder CIA; `assets/extract.py` makes its icon/banner/sound from YOUR Platinum ROM |
| `scripts/` | `rebuild_all.sh` (full from-scratch build), `build.sh` (incremental) |
| `tools/` | `emu_smoke.sh` (headless Azahar test harness), `savecheck.py` (save block CRC check) |
| `PROGRESS.md` | the development log, run by run, with measurements |

The scripts still contain this project's absolute paths (`/workspace/platinum3ds/...`); adjust them to your checkout.

## Building

```sh
# 1. layout next to this repo's files
mkdir -p baserom roms upstream
cp /path/to/your/platinum.nds baserom/platinum_us_rev1.nds      # SHA1 above
cp /path/to/your/soulsilver.nds roms/soulsilver.nds             # optional

# 2. pspoke base + our pspoke commits
git clone https://github.com/IbrahimIrfan/pspoke.git
git -C pspoke checkout -b platinum3ds $(cat pspoke-patches/BASE_COMMIT)
git -C pspoke am ../pspoke-patches/*.patch
#    pspoke fetches its pinned third-party sources (decomps, libntr, ...) itself during the build.

# 3. full build: pspoke phases with our toolchain wrappers, then the 3DS link
bash scripts/rebuild_all.sh      # its final link is still without the work-tree changes below
#    after the generated tree exists (pspoke/.work), apply our work-tree changes:
#    patches/*.patch and work_patches/** (patch -p1 in the matching directory of
#    pspoke/.work/tree/test_out/...; copy work_patches/overlays/p3ds_follower.h to
#    native-audio-app/overlays/source/), then rebuild the touched objects:
bash scripts/build.sh            # incremental: rebuilds overlay objects + links platinum3ds/full
# result: platinum3ds/full/platinum3ds_full.3dsx

# 4. optional: following-Pokemon sprites from your SoulSilver ROM
python3 followers/extract.py roms/soulsilver.nds platinum3ds/full/followers.bin
```

The work-tree patches are reference diffs of the current state (the generated tree is not committed anywhere);
if a hunk does not apply, compare with `PROGRESS.md`, which documents every change.

## SD card layout

```
sdmc:/3ds/platinum3ds/
    platinum3ds_full.3dsx     (launch from the Homebrew Launcher; any name/location for the .3dsx works)
    platinum.nds              YOUR Platinum ROM (US Rev 1, SHA1 above)
    platinum.sav              the save (created when you first save in game)
    followers.bin             optional, from followers/extract.py
    config.txt                optional settings, one key=value per line
    log.txt                   written by the game (diagnostics)
```

Back up your save before trying new builds.

## Settings (`config.txt`)

All options and defaults are documented in [`platinum3ds/full/README_settings.txt`](platinum3ds/full/README_settings.txt).
Most can also be changed in game with **L+R+Y** (settings menu; saved to `config.txt`). Common ones:

| Key | Default | Meaning |
|---|---|---|
| `gpuscale` | 1 | top screen scale: 0 = 1:1, 1 = 1.25x, 2 = fill |
| `wideview` | 0 | 1 = wider overworld view (more world at the sides) |
| `fps_counter` | 0 | FPS counter (also L+R+START) |
| `follower` | 0 | 1 = lead Pokémon follows you (needs `followers.bin`) |
| `cam_zoom` | 0 | overworld camera -2..+2 |
| `stereo_strength` | 0 | 3D slider depth 1x / 1.5x / 2x |
| `split_top` | 1 | composite the top screen on two cores |
| `gpu_compose` | 0 | GPU 2D compositor: 0 off, 1 analyzer, 2 on (simple frames, pixel-identical in tests), 3 verify |
| `log_level` | 1 | 1 short log, 2 detailed |

Hotkeys: L+R+SELECT scale, L+R+X wide view, L+R+START FPS, L+R+START+SELECT soft reset, L+R+X+Y debug screenshots.

## Licences

This project is a derivative of pspoke and is distributed under the **GNU GPL v3** (`LICENSE`). See
`THIRD_PARTY.md` for the components it builds on and their terms.

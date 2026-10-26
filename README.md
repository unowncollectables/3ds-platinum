# Platinum 3DS v1.0

A native **New Nintendo 3DS** homebrew port of Pokémon Platinum (US, Rev 1), built on top of
[pspoke](https://github.com/IbrahimIrfan/pspoke) (GPL-3.0), which compiles the community decompilation
([pokeplatinum](https://github.com/cybervisi0n/pokeplatinum)) for a non-DS target. This repository adds the 3DS
platform layer (citro3d/libctru), a GPU 3D path, a GPU 2D compositor, screen scaling / wide view, stereoscopic 3D,
audio fixes, a settings menu and HGSS-style following Pokémon.

> **Bring your own game.** This repository contains **no** ROMs, no game code or data extracted from a ROM, no
> graphics, audio or text from the games, no saves and no prebuilt binaries. You build everything yourself from the
> decompilation projects and **your own legally dumped cartridge**. Not affiliated with or endorsed by Nintendo,
> Game Freak, Creatures or The Pokémon Company. Do not ask for ROMs or builds.

New 3DS / New 2DS only (needs the extra RAM and CPU cores). Private personal project.

**Version 1.0** (run 100): wide view and wide battle/menu layouts, stereoscopic 3D, following Pokémon, a settings app
(L+R+Y) with per-option help, optional bottom-screen icon menu (ported from GabeSud0), shiny-hunt tools, and an animated
3D HOME Menu banner for the `.cia`. The save format is unchanged: only the game writes `platinum.sav`.

**Credits:** port by Unown Collectables (with 3DS Homebrew Dev); built on [pspoke](https://github.com/IbrahimIrfan/pspoke)
and the pret/pokeplatinum decompilation; wide layouts, banner and the bottom icon menu adapted from
[GabeSud0/pokeplatinum-3ds](https://github.com/GabeSud0/pokeplatinum-3ds) (GPL-3.0). See `THIRD_PARTY.md`.

## What you need

- A **New Nintendo 3DS, New 3DS XL or New 2DS XL** (the older models do not have enough RAM or CPU) with custom
  firmware (Luma3DS) and the Homebrew Launcher. Optional: FBI, to install the HOME Menu version (`.cia`).
- **Your own dump** of the game, from your own cartridge:

| File | Required | SHA1 |
|---|---|---|
| Pokémon Platinum (US, Rev 1) `.nds` | yes | `0862ec35b24de5c7e2dcb88c9eea0873110d755c` |
| Pokémon SoulSilver (US, `IPGE`) `.nds` | optional (following Pokémon sprites) | tested dump `0959fe9e64dc749382460f1ccf90996f5ef4088d` (pspoke documents `f8dc38ea20c17541a43b58c5e6d18c1732c7e582`; the extractor only checks the game code) |

  Not sure which one you have? The builder below checks the file and says in plain words what is wrong
  (other region, Rev 0, trimmed, patched...).
- A Windows PC (the builder sets up WSL, a Linux inside Windows, for you) or a Linux PC, about **10 GB free disk**
  and an internet connection. **The first build takes an hour or more**; later builds take minutes.

## Build it: the easy way (recommended)

Use **[Pokemon 3DS Builder](https://github.com/unowncollectables/patch-builder)**, a one-window builder made for this
project. It installs the tools, downloads this project, checks your `.nds`, builds, and copies everything to your SD
card (backing up any save first). Its README has the 5 steps; in short:

1. Download the builder (green **Code** button, **Download ZIP**), unzip, install Python 3 from python.org, double-click
   `build_exe.bat` (or run `python p3ds_builder.py`).
2. Tick **Pokemon Platinum**, press **Browse** and pick your `.nds`. A green tick means it is the right file.
3. Plug in your SD card and press **Find it**.
4. Press **Build and install**. If something goes wrong, the red box says what to do; **Copy details for help** copies
   the full log for a GitHub issue.
5. Put the SD card back in the 3DS and start the game from the Homebrew Launcher (or install
   `cias/platinum3ds.cia` with FBI if you ticked the HOME Menu option).

## Build it by hand (Linux / WSL2)

The builder's script is also the reference command-line build; it runs every step below in order and is safe to repeat.

```sh
git clone https://github.com/unowncollectables/3ds-platinum.git ~/3ds-platinum
git clone https://github.com/unowncollectables/patch-builder.git ~/patch-builder
cd ~/patch-builder
sudo P3DS_ROOT=~/3ds-platinum bash p3ds_build.sh setup     # once: apt packages, devkitPro 3ds-dev, ndspy/pillow, /workspace link
P3DS_ROOT=~/3ds-platinum P3DS_PLATINUM=/path/to/platinum.nds \
  [P3DS_SOULSILVER=/path/to/soulsilver.nds] [P3DS_SD=/path/to/sdcard] [P3DS_CIA=1] \
  bash p3ds_build.sh all                                     # later: ... bash p3ds_build.sh quick
# result: ~/3ds-platinum/platinum3ds/full/platinum3ds_full.3dsx (and platinum3ds_full.cia with P3DS_CIA=1)
```

`bash p3ds_build.sh check` lists anything missing. What `all` does, if you want to do it yourself:

1. Tools: Python 3 with `pip install ndspy pillow`, git, make, patch, and **devkitPro** (`dkp-pacman`): `devkitARM`
   (tested r68), `libctru` (2.7.0), `citro3d` (1.7.1), `3dstools`, `picasso` (`sudo dkp-pacman -S 3ds-dev`, see
   <https://devkitpro.org/wiki/Getting_Started>). For the `.cia`: `makerom` and `bannertool` (the builder compiles them).
2. The project scripts use the absolute path `/workspace/platinum3ds`; make it point at your clone:
   `sudo mkdir -p /workspace && sudo ln -sfn ~/3ds-platinum /workspace/platinum3ds`.
3. Your ROMs: `baserom/platinum_us_rev1.nds` (SHA1 above) and optionally `roms/soulsilver.nds`.
4. pspoke plus our commits, inside your clone: `git clone https://github.com/IbrahimIrfan/pspoke.git`, then
   `git -C pspoke checkout -b platinum3ds $(cat pspoke-patches/BASE_COMMIT)` and `git -C pspoke am ../pspoke-patches/*.patch`.
5. `bash scripts/rebuild_all.sh` with `toolchain-3ds/bin` and devkitARM on `PATH`: the long from-scratch build of the
   generated tree (`pspoke/.work/tree/test_out`). It does not link the game yet.
6. Install `work_patches/` into the generated tree (`overlays/*.c.patch` into `native-audio-app/overlays/source/`,
   `include_*.patch` into `native-probe/pokeplatinum`, `native/*.patch` into `test_out`), recompile the touched overlay
   objects (`python3 rebuild_obj.py <files>` in `native-audio-app/overlays`), then `make` in `platinum3ds/full`.
   The builder's `apply_worktree.py` does this all-or-nothing and remembers what it applied.
7. Optional: `python3 followers/extract.py roms/soulsilver.nds platinum3ds/full/followers.bin`;
   `make cia` in `platinum3ds/full` for the HOME Menu version.

## Repository layout

| Path | What |
|---|---|
| `platinum3ds/platform/` | the 3DS platform layer: main loop, input, threads, renderer glue (`render3ds.cpp`), GPU 3D (`g3gpu.c`), GPU 2D compositor (`gpucomp.c`), scaling/present (`gpuscale.c`), ROM cache, shaders |
| `platinum3ds/full/` | the full-game link (`Makefile`, linker script generator, `README_settings.txt` = every config option) |
| `platinum3ds/{source,shim,game,hosttest}` + `platinum3ds/Makefile` | early bring-up builds (kept for reference) |
| `toolchain-3ds/` | `psp-gcc`/`psp-g++` wrapper scripts and PSP-SDK header shims that make pspoke's build emit devkitARM objects |
| `pspoke-patches/` | our commits on top of pspoke (`*.patch`, `git format-patch`, base commit in `BASE_COMMIT`); the `*.c` files there are old full-file copies kept for reference, not used by the build |
| `patches/` | per-feature patches applied to pspoke's generated work tree (audio, 3D, ROM fast read, ...) |
| `work_patches/` | the complete current delta of the generated work tree: `overlays/*.patch` against pokeplatinum `47c28d53` (the commit pspoke pins), `native/*.patch` against pspoke `port/`, plus our new header `p3ds_follower.h` |
| `followers/` | `extract.py` builds `followers.bin` from YOUR SoulSilver ROM (`hgss_luts.py` = species tables transcribed from the pokeheartgold decomp source) |
| `forwarder/` | optional HOME-menu forwarder CIA; `assets/extract.py` makes its icon/banner/sound from YOUR Platinum ROM |
| `scripts/` | `rebuild_all.sh` (full from-scratch build of the generated tree), `build.sh` (dev shortcut: recompiles `fieldmap.c` and links) |
| `tools/` | `emu_smoke.sh` (headless Azahar test harness), `savecheck.py` (save block CRC check) |
| `PROGRESS.md` | the development log, run by run, with measurements |

The scripts use the absolute path `/workspace/platinum3ds/...`; the builder (or step 2 of the hand build) links it to your checkout.

## SD card layout

```
sdmc:/3ds/platinum3ds/
    platinum3ds_full.3dsx     (launch from the Homebrew Launcher; any name/location for the .3dsx works)
    platinum.nds              YOUR Platinum ROM (US Rev 1, SHA1 above)
    platinum.sav              the save (created when you first save in game)
    followers.bin             optional, from followers/extract.py
    config.txt                optional settings, one key=value per line
    log.txt                   written by the game (diagnostics)
    save_backups/             the builder's copies of platinum.sav, one per install
sdmc:/cias/platinum3ds.cia    optional HOME Menu version (install with FBI)
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
| `gpu_compose` | 2 | GPU 2D compositor: 0 off, 1 analyzer, 2 on (simple frames, pixel-identical in tests), 3 verify |
| `log_level` | 1 | 1 short log, 2 detailed |

Hotkeys: L+R+SELECT scale, L+R+X wide view, L+R+START FPS, L+R+START+SELECT soft reset, L+R+X+Y debug screenshots.

## If something goes wrong

- **The builder says the ROM is wrong:** only US Platinum Rev 1 (`CPUE`, SHA1 above) works. Rev 0, other regions,
  trimmed or patched files do not.
- **Black screen or crash right at start:** check that `platinum.nds` is in `sdmc:/3ds/platinum3ds/` next to the
  `.3dsx`, and that you are on a *New* 3DS / 2DS.
- **Anything else:** keep `sdmc:/3ds/platinum3ds/log.txt` and open an issue with it (and, from the builder, the text from
  **Copy details for help**). Never attach ROMs or saves.

## Licences

This project is a derivative of pspoke and is distributed under the **GNU GPL v3** (`LICENSE`). See
`THIRD_PARTY.md` for the components it builds on and their terms.

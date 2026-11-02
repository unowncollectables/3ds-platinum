# Platinum 3DS

A personal, native **New Nintendo 3DS** port of Pokémon Platinum (US, Rev 1), built on
[pspoke](https://github.com/IbrahimIrfan/pspoke) and the pokeplatinum decompilation. It adds a 3DS platform layer
(libctru / citro3d), a GPU 3D path, a GPU 2D compositor, wide view, stereo 3D and a settings app.

> **Bring your own game.** You need your own legally dumped cartridge. This repository contains **no** ROMs, keys,
> saves, game assets or prebuilt builds. Not affiliated with Nintendo, Game Freak, Creatures or The Pokémon Company.
> Please do not ask for ROMs or builds.

New 3DS / New 3DS XL / New 2DS XL only (it needs the extra RAM and CPU cores).

## Features

- **Wide view**: more of the world at the sides, plus wide battle and menu layouts.
- **Stereo 3D** through the GPU 3D path, with adjustable depth.
- **Smoother battles and speed tweaks**: split-core top screen, fast upload / 3D convert, ROM cache, speed tweak levels.
- **Fast / Sharp 3D** modes (Sharp 3D is still experimental on hardware).
- **Audio styles**: Clean, Light or DS Raw.
- **Save backups and export**: rotating `platinum.sav.bak1-3` at launch, and an Export Save action with a read-back check.
- **Wireless**: local trading and play between ports over 3DS local wireless (UDS), and an online mode through the
  [Poké Classic Network](https://pkmnclassic.net/) community DNS (online is still being worked on, see [`docs/WIRELESS.md`](docs/WIRELESS.md)).
- **Settings app**: press **L+R+Y** in game; every option has help text and is saved to `config.txt`.
- **Shiny hunt tools**: encounter counters, odds, shiny alert and export of your hunts.
- **Lower menu**: an optional bottom-screen icon menu.
- Also: HGSS-style following Pokémon (needs your own SoulSilver dump) and an animated 3D HOME Menu banner for the `.cia`.

Most new features are **toggles in the settings app and are off by default**. Every option is listed in
[`platinum3ds/full/README_settings.txt`](platinum3ds/full/README_settings.txt).

## What you need

- A New 3DS family console with custom firmware (Luma3DS) and the Homebrew Launcher (FBI for the `.cia`).
- **Your own dump** of Pokémon Platinum (US, Rev 1), SHA1 `0862ec35b24de5c7e2dcb88c9eea0873110d755c`.
  Optional: your own Pokémon SoulSilver (US) dump for following Pokémon sprites.
- About 10 GB of free disk. The first build takes an hour or more.

## Build it: the easy way (Windows)

Download **Pokemon3DSBuilder.exe** from the [patch-builder releases](https://github.com/unowncollectables/patch-builder/releases)
and run it. It installs the tools, checks your `.nds`, builds, and copies everything to your SD card (backing up your
save first). See the [patch-builder README](https://github.com/unowncollectables/patch-builder) for details.

> **Windows SmartScreen:** if Windows says "Windows protected your PC", press **More info**, then **Run anyway**.
> This is normal for a new, unsigned download.

## Build it by hand (Linux / WSL2)

1. Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) (`sudo dkp-pacman -S 3ds-dev`), plus Python 3
   (`pip install ndspy pillow`), git, make and patch.
2. Put your ROM at `baserom/platinum_us_rev1.nds`.
3. Run the reference build script from patch-builder:

```sh
git clone https://github.com/unowncollectables/3ds-platinum.git ~/3ds-platinum
git clone https://github.com/unowncollectables/patch-builder.git ~/patch-builder
cd ~/patch-builder
sudo P3DS_ROOT=~/3ds-platinum bash p3ds_build.sh setup   # once
P3DS_ROOT=~/3ds-platinum P3DS_PLATINUM=/path/to/platinum.nds bash p3ds_build.sh all
```

That prepares pspoke with `pspoke-patches/`, runs `scripts/rebuild_all.sh`, applies `work_patches/`, then runs
`make` in `platinum3ds/full` (output: `platinum3ds_full.3dsx`). Run `make cia` there for the HOME Menu version.
`bash p3ds_build.sh check` lists anything missing.

## SD card layout

```
sdmc:/3ds/platinum3ds/
    platinum3ds_full.3dsx   launch from the Homebrew Launcher
    platinum.nds            YOUR Platinum ROM
    platinum.sav            your save
    config.txt              settings (written by the settings app)
sdmc:/cias/platinum3ds.cia  optional HOME Menu version
```

## Repository layout

| Path | What |
|---|---|
| `platinum3ds/platform/` | 3DS platform layer: main loop, input, renderer, GPU 3D/2D, scaling, wireless |
| `platinum3ds/full/` | full-game link (`Makefile`, `README_settings.txt`) |
| `pspoke-patches/`, `patches/`, `work_patches/` | our changes on top of pspoke and the decomp (diffs only) |
| `toolchain-3ds/` | wrappers that make pspoke's build emit devkitARM objects |
| `followers/`, `forwarder/` | following Pokémon data and HOME Menu assets, extracted from YOUR ROMs |
| `docs/`, `PROGRESS.md` | notes and the development log |

## Troubleshooting

- Only US Platinum Rev 1 works (not Rev 0, other regions, trimmed or patched files).
- Black screen at start: check `platinum.nds` is next to the `.3dsx` and that you are on a *New* 3DS / 2DS.
- Anything else: open an issue with `sdmc:/3ds/platinum3ds/log.txt`. Never attach ROMs or saves.

## Credits

- [pspoke](https://github.com/IbrahimIrfan/pspoke): the base this port is built on.
- [melonDS](https://github.com/melonDS-emu/melonDS): the 2D renderer used via pspoke.
- [pret/pokeplatinum](https://github.com/pret/pokeplatinum): the Platinum decompilation.
- [GabeSud0/pokeplatinum-3ds](https://github.com/GabeSud0/pokeplatinum-3ds): the lower menu and other menu/layout ideas.
- The sister [SoulSilver New 3DS port](https://github.com/unowncollectables/Pokemon-Soulsilver-New3ds), where several
  features (wireless, speed tweaks, audio styles) came from.

See [`THIRD_PARTY.md`](THIRD_PARTY.md) for details and licences. Licensed under the **GNU GPL v3** ([`LICENSE`](LICENSE)).

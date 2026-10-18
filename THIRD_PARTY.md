# Third-party components

Nothing below is vendored in this repository unless stated; they are fetched or installed at build time.

| Component | Use | Licence |
|---|---|---|
| [pspoke](https://github.com/IbrahimIrfan/pspoke) | base build system, platform code, patches; our changes are in `pspoke-patches/` | GPL-3.0 (`LICENSE`, copied from pspoke) |
| melonDS 2D renderer (`GPU2D_Soft.cpp`, via pspoke) | DS 2D compositing | GPL-3.0 |
| [pokeplatinum decomp](https://github.com/cybervisi0n/pokeplatinum) (pinned 47c28d53 by pspoke) | game source; `work_patches/overlays/*.patch` are diffs against it | no licence published (all rights reserved by its authors) - only small diffs are kept here, never whole files |
| [pokeheartgold decomp](https://github.com/pret/pokeheartgold) | `followers/hgss_luts.py` species->sprite tables were transcribed from it | no licence published |
| [libntr](https://github.com/cybervisi0n/libntr) and the other libraries pinned in pspoke's `third_party.lock` | DS SDK replacement | MIT / none published (see pspoke) |
| libctru, citro3d (devkitPro) | 3DS system libraries, linked at build time | zlib |
| devkitARM | compiler | GPL (toolchain) |

Pokémon and all game content are © Nintendo / Creatures / GAME FREAK / The Pokémon Company. No game content is
included; it comes from the user's own cartridge dump at build/run time.

Licence notes for a private repo: GPL-3.0 obligations (source availability) apply when binaries are *conveyed* to
others; keeping the repo private and not distributing builds is fine. If builds are ever shared, the full
corresponding source (this repo + pspoke base) must be offered under GPL-3.0. The unlicensed decomp projects are
the reason only diffs, not copies of their files, are stored here.

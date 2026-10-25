# AGENTS.md - handoff for any AI (Claude, Grok, Codex...)

**What this is:** `3ds-platinum` is Nathan's (`unowncollectables`) native New 3DS port of Pokemon Platinum (US Rev 1), built on pspoke (IbrahimIrfan/pspoke, GPL-3.0) retargeted from PSP to the 3DS with devkitARM. `psp_*` names in code are shims over real 3DS calls (libctru/citro3d). It is the **origin of most lessons and code** reused by the SoulSilver port (`unowncollectables/Pokemon-Soulsilver-New3ds`, which has the fullest AGENTS.md). Check here before inventing a fix there.

**Rules:** never commit ROMs, saves, extracted assets or builds; back up saves before any test and never touch a live save; experimental features get an in-game toggle (settings menu L+R+Y); work in a branch, PR to main, small diffs.

**Where things are:** `platinum3ds/platform/` 3DS platform layer; `platinum3ds/full/` full link + `README_settings.txt` (every config key); `pspoke-patches/` commits on pspoke (base in `BASE_COMMIT`); `patches/` and `work_patches/` per-feature and full-delta patches of the generated tree (against pokeplatinum `47c28d53`); `followers/` follower extractor (needs the user's SoulSilver ROM); `forwarder/` HOME-menu CIA assets; `scripts/` `rebuild_all.sh`, `build.sh`; `tools/` `emu_smoke.sh` (headless Azahar harness), `savecheck.py`; **`PROGRESS.md` is the run-by-run development log with measurements. Read it (it is large) before changing audio, threads, 3D or save code.**

**Build:** see README.md. Linux/WSL2, devkitPro `3ds-dev`. The scripts contain absolute `/workspace/platinum3ds` paths. Friendly one-button builder for non-developers: `unowncollectables/patch-builder` (Platinum only).

**Status:** playable port with settings menu, wide view, GPU 3D/2D, followers, stereo. Open issues are tracked in PROGRESS.md. `pokeplatinum-3ds` is a separate repo with a different design; do not mix the two.

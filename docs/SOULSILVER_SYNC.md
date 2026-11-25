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
* `gpucomp.c`: SoulSilver's compositor. Platinum's `wide_gpu` (GPU wide battle layout), `frame_pacing`, `gpu_mbright`, `gpu_blend_3d`, `gc_atlas16` are not on it. `frame_pacing` is replaced by the overlap path, `gpu_mbright`/`gpu_blend_3d` by fps_tweaks 3/4. `wide_gpu` fell back to the CPU wide layout after phase 1; phase 2 puts it back on this compositor (see below).
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
| Sharp game text | 0289: `bg_window.c` glyph / fill / rect / scroll / upload hooks, `unk_02013534.c` TextOBJ copy | `bg_window.c.patch` (`Window_CopyGlyph`, `Window_LoadTiles`, affine copies, `Window_FillTilemap`, `Window_FillRectWithColor`, `Window_BlitBitmapRectWithTransparency`, `Window_Scroll`, `Window_Remove`, `Windows_Delete`), new `unk_02012744.c.patch` (`sub_02012C60` = SoulSilver's `sub_02013A50`) | `sharp_game_text`: **default OFF on Platinum** until it has been seen on hardware (SoulSilver ships it ON); Video tab row |
| Encounter chances / finder | 0209/0286/0287 build the Pokegear encounter finder and its chances | not ported: they are Pokegear (Johto map) screens. Platinum's settings Map tab ("Wild Pok&eacute;mon here") already lists every species of the current map with level range and chance per method since run 65, and the game's own Pok&eacute;dex area map covers "where does it live" | none |
| Mobile PC | 0293: BOXES chip on the HGSS party screen, `field_control.c` launches `PCBox_LaunchApp` mode 2 after the Hall of Fame, first-time unlock fanfare + dialog | `overlay005__field_control.c.patch`: `FieldInput_Process` (where the Start menu opens) runs a field task: fade out, `FieldSystem_OpenPokemonStorage` (`PC_MODE_MOVE_MONS`, no journal entry), map back + fade in. Trigger is a "Boxes" tile in the settings Quick menu (Platinum has no SoulSilver party chip). Gate `SystemFlag_CheckGameCompleted` (read only). **Not ported:** the party-screen chip and its zoom, and the one-time unlock fanfare/dialog | `mobile_pc` (Game tab), `mobile_pc_dev` (Debug tab) |
| Black 2 sprites | 0298 (`pokemon.c` note, `pokepic.c` palette + draw), 0299 (every picture), 0300 (`Battle_Run` HP/status feel); `b2sprites.c` + NARC reader link wraps | `b2sprites.c` copied (pack read from `sdmc:/3ds/platinum3ds/`, else SoulSilver's folder; pokeplatinum NARC names `NARC_ctor`, `NARC_AllocAndReadWholeMemberByIndexPair`, `NARC_ReadWholeMemberByIndexPair`, ...); `pokemon.c.patch` (`BuildPokemonSpriteTemplate`), `pokemon_sprite.c.patch` (`BufferPokemonSpritePlttData`, `PokemonSpriteManager_DrawSprites`; negative `scaleX` = flip), `battle__battle_main.c.patch` (`Battle_Main` sets `P3DS_InBattle`, `BATTLE_STATE_MAIN` reports HP/status). `BuildPokemonSpriteTemplateDP` (the DP-style pictures some screens ask for) is not hooked, so those keep the game's art | `b2_sprites` (Video tab) |

`P3DS_InBattle` on Platinum was only set once a wild battle's command menu opened; `Battle_Main` now sets it for every
battle from its first frame (the field coming back clears it, as before).

| `wide_gpu` on the SoulSilver compositor | (Platinum's own run 111-114 feature, dropped by phase 1) | `gpucomp.c`: Platinum's clip-rectangle quads, `drawBGWide` (plan column maps), WIDE_OBJS sprite shift / stretched edge, 3D layer at `32 + dx3d`, centre-only brighten/darken on BG0, 512x256 target freed for the field wide view (`P3DS_GcWideRelease`), all behind a `gW` flag inside SoulSilver's `P3DS_GcComposeA`. Wide frames always wait for their own draw: no deferred output, no overlap, no 60 fps in-between picture. A rotating/scaling sprite now sends a wide frame to widecomp's CPU path (Platinum's old compositor never drew them at all). FOE DEEPER's per-row shift is not applied on wide frames. `render3ds.cpp` reads `wide_gpu` again (default 1) | `wide_gpu`, `wide_gpu_precheck`, `wide_gpu_verify` (as before) |
| Slot 2 (GBA migrate / Pal Park) | 0294-0297: a GBA SLOT button and pak list on the HGSS continue menu, `ss3ds_ctrdg.c` virtual CTRDG, `ss3ds_slot2.c` pak list + work copy, overlay 74 `PmAgbCartridge*` wraps | new `p3ds_slot2.c`: the same virtual CTRDG link wraps (no game hook needed: Platinum's `ov97_02235D18.c`, `gba_migrator.c` and `main_menu.c` already use the NitroSDK CTRDG API) and the same work-copy rule (`slot2/<name>.gba` + `.sav` read only, the game writes `slot2-work/<name>.sav`). **Not ported:** the continue-menu GBA SLOT button and list; the pak is chosen in the settings app instead (`GBA pak` = n-th `.gba` by name) and applies at the next start / soft reset, when the main menu looks at the slot. A pak in the slot also turns on Platinum's own GBA-slot wild Pok&eacute;mon, as a real pak does | `slot2`, `slot2_pak` (Game tab) |
| Renderer speed-ups | 0277 (`GPU2D_Soft.cpp`: sprite priorities bucketed in one OAM walk per scanline, disabled sprites skipped first), 0278 (display-list cache probes a packed key array: ~10% of busy CPU time in SoulSilver's profile). 0279's MI trim does not apply (Platinum's `mi_memory.c` has no address-window walk); its party-move cache is for HGSS's `GetIdxOfFirstPartyMonWithMove` | new `work_patches/native/native-render-opt_GPU2D_Soft.cpp.patch`; 0278 merged into `native-stack-render_frontend.cpp.patch` (Platinum's display-list cache is the same code). Same pictures, less CPU | always |
| Visible wilds | 0281 step aside for NPCs / tile kept in transit / scatter after battle, 0282 herds, alarm calls, curious ones, rain splashes, 0283 families, dusk and dawn, rain species, chases, cave Zubat, quirks, clock-seeded randomness, 0284 shore battles, 0288 Safari Zone (all inside SoulSilver's 3,400-line hook 0110, a different design: map-object based state in the port) | Platinum keeps its own run 73 `wild_vis` (`p3ds_follower.h`, port-drawn quads, grass only) and gets the parts that fit it: never onto an NPC or another wild (the old tile stays taken until a step ends), an NPC walking onto one makes it step aside or slip away, clock-seeded spawns (always on with `wild_vis`); with `wild_vis_alive`: pace quirks (calm / lazy / lively), looking around, curious ones coming over and stopping next to a still player, lively ones stepping away, the old ones going home when the time of day changes; with `wild_vis_cries`: panned, distance-quiet calls. **Not ported:** herds and families (young ones drawn smaller), alarm calls, rain species and splashes, chases, cave fly-bys, surf/shore wilds and the Safari Zone (Platinum's wilds are grass only), and scatter after a battle (Platinum's wilds all leave when a battle starts) | `wild_vis_alive`, `wild_vis_cries` (Hunt tab) |

Nothing in phase 2 has been run on a 3DS or an emulator yet: each group is checked by compiling the patched overlay
object and linking `platinum3ds_full.elf` only.

## Settings app (2026-11-25)

The settings app (L+R+Y, or hold the top-left corner of the touch screen) is now SoulSilver's design (`ss3ds_settings_ui.h`):
tabs LOOK / BATTLE / PLAY / PALS / SYSTEM, SELECT shows DEBUG, a scrolling list with the selected row's explanation on the top
screen. Every row is still the same config.txt key. The hunt stats page, the "Wild Pokemon here" list and the quick menu are
rows that open those pages of the old app.

Defaults now follow SoulSilver's for every key the two ports share (`psp_ctru.c` kCfgDefaults: battle Y screen, damage check,
type hint, hunt counter + ZR stats, visible wild Pokemon with behave + calls, SELECT reset, skip intro, IV/EV on summary,
reusable TMs, field moves without HMs, repel prompt, fast text 2X, FPS counter, sharp game text, FOE DEEPER ...). A line in
config.txt still wins, so anything already set in the settings app keeps its value.

## Ambient weather and seasonal trees (2026-11-25)

SoulSilver's weekly seasons (0248/0261/0268/0280, inside its 0110 hook), rebuilt for Sinnoh in `p3ds_follower.h`:
`ambient_weather` gives outdoor towns and routes whose own weather is clear showers (snow in winter, more often on Route 210
north, Route 211 east and Celestic Town) from one roll per area name and three-hour window. Only the picture changes
(`ov5_021D5F7C` / the three weather calls in `fieldmap.c`); the saved weather is never written, so battles and scripts see the
game's own weather. `seasonal_trees` tints the leaves of Platinum's tree palettes (`tree01`, `tree2_*`, `tree3_*`,
`conttree*`, `g1_tree*`, found in the ROM's map_tex_set.narc) when an area loads (`area_data.c`). Both default ON, like
SoulSilver. Not ported: the Johto weather radar and seasonal particles (petals, leaves).

## Event Pokemon without the event (2026-11-25)

SoulSilver's `event_any_mon` (Ilex shrine / Arceus scenes without event Pokemon), Platinum version: `scrcmd_item.c`
(`ScrCmd_CheckItem`: Member Card, Oak's Letter, Azure Flute, Secret Key count as in the bag) and `scrcmd.c`
(`ScrCmd_CheckDistributionEvent`: the Darkrai, Shaymin, Arceus and Rotom distributions count as received). Nothing is added
to the bag; the scripts' other conditions (game cleared, National Pokedex, ...) stay. Default ON like SoulSilver.

## Options that had no game data on Platinum (2026-11-25)

Earlier Platinum runs copied several SoulSilver platform features without the SoulSilver game hooks that feed them, so their
rows did nothing (or showed Johto content). Now fed from Platinum's game code:

| Option | SoulSilver hook | Platinum |
|---|---|---|
| Move type hint | 0121 + 0214 (`battle_input.c`) | `battle__battle_subscreen.c.patch`: `BattleSubscreen_SetupBackground` publishes `P3DS_MoveHint` while the move-select menu is up, clears it for any other menu |
| GTS server picker | 0270 (title menu: save's trainer id) | `main_menu__main_menu.c.patch`: `P3DS_GtsPreviewTrainer(TrainerInfo_ID(...))`. Without it the StarMe choice could never connect (`DWC_CheckValidConsole` needs it) |
| Achievements | 0145 + 0213 (`field_control.c`) | `achievements.c` list rewritten for Sinnoh (8 badges, Pokedex / Frontier / Hall of Fame milestones, Giratina, Platinum GameRecords ids); `p3ds_follower.h` `P3DS_AchFieldFrame` publishes every 2 s |
| Roamer + Day-Care tracker | 0120 (`field_control.c`) | `p3ds_follower.h` `P3DS_TrkFieldFrame` (roamers via `SpecialEncounter_*`, place names from the game's location bank, Day-Care compatibility / Masuda / egg); new settings page "Roamers & Day Care" (BATTLE tab) |
| Ribbon checklist | capture in `ss3ds_glue.c` | `applications__pokemon_summary_screen__main.c.patch` `SetMonDataFromMon` captures; `ribbons.c` groups ribbons by what Platinum can earn (Sinnoh Champ, Ability set, dailies, Gorgeous/Royal, Footprint, Effort, Super Contest) |

Checked and not needed on Platinum: 0197 (grey title options: Platinum only lists Ranger / Wii rows when such a device is
found, which never happens on a 3DS), 0226 steady music (HGSS soundplates), 0279 party move cache (Platinum's own HM check).

Platinum 3DS port - settings (sdmc:/3ds/platinum3ds/config.txt)
================================================================
Put one setting per line, like:   wideview=1
Anything not in the file uses its default. Delete a line to go back to the default.
If something looks or sounds wrong after a change, delete that line (or the whole config.txt).

HOTKEYS (hold L and R, then press the button; the game never sees these presses)
  L+R+SELECT  cycle screen scale: 1:1 / 1.25x / fill (remembered) - happens when you let go of SELECT
  L+R+X       wide view on/off (see more of the overworld at the sides)
  L+R+A       (does nothing: sharp 3D is disabled)
  L+R+START   FPS counter on/off - happens when you let go of START
  L+R+START+SELECT  soft reset, like the original game (the scale/FPS toggles do not fire with it)
  L+R+X+Y     save debug screenshots (BMP files) to the SD card
  L+R+Y       SETTINGS MENU (the game pauses). Up/down = choose, left/right or A = change, B or START = close.
              Changes: FPS counter, wide view, scale, C-stick mode, ZL run, ZR item, circle pad as D-pad,
              wide view effects, log detail, camera zoom, 3D strength, and the speed options below
              (fast upload, fast 3D convert, split top). The list scrolls (10 rows shown). They apply when you
              close the menu and are saved to config.txt.
  A small box at the top of the screen confirms each toggle.

FOLLOWING POKEMON (run 58)
  follower=0        1 = the first Pokemon in your party walks behind you in the overworld, HGSS style (HGSS sprites,
                    all 493 species, shiny colours, forms, female variants). Default 0 (new; turn it on to try it).
                    (Menu: FOLLOWING POKEMON)
                    Needs followers.bin next to the game in sdmc:/3ds/platinum3ds/ (made on the box from YOUR
                    SoulSilver ROM by followers/extract.py). Without that file the follower is simply off.
                    It is drawn by the port only: it is not a game object, is never written to your save, has no
                    collision and is invisible to the game's scripts. It hides on the bike, while surfing/using field
                    moves, in the Distortion World, while a story partner walks with you, and after warps / scripted
                    movement until you take your next step. Eggs don't follow.
  follower_behind=1 1 = the follower is always drawn behind you (no more covering your legs when it walks behind
                    you going up). 0 = old run-58 order. Default 1.
  follower_debug=0  1 = draw it two tiles right of the player (alignment test only). Default 0.
  follower_slot=0   which party Pokemon follows: 0 = LEAD (first in the party), 1-6 = that party slot. An empty slot
                    or an egg falls back to the lead. Stored in config.txt only, never in the save. Default 0.
                    (Menu: FOLLOWER SLOT)  [run 61]
  follower_talk=0   1 = press A while standing still and facing your follower: a short text box ("BOOTS IS HAPPY TO
                    BE WITH YOU!", 8 different lines) appears at the bottom of the top screen for ~2.6 s. Drawn by the
                    port over the picture: no game message, no script, nothing saved; it never shows if the game
                    starts something itself (hidden item, sign) and disappears when you move or a script starts.
                    Default 0. (Menu: FOLLOWER TALK)  [run 61]
  follower_sidefix=1  walking left/right the follower now faces the way it walks (run 58-60 showed the side views
                    mirrored the wrong way round). 0 = old look. Default 1.  [run 61]

PICTURE
  cam_zoom=0        overworld camera distance: -2 / -1 closer, 0 original, +1 / +2 farther (-14%..+16%). Zooming out
                    also makes fog areas look foggier. Default 0. (Menu: CAMERA ZOOM, applies at once)
  stereo_strength=0 3D slider depth: 0 = normal, 1 = 1.5x, 2 = 2x deeper (whole 3D world incl. people/Pokemon in the
                    overworld; battle backgrounds follow). Default 0. (Menu: 3D STRENGTH)
  gpuscale=1        screen scale at start: 0 = 1:1, 1 = 1.25x, 2 = fill. Default 1.
  wideview=0        1 = wide view in the overworld (more world at the sides). Default 0 (off).
  wide_cull=1       with wide view: also draw buildings/objects in the side areas. 0 = off. Default 1.
  land_fastload=1   with wide view: load new map blocks in one step (less pop-in at the corners). 0 = off. Default 1.
  wide_fadefix=1    with wide view: the side areas fade to black/white together with the screen on area changes
                    (no early peek at the new area). 0 = off. Default 1. (Only used when wide_fx=0.)
  wide_fx=0         with wide view: the side areas copy the middle picture's brightness, fades, wipes, flashes and
                    weather. SLOW (about 14 ms per frame on hardware) and can copy menu edges into the sides.
                    0 = off (wide_fadefix is used instead). Default 0.
  wide_fill=1       with wide view: screens without real wide 3D (battles, title, bag/party/Pokedex menus) fill the
                    sides with a soft mirrored copy of the picture's edge instead of black bars. Looks poor and can copy menu
                    edges into the sides. 0 = black bars (the run 54 look). Default 0.
  wide_menus=1      (run 65) with wide view and 1.25x: menus, battles, the title and other 2D screens fill the full
                    400 px width by continuing the DS's own background tiles past the screen edges (same 1.25x size as
                    the overworld, not mirrored, not blurred; fades/brightness follow). Sprites are not continued.
                    Costs about 0.3-1 ms per frame (unchanged frames reuse the last strips). 0 = off (wide_fill / black
                    bars as before). Default 1.
  encounter_test=1  (run 65, DEBUG ONLY) every step on any tile starts a wild grass battle. Default 0. Leave off.
  fog3d=1           DS 3D fog (Route 210 etc.; also in the wide view sides). 1 = done by the 3DS GPU's fog unit,
                    practically free. 2 = the run 55 exact method (much slower, about 6-10 ms per frame in fog areas
                    and on the title). 0 = no fog. Default 1.
  stereo_2d=2       3D slider, battles/menus: how far the background layers sit behind the screen (text, menus,
                    HP boxes stay at the screen). 2 = GENTLE (new default: back layer 3 px, middle 1-2 px per eye
                    at full slider). 1 = STRONG (run 55-60: 12 / 9 px - far more depth than the battle Pokemon
                    themselves have, which made the Pokemon look doubled/ghosted against the background).
                    0 = FLAT. Default 2. (Menu: BATTLE 3D DEPTH)  [run 61]
  wide_haze=1       with wide view: a 2D haze/fog layer that the game blends over the 3D world (foggy areas) is
                    continued into the side areas (tint measured from the picture's own edges, applied by the GPU;
                    ~0.1 ms). 0 = off. Default 1. (Menu: WIDE HAZE)  [run 61]
  battle_monobj_hide=1  fixes the Pokemon showing twice while a Poke Ball is thrown in battle. 0 = off. Default 1.
  (res3d / res3d_lindepth: sharp 3D is DISABLED since run 52 - broken on hardware; these lines are ignored.)
  tex_postvblank=1  3D sprites pick up their new picture in the same frame (fixes the one-frame coloured box
                    when getting on the bike, and late sprite frames). 0 = off. Default 1.
  stereo_depth=1    3D-slider depth for the 3D scene. 0 = flat 3D layer. Default 1.

SPEED (all safe defaults; only change if something looks wrong)
  gpu_swizzle=1     the finished screens go straight to the 3DS GPU (no CPU colour-swap pass per screen). Picture is
                    identical. 0 = old way. Default 1. (Menu: FAST UPLOAD)
  fast_conv=1       faster conversion of the 3D picture for the 2D compositor (same result, bit for bit). 0 = old.
                    Default 1. (Menu: FAST 3D CONVERT)
  split_top=1       3D slider OFF only: the top screen is composed by two CPU cores (top half / bottom half). Biggest
                    gain in battles and menus. 0 = one core (old). Default 1. (Menu: SPLIT TOP (MONO))
  bottom_skip=1     reuse the bottom screen picture when nothing changed (faster). 0 = always redraw. Default 1.
  present_skipbot=1 skip re-uploading an unchanged bottom screen (faster). 0 = off. Default 1.
  bottom_refresh=8  after scene changes, fully redraw the bottom screen for this many frames. 0 = off. Default 8.
  early_b=1         draw the bottom screen while the GPU draws 3D (faster). 0 = off. Default 1.
  split_top=0       1 = split the top-screen drawing over two CPU cores (2D/no-3D-slider only). Experimental,
                    untested on hardware. If the top screen shows a seam or wrong half, set 0. Default 0.
  rom_cache_mb=32   game data kept in RAM (MiB) to shorten loading hitches. 0 = off. Default 32 (uses less
                    automatically if memory is short: 24/16/8...).
  zone_prefetch=1   while you stand/walk around, quietly load the data of the areas you usually go to next, so
                    area changes hitch less. It learns as you play (first visit to an area is not faster) and
                    remembers in sdmc:/3ds/platinum3ds/prefetch.bin (safe to delete any time). It pauses while
                    the game loads, while saving and while the log is written. To turn it off: zone_prefetch=0.
                    Default 1.

SAVING
  save_anim=2       2 = our SAVING box replaces the game's save text; 1 = small corner box; 0 = off. Default 2.
  save_seq=2        2 = our box (with our YES/NO) covers the whole save prompt; 1 = without our YES/NO;
                    0 = show the game's own save text. Default 2. (The save itself is always the game's own.)
  save_minms=1500   how long (ms) "SAVING" shows at least. Default 1500.

BUTTONS (New 3DS)
  Circle pad  walks like the D-pad (4 directions, small dead zone).
  ZR          uses your registered key item (same as Y, e.g. the bike).
  ZL          running on/off (with it on you run without holding B; hold B to walk). A box shows RUN ON / RUN OFF.
  C-stick     works like the D-pad (walk, menus). With cstick_mode=1: up/down = Poketch side button instead.
  Soft reset (L+R+START+SELECT) closes the save file cleanly and restarts the game. If your homebrew loader can't
  restart it, the game just closes (nothing is lost; start it again).

CONTROLS / TOUCH
  cpad_dpad=1       circle pad moves like the D-pad. 0 = off. Default 1.
  zr_item=1         ZR = Y (registered item). 0 = off. Default 1.
  zl_run=1          ZL toggles running. 0 = off. Default 1.
  cstick_mode=2     C-stick: 0 = off, 1 = up/down flick presses the Poketch button (overworld only),
                    2 = C-stick moves like the D-pad (walking and menus, same as the circle pad). Default 2.
                    (the old cstick_poketch line is ignored now.)
  menu_combo=1      L+R+Y opens the settings menu. 0 = off. Default 1.
  fps_counter=0     FPS counter shown at start (the menu saves this; L+R+START still toggles). Default 0 (run 64).
  soft_reset=1      L+R+START+SELECT soft reset. 0 = off (the combo does nothing). Default 1.
  hotkey_swallow=1  hide hotkey presses from the game. 0 = off. Default 1.
  touch_latch=1     catch very quick taps on the touch screen. 0 = off. Default 1.

SOUND
  audio_latency=16  sound buffer (lower = less delay, higher = safer against crackles). Default 16.
  audio_lead=8      output buffer; if sound crackles, try 16. Default 8.
  audio_adapt=1     keep sound smooth when the game slows down. 0 = off. Default 1.

LOGGING
  stereo_blobdiag=1 with the 3D slider up, logs [STEREO] blob lines and saves stereo_blob_*.bmp once if one eye
                    shows a black patch the other doesn't (for the black-blob report). 0 = off. Default 1.
  gpu_compose=2     GPU 2D compositor for the TOP screen (engine A). Default 2 since run 61 (was 0; you confirmed on
                    hardware that it runs much better). 0 = off (old CPU path, unchanged).
                    1 = ANALYZE: only writes [GPUC] statistics lines to log.txt.
                    2 = GPU: frames that use only "simple" 2D features (text BGs, 3D layer, plain sprites, no
                        windows/blending/brightness/mosaic) are drawn by the 3DS GPU with a tile cache; every other
                        frame (battles with effects, fades, Cut, ...) uses the old CPU path automatically.
                        Azahar: overworld composite 14 ms -> 5.5 ms, pixel-identical (run 60).
                    3 = VERIFY (debugging): picture comes from the old path, the GPU result is made too and compared
                        pixel by pixel; log.txt gets "[GPUC] verify" lines, the first mismatches are saved as
                        gpuc_mmN_cpu.bmp / gpuc_mmN_gpu.bmp. Slower than 0. (Menu: GPU COMPOSE)
  gpu_compose_stereo=1  with gpu_compose=2 and the 3D slider up: overworld frames are GPU-composed for BOTH eyes
                    (before run 61 the 3D slider always forced the slow CPU path: ~17 ms, 25 fps). 0 = old.
                    Default 1. (Menu: GPU COMPOSE 3D)  [run 61]
  menu_audio=1      music and sounds keep playing while the L+R+Y settings menu is open (the game itself stays
                    paused). 0 = frozen like before. Default 1. (Menu: MENU MUSIC)  [run 61]
  log_level=1       1 = short log.txt; 2 = detailed diagnostics (writes much more to the SD card and can cost
                    some speed - only for bug hunting). Default 1.

Everything else in old config files (stereo_c1, steal_b, gpu3d, tearfix, rb_sync, ...) is for diagnosis only;
leave it out unless asked.

stereo_edgefix=1        (run 62, default 1) 3D slider + wide view: keep the 3D triangles that the per-eye stereo
                        shift brings on screen at the far left (right eye) / far right (left eye). Fixes the thin
                        black sliver at the left edge of the right eye. 0 = old behaviour.

fog_animfix=1           (run 64, default 1) Fixes the player's walk animation freezing (or facing the wrong way)
                        after a seamless map change such as Route 210 South -> North in the fog. The game freed the
                        player's walk-frame table while the sprite still used it; the port now keeps it. 0 = old.

credits_splash=1        (run 64, default 1) Boot credits "Patch by Unown Collectables & 3DS Homebrew Dev" on the top
                        screen while the game starts (drawn by the port in the game's own font, read from your
                        ROM). A skips it. 0 = off.
splash_ms=2000          (run 64) Splash length in ms including fades (300..5000).

ui_gamefont=1           (run 64, default 1) Port text boxes (follower talk, toasts, save box, settings app) use the
                        game's own font and message-box frame, read from your ROM at start. 0 = old port font.
ui_frame=0              (run 64) Which of the game's 20 message-box frames the port boxes use (0..19).
settings_app=1          (run 64, default 1) L+R+Y opens the touch settings app on the bottom screen (tabs Video /
                        Speed / Game (2 pages: tap the tab again) / Cheats / Menu / Map; tap a cell to change it,
                        L/R switch tabs, B closes). The game is paused while it is open; the Poketch comes back
                        when it closes. Menu tab: tap Pokemon / Bag / Pokedex / Trainer card / Save / Options to
                        open the game's own screen. Map tab (run 65): where you are and a grid of this map's wild
                        Pokemon - each with its party icon (read from your ROM), encounter chance (the game's slot
                        rates, duplicates summed) and level range - in sections Grass / cave (day or night table),
                        Surfing, Old Rod, Good Rod, Super Rod. Tap the panel for the next page. Built once per map
                        when you stand still; swarms and the Poke Radar are not included. 0 = the old text list on the top screen.
fast_text=0             (run 64, default 0 = off) 2, 3 or 4: the game prints that many letters per frame. Stops at
                        every pause / button prompt, so no text is skipped. Also in the settings app (Game tab).
reusable_tms=0          (run 64, default 0) 1 = TMs are not used up when you teach them. Also in the settings app.
follower_gap=1          (run 64, default 1) The following Pokemon walks a bit farther back (more for big Pokemon), so
                        it no longer covers your head. 0 = old spacing.
follower_area_talk=1    (run 64, default 1) With follower_talk=1: what your Pokemon says depends on where you are
                        (town, route, cave, lake, legendary place), the weather, the time of day and its type.
follower_cry=1          (run 64, default 1) Your Pokemon cries when you talk to it (the game's own cry sound).
follower_items=0        (run 64, default 0) 1 = now and then, after you talk to it, your Pokemon finds a common item
                        (Potion, Poke Ball, Repel, berries ...). It is put in your Bag the normal game way, so your
                        save stays a normal Platinum save. At most once every 120 steps.
follower_ballpop=1      (run 64, default 1) After a door, warp or cutscene your Pokemon comes out of its ball: the
                        game's ball sound and it grows in for a moment. 0 = it just appears.
settings_touch=1        (run 66, default 1) Overworld touch shortcuts in the bottom screen's corners where the Poketch has
                        no screen or button. Touch and HOLD about 0.3 s: top-left = this settings app (same as L+R+Y),
                        top-right = Pokemon (party), bottom-right = Bag. Party and Bag are the game's own screens, opened
                        through the game's own Start menu (same as the Menu tab), so saving works normally. A short tap
                        in a corner does nothing. 0 = off.
Cheats tab (run 67, every cheat OFF by default, stored in config.txt; values written to the save are ones real
Platinum accepts):
cheat_money=1           Infinite money: in the overworld your money is kept at 999,999 (the game's own maximum).
cheat_catch=1           Always catch: every Poke Ball thrown in a wild battle catches (the game's own "always catch" result).
cheat_shiny=1           Every wild Pokemon is shiny (uses the game's own Poke Radar shiny code; Cute Charm/Synchronize kept).
cheat_eggs=1            Fast eggs: eggs in your party hatch after a few steps (egg cycles set to 0, the normal hatch event).
cheat_exp=1 / 2         EXP boost: battle EXP x2 / x3.
cheat_candy=1           Rare Candy 99: in the overworld your Bag is topped up to 99 Rare Candies (the Bag allows 999).
cheat_no_encounters=1   (run 64) no random wild battles.
Debug tab (run 67): the old Speed tab (fast upload, fast 3D convert, GPU compose, GPU compose 3D, log detail) is hidden;
press SELECT inside the settings app to show / hide it. The config.txt keys still work as before.
Run 67: config.txt is read from the SD card once and then kept in memory (it used to be re-read after every map
change / battle, ~13 ms each on hardware).
lid_sound=1             (run 64, default 1; run 65 sounds) Closing the lid plays the game's Poke Ball recall
                        (return) sound, opening it the Poke Ball release (send-out) sound - the same sounds as in battle
                        (overworld only; the music keeps playing / resumes). NOTE: those two sounds are in the game's battle
                        sound bank, which the game doesn't keep loaded in the overworld; when they can't start (always so far
                        in testing) the game's overworld Poke Ball sound (the Poke Center ball sound) plays instead. 0 = silent.
lid_test=1              (run 65, DEBUG ONLY) fakes a lid close / open about 5 and 7 s into the overworld. Default 0.
summary_ivs=1           (run 64, default 1) On the summary's Skills page (the stats page) press Y to show / hide a box
                        with the Pokemon's IVs and EVs, in the order HP/Atk/Def/SpA/SpD/Spe. Read only.
                        Also in the settings app (Game page 2).
repel_prompt=1          (run 64, default 1) When a Repel wears off and you still have one, after the game's "Repel's
                        effect wore off" message you are asked "Use another Repel?" (Up/Down, A = yes, B = no).
                        Yes uses it exactly like the Bag does (one taken from the Bag, normal Repel length).
                        Uses the kind you used last (else Repel, Super Repel, Max Repel). Game page 2 of the app.
cheat_no_encounters=0   (run 64 cheat, default 0) 1 = no random wild battles while walking / surfing; Sweet Scent and
                        Great Marsh mud encounters are blocked too. Fishing, Honey trees and scripted /
                        legendary Pokemon are unchanged.
                        Nothing is written to your save. Settings app, Cheats tab.

Run 73
  wild_vis=0          visible wild Pokemon in grass (port-drawn, nothing saved); walking into one starts that battle
  wild_vis_max=2      how many at once (0..4)
  wild_vis_test=0     debug: one wild_vis_species sprite 2 tiles in front of the player (drawing test, no battle)
  wild_vis_species=25 species for wild_vis_test
  wild_vis_replace=0  1 = random grass encounters are off while wild_vis is on (only the visible ones battle)
  (no new key) wide_weather copies only grouped weather particles (no cursor / lone sprites); wide_edgefill is off
  indoors and only extends repeating background; wide_menus continues only the screen background layer; wide-view
  strips hold still during attack screen shake. 3D vertex buffer raised (Spear Pillar cutscene abort).
cheat_pp=0  (1 = your moves use no PP in battle; runtime only)
cheat_speed=0        (1 = holding B runs at bike speed; runtime only)
cheat_walk_walls=0   (1 = hold R while walking to pass through walls/objects; warps and ledges still work)
cheat_time=0         (0 off, 1 morning, 2 day, 3 evening, 4 night: overrides time-of-day only; the save clock is untouched)
catch_overlay=0      (1 = in a wild battle, press Y at the command menu for the Ball tab: species, nature, ability, IVs, Pokerus, HP/status, catch odds)
battle_anim_speed=0  (1 = skip plain move animations; Fly/Dig/Dive/Bounce/Shadow Force/Transform/Substitute/Minimize still play)
Run 76
  battle_anim_speed=2  (also makes the HP bar drain 3x faster; damage and end values unchanged)
  display_capture=1    (screen capture used by the Spear Pillar / Distortion World transition; 0 = old behaviour)
  dw_depth=2           (Distortion World 3D depth: 0 off, 1 subtle, 2 moderate, 3 strong; eases in over ~0.75 s)
  dw_debris=0          (1 = drifting debris and dust over the Distortion World, with depth in 3D)
  dw_unease=0          (1 = occasional uneasy line from your Pokemon in the Distortion World; text only)
  dw_cry=0             (1 = a distant Giratina cry now and then in the Distortion World)
  hunt_counter=0       (1 = counts wild encounters per species in hunt.txt next to config.txt; shown on the Ball tab)
  tod_tint=0           (1 = soft evening / night tint outdoors; follows cheat_time when set)
  bottom_vramgen=1     (Underground fix: bottom screen redraws when its VRAM is written; 0 = old reuse)
  Ball tab now also lists Dusk and Net Ball odds. Log adds [PERF3] (worst frame, bottom reuse) every ~10 s.
  item_ping=0          (1 = soft Poketch dowsing blip when a hidden item comes within 2 tiles; read-only)
Run 77
  touch_menu=0         (1 = touch and hold the bottom-left corner of the bottom screen for 0.3 s in the overworld: quick menu
                        with Pokemon / Bag / Pokedex / Trainer card / Save / Options, Map tab next to it; uses the real Start menu)
  dw_roll=0            (1 = Distortion World: ~400 ms damped tilt plus a small 3D depth pulse when you move to a new gravity plane)
Run 78
  bottom_strip=0       (1 = overworld only: Menu / Wide / Ball / Set buttons in the bottom border below the Poketch; taps there never reach the Poketch. Wide toggles wide view for this session only)
Run 79
  (no new keys) The HOME Menu .cia (make cia) uses the same sdmc:/3ds/platinum3ds/ config.txt, platinum.sav and log.txt as the .3dsx.
Run 80
  text_hold_b=0        (1 = holding B prints text instantly and advances finished text boxes; never picks yes/no or menu choices)
  text_auto=0          (1 = text boxes advance by themselves after ~1.5 s; choices still wait for you)

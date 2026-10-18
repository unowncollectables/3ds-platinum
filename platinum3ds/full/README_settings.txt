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
  fog3d=1           DS 3D fog (Route 210 etc.; also in the wide view sides). 1 = done by the 3DS GPU's fog unit,
                    practically free. 2 = the run 55 exact method (much slower, about 6-10 ms per frame in fog areas
                    and on the title). 0 = no fog. Default 1.
  stereo_2d=1       3D slider: in battles the background layers sit behind the screen (text and menus stay at the
                    screen). 0 = off. Default 1.
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
  fps_counter=1     FPS counter shown at start (the menu saves this; L+R+START still toggles). Default 1.
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
  gpu_compose=0     GPU 2D compositor for the TOP screen (engine A). Default 0 = off (old CPU path, unchanged).
                    1 = ANALYZE: only writes [GPUC] statistics lines to log.txt.
                    2 = GPU: frames that use only "simple" 2D features (text BGs, 3D layer, plain sprites, no
                        windows/blending/brightness/mosaic) are drawn by the 3DS GPU with a tile cache; every other
                        frame (battles with effects, fades, Cut, ...) uses the old CPU path automatically.
                        Azahar: overworld composite 14 ms -> 5.5 ms, pixel-identical (run 60). Not yet tested on
                        hardware - try it and report.
                    3 = VERIFY (debugging): picture comes from the old path, the GPU result is made too and compared
                        pixel by pixel; log.txt gets "[GPUC] verify" lines, the first mismatches are saved as
                        gpuc_mmN_cpu.bmp / gpuc_mmN_gpu.bmp. Slower than 0. (Menu: GPU COMPOSE)
  log_level=1       1 = short log.txt; 2 = detailed diagnostics. Default 1.

Everything else in old config files (stereo_c1, steal_b, gpu3d, tearfix, rb_sync, ...) is for diagnosis only;
leave it out unless asked.

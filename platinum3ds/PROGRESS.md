
## scale-modes (19:0x build)
- L+R+SELECT used to overwrite config.txt with only "gpuscale=N", wiping every other setting (gpu3d_cpuclip, cull3d,
  g3cache, audio_*, tearfix...). saveScale now rewrites only the gpuscale line.
- 3 modes again (cycle 0->1->2): 0 = 1:1 CPU 256x192; 1 = GPU 1.25x 320x240 pillarboxed (default); 2 = GPU 1.5625x
  400x300 aspect fill, crops 30 rows top/bottom (no stretch). Geometry: 256x192 * 1.5625 = 400x300, 4:3 kept.
  Crisp: NEAREST into a 2x 512x384 intermediate, then LINEAR down (sharp-bilinear). Log: "[3DS] screen mode N: ...".

## perf-chunk3 (19:0x build)
- Stereo (right eye on core 2): removed two 196 KB memcpys per frame (stereoL <- left, display[0] <- right); present now
  reads display[0] (left) and rawR (right) in place. Same pixels, ~0.3-0.5 ms/frame in stereo. No toggle (pure plumbing).
- Not done: box-side synthetic profiling of GPU2D_Soft (needs an x86 build of the melonDS 2D core + pspoke glue);
  compositing straight into the GPU texture (the compositor writes 0x00BBGGRR, the PICA needs RGBA order and a tiled
  layout; the bswap+display transfer is the cheap form of that already).

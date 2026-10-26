# uds_probe (wireless milestone M0)

A tiny standalone 3DS app, separate from the game, that measures 3DS-to-3DS local wireless (UDS) with the packet
pattern Platinum and SoulSilver use for DS wireless: the host sends 192 bytes per cycle at 60 or 30 Hz and each
client answers with 38 bytes. See `docs/WIRELESS.md`. The app doesn't touch the game or any save. It only appends
results to `sdmc:/3ds/uds_probe.txt`.

Build (devkitPro `3ds-dev`): `make` makes `uds_probe.3dsx`. Copy it to `sdmc:/3ds/` on both consoles.

Run:
1. Turn wireless on for both 3DSes. Open uds_probe from the Homebrew Launcher on both.
2. On console 1 press **A** (host). On console 2 press **X** (join). The test starts 3 s after the client joins and
   runs for 60 s. Keep the consoles about 1 m apart for the first run.
3. Press UP/DOWN before starting to switch the rate between 60 and 30 Hz (do one run of each). START quits.
4. Send `sdmc:/3ds/uds_probe.txt` from both consoles.

What it answers for M1: round-trip time (p50/p95/p99), lost and late replies, and how evenly the host's packets
arrive at the client. If p99 RTT stays under one cycle (16.7 ms at 60 Hz) with almost no loss, a frame-paced WM
layer is fine. If not, M1 needs wall-clock pacing and more slack.

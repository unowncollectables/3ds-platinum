# Wireless and Wi-Fi: feasibility and plan

Written 2026-10-26 for both native ports (Platinum `3ds-platinum`, SoulSilver `pokemon-soulsilver-new3ds`). The same
file lives in both repos; keep them in sync. Nothing here is built yet. This is research plus a plan.

Nathan's ask: trading between the port and legit games, the Underground with other players, "everything like normal",
in Platinum and SoulSilver.

## Short answer

| Goal | Realistic? | How |
|---|---|---|
| Port to port, local (two 3DSes in one room): trade, battle, Union Room, **Underground**, Platinum to SoulSilver trades | **Yes.** Best first target | Replace pspoke's fake DS radio with one that runs on the 3DS's own local wireless (UDS) |
| Port to a real DS or a real cart, **local** wireless | **No** (as far as anyone knows) | The 3DS in native mode can't speak the DS's wireless protocol. See "Why not local with a real DS" |
| Port to a real DS or cart, **online** (GTS, Wi-Fi Club trades and battles, maybe Wi-Fi Plaza) | **Yes, harder** | Run the game's own Wi-Fi code over 3DS sockets and point it at a revived Nintendo WFC server (Wiimmfi). The real DS joins the same server |
| Port to a real cart, no wireless at all | **Already works** | The save is byte-compatible with a cart. PKHeX or a save manager can move Pokemon between saves today |
| Underground with a real DS | **No** | The Underground is local-only on the DS too (there is no online Underground), so this needs local DS wireless |
| Voice chat, DS Download Play, Pal Park (GBA slot) | No / not worth it | No GBA slot. Voice chat (VCT) stays stubbed |
| HGSS Pokewalker (infrared) | Maybe, later | The 3DS has an IR port (`ir:USER`). It has not been checked whether it can talk to a Pokewalker |

So "everything like normal" splits into two parts. **Local play works between ports** (3DS to 3DS, Platinum and
SoulSilver mixed). **Playing with legit games works online through a revival server**, not locally.

## What the ports do today

- Both ports link pspoke's `port/native-audio-app/services/wm_absent.c` (Platinum: `platinum3ds/full/Makefile`
  `APPOBJS`; SoulSilver: through pspoke's object list). It implements the whole DS `WM_*` API as **a radio with nobody
  in range**: every call succeeds a frame later, scans never find a parent, a parent sends beacons that nobody
  answers. The game takes that as a DS on its own, so the Underground runs solo and Mystery Gift searches forever.
  Callbacks run from `PSPNativeWMPump()` in `frame.c`, on the main thread between frames (where the DS's FIFO
  interrupt would fire). Wi-Fi calls (`WM_StartDCF`, `WM_StartScanEx`, `WM_SetWEPKeyEx`) fail at once.
- The game's own Wi-Fi stack is **already compiled as C**: pspoke builds cybervisi0n's decompiled `libntrdwc`
  (NitroDWC: login, matchmaking, GTS, ranking, friends) and `libntrwifi` (NitroWiFi: WCM access-point manager,
  CPS TCP/IP, SOC sockets, SSL) in `port/native-probe/compile-network.py`. The Platinum run 74 audit counted 141
  `DWC_` and 35 `WM_` symbols linked, with no 3DS network service (soc/ac/uds) used.
- `libntrwifi` already has non-DS backends: on Linux, Windows and Switch (libnx) its CPS layer
  (`libraries/cps/src/network.c`) maps the DS sockets straight onto BSD sockets (`socket`, `connect`,
  `gethostbyname`). 3DS libctru offers BSD sockets after `socInit`, so a 3DS backend is a port of an existing
  path, not new code.
- `native-offline/offline.c` stubs voice chat (VCT) and `getlocalhost`.
- Both CIA `.rsf` files already list `nwm::UDS`, `soc:U`, `ac:u`, `ssl:C` and `ir:USER`. The `.3dsx` under Luma can
  reach every service. No permission work is needed.
- GabeSud0's `pokeplatinum-3ds` doesn't port networking either. Its Makefile compiles the NitroDWC and NitroWiFi
  headers and links generated stubs (`tools/make_game_wifi_stubs.py`, "networking is not ported"). There is
  nothing to borrow there yet.

How the games use local wireless (pret/pokeplatinum `src/wireless_manager.c`; HGSS is the same design): a classic DS
**MP** session. One parent and up to 7 children (the Underground allows 5 players). The parent sends one packet per
MP cycle, at most 192 bytes. Each child answers with 12 bytes (38 bytes in the Underground). Beacons carry `WM_SetGameInfo`
data (GGID and the game's own info, at most 112 user bytes). `CommSys` / `comm_ring.c` sit on top of that and buffer
commands in rings, which tolerates a little jitter.

## Why not local with a real DS

DS local wireless is Nintendo's own 802.11 MP protocol (beacons with a GGID, then a parent poll and child reply each
frame, with tight timing). On a 3DS, only DS mode (TWL_FIRM, a real cart or nds-bootstrap) drives the radio that way.
A native 3DS app gets the `nwm::UDS` service, which is the 3DS's own local protocol: 3DS to 3DS only, with
encrypted frames. No 3DS service exposes raw 802.11 frames or DS MP. Talking to a DS would need custom Wi-Fi
firmware or a PC with an injection-capable Wi-Fi card acting as a bridge. That is research-project territory, not a
feature. **Plan around it: legit-game trades go online.**

## Option A: local 3DS to 3DS over UDS (recommended first)

Write `wm_uds.c`, a drop-in replacement for `wm_absent.c` with the same API and the same pump. When the toggle is off,
`wm_absent.c` behaviour stays exactly as it is today.

Mapping (libctru `3ds/services/uds.h`):

| DS WM | 3DS UDS |
|---|---|
| `WM_Initialize` / `WM_End` | `udsInit(0x3000, name)` / `udsExit()` |
| `WM_SetParentParameter`, `WM_SetGameInfo`, `WM_StartParent` | `udsGenerateDefaultNetworkStruct` (one fixed wlancommID shared by both ports, max nodes 8) + `udsSetApplicationData` (GGID and game info, up to 200 bytes) + `udsCreateNetwork` |
| `WM_StartScan` result (`WMBssDesc`, `gameInfo`) | `udsScanBeacons` then `udsGetNetworkStructApplicationData`, rebuilt as a DS `WMBssDesc` so the game's GGID filter and room lists work |
| `WM_StartConnectEx` (aid) | `udsConnectNetwork`, our node ID mapped to a WM aid |
| `WM_StartMP` + `WM_SetMPDataToPortEx` | parent: one UDS broadcast per MP cycle (once per game frame); child: one unicast reply |
| port receive callbacks / `WM_ReadMPData` | `udsPullPacket` in `PSPNativeWMPump`, delivered as `WM_STATECODE_PORT_RECV` / MP-end with a real `WMMpRecvBuf` |
| `WM_Disconnect`, child leaving | `udsEjectClient`, `udsDisconnectNetwork`; status events as `WM_STATECODE_DISCONNECTED` |
| `WM_GetLinkLevel` | `udsGetChannel` / link level from `udsGetConnectionStatus` |

Sizes fit easily. A UDS frame holds up to 1,478 bytes (`UDS_DATAFRAME_MAXSIZE`) against the game's 192, and UDS allows
16 nodes against the game's 8.

What makes it work across both ports: Platinum and HGSS already trade, battle and use the Union Room with each other
on real DSes (the same Gen 4 protocol, different GGIDs that each game accepts). If both ports use the same UDS
wlancommID and copy the beacon game info byte for byte, **Platinum port to SoulSilver port should work as it does on
DS**.

Risks, honestly:
1. **Timing.** DS MP is hardware-paced at about 60 Hz. The ports run around 30 fps, and UDS latency is a few ms but
   not guaranteed. CommSys tolerates jitter, but some steps (trade confirmation, battle turn sync, Underground
   position sync) have timeouts that end in the "communication error" reset. The prototype must measure this first.
   Pacing MP cycles by wall clock (not by game frame) is the fallback.
2. **Exact state machine.** The game reacts to many WM state codes (`BEACON_LOST`, `CONNECTED`, `DISCONNECTED`, MP
   start and end). They must arrive in the order the SDK produces. pspoke's notes record that failing WM calls sends
   the game to its comm-error reset, so every path needs testing: cancel, out of range, power button, HOME, sleep
   (UDS drops on sleep).
3. **Save writes.** A trade writes the save by design. It uses the game's own code, so the format stays the same.
   The rolling save backups must run before the first link session. Test on save copies only (two consoles, two
   copied saves).
4. **Testing needs two New 3DSes.** Azahar has no working
   multiplayer UDS for this, so testing is mostly on hardware. Good logging is essential.

## Option B: online through a revival server (for legit-game trades)

Nintendo WFC closed in May 2014. Community servers run its protocol again:
- **Wiimmfi**: the main one for DS games. Gen 4 GTS, Wi-Fi battles and Wi-Fi Club work. HGSS connects as is. **Diamond,
  Pearl and Platinum need a certificate patch** (Wiimmfi ships it as a cheat code) because their SSL rejects
  Wiimmfi's certificate. Real DSes connect by setting their DNS to Wiimmfi. A DS needs an open or WEP network
  (or a DSi/3DS in DS mode with the same limit).
- **WiiLink WFC**: open source (Go). It added early DS support. Gen 4 Pokemon support is not confirmed.
- **gts-rs** (Codeberg, Bolu) and older "IR-GTS"-style scripts: small self-hosted GTS servers. Good for testing the
  port's GTS on Nathan's own PC without touching a public server.

What the port needs:
1. **WCM shim** (NitroWiFi access-point manager): "connected" comes from the 3DS's own Wi-Fi via
   `acWaitInternetConnection` / `ACU_GetWifiStatus`. The 3DS's saved connection is used, so WPA2 works for the port,
   unlike a real DS.
2. **CPS/SOC backend for the 3DS**: add a `SDK_BUILD_CTR` branch next to the Linux/Switch one in
   `libntrwifi/libraries/cps/src/network.c` (`socInit` with a 0x100000-byte aligned buffer, non-blocking sockets,
   `poll`). That is a pspoke-patch on the fetched `libntrwifi`.
3. **Server redirect**: the game asks for `nas.nintendowifi.net`, `gamestats2.gs.nintendowifi.net`, `*.gamespy.com`
   and others. A small resolver override sends those names to the chosen server (Wiimmfi's DNS, or Nathan's PC for
   tests). Config: `wifi_server=` and `wifi_dns=`.
4. **SSL**: the game's own SSL code (decompiled in `libntrwifi/libraries/ssl`) keeps running over plain sockets.
   Platinum needs the same certificate check change that Wiimmfi's DPPt cheat makes, behind a toggle. SoulSilver
   needs none.
5. **Stay on the game's code path**: the GTS, Wi-Fi Club and Plaza screens are the game's own, so the server sees
   normal game traffic.

Risks and limits:
- Much more code than Option A, and GameSpy peer-to-peer (Wi-Fi Club trades and battles use NAT negotiation and UDP
  between the two players) is the hardest part. GTS is plain HTTP request and response, so **GTS is the first
  online target**.
- **The port can't use local UDS and the internet at once.** UDS takes over the radio. That is fine because the game
  never needs both.
- **Wiimmfi's rules.** It is a free community service that checks games and bans cheated Pokemon. Connecting a
  non-cart client needs the same care: ask Wiimmfi before using the public server, develop against a self-hosted
  server first, and keep `cheat_*` features from ever reaching online play. Gate it: online is refused while any
  cheat key is on.
- **Wi-Fi Plaza** (`ppwlobby`) server support on revival servers is not confirmed.

## Legal and safety notes

- Everything runs from Nathan's own ROM dump, and nothing ROM-derived is committed, as before. The certificates and
  keys inside the game come from that ROM at build time and are never put in git.
- The community servers are run by volunteers. Respect their terms, and don't hammer them while debugging.
- **Save rule:** wireless only writes the save through the game's own trade, battle-record and Underground code
  (that is its explicit purpose). The format stays byte-compatible with a cart. Back up the save before every test
  and only use copies, as `AGENTS.md` requires.
- **Toggle rule:** one master toggle per mode (below). Off means today's `wm_absent.c` behaviour, bit for bit.

## Recommended first milestone: M1, local trade between two 3DSes

"Two New 3DSes, each running the Platinum port with a copied save, meet in the Pokemon Center's Union Room (or the
2F Wireless Trade desk) and complete one trade. Both saves still load on a real cart or DeSmuME afterwards."

Why this one: it is the cheapest end-to-end proof. It reuses the existing `wm_absent.c` seam, it needs no server,
and it unlocks the Underground, local battles and Platinum-to-SoulSilver trades with the same code.

## Work breakdown (rough)

| Step | What | Where | Size |
|---|---|---|---|
| M0 | UDS probe: **written**, `tools/uds_probe/` (a standalone .3dsx, no game code). It hosts or joins a UDS network and logs round-trip time, loss and jitter for 192/38-byte packets at 60 or 30 Hz over 60 s to `sdmc:/3ds/uds_probe.txt` (`[UDS]` lines). Needs two consoles. Nathan has a New 3DS XL and a New 3DS | `tools/uds_probe/` | small, done |
| M1a | `wm_uds.c`: the full WM API on UDS (table above), selected at run time by `wireless=1` (System tab, default OFF). Off falls through to the current `wm_absent.c` code (both in one file, or two files behind a dispatch). Log tag `[WIRELESS]`: every state change and connect/disconnect, MP cycle stats every 10 s | pspoke-patch (services/) + Makefile | medium |
| M1b | Make the beacon game info and scan results byte-exact, so the Union Room lists the other 3DS. Map aids and node IDs | same | small |
| M1c | Hardware test: Union Room trade, then the Wireless Trade desk, then a local battle, on two copied saves. Then savecheck.py and a cart/DeSmuME load of both saves | Nathan + logs | test |
| M2 | Underground with 2 to 5 players (the most timing-sensitive; may need wall-clock MP pacing) | wm_uds.c | medium |
| M3 | SoulSilver: the same `wm_uds.c` (shared pspoke service), then a Platinum port to SoulSilver port trade | soulsilver repo | small once M1 works |
| M4 | Online base: 3DS CPS/SOC backend + WCM shim + resolver override, toggle `wifi_online=0`. Reach the "connected to Nintendo WFC" screen against a self-hosted server | libntrwifi pspoke-patch + platform | large |
| M5 | GTS deposit and search on a self-hosted GTS (gts-rs), then on Wiimmfi once its maintainers are OK with it. HGSS first (no cert patch), then Platinum with the cert toggle | | medium |
| M6 | Wi-Fi Club trade/battle with a real DS over Wiimmfi (GameSpy P2P) | | large, uncertain |
| Later | Pokewalker over `ir:USER` (HGSS), Wi-Fi Plaza if a server supports it | | research |

Order: M0, then M1, then M3 (cheap, and it gives Platinum to SoulSilver trades), then M2, then M4 to M6.

## Sources

- pspoke: `NOTES.md` item 15 and `port/native-audio-app/services/wm_absent.c` (commit 7477648)
- pret/pokeplatinum `src/wireless_manager.c` (MP sizes and GGID handling)
- cybervisi0n/libntrwifi (`libraries/cps/src/network.c` Linux/Switch socket backend), cybervisi0n/libntrdwc
- GabeSud0 design fork, `unowncollectables/pokeplatinum-3ds` `3ds_port/game/Makefile` (networking stubbed)
- Wiimmfi Gen 4 status (2026 guide): https://tech-insider.org/how-to-set-up-wiimmfi-pokemon-ds-2026/
- WiiLink WFC: https://wiilink.wiki.gg/wiki/WiiLink_WFC
- GTS protocol: https://projectpokemon.org/wiki/GTS_protocol, gts-rs: https://codeberg.org/Bolu/gts-rs

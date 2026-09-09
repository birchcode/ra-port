# Multiplayer handoff — 2026-09-09

## Audit update — 2026-09-09

Read `docs/MULTIPLAYER_CONTRACT.md` for the completed contract trace, local evidence and remaining acceptance checklist. Fixed the LP64 reliable-sequence startup failure and UDP truncation handling; both peers must rebuild. Added `tests/connection_test.cpp` and expanded the socket regression. Queue checks, authorized loopback test, script suite and macOS game build pass. No two-machine match has been verified. Parser validation, explicit build/data compatibility and real-network checks still gate internet transport. Current audit base is `ea5aa8f` plus dirty multiplayer/CRT edits; preserve concurrent work.

## Mac Pro installation — 2026-09-09

Installed at `rmp@rmp-macpro5-1.local` (`192.168.1.130`), `/home/rmp/ra-port-network-audit-20260909`. Open **Red Alert — LAN Test** from the applications menu/desktop. Older installations are preserved; assets link to `/home/rmp/ra-port-lan/assets`. Both network regressions pass on Linux, startup stays alive for 12 seconds under Xvfb, and all eight MIX archives match the Mac. No match has yet been played. Installation hashes/evidence: `build/network-audit/macpro-install.json`; remote logs are in the installation directory. Linux exposed test header-order and unused-vtable linking issues, corrected in `CMakeLists.txt` and `tests/connection_test.cpp`.

## Linux freeze diagnosis — 2026-09-09

Clicking Multiplayer froze before networking: debugger thread 1 showed `Select_MPlayer_Game → Draw_Line → Unlock → MacSDL_Present8 → MacMM_PumpTimers → Process_Mouse → Block_Mouse → EnterCriticalSection`, blocked in a futex. `InitializeCriticalSection` incorrectly inspected an uninitialized flag in freshly allocated mouse storage, sometimes skipping creation of the recursive mutex. Fixed in `PORT/MAC/include/windows.h`; `tests/timer_shim_test.cpp` now starts with poisoned storage and verifies recursive acquisition. The regression failed before the fix and passes locally afterward. Remote trace: `/home/rmp/ra-port-network-audit-20260909/freeze-debug.log`. Runtime retry and rebuilt Linux results are pending below until verified. Preserve newer concurrent AI/movie/CRT edits; only the lock fix is being transferred into the installed snapshot.

## Goal and current state

Make Red Alert easy to play privately with a friend over modern internet, presented as nostalgic dial-up: **Multiplayer → Dial a friend → Wait for call / Dial → host accepts → existing lobby**. Temporary invitation link/key; short skippable modem sounds. No artificial gameplay lag. Accounts, public matchmaking, saved contacts and reconnect/resume are outside the first milestone.

LAN host/join is implemented and merged. Internet signaling, invite service, NAT traversal and relay transport are **not implemented**. Existing transport smoke tests are not proof of a complete synchronized match. A two-machine 30-minute match, results/rematch, disconnect handling and mixed macOS/Linux validation remain open.

Run identical builds and game data on both computers. Choose **Multiplayer Game → Network**; host selects **New**, guest selects the advertised game and **Join**. Same subnet; UDP port **34835**. See README.md for build/run instructions.

## Read first / implementation map

- `PORT/MAC/src/lan_udp.cpp`: native UDP backend, `PacketTransport`, socket lifecycle, send/receive and subnet broadcast. Address mapping stores IPv4 and port in the legacy node address. Several base methods are stubs; inspect concrete overrides and callers before assigning transport semantics.
- `CODE/WSPROTO.H`, `CODE/WSPUDP.H`: transport interfaces.
- `CODE/IPXMGR.CPP`, `CODE/QUEUE.CPP`: inspect reliability, sequencing, synchronization, timeouts and command serialization before choosing data-channel delivery semantics.
- `CODE/NETDLG.CPP`: network lobby/game creation. `CODE/MPLAYER.CPP`, `CODE/NULLDLG.CPP`: multiplayer entry and legacy modem/serial flows.
- `tests/lan_udp_test.cpp`: existing socket smoke test; target `lan_udp_test` in CMakeLists.txt. Build with `cmake --build build --target lan_udp_test`, run `build/lan_udp_test`. Use existing script suite for shared-code regressions; its default cleanup permanently deletes temporary files, so retain artifacts or substitute Trash cleanup under this workspace's deletion rule.
- `docs/PROJECT_PLAN.md`, section “Dial-up over modern internet”: agreed experience, architecture proposal and acceptance gates.

## Next bounded deliverable: validate a complete LAN match

The contract audit is recorded in `docs/MULTIPLAYER_CONTRACT.md`. Use two real machines and its acceptance checklist; record exact commits, dirty changes, binary/data hashes, platforms and outcomes. Verify ready/start, 30-minute sustained play, results/rematch, host/guest disconnect and incompatible builds/maps. Fix demonstrated defects with narrow tests. Complete parser validation and explicit compatibility checks before implementing the internet adapter. Available local checks passed; no full match or mixed-platform result is claimed.

## Following deliverable: two-peer internet transport spike

Current proposal, not a selected dependency: a maintained WebRTC data-channel library such as libdatachannel behind a small C boundary; the game remains C++98. Verify current API/build/license requirements before adoption. Reuse the packet interface and game synchronization. Do not emulate physical serial wiring or rewrite the whole multiplayer stack.

Use a small HTTPS/WebSocket rendezvous service for invitation lookup and connection metadata, ICE/STUN for direct routes, and TURN fallback. An invitation key does not eliminate signaling or relay requirements. Preserve message boundaries and stable peer identities; choose reliability/ordering only after the contract audit. GameNetworkingSockets is an alternative; standalone use does not automatically supply Steam's services.

Invites must be random, expiring and revocable, with host acceptance and connection binding. Short human-entered codes need attempt limits. Use authenticated encrypted transport, short-lived relay credentials, bounded packet/queue sizes and explicit protocol/build/map-data compatibility. Do not expose the existing unauthenticated LAN socket directly to the internet.

Acceptance: connect across separate home networks without port forwarding; test direct and forced relay, cancel/expire/reject, representative latency/loss, full match without desync and clean return to menus after failure. Measure relay traffic before estimating operating cost. Add modem presentation only after this works. Do not deploy services or send invitations to others without authorization.

## Shared-workspace cautions and presentation

HEAD at handoff: `9c7d527`. Concurrent uncommitted AI and CRT work exists in HOUSE.CPP, TECHNO.CPP, ra_crt_gl.cpp, shared plan/tests and new AI/CRT test files. Inspect fresh status; preserve those edits. Identical commits alone do not imply identical binaries if built from dirty checkouts; game-logic differences matter for deterministic LAN play.

Widescreen title now remains behind compact dialogs. Full-screen network setup still has its legacy layout; the expanded creation screen at `build/art-study/v2/index.html` is only a study. Keep small dialogs compact. Responsive network creation is separate from transport correctness. Original-art and physical-CRT overrides remain `RA_TITLE_ART=original` and `RA_CRT=0`.

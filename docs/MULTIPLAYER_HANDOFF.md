# Multiplayer handoff — 2026-09-10

## Verified state

Mac ↔ Linux LAN discovery, join and match startup work. The user confirmed the retry worked after fixing a frame-33 desync. **A 30-minute synchronized match, results/rematch and disconnect recovery are not yet verified. Internet transport is not implemented.**

The desync came with divergent AI destinations and simulation RNG state. A real-engine regression reproduced different results under Clang/GCC from the same seed: argument evaluation reversed direction/distance random draws. `CODE/HOUSE.CPP::Random_Cell_In_Zone` now sequences direction then distance; `CODE/SCENARIO.CPP` sequences fallback spawn X/Y. The fixed-seed fixture in `tests/ai_campaign_test.cpp` failed on Linux before the fix and passes on both platforms afterward. Reports, diff, screenshots and then-installed hashes: `build/network-audit/desync-20260910/`.

Both launchers enable `RA_FULLSCREEN=1 RA_WIDESCREEN=1 RA_CRT=1 RA_TITLE_ART=wide`. Linux CRT had been omitted from its launcher; enabling it restored the 856×400 logical canvas at 2560×1440 and readable copyright. The non-CRT 712-wide path had dropped columns while shrinking the 854-wide artwork. Verified screenshot: `build/network-audit/desync-20260910/linux-fixed-title.png`. Other aspect ratios/non-CRT title resampling are not generally fixed.

## Combat determinism completed — acceptance pair

All 11 unsequenced combat expressions in `CODE/BUILDING.CPP` and `CODE/ANIM.CPP` now use explicit draw order: scatter → delay → loops; crater type → scatter. `tests/combat_rng_test.cpp`, run through `python3 tests/run_ai_campaign_test.py build combat` (Linux: `build-linux`), exercises real napalm, debris, major damage and destruction over 64 seeds each. It checks animation type/position/delay/loops, ground scars and RNG state, and asserts fire/smoke/crater coverage. Fixed digests agree under Mac Clang and Linux GCC: `db093242 / 36e5458a / ca772854 / dc1ce682`. The old expressions agree with the fixture on Clang but fail **all four paths** on GCC. Production checksum/desync checks are unchanged.

Evidence and exact build/source/data identities: `build/combat-regression/` (`source-manifest.json`, `mac-identity.json`, `linux-identity.json`, `installed-builds.json`, platform negative-control logs). All eight MIX archives and presentation files match. AI, menu, camera, timer, connection and UDP regressions pass on both platforms; the full Mac script suite passes with temporary artifacts retained. Mac UDP required sockets outside the sandbox. Linux source/object timestamps required forcing recompilation after synchronization. Camera fixes and the concurrent September 10 icon/runtime update are preserved in the prepared pair.

## Installations and fresh-build caution

- Mac: `build/RA Widescreen Playtest.app`; packaging entry point `scripts/package_mac_playtest.py` (inspect current options before use).
- Linux: `rmp@192.168.1.130` / `rmp-macpro5-1.local`, `/home/rmp/ra-port-current-20260910-073645`; desktop/app-menu shortcut **Red Alert — LAN Test**. Assets link to `/home/rmp/ra-port-lan/assets`. All eight MIX archives matched at installation. Older installations are retained.
- LAN: **Multiplayer Game → Network → New / Join**, same subnet, UDP 34835. Do not expose this unauthenticated socket to the internet.
- HEAD is `af214ed` with substantial concurrent uncommitted AI, camera, timer, UI and packaging work. Work directly on `main`; preserve all changes. **Use the combat-regression identity ledger, not the older menu-install ledger. Concurrent packaging can change installed binaries; recheck hashes before acceptance and never reuse old PIDs.** Check current source snapshots, binary/data hashes and running processes before testing. Mac ad-hoc signing changes the packaged binary hash.

## Subsequent work to preserve

The in-match menu cursor fix in `CODE/CONQUER.CPP::Sync_Delay` pumps SDL input during dialog waits with a 1 ms yield. `PORT/MAC/src/mac_timer.cpp` uses 32-bit `LONG` deadline comparisons; native 64-bit `long` fired timers early. The engine menu regression improved from zero to 12 cursor updates per 200 ms on both platforms, with no frame advance and Escape still queued. Both game builds, campaign AI regressions, Mac script suite and Linux timer/network regressions passed. Evidence: `build/menu-regression/`, especially `installed-builds.json` and `linux-validation.log`; Linux backup `menu-timing-backup-68m4wg4p`. Live Options/Game Controls pointer acceptance remains open.

Newer camera/zoom and overlap-list crash work is recorded in `docs/PROJECT_PLAN.md` and `build/camera-regression/test.log`; inspect affected current files without reverting it. Its installation on Linux has not been established here.

## Next bounded deliverable: full LAN acceptance

The prepared builds reached their title screens with fullscreen/widescreen/CRT enabled (856×400 logical canvas at 2560×1440). Linux title screenshot: `build/combat-regression/linux-installed.png`; Mac title was visually inspected through the packaged app. Automated combat regression is **not** full LAN acceptance.

1. Recheck installed binary hashes against `build/combat-regression/installed-builds.json`, then use **Multiplayer Game → Network → New / Join** on the two machines. Existing discovery/join/startup acceptance predates this new pair and must be repeated.
2. Play at least 30 minutes with AI, construction, harvesting, combat, fire/napalm and building destruction. Record synchronized completion, not just process survival. Test zoom/panning and Options/Game Controls cursor movement.
3. Verify results, rematch, host/guest exit, and network loss during lobby/loading/gameplay. These acceptance outcomes remain **open**.
4. On desync, preserve **both resource roots’ `OUT.TXT` immediately**, before another failure overwrites them. Mac resource root: `build/RA Widescreen Playtest.app/Contents/MacOS`; Linux root: `/home/rmp/ra-port-current-20260910-073645`. Compare frame, RNG state and object destinations. Previous failing report stopped at frame 33 with zero player commands.

## Gate before internet work

`docs/MULTIPLAYER_CONTRACT.md` contains the packet/lockstep trace, native-structure ABI assumptions, parser length/type gaps and missing explicit build/protocol/data compatibility. Close those gaps and LAN acceptance before adding internet transport. Key files: `PORT/MAC/src/lan_udp.cpp`, `CODE/WSPROTO.H`, `CODE/CONNECT.CPP`, `CODE/IPXMGR.CPP`, `CODE/QUEUE.CPP`, `CODE/NETDLG.CPP`. Prior reliable-sequence LP64, truncated-UDP and Linux recursive-mutex fixes have passing regressions; details remain in the contract and older evidence directories.

Then implement the agreed private **Dial a friend → Wait for call / Dial → host accepts → existing lobby** experience. A WebRTC data-channel adapter behind a C boundary is a candidate, not a chosen dependency; retain C++98 and existing lockstep. Require encrypted authenticated transport, stable peer binding, bounded packets/queues, random expiring/revocable invitations, rate limits for short codes, signaling and direct/relay routing with short-lived relay credentials. Verify separate home networks without port forwarding, forced relay, loss/latency, cancellation/expiry/rejection and clean failure recovery. Measure relay traffic before cost estimates. Add skippable modem presentation afterward. Accounts, public matchmaking and reconnect/resume are outside the first milestone. No service deployment or invitations without authorization.

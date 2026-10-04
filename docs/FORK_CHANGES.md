# Changes in the birchcode fork

This fork builds on [dk8827/ra-port](https://github.com/dk8827/ra-port). The source snapshot [`c33a31b`](https://github.com/birchcode/ra-port/commit/c33a31b3a3c72145af9c4aafa8c3858a828e0269), saved on 4 October 2026, contains 47 commits beyond the shared upstream checkpoint `b270206`, changing 104 files. The native macOS/Linux runtimes and Android/iOS debug targets already existed upstream. This summary describes the additional desktop gameplay, networking, presentation, and tooling work.

## LAN multiplayer and simulation determinism

Added UDP LAN discovery and the existing game lobby's host/join path, using port `34835`. macOS/Linux discovery, joining, and match startup were exercised on real machines. Reliable-sequence handling on 64-bit systems, truncated UDP handling, and Linux recursive-mutex behavior received fixes and regressions.

Cross-platform desync investigations found that expressions containing several random draws could evaluate differently under Clang and GCC. AI destination/spawn generation and eleven combat expressions now use explicit draw order. The combat fixture exercises real napalm, debris, building damage/destruction, scars, and final RNG state across fixed seeds; the September acceptance pair produced matching Mac/Linux digests. Existing simulation checksums remain in place.

The movement changes bump `GAME_VERSION` to `0x00030004`; use matching source builds and original game data on all peers. A synchronized 30-minute match, results/rematch, and disconnect recovery remain unverified. Internet transport is not implemented. The unauthenticated LAN socket is not an internet-facing multiplayer service. Details: [multiplayer handoff](MULTIPLAYER_HANDOFF.md) and [protocol contract](MULTIPLAYER_CONTRACT.md).

## Fullscreen, widescreen, and camera controls

Added desktop fullscreen toggles, a wider tactical viewport with corresponding input coordinates, and presentation pacing work. Network browsing, network/skirmish creation, and serial/modem setup expand their original background and lists to the logical viewport; compact dialogs remain centered and restore the previous viewport afterward.

Middle-button dragging pans the battlefield. Vertical wheel and trackpad input zoom around the pointer from the original 1× view down to ½×, while wheel input over the sidebar scrolls its lists. Gestures stop on dialog entry, focus loss, Escape, or mode changes. Rendering, picking, selection, map bounds, and dirty redraw handling were updated together. A complete overlap-list snapshot fixes the tooltip cleanup crash exposed by zooming out.

Optional widescreen title artwork uses the original palette, with the original plaque/copyright pixels restored. `RA_TITLE_ART=original` keeps the original title; classic mode or an invalid/missing optional image also falls back. See [title artwork provenance](../presentation/README.md). Other aspect ratios and title resampling still need broader playtests.

## CRT rendering

Added an opt-in desktop OpenGL 2.1 CRT path with beam reconstruction, scanlines, and an RGB phosphor mask. Controls include mask strength, a clean/CRT split view, diagnostic patterns/capture, a legacy shader, and optional highlight glow. The default phosphor pitch is an output-pixel pattern, not physical monitor-density calibration. Mobile retains its SDL renderer.

Shader regressions and captured gameplay informed readability/brightness tuning; full 4K gameplay, panel scaling, motion, and physical viewing remain checks. For a physical CRT, the simulated treatment can be disabled independently of widescreen gameplay. See [CRT review](CRT_REVIEW.md).

## Enemy AI and campaign economy

Added a heuristic ground/base controller for defensive posts, scouting, home reserves, economic-target selection, rallying/group attacks with available air support, incursion response, retreat, and vehicle repair. Campaign military-base activation, unit production choices, construction budgeting, power priorities, placement preferences, and ordinary-building rebuild queues were revised. Human orders and scripted-team behavior remain outside the new controller.

Easy difficulty now makes fewer decisions and sends smaller attacks after an opening grace period, with limits on fresh reinforcements and autonomous army production. Existing units and scripted teams are not removed or capped. Engine fixtures cover the controller's decisions and economy, and the user playtested the stronger normal-difficulty AI. Naval strategy, terrain-aware flanking/chokepoint analysis, factory exits/building placement, and wider campaign balance remain incomplete or unverified. Development details: [project plan](PROJECT_PLAN.md).

## Movement, orders, and transport reliability

Human move orders and harvesters compare a traffic route with a clear detour, retain destinations during bounded waits, and avoid automatically attacking destroyable obstacles during ordinary movement. Route selection considers accumulated cost; same-owner harvesters and existing formation groups have local yielding rules. Fresh/repeated orders clear stale retries and cached routes without discarding queued waypoints.

Damage can alert nearby eligible idle infantry/vehicles while preserving existing orders. Helicopters update firing positions as targets move, and transport/passenger radio handling retains intended boarding through staging interruptions. These are local reliability improvements, not a global path planner or a guarantee that every blocked road is traversable. Nearby defense excludes ships and aircraft. The path fixture covers twenty synthetic-terrain scenario groups using actual engine methods; live bridge, mixed-traffic, and boarding checks remain open. See [movement handoff](PATHFINDING_HANDOFF.md).

## Desktop packaging, input, and parsing

In-game menu waits now pump SDL input without advancing the simulation, and timer deadline comparisons use the engine's 32-bit type on 64-bit hosts. Desktop icons and a Linux application-menu template were added. `scripts/package_mac_playtest.py` creates `build/RA Widescreen Playtest.app`, copies optional presentation files, and links to the checkout's ignored game assets. Its launcher sets explicit fullscreen/widescreen/CRT/title defaults while respecting environment overrides. It is a local playtest package rather than a portable release.

CPS/LCW and VQA decoding gained bounds/format checks and regression cases. Original game data stays excluded from Git; cloning the repository restores source, tooling, and optional presentation files, but you must prepare your own original assets and rebuild executables. No mobile store release or standalone installer is delivered by this checkpoint.

## Verification at the saved checkpoint

- [Linux CI for `c33a31b` passed](https://github.com/birchcode/ra-port/actions/runs/37217102852), including its desktop/UDP build and script-suite steps.
- [macOS CI for `c33a31b` reached the test step and failed](https://github.com/birchcode/ra-port/actions/runs/37217102868) at `tests/input_shim_test.cpp:43`, where `camera_input_test` asserts that its expected SDL window exists. The local 4 October 2026 suite hit the same assertion. The current full macOS suite is not passing.
- September handoffs record earlier successful engine, camera, combat, networking, and shader checks. Their evidence describes those tested snapshots; ignored build logs, binaries, and captures are not included in a fresh clone.
- Full LAN acceptance, live pathing and Easy-difficulty balance, longer-session save/load behavior, fresh Android/iOS builds, and release packaging/device validation remain open.

Run commands are in the [README tests section](../README.md#tests). This documentation update does not change runtime behavior or fix the macOS test failure.

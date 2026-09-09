# Project state and next playable milestones

Updated 2026-09-09. Baseline after worktree consolidation and desktop camera implementation.

## Handoff

- Enemy AI, 2026-09-09: user explicitly broadened the request from reactive fixes to active tactical/economic play. `CODE/AI_TACTICS.CPP` replaces the computer expert-system tick with a 2-second controller: distribute defensive posts around buildings/harvesters, scout with one unit when no target is known, choose exposed economic targets, rally surplus troops, retain a home reserve, attack as a group with available air support, respond to incursions and retreat/seek vehicle repair. Existing mission/target/timer fields retain orders across saves; human and scripted team orders are excluded.
- AI economy: hostile campaign military bases now activate automatically (Allied mission 1 previously had IQ 0, 2500 credits and no production trigger). Unit mix scores weapon effectiveness, enemy composition, cost and existing quantities; construction cash is reserved only when a yard exists. Autonomous choices obey faction/tech/prerequisites; scripted queues retain their bypass. Power shortages take priority; defense placement favours the front, economy the core/rear. Ordinary lost buildings join the campaign rebuild queue; trigger-bearing objectives keep their original rebuild policy. Mobile rescue also fixes blocked-footprint reachability and Sticky guards.
- AI verification: `python3 tests/run_ai_campaign_test.py` links the actual desktop engine and checks activation/production/rebuild start, separate stable defensive posts, rally-to-assault transition with reserves, retreat, incursion targeting, counter-production, construction budgeting and single-scout behaviour. Evidence: `build/ai-regression/test.log`; no proprietary assets required. It simulates arrival at ordered posts; it does not run a rendered battle or validate factory exit/building placement. Full source/script suite evidence: `/tmp/ra-tactics-suite.log`. Test artifacts are retained.
- AI difficulty follow-up (2026-09-09): user found Easy too close to Normal. Controller now respects the inverted computer handicap (DIFF_HARD = player Easy): Easy thinks every 5s, holds half the available garrison home, caps attacks at 4 with a 120s cooldown and 3-minute opening grace, omits coordinated air support, limits fresh defense reinforcements to 4, and caps autonomous army production at max(8, building count). Normal retains its 2s tick, one-third reserve and 30s attack cooldown; Hard uses a 1s tick, one-quarter reserve and 15s cooldown. Existing scripted teams and combat/stat handicaps remain intact. Real-engine regression verifies identical 12-unit forces launch 4/8/9 units, their timers and Easy production/defense limits; evidence `build/ai-regression/test.log`. Live difficulty balance needs a fresh Easy playtest.
- AI playtest: user confirmed on 2026-09-09 that normal difficulty was a real challenge after this upgrade. Next validation: further campaign/skirmish balance, pathing and economy checks. This is a heuristic ground/base controller, not human-level strategy: naval planning and terrain-aware flank/chokepoint analysis remain legacy/unimplemented; knowledge uses the engine computer-discovery flag and existing house counters. Mobile builds and cross-platform LAN determinism have not been exercised. All LAN peers need matching builds.
- All six worktree histories and pending GPU CRT work were consolidated into main (`874c847`). Worktree directories and asset symlinks remain intact; nothing was pushed.
- Desktop camera controls now support middle-button grab drag, wheel/Shift+wheel, fractional two-axis trackpad input, sidebar wheel scrolling, and cancellation on Escape, focus loss, mode changes and dialog entry. Selection and right-button actions suppress panning.
- `RA_EDGE_SCROLL=0` disables edge scrolling; `RA_PAN_SPEED=0.1..10` and `RA_PAN_REVERSE=1` tune wheel input. In-game settings and visible controls hints remain milestone 2.
- Camera deltas use viewport scaling and the pending tactical position, clamp before coordinate packing, and request a complete redraw because legacy dirty strips leave artifacts on large jumps. Long-session performance still needs evaluation.
- User requested a finer CRT pattern: RGB mask repeat is halved from 6×8 to 3×4 output pixels; original raster scanlines, brightness settings and physical-subpixel calibration remain.
- Changed paths: `PORT/MAC/src/mac_sdl_runtime.cpp`, its header, `CODE/{GSCREEN,GADGET,SCROLL}.CPP`, `tests/input_shim_test.cpp`, and `PORT/MAC/src/ra_crt_gl.cpp`; controls documented in README.
- Verified: macOS build; full source/script suite; SDL event tests for drag, wheel, fractional axes, scaling, sidebar/menu routing, selection and cancellation. Logs: `/tmp/ra-camera-build.log`, `/tmp/ra-camera-tests.log`, `/tmp/ra-camera-input-build.log`. Test artifacts retained instead of permanently deleted.
- User confirmed the camera controls and finer CRT effect all worked after playtesting on 2026-09-09. Automated checks and macOS build also passed. Other platforms and extended performance checks remain open.
- A temporary `/tmp/RA Camera Playtest.app` wraps the current binary for UI tooling, links only local assets, and launches with CRT/fullscreen/widescreen. It is not a distributable package. The user was interacting with the running game at the end of the check.
- Next bounded deliverable: explore responsive interface composition from original art slices (below), then build display/control settings using that visual vocabulary. Validate full LAN matches before internet transport work.

## Where the project stands

The existing port supports campaigns, skirmish, sound and movies, native macOS/Linux desktop targets, and Android/iOS debug targets. Those gameplay claims come from the existing project documentation; this consolidation verified builds/tests and startup, not every mission.

Merged work adds LAN lobby/UDP transport, VQA compatibility improvements, presentation pacing, widescreen battlefield geometry, centered legacy screens, and optional CRT treatment. Fullscreen now has desktop wheel/trackpad panning and middle-button camera dragging; the controls still need in-game discovery and settings.

This is a playable development build with several features still exposed through environment variables. The concept images are design assets, not an implemented replacement title screen. LAN is implemented, but the transport smoke test is not evidence of a complete synchronized match.

CRT beam pass (2026-09-09): implemented 3×5 beam reconstruction with brightness-dependent width, independent fine-mask contrast, sRGB transfer and SDR highlight headroom. `RA_CRT_MODEL=legacy` keeps the previous look for comparison. Build/input/mobile checks and active GPU flat-field checks passed at 1080p/1440p/2160p/fractional sizes; bright line measured 33% wider than dim. Diagnostic captures/timings: `build/crt-review/gpu-check.log`; implementation, runnable GPU check and limits: [CRT_REVIEW.md](CRT_REVIEW.md). Fixed output-pixel mask pitch retained with small-scale contrast attenuation; physical density calibration deferred. Opt-in glow experiment added with `RA_CRT_GLOW=25` (default 0): reuses the same taps, adds a local 2D highlight halo, and passes locality/dark-field/flat-field checks. Full-width 4K Present+completion medians ~7.3–7.8 ms on M4 with/without glow; all 30-sample runs below 16.67 ms, not gameplay pacing proof. Corrected timing to drain preceding GPU work. Evidence: `build/crt-review/glow-{check,art-check,flat-check}.log` and art off/on PNGs. Next: normal-distance gameplay/glow and panning/presentation-cadence checks. Screen stays flat per user; no trails, input mapping, game-speed or vsync changes.

Widescreen title integrated (2026-09-09): `presentation/title-wide-854x400.idx` uses the original palette and exact original Westwood/copyright crops. Follow-up fixed the original-to-wide flash and New Game fallback: every `Load_Title_Page` now loads a centered legacy canvas with a wide presentation background. Main menu and compact dialogs keep original control coordinates; pointer mapping follows that centered canvas. Full-screen dialogs, movies, non-title pictures and gameplay clear the wings. `RA_TITLE_ART=original` and `RA_CRT=0` remain independent. Explicit launcher packaged with `scripts/package_mac_playtest.py`; local app `build/RA Widescreen Playtest.app`. SDL readback tests cover wings, center boundaries, repeated menu entry, clearing and gameplay transition; full script suite passed. Physical CRT and end-to-end campaign flow remain playtest checks. Other aspect ratios fit the 16:9 composition.

## Ordered plan

| Order | Deliverable | Why it helps | Acceptance |
| --- | --- | --- | --- |
| 1 | Camera controls complete; user playtest passed | Immediate benefit in every fullscreen match | Middle drag, wheel and trackpad pan work in windowed/fullscreen, clean/CRT/widescreen; no accidental orders or selection; correct bounds and focus handling |
| 2 | Small in-game display/control settings and a short controls hint | Removes terminal-only setup and makes controls discoverable | Persist fullscreen, widescreen, CRT and pan preferences; opening/closing menus does not change selection or move camera |
| 3 | Faster return to play | Less menu friction between battles | Remember skirmish choices; clear play-again path; smoke-check save/load and return-to-menu behavior before changing it |
| 4 | Prove LAN through a full match | Establishes a trustworthy foundation for internet play | Two real machines host/join/start, play 30 minutes, reach results/rematch; test disconnect, mismatched build/data, and mixed macOS/Linux when available |
| 5 | Private “dial-up” internet prototype | Makes playing with a friend easy and memorable | Two separate home networks connect by invite; direct and forced-relay paths work; no port forwarding; mismatch/timeout/cancel leave usable menus |
| 6 | Finish presentation and distribution | Makes the build easy to share and start | Decide whether title concepts improve the existing art; readable UI at common displays; macOS app bundle with external asset setup; documented Linux package path |

Do 1–3 before adding more visual effects. A good short-term milestone is a fullscreen skirmish that is easy to move around, easy to configure, and easy to replay.

## Next exploration: responsive composition from original art

Direction agreed for exploration on 2026-09-09: preserve original pixels and palette, find repeatable sections and compose them at modern screen dimensions. Generated replacement title concepts lost the original style and are not the preferred path.

Evidence: `CODE/DIALOG.CPP:34` already composes `DD-BKGND.SHP`, `DD-EDGE.SHP`, side/top/bottom bars and corner caps. Its background and bar coverage still contains fixed extents; do not assume it handles arbitrary dimensions today. `CODE/INIT.CPP:2485` loads the title as a single `TITLE.PCX`/`TITLE.CPS` image. The sidebar already has separate SHP assets.

First study superseded: user rejected repeated side modules and stretching small dialogs. Keep small dialogs compact. Full-screen creation/setup screens should use one full-width original frame and a continuous red command-table background. Title composition is a separate problem.

Latest local review: `build/art-study/v2/index.html`, produced by `build/art-study/compose-v2.py`. Includes setup and split-surround title at 4:3, 16:10, 16:9 and 21:9. Original DD sprites, palette and extracted bitmap fonts are reused; only a clean grid cell repeats, with the unique background plate placed once. Setup is a static first layout using representative existing fields; map preview, difficulty and network player roster are not yet composed. Title moves the exterior shell halves outward and keeps the logo centered; upper silhouette and lower joins still need review/refinement. This is not production-ready title artwork.

Verification: all eight indexed compositions retain the original palette; 4:3 title remains byte-identical; output displayed with 5:6 pixel aspect. 16:9 previews visually inspected. No runtime or hitbox changes; no CRT baked into previews. Extracted assets and helper scripts remain ignored under `build/art-study/`. Next: review this direction, finish one complete creation-screen layout, then integrate its drawing and input rectangles together. Small dialogs retain their original compact dimensions.

- Fixed pieces: logos, lettering, unique illustrations, corner caps, bolts and curved frame segments. Keep their proportions and pixel detail.
- Repeatable pieces: verified seamless metal/rib strips, flat dark panels, straight borders, and red grid sections. Align grid phase and inspect tile seams; a crop is not automatically seamless.
- Reposition rather than stretch: centered title/command group, corners anchored to frame bounds, sidebar anchored to the right. Let added space expose battlefield or useful rows when the chosen logical resolution allows it.
- The title's curved mechanical surround may not tile cleanly. Test intact repositioned shell sections; do not add repeated side modules. Do not mirror lighting, stretch the logo, or invent repeated distinctive damage.
- Reuse the existing dialog composition code before introducing a generalized skin system. Derive drawing rectangles and hitboxes from the same layout values.

Accept the visual direction before integration: original-size parity, recognizable original composition, no conspicuous seams, preserved palette/pixel aspect, readable labels, and clean plus CRT inspection. Once selected, integrate one full-screen creation/setup screen first, then reuse proven pieces for settings, skirmish setup and other dialogs. In-game settings and controls hints remain the next functional feature; private dial-up multiplayer follows LAN validation.

## Camera specification

- Middle-button drag grabs the battlefield; movement follows the hand. Retain left drag for selection and existing right-click commands/cancel behavior.
- Wheel pans vertically over the battlefield; Shift+wheel pans horizontally. Use horizontal wheel events and both trackpad axes directly, including small deltas. Honor SDL flipped/natural-scroll direction consistently and expose direction/speed preferences.
- Wheel over the sidebar belongs to its build list. Menus and dialogs retain their own input. Pan only during tactical play; suppress edge scrolling during drag and release capture on focus loss, Escape, or mouse-up.
- Keep edge scrolling available, with a setting and a visible hint for the new methods. Offer a keyboard-assisted drag alternative after checking existing key bindings; do not casually rebind Space or WASD.
- Reuse the map's scrolling/bounds logic (`CODE/CONQUER.CPP`, `CODE/SCROLL.CPP`, `DisplayClass::Scroll_Map`) and SDL coordinate mapping (`PORT/MAC/src/mac_sdl_runtime.cpp`). Inspect the mobile delta accumulator for reusable behavior, but avoid driving desktop dragging through coarse synthetic arrow-key bursts if that loses precision.
- No zoom in this first change: it affects pixel scale, UI and picking and is a separate decision.
- Check diagonal motion, all map edges, high-DPI scaling, CRT aspect mapping, minimized/focus-changed windows, selection rectangles, sidebar, and menus. Use the existing input harness for meaningful regression coverage, then an actual skirmish playtest.

## Dial-up over modern internet

### Player experience

Choose **Multiplayer → Dial a friend**. The host chooses **Wait for call** and gets a temporary shareable invitation, displayed like a modem address. Their friend pastes it into **Dial**. The host sees an incoming call and accepts. A brief, skippable modem handshake leads into the existing lobby, where both players choose sides/map and ready up.

Keep the nostalgia in sounds, typography and connection states: dialing, ringing, negotiating, connected, busy, no answer. Do not intentionally add gameplay lag or force a long sound sequence. No account, matchmaking ladder or public server browser in the first version. “Saved numbers” can wait until repeated use justifies it.

### Recommended transport direction

Reuse the network game and its command/frame synchronization (`CODE/QUEUE.CPP`, `CODE/IPXMGR.CPP`, `CODE/NETDLG.CPP`); add an internet transport adapter at the existing packet boundary (`PORT/MAC/src/lan_udp.cpp`, `CODE/WSPROTO.H`). Do not revive physical serial/modem emulation. The familiar modem menu is the entrance to a modern network session.

Prototype a maintained WebRTC data-channel library such as libdatachannel behind a small C boundary: the game is C++98, so build the library separately with its required standard. Its C API and ICE/STUN/TURN support make it a candidate, not yet a tested dependency choice. Start with two players and preserve message boundaries and stable peer identities. Audit current reliability/retransmission first; select unordered/unreliable data-channel settings only if existing game networking supplies the required guarantees.

The service footprint is a small HTTPS/WebSocket rendezvous service plus STUN and TURN relay access. The rendezvous maps the temporary invite to the waiting host and exchanges connection metadata. ICE attempts a direct route; TURN carries encrypted traffic when NAT/firewall conditions prevent it. A key alone cannot make two unreachable peers connect, and a usable P2P product cannot assume every router permits hole punching.

Invites should be random, expiring and revocable. Prefer a sufficiently long copy/paste token or link; any shortened human-entered code requires strict attempt limits and host acceptance. Bind the accepted connection to the invitation, use encrypted authenticated transport, short-lived relay credentials, packet-size limits, and explicit build/protocol/map-data compatibility checks before starting the game. Never expose the existing unauthenticated LAN socket as the internet solution.

GameNetworkingSockets is another credible candidate, but its standalone library does not automatically supply Steam's signaling/authentication/relay services. Do not plan around free Steam infrastructure. A relay-first prototype can validate the experience before optimizing direct P2P, but use an established encrypted transport rather than inventing a secure UDP protocol.

### Prototype gates

1. First establish complete LAN match correctness and document packet delivery/synchronization assumptions, including cross-platform integer widths.
2. Build a two-peer transport spike with invite, accept, cancellation, expiration and compatibility rejection; no modem art yet.
3. Run a full match on separate networks, then force relay and inject representative latency/loss. Confirm no desync, bounded queues and clear timeout behavior. Measure traffic to estimate relay operating cost.
4. Add the modem presentation and connect it to the existing lobby. Returning after failed calls must work. Reconnect/resume is deferred until the game's session behavior supports it; initially offer clear disconnect and restart.

Technical references checked 2026-09-08:

- [libdatachannel implementation and platform/API support](https://github.com/paullouisageneau/libdatachannel)
- [WebRTC peer connections, signaling and ICE](https://webrtc.org/getting-started/peer-connections)
- [GameNetworkingSockets P2P integration](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/README_P2P.md)

## Remaining loose ends

- Fresh Linux CI after the new OpenGL dependency; mobile SDK/device builds after the shared-runtime merge. Desktop-only CRT must stay isolated.
- Measure clean and CRT frame pacing in gameplay, not just startup; validate fallback/error behavior on unsupported graphics contexts. Preserve legibility before increasing effects.
- Fullscreen/widescreen menu alignment, movie aspect ratios, sidebar picking, and returning from movie/menu to battlefield.
- Save/load round trips, campaign progression, skirmish exit/rematch and long-session stability; expand tests only around failures or changed behavior.
- LAN broadcast discovery, firewall errors, disconnect UX and complete mixed-platform matches. Current loopback test covers only packet send/receive and sender addresses.
- Intel macOS and physical mobile device validation; mobile release packaging; expansion content support remain later priorities.
- Package/legal asset separation is already documented: keep local game data out of commits and distributable bundles. No new asset distribution is part of this plan.

Continuation prompt: “Read docs/PROJECT_PLAN.md. Explore responsive title/menu composition using original local game art: identify fixed and tileable crops, produce a local contact sheet and aspect-ratio previews, preserve the original palette and pixel aspect, and keep extracted art out of Git. Reuse existing dialog sprites where possible. Do not integrate the layout until the visual direction has been reviewed.”

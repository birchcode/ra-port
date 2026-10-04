# Movement reliability — 2026-09-10

Implemented in the main checkout; existing unrelated edits preserved. No installation or multiplayer deployment.

- FOOT.CPP / FINDPATH.CPP: human move orders compare a complete moving-traffic route against a clear detour; use the shortcut when the detour costs over 1.5 times as much, retaining traffic costs. Extended to human harvesters. Copy the chosen path's own length because Find_Path reuses static results. Ordinary human move orders cannot plan through destroyable blockers. These policies use the pending mission when destination assignment starts routing before the next AI update.
- FOOT.H / FOOT.CPP / DRIVE.CPP / INFANTRY.CPP: shared traffic wait starts at blockage, lasts at least three seconds, and resets on successful movement. Persistent queues skip the traffic shortcut for one search. Uses existing retry/timer storage; no saved-object layout change. Vehicles and infantry retain their destinations. Actual collision rules still apply.
- DRIVE.H / DRIVE.CPP: head-on harvesters of the same human owner choose a yielding truck by unit ID. One waits while the other takes a free step back/sideways; economic destinations remain intact. No yield if docking, dumping, moving, paralyzed, or no escape cell exists. Prevent automatic obstacle-attack overrides during ordinary human movement, including two-cell tracks.
- FINDPATH.CPP: when a route must choose between the two sides of a temporary obstruction, compare accumulated movement cost first and path length second. This prevents a short but expensive detour from winning over a longer, cheaper route around a bridge or chokepoint.
- RAWOLAPI.H / cmake/ra95_common.cmake: bumped the network/build game version from `0x00030003` to `0x00030004` so peers reject mismatched movement rules instead of silently desynchronizing.
- DRIVE.H / DRIVE.CPP: formation movement now has a local yielding rule. When a faster same-group formation member is blocked by a slower stationary member, the slower member requests a temporary legal scatter step so the faster unit can keep moving. This is a bounded first step toward full corridor reservation and does not invent formation metadata for ordinary selections.
- FOOT.H / FOOT.CPP: damage alerts up to two nearby idle same-house infantry or vehicles with a usable weapon. Existing move, attack, tether, and mission orders are preserved; only eligible guard/stop/idle units within six cells respond. Vessels and aircraft are not included yet.
- EVENT.CPP: fresh orders reset stale path delay, retries, and cached route, including repeated destinations. A repeated current order also clears a different pending mission; queued waypoints preserve current movement.
- AIRCRAFT.CPP: helicopter attack flight repicks a firing position when the target moves beyond its range; armed human helicopters retry unavailable firing positions. Transport helicopters establish exclusive radio contact with each waiting passenger.
- INFANTRY.CPP / UNIT.CPP / FOOT.CPP: retain the intended transport through staging/radio interruptions; moving transports with capacity can defer docking without losing the boarding order. Full or invalid transports still abort.

Verification: desktop build and 20 scenario groups in tests/path_traffic_test.cpp passed, including real order execution, vehicle/infantry movement starts, head-on yield/no-space cases, formation yielding, nearby ally response, helicopter attack states/takeoff, and transport radio handoff. Routing terrain is synthetic; these are real engine methods without rendering/assets. Existing AI campaign, combat RNG, and menu regressions passed. Evidence: build/{path,ai,combat,menu}-regression/test.log. Run: python3 tests/run_ai_campaign_test.py build path (or ai/combat/menu).

Limits / next verification: live six-unit bridge and ore-truck playtest, mixed infantry/vehicles, crowded landing and boarding. Formation yielding is local rather than a full group reservation system, and nearby defense currently excludes vessels and aircraft. This is not global enemy-threat avoidance; fully blocked roads may remain unreachable. Explicit combat retains normal friendly splash damage. Multiplayer peers require matching builds. No claim that every community report is reproduced or eliminated.

## Player reports for follow-up (not verified defects in this port)

- Army rear takes a map-wide detour while front crosses a bridge; partial attack response and transport boarding failures: https://backloggd.com/u/Icelight/review/2244064/ (search-index excerpt; full page blocked).
- Red Alert units require repeated orders, Hind tracks a target's old position, some aircraft remain grounded, dangerous detours and splash damage near chokepoints: https://steamcommunity.com/app/1213210/discussions/0/2789369351340230957/ (July 2020, full discussion read).
- Harvester bridge jams: https://www.reddit.com/r/commandandconquer/comments/r78k3n/ ; https://www.reddit.com/r/commandandconquer/comments/gxjekc/ (second discussion spans both remastered games).
- Broader complaints about attack-move and AI: https://www.reddit.com/r/commandandconquer/comments/gy9zuq/ (mixed remastered collection discussion, includes explicit Red Alert feedback).

These are qualitative community reports, not a frequency ranking. Keep Tiberian Dawn-only complaints separate from Red Alert.

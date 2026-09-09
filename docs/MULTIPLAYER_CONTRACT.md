# Multiplayer packet and synchronization audit — 2026-09-09

## Result

Local transport/reliability checks and the macOS game build pass after fixing two defects. **LAN match correctness remains unverified.** No two-machine match has been run; internet transport remains gated on the checks below.

Audited checkout: `ea5aa8f` plus these multiplayer edits and concurrent CRT edits. The earlier staged AI changes were committed by concurrent work during the audit; they were not edited here. A Git commit alone is not a binary identity for a dirty checkout.

## Demonstrated fixes

- `CODE/CONNECT.H`: packet IDs, send/receive sequence counters and last-read/last-sequential IDs now use `uint32_t`. On LP64, `0xffffffff + 1` in the original unsigned-long counters was 4294967296, so reliable packet zero could neither advance the sequence nor be read. The regression failed at the first expected delivery before the change and passes afterward. This changes the LP64 communication header layout; **rebuild both peers**. It does not make other native-layout payloads portable or establish sequence-number rollover safety after billions of packets.
- `PORT/MAC/src/lan_udp.cpp`: `recvmsg` detects and discards truncated datagrams. Previously an oversized UDP datagram was returned as a complete prefix. Empty datagrams are skipped as well. The regression queues oversized and empty messages ahead of a valid message and verifies only the valid message is returned.

## Contract traced from current code

| Layer | Actual behavior and constraints |
| --- | --- |
| Native transport | `lan_udp.cpp`: IPv4 UDP, nonblocking polling, one message per datagram, no transport reliability or encryption. Default game port 34835. Sends return no status to the caller; legacy `IPXConnClass::Send_To` reports success after `WriteTo`. |
| Peer identity | Legacy four-byte network number is zero; six-byte node carries IPv4 bytes and network-order port. Port bytes are zero when source uses the local configured game port; `WriteTo` substitutes that port. Private channels match this address. This is endpoint identity, not authentication. |
| Discovery | Broadcasts go to up to 16 enumerated interface broadcast addresses, with limited-broadcast fallback. No active-interface filtering beyond address family and broadcast flag. Locally sourced packets with the configured source port are discarded. Two ordinary game instances sharing one host/port are not a substitute for two-machine validation. |
| Lobby | `NETDLG.CPP`: query/answer discovery, `NET_QUERY_JOIN`, host confirmation/rejection, scenario/options exchange; host sends ACK-required `NET_GO` or `NET_LOADGAME`, waits for queue completion, then `NET_READY_TO_GO` or scenario requests. The response wait is 10 seconds. Scenario transfer can precede entry to play. |
| Global channel | `IPXGCONN.CPP`: `GlobalHeaderType` is a communication header plus product ID. Global messages may be broadcast or directed and may request ACKs. Duplicate suppression tracks only the last four received address/ID pairs; it is not an ordered exactly-once channel over arbitrary delays. |
| Private channel | `CONNECT.CPP`: ACK-required data has its own incrementing sequence. Out-of-order data queues; application delivery waits for the next ID. Duplicates are re-ACKed without another delivery. One receive slot is reserved to allow a missing next packet to enter. NOACK data is not sequence-blocked or deduplicated. |
| Reliability | Polling `Service` retransmits unacknowledged data after `RetryDelta`, removes ACKed entries, and marks NOACK messages complete after one send. Retry/timeout failure is evaluated when a send occurs. `IPXMGR.CPP` discards an expired global send and reports a failed private connection. There is no transport-level reconnect/resume. |
| Capacity | `GLOBALS.CPP`: global queues have 160 entries and payload capacity `max(sizeof(GlobalPacketType), sizeof(RemoteFileTransferType))`; private queues have 32 entries and payload capacity `floor((546-sizeof(CommHeaderType))/sizeof(EventClass))*sizeof(EventClass)`. Headers are added by the connection layer. `IPXMGR.CPP::Service` reads into 1024 bytes. `COMBUF.CPP` rejects packets above queue capacity, but this is not complete structural validation. |
| Lockstep | `QUEUE.CPP`: commands are scheduled ahead by `Session.MaxAhead` and transmitted reliably; `FRAMESYNC` progress packets are NOACK. `Can_Advance` requires adequate peer frame progress and receipt of the command counts advertised by peers. A progress packet overtaking lost command data must not permit advance. CRC checks detect some divergent state; there is no authoritative state repair. |
| Timing | Lobby starts with retry 30 ticks, unlimited retries, timeout 600 ticks (60 Hz). Loading and gameplay adjust timing. Frame-sync dialog/timeout bases are 3 and 15 seconds, multiplied/adjusted by gameplay settings and response time; do not treat these as a single fixed disconnect deadline. |
| Cleanup | Native `Close` closes the socket; reopening resets interface lists. `Stop_Listening`, `Discard_In_Buffers`, and `Discard_Out_Buffers` are currently no-ops. In particular, the input-discard call in `INIT.CPP` does not drain a live socket. Rematch/stale-packet behavior needs runtime validation. |

## Compatibility and validation gaps

- Lobby version negotiation uses legacy min/max versions and protocol selection, not a source/build hash. Scenario name/digest lookup and transfer exist; these do not prove identical rules, all game data, compiler ABI, or concurrent AI logic. Explicit build/protocol/data compatibility is needed before an internet release.
- Headers and event/global payloads are copied as native C++ structures. The sequence fix only standardizes those fields. Other `long` fields, enum sizes, padding, bitfields, pointers in variable-event paths, and host byte order remain ABI assumptions. No canonical wire encoding was found in the traced flow. Same architecture/build/data is the initial test requirement; mixed macOS/Linux remains an acceptance test, not an established guarantee.
- The manager checks only the common minimum header before routing. Global parsing requires a larger header; gameplay processing reads frame fields before validating payload length. Compressed event decoding has further type/length assumptions. Queue capacity checks and dropping truncated UDP datagrams do not make these parsers safe against malformed peers. Full structural validation is still required before accepting internet packets.
- UDP source addresses and product magic numbers are unauthenticated. `GAME_INTERNET` also contains a legacy address-rebinding heuristic based on frame-sync data. Do not reuse that heuristic as authenticated peer binding.
- A future adapter must preserve message boundaries and stable peer addresses. An unreliable datagram mode most closely matches this contract; reliable ordered delivery would add another retry/order layer and could delay frame-sync traffic behind lost data. No WebRTC dependency or delivery mode is selected by this audit.

## Local evidence

- `tests/connection_test.cpp`: real `ConnectionClass`/`CommBufferClass`; first reliable delivery, out-of-order buffering, duplicate suppression, ACK cleanup, independent NOACK delivery, dropped-data retransmission and timeout. Clock/debug platform stubs only; no simulation or lobby replacement. Assertions explicitly enabled because game headers otherwise disable them.
- `tests/lan_udp_test.cpp`: real loopback socket, incoming/outgoing payload and IPv4/port mapping; oversized/empty datagram rejection. Socket execution needed permission outside the sandbox; the final authorized socket test passed.
- Existing `tests/run_script_tests.sh`: passed, including current shared-code checks. Invoked with an exported no-op `rm` shell function to retain temporary artifacts; neither its cleanup nor the nested include generator permanently deleted files.
- Full existing macOS target: 167 build steps including successful link. Used a retained copy of the existing Ninja manifest to avoid rerunning the include generator's deletion step; no changes to existing runtime target configuration. New `connection_test` CMake target was compiled directly with the existing compile-database flags; fresh CMake generation was subsequently verified on the Mac Pro Linux installation (see handoff).
- Evidence: `build/network-audit/game-build.log`, `script-tests.log`, `compile.log`; binaries `connection_test`, `lan_udp_test`, and `build/redalert_mac`. The compile log includes the expected sandbox socket-bind failure; the authorized run succeeds separately. This is not an end-to-end match test or sanitizer/fuzz coverage.

## Second-machine acceptance record to complete

Record both OS/architecture versions, commit plus dirty diff, binary SHA-256, game-data hashes and selected map/settings. Use identical rebuilt code/data first, then repeat across macOS/Linux.

1. Discover, join, update lobby settings, ready/start, and issue commands from both players. Verify initial commands execute on both sides.
2. Play at least 30 minutes with construction, harvesting, combat and AI where enabled; verify neither stalls nor reports desync. Capture logs/results from both peers.
3. Finish normally, compare results, return to menus, and start a rematch without stale peers/commands.
4. Repeat with host exit, guest exit and network loss during lobby, loading and gameplay; record actual timeout and recovery/menu behavior.
5. Try incompatible builds, rules/data and missing/changed maps; record whether rejected, transferred, or incorrectly accepted. Legacy version acceptance alone is insufficient.
6. Test representative packet loss, duplication, delay and reordering. The local queue tests verify mechanisms, not whole-game behavior under impairment.

Next gate: close these LAN checks and parser/compatibility gaps before implementing the two-peer internet adapter. No service deployment or invitations were performed.

## Linux installation follow-up

The current game source is installed separately at `/home/rmp/ra-port-network-audit-20260909` on `rmp@rmp-macpro5-1.local`. Fresh CMake configuration, game build and both network tests pass. Test-only fixes select game quoted headers ahead of duplicate library names and discard unused legacy vtables at Linux test link time. Xvfb startup stayed alive for 12 seconds; no screenshot was captured because ImageMagick is unavailable. All eight MIX archives match the Mac. Launcher: **Red Alert — LAN Test**. Exact binary hashes and source-snapshot notes are in `build/network-audit/macpro-install.json`. A real synchronized match remains open.

# How clients communicate in a multiplayer match

## Direct first, with server relay fallback (2026-11-23)

Online pairs prefer direct UDP. When joining a room, each pair gets three direct
attempts, each bounded at **5 seconds**, including the peer login, identity and
connection test. After three failures the clients agree to use the server relay.
If a local endpoint is available, attempts use local, public, then local; otherwise
all three use the public endpoint. Pairs are independent: one player can use a
direct connection to one opponent and relay to another.

The initiator still registers the retained peer socket with the server, and both
ends punch from their retained sockets. Bound local endpoints are exchanged before
login using `CClientGameConnectInfo` inside `CDirectPacketToClient`. This avoids a
public-address hairpin when two instances run on one PC or a reachable LAN. Local
address discovery has a one-second collection window. A normally responsive lobby
therefore reaches relay after roughly 16 seconds of unsuccessful peer setup,
plus relay testing and confirmation. Registration has its separate existing
20-second allowance, route confirmation has 5 seconds, and a 45-second overall
join bound prevents an indefinite joining screen. A failed join leaves the server
room and emits `CConnectGameFailed`.

Only the joining side selects a pair's route. `CPeerSetupPacket` carries the room,
setup token and attempt number inside the server's existing forwarding envelope.
UDP login starts after the accepting side acknowledges retiring the previous
attempt. UDP identities carry the same token and attempt number, so delayed replies
cannot finish another attempt. Connections remain tentative until the connection
test and route agreement complete. This fixes the historical asymmetric fallback
race where one client used direct transport while the other used relay.

### Recovery during a match

An established direct connection gets **one direct retry**, bounded at 5 seconds,
then switches to relay if that retry fails. The retry reuses the registered socket
and last working endpoint, without another registration round. Either endpoint
can request recovery; the original initiator remains the route-selection authority.
A successful retry stays direct. After fallback, the pair stays on relay for the
rest of that session.

Transport failure triggers recovery. Session heartbeats run every second, and
five seconds without incoming session traffic or acknowledgement progress also
triggers it. A silent failure can therefore take about five seconds to detect,
followed by up to five seconds for the direct retry, then relay negotiation.
These are wall-clock network timers, not simulation segment sizes.

Every ordinary game message is snapshotted, numbered and retained until a
cumulative application acknowledgement arrives. Sequence numbers survive route
changes. Recovery queues new outgoing messages, resends the unacknowledged suffix
on the selected route, and delivers incoming messages in sequence exactly once.
Duplicate messages and acknowledgements from the old route cannot repeat commands.
Recovery does not emit another `CNewGameClient` or reset player slots. The existing
reliable UDP transport still handles ordinary packet retransmission on each route.

The lobby must remain reachable for relay to help. During an established match,
an unresponsive lobby leaves messages queued and the game's existing lag/departure
policy in control; a network timeout alone does not invent a player departure.
Departure, recovery-of-command-history, kick and outcome messages retain their
existing persistent server paths. Server-ordered removal drains received session
messages before publishing departure, including while a route switch is pending.
Simulation timing and replay command semantics are unchanged.

### Room ping and installation

Room ping follows the currently selected connection in both directions. The tooltip
appends **Direct UDP**, **Server relay**, or **Connecting** to the existing ping text.
It queries this client's route to that slot, rather than another player's metadata.
A relayed ping includes the server detour; a direct ping does not. The ping remains
an application RTT including frame processing, queues and retransmission.

Use the updated build on every client: the new peer setup and session packets
require matching clients. The existing server executable can forward these opaque
payloads without modification. On Windows the updated UI and protocol are in
`GameX.dll` and `Client.dll`. Keep the previously updated `Net.dll`, whose endpoint
enumeration export is required by the local-address setup fix.

For a live test, first check that a normal room shows Direct UDP. Block only peer
UDP while leaving the lobby reachable: joining should try direct three times and
then show Server relay. During a match, interrupt a direct pair and check for one
retry followed by relay, resumed command delivery, and no extra player/slot event.
Also test a three-player room with mixed routes and a departure during recovery.
Logs identify each endpoint attempt, recovery request and final selected route.

Automated regression coverage uses real local UDP for registration, peer login,
identity, connection testing and ping, with lobby introduction/forwarding modeled.
It exercises local/public selection, three blocked joining attempts, one-sided
setup, successful mid-match direct retry, blocked retry with relay recovery, and
recovery requested by the accepting peer. Session tests cover retained messages,
cumulative acknowledgements, stale session rejection, duplicates, ordering and
mixed direct/relay routing. Existing departure and room-start tests remain relevant.
Internet NAT traversal and live tooltip appearance require live game checks.

Direct UDP removes the server detour but does not select an optimized Internet
route. The existing 50 ms per-peer send spacing remains, and the unfocused game
adds a 40 ms sleep per main loop in [Game/main.cpp](../Versions/Temporary/Engine/Sources/Game/main.cpp).
These can contribute to same-PC RTT; the original 200-300 ms measurement alone
cannot establish which component caused the delay.

## Baseline design before the direct-only experiment

The following describes the baseline relay-capable design and its timing. The
current implementation above supersedes its fallback and setup-bug descriptions.

In an online custom match, the standalone `Server` introduces the players. Each pair first tries to connect directly over UDP and can fall back to a relay through that server. If Alice hosts a three-player game, Bob can send an order straight to Carol even when his connection to Alice uses the relay. All three machines run the simulation. At normal speed, even an order that arrives immediately takes **600 ms of simulation time from packing to execution**. The room's ping measures a different interval. [Peer connection setup][setup] · [Game packet routing][fanout] · [Simulation timing][ticks]

## From joining a room to issuing an order

1. **Join.** Clients log in and create or join a room on the configured server (UDP port `4200` by default). On a successful join, `CGameLobby` sends the newcomer the IDs of the existing members. The newcomer starts one connection attempt per member. The server exchanges observed IP addresses and ports, and the two clients try a peer login and identity exchange. A test packet on the selected connection produces the game's new-client notification. [Connection settings][settings] · [Member list][members] · [Address exchange][addresses] · [Connection tests][connections]
2. **Start.** The room host broadcasts the final slot roster and assignments. Each client maps network client IDs to simulation slots and loads the map locally. The transceiver broadcasts `ET_LOADED` and waits for the participating peers before advancing. The online host also closes the room to joins and maintains the server heartbeat. [Start packet][start] · [Loading barrier][loaded] · [Online host duties][host]
3. **Play.** Each client collects local orders. On every pack boundary it broadcasts a `CAISegmentFinishedPacket` with the absolute segment number, serialized commands, checksum and reinforcement checkpoint data. An empty command list still reports progress. Each receiver files commands under the issuing player's slot. All peers, including the issuer, execute them in slot order and in list order within a slot, and record the execution segment for replay. Unit positions and movement are computed locally. [Order submission][orders] · [Pack construction][pack] · [Receive path][receive] · [Execution and history][execute]

| Traffic | Route |
| --- | --- |
| Ordinary commands, loaded notices, room pings | `SendGamePacket` uses the selected connection for each recipient. A direct pair uses `CSimpleConnection`. A relayed pair wraps the payload in `CThroughServerGamePacket` on the lobby connection. The server swaps the addressing fields and forwards it. A broadcast with *N* human players has *N - 1* remote deliveries per sender. [Fan-out][fanout] · [Per-peer senders][connections] · [Relay handler][relay] |
| Chat | Online lobby chat uses the server. Room chat uses the game connections. In-match chat sends to each eligible player, filtered by team for team-only messages. [Lobby and room chat][chat] · [In-match chat][match-chat] |
| Departure, recovery and outcome | Online `BroadcastGameControlPacket` explicitly sends a `CDirectPacketToClient` envelope through the persistent lobby connection. For example, a leaving client resends its final command packs and announces its cutoff there before tearing down its game connections. Server traffic during a direct match is therefore expected. [Control route][control] · [Final commands before leave][leave] |

**LAN** uses UDP discovery on port `4201`. The host advertises the room and accepts joins. The LAN driver distributes peer addresses, and `CLANClient::SendGamePacket` sends directly to each listed recipient. From there LAN uses the same transceiver, timing and receipt barrier as online play. [LAN room setup][lan-room] · [Peer introduction][lan-peers] · [LAN send path][lan]

Both modes use the engine's reliable, ordered byte stream over IPv4 UDP. One application packet can span multiple datagrams, and several small messages can share a datagram. The driver acknowledges datagrams, resends unacknowledged stream blocks and holds later bytes until a gap closes. On a relayed connection this happens independently on each leg. [UDP socket][socket] · [Stream framing and retransmission][write-stream] · [Ordered receive][stream]

## What the room ping and command delay measure

**Room ping** is the application round-trip time (RTT) to the player whose slot you are viewing. About every five seconds the client sends a timestamped `CPingPacket` over the game connections. The recipient echoes it to the sender, which subtracts the original timestamp using its own absolute clock. This includes transport queues, both clients' packet processing and, for a relayed pair, two passes through the server. The LAN browser measures discovery request/reply time to the host separately. The transport also estimates RTT from acknowledgements on each physical connection. [Room timer][room-ping] · [Echo and display value][echo] · [LAN discovery RTT][discovery] · [Transport RTT][acks]

**Order delay** follows the simulation clock. The game sets a segment to 200 ms. Multiplayer starts with `nLatency = 2` and `nSegmentsPackSize = 1`. For an order packed at segment 100, the current code does the following. [Game clock][clock] · [Multiplayer settings][ticks] · [Command indexing][receive]

| Segment | What happens on an uninterrupted 1x game clock |
| --- | --- |
| 100 | The sender packs the order and reserves command slot 101. Receivers put the order in the same slot. |
| 102 | The ordinary receipt barrier requires packet 100 from each participating peer. A missing packet pauses progress here. |
| 103 | The executor consumes command slot 101. The order runs on the issuer and the other clients at this simulation boundary. |

The formula is **execution segment = packet segment + pack size + latency**. An order queued between boundaries also waits for the next pack, giving roughly **600 to 800 ms from queuing to execution** at 1x with regular frames and timely delivery. Network transit overlaps this interval. A late required packet or a slow client can extend wall-clock time for everyone. The existing departure test illustrates the same rule with packet 14667 and a command recorded at 14670. [Receipt barrier][barrier] · [Timing formula][formula] · [Departure test][drop-test]

## Where latency can be reduced

**1. Help more pairs connect directly.** The initiating client opens a per-peer socket but sends its registration on the persistent lobby connection. The server advertises the source address and port it saw on that registration. On both sides, `SendBreakThroughPacket` opens yet another socket on an OS-selected port. A router performing network address translation (NAT) can therefore open a mapping for a different port from the one that needs to receive the game traffic. Test registering and probing from the **same bound socket used for the peer connection**. Compare direct-connection success with players behind separate routers and keep relay fallback. For pairs that need the relay, server operators can choose a location with a low combined RTT to both players. [Initiator registration][registration] · [Server observation][relay] · [Probe socket][probe-socket] · [UDP mappings and filtering][nat]

**2. Forward relayed packets sooner.** The server drains incoming packets and sends replies on a **50 ms** loop. A packet arriving just after a drain can wait nearly one period on each relay traversal, longer under load. A relayed ping passes that point twice. Wake the forwarding loop on queued traffic while keeping lobby and database maintenance on their own schedules. Preserve ordering between forwarded commands and departure notices. This change targets relayed pairs. [Server loop][server-loop] · [Receive and send order][server-pump]

**3. Reduce queues during bursts and loss.** Normal updates to one transport peer are spaced by at least **50 ms** and carry at most **255 bytes of stream data** each, about 5 KB/s per direction before retransmissions. An idle sender with an open congestion window can send promptly. Acknowledgements (ACKs) usually ride outgoing packets, with an idle send after one second. A quiet receiver can therefore leave a sender waiting for an ACK. Benchmark shorter send spacing and a bounded delay for pending ACKs under asymmetric traffic. Keep congestion control and track the cost in datagrams per second. [Send gate][pacing] · [Stream block limit][block] · [ACK and retransmit timing][acks] · [UDP pacing guidance][udp]

**4. Shorten order delay when ping is already low.** Prototype executing packet 100's commands at its existing receipt barrier (102). This would save **one 200 ms tick at 1x** while retaining the current delivery deadline. Every peer must agree on the rule. Check replay dispatch, checksums and the last command accepted before a player drops, all of which depend on execution boundaries. Simply setting `nLatency` to 1 makes `nLatency / 2` zero, which the pack-boundary check uses as a modulo divisor. `MaxLatency` sizes history buffers. Lag and disconnect timeouts govern how long to wait for missing peers. [Pack sizing][ticks] · [Receipt barrier][barrier] · [Pack-boundary check][pack-check] · [Drop calculations][formula]

## How to tell which change helped

1. **Identify the route per pair.** During room setup, the server can log `clients ... are connected through the server` when console logging is enabled. A relay subscription can also be requested when a direct connection closes. To confirm steady-state routing, record the selected connection type per pair or inspect the endpoints of sustained gameplay traffic. Lobby heartbeats alone give no route information for orders. [Server relay logging][route-log] · [Direct connection teardown][teardown]
2. **Measure three intervals separately.** Compare room echo RTT, time queued before transport send, and local order enqueue-to-execution time. Use a monotonic clock on the same machine for each interval. Record the segment and missing-player mask when the simulation waits. The existing `last_match_packets.txt` correlates sent and received packs by segment and sender. Its event rows contain no wall-clock timestamp, so RTT needs a separate measurement. [Match trace format][trace]
3. **Compare like with like.** Use the same map, speed and commands on direct LAN, direct online and relayed online connections. Include a three-player mixed-route match and a run with loss or jitter. Compare median and 95th-percentile RTT, time spent stalled, command response, retransmissions and datagrams per second. Check that peers and replay agree on command segments and checksums after a player leaves. [Transport regression tests][net-tests] · [Departure regression tests][drop-test]

The timing figures above are derived from source at `10fc837cd`. Local Windows Release tests passed for ordered transport under loss/reordering, connection removal, graceful leave and replay command timing. Internet RTT, NAT traversal success rates and the proposed improvements still require measurements on live player connections.

[settings]: ../OBK2_Examples/MultiplayerConnectionSettings/README.md
[members]: ../Versions/Temporary/Engine/Sources/Server/GameLobby.cpp#L272
[addresses]: ../Versions/Temporary/Engine/Sources/Server/GameLobby.cpp#L331
[setup]: ../Versions/Temporary/Engine/Sources/Client/PlayGameProcessor.cpp#L81
[connections]: ../Versions/Temporary/Engine/Sources/Client/ConnectionInternal.cpp#L112
[fanout]: ../Versions/Temporary/Engine/Sources/Client/PlayGameProcessor.cpp#L384
[start]: ../Versions/Temporary/Engine/Sources/GameX/MPManagerMode.cpp#L316
[loaded]: ../Versions/Temporary/Engine/Sources/GameX/MPTransceiver.cpp#L1148
[host]: ../Versions/Temporary/Engine/Sources/GameX/MPManagerModeNivalNet.cpp#L213
[orders]: ../Versions/Temporary/Engine/Sources/GameX/MPTransceiver.cpp#L158
[pack]: ../Versions/Temporary/Engine/Sources/GameX/MPTransceiver.cpp#L533
[receive]: ../Versions/Temporary/Engine/Sources/GameX/MPTransceiver.cpp#L980
[execute]: ../Versions/Temporary/Engine/Sources/GameX/MPTransceiver.cpp#L419
[relay]: ../Versions/Temporary/Engine/Sources/Server/ControlLobby.cpp#L272
[chat]: ../Versions/Temporary/Engine/Sources/GameX/MPManagerModeNivalNetChat.cpp#L21
[match-chat]: ../Versions/Temporary/Engine/Sources/GameX/MPManagerModeMessages.cpp#L403
[control]: ../Versions/Temporary/Engine/Sources/GameX/MPManagerModeNivalNet.cpp#L339
[leave]: ../Versions/Temporary/Engine/Sources/GameX/MPManagerModeMessages.cpp#L289
[lan-room]: ../Versions/Temporary/Engine/Sources/GameX/MPManagerModeLAN.cpp#L374
[lan-peers]: ../Versions/Temporary/Engine/Sources/Net/NetPeer2Peer.cpp#L523
[lan]: ../Versions/Temporary/Engine/Sources/Client/LANClient.cpp#L84
[socket]: ../Versions/Temporary/Engine/Sources/Net/LinksManager.cpp#L84
[write-stream]: ../Versions/Temporary/Engine/Sources/Net/NetStream.cpp#L27
[room-ping]: ../Versions/Temporary/Engine/Sources/GameX/MPManagerMode.cpp#L31
[echo]: ../Versions/Temporary/Engine/Sources/GameX/MPManagerModePackets.cpp#L652
[discovery]: ../Versions/Temporary/Engine/Sources/Net/NetServerInfo.cpp#L89
[clock]: ../Versions/Temporary/Engine/Sources/GameX/Initialization.cpp#L82
[ticks]: ../Versions/Temporary/Engine/Sources/GameX/MPTransceiver.cpp#L143
[barrier]: ../Versions/Temporary/Engine/Sources/GameX/MPTransceiver.cpp#L291
[formula]: ../Versions/Temporary/Engine/Sources/GameX/MPPlayerDrop.h#L18
[drop-test]: ../Versions/Temporary/Engine/Sources/GameX/test/MPGracefulLeave.cpp#L543
[registration]: ../Versions/Temporary/Engine/Sources/Client/ConnectionEffortInternal.cpp#L149
[probe-socket]: ../Versions/Temporary/Engine/Sources/Client/ConnectionEffort.cpp#L11
[nat]: https://www.rfc-editor.org/rfc/rfc4787.html#section-4.1
[server-loop]: ../Versions/Temporary/Engine/Sources/Server/main.cpp#L149
[server-pump]: ../Versions/Temporary/Engine/Sources/Server/Server.cpp#L277
[pacing]: ../Versions/Temporary/Engine/Sources/Net/NetA4.cpp#L938
[block]: ../Versions/Temporary/Engine/Sources/Net/NetStream.h#L98
[stream]: ../Versions/Temporary/Engine/Sources/Net/NetStream.cpp#L133
[acks]: ../Versions/Temporary/Engine/Sources/Net/NetAcks.cpp#L80
[udp]: https://www.rfc-editor.org/rfc/rfc8085.html#section-3.1.6
[pack-check]: ../Versions/Temporary/Engine/Sources/GameX/MPTransceiver.cpp#L486
[route-log]: ../Versions/Temporary/Engine/Sources/Server/GameLobby.cpp#L676
[teardown]: ../Versions/Temporary/Engine/Sources/Client/PlayGameProcessor.cpp#L193
[trace]: ../Versions/Temporary/Engine/Sources/GameX/MPPacketTraceLog.cpp#L198
[net-tests]: ../Versions/Temporary/Engine/Sources/Net/test/NetTransport.cpp#L296

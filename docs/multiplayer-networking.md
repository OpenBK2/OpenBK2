# How clients communicate in a multiplayer match

## Direct-only test build (2026-11-23)

The current client disables relay fallback for ordinary game traffic. Commands,
room chat and room ping use direct peer connections exclusively. Both initiating
and accepting connection attempts stop on failure instead of constructing a
`CThroughServerEffort`. There is no runtime switch: use this build on every test
client. The server executable and packet formats do not change. On Windows,
install both `Client.dll` and `Net.dll`: local endpoint enumeration now has the
DLL export that the client needs.

The lobby still handles login, discovery, peer introductions and heartbeats.
Departure, recovery and outcome traffic keeps its existing persistent server
route, including final command retransmissions before leaving. The
`CThroughServerConnectionPacket` sent when a direct link closes still subscribes
to ordered departure notifications; it does not install a game relay.

To make direct connections usable, the client now registers its retained peer
socket with the server and sends hole-punch packets from that same socket. The
accepting side also punches from its retained socket. Setup failures are drained
even without incoming application packets, both setup phases have a timeout,
and packets received before an accepting connection is installed are delivered
in order on installation. These address the socket, timeout and early-packet
findings in the historical [connection audit](netcode-connection-audit.txt) and
[netcode audit](netcode-audit.txt).

The first direct-only build still failed the same-PC online join: the server
introduced both instances using their public IP and different UDP ports, but
the peer login timed out. The local-address shortcut only runs after login and
could not rescue that initial connection. The logs establish the failed public
path; they do not distinguish router hairpin support from firewall filtering.
The joining screen also kept waiting because the failure was reported as a
player removal before that player had a slot.

The corrected client forwards its bound local endpoints during setup using
`CClientGameConnectInfo` inside the existing `CDirectPacketToClient` envelope.
This is address discovery only, not relayed gameplay or ping. The initiator
collects offers for one second, tries a local address on its own subnet (preferring
an address on the same machine), then tries the public endpoint if that attempt
fails. An individual peer attempt is bounded at seven seconds, within the existing
overall join timeout. Peers without offers still use the public endpoint.
An unsuccessful join now leaves the server room and emits `CConnectGameFailed`,
including when the final connection-test exchange never completes.

Test procedure:

1. Build/install the updated client on both instances. A successful `Client`
   build alone does not update an installed game's DLLs. Keep the same
   server, map and game settings as the baseline test.
2. Join an online room and wait for several ping updates (one every five seconds).
   Record both displayed RTTs, frame rates and which instance has focus. Where
   packet logging is enabled, `Logs/client.log` records `direct peer ... connected;
   game relay disabled` or `direct peer ... failed; game relay disabled`.
   The `direct peer ... trying IP:port` lines show each attempted endpoint.
3. Compare with a LAN room on the same PC. Use a packet capture to check the actual
   peer endpoints; the absence of ordinary relay traffic does not mean the lobby
   connection is idle. Some routers cannot connect two clients through their own
   public address; the new local-address offers avoid that path on a reachable
   local network. Some NAT/firewall combinations still prevent direct connections.
   Such pairs fail instead of silently using the relay.
4. Start a match and test one player leaving, since final commands still use the
   preserved recovery path. The simulation's command delay has not changed.

Direct UDP removes the server detour but does not select an optimized Internet
route. The transport does attempt advertised local-address shortcuts after peer
login (`TRY_SHORTCUT` in [NetA4.cpp][pacing]); this is endpoint selection, not an
Internet routing service. Room ping remains an application RTT, including queues
and frame processing. The existing 50 ms per-peer send spacing remains, and the
unfocused game adds a 40 ms sleep per main loop in [Game/main.cpp](../Versions/Temporary/Engine/Sources/Game/main.cpp).
These can contribute to same-PC RTT; the reported 200-300 ms is not yet attributed
to a measured cause.

Validation: the Windows x64 Release `Client` and `ClientConnectionRemoval_test`
targets build with the Visual Studio toolchain. The client regression suite covers
direct-only failure, retained-socket registration/punching, early packet delivery,
room ping routing and join-failure notification. Three tests use real UDP sockets
for registration, login, identity, connection testing and ping: a working local
offer with an unanswered public endpoint, public retry after a failed local
attempt, and a public connection without local offers. Only lobby introduction
and forwarding are modeled in those tests. Live game UI RTT and Internet NAT
traversal still need measurement.

## Baseline design before the direct-only experiment

The following describes the baseline relay-capable design and its timing. The
direct-only experiment above supersedes its fallback and setup-bug descriptions.

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

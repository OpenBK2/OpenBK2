# Multiplayer connection settings in game data

The game client can read its online server address, UDP port, and network
protocol version from `SMultiplayerConsts`. Merge the three fields in
[MultiplayerConsts.fields.xml](MultiplayerConsts.fields.xml) into the existing
`MultiplayerConsts` record referenced by `GameConsts.Multiplayer`. Keep its
other fields, including sides, technology levels, and player colors. The
existing `mp_consts` resource override also works.

```xml
<NivalNetIP>server.nival.net</NivalNetIP>
<NivalNetPort>4200</NivalNetPort>
<NetGameVersion>3</NetGameVersion>
```

These are the values from the supplied `Profiles/game.cfg`. Replace the hostname
with your server's reachable hostname or IPv4 address.

Use the regenerated `types.xml` and matching game/editor binaries. The schema
is defined in [DBMPConsts.cll](../../Versions/Temporary/Engine/Sources/GameX/DBMPConsts.cll).

## Precedence and compatibility

Each field is resolved independently when creating a connection:

| Field | Database value used when | Otherwise | Default without a profile value |
|---|---|---|---|
| `NivalNetIP` | Non-empty | `NivalNetIP` setting | `localhost` |
| `NivalNetPort` | Positive | `NivalNetPort` setting | `4200` |
| `NetGameVersion` | Zero or positive | `NetGameVersion` setting | `1` |

The schema defaults are an empty address, port `0`, and version `-1`. Omitted
fields in old XML or packed binary records therefore keep the previous profile
behavior. Use a real UDP port in the range 1 through 65535. Version `0` is an
explicit protocol identifier; `-1` requests fallback.

Authored database values take precedence over later console/profile changes.
Fallback fields still read the settings. Neither path overwrites the profile.
Settings are resolved again for the next connection, so changing the active mod
does not leave a cached endpoint from the previous mod. An established connection
keeps the values with which it was created.

`NetGameVersion` applies to both online and LAN multiplayer, including LAN driver
recreation. The online address and port do not change LAN discovery, which still
uses UDP port `4201`. The three fields describe connection metadata and are
excluded from gameplay checksums; the existing network handshake checks the
effective protocol identifier separately. Changing the identifier alone does
not make incompatible game builds or mods compatible.

## What is needed to connect

These are the three configurable inputs used to select and connect to the
online lobby server. `NivalNetIP` can be a DNS name or IPv4 address, `NivalNetPort`
selects its UDP endpoint, and `NetGameVersion` identifies the network protocol.
The server must be reachable, and its `server.xml` values `Port` and `NetVersion`
must match the client's effective port and protocol version. Login requires an
account and password; registration also supplies a CD key and email. These
remain user-specific and are not stored in the multiplayer resource.

The standalone `Server` executable continues to use `server.xml`; it does not
load the game's resource database. See the [server configuration example](../ExampleServerConfig/README.md)
for server setup, account requirements, and networking details. Existing
connection timeouts are unchanged.

## Implementation and validation

[MPConnectionSettings.cpp](../../Versions/Temporary/Engine/Sources/GameX/MPConnectionSettings.cpp)
resolves database fields and legacy fallbacks. `MPManagerModeNivalNet` passes the
result into `CServerClient`, including the peer-connection setup it owns.
`MPManagerModeLAN` uses the same protocol resolver at both driver creation sites.

`MPConnectionSettings_test` covers default and profile fallbacks, independent
field overrides, profile changes while database values are selected, source
switching, version zero, old XML/binary data, binary round trips, and checksum
exclusion. These tests resolve settings without contacting an external server.

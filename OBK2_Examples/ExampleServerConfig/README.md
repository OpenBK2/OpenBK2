# Example Server configuration

[server.xml](server.xml) configures the current OpenBK2 `Server` executable with
SQLite, UDP port **4200**, and TCP administration port **4210**. It includes all
settings read by the server and the complete ladder tables from the historical
[NivalNET configuration](../../Versions/Server/nivalnet/server.xml), with comments
explaining their meaning. Time intervals in this XML are in **milliseconds**.

`Server` provides accounts, chat, custom-game listings, ladder matchmaking, and
packet relaying. The players' machines run the match simulation. This executable
does not need game maps, resource packs, a graphics setup, or a selected scenario
to start.

## Files and working directory

Use a server directory arranged like this:

```text
my-server/
    server.xml                 # Required; copy from this example
    Messages/                  # Optional; used by the broadcast command
        hello.xml
        empty.xml
    bin/                       # Start the process with this working directory
        Server                 # Linux, or Server.exe on Windows
        ...                    # Runtime libraries supplied with your build
    nivalnet.db                # Created automatically with this configuration
```

The executable always reads **`server.xml` from the parent of its current working
directory**. It does not locate it relative to the executable, and there is no
command-line option to select another configuration file. Keep the lowercase
filename on Linux. Launching `/path/to/bin/Server` from an unrelated directory
will look in the wrong place.

Copy this example's `server.xml` and, optionally, `Messages` into `my-server/`.
Use the current `Server` build and its required runtime libraries in `bin/`;
the old binaries under `Versions/Server/` predate the SQLite options. An existing
OpenBK2 installation's `bin/` can also be used, with the XML in its parent.
The CMake target is `Server`, enabled by `BUILD_SERVER` (on by default); these
example files are not installed automatically by that target.

On Linux:

```sh
cd /path/to/my-server/bin
./Server
```

On Windows, in PowerShell:

```powershell
Set-Location 'C:\path\to\my-server\bin'
.\Server.exe
```

You can also invoke an existing build by absolute path after changing into the
intended `bin/` directory. IDE launch configurations and services need the same
working-directory setting.

The SQLite database and its journal need write access to `my-server/`.
`DatabaseFile` is passed directly to SQLite: `../nivalnet.db` is relative to
`bin/`, so it places the database beside the XML. SQLite creates the file and
tables, but does not create missing parent directories. No database daemon,
credentials, SQL import, `client.xml`, or precreated user accounts are needed
for this example. The game client can register accounts after startup.

## Connecting game clients

Set these variables in each game's `Profiles/game.cfg`, replacing existing
values where present:

```text
setvar NivalNetIP = 127.0.0.1
setvar NivalNetPort = 4200
setvar NetGameVersion = 3
```

Use the server's reachable IPv4 address instead of `127.0.0.1` for players on
other machines. Enter the game's Nival.NET multiplayer mode. The server XML's
`Port` and `NetVersion` must match `NivalNetPort` and `NetGameVersion` respectively.
Version 3 matches the repository's current `Profiles/game.cfg`; changing this
identifier alone does not make incompatible game builds or mods compatible.

Allow inbound **UDP 4200** to the server and forward it when needed through the
server's router. The lobby socket binds all local IPv4 interfaces. The TCP
administration port is separate and is not needed by players.

`CheckCDKeyIsValid=0` skips the database allowlist check during registration.
It does not skip account/password checks, bans, or the check for an already
online CD key. Use a non-empty, distinct CD key for each player: the current
server treats an empty key as already online. With `CheckCDKeyIsValid=1`, populate
the `validcdkeys` table with the keys you intend to accept before registering
players. A newly created SQLite database has an empty allowlist.

## Configuration reference

Keep the complete `<Base>` document. Several numeric settings have no reliable
fallback if omitted. Unknown XML keys are not a way to configure additional
features. XML element names are case-sensitive; escape text such as `&` as
`&amp;`.

| XML element | Example | Meaning |
| --- | --- | --- |
| `NetVersion` | `3` | Network protocol identifier, matching clients. |
| `Port` | `4200` | UDP lobby and relay listening port. |
| `DatabaseBackend` | `sqlite` | Use `sqlite` or `mysql`. If omitted, defaults to `mysql`. |
| `DatabaseFile` | `../nivalnet.db` | SQLite file path relative to the working directory, or an absolute path. If omitted, defaults to `nivalnet.db` in the working directory. |
| `MySQLServer` | `127.0.0.1` | Database host for the `mysql` backend; ignored for SQLite. |
| `MySQLDBName` | `nivalnet` | Existing database name for the `mysql` backend; ignored for SQLite. |
| `GameTimeout` | `30000` | Remove a lobby game after 30 seconds without a heartbeat. |
| `GameLoadingTimeout` | `300000` | Extra grace period on loading: moves the heartbeat timestamp 5 minutes ahead. Together with `GameTimeout`, gives 330 seconds before expiry if no subsequent heartbeat extends it. |
| `ChatSegmentLength` | `500` | Interval between chat cleanup passes for offline clients. |
| `MaxFriends` | `20` | Configured friend-list limit. The current check rejects additions only when the existing count is greater than this value, so 20 permits a 21st entry. |
| `ServerLogPeriod` | `900000` | Interval between statistics writes to the database, here 15 minutes. This is not a text-log rotation setting. |
| `Welcome` | Welcome message | UTF-8 text sent when a player joins a chat channel; empty text suppresses it. |
| `CheckCDKeyIsValid` | `0` | `1` requires a match in `validcdkeys` at registration; `0` skips that check. |
| `TerminalPort` | `4210` | TCP administration port on all local IPv4 interfaces. There is no XML bind-address or authentication setting. `0` requests an OS-assigned port; it does not disable the console. |
| `LadderConsts` | Complete block | Matchmaking, medals, and experience settings, described in the XML and below. |

The custom and ladder lobbies are both created at startup, even if players only
use custom games. Keep `LadderConsts` in the configuration. Its original balance
values are retained here:

- `LevelDelta=5` limits the difference between average team levels. Initial
  candidate filtering allows twice that difference from the selected player.
- `WaitTime1=20000` and `WaitTime2=40000` relax map/tech-level preferences after
  20 and 40 seconds. They do not relax the level difference.
- `MaxTeamSize=4` controls random team-size selection for players who request
  any size. `PlayersPerStep=5` limits matchmaking attempts per 200 ms tick;
  `MistakesPerStep=10` is a retry multiplier for candidate team assignments.
- Keep `NumberOfRaces=4` and `MaxUnitTypes=30` consistent with clients and stored
  statistics. The server also has a separate hardcoded reinforcement limit of
  30, so changing `MaxUnitTypes` alone does not extend it.
- Keep three entries in each medal array. `KilledMedals` uses lifetime kills;
  `KilledLostMedals` uses kills versus losses in the latest match;
  `WinsInSeriesMedals` uses consecutive wins. `FirstPlaceMedalLevel=20` is the
  minimum level for the highest-XP medal.
- `Experience/Win` and `Experience/Lose` map opponent-minus-own average team
  level differences to base XP amounts. `Lose` stores positive amounts that the
  server subtracts. Both maps cover every integer from `-5` through `5`.
  Extend both ranges if increasing `LevelDelta`; missing keys produce zero XP.
- `NewbieLevel=10` applies `LossFactor` to losses up to and including level 10.
  The factor map covers levels 1 through 10. `Levels` contains 100 ascending,
  cumulative XP thresholds, with the first entry representing level 1.

## Administration and optional messages

**Restrict TCP 4210 to trusted administrators with a firewall or private network.**
The current console accepts unauthenticated commands on all IPv4 interfaces.
Choosing an unprivileged port avoids the historical ports 100/101, but does not
restrict who can connect. There is no configuration switch to disable it.

Connect using a plain TCP terminal, for example `nc 127.0.0.1 4210` on Linux
or a client in raw TCP mode on Windows. Commands are read from this socket,
not from the server process's standard input. Send one short command at a time
and wait for its response; the current reader treats each received buffer as a
command rather than assembling a stream of lines. Avoid blank commands. Broadcast
commands do not return an acknowledgement.

| Command | Effect |
| --- | --- |
| `help` | List available commands. |
| `clients` | List online players. |
| `clients custom` | List players in the custom lobby. |
| `client_state PlayerName` | Show a player's state. |
| `games custom` | List custom games. |
| `statistics` | Show runtime counters and database query load. |
| `kick PlayerName` | Disconnect a player. |
| `reload_config` | Reload the lobby settings described below. |
| `broadcast hello.xml` | Send the included message to online players. |
| `broadcast empty.xml` | Clear the welcome text held in memory. |

`broadcast hello.xml` reads `../Messages/hello.xml` relative to the working
directory. The included [hello.xml](Messages/hello.xml) illustrates the required
`<Base><Text>...</Text></Base>` format. Messages are optional and not read at
startup. Keep `Messages` capitalized on Linux. ASCII message text, as supplied
here, avoids relying on the legacy broadcast conversion's character encoding.

A broadcast also replaces the welcome text in memory for subsequent chat joins.
The [empty.xml](Messages/empty.xml) example clears that text; neither command
edits `server.xml`. `reload_config` restores the XML's `Welcome` value.

`reload_config` rereads `GameTimeout`, `GameLoadingTimeout`, `ChatSegmentLength`,
`MaxFriends`, `Welcome`, `CheckCDKeyIsValid`, and `LadderConsts`. Restart the
process to change ports, protocol version, database settings, or
`ServerLogPeriod`. Preserve the ladder structure when reloading an active server.
Stop a foreground server with **Ctrl+C**; Linux also supports **SIGTERM**.

## Database overrides and the MySQL alternative

The supported command-line options override the corresponding XML values:

```text
Server --sqlite
Server --database sqlite --database-file ../another-server.db
Server --database mysql
```

Use `./Server` or `.\Server.exe` as appropriate for your shell. `--sqlite` is
shorthand for `--database sqlite`. `--database-file` only changes the path; it
does not select SQLite. These flags still require `server.xml` and the correct
working directory. Each database file has its own accounts and statistics;
switching backends or paths does not migrate existing data.

For MariaDB/MySQL, set `DatabaseBackend` to `mysql`, supply `MySQLServer` and
`MySQLDBName`, and prepare a compatible schema and account before startup.
The source fixes the database username to **`NivalNET`**, leaves the password
empty, and uses the database client library's default port. There are no
`MySQLUser`, `MySQLPassword`, or `MySQLPort` XML settings. The account needs
permissions for the server's reads, writes, and runtime statistics-column
additions (`ALTER TABLE`).

The historical [dbstruct.sql](../../Versions/Server/nivalnet/dbstruct.sql) is a
schema reference, not a ready-to-run modern initialization script: its opening
database statement is malformed, foreign-key creation order needs attention,
and table/database casing and legacy defaults need to match the chosen database
server. Only SQLite creates its base schema automatically. This example uses
SQLite so none of those manual setup steps are necessary.

## Checking startup

Normal startup prints `Database connection established.`, the selected database
path, `Server started, port 4200, gameversion 3`, and `Ladder lobby configured.`
to standard output. Check that no database or socket errors precede the final
`Server running` message: that message alone does not establish a healthy server.

- `Cannot open the configuration file`: check the reported path, working
  directory, filename casing, and read permissions.
- `Database connection FAILED`: for SQLite, check the database path and parent
  directory permissions. Stop the process, correct the problem, and restart;
  the server continues after the initial failure and can later stall in its
  database health check.
- `Cannot bind socket` or `Terminal: cannot bind port`: check for port conflicts
  and use unprivileged ports. Update clients if the UDP port changes.
- Login failure: check the address, UDP access, matching protocol version,
  account registration, and non-empty/distinct CD keys.

`ServerLogPeriod` writes to the database. Additional per-player packet logging
depends on build settings; no pre-existing `Logs` directory is required. Capture
stdout/stderr with your shell or service manager when you need a startup log.

Validation: the annotated XML was loaded by an existing Linux `Server` build
using a temporary database and unused ports. Startup, SQLite schema creation,
statistics writes, console commands, configuration reload, both broadcast files,
and SIGTERM shutdown passed. XML keys were checked against the source readers,
and all historical ladder values were preserved. This did not test Windows,
MariaDB/MySQL, or a multiplayer match.

## Source references

These examples follow the current source, including the console entry point and
SQLite backend:

- [main.cpp](../../Versions/Temporary/Engine/Sources/Server/main.cpp): working directory, CLI options, lobbies, shutdown.
- [Server.cpp](../../Versions/Temporary/Engine/Sources/Server/Server.cpp): network/database settings, reload scope, broadcasts.
- [DatabaseSqlite.cpp](../../Versions/Temporary/Engine/Sources/Server/DatabaseSqlite.cpp) and [SqliteSchema.inc](../../Versions/Temporary/Engine/Sources/Server/SqliteSchema.inc): automatic database creation.
- [ControlLobby.cpp](../../Versions/Temporary/Engine/Sources/Server/ControlLobby.cpp) and [clients.cpp](../../Versions/Temporary/Engine/Sources/Server/clients.cpp): registration, login, and CD-key checks.
- [Chat.cpp](../../Versions/Temporary/Engine/Sources/Server/Chat.cpp), [GameLobby.cpp](../../Versions/Temporary/Engine/Sources/Server/GameLobby.cpp), and [LadderLobby.cpp](../../Versions/Temporary/Engine/Sources/Server/LadderLobby.cpp): lobby settings and ladder tables.
- [Terminal.cpp](../../Versions/Temporary/Engine/Sources/Server/Terminal.cpp) and [Commands.cpp](../../Versions/Temporary/Engine/Sources/Server_Client_Common/Commands.cpp): administration socket and command syntax.
- [MPManagerModeNivalNet.cpp](../../Versions/Temporary/Engine/Sources/GameX/MPManagerModeNivalNet.cpp) and [game.cfg](../../Versions/Current/Profiles/game.cfg): game-client connection variables.

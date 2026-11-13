# COOP custom multiplayer maps

COOP runs two or more human players on diplomacy side 0 against map-configured
AI_BOT players on side 1. The last map player is neutral (side 2). Existing maps
default to CLASSIC. Each COOP bot has a strategic AI General using that player's
existing `general` configuration. Mission Lua can still direct separate scripted
groups. See the [AI General guide](../CoopAIGeneral/README.md) for setup and checks.

These files are authoring examples, not a complete playable map or a replacement
for your mod's database. Start with a working multiplayer map and its terrain,
then merge the fields described below in the editor. Rebuild/repackage data with
the regenerated `types.xml` from this source revision.

## Map descriptor

`MultiplayerMap.xdb` shows the mode and map settings. Adjust its map/name
references and dimensions to your mission:

| Field | Default | Meaning |
| --- | --- | --- |
| `GameType` | `CLASSIC` | Set to `COOP` for cooperative rules. |
| `ForcedTechLevel` | `-1` | A zero-based MultiplayerConsts tech index; -1 permits the host's selection. |
| `AllowedNations` | Empty | Zero-based MultiplayerConsts side indexes for human players. Empty permits every side available at the selected tech. |
| `SpawnStartingUnits` | `true` | Spawn the automatic nation/tech-level starting army. Set to `false` to skip it in either CLASSIC or COOP. |
| `AddReinfsWithTimer` | `true` | Periodically grant extra human reinforcement calls. Set to `false` to disable these grants and hide the HUD countdown in either mode. |
| `DisableReinforcementOverride` | `false` | Keep the map's exact reinforcement recipes instead of substituting the selected nation/tech catalogue. |
| `DisableNationSelection` | `false` | Hide nation selection and use each human slot's map-authored `PartyInfo`. Existing nation and tech restrictions still apply. |

`Players` remains the maximum number of **humans**, not the total number of map
players. At least two humans must be connected and ready to start. If the host
selects a smaller room, only the first that many HUMAN positions are available.
Empty human positions remain absent; they do not turn into bots.

For a forced nation, put its index alone in `AllowedNations`. Multiple entries
restrict the choice, including Random. An invalid index or a list with no nation
available at the chosen tech is a data error and prevents starting. Forced tech
is displayed but cannot be changed. COOP hides team selection and disables
random-placement and match time-limit controls; all humans join side 0.

The [map override guide](../MultiplayerMapOverrides/README.md) includes an example
for keeping authored reinforcement recipes and locking nations per human slot.

Difficulty is selected by the host in the multiplayer game settings. Populate
`MapInfo.CustomDifficultyLevels` with `DifficultyLevel` records in the desired
dropdown order; their `LocalizedNameFileRef` supplies each label. Lua
`GetDifficultyLevel()` returns the selected zero-based index, which is synchronized
to every client and saved in replays. The selector is hidden for CLASSIC maps or an
empty list, using index `0`. These records
do not apply automatic unit-stat or reinforcement modifiers in MP; scripts decide
what each difficulty does. The old `MultiplayerMap.DifficultyLevel` property was
removed. See the [difficulty guide](../MultiplayerDifficulty/README.md) for the GUI
package and examples.

## Map players and armies

Each `MapInfo.Players.Item` gains `PlayerType`, defaulting to `HUMAN`. Set bot
entries to `AI_BOT`. `MapInfo.players.fragment.xml` illustrates an interleaved
layout with humans at IDs 0 and 2, an AI at ID 1, and neutral at ID 3. Merge the
role/side fields into your existing player entries; retain their party data,
deployment locations, cameras and reinforcement definitions.

Map IDs stay fixed throughout lobby setup, scripts and replays. Bot/neutral
entries never appear as joinable lobby positions. The current protocol supports
at most 16 map player entries, including neutral.

Configure AI `PartyInfo`, `ReinforcementTypes`, `ScriptReinforcements`, spawn
points, call counts and recycle coefficients on the map. Human nation and tech
restrictions do not replace these bot settings. Place bot units in the editor or
spawn them with the existing reinforcement script functions. Bots do not receive
the automatic human MP starting army. Humans receive normal MP starting armies
unless `SpawnStartingUnits` is `false`. Map-placed and script-created units, base
capture and reinforcement credits are unaffected. See the
[starting units guide](../MultiplayerStartingUnits/README.md) for an example.

`AddReinfsWithTimer` independently controls periodic extra human reinforcement
calls. Disabling it leaves initial credits, explicit grants and normal cooldowns
intact. See the [timed reinforcement guide](../MultiplayerReinforcementTimer/README.md).

Set each bot's `LocalizedPlayerNameFileRef` to its general/player name text file.
The match results list each bot on the winning or losing team, including its
kills, losses and scores. `AIGeneralName.txt` is an example name; multiple bots can
use different files. The neutral player is excluded from the results.

Set each bot's `MPStartPos` to the position where its marker should appear on the
loading-screen minimap. Bots show their localized names on the red team, with red
player markers; humans stay on the green team with their chosen individual colors.
The example bot position is illustrative; adjust it to the mission's layout.

New COOP replays capture bot names and `MPStartPos` values at match start. The
replay preview shows those saved names and positions even if their source data
is later edited. Older COOP recordings use the current map data as a fallback.
The replay preview lists individual players. Both humans and AI bots can be
selected as viewpoints, with the first human selected initially. Bot entries stay
outside the human network roster, and the replay list's player count still counts
humans. A bot viewpoint retains its map-configured AI role.
Replay bot nation icons use that player's current map `PartyInfo`, while names
and marker positions continue to come from the replay snapshot.

## Mission script

Received objectives use their existing `MapPositions` to display arrows in the
3D scene and on the minimap, as in single player. Completing, failing or removing
an objective updates its markers and the normal objectives list. Replay script
feedback restores the same presentation; this does not award campaign XP or
change the map's scripted win/loss rules. No additional GUI data or `.pak` is needed.

Point `MapInfo.ScriptFileRef` to your mission script. `mission.lua` demonstrates
an AI attack, an appended second order, and script-controlled victory/defeat.
Before using it, create the script groups and areas named in its comments.

- `Win(0)` wins for every human; `Win(1)` loses for every human.
- `Loose()` also means a COOP human defeat. The first result wins; later calls do
  not replace it.
- `Draw()` is unsupported in COOP and reports a script error; use one of the two
  team outcomes above.
- Base losses, unit elimination, CLASSIC score/time limits, and having no enemy
  human peers do not automatically decide COOP results.
- A map without a script, or whose script never calls a result function, keeps
  running until players leave. Disconnect/network cancellation remains possible.
- `GetDifficultyLevel()` returns the host-selected index. `GetMultiplayerMatchTechLevel()`
  and `GetPlayerMultiplayerNation(player)` expose the human MP configuration.
- `IsPlayerPresent(player)` lets scripts handle unoccupied or departed humans.
  On disconnect, the existing rule stops and transfers that player's units to a
  remaining ally, or neutral if no ally remains.

COOP victory and defeat use the single-player camera orbit, outcome music and
fade before showing multiplayer statistics. World sounds and particle effects
pause during this sequence; the camera, music and fade continue.
Surrender also shows the defeat sequence and results for the departing player,
without declaring a team outcome for the humans and AI still playing. That
player's replay remains unfinished unless the script already declared a result.
`FinalPositions[0]` is the victory
camera anchor; `FinalPositions[1]`, when present, is the defeat anchor. Without
these entries the camera orbits its current position. The usual key press skips
the animation. Set `MusicWin` and `MusicLost` as for a single-player mission.

`Cmd`, `QCmd`, `UnitCmd`, and `UnitQCmd` retain their existing arguments. In COOP
they snapshot stable unit IDs and enqueue derived simulation orders. The queue
drains after the mission's Lua threads for that tick. `QCmd`/`UnitQCmd` append to
the units' existing order queues. Commands generated by command callbacks during
the drain execute on the next simulation tick. Dead/missing targets are skipped.
Use simulation waits before querying the effects of an order issued in the same
Lua invocation.

Gameplay must originate from simulation threads. Local console/camera callback
script calls are discarded in COOP, so a skipped movie or different frame rate
cannot issue gameplay orders or consume gameplay RNG. Put gameplay associated
with a cinematic in an independently timed simulation thread. Lua table storage
now iterates deterministically, but scripts must still avoid local settings or
presentation state when making gameplay decisions.

The mission and configured common Lua source files are hashed for COOP joining
and replay compatibility. Distribute identical files to all players and retain
them for replay playback. Script-derived orders are regenerated by the replay;
they are not also recorded as duplicate human commands.
Replay completion also checks the recorded script outcome, including an unfinished
match. Lua table traversal is now deterministic; scripts must not depend on the
old unspecified table order, and older scripted recordings may verify differently.

## UI data

The implementation reuses existing multiplayer controls and displays CLASSIC or
COOP in map/game lists, so no additional required button or window records are
needed. [MessageParts](../MessageParts/README.md) supplies the current localized
mode names, lobby errors, player-name prefixes and timer labels through
`UIConstsB2.MessageParts`. Its English `.pak` contains game-ready UTF-16LE text.
`UIConsts.fragment.xml` illustrates the older `MPLocalizedGameTypes` field;
the current mode labels use `<val MP_GAME_TYPE_CLASSIC>` and `<val MP_GAME_TYPE_COOP>`.

COOP's lobby time limit displays `--:--`. Its minimum-player warning uses the
normal game text style. The message catalogue takes priority; existing screen
translations remain backups. For older data, merge `MPGameRoom.relatedTexts.fragment.xml` into the existing
`MPGameRoom_WindowScreen.xdb` `RelatedTexts` list and resolve its `TextFileRef`
relative to that screen. The text files beside this older fragment are UTF-8
examples and need conversion to UTF-16LE with a BOM for the game. An English
fallback works without this optional entry.
A disabled reinforcement-credit timer displays `never` instead of a countdown.

## Suggested mission check

1. Join with two clients and confirm only HUMAN positions are available, with
   fixed team and the expected tech/nation restrictions.
2. Confirm authored AI units and reinforcements appear at their original IDs.
3. Run `mission.lua` with the required groups/areas and check both outcomes on
   both clients. Confirm a scriptless copy does not end from CLASSIC rules.
4. Repeat after one human disconnects and verify the remaining human can continue.
5. Replay from each recorded human viewpoint with checksum verification enabled.
6. Check victory and defeat camera sequences, including skipping them, and verify
   both human and named AI rows in the results.
7. Give an objective with `MapPositions`, check its arrows on both maps, then
   complete or remove it and confirm the arrows disappear during normal play.

The example does not create terrain, unit records, General parcels or
campaign-style objectives.

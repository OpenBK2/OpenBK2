# Replay player selection

The replay preview now uses one row per recorded player. Selecting a row chooses
that player's actual map ownership ID, including sparse IDs and COOP AI bots.
Single-player recordings retain player 0 as their only viewpoint.

The existing ReplaySaveLoad GUI resources are reused, so no GUI patch or `.pak`
is required:

- `ItemHeaderWin` displays the player's name in its existing header style.
  `ItemHeaderLose` is hidden, or used as a fallback if the first header is absent.
- `ItemPlayers` displays `Winner`, `Loser`, or `Unfinished` in its normal style.
- `ItemFlag0` and `ItemFlag1` retain the green and red team artwork.
- The existing player-list scrollbar handles the additional rows.

The first valid human is selected initially. COOP bots can also be selected, using
their recorded names and IDs. Older COOP recordings use map data for bot names.
Rows with unavailable or invalid map ownership data remain visible but cannot be
used to start playback. Selecting a bot does not change its AI role or add it to
the human network roster.

COOP recordings without a scripted result show `Unfinished`. CLASSIC recordings
with no recorded winning side also show `Unfinished`.

For localization, use these names in [MessageParts](../MessageParts/README.md).
The existing optional `RelatedTexts` entries on the ReplaySaveLoad screen remain
backups when neither MessageParts nor general `<val>` values supplies a
translation. Existing data works without them, using the English text shown
below. The row's font and alignment still come from its existing GUI data.

| Entry | Default text |
| --- | --- |
| `REPLAY_PLAYER_WINNER` | Winner |
| `REPLAY_PLAYER_LOSER` | Loser |
| `REPLAY_PLAYER_UNFINISHED` | Unfinished |

To check the change in game, select the second or third player, switch to another
recording, and repeat. Each selection should start playback from that exact
player's view. Also check a COOP AI player, an unfinished recording, and a replay
whose filename contains spaces.

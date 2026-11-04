# Multiplayer difficulty selector

`MultiplayerDifficulty.pak` adds a Difficulty dropdown to the host's Create Custom
Game settings. It uses the campaign selector's fonts and skin. The existing game
speed control occupies the left half of the final settings row; difficulty occupies
the right half. Compact row spacing fits both controls inside the original 220-unit
settings frame, retaining the normal fonts. The closed difficulty selector is 28
units high, with a scaled button skin and centered text. The popup has space for
four rows and includes a compact scrollbar
for longer map-authored lists. Shorter lists leave unused space; their scroll arrows
are disabled. Long labels are clipped to the normal single-row text area.

Install the package in the game's `Data` directory with the matching OpenBK2
executable. The package overrides `MPCreateCustomGame` layout records and adds
its own resources under `/OpenBK2/MultiplayerDifficulty/`. It does not replace
campaign records, multiplayer constants, or maps. If a mod also replaces these menu
records, merge the changes into that mod's active UI data. The game selects duplicate
archive paths by each entry's timestamp; this revision uses a fixed November 4, 2026
12:00 timestamp to supersede stock data and the initial package. Replace the old
`MultiplayerDifficulty.pak` with this one. Newer mod entries take precedence and require
merging these changes. Without the updated menu,
older UI data remains usable but the host's difficulty is fixed to index 0.

The dropdown and its label appear only for COOP maps with a nonempty
`MapInfo.CustomDifficultyLevels` list. The dropdown reads that list in exactly
the authored order. Its selected zero-based index is sent to all players and exposed
to the map script by `GetDifficultyLevel()`. There is no campaign-style reordering.
The initial index is 0. Changing maps keeps the index if the new list contains a valid
record there; otherwise it resets to 0. CLASSIC maps and maps with an empty list
hide both controls and use index 0. The difficulty records' single-player stat and
reinforcement modifiers are not applied automatically in multiplayer; the mission script decides
what each index means.

For a usable three-level example, merge
`MapInfo.CustomDifficultyLevels.fragment.xml` into a map's `MapInfo.xdb`.
The package supplies these records and their English names:

| Index | Label | Record |
| --- | --- | --- |
| 0 | Easy | `/OpenBK2/MultiplayerDifficulty/Levels/Easy.xdb` |
| 1 | Normal | `/OpenBK2/MultiplayerDifficulty/Levels/Normal.xdb` |
| 2 | Hard | `/OpenBK2/MultiplayerDifficulty/Levels/Hard.xdb` |

All participating clients need the same map difficulty configuration and records.
This example fragment is not a complete map and is deliberately not installed into
an existing map by the package.

`Data/` contains the loose package sources. Edit `strings.json` for the menu label,
tooltip, and sample difficulty names, then run `python build.py`. The script emits
UTF-16LE game text and rebuilds the archive with fixed metadata; it writes only to
this example directory. Archive bytes are reproducible with the same Python/zlib
versions; different compression versions may produce different archive bytes from
identical resources. Existing maps can instead reference their own localized
`DifficultyLevel.LocalizedNameFileRef` files.

For custom menu integrations, the interface recursively looks for an `IComboBox`
named `Difficulty`. Each combo item must contain an `ITextView` named `ItemComboText`
for the MP text viewer. Name its caption `DifficultyLabel` so the code hides and
restores it together with the dropdown. The supplied local campaign-style templates
provide those names and preserve the normal game text formatting.

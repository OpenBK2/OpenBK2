# Multiplayer map reinforcement and nation rules

Merge `MultiplayerMap.fragment.xml` into an existing `MultiplayerMap.xdb`.
Both options default to `false` and apply to CLASSIC and COOP independently.
Use the matching game/editor build and regenerated `types.xml`.

## Keep authored reinforcement recipes

Set `DisableReinforcementOverride` to `true` to use each map player's exact
`MapInfo.Players.Item.ReinforcementTypes` recipes. The selected nation and tech
level no longer replace these recipes from `MultiplayerConsts`. This also works
when that nation's reinforcement catalogue is empty. COOP bots already use their
authored recipes regardless of this option.

The option does not change call credits, cooldowns, entry points, or automatic
starting armies. Set `SpawnStartingUnits` to `false` separately if the mission
provides its own initial units. With nation selection enabled, changing a nation
can still change its flag and starting army while the authored reinforcement
recipes remain fixed.

The reinforcement policy is saved with the match and snapshotted in replays.
Older saves and recordings default to the original replacement policy.

## Use each slot's authored nation

Set `DisableNationSelection` to `true` to hide the nation dropdown and use each
human slot's `MapInfo.Players.Item.PartyInfo`. Each party must match a side in
`MultiplayerConsts`, be available at the selected tech level, and satisfy
`AllowedNations` if that list is populated. Missing or conflicting data prevents
starting the match instead of silently selecting a different nation.

Different human slots can have different authored nations. Random placement in
CLASSIC still works: the nation follows the destination map slot after shuffling.
The finalized nation is retained in the replay roster. Tech-level selection is
unchanged; use `ForcedTechLevel` to lock it.

No GUI `.pak` is needed; the existing lobby controls are hidden by game code.
Distribute the same map data and runtime build to every multiplayer participant.

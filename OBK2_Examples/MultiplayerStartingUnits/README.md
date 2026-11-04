# Multiplayer starting units

`SMultiplayerMap.SpawnStartingUnits` is a boolean that defaults to `true`.
It applies to both CLASSIC and COOP matches.

To disable the automatic starting army, merge this field into your map's
`MultiplayerMap.xdb`, keeping all its other settings:

```xml
<SpawnStartingUnits>false</SpawnStartingUnits>
```

`MultiplayerMap.fragment.xml` contains the same example. It is a fragment for
an existing map descriptor, not a complete playable map or a global override.
Set the field to `true`, or omit it, to retain normal deployment.

The flag controls the starting reinforcement configured by the human player's
nation and selected tech level in `MultiplayerConsts`. It does not remove units
placed in the editor or prevent Lua from spawning units. Regular reinforcement
calls still work, including activation of the player's initial reinforcement
point. COOP bots retain their map-configured armies.

Use the rebuilt game/editor and regenerated `Versions/Current/Data/types.xml`
when editing or packaging maps with this field. Install the matching binaries
too: replacing `types.xml` alone does not update the game's behavior.
No menu changes or GUI `.pak`
are needed. Every participant must use the same map setting; the multiplayer
map checksum includes it.

New replays store the starting-unit setting used by the match and restore it
before deployment. Older maps, saved multiplayer trackers and recordings that
predate the field retain the default enabled behavior.

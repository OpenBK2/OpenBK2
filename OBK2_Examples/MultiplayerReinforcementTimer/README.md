# Multiplayer timed reinforcement calls

`SMultiplayerMap.AddReinfsWithTimer` is a boolean that defaults to `true`.
It applies to both CLASSIC and COOP matches.

Merge this field into your map's `MultiplayerMap.xdb` to stop periodically
granting extra reinforcement calls to human players:

```xml
<AddReinfsWithTimer>false</AddReinfsWithTimer>
```

`MultiplayerMap.fragment.xml` contains the same example. Set the field to `true`,
or omit it, to keep the existing timed grants. This setting is independent of
`SpawnStartingUnits`.

When disabled, the mission HUD hides both "Next reinforcement in" and its timer.
Initial reinforcement credits, explicit script grants, capture bonuses and
normal reinforcement cooldowns are unaffected. COOP AI bots continue to use
their map-configured reinforcement rules.

The setting is included in the multiplayer map checksum and saved in replays.
Playback restores the recorded value before initializing the mission. Older
maps, saves and replays default to enabled.

Install the rebuilt game/editor binaries and regenerated
`Versions/Current/Data/types.xml` together. Updating the schema alone does not
update the runtime behavior. Every player needs the matching game build and
map data. No GUI changes or additional `.pak` are required.

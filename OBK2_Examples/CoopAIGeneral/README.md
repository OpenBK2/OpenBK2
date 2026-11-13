# COOP AI General

Every present, non-neutral `AI_BOT` map player in COOP gets its own strategic
General. Its existing `MapInfo.players.Item.general` config supplies the mobile
groups, defensive parcels and reserve positions. No new database fields or GUI
package are required. CLASSIC does not run strategic Generals; single player
retains one General for the enemy side.

## Configure a bot

1. Keep the map player's `PlayerType` as `AI_BOT` and `DiplomacySide` as `1`.
   The final player entry remains neutral. Player IDs can be interleaved with
   humans: a bot at player 3 uses player 3's armies and reinforcement pool.
2. Merge `general.fragment.xml` into that bot's player entry. Replace its example
   coordinates with positions inside your map. Place a group of bot vehicles or
   infantry with script ID `100`, or change `mobileScriptIDs` to your own group IDs.
3. Configure `EPATCH_DEFENCE` parcels for positions to protect and
   `EPATCH_REINFORCE` parcels for reserve assembly positions. `DefenceDirection`
   uses the game's 0..65535 direction units. Reinforcement-point coordinates are
   offsets relative to the parcel, rotated by its direction.
4. For automatic reinforcements, configure that same player's `PartyInfo`,
   `ReinforcementTypes`, `ReinforcementPoints`, `ReinforcementCalls`, and
   `RecycleTimeCoefficient`. Available ground forces, aircraft, artillery and
   supply vehicles use the existing single-player General behavior.

`MaxMobileTanks` is the existing General limit used when deciding whether to call
more ground reinforcements. It is not an absolute limit on all map or scripted
units. The General uses the bot's own calls and cooldowns, independently of human
MP timed credit grants and `SpawnStartingUnits`.

By default, human reinforcement types come from the selected nation and tech level in
`MultiplayerConsts`: the map's `ReinforcementTypes` list selects which categories
are available. Those categories must also exist in that nation/tech catalogue.
An empty catalogue leaves the human player with no types, even with call credits
and a default reinforcement point. A default point does not require an owned base.
Set `MultiplayerMap.DisableReinforcementOverride` to `true` to keep the human
slots' exact authored recipes instead. See the
[map override guide](../MultiplayerMapOverrides/README.md) for both map options.

`MapInfo.Weather.WeatherPeriod` controls automatic bad weather independently of
the `Visuals` reference. Set the period to `0` for a mission without automatic bad
weather; an empty `Visuals` reference alone does not disable it. The General waits
for flyable weather before ordering ordinary aircraft. Aircraft with an unlocked
Master Pilot ability retain their all-weather exception.

Keep groups controlled by repeated Lua `Cmd`/`QCmd` orders out of
`mobileScriptIDs`; otherwise the script and General can both issue orders to
them. Ungrouped units arriving through normal reinforcement calls are handed to
the General automatically. Map-placed units need their script group listed.
An empty mobile-group list does not disable automatic reinforcement decisions:
for a purely scripted bot, leave its regular reinforcement pool/calls unavailable
and use separately grouped scripted reinforcements.

## Synchronization and replays

Generals run on every peer from synchronized simulation state. Ownership is per
bot, while fog and enemy sightings are shared by diplomacy side. Selecting an AI
replay viewpoint does not change its decisions. Pending air requests, threat
contributions and incremental scan positions are saved; General state is included
in both normal and light COOP checksums.

COOP ignores machine-local `nogeneral`, `temp.nogeneral_sript`,
`temp.general_reinforcement` and `AI.General.*` console overrides, using the
General's built-in defaults. Single-player console tuning remains available.
The host's difficulty index remains available to mission Lua; this change does
not add automatic difficulty scaling.

All peers need the updated build and identical map data. Older COOP recordings
were made with strategic Generals disabled and can diverge under this build;
keep the old executable/runtime if you need to play those recordings faithfully.

## Mission checks

- Start with two humans and at least two bots at different player IDs on side 1.
  Verify each bot uses its own groups, parcels, reinforcement stock and entry points.
- Reveal human units near a defended parcel, then move them to another area.
  Check reserve responses, artillery and configured aircraft on both clients.
- Test supply trucks and capture an AI gun or transfer units between bots; the
  former owner must stop issuing orders to those units.
- Record the match and replay it from both human and AI viewpoints with checksum
  verification enabled. Check scripted victory/defeat still controls the result.

The fragment is an authoring example, not a complete playable map. It does not
replace your installed mission or create the referenced script groups.

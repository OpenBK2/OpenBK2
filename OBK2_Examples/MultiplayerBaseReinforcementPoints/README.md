# Multiplayer base reinforcement points

Merge the fields in `MultiplayerMap.fragment.xml` into your map's
`MultiplayerMap.xdb` to grant three reinforcement points on capturing a base
and remove two on losing one.

- `OnBaseCaptureReinfPointsGain`: integer, defaults to `1`.
- `OnBaseLostReinfPointsLoss`: integer, defaults to `-1`.

Both values are signed changes to each eligible teammate's reinforcement
credits. A negative value removes points, a positive value adds points, and
`0` leaves credits unchanged. Losses cannot reduce credits below zero;
large gains saturate at the maximum integer credit count.

The settings apply to CLASSIC and COOP under the existing base bonus rules.
Neutral players and map-controlled AI bots receive no credit changes.
Initial map placement and transfers within the same team grant nothing.
The existing `multiplayer_flag_calls_bonus` switch still controls these bonuses.

Base entry points and reinforcement timers keep their existing behavior,
including the capture timer reset when the credit change is zero. These
settings are independent of `SpawnStartingUnits` and `AddReinfsWithTimer`.

Missing fields in older XML or packed binary maps use `1` and `-1`.
Both settings participate in the multiplayer map checksum. Saves and replays
store the active values; older saves and replays use the original rules.

Install matching rebuilt game/editor binaries and the regenerated
`Versions/Current/Data/types.xml` together. Every player needs matching
game code and map data. No GUI changes or additional `.pak` are required.

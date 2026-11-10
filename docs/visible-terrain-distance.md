# Distance to a side's visible terrain

`CGlobalWarFog::GetNormalizedDistanceToVisibleTerrain` measures how far a unit or
position is from terrain currently visible to a specified combat side. Both
overloads are exported from AILogic through `CGlobalWarFog` in
[`GlobalWarFog.h`](../Versions/Temporary/Engine/Sources/AILogic/GlobalWarFog.h).

```cpp
#include "AILogic/CommonUnit.h"
#include "AILogic/GlobalWarFog.h"
#include "Stats_B2_M1/Vis2AI.h"

float GetEnemyDistanceFromOurVision( const CCommonUnit &enemy, int nOurParty )
{
	// Limits use AI coordinates: 32 AI coordinates equal one gameplay meter.
	const float fDistanceLimit = 100.0f * AI_TILE_SIZE;
	return theWarFog.GetNormalizedDistanceToVisibleTerrain( enemy, nOurParty, fDistanceLimit );
}
```

The position overload accepts a `const CVec2 &` in the same AI coordinate system,
for example `theWarFog.GetNormalizedDistanceToVisibleTerrain( vPosition, nOurParty,
fDistanceLimit )`. A unit query uses `GetCenterPlain()` for its XY position.

## Meaning of the result

- `0.0f`: the unit is already visible to the requested side, or its position has
  zero distance to that side's visible terrain.
- Between `0.0f` and `1.0f`: the nearest visible terrain is `result * distanceLimit`
  AI coordinates away.
- `1.0f`: no visible terrain is closer than the limit, including when that side
  sees no terrain at all.

The calculation is `min( distance / distanceLimit, 1 )`. With a 100-meter limit,
25 meters gives `0.25`. Larger values mean farther from visibility coverage;
`1 - result` gives a proximity score where larger means nearer.

Camouflage does not change terrain visibility for this query. A hidden,
camouflaged unit standing on visible terrain returns zero. The unit overload
first checks `unit.IsVisible( nParty )`, preserving visibility exceptions such as
already-visible aircraft even when the terrain beneath them is hidden. The
position overload has no unit-specific visibility shortcut.

Pass the **observing diplomacy party**, `0` or `1`. For a map player slot, obtain
the party with `theDipl.GetNParty( nPlayer )`. To measure enemy units relative to
your side's vision, pass those enemy units and your side's party. The caller
selects which units to query; the function does not filter ownership.

## Geometry and visibility rules

The distance is straight-line Euclidean distance in XY from the exact input
position to the nearest point in any visible cell, including its boundary.
Ordinary fog cells are 64 AI coordinates wide, or two gameplay meters. Measuring
to cell edges preserves subcell movement without rounding positions to centers.
A point exactly on a visible cell's boundary has distance zero even if its own
containing cell is hidden. Height and traversability do not enter this distance.

The search uses current `IsTileVisible` semantics. Terrain occlusion, sight
sectors and observers' sight ranges have already been applied to the fog grid.
Single-player script reveals are sampled at their finer 32-coordinate AI tile
resolution, so revealing half of a normal fog cell does not reveal the whole
cell for distance purposes. Existing fog-off behavior is honored. Script
reveals retain the existing single-player/local-party scope and do not affect
multiplayer results.

The function reads the simulation's current fog and cached unit visibility;
it does not force an observer update or refresh cached unit visibility. Queries
therefore inherit the existing update timing. Use an explicit party and a
consistent simulation phase for deterministic gameplay decisions. No state,
RNG, minimap data, save fields or query cache is changed.

## Limits, invalid inputs and cost

The limit must be positive and finite. Invalid limits and party values other
than `0` or `1` return `1.0f`, even for an already-visible unit. Nonfinite
positions return `1.0f` in the position query. An unavailable/cleared fog grid
also returns `1.0f`; the unit overload can still return zero from its cached
visibility shortcut.

Finite positions outside the map can measure distance to visible terrain inside
the map. Search bounds are clipped before converting to integer indices. Double
intermediates avoid overflow when squaring large finite float inputs.

Lower limits reduce the maximum search work. For ordinary fog, a query visits at
most roughly `(2 * distanceLimit / 64 + 2)^2` candidate cells, capped by the map
size. Halving a sufficiently large limit therefore cuts worst-case candidate
work to about one quarter. When single-player script reveals are active, the
cell width is 32 instead. This is an operation bound, not a timing benchmark.

Already-visible units and positions return immediately. The bounded scan skips
rows and visibility checks that cannot improve the best distance found, uses
squared distances until one final square root, and allocates no search storage.
Small limits and occasional queries are the intended use. Many units querying
large limits every tick may justify a shared distance field after profiling.

## Tests

The movement-audio consumer is documented in
[`OBK2_Examples/FOWSound`](../OBK2_Examples/FOWSound/README.md). Its authored limit
is in meters and is converted to AI coordinates; the query itself still uses AI
coordinates. Client visibility notifications refresh the audio gain without
adding observer-dependent state to the simulation.

`VisibleTerrainDistance_test` covers exact axial/diagonal distances, fractional
positions, saturation, camouflage, unit visibility overrides, party isolation,
multiplayer viewpoint independence, observer movement/transfer/removal, map
edges, invalid/large inputs, and partial script reveals. A separate exhaustive
one-meter-tile reference checks bounded search results across both sides and
multiple limits.

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
RNG, minimap data or saved state is changed by a query.

## Upper fog tiles

Each combat side has a hierarchy of flat arrays summarizing the ordinary fog
grid. Base blocks cover **8 x 8 fog tiles**. Each higher block covers four lower
blocks, doubling the width: 16, 32, 64, 128, and so on. Construction stops when
the top level has at most two blocks along each map axis.

One editor terrain patch contains 16 x 16 fog tiles (32 x 32 AI tiles). Thus a
1 x 1 patch map has exactly 2 x 2 base blocks, each covering 8 x 8 fog tiles.

| Square map size in terrain patches | Real fog tiles per side | Top block width in fog tiles | Top grid |
|---|---:|---:|---|
| 1 x 1 | 16 | 8 | 2 x 2 |
| 2 x 2 | 32 | 16 | 2 x 2 |
| 4 x 4 | 64 | 32 | 2 x 2 |
| 8 x 8 | 128 | 64 | 2 x 2 |
| 16 x 16 | 256 | 128 | 2 x 2 |
| 32 x 32 | 512 | 256 | 2 x 2 |

The current editor creation limit is 32 x 32 patches. Hierarchy dimensions are
derived from the loaded map rather than hard-capped at that limit. Rectangular
maps and partially occupied edge blocks use their actual extents.

Each block has three derived states: `Invisible`, `PartiallyVisible`, and
`Visible`. `Visible` means every real tile covered by the block is visible.
Base blocks count visible real tiles. Higher blocks count nonempty children and
fully visible children. Each block also records its actual child count, so
off-map padding does not prevent edge blocks from becoming fully visible.

Counters change immediately when the ordinary fog observer count crosses zero.
Adding or removing an overlapping observer does not change them while the real
tile remains visible. A change propagates upward only when the child's state
changes; changes within a partially visible block stop at that block. Updates
therefore use the same simulation timing as the underlying fog.

Script reveals maintain a separate hierarchy over one-meter AI tiles, updated
when their reveal bits change. A query searches the union of ordinary fog and
eligible script reveals by taking the nearer result. The existing single-player
scope and overlapping-script-mask behavior remain unchanged. Fog-off queries
treat the whole map rectangle as visible.

These summaries are derived caches, excluded from saves and checksums. Init and
load rebuild them from the current fog counts and script mask, including the
applied visibility of observers with pending updates. Clear discards them.

## Limits, invalid inputs and cost

The limit must be positive and finite. Invalid limits and party values other
than `0` or `1` return `1.0f`, even for an already-visible unit. Nonfinite
positions return `1.0f` in the position query. An unavailable/cleared fog grid
also returns `1.0f`; the unit overload can still return zero from its cached
visibility shortcut.

Finite positions outside the map can measure distance to visible terrain inside
the map. Search bounds are clipped before converting to integer indices. Double
intermediates avoid overflow when squaring large finite float inputs.

Already-visible units and positions return immediately. Other queries select a
hierarchy level whose blocks cover the search diameter, or the top level for
larger limits. Only blocks intersecting the bounded search square are visited;
touching boundary blocks are included to preserve exact zero distances for tiny
limits. Invisible blocks are skipped, fully visible blocks provide an exact
distance to their rectangle, and partial blocks descend toward real tiles.
Blocks and tiles that cannot improve the best distance are pruned.

This lets a deep-fog query finish after a few block checks without reading any
real tiles. An entirely hidden map, even at the largest editor size, needs at
most four top-block checks for a map-wide query. Mixed visibility can still
require descent and leaf scans; there is no universal O(1) query guarantee.
The hierarchy level selection itself also depends on the number of levels.

Lower limits bound the searched region more tightly, but elapsed work no longer
necessarily scales with its area because whole blocks can be skipped. The worst
case still grows with the number of tiles within the limit, plus the summaries
visited. A tile visibility change is constant work when its base block keeps
the same state, and at worst touches one block at each level. Queries allocate
no heap storage and use squared distances until the final square root.

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

`WarFogVisibilityHierarchy_test` verifies empty/full-block pruning using work
counts, all three states, transitions back to invisible, partial map edges,
multiple hierarchy levels, and exhaustive results across changing visibility
patterns. Integration tests cover overlapping observers, overlapping script
reveals, and rebuilding summaries from a save with pending fog updates.

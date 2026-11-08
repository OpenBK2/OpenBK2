# AI properties in game data

`Data/Consts/AIPropertyConsts.xdb` contains the configurable values handled by
`AILogic/AIConsts.cpp`: 166 scalar properties and the per-unit-type reveal table.
It includes plane heights, formation dispersion, cover, splash damage, engineer
costs, morale, weather, and the other settings registered in that file. Fixed
engine constants, such as tile size and path limits, stay in code. Settings
registered elsewhere, such as `AI.General.*`, are outside this resource.

## Use the example

Copy the example's `Data` contents into your mod's data tree. In the existing
`AIGameConsts` record referenced by `GameConsts.AI`, add:

```xml
<AIPropertyConsts href="/Consts/AIPropertyConsts.xdb"/>
```

Keep the rest of that `AIGameConsts` record. Use the regenerated `types.xml` and
matching engine/editor binaries so the new resource type is recognized.

- A resolved resource supplies every property. Omitted XML fields use schema
  defaults, not values from the local profile.
- An absent or unresolved link uses the original `AIConsts` profile/settings
  behavior. Existing data needs no changes.
- While a resource is active, console/profile changes to these settings only
  update the retained legacy settings. They take effect if a later mission uses
  the legacy fallback. `getvar` still reports that legacy setting, not the
  effective database property.
- The local `temp.plane_min_height` debug override is ignored while a resource is
  active, so it cannot override `PlaneMinHeight`.

The source is selected at mission initialization and before global AI objects
are restored from a save. Saves retain the existing `AIGameConsts` database
reference; they do not embed a separate snapshot of the resource. The linked
resource participates in the parent's checksum and therefore the existing
multiplayer game-data checksum. Players and replay playback need matching game
data. Legacy fallback still depends on local settings.

## Names and units

For regular fields, remove `AI.` and the remaining dots from the old setting
name. Array slots become individual properties. For example:

| Legacy setting | Resource field |
|---|---|
| `AI.Common.AreaDamageCoeff` | `CommonAreaDamageCoeff` |
| `AI.Common.ArmorForAreaDamage` | `CommonArmorForAreaDamage` |
| `AI.Artillery.DispersionRatio.LineMin` | `ArtilleryDispersionRatioLineMin` |
| `plane_min_height` | `PlaneMinHeight` |
| `plane_max_height` | `PlaneMaxHeight` |
| `use_round_units` | `UseRoundUnits` |

Each field in [DBAIConsts.cll](../../Versions/Temporary/Engine/Sources/AILogic/DBAIConsts.cll)
documents its exact old setting name. Values use the same units as the old
profile. Existing load-time conversions and limits still apply, including
fighter patrol seconds to milliseconds, angle conversion, and help-radius
clamping to 3840 AI coordinates. Angles retain the original conversion using
365, including its existing behavior.

`RevealInfo` uses the same unit-type aliases as `AI.RevealInfo.<type>.*`:

```xml
<RevealInfo>
	<Item>
		<UnitType>soldier</UnitType>
		<Query>0.1</Query>
		<MovingOff>1.0</MovingOff>
		<Distance>64</Distance>
		<Time>1000</Time>
	</Item>
</RevealInfo>
```

Missing types and omitted entry values have zero reveal values. Duplicate types
use the last entry. Unknown type names are logged and skipped. Loading a
resource clears the previous resource's reveal table, so entries cannot leak
between missions. Resource entries are applied in their stored order without
iterating an unordered map.

## Where the example values came from

This example was converted from
`C:\Users\Adam\Desktop\BK2E-GBC\Profiles\consts.cfg` on 2026-11-08. It includes
115 scalar values assigned by that file, 51 existing runtime defaults for values
it does not assign, and 16 reveal entries. For example, splash uses
`CommonAreaDamageCoeff = 0.3` and `CommonArmorForAreaDamage = 300`.

Defaults match the actual C++ variable initializers, which sometimes differ
from the defaults stored in the console registry. A partial resource therefore
does not silently import values from a player's settings. All scalar values
are explicit in this example.

To convert another profile, run from the repository root:

```powershell
python scripts/convert_ai_consts.py "path/to/Profiles/consts.cfg" "path/to/Data/Consts/AIPropertyConsts.xdb"
```

The converter reads `setvar` assignments from that one file, keeps the last
assignment to each key, fills missing fields from the schema, and writes UTF-8
XML. It ignores unrelated settings and does not execute the profile or follow
script includes. Convert the file containing the final values you intend to use.

See [hit processing](../../docs/hit-processing.md) for the combat formulas that
consume the cover, dispersion, and splash properties.

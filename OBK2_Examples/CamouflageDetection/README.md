# Camouflage detection ranges and overlay

Select friendly units and hold **Alt+E** to show their camouflage detection
distance as cyan circles on the map and minimap. Either Alt key works. Release
the keys to hide the circles. Alt+R weapon ranges can be displayed at the same
time. The circles follow movement, selection changes and sight-range changes.

## Existing profiles

The repository's default input profile includes the shortcut. For an existing
installation or mod, merge the two `bind` lines from [input.cfg](input.cfg) into
the `bindsection mission` section of the input profile that it loads, then
restart the game. Do not replace a complete input profile with this fragment.
Both lines are needed: the `!` binding handles releasing the shortcut.

The shortcut itself requires no additional XDB resources, textures or UI buttons.

## Per-unit detection range

`UnitBaseRPGStats.SpyDiscoveryRange` is a float in **meters**, matching `Sight`.
It is inherited by mechanical and infantry stats. Its default is `-1`:

- A value greater than zero replaces `AI.WarFog.CamouflageOpenRadius` for that observer.
- Zero or a negative value uses the global setting.
- The effective radius is always capped by the observer's current sight, including
  sight modifiers and special sight sectors. An override does not grant extra sight.

Copy [SpyDiscoveryRange.fragment.xml](SpyDiscoveryRange.fragment.xml) into the
unit's `MechUnitRPGStats` or `InfantryRPGStats` resource, alongside `Sight`.
The example sets a 24-meter detection radius, compared to the default global
limit of 16 meters. A unit with only 10 meters of sight still detects hidden
units only within 10 meters. The setting belongs to the detecting unit, not
the hidden target, and applies to all camouflaged targets.

Use a build and `types.xml` generated from the updated schema. Omitted fields
in existing XML and binary resources retain the default behavior. This stat
contributes to game-data checksums, so use matching unit data for all players.

## What the circle means

The overlay queries the unit's registered fog-of-war sight. Its radius is the
smaller of the unit's maximum sight radius and its positive `SpyDiscoveryRange`
override, or the `AI.WarFog.CamouflageOpenRadius` setting when no override is
active. The global setting defaults to 8 visibility tiles. One visibility tile
is 2 meters in unit stats (2 AI tiles, or 64 AI coordinate units), so the
default maximum circle has a radius of 16 meters or 512 AI coordinate units.
Detection ranges are rounded down to whole visibility tiles after the sight cap.

This is a distance limit. Terrain, buildings and restricted sight sectors can
prevent detection inside the circle, and tile-based fog calculations can make
the actual edge slightly different from the smooth outline. The overlay does
not reveal hidden enemies or change visibility. Squads show each soldier's
circle; selected buildings use their garrison's observers. Units with no active
fog observer or zero sight do not produce a circle.

The existing setting is read by `GlobalWarFog.cpp`; it is not one of the
`AIPropertyConsts` database fields. No value needs to change to use the overlay.
If a mod changes it, configure the same nonnegative value for every player
before starting a match, as with other simulation settings.

`GetCloseVisibilityRadius` is shared by fog detection and the display query.
Future detection bonuses should extend that calculation and the corresponding
fog observer updates, so the overlay continues to use the simulation's effective
radius, including `SpyDiscoveryRange`.

The held-key state is local and transient: leaving the mission interface or
loading a save clears the overlay. It does not issue multiplayer commands or
change replay simulation.

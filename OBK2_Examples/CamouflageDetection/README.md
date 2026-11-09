# Camouflage detection overlay

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

No XDB resources, schema regeneration, textures or UI buttons are required.

## What the circle means

The overlay queries the unit's registered fog-of-war sight. Its radius is the
smaller of the unit's maximum sight radius and the existing
`AI.WarFog.CamouflageOpenRadius` setting. The latter defaults to 8 visibility
tiles. One visibility tile is 2 AI tiles, or 64 AI coordinate units, so the
default maximum circle has a radius of 512 AI coordinate units.

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
radius. No future bonus fields are added by this feature.

The held-key state is local and transient: leaving the mission interface or
loading a save clears the overlay. It does not issue multiplayer commands or
change replay simulation.

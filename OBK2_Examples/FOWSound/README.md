# Unit movement sounds in fog

Add these fields to your existing `AIGameConsts` resource:

```xml
<FOWSoundDistance>30</FOWSoundDistance>
<FOWSoundMinBasePercentage>0.1</FOWSoundMinBasePercentage>
```

`FOWSoundDistance` is in gameplay meters, converted to AI coordinates before the
visible-terrain query. The default `-1`, or any negative value, disables the
feature. A zero limit allows only units already visible to the observing side.
Choose a modest positive limit: the fog search cost grows with its area.

For a positive limit, visible units and units on visible terrain have gain `1`.
Hidden units at or beyond the limit are silent. Inside the limit, gain is:

```text
proximity = 1 - distanceToVisibleTerrain / FOWSoundDistance
gain = FOWSoundMinBasePercentage + (1 - FOWSoundMinBasePercentage) * proximity
```

With the example settings, a unit 15 meters beyond visible terrain has gain
`0.55`; just inside 30 meters it approaches `0.1`, and at 30 meters it is silent.
The base percentage is a fraction in `[0, 1]`, clamped to that range. Camera
distance, sound descriptors and the user's volume settings still apply.

Movement loops, engine idle loops, and movement start/stop sounds receive this
gain at the normal unit visibility refresh rate, including while the unit stays
hidden. Sound positions follow hidden units without updating their visual
effects. Initial gain is supplied before a new unit starts its loops, and sound
gain and position survive saving and loading.

The observer is the same side used for the displayed fog, including replay
viewpoints. The gain is evaluated only for presentation and changes no simulation
state. Camouflaged units on visible terrain remain audible, as specified by the
visible-terrain query. Firing, shell impacts on terrain/buildings/units,
explosions, death effects, falling/dive effects, ambient sounds and acknowledgments
keep their existing behavior.

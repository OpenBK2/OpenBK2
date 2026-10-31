# Sharp SVG flags for multiplayer sides

The multiplayer game room can draw a side's flag from an SVG file instead of its
32x22 `ListItemIcon` bitmap. The game renders the SVG at exactly the size the flag
takes on screen, so it stays sharp at any resolution, and keeps the drawing's own
proportions, centred in the window. This folder shows how a mod wires its own
flags, where to get drawings, and which flags OpenBK2 does not ship and why.

## What OpenBK2 ships

`obk2_flags.pak`, installed into `Data` by the build, carries drawings and records
under `Flags/`, and `Flags/Mapping.xdb`, which gives sharp flags to games whose
multiplayer data it cannot edit: the original game, Fire of the Rising,
Liberation, and Total Conversion. For a mod, it redraws what the mod's own icon
shows and changes nothing about it: a mod's choice of flag is the mod's, even
when it is not what history flew, since a mod may need two sides to look
different. Sides it does not recognise keep their own icon.

## How the game picks a side's flag

For each side and the chosen tech level:

1. The tech level's year: its `FromYear`, or, if it has none, the year
   `Flags/Mapping.xdb` gives its name.
2. If the side has a `FlagAuthority` or a `FlagTerritory`, the flag comes from
   there, for that year, and the mapping is not consulted. **A side's own
   references need the tech level to have a year;** without one the side shows
   its `ListItemIcon`.
3. Otherwise `Flags/Mapping.xdb` is looked up by the bytes of the side's
   `ListItemIcon` image file.
4. If nothing matches, the side shows its `ListItemIcon`, as before.

The "random country" item has no side record, so only `Flags/Mapping.xdb` can
give it a drawing.

## Wiring your own flag

A mod that ships its own multiplayer consts can name its flags directly. The
files under `MyMod/Flags` here are a complete example; put them in your mod's
data or archive at the same paths, or at your own.

1. **The drawing**, `MyMod/Flags/Example_Faction.svg`.
2. **An image record** for it, `MyMod/Flags/Example_Faction.xdb`:

   ```xml
   <FlagImage>
   	<Image href="Example_Faction.svg" />
   </FlagImage>
   ```

   A relative `href` is relative to the record; one starting with `/` is from the
   data root.

3. **An authority**, the government or faction the flag belongs to, with the
   years each flag was used, `MyMod/Flags/Example_Faction_Authority.xdb`. A period
   from 0 to 0 covers every year, which is what a side with one flag wants:

   ```xml
   <FlagAuthority>
   	<NameFileRef href="" />
   	<Periods>
   		<Item>
   			<FromYear>0</FromYear>
   			<ToYear>0</ToYear>
   			<Image href="/MyMod/Flags/Example_Faction.xdb#xpointer(/FlagImage)" />
   		</Item>
   	</Periods>
   </FlagAuthority>
   ```

   For a flag that changed, list one period per flag. A year in two periods, as
   the year of a change is, takes the period listed first.

4. **In your `MultiplayerConsts`**, give every tech level its years, and the side
   its authority:

   ```xml
   <TechLevels>
   	<Item>
   		<NameFileRef href="..." />
   		<DescriptionFileRef href="..." />
   		<FromYear>1941</FromYear>
   		<ToYear>1942</ToYear>
   	</Item>
   </TechLevels>
   <Sides>
   	<Item>
   		...
   		<FlagAuthority href="/MyMod/Flags/Example_Faction_Authority.xdb#xpointer(/FlagAuthority)" />
   	</Item>
   </Sides>
   ```

   `FlagTerritory` does the same for a territory record, which lists several
   authorities in order of importance; a year takes the first of them with a flag
   for it. `FlagAuthority` wins when a side has both. Keep `ListItemIcon`: it is
   what a game without SVG flags shows.

Old builds ignore the new fields, so the same data still loads there, with the
bitmaps. None of these fields is part of the multiplayer checksum.

Use your own folder, as `MyMod/Flags` here, rather than `Flags/`: a file of yours at
one of OpenBK2's paths replaces OpenBK2's for every side that uses it. In
particular, a `Flags/Mapping.xdb` of yours replaces the whole mapping.

## Mapping data you cannot edit

`Flags/Mapping.xdb` is for multiplayer consts that live in an archive you do not
own. A side is recognised by the FNV-1a 64-bit hash of its `ListItemIcon` image
file (the `.dds`), written as 16 lowercase hex digits, and mapped to an `Image`
(the same drawing for every year), an `Authority` or a `Territory`. A tech level
is recognised by its name, with every kind of dash read as a hyphen. To hash a
file:

```python
data = open('FlagGermany_Texture.dds', 'rb').read()
h = 0xcbf29ce484222325
for b in data:
    h = ((h ^ b) * 0x100000001b3) & 0xffffffffffffffff
print('%016x' % h)
```

If you want your mod's icons in OpenBK2's own mapping, send the hashes and the
drawings in a pull request rather than shipping a `Flags/Mapping.xdb`.

## Getting drawings

- **Wikimedia Commons** has most flags, historical ones included. Check each
  file's licence: public domain is simplest. CC BY-SA requires credit, and
  anything you derive from the file has to carry the same licence. Record the
  source page, author, licence and the file's SHA-1 in a comment in the SVG.
- **Flags of the World** (fotw.info) documents construction and colours, for
  drawing a flag yourself. Simple flags (bands, crosses, discs, stars) are a few
  lines of SVG each; `Flags/Images` has many examples.
- `scripts/flags/dice.py` shows how to generate a drawing from a script when it is
  easier to compute than to write.

What the renderer (lunasvg) handles: paths and shapes, gradients, `clipPath`,
masks, `<use>`, CSS styles. What it does not:

- **Filters**: no blur, no drop shadow. Fake a soft shadow with a radial gradient
  fading to transparent, as `Random_Dice.svg` does.
- **Text**: system fonts are not loaded, so convert text to paths.
- **External files**: an `<image>` must embed its picture as a `data:` URI, and a
  picture scaled down by the renderer loses its sharpness, which is the point of
  an SVG.

The flag window is small (about 80x46 pixels at 2560x1600), so check a drawing at
that size: fine detail in a coat of arms turns to noise.

## Flags OpenBK2 does not ship

OpenBK2 does not distribute the following, in `obk2_flags.pak` or anywhere else in
the tree. This is about distributing them, not only about showing them: a file
that is shipped but switched off by default would carry the same risk.

| Symbol | Why | What OpenBK2 shows instead |
|---|---|---|
| The swastika, in any flag or emblem (the flags of Nazi Germany from 1933 to 1945, its war ensigns, and any other flag carrying it) | In Germany, distributing or publicly using the symbols of unconstitutional organisations is an offence under section 86a of the Criminal Code (StGB). Its social-adequacy exception covers art, science, research and teaching, and since 2018 the USK may rate a game showing them case by case, but that is a decision per published game. Austria prohibits public display of these symbols under the Abzeichengesetz of 1960. | In this tree's own data, the German side shows a stand-in after the original game's own icon, without a swastika. A mod's German side that shows a swastika is left unmapped, so it keeps the mod's own icon. |
| The flag of Francoist Spain with the eagle of St John (1938 to 1981) | Spain's Democratic Memory Law (Ley 20/2022) requires the removal of Francoist symbols from public view, with sanctions. It is not a ban on distribution, but it is the same kind of risk. Commons also has no public domain drawing of it, only CC BY-SA ones. | Total Conversion's Spanish Nationalists side is left unmapped and keeps the mod's own icon. |

These *are* shipped, and you should know that some countries restrict them:

- **Communist symbols.** The Soviet flag with the hammer and sickle, and the flag of
  People's Poland with its crownless eagle, are in `obk2_flags.pak`. Several
  countries ban or restrict the public display or propagation of communist
  symbols, among them Lithuania, Latvia, Ukraine and Indonesia.

Laws change and depend on where a mod is published, so check the current law
where you distribute. None of this is legal advice.

## If your mod needs one of these symbols

OpenBK2 does not ship them, and does not stop a mod from doing so. To show one
sharp, ship the SVG in your own mod's data or archive and wire it to your side
with `FlagAuthority` as above. The side's own reference always wins over
`Flags/Mapping.xdb`, so nothing OpenBK2 ships overrides your choice. Distributing
it is then your decision, under the law of wherever you publish.

## Sides left with their own icons

For reference, the sides OpenBK2 knows about and leaves alone:

- Total Conversion: Germany (swastika), Spanish Nationalists (Francoist eagle), and
  the three coalition sides (Allies, Bulgaria with partners, China), which combine
  several flags in one icon. Composite flags are not supported yet.
- MPBMod6: its own USA and Japan icons. Its Germany, USSR and Great Britain use
  the original game's icons and get those sides' drawings.
- Universal MOD-18: not mapped yet.

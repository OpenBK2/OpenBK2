# Blurry UI text

Text in the game UI is soft at anything above the resolutions Nival shipped
atlases for. There are two independent causes, and neither one alone accounts
for it. The asset pipeline threw away the antialiasing it asked GDI to compute,
and the runtime magnifies a single small atlas rather than drawing one that
matches the size on screen.

Both are fixed. The game now rasterises its fonts itself with FreeType, at
exactly the pixel size the UI asks for, so text draws texel for pixel at every
resolution and aspect; see "Runtime fonts" below. The rest of this document is
the investigation that led there, which still explains the baked fonts that old
data and mods fall back to.

## How text reaches the screen

Originally, and still for a font record without a FontFile: FontGen rasterises
a character set into a glyph atlas and a binary blob of metrics. The blob is
one CFontFormatInfo, the same class the engine carries in 3Dmotor/FontFormat.h
under the same class id, so the tool and the runtime agree on the format by
sharing it. CTextLocaleInfo::AddFont in 3Dmotor/GLocale.cpp, called for every
font record of the game root when a MOD is set up (GameX/MODSetup.cpp), opens
the blobs through the database; UI/UIML.cpp lays glyphs out from the metrics,
and Direct3D draws textured quads. For such a font no glyph rasterisation
happens at run time: whatever the atlas contains is what the screen gets,
resampled.

## What the game actually loads

Measured with a temporary log in AddFont (see "Diagnostics" below), the
database holds exactly four font records, all in Data/texts.pak dated
2005-12-06:

    record          name     face    baked  cell  texture
    Fonts/Body      body     System  18     16    256x256 DXT3
    Fonts/Header1   h1       Impact  38     37    512x512 DXT3
    Fonts/Header2   h2       Impact  20     20    256x256 DXT3
    Fonts/Numeric   numeric  Arial   14     14    256x128 DXT3, not antialiased

Their metrics files are bin/Fonts/<uid>. A few consequences:

- There is one atlas per name. CTextLocaleInfo::SearchFont picks the nearest
  height among records with the requested name, so with one candidate it
  returns the same atlas for every size. h1 is used at both 38pt and 48pt.
- The "System" default face in UIML.cpp names no record; nothing registers
  under it. Earlier notes assumed a System atlas and a set of 800x600 and
  1024x768 records; none of those is reachable at run time.
- The body face is the Windows "System" bitmap font. GDI cannot antialias a
  bitmap font and can only scale it by whole multiples, which is why
  Fonts/Body has two alpha levels, 0 and 255.
- Line space equals height for all four, so the height against line space
  mismatch noted earlier (five of 35 blobs in bin/fonts) does not affect any
  font the game draws.

## Cause one: the atlas is a 1 bit mask

FontGen asked GDI for ClearType and then read the result as if it were
greyscale: it kept only the green channel as alpha. Under ClearType the three
channels are coverage of three different subpixels, and ClearType grid fits
hard on top of that, so the green channel alone is close to on or off. The
shipped atlases measure out accordingly: Body, Header2 and Numeric carry two
alpha levels; Header1 has eleven, with everything below roughly 40 percent
coverage crushed to zero. That crush is also why Impact's already short
descenders lost a row in the shipped Header2.

This is fixed. FontGen keeps ClearType and averages the three subpixels, which
is a downsample of a 3x horizontal supersample and measures better than
ANTIALIASED_QUALITY everywhere. Distinct alpha levels strictly between 16 and
240, on Arial with -russian:

    height            16    24    32
    green only         5     5     5
    ANTIALIASED       none  12    12
    averaged          31    35    33

The subpixel data itself is deliberately not kept. Baking it would encode the
stripe order of the panel it was baked on and be wrong on a BGR or a rotated
display, with no way to correct it at draw time.

DXT3 alpha is 4 bits, so the shipped texture format keeps only 16 levels. The
test bakes below use uncompressed A8R8G8B8 instead.

## Cause two: one atlas, magnified

The sizing path converts a size in points to pixels, and fScale is the ratio of
that to the atlas's line space. At 2560x1600, before any fix, a size=16 run
asked for 40 pixels from a 16 pixel atlas: 2.5x magnification, drawn with
FILTER_LINEAR (GfxUtils.cpp). That is the blur.

It also stretched text sideways. The UI is laid out on a 1024x768 virtual
screen that UI/UI.cpp maps to the real one with independent X and Y factors.
Points were converted with the X factor and only scale.y was corrected for
aspect, so glyphs were drawn 2.5 across and 2.08 down, 20 percent too wide.

The first fix made both axes follow the Y factor. This also made labels
noticeably narrower than the original UI on widescreen. The renderer now keeps
both pixel dimensions from the 1024x768 virtual screen, including the original
horizontal stretching. FreeType rasterises at that width directly; the UI draws
the result at scale 1 instead of stretching a small bitmap. Baked fallback
fonts and outlines follow the same two scales.

That leaves the magnification. At 2560x1600 the four fonts are requested at 33
(body 16pt), 41 (h2 20pt), 79 and 100 (h1 38pt and 48pt) and 29 (numeric 14pt)
pixels, against atlases of 16, 20, 37 and 14.

## Magnification also trims the edge rows

C2DQuadsRenderer::AddRect in GfxUtils.cpp shrinks the sampled source rect by
half a texel at each edge whenever a quad is magnified, so that linear
filtering does not pick up the neighbouring cell; FontGen packs glyph rows with
no padding between them. The side effect is that the first and last texel row
of every cell get half the screen height of the others. Descenders end on the
last row, so they come out short. At scale 1.0 the code path is skipped.

## Clipped label edges

CForegroundTextString and CPlacedText converted the pixel size of their text to
virtual units through the integer overload of ScreenToVirtual, which truncates,
and used the result as the clip window. A virtual unit is one pixel at
1024x768, so nothing was lost there, but at 2560x1600 it is 2.5 pixels across:
the last letter of a label lost up to 2.5 px on the right and the line up to
2 px at the bottom. Fixed; the size is kept fractional for clipping and rounded
up where a control is sized from it.

## Demonstrated: baking at the drawn size

With the fixes above, a patch pak holding atlases baked at 2560x1600's sizes
draws body, h2 and h1 at 48pt at exactly scale 1.0000, and the result is sharp.
The reference set is kept outside the tree in
C:\Games\bk2\fontpaks_reference\gdi_2560x1600, with a README giving the exact
FontGen arguments and the command that rebuilds the pak.

How a patch overrides retail data, from System/WinVFS.cpp: every .pak in Data
is opened, and for each path the entry with the newer timestamp *inside its
archive* wins; on a tie the first archive enumerated keeps it. The .pak file's
own date does not matter. A font override needs only two files per record,
bin/Fonts/<uid> and <record>/Texture.dds, because the texture loader takes size
and pixel format from the DDS header, so no .xdb changes. Uncompressed
A8R8G8B8 with one mip level loads fine.

What the bake could not do:

- Body is Tahoma rather than System. GDI renders System at 33 only by doubling
  it to 32, still 1 bit.
- GDI cannot produce every cell height. It rounds to a whole-pixel em, so
  Impact gives 76 or 80 but not 79, and Arial 28 or 31 but not 29.
- h1 is one atlas for two sizes. Baked at 100, 48pt is exact and 38pt draws at
  0.79, minified without mipmaps.
- The bake is per resolution. At any other resolution the text is resampled
  again.

Impact's short descenders are the face, not a bug: it has a very tall x-height,
and "p" drops about 0.1 em, 3 px at a 41 px cell.

## Ruled out

Half texel alignment is correct already. FillRect in 3Dmotor/GfxUtils.cpp
subtracts 0.5 from every vertex and AddRect snaps to integers.

FontGen does not clip glyphs: every bake measured leaves the descenders inside
the cell, Tahoma with a blank row below them.

## Fixed alongside

Float2Int truncated after a port commit replaced the fld and fistp pair; it
rounds by the current rounding mode again, via cvtss2si, which callers depend
on (the simulation rounds to nearest, DB loading and console SetVar truncate on
purpose). It did not affect the blur.

FontGen converted the character codes it was baking to Unicode as if they were
UTF-8. They are single code page bytes, so a -russian bake silently lost all 64
Cyrillic characters from its metrics while the texture stayed correct. It now
converts through the code page the charset implies; verified against the 2005
x86 binary.

## FreeType in FontGen

FontRaster (Versions/Temporary/Engine/Sources/FontRaster) is FreeType behind a
small interface of its own, producing an atlas and metrics in the conventions
CFontFormatInfo has always used, which are GDI's. FontGen uses it and nothing
else now: the GDI path is gone and the tool builds and runs on Linux, with the
code page translation reproduced from Windows' own answers
(FontRaster/CodePageTables.inc, regenerated by gen_codepages.py). Faces named
the GDI way ("Tahoma", -w400) are found among the installed fonts by
FontFinder; a path to a font file works too.

Against GDI on the same Windows faces: ascent, descent, external leading and
average width come out identical; FreeType takes a fractional em, so every cell
height is reachable; lines run 1 to 2 percent wider under light hinting, which
keeps the designed advances; strokes are a little lighter, FreeType's coverage
being linear. Bakes are byte-identical run to run and between Windows and
Linux, after two BinSaver fixes: objects are written under sequential IDs
rather than their addresses, and a det_map writes its element count where the
standard library's bucket count used to go.

Fonts made for many scripts set usWinAscent and usWinDescent high enough for
stacked accents and scripts the game never draws, and so fill a cell sized the
GDI way poorly: Noto Sans's capitals reach 0.47 of the cell against Tahoma's
0.60. -cell=ink fits the cell to the ink of the characters actually baked
instead, which puts every font tried at 0.60 to 0.68.

## Runtime fonts

A font record with a FontFile is rendered by the game itself (3Dmotor/
GRuntimeFont.h). Each requested pair of pixel dimensions gets its own
CGlyphAtlas. The font file and the original baked metrics/texture are read
through the VFS. The capital H in the original atlas supplies the visible
capital-height ratio; the total advance of A-Z, a-z and 0-9 supplies the width
ratio. The replacement is fitted to those dimensions, so changing font families
does not silently change the apparent UI text size. No GUI font sizes change.
If no usable reference is available, the previous ink fitting is used.

The NGlobal user setting `ui_font_scale` adjusts all runtime font sizes and
is saved with the user configuration. It defaults to `1.2` (20% larger in both
dimensions); `1.0` restores the original reference size. Enter
`ui_font_scale 1.4` in the console, or use
`NGlobal::SetVar( "ui_font_scale", 1.4f )` from code. Read it with
`NGlobal::GetVar( "ui_font_scale", 1.2f ).GetFloat()`.

Changes refresh existing text layouts and their font cache without a restart
or rebuild. Rasterisation and layout use the same effective scale, limited to
0.25 through 4.0; zero, negative and non-finite values use the default. The
multiplier is applied before rasterisation, retaining sharpness. Saved atlases
store final pixel dimensions, so loading a save never multiplies them again;
newly saved layouts also keep their wrapping width and scale for live refresh.

Scrollable descriptions also retain their full content extent when a parent
window moves or resizes. Previously, repositioning reset the inner clipping
window to the viewport height, even while the description and scrollbar still
reserved space for the entire text. This cut off later lines in campaign
panels after those panels were moved into the outer campaign list. The scroll
container now remeasures its children and restores the content extent and
scroll range after repositioning.

Fixed-height text fields, such as the selected unit's weapon statistics,
vertically center a single line when its font cell exceeds the field height.
This distributes the cell's accent/descender padding across both edges instead
of cutting off the digits below it. The draw origin snaps to screen pixels to
preserve sharpness. Wrapped/multiline and automatically sized descriptions
keep their original top alignment.

The first runtime version fitted the tallest ink across six European code
pages into the requested line height. Oswald's accents and low marks then
reduced its capitals to about 56% of the cell, versus 65% in the shipped h2
atlas. Matching cap height fixes this; the raster cell grows when necessary to
retain accents and descenders. Text layout uses that full cell at scale 1, so
labels account for the extra ink instead of clipping it or shrinking it again.
Each character is rasterised on first use, through CFontInfo::PrepareGlyphs.
The horizontal correction is applied by FreeType to outlines, advances and
kerning before rasterisation, preserving sharpness at widescreen dimensions.

- Characters the font lacks come from the record's FallbackFontFiles, then from
  a list of well-covered fonts installed on the system (Segoe UI, Microsoft
  YaHei, Nirmala UI, Noto, DejaVu and others), whose directories are read once
  on the first such character, and otherwise show the font's .notdef box. That
  is what lets nicknames and chat in any script display.
- Laid out text keeps the atlas coordinates of its glyphs, so a glyph never
  moves once placed. An atlas that fills up gives further characters the
  default box.
- A save game holds the record, both requested dimensions, the sizing mode and
  the characters in placement order. Saves made before reference sizing keep
  their original ink fitting, preserving coordinates in already saved layouts.
- A record without a FontFile, or whose file cannot be used, is drawn from its
  baked atlas as before.

The shipped records use open fonts, since the Windows faces cannot be shipped:
PT Sans for body, Oswald Bold for h1 and h2 (Thickness 700 picks the named
instance), Liberation Sans for numeric. They are in Data/Fonts/Files with their
licences. The Windows faces and three candidates for body (PT Sans, Noto Sans,
DejaVu Sans) were compared side by side before choosing; Anton, the usual Impact
look-alike, has no Cyrillic, and no open face matched Impact's weight.

## Still open

- Atlases are sized for about 512 glyphs, so h1's two sizes take 2048 square
  each. Starting large sizes small and growing the atlas by rows, which moves
  no glyph, would save most of that.
- No shaping: Arabic, Hebrew and the Indic and Southeast Asian scripts show
  their own glyphs but unjoined and unordered. HarfBuzz and a bidi algorithm
  per line would fix it; the atlas takes characters one at a time, and would
  need to take glyph indices for that.
- Kerning comes from a font's kern table, or from its GPOS pair adjustments
  when it has none (FontRaster/GposKerning.h), which is how Oswald and Noto
  Sans are kerned. Contextual kerning and a variable font's per-instance
  kerning deltas are not read, so Oswald Bold is kerned as the Regular is.
- The first character that needs a system font costs a scan of the installed
  fonts, up to about a second over Windows' own.

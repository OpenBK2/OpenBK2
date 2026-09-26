# Blurry UI text

Text in the game UI is soft at anything above the resolutions Nival shipped
atlases for. There are two independent causes, and neither one alone accounts
for it. The asset pipeline threw away the antialiasing it asked GDI to compute,
and the runtime magnifies a single small atlas rather than drawing one that
matches the size on screen.

Both are now understood and the second has been demonstrated fixed: with
atlases baked at the drawn size, text at 2560x1600 draws texel for pixel and is
sharp. What remains is making that hold at every resolution, which is the
FreeType work at the end of this document.

## How text reaches the screen

FontGen rasterises a character set into a glyph atlas and a binary blob of
metrics. The blob is one CFontFormatInfo, the same class the engine carries in
3Dmotor/FontFormat.h under the same class id, so the tool and the runtime agree
on the format by sharing it. CTextLocaleInfo::AddAllAvailableFonts in
3Dmotor/GLocale.cpp opens the blobs through the database, UI/UIML.cpp and
UI/mlVisObjects.cpp lay glyphs out from the metrics, and Direct3D draws
textured quads. No glyph rasterisation happens at run time, on any platform.

That is worth stating plainly because it disposes of a whole category of
suspicion: there is no ClearType, no CoreText, no FreeType and no subpixel
rendering in the running game to go wrong. Whatever the atlas contains is what
the screen gets, resampled.

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

Fixed: NGScene::FontPointsToPixels in 3Dmotor/GLocale.h converts with the Y
factor, and scale.x equals scale.y, in all three copies of the conversion
(UIML.cpp, mlVisObjects.cpp, GText.cpp). Vertical size is unchanged; letters
keep their proportions and wide windows get more room beside the text. Images
and window layout still stretch.

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

## Diagnostics

3Dmotor/GLocale.cpp and the GetFontFormatInfo functions in UIML.cpp and
GText.cpp carry temporary logging, marked TEMPORARY and not committed, that
writes fontdiag.log in the game's working directory: every font record with
the archive its files came from, and every distinct size request with the font
chosen and the resulting scale. The VFS side is permanent: SFileStats::pszName
now names the archive a file was served from.

## Plan from here

1. Replace FontGen's rasteriser with FreeType, written as a library that
   produces a CFontFormatInfo and an atlas, so that the game can use the same
   code later. Bake the same four Windows faces at the same sizes and compare
   against the GDI reference through the same pak pipeline, so that any
   difference is the rasteriser. FreeType takes fractional em sizes, so every
   cell height is reachable, and the atlas gets padding between cells.
2. Reuse that library at run time as a glyph cache keyed by face and pixel
   size, behind CFontFormatInfo's GetChar, GetKern and GetLineSpace, so every
   size at every resolution is native and h1's two sizes stop sharing an
   atlas. This needs font files the game may ship, which the Windows faces are
   not; open faces with full Cyrillic (PT Sans, Liberation Sans, a condensed
   display face for Impact) are to be compared against the Windows ones first.

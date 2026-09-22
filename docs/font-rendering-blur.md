# Blurry UI text

Text in the game UI is soft at anything above the resolutions Nival shipped
atlases for. There are two independent causes, and neither one alone accounts
for it. The asset pipeline throws away the antialiasing it asks GDI to compute,
and the runtime magnifies a single small atlas rather than selecting one that
matches the size being drawn.

Fixing only the first gives correctly antialiased glyphs that are still
magnified. Fixing only the second gives crisply sized glyphs that are still
1 bit masks. The order below does the cheap independent fixes first, then the
baking, then the magnification, because each step is separately verifiable.

## How text reaches the screen

FontGen rasterises a character set into a glyph atlas and a binary blob of
metrics. The blob is one CFontFormatInfo, the same class the engine carries in
3Dmotor/FontFormat.h under the same class id, so the tool and the runtime agree
on the format by sharing it. CTextLocaleInfo::AddAllAvailableFonts in
3Dmotor/GLocale.cpp opens the blobs through the database, GText.cpp lays glyphs
out from the metrics, and Direct3D draws textured quads. No glyph rasterisation
happens at run time, on any platform.

That is worth stating plainly because it disposes of a whole category of
suspicion: there is no ClearType, no CoreText, no FreeType and no subpixel
rendering in the running game to go wrong. Whatever the atlas contains is what
the screen gets, resampled.

## Cause one: the atlas is a 1 bit mask

FontGen asks GDI for ClearType and then reads the result as if it were
greyscale.

FontGen/FontGen.cpp:367 computes the quality argument for CreateFont as
`bAntialias ? (IsWinXPOrLater() ? 6 : ANTIALIASED_QUALITY) : NONANTIALIASED_QUALITY`.
The literal 6 is CLEARTYPE_QUALITY, written as a number because the 2003
Platform SDK predates the named constant. Every machine that has run this tool
since is XP or later, so the branch is always taken.

FontGen.cpp:468 then walks the 24 bit bitmap and keeps only the green channel as
alpha, with the blue and red reads commented out beside it. Under ClearType the
three channels are coverage of three different subpixels rather than three
copies of one greyscale value, and ClearType grid fits hard on top of that, so
the green channel alone is close to on or off. The shipped atlases measure out
accordingly: Fonts/Body, Fonts/Header2, Fonts/Numeric and
Fonts/common/system.dds each carry two distinct alpha levels, 0 and 255, despite
`<Antialiased>true</Antialiased>` in their records. Fonts/Header1 has eleven
levels with everything below roughly 40 percent coverage crushed to zero.

ANTIALIASED_QUALITY makes green a real greyscale ramp and is the one line change
that tests this. A FreeType based rasteriser fixes it by construction, since
FreeType hands back an 8 bit coverage bitmap with no channel packing to
misread.

Two things bound the win. The textures are DXT3, whose alpha is 4 bits, so any
new atlas is capped at 16 coverage levels until that changes on the data side.
And a correctly antialiased atlas is still per resolution data.

## Cause two: one atlas, magnified

UI/UIML.cpp:16 makes System the default face, and only one System atlas ships,
at height 16. CTextLocaleInfo::SearchFont in 3Dmotor/GLocale.cpp:51 picks the
nearest height among the fonts registered under a given name, so with one
candidate it returns the 16 pixel atlas for every requested size. The engine
scales an atlas; it does not select one.

The scale follows from the sizing path. UI/mlHandlers.cpp:120 treats an
unsuffixed `size=N` as points, UI/UIML.cpp:48 converts points to pixels as
`N * screenWidth / 1024`, and UI/UIML.cpp:67 forms `fScale = nSize /
GetLineSpace()`. GText.cpp:332 and :613 do the same. At 1920 x 1080 a `size=16`
run asks for 30 pixels from a 16 pixel atlas, so fScale is about 1.875, and
GfxUtils.cpp:332 binds the font texture FILTER_LINEAR. That is the blur.

Nival shipped Fonts/1024x768 and Fonts/800x600 and stopped, which is where this
approach runs out: baking covers the resolutions somebody baked.

UI/UI.cpp:7-20 maps a virtual 1024 x 768 layout onto the viewport with
independent X and Y factors and no letterboxing, and UIML.cpp:69 aspect corrects
scale.y only, so in non 4:3 modes the two axes resample differently even when
the atlas size is right.

## Ruled out

Half texel alignment is correct already and is not worth investigating again.
FillRect in 3Dmotor/GfxUtils.cpp:209-219 subtracts 0.5 from x on every vertex,
AddRect snaps to integers at :247, and there is a magnification source rect
inset at :255-263.

## Two independent bugs found alongside

Both are small, neither depends on the decisions above, and the second causes
blur at native resolution.

Float2Int no longer rounds. Misc/Tools.h:184 is a plain static_cast<int>, which
truncates, after a port commit replaced the fld and fistp pair that rounded to
nearest. There are around 225 call sites, including the UI quad snapping at
GfxUtils.cpp:247-250 and the UV quantisation at :184-185.
`_mm_cvt_ss2si( _mm_set_ss( fVal ) )` restores the original behaviour.

Fonts are registered by height and scaled by line space. GLocale.cpp:39
registers under GetHeight, while UIML.cpp:67 and GText.cpp:613 divide by
GetLineSpace, which is height plus external leading. Five of the 35 blobs in
Versions/Current/Data/bin/fonts have leading 1, so fScale comes out as 17/18,
about 0.944. Those runs are minified even at 1024 x 768.

## Plan

1. Fix Float2Int and the GetHeight against GetLineSpace mismatch. Independent of
   everything else, and the second removes a blur source at native resolution.
2. Build FontGen, which currently has no CMake wiring at all. This makes the
   ANTIALIASED_QUALITY change testable and tells us how much of the blur was
   baking before committing to anything larger.
3. Replace FontGen's rasteriser with FreeType, keeping the CFontFormatInfo
   output format unchanged so no engine code and no .xdb records move. FreeType
   is a software rasteriser in fixed point arithmetic, so the same version and
   font file produce the same bytes on every platform, which makes the output
   golden hashable in CI the way the unit tests already work.
4. If the magnification still dominates, reuse the same FreeType layer at run
   time as a glyph cache keyed by face and pixel size, so fScale is always 1 and
   every resolution is native. Kept behind CFontFormatInfo's existing GetChar,
   GetKern and GetLineSpace, this leaves UIML and GText untouched. It does make
   FreeType a runtime dependency of the shipped game rather than a build tool,
   which brings its attribution requirement with it.

Steps 3 and 4 are the same FreeType integration used twice, which is why the
rasteriser should be written as a layer behind that interface rather than as
part of a tool.

## Not yet verified

None of this has been confirmed visually. No screenshot, no running game, and
no before and after comparison at a known resolution.

Before baking anything, measure one glyph's on screen pixel size against its
texel size in both axes, since the per axis UI scaling means those can disagree.

The alpha histograms were taken from five atlases. The roughly 21 records under
Other/Font/800x600 and Other/Font/1024x768 were not decoded, and whether they
are reachable at run time was not established.

Whether moving the font textures from DXT3 to uncompressed A8 helps is a data
side change that has not been tried.

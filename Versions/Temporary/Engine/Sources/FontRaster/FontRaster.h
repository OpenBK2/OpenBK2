#pragma once

// Glyph rasterisation with FreeType, producing a font atlas and metrics in the
// conventions the engine's CFontFormatInfo has always used, which are GDI's.
//
// FontGen used to rasterise with GDI (CreateFont and TextOut into a DIB). This
// does the same job portably, and at an exact cell height: GDI rounds to a
// whole-pixel em, so some heights were unreachable, where FreeType takes a
// fractional em. It knows nothing of the engine's object model or file
// formats, so that FontGen can bake with it now and the game can render with
// it later. See docs/font-rendering-blur.md.
//
// The GDI conventions it reproduces, since the engine lays text out by them:
//
//   cell        every glyph occupies a cell nCellHeight tall whose top is the
//               ascent line, so the baseline sits nAscent rows down
//   A, B, C     A is the gap from the pen to the ink, B the ink's width, C the
//               gap from the ink to the next pen position; the advance is
//               A + B + C, and C is negative when the ink overhangs it
//   rect        the atlas rect starts at the ink and is B + max(C, 0) wide
//   metrics     ascent and descent from the OS/2 table's usWinAscent and
//               usWinDescent, external leading from GDI's own formula over the
//               hhea table, average width from xAvgCharWidth and maximum width
//               from advanceWidthMax, all scaled and rounded

#include <cstdint>
#include <string>
#include <vector>

namespace NFontRaster
{

enum EHinting
{
	// outlines as designed, positioned to a fraction of a pixel; nothing snaps
	// to the pixel grid, so stems can fall across two pixels
	HINTING_NONE,
	// the auto-hinter snapping vertical metrics only (baseline, x-height, cap
	// height), leaving glyph shapes and widths as designed; FreeType's
	// recommendation for antialiased text and the default here
	HINTING_LIGHT,
	// the font's own hinting instructions, both axes
	HINTING_NORMAL,
};

struct SOptions
{
	std::string szFontFile;			// a TrueType or OpenType file
	int nFaceIndex = 0;					// which face, for a collection (.ttc)
	int nCellHeight = 16;				// ascent + descent in pixels, exactly
	EHinting eHinting = HINTING_LIGHT;
	bool bAntialias = true;			// false renders one bit per pixel
	// Applied to coverage as alpha = coverage ^ (1 / fGamma). 1 leaves FreeType's
	// linear coverage alone; above 1 thickens the antialiased edges, which is
	// roughly what GDI's ClearType contrast does.
	float fGamma = 1.0f;
	// Blank pixels between neighbouring cells in the atlas, both across and
	// down, so that bilinear filtering never picks up a neighbour's ink.
	int nPadding = 2;
};

// Where one glyph sits in the atlas and how it advances the pen, in pixels
struct SGlyph
{
	uint32_t nCodePoint = 0;
	int x1 = 0, y1 = 0, x2 = 0, y2 = 0;	// the rect in the atlas: x2 - x1 is B + max(C, 0), y2 - y1 the cell height
	int nA = 0;													// pen to ink
	int nBC = 0;												// ink to the next pen position, B + C
	int nWidth = 0;											// B + max(C, 0)
};

struct SKerningPair
{
	uint32_t nLeft = 0, nRight = 0;			// code points
	int nAmount = 0;										// pixels added to the pen between them
};

struct SFont
{
	int nCellHeight = 0;
	int nAscent = 0;
	int nDescent = 0;
	int nExternalLeading = 0;
	int nAveCharWidth = 0;
	int nMaxCharWidth = 0;
	// the code points asked for, in order, each with its rect; a code point the
	// font has no glyph for gets the font's .notdef glyph, as GDI would draw
	std::vector<SGlyph> glyphs;
	std::vector<SKerningPair> kerns;
	// the atlas, one byte of coverage per pixel, row after row from the top
	int nAtlasWidth = 0, nAtlasHeight = 0;
	std::vector<uint8_t> atlas;
	// diagnostics, for the caller to report
	std::vector<uint32_t> missing;			// code points that fell back to .notdef
	std::vector<uint32_t> clipped;			// code points whose ink reached outside their cell
	double fPixelsPerEm = 0;						// the em size that gives nCellHeight
};

// Rasterises codePoints from options.szFontFile into pResult. Returns false and
// fills *pszError when the font cannot be opened or the atlas cannot hold it.
bool Rasterise( const SOptions &options, const std::vector<uint32_t> &codePoints, SFont *pResult, std::string *pszError );

}

#pragma once

// One font face, open for as long as it is needed, fitted once to a cell height
// and then asked for glyphs one at a time.
//
// This is the part of FontRaster that a runtime glyph cache needs: the game
// opens a face from a font file in its paks, fits it to the pixel size the UI
// asks for, and rasterises each character the first time some text uses it.
// Rasterise() in FontRaster.h is the batch form FontGen uses, built on this.

#include "FontRaster.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace NFontRaster
{

// A glyph as rasterised, before it is placed anywhere: its ink as coverage and
// where that ink sits relative to the pen and the baseline, in GDI's ABC terms
struct SGlyphBitmap
{
	uint32_t nCodePoint = 0;
	bool bMissing = false;						// the face has no glyph for it; this is .notdef
	int nA = 0, nB = 0, nC = 0;				// pen to ink, ink width, ink to next pen
	int nTop = 0;										// rows from the ink's top down to the baseline
	int nRows = 0;
	std::vector<uint8_t> coverage;		// nB * nRows, row after row from the top
};

// What a face was fitted to, and the metrics the engine lays text out by
struct SFaceMetrics
{
	int nCellHeight = 0;
	int nAscent = 0;
	int nDescent = 0;
	int nExternalLeading = 0;
	int nAveCharWidth = 0;
	int nMaxCharWidth = 0;
	double fPixelsPerEm = 0;
};

class CFace
{
	struct SImpl;
	std::unique_ptr<SImpl> pImpl;
	CFace();
public:
	~CFace();
	CFace( const CFace& ) = delete;
	CFace& operator=( const CFace& ) = delete;

	// From a file. nFaceIndex as SOptions::nFaceIndex takes it.
	static std::unique_ptr<CFace> OpenFile( const std::string &szFile, int nFaceIndex, std::string *pszError );
	// From bytes already in memory, which the face keeps alive for as long as it
	// lives; this is how a font in the game's paks is opened.
	static std::unique_ptr<CFace> OpenMemory( std::shared_ptr<const std::vector<uint8_t>> data, int nFaceIndex, std::string *pszError );

	// Sizes the face so that its cell is exactly options.nCellHeight pixels,
	// measured the way options.eCellMetrics says. For CELL_INK, sizingCodePoints
	// are the characters whose ink the cell must hold, and they are rasterised
	// here to place the baseline. Rendering options (hinting, antialiasing,
	// gamma) are taken from options too. Must be called before any glyph is.
	bool Fit( const SOptions &options, const std::vector<uint32_t> &sizingCodePoints, std::string *pszError );
	const SFaceMetrics &GetMetrics() const;

	// Whether the face has a glyph of its own for nCodePoint, rather than .notdef
	bool HasGlyph( uint32_t nCodePoint ) const;
	// Rasterises one character at the fitted size
	bool RenderGlyph( uint32_t nCodePoint, SGlyphBitmap *pGlyph, std::string *pszError );
	// Pixels to add to the pen between two characters, from the kern table, or
	// from GPOS pair kerning for a face without one (GposKerning.h);
	// 0 when either is missing or the pair is not kerned
	int GetKerning( uint32_t nLeft, uint32_t nRight ) const;
	bool HasKerning() const;
};

}

#pragma once

// Fonts rasterised by the game itself, at exactly the pixel size the UI asks for.
//
// A font record with a FontFile is not drawn from its baked Texture and metrics.
// Each size the UI requests gets its own CGlyphAtlas, which opens the font from
// the game data with FreeType (through FontRaster), fits it to that size and
// rasterises each character the first time some text uses it, so text draws
// texel for pixel at any resolution. Characters the font lacks come from the
// record's FallbackFontFiles, then from fonts installed on the system, and
// otherwise show the font's .notdef box. See docs/font-rendering-blur.md.
//
// Text layouts keep the atlas coordinates of their glyphs, so a glyph never
// moves once placed: the atlas only fills up. A save game keeps the atlases its
// text refers to as the font record, the size and the characters in the order
// they were added, and rebuilding from those puts every glyph back where it
// was, since placement depends on nothing else.

#include "GLocale.h"
#include "FontFormat.h"
#include "GfxBuffers.h"
#include "System/Dg.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace NDb
{
	struct SFont;
}

namespace NGScene
{

// One font record at one cell height: the glyphs rasterised so far, their
// metrics, and the atlas texture they are drawn from
class CGlyphAtlas : public CPtrFuncBase<NGfx::CTexture>
{
	OBJECT_NOCOPY_METHODS( CGlyphAtlas );
	struct SState;
	std::unique_ptr<SState> pState;
	// what a save game keeps, and all a load needs to rebuild
	const NDb::SFont *pRecord;
	int nCellHeight;
	int nCellWidth = 0;
	// Absent in older saves: rebuild those atlases with their original sizing,
	// since saved text already holds their glyph coordinates.
	bool bMatchBakedSize = false;
	std::vector<uint16_t> order;					// characters in the order they were placed

	bool Rebuild();
	bool AddGlyph( uint16_t wChar );
protected:
	bool NeedUpdate();
	void Recalc();
public:
	CGlyphAtlas();
	~CGlyphAtlas();

	// Matches the baked font's visible dimensions at the requested pixel size;
	// the raster cell can be taller to retain the replacement's accents.
	// False when the font file cannot be used, so the baked font answers.
	bool Init( const NDb::SFont *pRecord, int nCellHeight, int nCellWidth = 0, bool bMatchBakedSize = true );
	// Makes sure every character of wsText has a glyph, and every adjacent pair
	// its kerning, before text is laid out with them
	void Prepare( const std::wstring &wsText );
	CFontFormatInfo *GetFormat();

	int operator&( IBinSaver &saver );
};

// The atlas's metrics as the node CFontInfo reads them from
class CRuntimeFontFormat : public CPtrFuncBase<CFontFormatInfo>
{
	OBJECT_NOCOPY_METHODS( CRuntimeFontFormat );
	CObj<CGlyphAtlas> pAtlas;
protected:
	void Recalc();
public:
	CRuntimeFontFormat() {}
	explicit CRuntimeFontFormat( CGlyphAtlas *_pAtlas ) : pAtlas( _pAtlas ) {}
	int operator&( IBinSaver &saver );
};

// A CFontInfo whose glyphs are made on demand. Never saved: CTextLocaleInfo
// keeps these outside the font list it saves, and text layout holds a font only
// while it generates. What saved text keeps is the atlas, its texture.
class CRuntimeFontInfo : public CFontInfo
{
	OBJECT_NOCOPY_METHODS( CRuntimeFontInfo );
	CObj<CGlyphAtlas> pAtlas;
public:
	CRuntimeFontInfo() {}
	CRuntimeFontInfo( const SFont &sFont, CGlyphAtlas *pAtlas );
	void PrepareGlyphs( const std::wstring &wsText ) override;
	bool IsRasterized() const override { return true; }
};

}

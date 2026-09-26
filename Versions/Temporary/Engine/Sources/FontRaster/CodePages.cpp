#include "CodePages.h"

namespace NCodePages
{

namespace
{

struct SCodePage
{
	int nCharset;
	int nCodePage;
	const uint16_t *pUpperHalf;			// bytes 0x80..0xFF
};

#include "CodePageTables.inc"

const SCodePage *FindCodePage( int nCharset )
{
	// TranslateCharsetInfo has no code page for DEFAULT, MAC or OEM, and FontGen
	// then fell back to the build machine's own ANSI code page, so a bake
	// depended on the locale of whoever ran it. A portable tool has no such
	// thing to fall back to, and a machine-dependent result is not worth
	// reproducing anyway, so those three read as 1252, which is ANSI_CHARSET.
	if ( nCharset == CHARSET_DEFAULT || nCharset == CHARSET_MAC || nCharset == CHARSET_OEM )
	{
		nCharset = CHARSET_ANSI;
	}
	for ( const SCodePage &page : CODE_PAGES )
	{
		if ( page.nCharset == nCharset )
		{
			return &page;
		}
	}
	return nullptr;
}

}

int GetCodePage( int nCharset )
{
	if ( nCharset == CHARSET_SYMBOL )
	{
		return 42;
	}
	const SCodePage *pPage = FindCodePage( nCharset );
	return pPage != nullptr ? pPage->nCodePage : 0;
}

bool Translate( int nCharset, uint8_t nByte, uint32_t *pnCodePoint )
{
	// A symbol font's cmap puts its glyphs at U+F000 + byte, in the private use
	// area, which is where GDI draws them from. Code page 42 refuses every byte
	// under MB_ERR_INVALID_CHARS, so the GDI path baked a SYMBOL_CHARSET font
	// with no characters at all; this is the mapping it should have used.
	if ( nCharset == CHARSET_SYMBOL )
	{
		*pnCodePoint = 0xF000 + nByte;
		return true;
	}
	const SCodePage *pPage = FindCodePage( nCharset );
	if ( pPage == nullptr )
	{
		return false;
	}
	// every code page in the tables is the identity below 0x80, which
	// gen_codepages.py checks
	if ( nByte < 0x80 )
	{
		*pnCodePoint = nByte;
		return true;
	}
	const uint16_t nCodePoint = pPage->pUpperHalf[nByte - 0x80];
	if ( nCodePoint == 0 )
	{
		return false;
	}
	*pnCodePoint = nCodePoint;
	return true;
}

std::vector<uint32_t> GetPrintableCodePoints( int nCharset )
{
	std::vector<uint32_t> codePoints;
	for ( int nByte = 32; nByte < 256; ++nByte )
	{
		uint32_t nCodePoint = 0;
		if ( Translate( nCharset, static_cast<uint8_t>( nByte ), &nCodePoint ) )
		{
			codePoints.push_back( nCodePoint );
		}
	}
	return codePoints;
}

}

#pragma once

#include <cstdint>

namespace NFontGen
{
const int N_LEADING_PIXELS = 2;

class CFontInfo
{
public:
	struct SSourceParams
	{
		std::string szFaceName;
		uint32_t dwCharSet;
		int nWeight;
		int nHeight;
		bool bItalic;
		bool bAntialias;
		uint32_t dwPitch;
	};
private:
	SSourceParams source;							// source params, this font info was created with
	HFONT hFont;											// HFONT used to draw with this font
	TEXTMETRIC tm;										// text metrics, e.g. character height
	std::vector<ABC> abc;									// character ABC widths
	std::vector<KERNINGPAIR> kps;					// kerning pairs
	int nTextureSizeX, nTextureSizeY;	// estimated texture size
	std::unordered_map<uint16_t, uint16_t> translate;		// MBCS => UNICODE translation table
	std::vector<uint16_t> mbcsChars;
	//
	bool EstimateTextureSize( uint32_t dwNumChars );
	bool MeasureFont( HDC hdc, std::vector<uint16_t> *pChars );
public:
	CFontInfo() : hFont( 0 ), nTextureSizeX( 0 ), nTextureSizeY( 0 ) {  }
	~CFontInfo() { if ( hFont ) DeleteObject( hFont ); }
	//
	bool LoadFontInfo( const SSourceParams &_source, std::vector<uint16_t> *pChars, HWND hWnd );
	// MBCS => UNICODE
	uint16_t Translate( uint16_t code ) const;
	//
	HFONT GetFont() const { return hFont; }
	const TEXTMETRIC &GetTextMetrics() const { return tm; }
	const std::vector<ABC> &GetABC() const { return abc; }
	const std::vector<KERNINGPAIR> &GetKerningPairs() const { return kps; }
	const std::vector<uint16_t> &GetMBCSChars() const { return mbcsChars; }
	CTPoint<int> GetTextureSize() const { return CTPoint<int>(nTextureSizeX, nTextureSizeY); }
};

}

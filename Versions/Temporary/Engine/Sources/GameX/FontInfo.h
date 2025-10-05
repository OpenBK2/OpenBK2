#pragma once

namespace NFontGen
{
const int N_LEADING_PIXELS = 2;

class CFontInfo
{
public:
	struct SSourceParams
	{
		std::string szFaceName;
		DWORD dwCharSet;
		int nWeight;
		int nHeight;
		bool bItalic;
		bool bAntialias;
		DWORD dwPitch;
	};
private:
	SSourceParams source;							// source params, this font info was created with
	HFONT hFont;											// HFONT used to draw with this font
	TEXTMETRIC tm;										// text metrics, e.g. character height
	std::vector<ABC> abc;									// character ABC widths
	std::vector<KERNINGPAIR> kps;					// kerning pairs
	int nTextureSizeX, nTextureSizeY;	// estimated texture size
	std::unordered_map<WORD, WORD> translate;		// MBCS => UNICODE translation table
	std::vector<WORD> mbcsChars;
	//
	bool EstimateTextureSize( DWORD dwNumChars );
	bool MeasureFont( HDC hdc, std::vector<WORD> *pChars );
public:
	CFontInfo() : hFont( 0 ), nTextureSizeX( 0 ), nTextureSizeY( 0 ) {  }
	~CFontInfo() { if ( hFont ) DeleteObject( hFont ); }
	//
	bool LoadFontInfo( const SSourceParams &_source, std::vector<WORD> *pChars, HWND hWnd );
	// MBCS => UNICODE
	WORD Translate( WORD code ) const;
	//
	HFONT GetFont() const { return hFont; }
	const TEXTMETRIC &GetTextMetrics() const { return tm; }
	const std::vector<ABC> &GetABC() const { return abc; }
	const std::vector<KERNINGPAIR> &GetKerningPairs() const { return kps; }
	const std::vector<WORD> &GetMBCSChars() const { return mbcsChars; }
	CTPoint<int> GetTextureSize() const { return CTPoint<int>(nTextureSizeX, nTextureSizeY); }
};

}

#pragma once

namespace NFontGen
{
const int N_LEADING_PIXELS = 2;

class CFontInfo
{
public:
	struct SSourceParams
	{
		string szFaceName;
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
	vector<ABC> abc;									// character ABC widths
	vector<KERNINGPAIR> kps;					// kerning pairs
	int nTextureSizeX, nTextureSizeY;	// estimated texture size
	hash_map<WORD, WORD> translate;		// MBCS => UNICODE translation table
	vector<WORD> mbcsChars;
	//
	bool EstimateTextureSize( DWORD dwNumChars );
	bool MeasureFont( HDC hdc, vector<WORD> *pChars );
public:
	CFontInfo() : hFont( 0 ), nTextureSizeX( 0 ), nTextureSizeY( 0 ) {  }
	~CFontInfo() { if ( hFont ) DeleteObject( hFont ); }
	//
	bool LoadFontInfo( const SSourceParams &_source, vector<WORD> *pChars, HWND hWnd );
	// MBCS => UNICODE
	WORD Translate( WORD code ) const;
	//
	HFONT GetFont() const { return hFont; }
	const TEXTMETRIC &GetTextMetrics() const { return tm; }
	const vector<ABC> &GetABC() const { return abc; }
	const vector<KERNINGPAIR> &GetKerningPairs() const { return kps; }
	const vector<WORD> &GetMBCSChars() const { return mbcsChars; }
	CTPoint<int> GetTextureSize() const { return CTPoint<int>(nTextureSizeX, nTextureSizeY); }
};

}

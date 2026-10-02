#pragma once

#include <cstdint>

#pragma pack( 4 )
// complete necessary one letter description
struct STFCharacter
{
  int x1, y1, x2, y2;                 // rect in texture's coords [0..1]
  int nA;                             // character's pre-space
  int nBC;                            // character's B + C = distance to the next character
  int nWidth;                         // lone character's width (B + (C > 0 ? C : 0))
};
#pragma pack()

class CFontFormatInfo: public CObjectBase
{
	OBJECT_BASIC_METHODS( CFontFormatInfo );
	typedef std::unordered_map<uint16_t, STFCharacter> CCharacterMap;
	typedef std::unordered_map<uint32_t, int> CKernMap;
	//
  CCharacterMap chars;                  // all available characters map
  CKernMap kerns;                       // kerning pairs for the characters in the font.
	// Runtime ink, relative to the glyph cell. Keep STFCharacter unchanged:
	// its full-cell rectangle is also the legacy baked font file format.
	std::unordered_map<uint16_t, CTRect<int>> inkBounds;
	//
	int nHeight;													// native height of this font (in native pixels!)
	int nExternalLeading;									// extra leading (space) that the application adds between rows
  int nAveCharWidth;										// average width of characters in the font (generally defined as the width of the letter x).
  int nMaxCharWidth;										// width of the widest character in the font
  uint8_t cCharSet;                        // character set of the font
	uint16_t wDefaultChar;										// value of the character to be substituted for characters not in the font
public:
	// retrieve character description
  const STFCharacter& GetChar( const uint16_t c ) const
	{
		CCharacterMap::const_iterator pos = chars.find( c );
		if ( pos == chars.end() )
		{
			pos = chars.find( wDefaultChar );
			if ( pos == chars.end() )
			{
//				NI_ASSERT( false, StrFmt("Can't find neither target character (0x%.2x) nor default one (0x%.2x)", (unsigned int)(c), (unsigned int)(wDefaultChar)) );
				return chars.begin()->second;
			}
		}
		return pos->second;
	}
	// retrieve kerning pair width
	int GetKern( uint16_t wChar, uint16_t wLastChar ) const
	{
    CKernMap::const_iterator pos = kerns.find( (uint32_t(wLastChar) << 16) | uint32_t(wChar) );
		return pos != kerns.end() ? pos->second : 0;
	}
	//
	int GetHeight() const { return nHeight; }
	int GetLineSpace() const { return nHeight + nExternalLeading; }
	int GetAveCharWidth() const { return nAveCharWidth; }
	int GetMaxCharWidth() const { return nMaxCharWidth; }
	//
	// For runtime fonts (GRuntimeFont.h), which start empty and gain a character
	// the first time some text uses it. A baked font is loaded whole and never
	// calls these. References GetChar handed out stay valid as characters are
	// added, the map being node based.
	void SetMetrics( int _nHeight, int _nExternalLeading, int _nAveCharWidth, int _nMaxCharWidth, uint8_t _cCharSet, uint16_t _wDefaultChar )
	{
		nHeight = _nHeight;
		nExternalLeading = _nExternalLeading;
		nAveCharWidth = _nAveCharWidth;
		nMaxCharWidth = _nMaxCharWidth;
		cCharSet = _cCharSet;
		wDefaultChar = _wDefaultChar;
	}
	bool HasChar( const uint16_t c ) const { return chars.find( c ) != chars.end(); }
	void SetChar( const uint16_t c, const STFCharacter &character ) { chars[c] = character; }
	void SetInkBounds( uint16_t c, const CTRect<int> &bounds ) { inkBounds[c] = bounds; }
	const CTRect<int>* GetInkBounds( uint16_t c ) const
	{
		// Match GetChar's replacement glyph when a character is unavailable.
		if ( !HasChar( c ) )
			c = HasChar( wDefaultChar ) ? wDefaultChar : ( chars.empty() ? c : chars.begin()->first );
		const auto found = inkBounds.find( c );
		return found == inkBounds.end() ? nullptr : &found->second;
	}
	bool HasKern( uint16_t wChar, uint16_t wLastChar ) const { return kerns.find( (uint32_t(wLastChar) << 16) | uint32_t(wChar) ) != kerns.end(); }
	void SetKern( uint16_t wChar, uint16_t wLastChar, int nKern ) { kerns[(uint32_t(wLastChar) << 16) | uint32_t(wChar)] = nKern; }
	//
	int operator&( CStructureSaver &f );
};



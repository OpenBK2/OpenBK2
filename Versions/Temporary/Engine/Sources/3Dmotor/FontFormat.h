#pragma once

class CFontFormatInfo;
namespace NFontGen
{
	class CFontInfo;
	void CreateFontFormat( CFontFormatInfo *pRes, const CFontInfo &fi );
}

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
	typedef std::unordered_map<WORD, STFCharacter> CCharacterMap;
	typedef std::unordered_map<DWORD, int> CKernMap;
	//
  CCharacterMap chars;                  // all available characters map
  CKernMap kerns;                       // kerning pairs for the characters in the font.
	//
	int nHeight;													// native height of this font (in native pixels!)
	int nExternalLeading;									// extra leading (space) that the application adds between rows
  int nAveCharWidth;										// average width of characters in the font (generally defined as the width of the letter x).
  int nMaxCharWidth;										// width of the widest character in the font
  BYTE cCharSet;                        // character set of the font
	WORD wDefaultChar;										// value of the character to be substituted for characters not in the font
public:
	// retrieve character description
  const STFCharacter& GetChar( const WORD c ) const
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
	int GetKern( WORD wChar, WORD wLastChar ) const
	{
    CKernMap::const_iterator pos = kerns.find( (DWORD(wLastChar) << 16) | DWORD(wChar) );
		return pos != kerns.end() ? pos->second : 0;
	}
	//
	int GetHeight() const { return nHeight; }
	int GetLineSpace() const { return nHeight + nExternalLeading; }
	int GetAveCharWidth() const { return nAveCharWidth; }
	int GetMaxCharWidth() const { return nMaxCharWidth; }
	//
	int operator&( CStructureSaver &f );
	//
	friend void NFontGen::CreateFontFormat( CFontFormatInfo *pRes, const NFontGen::CFontInfo &fi );
};



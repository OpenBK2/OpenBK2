#pragma once

#include <cstdint>

#include "System/det_map.h"

#include <fmt/format.h>

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
	// det_map rather than the std::unordered_map the engine's copy of this class
	// uses, because this is the side that writes. FontFormat.cpp serializes both
	// containers whole, so their iteration order is the byte order of the blob,
	// and unordered_map does not promise the same order across implementations.
	// det_map keeps insertion order, so a rebaked font is reproducible.
	//
	// Not std::map, which looks like the obvious answer and is not: BinSaver has
	// Add() overloads for unordered_map and det_map and none for it, so the call
	// silently resolves elsewhere and writes nothing. BinSaver keeps det_map's
	// chunk compatible with unordered_map saves, so the engine reads it as it
	// always has.
	typedef det_map<uint16_t, STFCharacter> CCharacterMap;
	typedef det_map<uint32_t, int> CKernMap;
	//
  CCharacterMap chars;                  // all available characters map
  CKernMap kerns;                       // kerning pairs for the characters in the font.
	//
	int nHeight;													// native height of this font (in native pixels!)
	int nExternalLeading;									// extra leading (space) that the application adds between rows
  int nAveCharWidth;										// average width of characters in the font (generally defined as the width of the letter x).
  int nMaxCharWidth;										// width of the widest character in the font
  uint8_t cCharSet;                        // character set of the font
	uint16_t wDefaultChar;										// alue of the character to be substituted for characters not in the font

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
				NI_ASSERT( false, fmt::format("Can't find neither target character (0x{:02x}) nor default one (0x{:02x})", (unsigned int)(c), (unsigned int)(wDefaultChar)) );
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
	int operator&( CStructureSaver &f );
	friend class CFontGen;
};



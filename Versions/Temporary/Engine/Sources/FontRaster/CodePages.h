#pragma once

// The Windows charsets FontGen takes and the Unicode code points their bytes
// stand for, without asking Windows.
//
// FontGen bakes single code page bytes, 32..255 by default, and the engine
// looks glyphs up by code point, so every byte is translated first. That used
// to be TranslateCharsetInfo and MultiByteToWideChar, which kept FontGen on
// Windows. The tables in CodePageTables.inc are Windows' own answers to the
// same calls, recorded by gen_codepages.py, so a bake translates identically on
// any platform.

#include <cstdint>
#include <vector>

namespace NCodePages
{

// Windows' *_CHARSET values. They are also what the font blob stores as its
// character set, so they keep Windows' numbers.
enum ECharset
{
	CHARSET_ANSI = 0,
	CHARSET_DEFAULT = 1,
	CHARSET_SYMBOL = 2,
	CHARSET_MAC = 77,
	CHARSET_SHIFTJIS = 128,
	CHARSET_HANGUL = 129,
	CHARSET_GB2312 = 134,
	CHARSET_CHINESEBIG5 = 136,
	CHARSET_GREEK = 161,
	CHARSET_TURKISH = 162,
	CHARSET_HEBREW = 177,
	CHARSET_ARABIC = 178,
	CHARSET_BALTIC = 186,
	CHARSET_RUSSIAN = 204,
	CHARSET_THAI = 222,
	CHARSET_EASTEUROPE = 238,
	CHARSET_OEM = 255,
};

// The code page nCharset's bytes are read in, for reporting
int GetCodePage( int nCharset );

// The code point byte nByte stands for in nCharset. Returns false for a byte
// the code page does not define, which FontGen then leaves out, as it did when
// MultiByteToWideChar refused it.
bool Translate( int nCharset, uint8_t nByte, uint32_t *pnCodePoint );

// The code points of bytes 32..255 in nCharset that it defines, in byte order:
// the character set FontGen bakes by default, and what the game's runtime fonts
// size their cell by
std::vector<uint32_t> GetPrintableCodePoints( int nCharset );

}

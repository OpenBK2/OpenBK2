#include "stdafx.h"
#include "FontFormat.h"
#include "Image/Targa.h"
#include "Misc/2Darray.h"
#include "Misc/StrProc.h"
#include <algorithm>
#include "System/FileUtils.h"
#include "CodePages.h"
#include "FontFinder.h"
#include "FontRaster.h"

#include "port/cdecl.h"

#include <cstdint>

#include <fmt/format.h>

// FontGen bakes a font into the two files the engine reads for it: a glyph atlas,
// written as a TGA, and a binary blob of metrics, one CFontFormatInfo. The
// rasteriser is FontRaster, which is FreeType; this file turns FontGen's command
// line, its character set and its output formats into a FontRaster call.
//
// It used to rasterise with GDI (CreateFont and TextOut into a DIB), which kept
// it on Windows and could not produce every cell height. The command line is the
// one the GDI tool had, because ED_Common/FontExporter.cpp still builds it: the
// face is named the GDI way ("Tahoma", -w400, -it) and found among the installed
// fonts by FontFinder, or given directly as a font file.

namespace NImage
{
	typedef CArray2D<CVec4> CImage;
}

// CFontGen

class CFontGen
{
public:
	static void CreateFontFormat( const char *pszDestFile, const NFontRaster::SFont &font, uint8_t cCharSet, uint16_t wDefaultChar );
};

// FontRaster already works in the conventions CFontFormatInfo has always used,
// GDI's cell and ABC widths, so this is a field for field copy, with the
// characters keyed by code point as the engine looks them up.
void CFontGen::CreateFontFormat( const char *pszDestFile, const NFontRaster::SFont &font, uint8_t cCharSet, uint16_t wDefaultChar )
{
	CObj<CFontFormatInfo> pFormat( new CFontFormatInfo );
	CFontFormatInfo &format = *pFormat;
	format.nHeight = font.nCellHeight;
	format.nExternalLeading = font.nExternalLeading;
	format.nAveCharWidth = font.nAveCharWidth;
	format.nMaxCharWidth = font.nMaxCharWidth;
	format.cCharSet = cCharSet;
	format.wDefaultChar = wDefaultChar;
	for ( const NFontRaster::SKerningPair &pair : font.kerns )
	{
		format.kerns[( pair.nLeft << 16 ) | pair.nRight] = pair.nAmount;
	}
	for ( const NFontRaster::SGlyph &glyph : font.glyphs )
	{
		STFCharacter &character = format.chars[static_cast<uint16_t>( glyph.nCodePoint )];
		character.x1 = glyph.x1;
		character.y1 = glyph.y1;
		character.x2 = glyph.x2;
		character.y2 = glyph.y2;
		character.nA = glyph.nA;
		character.nBC = glyph.nBC;
		character.nWidth = glyph.nWidth;
	}
	CFileStream file( pszDestFile, CFileStream::WIN_CREATE );
	if ( file.IsOk() )
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &file, SAVER_MODE_WRITE );
		pSaver->Add( 1, &pFormat );
	}
}

// GDI's tmDefaultChar for every TrueType face measured (Tahoma, Impact and Arial,
// RUSSIAN_CHARSET), and so the code the default glyph has always been stored
// under. No font maps it, so it gets .notdef, the box GDI drew.
const uint16_t W_DEFAULT_CHAR = 0x1F;

// Bakes chars, code page bytes in nCharset, with options into the two output
// files. Returns false when the font cannot be rasterised.
bool Generate( const char *pszDstPngFile, const char *pszDstFile, const NFontRaster::SOptions &options,
	int nCharset, std::vector<uint16_t> *pChars )
{
	std::vector<uint16_t> &chars = *pChars;
	if ( find( chars.begin(), chars.end(), W_DEFAULT_CHAR ) == chars.end() )
		chars.push_back( W_DEFAULT_CHAR );
	sort( chars.begin(), chars.end() );
	// The engine looks glyphs up by code point, so the bytes are translated first;
	// a byte the code page does not define has no code point to be stored under
	// and is left out. The default character is not a byte of any code page but
	// the key GDI filed the default glyph under, so it goes in as it is.
	std::vector<uint32_t> codePoints;
	codePoints.reserve( chars.size() );
	int nUnmapped = 0;
	for ( const uint16_t nChar : chars )
	{
		uint32_t nCodePoint = 0;
		if ( nChar == W_DEFAULT_CHAR )
			codePoints.push_back( W_DEFAULT_CHAR );
		else if ( nChar <= 0xFF && NCodePages::Translate( nCharset, static_cast<uint8_t>( nChar ), &nCodePoint ) )
			codePoints.push_back( nCodePoint );
		else
			++nUnmapped;
	}
	if ( nUnmapped > 0 )
	{
		// a double byte character from a characters file lands here too: the
		// tables hold single bytes, which is all the translation ever handled
		fmt::print( "WARNING: {} of {} characters have no mapping in code page {}\n",
		        nUnmapped, static_cast<int>( chars.size() ), NCodePages::GetCodePage( nCharset ) );
	}

	NFontRaster::SFont font;
	std::string szError;
	if ( !NFontRaster::Rasterise( options, codePoints, &font, &szError ) )
	{
		fmt::print( "ERROR: {}\n", szError );
		return false;
	}
	fmt::print( "cell {} = ascent {} + descent {}, external leading {}, average width {}, max width {}, {:.3f} px per em\n",
	        font.nCellHeight, font.nAscent, font.nDescent, font.nExternalLeading, font.nAveCharWidth, font.nMaxCharWidth,
	        font.fPixelsPerEm );
	fmt::print( "{} glyphs, {} kerning pairs, atlas {}x{}\n", static_cast<int>( font.glyphs.size() ),
	        static_cast<int>( font.kerns.size() ), font.nAtlasWidth, font.nAtlasHeight );
	// .notdef for the default character is expected; anything more is a
	// character set the font does not cover
	for ( const uint32_t nCodePoint : font.missing )
	{
		if ( nCodePoint != W_DEFAULT_CHAR )
			fmt::print( "WARNING: U+{:04X} is not in the font and got its .notdef glyph\n", static_cast<unsigned>( nCodePoint ) );
	}
	for ( const uint32_t nCodePoint : font.clipped )
		fmt::print( "WARNING: U+{:04X} reaches outside its cell and was clipped to it\n", static_cast<unsigned>( nCodePoint ) );

	fmt::print( "image...\n" );
	// white, with the coverage as alpha
	NImage::CImage image;
	image.SetSizes( font.nAtlasWidth, font.nAtlasHeight );
	for ( int y = 0; y < font.nAtlasHeight; ++y )
	{
		for ( int x = 0; x < font.nAtlasWidth; ++x )
			image[y][x] = CVec4( 1, 1, 1, font.atlas[static_cast<size_t>( y ) * font.nAtlasWidth + x] / 255.0f );
	}
	CFileStream stream( pszDstPngFile, CFileStream::WIN_CREATE );
	if ( stream.IsOk() )
	{
		CArray2D<uint32_t> image2( image.GetSizeX(), image.GetSizeY() );
		NImage::Convert( &image2, image );
		NImage::SaveAsTGA( image2, &stream );
	}

	fmt::print( "font data...\n" );
	CFontGen::CreateFontFormat( pszDstFile, font, static_cast<uint8_t>( nCharset ), W_DEFAULT_CHAR );
	fmt::print( "well done\n" );
	return true;
}

#if !defined(ELK)

static void ShowUsage()
{
	fmt::print( "FontGenerator utility\n(C) Nival Interactive, 2000\n" );
	fmt::print( "Usage: FontGen [options] <\"Font Face Name\" | font file> <BinDstName> <PicDstName> [<CharsSrcName>]\n" );
	fmt::print( "   -h# \t\t cell height in pixels, exactly\n" );
	fmt::print( "   -w# \t\t font weight (400 = normal. 100 <= w <= 900), for finding the face\n" );
	fmt::print( "   -it \t\t italic, for finding the face\n" );
	fmt::print( "   -aa \t\t antialiased\n" );
	fmt::print( "   -<charset>\t character set of the characters baked\n" );
	fmt::print( "    charsets: ansi, baltic, chinesebig5, default, easteurope, gb2312,\n" );
	fmt::print( "              greek, hangul, mac, oem, russian, shiftjis, symbol,\n" );
	fmt::print( "              turkish, hebrew, arabic, thai\n" );
	fmt::print( "   [<CharsSrcName>] chars in MBCS formart, all in doublebytes (words)\n" );
	fmt::print( "\n" );
	fmt::print( "   The face is looked up by family name among the installed fonts and any\n" );
	fmt::print( "   -fontdir, the closest to -w and -it winning; a path to an existing\n" );
	fmt::print( "   font file is used as it is.\n" );
	fmt::print( "   -fontdir=<dir>\t also search this directory, recursively; repeatable\n" );
	fmt::print( "   -hint=<mode>\t hinting: none, light (default) or normal\n" );
	fmt::print( "   -gamma=<g>\t coverage gamma, default 1; above 1 thickens edges\n" );
	fmt::print( "   -pad=<n>\t blank pixels between atlas cells, default 2\n" );
	fmt::print( "   -cell=<m>\t what -h is fitted to: win (default), usWinAscent + usWinDescent\n" );
	fmt::print( "       \t\t as GDI did, or ink, the tallest and deepest of the characters\n" );
	fmt::print( "       \t\t baked, which sizes fonts made for many scripts comparably\n" );
	fmt::print( "   -instance=<n>\t a variable font's named instance, counted from 1\n" );
	fmt::print( "   -ft, -default, -fixed, -variable\t accepted for older command lines; no effect\n" );
}

int PORT_CDECL main( int argc, char *argv[] )
{
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
	// Options are matched in lower case, as they always were. The file names are
	// kept as given: lower-casing them was harmless on Windows and names a file
	// that does not exist anywhere else.
	std::vector<std::string> szParams( argc - 1 );
	for ( int i=0; i<argc - 1; ++i )
	{
		szParams[i] = argv[i + 1];
		if ( !szParams[i].empty() && szParams[i][0] == '-' )
			NStr::ToLower( &szParams[i] );
		//
		if ( szParams[i] == "-show-version" )
		{
			fmt::print( "Version: {}\n", REVISION_NUMBER_STR );
			fmt::print( "Build date/time: {}\n", BUILD_DATE_TIME_STR );
			return 0;
		}
	}
	//
	if ( szParams.empty() )
	{
		ShowUsage();
		return 0xDEAD;
	}
	std::unordered_map<std::string, int> charsets;
	charsets["-ansi"]        = NCodePages::CHARSET_ANSI;
	charsets["-baltic"]      = NCodePages::CHARSET_BALTIC;
	charsets["-chinesebig5"] = NCodePages::CHARSET_CHINESEBIG5;
	charsets["-default"]     = NCodePages::CHARSET_DEFAULT;
	charsets["-def_charset"] = NCodePages::CHARSET_DEFAULT;
	charsets["-easteurope"]  = NCodePages::CHARSET_EASTEUROPE;
	charsets["-gb2312"]      = NCodePages::CHARSET_GB2312;
	charsets["-greek"]       = NCodePages::CHARSET_GREEK;
	charsets["-hangul"]      = NCodePages::CHARSET_HANGUL;
	charsets["-mac"]         = NCodePages::CHARSET_MAC;
	charsets["-oem"]         = NCodePages::CHARSET_OEM;
	charsets["-russian"]     = NCodePages::CHARSET_RUSSIAN;
	charsets["-shiftjis"]    = NCodePages::CHARSET_SHIFTJIS;
	charsets["-symbol"]      = NCodePages::CHARSET_SYMBOL;
	charsets["-turkish"]     = NCodePages::CHARSET_TURKISH;
	charsets["-hebrew"]      = NCodePages::CHARSET_HEBREW;
	charsets["-arabic"]      = NCodePages::CHARSET_ARABIC;
	charsets["-thai"]        = NCodePages::CHARSET_THAI;

	int nHeight = 20;
	int nWeight = 400;
	bool bItalic = false;
	bool bAntialias = false;
	int nCharset = NCodePages::CHARSET_ANSI;
	std::string szFaceName = "Times New Roman", szDstFile, szDstPngFile, szCharsSrcName;
	int nOrdinaryParamCount = 0;
	NFontRaster::SOptions options;
	std::vector<std::string> fontDirectories;
	// -h20 -w400 -it -russian -aa "Times New Roman"
	for ( std::vector<std::string>::const_iterator pos = szParams.begin(); pos != szParams.end(); ++pos )
	{
		// The GDI rasteriser's switch, and its pitch, which chose among faces;
		// both are still accepted so that older command lines keep working.
		// "-default" is taken as the charset below, as it always was.
		if ( *pos == "-ft" || *pos == "-fixed" || *pos == "-variable" )
			continue;
		// before the -h test below, which would otherwise take -hint=... for a height
		if ( pos->find( "-hint=" ) == 0 )
		{
			const std::string szMode = pos->substr( 6 );
			if ( szMode == "none" )
				options.eHinting = NFontRaster::HINTING_NONE;
			else if ( szMode == "light" )
				options.eHinting = NFontRaster::HINTING_LIGHT;
			else if ( szMode == "normal" )
				options.eHinting = NFontRaster::HINTING_NORMAL;
			else
			{
				fmt::print( "ERROR: unknown hinting mode \"{}\"\n", szMode );
				return 0xDEAD;
			}
			continue;
		}
		if ( pos->find( "-gamma=" ) == 0 )
		{
			options.fGamma = static_cast<float>( atof( pos->c_str() + 7 ) );
			continue;
		}
		if ( pos->find( "-pad=" ) == 0 )
		{
			options.nPadding = atoi( pos->c_str() + 5 );
			continue;
		}
		if ( pos->find( "-cell=" ) == 0 )
		{
			const std::string szMode = pos->substr( 6 );
			if ( szMode == "win" )
				options.eCellMetrics = NFontRaster::CELL_WIN;
			else if ( szMode == "ink" )
				options.eCellMetrics = NFontRaster::CELL_INK;
			else
			{
				fmt::print( "ERROR: unknown cell metrics \"{}\"\n", szMode );
				return 0xDEAD;
			}
			continue;
		}
		if ( pos->find( "-instance=" ) == 0 )
		{
			// a variable font's named instance, counted from 1, in the high half
			// of the face index as FreeType takes it
			options.nFaceIndex = ( options.nFaceIndex & 0xFFFF ) | ( atoi( pos->c_str() + 10 ) << 16 );
			continue;
		}
		if ( pos->find( "-fontdir=" ) == 0 )
		{
			// from argv rather than *pos, which is lower case
			fontDirectories.push_back( argv[1 + ( pos - szParams.begin() )] + 9 );
			continue;
		}
		if ( charsets.find(*pos) != charsets.end() )
			nCharset = charsets[*pos];
		else if ( pos->find( "-h" ) == 0 )
			nHeight = atoi( &((*pos)[2]) );
		else if ( pos->find( "-w" ) == 0 )
			nWeight = atoi( &((*pos)[2]) );
		else if ( *pos == "-it" )
			bItalic = true;
		else if ( *pos == "-aa" )
			bAntialias = true;
		else if ( !pos->empty() && ( *pos )[0] == '-' )
		{
			fmt::print( "ERROR: unknown option \"{}\"\n", *pos );
			return 0xDEAD;
		}
		else
		{
			nOrdinaryParamCount++;
			switch( nOrdinaryParamCount )
			{
				case 1: szFaceName = *pos; break;
				case 2: szDstFile = *pos; break;
				case 3: szDstPngFile = *pos; break;
				case 4: szCharsSrcName = *pos; break;
			}
		}
	}
	if ( nOrdinaryParamCount < 3 )
	{
		ShowUsage();
		return 0xDEAD;
	}
	//
	NStr::TrimInside( szFaceName, '"' );
	NStr::TrimInside( szDstFile, '"' );
	NStr::TrimInside( szDstPngFile, '"' );
	NStr::TrimInside( szCharsSrcName, '"' );
	//
	fmt::print( "generating font \"{}\" ({}:{}:{:d}:{:d})\n", szFaceName, nHeight, nWeight, bItalic, bAntialias );

	// A font file given directly is used as it is; otherwise the name is a
	// family, found among the installed fonts and -fontdir
	if ( NFile::DoesFileExist( szFaceName ) )
	{
		options.szFontFile = szFaceName;
	}
	else
	{
		for ( const std::string &szDirectory : NFontRaster::GetSystemFontDirectories() )
			fontDirectories.push_back( szDirectory );
		NFontRaster::SFontMatch match;
		if ( !NFontRaster::FindFont( szFaceName, nWeight, bItalic, fontDirectories, &match ) )
		{
			fmt::print( "ERROR: no font file with the family name \"{}\" in:\n", szFaceName );
			for ( const std::string &szDirectory : fontDirectories )
				fmt::print( "   {}\n", szDirectory );
			return 1;
		}
		fmt::print( "face \"{} {}\", weight {}{}, from {}{}\n", match.szFamily, match.szStyle, match.nWeight,
		        match.bItalic ? ", italic" : "", match.szFile,
		        ( match.nFaceIndex >> 16 ) != 0 ? fmt::format( " instance {}", match.nFaceIndex >> 16 ) : std::string() );
		// GDI would have synthesised a bold or slanted face from a regular one;
		// FreeType rasterises what the file holds, so say when that differs
		if ( std::abs( match.nWeight - nWeight ) > 150 || match.bItalic != bItalic )
			fmt::print( "WARNING: the closest face installed is not the weight or slant asked for\n" );
		options.szFontFile = match.szFile;
		// an explicit -instance wins over the one the search found
		if ( ( options.nFaceIndex >> 16 ) == 0 )
			options.nFaceIndex = match.nFaceIndex;
	}

	// characters from the chars file, or 32..255 by default
	std::vector<uint16_t> chars;
	if ( !szCharsSrcName.empty() && NFile::DoesFileExist( szCharsSrcName ) )
	{
		CFileStream fileStream( szCharsSrcName, CFileStream::WIN_READ_ONLY );
		if ( fileStream.IsOk() && fileStream.GetSize() > 0 )
		{
			const int nCharCount = fileStream.GetSize() / sizeof( uint16_t );
			if ( nCharCount > 0 )
			{
				chars.resize( nCharCount );
				fileStream.Read( &( chars[0] ), nCharCount * 2 );
				//
				sort( chars.begin(), chars.end() );
				chars.erase( unique( chars.begin(), chars.end() ), chars.end() );
			}
		}
	}
	if ( chars.empty() )
	{
		chars.reserve( 256 );
		for ( int i=32; i<256; ++i )
		{
			chars.push_back( i );
		}
	}
	options.nCellHeight = nHeight;
	options.bAntialias = bAntialias;
	return Generate( szDstPngFile.c_str(), szDstFile.c_str(), options, nCharset, &chars ) ? 0 : 1;
}

#endif //#if !defined(ELK)

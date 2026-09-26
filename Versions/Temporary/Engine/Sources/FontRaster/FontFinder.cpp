#include "FontFinder.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace NFontRaster
{

namespace
{

std::string ToLower( std::string sz )
{
	std::transform( sz.begin(), sz.end(), sz.begin(), []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
	return sz;
}

bool IsFontFile( const std::filesystem::path &path )
{
	const std::string szExtension = ToLower( path.extension().string() );
	return szExtension == ".ttf" || szExtension == ".otf" || szExtension == ".ttc" || szExtension == ".otc";
}

// A named instance carries its weight only in its name ("Bold", "SemiBold"),
// since the OS/2 table is the default instance's. The usual names, longest
// first so that "semibold" is not read as "bold".
int WeightFromStyleName( const std::string &szStyle, int nDefault )
{
	static const struct { const char *pszName; int nWeight; } WEIGHTS[] =
	{
		{ "extralight", 200 }, { "ultralight", 200 }, { "extrabold", 800 }, { "ultrabold", 800 },
		{ "semibold", 600 }, { "demibold", 600 }, { "thin", 100 }, { "light", 300 }, { "medium", 500 },
		{ "bold", 700 }, { "black", 900 }, { "heavy", 900 }, { "regular", 400 }, { "normal", 400 }, { "book", 400 },
	};
	const std::string szLower = ToLower( szStyle );
	for ( const auto &weight : WEIGHTS )
	{
		if ( szLower.find( weight.pszName ) != std::string::npos )
		{
			return weight.nWeight;
		}
	}
	return nDefault;
}

// Describes the face at nFaceIndex in a font FreeType opens through args, a
// file or bytes in memory, or returns false if FreeType cannot open it. szFile
// is only recorded in the result.
bool DescribeFace( const FT_Library pLibrary, const FT_Open_Args &args, const std::string &szFile, const int nFaceIndex,
	SFontMatch *pFace, int *pnNamedInstances, int *pnFaces )
{
	FT_Face pOpened = nullptr;
	if ( FT_Open_Face( pLibrary, &args, nFaceIndex, &pOpened ) != 0 )
	{
		return false;
	}
	pFace->szFile = szFile;
	pFace->nFaceIndex = nFaceIndex;
	pFace->szFamily = pOpened->family_name != nullptr ? pOpened->family_name : "";
	pFace->szStyle = pOpened->style_name != nullptr ? pOpened->style_name : "";
	const TT_OS2 *pOS2 = static_cast<const TT_OS2*>( FT_Get_Sfnt_Table( pOpened, FT_SFNT_OS2 ) );
	const int nTableWeight = ( pOS2 != nullptr && pOS2->version != 0xFFFF && pOS2->usWeightClass != 0 ) ? pOS2->usWeightClass :
		( ( pOpened->style_flags & FT_STYLE_FLAG_BOLD ) != 0 ? 700 : 400 );
	pFace->nWeight = ( nFaceIndex >> 16 ) != 0 ? WeightFromStyleName( pFace->szStyle, nTableWeight ) : nTableWeight;
	pFace->nWidthClass = ( pOS2 != nullptr && pOS2->version != 0xFFFF && pOS2->usWidthClass != 0 ) ? pOS2->usWidthClass : 5;
	pFace->bItalic = ( pOpened->style_flags & FT_STYLE_FLAG_ITALIC ) != 0 ||
		ToLower( pFace->szStyle ).find( "italic" ) != std::string::npos ||
		ToLower( pFace->szStyle ).find( "oblique" ) != std::string::npos;
	*pnNamedInstances = static_cast<int>( pOpened->style_flags >> 16 );
	*pnFaces = static_cast<int>( pOpened->num_faces );
	FT_Done_Face( pOpened );
	return true;
}

}

std::vector<std::string> GetSystemFontDirectories()
{
	std::vector<std::string> candidates;
#if defined( _WIN32 )
	if ( const char *pszWindows = std::getenv( "WINDIR" ) )
	{
		candidates.push_back( std::string( pszWindows ) + "\\Fonts" );
	}
	// fonts installed for the current user only, since Windows 10 1809
	if ( const char *pszLocal = std::getenv( "LOCALAPPDATA" ) )
	{
		candidates.push_back( std::string( pszLocal ) + "\\Microsoft\\Windows\\Fonts" );
	}
#else
	candidates.push_back( "/usr/share/fonts" );
	candidates.push_back( "/usr/local/share/fonts" );
	if ( const char *pszHome = std::getenv( "HOME" ) )
	{
		candidates.push_back( std::string( pszHome ) + "/.local/share/fonts" );
		candidates.push_back( std::string( pszHome ) + "/.fonts" );
	}
#endif
	std::vector<std::string> directories;
	for ( const std::string &szDirectory : candidates )
	{
		std::error_code error;
		if ( std::filesystem::is_directory( szDirectory, error ) )
		{
			directories.push_back( szDirectory );
		}
	}
	return directories;
}

CFontCatalog::CFontCatalog( const std::vector<std::string> &directories )
{
	std::vector<std::string> files;
	for ( const std::string &szDirectory : directories )
	{
		// error codes rather than exceptions throughout: an unreadable
		// subdirectory is skipped, not fatal
		std::error_code error;
		std::filesystem::recursive_directory_iterator it( szDirectory, std::filesystem::directory_options::skip_permission_denied, error );
		for ( ; !error && it != std::filesystem::recursive_directory_iterator(); it.increment( error ) )
		{
			std::error_code fileError;
			if ( it->is_regular_file( fileError ) && IsFontFile( it->path() ) )
			{
				files.push_back( it->path().string() );
			}
		}
	}
	std::sort( files.begin(), files.end() );

	FT_Library pLibrary = nullptr;
	if ( FT_Init_FreeType( &pLibrary ) != 0 )
	{
		return;
	}
	for ( const std::string &szFile : files )
	{
		FT_Open_Args args = {};
		args.flags = FT_OPEN_PATHNAME;
		args.pathname = const_cast<FT_String*>( szFile.c_str() );
		SFontMatch first;
		int nInstances = 0, nFaces = 0;
		if ( !DescribeFace( pLibrary, args, szFile, 0, &first, &nInstances, &nFaces ) )
		{
			continue;
		}
		for ( int nFace = 0; nFace < nFaces; ++nFace )
		{
			SFontMatch face = first;
			if ( nFace != 0 && !DescribeFace( pLibrary, args, szFile, nFace, &face, &nInstances, &nFaces ) )
			{
				continue;
			}
			// the face itself, then each named instance of a variable one
			faces.push_back( face );
			for ( int nInstance = 1; nInstance <= nInstances; ++nInstance )
			{
				SFontMatch instance;
				int nIgnoredInstances = 0, nIgnoredFaces = 0;
				if ( DescribeFace( pLibrary, args, szFile, ( nInstance << 16 ) | nFace, &instance, &nIgnoredInstances, &nIgnoredFaces ) )
				{
					faces.push_back( instance );
				}
			}
		}
	}
	FT_Done_FreeType( pLibrary );
}

bool CFontCatalog::Find( const std::string &szFamily, int nWeight, bool bItalic, SFontMatch *pMatch ) const
{
	const std::string szWanted = ToLower( szFamily );
	bool bFound = false;
	int nBestScore = 0;
	for ( const SFontMatch &candidate : faces )
	{
		if ( ToLower( candidate.szFamily ) != szWanted )
		{
			continue;
		}
		// weights differ by at most 800, so a width class step, at 1000, always
		// outweighs them, and a slant mismatch outweighs the width classes'
		// whole range
		const int nScore = ( candidate.bItalic != bItalic ? 100000 : 0 ) + std::abs( candidate.nWidthClass - 5 ) * 1000 +
			std::abs( candidate.nWeight - nWeight );
		if ( !bFound || nScore < nBestScore )
		{
			*pMatch = candidate;
			nBestScore = nScore;
			bFound = true;
		}
	}
	return bFound;
}

bool FindFont( const std::string &szFamily, int nWeight, bool bItalic, const std::vector<std::string> &directories,
	SFontMatch *pMatch )
{
	return CFontCatalog( directories ).Find( szFamily, nWeight, bItalic, pMatch );
}

int SelectFace( const std::vector<uint8_t> &data, int nWeight, bool bItalic )
{
	FT_Library pLibrary = nullptr;
	if ( data.empty() || FT_Init_FreeType( &pLibrary ) != 0 )
	{
		return 0;
	}
	FT_Open_Args args = {};
	args.flags = FT_OPEN_MEMORY;
	args.memory_base = data.data();
	args.memory_size = static_cast<FT_Long>( data.size() );
	// the same walk and the same scoring as the catalog, over one font's faces,
	// the family being given already
	int nBestIndex = 0, nBestScore = -1;
	SFontMatch first;
	int nInstances = 0, nFaces = 0;
	if ( DescribeFace( pLibrary, args, std::string(), 0, &first, &nInstances, &nFaces ) )
	{
		for ( int nFace = 0; nFace < nFaces; ++nFace )
		{
			SFontMatch face = first;
			if ( nFace != 0 && !DescribeFace( pLibrary, args, std::string(), nFace, &face, &nInstances, &nFaces ) )
			{
				continue;
			}
			for ( int nInstance = 0; nInstance <= nInstances; ++nInstance )
			{
				SFontMatch candidate = face;
				int nIgnoredInstances = 0, nIgnoredFaces = 0;
				if ( nInstance != 0 && !DescribeFace( pLibrary, args, std::string(), ( nInstance << 16 ) | nFace, &candidate,
					&nIgnoredInstances, &nIgnoredFaces ) )
				{
					continue;
				}
				const int nScore = ( candidate.bItalic != bItalic ? 100000 : 0 ) + std::abs( candidate.nWidthClass - 5 ) * 1000 +
					std::abs( candidate.nWeight - nWeight );
				if ( nBestScore < 0 || nScore < nBestScore )
				{
					nBestIndex = candidate.nFaceIndex;
					nBestScore = nScore;
				}
			}
		}
	}
	FT_Done_FreeType( pLibrary );
	return nBestIndex;
}

}

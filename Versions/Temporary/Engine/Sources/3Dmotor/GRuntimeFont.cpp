#include "stdafx.h"
#include "GRuntimeFont.h"
#include "DBScene.h"
#include "GFont.h"
#include "System/BasicShare.h"
#include "Image/Image.h"
#include "Image/ImageDDS.h"
#include "Misc/StrProc.h"
#include "Misc/2Darray.h"
#include "System/VFSOperations.h"

#include "CodePages.h"
#include "FontFace.h"
#include "FontFinder.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>


namespace NGScene
{

extern CBasicShare<SIntResKey, CFileFont> shareFonts;

namespace
{

// blank pixels between cells, and from the atlas edge, so that bilinear
// filtering never reaches a neighbour's ink
const int N_PADDING = 2;
// GDI's tmDefaultChar, and so the key a missing character's .notdef box has
// always been filed under; FontGen stores it under the same key
const uint16_t W_DEFAULT_CHAR = 0x1F;
const int N_MIN_ATLAS = 256;
const int N_MAX_ATLAS = 2048;
// room for about this many glyphs of average width at a given size: all of a
// code page with plenty to spare for names and chat in other scripts
const int N_GLYPHS_PLANNED = 512;

// The database's charset enum, which is its own numbering, as the Windows
// charset NCodePages takes
int ToWindowsCharset( const NDb::SFont::ECharset eCharset )
{
	switch ( eCharset )
	{
	case NDb::SFont::ANSI: return NCodePages::CHARSET_ANSI;
	case NDb::SFont::BALTIC: return NCodePages::CHARSET_BALTIC;
	case NDb::SFont::CHINESEBIG5: return NCodePages::CHARSET_CHINESEBIG5;
	case NDb::SFont::DEF_CHARSET: return NCodePages::CHARSET_DEFAULT;
	case NDb::SFont::EASTEUROPE: return NCodePages::CHARSET_EASTEUROPE;
	case NDb::SFont::GB2312: return NCodePages::CHARSET_GB2312;
	case NDb::SFont::GREEK: return NCodePages::CHARSET_GREEK;
	case NDb::SFont::HANGUL: return NCodePages::CHARSET_HANGUL;
	case NDb::SFont::RUSSIAN: return NCodePages::CHARSET_RUSSIAN;
	case NDb::SFont::SHIFTJIS: return NCodePages::CHARSET_SHIFTJIS;
	case NDb::SFont::SYMBOL: return NCodePages::CHARSET_SYMBOL;
	case NDb::SFont::TURKISH: return NCodePages::CHARSET_TURKISH;
	case NDb::SFont::HEBREW: return NCodePages::CHARSET_HEBREW;
	case NDb::SFont::ARABIC: return NCodePages::CHARSET_ARABIC;
	case NDb::SFont::THAI: return NCodePages::CHARSET_THAI;
	default: return NCodePages::CHARSET_ANSI;
	}
}

// The characters every runtime font reserves room for, whatever charset its
// record names: the printable half of each European code page, together.
//
// The charset is a baking instruction; text reaches the engine as UTF-16 and
// glyphs are looked up by code point, so a runtime font draws any character
// its file has whatever the record says. Fitting by the record's own charset
// would size the same font differently in each language edition, since each
// edition's texts.pak carries its own records (the Russian edition's say
// RUSSIAN, the repository's ANSI), and one set of records shipped over any
// edition should look the same in all of them. The tallest of these, the accented
// capitals of Latin-1 and Latin Extended and Cyrillic's Й and Ё, are what the
// cell has to hold anyway for names and chat to fit.
const std::vector<uint32_t> &GetSizingCodePoints()
{
	static std::vector<uint32_t> codePoints;
	if ( codePoints.empty() )
	{
		const int CHARSETS[] = { NCodePages::CHARSET_ANSI, NCodePages::CHARSET_EASTEUROPE, NCodePages::CHARSET_RUSSIAN,
			NCodePages::CHARSET_GREEK, NCodePages::CHARSET_TURKISH, NCodePages::CHARSET_BALTIC };
		for ( const int nCharset : CHARSETS )
		{
			for ( const uint32_t nCodePoint : NCodePages::GetPrintableCodePoints( nCharset ) )
				codePoints.push_back( nCodePoint );
		}
		std::sort( codePoints.begin(), codePoints.end() );
		codePoints.erase( std::unique( codePoints.begin(), codePoints.end() ), codePoints.end() );
	}
	return codePoints;
}

// Measure the old atlas rather than guessing a scale factor for a replacement
// family. H supplies the visible height; a fixed Latin/digit sample supplies
// the width. Neither measurement depends on the current text or language.
const char REFERENCE_CHARACTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

struct SReferenceSize
{
	double fCapHeight = 0;
	double fAdvance = 0;
};

SReferenceSize ReadReferenceSize( const NDb::SFont *pRecord )
{
	SReferenceSize result;
	CDGPtr<CPtrFuncBase<CFontFormatInfo>> baked( shareFonts.Get( SResKey<int>( pRecord->uid, pRecord->GetRecordID() ) ) );
	baked.Refresh();
	const CFontFormatInfo *pFormat = baked->GetValue();
	if ( pFormat == nullptr || pFormat->GetLineSpace() <= 0 || !pFormat->HasChar( 'H' ) || pRecord->pTexture == nullptr )
		return result;
	CFileStream stream( NVFS::GetMainVFS(), pRecord->pTexture->szDestName );
	CArray2D<uint32_t> pixels;
	if ( !stream.IsOk() || !NImage::LoadImageDDS( &pixels, &stream ) )
		return result;
	const STFCharacter &capital = pFormat->GetChar( 'H' );
	if ( capital.x1 < 0 || capital.y1 < 0 || capital.x2 > pixels.GetSizeX() || capital.y2 > pixels.GetSizeY() )
		return result;
	int nTop = capital.y2, nBottom = capital.y1;
	for ( int y = capital.y1; y < capital.y2; ++y )
	{
		for ( int x = capital.x1; x < capital.x2; ++x )
		{
			if ( ( pixels[y][x] >> 24 ) != 0 )
			{
				nTop = (std::min)( nTop, y );
				nBottom = (std::max)( nBottom, y + 1 );
			}
		}
	}
	if ( nBottom <= nTop )
		return result;
	int nAdvance = 0;
	for ( const char c : REFERENCE_CHARACTERS )
	{
		if ( c == 0 )
			break;
		if ( !pFormat->HasChar( c ) )
			return result;
		const STFCharacter &glyph = pFormat->GetChar( c );
		nAdvance += glyph.nA + glyph.nBC;
	}
	result.fCapHeight = static_cast<double>( nBottom - nTop ) / pFormat->GetLineSpace();
	result.fAdvance = static_cast<double>( nAdvance ) / pFormat->GetLineSpace();
	return result;
}

// A font file from the game data, read once and shared by every size of every
// font that uses it; FreeType reads from these bytes for as long as a face is
// open on them
std::shared_ptr<const std::vector<uint8_t>> LoadFontData( const std::string &szPath )
{
	static std::unordered_map<std::string, std::shared_ptr<const std::vector<uint8_t>>> loaded;
	std::string szKey = szPath;
	NStr::ToLower( &szKey );
	std::unordered_map<std::string, std::shared_ptr<const std::vector<uint8_t>>>::const_iterator pos = loaded.find( szKey );
	if ( pos != loaded.end() )
		return pos->second;
	std::shared_ptr<std::vector<uint8_t>> pData;
	CFileStream stream( NVFS::GetMainVFS(), szPath );
	if ( stream.IsOk() && stream.GetSize() > 0 )
	{
		pData = std::make_shared<std::vector<uint8_t>>( stream.GetSize() );
		stream.Read( pData->data(), stream.GetSize() );
	}
	loaded[szKey] = pData;
	return pData;
}

// The families tried, in order, for a character no font of the record has.
// Wide coverage first; each is used only if installed. Windows' own set covers
// most of Unicode between them, and a Linux desktop usually has Noto.
const char *const SYSTEM_FALLBACK_FAMILIES[] =
{
	"Segoe UI", "Segoe UI Symbol", "Segoe UI Historic", "Microsoft YaHei", "Yu Gothic", "Malgun Gothic",
	"Nirmala UI", "Leelawadee UI", "Ebrima", "Gadugi", "Myanmar Text", "Mongolian Baiti", "Microsoft Himalaya",
	"Noto Sans", "Noto Sans CJK SC", "Noto Sans Arabic", "Noto Sans Hebrew", "Noto Sans Devanagari",
	"Noto Sans Thai", "Noto Sans Armenian", "Noto Sans Georgian", "Noto Sans Ethiopic", "DejaVu Sans",
	"Arial Unicode MS",
};

// The installed fonts, read the first time a character needs one: a second or
// so over Windows' fonts, once per run
const NFontRaster::CFontCatalog &GetSystemCatalog()
{
	static std::unique_ptr<NFontRaster::CFontCatalog> pCatalog;
	if ( pCatalog == nullptr )
		pCatalog.reset( new NFontRaster::CFontCatalog( NFontRaster::GetSystemFontDirectories() ) );
	return *pCatalog;
}

}

struct CGlyphAtlas::SState
{
	// the record's font first, then its fallbacks, then any system fonts
	std::vector<std::unique_ptr<NFontRaster::CFace>> faces;
	bool bSystemFacesAdded = false;
	NFontRaster::SOptions options;
	std::vector<uint32_t> sizing;						// what every face's cell is fitted to
	int nRasterHeight = 0;						// full cell, including the replacement's accents
	int nAscent = 0;												// the baseline every glyph sits on, the first face's
	CObj<CFontFormatInfo> pFormat;
	// which face each placed character came from, for kerning, which only
	// pairs within one face mean anything
	std::unordered_map<uint16_t, int> glyphFace;
	// characters no face has, which look up the default character's box
	std::unordered_set<uint16_t> missing;
	// the atlas: coverage, one byte a pixel, and the next free position
	int nSize = 0;
	std::vector<uint8_t> coverage;
	int nPenX = 0, nPenY = 0;
	bool bFull = false;
	// rows changed since the texture was last written
	bool bDirty = false;
	int nDirtyTop = 0, nDirtyBottom = 0;
};

CGlyphAtlas::CGlyphAtlas() : pRecord( 0 ), nCellHeight( 0 ) {}
CGlyphAtlas::~CGlyphAtlas() {}

// Fits a fallback face to the same cell as the first, by the same characters
// where it has them and by its own win metrics where it has none of them
static bool FitFallback( NFontRaster::CFace *pFace, const NFontRaster::SOptions &options, const std::vector<uint32_t> &sizing )
{
	std::string szError;
	if ( pFace->Fit( options, sizing, &szError ) )
		return true;
	NFontRaster::SOptions winOptions = options;
	winOptions.eCellMetrics = NFontRaster::CELL_WIN;
	winOptions.nCapHeight = 0; // a fallback without Latin H cannot match cap height
	return pFace->Fit( winOptions, sizing, &szError );
}

bool CGlyphAtlas::Init( const NDb::SFont *_pRecord, const int _nCellHeight, const int _nCellWidth, const bool _bMatchBakedSize )
{
	pRecord = _pRecord;
	nCellHeight = _nCellHeight;
	nCellWidth = _nCellWidth;
	bMatchBakedSize = _bMatchBakedSize;
	order.clear();
	pState.reset( new SState );
	pValue = 0;
	if ( pRecord == 0 || nCellHeight <= 0 )
		return false;
	SState &state = *pState;
	std::shared_ptr<const std::vector<uint8_t>> pData = LoadFontData( pRecord->szFontFile );
	if ( pData == nullptr )
	{
		DebugTrace( "runtime font: cannot read \"%s\" for font \"%s\"", pRecord->szFontFile.c_str(), pRecord->szName.c_str() );
		return false;
	}
	std::string szError;
	std::unique_ptr<NFontRaster::CFace> pFace = NFontRaster::CFace::OpenMemory( pData,
		NFontRaster::SelectFace( *pData, pRecord->nThickness, pRecord->bItalic ), &szError );
	state.options.nCellHeight = nCellHeight;
	state.options.eCellMetrics = NFontRaster::CELL_INK;
	state.options.eHinting = NFontRaster::HINTING_LIGHT;
	state.options.bAntialias = pRecord->bAntialiased;
	state.options.nPadding = N_PADDING;
	const int nCharset = ToWindowsCharset( pRecord->eCharset );
	state.sizing = GetSizingCodePoints();
	const SReferenceSize reference = bMatchBakedSize ? ReadReferenceSize( pRecord ) : SReferenceSize();
	const int nWidth = nCellWidth > 0 ? nCellWidth : nCellHeight;
	state.options.fWidthScale = static_cast<double>( nWidth ) / nCellHeight;
	if ( reference.fCapHeight > 0 && pFace != nullptr && pFace->HasGlyph( 'H' ) )
		state.options.nCapHeight = (std::max)( 1, static_cast<int>( std::lround( reference.fCapHeight * nCellHeight ) ) );
	if ( pFace == nullptr || !pFace->Fit( state.options, state.sizing, &szError ) )
	{
		DebugTrace( "runtime font: \"%s\" for font \"%s\": %s", pRecord->szFontFile.c_str(), pRecord->szName.c_str(), szError.c_str() );
		return false;
	}
	// Match the old text's average advance as well as its visible height.
	// FreeType then draws directly at that width, including on widescreen.
	if ( state.options.nCapHeight > 0 && reference.fAdvance > 0 )
	{
		int nAdvance = 0;
		for ( const char c : REFERENCE_CHARACTERS )
		{
			if ( c == 0 )
				break;
			NFontRaster::SGlyphBitmap glyph;
			if ( !pFace->HasGlyph( c ) || !pFace->RenderGlyph( c, &glyph, &szError ) )
			{
				nAdvance = 0;
				break;
			}
			nAdvance += glyph.nA + glyph.nB + glyph.nC;
		}
		if ( nAdvance > 0 )
		{
			state.options.fWidthScale *= reference.fAdvance * nWidth / nAdvance;
			if ( !pFace->Fit( state.options, state.sizing, &szError ) )
				return false;
		}
	}
	const NFontRaster::SFaceMetrics &metrics = pFace->GetMetrics();
	state.nRasterHeight = metrics.nCellHeight;
	state.nAscent = metrics.nAscent;
	state.pFormat = new CFontFormatInfo;
	// The replacement may need more vertical room for accents. Its visible
	// letters already match the old size, so the UI must keep drawing at 1:1.
	state.pFormat->SetMetrics( state.nRasterHeight, 0, metrics.nAveCharWidth, metrics.nMaxCharWidth, static_cast<uint8_t>( nCharset ), W_DEFAULT_CHAR );
	state.faces.push_back( std::move( pFace ) );
	for ( const NFile::CFilePath &szFallback : pRecord->fallbackFontFiles )
	{
		std::shared_ptr<const std::vector<uint8_t>> pFallbackData = LoadFontData( szFallback );
		if ( pFallbackData == nullptr )
			continue;
		std::unique_ptr<NFontRaster::CFace> pFallback = NFontRaster::CFace::OpenMemory( pFallbackData,
			NFontRaster::SelectFace( *pFallbackData, pRecord->nThickness, pRecord->bItalic ), &szError );
		if ( pFallback != nullptr && FitFallback( pFallback.get(), state.options, state.sizing ) )
		{
			state.faces.push_back( std::move( pFallback ) );
		}
	}
	// Square and a power of two, sized from the cell: the average glyph is
	// taken as six tenths of the cell wide
	const double fGlyphArea = ( state.nRasterHeight + N_PADDING ) * ( nWidth * 0.6 + N_PADDING );
	const int nWanted = static_cast<int>( std::sqrt( N_GLYPHS_PLANNED * fGlyphArea ) );
	state.nSize = N_MIN_ATLAS;
	while ( state.nSize < nWanted && state.nSize < N_MAX_ATLAS )
		state.nSize <<= 1;
	state.coverage.assign( static_cast<size_t>( state.nSize ) * state.nSize, 0 );
	state.nPenX = N_PADDING;
	state.nPenY = N_PADDING;
	// the box every character no face has is drawn with, first
	AddGlyph( W_DEFAULT_CHAR );
	return true;
}

bool CGlyphAtlas::AddGlyph( const uint16_t wChar )
{
	SState &state = *pState;
	// Which face draws it: the first that has it. The default character is the
	// first face's .notdef, which no face "has".
	int nFace = -1;
	if ( wChar == W_DEFAULT_CHAR )
		nFace = 0;
	for ( int i = 0; nFace < 0 && i < static_cast<int>( state.faces.size() ); ++i )
	{
		if ( state.faces[i]->HasGlyph( wChar ) )
			nFace = i;
	}
	if ( nFace < 0 && !state.bSystemFacesAdded )
	{
		// Only now, since reading the installed fonts takes a moment: every
		// family on the list that is installed joins the end of the chain
		state.bSystemFacesAdded = true;
		for ( const char *pszFamily : SYSTEM_FALLBACK_FAMILIES )
		{
			NFontRaster::SFontMatch match;
			if ( !GetSystemCatalog().Find( pszFamily, pRecord->nThickness, pRecord->bItalic, &match ) )
				continue;
			std::string szError;
			std::unique_ptr<NFontRaster::CFace> pFace = NFontRaster::CFace::OpenFile( match.szFile, match.nFaceIndex, &szError );
			if ( pFace != nullptr && FitFallback( pFace.get(), state.options, state.sizing ) )
			{
				state.faces.push_back( std::move( pFace ) );
			}
		}
		for ( int i = 0; nFace < 0 && i < static_cast<int>( state.faces.size() ); ++i )
		{
			if ( state.faces[i]->HasGlyph( wChar ) )
				nFace = i;
		}
	}
	if ( nFace < 0 || state.bFull )
	{
		state.missing.insert( wChar );
		return false;
	}
	NFontRaster::SGlyphBitmap glyph;
	std::string szError;
	if ( !state.faces[nFace]->RenderGlyph( wChar, &glyph, &szError ) )
	{
		state.missing.insert( wChar );
		return false;
	}
	const int nWidth = glyph.nB + (std::max)( glyph.nC, 0 );
	if ( nWidth + 2 * N_PADDING > state.nSize )
	{
		state.missing.insert( wChar );
		return false;
	}
	// rows left to right, top to bottom; a glyph never moves once placed,
	// because laid out text keeps its atlas coordinates
	if ( state.nPenX + nWidth + N_PADDING > state.nSize )
	{
		state.nPenX = N_PADDING;
		state.nPenY += state.nRasterHeight + N_PADDING;
	}
	if ( state.nPenY + state.nRasterHeight + N_PADDING > state.nSize )
	{
		state.bFull = true;
		DebugTrace( "runtime font \"%s\" %d px: atlas full at %d characters", pRecord->szName.c_str(), nCellHeight, static_cast<int>( order.size() ) );
		state.missing.insert( wChar );
		return false;
	}
	// every face's glyphs on the first face's baseline; ink beyond the cell,
	// which fitting keeps to fallbacks' rare characters, is cut off
	const int nFirstRow = state.nAscent - glyph.nTop;
	for ( int y = 0; y < glyph.nRows; ++y )
	{
		const int nCellRow = nFirstRow + y;
		if ( nCellRow < 0 || nCellRow >= state.nRasterHeight )
			continue;
		uint8_t *pDst = &state.coverage[static_cast<size_t>( state.nPenY + nCellRow ) * state.nSize + state.nPenX];
		std::copy_n( &glyph.coverage[static_cast<size_t>( y ) * glyph.nB], glyph.nB, pDst );
	}
	STFCharacter character;
	character.x1 = state.nPenX;
	character.y1 = state.nPenY;
	character.x2 = state.nPenX + nWidth;
	character.y2 = state.nPenY + state.nRasterHeight;
	character.nA = glyph.nA;
	character.nBC = glyph.nB + glyph.nC;
	character.nWidth = nWidth;
	state.pFormat->SetChar( wChar, character );
	state.glyphFace[wChar] = nFace;
	order.push_back( wChar );
	if ( !state.bDirty )
	{
		state.nDirtyTop = state.nPenY;
		state.nDirtyBottom = state.nPenY + state.nRasterHeight;
		state.bDirty = true;
	}
	state.nDirtyTop = (std::min)( state.nDirtyTop, state.nPenY );
	state.nDirtyBottom = (std::max)( state.nDirtyBottom, state.nPenY + state.nRasterHeight );
	state.nPenX += nWidth + N_PADDING;
	return true;
}

void CGlyphAtlas::Prepare( const std::wstring &wsText )
{
	if ( pState == nullptr || pState->faces.empty() )
		return;
	SState &state = *pState;
	for ( const wchar_t wch : wsText )
	{
		// control characters are laid out by the UI itself, never drawn; the
		// UTF-16 unit is the key, as it is in every font the engine reads
		const uint16_t wChar = static_cast<uint16_t>( wch );
		if ( wChar < 0x20 || state.pFormat->HasChar( wChar ) || state.missing.count( wChar ) != 0 )
			continue;
		AddGlyph( wChar );
	}
	// kerning for each adjacent pair, asked of the face both came from
	for ( size_t i = 1; i < wsText.size(); ++i )
	{
		const uint16_t wLast = static_cast<uint16_t>( wsText[i - 1] );
		const uint16_t wChar = static_cast<uint16_t>( wsText[i] );
		if ( state.pFormat->HasKern( wChar, wLast ) )
			continue;
		std::unordered_map<uint16_t, int>::const_iterator last = state.glyphFace.find( wLast );
		std::unordered_map<uint16_t, int>::const_iterator current = state.glyphFace.find( wChar );
		int nKern = 0;
		if ( last != state.glyphFace.end() && current != state.glyphFace.end() && last->second == current->second )
			nKern = state.faces[current->second]->GetKerning( wLast, wChar );
		// zero is stored too, so that a pair is asked about once
		state.pFormat->SetKern( wChar, wLast, nKern );
	}
}

CFontFormatInfo *CGlyphAtlas::GetFormat()
{
	return pState != nullptr ? pState->pFormat : 0;
}

bool CGlyphAtlas::Rebuild()
{
	// Init starts the order afresh with the default character, the first entry
	// of every saved order, and the rest go back in the same order to the same
	// places, since placement depends on nothing else
	const std::vector<uint16_t> saved = order;
	if ( !Init( pRecord, nCellHeight, nCellWidth, bMatchBakedSize ) )
		return false;
	for ( size_t i = 1; i < saved.size(); ++i )
	{
		if ( !pState->pFormat->HasChar( saved[i] ) )
			AddGlyph( saved[i] );
	}
	return true;
}

bool CGlyphAtlas::NeedUpdate()
{
	return pState != nullptr && pState->bDirty;
}

void CGlyphAtlas::Recalc()
{
	const bool b16Bit = NGfx::Is16BitTextures();
	if ( pState == nullptr || pState->nSize == 0 )
	{
		// a font that could not be made draws nothing rather than crashing
		if ( !IsValid( pValue ) )
		{
			pValue = NGfx::MakeTexture( 1, 1, 1, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::CLAMP );
			NGfx::CTextureLock<NGfx::SPixel8888> lock( pValue, 0, NGfx::INPLACE );
			lock[0][0].dwColor = 0;
		}
		return;
	}
	SState &state = *pState;
	int nTop = state.nDirtyTop, nBottom = state.nDirtyBottom;
	if ( !IsValid( pValue ) )
	{
		// Managed, so it survives a device reset; made again from the coverage,
		// which is the atlas, if it is ever lost
		// the two IDs are different enums, which MakeTexture takes as an int
		const int nPixelID = b16Bit ? static_cast<int>( NGfx::SPixel4444::ID ) : static_cast<int>( NGfx::SPixel8888::ID );
		pValue = NGfx::MakeTexture( state.nSize, state.nSize, 1, nPixelID, NGfx::REGULAR, NGfx::CLAMP );
		nTop = 0;
		nBottom = state.nSize;
	}
	// white, with the coverage as alpha, as FontGen's atlases are
	if ( b16Bit )
	{
		NGfx::CTextureLock<NGfx::SPixel4444> lock( pValue, 0, NGfx::INPLACE );
		for ( int y = nTop; y < nBottom; ++y )
		{
			const uint8_t *pSrc = &state.coverage[static_cast<size_t>( y ) * state.nSize];
			NGfx::SPixel4444 *pDst = lock[y];
			for ( int x = 0; x < state.nSize; ++x )
				pDst[x].wColor = static_cast<uint16_t>( ( ( pSrc[x] >> 4 ) << 12 ) | 0x0FFF );
		}
	}
	else
	{
		NGfx::CTextureLock<NGfx::SPixel8888> lock( pValue, 0, NGfx::INPLACE );
		for ( int y = nTop; y < nBottom; ++y )
		{
			const uint8_t *pSrc = &state.coverage[static_cast<size_t>( y ) * state.nSize];
			NGfx::SPixel8888 *pDst = lock[y];
			for ( int x = 0; x < state.nSize; ++x )
				pDst[x].dwColor = ( static_cast<uint32_t>( pSrc[x] ) << 24 ) | 0x00FFFFFF;
		}
	}
	state.bDirty = false;
}

int CGlyphAtlas::operator&( IBinSaver &saver )
{
	// The record, the size and the order, which is all the atlas is made of;
	// the glyphs themselves are made again on load
	CDBPtr<NDb::SFont> pSavedRecord( pRecord );
	saver.Add( 1, &pSavedRecord );
	saver.Add( 2, &nCellHeight );
	saver.Add( 3, &order );
	saver.Add( 4, &nCellWidth );
	saver.Add( 5, &bMatchBakedSize );
	if ( saver.IsReading() )
	{
		pRecord = pSavedRecord;
		if ( !Rebuild() )
			pState.reset();
		pValue = 0;
	}
	return 0;
}

void CRuntimeFontFormat::Recalc()
{
	pValue = IsValid( pAtlas ) ? pAtlas->GetFormat() : 0;
}

int CRuntimeFontFormat::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pAtlas );
	return 0;
}

CRuntimeFontInfo::CRuntimeFontInfo( const SFont &sFont, CGlyphAtlas *_pAtlas )
	: CFontInfo( sFont, _pAtlas, new CRuntimeFontFormat( _pAtlas ) ), pAtlas( _pAtlas )
{
}

void CRuntimeFontInfo::PrepareGlyphs( const std::wstring &wsText )
{
	if ( IsValid( pAtlas ) )
		pAtlas->Prepare( wsText );
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x1E7F0C01, CGlyphAtlas )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x1E7F0C02, CRuntimeFontFormat )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x1E7F0C03, CRuntimeFontInfo )

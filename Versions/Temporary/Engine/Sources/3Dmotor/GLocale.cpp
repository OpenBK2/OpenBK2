#include "stdafx.h"
#include "GLocale.h"
#include "GFont.h"
#include "GTexture.h"
#include "System/BasicShare.h"
#include "DBScene.h"
#include "GRuntimeFont.h"

#include <algorithm>
#include <cmath>

namespace NGScene
{

extern CBasicShare<SIntResKey, CFileFont> shareFonts;
extern CBasicShare<STextureKey, CFileTexture, STextureKeyHash> shareTextures;

struct SFontInfo
{
	int nSize;
	int nSizeIndex;
};

// CTextLocaleInfo

CTextLocaleInfo::CTextLocaleInfo()
{
}

void CTextLocaleInfo::AddAllAvailableFonts()
{
	CPtr<NDb::IDBIterator> pFontIterator = NDb::CreateDBIterator( NDb::SFont::typeID );
	NI_ASSERT( pFontIterator != 0, "cannot create font iterator" );
	for( ; !pFontIterator->IsEnd(); pFontIterator->MoveNext() )
		AddFont( static_cast<const NDb::SFont*>( pFontIterator->Get() ) );
}

void CTextLocaleInfo::AddFont( const NDb::SFont *pFont )
{
	if ( pFont == 0 )
		return;
	CDGPtr< CPtrFuncBase<CFontFormatInfo> > pFormatInfo( shareFonts.Get( SResKey<int>(pFont->uid, pFont->GetRecordID()) ) );
	pFormatInfo.Refresh();
	const CFontFormatInfo *pInfo = pFormatInfo->GetValue();
	// The baked font is registered even when the record names a font file, so
	// that it still answers should that file turn out to be unusable
	if ( pInfo != nullptr )
		fonts.push_back( new CFontInfo( SFont( pInfo->GetHeight(), pFont->szName ), shareTextures.Get( STextureKey( pFont->pTexture ) ), pFormatInfo ) );
	if ( !pFont->szFontFile.empty() )
		runtimeRecords[pFont->szName] = pFont;
}

void CTextLocaleInfo::ClearAllFonts()
{
	fonts.clear();
	// the records go with the MOD that is being detached, and fonts made from
	// them with the records
	runtimeRecords.clear();
	runtimeFonts.clear();
}

// Runtime fonts below this cell height are not made. The UI asks for them only
// in passing, such as while a window is still being created at a few pixels
// tall, and a font fitted to them would be unreadable and waste an atlas.
const int N_MIN_RUNTIME_FONT_SIZE = 6;

const float F_DEFAULT_RUNTIME_FONT_SCALE = 1.2f;

float GetRuntimeFontScale()
{
	const float fScale = NGlobal::GetVar( "ui_font_scale", F_DEFAULT_RUNTIME_FONT_SCALE ).GetFloat();
	// Console/config values must stay finite and bounded before they become
	// integer pixel sizes and atlas allocations. Invalid input uses the default.
	if ( !std::isfinite( fScale ) || fScale <= 0 )
		return F_DEFAULT_RUNTIME_FONT_SCALE;
	return std::clamp( fScale, 0.25f, 4.0f );
}

START_REGISTER( RuntimeFontScale )
	REGISTER_VAR( "ui_font_scale", 0, F_DEFAULT_RUNTIME_FONT_SCALE, STORAGE_USER )
FINISH_REGISTER

CFontInfo* CTextLocaleInfo::GetRuntimeFont( const SFont &sFont )
{
	if ( sFont.nSize < N_MIN_RUNTIME_FONT_SIZE )
		return 0;
	std::unordered_map<std::string, const NDb::SFont*>::const_iterator record = runtimeRecords.find( sFont.szName );
	if ( record == runtimeRecords.end() )
		return 0;
	const float fScale = GetRuntimeFontScale();
	if ( fCachedRuntimeFontScale != fScale )
	{
		// Existing layouts own their atlases; new layouts must get the new size.
		// Drop obsolete cache entries so repeated console edits do not retain
		// an ever-growing collection of unused font textures.
		runtimeFonts.clear();
		fCachedRuntimeFontScale = fScale;
	}
	const std::tuple<std::string, int, int> key( sFont.szName, sFont.nSize, sFont.nWidth );
	std::map<std::tuple<std::string, int, int>, CObj<CFontInfo>>::iterator made = runtimeFonts.find( key );
	if ( made == runtimeFonts.end() )
	{
		// Cache both dimensions. The reference atlas determines visible letter
		// size; extra room for the replacement's accents must not scale it down.
		CObj<CFontInfo> pFont;
		CObj<CGlyphAtlas> pAtlas = new CGlyphAtlas();
		const int nRasterHeight = (std::max)( 1, static_cast<int>( std::lround( sFont.nSize * fScale ) ) );
		const int nRasterWidth = sFont.nWidth > 0 ?
			(std::max)( 1, static_cast<int>( std::lround( sFont.nWidth * fScale ) ) ) : 0;
		// The atlas saves these final dimensions, so loading a save never applies
		// the multiplier a second time to its already laid out text.
		if ( pAtlas->Init( record->second, nRasterHeight, nRasterWidth ) )
			pFont = new CRuntimeFontInfo( sFont, pAtlas );
		else
			DebugTrace( "runtime font \"%s\" %d px could not be made, the baked font is used", sFont.szName.c_str(), sFont.nSize );
		made = runtimeFonts.insert( std::make_pair( key, pFont ) ).first;
	}
	return made->second;
}

void CTextLocaleInfo::Setup( const CVec2 &_vScreenRect )
{
	if ( vScreenRect == _vScreenRect )
		return;

//	fontCache.clear();
	vScreenRect = _vScreenRect;
}

CFontInfo* CTextLocaleInfo::SearchFont( const SFont &sFont )
{
	int nSize = 0;
	CPtr<CFontInfo> pResFontInfo;

	for ( std::vector< CObj<CFontInfo> >::iterator iTemp = fonts.begin(); iTemp != fonts.end(); iTemp++ )
	{
		const SFont &sDBFont = (*iTemp)->GetType();
		if ( sDBFont.szName != sFont.szName )
			continue;

		if ( IsValid( pResFontInfo ) )
		{
			if ( abs( sDBFont.nSize - sFont.nSize ) < abs( nSize - sFont.nSize ) )
			{
				nSize = sDBFont.nSize;
				pResFontInfo = (*iTemp);
			}
		}
		else
		{
			nSize = sDBFont.nSize;
			pResFontInfo = (*iTemp);
		}
	}

	return pResFontInfo;
}

CFontInfo* CTextLocaleInfo::GetFont( const SFont &sFont )
{
	CPtr<CFontInfo> pResFontInfo = GetRuntimeFont( sFont );
	if ( pResFontInfo )
		return pResFontInfo;

	pResFontInfo = SearchFont( sFont );
	if ( !pResFontInfo )
		pResFontInfo = SearchFont( SFont( 16, "System" ) );

	return pResFontInfo;
}

} // namespace
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02931162, CTextLocaleInfo )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x020c1140, CFontInfo )


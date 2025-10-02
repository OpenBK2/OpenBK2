#include "StdAfx.h"
#include "GLocale.h"
#include "GFont.h"
#include "GTexture.h"
#include "System/BasicShare.h"
#include "DBScene.h"

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
	if ( const CFontFormatInfo *pInfo = pFormatInfo->GetValue() )
		fonts.push_back( new CFontInfo( SFont( pInfo->GetHeight(), pFont->szName ), shareTextures.Get( STextureKey( pFont->pTexture ) ), pFormatInfo ) );
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

	for ( vector< CObj<CFontInfo> >::iterator iTemp = fonts.begin(); iTemp != fonts.end(); iTemp++ )
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
	CPtr<CFontInfo> pResFontInfo = 0;

	pResFontInfo = SearchFont( sFont );
	if ( !pResFontInfo )
		pResFontInfo = SearchFont( SFont( 16, "System" ) );

	return pResFontInfo;
}

} // namespace
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x02931162, CTextLocaleInfo )
REGISTER_SAVELOAD_CLASS( 0x020c1140, CFontInfo )


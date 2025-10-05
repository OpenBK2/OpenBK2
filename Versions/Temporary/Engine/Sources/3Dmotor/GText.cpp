#include "stdafx.h"
#include <limits>
#include "GText.h"
#include "Misc/StrProc.h"

namespace NGScene
{

//! Размер TAB-а
const int
	N_TAB_SIZE		= 32;
const WCHAR
	WC_TAG_BEGIN	= L'<',
	WC_TAG_END		= L'>';
const int
	FONT_SIZE_MASK			= 0x00FFFFFF,
	FONT_SIZE_PIXELS		= 0x10000000,
	FONT_SIZE_POINTS		= 0x20000000;
const int
	FONT_SIZE_SMALL			= 0x00000010 | FONT_SIZE_POINTS,
	FONT_SIZE_NORMAL		= 0x00000016 | FONT_SIZE_POINTS,
	FONT_SIZE_LARGE			= 0x00000020 | FONT_SIZE_POINTS,
	FONT_SIZE_XLARGE		= 0x00000030 | FONT_SIZE_POINTS,
	FONT_SIZE_XXLARGE		= 0x00000040 | FONT_SIZE_POINTS;

// Enums
enum ECharType
{
	CHAR_TAG,
	CHAR_TAB,
	CHAR_ALNUM,
	CHAR_SPACE,
	CHAR_NEWLINE,
	CHAR_PUNCTUATION
};
enum ECharStyle
{
	STYLE_NORMAL,
	STYLE_UNDERLINE
};
enum EAlignStyle
{
	ALIGN_LEFT,
	ALIGN_RIGHT,
	ALIGN_CENTER,
	ALIGN_JUSTIFY,
	ALIGN_NOWRAP
};

// Structures
struct SState
{
	ZDATA
	SFont sFont;
	NGfx::SPixel8888 sColor;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&sFont); f.Add(3,&sColor); return 0; }

	SState() {}
	SState( const SFont &_sFont, const NGfx::SPixel8888 &_sColor ): sFont( _sFont ), sColor( _sColor ) {}
};
struct SFontInfo
{
	ZDATA
	CVec2 scale;
	CPtr<CFontInfo> pFont;
	CPtr<CFontFormatInfo> pInfo;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&scale); f.Add(3,&pFont); f.Add(4,&pInfo); return 0; }
};
struct SChunk
{
	ZDATA
	int nCount;
	int nIndex;
	SState sState;
	std::wstring wsText;
	ECharType eType;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&nCount); f.Add(3,&nIndex); f.Add(4,&sState); f.Add(5,&wsText); f.Add(6,&eType); return 0; }
	
	SChunk() {}
	SChunk( ECharType _eType, int _nIndex, const SState &_sState, int _nCount )
		: eType( _eType ), nIndex( _nIndex ), sState( _sState ), nCount( _nCount ) {}
	SChunk( ECharType _eType, int _nIndex, const SState &_sState, const std::wstring &_wsText )
		: eType( _eType ), nIndex( _nIndex ), sState( _sState ), wsText( _wsText ) {}
};

// CTextFormater

class CTextFormater: public CFuncBase<SText>
{
	OBJECT_BASIC_METHODS(CTextFormater);
private:
	ZDATA
	bool bDynamic;
	float fLineWidth;
	float fLineHeight, fLastLineHeight;
	SState sState;
	SFontInfo sFontInfo;
	EAlignStyle eAlignStyle;
	std::list<SChunk> chunksList;
	CTPoint<int> sSize;
	CTPoint<float> sRealSize;
public:
	bool bProcessTAGs;
	CPtr<CTextLocaleInfo> pLocale;
	CDGPtr< CFuncBase<CVec2> > pScreenRect;
	CDGPtr< CFuncBase< std::wstring > > pText;
	CDGPtr< CFuncBase< CTPoint<int> > > pSize;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&bDynamic); f.Add(3,&fLineWidth); f.Add(4,&fLineHeight); f.Add(5,&fLastLineHeight); f.Add(6,&sState); f.Add(7,&sFontInfo); f.Add(8,&eAlignStyle); f.Add(9,&chunksList); f.Add(10,&sSize); f.Add(11,&sRealSize); f.Add(12,&bProcessTAGs); f.Add(13,&pLocale); f.Add(14,&pScreenRect); f.Add(15,&pText); f.Add(16,&pSize); return 0; }

protected:
	void Generate();
	void GenerateLine();
	CRectLayout* GetLayout( const SFont &sFont );
	void GetFontFormatInfo( const SFont &sFont, SFontInfo *pInfo );

	void TagFont( const std::wstring &wsTag );
	void TagColor( const std::wstring &wsTag );
	void ProcessTAG( const std::wstring &wsTag );

	void Recalc();
	bool NeedUpdate() { return pText.Refresh() | pSize.Refresh() | pScreenRect.Refresh(); }

public:
	CTextFormater() {}
};

CFuncBase<SText>* CreateTextFormater( CTextLocaleInfo *pInfo, CFuncBase<CVec2> *pScreenRect, CFuncBase< std::wstring > *pText, CFuncBase< CTPoint<int> > *pSize, bool bProcessTAGs )
{
	CTextFormater *pTextFormater = new CTextFormater;
	pTextFormater->bProcessTAGs = bProcessTAGs;
	pTextFormater->pText = pText;
	pTextFormater->pSize = pSize;
	pTextFormater->pLocale = pInfo;
	pTextFormater->pScreenRect = pScreenRect;

	return pTextFormater;
}

// CTextFormater

void CTextFormater::Recalc()
{
	value.sSize.x = 0;
	value.sSize.y = 0;
	value.charsSet.clear();
	value.rectLayouts.clear();

	Generate();
}

void CTextFormater::Generate()
{
	int nTemp = 0, nWordBegin = 0;
	float fWordWidth = 0, fWordHeight = 0;
	WCHAR wcChar = 0, wcLastChar = 0;
	ECharType eThisChar = CHAR_NEWLINE, eLastChar = CHAR_NEWLINE ;

	bDynamic = false;
	sState = SState( SFont( FONT_SIZE_NORMAL, "System" ), NGfx::SPixel8888( 255, 255, 255, 255 ) );
	fLineWidth = 0;
	fLineHeight = 0;
	fLastLineHeight = 0;
	eAlignStyle = ALIGN_LEFT;
	chunksList.clear();
	GetFontFormatInfo( sState.sFont, &sFontInfo );

	pText.Refresh();
	const std::wstring &wsText = pText->GetValue();
	pSize.Refresh();
	sSize = pSize->GetValue();

	if ( sSize.x == -1 )
	{
		bDynamic = true;
		sSize.x = 0x7fffffff;//numeric_limits<int>::max();
	}
	sRealSize = CTPoint<float>( 0, 0 );

	do
	{
		wcChar = wsText.c_str()[nTemp];

		if ( wcChar == L'\t' )
			eThisChar = CHAR_TAB;
		else if ( wcChar == L' ' )
			eThisChar = CHAR_SPACE;
		else if ( bProcessTAGs && ( wcChar == WC_TAG_BEGIN ) )
			eThisChar = CHAR_TAG;
		else if ( ( wcChar == L'\0' ) || ( wcChar == L'\n' ) )
			eThisChar = CHAR_NEWLINE;
		else if ( iswgraph( wcChar ) )
			eThisChar = CHAR_ALNUM;
		else
			ASSERT( 0 );

		if ( ( eThisChar != eLastChar ) || ( fWordWidth > sSize.x ) )
		{
			if ( eLastChar == CHAR_ALNUM )
			{
				fLineWidth += fWordWidth;
				fLineHeight = max( fLineHeight, fWordHeight );
				chunksList.push_back( SChunk( CHAR_ALNUM, nWordBegin, sState, wsText.substr( nWordBegin, nTemp - nWordBegin ) ) );
			}
			else if ( ( eLastChar == CHAR_SPACE ) && !chunksList.empty() )
			{
				fLineWidth += fWordWidth;
				fLineHeight = max( fLineHeight, fWordHeight );
				chunksList.push_back( SChunk( CHAR_SPACE, nWordBegin, sState, nTemp - nWordBegin ) );
			}
			else if ( eLastChar == CHAR_TAB )
			{
				fLineWidth += fWordWidth;
				fLineHeight = max( fLineHeight, fWordHeight );
				chunksList.push_back( SChunk( CHAR_TAB, nWordBegin, sState, fWordWidth ) );
			}

			nWordBegin = nTemp;
			fWordWidth = 0;
			fWordHeight = 0;
		}

		if ( ( eThisChar == CHAR_ALNUM ) || ( eThisChar == CHAR_SPACE ) )
		{
			const STFCharacter &sCharacter = sFontInfo.pInfo->GetChar( wcChar );
			float fDelta = sCharacter.nA + sFontInfo.pInfo->GetKern( wcChar, wcLastChar ) + sCharacter.nBC;
			fWordWidth += fDelta * sFontInfo.scale.x;
			fWordHeight = sFontInfo.pInfo->GetLineSpace() * sFontInfo.scale.y;
		}
		else if ( eThisChar == CHAR_TAB )
		{
			fWordWidth += N_TAB_SIZE - (int)( fLineWidth ) % N_TAB_SIZE;
			fWordHeight = sFontInfo.pInfo->GetLineSpace() * sFontInfo.scale.y;
		}

		if ( ( eThisChar == CHAR_NEWLINE ) || ( ( eAlignStyle != ALIGN_NOWRAP ) && ( fLineWidth + fWordWidth ) > sSize.x ) )
		{
			if ( fLineHeight == 0 )
				fLineHeight = fLastLineHeight;

			GenerateLine();
			sRealSize.x = max( fLineWidth, sRealSize.x );
			sRealSize.y += fLineHeight;

			fLastLineHeight = fLineHeight;

			fLineWidth = 0;
			fLineHeight = 0;
			chunksList.clear();
		}

		if ( eThisChar == CHAR_TAG )
		{
			int nPos = wsText.find( WC_TAG_END, nTemp );
			if ( nPos != std::wstring::npos )
			{
				const std::wstring &wsSubStr = wsText.substr( nTemp + 1, nPos - nTemp - 1 );
				ProcessTAG( wsSubStr );

				nTemp = nPos + 1;
				eLastChar = eThisChar;
				continue;
			}
		}

		nTemp++;
		eLastChar = eThisChar;
		wcLastChar = wcChar;
	}while( wcChar != 0 );

	value.sSize.x = (int)( sRealSize.x );
	value.sSize.y = (int)( sRealSize.y );

	return;
}

void CTextFormater::GenerateLine()
{
	/*
	float fX;
	float fSpace;
	int nY = (int)sRealSize.y;

	fX = 0;
	fSpace = 1;
	if ( !bDynamic ) // dynamic text didn't support align
	{
		switch( eAlignStyle )
		{
			case ALIGN_RIGHT:
				fX = sSize.x - fLineWidth;
				fSpace = 1;
				break;
			case ALIGN_CENTER:
				fX = ( sSize.x - fLineWidth ) / 2;
				fSpace = 1;
				break;
			case ALIGN_JUSTIFY:
				{
					fX = 0;

					int nSpaceCount = 0;
					float fSpaceWordsWidth = 0;
					for ( list<SChunk>::const_iterator iTemp = chunksList.begin(); iTemp != chunksList.end(); iTemp++ )
					{
						if ( iTemp->eType == CHAR_SPACE )
						{
							SFontInfo sFontInfo;
							GetFontFormatInfo( iTemp->sState.sFont, &sFontInfo );
							nSpaceCount += iTemp->nCount;
							fSpaceWordsWidth += iTemp->nCount * sFontInfo.pInfo->GetChar( ' ' ).nWidth * sFontInfo.scale.x;
						}
					}
					fSpace = (float)( sSize.x - fLineWidth + fSpaceWordsWidth ) / ( nSpaceCount * sFontInfo.pInfo->GetChar( ' ' ).nWidth * sFontInfo.scale.x );
				}
				break;
		}
	}

	for ( list<SChunk>::const_iterator iTemp = chunksList.begin(); iTemp != chunksList.end(); iTemp++ )
	{
		SFontInfo sFontInfo;
		GetFontFormatInfo( iTemp->sState.sFont, &sFontInfo );
		float fHeight = sFontInfo.pInfo->GetLineSpace() * sFontInfo.scale.y;

		if ( iTemp->eType == CHAR_ALNUM )
		{
			WCHAR wcLastChar = 0;
			CRectLayout* pLayout = GetLayout( iTemp->sState.sFont );

			int nCount = 0;
			float fShift = fX - ( (int) fX );
			for ( wstring::const_iterator iChar = iTemp->wsText.begin(); iChar != iTemp->wsText.end(); iChar++ )
			{
				const STFCharacter &sCharacter = sFontInfo.pInfo->GetChar( *iChar );

				value.charsSet.push_back( SText::SCharRect( iTemp->nIndex + nCount, CTRect<int>( (int)( fX ), nY, (int)( fX ) + sCharacter.nWidth * sFontInfo.scale.x, nY + fHeight ) ) );

				fX += ( sCharacter.nA + sFontInfo.pInfo->GetKern( *iChar, wcLastChar ) ) * sFontInfo.scale.x;
				pLayout->AddRect( fX - fShift, nY, CTRect<float>( sCharacter.x1, sCharacter.y1, sCharacter.x2, sCharacter.y2 ), iTemp->sState.sColor );
				fX += sCharacter.nBC * sFontInfo.scale.x;

				wcLastChar = *iChar;
				nCount++;
			}
		}
		else if ( iTemp->eType == CHAR_SPACE )
		{
			float fSpaceWidth = sFontInfo.pInfo->GetChar( L' ' ).nWidth * sFontInfo.scale.x;
			for ( int nTemp = 0; nTemp < iTemp->nCount; nTemp++ )
			{
				int nX = (int)( fX + fSpaceWidth * fSpace * nTemp );
				value.charsSet.push_back( SText::SCharRect( iTemp->nIndex + nTemp, CTRect<int>( nX, nY, nX + fSpaceWidth, nY + fHeight ) ) );
			}

			fX += fSpaceWidth * fSpace * iTemp->nCount;
		}
		else if ( iTemp->eType == CHAR_TAB )
		{
			fX += iTemp->nCount;
		}
	}
*/
	return;
}

void CTextFormater::ProcessTAG( const std::wstring &wsTag )
{
	if ( wsTag.compare( L"br" ) == 0 )
	{
		if ( fLineHeight == 0 )
			fLineHeight = fLastLineHeight;

		GenerateLine();
		sRealSize.x = max( fLineWidth, sRealSize.x );
		sRealSize.y += fLineHeight;

		fLastLineHeight = fLineHeight;

		fLineWidth = 0;
		fLineHeight = 0;
		chunksList.clear();
	}
	else if ( wsTag.compare( L"lb" ) == 0 )
	{
		fLineWidth += sFontInfo.pInfo->GetChar( L'<' ).nWidth * sFontInfo.scale.x;
		fLineHeight = Max( fLineHeight, sFontInfo.pInfo->GetLineSpace() * sFontInfo.scale.y );
		chunksList.push_back( SChunk( CHAR_ALNUM, -1, sState, L"<" ) );
	}
	else if ( wsTag.compare( L"rb" ) == 0 )
	{
		fLineWidth += sFontInfo.pInfo->GetChar( L'>' ).nWidth * sFontInfo.scale.x;
		fLineHeight = Max( fLineHeight, sFontInfo.pInfo->GetLineSpace() * sFontInfo.scale.y );
		chunksList.push_back( SChunk( CHAR_ALNUM, -1, sState, L">" ) );
	}
	else if ( wsTag.compare( L"left" ) == 0 )
		eAlignStyle = ALIGN_LEFT;
	else if ( wsTag.compare( L"right" ) == 0 )
		eAlignStyle	= ALIGN_RIGHT;
	else if ( wsTag.compare( L"center" ) == 0 )
		eAlignStyle	= ALIGN_CENTER;
	else if ( wsTag.compare( L"justify" ) == 0 )
		eAlignStyle	= ALIGN_JUSTIFY;
	else if ( wsTag.compare( L"nowrap" ) == 0 )
		eAlignStyle	= ALIGN_NOWRAP;
	else if ( wsTag.substr( 0, 4 ).compare( L"font" ) == 0 )
		TagFont( wsTag );
	else if ( wsTag.substr( 0, 5 ).compare( L"color" ) == 0 )
		TagColor( wsTag );
}

struct SWString
{
	ECharType eType;
	std::wstring wsString;

	SWString() {}
	SWString( ECharType _eType, const std::wstring &_wsString ): eType( _eType ), wsString( _wsString ) {}
};

void SplitString( const std::wstring &wsString, std::list<SWString> *pParts )
{
	int nTemp = 0, nWordBegin = 0;
	bool bBracketsBlock = false;
	WCHAR wcChar;
	ECharType eThisChar = CHAR_SPACE, eLastChar = CHAR_SPACE;

	do
	{
		wcChar = wsString.c_str()[nTemp];

		if ( ( !bBracketsBlock && ( wcChar == L' ' ) ) || ( wcChar == L'\0' ) || ( wcChar == L'\n' ) )
			eThisChar = CHAR_SPACE;
		else if ( wcChar == L'\"' )
			bBracketsBlock = !bBracketsBlock;
		else if ( iswpunct( wcChar ) )
			eThisChar = CHAR_PUNCTUATION;
		else if ( iswalnum( wcChar ) || ( bBracketsBlock && ( wcChar == L' ' ) ) )
			eThisChar = CHAR_ALNUM;
		else
			ASSERT( 0 );

		if ( eLastChar != eThisChar )
		{
			if ( ( eLastChar == CHAR_ALNUM ) || ( eLastChar == CHAR_PUNCTUATION ) )
				pParts->push_back( SWString( eLastChar, wsString.substr( nWordBegin, nTemp - nWordBegin ) ) );

			nWordBegin = nTemp;
		}

		nTemp++;
		eLastChar = eThisChar;
	}while( wcChar != 0 );
}

void CTextFormater::TagFont( const std::wstring &wsTag )
{
	std::list<SWString> partsList;
	SplitString( wsTag, &partsList );

	SFont sNewFont( sState.sFont );
	for ( std::list<SWString>::iterator iTemp = partsList.begin(); iTemp != partsList.end(); iTemp++ )
	{
		if ( iTemp->eType != CHAR_ALNUM )
			continue;

		if ( iTemp->wsString.compare( L"size" ) == 0 )
		{
			iTemp++;
			if ( ( iTemp->eType != CHAR_PUNCTUATION ) || ( iTemp->wsString.compare( L"=" ) != 0 ) )
				continue;
			iTemp++;
			if ( iTemp->eType != CHAR_ALNUM )
				continue;

			if ( iTemp->wsString.compare( L"small" ) == 0 )
				sNewFont.nSize = FONT_SIZE_SMALL;
			else if ( iTemp->wsString.compare( L"normal" ) == 0 )
				sNewFont.nSize = FONT_SIZE_NORMAL;
			else if ( iTemp->wsString.compare( L"large" ) == 0 )
				sNewFont.nSize = FONT_SIZE_LARGE;
			else if ( iTemp->wsString.compare( L"xlarge" ) == 0 )
				sNewFont.nSize = FONT_SIZE_XLARGE;
			else if ( iTemp->wsString.compare( L"xxlarge" ) == 0 )
				sNewFont.nSize = FONT_SIZE_XXLARGE;
			else
			{
				WCHAR wsString[128];
				int nParams = swscanf( iTemp->wsString.c_str(), L"%d%2s", &sNewFont.nSize, wsString );

				if ( nParams > 1 )
				{
					std::wstring wsSizeMod( wsString );
					if ( wsSizeMod.compare( L"px" ) == 0 )
						sNewFont.nSize |= FONT_SIZE_PIXELS;
					else if ( wsSizeMod.compare( L"pt" ) == 0 )
						sNewFont.nSize |= FONT_SIZE_POINTS;
				}
				else
					sNewFont.nSize |= FONT_SIZE_POINTS;
			}
		}
		else if ( iTemp->wsString.compare( L"face" ) == 0 )
		{
			iTemp++;
			if ( ( iTemp->eType != CHAR_PUNCTUATION ) || ( iTemp->wsString.compare( L"=" ) != 0 ) )
				continue;
			iTemp++;
			if ( iTemp->eType != CHAR_ALNUM )
				continue;

			sNewFont.szName = NStr::ToMBCS( iTemp->wsString );
		}
	}

	sState.sFont = sNewFont;
	GetFontFormatInfo( sNewFont, &sFontInfo );
}

void CTextFormater::TagColor( const std::wstring &wsTag )
{
	std::list<SWString> partsList;
	SplitString( wsTag, &partsList );

	std::list<SWString>::iterator iTemp = partsList.begin();
	iTemp++;
	if ( ( iTemp->eType != CHAR_PUNCTUATION ) || ( iTemp->wsString.compare( L"=" ) != 0 ) )
		return;
	iTemp++;
	if ( iTemp->eType != CHAR_ALNUM )
		return;

	if ( iTemp->wsString.compare( L"white" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 0xFF, 0xFF, 0xFF, 0xFF );
	else if ( iTemp->wsString.compare( L"red" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 0xFF, 0, 0, 0xFF );
	else if ( iTemp->wsString.compare( L"green" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 0, 0xFF, 0, 0xFF );
	else if ( iTemp->wsString.compare( L"blue" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 0, 0, 0xFF, 0xFF );
	else if ( iTemp->wsString.compare( L"yellow" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 0xFF, 0xFF, 0, 0xFF );
	else if ( iTemp->wsString.compare( L"cyan" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 0, 0xFF, 0xFF, 0xFF );
	else if ( iTemp->wsString.compare( L"orange" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 0xFF, 0x80, 0x40, 0xFF );
	else if ( iTemp->wsString.compare( L"pink" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 0xFF, 0x80, 0xFF, 0xFF );
	else if ( iTemp->wsString.compare( L"brown" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 110, 110, 56, 0xFF );
	else if ( iTemp->wsString.compare( L"grey" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 180, 180, 180, 0xFF );
	else if ( iTemp->wsString.compare( L"black" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 0, 0, 0, 0xFF );
	else if ( iTemp->wsString.compare( L"darkyellow" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 242, 192, 17, 0xFF );
	else if ( iTemp->wsString.compare( L"beige" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 209, 183, 167, 0xFF );
	else if ( iTemp->wsString.compare( L"lightblue" ) == 0 )
		sState.sColor = NGfx::SPixel8888( 0, 101, 213, 0xFF );
	else
		swscanf( iTemp->wsString.c_str(), L"%x", &sState.sColor.dwColor );
}

CRectLayout* CTextFormater::GetLayout( const SFont &sFont )
{
	SFont sSearch( sFont );
	sSearch.nSize = 12;
	sSearch.szName = "Courier";
	CPtr<CFontInfo> pInfo( pLocale->GetFont( sSearch ) );

	for ( int nTemp = 0; nTemp < value.rectLayouts.size(); nTemp++ )
	{
		const SText::SFontLayout &sLayout = value.rectLayouts[nTemp];
		if ( ( sLayout.sFont.nSize == sFont.nSize ) && ( sLayout.pFontInfo == pInfo ) )
			return &value.rectLayouts[nTemp].sLayout;
	}

	SFontInfo sFontInfo;
	GetFontFormatInfo( sFont, &sFontInfo );

	SText::SFontLayout *pLayout = &*value.rectLayouts.insert( value.rectLayouts.end(), SText::SFontLayout() );
	pLayout->sFont = sFont;
	pLayout->pFontInfo = sFontInfo.pFont;
//	pLayout->sLayout.scale.x = sFontInfo.scale.x;
//	pLayout->sLayout.scale.y = sFontInfo.scale.y;

	return &pLayout->sLayout;
}

void CTextFormater::GetFontFormatInfo( const SFont &sFont, SFontInfo *pFontInfo )
{
	SFont sSearch( sFont );
	pScreenRect.Refresh();
	const CVec2 &vScreen = pScreenRect->GetValue();

	sSearch.nSize = sFont.nSize & FONT_SIZE_MASK;
	if ( sFont.nSize & FONT_SIZE_POINTS )
		sSearch.nSize = (float)( sFont.nSize & FONT_SIZE_MASK ) * vScreen.x / 1024.0f;
	else if ( sFont.nSize & FONT_SIZE_PIXELS )
		sSearch.nSize = sFont.nSize & FONT_SIZE_MASK;
	else
		ASSERT( 0 );

	CPtr<CFontInfo> pFont = pLocale->GetFont( sSearch );
	CDGPtr< CPtrFuncBase<CFontFormatInfo> > pInfo( pFont->GetFormatInfo() );
	pInfo.Refresh();
	pFontInfo->pFont = pFont;
	pFontInfo->pInfo = pInfo->GetValue();

	float fScale = (float)sSearch.nSize / pFontInfo->pInfo->GetLineSpace();
	pFontInfo->scale.x = fScale;
	pFontInfo->scale.y = fScale;

	return;
}

} // namespace
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x02931161, CTextFormater );


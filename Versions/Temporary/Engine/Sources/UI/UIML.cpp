#include "stdafx.h"
#include "3Dmotor/RectLayout.h"
#include "3Dmotor/G2DView.h"
#include "3Dmotor/DBScene.h"
#include "3Dmotor/Locale.h"

#include "UIML.h"
#include "UIMLHandlers.h"
#include "System/Commands.h"

static const int N_TAB_SIZE = 4;
static std::string s_DefaultFontName = "System";

struct SFontInfo
{
	CVec2 scale;
	CPtr<CFontFormatInfo> pInfo;
	CPtr<NGScene::CFontInfo> pFont;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &scale );
		saver.Add( 2, &pInfo );
		saver.Add( 3, &pFont );
		return 0;
	}
};

static void GetFontFormatInfo(  const NGScene::SFont &sFont, int nMinSize, SFontInfo *pFontInfo )
{
	int nx, ny;
	Singleton<IUIInitialization>()->GetVirtualScreenController()->GetResolution( &nx, &ny );
	CVec2 vScreen( nx, ny );

	NGScene::SFont sSearch( sFont );
	CTPoint<float> src( 1024, 768 );
	CTPoint<float> dst;
	VirtualToScreen( src, &dst );
	CTPoint<float> src0( 0, 0 );
	CTPoint<float> dst0;
	VirtualToScreen( src0, &dst0 );

	sSearch.nSize = sFont.nSize & FONT_SIZE_MASK;
	if ( sFont.nSize & FONT_SIZE_POINTS )
		sSearch.nSize = (float)( sFont.nSize & FONT_SIZE_MASK ) * (dst.x - dst0.x) / 1024.0f;
	else if ( sFont.nSize & FONT_SIZE_PIXELS )
		sSearch.nSize = sFont.nSize & FONT_SIZE_MASK;
	else
		ASSERT( 0 );

	sSearch.nSize = Max( sSearch.nSize, nMinSize );

	CPtr<NGScene::CFontInfo> pFont = NGScene::GetTextLocaleInfo()->GetFont( sSearch );
	if ( !pFont )
		return;

	NI_VERIFY( pFont, StrFmt( "Font not found: %s", sSearch.szName.c_str() ), return );
	CDGPtr< CPtrFuncBase<CFontFormatInfo> > pInfo( pFont->GetFormatInfo() );
	pInfo.Refresh();
	pFontInfo->pFont = pFont;
	pFontInfo->pInfo = pInfo->GetValue();

	float fScale = (float)sSearch.nSize / pFontInfo->pInfo->GetLineSpace();
	pFontInfo->scale.x = fScale;
	pFontInfo->scale.y = fScale * 4.0f * vScreen.y / vScreen.x / 3.0f;
}

// CMLStream

void CMLStream::GetString( int nStart, int nSize, std::wstring *pRes )
{
	if ( nStart + nSize > wsText.length() )
	{
		ASSERT( 0 );
		return;
	}

	*pRes = wsText.substr( nStart, nSize );
}

void CMLStream::InsertString( const std::wstring &wsInsertText )
{
	std::wstring wsNewText( wsText.substr( 0, nPos ) + wsInsertText + wsText.substr( nPos ) );
	wsText = wsNewText;
	nSize = wsText.size();
}

// CMLTextObject

class CMLTextObject: public IMLObject
{
	OBJECT_BASIC_METHODS(CMLTextObject)
private:
	int nStrSize;
	int nStrStart;
	CPtr<CMLStream> pStream;
	////
	SState sState;
	CTPoint<float> sSize;
	CTPoint<float> sPosition;
	////
	std::list<float> edges;
	CRectLayout sNormal;
	CRectLayout sOutline;
	CObj<CPtrFuncBase<NGfx::CTexture> > pTexture;

public:
	CMLTextObject() : sPosition( 0.0f, 0.0f ) {}
	CMLTextObject( CMLStream *pStream, int nStart, int nSize );

	void Generate(  );
	void DynamicGenerate(  const SReflowInfo &sInfo ) {}

	const CTPoint<float>& GetSize() const;

	const SState& GetState() const;
	void SetState( const SState &sState );

	const CTPoint<float>& GetPosition() const;
	void SetPosition( const CTPoint<float> &sPosition );

	void Render( std::list<CTRect<float> > *pRender, const CTPoint<float> &sGlobalPosition, const CTRect<float> &sWindow );
	void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &sPosition, const CTRect<float> &sWindow );

	int operator&( IBinSaver &saver );
};

IMLObject* CreateIMLTextObject( CMLStream *pStream, int nStart, int nSize )
{
	return new CMLTextObject( pStream, nStart, nSize );
}

CMLTextObject::CMLTextObject( CMLStream *_pStream, int _nStart, int _nSize ):
	pStream( _pStream ), nStrStart( _nStart ), nStrSize( _nSize ), sPosition( 0.0f, 0.0f )
{
	sSize = CTPoint<float>( 20, 20 );
}

void CMLTextObject::Generate(  )
{
	int nx, ny;
	Singleton<IUIInitialization>()->GetVirtualScreenController()->GetResolution( &nx, &ny );
	CVec2 vScreenRect( nx, ny );

	SFontInfo sFontInfo;
	GetFontFormatInfo( sState.sFont, sState.nMinFontSize, &sFontInfo );

	if ( !sFontInfo.pInfo )
	{
		NGScene::SFont font( sState.sFont.nSize, "h2" );
		GetFontFormatInfo( font, sState.nMinFontSize, &sFontInfo );
		if ( !sFontInfo.pInfo )
			return;
	}

	sSize.y = sFontInfo.pInfo->GetLineSpace() * sFontInfo.scale.y;
	pTexture = sFontInfo.pFont->GetTexture();

	float fX = 0;
	WCHAR wcLastChar = 0;
	pStream->Seek( nStrStart );
	for ( int nTemp = 0; nTemp < nStrSize; nTemp++ )
	{
		WCHAR wcChar = pStream->GetChar();

		float fS = sState.nOutlineBorder * vScreenRect.x / 1024;

		if ( wcChar != L'\t' )
		{
			const STFCharacter &sCharacter = sFontInfo.pInfo->GetChar( wcChar );
			fX += sState.bForceFontSize ? 0 : fS;
			fX += ( sCharacter.nA + sFontInfo.pInfo->GetKern( wcChar, wcLastChar ) ) * sFontInfo.scale.x;

			CTRect<float> sCharRect( sCharacter.x1, sCharacter.y1, sCharacter.x2, sCharacter.y2 );
			float fCX = sCharRect.Width() * sFontInfo.scale.x;
			float fCY = sCharRect.Height() * sFontInfo.scale.y;

			sNormal.AddRect( fX, 0, fCX, fCY, sCharRect, sState.sColor );

 			if ( sState.nOutlineBorder )
			{
				sOutline.AddRect( fX, +fS, fCX, fCY, sCharRect, sState.sOutlineColor );
				sOutline.AddRect( fX, -fS, fCX, fCY, sCharRect, sState.sOutlineColor );
				sOutline.AddRect( fX + fS, 0, fCX, fCY, sCharRect, sState.sOutlineColor );
				sOutline.AddRect( fX - fS, 0, fCX, fCY, sCharRect, sState.sOutlineColor );
				sOutline.AddRect( fX + fS, +fS, fCX, fCY, sCharRect, sState.sOutlineColor );
				sOutline.AddRect( fX + fS, -fS, fCX, fCY, sCharRect, sState.sOutlineColor );
				sOutline.AddRect( fX - fS, +fS, fCX, fCY, sCharRect, sState.sOutlineColor );
				sOutline.AddRect( fX - fS, -fS, fCX, fCY, sCharRect, sState.sOutlineColor );
			}

			fX += sCharacter.nBC * sFontInfo.scale.x;
			fX += sState.bForceFontSize ? 0 : fS;
		}
		else
		{
			float fCharSize = 0;
			fCharSize += sState.bForceFontSize ? 0 : fS * 2;
			fCharSize += sFontInfo.pInfo->GetAveCharWidth() * sFontInfo.scale.x;
			ASSERT( fCharSize != 0 );
			if ( fCharSize == 0 )
				continue;

			int nPart = Float2Int( fX / fCharSize ) / N_TAB_SIZE + 1;
			fX = nPart * N_TAB_SIZE * fCharSize;
		}

		edges.push_back( fX );

		wcLastChar = wcChar;
		pStream->IncSeek();
	}

	sSize.x = fX;
}

const CTPoint<float>& CMLTextObject::GetSize() const
{
	return sSize;
}

const SState& CMLTextObject::GetState() const
{
	return sState;
}

void CMLTextObject::SetState( const SState &_sState )
{
	sState = _sState;
}

const CTPoint<float>& CMLTextObject::GetPosition() const
{
	return sPosition;
}

void CMLTextObject::SetPosition( const CTPoint<float> &_sPosition )
{
	sPosition = _sPosition;
}

void CMLTextObject::Render( std::list<CTRect<float> > *pRender, const CTPoint<float> &sGlobalPosition, const CTRect<float> &sWindow )
{
	CTPoint<float> sPos = sGlobalPosition + sPosition;

	float fLastX = 0;
	for ( std::list<float>::const_iterator iTemp = edges.begin(); iTemp != edges.end(); iTemp++ )
	{
		pRender->push_back( CTRect<float>( fLastX + sPos.x, sPos.y, *iTemp + sPos.x, sSize.y + sPos.y ) );
		fLastX = *iTemp;
	}
}

void CMLTextObject::Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &sGlobalPosition, const CTRect<float> &sWindow )
{
	if ( !sOutline.rects.empty() )
		pView->CreateDynamicRects( pTexture, sOutline, sGlobalPosition + sPosition, sWindow );

	pView->CreateDynamicRects( pTexture, sNormal, sGlobalPosition + sPosition, sWindow );
}

int CMLTextObject::operator&( IBinSaver &saver )
{
	saver.Add( 1, &nStrSize );
	saver.Add( 2, &nStrStart );
	saver.Add( 3, &pStream );
	////
	saver.Add( 4, &sState );
	saver.Add( 5, &sSize );
	saver.Add( 6, &sPosition );
	////
	saver.Add( 7, &edges );
	saver.Add( 8, &sNormal );
	saver.Add( 9, &sOutline );
	saver.Add( 10, &pTexture );
	return 0;
}

// CMLImageObject

class CMLImageObject: public IMLObject
{
	OBJECT_BASIC_METHODS(CMLImageObject)
private:
	int nBorder;
	int nWidth;
	int nHeight;
	SState::EHORAlign eAlign;
	////
	SState sState;
	CTPoint<float> sSize;
	CTPoint<float> sPosition;
	////
	CRectLayout sLayout;
	CDBPtr<NDb::STexture> pTexture;

public:
	CMLImageObject() {}
	CMLImageObject( NDb::STexture *pTexture, SState::EHORAlign eAlign, int nBorder, int nWidth, int nHeight );

	void Generate(  );
	void DynamicGenerate(  const SReflowInfo &sInfo ) {}

	const CTPoint<float>& GetSize() const;

	const SState& GetState() const;
	void SetState( const SState &sState );

	const CTPoint<float>& GetPosition() const;
	void SetPosition( const CTPoint<float> &sPosition );

	void Render( std::list<CTRect<float> > *pRender, const CTPoint<float> &sGlobalPosition, const CTRect<float> &sWindow );
	void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &sPosition, const CTRect<float> &sWindow );

	int operator&( IBinSaver &saver );
};

IMLObject* CreateIMLImageObject( NDb::STexture *pTexture, SState::EHORAlign eAlign, int nBorder, int nWidth, int nHeight )
{
	return new CMLImageObject( pTexture, eAlign, nBorder, nWidth, nHeight );
}

CMLImageObject::CMLImageObject( NDb::STexture *_pTexture, SState::EHORAlign _eAlign, int _nBorder, int _nWidth, int _nHeight ):
	pTexture( _pTexture ), eAlign( _eAlign ), nBorder( _nBorder ), nWidth( _nWidth ), nHeight( _nHeight ), sSize( 0, 0 )
{
}

void CMLImageObject::Generate(  )
{
	if ( pTexture )
	{
		if ( ( pTexture->nWidth == 0 ) || ( pTexture->nHeight == 0 ) )
			return;

		sSize.x = pTexture->nWidth;
		sSize.y = pTexture->nHeight;
		if ( nWidth != -1 )
			sSize.x = nWidth;
		if ( nHeight != -1 )
			sSize.y = nHeight;

		CTRect<float> sTexRect;
		sTexRect.x1 = 0;
		sTexRect.x2 = pTexture->nWidth;
		sTexRect.y1 = pTexture->nHeight;
		sTexRect.y2 = 0;
		sLayout.AddRect( 0, 0, sSize.x, sSize.y, sTexRect );
	}
}

const CTPoint<float>& CMLImageObject::GetSize() const
{
	return sSize;
}

const SState& CMLImageObject::GetState() const
{
	return sState;
}

void CMLImageObject::SetState( const SState &_sState )
{
	sState = _sState;
	if ( eAlign != SState::HORALIGN_DEFAULT )
		sState.eHAlign = eAlign;
}

const CTPoint<float>& CMLImageObject::GetPosition() const
{
	return sPosition;
}

void CMLImageObject::SetPosition( const CTPoint<float> &_sPosition )
{
	sPosition = _sPosition;
}

void CMLImageObject::Render( std::list<CTRect<float> > *pRender, const CTPoint<float> &sGlobalPosition, const CTRect<float> &sWindow )
{
	pRender->push_back( CTRect<float>( sGlobalPosition.x, sGlobalPosition.y, sSize.x + sGlobalPosition.x, sSize.y + sGlobalPosition.y ) );
}

void CMLImageObject::Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &sGlobalPosition, const CTRect<float> &sWindow )
{
	pView->CreateDynamicRects( pTexture, sLayout, sGlobalPosition + sPosition, sWindow );
}

int CMLImageObject::operator&( IBinSaver &saver )
{
	saver.Add( 1, &nBorder );
	saver.Add( 2, &nWidth );
	saver.Add( 3, &nHeight );
	saver.Add( 4, &eAlign );
	////
	saver.Add( 5, &sState );
	saver.Add( 6, &sSize );
	saver.Add( 7, &sPosition );
	////
	saver.Add( 8, &sLayout );
	saver.Add( 9, &pTexture );
	return 0;
}

// CMLImageObject

class CMLTabObject: public IMLObject
{
	OBJECT_BASIC_METHODS(CMLTabObject)
private:
	SState sState;
	CTPoint<float> sSize;
	CTPoint<float> sPosition;

public:
	CMLTabObject();

	void Generate(  ) {}
	void DynamicGenerate(  const SReflowInfo &sInfo );

	const CTPoint<float>& GetSize() const;

	const SState& GetState() const;
	void SetState( const SState &sState );

	const CTPoint<float>& GetPosition() const;
	void SetPosition( const CTPoint<float> &sPosition );

	void Render( std::list<CTRect<float> > *pRender, const CTPoint<float> &sGlobalPosition, const CTRect<float> &sWindow ) {}
	void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &sPosition, const CTRect<float> &sWindow ) {}

	int operator&( IBinSaver &saver );
};

IMLObject* CreateIMLTabObject()
{
	return new CMLTabObject();
}

CMLTabObject::CMLTabObject():
	sSize( 0, 0 )
{
}

void CMLTabObject::DynamicGenerate(  const SReflowInfo &sInfo )
{
	SFontInfo sFontInfo;
	GetFontFormatInfo( sState.sFont, sState.nMinFontSize, &sFontInfo );

	float fSize = N_TAB_SIZE * sFontInfo.pInfo->GetAveCharWidth() * sFontInfo.scale.y;
	int nPart = sInfo.fLineWidth / fSize;
	sSize.x = ( nPart + 1 ) * fSize - sInfo.fLineWidth;
	sSize.y = sInfo.fLastLineHeight;
}

const CTPoint<float>& CMLTabObject::GetSize() const
{
	return sSize;
}

const SState& CMLTabObject::GetState() const
{
	return sState;
}

void CMLTabObject::SetState( const SState &_sState )
{
	sState = _sState;
}

const CTPoint<float>& CMLTabObject::GetPosition() const
{
	return sPosition;
}

void CMLTabObject::SetPosition( const CTPoint<float> &_sPosition )
{
	sPosition = _sPosition;
}

int CMLTabObject::operator&( IBinSaver &saver )
{
	saver.Add( 1, &sState );
	saver.Add( 2, &sSize );
	saver.Add( 3, &sPosition );
	return 0;
}

// CMLLayout

class CMLLayout: public IMLLayout
{
	OBJECT_BASIC_METHODS(CMLLayout)
private:
	struct SCmdPair
	{
		ECommand eCmd;
		CPtr<IMLObject> pObject;

		SCmdPair() {}
		SCmdPair( ECommand _eCmd, IMLObject *_pObject ): eCmd( _eCmd ), pObject( _pObject ) {}

		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &eCmd );
			saver.Add( 2, &pObject );
			return 0;
		}
	};
	SState sState;
	CTPoint<float> sSize;
	std::list<SCmdPair> itemsList;
	std::list<SState> states;

protected:
	void CreateLine( SReflowInfo *pInfo, float fWidth, bool bEndBlock );
	void EstimateLine( SReflowInfo *pInfo, float fWidth );
	void AssembleLine( SReflowInfo *pInfo, bool bEndBlock );
	void ProcessWraped( SReflowInfo *pInfo );

public:
	CMLLayout();

	void AddObject( IMLObject *pObject );
	void AddCommand( ECommand eCommand, IMLObject *pObject = 0 );

	const CTPoint<float>& GetSize() const;

	const SState& GetState();
	void SetState( const SState &sState );
	void PushState();
	void PopState();

	void Generate(  float fWidth );

	void Render( std::list<CTRect<float> > *pRender, const CTPoint<float> &sPosition, const CTRect<float> &sWindow );
	void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &sPosition, const CTRect<float> &sWindow );

	int operator&( IBinSaver &saver );
};

CMLLayout::CMLLayout():
	sSize( 0, 0 )
{
	sState.sFont = NGScene::SFont( 16 | FONT_SIZE_POINTS, s_DefaultFontName );
	sState.sColor = NGfx::SPixel8888( 0xFF, 0xFF, 0xFF, 0xFF );
	sState.eHAlign = SState::HORALIGN_LEFT;
	sState.eVAlign = SState::VERTALIGN_MIDDLE;
}

void CMLLayout::AddObject( IMLObject *pObject )
{
	itemsList.push_back( SCmdPair( CMD_NULL, pObject ) );
	pObject->SetState( sState );
}

void CMLLayout::AddCommand( ECommand eCommand, IMLObject *pObject )
{
	itemsList.push_back( SCmdPair( eCommand, pObject ) );
	if ( IsValid( pObject ) )
		pObject->SetState( sState );
}

const CTPoint<float>& CMLLayout::GetSize() const
{
	return sSize;
}

const SState& CMLLayout::GetState()
{
	return sState;
}

void CMLLayout::SetState( const SState &_sState )
{
	sState = _sState;
}

void CMLLayout::PushState()
{
	states.push_back( sState );
}

void CMLLayout::PopState()
{
	if ( states.empty() )
	{
		ASSERT( 0 );
		return;
	}

	sState = states.back();
	states.pop_back();
}

void CMLLayout::Generate(  float fWidth )
{
	for( std::list<SCmdPair>::iterator iTemp = itemsList.begin(); iTemp != itemsList.end(); iTemp++ )
	{
		CPtr<IMLObject> pObject = iTemp->pObject;

		if ( IsValid( pObject ) )
			pObject->Generate( );
	}

	SReflowInfo sInfo;
	sInfo.fY = 0;
	sInfo.fMaxX = 0;
	sInfo.fLineWidth = sInfo.fLineHeight = 0;
	sInfo.fLastLineHeight = 0;
	sInfo.sLeft = SRange( 0, -1 );
	sInfo.sRight = SRange( fWidth, -1 );
	for( std::list<SCmdPair>::iterator iTemp = itemsList.begin(); iTemp != itemsList.end(); iTemp++ )
	{
		ECommand eCommand = iTemp->eCmd;
		if ( eCommand == CMD_BREAKLINE )
			CreateLine( &sInfo, fWidth, true );

		CPtr<IMLObject> pObject = iTemp->pObject;
		if ( IsValid( pObject ) )
		{
			pObject->DynamicGenerate( sInfo );

			const CTPoint<float> &sSize = pObject->GetSize();
			const SState &sState = pObject->GetState();

			switch( sState.eHAlign )
			{
			case SState::HORALIGN_DEFAULT:
			case SState::HORALIGN_LEFT:
			case SState::HORALIGN_RIGHT:
			case SState::HORALIGN_CENTER:
			case SState::HORALIGN_JUSTIFY:
				if ( ( sInfo.fLineWidth + sSize.x ) > ( sInfo.sRight.fValue - sInfo.sLeft.fValue ) )
					CreateLine( &sInfo, fWidth, false );

				if ( ( eCommand == CMD_SPACE ) && sInfo.line.empty() )
					break;

				sInfo.line.push_back( pObject );
				sInfo.fLineWidth += sSize.x;
				sInfo.fLineHeight = max( sSize.y, sInfo.fLineHeight );
				break;
			case SState::HORALIGN_NOWRAP:
				if ( ( eCommand == CMD_SPACE ) && sInfo.line.empty() )
					break;

				sInfo.line.push_back( pObject );
				sInfo.fLineWidth += sSize.x;
				sInfo.fLineHeight = max( sSize.y, sInfo.fLineHeight );
				break;
			case SState::HORALIGN_WRAP_LEFT:
				sInfo.leftWraped.push_back( pObject );
				break;
			case SState::HORALIGN_WRAP_RIGHT:
				sInfo.rightWraped.push_back( pObject );
				break;
			default:
				ASSERT( 0 );
				break;
			}
		}
	}

	sSize.x = sInfo.fMaxX;
	sSize.y = sInfo.fY;
}

void CMLLayout::Render( std::list<CTRect<float> > *pRender, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
{
	for( std::list<SCmdPair>::iterator iTemp = itemsList.begin(); iTemp != itemsList.end(); iTemp++ )
	{
		CPtr<IMLObject> pObject = iTemp->pObject;

		if ( IsValid( pObject ) )
			pObject->Render( pRender, sPosition, sWindow );
	}
}

void CMLLayout::Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
{
	for( std::list<SCmdPair>::iterator iTemp = itemsList.begin(); iTemp != itemsList.end(); iTemp++ )
	{
		CPtr<IMLObject> pObject = iTemp->pObject;

		if ( IsValid( pObject ) )
			pObject->Render( pView, sPosition, sWindow );
	}
}

void CMLLayout::CreateLine( SReflowInfo *pInfo, float fWidth, bool bEndBlock )
{
	AssembleLine( pInfo, bEndBlock );

	pInfo->line.clear();

	if ( pInfo->fLineHeight != 0 )
	{
		pInfo->fY += pInfo->fLineHeight;
		pInfo->fLastLineHeight = pInfo->fLineHeight;
	}
	else
		pInfo->fY += pInfo->fLastLineHeight;

	pInfo->fLineWidth = 0;
	pInfo->fLineHeight = 0;

	ProcessWraped( pInfo );
	if ( pInfo->fY > pInfo->sLeft.fHeight )
		pInfo->sLeft.fValue = 0;
	if ( pInfo->fY > pInfo->sRight.fHeight )
		pInfo->sRight.fValue = fWidth;
}

void CMLLayout::EstimateLine( SReflowInfo *pInfo, float fWidth )
{
	pInfo->line.clear();

	if ( pInfo->fLineHeight != 0 )
	{
		pInfo->fY += pInfo->fLineHeight;
		pInfo->fLastLineHeight = pInfo->fLineHeight;
	}
	else
		pInfo->fY += pInfo->fLastLineHeight;

	pInfo->fLineWidth = 0;
	pInfo->fLineHeight = 0;

	ProcessWraped( pInfo );
	if ( pInfo->fY > pInfo->sLeft.fHeight )
		pInfo->sLeft.fValue = 0;
	if ( pInfo->fY > pInfo->sRight.fHeight )
		pInfo->sRight.fValue = fWidth;
}

void CMLLayout::AssembleLine( SReflowInfo *pInfo, bool bEndBlock )
{
	if ( pInfo->line.empty() )
		return;

	float fX = pInfo->sLeft.fValue;
	float fSpace = 0;
	SState::EHORAlign eHAlign = pInfo->line.front()->GetState().eHAlign;
	if ( bEndBlock && ( eHAlign == SState::HORALIGN_JUSTIFY ) )
		eHAlign = SState::HORALIGN_LEFT;
	switch( eHAlign )
	{
	case SState::HORALIGN_RIGHT:
		{
			float fTotalWidth = pInfo->sRight.fValue - pInfo->sLeft.fValue;
			fX = pInfo->sLeft.fValue + fTotalWidth - pInfo->fLineWidth;
			fSpace = 0;
		}
		break;
	case SState::HORALIGN_CENTER:
		{
			float fTotalWidth = pInfo->sRight.fValue - pInfo->sLeft.fValue;
			fX = pInfo->sLeft.fValue + Float2Int( ( fTotalWidth - pInfo->fLineWidth ) / 2 );
			//fX = pInfo->sLeft.fValue + ( fTotalWidth - pInfo->fLineWidth ) / 2;
			fSpace = 0;
			break;
		}
	case SState::HORALIGN_JUSTIFY:
		{
			const float fTotalWidth = pInfo->sRight.fValue - pInfo->sLeft.fValue;
			fX = pInfo->sLeft.fValue;
			fSpace = float( fTotalWidth - pInfo->fLineWidth ) / (pInfo->line.size() > 1 ? pInfo->line.size() - 1 : 1);
			break;
		}
	}

	for( std::list<CPtr<IMLObject> >::iterator iTemp = pInfo->line.begin(); iTemp != pInfo->line.end(); iTemp++ )
	{
		const CTPoint<float> &sSize = (*iTemp)->GetSize();
		SState::EVERTAlign eVAlign = (*iTemp)->GetState().eVAlign;

		switch( eVAlign )
		{
		case SState::VERTALIGN_TOP:
			(*iTemp)->SetPosition( CTPoint<float>( fX, pInfo->fY ) );
			break;
		case SState::VERTALIGN_BOTTOM:
			(*iTemp)->SetPosition( CTPoint<float>( fX, pInfo->fY + pInfo->fLineHeight - sSize.y ) );
			break;
		default:
			(*iTemp)->SetPosition( CTPoint<float>( fX, pInfo->fY + ( pInfo->fLineHeight - sSize.y ) / 2 ) );
			break;
		}

		fX += (*iTemp)->GetSize().x;
		pInfo->fMaxX = Max( pInfo->fMaxX, fX );
		fX += fSpace;
	}
}

void CMLLayout::ProcessWraped( SReflowInfo *pInfo )
{
	for( std::list<CPtr<IMLObject> >::iterator iTemp = pInfo->leftWraped.begin(); iTemp != pInfo->leftWraped.end(); iTemp++ )
	{
		const CTPoint<float> &sSize = (*iTemp)->GetSize();

		(*iTemp)->SetPosition( CTPoint<float>( pInfo->sLeft.fValue, pInfo->fY ) );
		pInfo->fMaxX = Max( pInfo->fMaxX, pInfo->sLeft.fValue + sSize.x );

		pInfo->sLeft.fValue += sSize.x;
		pInfo->sLeft.fHeight = max( pInfo->sLeft.fHeight, pInfo->fY + sSize.y );
	}
	for( std::list<CPtr<IMLObject> >::iterator iTemp = pInfo->rightWraped.begin(); iTemp != pInfo->rightWraped.end(); iTemp++ )
	{
		const CTPoint<float> &sSize = (*iTemp)->GetSize();

		(*iTemp)->SetPosition( CTPoint<float>( pInfo->sRight.fValue - sSize.x, pInfo->fY ) );
		pInfo->fMaxX = Max( pInfo->fMaxX, pInfo->sRight.fValue );

		pInfo->sRight.fValue -= sSize.x;
		pInfo->sRight.fHeight = max( pInfo->sRight.fHeight, pInfo->fY + sSize.y );
	}

	pInfo->leftWraped.clear();
	pInfo->rightWraped.clear();
}

int CMLLayout::operator&( IBinSaver &saver )
{
	saver.Add( 5, &sState );
	saver.Add( 6, &sSize );
	saver.Add( 7, &itemsList );
	saver.Add( 8, &states );
	return 0;
}

// CML

class CML: public IML
{
	OBJECT_BASIC_METHODS(CML)
private:
	CTPoint<int> sSize;
	std::wstring wsText;
	CObj<CMLLayout> pLayout;
	CObj<CMLStream> pStream;
	std::unordered_map<std::wstring,CObj<IMLHandler> > tagsMap;
	int nIDForHandler;
	CObj<SFadeValue> sFadeValue;
public:
	CML();

	void SetText( const std::wstring &wsText, int nFlags );
	void SetHandler( const std::wstring &wsTAG, IMLHandler *pHandler );
	void SetIDForHandler( int nID );
	int GetIDForHandler() const;
	void SetFade( float fFade );

	CMLStream* GetStream() { return pStream; }
	const CTPoint<int>& GetSize();

	void Generate( int nWidth );

	void Render( std::list<CTRect<float> > *pRender, const CTPoint<float> &sPosition, const CTRect<float> &sWindow );
	void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &sPosition, const CTRect<float> &sWindow );

	int operator&( IBinSaver &saver );
};

CML::CML()
: sSize( 0, 0 ), nIDForHandler( -1 )
{
	sFadeValue = new SFadeValue( 1.0f );
	pLayout = new CMLLayout();
	pStream = new CMLStream();

	tagsMap[L"br"] = new CBRHandler;

	tagsMap[L"left"] = new CLEFTHandler;
	tagsMap[L"right"] = new CRIGHTHandler;
	tagsMap[L"center"] = new CCENTERHandler;
	tagsMap[L"nowrap"] = new CNOWRAPHandler;
	tagsMap[L"justify"] = new CJUSTIFYHandler;
	tagsMap[L"wrapleft"] = new CWRAPLEFTHandler;
	tagsMap[L"wrapright"] = new CWRAPRIGHTHandler;
	tagsMap[L"top"] = new CTOPHandler;
	tagsMap[L"bottom"] = new CBOTTOMHandler;
	tagsMap[L"middle"] = new CMIDDLEHandler;

	tagsMap[L"tab"] = new CTABHandler;
	tagsMap[L"font"] = new CFONTHandler;
	tagsMap[L"color"] = new CCOLORHandler( sFadeValue );
	tagsMap[L"image"] = new CIMAGEHandler;
	tagsMap[L"minfontsize"] = new CMINFONTSIZEHandler;

	tagsMap[L"push"] = new CStateStack( true );
	tagsMap[L"pop"] = new CStateStack( false );
}

void CML::SetText( const std::wstring &_wsText, int nFlags )
{
	wsText = _wsText;
}

void CML::SetHandler( const std::wstring &wsTAG, IMLHandler *pHandler )
{
	tagsMap[wsTAG] = pHandler;
}

void CML::SetIDForHandler( int nID )
{
	nIDForHandler = nID;
}

int CML::GetIDForHandler() const
{
	return nIDForHandler;
}

void CML::SetFade( float fFade )
{
	sFadeValue->fFadeValue = fFade;
}

const CTPoint<int>& CML::GetSize()
{
	const CTPoint<float> &sFPSize = pLayout->GetSize();
	sSize.x = Float2Int( sFPSize.x + 0.5f );
	sSize.y = Float2Int( sFPSize.y + 0.5f );
	return sSize;
}

void CML::Generate( int nWidth )
{
	enum ECharType
	{
		CHAR_NULL,
		CHAR_SPACE,
		CHAR_ALNUM,
		CHAR_PUNCTUATION,
		CHAR_EOL,
		CHAR_SKIP,
		////
		CHAR_TAG_END,
		CHAR_TAG_BEGIN
	};

	pLayout = new CMLLayout();
	pStream = new CMLStream();
	pStream->Seek( 0 );
	pStream->InsertString( wsText );

	int nWordBegin = 0;
	bool bTAG = false, bBracketsBlock = false;
	ECharType eThisChar = CHAR_NULL, eLastChar = CHAR_NULL;
	std::vector<std::wstring> paramsSet;

	for ( bool bContinue = true; bContinue; )
	{
		WCHAR wcChar = 0;
		if ( !pStream->IsEof() )
			wcChar = pStream->GetChar();
		else
			bContinue = false;
		eThisChar = CHAR_ALNUM;
		if ( wcChar == '>' )
			eThisChar = CHAR_TAG_END;
		else if ( wcChar == '<' )
			eThisChar = CHAR_TAG_BEGIN;
		else if ( bTAG && ( wcChar == '\"' ) )
			bBracketsBlock = !bBracketsBlock;
		else if ( iswalnum( wcChar ) != 0 )
			eThisChar = CHAR_ALNUM;
		else if ( iswpunct( wcChar ) )
			eThisChar = bTAG ? CHAR_PUNCTUATION : CHAR_ALNUM; //// CRAP: Tag need to separate '='
		else if  ( !bBracketsBlock && ( wcChar == L' ' ) )
			eThisChar = CHAR_SPACE;
		else if  ( bBracketsBlock && ( wcChar == L' ' ) )
			eThisChar = eLastChar;
		else if ( ( wcChar == L'\0' ) || ( wcChar == L'\n' ) )
			eThisChar = CHAR_EOL;
		else if ( wcChar == L'\r' ) 
			eThisChar = CHAR_SKIP;

		if ( eLastChar != eThisChar )
		{
			if ( !bTAG && ( ( eLastChar == CHAR_ALNUM ) || ( eLastChar == CHAR_PUNCTUATION ) ) )
				pLayout->AddObject( new CMLTextObject( pStream, nWordBegin, pStream->GetSeek() - nWordBegin ) );
			else if ( !bTAG && ( eLastChar == CHAR_SPACE ) )
				pLayout->AddCommand( CMD_SPACE, new CMLTextObject( pStream, nWordBegin, pStream->GetSeek() - nWordBegin ) );
			else if ( !bTAG && ( eLastChar == CHAR_EOL ) )
				pLayout->AddCommand( CMD_BREAKLINE );
			else if ( bTAG && ( ( eLastChar == CHAR_ALNUM ) || ( eLastChar == CHAR_PUNCTUATION ) ) )
			{
				std::wstring &wsTemp = paramsSet.emplace_back();
				pStream->GetString( nWordBegin, pStream->GetSeek() - nWordBegin, &wsTemp );
			}
			else if ( eThisChar == CHAR_SKIP ) 
				pStream->IncSeek();

			nWordBegin = pStream->GetSeek();
		}

		pStream->IncSeek();
		eLastChar = eThisChar;

		if ( eLastChar == CHAR_TAG_END )
		{
			bTAG = false;

			if ( !paramsSet.empty() )
			{
				std::unordered_map<std::wstring,CObj<IMLHandler> >::const_iterator iTemp = tagsMap.find( paramsSet.front() );
				if ( ( iTemp != tagsMap.end() ) && ( iTemp->second != 0 ) )
					iTemp->second->Exec( this, pLayout, paramsSet );
			}
		}
		else if ( eLastChar == CHAR_TAG_BEGIN )
		{
			bTAG = true;
			paramsSet.clear();
			paramsSet.reserve( 8 );
		}
	};

	pLayout->AddCommand( CMD_BREAKLINE );
	pLayout->Generate( nWidth );
}

void CML::Render( std::list<CTRect<float> > *pRender, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
{
	pLayout->Render( pRender, sPosition, sWindow );
}

void CML::Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
{
	pLayout->Render( pView, sPosition, sWindow );
}

int CML::operator&( IBinSaver &saver )
{
	saver.Add( 1, &sSize );
	saver.Add( 2, &wsText );
	saver.Add( 3, &pLayout );
	saver.Add( 4, &pStream );
	saver.Add( 5, &tagsMap );
	saver.Add( 6, &nIDForHandler );
	return 0;
}

// CreateML

IML* CreateML()
{
	return new CML;
}

START_REGISTER(MLText)

REGISTER_VAR_EX( "ml_text_default_font_name", NGlobal::VarStrHandler, &s_DefaultFontName, "System", STORAGE_NONE );

FINISH_REGISTER

BASIC_REGISTER_CLASS( IML )
BASIC_REGISTER_CLASS( IMLHandler )
REGISTER_SAVELOAD_CLASS( 0xB0829160, CMLTextObject )
REGISTER_SAVELOAD_CLASS( 0xB0829161, CMLImageObject )
REGISTER_SAVELOAD_CLASS( 0xB0829162, CMLLayout )
REGISTER_SAVELOAD_CLASS( 0xB0829163, CML )

REGISTER_SAVELOAD_CLASS( 0xB1003230, CLEFTHandler )
REGISTER_SAVELOAD_CLASS( 0xB1003231, CRIGHTHandler )
REGISTER_SAVELOAD_CLASS( 0xB1003232, CCENTERHandler )
REGISTER_SAVELOAD_CLASS( 0xB1003233, CJUSTIFYHandler )
REGISTER_SAVELOAD_CLASS( 0xB1003234, CWRAPLEFTHandler )
REGISTER_SAVELOAD_CLASS( 0xB1003235, CWRAPRIGHTHandler )
REGISTER_SAVELOAD_CLASS( 0xB1003236, CTOPHandler )
REGISTER_SAVELOAD_CLASS( 0xB1003237, CMIDDLEHandler )
REGISTER_SAVELOAD_CLASS( 0xB1003238, CBOTTOMHandler )
REGISTER_SAVELOAD_CLASS( 0xB1003239, CBRHandler )
REGISTER_SAVELOAD_CLASS( 0xB100323A, CCOLORHandler )
REGISTER_SAVELOAD_CLASS( 0xB100323B, CFONTHandler )
REGISTER_SAVELOAD_CLASS( 0xB100323C, CIMAGEHandler )
REGISTER_SAVELOAD_CLASS( 0x52353000, CNOWRAPHandler )
REGISTER_SAVELOAD_CLASS( 0xB100323d, CMLStream )
REGISTER_SAVELOAD_CLASS( 0xB3714190, CMINFONTSIZEHandler )
REGISTER_SAVELOAD_CLASS( 0xB3716160, CMLTabObject )
REGISTER_SAVELOAD_CLASS( 0xB3716161, CTABHandler )
REGISTER_SAVELOAD_CLASS( 0xB3716162, CStateStack )
REGISTER_SAVELOAD_CLASS( 0x30268440, SFadeValue )


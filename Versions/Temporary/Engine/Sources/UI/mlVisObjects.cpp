#include "StdAfx.h"
#include "3dMotor/RectLayout.h"
#include "3dMotor/G2DView.h"
#include "3dMotor/DBScene.h"
#include "3DMotor/Locale.h"
#include "3Dmotor/GLocale.h"
//
#include "mlMain.h"
#include "mlReflow.h"
#include "mlVisObjects.h"

namespace NML
{

static const int N_TAB_SIZE = 4;

struct SFontInfo
{
	CVec2 scale;
	CPtr<CFontFormatInfo> pInfo;
	CPtr<NGScene::CFontInfo> pFont;
	int operator&( CStructureSaver &f ) { ASSERT( 0 ); return 0; }
};

static void GetFontFormatInfo( const NGScene::SFont &font, int nMinSize, SFontInfo *pFontInfo )
{
	int nx, ny;
	Singleton<IUIInitialization>()->GetVirtualScreenController()->GetResolution( &nx, &ny );
	CVec2 screenSize( nx, ny );
	NGScene::SFont search( font );

	search.nSize = font.nSize & FONT_SIZE_MASK;
	if ( font.nSize & FONT_SIZE_POINTS )
		search.nSize = (float)( font.nSize & FONT_SIZE_MASK ) * screenSize.x / 1024.0f;
	else if ( font.nSize & FONT_SIZE_PIXELS )
		search.nSize = font.nSize & FONT_SIZE_MASK;
	else
		ASSERT( 0 );

	search.nSize = Max( search.nSize, nMinSize );

	CPtr<NGScene::CFontInfo> pFont = NGScene::GetTextLocaleInfo()->GetFont( search );
	CDGPtr< CPtrFuncBase<CFontFormatInfo> > pInfo( pFont->GetFormatInfo() );
	pInfo.Refresh();
	pFontInfo->pFont = pFont;
	pFontInfo->pInfo = pInfo->GetValue();

	float fScale = (float)search.nSize / pFontInfo->pInfo->GetLineSpace();
	pFontInfo->scale.x = fScale;
	pFontInfo->scale.y = fScale * 4.0f * screenSize.y / screenSize.x / 3.0f;
}

// CTextObject

class CTextObject: public IVisReflowObject
{
	OBJECT_BASIC_METHODS(CTextObject)
private:
	ZDATA
	int nStrSize;
	int nStrStart;
	CPtr<CMLStream> pStream;
	////
	CTPoint<float> size;
	CTPoint<float> position;
	////
	list<float> edges;
	CRectLayout normalRects;
	CRectLayout outlineRects;
	CObj<CPtrFuncBase<NGfx::CTexture> > pTexture;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nStrSize); f.Add(3,&nStrStart); f.Add(4,&pStream); f.Add(5,&size); f.Add(6,&position); f.Add(7,&edges); f.Add(8,&normalRects); f.Add(9,&outlineRects); f.Add(10,&pTexture); return 0; }

public:
	CTextObject() {}
	CTextObject( CMLStream *pStream, int nStart, int nSize );

	void Update( IReflowState *pState );

	const CTPoint<float>& GetPosition() const { return position; }
	void SetPosition( const CTPoint<float> &_position ) { position = _position; }

	void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &position, const CTRect<float> &window );
	void Render( list<CTRect<float> > *pRender, const CTPoint<float> &position, const CTRect<float> &window );

	const CTPoint<float>& GetSize() const { return size; }
};

CTextObject::CTextObject( CMLStream *_pStream, int _nStart, int _nSize ):
	pStream(_pStream), nStrStart(_nStart), nStrSize(_nSize)
{
	size = CTPoint<float>( 20, 20 );
}

void CTextObject::Update( IReflowState *pState )
{
	int nx, ny;
	Singleton<IUIInitialization>()->GetVirtualScreenController()->GetResolution( &nx, &ny );
	CVec2 screenSize( nx, ny );
	const SState &state = pState->GetState();

	SFontInfo fontInfo;
	GetFontFormatInfo( state.font, state.nMinFontSize, &fontInfo );

	size.y = fontInfo.pInfo->GetLineSpace() * fontInfo.scale.y;
	pTexture = fontInfo.pFont->GetTexture();

	int nCount = 0;
	float fX = 0;
	WCHAR nLastChar = 0;
	pStream->Seek( nStrStart );
	for ( int nTemp = 0; nTemp < nStrSize; nTemp++ )
	{
		WCHAR nChar = pStream->GetChar();

		float fSX = state.nOutlineBorder * screenSize.x / 1024;
		float fSY = state.nOutlineBorder * screenSize.y / 768;
		const STFCharacter &charInfo = fontInfo.pInfo->GetChar( nChar );

		fX += state.bForceFontSize ? 0 : fSX;
		fX += ( charInfo.nA + fontInfo.pInfo->GetKern( nChar, nLastChar ) ) * fontInfo.scale.x;

		CTRect<float> charRect( charInfo.x1, charInfo.y1, charInfo.x2, charInfo.y2 );
		float fCX = charRect.Width() * fontInfo.scale.x;
		float fCY = charRect.Height() * fontInfo.scale.y;

		normalRects.AddRect( fX, 0, fCX, fCY, charRect, state.color );

 		if ( state.nOutlineBorder )
		{
			outlineRects.AddRect( fX, +fSY, fCX, fCY, charRect, state.outlineColor );
			outlineRects.AddRect( fX, -fSY, fCX, fCY, charRect, state.outlineColor );
			outlineRects.AddRect( fX + fSX, 0, fCX, fCY, charRect, state.outlineColor );
			outlineRects.AddRect( fX - fSX, 0, fCX, fCY, charRect, state.outlineColor );
			outlineRects.AddRect( fX + fSX, +fSY, fCX, fCY, charRect, state.outlineColor );
			outlineRects.AddRect( fX + fSX, -fSY, fCX, fCY, charRect, state.outlineColor );
			outlineRects.AddRect( fX - fSX, +fSY, fCX, fCY, charRect, state.outlineColor );
			outlineRects.AddRect( fX - fSX, -fSY, fCX, fCY, charRect, state.outlineColor );
		}

		fX += charInfo.nBC * fontInfo.scale.x;
		fX += state.bForceFontSize ? 0 : fSX;
		edges.push_back( fX );

		nLastChar = nChar;
		pStream->IncSeek();
	}

	size.x = fX;
}

void CTextObject::Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &_position, const CTRect<float> &window )
{
	CTPoint<float> pos = _position + position;

	if ( !outlineRects.rects.empty() )
		pView->CreateDynamicRects( pTexture, outlineRects, pos, window );

	pView->CreateDynamicRects( pTexture, normalRects, pos, window );
}

void CTextObject::Render( list<CTRect<float> > *pRender, const CTPoint<float> &_position, const CTRect<float> &window )
{
	CTPoint<float> pos = _position + position;

	float fLastX = 0;
	for ( list<float>::const_iterator iTemp = edges.begin(); iTemp != edges.end(); iTemp++ )
	{
		pRender->push_back( CTRect<float>( fLastX + pos.x, pos.y, *iTemp + pos.x, size.y + pos.y ) );
		fLastX = *iTemp;
	}
}

IVisReflowObject* CreateTextObject( CMLStream *pStream, int nStart, int nSize ) { return new CTextObject( pStream, nStart, nSize ); }

// CImageObject

class CImageObject: public IVisReflowObject
{
	OBJECT_BASIC_METHODS(CImageObject)
private:
	ZDATA
	int nBorder;
	CTPoint<int> imageSize;
	CTPoint<float> size;
	CTPoint<float> position;
	////
	CRectLayout rects;
	CDBPtr<NDb::STexture> pTexture;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nBorder); f.Add(3,&imageSize); f.Add(4,&size); f.Add(5,&position); f.Add(6,&rects); f.Add(7,&pTexture); return 0; }

public:
	CImageObject() {}
	CImageObject( NDb::STexture *pTexture, const CTPoint<int> &size, int nBorder );

	void Update( IReflowState *pState );

	const CTPoint<float>& GetPosition() const { return position; }
	void SetPosition( const CTPoint<float> &_position ) { position = _position; }

	void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &position, const CTRect<float> &window );
	void Render( list<CTRect<float> > *pRender, const CTPoint<float> &position, const CTRect<float> &window );

	const CTPoint<float>& GetSize() const { return size; }
};

CImageObject::CImageObject( NDb::STexture *_pTexture, const CTPoint<int> &_size, int _nBorder ):
	pTexture(_pTexture), imageSize(_size), nBorder(_nBorder)
{
}

void CImageObject::Update( IReflowState *pState )
{
	int nx, ny;
	Singleton<IUIInitialization>()->GetVirtualScreenController()->GetResolution( &nx, &ny );
	CVec2 screenSize( nx, ny );

	CVec2 scale( screenSize.x / 1024.0f, screenSize.y / 768.0f );
	CVec2 imageScreenSize( imageSize.x * scale.x, imageSize.y * scale.y );
	CVec2 borderScreenSize( nBorder * scale.x, nBorder * scale.y );

	size.x = imageScreenSize.x + borderScreenSize.x * 2;
	size.y = imageScreenSize.y + borderScreenSize.y * 2;

	if ( !pTexture )
		return;

	CTRect<float> texRect;
	texRect.x1 = 0;
	texRect.x2 = pTexture->nWidth;
	texRect.y1 = pTexture->nHeight;
	texRect.y2 = 0;
	rects.AddRect(  borderScreenSize.x,  borderScreenSize.y, imageScreenSize.x, imageScreenSize.y, texRect );
}

void CImageObject::Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &_position, const CTRect<float> &window )
{
	pView->CreateDynamicRects( pTexture, rects, _position + position, window );
}

void CImageObject::Render( list<CTRect<float> > *pRender, const CTPoint<float> &_position, const CTRect<float> &window )
{
	pRender->push_back( CTRect<float>( _position.x, _position.y, size.x + _position.x, size.y + _position.y ) );
}

IVisReflowObject* CreateImageObject( NDb::STexture *pTexture, const CTPoint<int> &size, int nBorder ) { return new CImageObject( pTexture, size, nBorder ); }

// CTabObject

class CTabObject: public IVisReflowObject
{
	OBJECT_BASIC_METHODS(CTabObject)
private:
	ZDATA
	CTPoint<float> size;
	CTPoint<float> position;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&size); f.Add(3,&position); return 0; }

public:
	CTabObject();

	void Update( IReflowState *pState );

	const CTPoint<float>& GetPosition() const { return position; }
	void SetPosition( const CTPoint<float> &_position ) { position = _position; }

	void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &position, const CTRect<float> &window ) {}
	void Render( list<CTRect<float> > *pRender, const CTPoint<float> &position, const CTRect<float> &window ) {}

	const CTPoint<float>& GetSize() const { return size; }
};

CTabObject::CTabObject():
	size( 0, 0 )
{
}

void CTabObject::Update( IReflowState *pState )
{
	const SState &state = pState->GetState();

	SFontInfo fontInfo;
	GetFontFormatInfo( state.font, state.nMinFontSize, &fontInfo );

	float fSize = N_TAB_SIZE * fontInfo.pInfo->GetAveCharWidth() * fontInfo.scale.x;
//	int nPart = sInfo.fLineWidth / fSize;
//	size.x = ( nPart + 1 ) * fSize - sInfo.fLineWidth;
//	sSize.y = sInfo.fLastLineHeight;
}

IVisReflowObject* CreateTabObject() { return new CTabObject(); }

// CSpringObject

class CSpringObject: public IVisReflowObject
{
	OBJECT_BASIC_METHODS(CSpringObject)
private:
	ZDATA
	int nSize;
	CTPoint<float> size;
	CTPoint<float> position;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nSize); f.Add(3,&size); f.Add(4,&position); return 0; }

public:
	CSpringObject() {}
	CSpringObject( int nSize );

	void Update( IReflowState *pState );

	const CTPoint<float>& GetPosition() const { return position; }
	void SetPosition( const CTPoint<float> &_position ) { position = _position; }

	void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &position, const CTRect<float> &window ) {}
	void Render( list<CTRect<float> > *pRender, const CTPoint<float> &position, const CTRect<float> &window ) {}

	bool IsSpace() const { return true; }
	const CTPoint<float>& GetSize() const { return size; }
};

CSpringObject::CSpringObject( int _nSize ):
	nSize(_nSize), size( 0, 0 )
{
}

void CSpringObject::Update( IReflowState *pState )
{
	const SState &state = pState->GetState();

	SFontInfo fontInfo;
	GetFontFormatInfo( state.font, state.nMinFontSize, &fontInfo );

	size.x = nSize * fontInfo.pInfo->GetAveCharWidth() * fontInfo.scale.x;
	size.y = fontInfo.pInfo->GetHeight() * fontInfo.scale.y;
}

IVisReflowObject* CreateSpringObject( int nSize ) { return new CSpringObject( nSize ); }

} // NAMESPACE

using namespace NML;
REGISTER_SAVELOAD_CLASS( 0xB5529200, CTextObject )
REGISTER_SAVELOAD_CLASS( 0xB5529201, CImageObject )
REGISTER_SAVELOAD_CLASS( 0xB5529202, CTabObject )
REGISTER_SAVELOAD_CLASS( 0xB5529203, CSpringObject )


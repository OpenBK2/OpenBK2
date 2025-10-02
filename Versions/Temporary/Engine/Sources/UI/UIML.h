#pragma once

#include "UI_export.h"


#include "..\3dmotor\GPixelFormat.h"
#include "..\3Dmotor\GLocale.h"

interface IML;
interface IMLObject;
interface IMLLayout;

enum ECommand
{
	CMD_NULL,
	CMD_TAB,
	CMD_SPACE,
	CMD_BREAKLINE
};

namespace NGScene
{
	interface ILayoutFakeView;
}

const int
	FONT_SIZE_MASK			= 0x00FFFFFF,
	FONT_SIZE_PIXELS		= 0x10000000,
	FONT_SIZE_POINTS		= 0x20000000;

struct SState
{
	enum EHORAlign
	{
		HORALIGN_DEFAULT,
		HORALIGN_LEFT,
		HORALIGN_RIGHT,
		HORALIGN_CENTER,
		HORALIGN_NOWRAP,
		HORALIGN_JUSTIFY,
		HORALIGN_WRAP_LEFT,
		HORALIGN_WRAP_RIGHT
	};
	enum EVERTAlign
	{
		VERTALIGN_TOP,
		VERTALIGN_BOTTOM,
		VERTALIGN_MIDDLE
	};

	//// reflow
	EHORAlign eHAlign;
	EVERTAlign eVAlign;
	//// font
	NGScene::SFont sFont;
	NGfx::SPixel8888 sColor;
	//// outline font
	int nOutlineBorder;
	NGfx::SPixel8888 sOutlineColor;
	bool bForceFontSize;
	int nMinFontSize;

	SState(): eHAlign( HORALIGN_DEFAULT ), eVAlign( VERTALIGN_MIDDLE ), sFont( 16, "System" ), sColor( 0xFF, 0xFF, 0xFF, 0xFF ), nOutlineBorder( 0 ), sOutlineColor( 0xFF, 0xFF, 0xFF, 0xFF ), bForceFontSize( false ), nMinFontSize( 0 ) {}

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &eHAlign );
		saver.Add( 2, &eVAlign );
		saver.Add( 3, &sFont );
		saver.Add( 4, &sColor );
		saver.Add( 5, &nOutlineBorder );
		saver.Add( 6, &sOutlineColor );
		saver.Add( 7, &bForceFontSize );
		saver.Add( 8, &nMinFontSize );
		return 0;
	}
};

struct SRange
{
	float fValue;
	float fHeight;

	SRange() {}
	SRange( float _fValue, float _fHeight ): fValue( _fValue ), fHeight( _fHeight ) {}

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &fValue );
		saver.Add( 2, &fHeight );
		return 0;
	}
};

struct SReflowInfo
{
	float fY;
	float fMaxX;
	float fLineWidth, fLineHeight, fLastLineHeight;
	SRange sLeft, sRight;
	list<CPtr<IMLObject> > line, leftWraped, rightWraped;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &fY );
		saver.Add( 2, &fMaxX );
		saver.Add( 3, &fLineWidth );
		saver.Add( 4, &fLineHeight );
		saver.Add( 5, &fLastLineHeight );
		saver.Add( 6, &sLeft );
		saver.Add( 7, &sRight );
		saver.Add( 8, &line );
		saver.Add( 9, &leftWraped );
		saver.Add( 10, &rightWraped );
		return 0;
	}
};

// CMLStream

class UI_EXPORT CMLStream: public CObjectBase
{
	OBJECT_BASIC_METHODS(CMLStream)
private:
	int nPos;
	wstring wsText;
	int nSize;

public:
	CMLStream() : nPos( 0 ), nSize(0) {}

	int GetSeek() const { return nPos; }
	void Seek( int nSeek ) { ASSERT( nSeek <= wsText.length() ); nPos = nSeek; }
	void IncSeek() { ++nPos; }

 	WCHAR GetChar() const { return wsText[nPos]; }
	bool IsEof() const { return nPos >= nSize; }
	void GetString( int nStart, int nSize, wstring *pRes );
	void InsertString( const wstring &wsText );

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &nPos );
		saver.Add( 2, &wsText );
		saver.Add( 3, &nSize );
		return 0;
	}
};

// IMLObject

interface IMLObject: public CObjectBase
{
	virtual void Generate(  ) = 0;
	virtual void DynamicGenerate(  const SReflowInfo &sInfo ) = 0;

	virtual const CTPoint<float>& GetSize() const = 0;

	virtual const SState& GetState() const = 0;
	virtual void SetState( const SState &sState ) = 0;

	virtual const CTPoint<float>& GetPosition() const = 0;
	virtual void SetPosition( const CTPoint<float> &sPosition ) = 0;

	virtual void Render( list<CTRect<float> > *pRender, const CTPoint<float> &sGlobalPosition, const CTRect<float> &sWindow ) = 0;
	virtual void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &sPosition, const CTRect<float> &sWindow ) = 0;
};

IMLObject* CreateIMLTextObject( CMLStream *pStream, int nStart, int nSize );
IMLObject* CreateIMLImageObject( NDb::STexture *pTexture, SState::EHORAlign eAlign, int nBorder, int nWidth, int nHeight );
IMLObject* CreateIMLTabObject();

// IMLLayout

interface IMLLayout: public CObjectBase
{
	virtual void AddObject( IMLObject *pObject ) = 0;
	virtual void AddCommand( ECommand eCommand, IMLObject *pObject = 0 ) = 0;

	virtual const CTPoint<float>& GetSize() const = 0;

	virtual const SState& GetState() = 0;
	virtual void SetState( const SState &sState ) = 0;
	virtual void PushState() = 0;
	virtual void PopState() = 0;

	virtual void Generate(  float fWidth ) = 0;

	virtual void Render( list<CTRect<float> > *pRender, const CTPoint<float> &sPosition, const CTRect<float> &sWindow ) = 0;
	virtual void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &sPosition, const CTRect<float> &sWindow ) = 0;
};

// IMLHandler

interface UI_EXPORT IMLHandler: public CObjectBase
{
	virtual void Exec( IML *pIML, IMLLayout *pLayout, const vector<wstring> &paramsSet ) = 0;
};

// IML

interface IML: public CObjectBase
{
	virtual void SetText( const wstring &wsText, int nFlags ) = 0;
	virtual void SetFade( float fFade ) = 0;
	virtual void SetHandler( const wstring &wsTAG, IMLHandler *pHandler ) = 0;
	virtual void SetIDForHandler( int nID ) = 0;
	virtual int GetIDForHandler() const = 0;

	virtual CMLStream* GetStream() = 0;
	virtual const CTPoint<int>& GetSize() = 0;

	virtual void Generate(  int nWidth ) = 0;
	virtual void Render( list<CTRect<float> > *pRender, const CTPoint<float> &sPosition, const CTRect<float> &sWindow ) = 0;
	virtual void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &sPosition, const CTRect<float> &sWindow ) = 0;
};

IML* CreateML();



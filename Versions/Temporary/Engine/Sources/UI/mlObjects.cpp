#include "StdAfx.h"
#include "..\3dMotor\RectLayout.h"
#include "..\3dMotor\G2DView.h"
#include "..\3DMotor\Locale.h"
#include "..\3Dmotor\GLocale.h"
//
#include "mlMain.h"
#include "mlReflow.h"
#include "mlObjects.h"
#include "mlVisObjects.h"

namespace NML
{

// CLineBreakObject

class CLineBreakObject: public IReflowObject
{
	OBJECT_BASIC_METHODS(CLineBreakObject)
private:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

public:
	CLineBreakObject() {}

	void Update( IReflowState *pState ) { pState->CreateLine( true ); }
};

IReflowObject* CreateLineBreakObject() { return new CLineBreakObject; }

// CColorObject

class CColorObject: public IReflowObject
{
	OBJECT_BASIC_METHODS(CColorObject)
private:
	ZDATA
	NGfx::SPixel8888 color;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&color); return 0; }

public:
	CColorObject() {}
	CColorObject( const NGfx::SPixel8888 &_color ): color(_color) {}

	void Update( IReflowState *pState );
};

void CColorObject::Update( IReflowState *pState )
{
	SState state = pState->GetState();
	state.color = color;
	pState->SetState( state );
}

IReflowObject* CreateColorObject( const NGfx::SPixel8888 &color ) { return new CColorObject( color ); }

// CHAlignObject

class CHAlignObject: public IReflowObject
{
	OBJECT_BASIC_METHODS(CHAlignObject)
private:
	ZDATA
	EHAlign nAlign;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nAlign); return 0; }

public:
	CHAlignObject() {}
	CHAlignObject( EHAlign _nAlign ): nAlign(_nAlign) {}

	void Update( IReflowState *pState );
};

void CHAlignObject::Update( IReflowState *pState )
{
	SState state = pState->GetState();
	state.nHAlign = nAlign;
	pState->SetState( state );
}

IReflowObject* CreateHAlignObject( EHAlign nAlign ) { return new CHAlignObject( nAlign ); }

// CVAlignObject

class CVAlignObject: public IReflowObject
{
	OBJECT_BASIC_METHODS(CVAlignObject)
private:
	ZDATA
	EVAlign nAlign;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nAlign); return 0; }

public:
	CVAlignObject() {}
	CVAlignObject( EVAlign _nAlign ): nAlign(_nAlign) {}

	void Update( IReflowState *pState );
};

void CVAlignObject::Update( IReflowState *pState )
{
	SState state = pState->GetState();
	state.nVAlign = nAlign;
	pState->SetState( state );
}

IReflowObject* CreateVAlignObject( EVAlign nAlign ) { return new CVAlignObject( nAlign ); }

// CFontObject

class CFontObject: public IReflowObject
{
	OBJECT_BASIC_METHODS(CFontObject)
private:
	ZDATA
	int nFlags;
	int nOutlineSize;
	bool bForceFontSize;
	NGScene::SFont font;
	NGfx::SPixel8888 outlineColor;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nFlags); f.Add(3,&nOutlineSize); f.Add(4,&bForceFontSize); f.Add(5,&font); f.Add(6,&outlineColor); return 0; }

public:
	CFontObject() {}
	CFontObject( int _nFlags, const NGScene::SFont &_font, const NGfx::SPixel8888 &_outlineColor, int _nOutlineSize, bool _bForceFontSize ):
		nFlags(_nFlags), font(_font), outlineColor(_outlineColor), nOutlineSize(_nOutlineSize), bForceFontSize(_bForceFontSize) {}

	void Update( IReflowState *pState );
};

void CFontObject::Update( IReflowState *pState )
{
	SState state = pState->GetState();

	if ( nFlags & N_FONTOBJECT_NAME )
		state.font.szName = font.szName;
	if ( nFlags & N_FONTOBJECT_SIZE )
		state.font.nSize = font.nSize;
	if ( nFlags & N_FONTOBJECT_OUTLINESIZE )
		state.nOutlineBorder = nOutlineSize;
	if ( nFlags & N_FONTOBJECT_OUTLINECOLOR )
		state.outlineColor = outlineColor;
	if ( nFlags & N_FONTOBJECT_FORCEFONTSIZE )
		state.bForceFontSize = bForceFontSize;

	pState->SetState( state );
}

IReflowObject* CreateFontObject( int nFlags, const NGScene::SFont &font, const NGfx::SPixel8888 &outlineColor, int nOutlineSize, bool bForceFontSize )
{
	return new CFontObject( nFlags, font, outlineColor, nOutlineSize, bForceFontSize );
}

// CMinFontSizeObject

class CMinFontSizeObject: public IReflowObject
{
	OBJECT_BASIC_METHODS(CMinFontSizeObject)
private:
	ZDATA
	int nSize;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nSize); return 0; }

public:
	CMinFontSizeObject() {}
	CMinFontSizeObject( int _nSize ): nSize(_nSize) {}

	void Update( IReflowState *pState );
};

void CMinFontSizeObject::Update( IReflowState *pState )
{
	SState state = pState->GetState();
	state.nMinFontSize = nSize;
	pState->SetState( state );
}

IReflowObject* CreateMinFontSizeObject( int nSize ) { return new CMinFontSizeObject( nSize ); }

} // NAMESPACE

using namespace NML;
REGISTER_SAVELOAD_CLASS( 0xB5529190, CLineBreakObject )
REGISTER_SAVELOAD_CLASS( 0xB5529191, CColorObject )
REGISTER_SAVELOAD_CLASS( 0xB5529192, CHAlignObject )
REGISTER_SAVELOAD_CLASS( 0xB5529193, CVAlignObject )
REGISTER_SAVELOAD_CLASS( 0xB5529194, CFontObject )
REGISTER_SAVELOAD_CLASS( 0xB5529195, CMinFontSizeObject )

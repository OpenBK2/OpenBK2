#include "StdAfx.h"
#include "3dMotor/RectLayout.h"
#include "3dMotor/G2DView.h"
#include "3DMotor/Locale.h"
#include "3Dmotor/GLocale.h"

#include "mlReflow.h"

namespace NML
{

// CReflowState

class CReflowState: public IReflowState
{
private:
	typedef list<CPtr<IVisReflowObject> > TVisObjects;
	struct SRange
	{
		float fValue;
		float fHeight;

		SRange() {}
		SRange( float _fValue, float _fHeight ): fValue(_fValue), fHeight(_fHeight) {}
	};
	struct SLine
	{
		float fY;
		float fLeft;
		float fRight;
		float fWidth;
		float fHeight;
		EHAlign nHAlign;
		EVAlign nVAlign;
		TVisObjects line;

		SLine() {}
		SLine( float _fLeft, float _fRight, float _fY, float _fWidth, float _fHeight, EHAlign _nHAlign, EVAlign _nVAlign, const TVisObjects &_line ): 
			fLeft(_fLeft), fRight(_fRight), fY(_fY), fWidth(_fWidth), fHeight(_fHeight), nHAlign(_nHAlign), nVAlign(_nVAlign), line(_line) {}
	};

	float fWidth;
	SState state;
	NGScene::ILayoutFakeView* pView;
	////
	float fLineWidth;
	float fLineHeight;
	SRange rangeLeft, rangeRight;
	TVisObjects line, lineLeft, lineRight;
	list<SLine> lines;
	CTPoint<float> size;

protected:
	void ProcessWraped();
	void RemoveSpaces();

public:
	CReflowState( NGScene::ILayoutFakeView* pView, float fWidth );

	const SState& GetState() const { return state; }
	void SetState( const SState &_state ) { state = _state; }

	void AddObject( IVisReflowObject *pObject );
	void CreateLine( bool bEndBlock );
	void Finalize();

	const CTPoint<float>& GetSize() const { return size; }
	NGScene::ILayoutFakeView* GetScene() const { return pView; }
};

CReflowState::CReflowState( NGScene::ILayoutFakeView* _pView, float _fWidth ):
	pView(_pView), fWidth(_fWidth), fLineWidth(0), fLineHeight(0), rangeLeft(0,-1), rangeRight(_fWidth,-1), size(0,0)
{
}

void CReflowState::AddObject( IVisReflowObject *pObject )
{
	if ( !IsValid( pObject ) )
		return;

	const CTPoint<float> &visSize = pObject->GetSize();

	switch( state.nHAlign )
	{
	case EHA_LEFT:
	case EHA_RIGHT:
	case EHA_CENTER:
	case EHA_JUSTIFY:
		{
			if ( ( fLineWidth + visSize.x ) > ( rangeRight.fValue - rangeLeft.fValue ) )
				CreateLine( false );

			line.push_back( pObject );
			fLineWidth += visSize.x;
			fLineHeight = max( visSize.y, fLineHeight );
			break;
		}
	case EHA_NOWRAP:
		{
			line.push_back( pObject );
			fLineWidth += visSize.x;
			fLineHeight = max( visSize.y, fLineHeight );
			break;
		}
	case EHA_WRAP_LEFT:
		{
			lineLeft.push_back( pObject );
			break;
		}
	case EHA_WRAP_RIGHT:
		{
			lineRight.push_back( pObject );
			break;
		}
	}
}

void CReflowState::CreateLine( bool bEndBlock )
{
	ProcessWraped();
	RemoveSpaces();

	EHAlign nHAlign = !bEndBlock ? state.nHAlign : EHA_LEFT;
	lines.push_back( SLine( rangeLeft.fValue, rangeRight.fValue, size.y, fLineWidth, fLineHeight, nHAlign, state.nVAlign, line ) );

	size.y += fLineHeight;
	fLineWidth = 0;
	fLineHeight = 0;
	line.clear();

	if ( size.y - rangeLeft.fHeight > -FP_EPSILON )
		rangeLeft.fValue = 0;
	if ( size.y - rangeRight.fHeight > -FP_EPSILON )
		rangeRight.fValue = fWidth;
}

void CReflowState::Finalize()
{
	if ( !line.empty() )
		CreateLine( true );

	for ( list<SLine>::iterator iTemp = lines.begin(); iTemp != lines.end(); ++iTemp )
	{
		SLine &line = *iTemp;

		float fX = line.fLeft;
		float fSpaceMul = 1.0f;
		switch( line.nHAlign )
		{
		case EHA_RIGHT:
			{
				float fTotalWidth = line.fRight - line.fLeft;
				fX = line.fLeft + fTotalWidth - line.fWidth;
				break;
			}
		case EHA_CENTER:
			{
				float fTotalWidth = line.fRight - line.fLeft;
				fX = line.fLeft + Float2Int( ( fTotalWidth - line.fWidth ) / 2 );
				break;
			}
		case EHA_JUSTIFY:
			{
				float fSpaceSize = 0;
				for( TVisObjects::iterator iObject = line.line.begin(); iObject != line.line.end(); ++iObject )
				{
					if ( (*iObject)->IsSpace() )
						fSpaceSize += (*iObject)->GetSize().x;
				}

				float fTotalWidth = line.fRight - line.fLeft - line.fWidth + fSpaceSize;
				fSpaceMul = fSpaceSize > 1.0f ? fTotalWidth / fSpaceSize : 1.0f;
				break;
			}
		}

		for( TVisObjects::iterator iObject = line.line.begin(); iObject != line.line.end(); ++iObject )
		{
			const CTPoint<float> &visSize = (*iObject)->GetSize();

			switch( line.nVAlign )
			{
			case EVA_TOP:
				(*iObject)->SetPosition( CTPoint<float>( fX, line.fY ) );
				break;
			case EVA_BOTTOM:
				(*iObject)->SetPosition( CTPoint<float>( fX, line.fY + line.fHeight - visSize.y ) );
				break;
			default:
				(*iObject)->SetPosition( CTPoint<float>( fX, line.fY + ( line.fHeight - visSize.y ) / 2 ) );
				break;
			}

			if ( (*iObject)->IsSpace() )
				fX += visSize.x * fSpaceMul;
			else
				fX += visSize.x;

			size.x = Max( size.x, fX );
		}
	}
}

void CReflowState::ProcessWraped()
{
	for( TVisObjects::iterator iTemp = lineLeft.begin(); iTemp != lineLeft.end(); ++iTemp )
	{
		const CTPoint<float> &visSize = (*iTemp)->GetSize();

		(*iTemp)->SetPosition( CTPoint<float>( rangeLeft.fValue, size.y ) );
		size.x = Max( size.x, rangeLeft.fValue + visSize.x );

		rangeLeft.fValue += visSize.x;
		rangeLeft.fHeight = max( rangeLeft.fHeight, size.y + visSize.y );
	}

	for( TVisObjects::iterator iTemp = lineRight.begin(); iTemp != lineRight.end(); ++iTemp )
	{
		const CTPoint<float> &visSize = (*iTemp)->GetSize();

		(*iTemp)->SetPosition( CTPoint<float>( rangeRight.fValue - visSize.x, size.y ) );
		size.x = Max( size.x, rangeRight.fValue );

		rangeRight.fValue -= visSize.x;
		rangeRight.fHeight = max( rangeRight.fHeight, size.y + visSize.y );
	}

	lineLeft.clear();
	lineRight.clear();
}

void CReflowState::RemoveSpaces()
{
	for( TVisObjects::iterator iTemp = line.begin(); iTemp != line.end(); )
	{
		if ( !(*iTemp)->IsSpace() )
			break;

		iTemp = line.erase( iTemp );
	}

	while ( !line.empty() )
	{
		TVisObjects::iterator iTemp = line.end();
		--iTemp;

		if ( (*iTemp)->IsSpace() )
			line.erase( iTemp );
		else
			break;
	}
}

// CReflowLayout

class CReflowLayout: public IReflowLayout
{
	OBJECT_BASIC_METHODS(CReflowLayout)
private:
	typedef list<CPtr<IReflowObject> > TObjects;
	ZDATA
	TObjects objects;
	CTPoint<float> size;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&objects); f.Add(3,&size); return 0; }

public:
	CReflowLayout();

	void AddObject( IReflowObject *pObject );

	void Generate( NGScene::ILayoutFakeView *pView, float fWidth );

	void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &position, const CTRect<float> &window );
	void Render( list<CTRect<float> > *pRender, const CTPoint<float> &position, const CTRect<float> &window );

	const CTPoint<float>& GetSize() const { return size; }
};

CReflowLayout::CReflowLayout():
	size( 0, 0 )
{
}

void CReflowLayout::AddObject( IReflowObject *pObject )
{
	objects.push_back( pObject );
}

void CReflowLayout::Generate( NGScene::ILayoutFakeView *pView, float fWidth )
{
	CReflowState state( pView, fWidth );
	for( TObjects::iterator iTemp = objects.begin(); iTemp != objects.end(); ++iTemp )
	{
		IReflowObject *pObject = *iTemp;
		if ( !IsValid( pObject ) )
			continue;

		pObject->Update( &state );

		if ( CDynamicCast<IVisReflowObject> pVisObject = pObject )
			state.AddObject( pVisObject );
	}

	state.Finalize();
	size = state.GetSize();
}

void CReflowLayout::Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &position, const CTRect<float> &window )
{
	for( TObjects::iterator iTemp = objects.begin(); iTemp != objects.end(); ++iTemp )
	{
		if ( CDynamicCast<IVisReflowObject> pVisObject = *iTemp )
			pVisObject->Render( pView, position, window );
	}
}

void CReflowLayout::Render( list<CTRect<float> > *pRender, const CTPoint<float> &position, const CTRect<float> &window )
{
	for( TObjects::iterator iTemp = objects.begin(); iTemp != objects.end(); ++iTemp )
	{
		if ( CDynamicCast<IVisReflowObject> pVisObject = *iTemp )
			pVisObject->Render( pRender, position, window );
	}
}

// CreateReflowLayout

IReflowLayout* CreateReflowLayout()
{
	return new CReflowLayout();
}

} // NAMESPACE

using namespace NML;
REGISTER_SAVELOAD_CLASS( 0xB5529180, CReflowLayout )


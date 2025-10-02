#include "StdAfx.h"

#include "..\3DMotor\Gfx.h"
#include "..\3DMotor\GfxUtils.h"
#include "../System/dg.h"

#include "FullScreenFader.h"


class CFullScreenFader : public IFullScreenFader
{
	struct SColorModificatorDescr
	{
		CDGPtr<CFuncBase<CVec3> > pModificator;
		bool bModifyInterfaces;
		SColorModificatorDescr() : pModificator(0), bModifyInterfaces( true ) {}
		SColorModificatorDescr( CFuncBase<CVec3> *pColor, bool _bModifyInterfaces ) : pModificator( pColor ), bModifyInterfaces( _bModifyInterfaces ) {}
	};

	OBJECT_NOCOPY_METHODS( CFullScreenFader );
	//
	ZDATA
	NTimer::STime startTime;
	float fDuration;
	float fStartValue;
	float fEndValue;
	bool bFadeInterfaces;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&startTime); f.Add(3,&fDuration); f.Add(4,&fStartValue); f.Add(5,&fEndValue); f.Add(6,&bFadeInterfaces); return 0; }

	list<SColorModificatorDescr> colorModificatos;
	//
public:
	CFullScreenFader(): startTime( 0 ), fDuration( 0.0f ), fStartValue( 1.0f ), fEndValue( 1.0f ), bFadeInterfaces( true ) {}

	// IFullScreenFader
	virtual void Start( float _fDuration, float _fStartValue, float _fEndValue, bool bFadeInterfaces = true );

	virtual void AddColorModificator( CFuncBase<CVec3> *pvColor, bool bModifyInterfaces );
	virtual void RemoveColorModificator( CFuncBase<CVec3> *pvColor );

	virtual bool IsInProgress() { return fDuration != 0.0f; };
	virtual void Draw( const NTimer::STime &time, bool bAfterDrawingInterfaces );
	
private:
	float GetValue( const NTimer::STime & _time );
};

void CFullScreenFader::Start( float _fDuration, float _fStartValue, float _fEndValue, bool _bFadeInterfaces )
{
	startTime = 0;
	fDuration = _fDuration; 
	fStartValue = _fStartValue;
	fEndValue = _fEndValue;

	bFadeInterfaces = _bFadeInterfaces;
}

float CFullScreenFader::GetValue( const NTimer::STime &currentTime )
{
	if  ( fDuration < FP_EPSILON )
	{
		fDuration = 0.0f;
		return fEndValue;
	}

	if (startTime == 0 )
		startTime = currentTime;

	const float fFraction = 0.001f * (currentTime - startTime) / fDuration;
	if ( fFraction >= 1.0f )
	{
		fDuration = 0.0f;
		return fEndValue;
	}
	return fFraction * fEndValue + (1.0f - fFraction) * fStartValue;
}

void CFullScreenFader::Draw( const NTimer::STime &time, bool bAfterDrawingInterfaces )
{
	// ApplyModificators
	for ( list<SColorModificatorDescr>::iterator it = colorModificatos.begin(); it != colorModificatos.end(); ++it )
	{
		if ( bAfterDrawingInterfaces != it->bModifyInterfaces )
			continue;

		it->pModificator.Refresh();
		CVec4 vColor( it->pModificator->GetValue(), 1.0f );

		CVec2 vScreenSize = NGfx::GetScreenRect();

		NGfx::CRenderContext rc;
		rc.SetAlphaCombine( NGfx::COMBINE_ADD );

		NGfx::C2DQuadsRenderer quadRender;
		quadRender.SetTarget( rc, vScreenSize, NGfx::QRM_DEPTH_NONE );

		CTRect<float> rectTarget( 0, 0, vScreenSize.x, vScreenSize.y );
		const NGfx::SPixel8888 color = NGfx::GetDWORDColor( vColor );
		quadRender.AddRect( rectTarget, 0, rectTarget, color );
	}

	// Old realization
	if ( bAfterDrawingInterfaces != bFadeInterfaces )
		return;

	const int rgb = Float2Int( 255.f * GetValue( time ) );
	if ( rgb != 255 )
	{
		CVec2 vScreenSize = NGfx::GetScreenRect();

		NGfx::CRenderContext rc;
		rc.SetAlphaCombine( NGfx::COMBINE_MUL );

		NGfx::C2DQuadsRenderer quadRender;
		quadRender.SetTarget( rc, vScreenSize, NGfx::QRM_DEPTH_NONE );

		CTRect<float> rectTarget( 0, 0, vScreenSize.x, vScreenSize.y );
		const NGfx::SPixel8888 color( rgb, rgb, rgb, 255);
		quadRender.AddRect( rectTarget, 0, rectTarget, color );
	}
}

void CFullScreenFader::AddColorModificator( CFuncBase<CVec3> *pColor, bool bModifyInterfaces )
{
	RemoveColorModificator( pColor );
	colorModificatos.push_back( SColorModificatorDescr( pColor, bModifyInterfaces ) );
}

void CFullScreenFader::RemoveColorModificator( CFuncBase<CVec3> *pColor )
{
	for ( list<SColorModificatorDescr>::iterator it = colorModificatos.begin(); it != colorModificatos.end(); ++it )
	{
		if ( it->pModificator == pColor )
		{
			colorModificatos.erase( it );
			return;
		}
	}
}

IFullScreenFader *CreateFullScreenFader()
{
	return new CFullScreenFader;
}

BASIC_REGISTER_CLASS( IFullScreenFader );
REGISTER_SAVELOAD_CLASS( 0x3423B300, CFullScreenFader )


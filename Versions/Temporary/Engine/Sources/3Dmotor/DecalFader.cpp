#include "stdafx.h"
#include "GParts.h"
#include "GDecal.h"
#include "DecalFader.h"

namespace NGScene
{
static const float fFadeLimit = 0.01f;
static const float fFadeLog = log( fFadeLimit );

CDecalFader::CDecalFader( CObjectBase *_pDecal, STime _tFadeInStart, STime _tFadeInEnd, STime _tFadeOutStart, STime _tFadeOutEnd, CFuncBase<STime> *_pTime ) :
	pTime(_pTime),
	tFadeInStart( _tFadeInStart ),
	tFadeInEnd( _tFadeInEnd ),
	tFadeOutStart( _tFadeOutStart ),
	tFadeOutEnd( _tFadeOutEnd ),
	bFrozen( false )
{
	ASSERT( tFadeInStart <= tFadeInEnd );
	ASSERT( tFadeInEnd <= tFadeOutStart );
	ASSERT( tFadeOutStart <= tFadeOutEnd );

	if ( _pDecal )
	{
		CDynamicCast<CDecal> pTryDecal = _pDecal;
		if ( pTryDecal && _pTime )
			pDecal = pTryDecal;
	}
}

float CDecalFader::GetFadeValue( STime tCurrent ) const
{
	if ( tCurrent > tFadeOutEnd )
		return 0;

	if ( tCurrent > tFadeOutStart )
		return exp( fFadeLog * ( tCurrent - tFadeOutStart ) / ( tFadeOutEnd - tFadeOutStart ) );

	if ( tCurrent >= tFadeInEnd )
		return 1;

	if ( tCurrent > tFadeInStart )
		return exp( fFadeLog * ( tFadeInEnd - tCurrent ) / ( tFadeInEnd - tFadeInStart ) );
	
	return 0;
}

bool CDecalFader::Update( void *p )
{
	if ( !pDecal || !pTime )
		return false;

	if ( bFrozen )
		return true;

	pTime.Refresh();
	const STime tCurrent = pTime->GetValue();
	if ( tCurrent > tFadeOutEnd )
	{
		pDecal = 0;
		return false;
	}

	const float fVal = GetFadeValue( tCurrent );

	vector<CMObj<CObjectBase> > &decals = pDecal->GetDecals();
	for ( int i = 0; i < decals.size(); ++i )
	{
		CDynamicCast<ISomePart> pPart = decals[i];
		if ( pPart )
			pPart->SetFade( fVal );
	}

	return true;
}

void CDecalFader::ShiftTimes( STime tDelta )
{
	tFadeInStart += tDelta;
	tFadeInEnd += tDelta;
	tFadeOutStart += tDelta;
	tFadeOutEnd += tDelta;
}

void CDecalFader::SetToFadeIn()
{
	pTime.Refresh();
	const STime tCurrent = bFrozen ? 0 : pTime->GetValue();

	if ( tCurrent <= tFadeInEnd )
		return;

	if ( tCurrent <= tFadeOutStart )
	{
		ShiftTimes( tCurrent - tFadeInEnd );
		return;
	}

	const float fVal = GetFadeValue( tCurrent );
	const STime tNeed = tFadeOutStart + logf(fVal) * ( tFadeOutEnd - tFadeOutStart ) / fFadeLog;
	ShiftTimes( tCurrent - tNeed );
}

void CDecalFader::Freeze()
{
	if ( bFrozen )
		return;
	bFrozen = true;

	pTime.Refresh();
	const STime tCurrent = pTime->GetValue();

	ShiftTimes( -int(tCurrent) );
}

void CDecalFader::Unfreeze()
{
	if ( !bFrozen )
		return;
	bFrozen = false;

	pTime.Refresh();
	const STime tCurrent = pTime->GetValue();

	ShiftTimes( tCurrent );
}

}

using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x131AB340, CDecalFader )


#include "stdafx.h"

#include "CameraMapIterator.h"

CCameraMapIterator::CCameraMapIterator( float fFOV, float fYaw, float fPitch, float fDistance, 
																				const CVec2 &vScreenSize, float fMapSizeX, float fMapSizeY )
: nScreenSizeX( vScreenSize.x ), nScreenSizeY( vScreenSize.y ), nCurrX( 0 ), nCurrY( 0 )
{
	const float fStepX = 2.0f * fDistance * tan( fFOV * 0.5f );
	const float fStepY = vScreenSize.y * fStepX / ( vScreenSize.x * sin( fPitch ) );
	// 
	const float fAngle = -fYaw;
	const float fSin = sin( fAngle );
	const float fCos = cos( fAngle );
	//
	CVec2 v1( 0, fMapSizeY );
	CVec2 v2( fMapSizeX, 0 );
	CVec2 v3( fMapSizeX, fMapSizeY );
	//
	v1.Set( v1.x*fCos - v1.y*fSin, v1.x*fSin + v1.y*fCos );
	v2.Set( v2.x*fCos - v2.y*fSin, v2.x*fSin + v2.y*fCos );
	v3.Set( v3.x*fCos - v3.y*fSin, v3.x*fSin + v3.y*fCos );

	CTRect<float> rcBigRect;
	rcBigRect.minx = Min( 0.0f, v1.x );
	rcBigRect.minx = Min( rcBigRect.minx, v2.x );
	rcBigRect.minx = Min( rcBigRect.minx, v3.x );
	rcBigRect.miny = Min( 0.0f, v1.y );
	rcBigRect.miny = Min( rcBigRect.miny, v2.y );
	rcBigRect.miny = Min( rcBigRect.miny, v3.y );
	rcBigRect.maxx = Max( 0.0f, v1.x );
	rcBigRect.maxx = Max( rcBigRect.maxx, v2.x );
	rcBigRect.maxx = Max( rcBigRect.maxx, v3.x );
	rcBigRect.maxy = Max( 0.0f, v1.y );
	rcBigRect.maxy = Max( rcBigRect.maxy, v2.y );
	rcBigRect.maxy = Max( rcBigRect.maxy, v3.y );
	//
	const CVec3 vAxisX(	fCos, fSin, 0 );
	const CVec3 vAxisY( -fSin, fCos, 0 );
	vStepX.Set(	vAxisX.x * fStepX,	vAxisY.x * fStepX, 0 );
	vStepY.Set( -vAxisX.y * fStepY, -vAxisY.y * fStepY, 0 );
	vStartAnchor.Set( rcBigRect.minx*vAxisX.x + rcBigRect.maxy*vAxisX.y,
										rcBigRect.minx*vAxisY.x + rcBigRect.maxy*vAxisY.y, 0 );
	vCurrAnchor = vStartAnchor;
	//
	nNumStepsX = rcBigRect.GetSizeX() / fStepX + 1;
	if ( fmod(rcBigRect.GetSizeX(), fStepX) > fStepX*0.5f )
		nNumStepsX += 1;
	nNumStepsY = rcBigRect.GetSizeY() / fStepY + 1;
	if ( fmod(rcBigRect.GetSizeY(), fStepY) > fStepY*0.5f )
		nNumStepsY += 1;
}

bool CCameraMapIterator::Next()
{
	if ( nCurrY >= nNumStepsY )
		return false;
	if ( ++nCurrX < nNumStepsX )
	{
		CalcCurrAnchor();
		return true;
	}
	else
	{
		nCurrX = 0;
		if ( ++nCurrY < nNumStepsY )
		{
			CalcCurrAnchor();
			return true;
		}
	}
	return false;
}



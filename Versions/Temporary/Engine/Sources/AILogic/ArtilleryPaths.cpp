#include "stdafx.h"

#include "ArtilleryPaths.h"
#include "../Common_RTS_AI/BasePathUnit.h"
#include "../Common_RTS_AI/StaticMapHeights.h"

REGISTER_SAVELOAD_CLASS( 0x1108D4BA, CArtilleryCrewPath );
REGISTER_SAVELOAD_CLASS( 0x1108D4BB, CArtilleryBeingTowedPath );

////BASIC_REGISTER_CLASS( CArtilleryBeingTowedPath );

//*******************************************************************
//*												CArtilleryCrewPath												*
//*******************************************************************

CArtilleryCrewPath::CArtilleryCrewPath( CBasePathUnit *_pUnit, const CVec2 &vStartPoint, const CVec2 &_vEndPoint, const float fMaxSpeed )
: pUnit( _pUnit ), vCurPoint( vStartPoint )
{
	SetParams( _vEndPoint, fMaxSpeed );
}

/*void CArtilleryCrewPath::GetSpeed3( CVec3 *pSpeed ) const
{
	*pSpeed = vSpeed3;
}*/

void CArtilleryCrewPath::SetParams( const CVec2 &_vEndPoint, const float fMaxSpeed, const CVec2 &_vSpeed2 )
{ 
	SetParams( _vEndPoint, fMaxSpeed );
	vSpeed3 = CVec3( _vSpeed2, 0.0f );
}

void CArtilleryCrewPath::SetParams( const CVec2 &_vEndPoint, const float fMaxSpeed )
{ 
	bNotInitialized = false;
	
	vEndPoint = _vEndPoint;
	fSpeedLen = fMaxSpeed;
	bSelfSpeed = fMaxSpeed == 0.0f;
	vSpeed3 = VNULL3;
}

bool CArtilleryCrewPath::Init( CBasePathUnit *_pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) 
{
	CPtr<IPath> p = pPath;
	pUnit = _pUnit;
	vCurPoint = pUnit->GetCenterPlain();
	fSpeedLen = 0.0f;
	bNotInitialized = true;

	return true;
}

bool CArtilleryCrewPath::IsFinished() const
{
	return bNotInitialized || fabs2( vEndPoint - vCurPoint ) < 0.01f;
}

bool CArtilleryCrewPath::Init( IMemento *pMemento, CBasePathUnit *_pUnit, CAIMap *pAIMap )
{
	CPtr<IMemento> p = pMemento;
	pUnit = _pUnit;
	vCurPoint = pUnit->GetCenterPlain();
	fSpeedLen = 0.0f;
	bNotInitialized = true;

	return true;
}

void CArtilleryCrewPath::Segment( const NTimer::STime timeDiff )
{
	if ( vEndPoint == vCurPoint || bNotInitialized ) // уже дошли
		fSpeedLen = 0.0f;
	else
	{
		if ( bSelfSpeed )
			fSpeedLen = pUnit->GetMaxSpeedHere();
		
		float fPassedLenght = fSpeedLen * timeDiff;
		CVec2 vDir = vEndPoint - vCurPoint;
		float fDistToGo = fabs( vDir );
		if ( fDistToGo >= fPassedLenght )// еще нужно идти
		{
			Normalize( &vDir );	
			vDir *= fPassedLenght ;
			vCurPoint += vDir;
			pUnit->SetDirectionVec( vDir );
		}
		else
			vCurPoint = vEndPoint;		
	}
	pUnit->SetCenter( GetHeights()->Get3DPoint( vCurPoint ) );
}

//*******************************************************************
//*											CArtilleryBeingTowedPath										*
//*******************************************************************

CArtilleryBeingTowedPath::CArtilleryBeingTowedPath( const float fSpeedLen, const CVec2 &vCurPoint, const CVec2 &vSpeed )
{ 
	Init( fSpeedLen, vCurPoint, vSpeed ); 
}

bool CArtilleryBeingTowedPath::Init( float _fSpeedLen, const class CVec2 &_vCurPoint, const CVec2 &_vSpeed )
{ 
	fSpeedLen = _fSpeedLen;
	vCurPoint = GetHeights()->Get3DPoint( _vCurPoint );
	vCurPoint2D = _vCurPoint;
	vSpeed = _vSpeed;

	return true;
}



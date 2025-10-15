#include "stdafx.h"

#include "TankPitPath.h"
#include "Common_RTS_AI/BasePathUnit.h"
#include "Common_RTS_AI/StaticMapHeights.h"

#include "AILogic_export.h"

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D4C3, CTankPitPath );

//*******************************************************************
//*												CTankPitPath															*
//*******************************************************************

CTankPitPath::CTankPitPath ( CBasePathUnit *_pUnit, const class CVec2 &vStartPoint, const class CVec2 &vEndPoint )
: vCurPoint( vStartPoint ), vEndPoint( vEndPoint ), fSpeedLen( 0.0f ), pUnit( _pUnit )
{
}

bool CTankPitPath::IsFinished() const
{
	return vEndPoint == vCurPoint || pUnit == 0;
}

void CTankPitPath::Segment( const NTimer::STime timeDiff )
{
	if ( vEndPoint == vCurPoint ) // уже дошли
		fSpeedLen = 0;
	else if ( pUnit )
	{
		fSpeedLen = pUnit->GetMaxSpeedHere();
		float fPassedLenght = fSpeedLen*timeDiff;
		CVec2 vDir = vEndPoint - vCurPoint;
		float fDistToGo = fabs( vDir );
		if ( fDistToGo >= fPassedLenght )// еще нужно идти
		{
			Normalize( &vDir );	
			vDir *= fPassedLenght ;
			vCurPoint += vDir;
		}
		else
			vCurPoint = vEndPoint;		
	}
	
	pUnit->SetCenter( CVec3( vCurPoint.x, vCurPoint.y, GetHeights()->GetZ( vCurPoint ) ) );
}




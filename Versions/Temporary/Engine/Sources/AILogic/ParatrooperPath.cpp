#include "stdafx.h"

#include "ParatrooperPath.h"
#include "../Common_RTS_AI/AIMap.h"
#include "../Common_RTS_AI/StaticMapHeights.h"
#include "AIUnit.h"

REGISTER_SAVELOAD_CLASS( 0x1108D4B9, CParatrooperPath );
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern NTimer::STime curTime;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*												CParatrooperPath													*
//*******************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CParatrooperPath::CParatrooperPath( const CVec3 &startPoint, CAIUnit *_pUnit )
: vStartPoint ( startPoint ), vCurPoint(startPoint), pUnit( _pUnit )
{
	Init();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CParatrooperPath::FindFreeTile()
{
	//finishPoint = CVec3()
	CVec2 landPoint(vStartPoint.x, vStartPoint.y);
	SVector centerTile( AICellsTiles::GetTile( landPoint ) );

	if ( GetAIMap()->IsTileInside( centerTile ) && !GetTerrain()->IsLocked( centerTile, EAC_HUMAN ) )
	{
		landPoint.x = vStartPoint.x;
		landPoint.y = vStartPoint.y;
		vFinishPoint = CVec3(  landPoint, GetHeights()->GetZ( landPoint ) );
		return;//found
	}
	else
	{
		for ( int i = centerTile.x-SConsts::PARADROP_SPRED; i < centerTile.x+SConsts::PARADROP_SPRED; ++i )
		{
			for ( int j = centerTile.y-SConsts::PARADROP_SPRED; j < centerTile.y+SConsts::PARADROP_SPRED; ++j )
			{
				if ( GetAIMap()->IsTileInside( i, j ) && !GetTerrain()->IsLocked( i, j, EAC_HUMAN ) )
				{
					landPoint = AICellsTiles::GetPointByTile( SVector(i,j));
					vFinishPoint = CVec3(  landPoint, GetHeights()->GetZ( landPoint ) );
					return;//found
				}
			}
		}
	}
	
	//fall to locked tile ( death will occur ) (last)
	vFinishPoint = CVec3( landPoint, GetHeights()->GetZ( landPoint ) );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CParatrooperPath::IsFinished() const
{
	return vCurPoint.z <= GetHeights()->GetZ( vCurPoint.x, vCurPoint.y );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CParatrooperPath::Init()
{
	lastPathUpdateTime = curTime;
	FindFreeTile();
	// calculate horizontal speed.
	const CVec3 curP( vCurPoint.x, vCurPoint.y, 0 );
	const CVec3 finishP ( vFinishPoint.x, vFinishPoint.y, 0 );
	const int height = vStartPoint.z - GetHeights()->GetZ( finishP.x,finishP.y );
	const float fallTime = height/SConsts::PARATROOPER_FALL_SPEED;
	vHorSpeed = (finishP-curP)/fallTime;
	fSpeedLen = SConsts::TILE_SIZE / 3600.0f; // скорость парашютиста всегда такая, для анимации

	vFinishPoint2D.x = vFinishPoint.x;
	vFinishPoint2D.y = vFinishPoint.y;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CParatrooperPath::GetSpeed3( CVec3 *vSpeed ) const
{
	vSpeed->x = vHorSpeed.x;
	vSpeed->y = vHorSpeed.y;
	vSpeed->z = - SConsts::PARATROOPER_FALL_SPEED;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float CParatrooperPath::CalcFallTime( const float fZ )
{
	return fZ / SConsts::PARATROOPER_FALL_SPEED;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CParatrooperPath::Segment( const NTimer::STime timeDiff )
{
	if ( curTime - lastPathUpdateTime > SConsts::PARATROOPER_GROUND_SCAN_PERIOD &&
			 GetTerrain()->IsLocked( AICellsTiles::GetTile(vFinishPoint.x,vFinishPoint.y), EAC_HUMAN ) )
	{
		Init();
	}

	if ( !IsFinished() )
	{
		//horSpeed;
		vCurPoint += vHorSpeed * timeDiff;
		vCurPoint.z -= timeDiff * SConsts::PARATROOPER_FALL_SPEED;
	}			
	if ( IsFinished() )
		vCurPoint = vFinishPoint;

	pUnit->SetCenter( vCurPoint, true ); 
	return;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


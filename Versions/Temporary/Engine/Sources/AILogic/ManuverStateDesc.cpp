#include "StdAfx.h"
#include "./manuverstatedesc.h"
#include "IPlane.h"
#include "PlanePreferences.h"
#include "ManuverInternal.h"
#include "../Stats_B2_M1/DBPlaneManuvers.h"


//	CManuverStateDesc


void CManuverStateDesc::Init( const enum EPlanesAttitude _att, struct IPlane *pPos, struct IPlane *pEnemy )
{
	att = _att;

	const CVec3 vSpeed1 = pPos->GetSpeedB2();
	const CVec3 vPos1 = pPos->GetPosB2();
	const CVec3 vSpeed2 = pEnemy->GetSpeedB2();
	const CVec3 vPos2 = pEnemy->GetPosB2();
	const CVec3 vDist = vPos2 - vPos1;
	const CPlanePreferences &pref1 = pPos->GetPreferencesB2();
	const CPlanePreferences &pref2 = pEnemy->GetPreferencesB2();
	const float fTurnR1 = pref1.GetR( fabs( vSpeed1 ) );


	enemyDirection = DirsDifference( GetDirectionByVector( vSpeed1.x, vSpeed1.y ),
		GetDirectionByVector( vDist.x, vDist.y ) ) * 2.0f / 65536.0f;

	selfDirection = DirsDifference( GetDirectionByVector( vSpeed2.x, vSpeed2.y ),
		GetDirectionByVector( -vDist.x, -vDist.y ) ) * 2.0f / 65536.0f;

	speedAngle = DirsDifference( GetDirectionByVector( vSpeed2.x, vSpeed2.y ), GetDirectionByVector( vSpeed1.x, vSpeed1.y ) ) * 2.0f / 65536.0f;

	distance = fabs( vDist ) / fTurnR1;
	selfHeight = ( vPos1.z - SPlanesConsts::GetMinHeight() ) / fTurnR1;
	heightDifference = ( vPos2.z - vPos1.z ) / fTurnR1;
	selfSpeed = ( fabs( vSpeed1 ) - pref1.GetStallSpeed() ) / pref1.GetMaxSpeed();
	enemySpeed = ( fabs( vSpeed2 ) - pref2.GetStallSpeed() ) / pref2.GetMaxSpeed();
}

bool CManuverStateDesc::CheckSuitable( const NDb::SManuverDescriptor *pDesc ) const
{
	return 
		IsParamSuitable( enemyDirection, pDesc->conditions.pEnemyDirection ) &&
		IsParamSuitable( selfDirection, pDesc->conditions.pSelfDirection ) &&
		IsParamSuitable( speedAngle, pDesc->conditions.pSpeedAngle ) &&
		IsParamSuitable( distance, pDesc->conditions.pDistance ) &&
		IsParamSuitable( selfHeight, pDesc->conditions.pSelfHeight ) &&
		IsParamSuitable( heightDifference, pDesc->conditions.pHeightDifference ) &&
		IsParamSuitable( selfSpeed, pDesc->conditions.pSelfSpeed ) &&
		IsParamSuitable( enemySpeed, pDesc->conditions.pEnemySpeed );
}


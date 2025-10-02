#include "StdAfx.h"

#include "ManuverInternal.h"
#include "PlanePreferences.h"
#include "Stats_B2_M1/RPGStats.h"


void CPlanePreferences::Init( const SMechUnitRPGStats* _pStats, bool _bCanViolateHeghtLimits )
{
	bCanViolateHeghtLimits = _bCanViolateHeghtLimits;
	pStats =  _pStats;
}
WORD CPlanePreferences::GetDivingAngle() const
{
	return pStats->wDivingAngle;
}

float CPlanePreferences::GetPatrolHeight() const 
{
	return Clamp( pStats->fMaxHeight, SPlanesConsts::GetMinHeight(), SPlanesConsts::MAX_HEIGHT );
	//return (SPlanesConsts::MAX_HEIGHT + SPlanesConsts::GetMinHeight()) / 2.0f ;
}

float CPlanePreferences::GetR( const float fSpeed ) const 
{ 
	return pStats->fTurnRadius / pStats->fSpeed * fSpeed;
}

float CPlanePreferences::GetStallSpeed() const 
{ 
	return pStats->fSpeed / 3.0f; 
}

bool CPlanePreferences::CanFlip() const
{
	return bCanViolateHeghtLimits || 
				 pStats->etype == RPG_TYPE_AVIA_ATTACK ||
				 pStats->etype == RPG_TYPE_AVIA_FIGHTER;
}

float CPlanePreferences::GetMaxSpeed() const 
{
	return pStats->fSpeed * 2.0f; 
}

float CPlanePreferences::GetTiltSpeed() const 
{ 
	return pStats->fTiltSpeed; 
}

float CPlanePreferences::GetTiltAccell() const 
{ 
	return pStats->fTiltAcceleration; 
}

float CPlanePreferences::GetSpeed( const float fZ ) const
{
	return GetMaxSpeed() -
		( Clamp( fZ, SPlanesConsts::MIN_HEIGHT, SPlanesConsts::MAX_HEIGHT ) - SPlanesConsts::GetMinHeight() ) /
		( SPlanesConsts::MAX_HEIGHT - SPlanesConsts::GetMinHeight() ) *
		( GetMaxSpeed() - GetStallSpeed() );
}

bool CPlanePreferences::IsManuverAllowed( const enum NDb::EManuverID eManuver ) const
{
	return pStats->manuverMap.find( eManuver ) != pStats->manuverMap.end();
}


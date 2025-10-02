#include "stdafx.h"

#include "ArtilleryBulletStorage.h"

REGISTER_SAVELOAD_CLASS( 0x1108D4A0, CArtilleryBulletStorage );

//BASIC_REGISTER_CLASS( CArtilleryBulletStorage );

CArtilleryBulletStorage::CArtilleryBulletStorage( const SStaticObjectRPGStats * _pStats, const CVec3 &center, const float fHP, const int nFrameIndex, CAIUnit *_pOwner )
: CGivenPassabilityStObject( center, fHP, 0, nFrameIndex ), pStats( _pStats ), pOwner( _pOwner )
{
}

void CArtilleryBulletStorage::MoveTo( const CVec3 &newCenter )
{
	SetNewPlaceWithoutMapUpdate( newCenter );
	Init();
}



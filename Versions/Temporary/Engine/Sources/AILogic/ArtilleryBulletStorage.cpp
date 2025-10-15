#include "stdafx.h"

#include "ArtilleryBulletStorage.h"

#include "AILogic_export.h"

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D4A0, CArtilleryBulletStorage );

//BASIC_REGISTER_CLASS( AILOGIC, CArtilleryBulletStorage );

CArtilleryBulletStorage::CArtilleryBulletStorage( const SStaticObjectRPGStats * _pStats, const CVec3 &center, const float fHP, const int nFrameIndex, CAIUnit *_pOwner )
: CGivenPassabilityStObject( center, fHP, 0, nFrameIndex ), pStats( _pStats ), pOwner( _pOwner )
{
}

void CArtilleryBulletStorage::MoveTo( const CVec3 &newCenter )
{
	SetNewPlaceWithoutMapUpdate( newCenter );
	Init();
}



#pragma once

#include "AIUpdates.h"
#include "ActionNotify.h"

namespace NDb
{
	struct SHPObjectRPGStats;
}

struct SSuperWeaponControl : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SSuperWeaponControl )
public:
	ZDATA_( SAIBasicUpdate )
		int nPlayer;

		int nUnitID; // -1 - no superweapon unit on map
		CDBPtr<NDb::SHPObjectRPGStats> pUnit; // use this unit for superweapon stats
		bool bEnabled; // this player has super weapon, show button (if its me)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nPlayer); f.Add(3,&nUnitID); f.Add(4,&pUnit); f.Add(5,&bEnabled); return 0; }

	SSuperWeaponControl() : nPlayer( -1 ), nUnitID( -1 ), pUnit( 0 ), bEnabled( false ) {}
	SSuperWeaponControl( const int _nPlayer, const int _nUnitID, const NDb::SHPObjectRPGStats *_pUnit, const bool _bEnabled ) : SAIBasicUpdate( ACTION_NOTIFY_SUPERWEAPON_CONTROL, 0 ),
		nPlayer( _nPlayer ), nUnitID( _nUnitID ), pUnit( _pUnit ), bEnabled( _bEnabled ) {}

};

struct SSuperWeaponRecycle : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SSuperWeaponRecycle )
public:
	ZDATA_( SAIBasicUpdate )
		int nPlayer;
		float fPartComplete; // for recycle time, from 0.0 to 1.0
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nPlayer); f.Add(3,&fPartComplete); return 0; }

	SSuperWeaponRecycle() : nPlayer( -1 ), fPartComplete( 0.0f ) {}
	SSuperWeaponRecycle( const int _nPlayer, const float _fPartComplete ) : SAIBasicUpdate( ACTION_NOTIFY_SUPERWEAPON_RECYCLE, 0 ),
		nPlayer( _nPlayer ), fPartComplete( _fPartComplete ) {}
};


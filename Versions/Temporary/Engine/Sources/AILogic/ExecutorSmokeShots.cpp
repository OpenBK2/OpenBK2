#include "StdAfx.h"

#include "ExecutorSmokeShots.h"

CExecutorSmokeShots::CExecutorSmokeShots( CAIUnit *_pUnit	) :
CExecutorUnitBonus ( NDb::ABILITY_SMOKE_SHOTS, _pUnit, TID_SMOKE_SHOTS )
{
}

void CExecutorSmokeShots::SwitchingOffEnd()
{
	CExecutorUnitBonus::SwitchingOffEnd();
	// Switch to normal ammo
	GetAIUnit()->SetActiveShellType( NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH );
	//updater.AddUpdate( CreateStatusUpdate( EUS_SMOKE_SHOT, true, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
}

void CExecutorSmokeShots::SwitchOnEnd()
{
	CExecutorUnitBonus::SwitchOnEnd();
	// Switch to Smoke shots
	GetAIUnit()->SetActiveShellType( NDb::SWeaponRPGStats::SShell::DAMAGE_FOG );
	//updater.AddUpdate( CreateStatusUpdate( EUS_SMOKE_SHOT, false, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
}

int CExecutorSmokeShots::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CExecutorUnitBonus*>( this ) );

	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x1913B380, CExecutorSmokeShots )


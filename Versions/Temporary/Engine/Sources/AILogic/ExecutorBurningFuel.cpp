#include "stdafx.h"
#include "ExecutorBurningFuel.h"
#include "Shell.h"

#include "AILogic_export.h"

extern CShellsStore theShellsStore;
extern NTimer::STime curTime;

CExecutorBurningFuel::CExecutorBurningFuel( const CVec3 &_vPos, const NDb::SBurningFuel *_pStats )
: CExecutor( TID_BURNING_FUEL, 20 ), vPos( _vPos ), pStats( _pStats ), timeBurn( curTime + _pStats->nBurnTime ) 
{ 
	if ( pStats->pWeaponFireOnDesctruction )
	theShellsStore.AddShell
		( new CInvisShell( curTime, new CBurstExpl( 0, pStats->pWeaponFireOnDesctruction, vPos, VNULL3, 0, false, 1, true ), 0 ) );
}

int CExecutorBurningFuel::Segment()
{
	if ( pStats->pWeaponFireEachSecond )
	{
		theShellsStore.AddShell
			( new CInvisShell( curTime, new CBurstExpl( 0, pStats->pWeaponFireEachSecond, vPos, VNULL3, 0, false, 1, true ), 0 ) );
	}
	else
		return -1;

	if ( curTime > timeBurn )
		return -1;
	else
		{ RecordRandomCall(); return NRandom::Random( 10 ) + 10; }
}

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x101CE480, CExecutorBurningFuel )

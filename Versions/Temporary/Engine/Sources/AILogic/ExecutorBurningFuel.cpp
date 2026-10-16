#include "stdafx.h"

#include "ExecutorBurningFuel.h"
#include "Shell.h"

#include "AILogic_export.h"

extern CShellsStore theShellsStore;
extern NTimer::STime curTime;

CExecutorBurningFuel::CExecutorBurningFuel( const CVec3 &_vPos, const NDb::SBurningFuel *_pStats )
	: CExecutor( TID_BURNING_FUEL, 20 )
	, vPos( _vPos )
	, pStats( _pStats )
	, timeBurn( curTime + _pStats->nBurnTime )
{
	// Read the stats through the parameter, not back through the pStats CDBPtr.
	// The member re-runs the lazy load on every use, and that path tests the
	// stored pointer against null, which let GCC construct a path where
	// _pStats is null and so report the member initialiser above as a definite
	// null dereference. The sole caller, CExistingObject::Delete in
	// StaticObject.cpp, resolves pShootOnDestruction and checks it before
	// constructing us, so a null never arrives here.
	if ( _pStats->pWeaponFireOnDesctruction )
	{
		theShellsStore.AddShell( new CInvisShell(
				curTime, new CBurstExpl( 0, _pStats->pWeaponFireOnDesctruction, vPos, VNULL3, 0, false, 1, true ), 0 ) );
	}
}

int CExecutorBurningFuel::Segment()
{
	if ( pStats->pWeaponFireEachSecond )
	{
		theShellsStore.AddShell( new CInvisShell(
				curTime, new CBurstExpl( 0, pStats->pWeaponFireEachSecond, vPos, VNULL3, 0, false, 1, true ), 0 ) );
	}
	else
	{
		return -1;
	}

	if ( curTime > timeBurn )
	{
		return -1;
	}
	else
	{
		RecordRandomCall();
		return NRandom::Random( 10 ) + 10;
	}
}

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x101CE480, CExecutorBurningFuel )

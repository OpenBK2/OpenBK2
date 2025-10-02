#include "StdAfx.h"

#include "ExecutorLinkedGrenades.h"

CExecutorLinkedGrenades::CExecutorLinkedGrenades( CAIUnit *_pUnit	) :
CExecutorUnitBonus ( NDb::ABILITY_LINKED_GRENADES, _pUnit, TID_LINKED_GRENADES )
{
}

void CExecutorLinkedGrenades::SwitchingOffEnd()
{
	CExecutorUnitBonus::SwitchingOffEnd();
	// Indicate to use one grenade
	GetAIUnit()->SetMultiShot( 1 );
}

void CExecutorLinkedGrenades::SwitchOnEnd()
{
	CExecutorUnitBonus::SwitchOnEnd();
	// Indicate to use multiple grenades
	if ( const NDb::SUnitSpecialAblityDesc *pSA = GetAbilityDesc() )
		GetAIUnit()->SetMultiShot( pSA->fParameter );
	else
		GetAIUnit()->SetMultiShot( 1 );
}

int CExecutorLinkedGrenades::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CExecutorUnitBonus*>( this ) );

	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x19140C00, CExecutorLinkedGrenades )


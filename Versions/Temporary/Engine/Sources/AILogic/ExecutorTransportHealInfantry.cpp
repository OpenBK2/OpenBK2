#include "stdafx.h"
#include "executortransporthealinfantry.h"
#include "UnitsIterators2.h"
#include "AIUnit.h"

#include "AILogic_export.h"

REGISTER_SAVELOAD_CLASS(AILOGIC, 0x1105F484, CExecutorTransportHealInfantry)

int CExecutorTransportHealInfantry::Segment()
{
	if ( IsExecutorValidInternal() )
	{
		//медицинские грузовички лечат пехоту в радиусе
		for ( CUnitsIter<0,2> iter( pUnit->GetParty(), EDI_FRIEND, pUnit->GetCenterPlain(), SConsts::MED_TRUCK_HEAL_RADIUS );
					!iter.IsFinished(); iter.Iterate() )
		{
			if ( (*iter)->GetStats()->IsInfantry() )
				(*iter)->IncreaseHitPoints( SConsts::MED_TRUCK_HEAL_PER_UPDATEDURATION );
		}
	}
	else
		return -1;

	return GetNextTime();
}

bool CExecutorTransportHealInfantry::IsExecutorValidInternal() const
{
	return IsValidObj( pUnit );
}

bool CExecutorTransportHealInfantry::NotifyEvent( const CExecutorEvent &event )
{
	return false;
}


#include "stdafx.h"

#include "GeneralInternal.h"
#include "SerializeOwner.h"
#include "GeneralIntendant.h"
#include "GeneralConsts.h"

int CSupremeBeing::operator&( IBinSaver &saver )
{
	if ( !saver.IsChecksum() )
	{
		saver.Add( 1, &generals );
		saver.Add( 2, &ironmans );
		saver.Add( 3, &delayedTasks );
	}
	return 0;
}

int CCommander::operator&( IBinSaver &saver )
{
	
	saver.Add( 1, &tasks );
	saver.Add( 2, &fMeanSeverity );
	return 0;
}

void CGeneral::OnSerialize( IBinSaver &saver )
{
	if ( saver.IsReading() )
	{
		curProcessed = enemys.begin();

		if ( enemyByRType.size() == 0 )
			enemyByRType.resize( NDb::_RT_NONE );
		SGeneralConsts::Init();
	}
}

void CGeneralTaskToResupplyCell::OnSerialize( IBinSaver &saver )
{
	SerializeOwner( 2, &pCells, &saver );
}


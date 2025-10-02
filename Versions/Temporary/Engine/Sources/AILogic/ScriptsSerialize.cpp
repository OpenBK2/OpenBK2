#include "stdafx.h"

#include "Scripts.h"

extern CScripts *pScripts;

int CScripts::operator&( IBinSaver &saver )
{
	if ( !saver.IsChecksum() )
		saver.Add( 2, &pScript );

	if ( saver.IsReading() )
	{
		pScripts = this;
	}


	saver.Add( 3, &groups );
	saver.Add( 4, &pConsole );
	saver.Add( 10, &areas );
	saver.Add( 11, &groupUnits );
	saver.Add( 12, &reinforcs );
	saver.Add( 13, &suspendedReinforcs );
	saver.Add( 14, &lastTimeToCheckSuspendedReinforcs );
	
	if ( saver.IsReading() )
		reinforcsIter = suspendedReinforcs.begin();

	saver.Add( 15, &reservePositions );
	saver.Add( 16, &bShowErrors );
	saver.Add( 17, &pGlobeScriptHandler );
	saver.Add( 18, &pMapInfo );
	saver.Add( 19, &rememberedUnits );
	return 0;
}



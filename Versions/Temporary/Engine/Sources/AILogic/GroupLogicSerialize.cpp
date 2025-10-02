#include "stdafx.h"

#include "GroupLogic.h"
#include "AIUnit.h"

int CGroupLogic::operator&( IBinSaver &saver )
{
	if ( !saver.IsChecksum() )
	{
		saver.Add( 1, &groupIds );
		saver.Add( 2, &followingUnits );
		
		saver.Add( 3, &registeredGroups );
		saver.Add( 4, &ambushGroups );
		saver.Add( 5, &ambushUnits );
		saver.Add( 6, &groupUnits );
		saver.Add( 7, &lastSegmTime );
		saver.Add( 8, &lastAmbushCheck );

		saver.Add( 9, &segmUnits );
		saver.Add( 10, &freezeUnits );
		saver.Add( 11, &firstPathUnits );
		saver.Add( 22, &secondPathUnits );
		saver.Add( 23, &pCollisionsCollector );
	}

	if ( segmUnits.empty() )
		segmUnits.resize( 2 );

	return 0;
}

int CGroupUnit::operator&( IBinSaver &saver )
{
	if ( !saver.IsChecksum() )
	{
		saver.Add( 1, &nGroup );
		saver.Add( 2, &nPos );
		saver.Add( 3, &nSubGroup );
		saver.Add( 4, &vShift );
		saver.Add( 5, &nSpecialGroup );
		saver.Add( 6,	&nSpecialPos );
	}
	return 0;
}


#include "stdafx.h"

#include "GlobeUpdater.h"

#include "AILogic_export.h"

void CUpdates2Globe::AddUpdate( CObjectBase *pUpdate )
{
	singleUpdates.push_back( pUpdate );
}

CObjectBase* CUpdates2Globe::GetUpdate()
{
	if ( singleUpdates.empty() )
		return 0;

	CPtr<CObjectBase> pUpdate = singleUpdates.front();
	singleUpdates.pop_front();

	return pUpdate.Extract();
}

void CUpdates2Globe::Segment()
{
	// nobody takes updates, clear all
	if ( NGlobal::GetVar( "m1", 0 ) == 0 )
		singleUpdates.clear();
}

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x30133400, CUpdates2Globe );


#include "stdafx.h"

#include "ExecutorContainer.h"
#include "KillCorpseExecutor.h"
#include "FakeObjects.h"

#include "AILogic_export.h"

extern NTimer::STime curTime;

bool CKillCorpseExecutor::IsExecutorValid() const
{ 
	return IsValid( pObject ) && pObject->IsAlive();
}

CKillCorpseExecutor::CKillCorpseExecutor( CFakeCorpseStaticObject *_pObject )
: pObject( _pObject ), CExecutor( TID_FAKE_CORPSE, 0 )
{
	killTime = curTime + SConsts::TIME_TO_DISAPPEAR;
}

int CKillCorpseExecutor::Segment()
{
	if ( IsExecutorValid() )
	{
		// Legacy saves omitted the deadline. Start a deterministic lifetime here,
		// after the simulation clock has been restored, rather than deleting at load.
		if ( killTime == 0 )
			killTime = curTime + SConsts::TIME_TO_DISAPPEAR;

		if ( curTime >= killTime )
		{
			pObject->Die( pObject->GetHitPoints() + 1 );
			return -1;
		}

		const int nSegmentsLeft = (std::max)( 1, int(killTime - curTime)/SConsts::AI_SEGMENT_DURATION );
		return (std::min)( MAX_SEGMENT_DELAY - 1, nSegmentsLeft );
	}

	return -1;
}

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x30147C40, CKillCorpseExecutor )


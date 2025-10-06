#include "stdafx.h"

#include "ExecutorContainer.h"
#include "AllowFakeObjToCrushExecutor.h"
#include "FakeObjects.h"

extern NTimer::STime curTime;

CAllowFakeObjToCrushExecutor::CAllowFakeObjToCrushExecutor()
: changeTypeTime( 0 ), eNewType( ESOT_FAKE_CORPSE ),
	CExecutor( TID_ALLOW_FAKE_OBJ_TO_CRUSH, 0 )
{
}

bool CAllowFakeObjToCrushExecutor::IsExecutorValid() const
{ 
	return IsValid( pObject ) && pObject->IsAlive();
}

CAllowFakeObjToCrushExecutor::CAllowFakeObjToCrushExecutor( CFakeCorpseStaticObject *_pObject, const int nTimeDelta, const EStaticObjType eType )
: pObject( _pObject ), eNewType( eType )
{
	changeTypeTime = curTime + nTimeDelta;
}

int CAllowFakeObjToCrushExecutor::Segment()
{
	if ( IsExecutorValid() )
	{
		if ( curTime >= changeTypeTime )
		{
			pObject->ChangeType( eNewType );
			return -1;
		}

		const int nSegmentsLeft = (std::max)( 1, int(changeTypeTime - curTime)/SConsts::AI_SEGMENT_DURATION );
		return (std::min)( MAX_SEGMENT_DELAY - 1, nSegmentsLeft );
	}

	return -1;
}

REGISTER_SAVELOAD_CLASS( 0x3014AC01, CAllowFakeObjToCrushExecutor )


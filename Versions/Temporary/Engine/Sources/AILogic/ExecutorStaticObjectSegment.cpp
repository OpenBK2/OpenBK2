#include "StdAfx.h"
#include "./executorstaticobjectsegment.h"
#include "StaticObject.h"

extern NTimer::STime curTime;

REGISTER_SAVELOAD_CLASS(0x110684C0,CExecutorStaticObjectSegment)

CExecutorStaticObjectSegment::CExecutorStaticObjectSegment( CStaticObject *_pObj )
: CExecutor(TID_STATIC_OBJECT_SEGMENT, (_pObj->GetNextSegmentTime() - curTime ) / SConsts::AI_SEGMENT_DURATION), pObject( _pObj ) 
{
}

bool CExecutorStaticObjectSegment::IsExecutorValidInternal() const
{
	return IsValidObj( pObject );
}

int CExecutorStaticObjectSegment::Segment()
{
	if ( IsExecutorValidInternal() && !pObject->IsTerminateExecutors() )
	{
		pObject->Segment();
		
		if ( pObject->IsTerminateExecutors() )
			return -1;
		else
			return (pObject->GetNextSegmentTime() - curTime) / SConsts::AI_SEGMENT_DURATION ;
	}
	else
		return -1;

//	return GetNextTime();
}




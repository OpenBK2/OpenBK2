#include "stdafx.h"
#include "./executorrestoretransparencyqueue.h"
#include "StaticObject.h"
#include "ExecutorContainer.h"
#include "StaticObjectsIters.h"

#include "AILogic_export.h"

EXTERNVAR CExecutorContainer theExecutorContainer;

namespace NRestoreTranspConsts
{
	static const int nObjectsPerCalculation = 5;
	static const int nCalcPeriod = 1;
	static const int nCalcPeriodRandom = 2;
}

using namespace NRestoreTranspConsts;

int CExecutorRestoreTransparencyQueue::Segment()
{
	// ask all object around, fill queue

	for ( CObjectsToAskAround::const_iterator it = objectsToAskAround.begin();  it != objectsToAskAround.end(); ++it  )
	{
		for ( CStObjCircleIter<false> iter( it->first, SConsts::TILE_SIZE * 30 ); !iter.IsFinished(); iter.Iterate() )
		{

			const int nNewID = (*iter)->GetUniqueId();
			CObjectsToRestore::iterator pos = objectsToRestore.find( nNewID );
			if ( pos == objectsToRestore.end() )
			{
				objectsToRestore[nNewID] = true;
				objectQueue.push_back( nNewID );
			}
		}
	}
	objectsToAskAround.clear();

	if ( !objectQueue.empty() )
	{
		// process some objects, 
		int nRestored = 0;
		while ( nRestored < nObjectsPerCalculation && !objectQueue.empty() ) 
		{
			const int nID = objectQueue.front();
			CObjectsToRestore::iterator pos = objectsToRestore.find( nID );
			
			if ( pos != objectsToRestore.end() )
			{
				if ( CLinkObject::IsLinkObjectExists( nID ) )
				{
					CLinkObject * pLinkObj = CLinkObject::GetObjectByUniqueId( nID );
					CExistingObject *pObj = checked_cast<CExistingObject *>( pLinkObj );
					if ( pObj )
						pObj->RestoreTransparenciesImmidiately(); 
					++nRestored;
				}
				objectsToRestore.erase( pos );
			}
			objectQueue.pop_front();
		}
		RecordRandomCall();
		return nCalcPeriod + NRandom::Random( 0, nCalcPeriodRandom );
	}
	else
		return 0;
}

void CExecutorRestoreTransparencyQueue::RegisterOnEvents( IExecutorContainer *pContainer )
{
	SExecutorEventParam par( EID_ADD_TO_RESTORE_TRANSPARENCY_QUEUE, 0, 0 );
	pContainer->RegisterOnEvent( this, par );
}

bool CExecutorRestoreTransparencyQueue::NotifyEvent( const CExecutorEvent &_event )
{
	const CExecutorEventAddToRestoreTransparencyQueue *event = static_cast<const CExecutorEventAddToRestoreTransparencyQueue *>( &_event );

	const int nID = event->GetID();
	if ( nID != 0 )
	{
		// add this object to queue, wake up
		if ( CLinkObject::IsLinkObjectExists( nID ) )
		{
			CLinkObject * pLinkObj = CLinkObject::GetObjectByUniqueId( nID );
			CStaticObject *pObj = checked_cast<CStaticObject *>( pLinkObj );
			theExecutorContainer.RemoveSleeping( this );
			const CVec3 vCenter( pObj->GetCenter() );
			objectsToAskAround[CVec2(vCenter.x, vCenter.y)] = true;
			return true;
		}
	}

	return false;
}

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1111BB40, CExecutorRestoreTransparencyQueue )


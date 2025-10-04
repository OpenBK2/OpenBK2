#include "stdafx.h"
#include "executorcontainer.h"
#include "ExecutorSimpleEvent.h"

CExecutorContainer theExecutorContainer;
struct IExecutorContainer * pTheExecutorsContainer = 0;


REGISTER_SAVELOAD_CLASS( 0x1105F483, CExecutorContainer )

CExecutorContainer::CExecutorContainer()
{
	pTheExecutorsContainer = this;
}

CExecutorContainer::~CExecutorContainer()
{
	pTheExecutorsContainer = 0;
}

void CExecutorContainer::Clear()
{
	executors.clear();
	sleeping.clear();
	events.clear();
	executorIDs.Clear();
	nCurTime = 0;
}

void CExecutorContainer::Init()
{
	Clear();			// trying to keep it synced
	executors.resize( MAX_SEGMENT_DELAY );
	nCurTime = 0;
	checkIter = sleeping.end();
}

void CExecutorContainer::AddSleeping( CExecutor *pExecutor )
{
	sleeping.insert( pair<CPtr<CExecutor>, bool>(pExecutor,true) );
}

void CExecutorContainer::Add( CExecutor *pExecutor )
{
	executors[MakeIndex(pExecutor->GetNextTime())].push_back( SExID(pExecutor->GetID(), pExecutor) );
}

void CExecutorContainer::RegisterOnEvent( CExecutor *pExecutor, const SExecutorEventParam &ev )
{
	events[ev].push_back( pExecutor );
}

void CExecutorContainer::RaiseEvent( const SExecutorEventParam &param )
{
	CExecutorSimpleEvent event;
	event.SetParam( param );
	RaiseEvent( event );
}

void CExecutorContainer::RaiseEvent( const CExecutorEvent &event )
{
	static bool bEventRoze = false;
	NI_ASSERT( !bEventRoze, "DON'T CALL RaizeEvent from NotifyEvent, call it from Segment" );
	bEventRoze = true;

	CEvents::iterator pos = events.find( event.GetParam() );
	if ( pos == events.end() ) 
	{
		bEventRoze = false;
		return;
	}
	CExecutorIDList &cur = executors[MakeIndex(0)];
	for ( CExecutorList::iterator it = pos->second.begin(); it != pos->second.end(); ++it )
	{
		if ( (*it)->NotifyEvent( event ) )				// want to run this segment
		{
			(*it)->IncID();
			cur.push_back( SExID( (*it)->GetID(), (*it) ) );
		}
	}

	bEventRoze = false;
}


void CExecutorContainer::Segment()
{
	CExecutorIDList &cur = executors[MakeIndex(0)];
	for ( CExecutorIDList::iterator it = cur.begin(); it != cur.end(); )
	{
		CExecutor *pEx = (*it).pEx;
		const int nID = (*it).nID;

		if ( pEx->GetID() != nID )
		{
			it = cur.erase( it );
		}
		else
		{
			const int nNextTime = pEx->Segment();
			if ( nNextTime > 0 )									// move to desired time
			{
				CExecutorIDList::iterator next = it;
				++next;
				executors[MakeIndex(nNextTime)].splice( executors[MakeIndex(nNextTime)].end(), cur, it );
				it = next;
			}
			else if ( nNextTime == 0 )						// sleep
			{
				sleeping[pEx];
				it = cur.erase( it );
			}
			else																	// delete executor 
				it = cur.erase( it );
		}
	}
	NI_ASSERT( cur.empty(), "must be empty container after processing" );
	// advance to next segm time
	++nCurTime;
	nCurTime %= MAX_SEGMENT_DELAY;
	
	// clear invalid sleeping executors
	if ( checkIter == sleeping.end() )
		checkIter = sleeping.begin();
	int nIter = CHECK_SLEEPING_PER_SEGMENT;
	for ( ; checkIter != sleeping.end() && nIter >= 0; --nIter )
	{
		if ( !checkIter->first->IsExecutorValid() )
			sleeping.erase( checkIter++ );
		else
			++checkIter;
	}
}

void CExecutorContainer::RemoveSleeping( CExecutor *pExecutor )
{
	CSleepingExecutors::iterator pos = sleeping.find( pExecutor );
	if ( pos != sleeping.end() )
		sleeping.erase( pos );
	checkIter = sleeping.end();
}



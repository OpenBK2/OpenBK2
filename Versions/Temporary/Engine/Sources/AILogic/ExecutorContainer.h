#pragma once

#include "Executor.h"
#include "../System/FreeIds.h"


// executors may be delayed for this amount of segments
#define MAX_SEGMENT_DELAY 0x80
// to make index
#define MAKE_INDEX_HELPER 0x7f
// this number of sleeping executers are checked 
#define CHECK_SLEEPING_PER_SEGMENT 100

class CExecutorContainer :	public IExecutorContainer
{
	OBJECT_NOCOPY_METHODS( CExecutorContainer )
	// executors queue
	 	
	struct SExID
	{
		ZDATA
		int nID;
		CPtr<CExecutor> pEx;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&pEx); return 0; }
	public:
		SExID() { }
		SExID( const int _nID, CExecutor *_pEx ) : nID( _nID ), pEx( _pEx ) {  }
	};

	// executor lauched only when SExID::nID == pExecutor->GetID()
	// otherwize it is removed from list

	typedef list<SExID> CExecutorIDList;
  typedef vector<CExecutorIDList> CExecutors;
	typedef hash_map<CPtr<CExecutor>, bool, SExecutorPtrHash> CSleepingExecutors;

	typedef list< CPtr<CExecutor> > CExecutorList;
	typedef hash_map<SExecutorEventParam, CExecutorList, SExecutorEventParamHash> CEvents;


	CSleepingExecutors::iterator checkIter;				// not all invalid sleeping checked every segment

	CExecutors executors;
	int nCurTime;
	CFreeIds executorIDs;

	// sleeping executors
	CSleepingExecutors sleeping;

	// executers, that subscribed for events
	CEvents events;

public: 
	int operator&( IBinSaver &f)
	{
		f.Add(2,&executors);
		f.Add(3,&nCurTime);
		f.Add(4,&executorIDs);
		f.Add(5,&sleeping);
		f.Add(6,&events);
		if ( f.IsReading() )
			checkIter = sleeping.end();
		return 0;
	}

	inline int MakeIndex( const int nTimeShift ) const
	{
		NI_ASSERT( nTimeShift < MAX_SEGMENT_DELAY, StrFmt("cannot add with delay %i, maximum is %i", nTimeShift, MAX_SEGMENT_DELAY) );
		return (nCurTime + nTimeShift) & MAKE_INDEX_HELPER;
	}
public:
	CExecutorContainer();
	~CExecutorContainer();

	void Clear();
	void Init();
	int CreateID(){ return executorIDs.Get();	}
	void ReturnID( const int id ) { executorIDs.Return( id ); }
	
	virtual void AddSleeping( CExecutor *pExecutor );
	virtual void RemoveSleeping( CExecutor *pExecutor );
	virtual void Add( CExecutor *pExecutor );
	virtual void RegisterOnEvent( CExecutor *pExecutor, const SExecutorEventParam &ev );
	//CRAP{ UNTILL EXECUTOR'S STRUCTURE IMPROVEMENT
	virtual void RaiseEvent( const CExecutorEvent &event );
	virtual void RaiseEvent( const SExecutorEventParam &param );
	//CRAP}
	
	virtual void Segment();
};



#pragma once

#include "Executor.h"
#include "System/FreeIDs.h"

#include <fmt/format.h>

// executors may be delayed for this amount of segments
#define MAX_SEGMENT_DELAY 0x80
// to make index
#define MAKE_INDEX_HELPER 0x7f
// this number of sleeping executers are checked
#define CHECK_SLEEPING_PER_SEGMENT 100

class CExecutorContainer : public IExecutorContainer
{
	OBJECT_NOCOPY_METHODS( CExecutorContainer )

	// executors queue

	struct SExID
	{
		ZDATA
		// Defaults cover construction before Init/load; explicit constructor values still take precedence.
		int nID = {};
		CPtr<CExecutor> pEx;

		ZEND int operator&( IBinSaver &f )
		{
			f.Add( 2, &nID );
			f.Add( 3, &pEx );
			return 0;
		}

	public:
		SExID() {}

		SExID( const int _nID, CExecutor *_pEx )
			: nID( _nID )
			, pEx( _pEx )
		{
		}
	};

	// executor lauched only when SExID::nID == pExecutor->GetID()
	// otherwize it is removed from list

	typedef std::list<SExID> CExecutorIDList;
	typedef std::vector<CExecutorIDList> CExecutors;
	typedef det_map<CPtr<CExecutor>, bool, SExecutorPtrHash> CSleepingExecutors;

	typedef std::list<CPtr<CExecutor>> CExecutorList;
	typedef det_map<SExecutorEventParam, CExecutorList, SExecutorEventParamHash> CEvents;

	CSleepingExecutors::iterator checkIter; // not all invalid sleeping checked every segment

	CExecutors executors;
	int nCurTime = {};
	CFreeIds executorIDs;

	// sleeping executors
	CSleepingExecutors sleeping;

	// executers, that subscribed for events
	CEvents events;

public:
	int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &executors );
		f.Add( 3, &nCurTime );
		f.Add( 4, &executorIDs );
		f.Add( 5, &sleeping );
		f.Add( 6, &events );
		if ( f.IsReading() )
		{
			checkIter = sleeping.end();
		}
		return 0;
	}

	inline int MakeIndex( const int nTimeShift ) const
	{
		NI_ASSERT( nTimeShift < MAX_SEGMENT_DELAY,
		           fmt::format( "cannot add with delay {}, maximum is {}", nTimeShift, MAX_SEGMENT_DELAY ) );
		return ( nCurTime + nTimeShift ) & MAKE_INDEX_HELPER;
	}

public:
	CExecutorContainer();
	~CExecutorContainer() override;

	void Clear();
	void Init();

	int CreateID() override
	{
		return executorIDs.Get();
	}

	void ReturnID( const int id ) override
	{
		executorIDs.Return( id );
	}

	void AddSleeping( CExecutor *pExecutor ) override;
	virtual void RemoveSleeping( CExecutor *pExecutor );
	void Add( CExecutor *pExecutor ) override;
	void RegisterOnEvent( CExecutor *pExecutor, const SExecutorEventParam &ev ) override;
	//CRAP{ UNTILL EXECUTOR'S STRUCTURE IMPROVEMENT
	void RaiseEvent( const CExecutorEvent &event ) override;
	void RaiseEvent( const SExecutorEventParam &param ) override;
	//CRAP}

	void Segment() override;
};

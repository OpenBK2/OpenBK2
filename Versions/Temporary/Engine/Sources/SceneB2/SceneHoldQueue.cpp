#include "StdAfx.h"

#include "../Misc/Pool.h"

const int N_HOLD_TIME = 64; // seconds, must be power of 2
typedef CPool<CObj<CObjectBase>, 64> CScenePool;
static CScenePool sceneHoldQueue[N_HOLD_TIME];
static CScenePool serializableSceneHoldQueue[N_HOLD_TIME];
static NTimer::STime timeLastTime = 0;

void SetToSceneHoldQueue( CObjectBase *p, bool bSerialize )
{
	const int nCurrPos = int( timeLastTime ) & ( N_HOLD_TIME - 1 );
	if ( bSerialize )
		*serializableSceneHoldQueue[nCurrPos].Alloc() = p;
	else
		*sceneHoldQueue[nCurrPos].Alloc() = p;
}

void ClearSceneHoldQueue()
{
	for ( int i = 0; i < N_HOLD_TIME; ++i )
	{
		sceneHoldQueue[i].Clear();
		serializableSceneHoldQueue[i].Clear();
	}
}

void StepSceneHoldQueue( const NTimer::STime _timeCurrTime ) 
{
	const NTimer::STime timeCurrTime = _timeCurrTime / 1000;
	const int nPrevPos = int( timeLastTime ) & ( N_HOLD_TIME - 1 );
	const int nCurrPos = int( timeCurrTime ) & ( N_HOLD_TIME - 1 );
	if ( nPrevPos == nCurrPos )
		return;
	if ( nPrevPos < nCurrPos )
	{
		for ( int i = nPrevPos + 1; i <= nCurrPos; ++i )
		{
			sceneHoldQueue[i].Clear();
			serializableSceneHoldQueue[i].Clear();
		}
	}
	else
	{
		for ( int i = nPrevPos + 1; i < N_HOLD_TIME; ++i )
		{
			sceneHoldQueue[i].Clear();
			serializableSceneHoldQueue[i].Clear();
		}
		for ( int i = 0; i <= nCurrPos; ++i )
		{
			sceneHoldQueue[i].Clear();
			serializableSceneHoldQueue[i].Clear();
		}
	}
	//
	timeLastTime = timeCurrTime;
}

void GetSceneHoldedObjects( list<CObjectBase*> *pObjects, bool bSerializeable )
{
	if ( bSerializeable )
	{
		for ( int i = 0; i < N_HOLD_TIME; ++i )
		{
			for ( CScenePool::SIterator it( &(serializableSceneHoldQueue[i]) ); it.p != 0; --it )
				pObjects->push_back( *it.p );
		}
	}
	else
	{
		for ( int i = 0; i < N_HOLD_TIME; ++i )
		{
			for ( CScenePool::SIterator it( &(sceneHoldQueue[i]) ); it.p != 0; --it )
				pObjects->push_back( *it.p );
		}
	}
}

struct SSceneHoldQueueSerializer
{
	vector< list< CObj<CObjectBase> > > data;
	//
	SSceneHoldQueueSerializer(): data( N_HOLD_TIME ) {}
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &data );
		return 0;
	}
};
//
void SerializeSceneHoldQueue( IBinSaver::chunk_id chunkID, IBinSaver &saver )
{
	SSceneHoldQueueSerializer hq;
	if ( saver.IsReading() )
	{
		ClearSceneHoldQueue();
		saver.Add( chunkID, &hq );
		for ( int i = 0; i < N_HOLD_TIME; ++i )
		{
			for ( list< CObj<CObjectBase> >::iterator it = hq.data[i].begin(); it != hq.data[i].end(); ++it )
				*serializableSceneHoldQueue[i].Alloc() = it->GetPtr();
		}
	}
	else
	{
		for ( int i = 0; i < N_HOLD_TIME; ++i )
		{
			for ( CScenePool::SIterator it( &(serializableSceneHoldQueue[i]) ); it.p != 0; --it )
				hq.data[i].push_back( *it.p );
		}
		saver.Add( chunkID, &hq );
	}
}


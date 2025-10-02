
#pragma once
#include "ListsSet.h"

extern NTimer::STime curTime;

namespace NSegmObjs
{
enum { SEGM_UNITS_SIZE = 10000 };
//
inline const int GetSegmUnitsIndexByTime( const NTimer::STime &time ) { return time % SEGM_UNITS_SIZE; }
inline const int GetRegisterIndex( const bool bInitialization )
{
	return bInitialization ?
				 GetSegmUnitsIndexByTime( curTime - curTime % SConsts::AI_SEGMENT_DURATION ) :
				 GetSegmUnitsIndexByTime( curTime + SConsts::AI_SEGMENT_DURATION - curTime % SConsts::AI_SEGMENT_DURATION );
}
//
template<class TPObj>
class CContainer
{
	public: int operator&( IBinSaver &saver ); private:;

	CListsSet<TPObj> container;
	hash_set<int> registerdObjects;

	typedef TPObj TObjType;
public:
	CContainer() : container( SEGM_UNITS_SIZE ) { }
	void Clear() { container.Clear(); registerdObjects.clear(); }

	void RegisterSegments( const TPObj pObj, const bool bInitialization );
	void UnregisterSegments( const TPObj pObj ) { registerdObjects.erase( pObj->GetUniqueId() ); }
	bool IsRegistered( const TPObj pObj ) { return registerdObjects.find( pObj->GetUniqueID() ) != registerdObjects.end(); }

	//
	const int begin( const int nListNum ) const { return container.begin( nListNum ); }
	const int end() const{ return container.end(); }
	const int GetNext( const int nIter ) { return container.GetNext( nIter ); }
	TPObj& GetEl( const int nIter ) { return container.GetEl( nIter ); }
	const TPObj& GetEl( const int nIter ) const { return container.GetEl( nIter ); }
	void Add( const int nIter, TPObj pObj ) { container.Add( nIter, pObj ); }
	void DelList( const int nList, const int nLastPos ) { container.DelList( nList, nLastPos ); }
};
//
template< class TContainer, class TSegments>
void Segment( const NTimer::STime lastSegmTime, const NTimer::STime roundedCurTime, TContainer &container, TSegments* );
template< class TContainer, class TSegments>
void SegmentWOMove( const NTimer::STime lastSegmTime, const NTimer::STime roundedCurTime, TContainer &container, TSegments* );

//
//	implementation
//

template<class TPObj>
void CContainer<TPObj>::RegisterSegments( const TPObj pObj, const bool bInitialization )
{
	const int nUniqueId = pObj->GetUniqueId();
	if ( registerdObjects.find( pObj->GetUniqueId() ) == registerdObjects.end() )
	{
		const int nIndex = GetRegisterIndex( bInitialization );

		container.Add( nIndex, pObj );
		registerdObjects.insert( nUniqueId );
	}
}

template <class TPObj>
inline int CContainer<TPObj>::operator&( IBinSaver &saver )
{
	

	saver.Add( 1, &container );
	saver.Add( 2, &registerdObjects );

	return 0;
}

template< class TContainer, class TSegments>
inline void Segment( const NTimer::STime lastSegmTime, const NTimer::STime roundedCurTime, TContainer &container, TSegments* )
{
	for ( NTimer::STime time = lastSegmTime; time <= roundedCurTime; time += SConsts::AI_SEGMENT_DURATION )
	{
		const int nIndex = GetSegmUnitsIndexByTime(time);

		int nPredValue = 0;			
		for ( int iter = container.begin( nIndex ); iter != container.end(); iter = container.GetNext( iter ) )
		{
			TSegments::TObjType pObj = container.GetEl( iter );

			static TSegments check;			
			check.SetSegmentObject( pObj );
			if ( check.Check() /*&& container.IsRegistered( pObj )*/ )
			{
				const NTimer::STime nextSegmTime = check.ProcessSegment();

				const int nNewIndex =
					( nextSegmTime <= roundedCurTime + SConsts::AI_SEGMENT_DURATION ) ?
					GetSegmUnitsIndexByTime( roundedCurTime + SConsts::AI_SEGMENT_DURATION ) :
					GetSegmUnitsIndexByTime( nextSegmTime - nextSegmTime % SConsts::AI_SEGMENT_DURATION );

				container.Add( nNewIndex, pObj );
			}
			/*
			else if ( check.ShouldBeUnregistered() )
				container.UnregisterSegments( pObj );
			*/

			nPredValue = iter;
		}

		container.DelList( nIndex, nPredValue );
	}
}

template< class TContainer, class TSegments>
inline void SegmentWOMove( const NTimer::STime lastSegmTime, const NTimer::STime roundedCurTime, TContainer &container, TSegments* )
{
	for ( NTimer::STime time = lastSegmTime; time <= roundedCurTime; time += SConsts::AI_SEGMENT_DURATION )
	{
		const int nIndex = GetSegmUnitsIndexByTime(time);

		for ( int iter = container.begin( nIndex ); iter != container.end(); iter = container.GetNext( iter ) )
		{
			static TSegments check;			
			check.SetSegmentObject( container.GetEl( iter ) );
			if ( check.Check() )
				check.ProcessSegment();
		}
	}
}

};



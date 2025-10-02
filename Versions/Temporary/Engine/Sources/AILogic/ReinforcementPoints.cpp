#include "stdafx.h"
/*
#include "ReinforcementPoints.h"

extern CGlobalStock theStock;
extern CEventUpdater updater;
extern CDiplomacy theDipl;
CReinforcementPointsTracker theReinfPoints;
extern NTimer::STime curTime;

void CPlayerPointsTracker::Init( int nPlayerIndex, const vector<SReinforcementPosition> &positions, int nGlobalRecycle )
{
	nPlayer = nPlayerIndex;
	recycles[-1] = nGlobalRecycle;
	factories[-1].nCounter = 1;
	for ( int i = 0; i < positions.size(); ++i )
	{
		allPositions[i + 1] = positions[i];
		if ( positions[i].bIsDefault )
			SetPositionAvailible( i + 1 );
	}
	nMaxPointIndex = positions.size();
}

int CPlayerPointsTracker::AddReinforcementPoint( const NDb::SReinforcementPosition &point )
{
	allPositions[nMaxPointIndex + 1] = point;
	return nMaxPointIndex++;
}

const SReinforcementPosition* CPlayerPointsTracker::GetPositionInfo( int nPositionID ) const
{
	CPositionMap::const_iterator it = allPositions.find( nPositionID );
	if ( it == allPositions.end() )
		return 0;
	return &(it->second);
}

void CPlayerPointsTracker::SetPositionAvailible( int nPositionID )
{
	if ( allPositions.find( nPositionID ) == allPositions.end() )
		return;
	const NDb::SReinforcementPosition &position = allPositions[nPositionID];
	bool bEmpty = factories[position.nFactoryID].availibility[position.eType].empty();
	factories[position.nFactoryID].availibility[position.eType].push_back( position );
	UpdateReinfPoint( position.nFactoryID, nPositionID );
	if ( bEmpty )
		theStock[nPlayer]->AddAvailReinforcementType( position.nFactoryID, nPositionID, position.eType );
}

void CPlayerPointsTracker::RemovePositionAvailibility( int nPositionID )
{
	if ( allPositions.find( nPositionID ) == allPositions.end() )
		return;	
	const NDb::SReinforcementPosition &position = allPositions[nPositionID];
	CPositionList &lst = factories[position.nFactoryID].availibility[position.eType];
	for ( CPositionList::iterator it = lst.begin(); it != lst.end(); ++it )
	{
		if ( it->vPosition == position.vPosition )
		{
			it = lst.erase( it );
			if ( lst.empty() )
			{
				theStock[nPlayer]->RemoveAvailReinforcementType( position.nFactoryID, nPositionID );
				break;
			}
		}
	}
}

bool CPlayerPointsTracker::IsPositionEnabled( int nPositionID ) const
{
	CPositionMap::const_iterator pos = allPositions.find( nPositionID );
	if (  pos == allPositions.end() )
		return false;	
	const NDb::SReinforcementPosition &position = pos->second;
	
	CFactoryMap::const_iterator posFactory = factories.find( position.nFactoryID );
	if ( posFactory == factories.end() )
		return false;

	CAvailibilityMap::const_iterator posAvalability = posFactory->second.availibility.find( position.eType );
	if ( posAvalability == posFactory->second.availibility.end() )
		return false;

	const CPositionList &lst = posAvalability->second;
	for ( CPositionList::const_iterator it = lst.begin(); it != lst.end(); ++it )
	{
		if ( it->vPosition == position.vPosition )
			return true;
	}
	return false;
}

void CPlayerPointsTracker::StartRecycle( int nFactoryID )
{
	if ( recycles.find( nFactoryID ) == recycles.end() )
		return;
	pendingRecycles[nFactoryID] = curTime;
	UpdatePositionByFactory( nFactoryID );
}

bool CPlayerPointsTracker::IsRecycling( const int nFactoryID ) const
{
	return pendingRecycles.find( nFactoryID ) != pendingRecycles.end();
}

void CPlayerPointsTracker::UpdatePositionByFactory( const int nFactoryID )
{
	for ( CPositionMap::const_iterator it = allPositions.begin(); it != allPositions.end(); ++it )
	{
		if ( it->second.nFactoryID == nFactoryID )
			UpdateReinfPoint( nFactoryID, it->first );
	}
}

NTimer::STime CPlayerPointsTracker::GetNextRecycleEnd()
{
	hash_map<int,int>::iterator it = recycles.begin();
	NTimer::STime timeMin = curTime + it->second;
	NTimer::STime time;

	for ( it = recycles.begin(); it != recycles.end(); ++it )
	{
		hash_map<int,int>::const_iterator itPending = pendingRecycles.find( it->first );
		if ( itPending == pendingRecycles.end() )			//Recycle ready
			return curTime;
		else
		{
			time = itPending->second + it->second;
			if ( time < timeMin )
				timeMin = time;
		}
	}

	return timeMin;
}

void CPlayerPointsTracker::Segment()
{
	for ( hash_map<int,int>::iterator it = pendingRecycles.begin(); it != pendingRecycles.end(); )
	{
		float fPercent = float( curTime - it->second ) / recycles[it->first];
		UpdatePositionByFactory( it->first );
		if ( fPercent >= 1.0f )
			pendingRecycles.erase( it++ );
		else
			++it;
	}
}

NTimer::STime CPlayerPointsTracker::GetRecycleTyme( const int nFactoryID ) const
{
	hash_map<int,int>::const_iterator pos = recycles.find( nFactoryID );
	if ( pos != recycles.end() )
		return pos->second;
	return 0;
}

void CPlayerPointsTracker::SetRecycleTime( const int nFactory, const int nRecycleTime )
{
	recycles[nFactory] = nRecycleTime;
	Segment(); // to update recycle time in client
}

void CPlayerPointsTracker::UpdateReinfPoint( int nFactoryID, int nPointID ) const
{
	if ( nPlayer != theDipl.GetMyNumber() )
		return;
	
	CFactoryMap::const_iterator itFactory = factories.find( nFactoryID );
	if ( itFactory == factories.end() )
		return;
	const SFactoryData &data = itFactory->second;
	CPtr<SAIReinfPointUpdate> pUpdate = new SAIReinfPointUpdate;
	pUpdate->nReinfPointID = nPointID;
	pUpdate->nFactoryID = nFactoryID;
	pUpdate->bActivate = ( data.nCounter > 0 );
	hash_map<int,int>::const_iterator itRecycle = pendingRecycles.find( nFactoryID );
	pUpdate->fRecycle = ( itRecycle == pendingRecycles.end() ? 1.0f : ( float( curTime - itRecycle->second ) / recycles.find( nFactoryID )->second ) );
	pUpdate->bEnable = pUpdate->fRecycle >= 1.0f;
	updater.AddUpdate( pUpdate, ACTION_NOTIFY_REINF_POINT, 0, -1 );		
	
}

int CPlayerPointsTracker::operator&( IBinSaver &saver )
{
	if ( !saver.IsChecksum() )
		saver.Add( 1, &nPlayer );
	saver.Add( 2, &allPositions );
	saver.Add( 3, &factories );
	saver.Add( 4, &pendingRecycles );
	saver.Add( 5, &recycles );
	saver.Add( 6, &nMaxPointIndex );
	
	return 0;
}

int CPlayerPointsTracker::SFactoryData::operator&( IBinSaver &saver )
{
	saver.Add( 1, &availibility );
	saver.Add( 2, &nCounter );
	return 0;
}

void CReinforcementPointsTracker::Init( const vector<SMapPlayerInfo> &info )
{
	trackers.resize( info.size() );
	for ( int i = 0; i < info.size(); ++i )
	{
		trackers[i].Init( i, info[i].reinforcementPoints, info[i].nRecycleTime );
	}
	theStock.ForcedUpdate();
}

CPlayerPointsTracker* CReinforcementPointsTracker::operator[]( const int &nIndex )
{
	NI_ASSERT( nIndex >= 0 && nIndex < trackers.size(), "Player index out of range" );
	return &(trackers[nIndex]);
}

void CReinforcementPointsTracker::Segment()
{
	for ( int i = 0; i < trackers.size(); ++i )
		trackers[i].Segment();	
}

int CReinforcementPointsTracker::operator&( IBinSaver &saver )
{
	saver.Add( 1, &trackers );

	return 0;
}*/


#ifndef __REINFORCEMENT_POINTS_H__
#define __REINFORCEMENT_POINTS_H__

/*
#pragma once

class CPlayerPointsTracker
{
	typedef list<SReinforcementPosition> CPositionList;
	typedef hash_map<NDb::EReinforcementType,CPositionList,NReinforcement::SReinforcementTypeHash> CAvailibilityMap;
	struct SFactoryData
	{
		CAvailibilityMap availibility;
		int nCounter;
		SFactoryData() : nCounter( 0 ) {}
		int operator&( IBinSaver &saver );
	};
	typedef hash_map<int,SFactoryData> CFactoryMap;
	typedef hash_map< int, SReinforcementPosition > CPositionMap;

	int nPlayer;
	CPositionMap allPositions;
	CFactoryMap factories;
	hash_map<int,int> pendingRecycles;
	hash_map<int,int> recycles;
	int nMaxPointIndex;											// current max reinforcement point index
	
	void UpdateReinfPoint( int nFactoryID, int nPositionID ) const;
	void UpdatePositionByFactory( const int nFactoryID );
public:
	CPlayerPointsTracker() : nPlayer( -1 ), nMaxPointIndex( 0 ) {}
	void Init( int nPlayerIndex, const vector<SReinforcementPosition> &positions, int nGlobalRecycle );
	int  AddReinforcementPoint( const NDb::SReinforcementPosition &point );
	const SReinforcementPosition* GetPositionInfo( int nPositionID ) const;
	bool IsPositionEnabled( int nPositionID ) const;
	int GetMaxPositionIndex() const { return nMaxPointIndex; }
	void SetPositionAvailible( int nPositionID );
	void RemovePositionAvailibility( int nPositionID );
	void StartRecycle( int nFactoryID );
	bool IsRecycling( const int nFactoryID ) const;
	NTimer::STime GetRecycleTyme( const int nFactoryID ) const;
	NTimer::STime GetNextRecycleEnd();
	
	// time in milliseconds
	void SetRecycleTime( const int nFactory, const int nRecycleTime );

	void Segment();
	int operator&( IBinSaver &saver );
};

class CReinforcementPointsTracker
{
	vector<CPlayerPointsTracker> trackers;
public:
	CReinforcementPointsTracker() {}
	void Init( const vector<SMapPlayerInfo> &info );
	void Clear() { trackers.resize( 0 ); }
	void Segment();
	int operator&( IBinSaver &saver );
	CPlayerPointsTracker* operator[]( const int &nIndex );
};*/

#endif // __REINFORCEMENT_POINTS_H__

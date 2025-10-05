#pragma once

namespace NDb
{
	struct SMapInfo;
	struct SUnitStatsModifier;
}

class CBalanceTest
{
	CDBPtr<NDb::SMapInfo> pBalanceMapInfo;
	NTimer::STime timeBalanceStart;
	bool bTest;
	std::vector<int> shoot;

	void PrintBalanceTestData();
	void CollectBalanceTestData( int nIteration );
	void AllignSizes( std::string *szTitle, std::string *szSide0, std::string *szSide1, int nAdd = 0 );
public:
	CBalanceTest() : bTest( false ), shoot( 3, false ) { }
	void Clear() { bTest = false; }
	bool IsActive() const { return bTest; }
	bool HasShoot( int nPlayer ) const
	{
		return shoot[nPlayer];
	}
	void SetShoot( int nPlayer )
	{
		shoot[nPlayer] = true;
	}
	void SegmentBalanceTest();
	void InitBalanceTest( const NDb::SMapInfo *pMapInfo );
	const NDb::SUnitStatsModifier * GetModifier( int nPlayer ) const;
	void UnitDead( class CAIUnit *pUnit );
};



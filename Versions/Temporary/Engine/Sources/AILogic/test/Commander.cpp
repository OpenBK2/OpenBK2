#include "../stdafx.h"

#include "../Artillery.h"
#include "../Commander.h"
#include "../GeneralIntendant.h"
#include "System/BinSaver.h"
#include "System/Streams.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <gtest/gtest.h>

namespace
{
class CTestCommander : public CCommander
{
	OBJECT_NOCOPY_METHODS( CTestCommander );

public:
	CommonUnits workers;

	void AddTask( IGeneralTask *pTask )
	{
		tasks.push_back( pTask );
	}

	void EnumWorkers( EForceType eType, IWorkerEnumerator *pEnumerator ) override
	{
		EnumWorkersInternal( eType, pEnumerator, &workers );
	}

	void Give( CCommonUnit *pUnit ) override
	{
		workers.push_back( pUnit );
	}
};

class CTestTask : public IGeneralTask
{
	OBJECT_NOCOPY_METHODS( CTestTask );

public:
	int nID = 0;
	bool bFinished = false;
	int nReleases = 0;
	std::vector<int> *pRequests = nullptr;

	ETaskName GetName() const override
	{
		return ETN_DEFEND_PATCH;
	}

	float GetSeverity() const override
	{
		return 1;
	}

	bool IsFinished() const override
	{
		return bFinished;
	}

	void AskForWorker( ICommander *, float, bool ) override
	{
		if ( pRequests )
		{
			pRequests->push_back( nID );
		}
	}

	void ReleaseWorker( ICommander *, float ) override
	{
		++nReleases;
	}

	void CancelTask( ICommander * ) override {}

	void Segment() override {}
};

class CWorkerRequest : public IWorkerEnumerator
{
public:
	int nBest = 0;
	int nLimit = 1;
	bool bOddOnly = false;
	std::vector<int> accepted;

	bool EnumWorker( CCommonUnit *pUnit, EForceType ) override
	{
		accepted.push_back( pUnit->GetUniqueId() );
		return accepted.size() < nLimit;
	}

	bool EvaluateWorker( CCommonUnit *pUnit, EForceType ) const override
	{
		return !bOddOnly || pUnit->GetUniqueId() % 2 != 0;
	}

	int NeedNBest( EForceType ) const override
	{
		return nBest;
	}

	float EvaluateWorkerRating( CCommonUnit *pUnit, EForceType ) const override
	{
		return -float( std::abs( pUnit->GetUniqueId() - 3 ) );
	}
};

CArtillery *MakeArtillery( int nID )
{
	auto *pUnit = dynamic_cast<CArtillery *>( NObjectFactory::MakeObject( 0x1108D4AB ) );
	EXPECT_NE( pUnit, nullptr );
	if ( pUnit )
	{
		pUnit->nUniqueID = nID;
	}
	return pUnit;
}

std::vector<int> WorkerIDs( const CommonUnits &workers )
{
	std::vector<int> ids;
	for ( const auto &pWorker : workers )
	{
		ids.push_back( pWorker->GetUniqueId() );
	}
	std::sort( ids.begin(), ids.end() );
	return ids;
}

TEST( Commander, FinishedTaskReturnsItsWorkersBeforeCompaction )
{
	CObj<CTestCommander> commander = new CTestCommander;
	CObj<CTestTask> finished = new CTestTask;
	CObj<CTestTask> active = new CTestTask;
	finished->bFinished = true;
	commander->AddTask( finished );
	commander->AddTask( active );
	commander->Segment();
	EXPECT_EQ( finished->nReleases, 1 );
	EXPECT_EQ( active->nReleases, 1 );
	commander->Segment();
	EXPECT_EQ( finished->nReleases, 1 );
	EXPECT_EQ( active->nReleases, 2 );
}

TEST( Commander, EqualSeverityKeepsSavedTaskOrder )
{
	CObj<CTestCommander> commander = new CTestCommander;
	std::vector<CObj<CTestTask>> tasks;
	std::vector<SBinSaverExternalObject> external;
	std::vector<int> requests;
	for ( int nID : { 1, 2, 3 } )
	{
		CObj<CTestTask> task = new CTestTask;
		task->nID = nID;
		task->pRequests = &requests;
		commander->AddTask( task );
		external.emplace_back( nID, task.GetPtr() );
		tasks.push_back( task );
	}
	commander->Segment();
	EXPECT_EQ( requests, ( std::vector<int>{ 3, 2, 1 } ) );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
		saver->Add( 1, static_cast<CCommander *>( commander.GetPtr() ) );
	}
	CObj<CTestCommander> restored = new CTestCommander;
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->Add( 1, static_cast<CCommander *>( restored.GetPtr() ) );
	}
	requests.clear();
	restored->Segment();
	EXPECT_EQ( requests, ( std::vector<int>{ 3, 2, 1 } ) );
}

TEST( Commander, UnrankedSelectionKeepsRejectedAndUnrequestedWorkers )
{
	CObj<CTestCommander> commander = new CTestCommander;
	std::vector<CObj<CArtillery>> units;
	for ( int nID : { 3, 1, 4, 2 } )
	{
		units.push_back( MakeArtillery( nID ) );
		ASSERT_NE( units.back(), nullptr );
		commander->Give( units.back() );
	}
	CWorkerRequest request;
	request.nLimit = 1;
	request.bOddOnly = true;
	commander->EnumWorkers( FT_MOBILE_TANKS, &request );
	EXPECT_EQ( request.accepted, ( std::vector<int>{ 1 } ) );
	EXPECT_EQ( WorkerIDs( commander->workers ), ( std::vector<int>{ 2, 3, 4 } ) );
	request.accepted.clear();
	commander->EnumWorkers( FT_MOBILE_TANKS, &request );
	EXPECT_EQ( request.accepted, ( std::vector<int>{ 3 } ) );
	EXPECT_EQ( WorkerIDs( commander->workers ), ( std::vector<int>{ 2, 4 } ) );
}

TEST( Commander, BestWorkersUseRatingThenIDAndStopWhenSatisfied )
{
	std::vector<CObj<CArtillery>> units;
	for ( int nID : { 1, 2, 3, 4, 5 } )
	{
		units.push_back( MakeArtillery( nID ) );
		ASSERT_NE( units.back(), nullptr );
	}
	for ( bool bReverse : { false, true } )
	{
		CObj<CTestCommander> commander = new CTestCommander;
		for ( int i = 0; i < units.size(); ++i )
		{
			commander->Give( units[bReverse ? units.size() - 1 - i : i] );
		}
		CWorkerRequest request;
		request.nBest = 2;
		request.nLimit = 2;
		commander->EnumWorkers( FT_MOBILE_TANKS, &request );
		EXPECT_EQ( request.accepted, ( std::vector<int>{ 3, 2 } ) );
		EXPECT_EQ( WorkerIDs( commander->workers ), ( std::vector<int>{ 1, 4, 5 } ) );
	}
}

TEST( Commander, SingleBestTieUsesStableUnitID )
{
	CObj<CTestCommander> commander = new CTestCommander;
	CObj<CArtillery> high = MakeArtillery( 4 );
	CObj<CArtillery> low = MakeArtillery( 2 );
	ASSERT_NE( high, nullptr );
	ASSERT_NE( low, nullptr );
	commander->Give( high );
	commander->Give( low );
	CWorkerRequest request;
	request.nBest = 1;
	commander->EnumWorkers( FT_MOBILE_TANKS, &request );
	EXPECT_EQ( request.accepted, ( std::vector<int>{ 2 } ) );
	EXPECT_EQ( WorkerIDs( commander->workers ), ( std::vector<int>{ 4 } ) );
}

TEST( Commander, FreeArtilleryCursorResumesTheSameCandidateAfterLoad )
{
	CFreeArtilleryHolder holder;
	std::vector<CObj<CArtillery>> units;
	std::vector<SBinSaverExternalObject> external;
	for ( int nID : { 7, 3, 9 } )
	{
		units.push_back( MakeArtillery( nID ) );
		ASSERT_NE( units.back(), nullptr );
		external.emplace_back( nID, units.back().GetPtr() );
		holder.AddArtillery( units.back() );
	}
	holder.Segment();
	ASSERT_EQ( holder.Get(), units[1].GetPtr() );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
		saver->Add( 1, &holder );
	}
	holder.Segment();
	ASSERT_EQ( holder.Get(), units[2].GetPtr() );
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->Add( 1, &holder );
	}
	EXPECT_EQ( holder.Get(), units[1].GetPtr() );
	holder.Segment();
	EXPECT_EQ( holder.Get(), units[2].GetPtr() );
}
} // namespace

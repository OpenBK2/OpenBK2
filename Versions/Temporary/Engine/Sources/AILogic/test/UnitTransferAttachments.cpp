#include "../stdafx.h"

#include "../Artillery.h"
#include "../Commands.h"
#include "../Formation.h"
#include "../Soldier.h"
#include "../StaticMembers.h"
#include "../Technics.h"
#include "../Trains.h"
#include "../UnitStates.h"
#include "../UnitTransfer.h"
#include "System/BinSaver.h"
#include "System/Streams.h"

#include <functional>
#include <list>
#include <vector>

#include <gtest/gtest.h>

namespace
{
struct SFixtureFields
{
	std::function<void( IBinSaver & )> write;

	int operator&( IBinSaver &saver )
	{
		write( saver );
		return 0;
	}
};

class UnitTransferAttachments : public testing::Test
{
	std::vector<CObj<CCommonUnit>> units;
	std::vector<SBinSaverExternalObject> external;

protected:
	void TearDown() override
	{
		CDecksSet<CObj<CAICommand>> emptyCommands( 64 );
		LoadCommandQueue( emptyCommands );
		CLinkObject::Clear();
		units.clear();
	}

	template <class T>
	T *MakeUnit( int nTypeID, int nID )
	{
		T *pUnit = dynamic_cast<T *>( NObjectFactory::MakeObject( nTypeID ) );
		EXPECT_NE( pUnit, nullptr );
		if ( pUnit )
		{
			pUnit->nUniqueID = nID;
			pUnit->Mem2UniqueIdObjs();
			units.push_back( pUnit );
			external.emplace_back( nID, pUnit );
		}
		return pUnit;
	}

	void LoadFields( CObjectBase *pUnit, const std::function<void( IBinSaver & )> &write )
	{
		// Populate the real save fields without running map initialization or replacing attachment lookup.
		// External objects preserve the factory instances and their registered IDs across the load.
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			SFixtureFields fields{ write };
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, pUnit );
	}

	void SetAlive( CCommonUnit *pUnit, bool bAlive )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			bool bAliveSet = true;
			saver->Add( 2, &bAlive );
			saver->Add( 3, &bAliveSet );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		pUnit->CUpdatableObj::operator&( *loader );
	}

	void LoadCommandQueue( CDecksSet<CObj<CAICommand>> &commands )
	{
		// The queue is global save data, separate from each unit's current command and state.
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			SFixtureFields fields{ [&]( IBinSaver &saver ) { saver.Add( 1, &commands ); } };
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		CStaticMembers members;
		loader->Add( 1, &members );
	}

	void LinkCar( CTrainCar *pCar, CTrainLocomotive *pLocomotive, CTrainCar *pFront, CTrainCar *pBack )
	{
		CPtr<CTrainLocomotive> locomotive = pLocomotive;
		CPtr<CTrainCar> front = pFront;
		CPtr<CTrainCar> back = pBack;
		LoadFields( pCar,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 2, &locomotive );
									saver.Add( 3, &front );
									saver.Add( 4, &back );
								} );
	}

	void SetCars( CTrainLocomotive *pLocomotive, std::list<CPtr<CAIUnit>> cars )
	{
		LoadFields( pLocomotive, [&]( IBinSaver &saver ) { saver.Add( 2, &cars ); } );
	}

	void SetPassenger( CMilitaryCar *pCar, CSoldier *pSoldier )
	{
		std::list<CPtr<CSoldier>> passengers{ pSoldier };
		SFixtureFields militaryCar{ [&]( IBinSaver &saver ) { saver.Add( 5, &passengers ); } };
		LoadFields( pCar, [&]( IBinSaver &saver ) { saver.Add( 1, &militaryCar ); } );
		pSoldier->SetRestInside( true, pCar );
	}
};

TEST_F( UnitTransferAttachments, SelectingAnyTrainPartIncludesAllCarsAndPassengersInIDOrder )
{
	CTrainLocomotive *pLocomotive = MakeUnit<CTrainLocomotive>( 0x19171400, 30 );
	CTrainCar *pFront = MakeUnit<CTrainCar>( 0x191992C1, 40 );
	CTrainCar *pMiddle = MakeUnit<CTrainCar>( 0x191992C1, 20 );
	CTrainCar *pRear = MakeUnit<CTrainCar>( 0x191992C1, 10 );
	CSoldier *pPassenger = MakeUnit<CSoldier>( 0x1108D442, 50 );
	ASSERT_NE( pLocomotive, nullptr );
	ASSERT_NE( pFront, nullptr );
	ASSERT_NE( pMiddle, nullptr );
	ASSERT_NE( pRear, nullptr );
	ASSERT_NE( pPassenger, nullptr );
	SetCars( pLocomotive, { pFront, pMiddle, pRear } );
	LinkCar( pFront, pLocomotive, nullptr, pMiddle );
	LinkCar( pMiddle, pLocomotive, pFront, pRear );
	LinkCar( pRear, pLocomotive, pMiddle, nullptr );
	SetPassenger( pRear, pPassenger );

	const std::vector<int> expected{ 10, 20, 30, 40, 50 };
	for ( int nSelected : { 10, 20, 30, 40 } )
	{
		EXPECT_EQ( CollectTransferableUnits( { nSelected }, 0 ), expected );
	}
	EXPECT_EQ( CollectTransferableUnits( { 40, 10, 30, 20, 40 }, 0 ), expected );
	EXPECT_EQ( CollectTransferableUnits( { 20, 30, 10, 40 }, 0 ), expected );
	for ( bool bLocallySelectable : { false, true } )
	{
		pFront->SetSelectable( bLocallySelectable, false );
		EXPECT_EQ( CollectTransferableUnits( { 40 }, 0 ), expected );
	}
	pPassenger->SetPlayer( 1 );
	EXPECT_TRUE( CollectTransferableUnits( { 30 }, 0 ).empty() );
	pPassenger->SetPlayer( 0 );
	EXPECT_EQ( CollectTransferableUnits( { 30 }, 0 ), expected );

	// Destroyed cars retain their links. They are traversed but never donated, including in a detached consist.
	SetAlive( pMiddle, false );
	EXPECT_EQ( CollectTransferableUnits( { 40 }, 0 ), ( std::vector<int>{ 10, 30, 40, 50 } ) );
	LinkCar( pFront, nullptr, nullptr, pMiddle );
	LinkCar( pMiddle, nullptr, pFront, pRear );
	LinkCar( pRear, nullptr, pMiddle, nullptr );
	EXPECT_EQ( CollectTransferableUnits( { 40 }, 0 ), ( std::vector<int>{ 10, 40, 50 } ) );
}

TEST_F( UnitTransferAttachments, ForeignOwnedTrainPartRejectsTheWholeTrainButKeepsIndependentSelection )
{
	CTrainLocomotive *pLocomotive = MakeUnit<CTrainLocomotive>( 0x19171400, 30 );
	CTrainCar *pCar = MakeUnit<CTrainCar>( 0x191992C1, 10 );
	CAITransportUnit *pIndependent = MakeUnit<CAITransportUnit>( 0x1108D445, 20 );
	ASSERT_NE( pLocomotive, nullptr );
	ASSERT_NE( pCar, nullptr );
	ASSERT_NE( pIndependent, nullptr );
	SetCars( pLocomotive, { pCar } );
	LinkCar( pCar, pLocomotive, nullptr, nullptr );
	pCar->SetPlayer( 1 );
	EXPECT_TRUE( CollectTransferableUnits( { 30 }, 0 ).empty() );
	EXPECT_TRUE( CollectTransferableUnits( { 10 }, 1 ).empty() );
	EXPECT_EQ( CollectTransferableUnits( { 30, 20, 10 }, 0 ), ( std::vector<int>{ 20 } ) );
}

TEST_F( UnitTransferAttachments, GunThatStartsHookingAfterSelectionIsRejectedWhenTheCommandExecutes )
{
	CArtillery *pGun = MakeUnit<CArtillery>( 0x1108D4AB, 10 );
	CAITransportUnit *pTruck = MakeUnit<CAITransportUnit>( 0x1108D445, 20 );
	CAITransportUnit *pIndependent = MakeUnit<CAITransportUnit>( 0x1108D445, 30 );
	ASSERT_NE( pGun, nullptr );
	ASSERT_NE( pTruck, nullptr );
	ASSERT_NE( pIndependent, nullptr );
	EXPECT_EQ( CollectTransferableUnits( { 10, 30 }, 0 ), ( std::vector<int>{ 10, 30 } ) );

	// Preserve the submitted IDs while the gun enters the synchronized hooking state.
	CPtr<CAIUnit> hookingTransport = pTruck;
	LoadFields( pGun, [&]( IBinSaver &saver ) { saver.Add( 19, &hookingTransport ); } );
	EXPECT_TRUE( CollectTransferableUnits( { 10 }, 0 ).empty() );
	EXPECT_EQ( CollectTransferableUnits( { 10, 30 }, 0 ), ( std::vector<int>{ 30 } ) );
}

TEST_F( UnitTransferAttachments, TowedGunCannotBeARootButStillFollowsASelectedTruckWithItsCrew )
{
	CArtillery *pGun = MakeUnit<CArtillery>( 0x1108D4AB, 10 );
	CAITransportUnit *pTruck = MakeUnit<CAITransportUnit>( 0x1108D445, 20 );
	CFormation *pCrew = MakeUnit<CFormation>( 0x1108D498, 30 );
	CAITransportUnit *pIndependent = MakeUnit<CAITransportUnit>( 0x1108D445, 40 );
	ASSERT_NE( pGun, nullptr );
	ASSERT_NE( pTruck, nullptr );
	ASSERT_NE( pCrew, nullptr );
	ASSERT_NE( pIndependent, nullptr );
	CObj<IUnitState> towedState = dynamic_cast<IUnitState *>( NObjectFactory::MakeObject( 0x1108D4A5 ) );
	CObj<IUnitState> crewState = dynamic_cast<IUnitState *>( NObjectFactory::MakeObject( 0x1108D4BF ) );
	ASSERT_NE( towedState.GetPtr(), nullptr );
	ASSERT_NE( crewState.GetPtr(), nullptr );
	pGun->SetCurState( towedState );
	pCrew->SetCurState( crewState );
	CPtr<CArtillery> gun = pGun;
	CPtr<CFormation> crew = pCrew;
	LoadFields( pTruck,
	            [&]( IBinSaver &saver )
	            {
								saver.Add( 3, &gun );
								saver.Add( 6, &crew );
							} );
	EXPECT_TRUE( CollectTransferableUnits( { 10 }, 0 ).empty() );
	EXPECT_TRUE( CollectTransferableUnits( { 30 }, 0 ).empty() );
	EXPECT_EQ( CollectTransferableUnits( { 10, 40 }, 0 ), ( std::vector<int>{ 40 } ) );
	// Towing removes the crew from the gun and stores it on the truck separately. Both still travel together.
	// Selection restrictions apply only to submitted roots, never to the carrier's attached units.
	EXPECT_EQ( CollectTransferableUnits( { 20 }, 0 ), ( std::vector<int>{ 10, 20, 30 } ) );
	EXPECT_EQ( CollectTransferableUnits( { 10, 20, 30 }, 0 ), ( std::vector<int>{ 10, 20, 30 } ) );

	// The crew stays attached to the truck while walking to it, before any soldier has boarded.
	CObj<IUnitState> boardingState = dynamic_cast<IUnitState *>( NObjectFactory::MakeObject( 0x1508D485 ) );
	ASSERT_NE( boardingState.GetPtr(), nullptr );
	pCrew->SetCurState( boardingState );
	EXPECT_TRUE( CollectTransferableUnits( { 30 }, 0 ).empty() );
	EXPECT_EQ( CollectTransferableUnits( { 20 }, 0 ), ( std::vector<int>{ 10, 20, 30 } ) );
}

TEST_F( UnitTransferAttachments, QueuedTowingCommandRejectsTheGunBeforeItsTowedStateStarts )
{
	CArtillery *pGun = MakeUnit<CArtillery>( 0x1108D4AB, 10 );
	CAITransportUnit *pIndependent = MakeUnit<CAITransportUnit>( 0x1108D445, 20 );
	ASSERT_NE( pGun, nullptr );
	ASSERT_NE( pIndependent, nullptr );
	ASSERT_EQ( pGun->GetState(), nullptr );
	EXPECT_EQ( CollectTransferableUnits( { 10, 20 }, 0 ), ( std::vector<int>{ 10, 20 } ) );

	CObj<CAICommand> command = dynamic_cast<CAICommand *>( NObjectFactory::MakeObject( 0x1108D44D ) );
	ASSERT_NE( command.GetPtr(), nullptr );
	command->ToUnitCmd().nCmdType = ACTION_MOVE_BEING_TOWED;
	command->ToUnitCmd().bFromAI = true;
	CDecksSet<CObj<CAICommand>> commands( 64 );
	commands.Push( 10, command );
	LoadCommandQueue( commands );
	// Hooking has cleared its pointer, but the towing command has not yet installed EUSN_BEING_TOWED.
	EXPECT_EQ( pGun->GetState(), nullptr );
	EXPECT_TRUE( CollectTransferableUnits( { 10 }, 0 ).empty() );
	EXPECT_EQ( CollectTransferableUnits( { 10, 20 }, 0 ), ( std::vector<int>{ 20 } ) );
}

TEST_F( UnitTransferAttachments, QueuedCrewBoardingCannotSplitTheCrewFromItsTruck )
{
	CFormation *pCrew = MakeUnit<CFormation>( 0x1108D498, 10 );
	CAITransportUnit *pTruck = MakeUnit<CAITransportUnit>( 0x1108D445, 20 );
	ASSERT_NE( pCrew, nullptr );
	ASSERT_NE( pTruck, nullptr );
	CPtr<CFormation> crew = pCrew;
	LoadFields( pTruck, [&]( IBinSaver &saver ) { saver.Add( 6, &crew ); } );
	CObj<CAICommand> command = dynamic_cast<CAICommand *>( NObjectFactory::MakeObject( 0x1108D44D ) );
	ASSERT_NE( command.GetPtr(), nullptr );
	command->ToUnitCmd().nCmdType = ACTION_MOVE_ENTER_TRANSPORT_CHEAT_PATH;
	command->ToUnitCmd().bFromAI = true;
	CDecksSet<CObj<CAICommand>> commands( 64 );
	commands.Push( 10, command );
	LoadCommandQueue( commands );
	EXPECT_TRUE( CollectTransferableUnits( { 10 }, 0 ).empty() );
	EXPECT_EQ( CollectTransferableUnits( { 20 }, 0 ), ( std::vector<int>{ 10, 20 } ) );
}
} // namespace

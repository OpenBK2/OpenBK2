#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../B2AI.h"
#include "../Commands.h"
#include "../Formation.h"
#include "../FormationStates.h"
#include "../GroupLogic.h"
#include "../Soldier.h"
#include "../StatesFactory.h"
#include "../StaticMembers.h"
#include "../Technics.h"
#include "../UnitStates.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
struct SFields
{
	std::function<void( IBinSaver & )> visit;

	int operator&( IBinSaver &saver )
	{
		visit( saver );
		return 0;
	}
};

class DugInSurvivor : public testing::TestWithParam<EActionCommand>
{
	static std::filesystem::path directory;
	std::vector<CObj<CObjectBase>> objects;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<SBinSaverExternalObject> external;

protected:
	CPtr<CFormation> formation;
	CPtr<CSoldier> survivor;
	CPtr<CAICommand> command;

	static void SetUpTestSuite()
	{
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-dug-in-survivor-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( DUG_IN_SURVIVOR_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
	}

	static void TearDownTestSuite()
	{
		NDb::CloseDatabase();
		NVFS::SetMainVFS( nullptr );
		NVFS::SetMainFileCreator( nullptr );
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}

	template <class T>
	T *MakeObject( int type )
	{
		auto *object = dynamic_cast<T *>( NObjectFactory::MakeObject( type ) );
		if ( !object )
		{
			throw std::runtime_error( "Engine object factory failed" );
		}
		objects.push_back( object );
		external.emplace_back( int( external.size() + 1 ), object );
		return object;
	}

	template <class T>
	T *MakeRecord( const char *type )
	{
		static unsigned nextRecord = 0;
		const std::string name = std::string( type ) + std::to_string( nextRecord++ ) + ".xdb";
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( type );
		if ( !record || !NDb::AddNewObject( name, CDBID( name ), record ) )
		{
			throw std::runtime_error( "Database record factory failed" );
		}
		records.push_back( record );
		return dynamic_cast<T *>( record->GetObject() );
	}

	void LoadFields( CObjectBase *object, const std::function<void( IBinSaver & )> &visit )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			SFields fields{ visit };
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, object );
	}

	void SetQueue( bool includeCommand )
	{
		CDecksSet<CObj<CAICommand>> queues( 128 );
		if ( includeCommand )
		{
			queues.Push( formation->GetUniqueId(), command.GetPtr() );
		}
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 1, &queues ); } };
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		CStaticMembers members;
		loader->Add( 1, &members );
	}

	void Initialize( EActionCommand action, bool canDigIn, bool dugIn = true )
	{
		theGroupLogic.Init( nullptr );
		NSingleton::RegisterSingleton( MakeObject<IAILogic>( 0x1108D441 ), IAILogic::tidTypeID );
		formation = MakeObject<CFormation>( 0x1108D498 );
		survivor = MakeObject<CSoldier>( AI_INFANTRY );
		command = MakeObject<CAICommand>( 0x1108D44D );

		// Recreate the saved state after casualties leave a dug-in officer without the dig-in command.
		CArray1Bit availability( 128 );
		availability.FillZero();
		auto *actions = MakeRecord<NDb::SUnitActions>( "UnitActions" );
		if ( action < 1000 )
		{
			availability.SetData( action );
			actions->availCommands.SetData( action );
		}
		if ( canDigIn )
		{
			availability.SetData( ACTION_COMMAND_ENTRENCH_SELF );
			actions->availCommands.SetData( ACTION_COMMAND_ENTRENCH_SELF );
		}
		auto *stats = MakeRecord<NDb::SInfantryRPGStats>( "InfantryRPGStats" );
		stats->etype = NDb::RPG_TYPE_OFFICER;
		stats->pActions = actions;
		CDBPtr<NDb::SInfantryRPGStats> statsPointer = stats;
		bool finished = true;
		bool alive = true;
		float hitPoints = 15;
		SFields updatable{ [&]( IBinSaver &saver )
		                   {
												 saver.Add( 2, &alive );
												 saver.Add( 3, &alive );
											 } };
		SFields link{ [&]( IBinSaver &saver ) { saver.Add( 1, &updatable ); } };
		SFields soldierCommon{ [&]( IBinSaver &saver ) { saver.Add( 1, &link ); } };
		SFields aiUnit{ [&]( IBinSaver &saver )
		                {
											saver.Add( 1, &soldierCommon );
											saver.Add( 15, &hitPoints );
											saver.Add( 74, &dugIn );
										} };
		SFields soldier{ [&]( IBinSaver &saver )
		                 {
											 saver.Add( 1, &aiUnit );
											 saver.Add( 4, &statsPointer );
											 saver.Add( 18, &formation );
										 } };
		LoadFields( survivor, [&]( IBinSaver &saver ) { saver.Add( 1, &soldier ); } );
		SFields queue{ [&]( IBinSaver &saver ) { saver.Add( 3, &finished ); } };
		SFields common{ [&]( IBinSaver &saver )
		                {
											saver.Add( 1, &link );
											saver.Add( 3, &queue );
										} };
		std::vector<CPtr<CSoldier>> survivors{ survivor };
		LoadFields( formation,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 1, &common );
									saver.Add( 4, &availability );
									saver.Add( 24, &survivors );
								} );
		command->ToUnitCmd() = SAIUnitCmd( action, CVec2( 1000, 0 ) );
		command->SetFromAI( false );
		// Missing primitive save fields load as zero, so assign distinct queue IDs after loading.
		formation->nUniqueID = 1;
		survivor->nUniqueID = 2;
		SetQueue( true );
		ASSERT_EQ( survivor->IsInTankPit(), dugIn );
		ASSERT_EQ( formation->Size(), 1 );
		ASSERT_EQ( survivor->GetStats()->HasCommand( ACTION_COMMAND_ENTRENCH_SELF ), canDigIn );
		ASSERT_TRUE( formation->CanCommandBeExecuted( command ) );
	}

	void TearDown() override
	{
		SetQueue( false );
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		objects.clear();
	}
};

std::filesystem::path DugInSurvivor::directory;

TEST_P( DugInSurvivor, MovementStartsLeavingCoverWithoutPublicAbility )
{
	Initialize( GetParam(), false );
	// Before the fix this call spins forever recreating the same movement order.
	formation->Segment();
	ASSERT_NE( formation->GetState(), nullptr );
	EXPECT_NE( dynamic_cast<CFormationLeaveSelfEntrenchState *>( formation->GetState() ), nullptr );
	ASSERT_NE( formation->GetCurCmd(), nullptr );
	EXPECT_EQ( formation->GetCurCmd()->ToUnitCmd().nCmdType, ACTION_MOVE_LEAVE_SELF_ENTRENCH );
	ASSERT_NE( formation->GetNextCommand(), nullptr );
	EXPECT_EQ( formation->GetNextCommand()->ToUnitCmd().nCmdType, GetParam() );
}

TEST_P( DugInSurvivor, MovementStillLeavesCoverWhenPublicAbilityExists )
{
	Initialize( GetParam(), true );
	formation->Segment();
	ASSERT_NE( formation->GetState(), nullptr );
	EXPECT_NE( dynamic_cast<CFormationLeaveSelfEntrenchState *>( formation->GetState() ), nullptr );
	ASSERT_NE( formation->GetNextCommand(), nullptr );
	EXPECT_EQ( formation->GetNextCommand()->ToUnitCmd().nCmdType, GetParam() );
}

INSTANTIATE_TEST_SUITE_P( Movement,
                          DugInSurvivor,
                          testing::Values( ACTION_COMMAND_MOVE_TO,
                                           ACTION_COMMAND_ROTATE_TO,
                                           ACTION_COMMAND_ROTATE_TO_DIR,
                                           ACTION_COMMAND_SWARM_TO ) );
} // namespace

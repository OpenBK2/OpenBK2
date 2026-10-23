#include "../stdafx.h"

#include "../Commands.h"
#include "../QueueUnit.h"
#include "../StaticMembers.h"
#include "../UnitStates.h"
#include "System/BinSaver.h"
#include "System/Streams.h"

#include <vector>

#include <gtest/gtest.h>

namespace
{
// Only command acceptance is supplied; queue storage and transfer use CQueueUnit itself.
class CAcceptingQueueUnit : public CQueueUnit
{
	int id;

protected:
	const int GetUniqueIdQU() const override
	{
		return id;
	}

public:
	explicit CAcceptingQueueUnit( int id )
		: id( id )
	{
	}

	IStatesFactory *GetStatesFactory() const override
	{
		return nullptr;
	}

	bool CanCommandBeExecuted( CAICommand * ) override
	{
		return true;
	}

	bool CanCommandBeExecutedByStats( CAICommand * ) override
	{
		return true;
	}

	void NotifyAbilityRun( CAICommand * ) override {}

	void SendAcknowledgement( EUnitAckType, bool ) override {}

	void SendAcknowledgement( CAICommand *, EUnitAckType, bool ) override {}

	void FreezeByState( bool ) override {}
};

struct SCommandQueues
{
	CDecksSet<CObj<CAICommand>> queues{ 131080 };

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &queues );
		return 0;
	}
};

void LoadQueues( SCommandQueues &queues, const std::vector<SBinSaverExternalObject> &external )
{
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
		saver->Add( 1, &queues );
	}
	stream.Seek( 0 );
	CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
	CStaticMembers members;
	loader->Add( 1, &members );
}

TEST( QueueUnitIDs, TransferPreservesFullIDsAndUnrelatedLowQueues )
{
	CObj<CAICommand> sourceCommand = dynamic_cast<CAICommand *>( NObjectFactory::MakeObject( 0x1108D44D ) );
	CObj<CAICommand> lowCommand = dynamic_cast<CAICommand *>( NObjectFactory::MakeObject( 0x1108D44D ) );
	CObj<CAICommand> replacedCommand = dynamic_cast<CAICommand *>( NObjectFactory::MakeObject( 0x1108D44D ) );
	ASSERT_NE( sourceCommand, nullptr );
	ASSERT_NE( lowCommand, nullptr );
	ASSERT_NE( replacedCommand, nullptr );
	const std::vector<SBinSaverExternalObject> external{
		{ 1, sourceCommand }, { 2, lowCommand }, { 3, replacedCommand } };
	SCommandQueues queues;
	queues.queues.Push( 65537, sourceCommand );
	queues.queues.Push( 131074, replacedCommand );
	queues.queues.Push( 1, lowCommand );
	queues.queues.Push( 2, lowCommand );
	LoadQueues( queues, external );

	CAcceptingQueueUnit source( 65537 ), destination( 131074 ), lowSource( 1 ), lowDestination( 2 );
	destination.InitWCommands( &source );
	EXPECT_EQ( source.GetNextCommand(), nullptr );
	EXPECT_EQ( destination.GetNextCommand(), sourceCommand.GetPtr() );
	EXPECT_EQ( lowSource.GetNextCommand(), lowCommand.GetPtr() );
	EXPECT_EQ( lowDestination.GetNextCommand(), lowCommand.GetPtr() );

	SCommandQueues empty;
	LoadQueues( empty, external );
}
} // namespace

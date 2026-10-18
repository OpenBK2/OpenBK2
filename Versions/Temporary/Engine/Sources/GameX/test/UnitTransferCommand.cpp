#include "../stdafx.h"

#include "../AICmdsAutoMagic.h"
#include "../AILogicCommandInternal.h"
#include "../MPTransceiver.h"
#include "Stats_B2_M1/FeedBackUpdates.h"

#include <cstring>

#include <gtest/gtest.h>

TEST( UnitTransferCommand, MultiplayerTablePreservesOldIDsAndRejectsUnknownCommands )
{
	CObj<IAICmdsAutoMagic> registry = CreateAICmdsAutoMagic();
	const int existingTypes[] = { 0x300A73C0, 0x300A73C1, 0x300A73C2, 0x300A73C3 };
	for ( int i = 0; i < 4; ++i )
	{
		CObj<CObjectBase> command = registry->MakeCommand( i );
		// Evaluate the accessor before typeid to avoid Clang's potentially-evaluated-expression diagnostic.
		const CObjectBase *pCommand = command.GetPtr();
		ASSERT_NE( pCommand, nullptr );
		EXPECT_EQ( NObjectFactory::GetObjectTypeID( typeid( *pCommand ) ), existingTypes[i] );
		EXPECT_EQ( registry->GetCommandID( command ), i );
	}
	CObj<SFeedBackUnitsArray> notification = new SFeedBackUnitsArray;
	EXPECT_EQ( registry->GetCommandID( notification ), -1 );
	EXPECT_EQ( registry->GetCommandID( nullptr ), -1 );
	EXPECT_EQ( registry->MakeCommand( -1 ), nullptr );
	EXPECT_EQ( registry->MakeCommand( 256 ), nullptr );
}

TEST( UnitTransferCommand, MultiplayerWireFormatRecreatesTransferOnEachRemoteClient )
{
	// MP uses a separate compact type table and an untyped payload, unlike replay serialization.
	// Exercise both packet serialization and the actual receiver factory to catch missing registrations.
	CObj<IAICmdsAutoMagic> sender = CreateAICmdsAutoMagic();
	CObj<IAILogicCommandB2> original = new CTransferUnitsCommand( { 47, 12, 91 }, 0, 2 );
	CMPTransceiver::SRawCommand outgoing;
	outgoing.nTypeID = sender->GetCommandID( original );
	{
		CObj<IBinSaver> saver = CreateBinSaver( &outgoing.cmd, SAVER_MODE_WRITE );
		saver->AddPolymorphicBase( 1, original );
	}
	CMemoryStream packet;
	{
		CObj<IBinSaver> saver = CreateBinSaver( &packet, SAVER_MODE_WRITE );
		saver->Add( 1, &outgoing );
	}
	for ( int nRemoteClient = 0; nRemoteClient < 2; ++nRemoteClient )
	{
		CObj<IAICmdsAutoMagic> receiver = CreateAICmdsAutoMagic();
		CMPTransceiver::SRawCommand incoming;
		packet.Seek( 0 );
		{
			CObj<IBinSaver> saver = CreateBinSaver( &packet, SAVER_MODE_READ );
			saver->Add( 1, &incoming );
		}
		CObj<CObjectBase> received = receiver->MakeCommand( incoming.nTypeID );
		CTransferUnitsCommand *pTransfer = dynamic_cast<CTransferUnitsCommand *>( received.GetPtr() );
		ASSERT_NE( pTransfer, nullptr ) << "Remote client decoded a different command";
		incoming.cmd.Seek( 0 );
		{
			CObj<IBinSaver> saver = CreateBinSaver( &incoming.cmd, SAVER_MODE_READ );
			saver->AddPolymorphicBase( 1, pTransfer );
		}
		CMemoryStream rewritten;
		{
			CObj<IBinSaver> saver = CreateBinSaver( &rewritten, SAVER_MODE_WRITE );
			saver->AddPolymorphicBase( 1, pTransfer );
		}
		ASSERT_EQ( rewritten.GetSize(), outgoing.cmd.GetSize() );
		EXPECT_EQ( std::memcmp( rewritten.GetBuffer(), outgoing.cmd.GetBuffer(), rewritten.GetSize() ), 0 );
	}
}

TEST( UnitTransferCommand, PolymorphicCommandRoundTripsThroughReplaySerialization )
{
	CObj<IAILogicCommandB2> original = new CTransferUnitsCommand( { 47, 12, 91 }, 0, 2 );
	ASSERT_TRUE( original->NeedToBeStored() );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &original );
	}
	stream.Seek( 0 );
	CObj<IAILogicCommandB2> loaded;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, &loaded );
	}
	ASSERT_NE( dynamic_cast<CTransferUnitsCommand *>( loaded.GetPtr() ), nullptr );
	ASSERT_TRUE( loaded->NeedToBeStored() );
	CMemoryStream rewritten;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &rewritten, SAVER_MODE_WRITE );
		saver->Add( 1, &loaded );
	}
	ASSERT_EQ( rewritten.GetSize(), stream.GetSize() );
	EXPECT_EQ( std::memcmp( rewritten.GetBuffer(), stream.GetBuffer(), stream.GetSize() ), 0 );
}
